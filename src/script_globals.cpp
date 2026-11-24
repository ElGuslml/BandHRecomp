// ghbandhero - the game's script globals changed in guest memory (see script_globals.h).
//
// The script interpreter keeps every global in a table: the pointer at 0x82D78F44 (sub_823AAE40 reads it: lis
// r11,-32040; lwz r8,-31580(r11)) to 32768 buckets, indexed by the key's low 15 bits, of nodes (+2 type, +4 key, +12
// value, +16 next; type 26 names another global). A struct (type 10) is a 4-byte header (00000100) and its first
// component's pointer at +4; a component is +2 type, +4 name, +8 value, +12 next. An array (type 12) is +2 the
// elements' type, +4 their count, +8 the value when there is one, else a pointer to them. Integers are type 1, floats
// type 2; a vector (types 5 and 6, two or three floats) is a pointer to a 4-byte header and its floats. GH Warriors of
// Rock's layout, read in that game with a probe (its local_docs/47-mods-plan.md: the dumps of highway_guitar1,
// highway_pos_table, highway_scale_chart and difficulty_list_props), and the qb files' own with the offsets made
// pointers. This game's code is the same: its resolve function sub_823AAE40 is GH Warriors of Rock's sub_8239B040
// instruction for instruction (103/103: the bucket index, the alias loop, the types), sub_823A6890 reads a struct's
// first component at +4 and a component's type at +2, name at +4 and value at +8, and enable_crowd_models_cfunc
// (sub_82231870) reads an array's count at +4 and its value or pointer at +8; the disc's qb data decodes with the same
// layout in both games.
//
// A global's data written in place reaches the scripts the interpreter runs and the ones it runs compiled
// (scripts\qtocpp_scriptlist.qb's scriptstocompile, 62 here) alike: update_highway_scale, init_morph_targets and
// highway_scale_manager are compiled, here as in GH Warriors of Rock. The game loads every global at the start and keeps
// it; the changes are written at the first "$" read of any changed global, before that read, so nothing has read the
// console's value by then. In GH Warriors of Rock that first read was highway_guitar1's, at the start, by compiled code;
// this game's first reader was not looked for (the log's "[script globals]" lines show when the changes land).

#include "script_globals.h"

#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ghbandhero::script {
namespace {

constexpr uint32_t kGlobalTable = 0x82D78F44;
constexpr uint8_t kTypeInteger = 1;
constexpr uint8_t kTypeFloat = 2;
constexpr uint8_t kTypeVector2 = 5;
constexpr uint8_t kTypeVector3 = 6;
constexpr uint8_t kTypeStruct = 10;
constexpr uint8_t kTypeArray = 12;
constexpr uint8_t kTypeAlias = 26;

uint8_t* Base() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

// Whether the host has the guest's bytes there (a pointer followed from the game's data, checked before it is read).
bool Readable(uint32_t address, uint32_t size) {
  uint8_t* base = Base();
  if (!base || address < 0x10000 || address + size < address) {
    return false;
  }
#ifdef _WIN32
  uint8_t* p = base + address;
  uint8_t* end = p + size;
  while (p < end) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(p, &info, sizeof(info)) || info.State != MEM_COMMIT || !info.Protect ||
        (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
      return false;
    }
    p = static_cast<uint8_t*>(info.BaseAddress) + info.RegionSize;
  }
#endif
  return true;
}

uint32_t Load32(uint32_t address) {
  uint8_t* base = Base();
  if (!base || !Readable(address, 4)) {
    return 0;
  }
  uint32_t v;
  std::memcpy(&v, base + address, 4);
  return __builtin_bswap32(v);
}

void Store32(uint32_t address, uint32_t value) {
  uint8_t* base = Base();
  if (!base || !Readable(address, 4)) {
    return;
  }
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

uint8_t Load8(uint32_t address) {
  uint8_t* base = Base();
  return base && Readable(address, 1) ? base[address] : 0;
}

struct Step {
  enum Kind { kField, kIndex, kEveryStruct, kEveryElement } kind;
  uint32_t key = 0;
  uint32_t index = 0;
};

struct Change {
  std::string path;
  uint32_t root = 0;
  std::vector<Step> steps;
  bool scale = false;
  double value = 0.0;
  bool applied = false;
};

std::mutex g_mutex;
std::vector<Change> g_changes;
std::map<uint64_t, int32_t> g_reads;  // (script << 32 | global) -> the value read
std::atomic<bool> g_pending{false};
std::atomic<bool> g_has_reads{false};
std::atomic<bool> g_track_readers{false};
thread_local uint32_t t_last_reader = 0;

bool Parse(const std::string& path, Change& change) {
  size_t i = 0;
  auto ident = [&]() {
    size_t start = i;
    while (i < path.size() && (std::isalnum(uint8_t(path[i])) || path[i] == '_' || path[i] == '*')) {
      ++i;
    }
    return path.substr(start, i - start);
  };
  std::string root = ident();
  if (root.empty() || root == "*") {
    return false;
  }
  change.root = QbKey(root);
  while (i < path.size()) {
    if (path[i] == '.') {
      ++i;
      std::string field = ident();
      if (field == "*") {
        change.steps.push_back({Step::kEveryStruct});
      } else if (!field.empty()) {
        change.steps.push_back({Step::kField, QbKey(field)});
      } else {
        return false;
      }
    } else if (path[i] == '[') {
      size_t close = path.find(']', i);
      if (close == std::string::npos || close == i + 1) {
        return false;
      }
      Step step{Step::kIndex};
      if (path[i + 1] == '*') {
        step.kind = Step::kEveryElement;
      } else {
        step.index = uint32_t(std::strtoul(path.c_str() + i + 1, nullptr, 10));
      }
      change.steps.push_back(step);
      i = close + 1;
    } else {
      return false;
    }
  }
  return true;
}

void Add(std::string path, double value, bool scale) {
  Change change;
  change.path = std::move(path);
  change.scale = scale;
  change.value = value;
  if (!Parse(change.path, change)) {
    REXLOG_ERROR("[script globals] '{}': not a path", change.path);
    return;
  }
  std::lock_guard lock(g_mutex);
  g_changes.push_back(std::move(change));
  g_pending = true;
}

// The global's node, through the globals that name another.
uint32_t FindNode(uint32_t key) {
  for (int hops = 0; hops < 4; ++hops) {
    uint32_t table = Load32(kGlobalTable);
    if (!table) {
      return 0;
    }
    uint32_t node = Load32(table + (key & 0x7FFF) * 4);
    for (int guard = 0; node && Load32(node + 4) != key && guard < 100000; ++guard) {
      node = Load32(node + 16);
    }
    if (!node || Load8(node + 2) != kTypeAlias) {
      return node;
    }
    key = Load32(node + 12);
  }
  return 0;
}

uint32_t FirstComponent(uint32_t s) {
  uint32_t first = Load32(s + 4);
  return first && Readable(first, 16) ? first : 0;
}

// Each value the steps reach from a value of `type` held at `slot`: f(slot, type, path).
template <typename F>
void Walk(uint8_t type, uint32_t slot, const std::vector<Step>& steps, size_t at, const std::string& path, F&& f) {
  if (at == steps.size()) {
    f(slot, type);
    return;
  }
  const Step& step = steps[at];
  uint32_t value = Load32(slot);
  if (step.kind == Step::kIndex && (type == kTypeVector2 || type == kTypeVector3)) {
    // A vector: a 4-byte header, then its floats.
    if (step.index >= (type == kTypeVector2 ? 2u : 3u)) {
      REXLOG_ERROR("[script globals] {}: no component {} of a vector", path, step.index);
      return;
    }
    Walk(kTypeFloat, value + 4 + step.index * 4, steps, at + 1, path, f);
    return;
  }
  if (step.kind == Step::kIndex || step.kind == Step::kEveryElement) {
    uint32_t count = type == kTypeArray ? Load32(value + 4) : 0;
    if (step.kind == Step::kIndex && step.index >= count) {
      REXLOG_ERROR("[script globals] {}: no element {} (a type {} of {})", path, step.index, type, count);
      return;
    }
    const uint32_t first = step.kind == Step::kIndex ? step.index : 0;
    const uint32_t last = step.kind == Step::kIndex ? step.index + 1 : std::min<uint32_t>(count, 4096);
    for (uint32_t index = first; index < last; ++index) {
      uint32_t element = count == 1 ? value + 8 : Load32(value + 8) + index * 4;
      Walk(Load8(value + 2), element, steps, at + 1, path, f);
    }
    return;
  }
  if (type != kTypeStruct) {
    REXLOG_ERROR("[script globals] {}: a field of a type {}", path, type);
    return;
  }
  bool found = false;
  int guard = 0;
  for (uint32_t c = FirstComponent(value); c && guard < 512; c = Load32(c + 12), ++guard) {
    uint8_t ctype = Load8(c + 2);
    if (step.kind == Step::kEveryStruct ? ctype == kTypeStruct : Load32(c + 4) == step.key) {
      found = true;
      Walk(ctype, c + 8, steps, at + 1, path, f);
      if (step.kind == Step::kField) {
        break;
      }
    }
  }
  if (!found) {
    REXLOG_ERROR("[script globals] {}: field not found", path);
  }
}

float AsFloat(uint32_t bits) {
  float f;
  std::memcpy(&f, &bits, 4);
  return f;
}

uint32_t FloatBits(float f) {
  uint32_t bits;
  std::memcpy(&bits, &f, 4);
  return bits;
}

void ApplyChanges() {
  for (Change& change : g_changes) {
    if (change.applied) {
      continue;
    }
    uint32_t node = FindNode(change.root);
    if (!node) {
      continue;
    }
    change.applied = true;
    int count = 0;
    Walk(Load8(node + 2), node + 12, change.steps, 0, change.path, [&](uint32_t slot, uint8_t type) {
      if (type == kTypeFloat) {
        float before = AsFloat(Load32(slot));
        float after = float(change.scale ? before * change.value : change.value);
        Store32(slot, FloatBits(after));
        REXLOG_INFO("[script globals] {}: {} -> {}", change.path, before, after);
        ++count;
      } else if (type == kTypeInteger) {
        int32_t before = int32_t(Load32(slot));
        int32_t after = int32_t(std::lround(change.scale ? before * change.value : change.value));
        Store32(slot, uint32_t(after));
        REXLOG_INFO("[script globals] {}: {} -> {}", change.path, before, after);
        ++count;
      } else {
        REXLOG_ERROR("[script globals] {}: a type {}, left as it is", change.path, type);
      }
    });
    if (!count) {
      REXLOG_ERROR("[script globals] {}: nothing changed", change.path);
    }
  }
  g_pending = std::any_of(g_changes.begin(), g_changes.end(), [](const Change& c) { return !c.applied; });
}

}  // namespace

uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(std::tolower(uint8_t(c)));
    for (int i = 0; i < 8; ++i) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
    }
  }
  return crc;
}

void Set(std::string path, double value) { Add(std::move(path), value, false); }

void Scale(std::string path, double factor) { Add(std::move(path), factor, true); }

void ReadAs(std::string_view script, std::string_view global, int32_t value) {
  std::lock_guard lock(g_mutex);
  g_reads[(uint64_t(QbKey(script)) << 32) | QbKey(global)] = value;
  g_has_reads = true;
}

void TrackReaders() { g_track_readers = true; }

uint32_t LastReader() { return t_last_reader; }

bool Pending() { return g_pending.load(std::memory_order_relaxed); }

bool NeedsScriptNames() {
  return g_has_reads.load(std::memory_order_relaxed) || g_track_readers.load(std::memory_order_relaxed);
}

bool OnResolve(uint8_t* base, uint32_t component, uint32_t global, uint32_t script) {
  if (g_pending.load(std::memory_order_relaxed)) {
    std::lock_guard lock(g_mutex);
    if (g_pending && std::any_of(g_changes.begin(), g_changes.end(),
                                 [global](const Change& c) { return !c.applied && c.root == global; })) {
      ApplyChanges();
    }
  }
  if (script && g_track_readers.load(std::memory_order_relaxed)) {
    t_last_reader = script;
  }
  if (!script || !g_has_reads.load(std::memory_order_relaxed)) {
    return false;
  }
  // Without the lock: the reads are all added before the hooks first run (src/mods.cpp's Configure, from
  // src/debug_menu.cpp), and never after.
  auto it = g_reads.find((uint64_t(script) << 32) | global);
  if (it == g_reads.end()) {
    return false;
  }
  const int32_t value = it->second;
  base[component + 2] = kTypeInteger;
  uint32_t bits = __builtin_bswap32(uint32_t(value));
  std::memcpy(base + component + 8, &bits, 4);
  return true;
}

}  // namespace ghbandhero::script

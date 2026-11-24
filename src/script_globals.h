// ghbandhero - the game's script globals changed in guest memory, and globals read as another value by one script: the
// mechanism the mods share (src/mods.cpp; GH Warriors of Rock's local_docs/47-mods-plan.md, where it was made).

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace ghbandhero::script {

// A name's QBKey: the CRC-32 of its lower-case letters without the final inversion.
uint32_t QbKey(std::string_view name);

// A global's value changed at a path, before the game reads it: "global", ".field", "[index]" (an array's element or a
// vector's component), "[*]" (every element) and ".*" (every field that is a struct), e.g.
// "difficulty_list_props.*.scroll_time" or "highway_scale_chart[0].hw_y_scale". An integer stays an integer (rounded).
// Applied once, at the first "$" read of any changed global (every global is loaded at the start and kept), so the
// change reaches the scripts the interpreter runs and the compiled ones alike.
void Set(std::string path, double value);
void Scale(std::string path, double factor);

// A global read as `value` by the script named `script` only ("$name" in that script); the global keeps its value.
// Only before the interpreter's hooks first run (src/mods.cpp's Configure): the reads are looked up without a lock.
void ReadAs(std::string_view script, std::string_view global, int32_t value);

// The script that last read a global on this thread ("$name"), kept from now on: a C function the scripts call can
// tell which script it runs for (src/mods.cpp: a miss's GuitarEvent_MissedNote before the instrument's volume ramp).
// Only before the interpreter's hooks first run, as ReadAs.
void TrackReaders();
uint32_t LastReader();

// Whether changes wait for their first read, and whether the interpreter's hooks must know each read's script (a read as
// another value, or TrackReaders; src/debug_menu.cpp).
bool Pending();
bool NeedsScriptNames();

// From src/debug_menu.cpp's GHBandHeroGlobalResolveHook: the component (r3) is about to become `global`'s value, read by the
// script named `script` (0 when not known). True when the component was written here (read as another value).
bool OnResolve(uint8_t* base, uint32_t component, uint32_t global, uint32_t script);

}  // namespace ghbandhero::script

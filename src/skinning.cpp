// ghbandhero - the skinning waits for the animation that writes its bones (GH5's src/skinning.cpp).
//
// The engine skins on the CPU: the draws' constants hold no bones. The three job workers turn each model's vertices with
// its palette (sub_82548500, the job sub_8254B140: [model + 116] 4x4 matrices, [model + 120] of them, 128 on a musician,
// the pieces of a musician sharing one) into a buffer that the draw then reads. A skeleton's update has two steps:
// - sub_82477FB8 starts the animation jobs if this frame updates it (sub_824322B0), which write the palette in stages,
//   and sets +136.
// - sub_82476758 queues the skinning of each piece (sub_8254B268), and only then, if +136 is set, waits for those jobs
//   (sub_8255CC80 on the group at 0x82E901F8 + 1160 + [+108] * 4).
// So the skinning can read a palette half written. In GH5, the same functions instruction for instruction (sub_824767F0
// there), what hangs from the clothes' and the physics' bones shot out for one frame in long wedges above 60 FPS
// (Gustavo: Axel Steel's coat, at 110, 130 and 160 FPS, on both renderers); a probe comparing the palette as the skinning
// began and ended found it changed during it 19 to 48 times a song, and at the very second of a stretch he noted down. It
// also happens at 60 FPS, fewer times. GH Warriors of Rock's own code (sub_8245D308) already waits first.
//
// Here the wait comes first, so the skinning starts on a finished palette: in GH5 the race went down to 1 to 3 times a
// song, one of them as the band is placed, and Gustavo played four songs with no stretch. The game's own wait stays and
// finds nothing to wait for. GH5's measured cost: the one frame where the whole band moves, about 30 ms to 44 in Party Play.

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>

#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghbandhero_skinning_order_fix, true, "GHBandHero",
                    "A musician's vertices are computed once the animation has finished writing the bones (false = as "
                    "the game does: it can read them half written, and clothes shoot out for a frame)");

namespace {

constexpr uint32_t kJobGroups = 0x82E901F8;

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

}  // namespace

REX_EXTERN(__imp__sub_82476758);
REX_EXTERN(__imp__sub_8255CC80);

// sub_82476758, r3 = the skeleton component: +136 its animation jobs are running, +108 its group's index.
REX_HOOK_RAW(sub_82476758) {
  if (REXCVAR_GET(ghbandhero_skinning_order_fix) && LoadWord(base, ctx.r3.u32 + 136) != 0) {
    const uint64_t component = ctx.r3.u64;
    ctx.r3.u64 = LoadWord(base, kJobGroups + 1160 + LoadWord(base, ctx.r3.u32 + 108) * 4);
    ctx.r4.u64 = 0;
    __imp__sub_8255CC80(ctx, base);
    ctx.r3.u64 = component;
  }
  __imp__sub_82476758(ctx, base);
}

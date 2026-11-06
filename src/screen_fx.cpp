// ghbandhero - the game's post-processing effects, each of which can be turned off.
//
// The engine's screen effects (the scripts' ScreenFX_AddFXInstance and ScreenFX_UpdateFXInstanceParams; the
// instances are made by sub_824DDD40) are created once per scene and then given new parameters by the scripts. All of
// them work on the 3D scene: the note highway and the HUD are drawn after them.
//
// GH Van Halen's composite applied six effects in one pass by a mask. Band Hero's, sub_8263E130, has two passes of the shader
// _Uber_ScreenFX (Pass0: _motionblur1, _bloom1, _dof1, _projection1, _tonemap1, _color1; Pass1: _motionblur1, _noise1,
// _vignette1), chosen from a word of flags with one bit per effect type that is on (+4 of the effects manager: 0x1
// MotionBlur, 0x2 Bloom2, 0x4 Godray (the bloom's variant too), 0x8 DOF3, 0x10 Tonemap, 0x20 Bright_Sat (the color),
// 0x40 Noise2 (the grain), 0x80 Vignette, 0x200 the projection). GH5's composite with _projection1 added to Pass0 and
// its _visualizer1 no longer chosen by Pass1 (the shader keeps the name, nothing sets the variant's bit): the bits below
// are GH5's, checked in the code that builds the variants' numbers. The composite reads that word three times: once for
// both passes' variants and their constants, then once for each pass's way of drawing (taking the scene and
// replacing it, or blending over it). The hooks take the bits of the effects turned off out of each read: the passes
// are then the variants without them, drawn the way the remaining effects need. The word itself is left alone, so an
// effect turned back on is applied from the next frame. The passes that feed them (the bloom's and the depth of
// field's blurred copies) still run; their result is not used.
//
// GH Van Halen's ink blotches are not part of Band Hero's composite (InkBlotch has no bit), so Band Hero has no switch for them.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghbandhero_fx_bloom, true, "GHBandHero",
                    "Screen effects: the bloom, the glow around lights and bright surfaces; false = off");
REXCVAR_DEFINE_BOOL(ghbandhero_fx_depth_of_field, true, "GHBandHero",
                    "Screen effects: the depth of field, the background blurred in some camera shots and behind the "
                    "menus and the pause menu; false = off");
REXCVAR_DEFINE_BOOL(ghbandhero_fx_grain, true, "GHBandHero",
                    "Screen effects: the film grain over the image; false = off (always off while the color "
                    "correction is off)");
REXCVAR_DEFINE_BOOL(ghbandhero_fx_color, true, "GHBandHero",
                    "Screen effects: the color correction (brightness, contrast, saturation): each venue's look, fades "
                    "to black, the image greyed while the rock meter is red; false = off, and the grain with it");
REXCVAR_DEFINE_BOOL(ghbandhero_fx_vignette, true, "GHBandHero",
                    "Screen effects: the vignette, the corners darkened where a venue or a moment turns it on; "
                    "false = off");

namespace {

constexpr uint32_t kBloom = 0x2;
constexpr uint32_t kDepthOfField = 0x8;
constexpr uint32_t kColor = 0x20;
constexpr uint32_t kGrain = 0x40;
constexpr uint32_t kVignette = 0x80;

uint32_t EffectsOff() {
  uint32_t off = 0;
  off |= REXCVAR_GET(ghbandhero_fx_bloom) ? 0 : kBloom;
  off |= REXCVAR_GET(ghbandhero_fx_depth_of_field) ? 0 : kDepthOfField;
  off |= REXCVAR_GET(ghbandhero_fx_grain) ? 0 : kGrain;
  off |= REXCVAR_GET(ghbandhero_fx_color) ? 0 : kColor;
  off |= REXCVAR_GET(ghbandhero_fx_vignette) ? 0 : kVignette;
  // In GH Van Halen the color pass also toned the grain down, and without it the grain was far stronger, so the grain
  // goes with the color (the launcher's rule). Here the grain is in the other pass: kept the same until it is seen.
  if (off & kColor) {
    off |= kGrain;
  }
  return off;
}

}  // namespace

// [[midasm_hook]] 0x8263E508, 0x8263E658 and 0x8263E71C (sub_8263E130, the screen effects' composite), each right after
// a "lwz rN,4(r19)": the effects' flags, for both passes' variants and constants (r11), Pass0's way of drawing (r4) and
// Pass1's (r11).
void GHBandHeroScreenFxFlagsHook(PPCRegister& flags) {
  const uint32_t off = EffectsOff();
  if (!off) {
    return;
  }
  static std::atomic<uint32_t> logged{0};
  if (logged.exchange(off) != off) {
    REXLOG_INFO("[screen fx] effects turned off: flags 0x{:03X} (ghbandhero_fx_*)", off);
  }
  flags.u32 &= ~off;
}

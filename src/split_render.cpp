// ghbandhero - the 3D scene drawn every frame (ghbandhero_scene_every_frame; off: the console's way, the scene over two frames).
//
// GH Warriors of Rock's src/split_render.cpp, the same engine. The engine splits its 3D scene over two frames ("split
// render mode"; not in GH Metallica, GH Van Halen or GH Aerosmith): the render state's mode (+3928 here, +1896 in GH
// Warriors of Rock: Band Hero's render state is laid out differently, its five split fields in the same order) is 1 from its
// init (sub_82413018 stores it at 0x82D9DFB8, the render thread's copy of the state, 0x82D9D000, with the split's render
// stage 7 at +3944; the main thread's copy 0x82DA6E70 carries it too). The script function SetSplitRenderMode
// (sub_824EB148: "mode" -> +3928, 0x039F4780 -> +3940, 0xBD09544C -> +3944) sets them from the boot's script.
// - The main thread's frame (sub_82414058) counts the frames while the mode is on: counter +3936 = (counter + 1) & 3,
//   the frame's kind +3932 = (counter & 1) + 1; with it off 7 and 1.
// - The scene renderer (sub_82565680): in a kind 1 frame the main scene runs the render stages up to the split's and
//   resolves the scene (sub_825669A0 at 0x82565C78); in a kind 2 frame it runs them from that stage on, the image
//   restored (NativeXRenderer's D113); each scene's mask names the kinds of frame it is drawn in. The main thread's
//   collection of the objects to draw (sub_82563FB8, sub_82564278, sub_82560A90) skips kind 2 frames, and
//   IsPrimaryFrame (sub_821B64A0) is true with the mode off or in a kind 1 frame.
// So each half of the 3D image is drawn every other frame: 30 times a second on the console at 60 FPS, half the chosen
// frame rate here; the note highway and the HUD every frame. With the mode off the renderer runs every stage of every
// scene in each frame (the engine's own path), and IsPrimaryFrame is always true (the scripts that align a camera cut
// or a musician's change to a kind of frame, src/framerate.cpp's kFrameExactScripts, then wait no frame or always one).
//
// GH Warriors of Rock measured (2026-11-15, Party Play): every frame about 1060-1090 indexed draws (468 and 416 in turn
// before), 0 failed draws; at 160 FPS with a 3440x1440 internal resolution 155-159 FPS, the process 2.60 cores (was
// 1.74), NativeXRenderer's thread 80 % of a core (was 44 %), the GPU about 65 % (was about 42 %).

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghbandhero_scene_every_frame, false, "GHBandHero",
                    "The 3D scene (the venue, the crowd and the band) drawn in every frame: the engine's split render "
                    "mode off. false = as on the console: half of the scene in one frame, the other half in the next, "
                    "so the scene is drawn at half the frame rate (the note highway and the HUD at the full rate)");

namespace {

// The split render mode (+3928) in the render thread's copy of the render state and in the main thread's.
constexpr uint32_t kRenderModeRender = 0x82D9DFB8;
constexpr uint32_t kRenderModeMain = 0x82DA7DC8;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

}  // namespace

// [[midasm_hook]] 0x82414074 (sub_82414058, the main thread's frame: "lwz r11,3928(r31)", the split render mode read
// before the frame's kind is counted). Written every frame, so a later SetSplitRenderMode cannot turn it back on.
void GHBandHeroSplitRenderHook() {
  if (!REXCVAR_GET(ghbandhero_scene_every_frame)) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  uint32_t was;
  std::memcpy(&was, base + kRenderModeMain, 4);
  static bool logged = false;
  if (!logged) {
    logged = true;
    REXLOG_INFO("[split render] off: the 3D scene is drawn in every frame (ghbandhero_scene_every_frame; the game's mode was "
                "{})",
                __builtin_bswap32(was));
  }
  const uint32_t off = 0;
  std::memcpy(base + kRenderModeMain, &off, 4);
  std::memcpy(base + kRenderModeRender, &off, 4);
}

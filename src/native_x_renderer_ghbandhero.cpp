// ghbandhero - NativeXRenderer for Band Hero: the game's D3D entry points and its D3D device and
// object layouts.
//
// The XDK Direct3D library is linked into default.xex (XDK 2.0.8276) at 0x8230AA90-0x8232A8EC, the draws included
// (0x8230C088-0x8230D358), with XGraphics-style helpers inside it (0x82312708-0x82314228). It is GH Van Halen's
// (XDK 2.0.7978) code: 561 of its 575 functions the same instruction for instruction, Present among the 14 others
// (recompiled, the same logic and offsets), so GH Van Halen's map and layouts are GH5's with the addresses translated,
// and GH5's, the same XDK and the same library, are this game's: its 36 hooked functions the same instruction for
// instruction but for branch targets and data addresses. The device layout is Dante's Inferno's up to the register shadow (POTCAWE's + 128), the current objects
// POTCAWE's + 144; the resource objects are POTCAWE's. Every address and offset here, with its evidence, is in
// docs/native-x-renderer-ghbandhero.md. The hooks only run the original function unless a NativeXRenderer feature
// is on (--native_x_renderer_trace, --native_x_renderer_verify, --native_x_renderer).

#include "native_x_renderer_ghbandhero.h"

#include <native_x_renderer/hooks.h>

// Draws. DrawVerticesUP and DrawIndexedVerticesUP (sub_8230CA80, 4 engine sites, no call of its own) commit their
// packets with an inlined EndVertices ([dev+48] = [dev+13444]); the engine's three BeginVertices / EndVertices pairs
// (sub_822FE4F0, sub_822FFC18, sub_82303DD8) call EndVertices (sub_8230C078), which is that same store. EndVertices is
// not hooked: inline_draw_end commits all of them.
NATIVE_X_RENDERER_HOOK(8230CF10, DrawIndexedVertices)
NATIVE_X_RENDERER_HOOK(8230CB20, DrawVertices)
NATIVE_X_RENDERER_HOOK(8230C520, DrawVerticesUP)  // calls BeginVertices, inlines the end
NATIVE_X_RENDERER_HOOK(8230C088, BeginVertices)
NATIVE_X_RENDERER_HOOK(8230C568, BeginIndexedVertices)  // only from DrawIndexedVerticesUP

// Render targets, clears, resolves, tiling. The engine sets targets through sub_82310A10, an alias that branches to
// SetRenderTarget. EndTiling resolves the tiles itself (the engine passes flags 0x300 and a destination).
NATIVE_X_RENDERER_HOOK(823103E8, SetRenderTarget)
NATIVE_X_RENDERER_HOOK(82310738, SetDepthStencilSurface)
NATIVE_X_RENDERER_HOOK(82315C90, Clear)  // calls ClearF
NATIVE_X_RENDERER_HOOK(82315B68, ClearF)
NATIVE_X_RENDERER_HOOK(8231F238, Resolve)
NATIVE_X_RENDERER_HOOK(82321218, BeginTiling)
NATIVE_X_RENDERER_HOOK(82321588, EndTiling)
NATIVE_X_RENDERER_HOOK(823210A8, SetPredication)
NATIVE_X_RENDERER_HOOK(823102B8, SetViewport)  // D3DVIEWPORT9 (integers), stored as floats at +12656
NATIVE_X_RENDERER_HOOK(8230F918, SetScissorRect)

// Shaders, vertex input, textures, constants. The engine writes constants straight into the device too (in GH Van
// Halen VS in 18 functions, PS in 33; not counted here) and patches sampler states in the fetch constants, without a
// call. SetTexture, SetStreamSource
// and the constant setters take the dirty bit or mask from their caller.
NATIVE_X_RENDERER_HOOK(8230B768, SetVertexShader)
NATIVE_X_RENDERER_HOOK(8230B5A8, SetPixelShader)
NATIVE_X_RENDERER_HOOK(8230B938, SetVertexDeclaration)
NATIVE_X_RENDERER_HOOK(8230FA88, SetStreamSource)
NATIVE_X_RENDERER_HOOK(8230FC30, SetIndices)
NATIVE_X_RENDERER_HOOK(82314838, SetTexture)
NATIVE_X_RENDERER_HOOK(8230B320, SetVertexShaderConstantF)
NATIVE_X_RENDERER_HOOK(8230B3F8, SetPixelShaderConstantF)

// Resources: typed thunks over the common Lock (sub_82312028) and Unlock (sub_82311258), and the cache flush over a
// range. The locks return the pointer (no data argument) and ignore the offset and size.
NATIVE_X_RENDERER_HOOK(823123F0, VertexBufferLock)
NATIVE_X_RENDERER_HOOK(823115A0, VertexBufferUnlock)
NATIVE_X_RENDERER_HOOK(82312440, IndexBufferLock)
NATIVE_X_RENDERER_HOOK(823115B0, IndexBufferUnlock)
NATIVE_X_RENDERER_HOOK(82312840, TextureUnlock)
NATIVE_X_RENDERER_HOOK(8231E5E8, FlushCpuRange)

// Command buffer, queries, frame. The kick submits the segment and continues in the same memory; NewSegment moves the
// write pointer to a newly allocated segment (from the kick, the reservation sub_82317578 and Present). The engine
// calls Synchronize (sub_82320950, Swap with no front buffer), then Present, which calls VdSwap with the front buffer
// in r4; the library's worker thread sub_82322F38 can present again.
NATIVE_X_RENDERER_HOOK(82317340, KickSegment)
NATIVE_X_RENDERER_HOOK(82316878, NewSegment)
NATIVE_X_RENDERER_HOOK(8230BAC0, SetShaderGPRAllocation)
NATIVE_X_RENDERER_HOOK(8231B2C0, QueryIssue)
NATIVE_X_RENDERER_HOOK(8231B548, QueryGetData)
NATIVE_X_RENDERER_HOOK(82320528, Swap)
NATIVE_X_RENDERER_HOOK(82320958, Present)

namespace ghbandhero {

const native_x_renderer::Profile& NativeXRendererProfile() {
  static const native_x_renderer::Profile profile = [] {
    native_x_renderer::Profile p;
    p.game = "Band Hero (4156085C)";
    p.xdk_version = "2.0.8276";
    p.output_width = 1280;  // the presentation parameters come from a video-mode table at run time
    p.output_height = 720;
    // The finished scene's copy, before the HUD and the note highway: every resolve goes through the render targets'
    // helper sub_82566170 (its color resolve returns to 0x8256620C), as in GH5 and GH WoR, and the game alternates two
    // kinds of frame, both shown, that copy the scene from different places into the same 1280x720 texture. Antialiasing
    // one of them only would leave every other frame's edges as they were, so, as GH WoR, every copy through the helper
    // of the output's size is antialiased (NativeXRenderer takes no other), and the HUD and the highway are drawn after.
    p.scene_resolve_caller = 0x8256620C;
    // The ultrawide hooks (src/widescreen.cpp) widen the 16:9 cameras by AspectFactor() and keep the 2D layer at
    // 16:9 in the center: NativeXRenderer may fill a wider output (native_x_renderer_fill_screen).
    p.widens_cameras = true;
    // The screen effects' reduced copies of the screen (640x360, 320x180, 160x90) at the game's resolution (D127). Their
    // blurs step by constants for 320x180 texels, so at the render scale the taps skip host texels: in GH Warriors of
    // Rock, this engine, the background its depth of field blurs showed a grid at 3440x1440 (the rocker creator's
    // character screen), smooth with this. Set here and in GH5 as there.
    p.unscaled_screen_fractions = true;
    native_x_renderer::DeviceLayout& d = p.device;
    // Six dirty masks from +0: VS constants, PS constants, register groups and the program, fetch slots (low half,
    // slot 0 in bit 31) and the 0x2280 group, the 0x2300/0x2380 groups with the clip planes and bool/loop constants,
    // the tile-dependent state.
    d.vs_constants_dirty = 0;
    d.ps_constants_dirty = 8;
    d.fetch_constants_dirty = 24;
    d.fetch_dirty_slot0_bit = 31;
    d.command_buffer_write = 48;
    d.fetch_constants = 1152;
    d.vs_float_constants = 1920;
    d.ps_float_constants = 6016;
    d.bool_loop_constants = 10112;
    d.register_groups = {{
        {10368, 0x2000, 16},
        {10444, 0x2100, 21},
        {10528, 0x2180, 5},
        {10548, 0x2200, 12},
        {10596, 0x2280, 21},
        {10680, 0x2300, 38},
        {10832, 0x2380, 8},
    }};
    d.fence_counter = 10908;
    d.write_back_block = 10896;
    d.flags = 10940;
    d.vertex_declaration = 11824;
    d.index_buffer = 12444;
    d.render_targets = 12448;
    d.depth_stencil = 12464;
    d.vertex_buffers = 12468;  // one pointer per stream
    d.textures = 12552;
    d.viewport = 12656;  // x, y, width, height, min z, max z (floats)
    d.pixel_shader = 12700;
    d.vertex_shader = 12704;
    d.predication_mask = 12716;
    d.tile_count = 12756;
    d.tile_rects = 12760;
    // DrawVerticesUP, DrawIndexedVerticesUP and EndVertices commit BeginVertices' / BeginIndexedVertices' packets
    // with [dev+48] = [dev+13444].
    d.inline_draw_end = 13444;
    native_x_renderer::ObjectLayout& o = p.objects;
    o.texture_fetch = 28;
    o.surface_info = 24;
    o.surface_color_depth_info = 28;
    o.surface_size = 36;
    o.surface_width_minus_one = {18, 14};
    o.surface_height_minus_one = {3, 15};
    o.buffer_address = 24;
    o.buffer_size = 28;
    return p;
  }();
  return profile;
}

}  // namespace ghbandhero

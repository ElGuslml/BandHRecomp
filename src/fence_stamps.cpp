// Band Hero: stamps the game writes into memory the GPU reads, written where the page watches do not see them (GH Warriors
// of Rock's src/fence_stamps.cpp, its docs/fence-stamps.md; the same D3D code, other addresses).
//
// When SetStreamSource (sub_8230FA88) or SetIndices (sub_8230FC30) unbinds a vertex or index buffer, D3D writes the
// device's current fence (+10908 of the device) into the buffer's header (resource + 8), to know later when the GPU is
// done with it. The game keeps those headers in physical memory, next to vertex data and inside textures' packed mip
// tails, and the renderers watch physical memory for the game's writes: each stamp on a watched page faults (on the
// game's render thread) and invalidates the page, which is then uploaded, protected or hashed again; a page stamped
// every frame is left unwatched (NativeXRenderer D97) and compared at every request. In GH Warriors of Rock 85 % of the
// writes that faulted in a song were these two stores; at 240 FPS NativeXRenderer's copies into its mirror went from 644
// to 231 a frame and its renderer from 3.65 to 3.00 ms a frame.
//
// Two more (GH Warriors of Rock's performance plan, Phase 3, a9fd845): a render object's last drawn frame (sub_8263C278
// writes the frame number at object + 128 each time it draws the object; GH Warriors of Rock's sub_8263D160, +112) and
// SetTexture's fence in the previous texture's header (sub_82314838, texture + 8). The render objects share pages with
// textures (the free space of their packed mip tails), so in GH Warriors of Rock nearly every write fault left on a
// texture's page in a song was the object's stamp (9-26 a frame): each a fault on the game's thread, an invalidation,
// the page protected again and its textures checked, and a small texture hashed whole loaded again with the same image.
// There no vertex or index range a draw read held a word these two stamped in the last 20 ms over two songs, texture
// ranges only in the mip tails' free space, which no image is made of (NativeXRenderer D148); written so, texture loads
// went from 0.5-1 a frame to none and the process used about 1 % less CPU.
//
// The hooks write the stamp through the host view of physical memory the watches never protect (the one the SDK's
// data providers write through) and skip the game's store: the same bytes in the same memory, read back by D3D's own
// checks through its view, without a fault or an invalidation. No GPU work reads a resource's header or a render
// object; a real write to the page (new vertex data) still faults and uploads it, the current stamp with it.
// ghbandhero_fence_stamps_unwatched = false (the D3D fences of vertex and index buffers) and ghbandhero_draw_stamps_unwatched = false
// (the render objects' frame and the textures' fence) store as the game does.

#include <rex/cvar.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <bit>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghbandhero_fence_stamps_unwatched, true, "GHBandHero",
                    "The fence D3D writes into a vertex or index buffer's header when SetStreamSource or SetIndices "
                    "unbinds it is written where the renderers' page watches do not see it: no fault, and the page "
                    "is not uploaded or compared again for it. false = the game's own store");
REXCVAR_DEFINE_BOOL(ghbandhero_draw_stamps_unwatched, true, "GHBandHero",
                    "The frame a render object was last drawn in, and the fence D3D writes into a texture's header when "
                    "SetTexture unbinds it, are written where the renderers' page watches do not see them: no fault on "
                    "the textures' pages they share. false = the game's own store");

namespace {

// Writes `value` at `address` (guest virtual) through the physical view; false if it is not in physical memory (no
// watch there: the game's store does it).
bool StoreUnwatched(bool enabled, uint32_t address, uint32_t value) {
  if (!enabled) {
    return false;
  }
  auto* kernel = rex::system::kernel_state();
  rex::memory::Memory* memory = kernel ? kernel->memory() : nullptr;
  if (!memory) {
    return false;
  }
  if ((address & 3) != 0) {
    return false;
  }
  const uint32_t physical = memory->GetPhysicalAddress(address);
  if (physical == UINT32_MAX) {
    return false;
  }
  *memory->TranslatePhysical<uint32_t*>(physical) = std::byteswap(value);  // guest memory is big-endian
  return true;
}

}  // namespace

// stw r11,8(r28) at 0x8230FB18 (SetStreamSource: the stream's previous vertex buffer).
bool GHBandHeroStreamFenceHook(PPCRegister& r11, PPCRegister& r28) {
  return StoreUnwatched(REXCVAR_GET(ghbandhero_fence_stamps_unwatched), r28.u32 + 8, r11.u32);
}

// stw r11,8(r30) at 0x8230FC5C (SetIndices: the previous index buffer).
bool GHBandHeroIndicesFenceHook(PPCRegister& r11, PPCRegister& r30) {
  return StoreUnwatched(REXCVAR_GET(ghbandhero_fence_stamps_unwatched), r30.u32 + 8, r11.u32);
}

// stw r11,8(r28) at 0x82314954 (SetTexture: the sampler's previous texture).
bool GHBandHeroTextureFenceHook(PPCRegister& r11, PPCRegister& r28) {
  return StoreUnwatched(REXCVAR_GET(ghbandhero_draw_stamps_unwatched), r28.u32 + 8, r11.u32);
}

// stw r11,128(r31) at 0x8263C3C8 (sub_8263C278: the frame the render object is drawn in).
bool GHBandHeroObjectFrameHook(PPCRegister& r11, PPCRegister& r31) {
  return StoreUnwatched(REXCVAR_GET(ghbandhero_draw_stamps_unwatched), r31.u32 + 128, r11.u32);
}

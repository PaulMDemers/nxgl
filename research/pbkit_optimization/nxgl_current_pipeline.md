# NXGL Current Pipeline

## Layering

NXGL is already split into:

- public OpenGL-like API: `include/nxgl.h`
- GL state and compatibility layer: `src/nxgl.c`
- pbkit/NV097 backend: `src/backend/nxgl_backend.c`

The architecture doc describes this as a PBKit renderer backend, and the code
matches that: the GL layer knows backend-facing types and calls
`nxgl_backend_*()` functions rather than emitting pbkit commands directly.

## Hardware Path Today

`src/backend/nxgl_backend.c` does real hardware work:

- calls `XVideoSetMode()` and `pb_init()`
- uses `pb_show_front_screen()`
- allocates a write-combined contiguous vertex buffer
- starts frames with `pb_wait_for_vbl()`, `pb_reset()`, and
  `pb_target_back_buffer()`
- clears through NV20/NV097 clear registers and
  `pb_erase_depth_stencil_buffer()`
- tracks depth, cull, blend, scissor, viewport, texture, and texenv state
- groups vertices into batches by render state
- binds vertex arrays through `NV097_SET_VERTEX_DATA_ARRAY_FORMAT` and
  `NV097_SET_VERTEX_DATA_ARRAY_OFFSET`
- uploads an MVP matrix to transform constants
- draws through `NV097_DRAW_ARRAYS`
- swaps through `pb_finished()`

This means NXGL is not bypassing pbkit. The issue is that the hardware path is
not yet the dominant path for enough features.

## Shader Usage Today

The backend has shader variants for:

- untextured color
- single texture modes: modulate, replace, decal, blend, add, subtract,
  add-signed, interpolate
- fixed two-texture modulation
- cube-map sampling
- 3D texture sampling

`nxgl.mk` lists every shader-generated `.inl` as a build artifact. That makes
shader inclusion visible to consumers even when a sample uses only simple
geometry.

The backend currently reloads vertex/pixel program state per batch based on the
batch texture mode. This is correct but expensive and more visible than it
needs to be.

Initial backend caching now skips identical shader uploads and unchanged
depth/cull/blend/scissor setup within a frame. The shared vertex attribute
pointers are also emitted once per flush instead of once per batch. Texture
stage descriptors and texture disables are cached per unit, with single-texture
and multitexture paths using distinct setup helpers so unit enables stay
explicit.

## Software/CPU Compatibility Paths

`src/nxgl.c` keeps CPU-visible shadows:

- `shadow_color_buffer`
- `shadow_depth_buffer`
- `shadow_stencil_buffer`

Those buffers are allocated by default during `nxglInit()`. The default is
compatibility/readback mode because the probe suite validates APIs that need
CPU-readable framebuffer state.

Major CPU paths:

- software color/depth/stencil clears for the shadow buffers
- `shadow_fill_bounds()` for primitive readback, stencil/depth interlocks,
  alpha test, logic op, color masks, stipple, and approximate raster coverage
- CPU texture sampling for shadow readback and extended texture env/combine
  behavior
- selection and feedback record generation
- user clip planes and projected clip-volume handling
- point/line/polygon raster lowering for wide points, wide lines, polygon
  point/line mode, and stipple
- pixel transfer APIs: `glReadPixels`, `glDrawPixels`, `glCopyPixels`,
  `glBitmap`
- legacy lighting/material approximation before backend emission
- display-list capture/replay and state query semantics

These paths are valuable for conformance tests, but they are overkill for
render-only demos such as NeHe and most samples.

## Existing Fast-Mode Hook

NXGL has both runtime and init-time readback controls. `nxglInitFast()` and
`nxglSetDefaultReadbackEnabled(GL_FALSE)` before `nxglInit()` skip CPU
color/depth/stencil shadow allocation entirely. `nxglSetReadbackEnabled()` can
still toggle the runtime behavior after init and lazily allocates the shadow
buffers if readback is re-enabled.

## Current Bottlenecks

1. CPU shadow allocation and updates are enabled by default in compatibility
   mode, but render-only fast init can skip allocation.
2. `shadow_fill_bounds()` loops over bounding boxes and runs depth/stencil/blend
   logic in software.
3. Selection/feedback and pixel-transfer paths force CPU-side projected
   geometry and framebuffer state.
4. The backend now submits native `QUADS` batches for four-vertex quads and
   compact fast-mode strip/fan primitive runs, but larger mesh-like primitives
   still lack indexed mesh-style submission.
5. Shader variants are cached by current shader key, but program upload versus
   shader select has not been split yet.
6. Texture stage state is cached per unit, but the descriptor values still use
   magic constants that need named builders.
7. `pb_begin()` / `pb_end()` calls are frequent, especially during shader loads,
   texture setup, and per-batch state setup.
8. The backend waits for `pb_busy()` at flush boundaries, limiting overlap.
9. Lighting/fog/texgen are mostly precomputed in CPU-side compatibility code
   rather than carried as vertex-program inputs/constants.
10. Cube/3D texture native presentation is disabled or partial because current
    hardware encoding needs more validation.

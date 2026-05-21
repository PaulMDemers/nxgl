# PBKit Graphics API Survey

This survey is based on the local nxdk checkout at
`../nxdk/lib/pbkit` and the nxdk pbkit samples.

## What PBKit Provides

### Frame and buffer lifecycle

`lib/pbkit/pbkit.h` exposes the core frame lifecycle:

- `pb_init()` / `pb_kill()`
- `pb_show_front_screen()` / `pb_show_debug_screen()` /
  `pb_show_depth_screen()`
- `pb_wait_for_vbl()`
- `pb_reset()`
- `pb_finished()`
- `pb_target_back_buffer()` / `pb_target_extra_buffer()`
- `pb_back_buffer_width()`, `pb_back_buffer_height()`,
  `pb_back_buffer_pitch()`
- `pb_erase_depth_stencil_buffer()`
- `pb_set_viewport()`

The header comment explicitly warns to call `pb_erase_depth_stencil_buffer()`
at the beginning of a frame to avoid a performance hit from stale depth tile
data.

### Pushbuffer access

`lib/pbkit/pbkit_pushbuffer.h` exposes the low-level command API:

- `pb_begin()` / `pb_end()`
- `pb_push*()` helpers for integer and float parameters
- matrix upload helpers such as `pb_push_transposed_matrix()`
- subchannel constants such as `SUBCH_3D`

The same header documents a hard pushbuffer-size limit between flushes. This is
important for NXGL because excessive per-primitive `pb_begin()` / `pb_end()`
traffic or huge unflushed batches can both be bad.

### Basic helpers

`lib/pbkit/pbkit_draw.h` provides:

- `pb_fill()`
- `pb_set_depth_stencil_buffer_region()`

These are useful for clears and rectangles, but they are not a GL renderer.

### NV register/object definitions

`lib/pbkit/nv_objects.h` exposes the NV04/NV10/NV20/NV097 register surface
used by pbkit callers. Relevant graphics definitions include:

- primitive modes: `POINTS`, `LINES`, `TRIANGLES`, `QUADS`, `POLYGON`
- texture stage registers: `NV20_TCL_PRIMITIVE_3D_TX_OFFSET`,
  `NV20_TCL_PRIMITIVE_3D_TX_FORMAT`,
  `NV20_TCL_PRIMITIVE_3D_TX_ENABLE`
- vertex/index submission:
  `NV097_SET_VERTEX_DATA_ARRAY_FORMAT`,
  `NV097_SET_VERTEX_DATA_ARRAY_OFFSET`,
  `NV097_DRAW_ARRAYS`, and `NV20_TCL_PRIMITIVE_3D_INDEX_DATA`
- shader/program registers:
  `NV097_SET_TRANSFORM_PROGRAM_START`,
  `NV097_SET_TRANSFORM_PROGRAM_LOAD`,
  `NV097_SET_TRANSFORM_PROGRAM`,
  `NV097_SET_SHADER_STAGE_PROGRAM`, and combiner state

`lib/pbkit/nv20_shader.h` defines the NV20 vertex-program instruction layout.
This is another sign that shader/program upload is a normal part of pbkit-era
3D rendering rather than an NXGL-specific invention.

## What PBKit Does Not Provide

pbkit does not expose a high-level OpenGL-style fixed-function API for:

- modelview/projection/texture matrix stacks
- immediate-mode GL state tracking
- GL texture object names, mip completeness, or pixel-store unpacking
- OpenGL lighting/material state
- selection or feedback modes
- `glReadPixels`, `glDrawPixels`, `glCopyPixels`, `glBitmap`
- OpenGL display-list capture/replay
- OpenGL validation/error semantics

Those behaviors must be implemented above pbkit, approximated through NV097
state/shaders, or intentionally left unsupported.

## What nxdk Samples Do

The nxdk `samples/triangle` path is a minimal pbkit 3D example. It:

- initializes pbkit
- starts each frame with `pb_wait_for_vbl()`, `pb_reset()`, and
  `pb_target_back_buffer()`
- clears depth and color with `pb_erase_depth_stencil_buffer()` and `pb_fill()`
- uploads a vertex shader and pixel shader/combiner program
- sets vertex array attributes with `NV097_SET_VERTEX_DATA_ARRAY_FORMAT` and
  `NV097_SET_VERTEX_DATA_ARRAY_OFFSET`
- draws through `NV097_DRAW_ARRAYS`

The nxdk `samples/mesh` path adds texture setup and indexed drawing. It uploads
texture stage registers, then submits index data with
`NV20_TCL_PRIMITIVE_3D_INDEX_DATA`.

## Implication for NXGL

"Just use pbkit" should mean:

- use pbkit/NV097 hardware state and pushbuffer submission more directly
- avoid redundant CPU-side raster/shadow work for render-only paths
- avoid redundant shader/state uploads
- use indexed submission where NXGL duplicates vertices

It should not mean removing shaders entirely. The pbkit samples themselves use
program upload, and NXGL needs programmable vertex transforms and combiner
programs to emulate OpenGL fixed-function behavior on NV2A.


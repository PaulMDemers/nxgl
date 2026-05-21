# Architecture

NXGL is split into a public API, a GL state implementation, and a PBKit renderer
backend.

## Public API

`include/nxgl.h` defines the GL-compatible entry points, constants, and legacy
types exposed to applications.

## GL State Layer

`src/nxgl.c` remains a single translation unit so the compatibility layer can
keep file-local state and helpers. The implementation body is split under
`src/core/`:

- `internal_state.inc`: private types, global GL state, prototypes, and perf
  counter storage
- `state_helpers.inc`: display-list, texture-object, pixel-store, evaluator,
  texgen, and validation helpers
- `transform_emit.inc`: math, lighting/fog application, clipping,
  selection/feedback emission, and primitive submission
- `shadow_compat.inc`: CPU shadow/readback raster and texture sampling paths
- `core_api.inc`: initialization, queries, viewport, clear, and accumulation
- `immediate_api.inc`: matrices and immediate-mode entry points
- `pixel_selection_api.inc`: pixel transfer, raster position, bitmap,
  read/draw/copy pixels, select, and feedback APIs
- `state_array_list_api.inc`: enables, attrib stacks, client arrays, draw
  arrays/elements, and display lists
- `texture_api.inc`: wrapper for the texture implementation sections
- `texture_lifecycle_env.inc`: texture name/bind/residency APIs and texenv
  state
- `texture_validation_convert.inc`: texture/pixel validation, packing, and
  conversion helpers
- `texture_upload_copy.inc`: uncompressed texture parameter, image,
  sub-image, copy, 1D/2D/3D, and LOD APIs
- `texture_compressed_api.inc`: compressed texture image and sub-image APIs

Together, these sections own OpenGL-like behavior:

- matrix stacks and transforms
- texture object state and upload paths
- lighting, material, fog, texenv, and combine state
- client arrays and immediate-mode assembly
- selection and feedback modes
- raster state, pixel transfer, depth, stencil, blend, and logic interlocks

This layer should remain backend-aware only through the backend contract.

## Performance Counters

`NxglPerfCounters` exposes lightweight runtime counters for optimization work.
They are intentionally coarse: frame swaps, clears, immediate/array/list draw
entry points, backend primitive pushes, backend shader/render-state and
texture-stage cache behavior, shadow/readback buffer allocation/free activity,
shadow fragments, pixel-transfer calls, and texture uploads. Use `nxglResetPerfCounters()` and
`nxglGetPerfCounters()` around a workload to compare optimization passes.

Render-only applications should prefer `nxglInitFast()` or
`nxglSetDefaultReadbackEnabled(GL_FALSE)` before `nxglInit()`. That skips
allocation of the CPU color/depth/stencil shadow buffers and keeps the
compatibility/readback path out of the hot loop unless the application opts
back in with `nxglSetReadbackEnabled(GL_TRUE)`. Disabling readback after
startup frees existing shadow buffers; re-enabling it allocates fresh shadows
cleared to the current clear values.

## PBKit Backend

`src/backend/nxgl_backend.h` and `src/backend/nxgl_backend.c` provide the current
renderer contract used by the GL state layer. The backend owns PBKit setup,
GPU-facing texture storage, draw submission, scissor/depth/blend/cull state, and
shader selection.

The state layer only refers to backend-facing types and calls through the
`NxglBackend*` and `nxgl_backend_*` namespace. Keeping the PBKit implementation
under `src/backend/` makes the current renderer explicit while leaving room for
the interface to be moved, mocked, or swapped later.

## Shaders

Shader sources live as `.cg` files. Generated `.inl` files are build outputs.

## Compatibility Tracking

Compatibility is tracked in `docs/opengl_coverage.md`. A feature should only be
marked verified when it has focused probe coverage and a runnable nxdk build.
The runnable probes live in `validation/`; the autorun suite regenerates its
probe table from that directory at build time.

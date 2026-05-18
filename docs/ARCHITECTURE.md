# Architecture

NXGL is split into a public API, a GL state implementation, and a PBKit renderer
backend.

## Public API

`include/nxgl.h` defines the GL-compatible entry points, constants, and legacy
types exposed to applications.

## GL State Layer

`src/nxgl.c` owns OpenGL-like behavior:

- matrix stacks and transforms
- texture object state and upload paths
- lighting, material, fog, texenv, and combine state
- client arrays and immediate-mode assembly
- selection and feedback modes
- raster state, pixel transfer, depth, stencil, blend, and logic interlocks

This layer should remain backend-aware only through the backend contract.

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

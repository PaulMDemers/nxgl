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

The backend is still bundled because the current implementation shares concrete
types between the state layer and renderer. Keeping it under `src/backend/`
makes that coupling explicit and leaves a path for a cleaner backend interface.

## Shaders

Shader sources live as `.cg` files. Generated `.inl` files are build outputs.

## Compatibility Tracking

Compatibility is tracked in `docs/opengl_coverage.md`. A feature should only be
marked verified when it has focused probe coverage and a runnable nxdk build.

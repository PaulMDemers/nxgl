# NXGL

NXGL is an OpenGL-style compatibility layer for nxdk applications on the
original Xbox. It provides a growing fixed-function OpenGL 1.x API surface and
translates that state into the current PBKit/NV2A renderer backend.

The project is intentionally laid out so it can be consumed as a sibling
checkout today and as an nxdk submodule in the future.

## Status

NXGL is active work. The API surface is broad, but compatibility is tracked by
focused ROM-level probes rather than by API presence alone. See
[`docs/opengl_coverage.md`](docs/opengl_coverage.md) for the current coverage
map.

## Layout

- `include/nxgl.h` exposes the public GL-compatible API.
- `src/nxgl.c` is the single translation-unit driver for the compatibility
  layer.
- `src/core/` contains the split NXGL implementation sections: state/helpers,
  transform and primitive emission, shadow/readback compatibility, public GL
  APIs, array/list handling, and texture APIs.
- `src/backend/` contains the current PBKit renderer backend and shader sources.
- `src/common3d/` contains shared backend shader sources.
- `examples/` contains small standalone nxdk consumers of `nxgl.mk`.
- `validation/` contains the focused probe apps and autorun suite used to track
  compatibility behavior.
- `nxgl.mk` exports the source, include, and shader variables used by nxdk apps.

Generated shader `.inl` files are build artifacts and are not tracked.

## Using NXGL

Add NXGL as a sibling checkout next to your nxdk app, or override `NXGL_DIR` to
point at the checkout.

```make
NXGL_DIR ?= $(CURDIR)/../nxgl
include $(NXGL_DIR)/nxgl.mk

SRCS += $(NXGL_SRCS)
SHADER_OBJS += $(NXGL_SHADER_OBJS)
CFLAGS += $(NXGL_CFLAGS)
```

Then include the API from application code:

```c
#include "nxgl.h"
```

For render-only applications that do not call `glReadPixels`, pixel transfer,
or accumulation APIs, use the fast initializer to skip CPU readback shadow
allocation:

```c
nxglInitFast();
```

Readback is enabled by default for compatibility and for the validation probes.
With readback disabled, filled triangle and quad primitives can take a native
fast path, while readback-dependent APIs such as `glReadPixels`,
`glDrawPixels`, `glCopyPixels`, `glBitmap`, and `glAccum` report
`GL_INVALID_OPERATION`.

Applications that need custom startup flow can call
`nxglSetDefaultReadbackEnabled(GL_FALSE)` before `nxglInit()`, or call
`nxglSetReadbackEnabled(GL_FALSE)` after init. Disabling readback at runtime
frees any CPU color/depth/stencil shadow buffers; enabling it again reallocates
and clears them to the current clear values.

For optimization work, NXGL exposes lightweight counters:

```c
NxglPerfCounters counters;
nxglResetPerfCounters();
/* render workload */
nxglGetPerfCounters(&counters);
```

The counters distinguish backend primitive pushes, CPU array expansion,
position/normal transform work, lighting/fog evaluations, clipping work,
backend pbkit command-block starts, shader/render-state and texture-stage cache
behavior, backend flush/finish timing, shadow/readback buffer allocation/free
activity, shadow fragments, pixel-transfer calls, texture uploads, and
frame-level activity. `validation/110_gl_perf_counter_probe` runs a small set
of representative render and readback workloads and prints these counters in a
stable `PERF ...` format.

## Documentation

- [`docs/BUILDING.md`](docs/BUILDING.md): build environment and smoke checks.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): library layout and backend
  boundaries.
- [`docs/COMPATIBILITY_PASS.md`](docs/COMPATIBILITY_PASS.md): current texture,
  lighting, and raster-edge validation focus.
- [`docs/RELEASE.md`](docs/RELEASE.md): release checklist.
- [`docs/opengl_coverage.md`](docs/opengl_coverage.md): compatibility coverage.

For the standard local verification pass on Windows, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\verify_nxgl.ps1
```

## License

NXGL is released under the MIT License. See [`LICENSE`](LICENSE).

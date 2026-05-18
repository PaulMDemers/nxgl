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
- `src/nxgl.c` contains GL state, validation, matrix, texture, lighting, raster,
  selection, feedback, and pixel-path behavior.
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

## Documentation

- [`docs/BUILDING.md`](docs/BUILDING.md): build environment and smoke checks.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): library layout and backend
  boundaries.
- [`docs/COMPATIBILITY_PASS.md`](docs/COMPATIBILITY_PASS.md): current texture,
  lighting, and raster-edge validation focus.
- [`docs/RELEASE.md`](docs/RELEASE.md): release checklist.
- [`docs/opengl_coverage.md`](docs/opengl_coverage.md): compatibility coverage.

## License

NXGL is released under the MIT License. See [`LICENSE`](LICENSE).

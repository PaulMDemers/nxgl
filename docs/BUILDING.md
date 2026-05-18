# Building NXGL

NXGL is consumed by nxdk applications rather than built as a standalone static
library. The repository exports Make variables through `nxgl.mk`.

## Requirements

- Windows with MSYS2, or another shell environment compatible with nxdk builds.
- A working nxdk checkout.
- `NXDK_DIR` pointing at the nxdk checkout.
- nxdk shader tools available on `PATH`.

Example MSYS2 setup:

```sh
export MSYSTEM=MINGW64
export NXDK_DIR=/c/path/to/.nxdk
export PATH="$NXDK_DIR/bin:$NXDK_DIR/tools/cg/win:/usr/bin:$PATH"
```

## Smoke Check

The repository root Makefile verifies that `nxgl.mk` can be included:

```sh
make
make print-vars
```

The smallest real nxdk consumer is included in `examples/hello_triangle`:

```sh
make -C examples/hello_triangle
```

The repository root also exposes this as:

```sh
make example
```

For an external build, include NXGL from an nxdk consumer app:

```sh
cd /path/to/consumer
make clean
make
```

## Generated Files

The nxdk shader path generates `.inl` files from `.cg` shader sources. Those
files are ignored and should not be committed.

## Validation Probes

Focused compatibility probes live in `validation/`. Build the autorun suite or a
single probe with:

```sh
make -C validation/autorun_suite
make -C validation/48_gl_cubemap_render_probe
```

The autorun suite is also available from the root:

```sh
make validation-autorun
```

Grouped probe targets are available for the current compatibility focus:

```sh
make -C validation texture
make -C validation lighting
make -C validation raster
```

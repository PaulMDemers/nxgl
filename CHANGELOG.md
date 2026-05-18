# Changelog

## Unreleased

- Added a standalone `examples/hello_triangle` nxdk consumer.
- Moved focused compatibility probes into `validation/` with grouped texture,
  lighting, raster, and autorun build targets.
- Cleaned the PBKit renderer boundary to use `NxglBackend*` types and
  `nxgl_backend_*` entry points.
- Initial public repository layout.
- Added MIT license and public project documentation.
- Added `nxgl.mk` for sibling checkout and future submodule consumption.
- Kept generated shader `.inl` files out of source control.
- Cleaned NXGL package so it does not contain application-specific source names
  or demo helpers.

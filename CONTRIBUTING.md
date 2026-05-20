# Contributing

Contributions are welcome. NXGL is still evolving, so changes should be small,
focused, and easy to validate.

## Guidelines

- Keep NXGL free of demo-specific code and assets.
- Prefer adding behavior behind the existing GL API surface rather than creating
  application-specific helpers.
- Keep backend-specific code under `src/backend/`.
- Do not commit generated `.inl`, `.obj`, `.d`, `.xbe`, `.iso`, or `.exe` files.
- Update `docs/opengl_coverage.md` when compatibility status changes.

## Validation

At minimum, verify that a consumer app still builds:

```sh
make
cd /path/to/consumer
make
```

For GL behavior changes, add or update focused probes before marking coverage as
verified.

Before a release-facing change, run the standard verification wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\verify_nxgl.ps1
```

For a fast layout-only check before larger builds:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\verify_nxgl_layout.ps1
```

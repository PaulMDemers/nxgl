# Release Checklist

Use this checklist before tagging or publishing NXGL.

## Source Hygiene

- `rg "app-specific|demo helper" src include nxgl.mk README.md` returns no
  matches unless it is part of documentation text.
- No generated `.inl`, `.obj`, `.d`, `.xbe`, `.iso`, or `.exe` files are staged.
- `LICENSE` is MIT and present at the repository root.
- `docs/opengl_coverage.md` reflects the current probe status.

## Build Smoke

From MSYS2 with nxdk configured:

```sh
make
make print-vars
cd /path/to/consumer
make clean
make
```

## Version Notes

Before tagging, update `CHANGELOG.md` with compatibility changes, known gaps,
and the latest successful consumer build.

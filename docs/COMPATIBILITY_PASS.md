# Compatibility Pass

The current NXGL validation pass is organized around the areas that most often
show visual regressions in demo ports and standalone probes.

## Texture State

Build with:

```sh
make -C validation texture
```

This group covers upload/readback paths, mip and LOD controls, texture env and
combine state, multitexture, cube maps, 3D textures, compressed textures, copy
texture behavior, texgen, and pixel-store stride handling.

## Lighting

Build with:

```sh
make -C validation lighting
```

This group covers material state, color material, light model flags, shade model,
spot attenuation, two-sided lighting, and local-viewer specular behavior.

## Raster Edge Behavior

Build with:

```sh
make -C validation raster
```

This group covers viewport and projection clipping, polygon and line clipping,
point/raster edge acceptance, cull behavior, masked/scissored clears, pixel
bounds, and depth/stencil/blend/logic interlocks.

## Autorun

The full autorun suite remains the release gate:

```sh
make -C validation/autorun_suite
```

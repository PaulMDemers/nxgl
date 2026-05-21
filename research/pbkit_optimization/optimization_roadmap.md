# NXGL Optimization Roadmap

## Goals

- Keep the validation-suite compatibility mode intact.
- Make samples and render-only applications use a hardware-first path by
  default or with a very obvious switch.
- Reduce pbkit state churn and shader reloads.
- Move fixed-function work to NV2A where it is representable.
- Keep unsupported or readback-heavy OpenGL APIs explicitly marked as
  compatibility paths.

## Pass 0: Instrument Before Changing Behavior

Status: initial coarse counters are implemented through `NxglPerfCounters`.
Backend shader/render-state and texture-stage cache upload/hit counters are
also exposed.

Add optional counters behind `NXGL_PERF_OVERLAY` or a new lightweight profiling
flag:

- GL primitives submitted
- backend vertices emitted
- batches emitted
- shader program uploads and cache hits
- texture-stage uploads/disables and cache hits
- pbkit command blocks started
- CPU shadow primitives touched
- shadow pixels touched
- readback/pixel-transfer API calls
- time spent in GL assembly, shadow update, backend flush, and frame finish

Expected payoff: lets us prove whether later passes improve real workloads and
helps answer reviewer concerns with measurements instead of vibes.

## Pass 1: Make Render-Only Fast Mode First-Class

Status: initial fast init is implemented with `nxglInitFast()` and
`nxglSetDefaultReadbackEnabled(GL_FALSE)`.

Current hooks: `nxglInitFast()`, `nxglSetDefaultReadbackEnabled(GL_FALSE)`,
and `nxglSetReadbackEnabled(GL_FALSE)`.

Actions:

- Add a documented compile-time or init-time fast-mode helper, e.g.
  `nxglInitFast()` or `nxglSetDefaultReadbackEnabled(GL_FALSE)` before
  allocation.
- Update NXGL examples and NeHe NXGL demos to disable readback unless the demo
  calls readback/pixel APIs.
- In fast mode, avoid allocating color/depth/stencil shadow buffers at init.
  Today readback is toggled after init, so the memory cost is already paid.
- Add validation tests proving readback APIs return `GL_INVALID_OPERATION` in
  fast mode.

Expected payoff: removes the largest CPU/memory cost from demos without
changing compatibility mode.

Risk: API behavior change if done implicitly. Prefer explicit fast init or a
sample-level call first.

## Pass 2: Cache Backend State and Shader Programs

Status: initial shader/render-state caching is implemented.

Actions:

- Track current backend shader key:
  `{color, tex mode, multitexture, cube, 3d}`.
- Track current texture stage binding/filter/wrap/format per unit.
- Track current depth/cull/blend/scissor state.
- Only emit pbkit commands when the key changes.
- Split shader upload from shader select where possible.
- Consider loading all common programs once at init if NV2A program memory and
  combiner state permit; otherwise cache "last uploaded" and avoid reuploading
  identical programs batch-to-batch.
- Hoist fixed vertex attribute pointer setup out of the per-batch loop when
  drawing from the shared backend vertex buffer.

Implemented so far:

- Shader uploads are skipped when the next batch uses the same shader key.
- Depth/cull/blend/scissor registers are skipped when unchanged.
- Texture-stage descriptors and disables are skipped per unit when unchanged.
- Vertex attribute pointers are emitted once per flush instead of once per
  batch.
- `108_gl_backend_state_cache_probe` covers adjacent batch transitions across
  shader keys, texture descriptors, multitexture unit enables, cube/2D texture
  modes, blend, cull, and scissor.

Remaining:

- Split shader upload from shader select where possible.

Expected payoff: lower per-batch overhead and less pushbuffer traffic.

Risk: stale state bugs. Add tiny focused probes for changing texture env,
multitexture, cull/depth/blend, and scissor across adjacent draws.

## Pass 3: Indexed Submission

nxdk's mesh sample uses `NV20_TCL_PRIMITIVE_3D_INDEX_DATA`. NXGL currently
triangulates and emits duplicated vertices, then draws with `NV097_DRAW_ARRAYS`.

Actions:

- Add an optional backend index buffer path.
- For quads, quad strips, fans, and display-list replay, keep unique vertices
  and emit indices.
- Preserve current array path as fallback.
- Benchmark NeHe cubes and repeated textured quads.

Expected payoff: lower vertex bandwidth and CPU copy cost for cube-heavy and
mesh-like scenes.

Risk: index batching limits; mesh sample batches index dwords in groups of 120.
Need similar chunking and tests.

## Pass 4: Hardware Fixed-Function Subsets

Move representable GL fixed-function work into vertex programs/constants:

- modelview/projection/viewport transforms
- normal transform/normalization/rescale where enabled
- one- and two-light diffuse/specular common cases
- fog coordinate/fog factor for common modes
- texgen object/eye linear common cases

Keep exact/rare compatibility behavior CPU-side until proven.

Expected payoff: less per-vertex CPU work, cleaner normal/lighting behavior for
samples.

Risk: shader constant layout churn. The nxdk samples already warn that changing
shader source affects constant locations, so document and test constant maps.

## Pass 5: Native Texture Coverage

Actions:

- Replace magic texture format/filter/wrap values with named builders using
  `nv_objects.h` bitfields.
- Validate cube-map format encoding on hardware and xemu; re-enable native cube
  presentation once stable.
- Revisit native 3D texture format descriptors.
- Avoid rebuilding native textures on every LOD parameter change if the chosen
  effective level did not change.

Expected payoff: fewer CPU shadow-only texture cases and less texture upload
work.

Risk: hardware-specific texture descriptor behavior. Keep focused probes.

## Pass 6: Compatibility Path Isolation

Actions:

- Split CPU readback/shadow code into a clearly named module or section.
- Add docs that call it "compatibility/readback path", not "renderer".
- Make APIs that force compatibility mode explicit in documentation.
- Add sample guidance:
  - render-only apps: fast mode
  - tests/tools/readback: compatibility mode

Expected payoff: easier review, easier maintenance, less confusion about why
software paths exist.

## Suggested Execution Order

1. Instrument counters.
2. Add pre-init fast mode and update samples/demos.
3. Cache backend shader/state emission.
4. Add indexed submission for quads/cubes.
5. Move common lighting/fog/texgen into vertex programs.
6. Revisit cube/3D native texture encoding.

## Validation Plan

After each pass:

- run the NXGL autorun suite in compatibility mode
- run NeHe NXGL captures in fast mode
- run LazyFoo GL 50/51 captures if they depend on NXGL
- add one focused regression probe for the changed path
- record before/after perf counters in `docs/` or `research/`

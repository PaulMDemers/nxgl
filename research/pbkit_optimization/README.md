# NXGL PBKit Optimization Research

Date: 2026-05-21

This folder captures the first research pass for moving more NXGL work onto
the NV2A through pbkit while preserving the compatibility wins from the current
OpenGL probe suite.

## Short Answer for Reviewers

NXGL already uses pbkit as its hardware submission layer. The current backend
sets up pbkit, allocates write-combined GPU buffers, uploads NV097 state, binds
textures, and submits vertex arrays through `NV097_DRAW_ARRAYS`.

The expensive software work is mostly in the GL compatibility layer, not in a
separate renderer competing with pbkit. It exists because the validation suite
covers OpenGL behaviors that pbkit does not provide directly: `glReadPixels`,
selection/feedback, pixel transfer, CPU-visible depth/stencil/color readback,
OpenGL clip rules, legacy lighting, display lists, and exact state queries.

The shader files are present because pbkit is a pushbuffer/register helper, not
a complete fixed-function OpenGL implementation. nxdk's own pbkit triangle and
mesh samples upload vertex programs and pixel combiner programs directly. NXGL
does the same today, but too aggressively reloads shader variants and carries
more CPU shadowing than render-only applications need.

## Contents

- `pbkit_api_survey.md` - what pbkit exposes and what it does not.
- `nxgl_current_pipeline.md` - how NXGL currently uses pbkit and where software
  paths enter.
- `optimization_roadmap.md` - concrete optimization passes, risk, and expected
  payoff.
- `reviewer_response.md` - concise explanation suitable for PR/review threads.

## High-Level Direction

1. Make render-only fast mode the normal demo/sample path.
2. Keep compatibility/readback mode for validation and legacy APIs.
3. Cache pbkit/NV097 state and shader variants instead of reuploading every
   batch.
4. Add indexed primitive submission to avoid duplicated vertex traffic.
5. Move fixed-function lighting/fog/texgen subsets into vertex programs where
   practical.
6. Keep selection, feedback, and pixel readback explicitly CPU/compatibility
   paths unless a later pass adds hardware readback.


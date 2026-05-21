# Reviewer Response Draft

NXGL is not trying to replace pbkit. It already uses pbkit as the backend for
NV2A submission: frame setup, pushbuffer writes, texture binding, vertex array
attributes, draw calls, clears, depth/blend/cull/scissor state, and buffer
swaps all go through pbkit/NV097 commands.

The software-heavy parts are compatibility layers above pbkit. They exist
because the current validation target includes legacy OpenGL features that
pbkit does not expose directly: readback, pixel transfer, selection/feedback,
display lists, exact error/state query behavior, CPU-visible depth/stencil
interlocks, and several fixed-function approximations.

The shaders are there because pbkit is not a fixed-function OpenGL renderer.
It is a low-level pushbuffer/register helper. The nxdk pbkit triangle and mesh
samples also upload vertex programs and pixel/combiner programs. NXGL needs
shader/combiner variants to emulate OpenGL texture env, transforms, cube/3D
texture sampling, and eventually fixed-function lighting/fog/texgen on NV2A.

That said, the review concern is valid: NXGL currently keeps compatibility
readback enabled by default and reloads more backend state/shader data than it
should. The optimization plan is to keep compatibility mode for probes, but
make render-only fast mode first-class for demos/samples, cache pbkit state and
shader variants, use indexed submission where possible, and move more
fixed-function work into NV2A vertex programs.


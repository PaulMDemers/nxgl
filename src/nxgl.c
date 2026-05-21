#include "nxgl.h"

#include "nxgl_backend.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xboxkrnl/xboxkrnl.h>

/*
 * NXGL is kept as one translation unit so internal helpers and state can remain
 * file-local while the implementation is split into readable sections.
 */
#include "core/internal_state.inc"
#include "core/state_helpers.inc"
#include "core/transform_emit.inc"
#include "core/shadow_compat.inc"
#include "core/core_api.inc"
#include "core/immediate_api.inc"
#include "core/pixel_selection_api.inc"
#include "core/state_array_list_api.inc"
#include "core/texture_api.inc"

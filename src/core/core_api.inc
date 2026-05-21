int nxglInit(void)
{
    int status;

    matrix_identity(modelview);
    matrix_identity(projection);
    matrix_identity(texture_matrix);
    matrix_mode = GL_MODELVIEW;
    modelview_stack_top = 0;
    projection_stack_top = 0;
    texture_stack_top = 0;
    attrib_stack_top = 0;
    client_attrib_stack_top = 0;
    current_color = (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    current_index = 0.0f;
    active_texture = GL_TEXTURE0;
    client_active_texture = GL_TEXTURE0;
    vertex_array = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    color_array = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    normal_array = (ClientArray){ false, 3, GL_FLOAT, 0, NULL };
    for (int i = 0; i < 4; ++i) {
        texcoord_array[i] = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    }
    memset(texture_1d_enabled, 0, sizeof(texture_1d_enabled));
    memset(texture_2d_enabled, 0, sizeof(texture_2d_enabled));
    memset(texture_3d_enabled, 0, sizeof(texture_3d_enabled));
    memset(texture_cube_map_enabled, 0, sizeof(texture_cube_map_enabled));
    init_texgen_state();
    memset(texture_binding_1d, 0, sizeof(texture_binding_1d));
    memset(texture_binding_2d, 0, sizeof(texture_binding_2d));
    memset(texture_binding_3d, 0, sizeof(texture_binding_3d));
    memset(texture_binding_cube_map, 0, sizeof(texture_binding_cube_map));
    for (int i = 0; i < 4; ++i) {
        current_u[i] = 0.0f;
        current_v[i] = 0.0f;
        current_r[i] = 0.0f;
        texture_env_mode[i] = GL_MODULATE;
        texture_env_color[i] = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
        texture_combine_rgb[i] = GL_MODULATE;
        texture_combine_alpha[i] = GL_MODULATE;
        texture_source_rgb[i][0] = GL_TEXTURE;
        texture_source_rgb[i][1] = GL_PREVIOUS;
        texture_source_rgb[i][2] = GL_CONSTANT;
        texture_source_alpha[i][0] = GL_TEXTURE;
        texture_source_alpha[i][1] = GL_PREVIOUS;
        texture_source_alpha[i][2] = GL_CONSTANT;
        texture_operand_rgb[i][0] = GL_SRC_COLOR;
        texture_operand_rgb[i][1] = GL_SRC_COLOR;
        texture_operand_rgb[i][2] = GL_SRC_ALPHA;
        texture_operand_alpha[i][0] = GL_SRC_ALPHA;
        texture_operand_alpha[i][1] = GL_SRC_ALPHA;
        texture_operand_alpha[i][2] = GL_SRC_ALPHA;
        texture_rgb_scale[i] = 1.0f;
        texture_alpha_scale[i] = 1.0f;
    }
    for (int i = 0; i < 16; ++i) {
        init_texture_object(&texture_objects[i]);
    }
    for (int i = 0; i < 8; ++i) {
        init_light_object(&lights[i], i);
    }
    recording_list = 0;
    recording_mode = 0;
    list_base = 0;
    replaying_list = false;
    begin_mode = NXGL_NO_BEGIN_MODE;
    pending_count = 0;
    point_size = 1.0f;
    line_width = 1.0f;
    line_stipple_factor = 1;
    line_stipple_pattern = 0xffffu;
    line_stipple_enabled = false;
    polygon_mode = GL_FILL;
    memset(polygon_stipple_pattern, 0xff, sizeof(polygon_stipple_pattern));
    polygon_stipple_enabled = false;
    polygon_offset_point_enabled = false;
    polygon_offset_line_enabled = false;
    polygon_offset_fill_enabled = false;
    polygon_offset_factor = 0.0f;
    polygon_offset_units = 0.0f;
    shade_model = GL_SMOOTH;
    cull_enabled = false;
    cull_face_mode = GL_BACK;
    front_face_mode = GL_CCW;
    current_edge_flag = GL_TRUE;
    perspective_correction_hint = GL_DONT_CARE;
    point_smooth_hint = GL_DONT_CARE;
    line_smooth_hint = GL_DONT_CARE;
    polygon_smooth_hint = GL_DONT_CARE;
    fog_hint = GL_DONT_CARE;
    clear_color_value[0] = 0.0f;
    clear_color_value[1] = 0.0f;
    clear_color_value[2] = 0.0f;
    clear_color_value[3] = 0.0f;
    clear_index_value = 0.0f;
    clear_color = 0x00000000;
    current_normal = (NxglBackendVec3){ 0.0f, 0.0f, 1.0f };
    lighting_enabled = false;
    light_model_local_viewer = false;
    light_model_two_side = false;
    light_model_color_control = GL_SINGLE_COLOR;
    light_model_ambient = (NxglBackendColor){ 0.2f, 0.2f, 0.2f, 1.0f };
    color_material_enabled = false;
    fog_enabled = false;
    normalize_enabled = false;
    rescale_normal_enabled = false;
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        clip_planes[i].enabled = GL_FALSE;
        clip_planes[i].equation[0] = 0.0;
        clip_planes[i].equation[1] = 0.0;
        clip_planes[i].equation[2] = 0.0;
        clip_planes[i].equation[3] = 0.0;
    }
    scissor_test_enabled = false;
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = 640;
    viewport[3] = 480;
    scissor_box[0] = 0;
    scissor_box[1] = 0;
    scissor_box[2] = 640;
    scissor_box[3] = 480;
    depth_clear_value = 1.0f;
    depth_range_near = 0.0f;
    depth_range_far = 1.0f;
    depth_func = GL_LEQUAL;
    depth_test_enabled = true;
    depth_write_enabled = true;
    stencil_test_enabled = false;
    stencil_clear_value = 0;
    stencil_func = GL_ALWAYS;
    stencil_ref = 0;
    stencil_value_mask = 0xffffffffu;
    stencil_write_mask = 0xffffffffu;
    stencil_fail = GL_KEEP;
    stencil_zfail = GL_KEEP;
    stencil_zpass = GL_KEEP;
    blend_enabled = false;
    draw_buffer_mode = GL_BACK;
    read_buffer_mode = GL_BACK;
    alpha_test_enabled = false;
    alpha_test_func = GL_ALWAYS;
    alpha_test_ref = 0.0f;
    color_write_mask[0] = GL_TRUE;
    color_write_mask[1] = GL_TRUE;
    color_write_mask[2] = GL_TRUE;
    color_write_mask[3] = GL_TRUE;
    color_logic_op_enabled = false;
    logic_op_mode = GL_COPY;
    multisample_enabled = true;
    sample_alpha_to_coverage_enabled = false;
    sample_alpha_to_one_enabled = false;
    sample_coverage_enabled = false;
    sample_coverage_value = 1.0f;
    sample_coverage_invert = GL_FALSE;
    fog_mode = GL_EXP;
    fog_color = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
    fog_density = 1.0f;
    fog_start = 0.0f;
    fog_end = 1.0f;
    accum_clear_value[0] = 0.0f;
    accum_clear_value[1] = 0.0f;
    accum_clear_value[2] = 0.0f;
    accum_clear_value[3] = 0.0f;
    pack_alignment = 4;
    pack_row_length = 0;
    pack_skip_rows = 0;
    pack_skip_pixels = 0;
    pack_image_height = 0;
    pack_skip_images = 0;
    pack_swap_bytes = GL_FALSE;
    pack_lsb_first = GL_FALSE;
    unpack_alignment = 4;
    unpack_row_length = 0;
    unpack_skip_rows = 0;
    unpack_skip_pixels = 0;
    unpack_image_height = 0;
    unpack_skip_images = 0;
    unpack_swap_bytes = GL_FALSE;
    unpack_lsb_first = GL_FALSE;
    pixel_zoom_x = 1.0f;
    pixel_zoom_y = 1.0f;
    for (int i = 0; i < 4; ++i) {
        pixel_transfer_scale[i] = 1.0f;
        pixel_transfer_bias[i] = 0.0f;
    }
    for (int i = 0; i < 10; ++i) {
        pixel_maps[i].size = 2;
        pixel_maps[i].values[0] = 0.0f;
        pixel_maps[i].values[1] = 1.0f;
    }
    pixel_maps[pixel_map_index(GL_PIXEL_MAP_I_TO_I)].size = 256;
    pixel_maps[pixel_map_index(GL_PIXEL_MAP_S_TO_S)].size = 256;
    for (int i = 0; i < 256; ++i) {
        GLfloat value = (GLfloat)i / 255.0f;
        pixel_maps[pixel_map_index(GL_PIXEL_MAP_I_TO_I)].values[i] = value;
        pixel_maps[pixel_map_index(GL_PIXEL_MAP_S_TO_S)].values[i] = value;
    }
    init_eval_maps();
    auto_normal_enabled = GL_FALSE;
    map_grid1_n = 1;
    map_grid1_u1 = 0.0f;
    map_grid1_u2 = 1.0f;
    map_grid2_un = 1;
    map_grid2_vn = 1;
    map_grid2_u1 = 0.0f;
    map_grid2_u2 = 1.0f;
    map_grid2_v1 = 0.0f;
    map_grid2_v2 = 1.0f;
    current_raster_position[0] = 0.0f;
    current_raster_position[1] = 0.0f;
    current_raster_position[2] = 0.0f;
    current_raster_position[3] = 1.0f;
    current_raster_position_valid = true;
    render_mode = GL_RENDER;
    shadow_readback_enabled = true;
    selection_buffer = NULL;
    selection_buffer_size = 0;
    selection_write_count = 0;
    selection_hits = 0;
    selection_overflow = false;
    name_stack_depth = 0;
    feedback_buffer = NULL;
    feedback_buffer_size = 0;
    feedback_write_count = 0;
    feedback_overflow = false;
    feedback_type = GL_2D;
    color_material_face = GL_FRONT_AND_BACK;
    color_material_parameter = GL_AMBIENT_AND_DIFFUSE;
    material_state = (MaterialState){
        { 0.2f, 0.2f, 0.2f, 1.0f },
        { 0.8f, 0.8f, 0.8f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        0.0f
    };
    material_back_state = material_state;
    camera_x = 0.0f;
    camera_y = 0.0f;
    camera_z = -6.0f;
    last_error = GL_NO_ERROR;
    native_frame_started = false;
    status = nxgl_backend_init();
    if (status != 0) {
        return status;
    }
    shadow_width = nxgl_backend_back_buffer_width();
    shadow_height = nxgl_backend_back_buffer_height();
    viewport[2] = shadow_width;
    viewport[3] = shadow_height;
    scissor_box[2] = shadow_width;
    scissor_box[3] = shadow_height;
    if (shadow_width > SHADOW_MAX_WIDTH || shadow_height > SHADOW_MAX_HEIGHT) {
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_color_buffer = (uint32_t *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(uint32_t),
                                                                    0,
                                                                    SHADOW_MAXRAM,
                                                                    0,
                                                                    PAGE_READWRITE);
    if (shadow_color_buffer == NULL) {
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_depth_buffer = (float *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(float),
                                                                0,
                                                                SHADOW_MAXRAM,
                                                                0,
                                                                PAGE_READWRITE);
    if (shadow_depth_buffer == NULL) {
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_stencil_buffer = (uint8_t *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(uint8_t),
                                                                    0,
                                                                    SHADOW_MAXRAM,
                                                                    0,
                                                                    PAGE_READWRITE);
    if (shadow_stencil_buffer == NULL) {
        MmFreeContiguousMemory(shadow_depth_buffer);
        shadow_depth_buffer = NULL;
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_clear(clear_color);
    shadow_clear_depth(depth_clear_value);
    shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    return 0;
}

void nxglShutdown(void)
{
    if (shadow_stencil_buffer != NULL) {
        MmFreeContiguousMemory(shadow_stencil_buffer);
        shadow_stencil_buffer = NULL;
    }
    if (shadow_depth_buffer != NULL) {
        MmFreeContiguousMemory(shadow_depth_buffer);
        shadow_depth_buffer = NULL;
    }
    if (shadow_color_buffer != NULL) {
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
    }
    free(accum_buffer);
    accum_buffer = NULL;
    for (int i = 1; i < 16; ++i) {
        if (texture_objects[i].allocated) {
            destroy_texture_image(&texture_objects[i]);
            texture_objects[i].allocated = false;
        }
    }
    for (int i = 1; i < 256; ++i) {
        clear_display_list(&display_lists[i]);
    }
    nxgl_backend_shutdown();
}

void nxglSetCamera(float x, float y, float z, float rx, float ry, float rz)
{
    camera_x = x;
    camera_y = y;
    camera_z = z;
    nxgl_backend_set_camera(x, y, z, rx, ry, rz);
}

void nxglSetReadbackEnabled(GLboolean enabled)
{
    bool next = enabled != GL_FALSE;
    if (shadow_readback_enabled == next) {
        return;
    }
    shadow_readback_enabled = next;
    if (shadow_readback_enabled) {
        shadow_clear(clear_color);
        shadow_clear_depth(depth_clear_value);
        shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    }
}

void nxglResetPerfCounters(void)
{
    memset(&perf_counters, 0, sizeof(perf_counters));
}

void nxglGetPerfCounters(NxglPerfCounters *counters)
{
    if (counters == NULL) {
        return;
    }
    *counters = perf_counters;
}

void nxglSwapBuffers(const char *title, const char *detail)
{
    ensure_native_frame_started();
    nxgl_backend_finish(title, detail);
    native_frame_started = false;
    ++perf_counters.frames;
}

const GLubyte *glGetString(GLenum name)
{
    switch (name) {
    case GL_VENDOR:
        return (const GLubyte *)"nxdk";
    case GL_RENDERER:
        return (const GLubyte *)"NXGL NV2A fixed-function shim";
    case GL_VERSION:
        return (const GLubyte *)"1.1 NXGL draft";
    case GL_EXTENSIONS:
        return (const GLubyte *)"GL_ARB_multitexture GL_EXT_bgra GL_EXT_texture_edge_clamp";
    default:
        set_error(GL_INVALID_ENUM);
        return NULL;
    }
}

GLenum glGetError(void)
{
    GLenum error = last_error;
    last_error = GL_NO_ERROR;
    return error;
}

GLboolean glIsEnabled(GLenum cap)
{
    int unit = active_texture_index();
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    if (eval_index >= 0) {
        return map2 ? eval_maps2[eval_index].enabled : eval_maps1[eval_index].enabled;
    }
    if (texgen_coord >= 0) {
        return unit >= 0 ? texgen_state[unit][texgen_coord].enabled : GL_FALSE;
    }
    if (clip >= 0) {
        return clip_planes[clip].enabled;
    }
    if (light >= 0) {
        return lights[light].enabled ? GL_TRUE : GL_FALSE;
    }
    switch (cap) {
    case GL_BLEND:
        return blend_enabled ? GL_TRUE : GL_FALSE;
    case GL_LINE_STIPPLE:
        return line_stipple_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_STIPPLE:
        return polygon_stipple_enabled ? GL_TRUE : GL_FALSE;
    case GL_DEPTH_TEST:
        return depth_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_CULL_FACE:
        return cull_enabled ? GL_TRUE : GL_FALSE;
    case GL_SCISSOR_TEST:
        return scissor_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_STENCIL_TEST:
        return stencil_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_ALPHA_TEST:
        return alpha_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_LOGIC_OP:
    case GL_INDEX_LOGIC_OP:
        return color_logic_op_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_POINT:
        return polygon_offset_point_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_LINE:
        return polygon_offset_line_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_FILL:
        return polygon_offset_fill_enabled ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_1D:
        return unit >= 0 && texture_1d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_2D:
        return unit >= 0 && texture_2d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_3D:
        return unit >= 0 && texture_3d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_CUBE_MAP:
        return unit >= 0 && texture_cube_map_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_LIGHTING:
        return lighting_enabled ? GL_TRUE : GL_FALSE;
    case GL_FOG:
        return fog_enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_MATERIAL:
        return color_material_enabled ? GL_TRUE : GL_FALSE;
    case GL_NORMALIZE:
        return normalize_enabled ? GL_TRUE : GL_FALSE;
    case GL_RESCALE_NORMAL:
        return rescale_normal_enabled ? GL_TRUE : GL_FALSE;
    case GL_CURRENT_RASTER_POSITION_VALID:
        return current_raster_position_valid ? GL_TRUE : GL_FALSE;
    case GL_EDGE_FLAG:
        return current_edge_flag;
    case GL_VERTEX_ARRAY:
        return vertex_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_ARRAY:
        return color_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_NORMAL_ARRAY:
        return normal_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_COORD_ARRAY:
        return unit >= 0 && texcoord_array[unit].enabled ? GL_TRUE : GL_FALSE;
    case GL_AUTO_NORMAL:
        return auto_normal_enabled;
    case GL_MULTISAMPLE:
        return multisample_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        return sample_alpha_to_coverage_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_ALPHA_TO_ONE:
        return sample_alpha_to_one_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_COVERAGE:
        return sample_coverage_enabled ? GL_TRUE : GL_FALSE;
    default:
        set_error(GL_INVALID_ENUM);
        return GL_FALSE;
    }
}

void glGetIntegerv(GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    int client_unit = client_texture_index();
    int map_index = pixel_map_index_from_size_pname(pname);

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if (map_index >= 0) {
        params[0] = pixel_maps[map_index].size;
        return;
    }

    switch (pname) {
    case GL_MATRIX_MODE:
        params[0] = (GLint)matrix_mode;
        break;
    case GL_MODELVIEW_STACK_DEPTH:
        params[0] = modelview_stack_top + 1;
        break;
    case GL_PROJECTION_STACK_DEPTH:
        params[0] = projection_stack_top + 1;
        break;
    case GL_TEXTURE_STACK_DEPTH:
        params[0] = texture_stack_top + 1;
        break;
    case GL_VIEWPORT:
        memcpy(params, viewport, sizeof(viewport));
        break;
    case GL_MAX_VIEWPORT_DIMS:
        params[0] = shadow_width;
        params[1] = shadow_height;
        break;
    case GL_SCISSOR_BOX:
        memcpy(params, scissor_box, sizeof(scissor_box));
        break;
    case GL_POLYGON_MODE:
        params[0] = (GLint)polygon_mode;
        params[1] = (GLint)polygon_mode;
        break;
    case GL_LINE_STIPPLE_PATTERN:
        params[0] = (GLint)line_stipple_pattern;
        break;
    case GL_LINE_STIPPLE_REPEAT:
        params[0] = line_stipple_factor;
        break;
    case GL_POINT_SIZE_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = 1;
        params[1] = 64;
        break;
    case GL_POINT_SIZE_GRANULARITY:
    case GL_LINE_WIDTH_GRANULARITY:
        params[0] = 1;
        break;
    case GL_CULL_FACE_MODE:
        params[0] = (GLint)cull_face_mode;
        break;
    case GL_FRONT_FACE:
        params[0] = (GLint)front_face_mode;
        break;
    case GL_PERSPECTIVE_CORRECTION_HINT:
        params[0] = (GLint)perspective_correction_hint;
        break;
    case GL_POINT_SMOOTH_HINT:
        params[0] = (GLint)point_smooth_hint;
        break;
    case GL_LINE_SMOOTH_HINT:
        params[0] = (GLint)line_smooth_hint;
        break;
    case GL_POLYGON_SMOOTH_HINT:
        params[0] = (GLint)polygon_smooth_hint;
        break;
    case GL_FOG_HINT:
        params[0] = (GLint)fog_hint;
        break;
    case GL_STENCIL_CLEAR_VALUE:
        params[0] = stencil_clear_value;
        break;
    case GL_STENCIL_FUNC:
        params[0] = (GLint)stencil_func;
        break;
    case GL_STENCIL_REF:
        params[0] = stencil_ref;
        break;
    case GL_STENCIL_VALUE_MASK:
        params[0] = (GLint)stencil_value_mask;
        break;
    case GL_STENCIL_WRITEMASK:
        params[0] = (GLint)stencil_write_mask;
        break;
    case GL_STENCIL_FAIL:
        params[0] = (GLint)stencil_fail;
        break;
    case GL_STENCIL_PASS_DEPTH_FAIL:
        params[0] = (GLint)stencil_zfail;
        break;
    case GL_STENCIL_PASS_DEPTH_PASS:
        params[0] = (GLint)stencil_zpass;
        break;
    case GL_STENCIL_BITS:
        params[0] = 8;
        break;
    case GL_DEPTH_BITS:
        params[0] = 24;
        break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
        params[0] = 8;
        break;
    case GL_SUBPIXEL_BITS:
        params[0] = 4;
        break;
    case GL_AUX_BUFFERS:
        params[0] = 0;
        break;
    case GL_DRAW_BUFFER:
        params[0] = (GLint)draw_buffer_mode;
        break;
    case GL_READ_BUFFER:
        params[0] = (GLint)read_buffer_mode;
        break;
    case GL_DOUBLEBUFFER:
    case GL_RGBA_MODE:
        params[0] = GL_TRUE;
        break;
    case GL_STEREO:
        params[0] = GL_FALSE;
        break;
    case GL_INDEX_MODE:
        params[0] = GL_FALSE;
        break;
    case GL_ALPHA_TEST_FUNC:
        params[0] = (GLint)alpha_test_func;
        break;
    case GL_COLOR_WRITEMASK:
        params[0] = color_write_mask[0];
        params[1] = color_write_mask[1];
        params[2] = color_write_mask[2];
        params[3] = color_write_mask[3];
        break;
    case GL_LOGIC_OP_MODE:
        params[0] = (GLint)logic_op_mode;
        break;
    case GL_SHADE_MODEL:
        params[0] = (GLint)shade_model;
        break;
    case GL_FOG_MODE:
        params[0] = (GLint)fog_mode;
        break;
    case GL_COLOR_MATERIAL_FACE:
        params[0] = (GLint)color_material_face;
        break;
    case GL_COLOR_MATERIAL_PARAMETER:
        params[0] = (GLint)color_material_parameter;
        break;
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
        params[0] = light_model_local_viewer ? GL_TRUE : GL_FALSE;
        break;
    case GL_LIGHT_MODEL_TWO_SIDE:
        params[0] = light_model_two_side ? GL_TRUE : GL_FALSE;
        break;
    case GL_LIGHT_MODEL_COLOR_CONTROL:
        params[0] = (GLint)light_model_color_control;
        break;
    case GL_DEPTH_FUNC:
        params[0] = (GLint)depth_func;
        break;
    case GL_DEPTH_RANGE:
        params[0] = (GLint)depth_range_near;
        params[1] = (GLint)depth_range_far;
        break;
    case GL_BLEND_SRC:
        params[0] = (GLint)blend_sfactor;
        break;
    case GL_BLEND_DST:
        params[0] = (GLint)blend_dfactor;
        break;
    case GL_DEPTH_WRITEMASK:
        params[0] = depth_write_enabled ? GL_TRUE : GL_FALSE;
        break;
    case GL_CURRENT_INDEX:
        params[0] = (GLint)current_index;
        break;
    case GL_INDEX_CLEAR_VALUE:
        params[0] = (GLint)clear_index_value;
        break;
    case GL_CURRENT_RASTER_INDEX:
    case GL_CURRENT_RASTER_DISTANCE:
        params[0] = 0;
        break;
    case GL_MAX_LIGHTS:
        params[0] = 8;
        break;
    case GL_MAX_CLIP_PLANES:
        params[0] = NXGL_MAX_CLIP_PLANES;
        break;
    case GL_MAX_EVAL_ORDER:
        params[0] = NXGL_MAX_EVAL_ORDER;
        break;
    case GL_MAX_LIST_NESTING:
        params[0] = 64;
        break;
    case GL_MAX_PIXEL_MAP_TABLE:
        params[0] = NXGL_PIXEL_MAP_MAX;
        break;
    case GL_MAX_ATTRIB_STACK_DEPTH:
        params[0] = NXGL_ATTRIB_STACK_MAX;
        break;
    case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
        params[0] = NXGL_CLIENT_ATTRIB_STACK_MAX;
        break;
    case GL_MAX_NAME_STACK_DEPTH:
        params[0] = (GLint)(sizeof(name_stack) / sizeof(name_stack[0]));
        break;
    case GL_ATTRIB_STACK_DEPTH:
        params[0] = attrib_stack_top;
        break;
    case GL_CLIENT_ATTRIB_STACK_DEPTH:
        params[0] = client_attrib_stack_top;
        break;
    case GL_MAX_TEXTURE_SIZE:
        params[0] = 4096;
        break;
    case GL_MAX_MODELVIEW_STACK_DEPTH:
        params[0] = 32;
        break;
    case GL_MAX_PROJECTION_STACK_DEPTH:
        params[0] = 8;
        break;
    case GL_MAX_TEXTURE_STACK_DEPTH:
        params[0] = 8;
        break;
    case GL_MAX_TEXTURE_UNITS:
        params[0] = 4;
        break;
    case GL_MAX_3D_TEXTURE_SIZE:
        params[0] = 256;
        break;
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_ELEMENTS_INDICES:
        params[0] = 1024;
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        params[0] = 4;
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        params[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        params[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        params[2] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        params[3] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    case GL_ACTIVE_TEXTURE:
        params[0] = (GLint)active_texture;
        break;
    case GL_CLIENT_ACTIVE_TEXTURE:
        params[0] = (GLint)client_active_texture;
        break;
    case GL_LIST_BASE:
        params[0] = (GLint)list_base;
        break;
    case GL_RENDER_MODE:
        params[0] = (GLint)render_mode;
        break;
    case GL_NAME_STACK_DEPTH:
        params[0] = name_stack_depth;
        break;
    case GL_SELECTION_BUFFER_SIZE:
        params[0] = selection_buffer_size;
        break;
    case GL_FEEDBACK_BUFFER_SIZE:
        params[0] = feedback_buffer_size;
        break;
    case GL_TEXTURE_BINDING_2D:
        params[0] = unit >= 0 ? (GLint)texture_binding_2d[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_3D:
        params[0] = unit >= 0 ? (GLint)texture_binding_3d[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        params[0] = unit >= 0 ? (GLint)texture_binding_cube_map[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_1D:
        params[0] = unit >= 0 ? (GLint)texture_binding_1d[unit] : 0;
        break;
    case GL_PACK_ALIGNMENT:
        params[0] = pack_alignment;
        break;
    case GL_PACK_ROW_LENGTH:
        params[0] = pack_row_length;
        break;
    case GL_PACK_SKIP_ROWS:
        params[0] = pack_skip_rows;
        break;
    case GL_PACK_SKIP_PIXELS:
        params[0] = pack_skip_pixels;
        break;
    case GL_PACK_IMAGE_HEIGHT:
        params[0] = pack_image_height;
        break;
    case GL_PACK_SKIP_IMAGES:
        params[0] = pack_skip_images;
        break;
    case GL_PACK_SWAP_BYTES:
        params[0] = pack_swap_bytes;
        break;
    case GL_PACK_LSB_FIRST:
        params[0] = pack_lsb_first;
        break;
    case GL_UNPACK_ALIGNMENT:
        params[0] = unpack_alignment;
        break;
    case GL_UNPACK_ROW_LENGTH:
        params[0] = unpack_row_length;
        break;
    case GL_UNPACK_SKIP_ROWS:
        params[0] = unpack_skip_rows;
        break;
    case GL_UNPACK_SKIP_PIXELS:
        params[0] = unpack_skip_pixels;
        break;
    case GL_UNPACK_IMAGE_HEIGHT:
        params[0] = unpack_image_height;
        break;
    case GL_UNPACK_SKIP_IMAGES:
        params[0] = unpack_skip_images;
        break;
    case GL_UNPACK_SWAP_BYTES:
        params[0] = unpack_swap_bytes;
        break;
    case GL_UNPACK_LSB_FIRST:
        params[0] = unpack_lsb_first;
        break;
    case GL_INDEX_BITS:
    case GL_ACCUM_RED_BITS:
    case GL_ACCUM_GREEN_BITS:
    case GL_ACCUM_BLUE_BITS:
    case GL_ACCUM_ALPHA_BITS:
        params[0] = 0;
        break;
    case GL_VERTEX_ARRAY_SIZE:
        params[0] = vertex_array.size;
        break;
    case GL_VERTEX_ARRAY_TYPE:
        params[0] = (GLint)vertex_array.type;
        break;
    case GL_VERTEX_ARRAY_STRIDE:
        params[0] = vertex_array.stride;
        break;
    case GL_COLOR_ARRAY_SIZE:
        params[0] = color_array.size;
        break;
    case GL_COLOR_ARRAY_TYPE:
        params[0] = (GLint)color_array.type;
        break;
    case GL_COLOR_ARRAY_STRIDE:
        params[0] = color_array.stride;
        break;
    case GL_TEXTURE_COORD_ARRAY_SIZE:
        params[0] = client_unit >= 0 ? texcoord_array[client_unit].size : 0;
        break;
    case GL_TEXTURE_COORD_ARRAY_TYPE:
        params[0] = client_unit >= 0 ? (GLint)texcoord_array[client_unit].type : 0;
        break;
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
        params[0] = client_unit >= 0 ? texcoord_array[client_unit].stride : 0;
        break;
    case GL_NORMAL_ARRAY_TYPE:
        params[0] = (GLint)normal_array.type;
        break;
    case GL_NORMAL_ARRAY_STRIDE:
        params[0] = normal_array.stride;
        break;
    case GL_BLEND:
    case GL_LINE_STIPPLE:
    case GL_POLYGON_STIPPLE:
    case GL_DEPTH_TEST:
    case GL_CULL_FACE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
    case GL_ALPHA_TEST:
    case GL_COLOR_LOGIC_OP:
    case GL_INDEX_LOGIC_OP:
    case GL_POLYGON_OFFSET_POINT:
    case GL_POLYGON_OFFSET_LINE:
    case GL_POLYGON_OFFSET_FILL:
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_LIGHTING:
    case GL_FOG:
    case GL_COLOR_MATERIAL:
    case GL_NORMALIZE:
    case GL_RESCALE_NORMAL:
    case GL_CURRENT_RASTER_POSITION_VALID:
    case GL_EDGE_FLAG:
    case GL_VERTEX_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_AUTO_NORMAL:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_INDEX:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_INDEX:
    case GL_MULTISAMPLE:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_ALPHA_TO_ONE:
    case GL_SAMPLE_COVERAGE:
        params[0] = glIsEnabled(pname);
        break;
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
        params[0] = 0;
        break;
    case GL_SAMPLE_COVERAGE_VALUE:
        params[0] = sample_coverage_value >= 0.5f ? 1 : 0;
        break;
    case GL_SAMPLE_COVERAGE_INVERT:
        params[0] = sample_coverage_invert ? GL_TRUE : GL_FALSE;
        break;
    case GL_CURRENT_COLOR:
        params[0] = (GLint)current_color.r;
        params[1] = (GLint)current_color.g;
        params[2] = (GLint)current_color.b;
        params[3] = (GLint)current_color.a;
        break;
    case GL_CURRENT_NORMAL:
        params[0] = (GLint)current_normal.x;
        params[1] = (GLint)current_normal.y;
        params[2] = (GLint)current_normal.z;
        break;
    case GL_CURRENT_TEXTURE_COORDS:
        params[0] = unit >= 0 ? (GLint)current_u[unit] : 0;
        params[1] = unit >= 0 ? (GLint)current_v[unit] : 0;
        params[2] = unit >= 0 ? (GLint)current_r[unit] : 0;
        params[3] = 1;
        break;
    case GL_CLEAR_COLOR:
        params[0] = (GLint)clear_color_value[0];
        params[1] = (GLint)clear_color_value[1];
        params[2] = (GLint)clear_color_value[2];
        params[3] = (GLint)clear_color_value[3];
        break;
    case GL_CURRENT_RASTER_POSITION:
        params[0] = (GLint)current_raster_position[0];
        params[1] = (GLint)current_raster_position[1];
        params[2] = (GLint)current_raster_position[2];
        params[3] = (GLint)current_raster_position[3];
        break;
    case GL_CURRENT_RASTER_COLOR:
        params[0] = (GLint)current_color.r;
        params[1] = (GLint)current_color.g;
        params[2] = (GLint)current_color.b;
        params[3] = (GLint)current_color.a;
        break;
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
        params[0] = (GLint)current_u[0];
        params[1] = (GLint)current_v[0];
        params[2] = (GLint)current_r[0];
        params[3] = 1;
        break;
    case GL_ACCUM_CLEAR_VALUE:
        params[0] = (GLint)accum_clear_value[0];
        params[1] = (GLint)accum_clear_value[1];
        params[2] = (GLint)accum_clear_value[2];
        params[3] = (GLint)accum_clear_value[3];
        break;
    case GL_DEPTH_CLEAR_VALUE:
        params[0] = (GLint)depth_clear_value;
        break;
    case GL_LIGHT_MODEL_AMBIENT:
        params[0] = (GLint)light_model_ambient.r;
        params[1] = (GLint)light_model_ambient.g;
        params[2] = (GLint)light_model_ambient.b;
        params[3] = (GLint)light_model_ambient.a;
        break;
    case GL_FOG_COLOR:
        params[0] = (GLint)fog_color.r;
        params[1] = (GLint)fog_color.g;
        params[2] = (GLint)fog_color.b;
        params[3] = (GLint)fog_color.a;
        break;
    case GL_FOG_DENSITY:
        params[0] = (GLint)fog_density;
        break;
    case GL_FOG_START:
        params[0] = (GLint)fog_start;
        break;
    case GL_FOG_END:
        params[0] = (GLint)fog_end;
        break;
    case GL_ALPHA_TEST_REF:
        params[0] = (GLint)alpha_test_ref;
        break;
    case GL_ZOOM_X:
        params[0] = (GLint)pixel_zoom_x;
        break;
    case GL_ZOOM_Y:
        params[0] = (GLint)pixel_zoom_y;
        break;
    case GL_RED_SCALE:
        params[0] = (GLint)pixel_transfer_scale[0];
        break;
    case GL_GREEN_SCALE:
        params[0] = (GLint)pixel_transfer_scale[1];
        break;
    case GL_BLUE_SCALE:
        params[0] = (GLint)pixel_transfer_scale[2];
        break;
    case GL_ALPHA_SCALE:
        params[0] = (GLint)pixel_transfer_scale[3];
        break;
    case GL_RED_BIAS:
        params[0] = (GLint)pixel_transfer_bias[0];
        break;
    case GL_GREEN_BIAS:
        params[0] = (GLint)pixel_transfer_bias[1];
        break;
    case GL_BLUE_BIAS:
        params[0] = (GLint)pixel_transfer_bias[2];
        break;
    case GL_ALPHA_BIAS:
        params[0] = (GLint)pixel_transfer_bias[3];
        break;
    case GL_POINT_SIZE:
        params[0] = (GLint)point_size;
        break;
    case GL_LINE_WIDTH:
        params[0] = (GLint)line_width;
        break;
    case GL_POLYGON_OFFSET_FACTOR:
        params[0] = (GLint)polygon_offset_factor;
        break;
    case GL_POLYGON_OFFSET_UNITS:
        params[0] = (GLint)polygon_offset_units;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
        break;
    }
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
    case GL_CURRENT_COLOR:
        params[0] = current_color.r;
        params[1] = current_color.g;
        params[2] = current_color.b;
        params[3] = current_color.a;
        break;
    case GL_CURRENT_TEXTURE_COORDS: {
        int unit = active_texture_index();
        if (unit < 0) {
            set_error(GL_INVALID_ENUM);
            params[0] = 0.0f;
            break;
        }
        params[0] = current_u[unit];
        params[1] = current_v[unit];
        params[2] = current_r[unit];
        params[3] = 1.0f;
        break;
    }
    case GL_CLEAR_COLOR:
        memcpy(params, clear_color_value, sizeof(clear_color_value));
        break;
    case GL_CURRENT_NORMAL:
        params[0] = current_normal.x;
        params[1] = current_normal.y;
        params[2] = current_normal.z;
        break;
    case GL_CURRENT_RASTER_POSITION:
        memcpy(params, current_raster_position, sizeof(current_raster_position));
        break;
    case GL_CURRENT_RASTER_COLOR:
        params[0] = current_color.r;
        params[1] = current_color.g;
        params[2] = current_color.b;
        params[3] = current_color.a;
        break;
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
        params[0] = current_u[0];
        params[1] = current_v[0];
        params[2] = current_r[0];
        params[3] = 1.0f;
        break;
    case GL_CURRENT_INDEX:
        params[0] = current_index;
        break;
    case GL_INDEX_CLEAR_VALUE:
        params[0] = clear_index_value;
        break;
    case GL_CURRENT_RASTER_INDEX:
    case GL_CURRENT_RASTER_DISTANCE:
        params[0] = 0.0f;
        break;
    case GL_ACCUM_CLEAR_VALUE:
        memcpy(params, accum_clear_value, sizeof(accum_clear_value));
        break;
    case GL_DEPTH_CLEAR_VALUE:
        params[0] = depth_clear_value;
        break;
    case GL_DEPTH_RANGE:
        params[0] = depth_range_near;
        params[1] = depth_range_far;
        break;
    case GL_LIGHT_MODEL_AMBIENT:
        params[0] = light_model_ambient.r;
        params[1] = light_model_ambient.g;
        params[2] = light_model_ambient.b;
        params[3] = light_model_ambient.a;
        break;
    case GL_FOG_COLOR:
        params[0] = fog_color.r;
        params[1] = fog_color.g;
        params[2] = fog_color.b;
        params[3] = fog_color.a;
        break;
    case GL_FOG_DENSITY:
        params[0] = fog_density;
        break;
    case GL_FOG_START:
        params[0] = fog_start;
        break;
    case GL_FOG_END:
        params[0] = fog_end;
        break;
    case GL_ALPHA_TEST_REF:
        params[0] = alpha_test_ref;
        break;
    case GL_SAMPLE_COVERAGE_VALUE:
        params[0] = sample_coverage_value;
        break;
    case GL_SAMPLE_COVERAGE_INVERT:
        params[0] = sample_coverage_invert ? 1.0f : 0.0f;
        break;
    case GL_ZOOM_X:
        params[0] = pixel_zoom_x;
        break;
    case GL_ZOOM_Y:
        params[0] = pixel_zoom_y;
        break;
    case GL_RED_SCALE:
        params[0] = pixel_transfer_scale[0];
        break;
    case GL_GREEN_SCALE:
        params[0] = pixel_transfer_scale[1];
        break;
    case GL_BLUE_SCALE:
        params[0] = pixel_transfer_scale[2];
        break;
    case GL_ALPHA_SCALE:
        params[0] = pixel_transfer_scale[3];
        break;
    case GL_RED_BIAS:
        params[0] = pixel_transfer_bias[0];
        break;
    case GL_GREEN_BIAS:
        params[0] = pixel_transfer_bias[1];
        break;
    case GL_BLUE_BIAS:
        params[0] = pixel_transfer_bias[2];
        break;
    case GL_ALPHA_BIAS:
        params[0] = pixel_transfer_bias[3];
        break;
    case GL_POINT_SIZE:
        params[0] = point_size;
        break;
    case GL_POINT_SIZE_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
        params[0] = 1.0f;
        params[1] = 64.0f;
        break;
    case GL_POINT_SIZE_GRANULARITY:
        params[0] = 1.0f;
        break;
    case GL_LINE_WIDTH:
        params[0] = line_width;
        break;
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = 1.0f;
        params[1] = 64.0f;
        break;
    case GL_LINE_WIDTH_GRANULARITY:
        params[0] = 1.0f;
        break;
    case GL_SCISSOR_BOX:
        params[0] = (GLfloat)scissor_box[0];
        params[1] = (GLfloat)scissor_box[1];
        params[2] = (GLfloat)scissor_box[2];
        params[3] = (GLfloat)scissor_box[3];
        break;
    case GL_VIEWPORT:
        params[0] = (GLfloat)viewport[0];
        params[1] = (GLfloat)viewport[1];
        params[2] = (GLfloat)viewport[2];
        params[3] = (GLfloat)viewport[3];
        break;
    case GL_POLYGON_OFFSET_FACTOR:
        params[0] = polygon_offset_factor;
        break;
    case GL_POLYGON_OFFSET_UNITS:
        params[0] = polygon_offset_units;
        break;
    case GL_MODELVIEW_MATRIX:
        memcpy(params, modelview, sizeof(Matrix));
        break;
    case GL_PROJECTION_MATRIX:
        memcpy(params, projection, sizeof(Matrix));
        break;
    case GL_TEXTURE_MATRIX:
        memcpy(params, texture_matrix, sizeof(Matrix));
        break;
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
        matrix_transpose(params, modelview);
        break;
    case GL_TRANSPOSE_PROJECTION_MATRIX:
        matrix_transpose(params, projection);
        break;
    case GL_TRANSPOSE_TEXTURE_MATRIX:
        matrix_transpose(params, texture_matrix);
        break;
    case GL_TRANSPOSE_COLOR_MATRIX:
        matrix_identity(params);
        break;
    default: {
        GLint ints[16];
        GLenum before = last_error;
        int count;

        last_error = GL_NO_ERROR;
        glGetIntegerv(pname, ints);
        if (last_error == GL_NO_ERROR) {
            count = get_component_count(pname);
            for (int i = 0; i < count; ++i) {
                params[i] = (GLfloat)ints[i];
            }
            last_error = before;
            return;
        }

        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        break;
    }
    }
}

void glGetDoublev(GLenum pname, GLdouble *params)
{
    GLfloat floats[16];
    GLint ints[16];
    GLenum before;
    int count;

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    before = last_error;
    last_error = GL_NO_ERROR;
    glGetFloatv(pname, floats);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = (GLdouble)floats[i];
        }
        last_error = before;
        return;
    }

    last_error = GL_NO_ERROR;
    glGetIntegerv(pname, ints);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = (GLdouble)ints[i];
        }
        last_error = before;
        return;
    }

    last_error = before;
    set_error(GL_INVALID_ENUM);
    params[0] = 0.0;
}

void glGetBooleanv(GLenum pname, GLboolean *params)
{
    GLdouble values[16];
    GLenum before;
    int count;

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    before = last_error;
    last_error = GL_NO_ERROR;
    glGetDoublev(pname, values);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = values[i] != 0.0 ? GL_TRUE : GL_FALSE;
        }
        last_error = before;
        return;
    }

    last_error = before;
    set_error(GL_INVALID_ENUM);
    params[0] = GL_FALSE;
}

void glGetPointerv(GLenum pname, GLvoid **params)
{
    int client_unit = client_texture_index();

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
    case GL_VERTEX_ARRAY_POINTER:
        *params = (GLvoid *)vertex_array.pointer;
        break;
    case GL_COLOR_ARRAY_POINTER:
        *params = (GLvoid *)color_array.pointer;
        break;
    case GL_NORMAL_ARRAY_POINTER:
        *params = (GLvoid *)normal_array.pointer;
        break;
    case GL_TEXTURE_COORD_ARRAY_POINTER:
        *params = client_unit >= 0 ? (GLvoid *)texcoord_array[client_unit].pointer : NULL;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        *params = NULL;
        break;
    }
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    ListCommand command = { LIST_CMD_VIEWPORT };

    if (reject_inside_begin()) {
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    record_command(command);
    if (compile_only()) {
        return;
    }

    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = width;
    viewport[3] = height;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    ListCommand command = { LIST_CMD_SCISSOR };

    if (reject_inside_begin()) {
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    record_command(command);
    if (compile_only()) {
        return;
    }

    scissor_box[0] = x;
    scissor_box[1] = y;
    scissor_box[2] = width;
    scissor_box[3] = height;
    sync_native_state();
}

void glClearColor(float r, float g, float b, float a)
{
    ListCommand command = { LIST_CMD_CLEAR_COLOR };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = r;
    command.f[1] = g;
    command.f[2] = b;
    command.f[3] = a;
    record_command(command);
    if (compile_only()) {
        return;
    }

    clear_color_value[0] = clamp01(r);
    clear_color_value[1] = clamp01(g);
    clear_color_value[2] = clamp01(b);
    clear_color_value[3] = clamp01(a);
    clear_color = ((uint32_t)channel(a) << 24) |
                  ((uint32_t)channel(r) << 16) |
                  ((uint32_t)channel(g) << 8) |
                  (uint32_t)channel(b);
}

void glClearIndex(GLfloat c)
{
    ListCommand command = { LIST_CMD_CLEAR_INDEX };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = c;
    record_command(command);
    if (compile_only()) {
        return;
    }

    clear_index_value = c;
}

void glClearDepth(GLclampf depth)
{
    ListCommand command = { LIST_CMD_CLEAR_DEPTH };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = depth;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_clear_value = clamp01(depth);
}

void glDepthRange(GLclampf z_near, GLclampf z_far)
{
    ListCommand command = { LIST_CMD_DEPTH_RANGE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = z_near;
    command.f[1] = z_far;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_range_near = clamp01(z_near);
    depth_range_far = clamp01(z_far);
}

void glClearStencil(GLint s)
{
    ListCommand command = { LIST_CMD_CLEAR_STENCIL };
    if (reject_inside_begin()) {
        return;
    }
    command.u = (GLuint)s;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_clear_value = s;
}

void glClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    ListCommand command = { LIST_CMD_CLEAR_ACCUM };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = r;
    command.f[1] = g;
    command.f[2] = b;
    command.f[3] = a;
    record_command(command);
    if (compile_only()) {
        return;
    }
    accum_clear_value[0] = clamp_accum_clear(r);
    accum_clear_value[1] = clamp_accum_clear(g);
    accum_clear_value[2] = clamp_accum_clear(b);
    accum_clear_value[3] = clamp_accum_clear(a);
}

void glClear(uint32_t mask)
{
    ListCommand command = { LIST_CMD_CLEAR };
    const GLbitfield valid_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((mask & ~valid_mask) != 0u) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }

    ++perf_counters.clears;
    ensure_native_frame_started();
    nxgl_backend_flush();
    {
        int min_x, min_y, max_x, max_y;
        if (shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
            int width = max_x - min_x + 1;
            int height = max_y - min_y + 1;
            if ((mask & GL_COLOR_BUFFER_BIT) != 0 && draw_buffer_mode != GL_NONE) {
                nxgl_backend_clear_color(clear_color,
                               color_write_mask[0] == GL_TRUE,
                               color_write_mask[1] == GL_TRUE,
                               color_write_mask[2] == GL_TRUE,
                               color_write_mask[3] == GL_TRUE,
                               min_x, min_y, width, height);
            }
            if (((mask & GL_DEPTH_BUFFER_BIT) != 0 && depth_write_enabled) ||
                ((mask & GL_STENCIL_BUFFER_BIT) != 0 && (stencil_write_mask & 0xffu) == 0xffu)) {
                nxgl_backend_clear_depth_stencil((mask & GL_DEPTH_BUFFER_BIT) != 0 && depth_write_enabled,
                                       depth_clear_value,
                                       (mask & GL_STENCIL_BUFFER_BIT) != 0 && (stencil_write_mask & 0xffu) == 0xffu,
                                       (uint8_t)(stencil_clear_value & 0xff),
                                       min_x, min_y, width, height);
            }
        }
    }
    if ((mask & GL_COLOR_BUFFER_BIT) != 0 && draw_buffer_mode != GL_NONE) {
        shadow_clear(clear_color);
    }
    if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
        shadow_clear_depth(depth_clear_value);
    }
    if ((mask & GL_STENCIL_BUFFER_BIT) != 0) {
        shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    }
    if ((mask & GL_ACCUM_BUFFER_BIT) != 0 && ensure_accum_buffer()) {
        int min_x, min_y, max_x, max_y;
        if (shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    size_t index = ((size_t)y * (size_t)shadow_width + (size_t)x) * 4u;
                    accum_buffer[index + 0u] = accum_clear_value[0];
                    accum_buffer[index + 1u] = accum_clear_value[1];
                    accum_buffer[index + 2u] = accum_clear_value[2];
                    accum_buffer[index + 3u] = accum_clear_value[3];
                }
            }
        }
    }
    sync_native_state();
}

void glAccum(GLenum op, GLfloat value)
{
    ListCommand command = { LIST_CMD_ACCUM };
    size_t count;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (op != GL_ACCUM && op != GL_LOAD && op != GL_RETURN && op != GL_MULT && op != GL_ADD) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (shadow_color_buffer == NULL || !ensure_accum_buffer()) {
        return;
    }

    command.a = op;
    command.f[0] = value;
    record_command(command);
    if (compile_only()) {
        return;
    }

    nxgl_backend_flush();
    count = (size_t)shadow_width * (size_t)shadow_height;
    if (op == GL_ACCUM || op == GL_LOAD) {
        for (size_t i = 0; i < count; ++i) {
            uint32_t pixel = shadow_color_buffer[i];
            float r = (float)((pixel >> 16) & 0xff) / 255.0f;
            float g = (float)((pixel >> 8) & 0xff) / 255.0f;
            float b = (float)(pixel & 0xff) / 255.0f;
            float *dst = accum_buffer + i * 4u;

            if (op == GL_LOAD) {
                dst[0] = r * value;
                dst[1] = g * value;
                dst[2] = b * value;
                dst[3] = value;
            } else {
                dst[0] += r * value;
                dst[1] += g * value;
                dst[2] += b * value;
                dst[3] += value;
            }
        }
    } else if (op == GL_ADD) {
        for (size_t i = 0; i < count * 4u; ++i) {
            accum_buffer[i] += value;
        }
    } else if (op == GL_MULT) {
        for (size_t i = 0; i < count * 4u; ++i) {
            accum_buffer[i] *= value;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            const float *src = accum_buffer + i * 4u;
            uint8_t r = channel(clamp01(src[0] * value));
            uint8_t g = channel(clamp01(src[1] * value));
            uint8_t b = channel(clamp01(src[2] * value));
            shadow_color_buffer[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}


static void set_error(GLenum error)
{
    if (last_error == GL_NO_ERROR) {
        last_error = error;
    }
}

static void capture_attrib_snapshot(AttribSnapshot *snapshot, GLbitfield mask)
{
    snapshot->mask = mask & GL_ALL_ATTRIB_BITS;
    snapshot->current_color = current_color;
    snapshot->current_normal = current_normal;
    memcpy(snapshot->current_u, current_u, sizeof(current_u));
    memcpy(snapshot->current_v, current_v, sizeof(current_v));
    memcpy(snapshot->current_r, current_r, sizeof(current_r));
    snapshot->current_edge_flag = current_edge_flag;
    memcpy(snapshot->current_raster_position, current_raster_position, sizeof(current_raster_position));
    snapshot->current_raster_position_valid = current_raster_position_valid;
    memcpy(snapshot->clip_planes, clip_planes, sizeof(clip_planes));
    snapshot->point_size = point_size;
    snapshot->line_width = line_width;
    snapshot->line_stipple_factor = line_stipple_factor;
    snapshot->line_stipple_pattern = line_stipple_pattern;
    snapshot->line_stipple_enabled = line_stipple_enabled;
    snapshot->polygon_mode = polygon_mode;
    memcpy(snapshot->polygon_stipple_pattern, polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
    snapshot->polygon_stipple_enabled = polygon_stipple_enabled;
    snapshot->polygon_offset_point_enabled = polygon_offset_point_enabled;
    snapshot->polygon_offset_line_enabled = polygon_offset_line_enabled;
    snapshot->polygon_offset_fill_enabled = polygon_offset_fill_enabled;
    snapshot->polygon_offset_factor = polygon_offset_factor;
    snapshot->polygon_offset_units = polygon_offset_units;
    snapshot->cull_enabled = cull_enabled;
    snapshot->cull_face_mode = cull_face_mode;
    snapshot->front_face_mode = front_face_mode;
    snapshot->pixel_zoom_x = pixel_zoom_x;
    snapshot->pixel_zoom_y = pixel_zoom_y;
    memcpy(snapshot->pixel_transfer_scale, pixel_transfer_scale, sizeof(pixel_transfer_scale));
    memcpy(snapshot->pixel_transfer_bias, pixel_transfer_bias, sizeof(pixel_transfer_bias));
    memcpy(snapshot->pixel_maps, pixel_maps, sizeof(pixel_maps));
    snapshot->lighting_enabled = lighting_enabled;
    memcpy(snapshot->lights, lights, sizeof(lights));
    snapshot->shade_model = shade_model;
    snapshot->light_model_local_viewer = light_model_local_viewer;
    snapshot->light_model_two_side = light_model_two_side;
    snapshot->light_model_color_control = light_model_color_control;
    snapshot->light_model_ambient = light_model_ambient;
    snapshot->color_material_enabled = color_material_enabled;
    snapshot->color_material_face = color_material_face;
    snapshot->color_material_parameter = color_material_parameter;
    snapshot->material_state = material_state;
    snapshot->material_back_state = material_back_state;
    snapshot->fog_enabled = fog_enabled;
    snapshot->fog_mode = fog_mode;
    snapshot->fog_hint = fog_hint;
    snapshot->fog_color = fog_color;
    snapshot->fog_density = fog_density;
    snapshot->fog_start = fog_start;
    snapshot->fog_end = fog_end;
    snapshot->depth_test_enabled = depth_test_enabled;
    snapshot->depth_write_enabled = depth_write_enabled;
    snapshot->depth_clear_value = depth_clear_value;
    snapshot->depth_range_near = depth_range_near;
    snapshot->depth_range_far = depth_range_far;
    snapshot->depth_func = depth_func;
    memcpy(snapshot->accum_clear_value, accum_clear_value, sizeof(accum_clear_value));
    snapshot->stencil_test_enabled = stencil_test_enabled;
    snapshot->stencil_clear_value = stencil_clear_value;
    snapshot->stencil_func = stencil_func;
    snapshot->stencil_ref = stencil_ref;
    snapshot->stencil_value_mask = stencil_value_mask;
    snapshot->stencil_write_mask = stencil_write_mask;
    snapshot->stencil_fail = stencil_fail;
    snapshot->stencil_zfail = stencil_zfail;
    snapshot->stencil_zpass = stencil_zpass;
    memcpy(snapshot->viewport, viewport, sizeof(viewport));
    memcpy(snapshot->scissor_box, scissor_box, sizeof(scissor_box));
    snapshot->scissor_test_enabled = scissor_test_enabled;
    snapshot->matrix_mode = matrix_mode;
    snapshot->normalize_enabled = normalize_enabled;
    snapshot->rescale_normal_enabled = rescale_normal_enabled;
    snapshot->blend_enabled = blend_enabled;
    snapshot->blend_sfactor = blend_sfactor;
    snapshot->blend_dfactor = blend_dfactor;
    snapshot->alpha_test_enabled = alpha_test_enabled;
    snapshot->alpha_test_func = alpha_test_func;
    snapshot->alpha_test_ref = alpha_test_ref;
    memcpy(snapshot->color_write_mask, color_write_mask, sizeof(color_write_mask));
    snapshot->color_logic_op_enabled = color_logic_op_enabled;
    snapshot->logic_op_mode = logic_op_mode;
    snapshot->multisample_enabled = multisample_enabled;
    snapshot->sample_alpha_to_coverage_enabled = sample_alpha_to_coverage_enabled;
    snapshot->sample_alpha_to_one_enabled = sample_alpha_to_one_enabled;
    snapshot->sample_coverage_enabled = sample_coverage_enabled;
    snapshot->sample_coverage_value = sample_coverage_value;
    snapshot->sample_coverage_invert = sample_coverage_invert;
    snapshot->draw_buffer_mode = draw_buffer_mode;
    snapshot->read_buffer_mode = read_buffer_mode;
    snapshot->perspective_correction_hint = perspective_correction_hint;
    snapshot->point_smooth_hint = point_smooth_hint;
    snapshot->line_smooth_hint = line_smooth_hint;
    snapshot->polygon_smooth_hint = polygon_smooth_hint;
    memcpy(snapshot->eval_maps1, eval_maps1, sizeof(eval_maps1));
    memcpy(snapshot->eval_maps2, eval_maps2, sizeof(eval_maps2));
    snapshot->auto_normal_enabled = auto_normal_enabled;
    snapshot->map_grid1_n = map_grid1_n;
    snapshot->map_grid1_u1 = map_grid1_u1;
    snapshot->map_grid1_u2 = map_grid1_u2;
    snapshot->map_grid2_un = map_grid2_un;
    snapshot->map_grid2_vn = map_grid2_vn;
    snapshot->map_grid2_u1 = map_grid2_u1;
    snapshot->map_grid2_u2 = map_grid2_u2;
    snapshot->map_grid2_v1 = map_grid2_v1;
    snapshot->map_grid2_v2 = map_grid2_v2;
    snapshot->list_base = list_base;
    snapshot->active_texture = active_texture;
    memcpy(snapshot->texture_1d_enabled, texture_1d_enabled, sizeof(texture_1d_enabled));
    memcpy(snapshot->texture_2d_enabled, texture_2d_enabled, sizeof(texture_2d_enabled));
    memcpy(snapshot->texture_3d_enabled, texture_3d_enabled, sizeof(texture_3d_enabled));
    memcpy(snapshot->texture_cube_map_enabled, texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
    memcpy(snapshot->texture_binding_1d, texture_binding_1d, sizeof(texture_binding_1d));
    memcpy(snapshot->texture_binding_2d, texture_binding_2d, sizeof(texture_binding_2d));
    memcpy(snapshot->texture_binding_3d, texture_binding_3d, sizeof(texture_binding_3d));
    memcpy(snapshot->texture_binding_cube_map, texture_binding_cube_map, sizeof(texture_binding_cube_map));
    memcpy(snapshot->texgen_state, texgen_state, sizeof(texgen_state));
    memcpy(snapshot->texture_env_mode, texture_env_mode, sizeof(texture_env_mode));
    memcpy(snapshot->texture_env_color, texture_env_color, sizeof(texture_env_color));
    memcpy(snapshot->texture_combine_rgb, texture_combine_rgb, sizeof(texture_combine_rgb));
    memcpy(snapshot->texture_combine_alpha, texture_combine_alpha, sizeof(texture_combine_alpha));
    memcpy(snapshot->texture_source_rgb, texture_source_rgb, sizeof(texture_source_rgb));
    memcpy(snapshot->texture_source_alpha, texture_source_alpha, sizeof(texture_source_alpha));
    memcpy(snapshot->texture_operand_rgb, texture_operand_rgb, sizeof(texture_operand_rgb));
    memcpy(snapshot->texture_operand_alpha, texture_operand_alpha, sizeof(texture_operand_alpha));
    memcpy(snapshot->texture_rgb_scale, texture_rgb_scale, sizeof(texture_rgb_scale));
    memcpy(snapshot->texture_alpha_scale, texture_alpha_scale, sizeof(texture_alpha_scale));
}

static void restore_attrib_snapshot(const AttribSnapshot *snapshot)
{
    GLbitfield mask = snapshot->mask;

    if ((mask & GL_CURRENT_BIT) != 0) {
        current_color = snapshot->current_color;
        current_normal = snapshot->current_normal;
        memcpy(current_u, snapshot->current_u, sizeof(current_u));
        memcpy(current_v, snapshot->current_v, sizeof(current_v));
        memcpy(current_r, snapshot->current_r, sizeof(current_r));
        current_edge_flag = snapshot->current_edge_flag;
        memcpy(current_raster_position, snapshot->current_raster_position, sizeof(current_raster_position));
        current_raster_position_valid = snapshot->current_raster_position_valid;
        apply_color_material(current_color);
    }
    if ((mask & GL_POINT_BIT) != 0) {
        point_size = snapshot->point_size;
    }
    if ((mask & GL_LINE_BIT) != 0) {
        line_width = snapshot->line_width;
        line_stipple_factor = snapshot->line_stipple_factor;
        line_stipple_pattern = snapshot->line_stipple_pattern;
        line_stipple_enabled = snapshot->line_stipple_enabled;
    }
    if ((mask & GL_POLYGON_BIT) != 0) {
        polygon_mode = snapshot->polygon_mode;
        polygon_offset_point_enabled = snapshot->polygon_offset_point_enabled;
        polygon_offset_line_enabled = snapshot->polygon_offset_line_enabled;
        polygon_offset_fill_enabled = snapshot->polygon_offset_fill_enabled;
        polygon_offset_factor = snapshot->polygon_offset_factor;
        polygon_offset_units = snapshot->polygon_offset_units;
        cull_enabled = snapshot->cull_enabled;
        cull_face_mode = snapshot->cull_face_mode;
        front_face_mode = snapshot->front_face_mode;
    }
    if ((mask & GL_POLYGON_STIPPLE_BIT) != 0) {
        memcpy(polygon_stipple_pattern, snapshot->polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
        polygon_stipple_enabled = snapshot->polygon_stipple_enabled;
    }
    if ((mask & GL_PIXEL_MODE_BIT) != 0) {
        pixel_zoom_x = snapshot->pixel_zoom_x;
        pixel_zoom_y = snapshot->pixel_zoom_y;
        memcpy(pixel_transfer_scale, snapshot->pixel_transfer_scale, sizeof(pixel_transfer_scale));
        memcpy(pixel_transfer_bias, snapshot->pixel_transfer_bias, sizeof(pixel_transfer_bias));
        memcpy(pixel_maps, snapshot->pixel_maps, sizeof(pixel_maps));
    }
    if ((mask & GL_LIGHTING_BIT) != 0) {
        lighting_enabled = snapshot->lighting_enabled;
        memcpy(lights, snapshot->lights, sizeof(lights));
        shade_model = snapshot->shade_model;
        light_model_local_viewer = snapshot->light_model_local_viewer;
        light_model_two_side = snapshot->light_model_two_side;
        light_model_color_control = snapshot->light_model_color_control;
        light_model_ambient = snapshot->light_model_ambient;
        color_material_enabled = snapshot->color_material_enabled;
        color_material_face = snapshot->color_material_face;
        color_material_parameter = snapshot->color_material_parameter;
        material_state = snapshot->material_state;
        material_back_state = snapshot->material_back_state;
    }
    if ((mask & GL_FOG_BIT) != 0) {
        fog_enabled = snapshot->fog_enabled;
        fog_mode = snapshot->fog_mode;
        fog_hint = snapshot->fog_hint;
        fog_color = snapshot->fog_color;
        fog_density = snapshot->fog_density;
        fog_start = snapshot->fog_start;
        fog_end = snapshot->fog_end;
    }
    if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
        depth_test_enabled = snapshot->depth_test_enabled;
        depth_write_enabled = snapshot->depth_write_enabled;
        depth_clear_value = snapshot->depth_clear_value;
        depth_range_near = snapshot->depth_range_near;
        depth_range_far = snapshot->depth_range_far;
        depth_func = snapshot->depth_func;
    }
    if ((mask & GL_ACCUM_BUFFER_BIT) != 0) {
        memcpy(accum_clear_value, snapshot->accum_clear_value, sizeof(accum_clear_value));
    }
    if ((mask & GL_STENCIL_BUFFER_BIT) != 0) {
        stencil_test_enabled = snapshot->stencil_test_enabled;
        stencil_clear_value = snapshot->stencil_clear_value;
        stencil_func = snapshot->stencil_func;
        stencil_ref = snapshot->stencil_ref;
        stencil_value_mask = snapshot->stencil_value_mask;
        stencil_write_mask = snapshot->stencil_write_mask;
        stencil_fail = snapshot->stencil_fail;
        stencil_zfail = snapshot->stencil_zfail;
        stencil_zpass = snapshot->stencil_zpass;
    }
    if ((mask & GL_VIEWPORT_BIT) != 0) {
        memcpy(viewport, snapshot->viewport, sizeof(viewport));
    }
    if ((mask & GL_SCISSOR_BIT) != 0) {
        memcpy(scissor_box, snapshot->scissor_box, sizeof(scissor_box));
        scissor_test_enabled = snapshot->scissor_test_enabled;
    }
    if ((mask & GL_TRANSFORM_BIT) != 0) {
        matrix_mode = snapshot->matrix_mode;
        normalize_enabled = snapshot->normalize_enabled;
        rescale_normal_enabled = snapshot->rescale_normal_enabled;
        memcpy(clip_planes, snapshot->clip_planes, sizeof(clip_planes));
    }
    if ((mask & GL_ENABLE_BIT) != 0) {
        blend_enabled = snapshot->blend_enabled;
        depth_test_enabled = snapshot->depth_test_enabled;
        cull_enabled = snapshot->cull_enabled;
        scissor_test_enabled = snapshot->scissor_test_enabled;
        stencil_test_enabled = snapshot->stencil_test_enabled;
        alpha_test_enabled = snapshot->alpha_test_enabled;
        color_logic_op_enabled = snapshot->color_logic_op_enabled;
        multisample_enabled = snapshot->multisample_enabled;
        sample_alpha_to_coverage_enabled = snapshot->sample_alpha_to_coverage_enabled;
        sample_alpha_to_one_enabled = snapshot->sample_alpha_to_one_enabled;
        sample_coverage_enabled = snapshot->sample_coverage_enabled;
        line_stipple_enabled = snapshot->line_stipple_enabled;
        polygon_stipple_enabled = snapshot->polygon_stipple_enabled;
        polygon_offset_point_enabled = snapshot->polygon_offset_point_enabled;
        polygon_offset_line_enabled = snapshot->polygon_offset_line_enabled;
        polygon_offset_fill_enabled = snapshot->polygon_offset_fill_enabled;
        lighting_enabled = snapshot->lighting_enabled;
        fog_enabled = snapshot->fog_enabled;
        color_material_enabled = snapshot->color_material_enabled;
        normalize_enabled = snapshot->normalize_enabled;
        rescale_normal_enabled = snapshot->rescale_normal_enabled;
        auto_normal_enabled = snapshot->auto_normal_enabled;
        for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
            clip_planes[i].enabled = snapshot->clip_planes[i].enabled;
        }
        memcpy(lights, snapshot->lights, sizeof(lights));
        memcpy(texture_1d_enabled, snapshot->texture_1d_enabled, sizeof(texture_1d_enabled));
        memcpy(texture_2d_enabled, snapshot->texture_2d_enabled, sizeof(texture_2d_enabled));
        memcpy(texture_3d_enabled, snapshot->texture_3d_enabled, sizeof(texture_3d_enabled));
        memcpy(texture_cube_map_enabled, snapshot->texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
        memcpy(texgen_state, snapshot->texgen_state, sizeof(texgen_state));
        update_texgen_enabled_cache();
        for (int i = 0; i < NXGL_EVAL_MAP_COUNT; ++i) {
            eval_maps1[i].enabled = snapshot->eval_maps1[i].enabled;
            eval_maps2[i].enabled = snapshot->eval_maps2[i].enabled;
        }
    }
    if ((mask & GL_COLOR_BUFFER_BIT) != 0) {
        blend_enabled = snapshot->blend_enabled;
        blend_sfactor = snapshot->blend_sfactor;
        blend_dfactor = snapshot->blend_dfactor;
        alpha_test_enabled = snapshot->alpha_test_enabled;
        alpha_test_func = snapshot->alpha_test_func;
        alpha_test_ref = snapshot->alpha_test_ref;
        memcpy(color_write_mask, snapshot->color_write_mask, sizeof(color_write_mask));
        color_logic_op_enabled = snapshot->color_logic_op_enabled;
        logic_op_mode = snapshot->logic_op_mode;
        multisample_enabled = snapshot->multisample_enabled;
        sample_alpha_to_coverage_enabled = snapshot->sample_alpha_to_coverage_enabled;
        sample_alpha_to_one_enabled = snapshot->sample_alpha_to_one_enabled;
        sample_coverage_enabled = snapshot->sample_coverage_enabled;
        sample_coverage_value = snapshot->sample_coverage_value;
        sample_coverage_invert = snapshot->sample_coverage_invert;
        draw_buffer_mode = snapshot->draw_buffer_mode;
        read_buffer_mode = snapshot->read_buffer_mode;
    }
    if ((mask & GL_HINT_BIT) != 0) {
        perspective_correction_hint = snapshot->perspective_correction_hint;
        point_smooth_hint = snapshot->point_smooth_hint;
        line_smooth_hint = snapshot->line_smooth_hint;
        polygon_smooth_hint = snapshot->polygon_smooth_hint;
        fog_hint = snapshot->fog_hint;
    }
    if ((mask & GL_EVAL_BIT) != 0) {
        memcpy(eval_maps1, snapshot->eval_maps1, sizeof(eval_maps1));
        memcpy(eval_maps2, snapshot->eval_maps2, sizeof(eval_maps2));
        auto_normal_enabled = snapshot->auto_normal_enabled;
        map_grid1_n = snapshot->map_grid1_n;
        map_grid1_u1 = snapshot->map_grid1_u1;
        map_grid1_u2 = snapshot->map_grid1_u2;
        map_grid2_un = snapshot->map_grid2_un;
        map_grid2_vn = snapshot->map_grid2_vn;
        map_grid2_u1 = snapshot->map_grid2_u1;
        map_grid2_u2 = snapshot->map_grid2_u2;
        map_grid2_v1 = snapshot->map_grid2_v1;
        map_grid2_v2 = snapshot->map_grid2_v2;
    }
    if ((mask & GL_LIST_BIT) != 0) {
        list_base = snapshot->list_base;
    }
    if ((mask & GL_TEXTURE_BIT) != 0) {
        active_texture = snapshot->active_texture;
        memcpy(texture_1d_enabled, snapshot->texture_1d_enabled, sizeof(texture_1d_enabled));
        memcpy(texture_2d_enabled, snapshot->texture_2d_enabled, sizeof(texture_2d_enabled));
        memcpy(texture_3d_enabled, snapshot->texture_3d_enabled, sizeof(texture_3d_enabled));
        memcpy(texture_cube_map_enabled, snapshot->texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
        memcpy(texture_binding_1d, snapshot->texture_binding_1d, sizeof(texture_binding_1d));
        memcpy(texture_binding_2d, snapshot->texture_binding_2d, sizeof(texture_binding_2d));
        memcpy(texture_binding_3d, snapshot->texture_binding_3d, sizeof(texture_binding_3d));
        memcpy(texture_binding_cube_map, snapshot->texture_binding_cube_map, sizeof(texture_binding_cube_map));
        memcpy(texgen_state, snapshot->texgen_state, sizeof(texgen_state));
        update_texgen_enabled_cache();
        memcpy(texture_env_mode, snapshot->texture_env_mode, sizeof(texture_env_mode));
        memcpy(texture_env_color, snapshot->texture_env_color, sizeof(texture_env_color));
        memcpy(texture_combine_rgb, snapshot->texture_combine_rgb, sizeof(texture_combine_rgb));
        memcpy(texture_combine_alpha, snapshot->texture_combine_alpha, sizeof(texture_combine_alpha));
        memcpy(texture_source_rgb, snapshot->texture_source_rgb, sizeof(texture_source_rgb));
        memcpy(texture_source_alpha, snapshot->texture_source_alpha, sizeof(texture_source_alpha));
        memcpy(texture_operand_rgb, snapshot->texture_operand_rgb, sizeof(texture_operand_rgb));
        memcpy(texture_operand_alpha, snapshot->texture_operand_alpha, sizeof(texture_operand_alpha));
        memcpy(texture_rgb_scale, snapshot->texture_rgb_scale, sizeof(texture_rgb_scale));
        memcpy(texture_alpha_scale, snapshot->texture_alpha_scale, sizeof(texture_alpha_scale));
    }
    sync_native_state();
}

static void capture_client_attrib_snapshot(ClientAttribSnapshot *snapshot, GLbitfield mask)
{
    snapshot->mask = mask;
    snapshot->client_active_texture = client_active_texture;
    snapshot->vertex_array = vertex_array;
    snapshot->color_array = color_array;
    memcpy(snapshot->texcoord_array, texcoord_array, sizeof(texcoord_array));
    snapshot->normal_array = normal_array;
    snapshot->pack_alignment = pack_alignment;
    snapshot->pack_row_length = pack_row_length;
    snapshot->pack_skip_rows = pack_skip_rows;
    snapshot->pack_skip_pixels = pack_skip_pixels;
    snapshot->pack_image_height = pack_image_height;
    snapshot->pack_skip_images = pack_skip_images;
    snapshot->pack_swap_bytes = pack_swap_bytes;
    snapshot->pack_lsb_first = pack_lsb_first;
    snapshot->unpack_alignment = unpack_alignment;
    snapshot->unpack_row_length = unpack_row_length;
    snapshot->unpack_skip_rows = unpack_skip_rows;
    snapshot->unpack_skip_pixels = unpack_skip_pixels;
    snapshot->unpack_image_height = unpack_image_height;
    snapshot->unpack_skip_images = unpack_skip_images;
    snapshot->unpack_swap_bytes = unpack_swap_bytes;
    snapshot->unpack_lsb_first = unpack_lsb_first;
}

static void restore_client_attrib_snapshot(const ClientAttribSnapshot *snapshot)
{
    GLbitfield mask = snapshot->mask;

    if ((mask & GL_CLIENT_VERTEX_ARRAY_BIT) != 0) {
        client_active_texture = snapshot->client_active_texture;
        vertex_array = snapshot->vertex_array;
        color_array = snapshot->color_array;
        memcpy(texcoord_array, snapshot->texcoord_array, sizeof(texcoord_array));
        normal_array = snapshot->normal_array;
    }
    if ((mask & GL_CLIENT_PIXEL_STORE_BIT) != 0) {
        pack_alignment = snapshot->pack_alignment;
        pack_row_length = snapshot->pack_row_length;
        pack_skip_rows = snapshot->pack_skip_rows;
        pack_skip_pixels = snapshot->pack_skip_pixels;
        pack_image_height = snapshot->pack_image_height;
        pack_skip_images = snapshot->pack_skip_images;
        pack_swap_bytes = snapshot->pack_swap_bytes;
        pack_lsb_first = snapshot->pack_lsb_first;
        unpack_alignment = snapshot->unpack_alignment;
        unpack_row_length = snapshot->unpack_row_length;
        unpack_skip_rows = snapshot->unpack_skip_rows;
        unpack_skip_pixels = snapshot->unpack_skip_pixels;
        unpack_image_height = snapshot->unpack_image_height;
        unpack_skip_images = snapshot->unpack_skip_images;
        unpack_swap_bytes = snapshot->unpack_swap_bytes;
        unpack_lsb_first = snapshot->unpack_lsb_first;
    }
}

static void clear_display_list(DisplayList *list)
{
    for (size_t i = 0; i < list->count; ++i) {
        free(list->commands[i].data);
        list->commands[i].data = NULL;
        list->commands[i].data_size = 0;
    }
    free(list->commands);
    list->commands = NULL;
    list->count = 0;
    list->capacity = 0;
    list->allocated = false;
}

static bool is_recording(void)
{
    return recording_list != 0 && !replaying_list;
}

static bool compile_only(void)
{
    return is_recording() && recording_mode == GL_COMPILE;
}

static bool reject_inside_begin(void)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return true;
    }
    return false;
}

static void record_command(ListCommand command)
{
    DisplayList *list;
    ListCommand *grown;
    size_t new_capacity;

    if (!is_recording() || recording_list >= 256) {
        free(command.data);
        return;
    }

    list = &display_lists[recording_list];
    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        grown = (ListCommand *)realloc(list->commands, new_capacity * sizeof(ListCommand));
        if (grown == NULL) {
            free(command.data);
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        list->commands = grown;
        list->capacity = new_capacity;
    }
    list->commands[list->count++] = command;
}

static bool attach_command_data(ListCommand *command, const void *data, size_t size)
{
    if (size == 0) {
        return true;
    }
    command->data = malloc(size);
    if (command->data == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return false;
    }
    memcpy(command->data, data, size);
    command->data_size = size;
    return true;
}

static int texture_unit_index(GLenum texture)
{
    if (texture < GL_TEXTURE0 || texture > GL_TEXTURE3) {
        return -1;
    }
    return (int)(texture - GL_TEXTURE0);
}

static int pixel_map_index(GLenum map)
{
    if (map >= GL_PIXEL_MAP_I_TO_I && map <= GL_PIXEL_MAP_A_TO_A) {
        return (int)(map - GL_PIXEL_MAP_I_TO_I);
    }
    return -1;
}

static int eval_map_components(GLenum target)
{
    switch (target) {
    case GL_MAP1_INDEX:
    case GL_MAP2_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_1:
        return 1;
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_2:
        return 2;
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_VERTEX_3:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_VERTEX_3:
        return 3;
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_4:
        return 4;
    default:
        return 0;
    }
}

static int eval_map_index(GLenum target, bool *map2)
{
    if (map2 != NULL) {
        *map2 = false;
    }
    switch (target) {
    case GL_MAP1_COLOR_4: return 0;
    case GL_MAP1_NORMAL: return 1;
    case GL_MAP1_TEXTURE_COORD_1: return 2;
    case GL_MAP1_TEXTURE_COORD_2: return 3;
    case GL_MAP1_TEXTURE_COORD_3: return 4;
    case GL_MAP1_TEXTURE_COORD_4: return 5;
    case GL_MAP1_VERTEX_3: return 6;
    case GL_MAP1_VERTEX_4: return 7;
    case GL_MAP1_INDEX: return 8;
    case GL_MAP2_COLOR_4: if (map2 != NULL) *map2 = true; return 0;
    case GL_MAP2_NORMAL: if (map2 != NULL) *map2 = true; return 1;
    case GL_MAP2_TEXTURE_COORD_1: if (map2 != NULL) *map2 = true; return 2;
    case GL_MAP2_TEXTURE_COORD_2: if (map2 != NULL) *map2 = true; return 3;
    case GL_MAP2_TEXTURE_COORD_3: if (map2 != NULL) *map2 = true; return 4;
    case GL_MAP2_TEXTURE_COORD_4: if (map2 != NULL) *map2 = true; return 5;
    case GL_MAP2_VERTEX_3: if (map2 != NULL) *map2 = true; return 6;
    case GL_MAP2_VERTEX_4: if (map2 != NULL) *map2 = true; return 7;
    case GL_MAP2_INDEX: if (map2 != NULL) *map2 = true; return 8;
    default: return -1;
    }
}

static void init_eval_maps(void)
{
    static const GLenum map1_targets[NXGL_EVAL_MAP_COUNT] = {
        GL_MAP1_COLOR_4, GL_MAP1_NORMAL, GL_MAP1_TEXTURE_COORD_1, GL_MAP1_TEXTURE_COORD_2,
        GL_MAP1_TEXTURE_COORD_3, GL_MAP1_TEXTURE_COORD_4, GL_MAP1_VERTEX_3, GL_MAP1_VERTEX_4, GL_MAP1_INDEX
    };
    static const GLenum map2_targets[NXGL_EVAL_MAP_COUNT] = {
        GL_MAP2_COLOR_4, GL_MAP2_NORMAL, GL_MAP2_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_2,
        GL_MAP2_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_4, GL_MAP2_VERTEX_3, GL_MAP2_VERTEX_4, GL_MAP2_INDEX
    };

    memset(eval_maps1, 0, sizeof(eval_maps1));
    memset(eval_maps2, 0, sizeof(eval_maps2));
    for (int i = 0; i < NXGL_EVAL_MAP_COUNT; ++i) {
        eval_maps1[i].target = map1_targets[i];
        eval_maps1[i].components = eval_map_components(map1_targets[i]);
        eval_maps1[i].u2 = 1.0f;
        eval_maps2[i].target = map2_targets[i];
        eval_maps2[i].components = eval_map_components(map2_targets[i]);
        eval_maps2[i].u2 = 1.0f;
        eval_maps2[i].v2 = 1.0f;
    }
}

static GLenum pixel_map_size_pname(GLenum map)
{
    int index = pixel_map_index(map);
    return index >= 0 ? (GLenum)(GL_PIXEL_MAP_I_TO_I_SIZE + index) : 0;
}

static int pixel_map_index_from_size_pname(GLenum pname)
{
    if (pname >= GL_PIXEL_MAP_I_TO_I_SIZE && pname <= GL_PIXEL_MAP_A_TO_A_SIZE) {
        return (int)(pname - GL_PIXEL_MAP_I_TO_I_SIZE);
    }
    return -1;
}

static int active_texture_index(void)
{
    return texture_unit_index(active_texture);
}

static int client_texture_index(void)
{
    return texture_unit_index(client_active_texture);
}

static int texgen_coord_index(GLenum coord)
{
    switch (coord) {
    case GL_S: return 0;
    case GL_T: return 1;
    case GL_R: return 2;
    case GL_Q: return 3;
    default: return -1;
    }
}

static GLenum texgen_cap_from_index(int index)
{
    static const GLenum caps[4] = { GL_TEXTURE_GEN_S, GL_TEXTURE_GEN_T, GL_TEXTURE_GEN_R, GL_TEXTURE_GEN_Q };
    return index >= 0 && index < 4 ? caps[index] : 0;
}

static int texgen_index_from_cap(GLenum cap)
{
    switch (cap) {
    case GL_TEXTURE_GEN_S: return 0;
    case GL_TEXTURE_GEN_T: return 1;
    case GL_TEXTURE_GEN_R: return 2;
    case GL_TEXTURE_GEN_Q: return 3;
    default: return -1;
    }
}

static bool valid_texgen_mode(GLenum coord, GLenum mode)
{
    if (mode == GL_OBJECT_LINEAR || mode == GL_EYE_LINEAR) {
        return true;
    }
    if (mode == GL_SPHERE_MAP) {
        return coord == GL_S || coord == GL_T;
    }
    return false;
}

static void init_texgen_state(void)
{
    texgen_any_enabled = false;
    for (int unit = 0; unit < 4; ++unit) {
        for (int coord = 0; coord < 4; ++coord) {
            TexGenState *state = &texgen_state[unit][coord];
            memset(state, 0, sizeof(*state));
            state->mode = GL_EYE_LINEAR;
            state->object_plane[coord] = 1.0f;
            state->eye_plane[coord] = 1.0f;
        }
    }
}

static bool valid_texture_env_mode(GLint mode)
{
    return mode == GL_MODULATE ||
           mode == GL_DECAL ||
           mode == GL_BLEND ||
           mode == GL_REPLACE ||
           mode == GL_COMBINE ||
           mode == GL_ADD;
}

static bool valid_combine_mode(GLint mode)
{
    return mode == GL_REPLACE ||
           mode == GL_MODULATE ||
           mode == GL_ADD ||
           mode == GL_ADD_SIGNED ||
           mode == GL_INTERPOLATE ||
           mode == GL_SUBTRACT ||
           mode == GL_DOT3_RGB ||
           mode == GL_DOT3_RGBA;
}

static bool valid_combine_source(GLint source)
{
    return source == GL_TEXTURE ||
           source == GL_CONSTANT ||
           source == GL_PRIMARY_COLOR ||
           source == GL_PREVIOUS ||
           (source >= GL_TEXTURE0 && source <= GL_TEXTURE3);
}

static bool valid_combine_rgb_operand(GLint operand)
{
    return operand == GL_SRC_COLOR ||
           operand == GL_ONE_MINUS_SRC_COLOR ||
           operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_ALPHA;
}

static bool valid_combine_alpha_operand(GLint operand)
{
    return operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_ALPHA;
}

static bool valid_combine_scale(GLfloat scale)
{
    return scale == 1.0f || scale == 2.0f || scale == 4.0f;
}

static int light_index(GLenum light)
{
    if (light < GL_LIGHT0 || light > GL_LIGHT7) {
        return -1;
    }
    return (int)(light - GL_LIGHT0);
}

static void init_texture_object(TextureObject *texture)
{
    memset(texture, 0, sizeof(*texture));
    texture->min_filter = GL_NEAREST_MIPMAP_LINEAR;
    texture->mag_filter = GL_LINEAR;
    texture->wrap_s = GL_REPEAT;
    texture->wrap_t = GL_REPEAT;
    texture->wrap_r = GL_REPEAT;
    texture->min_lod = -1000.0f;
    texture->max_lod = 1000.0f;
    texture->lod_bias = 0.0f;
    texture->priority = 1.0f;
    texture->base_level = 0;
    texture->max_level = 1000;
}

static void init_light_object(LightObject *light, int index)
{
    light->enabled = false;
    light->ambient = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 1.0f };
    light->diffuse = (NxglBackendColor){ index == 0 ? 1.0f : 0.0f, index == 0 ? 1.0f : 0.0f, index == 0 ? 1.0f : 0.0f, 1.0f };
    light->specular = light->diffuse;
    light->position[0] = 0.0f;
    light->position[1] = 0.0f;
    light->position[2] = 1.0f;
    light->position[3] = 0.0f;
    light->spot_direction[0] = 0.0f;
    light->spot_direction[1] = 0.0f;
    light->spot_direction[2] = -1.0f;
    light->spot_exponent = 0.0f;
    light->spot_cutoff = 180.0f;
    light->constant_attenuation = 1.0f;
    light->linear_attenuation = 0.0f;
    light->quadratic_attenuation = 0.0f;
}

static int cube_face_index(GLenum target)
{
    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        return (int)(target - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    }
    return -1;
}

static bool valid_texture_binding_target(GLenum target)
{
    return target == GL_TEXTURE_1D || target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP;
}

static bool valid_texture_image_target(GLenum target)
{
    return target == GL_TEXTURE_1D || target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || cube_face_index(target) >= 0;
}

static bool valid_texture_2d_image_target(GLenum target)
{
    return target == GL_TEXTURE_2D || cube_face_index(target) >= 0;
}

static bool valid_compressed_texture_format(GLenum format)
{
    return format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
}

static int compressed_block_size(GLenum format)
{
    if (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
        return 8;
    }
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
        return 16;
    }
    return 0;
}

static GLsizei compressed_image_size(GLsizei width, GLsizei height, GLenum format)
{
    int block_size = compressed_block_size(format);
    GLsizei block_w;
    GLsizei block_h;

    if (width <= 0 || height <= 0 || block_size == 0) {
        return 0;
    }
    block_w = (width + 3) / 4;
    block_h = (height + 3) / 4;
    return block_w * block_h * block_size;
}

static GLsizei compressed_image_size_3d(GLsizei width, GLsizei height, GLsizei depth, GLenum format)
{
    GLsizei slice_size;

    if (depth <= 0) {
        return 0;
    }
    slice_size = compressed_image_size(width, height, format);
    if (slice_size <= 0) {
        return 0;
    }
    return slice_size * depth;
}

static void destroy_texture_level_image(TextureLevel *level)
{
    free(level->rgba);
    free(level->compressed_data);
    level->rgba = NULL;
    level->compressed_data = NULL;
    level->defined = false;
    level->compressed = false;
    level->internal_format = 0;
    level->width = 0;
    level->height = 0;
    level->depth = 0;
    level->compressed_size = 0;
}

static void destroy_texture_level(TextureObject *texture, GLenum target, int level)
{
    int face = cube_face_index(target);

    if (target == GL_TEXTURE_1D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native_1d);
        memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    } else if (target == GL_TEXTURE_2D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native);
        memset(&texture->native, 0, sizeof(texture->native));
    } else if (target == GL_TEXTURE_3D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native_3d);
        memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    }
    if (target == GL_TEXTURE_1D) {
        destroy_texture_level_image(&texture->levels_1d[level]);
    } else if (target == GL_TEXTURE_2D) {
        destroy_texture_level_image(&texture->levels[level]);
    } else if (target == GL_TEXTURE_3D) {
        destroy_texture_level_image(&texture->levels_3d[level]);
    } else if (face >= 0) {
        destroy_texture_level_image(&texture->cube_faces[face][level]);
    }
}

static void destroy_texture_image(TextureObject *texture)
{
    nxgl_backend_texture_destroy(&texture->native);
    memset(&texture->native, 0, sizeof(texture->native));
    nxgl_backend_texture_destroy(&texture->native_1d);
    memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    nxgl_backend_texture_destroy(&texture->native_3d);
    memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    nxgl_backend_texture_destroy(&texture->native_cube);
    memset(&texture->native_cube, 0, sizeof(texture->native_cube));
    for (int level = 0; level < NXGL_MAX_TEXTURE_LEVELS; ++level) {
        destroy_texture_level_image(&texture->levels[level]);
        destroy_texture_level_image(&texture->levels_1d[level]);
        destroy_texture_level_image(&texture->levels_3d[level]);
        for (int face = 0; face < 6; ++face) {
            destroy_texture_level_image(&texture->cube_faces[face][level]);
        }
    }
}

static TextureObject *bound_texture_object_for_target(GLenum target)
{
    int unit = active_texture_index();
    GLuint id = 0;

    if (unit < 0) {
        return NULL;
    }
    if (target == GL_TEXTURE_1D) {
        id = texture_binding_1d[unit];
    } else if (target == GL_TEXTURE_2D) {
        id = texture_binding_2d[unit];
    } else if (target == GL_TEXTURE_3D) {
        id = texture_binding_3d[unit];
    } else if (target == GL_TEXTURE_CUBE_MAP || cube_face_index(target) >= 0) {
        id = texture_binding_cube_map[unit];
    }

    if (id == 0 || id >= 16) {
        return NULL;
    }
    return &texture_objects[id];
}

static TextureObject *bound_texture_object(void)
{
    return bound_texture_object_for_target(GL_TEXTURE_2D);
}

static TextureLevel *texture_level_for_target(TextureObject *texture, GLenum target, GLint level)
{
    int face = cube_face_index(target);

    if (texture == NULL || level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        return NULL;
    }
    if (target == GL_TEXTURE_1D) {
        return &texture->levels_1d[level];
    }
    if (target == GL_TEXTURE_2D) {
        return &texture->levels[level];
    }
    if (target == GL_TEXTURE_3D) {
        return &texture->levels_3d[level];
    }
    if (face >= 0) {
        return &texture->cube_faces[face][level];
    }
    return NULL;
}

static TextureLevel *select_texture_level_for_lod(TextureObject *texture, TextureLevel *levels, GLfloat lambda)
{
    GLint base;
    GLint max;
    GLint end;
    GLint selected;
    GLfloat lod;

    if (texture == NULL || levels == NULL) {
        return NULL;
    }
    base = texture->base_level;
    max = texture->max_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return NULL;
    }
    if (max >= NXGL_MAX_TEXTURE_LEVELS) {
        max = NXGL_MAX_TEXTURE_LEVELS - 1;
    }
    if (!levels[base].defined) {
        return NULL;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return &levels[base];
    }
    if (!texture_levels_complete(texture, levels)) {
        return NULL;
    }

    lod = lambda + texture->lod_bias;
    if (lod < texture->min_lod) {
        lod = texture->min_lod;
    }
    if (lod > texture->max_lod) {
        lod = texture->max_lod;
    }
    if (lod < 0.0f) {
        lod = 0.0f;
    }

    selected = base + (GLint)floorf(lod + 0.5f);
    end = mip_chain_end_level(levels[base].width, levels[base].height, levels[base].depth, base, max);
    if (selected < base) {
        selected = base;
    }
    if (selected > end) {
        selected = end;
    }
    if (selected >= NXGL_MAX_TEXTURE_LEVELS || !levels[selected].defined) {
        return NULL;
    }
    return &levels[selected];
}

static bool cube_base_complete(const TextureObject *texture)
{
    GLint base;
    GLsizei size;
    GLint internal_format;

    if (texture == NULL) {
        return false;
    }
    base = texture->base_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS) {
        return false;
    }

    if (!texture->cube_faces[0][base].defined || texture->cube_faces[0][base].width != texture->cube_faces[0][base].height) {
        return false;
    }

    size = texture->cube_faces[0][base].width;
    internal_format = texture->cube_faces[0][base].internal_format;
    for (int face = 0; face < 6; ++face) {
        const TextureLevel *image = &texture->cube_faces[face][base];
        if (!image->defined ||
            image->rgba == NULL ||
            image->width != size ||
            image->height != size ||
            image->internal_format != internal_format) {
            return false;
        }
    }
    return true;
}

static bool cube_levels_complete(const TextureObject *texture)
{
    GLint base;
    GLint max;
    GLint end;
    GLsizei expected_size;
    GLint internal_format;

    if (texture == NULL) {
        return false;
    }
    base = texture->base_level;
    max = texture->max_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return false;
    }
    if (max >= NXGL_MAX_TEXTURE_LEVELS) {
        max = NXGL_MAX_TEXTURE_LEVELS - 1;
    }
    if (!cube_base_complete(texture)) {
        return false;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return true;
    }

    expected_size = texture->cube_faces[0][base].width;
    internal_format = texture->cube_faces[0][base].internal_format;
    end = mip_chain_end_level(expected_size, expected_size, 1, base, max);
    for (GLint level = base; level <= end; ++level) {
        for (int face = 0; face < 6; ++face) {
            const TextureLevel *image = &texture->cube_faces[face][level];
            if (!image->defined ||
                image->rgba == NULL ||
                image->width != expected_size ||
                image->height != expected_size ||
                image->depth != 1 ||
                image->internal_format != internal_format) {
                return false;
            }
        }
        expected_size = expected_size > 1 ? expected_size / 2 : 1;
    }
    return true;
}

static GLint select_cube_level_for_lod(TextureObject *texture, GLfloat lambda)
{
    TextureLevel *selected;

    if (texture == NULL || !cube_levels_complete(texture)) {
        return -1;
    }
    selected = select_texture_level_for_lod(texture, texture->cube_faces[0], lambda);
    if (selected == NULL) {
        return -1;
    }
    return (GLint)(selected - texture->cube_faces[0]);
}

static bool rebuild_native_cube_texture(TextureObject *texture)
{
    const uint8_t *faces[6];
    GLint level;

    nxgl_backend_texture_destroy(&texture->native_cube);
    memset(&texture->native_cube, 0, sizeof(texture->native_cube));
    level = select_cube_level_for_lod(texture, 0.0f);
    if (level < 0) {
        return true;
    }

    for (int face = 0; face < 6; ++face) {
        faces[face] = texture->cube_faces[face][level].rgba;
    }
    return nxgl_backend_texture_create_cube_rgba(&texture->native_cube, (uint16_t)texture->cube_faces[0][level].width, faces) == 0;
}

static NxglBackendCompressedTextureFormat native_compressed_format(GLenum format)
{
    if (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
        return NXGL_BACKEND_COMPRESSED_DXT1;
    }
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
        return NXGL_BACKEND_COMPRESSED_DXT3;
    }
    return NXGL_BACKEND_COMPRESSED_DXT5;
}

static bool rebuild_native_texture2d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native);
    memset(&texture->native, 0, sizeof(texture->native));

    image = select_texture_level_for_lod(texture, texture->levels, 0.0f);
    if (image == NULL) {
        return true;
    }
    if (image->compressed) {
        if (image->compressed_data == NULL || image->compressed_size <= 0) {
            return true;
        }
        return nxgl_backend_texture_create_compressed(&texture->native,
                                            (uint16_t)image->width,
                                            (uint16_t)image->height,
                                            native_compressed_format((GLenum)image->internal_format),
                                            image->compressed_data,
                                            (uint32_t)image->compressed_size) == 0;
    }
    if (image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba(&texture->native, (uint16_t)image->width, (uint16_t)image->height, image->rgba) == 0;
}

static bool rebuild_native_compressed_texture(TextureObject *texture)
{
    return rebuild_native_texture2d(texture);
}

static bool rebuild_native_texture1d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native_1d);
    memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    if (texture->base_level != 0 || is_mipmap_filter(texture->min_filter)) {
        return true;
    }
    image = &texture->levels_1d[0];
    if (image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba(&texture->native_1d, (uint16_t)image->width, 1, image->rgba) == 0;
}

static bool rebuild_native_texture3d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native_3d);
    memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    image = select_texture_level_for_lod(texture, texture->levels_3d, 0.0f);
    if (image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba3d(&texture->native_3d,
                                    (uint16_t)image->width,
                                    (uint16_t)image->height,
                                    (uint16_t)image->depth,
                                    image->rgba) == 0;
}

static int type_size(GLenum type)
{
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    default:
        return 0;
    }
}

static int get_component_count(GLenum pname)
{
    switch (pname) {
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
    case GL_TRANSPOSE_PROJECTION_MATRIX:
    case GL_TRANSPOSE_TEXTURE_MATRIX:
    case GL_TRANSPOSE_COLOR_MATRIX:
        return 16;
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_COMPRESSED_TEXTURE_FORMATS:
    case GL_COLOR_WRITEMASK:
    case GL_CURRENT_COLOR:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_CLEAR_COLOR:
    case GL_CURRENT_RASTER_POSITION:
    case GL_CURRENT_RASTER_COLOR:
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
    case GL_FOG_COLOR:
    case GL_ACCUM_CLEAR_VALUE:
    case GL_LIGHT_MODEL_AMBIENT:
        return 4;
    case GL_CURRENT_NORMAL:
        return 3;
    case GL_DEPTH_RANGE:
    case GL_POLYGON_MODE:
    case GL_POINT_SIZE_RANGE:
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        return 2;
    default:
        return 1;
    }
}

static bool is_float_or_integer_type(GLenum type)
{
    return type_size(type) != 0;
}

static GLsizei effective_stride(const ClientArray *array)
{
    GLsizei stride = array->stride;
    if (stride == 0) {
        stride = array->size * type_size(array->type);
    }
    return stride;
}

static size_t aligned_row_bytes(size_t bytes, GLint alignment)
{
    size_t align = alignment > 0 ? (size_t)alignment : 1;
    return (bytes + align - 1) & ~(align - 1);
}

static GLsizei storage_row_pixels(GLsizei width, GLint row_length, GLint skip_pixels)
{
    GLsizei row_pixels = row_length > 0 ? row_length : width;
    GLsizei needed = skip_pixels + width;
    return row_pixels < needed ? needed : row_pixels;
}

static size_t pixel_store_row_stride(GLsizei width, GLint row_length, GLint skip_pixels, size_t pixel_bytes, GLint alignment)
{
    if (width <= 0 || pixel_bytes == 0) {
        return 0;
    }
    return aligned_row_bytes((size_t)storage_row_pixels(width, row_length, skip_pixels) * pixel_bytes, alignment);
}

static size_t pixel_store_data_size(GLsizei width, GLsizei height, GLint row_length, GLint skip_rows, GLint skip_pixels, size_t pixel_bytes, GLint alignment)
{
    size_t stride;

    if (width <= 0 || height <= 0 || pixel_bytes == 0) {
        return 0;
    }
    stride = pixel_store_row_stride(width, row_length, skip_pixels, pixel_bytes, alignment);
    return (size_t)(skip_rows + height - 1) * stride + (size_t)(skip_pixels + width) * pixel_bytes;
}

static GLsizei storage_image_rows(GLsizei height, GLint image_height, GLint skip_rows)
{
    GLsizei rows = image_height > 0 ? image_height : height;
    GLsizei needed = skip_rows + height;
    return rows < needed ? needed : rows;
}

static size_t pixel_store_slice_stride(GLsizei width, GLsizei height, GLint row_length, GLint skip_rows, GLint skip_pixels, GLint image_height, size_t pixel_bytes, GLint alignment)
{
    size_t row_stride;

    if (width <= 0 || height <= 0 || pixel_bytes == 0) {
        return 0;
    }
    row_stride = pixel_store_row_stride(width, row_length, skip_pixels, pixel_bytes, alignment);
    return row_stride * (size_t)storage_image_rows(height, image_height, skip_rows);
}

static size_t unpack_color_pixel_bytes(GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);

    if (comps == 0 || elem_size == 0) {
        return 0;
    }
    return type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
}

static size_t bitmap_unpack_row_stride(GLsizei width)
{
    GLsizei row_pixels;

    if (width <= 0) {
        return 0;
    }
    row_pixels = storage_row_pixels(width, unpack_row_length, unpack_skip_pixels);
    return aligned_row_bytes((size_t)((row_pixels + 7) / 8), unpack_alignment);
}

static size_t pixel_source_data_size(GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    size_t pixel_bytes;

    if (width <= 0 || height <= 0) {
        return 0;
    }
    if (format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA) {
        pixel_bytes = unpack_color_pixel_bytes(format, type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else if (format == GL_COLOR_INDEX) {
        pixel_bytes = (size_t)type_size(type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else if (format == GL_DEPTH_COMPONENT || format == GL_STENCIL_INDEX) {
        pixel_bytes = (size_t)type_size(type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else {
        return 0;
    }
    return pixel_store_data_size(width, height, unpack_row_length, unpack_skip_rows, unpack_skip_pixels, pixel_bytes, unpack_alignment);
}

static size_t bitmap_source_data_size(GLsizei width, GLsizei height)
{
    size_t stride;
    GLsizei final_bit;

    if (width <= 0 || height <= 0) {
        return 0;
    }
    stride = bitmap_unpack_row_stride(width);
    final_bit = unpack_skip_pixels + width;
    return (size_t)(unpack_skip_rows + height - 1) * stride + (size_t)((final_bit + 7) / 8);
}


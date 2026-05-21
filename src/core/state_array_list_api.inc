void glEnable(uint32_t cap)
{
    int unit = active_texture_index();
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    ListCommand command = { LIST_CMD_ENABLE };
    command.a = cap;

    if (reject_inside_begin()) {
        return;
    }
    if (eval_index >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (map2) {
            eval_maps2[eval_index].enabled = GL_TRUE;
        } else {
            eval_maps1[eval_index].enabled = GL_TRUE;
        }
    } else if (texgen_coord >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texgen_state[unit][texgen_coord].enabled = GL_TRUE;
            texgen_any_enabled = true;
        }
    } else if (clip >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        clip_planes[clip].enabled = GL_TRUE;
    } else if (light >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lights[light].enabled = true;
    } else if (cap == GL_BLEND) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        blend_enabled = true;
        sync_native_state();
    } else if (cap == GL_LINE_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        line_stipple_enabled = true;
    } else if (cap == GL_POLYGON_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        polygon_stipple_enabled = true;
    } else if (cap == GL_DEPTH_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        depth_test_enabled = true;
        sync_native_state();
    } else if (cap == GL_CULL_FACE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        cull_enabled = true;
        sync_native_state();
    } else if (cap == GL_SCISSOR_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        scissor_test_enabled = true;
        sync_native_state();
    } else if (cap == GL_STENCIL_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        stencil_test_enabled = true;
    } else if (cap == GL_ALPHA_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        alpha_test_enabled = true;
    } else if (cap == GL_COLOR_LOGIC_OP || cap == GL_INDEX_LOGIC_OP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_logic_op_enabled = true;
    } else if (cap == GL_POLYGON_OFFSET_POINT ||
               cap == GL_POLYGON_OFFSET_LINE ||
               cap == GL_POLYGON_OFFSET_FILL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_POLYGON_OFFSET_POINT) {
            polygon_offset_point_enabled = true;
        } else if (cap == GL_POLYGON_OFFSET_LINE) {
            polygon_offset_line_enabled = true;
        } else {
            polygon_offset_fill_enabled = true;
        }
    } else if (cap == GL_TEXTURE_1D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_1d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_2D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_2d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_3D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_3d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_CUBE_MAP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_cube_map_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_LIGHTING) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lighting_enabled = true;
    } else if (cap == GL_FOG) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        fog_enabled = true;
    } else if (cap == GL_COLOR_MATERIAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_material_enabled = true;
        apply_color_material(current_color);
    } else if (cap == GL_NORMALIZE || cap == GL_RESCALE_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_NORMALIZE) {
            normalize_enabled = true;
        } else {
            rescale_normal_enabled = true;
        }
    } else if (cap == GL_AUTO_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        auto_normal_enabled = GL_TRUE;
    } else if (cap == GL_MULTISAMPLE ||
               cap == GL_SAMPLE_ALPHA_TO_COVERAGE ||
               cap == GL_SAMPLE_ALPHA_TO_ONE ||
               cap == GL_SAMPLE_COVERAGE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_MULTISAMPLE) {
            multisample_enabled = true;
        } else if (cap == GL_SAMPLE_ALPHA_TO_COVERAGE) {
            sample_alpha_to_coverage_enabled = true;
        } else if (cap == GL_SAMPLE_ALPHA_TO_ONE) {
            sample_alpha_to_one_enabled = true;
        } else {
            sample_coverage_enabled = true;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glDisable(uint32_t cap)
{
    int unit = active_texture_index();
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    ListCommand command = { LIST_CMD_DISABLE };
    command.a = cap;

    if (reject_inside_begin()) {
        return;
    }
    if (eval_index >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (map2) {
            eval_maps2[eval_index].enabled = GL_FALSE;
        } else {
            eval_maps1[eval_index].enabled = GL_FALSE;
        }
    } else if (texgen_coord >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texgen_state[unit][texgen_coord].enabled = GL_FALSE;
            update_texgen_enabled_cache();
        }
    } else if (clip >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        clip_planes[clip].enabled = GL_FALSE;
    } else if (light >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lights[light].enabled = false;
    } else if (cap == GL_BLEND) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        blend_enabled = false;
        sync_native_state();
    } else if (cap == GL_LINE_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        line_stipple_enabled = false;
    } else if (cap == GL_POLYGON_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        polygon_stipple_enabled = false;
    } else if (cap == GL_DEPTH_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        depth_test_enabled = false;
        sync_native_state();
    } else if (cap == GL_CULL_FACE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        cull_enabled = false;
        sync_native_state();
    } else if (cap == GL_SCISSOR_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        scissor_test_enabled = false;
        sync_native_state();
    } else if (cap == GL_STENCIL_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        stencil_test_enabled = false;
    } else if (cap == GL_ALPHA_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        alpha_test_enabled = false;
    } else if (cap == GL_COLOR_LOGIC_OP || cap == GL_INDEX_LOGIC_OP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_logic_op_enabled = false;
    } else if (cap == GL_POLYGON_OFFSET_POINT ||
               cap == GL_POLYGON_OFFSET_LINE ||
               cap == GL_POLYGON_OFFSET_FILL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_POLYGON_OFFSET_POINT) {
            polygon_offset_point_enabled = false;
        } else if (cap == GL_POLYGON_OFFSET_LINE) {
            polygon_offset_line_enabled = false;
        } else {
            polygon_offset_fill_enabled = false;
        }
    } else if (cap == GL_TEXTURE_1D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_1d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_2D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_2d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_3D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_3d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_CUBE_MAP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_cube_map_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_LIGHTING) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lighting_enabled = false;
    } else if (cap == GL_FOG) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        fog_enabled = false;
    } else if (cap == GL_COLOR_MATERIAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_material_enabled = false;
    } else if (cap == GL_NORMALIZE || cap == GL_RESCALE_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_NORMALIZE) {
            normalize_enabled = false;
        } else {
            rescale_normal_enabled = false;
        }
    } else if (cap == GL_AUTO_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        auto_normal_enabled = GL_FALSE;
    } else if (cap == GL_MULTISAMPLE ||
               cap == GL_SAMPLE_ALPHA_TO_COVERAGE ||
               cap == GL_SAMPLE_ALPHA_TO_ONE ||
               cap == GL_SAMPLE_COVERAGE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_MULTISAMPLE) {
            multisample_enabled = false;
        } else if (cap == GL_SAMPLE_ALPHA_TO_COVERAGE) {
            sample_alpha_to_coverage_enabled = false;
        } else if (cap == GL_SAMPLE_ALPHA_TO_ONE) {
            sample_alpha_to_one_enabled = false;
        } else {
            sample_coverage_enabled = false;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glSampleCoverage(GLclampf value, GLboolean invert)
{
    if (reject_inside_begin()) {
        return;
    }
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    sample_coverage_value = value;
    sample_coverage_invert = invert ? GL_TRUE : GL_FALSE;
}

void glPushAttrib(GLbitfield mask)
{
    ListCommand command = { LIST_CMD_PUSH_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((mask & ~GL_ALL_ATTRIB_BITS) != 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (attrib_stack_top >= NXGL_ATTRIB_STACK_MAX) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    capture_attrib_snapshot(&attrib_stack[attrib_stack_top++], mask);
}

void glPopAttrib(void)
{
    ListCommand command = { LIST_CMD_POP_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (attrib_stack_top <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    restore_attrib_snapshot(&attrib_stack[--attrib_stack_top]);
}

void glPushClientAttrib(GLbitfield mask)
{
    ListCommand command = { LIST_CMD_PUSH_CLIENT_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    mask &= GL_CLIENT_ALL_ATTRIB_BITS;
    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (client_attrib_stack_top >= NXGL_CLIENT_ATTRIB_STACK_MAX) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    capture_client_attrib_snapshot(&client_attrib_stack[client_attrib_stack_top++], mask);
}

void glPopClientAttrib(void)
{
    ListCommand command = { LIST_CMD_POP_CLIENT_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (client_attrib_stack_top <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    restore_client_attrib_snapshot(&client_attrib_stack[--client_attrib_stack_top]);
}

void glDepthFunc(GLenum func)
{
    ListCommand command = { LIST_CMD_DEPTH_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_depth_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    record_command(command);
    if (compile_only()) {
        return;
    }
    depth_func = func;
}

void glDepthMask(GLboolean flag)
{
    ListCommand command = { LIST_CMD_DEPTH_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.u = flag;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_write_enabled = flag != GL_FALSE;
    sync_native_state();
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    ListCommand command = { LIST_CMD_STENCIL_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    command.f[0] = (GLfloat)ref;
    command.u = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_func = func;
    stencil_ref = ref;
    stencil_value_mask = mask;
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    ListCommand command = { LIST_CMD_STENCIL_OP };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_op(fail) || !valid_stencil_op(zfail) || !valid_stencil_op(zpass)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = fail;
    command.b = zfail;
    command.u = zpass;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_fail = fail;
    stencil_zfail = zfail;
    stencil_zpass = zpass;
}

void glStencilMask(GLuint mask)
{
    ListCommand command = { LIST_CMD_STENCIL_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.u = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_write_mask = mask;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (reject_inside_begin()) {
        return;
    }
    if (!valid_blend_src_factor(sfactor) || !valid_blend_dst_factor(dfactor)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_BLEND_FUNC };
    command.a = sfactor;
    command.b = dfactor;
    record_command(command);
    if (compile_only()) {
        return;
    }

    blend_sfactor = sfactor;
    blend_dfactor = dfactor;
    sync_native_state();
}

void glActiveTexture(GLenum texture)
{
    if (texture_unit_index(texture) < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_ACTIVE_TEXTURE };
    command.a = texture;
    record_command(command);
    if (compile_only()) {
        return;
    }

    active_texture = texture;
    sync_native_state();
}

void glClientActiveTexture(GLenum texture)
{
    if (texture_unit_index(texture) < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    client_active_texture = texture;
}

void glEnableClientState(GLenum array)
{
    int unit = client_texture_index();

    if (array == GL_VERTEX_ARRAY) {
        vertex_array.enabled = true;
    } else if (array == GL_COLOR_ARRAY) {
        color_array.enabled = true;
    } else if (array == GL_NORMAL_ARRAY) {
        normal_array.enabled = true;
    } else if (array == GL_TEXTURE_COORD_ARRAY) {
        if (unit >= 0) {
            texcoord_array[unit].enabled = true;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glDisableClientState(GLenum array)
{
    int unit = client_texture_index();

    if (array == GL_VERTEX_ARRAY) {
        vertex_array.enabled = false;
    } else if (array == GL_COLOR_ARRAY) {
        color_array.enabled = false;
    } else if (array == GL_NORMAL_ARRAY) {
        normal_array.enabled = false;
    } else if (array == GL_TEXTURE_COORD_ARRAY) {
        if (unit >= 0) {
            texcoord_array[unit].enabled = false;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (size < 2 || size > 4 || !is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    vertex_array.size = size;
    vertex_array.type = type;
    vertex_array.stride = stride;
    vertex_array.pointer = (const uint8_t *)pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (size < 3 || size > 4 || !is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    color_array.size = size;
    color_array.type = type;
    color_array.stride = stride;
    color_array.pointer = (const uint8_t *)pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    int unit = client_texture_index();

    if (size < 1 || size > 4 || !is_float_or_integer_type(type) || stride < 0 || unit < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    texcoord_array[unit].size = size;
    texcoord_array[unit].type = type;
    texcoord_array[unit].stride = stride;
    texcoord_array[unit].pointer = (const uint8_t *)pointer;
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (!is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    normal_array.size = 3;
    normal_array.type = type;
    normal_array.stride = stride;
    normal_array.pointer = (const uint8_t *)pointer;
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    const uint8_t *base = (const uint8_t *)pointer;
    GLsizei natural_stride = 0;
    GLint vertex_size = 0;
    GLint tex_size = 0;
    bool has_color = false;
    bool has_normal = false;
    bool has_texcoord = false;
    GLenum color_type = GL_FLOAT;
    GLint color_size = 4;
    size_t color_offset = 0;
    size_t normal_offset = 0;
    size_t tex_offset = 0;
    size_t vertex_offset = 0;
    int unit = client_texture_index();

    if (stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (unit < 0) {
        unit = 0;
    }

    switch (format) {
    case GL_V2F:
        vertex_size = 2; vertex_offset = 0; natural_stride = 2 * 4;
        break;
    case GL_V3F:
        vertex_size = 3; vertex_offset = 0; natural_stride = 3 * 4;
        break;
    case GL_C4UB_V2F:
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 0;
        vertex_size = 2; vertex_offset = 4; natural_stride = 4 + 2 * 4;
        break;
    case GL_C4UB_V3F:
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 0;
        vertex_size = 3; vertex_offset = 4; natural_stride = 4 + 3 * 4;
        break;
    case GL_C3F_V3F:
        has_color = true; color_type = GL_FLOAT; color_size = 3; color_offset = 0;
        vertex_size = 3; vertex_offset = 3 * 4; natural_stride = 6 * 4;
        break;
    case GL_N3F_V3F:
        has_normal = true; normal_offset = 0;
        vertex_size = 3; vertex_offset = 3 * 4; natural_stride = 6 * 4;
        break;
    case GL_C4F_N3F_V3F:
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 0;
        has_normal = true; normal_offset = 4 * 4;
        vertex_size = 3; vertex_offset = 7 * 4; natural_stride = 10 * 4;
        break;
    case GL_T2F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        vertex_size = 3; vertex_offset = 2 * 4; natural_stride = 5 * 4;
        break;
    case GL_T4F_V4F:
        has_texcoord = true; tex_size = 4; tex_offset = 0;
        vertex_size = 4; vertex_offset = 4 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_C4UB_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 2 * 4 + 4; natural_stride = 2 * 4 + 4 + 3 * 4;
        break;
    case GL_T2F_C3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 3; color_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 5 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_N3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_normal = true; normal_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 5 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_C4F_N3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 2 * 4;
        has_normal = true; normal_offset = 6 * 4;
        vertex_size = 3; vertex_offset = 9 * 4; natural_stride = 12 * 4;
        break;
    case GL_T4F_C4F_N3F_V4F:
        has_texcoord = true; tex_size = 4; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 4 * 4;
        has_normal = true; normal_offset = 8 * 4;
        vertex_size = 4; vertex_offset = 11 * 4; natural_stride = 15 * 4;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }

    if (stride == 0) {
        stride = natural_stride;
    }

    vertex_array.enabled = true;
    vertex_array.size = vertex_size;
    vertex_array.type = GL_FLOAT;
    vertex_array.stride = stride;
    vertex_array.pointer = base != NULL ? base + vertex_offset : NULL;

    color_array.enabled = has_color;
    color_array.size = color_size;
    color_array.type = color_type;
    color_array.stride = stride;
    color_array.pointer = has_color && base != NULL ? base + color_offset : NULL;

    normal_array.enabled = has_normal;
    normal_array.size = 3;
    normal_array.type = GL_FLOAT;
    normal_array.stride = stride;
    normal_array.pointer = has_normal && base != NULL ? base + normal_offset : NULL;

    texcoord_array[unit].enabled = has_texcoord;
    texcoord_array[unit].size = has_texcoord ? tex_size : 4;
    texcoord_array[unit].type = GL_FLOAT;
    texcoord_array[unit].stride = stride;
    texcoord_array[unit].pointer = has_texcoord && base != NULL ? base + tex_offset : NULL;
}

void glArrayElement(GLint index)
{
    NxglBackendVertex vertex;

    if (begin_mode == NXGL_NO_BEGIN_MODE || index < 0 || pending_count >= (int)(sizeof(pending) / sizeof(pending[0]))) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    vertex = read_array_vertex(index);
    pending[pending_count++] = vertex;
}

static bool validate_array_draw(GLenum mode, GLsizei count)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return false;
    }
    if (!vertex_array.enabled || vertex_array.pointer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (mode != GL_POINTS &&
        mode != GL_LINES &&
        mode != GL_LINE_LOOP &&
        mode != GL_LINE_STRIP &&
        mode != GL_TRIANGLES &&
        mode != GL_TRIANGLE_STRIP &&
        mode != GL_TRIANGLE_FAN &&
        mode != GL_QUADS &&
        mode != GL_QUAD_STRIP &&
        mode != GL_POLYGON) {
        set_error(GL_INVALID_ENUM);
        return false;
    }
    return true;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLint *indices;

    if (!validate_array_draw(mode, count)) {
        return;
    }
    if (first < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (count == 0) {
        return;
    }

    ++perf_counters.array_draws;
    indices = (GLint *)malloc((size_t)count * sizeof(GLint));
    if (indices == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        indices[i] = first + i;
    }
    emit_array_vertices(mode, indices, count);
    free(indices);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    GLint *expanded;

    if (!validate_array_draw(mode, count)) {
        return;
    }
    if (indices == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (count == 0) {
        return;
    }

    ++perf_counters.element_draws;
    expanded = (GLint *)malloc((size_t)count * sizeof(GLint));
    if (expanded == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        if (type == GL_UNSIGNED_BYTE) {
            expanded[i] = ((const GLubyte *)indices)[i];
        } else if (type == GL_UNSIGNED_SHORT) {
            expanded[i] = ((const GLushort *)indices)[i];
        } else {
            expanded[i] = (GLint)((const GLuint *)indices)[i];
        }
    }
    emit_array_vertices(mode, expanded, count);
    free(expanded);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
    if (end < start) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glDrawElements(mode, count, type, indices);
}

static void execute_list_command(const ListCommand *command)
{
    switch (command->type) {
    case LIST_CMD_CLEAR_COLOR:
        glClearColor(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_CLEAR_INDEX:
        glClearIndex(command->f[0]);
        break;
    case LIST_CMD_CLEAR_ACCUM:
        glClearAccum(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_CLEAR_DEPTH:
        glClearDepth(command->f[0]);
        break;
    case LIST_CMD_DEPTH_RANGE:
        glDepthRange(command->f[0], command->f[1]);
        break;
    case LIST_CMD_CLEAR_STENCIL:
        glClearStencil((GLint)command->u);
        break;
    case LIST_CMD_VIEWPORT:
        glViewport((GLint)command->f[0], (GLint)command->f[1], (GLsizei)command->f[2], (GLsizei)command->f[3]);
        break;
    case LIST_CMD_PASS_THROUGH:
        glPassThrough(command->f[0]);
        break;
    case LIST_CMD_CLEAR:
        glClear(command->bits);
        break;
    case LIST_CMD_ACCUM:
        glAccum(command->a, command->f[0]);
        break;
    case LIST_CMD_MATRIX_MODE:
        glMatrixMode(command->a);
        break;
    case LIST_CMD_LOAD_IDENTITY:
        glLoadIdentity();
        break;
    case LIST_CMD_LOAD_MATRIX:
        glLoadMatrixf(command->matrix);
        break;
    case LIST_CMD_MULT_MATRIX:
        glMultMatrixf(command->matrix);
        break;
    case LIST_CMD_PUSH_MATRIX:
        glPushMatrix();
        break;
    case LIST_CMD_POP_MATRIX:
        glPopMatrix();
        break;
    case LIST_CMD_TRANSLATE:
        glTranslatef(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_SCALE:
        glScalef(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_ROTATE:
        glRotatef(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_COLOR4:
        glColor4f(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_INDEX:
        glIndexf(command->f[0]);
        break;
    case LIST_CMD_NORMAL3:
        glNormal3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_TEXCOORD2:
        glTexCoord2f(command->f[0], command->f[1]);
        break;
    case LIST_CMD_TEXCOORD3:
        glTexCoord3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_BEGIN:
        glBegin(command->a);
        break;
    case LIST_CMD_VERTEX3:
        glVertex3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_END:
        glEnd();
        break;
    case LIST_CMD_POINT_SIZE:
        glPointSize(command->f[0]);
        break;
    case LIST_CMD_LINE_WIDTH:
        glLineWidth(command->f[0]);
        break;
    case LIST_CMD_LINE_STIPPLE:
        glLineStipple((GLint)command->f[0], (GLushort)command->u);
        break;
    case LIST_CMD_POLYGON_STIPPLE:
        glPolygonStipple(command->bytes);
        break;
    case LIST_CMD_POLYGON_MODE:
        glPolygonMode(command->a, command->b);
        break;
    case LIST_CMD_POLYGON_OFFSET:
        glPolygonOffset(command->f[0], command->f[1]);
        break;
    case LIST_CMD_ALPHA_FUNC:
        glAlphaFunc(command->a, command->f[0]);
        break;
    case LIST_CMD_COLOR_MASK:
        glColorMask(command->f[0] != 0.0f, command->f[1] != 0.0f, command->f[2] != 0.0f, command->f[3] != 0.0f);
        break;
    case LIST_CMD_LOGIC_OP:
        glLogicOp(command->a);
        break;
    case LIST_CMD_CULL_FACE:
        glCullFace(command->a);
        break;
    case LIST_CMD_FRONT_FACE:
        glFrontFace(command->a);
        break;
    case LIST_CMD_HINT:
        glHint(command->a, command->b);
        break;
    case LIST_CMD_EDGE_FLAG:
        glEdgeFlag((GLboolean)command->u);
        break;
    case LIST_CMD_PIXEL_ZOOM:
        glPixelZoom(command->f[0], command->f[1]);
        break;
    case LIST_CMD_PIXEL_TRANSFER:
        glPixelTransferf(command->a, command->f[0]);
        break;
    case LIST_CMD_PIXEL_MAP:
        glPixelMapfv(command->a, (GLsizei)command->u, command->f);
        break;
    case LIST_CMD_BITMAP: {
        GLint saved_unpack_alignment = unpack_alignment;
        GLint saved_unpack_row_length = unpack_row_length;
        GLint saved_unpack_skip_rows = unpack_skip_rows;
        GLint saved_unpack_skip_pixels = unpack_skip_pixels;
        GLboolean saved_unpack_swap_bytes = unpack_swap_bytes;
        GLboolean saved_unpack_lsb_first = unpack_lsb_first;
        unpack_alignment = command->i[0];
        unpack_row_length = command->i[1];
        unpack_skip_rows = command->i[2];
        unpack_skip_pixels = command->i[3];
        unpack_swap_bytes = (command->u & 1u) ? GL_TRUE : GL_FALSE;
        unpack_lsb_first = (command->u & 2u) ? GL_TRUE : GL_FALSE;
        glBitmap((GLsizei)command->f[0], (GLsizei)command->f[1],
                 command->f[2], command->f[3],
                 command->extra[0], command->extra[1],
                 (const GLubyte *)command->data);
        unpack_alignment = saved_unpack_alignment;
        unpack_row_length = saved_unpack_row_length;
        unpack_skip_rows = saved_unpack_skip_rows;
        unpack_skip_pixels = saved_unpack_skip_pixels;
        unpack_swap_bytes = saved_unpack_swap_bytes;
        unpack_lsb_first = saved_unpack_lsb_first;
        break;
    }
    case LIST_CMD_DRAW_PIXELS: {
        GLint saved_unpack_alignment = unpack_alignment;
        GLint saved_unpack_row_length = unpack_row_length;
        GLint saved_unpack_skip_rows = unpack_skip_rows;
        GLint saved_unpack_skip_pixels = unpack_skip_pixels;
        GLboolean saved_unpack_swap_bytes = unpack_swap_bytes;
        GLboolean saved_unpack_lsb_first = unpack_lsb_first;
        unpack_alignment = command->i[0];
        unpack_row_length = command->i[1];
        unpack_skip_rows = command->i[2];
        unpack_skip_pixels = command->i[3];
        unpack_swap_bytes = (command->u & 1u) ? GL_TRUE : GL_FALSE;
        unpack_lsb_first = (command->u & 2u) ? GL_TRUE : GL_FALSE;
        glDrawPixels((GLsizei)command->f[0], (GLsizei)command->f[1],
                     command->a, command->b, command->data);
        unpack_alignment = saved_unpack_alignment;
        unpack_row_length = saved_unpack_row_length;
        unpack_skip_rows = saved_unpack_skip_rows;
        unpack_skip_pixels = saved_unpack_skip_pixels;
        unpack_swap_bytes = saved_unpack_swap_bytes;
        unpack_lsb_first = saved_unpack_lsb_first;
        break;
    }
    case LIST_CMD_COPY_PIXELS:
        glCopyPixels((GLint)command->f[0], (GLint)command->f[1],
                     (GLsizei)command->f[2], (GLsizei)command->f[3], command->a);
        break;
    case LIST_CMD_SHADE_MODEL:
        glShadeModel(command->a);
        break;
    case LIST_CMD_LIGHT_MODEL:
        glLightModelfv(command->a, command->f);
        break;
    case LIST_CMD_LIGHT:
        glLightfv(command->a, command->b, command->f);
        break;
    case LIST_CMD_FOG:
        glFogfv(command->a, command->f);
        break;
    case LIST_CMD_CLIP_PLANE: {
        GLdouble equation[4] = { command->f[0], command->f[1], command->f[2], command->f[3] };
        glClipPlane(command->a, equation);
        break;
    }
    case LIST_CMD_RASTER_POS:
        glRasterPos4f(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_ENABLE:
        glEnable(command->a);
        break;
    case LIST_CMD_DISABLE:
        glDisable(command->a);
        break;
    case LIST_CMD_DEPTH_FUNC:
        glDepthFunc(command->a);
        break;
    case LIST_CMD_DEPTH_MASK:
        glDepthMask((GLboolean)command->u);
        break;
    case LIST_CMD_STENCIL_FUNC:
        glStencilFunc(command->a, (GLint)command->f[0], command->u);
        break;
    case LIST_CMD_STENCIL_OP:
        glStencilOp(command->a, command->b, command->u);
        break;
    case LIST_CMD_STENCIL_MASK:
        glStencilMask(command->u);
        break;
    case LIST_CMD_SCISSOR:
        glScissor((GLint)command->f[0], (GLint)command->f[1], (GLsizei)command->f[2], (GLsizei)command->f[3]);
        break;
    case LIST_CMD_BLEND_FUNC:
        glBlendFunc(command->a, command->b);
        break;
    case LIST_CMD_DRAW_BUFFER:
        glDrawBuffer(command->a);
        break;
    case LIST_CMD_READ_BUFFER:
        glReadBuffer(command->a);
        break;
    case LIST_CMD_ACTIVE_TEXTURE:
        glActiveTexture(command->a);
        break;
    case LIST_CMD_BIND_TEXTURE:
        glBindTexture(command->a, command->u);
        break;
    case LIST_CMD_TEX_PARAMETER:
        glTexParameteri(command->a, command->b, (GLint)command->u);
        break;
    case LIST_CMD_TEX_PARAMETER_F:
        glTexParameterf(command->a, command->b, command->f[0]);
        break;
    case LIST_CMD_TEX_ENV:
        glTexEnvfv(command->a, command->b, command->f);
        break;
    case LIST_CMD_TEX_GEN:
        if (command->b == GL_TEXTURE_GEN_MODE) {
            glTexGeni(command->a, command->b, (GLint)command->u);
        } else {
            glTexGenfv(command->a, command->b, command->f);
        }
        break;
    case LIST_CMD_PUSH_ATTRIB:
        glPushAttrib(command->bits);
        break;
    case LIST_CMD_POP_ATTRIB:
        glPopAttrib();
        break;
    case LIST_CMD_PUSH_CLIENT_ATTRIB:
        glPushClientAttrib(command->bits);
        break;
    case LIST_CMD_POP_CLIENT_ATTRIB:
        glPopClientAttrib();
        break;
    }
}

GLuint glGenLists(GLsizei range)
{
    if (range < 0) {
        set_error(GL_INVALID_VALUE);
        return 0;
    }
    if (range == 0 || range >= 256) {
        return 0;
    }

    for (GLuint first = 1; first + (GLuint)range <= 256; ++first) {
        bool free_range = true;
        for (GLsizei i = 0; i < range; ++i) {
            if (display_lists[first + (GLuint)i].allocated) {
                free_range = false;
                break;
            }
        }
        if (free_range) {
            for (GLsizei i = 0; i < range; ++i) {
                clear_display_list(&display_lists[first + (GLuint)i]);
                display_lists[first + (GLuint)i].allocated = true;
            }
            return first;
        }
    }
    return 0;
}

GLboolean glIsList(GLuint list)
{
    return list > 0 && list < 256 && display_lists[list].allocated ? GL_TRUE : GL_FALSE;
}

void glNewList(GLuint list, GLenum mode)
{
    if (list == 0 || list >= 256 || (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE)) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (recording_list != 0 || begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    clear_display_list(&display_lists[list]);
    display_lists[list].allocated = true;
    recording_list = list;
    recording_mode = mode;
}

void glEndList(void)
{
    if (recording_list == 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    recording_list = 0;
    recording_mode = 0;
}

void glCallList(GLuint list)
{
    DisplayList *display_list;
    bool was_replaying;

    if (list == 0 || list >= 256 || !display_lists[list].allocated) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    display_list = &display_lists[list];
    ++perf_counters.display_list_calls;
    was_replaying = replaying_list;
    replaying_list = true;
    for (size_t i = 0; i < display_list->count; ++i) {
        execute_list_command(&display_list->commands[i]);
    }
    replaying_list = was_replaying;
}

void glCallLists(GLsizei count, GLenum type, const GLvoid *lists)
{
    if (reject_inside_begin()) {
        return;
    }
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (lists == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint id;
        if (type == GL_UNSIGNED_BYTE) {
            id = list_base + ((const GLubyte *)lists)[i];
        } else if (type == GL_UNSIGNED_SHORT) {
            id = list_base + ((const GLushort *)lists)[i];
        } else {
            id = list_base + ((const GLuint *)lists)[i];
        }
        glCallList(id);
    }
}

void glDeleteLists(GLuint list, GLsizei range)
{
    if (range < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (list == 0 || range == 0) {
        return;
    }

    for (GLsizei i = 0; i < range; ++i) {
        GLuint id = list + (GLuint)i;
        if (id > 0 && id < 256) {
            clear_display_list(&display_lists[id]);
        }
    }
}

void glListBase(GLuint base)
{
    ListCommand command = { LIST_CMD_COLOR4 };
    (void)command;
    if (reject_inside_begin()) {
        return;
    }
    list_base = base;
}


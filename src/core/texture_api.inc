void glGenTextures(int count, uint32_t *out_textures)
{
    if (out_textures == NULL || count <= 0) {
        if (count < 0) {
            set_error(GL_INVALID_VALUE);
        }
        return;
    }

    for (int i = 0; i < count; ++i) {
        out_textures[i] = 0;
        for (int id = 1; id < 16; ++id) {
            if (!texture_objects[id].allocated) {
                init_texture_object(&texture_objects[id]);
                texture_objects[id].allocated = true;
                out_textures[i] = (uint32_t)id;
                break;
            }
        }
    }
}

void glDeleteTextures(GLsizei count, const GLuint *delete_textures)
{
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (delete_textures == NULL) {
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint id = delete_textures[i];
        if (id > 0 && id < 16) {
            destroy_texture_image(&texture_objects[id]);
            init_texture_object(&texture_objects[id]);
            for (int unit = 0; unit < 4; ++unit) {
                if (texture_binding_1d[unit] == id) {
                    texture_binding_1d[unit] = 0;
                }
                if (texture_binding_2d[unit] == id) {
                    texture_binding_2d[unit] = 0;
                }
                if (texture_binding_3d[unit] == id) {
                    texture_binding_3d[unit] = 0;
                }
                if (texture_binding_cube_map[unit] == id) {
                    texture_binding_cube_map[unit] = 0;
                }
            }
        }
    }
    sync_native_state();
}

GLboolean glIsTexture(GLuint texture)
{
    return texture > 0 && texture < 16 && texture_objects[texture].allocated ? GL_TRUE : GL_FALSE;
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
    if (n < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (textures == NULL || priorities == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < n; ++i) {
        GLuint id = textures[i];
        if (id > 0 && id < 16 && texture_objects[id].allocated) {
            texture_objects[id].priority = clamp_priority(priorities[i]);
        }
    }
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    GLboolean all_resident = GL_TRUE;

    if (n < 0) {
        set_error(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    if (textures == NULL || residences == NULL) {
        set_error(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    for (GLsizei i = 0; i < n; ++i) {
        GLuint id = textures[i];
        GLboolean resident = id > 0 && id < 16 && texture_objects[id].allocated ? GL_TRUE : GL_FALSE;
        residences[i] = resident;
        if (!resident) {
            all_resident = GL_FALSE;
        }
    }
    return all_resident;
}

void glBindTexture(uint32_t target, uint32_t texture)
{
    int unit = active_texture_index();

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture >= 16) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (texture > 0 && !texture_objects[texture].allocated) {
        init_texture_object(&texture_objects[texture]);
        texture_objects[texture].allocated = true;
    }
    ListCommand command = { LIST_CMD_BIND_TEXTURE };
    command.a = target;
    command.u = texture;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (unit >= 0) {
        if (target == GL_TEXTURE_1D) {
            texture_binding_1d[unit] = texture;
        } else if (target == GL_TEXTURE_2D) {
            texture_binding_2d[unit] = texture;
        } else if (target == GL_TEXTURE_3D) {
            texture_binding_3d[unit] = texture;
        } else {
            texture_binding_cube_map[unit] = texture;
        }
    }
    sync_native_state();
}

static int combine_source_index(GLenum pname, bool *alpha)
{
    if (pname >= GL_SOURCE0_RGB && pname <= GL_SOURCE2_RGB) {
        *alpha = false;
        return (int)(pname - GL_SOURCE0_RGB);
    }
    if (pname >= GL_SOURCE0_ALPHA && pname <= GL_SOURCE2_ALPHA) {
        *alpha = true;
        return (int)(pname - GL_SOURCE0_ALPHA);
    }
    return -1;
}

static int combine_operand_index(GLenum pname, bool *alpha)
{
    if (pname >= GL_OPERAND0_RGB && pname <= GL_OPERAND2_RGB) {
        *alpha = false;
        return (int)(pname - GL_OPERAND0_RGB);
    }
    if (pname >= GL_OPERAND0_ALPHA && pname <= GL_OPERAND2_ALPHA) {
        *alpha = true;
        return (int)(pname - GL_OPERAND0_ALPHA);
    }
    return -1;
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    int unit = active_texture_index();
    ListCommand command = { LIST_CMD_TEX_ENV };
    bool alpha = false;
    int index = -1;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        if (!valid_texture_env_mode((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        command.f[0] = params[0];
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else if (pname == GL_COMBINE_RGB || pname == GL_COMBINE_ALPHA) {
        if (!valid_combine_mode((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        if (!valid_combine_source((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        if (alpha) {
            if (!valid_combine_alpha_operand((GLint)params[0])) {
                set_error(GL_INVALID_ENUM);
                return;
            }
        } else if (!valid_combine_rgb_operand((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if (pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE) {
        if (!valid_combine_scale(params[0])) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        command.f[0] = params[0];
    } else {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = target;
    command.b = pname;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_TEXTURE_ENV_MODE) {
        texture_env_mode[unit] = (GLenum)((GLint)params[0]);
    } else {
        index = combine_source_index(pname, &alpha);
        if (index >= 0) {
            if (alpha) {
                texture_source_alpha[unit][index] = (GLenum)((GLint)params[0]);
            } else {
                texture_source_rgb[unit][index] = (GLenum)((GLint)params[0]);
            }
        } else {
            index = combine_operand_index(pname, &alpha);
            if (index >= 0) {
                if (alpha) {
                    texture_operand_alpha[unit][index] = (GLenum)((GLint)params[0]);
                } else {
                    texture_operand_rgb[unit][index] = (GLenum)((GLint)params[0]);
                }
            } else if (pname == GL_COMBINE_RGB) {
                texture_combine_rgb[unit] = (GLenum)((GLint)params[0]);
            } else if (pname == GL_COMBINE_ALPHA) {
                texture_combine_alpha[unit] = (GLenum)((GLint)params[0]);
            } else if (pname == GL_RGB_SCALE) {
                texture_rgb_scale[unit] = params[0];
            } else if (pname == GL_ALPHA_SCALE) {
                texture_alpha_scale[unit] = params[0];
            } else {
        texture_env_color[unit] = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
            }
        }
    }
    if (unit == 0) {
        sync_native_state();
    }
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    GLfloat values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        values[0] = (GLfloat)params[0];
        glTexEnvfv(target, pname, values);
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        values[0] = (GLfloat)params[0];
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
        glTexEnvfv(target, pname, values);
    } else {
        values[0] = (GLfloat)params[0];
        glTexEnvfv(target, pname, values);
    }
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    GLfloat value = (GLfloat)param;

    if (pname == GL_TEXTURE_ENV_COLOR) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glTexEnvfv(target, pname, &value);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    if (pname == GL_TEXTURE_ENV_COLOR) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glTexEnvfv(target, pname, &param);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    bool alpha = false;
    int index;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        params[0] = (GLint)texture_env_mode[unit];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        params[0] = (GLint)texture_env_color[unit].r;
        params[1] = (GLint)texture_env_color[unit].g;
        params[2] = (GLint)texture_env_color[unit].b;
        params[3] = (GLint)texture_env_color[unit].a;
    } else if (pname == GL_COMBINE_RGB) {
        params[0] = (GLint)texture_combine_rgb[unit];
    } else if (pname == GL_COMBINE_ALPHA) {
        params[0] = (GLint)texture_combine_alpha[unit];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        params[0] = (GLint)(alpha ? texture_source_alpha[unit][index] : texture_source_rgb[unit][index]);
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        params[0] = (GLint)(alpha ? texture_operand_alpha[unit][index] : texture_operand_rgb[unit][index]);
    } else if (pname == GL_RGB_SCALE) {
        params[0] = (GLint)texture_rgb_scale[unit];
    } else if (pname == GL_ALPHA_SCALE) {
        params[0] = (GLint)texture_alpha_scale[unit];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
    int unit = active_texture_index();
    bool alpha = false;
    int index;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        params[0] = (GLfloat)texture_env_mode[unit];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        params[0] = texture_env_color[unit].r;
        params[1] = texture_env_color[unit].g;
        params[2] = texture_env_color[unit].b;
        params[3] = texture_env_color[unit].a;
    } else if (pname == GL_COMBINE_RGB) {
        params[0] = (GLfloat)texture_combine_rgb[unit];
    } else if (pname == GL_COMBINE_ALPHA) {
        params[0] = (GLfloat)texture_combine_alpha[unit];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        params[0] = (GLfloat)(alpha ? texture_source_alpha[unit][index] : texture_source_rgb[unit][index]);
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        params[0] = (GLfloat)(alpha ? texture_operand_alpha[unit][index] : texture_operand_rgb[unit][index]);
    } else if (pname == GL_RGB_SCALE) {
        params[0] = texture_rgb_scale[unit];
    } else if (pname == GL_ALPHA_SCALE) {
        params[0] = texture_alpha_scale[unit];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

static bool valid_min_filter(GLint value)
{
    return value == GL_NEAREST ||
           value == GL_LINEAR ||
           value == GL_NEAREST_MIPMAP_NEAREST ||
           value == GL_LINEAR_MIPMAP_NEAREST ||
           value == GL_NEAREST_MIPMAP_LINEAR ||
           value == GL_LINEAR_MIPMAP_LINEAR;
}

static bool valid_mag_filter(GLint value)
{
    return value == GL_NEAREST || value == GL_LINEAR;
}

static bool valid_wrap(GLint value)
{
    return value == GL_CLAMP || value == GL_REPEAT || value == GL_CLAMP_TO_EDGE;
}

static bool valid_stencil_func(GLenum func)
{
    return func == GL_NEVER ||
           func == GL_LESS ||
           func == GL_EQUAL ||
           func == GL_LEQUAL ||
           func == GL_GREATER ||
           func == GL_NOTEQUAL ||
           func == GL_GEQUAL ||
           func == GL_ALWAYS;
}

static bool valid_stencil_op(GLenum op)
{
    return op == GL_KEEP ||
           op == GL_ZERO ||
           op == GL_REPLACE ||
           op == GL_INCR ||
           op == GL_DECR ||
           op == GL_INVERT;
}

static bool valid_depth_func(GLenum func)
{
    return valid_stencil_func(func);
}

static bool valid_blend_src_factor(GLenum factor)
{
    return factor == GL_ZERO ||
           factor == GL_ONE ||
           factor == GL_DST_COLOR ||
           factor == GL_ONE_MINUS_DST_COLOR ||
           factor == GL_SRC_ALPHA ||
           factor == GL_ONE_MINUS_SRC_ALPHA ||
           factor == GL_DST_ALPHA ||
           factor == GL_ONE_MINUS_DST_ALPHA ||
           factor == GL_SRC_ALPHA_SATURATE;
}

static bool valid_blend_dst_factor(GLenum factor)
{
    return factor == GL_ZERO ||
           factor == GL_ONE ||
           factor == GL_SRC_COLOR ||
           factor == GL_ONE_MINUS_SRC_COLOR ||
           factor == GL_SRC_ALPHA ||
           factor == GL_ONE_MINUS_SRC_ALPHA ||
           factor == GL_DST_ALPHA ||
           factor == GL_ONE_MINUS_DST_ALPHA;
}

static bool valid_logic_op(GLenum op)
{
    return op == GL_CLEAR ||
           op == GL_AND ||
           op == GL_AND_REVERSE ||
           op == GL_COPY ||
           op == GL_AND_INVERTED ||
           op == GL_NOOP ||
           op == GL_XOR ||
           op == GL_OR ||
           op == GL_NOR ||
           op == GL_EQUIV ||
           op == GL_INVERT ||
           op == GL_OR_REVERSE ||
           op == GL_COPY_INVERTED ||
           op == GL_OR_INVERTED ||
           op == GL_NAND ||
           op == GL_SET;
}

static bool valid_hint_target(GLenum target)
{
    return target == GL_PERSPECTIVE_CORRECTION_HINT ||
           target == GL_POINT_SMOOTH_HINT ||
           target == GL_LINE_SMOOTH_HINT ||
           target == GL_POLYGON_SMOOTH_HINT ||
           target == GL_FOG_HINT;
}

static bool valid_hint_mode(GLenum mode)
{
    return mode == GL_DONT_CARE ||
           mode == GL_FASTEST ||
           mode == GL_NICEST;
}

static bool valid_draw_buffer(GLenum mode)
{
    return mode == GL_NONE ||
           mode == GL_FRONT ||
           mode == GL_BACK ||
           mode == GL_FRONT_AND_BACK ||
           mode == GL_FRONT_LEFT ||
           mode == GL_FRONT_RIGHT ||
           mode == GL_BACK_LEFT ||
           mode == GL_BACK_RIGHT;
}

static bool valid_read_buffer(GLenum mode)
{
    return mode == GL_NONE ||
           mode == GL_FRONT ||
           mode == GL_BACK ||
           mode == GL_FRONT_LEFT ||
           mode == GL_FRONT_RIGHT ||
           mode == GL_BACK_LEFT ||
           mode == GL_BACK_RIGHT;
}

static bool is_mipmap_filter(GLint value)
{
    return value == GL_NEAREST_MIPMAP_NEAREST ||
           value == GL_LINEAR_MIPMAP_NEAREST ||
           value == GL_NEAREST_MIPMAP_LINEAR ||
           value == GL_LINEAR_MIPMAP_LINEAR;
}

static GLfloat clamp_priority(GLfloat value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static int mip_chain_end_level(GLsizei width, GLsizei height, GLsizei depth, GLint base_level, GLint max_level)
{
    int level = base_level;
    while ((width > 1 || height > 1 || depth > 1) && level + 1 < NXGL_MAX_TEXTURE_LEVELS && level < max_level) {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        depth = depth > 1 ? depth / 2 : 1;
        ++level;
    }
    return level;
}

static bool texture_levels_complete(const TextureObject *texture, const TextureLevel *levels)
{
    GLint base = texture->base_level;
    GLint max = texture->max_level;
    GLsizei expected_w;
    GLsizei expected_h;
    GLsizei expected_d;
    GLint end;

    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return false;
    }
    if (!levels[base].defined) {
        return false;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return true;
    }

    expected_w = levels[base].width;
    expected_h = levels[base].height;
    expected_d = levels[base].depth;
    end = mip_chain_end_level(expected_w, expected_h, expected_d, base, max);

    for (GLint level = base; level <= end; ++level) {
        if (!levels[level].defined ||
            levels[level].width != expected_w ||
            levels[level].height != expected_h ||
            levels[level].depth != expected_d ||
            levels[level].internal_format != levels[base].internal_format) {
            return false;
        }
        expected_w = expected_w > 1 ? expected_w / 2 : 1;
        expected_h = expected_h > 1 ? expected_h / 2 : 1;
        expected_d = expected_d > 1 ? expected_d / 2 : 1;
    }
    return true;
}

static bool texture_complete(const TextureObject *texture)
{
    return texture_levels_complete(texture, texture->levels);
}

static bool texture_1d_complete(const TextureObject *texture)
{
    return texture_levels_complete(texture, texture->levels_1d);
}

static int source_components(GLenum format)
{
    if (format == GL_COLOR_INDEX) {
        return 1;
    }
    if (format == GL_RGB || format == GL_BGR) {
        return 3;
    }
    if (format == GL_RGBA || format == GL_BGRA) {
        return 4;
    }
    return 0;
}

static size_t source_element_size(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
        return 1;
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        return 2;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
        return 4;
    default:
        return 0;
    }
}

static bool valid_pixel_type(GLenum type)
{
    return source_element_size(type) != 0;
}

static uint8_t expand_bits(uint32_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return (uint8_t)((value * 255u + max / 2u) / max);
}

static uint32_t pack_bits(uint8_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return ((uint32_t)value * max + 127u) / 255u;
}

static uint16_t read_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static uint32_t read_u32(const uint8_t *src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static void write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static bool unpack_pixel(const uint8_t *src, GLenum format, GLenum type, uint8_t *rgba)
{
    uint32_t v;

    if (format == GL_COLOR_INDEX) {
        GLuint index;

        if (type == GL_UNSIGNED_BYTE) {
            index = src[0];
        } else if (type == GL_UNSIGNED_SHORT) {
            index = read_u16(src);
        } else if (type == GL_UNSIGNED_INT) {
            index = read_u32(src);
        } else {
            return false;
        }
        index = map_integer_value(index, pixel_map_index(GL_PIXEL_MAP_I_TO_I), 255u);
        rgba[0] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_R)));
        rgba[1] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_G)));
        rgba[2] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_B)));
        rgba[3] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_A)));
        return true;
    }

    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGBA) {
            rgba[0] = src[0]; rgba[1] = src[1]; rgba[2] = src[2]; rgba[3] = src[3];
            return true;
        }
        if (format == GL_RGB) {
            rgba[0] = src[0]; rgba[1] = src[1]; rgba[2] = src[2]; rgba[3] = 255;
            return true;
        }
        if (format == GL_BGRA) {
            rgba[0] = src[2]; rgba[1] = src[1]; rgba[2] = src[0]; rgba[3] = src[3];
            return true;
        }
        if (format == GL_BGR) {
            rgba[0] = src[2]; rgba[1] = src[1]; rgba[2] = src[0]; rgba[3] = 255;
            return true;
        }
        return false;
    }

    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = 255;
    if (type == GL_UNSIGNED_BYTE_3_3_2) {
        if (format != GL_RGB) return false;
        v = src[0];
        rgba[0] = expand_bits((v >> 5) & 0x7, 3);
        rgba[1] = expand_bits((v >> 2) & 0x7, 3);
        rgba[2] = expand_bits(v & 0x3, 2);
    } else if (type == GL_UNSIGNED_BYTE_2_3_3_REV) {
        if (format != GL_RGB) return false;
        v = src[0];
        rgba[0] = expand_bits(v & 0x7, 3);
        rgba[1] = expand_bits((v >> 3) & 0x7, 3);
        rgba[2] = expand_bits((v >> 6) & 0x3, 2);
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format != GL_RGB) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 11) & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x3f, 6);
        rgba[2] = expand_bits(v & 0x1f, 5);
    } else if (type == GL_UNSIGNED_SHORT_5_6_5_REV) {
        if (format != GL_RGB) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x3f, 6);
        rgba[2] = expand_bits((v >> 11) & 0x1f, 5);
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 12) & 0xf, 4);
        rgba[1] = expand_bits((v >> 8) & 0xf, 4);
        rgba[2] = expand_bits((v >> 4) & 0xf, 4);
        rgba[3] = expand_bits(v & 0xf, 4);
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0xf, 4);
        rgba[1] = expand_bits((v >> 4) & 0xf, 4);
        rgba[2] = expand_bits((v >> 8) & 0xf, 4);
        rgba[3] = expand_bits((v >> 12) & 0xf, 4);
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 11) & 0x1f, 5);
        rgba[1] = expand_bits((v >> 6) & 0x1f, 5);
        rgba[2] = expand_bits((v >> 1) & 0x1f, 5);
        rgba[3] = (v & 0x1) ? 255 : 0;
    } else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x1f, 5);
        rgba[2] = expand_bits((v >> 10) & 0x1f, 5);
        rgba[3] = (v & 0x8000) ? 255 : 0;
    } else if (type == GL_UNSIGNED_INT_8_8_8_8) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = (uint8_t)((v >> 24) & 0xff);
        rgba[1] = (uint8_t)((v >> 16) & 0xff);
        rgba[2] = (uint8_t)((v >> 8) & 0xff);
        rgba[3] = (uint8_t)(v & 0xff);
    } else if (type == GL_UNSIGNED_INT_8_8_8_8_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = (uint8_t)(v & 0xff);
        rgba[1] = (uint8_t)((v >> 8) & 0xff);
        rgba[2] = (uint8_t)((v >> 16) & 0xff);
        rgba[3] = (uint8_t)((v >> 24) & 0xff);
    } else if (type == GL_UNSIGNED_INT_10_10_10_2) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = expand_bits((v >> 22) & 0x3ff, 10);
        rgba[1] = expand_bits((v >> 12) & 0x3ff, 10);
        rgba[2] = expand_bits((v >> 2) & 0x3ff, 10);
        rgba[3] = expand_bits(v & 0x3, 2);
    } else if (type == GL_UNSIGNED_INT_2_10_10_10_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = expand_bits(v & 0x3ff, 10);
        rgba[1] = expand_bits((v >> 10) & 0x3ff, 10);
        rgba[2] = expand_bits((v >> 20) & 0x3ff, 10);
        rgba[3] = expand_bits((v >> 30) & 0x3, 2);
    } else {
        return false;
    }

    if (format == GL_BGRA || format == GL_BGR) {
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
    }
    return true;
}

static bool pack_pixel(uint8_t *dst, const uint8_t *rgba, GLenum format, GLenum type)
{
    uint8_t r = rgba[0];
    uint8_t g = rgba[1];
    uint8_t b = rgba[2];
    uint8_t a = rgba[3];
    uint32_t v;

    if (format == GL_BGRA || format == GL_BGR) {
        uint8_t tmp = r;
        r = b;
        b = tmp;
    }

    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGBA || format == GL_BGRA) {
            dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
            return true;
        }
        if (format == GL_RGB || format == GL_BGR) {
            dst[0] = r; dst[1] = g; dst[2] = b;
            return true;
        }
        return false;
    }

    if (type == GL_UNSIGNED_BYTE_3_3_2) {
        if (format != GL_RGB) return false;
        dst[0] = (uint8_t)((pack_bits(r, 3) << 5) | (pack_bits(g, 3) << 2) | pack_bits(b, 2));
    } else if (type == GL_UNSIGNED_BYTE_2_3_3_REV) {
        if (format != GL_RGB) return false;
        dst[0] = (uint8_t)(pack_bits(r, 3) | (pack_bits(g, 3) << 3) | (pack_bits(b, 2) << 6));
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format != GL_RGB) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 5) << 11) | (pack_bits(g, 6) << 5) | pack_bits(b, 5)));
    } else if (type == GL_UNSIGNED_SHORT_5_6_5_REV) {
        if (format != GL_RGB) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 5) | (pack_bits(g, 6) << 5) | (pack_bits(b, 5) << 11)));
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 4) << 12) | (pack_bits(g, 4) << 8) | (pack_bits(b, 4) << 4) | pack_bits(a, 4)));
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 4) | (pack_bits(g, 4) << 4) | (pack_bits(b, 4) << 8) | (pack_bits(a, 4) << 12)));
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 5) << 11) | (pack_bits(g, 5) << 6) | (pack_bits(b, 5) << 1) | (a >= 128 ? 1 : 0)));
    } else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 5) | (pack_bits(g, 5) << 5) | (pack_bits(b, 5) << 10) | (a >= 128 ? 0x8000 : 0)));
    } else if (type == GL_UNSIGNED_INT_8_8_8_8) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u32(dst, ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a);
    } else if (type == GL_UNSIGNED_INT_8_8_8_8_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u32(dst, (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24));
    } else if (type == GL_UNSIGNED_INT_10_10_10_2) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = (pack_bits(r, 10) << 22) | (pack_bits(g, 10) << 12) | (pack_bits(b, 10) << 2) | pack_bits(a, 2);
        write_u32(dst, v);
    } else if (type == GL_UNSIGNED_INT_2_10_10_10_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = pack_bits(r, 10) | (pack_bits(g, 10) << 10) | (pack_bits(b, 10) << 20) | (pack_bits(a, 2) << 30);
        write_u32(dst, v);
    } else {
        return false;
    }
    return true;
}

static bool convert_to_rgba(uint8_t *dst, const uint8_t *src, GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t src_stride;

    if (comps == 0 || elem_size == 0) {
        return false;
    }

    if (src == NULL) {
        memset(dst, 0, (size_t)width * (size_t)height * 4);
        return true;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    src_stride = pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
    src += (size_t)unpack_skip_rows * src_stride + (size_t)unpack_skip_pixels * pixel_bytes;
    for (GLsizei y = 0; y < height; ++y) {
        const uint8_t *src_row = src + (size_t)y * src_stride;
        uint8_t *dst_row = dst + (size_t)y * (size_t)width * 4;
        for (GLsizei x = 0; x < width; ++x) {
            const uint8_t *s = src_row + (size_t)x * pixel_bytes;
            uint8_t *d = dst_row + (size_t)x * 4;
            if (!unpack_pixel(s, format, type, d)) {
                return false;
            }
        }
    }
    return true;
}

static bool convert_to_rgba_3d(uint8_t *dst, const uint8_t *src, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t src_stride;
    size_t src_slice_stride;

    if (comps == 0 || elem_size == 0) {
        return false;
    }

    if (src == NULL) {
        memset(dst, 0, (size_t)width * (size_t)height * (size_t)depth * 4u);
        return true;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    src_stride = pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
    src_slice_stride = pixel_store_slice_stride(width, height, unpack_row_length, unpack_skip_rows, unpack_skip_pixels,
                                                unpack_image_height, pixel_bytes, unpack_alignment);
    src += (size_t)unpack_skip_images * src_slice_stride +
           (size_t)unpack_skip_rows * src_stride +
           (size_t)unpack_skip_pixels * pixel_bytes;
    for (GLsizei z = 0; z < depth; ++z) {
        for (GLsizei y = 0; y < height; ++y) {
            const uint8_t *src_row = src + (size_t)z * src_slice_stride + (size_t)y * src_stride;
            uint8_t *dst_row = dst + ((size_t)z * (size_t)height + (size_t)y) * (size_t)width * 4u;
            for (GLsizei x = 0; x < width; ++x) {
                const uint8_t *s = src_row + (size_t)x * pixel_bytes;
                uint8_t *d = dst_row + (size_t)x * 4u;
                if (!unpack_pixel(s, format, type, d)) {
                    return false;
                }
            }
        }
    }
    return true;
}

void glTexParameteri(uint32_t target, uint32_t pname, int value)
{
    TextureObject *texture;

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    ListCommand command = { LIST_CMD_TEX_PARAMETER };
    command.a = target;
    command.b = pname;
    command.u = (GLuint)value;

    if (pname == GL_TEXTURE_MIN_FILTER) {
        if (!valid_min_filter(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_filter = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_1D && !rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_3D && !rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        if (!valid_mag_filter(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->mag_filter = value;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_s = value;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_t = value;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_r = value;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        if (value < 0 || value >= NXGL_MAX_TEXTURE_LEVELS) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->base_level = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_1D && !rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_3D && !rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        if (value < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_level = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_lod = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_lod = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->lod_bias = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_PRIORITY) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->priority = clamp_priority((GLfloat)value);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat value)
{
    TextureObject *texture;
    ListCommand command = { LIST_CMD_TEX_PARAMETER_F };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.a = target;
    command.b = pname;
    command.f[0] = value;

    if (pname == GL_TEXTURE_MIN_FILTER ||
        pname == GL_TEXTURE_MAG_FILTER ||
        pname == GL_TEXTURE_WRAP_S ||
        pname == GL_TEXTURE_WRAP_T ||
        pname == GL_TEXTURE_WRAP_R ||
        pname == GL_TEXTURE_BASE_LEVEL ||
        pname == GL_TEXTURE_MAX_LEVEL) {
        glTexParameteri(target, pname, (GLint)value);
        return;
    }

    if (pname == GL_TEXTURE_MIN_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_lod = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_lod = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->lod_bias = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_PRIORITY) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->priority = clamp_priority(value);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_MIN_LOD ||
        pname == GL_TEXTURE_MAX_LOD ||
        pname == GL_TEXTURE_LOD_BIAS ||
        pname == GL_TEXTURE_PRIORITY) {
        glTexParameterf(target, pname, (GLfloat)params[0]);
    } else {
        glTexParameteri(target, pname, params[0]);
    }
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexParameterf(target, pname, params[0]);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
    TextureObject *texture;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0;
        return;
    }

    if (pname == GL_TEXTURE_MIN_FILTER) {
        params[0] = texture->min_filter;
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        params[0] = texture->mag_filter;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        params[0] = texture->wrap_s;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        params[0] = texture->wrap_t;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        params[0] = texture->wrap_r;
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        params[0] = (GLint)texture->min_lod;
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        params[0] = (GLint)texture->max_lod;
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        params[0] = (GLint)texture->lod_bias;
    } else if (pname == GL_TEXTURE_PRIORITY) {
        params[0] = (GLint)texture->priority;
    } else if (pname == GL_TEXTURE_RESIDENT) {
        params[0] = texture->allocated ? GL_TRUE : GL_FALSE;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        params[0] = texture->base_level;
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        params[0] = texture->max_level;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
    TextureObject *texture;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0.0f;
        return;
    }

    if (pname == GL_TEXTURE_MIN_FILTER) {
        params[0] = (GLfloat)texture->min_filter;
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        params[0] = (GLfloat)texture->mag_filter;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        params[0] = (GLfloat)texture->wrap_s;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        params[0] = (GLfloat)texture->wrap_t;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        params[0] = (GLfloat)texture->wrap_r;
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        params[0] = texture->min_lod;
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        params[0] = texture->max_lod;
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        params[0] = texture->lod_bias;
    } else if (pname == GL_TEXTURE_PRIORITY) {
        params[0] = texture->priority;
    } else if (pname == GL_TEXTURE_RESIDENT) {
        params[0] = texture->allocated ? 1.0f : 0.0f;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        params[0] = (GLfloat)texture->base_level;
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        params[0] = (GLfloat)texture->max_level;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    TextureObject *texture;
    TextureLevel *image;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        params[0] = 0;
        return;
    }

    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0;
        return;
    }
    image = texture_level_for_target(texture, target, level);

    if (pname == GL_TEXTURE_WIDTH) {
        params[0] = image != NULL && image->defined ? image->width : 0;
    } else if (pname == GL_TEXTURE_HEIGHT) {
        params[0] = image != NULL && image->defined ? image->height : 0;
    } else if (pname == GL_TEXTURE_DEPTH) {
        params[0] = image != NULL && image->defined ? image->depth : 0;
    } else if (pname == GL_TEXTURE_INTERNAL_FORMAT) {
        params[0] = image != NULL && image->defined ? image->internal_format : 0;
    } else if (pname == GL_TEXTURE_BORDER) {
        params[0] = 0;
    } else if (pname == GL_TEXTURE_COMPRESSED) {
        params[0] = image != NULL && image->defined && image->compressed ? GL_TRUE : GL_FALSE;
    } else if (pname == GL_TEXTURE_COMPRESSED_IMAGE_SIZE) {
        params[0] = image != NULL && image->defined && image->compressed ? image->compressed_size : 0;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    GLint value = 0;
    GLenum before;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetTexLevelParameteriv(target, level, pname, &value);
    if (last_error == GL_NO_ERROR) {
        params[0] = (GLfloat)value;
        last_error = before;
    } else {
        GLenum error = last_error;
        last_error = before;
        set_error(error);
        params[0] = 0.0f;
    }
}

void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (reject_inside_begin()) {
        return;
    }
    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(rgba, (const uint8_t *)pixels, width, 1, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    ++perf_counters.texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexImage2D(uint32_t target, int level, int internalformat, int width, int height, int border, uint32_t format, uint32_t type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(rgba, (const uint8_t *)pixels, width, height, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    ++perf_counters.texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    const uint8_t *rgba;
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t dst_stride;
    size_t dst_slice_stride;

    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL || image == NULL || !image->defined) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (image->compressed || image->rgba == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (elem_size == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    dst_stride = pixel_store_row_stride(image->width, pack_row_length, pack_skip_pixels, pixel_bytes, pack_alignment);
    dst_slice_stride = target == GL_TEXTURE_3D ?
        pixel_store_slice_stride(image->width, image->height, pack_row_length, pack_skip_rows, pack_skip_pixels,
                                 pack_image_height, pixel_bytes, pack_alignment) :
        dst_stride * (size_t)image->height;
    rgba = image->rgba;
    pixels = (uint8_t *)pixels +
             (target == GL_TEXTURE_3D ? (size_t)pack_skip_images * dst_slice_stride : 0u) +
             (size_t)pack_skip_rows * dst_stride +
             (size_t)pack_skip_pixels * pixel_bytes;
    for (GLsizei z = 0; z < image->depth; ++z) {
        for (GLsizei y = 0; y < image->height; ++y) {
            uint8_t *dst_row = (uint8_t *)pixels + (size_t)z * dst_slice_stride + (size_t)y * dst_stride;
            const uint8_t *src_row = rgba + ((size_t)z * (size_t)image->height + (size_t)y) * (size_t)image->width * 4u;
            for (GLsizei x = 0; x < image->width; ++x) {
                uint8_t *dst = dst_row + (size_t)x * pixel_bytes;
                const uint8_t *src = src_row + (size_t)x * 4u;
                if (!pack_pixel(dst, src, format, type)) {
                    set_error(GL_INVALID_ENUM);
                    return;
                }
            }
        }
    }
}

void glGetCompressedTexImage(GLenum target, GLint level, GLvoid *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;

    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL || image == NULL || !image->defined || !image->compressed || image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    memcpy(pixels, image->compressed_data, (size_t)image->compressed_size);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *converted;
    int comps;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        width < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * 4u);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(converted, (const uint8_t *)pixels, width, 1, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }
    ++perf_counters.texture_sub_uploads;
    memcpy(image->rgba + (size_t)xoffset * 4u, converted, (size_t)width * 4u);
    free(converted);

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *level_rgba;
    uint8_t *converted;
    int comps;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(converted, (const uint8_t *)pixels, width, height, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }
    level_rgba = image->rgba;
    for (GLsizei y = 0; y < height; ++y) {
        uint8_t *dst = level_rgba + (((size_t)yoffset + (size_t)y) * (size_t)image->width + (size_t)xoffset) * 4;
        const uint8_t *src = converted + (size_t)y * (size_t)width * 4;
        memcpy(dst, src, (size_t)width * 4);
    }
    ++perf_counters.texture_sub_uploads;
    free(converted);

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (internalformat != GL_RGB && internalformat != GL_RGBA) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(rgba, x, y, width, height)) {
        free(rgba);
        return;
    }

    ++perf_counters.texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = false;

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (internalformat != GL_RGB && internalformat != GL_RGBA) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(rgba, x, y, width, 1)) {
        free(rgba);
        return;
    }

    ++perf_counters.texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, height)) {
        free(copy);
        return;
    }

    for (GLsizei row = 0; row < height; ++row) {
        uint8_t *dst = image->rgba + (((size_t)yoffset + (size_t)row) * (size_t)image->width + (size_t)xoffset) * 4u;
        const uint8_t *src = copy + (size_t)row * (size_t)width * 4u;
        memcpy(dst, src, (size_t)width * 4u);
    }
    ++perf_counters.texture_sub_uploads;
    free(copy);

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        width < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, 1)) {
        free(copy);
        return;
    }
    ++perf_counters.texture_sub_uploads;
    memcpy(image->rgba + (size_t)xoffset * 4u, copy, (size_t)width * 4u);
    free(copy);

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        depth <= 0 ||
        width > 256 ||
        height > 256 ||
        depth > 256) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * (size_t)depth * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba_3d(rgba, (const uint8_t *)pixels, width, height, depth, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    ++perf_counters.texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *level_rgba;
    uint8_t *converted;
    int comps;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        depth < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset + depth > image->depth) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0 || depth == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * (size_t)height * (size_t)depth * 4u);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba_3d(converted, (const uint8_t *)pixels, width, height, depth, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }

    level_rgba = image->rgba;
    for (GLsizei z = 0; z < depth; ++z) {
        for (GLsizei y = 0; y < height; ++y) {
            uint8_t *dst = level_rgba + (((size_t)zoffset + (size_t)z) * (size_t)image->height * (size_t)image->width +
                                          ((size_t)yoffset + (size_t)y) * (size_t)image->width +
                                          (size_t)xoffset) * 4u;
            const uint8_t *src = converted + ((size_t)z * (size_t)height + (size_t)y) * (size_t)width * 4u;
            memcpy(dst, src, (size_t)width * 4u);
        }
    }
    ++perf_counters.texture_sub_uploads;
    free(converted);

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset >= image->depth) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, height)) {
        free(copy);
        return;
    }

    for (GLsizei row = 0; row < height; ++row) {
        uint8_t *dst = image->rgba + ((size_t)zoffset * (size_t)image->height * (size_t)image->width +
                                      ((size_t)yoffset + (size_t)row) * (size_t)image->width +
                                      (size_t)xoffset) * 4u;
        const uint8_t *src = copy + (size_t)row * (size_t)width * 4u;
        memcpy(dst, src, (size_t)width * 4u);
    }
    ++perf_counters.texture_sub_uploads;
    free(copy);

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size(width, 1, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    ++perf_counters.compressed_texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    sync_native_state();
}

void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei src_blocks_w;
    GLsizei dst_block_x;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        width < 0 ||
        xoffset + width > image->width ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size(width, 1, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    src_blocks_w = (width + 3) / 4;
    dst_block_x = xoffset / 4;
    memcpy(image->compressed_data + (size_t)dst_block_x * (size_t)block_size,
           data,
           (size_t)src_blocks_w * (size_t)block_size);
    ++perf_counters.compressed_texture_sub_uploads;
    sync_native_state();
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size(width, height, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    ++perf_counters.compressed_texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_compressed_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei dst_blocks_w;
    GLsizei src_blocks_w;
    GLsizei src_blocks_h;
    GLsizei dst_block_x;
    GLsizei dst_block_y;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        (yoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width) ||
        ((height & 3) != 0 && yoffset + height != image->height)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size(width, height, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    dst_blocks_w = (image->width + 3) / 4;
    src_blocks_w = (width + 3) / 4;
    src_blocks_h = (height + 3) / 4;
    dst_block_x = xoffset / 4;
    dst_block_y = yoffset / 4;

    for (GLsizei row = 0; row < src_blocks_h; ++row) {
        uint8_t *dst = image->compressed_data + ((size_t)(dst_block_y + row) * (size_t)dst_blocks_w + (size_t)dst_block_x) * (size_t)block_size;
        const uint8_t *src = (const uint8_t *)data + (size_t)row * (size_t)src_blocks_w * (size_t)block_size;
        memcpy(dst, src, (size_t)src_blocks_w * (size_t)block_size);
    }
    ++perf_counters.compressed_texture_sub_uploads;
    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_compressed_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        depth <= 0 ||
        width > 256 ||
        height > 256 ||
        depth > 256) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size_3d(width, height, depth, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    ++perf_counters.compressed_texture_uploads;
    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    sync_native_state();
}

void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei dst_slice_size;
    GLsizei src_slice_size;
    GLsizei dst_blocks_w;
    GLsizei src_blocks_w;
    GLsizei src_blocks_h;
    GLsizei dst_block_x;
    GLsizei dst_block_y;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        depth < 0 ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset + depth > image->depth ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        (yoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width) ||
        ((height & 3) != 0 && yoffset + height != image->height)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size_3d(width, height, depth, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0 || depth == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    dst_blocks_w = (image->width + 3) / 4;
    src_blocks_w = (width + 3) / 4;
    src_blocks_h = (height + 3) / 4;
    dst_block_x = xoffset / 4;
    dst_block_y = yoffset / 4;
    dst_slice_size = compressed_image_size(image->width, image->height, format);
    src_slice_size = compressed_image_size(width, height, format);

    for (GLsizei slice = 0; slice < depth; ++slice) {
        for (GLsizei row = 0; row < src_blocks_h; ++row) {
            uint8_t *dst = image->compressed_data +
                           ((size_t)zoffset + (size_t)slice) * (size_t)dst_slice_size +
                           ((size_t)(dst_block_y + row) * (size_t)dst_blocks_w + (size_t)dst_block_x) * (size_t)block_size;
            const uint8_t *src = (const uint8_t *)data +
                                 (size_t)slice * (size_t)src_slice_size +
                                 (size_t)row * (size_t)src_blocks_w * (size_t)block_size;
            memcpy(dst, src, (size_t)src_blocks_w * (size_t)block_size);
        }
    }
    ++perf_counters.compressed_texture_sub_uploads;
    sync_native_state();
}

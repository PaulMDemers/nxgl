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


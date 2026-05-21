static uint8_t channel(float value)
{
    if (value < 0.0f) {
        value = 0.0f;
    }
    if (value > 1.0f) {
        value = 1.0f;
    }
    return (uint8_t)(value * 255.0f);
}

static uint32_t color_to_u32(NxglBackendColor color)
{
    return ((uint32_t)channel(color.a) << 24) |
           ((uint32_t)channel(color.r) << 16) |
           ((uint32_t)channel(color.g) << 8) |
           (uint32_t)channel(color.b);
}

static bool compare_alpha(GLfloat alpha, GLenum func, GLfloat ref)
{
    switch (func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return alpha < ref;
    case GL_EQUAL:
        return fabsf(alpha - ref) < 0.0001f;
    case GL_LEQUAL:
        return alpha <= ref || fabsf(alpha - ref) < 0.0001f;
    case GL_GREATER:
        return alpha > ref;
    case GL_NOTEQUAL:
        return fabsf(alpha - ref) >= 0.0001f;
    case GL_GEQUAL:
        return alpha >= ref || fabsf(alpha - ref) < 0.0001f;
    case GL_ALWAYS:
    default:
        return true;
    }
}

static uint32_t apply_logic_op(uint32_t src, uint32_t dst)
{
    uint32_t alpha = src & 0xff000000u;
    src &= 0x00ffffffu;
    dst &= 0x00ffffffu;
    switch (logic_op_mode) {
    case GL_CLEAR:
        return alpha;
    case GL_AND:
        return alpha | (src & dst);
    case GL_AND_REVERSE:
        return alpha | (src & ~dst);
    case GL_COPY:
        return alpha | src;
    case GL_AND_INVERTED:
        return alpha | (~src & dst);
    case GL_NOOP:
        return alpha | dst;
    case GL_XOR:
        return alpha | (src ^ dst);
    case GL_OR:
        return alpha | (src | dst);
    case GL_NOR:
        return alpha | (~(src | dst) & 0x00ffffffu);
    case GL_EQUIV:
        return alpha | (~(src ^ dst) & 0x00ffffffu);
    case GL_INVERT:
        return alpha | (~dst & 0x00ffffffu);
    case GL_OR_REVERSE:
        return alpha | (src | (~dst & 0x00ffffffu));
    case GL_COPY_INVERTED:
        return alpha | (~src & 0x00ffffffu);
    case GL_OR_INVERTED:
        return alpha | ((~src & 0x00ffffffu) | dst);
    case GL_NAND:
        return alpha | (~(src & dst) & 0x00ffffffu);
    case GL_SET:
        return alpha | 0x00ffffffu;
    default:
        return alpha | src;
    }
}

static GLfloat component_from_u32(uint32_t color, int component)
{
    uint32_t shift = component == 0 ? 16u : (component == 1 ? 8u : 0u);
    return (GLfloat)((color >> shift) & 0xffu) / 255.0f;
}

static GLfloat alpha_from_u32(uint32_t color)
{
    return (GLfloat)((color >> 24) & 0xffu) / 255.0f;
}

static GLfloat blend_factor_component(GLenum factor, int component, NxglBackendColor src, uint32_t dst)
{
    GLfloat src_component = component == 0 ? src.r : (component == 1 ? src.g : src.b);
    GLfloat dst_component = component_from_u32(dst, component);
    GLfloat dst_alpha = alpha_from_u32(dst);

    switch (factor) {
    case GL_ZERO:
        return 0.0f;
    case GL_ONE:
        return 1.0f;
    case GL_SRC_COLOR:
        return src_component;
    case GL_ONE_MINUS_SRC_COLOR:
        return 1.0f - src_component;
    case GL_DST_COLOR:
        return dst_component;
    case GL_ONE_MINUS_DST_COLOR:
        return 1.0f - dst_component;
    case GL_SRC_ALPHA:
        return src.a;
    case GL_ONE_MINUS_SRC_ALPHA:
        return 1.0f - src.a;
    case GL_DST_ALPHA:
        return dst_alpha;
    case GL_ONE_MINUS_DST_ALPHA:
        return 1.0f - dst_alpha;
    case GL_SRC_ALPHA_SATURATE:
        return fminf(src.a, 1.0f - dst_alpha);
    default:
        return 1.0f;
    }
}

static uint32_t apply_blend(NxglBackendColor src, uint32_t dst)
{
    NxglBackendColor out;

    if (!blend_enabled) {
        return color_to_u32(src);
    }

    out.r = clamp01(src.r * blend_factor_component(blend_sfactor, 0, src, dst) +
                    component_from_u32(dst, 0) * blend_factor_component(blend_dfactor, 0, src, dst));
    out.g = clamp01(src.g * blend_factor_component(blend_sfactor, 1, src, dst) +
                    component_from_u32(dst, 1) * blend_factor_component(blend_dfactor, 1, src, dst));
    out.b = clamp01(src.b * blend_factor_component(blend_sfactor, 2, src, dst) +
                    component_from_u32(dst, 2) * blend_factor_component(blend_dfactor, 2, src, dst));
    out.a = src.a;
    return color_to_u32(out);
}

static uint32_t apply_color_write_mask(uint32_t src, uint32_t dst)
{
    uint32_t out = dst;
    if (color_write_mask[0]) {
        out = (out & 0x0000ffffu) | (src & 0x00ff0000u);
    }
    if (color_write_mask[1]) {
        out = (out & 0x00ff00ffu) | (src & 0x0000ff00u);
    }
    if (color_write_mask[2]) {
        out = (out & 0x00ffff00u) | (src & 0x000000ffu);
    }
    if (color_write_mask[3]) {
        out = (out & 0x00ffffffu) | (src & 0xff000000u);
    }
    return out;
}

static bool compare_depth(GLfloat incoming, GLfloat current)
{
    if (!depth_test_enabled) {
        return true;
    }
    switch (depth_func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return incoming < current;
    case GL_EQUAL:
        return incoming == current;
    case GL_LEQUAL:
        return incoming <= current;
    case GL_GREATER:
        return incoming > current;
    case GL_NOTEQUAL:
        return incoming != current;
    case GL_GEQUAL:
        return incoming >= current;
    case GL_ALWAYS:
        return true;
    default:
        return incoming <= current;
    }
}

static bool compare_stencil(uint8_t value)
{
    GLuint lhs = ((GLuint)stencil_ref) & stencil_value_mask;
    GLuint rhs = ((GLuint)value) & stencil_value_mask;

    if (!stencil_test_enabled) {
        return true;
    }

    switch (stencil_func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return lhs < rhs;
    case GL_EQUAL:
        return lhs == rhs;
    case GL_LEQUAL:
        return lhs <= rhs;
    case GL_GREATER:
        return lhs > rhs;
    case GL_NOTEQUAL:
        return lhs != rhs;
    case GL_GEQUAL:
        return lhs >= rhs;
    case GL_ALWAYS:
    default:
        return true;
    }
}

static uint8_t apply_stencil_op(uint8_t current, GLenum op)
{
    switch (op) {
    case GL_KEEP:
        return current;
    case GL_ZERO:
        return 0;
    case GL_REPLACE:
        return (uint8_t)(stencil_ref & 0xff);
    case GL_INCR:
        return current == 0xff ? 0xff : (uint8_t)(current + 1u);
    case GL_DECR:
        return current == 0 ? 0 : (uint8_t)(current - 1u);
    case GL_INVERT:
        return (uint8_t)~current;
    default:
        return current;
    }
}

static void write_stencil_value(size_t index, uint8_t value)
{
    uint8_t mask = (uint8_t)(stencil_write_mask & 0xffu);
    uint8_t current;

    if (shadow_stencil_buffer == NULL) {
        return;
    }
    current = shadow_stencil_buffer[index];
    shadow_stencil_buffer[index] = (uint8_t)((current & (uint8_t)~mask) | (value & mask));
}

static bool shadow_clear_bounds(int *min_x, int *min_y, int *max_x, int *max_y)
{
    *min_x = 0;
    *min_y = 0;
    *max_x = shadow_width - 1;
    *max_y = shadow_height - 1;
    if (scissor_test_enabled) {
        int sx1 = scissor_box[0];
        int sy1 = shadow_height - (scissor_box[1] + scissor_box[3]);
        int sx2 = scissor_box[0] + scissor_box[2] - 1;
        int sy2 = shadow_height - scissor_box[1] - 1;
        if (sx1 < 0) sx1 = 0;
        if (sy1 < 0) sy1 = 0;
        if (sx2 >= shadow_width) sx2 = shadow_width - 1;
        if (sy2 >= shadow_height) sy2 = shadow_height - 1;
        if (*min_x < sx1) *min_x = sx1;
        if (*max_x > sx2) *max_x = sx2;
        if (*min_y < sy1) *min_y = sy1;
        if (*max_y > sy2) *max_y = sy2;
    }
    return *min_x <= *max_x && *min_y <= *max_y;
}

static void shadow_clear(uint32_t color)
{
    int min_x, min_y, max_x, max_y;
    if (!shadow_readback_enabled || shadow_color_buffer == NULL) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        uint32_t *row = shadow_color_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            row[x] = apply_color_write_mask(color, row[x]);
        }
    }
}

static void shadow_clear_depth(GLfloat depth)
{
    int min_x, min_y, max_x, max_y;

    if (!shadow_readback_enabled || shadow_depth_buffer == NULL || !depth_write_enabled) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        GLfloat *row = shadow_depth_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            row[x] = depth;
        }
    }
}

static void shadow_clear_stencil(uint8_t stencil)
{
    int min_x, min_y, max_x, max_y;
    uint8_t mask = (uint8_t)(stencil_write_mask & 0xffu);
    if (!shadow_readback_enabled || shadow_stencil_buffer == NULL) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y) || mask == 0u) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        uint8_t *row = shadow_stencil_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            uint8_t current = row[x];
            row[x] = (uint8_t)((current & (uint8_t)~mask) | (stencil & mask));
        }
    }
}

static bool ensure_accum_buffer(void)
{
    size_t count;

    if (accum_buffer != NULL) {
        return true;
    }

    count = (size_t)shadow_width * (size_t)shadow_height * 4u;
    accum_buffer = (float *)malloc(count * sizeof(float));
    if (accum_buffer == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return false;
    }
    memset(accum_buffer, 0, count * sizeof(float));
    return true;
}

static bool shadow_project(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * (float)viewport[2] * 0.5f + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)x;
    *sy = shadow_height - 1 - (int)y_bottom;
    return true;
}

static bool shadow_project_wide_x(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;
    float cube_x_scale;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    cube_x_scale = (float)viewport[2] * (27.0f / 32.0f);
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * cube_x_scale + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)x;
    *sy = shadow_height - 1 - (int)y_bottom;
    return true;
}

static bool shadow_project_rounded(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * (float)viewport[2] * 0.5f + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)floorf(x + 0.5f);
    *sy = shadow_height - 1 - (int)floorf(y_bottom + 0.5f);
    return true;
}

static bool cube_texture_enabled_for_shadow(void)
{
    for (int unit = 0; unit < 4; ++unit) {
        if (texture_cube_map_enabled[unit]) {
            return true;
        }
    }
    return false;
}

static GLfloat shadow_depth_value(const NxglBackendVertex *vertices, int count)
{
    GLfloat sum = 0.0f;

    if (count <= 0) {
        return 1.0f;
    }
    if (projected_path_active()) {
        for (int i = 0; i < count; ++i) {
            sum += vertices[i].window_z;
        }
        return clamp01(sum / (GLfloat)count);
    }
    for (int i = 0; i < count; ++i) {
        sum += -(vertices[i].pos.z + camera_z);
    }
    return map_depth_range(clamp01((sum / (GLfloat)count) / 100.0f));
}

static void set_raster_position_from_vertex4(float x, float y, float z, float w)
{
    NxglBackendVertex vertex;
    int sx = 0;
    int sy = 0;

    memset(&vertex, 0, sizeof(vertex));
    init_vertex_position(&vertex, x, y, z, w);
    if (!point_inside_clip_planes(vertex.eye)) {
        current_raster_position_valid = false;
        return;
    }
    if (projected_path_active()) {
        for (int plane = 0; plane < 6; ++plane) {
            if (feedback_clip_plane_value(&vertex, plane) < 0.0f) {
                current_raster_position_valid = false;
                return;
            }
        }
    }
    if (!shadow_project_rounded(vertex.pos, &sx, &sy)) {
        current_raster_position_valid = false;
        return;
    }

    current_raster_position[0] = (GLfloat)sx;
    current_raster_position[1] = (GLfloat)(shadow_height - 1 - sy);
    current_raster_position[2] = projected_path_active() ? vertex.window_z : map_depth_range(eye_depth_normalized(vertex.pos));
    current_raster_position[3] = projected_path_active() ? vertex.clip_w : 1.0f;
    current_raster_position_valid = sx >= viewport[0] &&
                                    sx < viewport[0] + viewport[2] &&
                                    current_raster_position[1] >= viewport[1] &&
                                    current_raster_position[1] < viewport[1] + viewport[3] &&
                                    sx >= 0 && sy >= 0 && sx < shadow_width && sy < shadow_height;
}

static void set_raster_position_from_vertex(float x, float y, float z)
{
    set_raster_position_from_vertex4(x, y, z, 1.0f);
}

static uint32_t pixel_from_source(const uint8_t *src, GLenum format)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (format == GL_RGBA || format == GL_RGB) {
        r = src[0];
        g = src[1];
        b = src[2];
    } else {
        b = src[0];
        g = src[1];
        r = src[2];
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static GLfloat map_component(GLfloat value, int map_index)
{
    PixelMapState *map;
    GLfloat position;
    int lo;
    int hi;
    GLfloat frac;

    if (map_index < 0 || map_index >= 10) {
        return value;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 1) {
        return map->values[0];
    }
    position = clamp01(value) * (GLfloat)(map->size - 1);
    lo = (int)floorf(position);
    hi = lo + 1;
    if (lo < 0) lo = 0;
    if (hi >= map->size) hi = map->size - 1;
    frac = position - (GLfloat)lo;
    return clamp01(map->values[lo] * (1.0f - frac) + map->values[hi] * frac);
}

static GLuint map_integer_value(GLuint value, int map_index, GLuint max_value)
{
    PixelMapState *map;
    GLuint index;

    if (map_index < 0 || map_index >= 10) {
        return value;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 0) {
        return value;
    }
    index = value;
    if (index >= (GLuint)map->size) {
        index = (GLuint)map->size - 1u;
    }
    return (GLuint)(clamp01(map->values[index]) * (GLfloat)max_value + 0.5f);
}

static GLfloat map_index_to_component(GLuint index, int map_index)
{
    PixelMapState *map;

    if (map_index < 0 || map_index >= 10) {
        return 0.0f;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 0) {
        return 0.0f;
    }
    if (index >= (GLuint)map->size) {
        index = (GLuint)map->size - 1u;
    }
    return clamp01(map->values[index]);
}

static NxglBackendColor apply_pixel_transfer_rgba_color(const uint8_t rgba[4])
{
    GLfloat channels[4] = {
        (GLfloat)rgba[0] / 255.0f,
        (GLfloat)rgba[1] / 255.0f,
        (GLfloat)rgba[2] / 255.0f,
        (GLfloat)rgba[3] / 255.0f
    };
    channels[0] = map_component(clamp01(channels[0] * pixel_transfer_scale[0] + pixel_transfer_bias[0]), pixel_map_index(GL_PIXEL_MAP_R_TO_R));
    channels[1] = map_component(clamp01(channels[1] * pixel_transfer_scale[1] + pixel_transfer_bias[1]), pixel_map_index(GL_PIXEL_MAP_G_TO_G));
    channels[2] = map_component(clamp01(channels[2] * pixel_transfer_scale[2] + pixel_transfer_bias[2]), pixel_map_index(GL_PIXEL_MAP_B_TO_B));
    channels[3] = map_component(clamp01(channels[3] * pixel_transfer_scale[3] + pixel_transfer_bias[3]), pixel_map_index(GL_PIXEL_MAP_A_TO_A));
    return (NxglBackendColor){ channels[0], channels[1], channels[2], channels[3] };
}

static uint32_t apply_pixel_transfer_rgba(const uint8_t rgba[4])
{
    NxglBackendColor color = apply_pixel_transfer_rgba_color(rgba);
    return color_to_u32(color);
}

static uint32_t apply_pixel_transfer_u32(uint32_t pixel)
{
    uint8_t rgba[4] = {
        (uint8_t)((pixel >> 16) & 0xff),
        (uint8_t)((pixel >> 8) & 0xff),
        (uint8_t)(pixel & 0xff),
        (uint8_t)((pixel >> 24) & 0xff)
    };
    return apply_pixel_transfer_rgba(rgba);
}

static uint8_t apply_pixel_transfer_stencil(uint8_t stencil)
{
    return (uint8_t)map_integer_value(stencil, pixel_map_index(GL_PIXEL_MAP_S_TO_S), 255u);
}

static int pixel_zoom_replicate(GLfloat factor)
{
    GLfloat abs_factor = factor < 0.0f ? -factor : factor;
    int replicate = (int)(abs_factor + 0.0001f);
    return replicate < 1 ? 1 : replicate;
}

static bool pixel_inside_scissor(GLint x, GLint y)
{
    if (!scissor_test_enabled) {
        return true;
    }
    return x >= scissor_box[0] &&
           y >= scissor_box[1] &&
           x < scissor_box[0] + scissor_box[2] &&
           y < scissor_box[1] + scissor_box[3];
}

static void write_shadow_color_fragment(GLint x, GLint y, NxglBackendColor color, GLfloat depth)
{
    uint32_t *dst;
    uint32_t src;
    size_t index;
    bool stencil_pass;
    bool depth_pass;

    if (draw_buffer_mode == GL_NONE || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    if (alpha_test_enabled && !compare_alpha(color.a, alpha_test_func, alpha_test_ref)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    stencil_pass = compare_stencil(shadow_stencil_buffer != NULL ? shadow_stencil_buffer[index] : 0);
    depth_pass = shadow_depth_buffer == NULL || compare_depth(depth, shadow_depth_buffer[index]);
    if (!stencil_pass) {
        if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
            write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_fail));
        }
        return;
    }
    if (!depth_pass) {
        if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
            write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zfail));
        }
        return;
    }
    if (depth_test_enabled && depth_write_enabled && shadow_depth_buffer != NULL) {
        shadow_depth_buffer[index] = depth;
    }
    if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
        write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zpass));
    }
    dst = shadow_color_buffer + index;
    src = color_logic_op_enabled ? apply_logic_op(color_to_u32(color), *dst) : apply_blend(color, *dst);
    *dst = apply_color_write_mask(src, *dst);
}

static void write_shadow_pixel(GLint x, GLint y, uint32_t color)
{
    NxglBackendColor src = {
        (GLfloat)((color >> 16) & 0xffu) / 255.0f,
        (GLfloat)((color >> 8) & 0xffu) / 255.0f,
        (GLfloat)(color & 0xffu) / 255.0f,
        alpha_from_u32(color)
    };
    write_shadow_color_fragment(x, y, src, current_raster_position[2]);
}

static bool valid_depth_pixel_type(GLenum type)
{
    return type == GL_FLOAT || type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT;
}

static bool valid_stencil_pixel_type(GLenum type)
{
    return type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT;
}

static GLfloat unpack_depth_pixel(const uint8_t *src, GLenum type)
{
    if (type == GL_FLOAT) {
        return clamp01(((const GLfloat *)src)[0]);
    }
    if (type == GL_UNSIGNED_BYTE) {
        return (GLfloat)src[0] / 255.0f;
    }
    if (type == GL_UNSIGNED_SHORT) {
        return (GLfloat)((const GLushort *)src)[0] / 65535.0f;
    }
    return (GLfloat)((const GLuint *)src)[0] / 4294967295.0f;
}

static uint8_t unpack_stencil_pixel(const uint8_t *src, GLenum type)
{
    if (type == GL_UNSIGNED_BYTE) {
        return src[0];
    }
    if (type == GL_UNSIGNED_SHORT) {
        return (uint8_t)(((const GLushort *)src)[0] & 0xffu);
    }
    return (uint8_t)(((const GLuint *)src)[0] & 0xffu);
}

static void write_shadow_depth_pixel(GLint x, GLint y, GLfloat depth)
{
    size_t index;

    if (shadow_depth_buffer == NULL || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    if (!depth_test_enabled || compare_depth(depth, shadow_depth_buffer[index])) {
        if (!depth_test_enabled || depth_write_enabled) {
            shadow_depth_buffer[index] = depth;
        }
    }
}

static void write_shadow_stencil_pixel(GLint x, GLint y, uint8_t stencil)
{
    size_t index;

    if (shadow_stencil_buffer == NULL || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    write_stencil_value(index, stencil);
}

static uint16_t dxt_read_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static uint8_t expand_bits_u8(uint32_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return (uint8_t)((value * 255u + max / 2u) / max);
}

static void decode_rgb565(uint16_t value, uint8_t out[4])
{
    out[0] = expand_bits_u8((value >> 11) & 0x1Fu, 5);
    out[1] = expand_bits_u8((value >> 5) & 0x3Fu, 6);
    out[2] = expand_bits_u8(value & 0x1Fu, 5);
    out[3] = 255;
}

static NxglBackendColor sample_dxt_color(const TextureLevel *image, int x, int y)
{
    int block_size = compressed_block_size((GLenum)image->internal_format);
    int blocks_w = (image->width + 3) / 4;
    int block_x = x / 4;
    int block_y = y / 4;
    int local = (y & 3) * 4 + (x & 3);
    const uint8_t *block = image->compressed_data + ((size_t)block_y * (size_t)blocks_w + (size_t)block_x) * (size_t)block_size;
    const uint8_t *color_block = block;
    uint8_t colors[4][4];
    uint16_t color0;
    uint16_t color1;
    uint32_t indices;
    int index;
    uint8_t alpha = 255;

    if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
        alpha = (uint8_t)((block[local / 2] >> ((local & 1) ? 4 : 0)) & 0x0F);
        alpha = (uint8_t)((alpha << 4) | alpha);
        color_block = block + 8;
    } else if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
        uint8_t alpha0 = block[0];
        uint8_t alpha1 = block[1];
        uint64_t alpha_bits = 0;
        int alpha_index;
        uint8_t alpha_values[8];
        for (int i = 0; i < 6; ++i) {
            alpha_bits |= (uint64_t)block[2 + i] << (8 * i);
        }
        alpha_index = (int)((alpha_bits >> (3 * local)) & 0x7u);
        alpha_values[0] = alpha0;
        alpha_values[1] = alpha1;
        if (alpha0 > alpha1) {
            alpha_values[2] = (uint8_t)((6 * alpha0 + alpha1) / 7);
            alpha_values[3] = (uint8_t)((5 * alpha0 + 2 * alpha1) / 7);
            alpha_values[4] = (uint8_t)((4 * alpha0 + 3 * alpha1) / 7);
            alpha_values[5] = (uint8_t)((3 * alpha0 + 4 * alpha1) / 7);
            alpha_values[6] = (uint8_t)((2 * alpha0 + 5 * alpha1) / 7);
            alpha_values[7] = (uint8_t)((alpha0 + 6 * alpha1) / 7);
        } else {
            alpha_values[2] = (uint8_t)((4 * alpha0 + alpha1) / 5);
            alpha_values[3] = (uint8_t)((3 * alpha0 + 2 * alpha1) / 5);
            alpha_values[4] = (uint8_t)((2 * alpha0 + 3 * alpha1) / 5);
            alpha_values[5] = (uint8_t)((alpha0 + 4 * alpha1) / 5);
            alpha_values[6] = 0;
            alpha_values[7] = 255;
        }
        alpha = alpha_values[alpha_index];
        color_block = block + 8;
    }

    color0 = dxt_read_u16(color_block);
    color1 = dxt_read_u16(color_block + 2);
    decode_rgb565(color0, colors[0]);
    decode_rgb565(color1, colors[1]);
    if (color0 > color1 || image->internal_format != GL_COMPRESSED_RGB_S3TC_DXT1_EXT) {
        for (int c = 0; c < 3; ++c) {
            colors[2][c] = (uint8_t)((2 * colors[0][c] + colors[1][c]) / 3);
            colors[3][c] = (uint8_t)((colors[0][c] + 2 * colors[1][c]) / 3);
        }
        colors[2][3] = 255;
        colors[3][3] = 255;
    } else {
        for (int c = 0; c < 3; ++c) {
            colors[2][c] = (uint8_t)((colors[0][c] + colors[1][c]) / 2);
            colors[3][c] = 0;
        }
        colors[2][3] = 255;
        colors[3][3] = 0;
    }

    indices = (uint32_t)color_block[4] |
              ((uint32_t)color_block[5] << 8) |
              ((uint32_t)color_block[6] << 16) |
              ((uint32_t)color_block[7] << 24);
    index = (int)((indices >> (2 * local)) & 0x3u);
    if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && color0 <= color1 && index == 3) {
        alpha = 0;
    }
    return (NxglBackendColor){
        (GLfloat)colors[index][0] / 255.0f,
        (GLfloat)colors[index][1] / 255.0f,
        (GLfloat)colors[index][2] / 255.0f,
        (GLfloat)alpha / 255.0f
    };
}

static NxglBackendColor sample_bound_texture_unit_color(int unit, GLfloat u, GLfloat v)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    int x;
    int y;

    if (unit < 0 || unit >= 4) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    if (texture_2d_enabled[unit] && texture_binding_2d[unit] > 0 && texture_binding_2d[unit] < 16) {
        texture = &texture_objects[texture_binding_2d[unit]];
        image = select_texture_level_for_lod(texture, texture->levels, 0.0f);
    } else if (texture_1d_enabled[unit] && texture_binding_1d[unit] > 0 && texture_binding_1d[unit] < 16) {
        texture = &texture_objects[texture_binding_1d[unit]];
        image = select_texture_level_for_lod(texture, texture->levels_1d, 0.0f);
        v = 0.0f;
    } else {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    if (!texture->allocated || image == NULL || !image->defined) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (texture->wrap_s == GL_REPEAT) {
        u = u - floorf(u);
    } else {
        u = clamp01(u);
    }
    if (texture->wrap_t == GL_REPEAT) {
        v = v - floorf(v);
    } else {
        v = clamp01(v);
    }

    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;

    if (image->compressed) {
        if (image->compressed_data == NULL) {
            return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        return sample_dxt_color(image, x, y);
    }
    if (image->rgba == NULL) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    pixel = image->rgba + ((size_t)y * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static NxglBackendColor sample_bound_cube_texture_unit_color(int unit, GLfloat s, GLfloat t, GLfloat r)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    GLfloat abs_s = fabsf(s);
    GLfloat abs_t = fabsf(t);
    GLfloat abs_r = fabsf(r);
    GLfloat ma;
    GLfloat sc;
    GLfloat tc;
    GLfloat u;
    GLfloat v;
    int face;
    int x;
    int y;

    if (unit < 0 || unit >= 4 || !texture_cube_map_enabled[unit] || texture_binding_cube_map[unit] == 0 || texture_binding_cube_map[unit] >= 16) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    texture = &texture_objects[texture_binding_cube_map[unit]];
    if (!texture->allocated || !cube_levels_complete(texture)) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (abs_s >= abs_t && abs_s >= abs_r) {
        ma = abs_s;
        if (s >= 0.0f) {
            face = 0;
            sc = -r;
            tc = -t;
        } else {
            face = 1;
            sc = r;
            tc = -t;
        }
    } else if (abs_t >= abs_s && abs_t >= abs_r) {
        ma = abs_t;
        if (t >= 0.0f) {
            face = 2;
            sc = s;
            tc = r;
        } else {
            face = 3;
            sc = s;
            tc = -r;
        }
    } else {
        ma = abs_r;
        if (r >= 0.0f) {
            face = 4;
            sc = s;
            tc = -t;
        } else {
            face = 5;
            sc = -s;
            tc = -t;
        }
    }

    if (ma <= 0.0f) {
        face = 4;
        u = 0.5f;
        v = 0.5f;
    } else {
        u = clamp01((sc / ma + 1.0f) * 0.5f);
        v = clamp01((tc / ma + 1.0f) * 0.5f);
    }

    {
        GLint level = select_cube_level_for_lod(texture, 0.0f);
        if (level < 0) {
            return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        image = &texture->cube_faces[face][level];
    }
    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;

    pixel = image->rgba + ((size_t)y * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static NxglBackendColor sample_bound_texture3d_unit_color(int unit, GLfloat u, GLfloat v, GLfloat r)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    int x;
    int y;
    int z;

    if (unit < 0 || unit >= 4 || !texture_3d_enabled[unit] || texture_binding_3d[unit] == 0 || texture_binding_3d[unit] >= 16) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    texture = &texture_objects[texture_binding_3d[unit]];
    image = select_texture_level_for_lod(texture, texture->levels_3d, 0.0f);
    if (!texture->allocated || image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (texture->wrap_s == GL_REPEAT) {
        u = u - floorf(u);
    } else {
        u = clamp01(u);
    }
    if (texture->wrap_t == GL_REPEAT) {
        v = v - floorf(v);
    } else {
        v = clamp01(v);
    }
    if (texture->wrap_r == GL_REPEAT) {
        r = r - floorf(r);
    } else {
        r = clamp01(r);
    }

    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    z = (int)(r * (GLfloat)(image->depth - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (z < 0) z = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;
    if (z >= image->depth) z = image->depth - 1;

    pixel = image->rgba + (((size_t)z * (size_t)image->height + (size_t)y) * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static bool texture_unit_enabled_for_shadow(int unit)
{
    if (unit < 0 || unit >= 4) {
        return false;
    }
    return texture_2d_enabled[unit] ||
           texture_1d_enabled[unit] ||
           texture_3d_enabled[unit] ||
           texture_cube_map_enabled[unit];
}

static NxglBackendColor sample_bound_texture_color_for_unit(int unit, GLfloat u, GLfloat v, GLfloat r)
{
    if (texture_cube_map_enabled[unit]) {
        return sample_bound_cube_texture_unit_color(unit, u, v, r);
    }
    if (texture_3d_enabled[unit]) {
        return sample_bound_texture3d_unit_color(unit, u, v, r);
    }
    return sample_bound_texture_unit_color(unit, u, v);
}

static NxglBackendColor combine_source_color(int unit, GLenum source, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                    const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    if (source == GL_TEXTURE) {
        return texel;
    }
    if (source >= GL_TEXTURE0 && source <= GL_TEXTURE3) {
        int source_unit = (int)(source - GL_TEXTURE0);
        return sample_bound_texture_color_for_unit(source_unit, u[source_unit], v[source_unit], r[source_unit]);
    }
    if (source == GL_CONSTANT) {
        return texture_env_color[unit];
    }
    if (source == GL_PRIMARY_COLOR) {
        return primary;
    }
    return previous;
}

static GLfloat combine_operand_component(NxglBackendColor source, GLenum operand, int component)
{
    GLfloat color_component = component == 0 ? source.r : (component == 1 ? source.g : source.b);

    if (operand == GL_SRC_ALPHA) {
        return source.a;
    }
    if (operand == GL_ONE_MINUS_SRC_ALPHA) {
        return 1.0f - source.a;
    }
    if (operand == GL_ONE_MINUS_SRC_COLOR) {
        return 1.0f - color_component;
    }
    return color_component;
}

static GLfloat combine_arg_component(int unit, int arg, int component, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                     const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor source = combine_source_color(unit, texture_source_rgb[unit][arg], primary, previous, texel, u, v, r);
    return combine_operand_component(source, texture_operand_rgb[unit][arg], component);
}

static GLfloat combine_alpha_arg_component(int unit, int arg, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                           const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor source = combine_source_color(unit, texture_source_alpha[unit][arg], primary, previous, texel, u, v, r);

    if (texture_operand_alpha[unit][arg] == GL_ONE_MINUS_SRC_ALPHA) {
        return 1.0f - source.a;
    }
    return source.a;
}

static GLfloat combine_alpha_value(int unit, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                   const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    GLfloat arg0 = combine_alpha_arg_component(unit, 0, primary, previous, texel, u, v, r);
    GLfloat arg1 = combine_alpha_arg_component(unit, 1, primary, previous, texel, u, v, r);
    GLfloat arg2 = combine_alpha_arg_component(unit, 2, primary, previous, texel, u, v, r);
    GLfloat value;

    if (texture_combine_alpha[unit] == GL_REPLACE) {
        value = arg0;
    } else if (texture_combine_alpha[unit] == GL_ADD) {
        value = arg0 + arg1;
    } else if (texture_combine_alpha[unit] == GL_ADD_SIGNED) {
        value = arg0 + arg1 - 0.5f;
    } else if (texture_combine_alpha[unit] == GL_SUBTRACT) {
        value = arg0 - arg1;
    } else if (texture_combine_alpha[unit] == GL_INTERPOLATE) {
        value = arg0 * arg2 + arg1 * (1.0f - arg2);
    } else {
        value = arg0 * arg1;
    }
    return clamp01(value * texture_alpha_scale[unit]);
}

static NxglBackendColor combined_texture_env_color(int unit, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                          const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor out = previous;
    GLfloat arg[3];

    if (texture_combine_rgb[unit] == GL_DOT3_RGB || texture_combine_rgb[unit] == GL_DOT3_RGBA) {
        GLfloat dot = 0.0f;
        for (int c = 0; c < 3; ++c) {
            GLfloat a = combine_arg_component(unit, 0, c, primary, previous, texel, u, v, r) - 0.5f;
            GLfloat b = combine_arg_component(unit, 1, c, primary, previous, texel, u, v, r) - 0.5f;
            dot += a * b;
        }
        dot = clamp01(dot * 4.0f);
        out.r = dot;
        out.g = dot;
        out.b = dot;
        out.a = texture_combine_rgb[unit] == GL_DOT3_RGBA ? dot : combine_alpha_value(unit, primary, previous, texel, u, v, r);
        return out;
    }

    for (int c = 0; c < 3; ++c) {
        arg[0] = combine_arg_component(unit, 0, c, primary, previous, texel, u, v, r);
        arg[1] = combine_arg_component(unit, 1, c, primary, previous, texel, u, v, r);
        arg[2] = combine_arg_component(unit, 2, c, primary, previous, texel, u, v, r);

        if (texture_combine_rgb[unit] == GL_REPLACE) {
            if (c == 0) out.r = arg[0];
            if (c == 1) out.g = arg[0];
            if (c == 2) out.b = arg[0];
        } else if (texture_combine_rgb[unit] == GL_ADD) {
            GLfloat value = clamp01(arg[0] + arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_ADD_SIGNED) {
            GLfloat value = clamp01(arg[0] + arg[1] - 0.5f);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_SUBTRACT) {
            GLfloat value = clamp01(arg[0] - arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_INTERPOLATE) {
            GLfloat value = clamp01(arg[0] * arg[2] + arg[1] * (1.0f - arg[2]));
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else {
            GLfloat value = clamp01(arg[0] * arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        }
    }

    out.r = clamp01(out.r * texture_rgb_scale[unit]);
    out.g = clamp01(out.g * texture_rgb_scale[unit]);
    out.b = clamp01(out.b * texture_rgb_scale[unit]);
    out.a = combine_alpha_value(unit, primary, previous, texel, u, v, r);
    return out;
}

static NxglBackendColor texture_env_color_for_shadow_unit(int unit, NxglBackendColor primary, NxglBackendColor previous,
                                                 const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor texel = sample_bound_texture_color_for_unit(unit, u[unit], v[unit], r[unit]);
    NxglBackendColor env = texture_env_color[unit];
    NxglBackendColor out = previous;

    if (!texture_unit_enabled_for_shadow(unit)) {
        return previous;
    }

    if (texture_env_mode[unit] == GL_COMBINE) {
        return combined_texture_env_color(unit, primary, previous, texel, u, v, r);
    }

    if (texture_env_mode[unit] == GL_REPLACE) {
        out = texel;
    } else if (texture_env_mode[unit] == GL_DECAL) {
        out.r = previous.r * (1.0f - texel.a) + texel.r * texel.a;
        out.g = previous.g * (1.0f - texel.a) + texel.g * texel.a;
        out.b = previous.b * (1.0f - texel.a) + texel.b * texel.a;
        out.a = previous.a;
    } else if (texture_env_mode[unit] == GL_BLEND) {
        out.r = previous.r * (1.0f - texel.r) + env.r * texel.r;
        out.g = previous.g * (1.0f - texel.g) + env.g * texel.g;
        out.b = previous.b * (1.0f - texel.b) + env.b * texel.b;
        out.a = previous.a * texel.a;
    } else if (texture_env_mode[unit] == GL_ADD) {
        out.r = clamp01(previous.r + texel.r);
        out.g = clamp01(previous.g + texel.g);
        out.b = clamp01(previous.b + texel.b);
        out.a = previous.a * texel.a;
    } else {
        out.r = previous.r * texel.r;
        out.g = previous.g * texel.g;
        out.b = previous.b * texel.b;
        out.a = previous.a * texel.a;
    }

    out.r = clamp01(out.r);
    out.g = clamp01(out.g);
    out.b = clamp01(out.b);
    out.a = clamp01(out.a);
    return out;
}

static void mark_crossbar_source_units(int unit, bool source_only[4])
{
    if (texture_env_mode[unit] != GL_COMBINE) {
        return;
    }
    for (int arg = 0; arg < 3; ++arg) {
        GLenum rgb_source = texture_source_rgb[unit][arg];
        GLenum alpha_source = texture_source_alpha[unit][arg];

        if (rgb_source >= GL_TEXTURE0 && rgb_source <= GL_TEXTURE3) {
            int source_unit = (int)(rgb_source - GL_TEXTURE0);
            if (source_unit > unit) {
                source_only[source_unit] = true;
            }
        }
        if (alpha_source >= GL_TEXTURE0 && alpha_source <= GL_TEXTURE3) {
            int source_unit = (int)(alpha_source - GL_TEXTURE0);
            if (source_unit > unit) {
                source_only[source_unit] = true;
            }
        }
    }
}

static NxglBackendColor texture_env_color_for_shadow(NxglBackendColor primary, const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor color = primary;
    bool source_only[4] = { false, false, false, false };

    for (int unit = 0; unit < 4; ++unit) {
        if (source_only[unit]) {
            continue;
        }
        color = texture_env_color_for_shadow_unit(unit, primary, color, u, v, r);
        mark_crossbar_source_units(unit, source_only);
    }
    return color;
}

static bool shadow_stipple_allows(int x, int y, bool line)
{
    if (line && line_stipple_enabled) {
        int repeat = line_stipple_factor < 1 ? 1 : line_stipple_factor;
        int bit = ((x + y) / repeat) & 15;
        if (((line_stipple_pattern >> bit) & 1u) == 0u) {
            return false;
        }
    }
    if (!line && polygon_stipple_enabled) {
        int sx = x & 31;
        int sy = y & 31;
        uint8_t mask = polygon_stipple_pattern[sy * 4 + sx / 8];
        if ((mask & (1u << (sx & 7))) == 0u) {
            return false;
        }
    }
    return true;
}

static void shadow_fill_bounds(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d, bool quad, bool line)
{
    int xs[4];
    int ys[4];
    int count = quad ? 4 : 3;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    NxglBackendColor base_color = quad ? d.color : c.color;
    GLfloat sample_u[4] = {
        quad ? (a.u + b.u + c.u + d.u) * 0.25f : (a.u + b.u + c.u) / 3.0f,
        quad ? (a.u1 + b.u1 + c.u1 + d.u1) * 0.25f : (a.u1 + b.u1 + c.u1) / 3.0f,
        quad ? (a.u2 + b.u2 + c.u2 + d.u2) * 0.25f : (a.u2 + b.u2 + c.u2) / 3.0f,
        quad ? (a.u3 + b.u3 + c.u3 + d.u3) * 0.25f : (a.u3 + b.u3 + c.u3) / 3.0f
    };
    GLfloat sample_v[4] = {
        quad ? (a.v + b.v + c.v + d.v) * 0.25f : (a.v + b.v + c.v) / 3.0f,
        quad ? (a.v1 + b.v1 + c.v1 + d.v1) * 0.25f : (a.v1 + b.v1 + c.v1) / 3.0f,
        quad ? (a.v2 + b.v2 + c.v2 + d.v2) * 0.25f : (a.v2 + b.v2 + c.v2) / 3.0f,
        quad ? (a.v3 + b.v3 + c.v3 + d.v3) * 0.25f : (a.v3 + b.v3 + c.v3) / 3.0f
    };
    GLfloat sample_r[4] = {
        quad ? (a.r + b.r + c.r + d.r) * 0.25f : (a.r + b.r + c.r) / 3.0f,
        quad ? (a.r1 + b.r1 + c.r1 + d.r1) * 0.25f : (a.r1 + b.r1 + c.r1) / 3.0f,
        quad ? (a.r2 + b.r2 + c.r2 + d.r2) * 0.25f : (a.r2 + b.r2 + c.r2) / 3.0f,
        quad ? (a.r3 + b.r3 + c.r3 + d.r3) * 0.25f : (a.r3 + b.r3 + c.r3) / 3.0f
    };
    NxglBackendColor shaded;
    GLfloat depth;
    NxglBackendVertex vertices[4] = { a, b, c, d };
    bool cube_shadow = cube_texture_enabled_for_shadow();

    if (!shadow_readback_enabled) {
        return;
    }

    if (quad) {
        base_color = (NxglBackendColor){
            (a.color.r + b.color.r + c.color.r + d.color.r) * 0.25f,
            (a.color.g + b.color.g + c.color.g + d.color.g) * 0.25f,
            (a.color.b + b.color.b + c.color.b + d.color.b) * 0.25f,
            (a.color.a + b.color.a + c.color.a + d.color.a) * 0.25f
        };
        if (lighting_enabled && light_model_local_viewer) {
            NxglBackendVec3 center_eye = {
                (a.eye.x + b.eye.x + c.eye.x + d.eye.x) * 0.25f,
                (a.eye.y + b.eye.y + c.eye.y + d.eye.y) * 0.25f,
                (a.eye.z + b.eye.z + c.eye.z + d.eye.z) * 0.25f
            };
            NxglBackendVec3 center_normal = normalize_vec3((NxglBackendVec3){
                (a.normal.x + b.normal.x + c.normal.x + d.normal.x) * 0.25f,
                (a.normal.y + b.normal.y + c.normal.y + d.normal.y) * 0.25f,
                (a.normal.z + b.normal.z + c.normal.z + d.normal.z) * 0.25f
            });
            NxglBackendColor center_base = {
                (a.base_color.r + b.base_color.r + c.base_color.r + d.base_color.r) * 0.25f,
                (a.base_color.g + b.base_color.g + c.base_color.g + d.base_color.g) * 0.25f,
                (a.base_color.b + b.base_color.b + c.base_color.b + d.base_color.b) * 0.25f,
                (a.base_color.a + b.base_color.a + c.base_color.a + d.base_color.a) * 0.25f
            };
            base_color = lit_color(center_base, center_normal, center_eye);
        }
    } else {
        base_color = (NxglBackendColor){
            (a.color.r + b.color.r + c.color.r) / 3.0f,
            (a.color.g + b.color.g + c.color.g) / 3.0f,
            (a.color.b + b.color.b + c.color.b) / 3.0f,
            (a.color.a + b.color.a + c.color.a) / 3.0f
        };
    }
    shaded = texture_env_color_for_shadow(base_color, sample_u, sample_v, sample_r);

    if (shadow_color_buffer == NULL) {
        return;
    }
    if (alpha_test_enabled && !compare_alpha(shaded.a, alpha_test_func, alpha_test_ref)) {
        return;
    }
    depth = shadow_depth_value(vertices, count);
    for (int i = 0; i < count; ++i) {
        bool projected = cube_shadow ? shadow_project_wide_x(vertices[i].pos, &xs[i], &ys[i]) :
                                       shadow_project(vertices[i].pos, &xs[i], &ys[i]);
        if (!projected) {
            return;
        }
    }

    min_x = max_x = xs[0];
    min_y = max_y = ys[0];
    for (int i = 1; i < count; ++i) {
        if (xs[i] < min_x) min_x = xs[i];
        if (xs[i] > max_x) max_x = xs[i];
        if (ys[i] < min_y) min_y = ys[i];
        if (ys[i] > max_y) max_y = ys[i];
    }
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= shadow_width) max_x = shadow_width - 1;
    if (max_y >= shadow_height) max_y = shadow_height - 1;
    if (scissor_test_enabled) {
        int sx1 = scissor_box[0];
        int sy1 = shadow_height - (scissor_box[1] + scissor_box[3]);
        int sx2 = scissor_box[0] + scissor_box[2] - 1;
        int sy2 = shadow_height - scissor_box[1] - 1;
        if (sx1 < 0) sx1 = 0;
        if (sy1 < 0) sy1 = 0;
        if (sx2 >= shadow_width) sx2 = shadow_width - 1;
        if (sy2 >= shadow_height) sy2 = shadow_height - 1;
        if (min_x < sx1) min_x = sx1;
        if (max_x > sx2) max_x = sx2;
        if (min_y < sy1) min_y = sy1;
        if (max_y > sy2) max_y = sy2;
    }
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    ++perf_counters.shadow_primitives;
    perf_counters.shadow_fragments += (uint32_t)((max_x - min_x + 1) * (max_y - min_y + 1));
    for (int y = min_y; y <= max_y; ++y) {
        uint32_t *row = shadow_color_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            size_t index = (size_t)y * (size_t)shadow_width + (size_t)x;
            bool stencil_pass = compare_stencil(shadow_stencil_buffer != NULL ? shadow_stencil_buffer[index] : 0);
            bool depth_pass = shadow_depth_buffer == NULL || compare_depth(depth, shadow_depth_buffer[index]);
            uint32_t dst = row[x];
            uint32_t blended = apply_blend(shaded, dst);
            uint32_t src = color_logic_op_enabled ? apply_logic_op(color_to_u32(shaded), dst) : blended;

            if (!shadow_stipple_allows(x, y, line)) {
                continue;
            }
            if (!stencil_pass) {
                if (stencil_test_enabled) {
                    write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_fail));
                }
                continue;
            }
            if (!depth_pass) {
                if (stencil_test_enabled) {
                    write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zfail));
                }
                continue;
            }
            if (depth_test_enabled && depth_write_enabled && shadow_depth_buffer != NULL) {
                shadow_depth_buffer[index] = depth;
            }
            if (stencil_test_enabled) {
                write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zpass));
            }
            if (draw_buffer_mode != GL_NONE) {
                row[x] = apply_color_write_mask(src, dst);
            }
        }
    }
}


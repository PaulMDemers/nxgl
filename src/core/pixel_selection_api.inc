void glPixelStorei(GLenum pname, GLint param)
{
    switch (pname) {
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        if (pname == GL_PACK_ALIGNMENT) {
            pack_alignment = param;
        } else {
            unpack_alignment = param;
        }
        return;
    case GL_PACK_ROW_LENGTH:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_row_length = param;
        return;
    case GL_PACK_SKIP_ROWS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_rows = param;
        return;
    case GL_PACK_SKIP_PIXELS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_pixels = param;
        return;
    case GL_PACK_IMAGE_HEIGHT:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_image_height = param;
        return;
    case GL_PACK_SKIP_IMAGES:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_images = param;
        return;
    case GL_PACK_SWAP_BYTES:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_swap_bytes = (GLboolean)param;
        return;
    case GL_PACK_LSB_FIRST:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_lsb_first = (GLboolean)param;
        return;
    case GL_UNPACK_ROW_LENGTH:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_row_length = param;
        return;
    case GL_UNPACK_SKIP_ROWS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_rows = param;
        return;
    case GL_UNPACK_SKIP_PIXELS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_pixels = param;
        return;
    case GL_UNPACK_IMAGE_HEIGHT:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_image_height = param;
        return;
    case GL_UNPACK_SKIP_IMAGES:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_images = param;
        return;
    case GL_UNPACK_SWAP_BYTES:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_swap_bytes = (GLboolean)param;
        return;
    case GL_UNPACK_LSB_FIRST:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_lsb_first = (GLboolean)param;
        return;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    ListCommand command = { LIST_CMD_PIXEL_ZOOM };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = xfactor;
    command.f[1] = yfactor;
    record_command(command);
    if (compile_only()) {
        return;
    }
    pixel_zoom_x = xfactor;
    pixel_zoom_y = yfactor;
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
    ListCommand command = { LIST_CMD_PIXEL_TRANSFER };
    int index = -1;
    bool scale = false;

    if (reject_inside_begin()) {
        return;
    }
    switch (pname) {
    case GL_RED_SCALE: index = 0; scale = true; break;
    case GL_GREEN_SCALE: index = 1; scale = true; break;
    case GL_BLUE_SCALE: index = 2; scale = true; break;
    case GL_ALPHA_SCALE: index = 3; scale = true; break;
    case GL_RED_BIAS: index = 0; break;
    case GL_GREEN_BIAS: index = 1; break;
    case GL_BLUE_BIAS: index = 2; break;
    case GL_ALPHA_BIAS: index = 3; break;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = pname;
    command.f[0] = param;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (scale) {
        pixel_transfer_scale[index] = param;
    } else {
        pixel_transfer_bias[index] = param;
    }
}

void glPixelTransferi(GLenum pname, GLint param)
{
    glPixelTransferf(pname, (GLfloat)param);
}

void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
    ListCommand command = { LIST_CMD_PIXEL_MAP };
    int index = pixel_map_index(map);

    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX || values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize <= 4) {
        command.a = map;
        command.u = (GLuint)mapsize;
        for (GLsizei i = 0; i < mapsize; ++i) {
            command.f[i] = values[i];
        }
        record_command(command);
    }
    if (compile_only()) {
        return;
    }
    pixel_maps[index].size = mapsize;
    for (GLsizei i = 0; i < mapsize; ++i) {
        pixel_maps[index].values[i] = values[i];
    }
}

void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
    GLfloat converted[NXGL_PIXEL_MAP_MAX];
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < mapsize; ++i) {
        converted[i] = (GLfloat)values[i] / 4294967295.0f;
    }
    glPixelMapfv(map, mapsize, converted);
}

void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
    GLfloat converted[NXGL_PIXEL_MAP_MAX];
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < mapsize; ++i) {
        converted[i] = (GLfloat)values[i] / 65535.0f;
    }
    glPixelMapfv(map, mapsize, converted);
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        values[i] = pixel_maps[index].values[i];
    }
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        GLfloat value = clamp01(pixel_maps[index].values[i]);
        values[i] = value >= 1.0f ? 0xffffffffu : (GLuint)((double)value * 4294967295.0 + 0.5);
    }
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        values[i] = (GLushort)(clamp01(pixel_maps[index].values[i]) * 65535.0f + 0.5f);
    }
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);
    EvalMap1 *map;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (index < 0 || map2) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    map = &eval_maps1[index];
    if (u1 == u2 || stride < map->components || order <= 0 || order > NXGL_MAX_EVAL_ORDER || points == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map->defined = true;
    map->u1 = u1;
    map->u2 = u2;
    map->order = order;
    for (GLint i = 0; i < order; ++i) {
        for (GLint c = 0; c < map->components; ++c) {
            map->points[i * map->components + c] = points[i * stride + c];
        }
    }
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    GLfloat converted[NXGL_MAX_EVAL_ORDER * 4];
    int comps = eval_map_components(target);

    if (points == NULL) {
        glMap1f(target, (GLfloat)u1, (GLfloat)u2, stride, order, NULL);
        return;
    }
    if (comps <= 0 || order <= 0 || order > NXGL_MAX_EVAL_ORDER || stride < comps) {
        glMap1f(target, (GLfloat)u1, (GLfloat)u2, stride, order, NULL);
        return;
    }
    for (GLint i = 0; i < order; ++i) {
        for (GLint c = 0; c < comps; ++c) {
            converted[i * comps + c] = (GLfloat)points[i * stride + c];
        }
    }
    glMap1f(target, (GLfloat)u1, (GLfloat)u2, comps, order, converted);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);
    EvalMap2 *map;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (index < 0 || !map2) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    map = &eval_maps2[index];
    if (u1 == u2 || v1 == v2 || ustride < map->components || vstride < ustride * uorder ||
        uorder <= 0 || uorder > NXGL_MAX_EVAL_ORDER || vorder <= 0 || vorder > NXGL_MAX_EVAL_ORDER || points == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map->defined = true;
    map->u1 = u1;
    map->u2 = u2;
    map->v1 = v1;
    map->v2 = v2;
    map->uorder = uorder;
    map->vorder = vorder;
    for (GLint j = 0; j < vorder; ++j) {
        for (GLint i = 0; i < uorder; ++i) {
            for (GLint c = 0; c < map->components; ++c) {
                map->points[((size_t)j * (size_t)uorder + (size_t)i) * 4u + (size_t)c] = points[j * vstride + i * ustride + c];
            }
        }
    }
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    GLfloat converted[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int comps = eval_map_components(target);

    if (points == NULL) {
        glMap2f(target, (GLfloat)u1, (GLfloat)u2, ustride, uorder, (GLfloat)v1, (GLfloat)v2, vstride, vorder, NULL);
        return;
    }
    if (comps <= 0 || uorder <= 0 || uorder > NXGL_MAX_EVAL_ORDER || vorder <= 0 || vorder > NXGL_MAX_EVAL_ORDER ||
        ustride < comps || vstride < ustride * uorder) {
        glMap2f(target, (GLfloat)u1, (GLfloat)u2, ustride, uorder, (GLfloat)v1, (GLfloat)v2, vstride, vorder, NULL);
        return;
    }
    for (GLint j = 0; j < vorder; ++j) {
        for (GLint i = 0; i < uorder; ++i) {
            for (GLint c = 0; c < comps; ++c) {
                converted[((size_t)j * (size_t)uorder + (size_t)i) * (size_t)comps + (size_t)c] =
                    (GLfloat)points[j * vstride + i * ustride + c];
            }
        }
    }
    glMap2f(target, (GLfloat)u1, (GLfloat)u2, comps, uorder, (GLfloat)v1, (GLfloat)v2, comps * uorder, vorder, converted);
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!map2) {
        EvalMap1 *map = &eval_maps1[index];
        if (query == GL_DOMAIN) {
            v[0] = map->u1;
            v[1] = map->u2;
        } else if (query == GL_ORDER) {
            v[0] = (GLfloat)map->order;
        } else if (query == GL_COEFF) {
            memcpy(v, map->points, (size_t)map->order * (size_t)map->components * sizeof(GLfloat));
        } else {
            set_error(GL_INVALID_ENUM);
        }
    } else {
        EvalMap2 *map = &eval_maps2[index];
        if (query == GL_DOMAIN) {
            v[0] = map->u1;
            v[1] = map->u2;
            v[2] = map->v1;
            v[3] = map->v2;
        } else if (query == GL_ORDER) {
            v[0] = (GLfloat)map->uorder;
            v[1] = (GLfloat)map->vorder;
        } else if (query == GL_COEFF) {
            for (GLint j = 0; j < map->vorder; ++j) {
                for (GLint i = 0; i < map->uorder; ++i) {
                    for (GLint c = 0; c < map->components; ++c) {
                        *v++ = map->points[((size_t)j * (size_t)map->uorder + (size_t)i) * 4u + (size_t)c];
                    }
                }
            }
        } else {
            set_error(GL_INVALID_ENUM);
        }
    }
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
    GLfloat values[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int count = 1;
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glGetMapfv(target, query, values);
    if (glGetError() != GL_NO_ERROR) {
        return;
    }
    if (query == GL_DOMAIN) count = map2 ? 4 : 2;
    else if (query == GL_ORDER) count = map2 ? 2 : 1;
    else if (query == GL_COEFF) count = map2 ? eval_maps2[index].uorder * eval_maps2[index].vorder * eval_maps2[index].components
                                             : eval_maps1[index].order * eval_maps1[index].components;
    for (int i = 0; i < count; ++i) {
        v[i] = (GLint)values[i];
    }
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
    GLfloat values[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int count = 1;
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glGetMapfv(target, query, values);
    if (glGetError() != GL_NO_ERROR) {
        return;
    }
    if (query == GL_DOMAIN) count = map2 ? 4 : 2;
    else if (query == GL_ORDER) count = map2 ? 2 : 1;
    else if (query == GL_COEFF) count = map2 ? eval_maps2[index].uorder * eval_maps2[index].vorder * eval_maps2[index].components
                                             : eval_maps1[index].order * eval_maps1[index].components;
    for (int i = 0; i < count; ++i) {
        v[i] = (GLdouble)values[i];
    }
}

void glEvalCoord1f(GLfloat u)
{
    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    apply_eval_map1(u);
}

void glEvalCoord1d(GLdouble u)
{
    glEvalCoord1f((GLfloat)u);
}

void glEvalCoord2f(GLfloat u, GLfloat v)
{
    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    apply_eval_map2(u, v);
}

void glEvalCoord2d(GLdouble u, GLdouble v)
{
    glEvalCoord2f((GLfloat)u, (GLfloat)v);
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (un <= 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map_grid1_n = un;
    map_grid1_u1 = u1;
    map_grid1_u2 = u2;
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    glMapGrid1f(un, (GLfloat)u1, (GLfloat)u2);
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (un <= 0 || vn <= 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map_grid2_un = un;
    map_grid2_vn = vn;
    map_grid2_u1 = u1;
    map_grid2_u2 = u2;
    map_grid2_v1 = v1;
    map_grid2_v2 = v2;
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    glMapGrid2f(un, (GLfloat)u1, (GLfloat)u2, vn, (GLfloat)v1, (GLfloat)v2);
}

void glEvalPoint1(GLint i)
{
    GLfloat u = map_grid1_n != 0 ? map_grid1_u1 + ((map_grid1_u2 - map_grid1_u1) * (GLfloat)i / (GLfloat)map_grid1_n) : map_grid1_u1;
    glEvalCoord1f(u);
}

void glEvalPoint2(GLint i, GLint j)
{
    GLfloat u = map_grid2_un != 0 ? map_grid2_u1 + ((map_grid2_u2 - map_grid2_u1) * (GLfloat)i / (GLfloat)map_grid2_un) : map_grid2_u1;
    GLfloat v = map_grid2_vn != 0 ? map_grid2_v1 + ((map_grid2_v2 - map_grid2_v1) * (GLfloat)j / (GLfloat)map_grid2_vn) : map_grid2_v1;
    glEvalCoord2f(u, v);
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    if (mode == GL_POINT) {
        glBegin(GL_POINTS);
        for (GLint i = i1; i <= i2; ++i) glEvalPoint1(i);
        glEnd();
    } else if (mode == GL_LINE) {
        glBegin(GL_LINE_STRIP);
        for (GLint i = i1; i <= i2; ++i) glEvalPoint1(i);
        glEnd();
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    if (mode == GL_POINT) {
        glBegin(GL_POINTS);
        for (GLint j = j1; j <= j2; ++j) {
            for (GLint i = i1; i <= i2; ++i) glEvalPoint2(i, j);
        }
        glEnd();
    } else if (mode == GL_LINE) {
        for (GLint j = j1; j <= j2; ++j) {
            glBegin(GL_LINE_STRIP);
            for (GLint i = i1; i <= i2; ++i) glEvalPoint2(i, j);
            glEnd();
        }
        for (GLint i = i1; i <= i2; ++i) {
            glBegin(GL_LINE_STRIP);
            for (GLint j = j1; j <= j2; ++j) glEvalPoint2(i, j);
            glEnd();
        }
    } else if (mode == GL_FILL) {
        for (GLint j = j1; j < j2; ++j) {
            glBegin(GL_QUAD_STRIP);
            for (GLint i = i1; i <= i2; ++i) {
                glEvalPoint2(i, j);
                glEvalPoint2(i, j + 1);
            }
            glEnd();
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    int comps = 1;
    int elem_size = 1;
    uint8_t *dst = (uint8_t *)pixels;
    size_t pixel_bytes;
    size_t out_stride;
    bool color_read = format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA;
    bool depth_read = format == GL_DEPTH_COMPONENT;
    bool stencil_read = format == GL_STENCIL_INDEX;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!color_read && !depth_read && !stencil_read) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (color_read && type != GL_UNSIGNED_BYTE) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (depth_read && type != GL_FLOAT && type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (stencil_read && type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL && width > 0 && height > 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_read && shadow_color_buffer == NULL) ||
        (depth_read && shadow_depth_buffer == NULL) ||
        (stencil_read && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    nxgl_backend_flush();
    ++perf_counters.read_pixels_calls;

    if (color_read) {
        comps = source_components(format);
    }
    elem_size = color_read ? 1 : type_size(type);
    pixel_bytes = (size_t)comps * (size_t)elem_size;
    out_stride = pixel_store_row_stride(width, pack_row_length, pack_skip_pixels, pixel_bytes, pack_alignment);
    dst += (size_t)pack_skip_rows * out_stride + (size_t)pack_skip_pixels * pixel_bytes;
    for (GLsizei row = 0; row < height; ++row) {
        int src_y = shadow_height - 1 - (y + row);
        uint8_t *dst_row = dst + (size_t)row * out_stride;
        if (color_read) {
            const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                uint32_t pixel = src_row[col];
                uint8_t a = (uint8_t)((pixel >> 24) & 0xff);
                uint8_t r = (uint8_t)((pixel >> 16) & 0xff);
                uint8_t g = (uint8_t)((pixel >> 8) & 0xff);
                uint8_t b = (uint8_t)(pixel & 0xff);
                uint8_t *out = dst_row + (size_t)col * (size_t)comps;

                if (format == GL_RGBA) {
                    out[0] = r;
                    out[1] = g;
                    out[2] = b;
                    out[3] = a;
                } else if (format == GL_RGB) {
                    out[0] = r;
                    out[1] = g;
                    out[2] = b;
                } else if (format == GL_BGRA) {
                    out[0] = b;
                    out[1] = g;
                    out[2] = r;
                    out[3] = a;
                } else {
                    out[0] = b;
                    out[1] = g;
                    out[2] = r;
                }
            }
        } else if (depth_read) {
            const float *src_row = shadow_depth_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                GLfloat depth = clamp01(src_row[col]);
                uint8_t *out = dst_row + (size_t)col * (size_t)elem_size;

                if (type == GL_FLOAT) {
                    ((GLfloat *)out)[0] = depth;
                } else if (type == GL_UNSIGNED_BYTE) {
                    out[0] = (uint8_t)(depth * 255.0f + 0.5f);
                } else if (type == GL_UNSIGNED_SHORT) {
                    ((GLushort *)out)[0] = (GLushort)(depth * 65535.0f + 0.5f);
                } else {
                    ((GLuint *)out)[0] = (GLuint)(depth * 4294967295.0f + 0.5f);
                }
            }
        } else {
            const uint8_t *src_row = shadow_stencil_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                GLuint stencil = src_row[col];
                uint8_t *out = dst_row + (size_t)col * (size_t)elem_size;

                if (type == GL_UNSIGNED_BYTE) {
                    out[0] = (uint8_t)stencil;
                } else if (type == GL_UNSIGNED_SHORT) {
                    ((GLushort *)out)[0] = (GLushort)stencil;
                } else {
                    ((GLuint *)out)[0] = stencil;
                }
            }
        }
    }
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
    glRasterPos3f(x, y, 0.0f);
}

void glRasterPos2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2f(v[0], v[1]);
}

void glRasterPos2d(GLdouble x, GLdouble y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2d(v[0], v[1]);
}

void glRasterPos2i(GLint x, GLint y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2i(v[0], v[1]);
}

void glRasterPos2s(GLshort x, GLshort y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2s(v[0], v[1]);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    ListCommand command = { LIST_CMD_RASTER_POS };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    command.f[3] = 1.0f;
    record_command(command);
    if (compile_only()) {
        return;
    }
    set_raster_position_from_vertex(x, y, z);
}

void glRasterPos3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3f(v[0], v[1], v[2]);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3d(v[0], v[1], v[2]);
}

void glRasterPos3i(GLint x, GLint y, GLint z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3i(v[0], v[1], v[2]);
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3s(v[0], v[1], v[2]);
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ListCommand command = { LIST_CMD_RASTER_POS };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    command.f[3] = w;
    record_command(command);
    if (compile_only()) {
        return;
    }
    set_raster_position_from_vertex4(x, y, z, w);
}

void glRasterPos4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4f(v[0], v[1], v[2], v[3]);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4d(v[0], v[1], v[2], v[3]);
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4i(v[0], v[1], v[2], v[3]);
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4s(v[0], v[1], v[2], v[3]);
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    ListCommand command = { LIST_CMD_BITMAP };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    size_t src_stride;
    size_t data_size;
    uint32_t color;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (bitmap == NULL && width > 0 && height > 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (shadow_color_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)width;
    command.f[1] = (GLfloat)height;
    command.f[2] = xorig;
    command.f[3] = yorig;
    command.extra[0] = xmove;
    command.extra[1] = ymove;
    command.i[0] = unpack_alignment;
    command.i[1] = unpack_row_length;
    command.i[2] = unpack_skip_rows;
    command.i[3] = unpack_skip_pixels;
    command.u = (unpack_swap_bytes ? 1u : 0u) | (unpack_lsb_first ? 2u : 0u);
    data_size = bitmap_source_data_size(width, height);
    if (data_size != 0 && !attach_command_data(&command, bitmap, data_size)) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    ++perf_counters.draw_pixels_calls;
    if (!current_raster_position_valid) {
        return;
    }

    if (render_mode == GL_FEEDBACK && width > 0 && height > 0) {
        record_feedback_pixel_token(GL_BITMAP_TOKEN);
    } else if (width > 0 && height > 0) {
        src_stride = bitmap_unpack_row_stride(width);
        color = color_to_u32(current_color);
        for (GLsizei row = 0; row < height; ++row) {
            const GLubyte *src_row = bitmap + (size_t)unpack_skip_rows * src_stride + (size_t)row * src_stride;
            GLint dst_y = raster_y + row - (GLint)yorig;
            for (GLsizei col = 0; col < width; ++col) {
                GLsizei bit_index = unpack_skip_pixels + col;
                GLubyte mask = unpack_lsb_first ? (GLubyte)(1u << (bit_index & 7)) : (GLubyte)(0x80u >> (bit_index & 7));
                if ((src_row[bit_index >> 3] & mask) != 0) {
                    write_shadow_pixel(raster_x + col - (GLint)xorig, dst_y, color);
                }
            }
        }
    }

    current_raster_position[0] += xmove;
    current_raster_position[1] += ymove;
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ListCommand command = { LIST_CMD_DRAW_PIXELS };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    const uint8_t *src = (const uint8_t *)pixels;
    uint8_t *rgba = NULL;
    size_t data_size;
    int zoom_x = pixel_zoom_replicate(pixel_zoom_x);
    int zoom_y = pixel_zoom_replicate(pixel_zoom_y);
    bool color_draw = format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA || format == GL_COLOR_INDEX;
    bool depth_draw = format == GL_DEPTH_COMPONENT;
    bool stencil_draw = format == GL_STENCIL_INDEX;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!color_draw && !depth_draw && !stencil_draw) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if ((color_draw && !valid_pixel_type(type)) ||
        (depth_draw && !valid_depth_pixel_type(type)) ||
        (stencil_draw && !valid_stencil_pixel_type(type))) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_draw && shadow_color_buffer == NULL) ||
        (depth_draw && shadow_depth_buffer == NULL) ||
        (stencil_draw && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)width;
    command.f[1] = (GLfloat)height;
    command.a = format;
    command.b = type;
    command.i[0] = unpack_alignment;
    command.i[1] = unpack_row_length;
    command.i[2] = unpack_skip_rows;
    command.i[3] = unpack_skip_pixels;
    command.u = (unpack_swap_bytes ? 1u : 0u) | (unpack_lsb_first ? 2u : 0u);
    data_size = pixel_source_data_size(width, height, format, type);
    if (data_size != 0 && !attach_command_data(&command, pixels, data_size)) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (!current_raster_position_valid) {
        return;
    }
    if (pixel_zoom_x == 0.0f || pixel_zoom_y == 0.0f) {
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_pixel_token(GL_DRAW_PIXEL_TOKEN);
        return;
    }

    if (color_draw) {
        rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
        if (rgba == NULL) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (!convert_to_rgba(rgba, src, width, height, format, type)) {
            free(rgba);
            set_error(GL_INVALID_ENUM);
            return;
        }
    }

    for (GLsizei row = 0; row < height; ++row) {
        GLint base_y = pixel_zoom_y > 0.0f ? raster_y + row * zoom_y : raster_y - (row + 1) * zoom_y;
        size_t pixel_bytes = color_draw ? 0u : (size_t)type_size(type);
        size_t src_stride = color_draw ? 0u : pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
        const uint8_t *src_row = color_draw ? NULL : src + (size_t)unpack_skip_rows * src_stride + (size_t)unpack_skip_pixels * pixel_bytes + (size_t)row * src_stride;
        for (GLsizei col = 0; col < width; ++col) {
            GLint base_x = pixel_zoom_x > 0.0f ? raster_x + col * zoom_x : raster_x - (col + 1) * zoom_x;
            const uint8_t *src_pixel = color_draw ? rgba + ((size_t)row * (size_t)width + (size_t)col) * 4u
                                                  : src_row + (size_t)col * pixel_bytes;
            NxglBackendColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
            GLfloat depth = 0.0f;
            uint8_t stencil = 0;

            if (color_draw) {
                color = apply_pixel_transfer_rgba_color(src_pixel);
            } else if (depth_draw) {
                depth = unpack_depth_pixel(src_pixel, type);
            } else {
                stencil = apply_pixel_transfer_stencil(unpack_stencil_pixel(src_pixel, type));
            }
            for (int yy = 0; yy < zoom_y; ++yy) {
                for (int xx = 0; xx < zoom_x; ++xx) {
                    if (color_draw) {
                        write_shadow_color_fragment(base_x + xx, base_y + yy, color, current_raster_position[2]);
                    } else if (depth_draw) {
                        write_shadow_depth_pixel(base_x + xx, base_y + yy, depth);
                    } else {
                        write_shadow_stencil_pixel(base_x + xx, base_y + yy, stencil);
                    }
                }
            }
        }
    }
    free(rgba);
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    ListCommand command = { LIST_CMD_COPY_PIXELS };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    uint32_t *copy;
    float *depth_copy;
    uint8_t *stencil_copy;
    int zoom_x = pixel_zoom_replicate(pixel_zoom_x);
    int zoom_y = pixel_zoom_replicate(pixel_zoom_y);
    bool color_copy = type == GL_COLOR;
    bool depth_copy_mode = type == GL_DEPTH;
    bool stencil_copy_mode = type == GL_STENCIL;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (!color_copy && !depth_copy_mode && !stencil_copy_mode) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_copy && shadow_color_buffer == NULL) ||
        (depth_copy_mode && shadow_depth_buffer == NULL) ||
        (stencil_copy_mode && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    command.a = type;
    record_command(command);
    if (compile_only()) {
        return;
    }

    ++perf_counters.copy_pixels_calls;
    if (!current_raster_position_valid || width == 0 || height == 0) {
        return;
    }
    if (pixel_zoom_x == 0.0f || pixel_zoom_y == 0.0f) {
        return;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_pixel_token(GL_COPY_PIXEL_TOKEN);
        return;
    }

    if (color_copy) {
        nxgl_backend_flush();
    }

    copy = NULL;
    depth_copy = NULL;
    stencil_copy = NULL;
    if (color_copy) {
        copy = (uint32_t *)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    } else if (depth_copy_mode) {
        depth_copy = (float *)malloc((size_t)width * (size_t)height * sizeof(float));
    } else {
        stencil_copy = (uint8_t *)malloc((size_t)width * (size_t)height);
    }
    if ((color_copy && copy == NULL) ||
        (depth_copy_mode && depth_copy == NULL) ||
        (stencil_copy_mode && stencil_copy == NULL)) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei row = 0; row < height; ++row) {
        GLint src_y = shadow_height - 1 - (y + row);
        if (color_copy) {
            const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(copy + (size_t)row * (size_t)width, src_row, (size_t)width * sizeof(uint32_t));
        } else if (depth_copy_mode) {
            const float *src_row = shadow_depth_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(depth_copy + (size_t)row * (size_t)width, src_row, (size_t)width * sizeof(float));
        } else {
            const uint8_t *src_row = shadow_stencil_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(stencil_copy + (size_t)row * (size_t)width, src_row, (size_t)width);
        }
    }
    for (GLsizei row = 0; row < height; ++row) {
        GLint base_y = pixel_zoom_y > 0.0f ? raster_y + row * zoom_y : raster_y - (row + 1) * zoom_y;
        for (GLsizei col = 0; col < width; ++col) {
            GLint base_x = pixel_zoom_x > 0.0f ? raster_x + col * zoom_x : raster_x - (col + 1) * zoom_x;
            size_t src_index = (size_t)row * (size_t)width + (size_t)col;
            uint32_t color = color_copy ? apply_pixel_transfer_u32(copy[src_index]) : 0;
            GLfloat depth = depth_copy_mode ? depth_copy[src_index] : 0.0f;
            uint8_t stencil = stencil_copy_mode ? apply_pixel_transfer_stencil(stencil_copy[src_index]) : 0;
            for (int yy = 0; yy < zoom_y; ++yy) {
                for (int xx = 0; xx < zoom_x; ++xx) {
                    if (color_copy) {
                        write_shadow_pixel(base_x + xx, base_y + yy, color);
                    } else if (depth_copy_mode) {
                        write_shadow_depth_pixel(base_x + xx, base_y + yy, depth);
                    } else {
                        write_shadow_stencil_pixel(base_x + xx, base_y + yy, stencil);
                    }
                }
            }
        }
    }
    free(copy);
    free(depth_copy);
    free(stencil_copy);
}

void glDrawBuffer(GLenum mode)
{
    ListCommand command = { LIST_CMD_DRAW_BUFFER };

    if (!valid_draw_buffer(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    draw_buffer_mode = mode;
}

void glReadBuffer(GLenum mode)
{
    ListCommand command = { LIST_CMD_READ_BUFFER };

    if (!valid_read_buffer(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    read_buffer_mode = mode;
}

void glSelectBuffer(GLsizei size, GLuint *buffer)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode == GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (size < 0 || buffer == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    selection_buffer_size = size;
    selection_buffer = buffer;
    selection_write_count = 0;
    selection_hits = 0;
    selection_overflow = false;
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (size < 0 || buffer == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_2D &&
        type != GL_3D &&
        type != GL_3D_COLOR &&
        type != GL_3D_COLOR_TEXTURE &&
        type != GL_4D_COLOR_TEXTURE) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    feedback_buffer_size = size;
    feedback_buffer = buffer;
    feedback_write_count = 0;
    feedback_overflow = false;
    feedback_type = type;
}

GLint glRenderMode(GLenum mode)
{
    GLint result = 0;

    if (mode != GL_RENDER && mode != GL_SELECT && mode != GL_FEEDBACK) {
        set_error(GL_INVALID_ENUM);
        return 0;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }
    if (mode == GL_SELECT && selection_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }
    if (mode == GL_FEEDBACK && feedback_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }

    if (render_mode == GL_SELECT) {
        result = selection_overflow ? -1 : selection_hits;
    } else if (render_mode == GL_FEEDBACK) {
        result = feedback_overflow ? -1 : feedback_write_count;
    }

    render_mode = mode;
    if (mode == GL_SELECT) {
        selection_write_count = 0;
        selection_hits = 0;
        selection_overflow = false;
    } else if (mode == GL_FEEDBACK) {
        feedback_write_count = 0;
        feedback_overflow = false;
    }
    return result;
}

void glPassThrough(GLfloat token)
{
    ListCommand command = { LIST_CMD_PASS_THROUGH };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = token;
    record_command(command);
    if (compile_only()) {
        return;
    }
    record_feedback_pass_through(token);
}

void glInitNames(void)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    name_stack_depth = 0;
}

void glPushName(GLuint name)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth >= (GLint)(sizeof(name_stack) / sizeof(name_stack[0]))) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    name_stack[name_stack_depth++] = name;
}

void glPopName(void)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    --name_stack_depth;
}

void glLoadName(GLuint name)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth <= 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    name_stack[name_stack_depth - 1] = name;
}


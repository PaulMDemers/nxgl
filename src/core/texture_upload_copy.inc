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


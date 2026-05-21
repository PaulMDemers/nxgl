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

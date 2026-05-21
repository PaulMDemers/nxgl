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


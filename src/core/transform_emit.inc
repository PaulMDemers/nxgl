static GLfloat eval_normalized(GLfloat value, GLfloat a, GLfloat b)
{
    if (a == b) {
        return 0.0f;
    }
    return (value - a) / (b - a);
}

static GLfloat eval_lerp(GLfloat a, GLfloat b, GLfloat t)
{
    return a + (b - a) * t;
}

static void eval_curve_points(GLfloat *work, GLint order, GLint components, GLfloat t)
{
    for (GLint level = order - 1; level > 0; --level) {
        for (GLint i = 0; i < level; ++i) {
            for (GLint c = 0; c < components; ++c) {
                work[i * components + c] = eval_lerp(work[i * components + c], work[(i + 1) * components + c], t);
            }
        }
    }
}

static void eval_map1_value(const EvalMap1 *map, GLfloat u, GLfloat out[4])
{
    GLfloat work[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat t;

    memset(out, 0, sizeof(GLfloat) * 4u);
    if (map == NULL || !map->defined || map->order <= 0 || map->components <= 0) {
        return;
    }
    memcpy(work, map->points, (size_t)map->order * (size_t)map->components * sizeof(GLfloat));
    t = eval_normalized(u, map->u1, map->u2);
    eval_curve_points(work, map->order, map->components, t);
    for (GLint c = 0; c < map->components; ++c) {
        out[c] = work[c];
    }
}

static void eval_map2_value(const EvalMap2 *map, GLfloat u, GLfloat v, GLfloat out[4])
{
    GLfloat vwork[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat uwork[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat tu;
    GLfloat tv;

    memset(out, 0, sizeof(GLfloat) * 4u);
    if (map == NULL || !map->defined || map->uorder <= 0 || map->vorder <= 0 || map->components <= 0) {
        return;
    }
    tu = eval_normalized(u, map->u1, map->u2);
    tv = eval_normalized(v, map->v1, map->v2);
    for (GLint j = 0; j < map->vorder; ++j) {
        for (GLint i = 0; i < map->uorder; ++i) {
            for (GLint c = 0; c < map->components; ++c) {
                uwork[i * map->components + c] = map->points[((size_t)j * (size_t)map->uorder + (size_t)i) * 4u + (size_t)c];
            }
        }
        eval_curve_points(uwork, map->uorder, map->components, tu);
        for (GLint c = 0; c < map->components; ++c) {
            vwork[j * map->components + c] = uwork[c];
        }
    }
    eval_curve_points(vwork, map->vorder, map->components, tv);
    for (GLint c = 0; c < map->components; ++c) {
        out[c] = vwork[c];
    }
}

static void apply_eval_map1(GLfloat u)
{
    GLfloat value[4];
    int unit = active_texture_index();

    if (eval_maps1[0].enabled && eval_maps1[0].defined) {
        eval_map1_value(&eval_maps1[0], u, value);
        glColor4f(value[0], value[1], value[2], value[3]);
    }
    if (eval_maps1[1].enabled && eval_maps1[1].defined) {
        eval_map1_value(&eval_maps1[1], u, value);
        glNormal3f(value[0], value[1], value[2]);
    }
    for (int i = 2; i <= 5; ++i) {
        if (eval_maps1[i].enabled && eval_maps1[i].defined) {
            eval_map1_value(&eval_maps1[i], u, value);
            if (unit >= 0) {
                current_u[unit] = value[0];
                current_v[unit] = eval_maps1[i].components > 1 ? value[1] : 0.0f;
                current_r[unit] = eval_maps1[i].components > 2 ? value[2] : 0.0f;
            }
            break;
        }
    }
    if (eval_maps1[7].enabled && eval_maps1[7].defined) {
        eval_map1_value(&eval_maps1[7], u, value);
        if (value[3] != 0.0f) {
            glVertex3f(value[0] / value[3], value[1] / value[3], value[2] / value[3]);
        } else {
            glVertex3f(value[0], value[1], value[2]);
        }
    } else if (eval_maps1[6].enabled && eval_maps1[6].defined) {
        eval_map1_value(&eval_maps1[6], u, value);
        glVertex3f(value[0], value[1], value[2]);
    }
}

static void apply_eval_map2(GLfloat u, GLfloat v)
{
    GLfloat value[4];
    int unit = active_texture_index();

    if (eval_maps2[0].enabled && eval_maps2[0].defined) {
        eval_map2_value(&eval_maps2[0], u, v, value);
        glColor4f(value[0], value[1], value[2], value[3]);
    }
    if (eval_maps2[1].enabled && eval_maps2[1].defined) {
        eval_map2_value(&eval_maps2[1], u, v, value);
        glNormal3f(value[0], value[1], value[2]);
    }
    for (int i = 2; i <= 5; ++i) {
        if (eval_maps2[i].enabled && eval_maps2[i].defined) {
            eval_map2_value(&eval_maps2[i], u, v, value);
            if (unit >= 0) {
                current_u[unit] = value[0];
                current_v[unit] = eval_maps2[i].components > 1 ? value[1] : 0.0f;
                current_r[unit] = eval_maps2[i].components > 2 ? value[2] : 0.0f;
            }
            break;
        }
    }
    if (eval_maps2[7].enabled && eval_maps2[7].defined) {
        eval_map2_value(&eval_maps2[7], u, v, value);
        if (value[3] != 0.0f) {
            glVertex3f(value[0] / value[3], value[1] / value[3], value[2] / value[3]);
        } else {
            glVertex3f(value[0], value[1], value[2]);
        }
    } else if (eval_maps2[6].enabled && eval_maps2[6].defined) {
        eval_map2_value(&eval_maps2[6], u, v, value);
        glVertex3f(value[0], value[1], value[2]);
    }
}

static bool copy_shadow_rgba(uint8_t *dst, GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (dst == NULL) {
        return false;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (shadow_color_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return false;
    }

    nxgl_backend_flush();
    for (GLsizei row = 0; row < height; ++row) {
        int src_y = shadow_height - 1 - (y + row);
        const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
        uint8_t *dst_row = dst + (size_t)row * (size_t)width * 4u;
        for (GLsizei col = 0; col < width; ++col) {
            uint32_t pixel = src_row[col];
            uint8_t *out = dst_row + (size_t)col * 4u;
            out[0] = (uint8_t)((pixel >> 16) & 0xff);
            out[1] = (uint8_t)((pixel >> 8) & 0xff);
            out[2] = (uint8_t)(pixel & 0xff);
            out[3] = 255;
        }
    }
    return true;
}

static float read_component(const uint8_t *ptr, GLenum type, bool normalize)
{
    switch (type) {
    case GL_BYTE:
        return normalize ? (float)(*(const int8_t *)ptr) / 127.0f : (float)(*(const int8_t *)ptr);
    case GL_UNSIGNED_BYTE:
        return normalize ? (float)(*(const uint8_t *)ptr) / 255.0f : (float)(*(const uint8_t *)ptr);
    case GL_SHORT:
        return normalize ? (float)(*(const int16_t *)ptr) / 32767.0f : (float)(*(const int16_t *)ptr);
    case GL_UNSIGNED_SHORT:
        return normalize ? (float)(*(const uint16_t *)ptr) / 65535.0f : (float)(*(const uint16_t *)ptr);
    case GL_INT:
        return (float)(*(const int32_t *)ptr);
    case GL_UNSIGNED_INT:
        return (float)(*(const uint32_t *)ptr);
    case GL_FLOAT:
        return *(const float *)ptr;
    default:
        return 0.0f;
    }
}

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static float clamp_accum_clear(float value)
{
    if (value < -1.0f) {
        return -1.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static GLfloat normalized_glbyte(GLbyte value)
{
    return value < 0 ? (GLfloat)value / 128.0f : (GLfloat)value / 127.0f;
}

static GLfloat normalized_glshort(GLshort value)
{
    return value < 0 ? (GLfloat)value / 32768.0f : (GLfloat)value / 32767.0f;
}

static GLfloat normalized_glint(GLint value)
{
    return value < 0 ? (GLfloat)value / 2147483648.0f : (GLfloat)value / 2147483647.0f;
}

static GLfloat normalized_glubyte(GLubyte value)
{
    return (GLfloat)value / 255.0f;
}

static GLfloat normalized_glushort(GLushort value)
{
    return (GLfloat)value / 65535.0f;
}

static GLfloat normalized_gluint(GLuint value)
{
    return (GLfloat)value / 4294967295.0f;
}

static GLfloat map_depth_range(GLfloat normalized)
{
    GLfloat clamped = clamp01(normalized);
    return clamp01(depth_range_near + (depth_range_far - depth_range_near) * clamped);
}

static GLfloat eye_depth_normalized(NxglBackendVec3 pos)
{
    return clamp01((-(pos.z + camera_z)) / 100.0f);
}

static void viewport_top_left_bounds(int *min_x, int *min_y, int *max_x, int *max_y)
{
    int x1 = viewport[0];
    int y1 = shadow_height - (viewport[1] + viewport[3]);
    int x2 = viewport[0] + viewport[2] - 1;
    int y2 = shadow_height - viewport[1] - 1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= shadow_width) x2 = shadow_width - 1;
    if (y2 >= shadow_height) y2 = shadow_height - 1;

    *min_x = x1;
    *min_y = y1;
    *max_x = x2;
    *max_y = y2;
}

static NxglBackendVec3 normalize_vec3(NxglBackendVec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.00001f) {
        NxglBackendVec3 fallback = { 0.0f, 0.0f, 1.0f };
        return fallback;
    }
    v.x /= len;
    v.y /= len;
    v.z /= len;
    return v;
}

static void apply_texgen_to_coords(GLfloat obj[4], NxglBackendVec3 eye, NxglBackendVec3 normal, GLfloat *u, GLfloat *v, GLfloat *r, int unit)
{
    GLfloat generated[4] = { *u, *v, *r, 1.0f };
    GLfloat eye_vec[4] = { eye.x, eye.y, eye.z, 1.0f };

    if (unit < 0 || unit >= 4) {
        return;
    }
    for (int coord = 0; coord < 4; ++coord) {
        TexGenState *state = &texgen_state[unit][coord];
        GLfloat value = generated[coord];

        if (!state->enabled) {
            continue;
        }
        if (state->mode == GL_OBJECT_LINEAR) {
            value = obj[0] * state->object_plane[0] +
                    obj[1] * state->object_plane[1] +
                    obj[2] * state->object_plane[2] +
                    obj[3] * state->object_plane[3];
        } else if (state->mode == GL_EYE_LINEAR) {
            value = eye_vec[0] * state->eye_plane[0] +
                    eye_vec[1] * state->eye_plane[1] +
                    eye_vec[2] * state->eye_plane[2] +
                    eye_vec[3] * state->eye_plane[3];
        } else if (state->mode == GL_SPHERE_MAP && coord < 2) {
            NxglBackendVec3 n = normalize_vec3(normal);
            value = coord == 0 ? n.x * 0.5f + 0.5f : n.y * 0.5f + 0.5f;
        }
        generated[coord] = value;
    }
    if (generated[3] != 0.0f) {
        *u = generated[0] / generated[3];
        *v = generated[1] / generated[3];
        *r = generated[2] / generated[3];
    } else {
        *u = generated[0];
        *v = generated[1];
        *r = generated[2];
    }
}

static float dot_vec3(NxglBackendVec3 a, NxglBackendVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static NxglBackendVec3 subtract_vec3(NxglBackendVec3 a, NxglBackendVec3 b)
{
    NxglBackendVec3 out = { a.x - b.x, a.y - b.y, a.z - b.z };
    return out;
}

static float length_vec3(NxglBackendVec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static NxglBackendColor multiply_color(NxglBackendColor a, NxglBackendColor b)
{
    NxglBackendColor out = { a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a };
    return out;
}

static NxglBackendColor add_color(NxglBackendColor a, NxglBackendColor b)
{
    NxglBackendColor out = { a.r + b.r, a.g + b.g, a.b + b.b, a.a };
    return out;
}

static NxglBackendColor scale_color(NxglBackendColor color, float scale)
{
    NxglBackendColor out = { color.r * scale, color.g * scale, color.b * scale, color.a };
    return out;
}

static NxglBackendColor clamp_color(NxglBackendColor color)
{
    color.r = clamp01(color.r);
    color.g = clamp01(color.g);
    color.b = clamp01(color.b);
    color.a = clamp01(color.a);
    return color;
}

static void apply_color_material(NxglBackendColor color)
{
    if (!color_material_enabled) {
        return;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_AMBIENT || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_state.ambient = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_DIFFUSE || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_state.diffuse = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_SPECULAR) {
        material_state.specular = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_EMISSION) {
        material_state.emission = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_AMBIENT || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_back_state.ambient = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_DIFFUSE || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_back_state.diffuse = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_SPECULAR) {
        material_back_state.specular = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_EMISSION) {
        material_back_state.emission = color;
    }
}

static NxglBackendColor lit_color_with_material(NxglBackendColor base_color, NxglBackendVec3 normal, NxglBackendVec3 eye, const MaterialState *material)
{
    NxglBackendColor result;
    NxglBackendColor specular_accum = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool any_light = false;

    if (!lighting_enabled) {
        return base_color;
    }

    if (normalize_enabled || rescale_normal_enabled) {
        normal = normalize_vec3(normal);
    }

    result = material->emission;
    result = add_color(result, multiply_color(material->ambient, light_model_ambient));
    result.a = material->diffuse.a;

    for (int i = 0; i < 8; ++i) {
        if (!lights[i].enabled) {
            continue;
        }
        any_light = true;
        float light_scale = 1.0f;
        NxglBackendVec3 light_dir = { lights[i].position[0], lights[i].position[1], lights[i].position[2] };
        if (lights[i].position[3] != 0.0f) {
            NxglBackendVec3 light_pos = light_dir;
            NxglBackendVec3 to_vertex = subtract_vec3(eye, light_pos);
            float distance = length_vec3(to_vertex);
            float attenuation = lights[i].constant_attenuation +
                                lights[i].linear_attenuation * distance +
                                lights[i].quadratic_attenuation * distance * distance;
            light_dir = subtract_vec3(light_pos, eye);
            if (attenuation > 0.00001f) {
                light_scale = 1.0f / attenuation;
            }
            if (lights[i].spot_cutoff != 180.0f) {
                NxglBackendVec3 spot_dir = normalize_vec3((NxglBackendVec3){
                    lights[i].spot_direction[0],
                    lights[i].spot_direction[1],
                    lights[i].spot_direction[2]
                });
                float spot_cos = dot_vec3(normalize_vec3(to_vertex), spot_dir);
                float cutoff_cos = cosf(lights[i].spot_cutoff * 0.017453292519943295f);
                if (spot_cos < cutoff_cos) {
                    light_scale = 0.0f;
                } else if (lights[i].spot_exponent > 0.0f) {
                    light_scale *= powf(spot_cos < 0.0f ? 0.0f : spot_cos, lights[i].spot_exponent);
                }
            }
        }
        light_dir = normalize_vec3(light_dir);
        float diffuse = dot_vec3(normal, light_dir);
        if (diffuse < 0.0f) {
            diffuse = 0.0f;
        }
        result = add_color(result, scale_color(multiply_color(material->ambient, lights[i].ambient), light_scale));
        result = add_color(result, scale_color(multiply_color(material->diffuse, lights[i].diffuse), diffuse * light_scale));
        if (material->shininess > 0.0f && diffuse > 0.0f) {
            NxglBackendVec3 view_dir = light_model_local_viewer ? normalize_vec3((NxglBackendVec3){ -eye.x, -eye.y, -eye.z }) : (NxglBackendVec3){ 0.0f, 0.0f, 1.0f };
            NxglBackendVec3 half_vec = normalize_vec3((NxglBackendVec3){ light_dir.x + view_dir.x, light_dir.y + view_dir.y, light_dir.z + view_dir.z });
            float spec_angle = dot_vec3(normal, half_vec);
            float spec = 0.0f;
            if (spec_angle > 0.0f) {
                spec = powf(spec_angle, material->shininess * 0.25f) * light_scale;
            }
            NxglBackendColor contribution = scale_color(multiply_color(material->specular, lights[i].specular), spec);
            if (light_model_color_control == GL_SEPARATE_SPECULAR_COLOR) {
                specular_accum = add_color(specular_accum, contribution);
            } else {
                result = add_color(result, contribution);
            }
        }
    }

    if (!any_light) {
        result = multiply_color(material->diffuse, base_color);
    }
    result.a = base_color.a;
    if (light_model_color_control == GL_SEPARATE_SPECULAR_COLOR) {
        result = add_color(clamp_color(result), specular_accum);
        result.a = base_color.a;
    }
    return clamp_color(result);
}

static NxglBackendColor lit_color(NxglBackendColor base_color, NxglBackendVec3 normal, NxglBackendVec3 eye)
{
    return lit_color_with_material(base_color, normal, eye, &material_state);
}

static float fog_factor(float depth)
{
    float factor;

    if (!fog_enabled) {
        return 1.0f;
    }
    if (depth < 0.0f) {
        depth = -depth;
    }

    if (fog_mode == GL_LINEAR) {
        float range = fog_end - fog_start;
        if (range == 0.0f) {
            return depth <= fog_start ? 1.0f : 0.0f;
        }
        factor = (fog_end - depth) / range;
    } else if (fog_mode == GL_EXP2) {
        float d = fog_density * depth;
        factor = expf(-(d * d));
    } else {
        factor = expf(-(fog_density * depth));
    }
    return clamp01(factor);
}

static NxglBackendColor apply_fog(NxglBackendColor color, NxglBackendVec3 pos)
{
    float factor;
    NxglBackendColor out;

    if (!fog_enabled) {
        return color;
    }

    factor = fog_factor(pos.z);
    out.r = color.r * factor + fog_color.r * (1.0f - factor);
    out.g = color.g * factor + fog_color.g * (1.0f - factor);
    out.b = color.b * factor + fog_color.b * (1.0f - factor);
    out.a = color.a;
    return clamp_color(out);
}

static void matrix_identity(Matrix out)
{
    memset(out, 0, sizeof(Matrix));
    out[M11] = 1.0f;
    out[M22] = 1.0f;
    out[M33] = 1.0f;
    out[M44] = 1.0f;
}

static void matrix_multiply(Matrix out, const Matrix a, const Matrix b)
{
    Matrix w;
    w[M11] = a[M11] * b[M11] + a[M12] * b[M21] + a[M13] * b[M31] + a[M14] * b[M41];
    w[M12] = a[M11] * b[M12] + a[M12] * b[M22] + a[M13] * b[M32] + a[M14] * b[M42];
    w[M13] = a[M11] * b[M13] + a[M12] * b[M23] + a[M13] * b[M33] + a[M14] * b[M43];
    w[M14] = a[M11] * b[M14] + a[M12] * b[M24] + a[M13] * b[M34] + a[M14] * b[M44];
    w[M21] = a[M21] * b[M11] + a[M22] * b[M21] + a[M23] * b[M31] + a[M24] * b[M41];
    w[M22] = a[M21] * b[M12] + a[M22] * b[M22] + a[M23] * b[M32] + a[M24] * b[M42];
    w[M23] = a[M21] * b[M13] + a[M22] * b[M23] + a[M23] * b[M33] + a[M24] * b[M43];
    w[M24] = a[M21] * b[M14] + a[M22] * b[M24] + a[M23] * b[M34] + a[M24] * b[M44];
    w[M31] = a[M31] * b[M11] + a[M32] * b[M21] + a[M33] * b[M31] + a[M34] * b[M41];
    w[M32] = a[M31] * b[M12] + a[M32] * b[M22] + a[M33] * b[M32] + a[M34] * b[M42];
    w[M33] = a[M31] * b[M13] + a[M32] * b[M23] + a[M33] * b[M33] + a[M34] * b[M43];
    w[M34] = a[M31] * b[M14] + a[M32] * b[M24] + a[M33] * b[M34] + a[M34] * b[M44];
    w[M41] = a[M41] * b[M11] + a[M42] * b[M21] + a[M43] * b[M31] + a[M44] * b[M41];
    w[M42] = a[M41] * b[M12] + a[M42] * b[M22] + a[M43] * b[M32] + a[M44] * b[M42];
    w[M43] = a[M41] * b[M13] + a[M42] * b[M23] + a[M43] * b[M33] + a[M44] * b[M43];
    w[M44] = a[M41] * b[M14] + a[M42] * b[M24] + a[M43] * b[M34] + a[M44] * b[M44];
    memcpy(out, w, sizeof(Matrix));
}

static void matrix_transpose(Matrix out, const Matrix in)
{
    Matrix w;
    w[M11] = in[M11];
    w[M12] = in[M21];
    w[M13] = in[M31];
    w[M14] = in[M41];
    w[M21] = in[M12];
    w[M22] = in[M22];
    w[M23] = in[M32];
    w[M24] = in[M42];
    w[M31] = in[M13];
    w[M32] = in[M23];
    w[M33] = in[M33];
    w[M34] = in[M43];
    w[M41] = in[M14];
    w[M42] = in[M24];
    w[M43] = in[M34];
    w[M44] = in[M44];
    memcpy(out, w, sizeof(Matrix));
}

static Matrix *current_matrix(void)
{
    if (matrix_mode == GL_PROJECTION) {
        return &projection;
    }
    if (matrix_mode == GL_TEXTURE) {
        return &texture_matrix;
    }
    return &modelview;
}

static void invalidate_modelview_inverse_cache(void)
{
    modelview_inverse_cache_valid = false;
}

static int clip_plane_index(GLenum plane)
{
    if (plane >= GL_CLIP_PLANE0 && plane <= GL_CLIP_PLANE5) {
        return (int)(plane - GL_CLIP_PLANE0);
    }
    return -1;
}

static bool invert_matrix(Matrix out, const Matrix in)
{
    Matrix inv;
    GLfloat det;

    inv[0] = in[5]  * in[10] * in[15] -
             in[5]  * in[11] * in[14] -
             in[9]  * in[6]  * in[15] +
             in[9]  * in[7]  * in[14] +
             in[13] * in[6]  * in[11] -
             in[13] * in[7]  * in[10];
    inv[4] = -in[4]  * in[10] * in[15] +
              in[4]  * in[11] * in[14] +
              in[8]  * in[6]  * in[15] -
              in[8]  * in[7]  * in[14] -
              in[12] * in[6]  * in[11] +
              in[12] * in[7]  * in[10];
    inv[8] = in[4]  * in[9] * in[15] -
             in[4]  * in[11] * in[13] -
             in[8]  * in[5] * in[15] +
             in[8]  * in[7] * in[13] +
             in[12] * in[5] * in[11] -
             in[12] * in[7] * in[9];
    inv[12] = -in[4]  * in[9] * in[14] +
               in[4]  * in[10] * in[13] +
               in[8]  * in[5] * in[14] -
               in[8]  * in[6] * in[13] -
               in[12] * in[5] * in[10] +
               in[12] * in[6] * in[9];
    inv[1] = -in[1]  * in[10] * in[15] +
              in[1]  * in[11] * in[14] +
              in[9]  * in[2] * in[15] -
              in[9]  * in[3] * in[14] -
              in[13] * in[2] * in[11] +
              in[13] * in[3] * in[10];
    inv[5] = in[0]  * in[10] * in[15] -
             in[0]  * in[11] * in[14] -
             in[8]  * in[2] * in[15] +
             in[8]  * in[3] * in[14] +
             in[12] * in[2] * in[11] -
             in[12] * in[3] * in[10];
    inv[9] = -in[0]  * in[9] * in[15] +
              in[0]  * in[11] * in[13] +
              in[8]  * in[1] * in[15] -
              in[8]  * in[3] * in[13] -
              in[12] * in[1] * in[11] +
              in[12] * in[3] * in[9];
    inv[13] = in[0]  * in[9] * in[14] -
              in[0]  * in[10] * in[13] -
              in[8]  * in[1] * in[14] +
              in[8]  * in[2] * in[13] +
              in[12] * in[1] * in[10] -
              in[12] * in[2] * in[9];
    inv[2] = in[1]  * in[6] * in[15] -
             in[1]  * in[7] * in[14] -
             in[5]  * in[2] * in[15] +
             in[5]  * in[3] * in[14] +
             in[13] * in[2] * in[7] -
             in[13] * in[3] * in[6];
    inv[6] = -in[0]  * in[6] * in[15] +
              in[0]  * in[7] * in[14] +
              in[4]  * in[2] * in[15] -
              in[4]  * in[3] * in[14] -
              in[12] * in[2] * in[7] +
              in[12] * in[3] * in[6];
    inv[10] = in[0]  * in[5] * in[15] -
              in[0]  * in[7] * in[13] -
              in[4]  * in[1] * in[15] +
              in[4]  * in[3] * in[13] +
              in[12] * in[1] * in[7] -
              in[12] * in[3] * in[5];
    inv[14] = -in[0]  * in[5] * in[14] +
               in[0]  * in[6] * in[13] +
               in[4]  * in[1] * in[14] -
               in[4]  * in[2] * in[13] -
               in[12] * in[1] * in[6] +
               in[12] * in[2] * in[5];
    inv[3] = -in[1] * in[6] * in[11] +
              in[1] * in[7] * in[10] +
              in[5] * in[2] * in[11] -
              in[5] * in[3] * in[10] -
              in[9] * in[2] * in[7] +
              in[9] * in[3] * in[6];
    inv[7] = in[0] * in[6] * in[11] -
             in[0] * in[7] * in[10] -
             in[4] * in[2] * in[11] +
             in[4] * in[3] * in[10] +
             in[8] * in[2] * in[7] -
             in[8] * in[3] * in[6];
    inv[11] = -in[0] * in[5] * in[11] +
               in[0] * in[7] * in[9] +
               in[4] * in[1] * in[11] -
               in[4] * in[3] * in[9] -
               in[8] * in[1] * in[7] +
               in[8] * in[3] * in[5];
    inv[15] = in[0] * in[5] * in[10] -
              in[0] * in[6] * in[9] -
              in[4] * in[1] * in[10] +
              in[4] * in[2] * in[9] +
              in[8] * in[1] * in[6] -
              in[8] * in[2] * in[5];

    det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];
    if (det == 0.0f) {
        return false;
    }
    det = 1.0f / det;
    for (int i = 0; i < 16; ++i) {
        out[i] = inv[i] * det;
    }
    return true;
}

static void transform_clip_plane(GLdouble out[4], const GLdouble in[4])
{
    Matrix inverse;

    if (!invert_matrix(inverse, modelview)) {
        memcpy(out, in, sizeof(GLdouble) * 4u);
        return;
    }
    out[0] = in[0] * inverse[M11] + in[1] * inverse[M12] + in[2] * inverse[M13] + in[3] * inverse[M14];
    out[1] = in[0] * inverse[M21] + in[1] * inverse[M22] + in[2] * inverse[M23] + in[3] * inverse[M24];
    out[2] = in[0] * inverse[M31] + in[1] * inverse[M32] + in[2] * inverse[M33] + in[3] * inverse[M34];
    out[3] = in[0] * inverse[M41] + in[1] * inverse[M42] + in[2] * inverse[M43] + in[3] * inverse[M44];
}

static bool point_inside_clip_planes(NxglBackendVec3 pos)
{
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        const GLdouble *p = clip_planes[i].equation;
        GLdouble d;

        if (!clip_planes[i].enabled) {
            continue;
        }
        d = (GLdouble)pos.x * p[0] + (GLdouble)pos.y * p[1] + (GLdouble)pos.z * p[2] + p[3];
        if (d < 0.0) {
            return false;
        }
    }
    return true;
}

static bool primitive_rejected_by_clip_planes(const NxglBackendVertex *vertices, int count)
{
    if (vertices == NULL || count <= 0) {
        return true;
    }
    for (int plane = 0; plane < NXGL_MAX_CLIP_PLANES; ++plane) {
        bool all_outside = true;
        const GLdouble *p = clip_planes[plane].equation;

        if (!clip_planes[plane].enabled) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            GLdouble d = (GLdouble)vertices[i].eye.x * p[0] +
                         (GLdouble)vertices[i].eye.y * p[1] +
                         (GLdouble)vertices[i].eye.z * p[2] +
                         p[3];
            if (d >= 0.0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return true;
        }
    }
    return false;
}

static bool clip_planes_enabled(void)
{
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        if (clip_planes[i].enabled) {
            return true;
        }
    }
    return false;
}

static bool texgen_enabled(void)
{
    return texgen_any_enabled;
}

static void update_texgen_enabled_cache(void)
{
    texgen_any_enabled = false;
    for (int unit = 0; unit < 4; ++unit) {
        for (int coord = 0; coord < 4; ++coord) {
            if (texgen_state[unit][coord].enabled) {
                texgen_any_enabled = true;
                return;
            }
        }
    }
}

static bool native_fast_fill_enabled(void)
{
    return !shadow_readback_enabled &&
           render_mode == GL_RENDER &&
           polygon_mode == GL_FILL &&
           !clip_planes_enabled();
}

static bool default_combine_sources(int unit)
{
    return texture_source_rgb[unit][0] == GL_TEXTURE &&
           texture_source_rgb[unit][1] == GL_PREVIOUS &&
           texture_source_rgb[unit][2] == GL_CONSTANT &&
           texture_operand_rgb[unit][0] == GL_SRC_COLOR &&
           texture_operand_rgb[unit][1] == GL_SRC_COLOR &&
           (texture_operand_rgb[unit][2] == GL_SRC_COLOR || texture_operand_rgb[unit][2] == GL_SRC_ALPHA) &&
           texture_rgb_scale[unit] == 1.0f;
}

static NxglBackendTextureEnvMode native_texture_env_mode_for_unit(int unit)
{
    GLenum mode = texture_env_mode[unit];

    if (mode == GL_COMBINE && default_combine_sources(unit)) {
        switch (texture_combine_rgb[unit]) {
        case GL_REPLACE:
            return NXGL_BACKEND_TEXENV_REPLACE;
        case GL_ADD:
            return NXGL_BACKEND_TEXENV_ADD;
        case GL_ADD_SIGNED:
            return NXGL_BACKEND_TEXENV_ADD_SIGNED;
        case GL_SUBTRACT:
            return NXGL_BACKEND_TEXENV_SUBTRACT;
        case GL_INTERPOLATE:
            return NXGL_BACKEND_TEXENV_INTERPOLATE;
        case GL_MODULATE:
        default:
            return NXGL_BACKEND_TEXENV_MODULATE;
        }
    }

    switch (mode) {
    case GL_REPLACE:
        return NXGL_BACKEND_TEXENV_REPLACE;
    case GL_DECAL:
        return NXGL_BACKEND_TEXENV_DECAL;
    case GL_BLEND:
        return NXGL_BACKEND_TEXENV_BLEND;
    case GL_ADD:
        return NXGL_BACKEND_TEXENV_ADD;
    case GL_MODULATE:
    default:
        return NXGL_BACKEND_TEXENV_MODULATE;
    }
}

static void sync_native_state(void)
{
    TextureObject *texture0 = NULL;
    TextureObject *texture1d0 = NULL;
    TextureObject *texture1 = NULL;
    TextureObject *texture1d1 = NULL;
    nxgl_backend_set_depth(depth_test_enabled, depth_write_enabled);
    nxgl_backend_set_cull(cull_enabled);
    nxgl_backend_set_cull_mode(cull_face_mode, front_face_mode);
    nxgl_backend_set_blend(blend_enabled);
    nxgl_backend_set_blend_func(blend_sfactor, blend_dfactor);
    nxgl_backend_set_scissor(scissor_test_enabled, scissor_box[0], shadow_height - (scissor_box[1] + scissor_box[3]), scissor_box[2], scissor_box[3]);
    nxgl_backend_set_viewport(viewport[0], shadow_height - (viewport[1] + viewport[3]), viewport[2], viewport[3]);
    nxgl_backend_set_texture_env(native_texture_env_mode_for_unit(0), texture_env_color[0]);
    if (texture_2d_enabled[0] &&
        texture_binding_2d[0] > 0 &&
        texture_binding_2d[0] < 16 &&
        texture_objects[texture_binding_2d[0]].allocated) {
        texture0 = &texture_objects[texture_binding_2d[0]];
    }
    if (texture_1d_enabled[0] &&
        texture_binding_1d[0] > 0 &&
        texture_binding_1d[0] < 16 &&
        texture_objects[texture_binding_1d[0]].allocated) {
        texture1d0 = &texture_objects[texture_binding_1d[0]];
    }
    if (texture_2d_enabled[1] &&
        texture_binding_2d[1] > 0 &&
        texture_binding_2d[1] < 16 &&
        texture_objects[texture_binding_2d[1]].allocated) {
        texture1 = &texture_objects[texture_binding_2d[1]];
    }
    if (texture_1d_enabled[1] &&
        texture_binding_1d[1] > 0 &&
        texture_binding_1d[1] < 16 &&
        texture_objects[texture_binding_1d[1]].allocated) {
        texture1d1 = &texture_objects[texture_binding_1d[1]];
    }
    /*
     * Real NV2A hardware rejects the current cube/3D native descriptors at the
     * texture format register. Keep those targets in the GL shadow renderer
     * until the exact hardware encoding is known.
     */
    if (texture0 != NULL &&
        texture0->native.addr != NULL &&
        texture_complete(texture0)) {
        nxgl_backend_bind_texture(&texture0->native);
    } else if (texture1d0 != NULL &&
        texture1d0->base_level == 0 &&
        texture1d0->native_1d.addr != NULL &&
        texture_1d_complete(texture1d0)) {
        nxgl_backend_bind_texture(&texture1d0->native_1d);
    } else {
        nxgl_backend_bind_texture(NULL);
    }
    if (texture1 != NULL &&
        texture1->native.addr != NULL &&
        texture_complete(texture1)) {
        nxgl_backend_bind_texture1(&texture1->native);
    } else if (texture1d1 != NULL &&
        texture1d1->base_level == 0 &&
        texture1d1->native_1d.addr != NULL &&
        texture_1d_complete(texture1d1)) {
        nxgl_backend_bind_texture1(&texture1d1->native_1d);
    } else {
        nxgl_backend_bind_texture1(NULL);
    }
}

static void ensure_native_frame_started(void)
{
    if (!native_frame_started) {
        nxgl_backend_begin_frame(blend_enabled);
        native_frame_started = true;
        sync_native_state();
    }
}

static void transform_point4(const Matrix m, GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat out[4])
{
    out[0] = x * m[M11] + y * m[M21] + z * m[M31] + w * m[M41];
    out[1] = x * m[M12] + y * m[M22] + z * m[M32] + w * m[M42];
    out[2] = x * m[M13] + y * m[M23] + z * m[M33] + w * m[M43];
    out[3] = x * m[M14] + y * m[M24] + z * m[M34] + w * m[M44];
}

static bool projection_is_identity(void)
{
    Matrix identity;

    matrix_identity(identity);
    for (int i = 0; i < 16; ++i) {
        if (fabsf(projection[i] - identity[i]) > 0.00001f) {
            return false;
        }
    }
    return true;
}

static bool projected_path_active(void)
{
    return !projection_is_identity();
}

static void update_projected_position_from_clip(NxglBackendVertex *vertex)
{
    GLfloat inv_w;
    GLfloat ndc_x;
    GLfloat ndc_y;
    GLfloat ndc_z;
    GLfloat native_z;

    if (!projected_path_active() || fabsf(vertex->clip_w) < 0.000001f) {
        return;
    }

    inv_w = 1.0f / vertex->clip_w;
    ndc_x = vertex->clip_x * inv_w;
    ndc_y = vertex->clip_y * inv_w;
    ndc_z = vertex->clip_z * inv_w;
    vertex->window_z = map_depth_range(clamp01(ndc_z * 0.5f + 0.5f));
    native_z = 5.2f + vertex->window_z * 2.0f;
    vertex->pos.x = ndc_x * native_z - camera_x;
    vertex->pos.y = ndc_y * native_z - camera_y;
    vertex->pos.z = -camera_z - native_z;
}

static NxglBackendVec3 transform_vertex(float x, float y, float z)
{
    NxglBackendVec3 out;
    out.x = x * modelview[M11] + y * modelview[M21] + z * modelview[M31] + modelview[M41];
    out.y = x * modelview[M12] + y * modelview[M22] + z * modelview[M32] + modelview[M42];
    out.z = x * modelview[M13] + y * modelview[M23] + z * modelview[M33] + modelview[M43];
    return out;
}

static NxglBackendVec3 transform_normal_to_eye(NxglBackendVec3 normal)
{
    NxglBackendVec3 out;

    if (!modelview_inverse_cache_valid) {
        modelview_inverse_cache_invertible = invert_matrix(modelview_inverse_cache, modelview);
        modelview_inverse_cache_valid = true;
    }

    if (!modelview_inverse_cache_invertible) {
        out.x = normal.x * modelview[M11] + normal.y * modelview[M21] + normal.z * modelview[M31];
        out.y = normal.x * modelview[M12] + normal.y * modelview[M22] + normal.z * modelview[M32];
        out.z = normal.x * modelview[M13] + normal.y * modelview[M23] + normal.z * modelview[M33];
        return out;
    }

    out.x = normal.x * modelview_inverse_cache[M11] + normal.y * modelview_inverse_cache[M21] + normal.z * modelview_inverse_cache[M31];
    out.y = normal.x * modelview_inverse_cache[M12] + normal.y * modelview_inverse_cache[M22] + normal.z * modelview_inverse_cache[M32];
    out.z = normal.x * modelview_inverse_cache[M13] + normal.y * modelview_inverse_cache[M23] + normal.z * modelview_inverse_cache[M33];
    return out;
}

static void init_vertex_position(NxglBackendVertex *out, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLfloat eye[4];
    GLfloat clip[4];

    if (w == 0.0f) {
        w = 1.0f;
    }
    transform_point4(modelview, x, y, z, w, eye);
    out->eye = (NxglBackendVec3){ eye[0], eye[1], eye[2] };
    transform_point4(projection, eye[0], eye[1], eye[2], eye[3], clip);
    out->clip_x = clip[0];
    out->clip_y = clip[1];
    out->clip_z = clip[2];
    out->clip_w = clip[3];

    if (!projected_path_active() || fabsf(clip[3]) < 0.000001f) {
        out->pos = out->eye;
        out->window_z = map_depth_range(eye_depth_normalized(out->eye));
        return;
    }

    update_projected_position_from_clip(out);
}

static NxglBackendVec3 read_array_normal(GLint index)
{
    NxglBackendVec3 normal = current_normal;
    const uint8_t *base;
    int component_size;
    GLsizei stride;

    if (normal_array.enabled && normal_array.pointer != NULL) {
        component_size = type_size(normal_array.type);
        stride = normal_array.stride == 0 ? 3 * component_size : normal_array.stride;
        base = normal_array.pointer + (index * stride);
        normal.x = read_component(base, normal_array.type, false);
        normal.y = read_component(base + component_size, normal_array.type, false);
        normal.z = read_component(base + component_size * 2, normal_array.type, false);
    }
    return normalize_vec3(normal);
}

static NxglBackendVertex read_array_vertex(GLint index)
{
    NxglBackendVertex out;
    NxglBackendVec3 normal;
    GLfloat obj[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const uint8_t *base;
    int component_size;
    GLsizei stride;

    init_vertex_position(&out, 0.0f, 0.0f, 0.0f, 1.0f);
    out.color = current_color;
    out.u = current_u[0];
    out.v = current_v[0];
    out.r = current_r[0];
    out.u1 = current_u[1];
    out.v1 = current_v[1];
    out.r1 = current_r[1];
    out.u2 = current_u[2];
    out.v2 = current_v[2];
    out.r2 = current_r[2];
    out.u3 = current_u[3];
    out.v3 = current_v[3];
    out.r3 = current_r[3];

    if (vertex_array.enabled && vertex_array.pointer != NULL) {
        component_size = type_size(vertex_array.type);
        stride = effective_stride(&vertex_array);
        base = vertex_array.pointer + (index * stride);
        float x = vertex_array.size > 0 ? read_component(base, vertex_array.type, false) : 0.0f;
        float y = vertex_array.size > 1 ? read_component(base + component_size, vertex_array.type, false) : 0.0f;
        float z = vertex_array.size > 2 ? read_component(base + component_size * 2, vertex_array.type, false) : 0.0f;
        obj[0] = x;
        obj[1] = y;
        obj[2] = z;
        obj[3] = vertex_array.size > 3 ? read_component(base + component_size * 3, vertex_array.type, false) : 1.0f;
        init_vertex_position(&out, x, y, z, obj[3]);
    }

    if (color_array.enabled && color_array.pointer != NULL) {
        component_size = type_size(color_array.type);
        stride = effective_stride(&color_array);
        base = color_array.pointer + (index * stride);
        out.color.r = color_array.size > 0 ? read_component(base, color_array.type, color_array.type != GL_FLOAT) : current_color.r;
        out.color.g = color_array.size > 1 ? read_component(base + component_size, color_array.type, color_array.type != GL_FLOAT) : current_color.g;
        out.color.b = color_array.size > 2 ? read_component(base + component_size * 2, color_array.type, color_array.type != GL_FLOAT) : current_color.b;
        out.color.a = color_array.size > 3 ? read_component(base + component_size * 3, color_array.type, color_array.type != GL_FLOAT) : current_color.a;
    }

    if (texcoord_array[0].enabled && texcoord_array[0].pointer != NULL) {
        component_size = type_size(texcoord_array[0].type);
        stride = effective_stride(&texcoord_array[0]);
        base = texcoord_array[0].pointer + (index * stride);
        out.u = texcoord_array[0].size > 0 ? read_component(base, texcoord_array[0].type, false) : current_u[0];
        out.v = texcoord_array[0].size > 1 ? read_component(base + component_size, texcoord_array[0].type, false) : current_v[0];
        out.r = texcoord_array[0].size > 2 ? read_component(base + component_size * 2, texcoord_array[0].type, false) : current_r[0];
    }
    if (texcoord_array[1].enabled && texcoord_array[1].pointer != NULL) {
        component_size = type_size(texcoord_array[1].type);
        stride = effective_stride(&texcoord_array[1]);
        base = texcoord_array[1].pointer + (index * stride);
        out.u1 = texcoord_array[1].size > 0 ? read_component(base, texcoord_array[1].type, false) : current_u[1];
        out.v1 = texcoord_array[1].size > 1 ? read_component(base + component_size, texcoord_array[1].type, false) : current_v[1];
        out.r1 = texcoord_array[1].size > 2 ? read_component(base + component_size * 2, texcoord_array[1].type, false) : current_r[1];
    }
    if (texcoord_array[2].enabled && texcoord_array[2].pointer != NULL) {
        component_size = type_size(texcoord_array[2].type);
        stride = effective_stride(&texcoord_array[2]);
        base = texcoord_array[2].pointer + (index * stride);
        out.u2 = texcoord_array[2].size > 0 ? read_component(base, texcoord_array[2].type, false) : current_u[2];
        out.v2 = texcoord_array[2].size > 1 ? read_component(base + component_size, texcoord_array[2].type, false) : current_v[2];
        out.r2 = texcoord_array[2].size > 2 ? read_component(base + component_size * 2, texcoord_array[2].type, false) : current_r[2];
    }
    if (texcoord_array[3].enabled && texcoord_array[3].pointer != NULL) {
        component_size = type_size(texcoord_array[3].type);
        stride = effective_stride(&texcoord_array[3]);
        base = texcoord_array[3].pointer + (index * stride);
        out.u3 = texcoord_array[3].size > 0 ? read_component(base, texcoord_array[3].type, false) : current_u[3];
        out.v3 = texcoord_array[3].size > 1 ? read_component(base + component_size, texcoord_array[3].type, false) : current_v[3];
        out.r3 = texcoord_array[3].size > 2 ? read_component(base + component_size * 2, texcoord_array[3].type, false) : current_r[3];
    }

    normal = transform_normal_to_eye(read_array_normal(index));
    out.base_color = out.color;
    out.normal = normal;
    apply_texgen_to_coords(obj, out.eye, normal, &out.u, &out.v, &out.r, 0);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u1, &out.v1, &out.r1, 1);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u2, &out.v2, &out.r2, 2);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u3, &out.v3, &out.r3, 3);
    out.color = lit_color(out.color, normal, out.eye);
    out.color = apply_fog(out.color, out.eye);
    return out;
}

static bool primitive_intersects_viewport(const NxglBackendVertex *vertices, int count)
{
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int viewport_min_x;
    int viewport_min_y;
    int viewport_max_x;
    int viewport_max_y;

    if (count <= 0 || vertices == NULL) {
        return false;
    }
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (primitive_rejected_by_clip_planes(vertices, count)) {
        return false;
    }
    if (projected_path_active() && count == 1) {
        for (int plane = 0; plane < 6; ++plane) {
            if (feedback_clip_plane_value(&vertices[0], plane) < 0.0f) {
                return false;
            }
        }
    }
    for (int i = 0; i < count; ++i) {
        int sx = 0;
        int sy = 0;
        if (!shadow_project(vertices[i].pos, &sx, &sy)) {
            return false;
        }
        if (i == 0) {
            min_x = max_x = sx;
            min_y = max_y = sy;
        } else {
            if (sx < min_x) min_x = sx;
            if (sx > max_x) max_x = sx;
            if (sy < min_y) min_y = sy;
            if (sy > max_y) max_y = sy;
        }
    }
    viewport_top_left_bounds(&viewport_min_x, &viewport_min_y, &viewport_max_x, &viewport_max_y);
    return max_x >= viewport_min_x &&
           max_y >= viewport_min_y &&
           min_x <= viewport_max_x &&
           min_y <= viewport_max_y;
}

static NxglBackendVertex interpolate_vertex(const NxglBackendVertex *a, const NxglBackendVertex *b, GLfloat t)
{
    NxglBackendVertex out;

    out.pos.x = a->pos.x + (b->pos.x - a->pos.x) * t;
    out.pos.y = a->pos.y + (b->pos.y - a->pos.y) * t;
    out.pos.z = a->pos.z + (b->pos.z - a->pos.z) * t;
    out.eye.x = a->eye.x + (b->eye.x - a->eye.x) * t;
    out.eye.y = a->eye.y + (b->eye.y - a->eye.y) * t;
    out.eye.z = a->eye.z + (b->eye.z - a->eye.z) * t;
    out.color.r = a->color.r + (b->color.r - a->color.r) * t;
    out.color.g = a->color.g + (b->color.g - a->color.g) * t;
    out.color.b = a->color.b + (b->color.b - a->color.b) * t;
    out.color.a = a->color.a + (b->color.a - a->color.a) * t;
    out.base_color.r = a->base_color.r + (b->base_color.r - a->base_color.r) * t;
    out.base_color.g = a->base_color.g + (b->base_color.g - a->base_color.g) * t;
    out.base_color.b = a->base_color.b + (b->base_color.b - a->base_color.b) * t;
    out.base_color.a = a->base_color.a + (b->base_color.a - a->base_color.a) * t;
    out.normal.x = a->normal.x + (b->normal.x - a->normal.x) * t;
    out.normal.y = a->normal.y + (b->normal.y - a->normal.y) * t;
    out.normal.z = a->normal.z + (b->normal.z - a->normal.z) * t;
    out.u = a->u + (b->u - a->u) * t;
    out.v = a->v + (b->v - a->v) * t;
    out.r = a->r + (b->r - a->r) * t;
    out.u1 = a->u1 + (b->u1 - a->u1) * t;
    out.v1 = a->v1 + (b->v1 - a->v1) * t;
    out.r1 = a->r1 + (b->r1 - a->r1) * t;
    out.u2 = a->u2 + (b->u2 - a->u2) * t;
    out.v2 = a->v2 + (b->v2 - a->v2) * t;
    out.r2 = a->r2 + (b->r2 - a->r2) * t;
    out.u3 = a->u3 + (b->u3 - a->u3) * t;
    out.v3 = a->v3 + (b->v3 - a->v3) * t;
    out.r3 = a->r3 + (b->r3 - a->r3) * t;
    out.clip_x = a->clip_x + (b->clip_x - a->clip_x) * t;
    out.clip_y = a->clip_y + (b->clip_y - a->clip_y) * t;
    out.clip_z = a->clip_z + (b->clip_z - a->clip_z) * t;
    out.clip_w = a->clip_w + (b->clip_w - a->clip_w) * t;
    out.window_z = a->window_z + (b->window_z - a->window_z) * t;
    update_projected_position_from_clip(&out);
    return out;
}

static bool clip_line_interval(GLfloat f0, GLfloat f1, GLfloat *t0, GLfloat *t1)
{
    GLfloat delta = f1 - f0;
    GLfloat t;

    if (f0 < 0.0f && f1 < 0.0f) {
        return false;
    }
    if (delta > -0.000001f && delta < 0.000001f) {
        return f0 >= 0.0f;
    }

    t = -f0 / delta;
    if (f0 < 0.0f) {
        if (t > *t0) {
            *t0 = t;
        }
    } else if (f1 < 0.0f) {
        if (t < *t1) {
            *t1 = t;
        }
    }
    return *t0 <= *t1;
}

static bool clip_line_to_feedback_volume(const NxglBackendVertex *in_a, const NxglBackendVertex *in_b, NxglBackendVertex *out_a, NxglBackendVertex *out_b)
{
    GLfloat t0 = 0.0f;
    GLfloat t1 = 1.0f;

    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (projected_path_active()) {
        if (!clip_line_interval(in_a->clip_z + in_a->clip_w, in_b->clip_z + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_z, in_b->clip_w - in_b->clip_z, &t0, &t1) ||
            !clip_line_interval(in_a->clip_x + in_a->clip_w, in_b->clip_x + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_x, in_b->clip_w - in_b->clip_x, &t0, &t1) ||
            !clip_line_interval(in_a->clip_y + in_a->clip_w, in_b->clip_y + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_y, in_b->clip_w - in_b->clip_y, &t0, &t1)) {
            return false;
        }
    } else {
        GLfloat ax = in_a->pos.x + camera_x;
        GLfloat ay = in_a->pos.y + camera_y;
        GLfloat az = -(in_a->pos.z + camera_z);
        GLfloat bx = in_b->pos.x + camera_x;
        GLfloat by = in_b->pos.y + camera_y;
        GLfloat bz = -(in_b->pos.z + camera_z);
        if (!clip_line_interval(az - 0.1f, bz - 0.1f, &t0, &t1) ||
            !clip_line_interval(ax + az, bx + bz, &t0, &t1) ||
            !clip_line_interval(az - ax, bz - bx, &t0, &t1) ||
            !clip_line_interval(ay + az, by + bz, &t0, &t1) ||
            !clip_line_interval(az - ay, bz - by, &t0, &t1)) {
            return false;
        }
    }

    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        const GLdouble *plane = clip_planes[i].equation;
        GLfloat f0;
        GLfloat f1;

        if (!clip_planes[i].enabled) {
            continue;
        }
        f0 = (GLfloat)(plane[0] * in_a->eye.x + plane[1] * in_a->eye.y + plane[2] * in_a->eye.z + plane[3]);
        f1 = (GLfloat)(plane[0] * in_b->eye.x + plane[1] * in_b->eye.y + plane[2] * in_b->eye.z + plane[3]);
        if (!clip_line_interval(f0, f1, &t0, &t1)) {
            return false;
        }
    }

    if (t1 < t0) {
        return false;
    }
    {
        NxglBackendVertex clipped[2];
        clipped[0] = interpolate_vertex(in_a, in_b, t0);
        clipped[1] = interpolate_vertex(in_a, in_b, t1);
        normalize_lower_feedback_clip_edges(clipped, 2);
        *out_a = clipped[0];
        *out_b = clipped[1];
    }
    return true;
}

static void normalize_lower_feedback_clip_edge(NxglBackendVertex *vertex)
{
    GLfloat z;
    GLfloat y;

    if (vertex == NULL || projected_path_active()) {
        return;
    }

    z = -(vertex->pos.z + camera_z);
    y = vertex->pos.y + camera_y;
    if (z >= 0.0999f && fabsf(y + z) <= 0.002f) {
        vertex->pos.y = z - camera_y;
        vertex->eye.y = vertex->pos.y;
    }
}

static void normalize_lower_feedback_clip_edges(NxglBackendVertex *vertices, int count)
{
    bool touches_upper = false;

    if (vertices == NULL || count <= 0 || projected_path_active()) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        GLfloat z = -(vertices[i].pos.z + camera_z);
        GLfloat y = vertices[i].pos.y + camera_y;
        if (z >= 0.0999f && fabsf(z - y) <= 0.002f) {
            touches_upper = true;
            break;
        }
    }
    if (touches_upper) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        normalize_lower_feedback_clip_edge(&vertices[i]);
    }
}

static GLfloat feedback_clip_plane_value(const NxglBackendVertex *vertex, int plane)
{
    if (projected_path_active()) {
        switch (plane) {
        case 0:
            return vertex->clip_z + vertex->clip_w;
        case 1:
            return vertex->clip_w - vertex->clip_z;
        case 2:
            return vertex->clip_x + vertex->clip_w;
        case 3:
            return vertex->clip_w - vertex->clip_x;
        case 4:
            return vertex->clip_y + vertex->clip_w;
        case 5:
            return vertex->clip_w - vertex->clip_y;
        default: {
            int index = plane - 6;
            const GLdouble *p = clip_planes[index].equation;
            return (GLfloat)(p[0] * vertex->eye.x + p[1] * vertex->eye.y + p[2] * vertex->eye.z + p[3]);
        }
        }
    } else {
        GLfloat x = vertex->pos.x + camera_x;
        GLfloat y = vertex->pos.y + camera_y;
        GLfloat z = -(vertex->pos.z + camera_z);

        switch (plane) {
        case 0:
            return z - 0.1f;
        case 1:
            return x + z;
        case 2:
            return z - x;
        case 3:
            return y + z;
        case 4:
            return z - y;
        default: {
            int index = plane - 5;
            const GLdouble *p = clip_planes[index].equation;
            return (GLfloat)(p[0] * vertex->eye.x + p[1] * vertex->eye.y + p[2] * vertex->eye.z + p[3]);
        }
        }
    }
}

static bool clip_polygon_against_feedback_plane(const NxglBackendVertex *input, int input_count, NxglBackendVertex *output, int *output_count, int plane)
{
    NxglBackendVertex previous;
    GLfloat previous_value;
    bool previous_inside;
    int out_count = 0;

    if (input_count <= 0) {
        *output_count = 0;
        return false;
    }

    previous = input[input_count - 1];
    previous_value = feedback_clip_plane_value(&previous, plane);
    previous_inside = previous_value >= 0.0f;

    for (int i = 0; i < input_count; ++i) {
        NxglBackendVertex current = input[i];
        GLfloat current_value = feedback_clip_plane_value(&current, plane);
        bool current_inside = current_value >= 0.0f;

        if (current_inside != previous_inside) {
            GLfloat denom = previous_value - current_value;
            GLfloat t = denom > -0.000001f && denom < 0.000001f ? 0.0f : previous_value / denom;
            if (out_count >= NXGL_CLIPPED_POLYGON_MAX) {
                return false;
            }
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            output[out_count++] = interpolate_vertex(&previous, &current, t);
        }
        if (current_inside) {
            if (out_count >= NXGL_CLIPPED_POLYGON_MAX) {
                return false;
            }
            output[out_count++] = current;
        }

        previous = current;
        previous_value = current_value;
        previous_inside = current_inside;
    }

    *output_count = out_count;
    return out_count >= 3;
}

static bool clip_polygon_to_feedback_volume(const NxglBackendVertex *vertices, int count, NxglBackendVertex *output, int *output_count)
{
    NxglBackendVertex scratch_a[NXGL_CLIPPED_POLYGON_MAX];
    NxglBackendVertex scratch_b[NXGL_CLIPPED_POLYGON_MAX];
    NxglBackendVertex *in = scratch_a;
    NxglBackendVertex *out = scratch_b;
    int in_count = count;
    int out_count = 0;

    if (vertices == NULL || count < 3 || count > NXGL_CLIPPED_POLYGON_MAX || viewport[2] <= 0 || viewport[3] <= 0) {
        *output_count = 0;
        return false;
    }

    memcpy(scratch_a, vertices, (size_t)count * sizeof(NxglBackendVertex));
    int fixed_plane_count = projected_path_active() ? 6 : 5;
    for (int plane = 0; plane < fixed_plane_count; ++plane) {
        if (!clip_polygon_against_feedback_plane(in, in_count, out, &out_count, plane)) {
            *output_count = 0;
            return false;
        }
        NxglBackendVertex *tmp = in;
        in = out;
        out = tmp;
        in_count = out_count;
    }

    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        if (!clip_planes[i].enabled) {
            continue;
        }
        if (!clip_polygon_against_feedback_plane(in, in_count, out, &out_count, fixed_plane_count + i)) {
            *output_count = 0;
            return false;
        }
        NxglBackendVertex *tmp = in;
        in = out;
        out = tmp;
        in_count = out_count;
    }

    memcpy(output, in, (size_t)in_count * sizeof(NxglBackendVertex));
    *output_count = in_count;
    normalize_lower_feedback_clip_edges(output, in_count);
    return in_count >= 3;
}

static GLuint selection_depth_value(const NxglBackendVertex *vertex)
{
    float normalized = projected_path_active() ? vertex->window_z : map_depth_range(eye_depth_normalized(vertex->pos));
    return (GLuint)(normalized * 4294967295.0f);
}

static void record_selection_hit(const NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex clipped[2];
    NxglBackendVertex clipped_polygon[NXGL_CLIPPED_POLYGON_MAX];
    const NxglBackendVertex *depth_vertices = vertices;
    int depth_count = count;
    GLuint min_z;
    GLuint max_z;
    GLsizei needed;

    if (render_mode != GL_SELECT) {
        return;
    }
    if (count == 2) {
        if (vertices == NULL || !clip_line_to_feedback_volume(&vertices[0], &vertices[1], &clipped[0], &clipped[1])) {
            return;
        }
        depth_vertices = clipped;
        depth_count = 2;
    } else if (count >= 3) {
        if (!clip_polygon_to_feedback_volume(vertices, count, clipped_polygon, &depth_count)) {
            return;
        }
        depth_vertices = clipped_polygon;
    } else {
        if (!primitive_intersects_viewport(vertices, count)) {
            return;
        }
    }

    min_z = 0xffffffffu;
    max_z = 0u;
    for (int i = 0; i < depth_count; ++i) {
        GLuint z = selection_depth_value(&depth_vertices[i]);
        if (z < min_z) {
            min_z = z;
        }
        if (z > max_z) {
            max_z = z;
        }
    }

    needed = 3 + name_stack_depth;
    ++selection_hits;
    if (selection_buffer == NULL ||
        selection_buffer_size < 0 ||
        selection_write_count + needed > selection_buffer_size) {
        selection_overflow = true;
        return;
    }

    selection_buffer[selection_write_count++] = (GLuint)name_stack_depth;
    selection_buffer[selection_write_count++] = min_z;
    selection_buffer[selection_write_count++] = max_z;
    for (GLint i = 0; i < name_stack_depth; ++i) {
        selection_buffer[selection_write_count++] = name_stack[i];
    }
}

static void feedback_write(GLfloat value)
{
    if (feedback_buffer == NULL || feedback_buffer_size < 0 || feedback_overflow) {
        feedback_overflow = true;
        return;
    }
    if (feedback_write_count >= feedback_buffer_size) {
        feedback_overflow = true;
        return;
    }
    feedback_buffer[feedback_write_count++] = value;
}

static bool project_depth_for_window(NxglBackendVec3 pos, GLfloat *z_out)
{
    GLfloat z = -(pos.z + camera_z);

    if (z < 0.0999f || viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (z < 0.1f) {
        z = 0.1f;
    }
    *z_out = z;
    return true;
}

static void snap_feedback_viewport_edges(GLfloat *x)
{
    GLfloat left = (GLfloat)viewport[0];
    GLfloat right = (GLfloat)(viewport[0] + viewport[2]);

    if (*x >= left - 1.0f && *x <= left + 3.0f) {
        *x = left;
    } else if (*x >= right - 4.0f && *x <= right + 1.0f) {
        *x = right;
    }
}

static bool feedback_project_window(NxglBackendVec3 pos, GLfloat *sx, GLfloat *sy)
{
    GLfloat z;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    *sx = (GLfloat)viewport[0] + ((pos.x + camera_x) / z) * (GLfloat)viewport[2] * 0.5f + (GLfloat)viewport[2] * 0.5f;
    *sy = ((pos.y + camera_y) / z) * (GLfloat)shadow_height * 0.5f + (GLfloat)shadow_height * 0.5f;
    snap_feedback_viewport_edges(sx);
    return true;
}

static void feedback_write_vertex(const NxglBackendVertex *vertex)
{
    GLfloat x = vertex->pos.x;
    GLfloat y = vertex->pos.y;
    GLfloat z = projected_path_active() ? vertex->window_z : map_depth_range(eye_depth_normalized(vertex->pos));

    if (feedback_project_window(vertex->pos, &x, &y)) {
        /* x/y updated by helper */
    }

    feedback_write(x);
    feedback_write(y);
    if (feedback_type == GL_2D) {
        return;
    }
    feedback_write(z);
    if (feedback_type == GL_3D) {
        return;
    }
    if (feedback_type == GL_4D_COLOR_TEXTURE) {
        feedback_write(1.0f);
    }
    feedback_write(clamp01(vertex->color.r));
    feedback_write(clamp01(vertex->color.g));
    feedback_write(clamp01(vertex->color.b));
    feedback_write(clamp01(vertex->color.a));
    if (feedback_type == GL_3D_COLOR) {
        return;
    }
    feedback_write(vertex->u);
    feedback_write(vertex->v);
    feedback_write(vertex->r);
    feedback_write(1.0f);
    if (feedback_type == GL_3D_COLOR_TEXTURE) {
        return;
    }
}

static void feedback_write_raster_vertex(void)
{
    feedback_write(current_raster_position[0]);
    feedback_write(current_raster_position[1]);
    if (feedback_type == GL_2D) {
        return;
    }
    feedback_write(current_raster_position[2]);
    if (feedback_type == GL_3D) {
        return;
    }
    if (feedback_type == GL_4D_COLOR_TEXTURE) {
        feedback_write(current_raster_position[3]);
    }
    feedback_write(clamp01(current_color.r));
    feedback_write(clamp01(current_color.g));
    feedback_write(clamp01(current_color.b));
    feedback_write(clamp01(current_color.a));
    if (feedback_type == GL_3D_COLOR) {
        return;
    }
    feedback_write(current_u[0]);
    feedback_write(current_v[0]);
    feedback_write(current_r[0]);
    feedback_write(1.0f);
}

static void record_feedback_point(const NxglBackendVertex *vertex)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!primitive_intersects_viewport(vertex, 1)) {
        return;
    }
    feedback_write((GLfloat)GL_POINT_TOKEN);
    feedback_write_vertex(vertex);
}

static void record_feedback_line(const NxglBackendVertex *a, const NxglBackendVertex *b)
{
    NxglBackendVertex clipped_a;
    NxglBackendVertex clipped_b;

    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!clip_line_to_feedback_volume(a, b, &clipped_a, &clipped_b)) {
        return;
    }
    feedback_write((GLfloat)GL_LINE_TOKEN);
    feedback_write_vertex(&clipped_a);
    feedback_write_vertex(&clipped_b);
}

static void rotate_feedback_polygon_near_first(NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex scratch[NXGL_CLIPPED_POLYGON_MAX];
    int best = -1;
    GLfloat best_z = 1.0f;

    if (vertices == NULL || count <= 0 || count > NXGL_CLIPPED_POLYGON_MAX || projected_path_active()) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        GLfloat z = map_depth_range(eye_depth_normalized(vertices[i].pos));
        if (z < best_z) {
            best_z = z;
            best = i;
        }
    }
    if (best <= 0 || best_z > 0.01f) {
        return;
    }

    memcpy(scratch, vertices, (size_t)count * sizeof(NxglBackendVertex));
    for (int i = 0; i < count; ++i) {
        vertices[i] = scratch[(best + i) % count];
    }
}

static void trim_feedback_polygon_near_quad(NxglBackendVertex *vertices, int *count)
{
    if (vertices == NULL || count == NULL || *count <= 4 || projected_path_active()) {
        return;
    }
    if (map_depth_range(eye_depth_normalized(vertices[0].pos)) <= 0.01f) {
        *count = 4;
    }
}

static void record_feedback_polygon(const NxglBackendVertex *vertices, GLsizei count)
{
    NxglBackendVertex clipped[NXGL_CLIPPED_POLYGON_MAX];
    int clipped_count = 0;

    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!clip_polygon_to_feedback_volume(vertices, count, clipped, &clipped_count)) {
        return;
    }
    rotate_feedback_polygon_near_first(clipped, clipped_count);
    trim_feedback_polygon_near_quad(clipped, &clipped_count);
    feedback_write((GLfloat)GL_POLYGON_TOKEN);
    feedback_write((GLfloat)clipped_count);
    for (GLsizei i = 0; i < clipped_count; ++i) {
        feedback_write_vertex(&clipped[i]);
    }
}

static void record_feedback_pass_through(GLfloat token)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    feedback_write((GLfloat)GL_PASS_THROUGH_TOKEN);
    feedback_write(token);
}

static void record_feedback_pixel_token(GLenum token)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    feedback_write((GLfloat)token);
    feedback_write_raster_vertex();
}

static bool project_window_for_winding(NxglBackendVec3 pos, GLfloat *sx, GLfloat *sy)
{
    GLfloat z;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    *sx = (GLfloat)viewport[0] + ((pos.x + camera_x) / z) * (GLfloat)viewport[2] * 0.5f + (GLfloat)viewport[2] * 0.5f;
    *sy = (GLfloat)viewport[1] + ((pos.y + camera_y) / z) * (GLfloat)viewport[3] * 0.5f + (GLfloat)viewport[3] * 0.5f;
    return true;
}

static bool polygon_culled(const NxglBackendVertex *vertices, int count)
{
    GLfloat xs[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat ys[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat area = 0.0f;
    bool front;

    if (!cull_enabled || count < 3) {
        return false;
    }
    if (cull_face_mode == GL_FRONT_AND_BACK) {
        return true;
    }
    if (count > NXGL_CLIPPED_POLYGON_MAX) {
        count = NXGL_CLIPPED_POLYGON_MAX;
    }
    for (int i = 0; i < count; ++i) {
        if (!project_window_for_winding(vertices[i].pos, &xs[i], &ys[i])) {
            return false;
        }
    }
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        area += xs[i] * ys[next] - xs[next] * ys[i];
    }
    if (fabsf(area) < 0.0001f) {
        return false;
    }
    front = front_face_mode == GL_CCW ? area > 0.0f : area < 0.0f;
    return cull_face_mode == GL_FRONT ? front : !front;
}

static bool polygon_front_facing(const NxglBackendVertex *vertices, int count)
{
    GLfloat xs[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat ys[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat area = 0.0f;

    if (count < 3) {
        return true;
    }
    if (count > NXGL_CLIPPED_POLYGON_MAX) {
        count = NXGL_CLIPPED_POLYGON_MAX;
    }
    for (int i = 0; i < count; ++i) {
        if (!project_window_for_winding(vertices[i].pos, &xs[i], &ys[i])) {
            return true;
        }
    }
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        area += xs[i] * ys[next] - xs[next] * ys[i];
    }
    if (fabsf(area) < 0.0001f) {
        return true;
    }
    return front_face_mode == GL_CCW ? area > 0.0f : area < 0.0f;
}

static void apply_two_sided_lighting(NxglBackendVertex *vertices, int count)
{
    bool front;

    if (!lighting_enabled || !light_model_two_side || count < 3) {
        return;
    }
    front = polygon_front_facing(vertices, count);
    for (int i = 0; i < count; ++i) {
        NxglBackendVec3 normal = vertices[i].normal;
        const MaterialState *material = &material_state;
        if (!front) {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
            material = &material_back_state;
        }
        vertices[i].color = lit_color_with_material(vertices[i].base_color, normal, vertices[i].eye, material);
        vertices[i].color = apply_fog(vertices[i].color, vertices[i].eye);
    }
}

static void emit_clipped_fill_polygon(const NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex clipped[NXGL_CLIPPED_POLYGON_MAX];
    int clipped_count = 0;

    if (!clip_polygon_to_feedback_volume(vertices, count, clipped, &clipped_count)) {
        return;
    }
    if (polygon_culled(clipped, clipped_count)) {
        return;
    }

    if (clipped_count == 4) {
        shadow_fill_bounds(clipped[0], clipped[1], clipped[2], clipped[3], true, false);
        ensure_native_frame_started();
        nxgl_backend_push_quad(clipped[0], clipped[1], clipped[2], clipped[3]);
        perf_backend_quad();
        return;
    }

    for (int i = 1; i + 1 < clipped_count; ++i) {
        NxglBackendVertex a = clipped[0];
        NxglBackendVertex b = clipped[i];
        NxglBackendVertex c = clipped[i + 1];
        shadow_fill_bounds(a, b, c, c, false, false);
        ensure_native_frame_started();
        nxgl_backend_push_triangle(a, b, c);
        perf_backend_triangle();
    }
}

static void emit_array_triangle(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c)
{
    NxglBackendVertex vertices[3] = { a, b, c };
    if (primitive_rejected_by_clip_planes(vertices, 3)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 3);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 3);
        return;
    }
    emit_clipped_fill_polygon(vertices, 3);
}

static void emit_array_quad(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d)
{
    NxglBackendVertex vertices[4] = { a, b, c, d };
    if (primitive_rejected_by_clip_planes(vertices, 4)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 4);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 4);
        return;
    }
    emit_clipped_fill_polygon(vertices, 4);
}

static void emit_point_vertex(NxglBackendVertex v)
{
    float half = point_size * 0.0125f;
    NxglBackendVertex a = v;
    NxglBackendVertex b = v;
    NxglBackendVertex c = v;
    NxglBackendVertex d = v;

    if (primitive_rejected_by_clip_planes(&v, 1)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(&v, 1);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_point(&v);
        return;
    }
    if (!primitive_intersects_viewport(&v, 1)) {
        return;
    }

    a.pos.x -= half; a.pos.y += half;
    b.pos.x += half; b.pos.y += half;
    c.pos.x += half; c.pos.y -= half;
    d.pos.x -= half; d.pos.y -= half;
    {
        NxglBackendVertex vertices[4] = { a, b, c, d };
        emit_clipped_fill_polygon(vertices, 4);
    }
}

static void emit_degenerate_line_vertex(NxglBackendVertex v)
{
    GLfloat z;
    float half_x;
    float half_y;
    NxglBackendVertex a = v;
    NxglBackendVertex b = v;
    NxglBackendVertex c = v;
    NxglBackendVertex d = v;

    if (!project_depth_for_window(v.pos, &z)) {
        return;
    }

    half_x = line_width * z / (float)viewport[2];
    half_y = line_width * z / (float)viewport[3];
    a.pos.x -= half_x; a.pos.y += half_y;
    b.pos.x += half_x; b.pos.y += half_y;
    c.pos.x += half_x; c.pos.y -= half_y;
    d.pos.x -= half_x; d.pos.y -= half_y;
    shadow_fill_bounds(a, b, c, d, true, true);
    ensure_native_frame_started();
    nxgl_backend_push_quad(a, b, c, d);
    perf_backend_quad();
}

static void emit_line_vertices(NxglBackendVertex a, NxglBackendVertex b)
{
    NxglBackendVertex clipped_a;
    NxglBackendVertex clipped_b;
    float dx;
    float dy;
    float len;
    float half = line_width * 0.01f;
    float nx;
    float ny;
    NxglBackendVertex v0;
    NxglBackendVertex v1;
    NxglBackendVertex v2;
    NxglBackendVertex v3;
    NxglBackendVertex vertices[2] = { a, b };

    if (primitive_rejected_by_clip_planes(vertices, 2)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 2);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_line(&a, &b);
        return;
    }

    if (!clip_line_to_feedback_volume(&a, &b, &clipped_a, &clipped_b)) {
        return;
    }

    dx = clipped_b.pos.x - clipped_a.pos.x;
    dy = clipped_b.pos.y - clipped_a.pos.y;
    len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) {
        emit_degenerate_line_vertex(clipped_a);
        return;
    }

    nx = -dy / len * half;
    ny = dx / len * half;
    v0 = clipped_a;
    v1 = clipped_b;
    v2 = clipped_b;
    v3 = clipped_a;
    v0.pos.x += nx; v0.pos.y += ny;
    v1.pos.x += nx; v1.pos.y += ny;
    v2.pos.x -= nx; v2.pos.y -= ny;
    v3.pos.x -= nx; v3.pos.y -= ny;
    shadow_fill_bounds(v0, v1, v2, v3, true, true);
    ensure_native_frame_started();
    nxgl_backend_push_quad(v0, v1, v2, v3);
    perf_backend_quad();
}

static void emit_triangle_vertices(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c)
{
    NxglBackendVertex vertices[3];
    if (shade_model == GL_FLAT) {
        a.color = c.color;
        b.color = c.color;
    }
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    apply_two_sided_lighting(vertices, 3);
    if (shade_model == GL_FLAT) {
        vertices[0].color = vertices[2].color;
        vertices[1].color = vertices[2].color;
    }
    if (native_fast_fill_enabled()) {
        ensure_native_frame_started();
        nxgl_backend_push_triangle(vertices[0], vertices[1], vertices[2]);
        perf_backend_triangle();
        return;
    }
    if (primitive_rejected_by_clip_planes(vertices, 3)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 3);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 3);
        return;
    }
    if (polygon_mode == GL_POINT) {
        emit_point_vertex(a);
        emit_point_vertex(b);
        emit_point_vertex(c);
    } else if (polygon_mode == GL_LINE) {
        emit_line_vertices(a, b);
        emit_line_vertices(b, c);
        emit_line_vertices(c, a);
    } else {
        emit_clipped_fill_polygon(vertices, 3);
    }
}

static void emit_quad_vertices(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d)
{
    NxglBackendVertex vertices[4];
    if (shade_model == GL_FLAT) {
        a.color = d.color;
        b.color = d.color;
        c.color = d.color;
    }
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    vertices[3] = d;
    apply_two_sided_lighting(vertices, 4);
    if (shade_model == GL_FLAT) {
        vertices[0].color = vertices[3].color;
        vertices[1].color = vertices[3].color;
        vertices[2].color = vertices[3].color;
    }
    if (native_fast_fill_enabled()) {
        ensure_native_frame_started();
        nxgl_backend_push_quad(vertices[0], vertices[1], vertices[2], vertices[3]);
        perf_backend_quad();
        return;
    }
    if (primitive_rejected_by_clip_planes(vertices, 4)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 4);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 4);
        return;
    }
    if (polygon_mode == GL_POINT) {
        emit_point_vertex(a);
        emit_point_vertex(b);
        emit_point_vertex(c);
        emit_point_vertex(d);
    } else if (polygon_mode == GL_LINE) {
        emit_line_vertices(a, b);
        emit_line_vertices(b, c);
        emit_line_vertices(c, d);
        emit_line_vertices(d, a);
    } else {
        emit_clipped_fill_polygon(vertices, 4);
    }
}

static void emit_vertices(GLenum mode, const NxglBackendVertex *vertices, GLsizei count)
{
    if (mode == GL_TRIANGLES) {
        for (GLsizei i = 0; i + 2 < count; i += 3) {
            emit_triangle_vertices(vertices[i], vertices[i + 1], vertices[i + 2]);
        }
    } else if (mode == GL_QUADS) {
        for (GLsizei i = 0; i + 3 < count; i += 4) {
            emit_quad_vertices(vertices[i], vertices[i + 1], vertices[i + 2], vertices[i + 3]);
        }
    } else if (mode == GL_TRIANGLE_STRIP) {
        for (GLsizei i = 2; i < count; ++i) {
            if ((i & 1) == 0) {
                emit_triangle_vertices(vertices[i - 2], vertices[i - 1], vertices[i]);
            } else {
                emit_triangle_vertices(vertices[i - 1], vertices[i - 2], vertices[i]);
            }
        }
    } else if (mode == GL_TRIANGLE_FAN || mode == GL_POLYGON) {
        for (GLsizei i = 2; i < count; ++i) {
            emit_triangle_vertices(vertices[0], vertices[i - 1], vertices[i]);
        }
    } else if (mode == GL_QUAD_STRIP) {
        for (GLsizei i = 2; i + 1 < count; i += 2) {
            emit_quad_vertices(vertices[i - 2], vertices[i - 1], vertices[i + 1], vertices[i]);
        }
    } else if (mode == GL_POINTS) {
        for (GLsizei i = 0; i < count; ++i) {
            emit_point_vertex(vertices[i]);
        }
    } else if (mode == GL_LINES) {
        for (GLsizei i = 0; i + 1 < count; i += 2) {
            emit_line_vertices(vertices[i], vertices[i + 1]);
        }
    } else if (mode == GL_LINE_STRIP) {
        for (GLsizei i = 1; i < count; ++i) {
            emit_line_vertices(vertices[i - 1], vertices[i]);
        }
    } else if (mode == GL_LINE_LOOP) {
        for (GLsizei i = 1; i < count; ++i) {
            emit_line_vertices(vertices[i - 1], vertices[i]);
        }
        if (count > 2) {
            emit_line_vertices(vertices[count - 1], vertices[0]);
        }
    }
}

static void emit_array_vertices(GLenum mode, const GLint *indices, GLsizei count)
{
    NxglBackendVertex *vertices;

    if (count <= 0) {
        return;
    }
    vertices = (NxglBackendVertex *)malloc((size_t)count * sizeof(NxglBackendVertex));
    if (vertices == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        vertices[i] = read_array_vertex(indices[i]);
    }
    emit_vertices(mode, vertices, count);
    free(vertices);
}


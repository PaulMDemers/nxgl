void glMatrixMode(uint32_t mode)
{
    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_MODELVIEW && mode != GL_PROJECTION && mode != GL_TEXTURE) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_MATRIX_MODE };
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }

    matrix_mode = mode;
}

void glLoadIdentity(void)
{
    ListCommand command = { LIST_CMD_LOAD_IDENTITY };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    matrix_identity(*current_matrix());
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

void glPushMatrix(void)
{
    ListCommand command = { LIST_CMD_PUSH_MATRIX };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (matrix_mode == GL_MODELVIEW) {
        if (modelview_stack_top >= 31) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(modelview_stack[modelview_stack_top++], modelview, sizeof(Matrix));
    } else if (matrix_mode == GL_PROJECTION) {
        if (projection_stack_top >= 7) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(projection_stack[projection_stack_top++], projection, sizeof(Matrix));
    } else {
        if (texture_stack_top >= 7) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(texture_stack[texture_stack_top++], texture_matrix, sizeof(Matrix));
    }
}

void glPopMatrix(void)
{
    ListCommand command = { LIST_CMD_POP_MATRIX };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (matrix_mode == GL_MODELVIEW) {
        if (modelview_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(modelview, modelview_stack[--modelview_stack_top], sizeof(Matrix));
        invalidate_modelview_inverse_cache();
    } else if (matrix_mode == GL_PROJECTION) {
        if (projection_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(projection, projection_stack[--projection_stack_top], sizeof(Matrix));
    } else {
        if (texture_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(texture_matrix, texture_stack[--texture_stack_top], sizeof(Matrix));
    }
}

static void load_current_matrix(const GLfloat *m)
{
    memcpy(*current_matrix(), m, sizeof(Matrix));
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

static void mult_current_matrix(const GLfloat *m)
{
    Matrix *dst = current_matrix();
    matrix_multiply(*dst, *dst, m);
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

static void premult_current_matrix(const GLfloat *m)
{
    Matrix *dst = current_matrix();
    matrix_multiply(*dst, m, *dst);
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

void glLoadMatrixf(const GLfloat *m)
{
    ListCommand command = { LIST_CMD_LOAD_MATRIX };

    if (reject_inside_begin()) {
        return;
    }
    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.matrix, m, sizeof(Matrix));
    record_command(command);
    if (compile_only()) {
        return;
    }

    load_current_matrix(m);
}

void glMultMatrixf(const GLfloat *m)
{
    ListCommand command = { LIST_CMD_MULT_MATRIX };

    if (reject_inside_begin()) {
        return;
    }
    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.matrix, m, sizeof(Matrix));
    record_command(command);
    if (compile_only()) {
        return;
    }

    mult_current_matrix(m);
}

void glLoadMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glLoadMatrixf(converted);
}

void glMultMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glMultMatrixf(converted);
}

void glLoadTransposeMatrixf(const GLfloat *m)
{
    Matrix transposed;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    matrix_transpose(transposed, m);
    glLoadMatrixf(transposed);
}

void glMultTransposeMatrixf(const GLfloat *m)
{
    Matrix transposed;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    matrix_transpose(transposed, m);
    glMultMatrixf(transposed);
}

void glLoadTransposeMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glLoadTransposeMatrixf(converted);
}

void glMultTransposeMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glMultTransposeMatrixf(converted);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far)
{
    Matrix m;

    if (right == left || top == bottom || z_far == z_near) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    matrix_identity(m);
    m[M11] = (GLfloat)(2.0 / (right - left));
    m[M22] = (GLfloat)(2.0 / (top - bottom));
    m[M33] = (GLfloat)(-2.0 / (z_far - z_near));
    m[M41] = (GLfloat)(-(right + left) / (right - left));
    m[M42] = (GLfloat)(-(top + bottom) / (top - bottom));
    m[M43] = (GLfloat)(-(z_far + z_near) / (z_far - z_near));
    glMultMatrixf(m);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far)
{
    Matrix m;

    if (right == left || top == bottom || z_far == z_near || z_near <= 0.0 || z_far <= 0.0) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    memset(m, 0, sizeof(m));
    m[M11] = (GLfloat)((2.0 * z_near) / (right - left));
    m[M22] = (GLfloat)((2.0 * z_near) / (top - bottom));
    m[M31] = (GLfloat)((right + left) / (right - left));
    m[M32] = (GLfloat)((top + bottom) / (top - bottom));
    m[M33] = (GLfloat)(-(z_far + z_near) / (z_far - z_near));
    m[M34] = -1.0f;
    m[M43] = (GLfloat)(-(2.0 * z_far * z_near) / (z_far - z_near));
    glMultMatrixf(m);
}

void glTranslatef(float x, float y, float z)
{
    ListCommand command = { LIST_CMD_TRANSLATE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix t;
    matrix_identity(t);
    t[M41] = x;
    t[M42] = y;
    t[M43] = z;
    mult_current_matrix(t);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    ListCommand command = { LIST_CMD_SCALE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix s;
    matrix_identity(s);
    s[M11] = x;
    s[M22] = y;
    s[M33] = z;
    premult_current_matrix(s);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRotatef(float angle_degrees, float x, float y, float z)
{
    ListCommand command = { LIST_CMD_ROTATE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = angle_degrees;
    command.f[1] = x;
    command.f[2] = y;
    command.f[3] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix r;
    float radians = angle_degrees * 0.017453292519943295f;
    matrix_identity(r);

    float axis_len = sqrtf(x * x + y * y + z * z);
    if (axis_len > 0.0f) {
        float inv_axis_len = 1.0f / axis_len;
        float s = sinf(radians);
        float c = cosf(radians);
        float one_minus_c = 1.0f - c;

        x *= inv_axis_len;
        y *= inv_axis_len;
        z *= inv_axis_len;

        r[M11] = x * x * one_minus_c + c;
        r[M12] = x * y * one_minus_c + z * s;
        r[M13] = x * z * one_minus_c - y * s;
        r[M21] = x * y * one_minus_c - z * s;
        r[M22] = y * y * one_minus_c + c;
        r[M23] = y * z * one_minus_c + x * s;
        r[M31] = x * z * one_minus_c + y * s;
        r[M32] = y * z * one_minus_c - x * s;
        r[M33] = z * z * one_minus_c + c;
    }

    premult_current_matrix(r);
}

void glRotated(GLdouble angle_degrees, GLdouble x, GLdouble y, GLdouble z)
{
    glRotatef((GLfloat)angle_degrees, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glColor3f(float r, float g, float b)
{
    glColor4f(r, g, b, 1.0f);
}

#define NXGL_DEFINE_COLOR3(suffix, type, convert) \
    void glColor3##suffix(type r, type g, type b) \
    { \
        glColor4f(convert(r), convert(g), convert(b), 1.0f); \
    } \
    void glColor3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glColor3##suffix(v[0], v[1], v[2]); \
    }

#define NXGL_DEFINE_COLOR4(suffix, type, convert) \
    void glColor4##suffix(type r, type g, type b, type a) \
    { \
        glColor4f(convert(r), convert(g), convert(b), convert(a)); \
    } \
    void glColor4##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glColor4##suffix(v[0], v[1], v[2], v[3]); \
    }

#define NXGL_FLOAT(value) ((GLfloat)(value))

NXGL_DEFINE_COLOR3(b, GLbyte, normalized_glbyte)
NXGL_DEFINE_COLOR3(d, GLdouble, NXGL_FLOAT)
NXGL_DEFINE_COLOR3(i, GLint, normalized_glint)
NXGL_DEFINE_COLOR3(s, GLshort, normalized_glshort)
NXGL_DEFINE_COLOR3(ub, GLubyte, normalized_glubyte)
NXGL_DEFINE_COLOR3(ui, GLuint, normalized_gluint)
NXGL_DEFINE_COLOR3(us, GLushort, normalized_glushort)
NXGL_DEFINE_COLOR4(b, GLbyte, normalized_glbyte)
NXGL_DEFINE_COLOR4(d, GLdouble, NXGL_FLOAT)
NXGL_DEFINE_COLOR4(i, GLint, normalized_glint)
NXGL_DEFINE_COLOR4(s, GLshort, normalized_glshort)
NXGL_DEFINE_COLOR4(ub, GLubyte, normalized_glubyte)
NXGL_DEFINE_COLOR4(ui, GLuint, normalized_gluint)
NXGL_DEFINE_COLOR4(us, GLushort, normalized_glushort)

#undef NXGL_FLOAT
#undef NXGL_DEFINE_COLOR4
#undef NXGL_DEFINE_COLOR3

void glColor3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glColor3f(v[0], v[1], v[2]);
}

void glColor4f(float r, float g, float b, float a)
{
    if (is_recording()) {
        ListCommand command = { LIST_CMD_COLOR4 };
        command.f[0] = r;
        command.f[1] = g;
        command.f[2] = b;
        command.f[3] = a;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    current_color = (NxglBackendColor){ r, g, b, a };
    apply_color_material(current_color);
}

void glColor4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glColor4f(v[0], v[1], v[2], v[3]);
}

void glIndexf(GLfloat c)
{
    ListCommand command = { LIST_CMD_INDEX };
    command.f[0] = c;
    record_command(command);
    if (compile_only()) {
        return;
    }

    current_index = c;
}

void glIndexfv(const GLfloat *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexf(c[0]);
}

void glIndexd(GLdouble c)
{
    glIndexf((GLfloat)c);
}

void glIndexdv(const GLdouble *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexd(c[0]);
}

void glIndexi(GLint c)
{
    glIndexf((GLfloat)c);
}

void glIndexiv(const GLint *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexi(c[0]);
}

void glIndexs(GLshort c)
{
    glIndexf((GLfloat)c);
}

void glIndexsv(const GLshort *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexs(c[0]);
}

void glIndexub(GLubyte c)
{
    glIndexf((GLfloat)c);
}

void glIndexubv(const GLubyte *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexub(c[0]);
}

void glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    if (is_recording()) {
        ListCommand command = { LIST_CMD_NORMAL3 };
        command.f[0] = x;
        command.f[1] = y;
        command.f[2] = z;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }
    current_normal = (NxglBackendVec3){ x, y, z };
}

static void glNormal3f_normalized(GLfloat x, GLfloat y, GLfloat z)
{
    NxglBackendVec3 normal = normalize_vec3((NxglBackendVec3){ x, y, z });
    glNormal3f(normal.x, normal.y, normal.z);
}

void glNormal3d(GLdouble x, GLdouble y, GLdouble z)
{
    glNormal3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glNormal3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glNormal3d(v[0], v[1], v[2]);
}

#define NXGL_DEFINE_NORMAL3_INT(suffix, type) \
    void glNormal3##suffix(type x, type y, type z) \
    { \
        glNormal3f_normalized((GLfloat)x, (GLfloat)y, (GLfloat)z); \
    } \
    void glNormal3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glNormal3##suffix(v[0], v[1], v[2]); \
    }

NXGL_DEFINE_NORMAL3_INT(b, GLbyte)
NXGL_DEFINE_NORMAL3_INT(i, GLint)
NXGL_DEFINE_NORMAL3_INT(s, GLshort)

#undef NXGL_DEFINE_NORMAL3_INT

void glNormal3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glNormal3f(v[0], v[1], v[2]);
}

void glTexCoord1f(GLfloat s)
{
    glTexCoord3f(s, 0.0f, 0.0f);
}

void glTexCoord1fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord1f(v[0]);
}

void glTexCoord2f(float s, float t)
{
    int unit = active_texture_index();
    if (is_recording()) {
        ListCommand command = { LIST_CMD_TEXCOORD2 };
        command.f[0] = s;
        command.f[1] = t;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    if (unit >= 0) {
        current_u[unit] = s;
        current_v[unit] = t;
        current_r[unit] = 0.0f;
    }
}

void glTexCoord2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord2f(v[0], v[1]);
}

void glTexCoord3f(float s, float t, float r)
{
    int unit = active_texture_index();
    if (is_recording()) {
        ListCommand command = { LIST_CMD_TEXCOORD3 };
        command.f[0] = s;
        command.f[1] = t;
        command.f[2] = r;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    if (unit >= 0) {
        current_u[unit] = s;
        current_v[unit] = t;
        current_r[unit] = r;
    }
}

void glTexCoord3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord3f(v[0], v[1], v[2]);
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    if (q != 0.0f) {
        glTexCoord3f(s / q, t / q, r / q);
    } else {
        glTexCoord3f(s, t, r);
    }
}

void glTexCoord4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord4f(v[0], v[1], v[2], v[3]);
}

#define NXGL_DEFINE_TEXCOORDS(suffix, type) \
    void glTexCoord1##suffix(type s) \
    { \
        glTexCoord1f((GLfloat)s); \
    } \
    void glTexCoord1##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord1##suffix(v[0]); \
    } \
    void glTexCoord2##suffix(type s, type t) \
    { \
        glTexCoord2f((GLfloat)s, (GLfloat)t); \
    } \
    void glTexCoord2##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord2##suffix(v[0], v[1]); \
    } \
    void glTexCoord3##suffix(type s, type t, type r) \
    { \
        glTexCoord3f((GLfloat)s, (GLfloat)t, (GLfloat)r); \
    } \
    void glTexCoord3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord3##suffix(v[0], v[1], v[2]); \
    } \
    void glTexCoord4##suffix(type s, type t, type r, type q) \
    { \
        glTexCoord4f((GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q); \
    } \
    void glTexCoord4##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord4##suffix(v[0], v[1], v[2], v[3]); \
    }

NXGL_DEFINE_TEXCOORDS(d, GLdouble)
NXGL_DEFINE_TEXCOORDS(i, GLint)
NXGL_DEFINE_TEXCOORDS(s, GLshort)

#undef NXGL_DEFINE_TEXCOORDS

void glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t)
{
    int unit = texture_unit_index(texture);

    if (unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    current_u[unit] = s;
    current_v[unit] = t;
    current_r[unit] = 0.0f;
}

void glMultiTexCoord3f(GLenum texture, GLfloat s, GLfloat t, GLfloat r)
{
    int unit = texture_unit_index(texture);

    if (unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    current_u[unit] = s;
    current_v[unit] = t;
    current_r[unit] = r;
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;
    ListCommand command = { LIST_CMD_TEX_GEN };

    if (coord_index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_GEN_MODE && !valid_texgen_mode(coord, (GLenum)((GLint)params[0]))) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pname != GL_TEXTURE_GEN_MODE && pname != GL_OBJECT_PLANE && pname != GL_EYE_PLANE) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = coord;
    command.b = pname;
    if (pname == GL_TEXTURE_GEN_MODE) {
        command.u = (GLuint)((GLint)params[0]);
    } else {
        command.f[0] = params[0];
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (unit < 0) {
        return;
    }

    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        state->mode = (GLenum)((GLint)params[0]);
    } else if (pname == GL_OBJECT_PLANE) {
        memcpy(state->object_plane, params, sizeof(state->object_plane));
    } else {
        memcpy(state->eye_plane, params, sizeof(state->eye_plane));
    }
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    GLfloat converted[4];
    if (params == NULL) {
        glTexGenfv(coord, pname, NULL);
        return;
    }
    converted[0] = (GLfloat)params[0];
    if (pname != GL_TEXTURE_GEN_MODE) {
        converted[1] = (GLfloat)params[1];
        converted[2] = (GLfloat)params[2];
        converted[3] = (GLfloat)params[3];
    }
    glTexGenfv(coord, pname, converted);
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    GLfloat params[4] = { param, 0.0f, 0.0f, 0.0f };
    glTexGenfv(coord, pname, params);
}

void glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    GLfloat params[4] = { (GLfloat)param, 0.0f, 0.0f, 0.0f };
    glTexGenfv(coord, pname, params);
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (coord_index < 0 || unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        params[0] = (GLfloat)state->mode;
    } else if (pname == GL_OBJECT_PLANE) {
        memcpy(params, state->object_plane, sizeof(state->object_plane));
    } else if (pname == GL_EYE_PLANE) {
        memcpy(params, state->eye_plane, sizeof(state->eye_plane));
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (coord_index < 0 || unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        params[0] = (GLint)state->mode;
    } else if (pname == GL_OBJECT_PLANE) {
        for (int i = 0; i < 4; ++i) params[i] = (GLint)state->object_plane[i];
    } else if (pname == GL_EYE_PLANE) {
        for (int i = 0; i < 4; ++i) params[i] = (GLint)state->eye_plane[i];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glBegin(uint32_t mode)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
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
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_BEGIN };
        command.a = mode;
        record_command(command);
    }
    begin_mode = mode;
    pending_count = 0;
    if (compile_only()) {
        return;
    }
    ++perf_counters.immediate_begins;
}

void glVertex2d(GLdouble x, GLdouble y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2d(v[0], v[1]);
}

void glVertex2f(GLfloat x, GLfloat y)
{
    glVertex3f(x, y, 0.0f);
}

void glVertex2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2f(v[0], v[1]);
}

void glVertex2i(GLint x, GLint y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2i(v[0], v[1]);
}

void glVertex2s(GLshort x, GLshort y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2s(v[0], v[1]);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3d(v[0], v[1], v[2]);
}

void glVertex3f(float x, float y, float z)
{
    NxglBackendVertex vertex;
    GLfloat obj[4] = { x, y, z, 1.0f };
    GLfloat u0 = current_u[0];
    GLfloat v0 = current_v[0];
    GLfloat r0 = current_r[0];
    GLfloat u1 = current_u[1];
    GLfloat v1 = current_v[1];
    GLfloat r1 = current_r[1];
    GLfloat u2 = current_u[2];
    GLfloat v2 = current_v[2];
    GLfloat r2 = current_r[2];
    GLfloat u3 = current_u[3];
    GLfloat v3 = current_v[3];
    GLfloat r3 = current_r[3];

    if (begin_mode == NXGL_NO_BEGIN_MODE || pending_count >= (int)(sizeof(pending) / sizeof(pending[0]))) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_VERTEX3 };
        command.f[0] = x;
        command.f[1] = y;
        command.f[2] = z;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    memset(&vertex, 0, sizeof(vertex));
    init_vertex_position(&vertex, x, y, z, 1.0f);
    vertex.base_color = current_color;
    if (!lighting_enabled && !fog_enabled && !texgen_enabled()) {
        vertex.normal = current_normal;
        vertex.color = current_color;
    } else {
        NxglBackendVec3 normal = transform_normal_to_eye(current_normal);
        vertex.normal = normal;
        apply_texgen_to_coords(obj, vertex.eye, normal, &u0, &v0, &r0, 0);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u1, &v1, &r1, 1);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u2, &v2, &r2, 2);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u3, &v3, &r3, 3);
        vertex.color = lit_color(current_color, normal, vertex.eye);
        vertex.color = apply_fog(vertex.color, vertex.eye);
    }
    vertex.u = u0;
    vertex.v = v0;
    vertex.r = r0;
    vertex.u1 = u1;
    vertex.v1 = v1;
    vertex.r1 = r1;
    vertex.u2 = u2;
    vertex.v2 = v2;
    vertex.r2 = r2;
    vertex.u3 = u3;
    vertex.v3 = v3;
    vertex.r3 = r3;
    pending[pending_count++] = vertex;
}

void glVertex3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3f(v[0], v[1], v[2]);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3i(v[0], v[1], v[2]);
}

void glVertex3s(GLshort x, GLshort y, GLshort z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3s(v[0], v[1], v[2]);
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (w != 0.0f) {
        glVertex3f(x / w, y / w, z / w);
    } else {
        glVertex3f(x, y, z);
    }
}

void glVertex4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4f(v[0], v[1], v[2], v[3]);
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4d(v[0], v[1], v[2], v[3]);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4i(v[0], v[1], v[2], v[3]);
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4s(v[0], v[1], v[2], v[3]);
}

void glEnd(void)
{
    uint32_t mode;

    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_END };
        record_command(command);
    }
    if (compile_only()) {
        pending_count = 0;
        begin_mode = NXGL_NO_BEGIN_MODE;
        return;
    }

    mode = begin_mode;
    if (native_fast_fill_enabled() && !lighting_enabled && shade_model == GL_SMOOTH && mode == GL_QUADS) {
        ensure_native_frame_started();
        for (int i = 0; i + 3 < pending_count; i += 4) {
            nxgl_backend_push_quad(pending[i], pending[i + 1], pending[i + 2], pending[i + 3]);
            perf_backend_quad();
        }
    } else {
        emit_vertices(mode, pending, pending_count);
    }
    pending_count = 0;
    begin_mode = NXGL_NO_BEGIN_MODE;
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, -y1);
    glVertex2f(x2, -y1);
    glVertex2f(x2, -y2);
    glVertex2f(x1, -y2);
    glEnd();
}

void glRectfv(const GLfloat *v1, const GLfloat *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRectf(v1[0], v1[1], v2[0], v2[1]);
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectdv(const GLdouble *v1, const GLdouble *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRectd(v1[0], v1[1], v2[0], v2[1]);
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectiv(const GLint *v1, const GLint *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRecti(v1[0], v1[1], v2[0], v2[1]);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectsv(const GLshort *v1, const GLshort *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRects(v1[0], v1[1], v2[0], v2[1]);
}

void glPointSize(GLfloat size)
{
    ListCommand command = { LIST_CMD_POINT_SIZE };

    if (reject_inside_begin()) {
        return;
    }
    if (size <= 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = size;
    record_command(command);
    if (compile_only()) {
        return;
    }
    point_size = size;
}

void glLineWidth(GLfloat width)
{
    ListCommand command = { LIST_CMD_LINE_WIDTH };

    if (reject_inside_begin()) {
        return;
    }
    if (width <= 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = width;
    record_command(command);
    if (compile_only()) {
        return;
    }
    line_width = width;
}

void glLineStipple(GLint factor, GLushort pattern)
{
    ListCommand command = { LIST_CMD_LINE_STIPPLE };

    if (reject_inside_begin()) {
        return;
    }
    if (factor < 1) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (factor > 256) {
        factor = 256;
    }
    command.u = (GLuint)pattern;
    command.f[0] = (GLfloat)factor;
    record_command(command);
    if (compile_only()) {
        return;
    }
    line_stipple_factor = factor;
    line_stipple_pattern = pattern;
}

void glPolygonStipple(const GLubyte *mask)
{
    ListCommand command = { LIST_CMD_POLYGON_STIPPLE };

    if (reject_inside_begin()) {
        return;
    }
    if (mask == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.bytes, mask, sizeof(command.bytes));
    record_command(command);
    if (compile_only()) {
        return;
    }
    memcpy(polygon_stipple_pattern, mask, sizeof(polygon_stipple_pattern));
}

void glGetPolygonStipple(GLubyte *mask)
{
    if (reject_inside_begin()) {
        return;
    }
    if (mask == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(mask, polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
}

void glPolygonMode(GLenum face, GLenum mode)
{
    ListCommand command = { LIST_CMD_POLYGON_MODE };

    if (reject_inside_begin()) {
        return;
    }
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = face;
    command.b = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    (void)face;
    polygon_mode = mode;
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    ListCommand command = { LIST_CMD_POLYGON_OFFSET };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = factor;
    command.f[1] = units;
    record_command(command);
    if (compile_only()) {
        return;
    }
    polygon_offset_factor = factor;
    polygon_offset_units = units;
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    ListCommand command = { LIST_CMD_ALPHA_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    command.f[0] = ref;
    record_command(command);
    if (compile_only()) {
        return;
    }
    alpha_test_func = func;
    alpha_test_ref = clamp01(ref);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    ListCommand command = { LIST_CMD_COLOR_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = red ? 1.0f : 0.0f;
    command.f[1] = green ? 1.0f : 0.0f;
    command.f[2] = blue ? 1.0f : 0.0f;
    command.f[3] = alpha ? 1.0f : 0.0f;
    record_command(command);
    if (compile_only()) {
        return;
    }
    color_write_mask[0] = red ? GL_TRUE : GL_FALSE;
    color_write_mask[1] = green ? GL_TRUE : GL_FALSE;
    color_write_mask[2] = blue ? GL_TRUE : GL_FALSE;
    color_write_mask[3] = alpha ? GL_TRUE : GL_FALSE;
}

void glLogicOp(GLenum opcode)
{
    ListCommand command = { LIST_CMD_LOGIC_OP };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_logic_op(opcode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = opcode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    logic_op_mode = opcode;
}

void glShadeModel(GLenum mode)
{
    ListCommand command = { LIST_CMD_SHADE_MODEL };

    if (mode != GL_FLAT && mode != GL_SMOOTH) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    shade_model = mode;
}

void glCullFace(GLenum mode)
{
    ListCommand command = { LIST_CMD_CULL_FACE };

    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    cull_face_mode = mode;
    sync_native_state();
}

void glFrontFace(GLenum mode)
{
    ListCommand command = { LIST_CMD_FRONT_FACE };

    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_CW && mode != GL_CCW) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    front_face_mode = mode;
    sync_native_state();
}

void glHint(GLenum target, GLenum mode)
{
    ListCommand command = { LIST_CMD_HINT };

    if (!valid_hint_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_hint_mode(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = target;
    command.b = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (target == GL_PERSPECTIVE_CORRECTION_HINT) {
        perspective_correction_hint = mode;
    } else if (target == GL_POINT_SMOOTH_HINT) {
        point_smooth_hint = mode;
    } else if (target == GL_LINE_SMOOTH_HINT) {
        line_smooth_hint = mode;
    } else if (target == GL_POLYGON_SMOOTH_HINT) {
        polygon_smooth_hint = mode;
    } else {
        fog_hint = mode;
    }
}

void glEdgeFlag(GLboolean flag)
{
    ListCommand command = { LIST_CMD_EDGE_FLAG };
    command.u = flag ? GL_TRUE : GL_FALSE;
    record_command(command);
    if (compile_only()) {
        return;
    }
    current_edge_flag = flag ? GL_TRUE : GL_FALSE;
}

void glEdgeFlagv(const GLboolean *flag)
{
    if (flag == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glEdgeFlag(*flag);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_LIGHT_MODEL };

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        command.a = pname;
        memcpy(command.f, params, sizeof(command.f));
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER) {
        glLightModeli(pname, params[0] != 0.0f ? 1 : 0);
    } else if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        glLightModeli(pname, params[0] != 0.0f ? 1 : 0);
    } else if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
        glLightModeli(pname, (GLint)params[0]);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glLightModelf(GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glLightModelfv(pname, values);
}

void glLightModeliv(GLenum pname, const GLint *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        GLfloat values[4] = {
            (GLfloat)params[0],
            (GLfloat)params[1],
            (GLfloat)params[2],
            (GLfloat)params[3]
        };
        glLightModelfv(pname, values);
    } else {
        glLightModeli(pname, params[0]);
    }
}

void glLightModeli(GLenum pname, GLint param)
{
    ListCommand command = { LIST_CMD_LIGHT_MODEL };

    if (reject_inside_begin()) {
        return;
    }
    if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
        if (param != GL_SINGLE_COLOR && param != GL_SEPARATE_SPECULAR_COLOR) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_color_control = (GLenum)param;
    } else if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER) {
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_local_viewer = param != 0;
    } else if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_two_side = param != 0;
    } else if (pname == GL_LIGHT_MODEL_AMBIENT) {
        GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
        glLightModelfv(pname, values);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glLightfv(light, pname, values);
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glLightfv(light, pname, values);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_LIGHT };
    int index = light_index(light);

    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if (pname == GL_SPOT_EXPONENT) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname == GL_SPOT_CUTOFF) {
        if ((params[0] < 0.0f || params[0] > 90.0f) && params[0] != 180.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname == GL_CONSTANT_ATTENUATION ||
               pname == GL_LINEAR_ATTENUATION ||
               pname == GL_QUADRATIC_ATTENUATION) {
        if (params[0] < 0.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname != GL_AMBIENT &&
               pname != GL_DIFFUSE &&
               pname != GL_SPECULAR &&
               pname != GL_POSITION &&
               pname != GL_SPOT_DIRECTION) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = light;
    command.b = pname;
    command.f[0] = params[0];
    if (pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION) {
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        command.f[1] = params[1];
        command.f[2] = params[2];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_AMBIENT) {
        lights[index].ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_DIFFUSE) {
        lights[index].diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_SPECULAR) {
        lights[index].specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_POSITION) {
        lights[index].position[0] = params[0];
        lights[index].position[1] = params[1];
        lights[index].position[2] = params[2];
        lights[index].position[3] = params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        lights[index].spot_direction[0] = params[0];
        lights[index].spot_direction[1] = params[1];
        lights[index].spot_direction[2] = params[2];
    } else if (pname == GL_SPOT_EXPONENT) {
        lights[index].spot_exponent = params[0];
    } else if (pname == GL_SPOT_CUTOFF) {
        lights[index].spot_cutoff = params[0];
    } else if (pname == GL_CONSTANT_ATTENUATION) {
        lights[index].constant_attenuation = params[0];
    } else if (pname == GL_LINEAR_ATTENUATION) {
        lights[index].linear_attenuation = params[0];
    } else if (pname == GL_QUADRATIC_ATTENUATION) {
        lights[index].quadratic_attenuation = params[0];
    }
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = 0.0f;
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glLightfv(light, pname, values);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    int index = light_index(light);

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        return;
    }

    if (pname == GL_AMBIENT) {
        params[0] = lights[index].ambient.r;
        params[1] = lights[index].ambient.g;
        params[2] = lights[index].ambient.b;
        params[3] = lights[index].ambient.a;
    } else if (pname == GL_DIFFUSE) {
        params[0] = lights[index].diffuse.r;
        params[1] = lights[index].diffuse.g;
        params[2] = lights[index].diffuse.b;
        params[3] = lights[index].diffuse.a;
    } else if (pname == GL_SPECULAR) {
        params[0] = lights[index].specular.r;
        params[1] = lights[index].specular.g;
        params[2] = lights[index].specular.b;
        params[3] = lights[index].specular.a;
    } else if (pname == GL_POSITION) {
        memcpy(params, lights[index].position, sizeof(lights[index].position));
    } else if (pname == GL_SPOT_DIRECTION) {
        params[0] = lights[index].spot_direction[0];
        params[1] = lights[index].spot_direction[1];
        params[2] = lights[index].spot_direction[2];
    } else if (pname == GL_SPOT_EXPONENT) {
        params[0] = lights[index].spot_exponent;
    } else if (pname == GL_SPOT_CUTOFF) {
        params[0] = lights[index].spot_cutoff;
    } else if (pname == GL_CONSTANT_ATTENUATION) {
        params[0] = lights[index].constant_attenuation;
    } else if (pname == GL_LINEAR_ATTENUATION) {
        params[0] = lights[index].linear_attenuation;
    } else if (pname == GL_QUADRATIC_ATTENUATION) {
        params[0] = lights[index].quadratic_attenuation;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
    GLfloat values[4];
    GLenum before;
    int count = pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION ? 4 :
                pname == GL_SPOT_DIRECTION ? 3 : 1;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetLightfv(light, pname, values);
    if (last_error == GL_NO_ERROR) {
        for (int i = 0; i < count; ++i) {
            params[i] = (GLint)values[i];
        }
        last_error = before;
    } else {
        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glMaterialfv(face, pname, values);
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glMaterialfv(face, pname, values);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT) {
        material_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_DIFFUSE) {
        material_state.diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT_AND_DIFFUSE) {
        material_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
        material_state.diffuse = material_state.ambient;
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_SPECULAR) {
        material_state.specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_EMISSION) {
        material_state.emission = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_SHININESS) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        material_state.shininess = params[0];
    }

    if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT) {
        material_back_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_DIFFUSE) {
        material_back_state.diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT_AND_DIFFUSE) {
        material_back_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
        material_back_state.diffuse = material_back_state.ambient;
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_SPECULAR) {
        material_back_state.specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_EMISSION) {
        material_back_state.emission = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_SHININESS) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        material_back_state.shininess = params[0];
    }

    if (pname != GL_AMBIENT &&
        pname != GL_DIFFUSE &&
        pname != GL_AMBIENT_AND_DIFFUSE &&
        pname != GL_SPECULAR &&
        pname != GL_EMISSION &&
        pname != GL_SHININESS) {
        set_error(GL_INVALID_ENUM);
    }
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_AMBIENT ||
        pname == GL_DIFFUSE ||
        pname == GL_AMBIENT_AND_DIFFUSE ||
        pname == GL_SPECULAR ||
        pname == GL_EMISSION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glMaterialfv(face, pname, values);
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    const MaterialState *material = &material_state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        return;
    }
    if (face == GL_BACK) {
        material = &material_back_state;
    }

    if (pname == GL_AMBIENT) {
        params[0] = material->ambient.r;
        params[1] = material->ambient.g;
        params[2] = material->ambient.b;
        params[3] = material->ambient.a;
    } else if (pname == GL_DIFFUSE) {
        params[0] = material->diffuse.r;
        params[1] = material->diffuse.g;
        params[2] = material->diffuse.b;
        params[3] = material->diffuse.a;
    } else if (pname == GL_SPECULAR) {
        params[0] = material->specular.r;
        params[1] = material->specular.g;
        params[2] = material->specular.b;
        params[3] = material->specular.a;
    } else if (pname == GL_EMISSION) {
        params[0] = material->emission.r;
        params[1] = material->emission.g;
        params[2] = material->emission.b;
        params[3] = material->emission.a;
    } else if (pname == GL_SHININESS) {
        params[0] = material->shininess;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
    GLfloat values[4];
    GLenum before;
    int count = pname == GL_SHININESS ? 1 : 4;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetMaterialfv(face, pname, values);
    if (last_error == GL_NO_ERROR) {
        for (int i = 0; i < count; ++i) {
            params[i] = (GLint)values[i];
        }
        last_error = before;
    } else {
        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glColorMaterial(GLenum face, GLenum mode)
{
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mode != GL_AMBIENT &&
        mode != GL_DIFFUSE &&
        mode != GL_AMBIENT_AND_DIFFUSE &&
        mode != GL_SPECULAR &&
        mode != GL_EMISSION) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    color_material_face = face;
    color_material_parameter = mode;
    apply_color_material(current_color);
}

void glFogf(GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glFogfv(pname, values);
}

void glFogfv(GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_FOG };

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname != GL_FOG_MODE &&
        pname != GL_FOG_COLOR &&
        pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START &&
        pname != GL_FOG_END) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pname == GL_FOG_MODE) {
        GLenum mode = (GLenum)(GLint)params[0];
        if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
            set_error(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_FOG_DENSITY && params[0] < 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    command.a = pname;
    command.f[0] = params[0];
    if (pname == GL_FOG_COLOR) {
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else {
        command.f[1] = params[0];
        command.f[2] = params[0];
        command.f[3] = params[0];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_FOG_MODE) {
        fog_mode = (GLenum)(GLint)params[0];
    } else if (pname == GL_FOG_COLOR) {
        fog_color = clamp_color((NxglBackendColor){ params[0], params[1], params[2], params[3] });
    } else if (pname == GL_FOG_DENSITY) {
        fog_density = params[0];
    } else if (pname == GL_FOG_START) {
        fog_start = params[0];
    } else if (pname == GL_FOG_END) {
        fog_end = params[0];
    }
}

void glFogi(GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glFogfv(pname, values);
}

void glFogiv(GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_FOG_COLOR) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glFogfv(pname, values);
}

void glClipPlane(GLenum plane, const GLdouble *equation)
{
    int index = clip_plane_index(plane);
    ListCommand command = { LIST_CMD_CLIP_PLANE };
    GLdouble transformed[4];

    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (equation == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.a = plane;
    for (int i = 0; i < 4; ++i) {
        command.f[i] = (GLfloat)equation[i];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    transform_clip_plane(transformed, equation);
    memcpy(clip_planes[index].equation, transformed, sizeof(transformed));
}

void glGetClipPlane(GLenum plane, GLdouble *equation)
{
    int index = clip_plane_index(plane);

    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (equation == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    memcpy(equation, clip_planes[index].equation, sizeof(clip_planes[index].equation));
}


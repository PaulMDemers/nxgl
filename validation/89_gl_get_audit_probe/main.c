#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[12];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.002f;
}

static bool neard(GLdouble a, GLdouble b)
{
    GLdouble d = a - b;
    if (d < 0.0) d = -d;
    return d <= 0.002;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    static GLfloat vertices[9] = {
        -0.4f, -0.4f, 0.0f,
         0.4f, -0.4f, 0.0f,
         0.0f,  0.4f, 0.0f
    };
    static GLfloat colors[12] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    };
    static GLfloat normals[9] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    static GLfloat texcoords[6] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f
    };
    GLint iv[16];
    GLfloat fv[16];
    GLdouble dv[16];
    GLboolean bv[16];
    GLvoid *ptr = NULL;
    GLuint tex = 0;
    GLuint select[8];
    GLfloat feedback[8];
    GLuint list;
    const GLubyte *str;
    bool ok;

    reset_state();
    str = glGetString(GL_VENDOR);
    ok = str != NULL && str[0] != 0 && consume_error(GL_NO_ERROR);
    str = glGetString(GL_RENDERER);
    ok = ok && str != NULL && str[0] != 0 && consume_error(GL_NO_ERROR);
    str = glGetString(GL_VERSION);
    ok = ok && str != NULL && str[0] == '1' && consume_error(GL_NO_ERROR);
    str = glGetString(0xffffffffu);
    ok = ok && str == NULL && consume_error(GL_INVALID_ENUM);
    expect_bool("string queries", ok, 0);

    reset_state();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(0.25f, 0.50f, 0.0f);
    glGetIntegerv(GL_MATRIX_MODE, &iv[0]);
    glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &iv[1]);
    glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &iv[2]);
    glGetFloatv(GL_PROJECTION_MATRIX, fv);
    glGetDoublev(GL_TEXTURE_MATRIX, dv);
    ok = iv[0] == (GLint)GL_TEXTURE &&
         iv[1] == 2 && iv[2] == 2 &&
         nearf(fv[0], 0.5f) && nearf(fv[5], 0.6666667f) &&
         neard(dv[12], 0.25) && neard(dv[13], 0.50) &&
         consume_error(GL_NO_ERROR);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    expect_bool("matrix get audit", ok, 1);

    reset_state();
    glColor4f(0.20f, 0.40f, 0.60f, 0.80f);
    glNormal3f(0.0f, 0.50f, 1.0f);
    glTexCoord3f(0.25f, 0.50f, 0.75f);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetDoublev(GL_CURRENT_COLOR, dv);
    glGetFloatv(GL_CURRENT_NORMAL, fv);
    glGetFloatv(GL_CURRENT_TEXTURE_COORDS, &fv[4]);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, bv);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, &fv[8]);
    ok = neard(dv[0], 0.20) && neard(dv[3], 0.80) &&
         nearf(fv[1], 0.50f) && nearf(fv[2], 1.0f) &&
         nearf(fv[4], 0.25f) && nearf(fv[6], 0.75f) &&
         bv[0] == GL_TRUE && nearf(fv[8], 320.0f) && nearf(fv[9], 240.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("current value queries", ok, 2);

    reset_state();
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glGetBooleanv(GL_LIGHTING, &bv[0]);
    glGetIntegerv(GL_FOG, &iv[0]);
    glGetIntegerv(GL_TEXTURE_2D, &iv[1]);
    glGetBooleanv(GL_TEXTURE_GEN_S, &bv[1]);
    glGetIntegerv(GL_CLIP_PLANE0, &iv[2]);
    glGetBooleanv(GL_POLYGON_OFFSET_FILL, &bv[2]);
    ok = bv[0] == GL_TRUE && iv[0] == GL_TRUE && iv[1] == GL_TRUE &&
         bv[1] == GL_TRUE && iv[2] == GL_TRUE && bv[2] == GL_TRUE &&
         glIsEnabled(GL_TEXTURE_2D) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("enable queries", ok, 3);

    reset_state();
    glPointSize(7.0f);
    glLineWidth(3.0f);
    glLineStipple(4, 0xaaaa);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonOffset(1.25f, 2.50f);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);
    glHint(GL_FOG_HINT, GL_NICEST);
    glEdgeFlag(GL_FALSE);
    glGetFloatv(GL_POINT_SIZE, &fv[0]);
    glGetDoublev(GL_LINE_WIDTH, &dv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &iv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &iv[1]);
    glGetIntegerv(GL_POLYGON_MODE, &iv[2]);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &fv[1]);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &fv[2]);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[4]);
    glGetIntegerv(GL_FRONT_FACE, &iv[5]);
    glGetIntegerv(GL_FOG_HINT, &iv[6]);
    glGetBooleanv(GL_EDGE_FLAG, &bv[0]);
    ok = nearf(fv[0], 7.0f) && neard(dv[0], 3.0) &&
         iv[0] == 4 && iv[1] == 0xaaaa && iv[2] == (GLint)GL_LINE && iv[3] == (GLint)GL_LINE &&
         nearf(fv[1], 1.25f) && nearf(fv[2], 2.50f) &&
         iv[4] == (GLint)GL_FRONT && iv[5] == (GLint)GL_CW && iv[6] == (GLint)GL_NICEST &&
         bv[0] == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("raster state queries", ok, 4);

    reset_state();
    glClearDepth(0.35f);
    glClearStencil(0x5a);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL, 0.45f);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glLogicOp(GL_XOR);
    glStencilFunc(GL_NOTEQUAL, 3, 0x7f);
    glStencilOp(GL_INCR, GL_DECR, GL_REPLACE);
    glScissor(11, 22, 123, 234);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fv[0]);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &iv[0]);
    glGetIntegerv(GL_DEPTH_FUNC, &iv[1]);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &bv[0]);
    glGetIntegerv(GL_BLEND_SRC, &iv[2]);
    glGetIntegerv(GL_BLEND_DST, &iv[3]);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &iv[4]);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv[1]);
    glGetBooleanv(GL_COLOR_WRITEMASK, &bv[1]);
    glGetIntegerv(GL_LOGIC_OP_MODE, &iv[5]);
    glGetIntegerv(GL_STENCIL_FUNC, &iv[6]);
    glGetIntegerv(GL_STENCIL_REF, &iv[7]);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &iv[8]);
    glGetIntegerv(GL_STENCIL_FAIL, &iv[9]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &iv[10]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &iv[11]);
    glGetDoublev(GL_SCISSOR_BOX, dv);
    ok = nearf(fv[0], 0.35f) && iv[0] == 0x5a && iv[1] == (GLint)GL_GEQUAL &&
         bv[0] == GL_FALSE && iv[2] == (GLint)GL_DST_COLOR && iv[3] == (GLint)GL_ONE_MINUS_SRC_ALPHA &&
         iv[4] == (GLint)GL_GEQUAL && nearf(fv[1], 0.45f) &&
         bv[1] == GL_TRUE && bv[2] == GL_FALSE && bv[3] == GL_TRUE && bv[4] == GL_FALSE &&
         iv[5] == (GLint)GL_XOR && iv[6] == (GLint)GL_NOTEQUAL && iv[7] == 3 && iv[8] == 0x7f &&
         iv[9] == (GLint)GL_INCR && iv[10] == (GLint)GL_DECR && iv[11] == (GLint)GL_REPLACE &&
         neard(dv[0], 11.0) && neard(dv[3], 234.0) && consume_error(GL_NO_ERROR);
    expect_bool("depth blend stencil queries", ok, 5);

    reset_state();
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_PACK_ROW_LENGTH, 17);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 3);
    glPixelZoom(1.5f, -2.0f);
    glPixelTransferf(GL_RED_SCALE, 1.25f);
    glPixelTransferf(GL_BLUE_BIAS, 0.125f);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 3, (const GLfloat[]){ 0.0f, 0.5f, 1.0f });
    glGetIntegerv(GL_PACK_ALIGNMENT, &iv[0]);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &iv[1]);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &iv[2]);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &iv[3]);
    glGetFloatv(GL_ZOOM_X, &fv[0]);
    glGetFloatv(GL_ZOOM_Y, &fv[1]);
    glGetFloatv(GL_RED_SCALE, &fv[2]);
    glGetFloatv(GL_BLUE_BIAS, &fv[3]);
    glGetIntegerv(GL_PIXEL_MAP_R_TO_R_SIZE, &iv[4]);
    ok = iv[0] == 8 && iv[1] == 17 && iv[2] == 2 && iv[3] == 3 &&
         nearf(fv[0], 1.5f) && nearf(fv[1], -2.0f) &&
         nearf(fv[2], 1.25f) && nearf(fv[3], 0.125f) &&
         iv[4] == 3 && consume_error(GL_NO_ERROR);
    expect_bool("pixel store transfer queries", ok, 6);

    reset_state();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.75f);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (GLfloat)GL_BLEND);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (const GLfloat[]){ 0.2f, 0.3f, 0.4f, 0.5f });
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &iv[0]);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &iv[1]);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &iv[2]);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &iv[3]);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &fv[0]);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &iv[4]);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &fv[1]);
    ok = iv[0] == (GLint)tex && iv[1] == (GLint)GL_NEAREST &&
         iv[2] == (GLint)GL_LINEAR && iv[3] == (GLint)GL_CLAMP &&
         nearf(fv[0], 0.75f) && iv[4] == (GLint)GL_BLEND &&
         nearf(fv[1], 0.2f) && nearf(fv[4], 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("texture env parameter queries", ok, 7);

    reset_state();
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (const GLfloat[]){ 0.1f, 0.2f, 0.3f, 1.0f });
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, (const GLfloat[]){ 0.4f, 0.5f, 0.6f, 1.0f });
    glMaterialfv(GL_FRONT, GL_SPECULAR, (const GLfloat[]){ 0.7f, 0.6f, 0.5f, 1.0f });
    glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);
    glFogf(GL_FOG_DENSITY, 0.125f);
    glFogfv(GL_FOG_COLOR, (const GLfloat[]){ 0.2f, 0.25f, 0.30f, 1.0f });
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, fv);
    glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &iv[0]);
    glGetLightfv(GL_LIGHT1, GL_DIFFUSE, &fv[4]);
    glGetMaterialiv(GL_FRONT, GL_SHININESS, &iv[1]);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, &fv[8]);
    glGetFloatv(GL_FOG_DENSITY, &fv[12]);
    glGetDoublev(GL_FOG_COLOR, dv);
    ok = nearf(fv[0], 0.1f) && nearf(fv[2], 0.3f) && iv[0] == GL_TRUE &&
         nearf(fv[4], 0.4f) && nearf(fv[6], 0.6f) &&
         iv[1] == 32 && nearf(fv[8], 0.7f) &&
         nearf(fv[12], 0.125f) && neard(dv[0], 0.2) && neard(dv[2], 0.3) &&
         consume_error(GL_NO_ERROR);
    expect_bool("lighting material fog queries", ok, 8);

    reset_state();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glNormalPointer(GL_FLOAT, 0, normals);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    glGetIntegerv(GL_VERTEX_ARRAY, &iv[0]);
    glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &iv[1]);
    glGetIntegerv(GL_COLOR_ARRAY_SIZE, &iv[2]);
    glGetIntegerv(GL_NORMAL_ARRAY_TYPE, &iv[3]);
    glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &iv[4]);
    glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
    ok = iv[0] == GL_TRUE && iv[1] == 3 && iv[2] == 4 &&
         iv[3] == (GLint)GL_FLOAT && iv[4] == 2 && ptr == vertices &&
         consume_error(GL_NO_ERROR);
    expect_bool("client array queries", ok, 9);

    reset_state();
    memset(select, 0, sizeof(select));
    memset(feedback, 0, sizeof(feedback));
    glListBase(123);
    glSelectBuffer(8, select);
    glFeedbackBuffer(8, GL_3D, feedback);
    glGetIntegerv(GL_LIST_BASE, &iv[0]);
    glGetIntegerv(GL_SELECTION_BUFFER_SIZE, &iv[1]);
    glGetIntegerv(GL_FEEDBACK_BUFFER_SIZE, &iv[2]);
    glRenderMode(GL_SELECT);
    glPushName(9);
    glGetIntegerv(GL_RENDER_MODE, &iv[3]);
    glGetIntegerv(GL_NAME_STACK_DEPTH, &iv[4]);
    ok = iv[0] == 123 && iv[1] == 8 && iv[2] == 8 &&
         iv[3] == (GLint)GL_SELECT && iv[4] == 1 && glRenderMode(GL_RENDER) == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("selection feedback list queries", ok, 10);

    reset_state();
    glGetIntegerv(GL_TEXTURE_ENV, iv);
    ok = consume_error(GL_INVALID_ENUM);
    glGetFloatv(GL_VIEWPORT, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetPointerv(GL_TEXTURE_ENV, &ptr);
    ok = ok && ptr == NULL && consume_error(GL_INVALID_ENUM);
    glGetTexEnviv(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, iv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetTexParameteriv(GL_TEXTURE_ENV, GL_TEXTURE_MIN_FILTER, iv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("get validation audit", ok, 11);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
    glRectf(x - 0.11f, -0.72f, x + 0.11f, -0.88f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 12; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL get audit probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(0.15f, 0.45f, 0.85f);
        glRectf(-1.70f, 0.45f, 1.70f, -0.45f);
        for (int i = 0; i < 12; ++i) {
            draw_bar(-1.54f + (float)i * 0.28f, results[i]);
        }
        nxglSwapBuffers("NXGL get audit", all_passed() ? "all checks passed" : "get audit check failed");
        Sleep(16);
    }
}

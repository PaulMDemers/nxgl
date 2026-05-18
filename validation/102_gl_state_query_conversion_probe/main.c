#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool nearf(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= 0.001f;
}

static bool neard(GLdouble actual, GLdouble expected)
{
    GLdouble d = actual - expected;
    if (d < 0.0) d = -d;
    return d <= 0.001;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_MAP1_VERTEX_3);
    glDisable(GL_MAP2_COLOR_4);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    (void)glGetError();
}

static void run_probe(void)
{
    static const GLfloat map1_points[12] = {
        -1.0f, 0.0f, 0.0f,
         0.0f, 0.5f, 0.0f,
         0.5f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f
    };
    static const GLfloat map2_points[16] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f
    };
    GLint iv[16];
    GLfloat fv[16];
    GLdouble dv[16];
    GLboolean bv[16];
    GLuint tex = 0;
    bool ok;

    reset_state();
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_GEQUAL);
    glViewport(11, 22, 333, 244);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glGetFloatv(GL_BLEND, &fv[0]);
    glGetFloatv(GL_DEPTH_TEST, &fv[1]);
    glGetFloatv(GL_BLEND_SRC, &fv[2]);
    glGetFloatv(GL_DEPTH_FUNC, &fv[3]);
    glGetFloatv(GL_VIEWPORT, &fv[4]);
    glGetFloatv(GL_COLOR_WRITEMASK, &fv[8]);
    ok = nearf(fv[0], 1.0f) && nearf(fv[1], 1.0f) &&
         nearf(fv[2], (GLfloat)GL_DST_COLOR) && nearf(fv[3], (GLfloat)GL_GEQUAL) &&
         nearf(fv[4], 11.0f) && nearf(fv[7], 244.0f) &&
         nearf(fv[8], 1.0f) && nearf(fv[9], 0.0f) && nearf(fv[10], 1.0f) && nearf(fv[11], 0.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("float fallback for integer state", ok, 0);

    reset_state();
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);
    glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 4, map1_points);
    glMap2f(GL_MAP2_COLOR_4, 0.0f, 1.0f, 4, 2, 0.0f, 1.0f, 8, 2, map2_points);
    glEnable(GL_MAP1_VERTEX_3);
    glEnable(GL_MAP2_COLOR_4);
    glGetIntegerv(GL_LIGHT0, &iv[0]);
    glGetFloatv(GL_AUTO_NORMAL, &fv[0]);
    glGetBooleanv(GL_MAP1_VERTEX_3, &bv[0]);
    glGetDoublev(GL_MAP2_COLOR_4, &dv[0]);
    ok = iv[0] == GL_TRUE && nearf(fv[0], 1.0f) &&
         bv[0] == GL_TRUE && neard(dv[0], 1.0) && consume_error(GL_NO_ERROR);
    expect_bool("cap query conversion", ok, 1);

    reset_state();
    glPointSize(7.0f);
    glLineWidth(4.0f);
    glPolygonOffset(2.0f, -3.0f);
    glPixelZoom(2.0f, -3.0f);
    glAlphaFunc(GL_GREATER, 1.0f);
    glPixelTransferf(GL_RED_SCALE, 2.0f);
    glPixelTransferf(GL_BLUE_BIAS, -1.0f);
    glGetIntegerv(GL_POINT_SIZE, &iv[0]);
    glGetIntegerv(GL_LINE_WIDTH, &iv[1]);
    glGetIntegerv(GL_POLYGON_OFFSET_FACTOR, &iv[2]);
    glGetIntegerv(GL_POLYGON_OFFSET_UNITS, &iv[3]);
    glGetIntegerv(GL_ZOOM_X, &iv[4]);
    glGetIntegerv(GL_ZOOM_Y, &iv[5]);
    glGetIntegerv(GL_ALPHA_TEST_REF, &iv[6]);
    glGetIntegerv(GL_RED_SCALE, &iv[7]);
    glGetIntegerv(GL_BLUE_BIAS, &iv[8]);
    ok = iv[0] == 7 && iv[1] == 4 && iv[2] == 2 && iv[3] == -3 &&
         iv[4] == 2 && iv[5] == -3 && iv[6] == 1 && iv[7] == 2 && iv[8] == -1 &&
         consume_error(GL_NO_ERROR);
    expect_bool("integer fallback for float state", ok, 2);

    reset_state();
    glColor4f(1.0f, 0.0f, 1.0f, 0.0f);
    glNormal3f(-1.0f, 0.0f, 1.0f);
    glTexCoord3f(2.0f, 3.0f, 4.0f);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClearAccum(1.0f, -1.0f, 0.0f, 1.0f);
    glGetIntegerv(GL_CURRENT_COLOR, iv);
    glGetIntegerv(GL_CURRENT_NORMAL, &iv[4]);
    glGetIntegerv(GL_CURRENT_TEXTURE_COORDS, &iv[7]);
    glGetIntegerv(GL_CLEAR_COLOR, &iv[11]);
    glGetBooleanv(GL_ACCUM_CLEAR_VALUE, bv);
    ok = iv[0] == 1 && iv[1] == 0 && iv[2] == 1 && iv[3] == 0 &&
         iv[4] == -1 && iv[6] == 1 &&
         iv[7] == 2 && iv[8] == 3 && iv[9] == 4 && iv[10] == 1 &&
         iv[11] == 1 && iv[12] == 0 && iv[13] == 1 && iv[14] == 0 &&
         bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_FALSE && bv[3] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("current value conversions", ok, 3);

    reset_state();
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetFloatv(GL_ACTIVE_TEXTURE, &fv[0]);
    glGetFloatv(GL_TEXTURE_BINDING_2D, &fv[1]);
    glGetDoublev(GL_ACTIVE_TEXTURE, &dv[0]);
    ok = nearf(fv[0], (GLfloat)GL_TEXTURE1) && nearf(fv[1], (GLfloat)tex) &&
         neard(dv[0], (GLdouble)GL_TEXTURE1) && consume_error(GL_NO_ERROR);
    glActiveTexture(GL_TEXTURE0);
    expect_bool("texture integer conversions", ok, 4);

    reset_state();
    glStencilMask(0x3c);
    glStencilFunc(GL_NOTEQUAL, 5, 0x7f);
    glStencilOp(GL_INCR, GL_DECR, GL_REPLACE);
    glGetFloatv(GL_STENCIL_WRITEMASK, &fv[0]);
    glGetDoublev(GL_STENCIL_REF, &dv[0]);
    glGetFloatv(GL_STENCIL_FAIL, &fv[1]);
    glGetFloatv(GL_STENCIL_PASS_DEPTH_FAIL, &fv[2]);
    glGetFloatv(GL_STENCIL_PASS_DEPTH_PASS, &fv[3]);
    ok = nearf(fv[0], 60.0f) && neard(dv[0], 5.0) &&
         nearf(fv[1], (GLfloat)GL_INCR) && nearf(fv[2], (GLfloat)GL_DECR) &&
         nearf(fv[3], (GLfloat)GL_REPLACE) && consume_error(GL_NO_ERROR);
    expect_bool("stencil conversion queries", ok, 5);

    reset_state();
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_LIGHT0);
    glEnable(GL_MAP1_VERTEX_3);
    glGetFloatv(GL_ATTRIB_STACK_DEPTH, &fv[0]);
    glGetFloatv(GL_LIGHT0, &fv[1]);
    glGetFloatv(GL_MAP1_VERTEX_3, &fv[2]);
    glPopAttrib();
    glGetFloatv(GL_ATTRIB_STACK_DEPTH, &fv[3]);
    glGetFloatv(GL_LIGHT0, &fv[4]);
    ok = nearf(fv[0], 1.0f) && nearf(fv[1], 1.0f) && nearf(fv[2], 1.0f) &&
         nearf(fv[3], 0.0f) && nearf(fv[4], 0.0f) && consume_error(GL_NO_ERROR);
    expect_bool("attrib depth conversion", ok, 6);

    reset_state();
    glScissor(3, 4, 50, 60);
    glEnable(GL_SCISSOR_TEST);
    glGetBooleanv(GL_SCISSOR_BOX, bv);
    glGetBooleanv(GL_SCISSOR_TEST, &bv[4]);
    glGetDoublev(GL_SCISSOR_BOX, dv);
    ok = bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_TRUE &&
         bv[4] == GL_TRUE && neard(dv[0], 3.0) && neard(dv[3], 60.0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("boolean double conversion", ok, 7);

    reset_state();
    glGetFloatv(0xffffffffu, fv);
    ok = consume_error(GL_INVALID_ENUM);
    glGetIntegerv(0xffffffffu, iv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetFloatv(GL_VIEWPORT, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("conversion validation", ok, 8);

    reset_state();
    glBegin(GL_POINTS);
    glGetFloatv(GL_BLEND, fv);
    glEnd();
    ok = consume_error(GL_INVALID_OPERATION);
    expect_bool("begin end query rejection", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.85f, pass ? 0.75f : 0.10f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.13f, -0.75f, 0.0f);
    glVertex3f(x + 0.13f, -0.75f, 0.0f);
    glVertex3f(x + 0.13f, -0.98f, 0.0f);
    glVertex3f(x - 0.13f, -0.98f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 10; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL state query conversion probe starting\n");

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
        glRectf(-1.45f, 0.42f, 1.45f, -0.42f);
        for (int i = 0; i < 10; ++i) {
            draw_bar(-1.35f + (float)i * 0.30f, results[i]);
        }
        nxglSwapBuffers("NXGL state query conversions", all_passed() ? "all checks passed" : "state conversion failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

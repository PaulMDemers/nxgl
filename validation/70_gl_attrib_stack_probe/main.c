#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[9];
static GLfloat vertex_data[8] = { 0.0f };
static GLfloat vertex_data_b[8] = { 1.0f };

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.03f;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_LINE_STIPPLE);
    glLineWidth(1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xffffffffu);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xffffffffu);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 0, NULL);
}

static void run_probe(void)
{
    GLint iv[8];
    GLfloat fv[8];
    GLvoid *ptr;
    GLuint tex = 0;
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, iv);
    ok = iv[0] >= 16;
    glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, iv);
    ok = ok && iv[0] >= 16;
    glPushAttrib(GL_CURRENT_BIT);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, iv);
    ok = ok && iv[0] == 1;
    glPopAttrib();
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, iv);
    ok = ok && iv[0] == 0 && consume_error(GL_NO_ERROR);
    expect_bool("attrib limits and depth", ok, 0);

    reset_state();
    glColor4f(0.20f, 0.30f, 0.40f, 0.50f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(0.90f, 0.10f, 0.10f, 1.0f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glPopAttrib();
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = nearf(fv[0], 0.20f) && nearf(fv[1], 0.30f) && nearf(fv[2], 0.40f) && nearf(fv[3], 0.50f);
    glGetFloatv(GL_CURRENT_NORMAL, fv);
    ok = ok && nearf(fv[0], 0.0f) && nearf(fv[1], 1.0f) && nearf(fv[2], 0.0f) && consume_error(GL_NO_ERROR);
    expect_bool("current attrib restore", ok, 1);

    reset_state();
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_GEN_S);
    glPopAttrib();
    ok = glIsEnabled(GL_BLEND) == GL_FALSE &&
         glIsEnabled(GL_DEPTH_TEST) == GL_TRUE &&
         glIsEnabled(GL_TEXTURE_GEN_S) == GL_FALSE &&
         consume_error(GL_NO_ERROR);
    expect_bool("enable attrib restore", ok, 2);

    reset_state();
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.42f);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    glPopAttrib();
    glGetIntegerv(GL_BLEND_SRC, iv);
    ok = iv[0] == (GLint)GL_SRC_ALPHA;
    glGetIntegerv(GL_BLEND_DST, iv);
    ok = ok && iv[0] == (GLint)GL_ONE_MINUS_SRC_ALPHA;
    glGetIntegerv(GL_COLOR_WRITEMASK, iv);
    ok = ok && iv[0] == GL_TRUE && iv[1] == GL_TRUE && iv[2] == GL_TRUE && iv[3] == GL_TRUE;
    ok = ok && glIsEnabled(GL_ALPHA_TEST) == GL_FALSE && glIsEnabled(GL_BLEND) == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("color buffer attrib restore", ok, 3);

    reset_state();
    glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_FALSE);
    glClearDepth(0.25f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 7, 0x33u);
    glStencilOp(GL_REPLACE, GL_INCR, GL_DECR);
    glStencilMask(0x55u);
    glPopAttrib();
    glGetIntegerv(GL_DEPTH_FUNC, iv);
    ok = iv[0] == (GLint)GL_LEQUAL;
    glGetIntegerv(GL_DEPTH_WRITEMASK, iv);
    ok = ok && iv[0] == GL_TRUE;
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, fv);
    ok = ok && nearf(fv[0], 1.0f);
    glGetIntegerv(GL_STENCIL_FUNC, iv);
    ok = ok && iv[0] == (GLint)GL_ALWAYS && glIsEnabled(GL_STENCIL_TEST) == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("depth stencil attrib restore", ok, 4);

    reset_state();
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glPushAttrib(GL_TEXTURE_BIT);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDisable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_GEN_S);
    glPopAttrib();
    glGetIntegerv(GL_ACTIVE_TEXTURE, iv);
    ok = iv[0] == (GLint)GL_TEXTURE1 && glIsEnabled(GL_TEXTURE_2D) == GL_TRUE;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, iv);
    ok = ok && iv[0] == 0;
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, iv);
    ok = ok && iv[0] == (GLint)GL_MODULATE && consume_error(GL_NO_ERROR);
    expect_bool("texture attrib restore", ok, 5);

    reset_state();
    glClientActiveTexture(GL_TEXTURE1);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 12, vertex_data);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 8);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 8, vertex_data_b);
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 2);
    glPopClientAttrib();
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, iv);
    ok = iv[0] == (GLint)GL_TEXTURE1 && glIsEnabled(GL_VERTEX_ARRAY) == GL_TRUE;
    glGetIntegerv(GL_VERTEX_ARRAY_SIZE, iv);
    ok = ok && iv[0] == 3;
    glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, iv);
    ok = ok && iv[0] == 12;
    glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
    ok = ok && ptr == vertex_data;
    glGetIntegerv(GL_PACK_ALIGNMENT, iv);
    ok = ok && iv[0] == 1;
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, iv);
    ok = ok && iv[0] == 8 && consume_error(GL_NO_ERROR);
    expect_bool("client attrib restore", ok, 6);

    reset_state();
    glLineWidth(2.0f);
    glColor4f(0.1f, 0.2f, 0.3f, 1.0f);
    glPushAttrib(GL_LINE_BIT);
    glLineWidth(5.0f);
    glColor4f(0.9f, 0.9f, 0.1f, 1.0f);
    glPopAttrib();
    glGetFloatv(GL_LINE_WIDTH, fv);
    ok = nearf(fv[0], 2.0f);
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = ok && nearf(fv[0], 0.9f) && nearf(fv[1], 0.9f) && nearf(fv[2], 0.1f) && consume_error(GL_NO_ERROR);
    expect_bool("attrib mask isolation", ok, 7);

    reset_state();
    for (int i = 0; i < 16; ++i) glPushAttrib(GL_CURRENT_BIT);
    glPushAttrib(GL_CURRENT_BIT);
    ok = consume_error(GL_STACK_OVERFLOW);
    for (int i = 0; i < 16; ++i) glPopAttrib();
    glPopAttrib();
    ok = ok && consume_error(GL_STACK_UNDERFLOW);
    list = glGenLists(1);
    glColor4f(0.25f, 0.25f, 0.75f, 1.0f);
    glNewList(list, GL_COMPILE);
    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glPopAttrib();
    glEndList();
    glCallList(list);
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = ok && nearf(fv[0], 0.25f) && nearf(fv[1], 0.25f) && nearf(fv[2], 0.75f) && consume_error(GL_NO_ERROR);
    expect_bool("stack errors and list replay", ok, 8);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -1.42f, 0.0f);
    glVertex3f(x + 0.18f, -1.42f, 0.0f);
    glVertex3f(x + 0.18f, -1.68f, 0.0f);
    glVertex3f(x - 0.18f, -1.68f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 9; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL attrib stack probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        reset_state();
        nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);

        glColor3f(0.18f, 0.72f, 0.92f);
        glBegin(GL_QUADS);
        glVertex3f(-2.35f, 0.92f, 0.0f);
        glVertex3f(-0.75f, 0.92f, 0.0f);
        glVertex3f(-0.75f, -0.24f, 0.0f);
        glVertex3f(-2.35f, -0.24f, 0.0f);
        glEnd();

        glColor3f(0.92f, 0.18f, 0.32f);
        glBegin(GL_QUADS);
        glVertex3f(-0.48f, 0.68f, 0.0f);
        glVertex3f(0.48f, 0.68f, 0.0f);
        glVertex3f(0.48f, -0.48f, 0.0f);
        glVertex3f(-0.48f, -0.48f, 0.0f);
        glEnd();

        glColor3f(0.95f, 0.78f, 0.18f);
        glBegin(GL_QUADS);
        glVertex3f(0.75f, 0.92f, 0.0f);
        glVertex3f(2.35f, 0.92f, 0.0f);
        glVertex3f(2.35f, -0.24f, 0.0f);
        glVertex3f(0.75f, -0.24f, 0.0f);
        glEnd();

        for (int i = 0; i < 9; ++i) {
            draw_result_bar(-2.4f + (float)i * 0.6f, results[i]);
        }

        nxglSwapBuffers("NXGL attrib stack", all_passed() ? "attrib checks passed" : "one or more attrib checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

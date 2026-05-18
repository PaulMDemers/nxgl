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

static bool nearf(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d < 0.001f;
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
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

static void pop_to_base(GLenum mode)
{
    GLint depth = 0;
    glMatrixMode(mode);
    glGetIntegerv(mode == GL_MODELVIEW ? GL_MODELVIEW_STACK_DEPTH :
                  mode == GL_PROJECTION ? GL_PROJECTION_STACK_DEPTH : GL_TEXTURE_STACK_DEPTH,
                  &depth);
    while (depth > 1) {
        glPopMatrix();
        --depth;
    }
    (void)glGetError();
}

static bool stack_limit_case(GLenum mode, GLenum depth_pname, GLint max_depth)
{
    GLint depth = 0;
    bool ok = true;

    glMatrixMode(mode);
    glLoadIdentity();
    glGetIntegerv(depth_pname, &depth);
    ok = ok && depth == 1 && consume_error(GL_NO_ERROR);

    for (GLint i = 1; i < max_depth; ++i) {
        glPushMatrix();
        ok = ok && consume_error(GL_NO_ERROR);
    }
    glGetIntegerv(depth_pname, &depth);
    ok = ok && depth == max_depth && consume_error(GL_NO_ERROR);

    glPushMatrix();
    ok = ok && consume_error(GL_STACK_OVERFLOW);
    glGetIntegerv(depth_pname, &depth);
    ok = ok && depth == max_depth && consume_error(GL_NO_ERROR);

    for (GLint i = 1; i < max_depth; ++i) {
        glPopMatrix();
        ok = ok && consume_error(GL_NO_ERROR);
    }
    glGetIntegerv(depth_pname, &depth);
    ok = ok && depth == 1 && consume_error(GL_NO_ERROR);

    glPopMatrix();
    ok = ok && consume_error(GL_STACK_UNDERFLOW);
    glGetIntegerv(depth_pname, &depth);
    ok = ok && depth == 1 && consume_error(GL_NO_ERROR);
    return ok;
}

static void run_probe(void)
{
    static const GLfloat identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    static const GLfloat translate_matrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.25f, 0.75f, 1.0f
    };
    GLfloat mv[16];
    GLfloat pr[16];
    GLfloat tx[16];
    GLint iv[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &iv[0]);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &iv[1]);
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &iv[2]);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &iv[3]);
    ok = iv[0] == 32 && iv[1] == 8 && iv[2] == 8 && iv[3] == 1 && consume_error(GL_NO_ERROR);
    expect_bool("matrix stack limit queries", ok, 0);

    reset_state();
    expect_bool("modelview stack exact edges", stack_limit_case(GL_MODELVIEW, GL_MODELVIEW_STACK_DEPTH, 32), 1);
    pop_to_base(GL_MODELVIEW);

    reset_state();
    expect_bool("projection stack exact edges", stack_limit_case(GL_PROJECTION, GL_PROJECTION_STACK_DEPTH, 8), 2);
    pop_to_base(GL_PROJECTION);

    reset_state();
    expect_bool("texture stack exact edges", stack_limit_case(GL_TEXTURE, GL_TEXTURE_STACK_DEPTH, 8), 3);
    pop_to_base(GL_TEXTURE);

    reset_state();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.0f, 2.0f, 3.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(2.0f, 3.0f, 4.0f);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.25f, 0.50f, 0.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glGetFloatv(GL_PROJECTION_MATRIX, pr);
    glGetFloatv(GL_TEXTURE_MATRIX, tx);
    ok = nearf(mv[12], 1.0f) && nearf(mv[13], 2.0f) && nearf(mv[14], 3.0f) &&
         nearf(pr[0], 2.0f) && nearf(pr[5], 3.0f) && nearf(pr[10], 4.0f) &&
         nearf(tx[12], 0.25f) && nearf(tx[13], 0.50f) && consume_error(GL_NO_ERROR);
    expect_bool("matrix mode isolation", ok, 4);

    reset_state();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf(translate_matrix);
    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    ok = nearf(mv[12], 1.5f) && nearf(mv[13], -0.25f) && consume_error(GL_NO_ERROR);
    glPopMatrix();
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    ok = ok && nearf(mv[12], 0.5f) && nearf(mv[13], -0.25f) && nearf(mv[14], 0.75f) && consume_error(GL_NO_ERROR);
    expect_bool("push pop restores matrix", ok, 5);

    reset_state();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(1.0, 2.0, 3.0);
    glScaled(2.0, 3.0, 4.0);
    glRotated(90.0, 0.0, 0.0, 1.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    ok = nearf(mv[12], 1.0f) && nearf(mv[13], 2.0f) && nearf(mv[14], 3.0f) &&
         nearf(mv[0], 0.0f) && nearf(mv[1], 3.0f) &&
         nearf(mv[4], -2.0f) && nearf(mv[5], 0.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("double transform wrappers", ok, 6);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(0.75f, 0.0f, 0.0f);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.125f, 0.25f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_MATRIX_MODE, &iv[0]);
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &iv[1]);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glGetFloatv(GL_TEXTURE_MATRIX, tx);
    ok = iv[0] == (GLint)GL_MODELVIEW && iv[1] == 2 &&
         nearf(mv[12], 0.75f) && nearf(tx[12], 0.125f) && nearf(tx[13], 0.25f) &&
         consume_error(GL_NO_ERROR);
    glPopMatrix();
    glDeleteLists(list, 1);
    expect_bool("display-list stack replay", ok && consume_error(GL_NO_ERROR), 7);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE_AND_EXECUTE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-0.50f, 0.25f, 0.0f);
    glEndList();
    glGetFloatv(GL_PROJECTION_MATRIX, pr);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    ok = nearf(pr[0], 0.5f) && nearf(pr[5], 0.6666667f) &&
         nearf(mv[12], -0.50f) && nearf(mv[13], 0.25f) && consume_error(GL_NO_ERROR);
    glLoadIdentity();
    glCallList(list);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glDeleteLists(list, 1);
    expect_bool("compile execute matrix replay", ok && nearf(mv[12], -0.50f) && nearf(mv[13], 0.25f) && consume_error(GL_NO_ERROR), 8);

    reset_state();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(NULL);
    ok = consume_error(GL_INVALID_VALUE);
    glMultMatrixf(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glLoadMatrixd(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glMultMatrixd(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glMatrixMode(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glOrtho(1.0, 1.0, -1.0, 1.0, 1.0, 2.0);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glFrustum(-1.0, 1.0, -1.0, 1.0, 0.0, 2.0);
    expect_bool("matrix validation", ok && consume_error(GL_INVALID_VALUE), 9);

    reset_state();
    glBegin(GL_TRIANGLES);
    glMatrixMode(GL_PROJECTION);
    ok = consume_error(GL_INVALID_OPERATION);
    glLoadIdentity();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPushMatrix();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPopMatrix();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLoadMatrixf(identity);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glMultMatrixf(identity);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glTranslatef(1.0f, 0.0f, 0.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glScalef(1.0f, 2.0f, 1.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, 2.0);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 2.0);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, -0.1f, 0.0f);
    glVertex3f(0.0f, 0.1f, 0.0f);
    glEnd();
    expect_bool("matrix begin guards", ok && consume_error(GL_NO_ERROR), 10);

    reset_state();
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(0.33f, 0.44f, 0.0f);
    glLoadIdentity();
    glPopMatrix();
    glGetFloatv(GL_TEXTURE_MATRIX, tx);
    ok = nearf(tx[12], 0.0f) && nearf(tx[13], 0.0f) && consume_error(GL_NO_ERROR);
    expect_bool("texture stack restore", ok, 11);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
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
    debugPrint("NXGL matrix stack edge probe starting\n");

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
        for (int i = 0; i < 12; ++i) {
            draw_bar(-2.75f + (float)i * 0.48f, results[i]);
        }
        nxglSwapBuffers("NXGL matrix stack edge", all_passed() ? "all checks passed" : "matrix stack edge check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

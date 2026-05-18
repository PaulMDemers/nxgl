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

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 14;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
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
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void finish_triangle(void)
{
    glColor3f(0.20f, 0.70f, 0.95f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glIndexi(3);
    glVertex3f(-0.25f, -0.20f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.25f, -0.20f, 0.0f);
    glTexCoord2f(0.5f, 1.0f);
    glVertex3f(0.0f, 0.30f, 0.0f);
    glEnd();
}

static void begin_finish(void)
{
    glBegin(GL_TRIANGLES);
    finish_triangle();
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLint iv[4] = { 123, 123, 123, 123 };
    GLfloat px[4] = { 0.0f };
    GLuint tex = 0;
    GLuint list = 0;
    GLuint select[8];
    GLfloat feedback[8];
    const GLubyte texel[4] = { 255, 255, 255, 255 };
    bool ok;

    reset_state();
    glBegin(GL_TRIANGLES);
    glMatrixMode(GL_TEXTURE);
    ok = consume_error(GL_INVALID_OPERATION);
    glLoadIdentity();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPushMatrix();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    ok = ok && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_MATRIX_MODE, iv);
    ok = ok && iv[0] == (GLint)GL_MODELVIEW && consume_error(GL_NO_ERROR);
    expect_bool("matrix ops rejected in begin", ok, 0);

    reset_state();
    glBegin(GL_TRIANGLES);
    glViewport(7, 8, 9, 10);
    ok = consume_error(GL_INVALID_OPERATION);
    glScissor(7, 8, 9, 10);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glDepthRange(0.25f, 0.75f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    glGetIntegerv(GL_VIEWPORT, iv);
    ok = ok && iv[0] == 0 && iv[2] == 640 && consume_error(GL_NO_ERROR);
    expect_bool("viewport clear state rejected", ok, 1);

    reset_state();
    glBegin(GL_TRIANGLES);
    glGetIntegerv(GL_VIEWPORT, iv);
    ok = consume_error(GL_INVALID_OPERATION);
    glGetFloatv(GL_CURRENT_COLOR, px);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glGetBooleanv(GL_DEPTH_TEST, (GLboolean *)iv);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glGetPointerv(GL_VERTEX_ARRAY_POINTER, (GLvoid **)&tex);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    expect_bool("get queries rejected in begin", ok && consume_error(GL_NO_ERROR), 2);

    reset_state();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_TRIANGLES);
    glBindTexture(GL_TEXTURE_2D, tex);
    ok = consume_error(GL_INVALID_OPERATION);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    glDeleteTextures(1, &tex);
    expect_bool("texture ops rejected in begin", ok && consume_error(GL_NO_ERROR), 3);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glColor3f(0.9f, 0.1f, 0.1f);
    glEndList();
    glBegin(GL_TRIANGLES);
    glNewList(list, GL_COMPILE);
    ok = consume_error(GL_INVALID_OPERATION);
    glCallList(list);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glCallLists(1, GL_UNSIGNED_INT, &list);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glListBase(list);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    glDeleteLists(list, 1);
    expect_bool("display-list ops rejected in begin", ok && consume_error(GL_NO_ERROR), 4);

    reset_state();
    memset(select, 0, sizeof(select));
    memset(feedback, 0, sizeof(feedback));
    glSelectBuffer(8, select);
    glFeedbackBuffer(8, GL_3D, feedback);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(1);
    glBegin(GL_TRIANGLES);
    glRenderMode(GL_RENDER);
    ok = consume_error(GL_INVALID_OPERATION);
    glSelectBuffer(8, select);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glFeedbackBuffer(8, GL_3D, feedback);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPushName(2);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLoadName(3);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPassThrough(91.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    glRenderMode(GL_RENDER);
    expect_bool("select feedback ops rejected", ok && consume_error(GL_NO_ERROR), 5);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    ok = consume_error(GL_INVALID_OPERATION);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texel);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glCopyPixels(0, 0, 1, 1, GL_COLOR);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    expect_bool("pixel ops rejected in begin", ok && consume_error(GL_NO_ERROR), 6);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glColor3f(0.20f, 0.70f, 0.95f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glIndexf(7.0f);
    glVertex3f(-0.45f, -0.35f, 0.0f);
    glColor3f(0.20f, 0.70f, 0.95f);
    glVertex3f(0.45f, -0.35f, 0.0f);
    glColor3f(0.20f, 0.70f, 0.95f);
    glVertex3f(0.0f, 0.45f, 0.0f);
    glEnd();
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 51, 178, 242) && consume_error(GL_NO_ERROR);
    expect_bool("legal immediate calls still draw", ok, 7);

    reset_state();
    glBegin(GL_TRIANGLES);
    glClearIndex(2.0f);
    ok = consume_error(GL_INVALID_OPERATION);
    glClearDepth(0.5f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glClearStencil(3);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glClearAccum(0.1f, 0.2f, 0.3f, 0.4f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    finish_triangle();
    expect_bool("clear value ops rejected", ok && consume_error(GL_NO_ERROR), 8);

    reset_state();
    begin_finish();
    ok = consume_error(GL_NO_ERROR);
    glEnd();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glBegin(0xffffffffu);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("begin end envelope validation", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
    glRectf(x - 0.14f, -0.72f, x + 0.14f, -0.88f);
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
    debugPrint("NXGL begin/end illegal ops probe starting\n");

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
        for (int i = 0; i < 10; ++i) {
            draw_bar(-1.35f + (float)i * 0.30f, results[i]);
        }
        nxglSwapBuffers("NXGL begin/end illegal ops", all_passed() ? "all checks passed" : "begin/end checks failed");
        Sleep(16);
    }
}

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

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.002f;
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

static GLuint make_index_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glIndexf(42.5f);
    glClearIndex(9.0f);
    glEndList();
    return list;
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat fv[4];
    GLdouble dv[4];
    GLint iv[4];
    GLubyte index_ub = 7;
    GLshort index_s = 12;
    GLuint list = 0;
    GLuint lists[3] = { 0, 0, 0 };
    GLuint tex[2] = { 0, 0 };
    GLclampf priorities[2] = { 0.25f, 0.75f };
    GLboolean resident[2] = { GL_FALSE, GL_FALSE };
    GLuint select[8];
    bool ok;

    reset_state();
    glClearIndex(3.0f);
    glIndexubv(&index_ub);
    glGetFloatv(GL_CURRENT_INDEX, fv);
    glGetDoublev(GL_INDEX_CLEAR_VALUE, dv);
    glGetIntegerv(GL_INDEX_MODE, iv);
    ok = nearf(fv[0], 7.0f) && dv[0] == 3.0 && iv[0] == GL_FALSE && consume_error(GL_NO_ERROR);
    list = make_index_list();
    glIndexsv(&index_s);
    glCallList(list);
    glGetFloatv(GL_CURRENT_INDEX, fv);
    glGetIntegerv(GL_INDEX_CLEAR_VALUE, iv);
    ok = ok && nearf(fv[0], 42.5f) && iv[0] == 9 && consume_error(GL_NO_ERROR);
    glDeleteLists(list, 1);
    expect_bool("color-index stubs", ok, 0);

    reset_state();
    lists[0] = glGenLists(3);
    lists[1] = lists[0] + 1;
    lists[2] = lists[0] + 2;
    glNewList(lists[0], GL_COMPILE);
    glColor3f(0.8f, 0.1f, 0.1f);
    glEndList();
    glNewList(lists[1], GL_COMPILE_AND_EXECUTE);
    glColor3f(0.1f, 0.8f, 0.1f);
    glEndList();
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = lists[0] != 0 && glIsList(lists[0]) == GL_TRUE && glIsList(lists[2]) == GL_TRUE &&
         nearf(fv[1], 0.8f) && consume_error(GL_NO_ERROR);
    glDeleteLists(lists[0], 3);
    ok = ok && glIsList(lists[0]) == GL_FALSE && glIsList(lists[2]) == GL_FALSE && consume_error(GL_NO_ERROR);
    glGenLists(-1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("display-list lifecycle", ok, 1);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glColor3f(0.1f, 0.2f, 0.9f);
    glEndList();
    glListBase(list);
    {
        const GLubyte bytes[1] = { 0 };
        glColor3f(1.0f, 1.0f, 1.0f);
        glCallLists(1, GL_UNSIGNED_BYTE, bytes);
    }
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = nearf(fv[0], 0.1f) && nearf(fv[2], 0.9f) && consume_error(GL_NO_ERROR);
    glCallLists(-1, GL_UNSIGNED_BYTE, (const GLubyte[]){ 0 });
    ok = ok && consume_error(GL_INVALID_VALUE);
    glCallLists(1, GL_FLOAT, (const GLfloat[]){ 0.0f });
    ok = ok && consume_error(GL_INVALID_ENUM);
    glDeleteLists(list, 1);
    expect_bool("call-lists aliases", ok, 2);

    reset_state();
    glGenTextures(2, tex);
    glPrioritizeTextures(2, tex, priorities);
    ok = glAreTexturesResident(2, tex, resident) == GL_TRUE &&
         resident[0] == GL_TRUE && resident[1] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    glDeleteTextures(1, &tex[1]);
    ok = ok && glAreTexturesResident(2, tex, resident) == GL_FALSE &&
         resident[0] == GL_TRUE && resident[1] == GL_FALSE &&
         consume_error(GL_NO_ERROR);
    glPrioritizeTextures(-1, tex, priorities);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glAreTexturesResident(1, NULL, resident);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glDeleteTextures(1, &tex[0]);
    expect_bool("texture residency priority", ok, 3);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.85f, 0.10f, 0.10f);
    glRecti(-1, 1, 0, 0);
    glColor3f(0.10f, 0.80f, 0.20f);
    glRects(0, 1, 1, 0);
    glColor3f(0.12f, 0.35f, 0.90f);
    glRectdv((const GLdouble[]){ -0.5, -0.1 }, (const GLdouble[]){ 0.5, -0.6 });
    read_color(220, 160, pixel);
    ok = pixel_rgb(pixel, 217, 25, 25);
    read_color(420, 160, pixel);
    ok = ok && pixel_rgb(pixel, 25, 204, 51);
    read_color(320, 320, pixel);
    ok = ok && pixel_rgb(pixel, 31, 89, 230) && consume_error(GL_NO_ERROR);
    glRectfv(NULL, (const GLfloat[]){ 0.0f, 0.0f });
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("rect wrapper surface", ok, 4);

    reset_state();
    glEnd();
    ok = consume_error(GL_INVALID_OPERATION);
    glBegin(GL_TRIANGLES);
    glBegin(GL_LINES);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    ok = ok && consume_error(GL_NO_ERROR);
    glEnd();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glBegin(0xffffffffu);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("begin-end validation", ok, 5);

    reset_state();
    memset(select, 0, sizeof(select));
    glPushName(1);
    ok = consume_error(GL_INVALID_OPERATION);
    glSelectBuffer(8, select);
    glRenderMode(GL_SELECT);
    glLoadName(2);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glInitNames();
    for (int i = 0; i < 64; ++i) {
        glPushName((GLuint)i);
    }
    ok = ok && consume_error(GL_NO_ERROR);
    glPushName(65);
    ok = ok && consume_error(GL_STACK_OVERFLOW);
    for (int i = 0; i < 64; ++i) {
        glPopName();
    }
    ok = ok && consume_error(GL_NO_ERROR);
    glPopName();
    ok = ok && consume_error(GL_STACK_UNDERFLOW);
    glRenderMode(GL_RENDER);
    expect_bool("name stack edge cases", ok, 6);

    reset_state();
    glNewList(0, GL_COMPILE);
    ok = consume_error(GL_INVALID_VALUE);
    glNewList(250, GL_COMPILE);
    glNewList(251, GL_COMPILE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glEndList();
    ok = ok && consume_error(GL_NO_ERROR);
    glEndList();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glBegin(GL_TRIANGLES);
    glNewList(252, GL_COMPILE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    glDeleteLists(250, 1);
    expect_bool("new-list validation", ok, 7);

    reset_state();
    glIndexd(5.0);
    glGetBooleanv(GL_CURRENT_INDEX, (GLboolean *)iv);
    ok = ((GLboolean *)iv)[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    glIndexfv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glIndexdv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glIndexiv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glIndexsv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glIndexubv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("index vector validation", ok, 8);

    reset_state();
    glClearColor(0.02f, 0.03f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE_AND_EXECUTE);
    glColor3f(0.90f, 0.65f, 0.12f);
    glRectsv((const GLshort[]){ -1, 0 }, (const GLshort[]){ 1, -1 });
    glEndList();
    read_color(320, 320, pixel);
    ok = pixel_rgb(pixel, 230, 166, 31) && consume_error(GL_NO_ERROR);
    glDeleteLists(list, 1);
    expect_bool("compile-execute rect", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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
    debugPrint("NXGL API completeness probe starting\n");

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
        nxglSwapBuffers("NXGL API completeness", all_passed() ? "all checks passed" : "API completeness failed");
        Sleep(16);
    }
}

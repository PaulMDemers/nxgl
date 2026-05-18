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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 16;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= 0.025f;
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
    for (int i = 0; i < 6; ++i) {
        glDisable(GL_CLIP_PLANE0 + i);
    }
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glLogicOp(GL_COPY);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelZoom(1.0f, 1.0f);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    (void)glGetError();
}

static void clear_all(uint8_t r, uint8_t g, uint8_t b, GLfloat depth, GLint stencil)
{
    glClearColor((GLfloat)r / 255.0f, (GLfloat)g / 255.0f, (GLfloat)b / 255.0f, 1.0f);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void raster_at(GLint x, GLint y)
{
    GLfloat wx = ((GLfloat)x - 320.0f) * 5.2f / 320.0f;
    GLfloat wy = ((GLfloat)y - 240.0f) * 5.2f / 240.0f;
    glRasterPos3f(wx, wy, 0.0f);
}

static void draw_quad(float z, uint8_t r, uint8_t g, uint8_t b)
{
    glColor3f((GLfloat)r / 255.0f, (GLfloat)g / 255.0f, (GLfloat)b / 255.0f);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, z);
    glVertex3f(0.55f, 0.55f, z);
    glVertex3f(0.55f, -0.55f, z);
    glVertex3f(-0.55f, -0.55f, z);
    glEnd();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static GLfloat read_depth(GLint x, GLint y)
{
    GLfloat depth = 0.0f;
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    return depth;
}

static uint8_t read_stencil(GLint x, GLint y)
{
    uint8_t stencil = 0;
    glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    return stencil;
}

static void set_rgba(uint8_t rgba[4], uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    rgba[0] = r;
    rgba[1] = g;
    rgba[2] = b;
    rgba[3] = a;
}

static void run_probe(void)
{
    uint8_t pixel[4];
    uint8_t src[4 * 4];
    uint8_t packed[24];
    GLfloat depth_value;
    uint8_t stencil_value;
    GLuint list;
    bool ok;

    reset_state();
    clear_all(0, 0, 0, 1.0f, 0);
    draw_quad(0.0f, 230, 30, 30);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 230, 30, 30) && consume_error(GL_NO_ERROR);
    expect_bool("read flushes queued primitive", ok, 0);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 4);
    set_rgba(src, 230, 30, 30, 64);
    raster_at(320, 240);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 7, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && near_float(read_depth(320, 240), 1.0f) &&
         read_stencil(320, 240) == 4 && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels alpha blocks fragments", ok, 1);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 4);
    set_rgba(src, 40, 220, 80, 255);
    raster_at(320, 240);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 9, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 40, 220, 80) && read_depth(320, 240) < 0.12f &&
         read_stencil(320, 240) == 9 && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels alpha pass writes all", ok, 2);

    reset_state();
    clear_all(10, 20, 30, 0.02f, 5);
    set_rgba(src, 200, 200, 200, 255);
    raster_at(320, 240);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 10, 20, 30) && near_float(read_depth(320, 240), 0.02f) &&
         read_stencil(320, 240) == 6 && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels depth fail zfail", ok, 3);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 1);
    for (int i = 0; i < 4; ++i) {
        set_rgba(src + i * 4, 230, 180, 40, 255);
    }
    raster_at(319, 240);
    glEnable(GL_SCISSOR_TEST);
    glScissor(320, 240, 1, 1);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 7, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glDrawPixels(3, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    read_color(319, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && read_stencil(319, 240) == 1;
    read_color(320, 240, pixel);
    ok = ok && pixel_rgb(pixel, 230, 180, 40) && read_stencil(320, 240) == 7;
    read_color(321, 240, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && read_stencil(321, 240) == 1 && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels scissor gates writes", ok, 4);

    reset_state();
    clear_all(0, 0, 255, 1.0f, 0);
    set_rgba(src, 255, 255, 255, 255);
    raster_at(100, 100);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    raster_at(320, 240);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glCopyPixels(100, 100, 1, 1, GL_COLOR);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("copypixels color logic op", ok, 5);

    reset_state();
    clear_all(18, 52, 86, 1.0f, 0);
    set_rgba(src, 240, 15, 170, 255);
    raster_at(100, 100);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    raster_at(320, 240);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
    glCopyPixels(100, 100, 1, 1, GL_COLOR);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 240, 52, 170) && consume_error(GL_NO_ERROR);
    expect_bool("copypixels color mask", ok, 6);

    reset_state();
    clear_all(0, 0, 0, 0.20f, 0);
    depth_value = 0.50f;
    raster_at(100, 100);
    glDisable(GL_DEPTH_TEST);
    glDrawPixels(1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_value);
    raster_at(320, 240);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCopyPixels(100, 100, 1, 1, GL_DEPTH);
    ok = near_float(read_depth(320, 240), 0.20f) && consume_error(GL_NO_ERROR);
    expect_bool("copypixels depth test blocks", ok, 7);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 0xa0);
    stencil_value = 0x5c;
    glStencilMask(0x0f);
    raster_at(320, 240);
    glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_value);
    ok = read_stencil(320, 240) == 0xac && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels stencil write mask", ok, 8);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 2);
    stencil_value = 9;
    raster_at(100, 100);
    glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_value);
    raster_at(320, 240);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 10, 10);
    glCopyPixels(100, 100, 1, 1, GL_STENCIL);
    ok = read_stencil(320, 240) == 2 && consume_error(GL_NO_ERROR);
    expect_bool("copypixels stencil scissor", ok, 9);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 0);
    set_rgba(src, 20, 210, 240, 255);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    raster_at(320, 240);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glEndList();
    glCallList(list);
    glDeleteLists(list, 1);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 20, 210, 240) && consume_error(GL_NO_ERROR);
    expect_bool("display-list pixel interlock", ok, 10);

    reset_state();
    clear_all(40, 80, 120, 1.0f, 0);
    draw_quad(0.0f, 200, 60, 20);
    memset(packed, 0x55, sizeof(packed));
    glPixelStorei(GL_PACK_ROW_LENGTH, 3);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 1);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, packed);
    ok = packed[0] == 0x55 && pixel_rgb(packed + 4, 200, 60, 20) && consume_error(GL_NO_ERROR);
    glBegin(GL_POINTS);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glCopyPixels(320, 240, 1, 1, GL_COLOR);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    expect_bool("read pack and begin validation", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_bar(float x, bool pass)
{
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.78f : 0.12f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.10f, -0.75f, 0.0f);
    glVertex3f(x + 0.10f, -0.75f, 0.0f);
    glVertex3f(x + 0.10f, -0.98f, 0.0f);
    glVertex3f(x - 0.10f, -0.98f, 0.0f);
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
    debugPrint("NXGL pixel transfer interlock probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_all(4, 4, 10, 1.0f, 0);
        glColor3f(0.14f, 0.42f, 0.82f);
        glRectf(-1.55f, 0.42f, 1.55f, -0.42f);
        for (int i = 0; i < 12; ++i) {
            draw_bar(-1.45f + (float)i * 0.26f, results[i]);
        }
        nxglSwapBuffers("NXGL pixel transfer interlocks", all_passed() ? "all checks passed" : "pixel interlock failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 8;
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

static void set_integer_identity(GLenum map, int count)
{
    GLfloat values[16];

    if (count > 16) count = 16;
    for (int i = 0; i < count; ++i) {
        values[i] = (GLfloat)i / 255.0f;
    }
    glPixelMapfv(map, count, values);
}

static void reset_state(void)
{
    GLfloat rgba_identity[2] = { 0.0f, 1.0f };

    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPixelZoom(1.0f, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelTransferf(GL_RED_SCALE, 1.0f);
    glPixelTransferf(GL_GREEN_SCALE, 1.0f);
    glPixelTransferf(GL_BLUE_SCALE, 1.0f);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
    glPixelTransferf(GL_RED_BIAS, 0.0f);
    glPixelTransferf(GL_GREEN_BIAS, 0.0f);
    glPixelTransferf(GL_BLUE_BIAS, 0.0f);
    glPixelTransferf(GL_ALPHA_BIAS, 0.0f);
    set_integer_identity(GL_PIXEL_MAP_I_TO_I, 16);
    set_integer_identity(GL_PIXEL_MAP_S_TO_S, 16);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 2, rgba_identity);
    glPixelMapfv(GL_PIXEL_MAP_A_TO_A, 2, rgba_identity);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void setup_index_to_rgba_maps(void)
{
    GLfloat r[4] = { 0.0f, 0.20f, 0.80f, 1.0f };
    GLfloat g[4] = { 0.0f, 0.60f, 0.10f, 1.0f };
    GLfloat b[4] = { 0.0f, 0.90f, 0.30f, 1.0f };
    GLfloat a[4] = { 0.0f, 1.00f, 1.00f, 1.0f };

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 4, r);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 4, g);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 4, b);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 4, a);
}

static void clear_frame(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void draw_index_pixel(GLenum type, const void *value, uint8_t pixel[4])
{
    GLfloat raster[4] = { 0.0f };

    clear_frame();
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glDrawPixels(1, 1, GL_COLOR_INDEX, type, value);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    GLfloat fv[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint iv[4] = { 0, 0, 0, 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    uint8_t indices[8] = { 0 };
    GLushort us_index = 0;
    GLushort stencil_value = 0;
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_PIXEL_MAP_I_TO_R_SIZE, &iv[0]);
    glGetIntegerv(GL_PIXEL_MAP_I_TO_I_SIZE, &iv[1]);
    glGetIntegerv(GL_PIXEL_MAP_S_TO_S_SIZE, &iv[2]);
    ok = iv[0] == 2 && iv[1] == 16 && iv[2] == 16 && consume_error(GL_NO_ERROR);
    expect_bool("default indexed map sizes", ok, 0);

    reset_state();
    setup_index_to_rgba_maps();
    indices[0] = 1;
    draw_index_pixel(GL_UNSIGNED_BYTE, indices, pixel);
    ok = pixel_rgb(pixel, 51, 153, 230) && consume_error(GL_NO_ERROR);
    expect_bool("ubyte color-index draw", ok, 1);

    reset_state();
    setup_index_to_rgba_maps();
    us_index = 2;
    draw_index_pixel(GL_UNSIGNED_SHORT, &us_index, pixel);
    ok = pixel_rgb(pixel, 204, 25, 76) && consume_error(GL_NO_ERROR);
    expect_bool("ushort color-index draw", ok, 2);

    reset_state();
    setup_index_to_rgba_maps();
    {
        GLfloat i_to_i[4] = { 0.0f, 2.0f / 255.0f, 3.0f / 255.0f, 1.0f };
        glPixelMapfv(GL_PIXEL_MAP_I_TO_I, 4, i_to_i);
    }
    indices[0] = 1;
    draw_index_pixel(GL_UNSIGNED_BYTE, indices, pixel);
    ok = pixel_rgb(pixel, 204, 25, 76) && consume_error(GL_NO_ERROR);
    expect_bool("i-to-i remaps color index", ok, 3);

    reset_state();
    setup_index_to_rgba_maps();
    memset(indices, 0, sizeof(indices));
    indices[0] = 1; indices[1] = 1; indices[2] = 1;
    indices[4] = 2; indices[5] = 2; indices[6] = 2;
    clear_frame();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, fv);
    glDrawPixels(3, 2, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, indices);
    glReadPixels((GLint)fv[0] + 1, (GLint)fv[1] + 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb(pixel, 204, 25, 76) && consume_error(GL_NO_ERROR);
    expect_bool("color-index unpack stride", ok, 4);

    reset_state();
    {
        GLfloat s_to_s[4] = { 0.0f, 5.0f / 255.0f, 9.0f / 255.0f, 13.0f / 255.0f };
        uint8_t stencil_src = 2;
        glPixelMapfv(GL_PIXEL_MAP_S_TO_S, 4, s_to_s);
        clear_frame();
        glRasterPos3f(0.0f, 0.0f, 0.0f);
        glGetFloatv(GL_CURRENT_RASTER_POSITION, fv);
        glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_src);
        set_integer_identity(GL_PIXEL_MAP_S_TO_S, 16);
        glReadPixels((GLint)fv[0], (GLint)fv[1], 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_SHORT, &stencil_value);
        ok = stencil_value == 9 && consume_error(GL_NO_ERROR);
    }
    expect_bool("s-to-s draw stencil", ok, 5);

    reset_state();
    {
        GLfloat s_to_s[6] = { 0.0f, 1.0f / 255.0f, 2.0f / 255.0f, 3.0f / 255.0f, 11.0f / 255.0f, 5.0f / 255.0f };
        uint8_t stencil_src = 4;
        clear_frame();
        glRasterPos3f(-0.18f, 0.0f, 0.0f);
        glGetFloatv(GL_CURRENT_RASTER_POSITION, fv);
        GLint src_x = (GLint)fv[0];
        GLint src_y = (GLint)fv[1];
        glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_src);
        glPixelMapfv(GL_PIXEL_MAP_S_TO_S, 6, s_to_s);
        glRasterPos3f(0.18f, 0.0f, 0.0f);
        glGetFloatv(GL_CURRENT_RASTER_POSITION, fv);
        glCopyPixels(src_x, src_y, 1, 1, GL_STENCIL);
        set_integer_identity(GL_PIXEL_MAP_S_TO_S, 16);
        glReadPixels((GLint)fv[0], (GLint)fv[1], 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_SHORT, &stencil_value);
        ok = stencil_value == 11 && consume_error(GL_NO_ERROR);
    }
    expect_bool("s-to-s copy stencil", ok, 6);

    reset_state();
    {
        GLfloat rmap[4] = { 0.0f, 0.25f, 0.50f, 0.75f };
        GLfloat queried[4] = { 0.0f };
        list = glGenLists(1);
        glNewList(list, GL_COMPILE);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 4, rmap);
        glEndList();
        glCallList(list);
        glGetPixelMapfv(GL_PIXEL_MAP_I_TO_R, queried);
        glDeleteLists(list, 1);
        ok = queried[1] > 0.249f && queried[1] < 0.251f &&
             queried[3] > 0.749f && queried[3] < 0.751f &&
             consume_error(GL_NO_ERROR);
    }
    expect_bool("display-list indexed map", ok, 7);

    reset_state();
    {
        GLfloat rmap[4] = { 0.0f, 0.25f, 0.50f, 1.0f };
        GLuint uiv[4] = { 0, 0, 0, 0 };
        GLushort usv[4] = { 0, 0, 0, 0 };
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 4, rmap);
        glGetPixelMapuiv(GL_PIXEL_MAP_I_TO_R, uiv);
        glGetPixelMapusv(GL_PIXEL_MAP_I_TO_R, usv);
        ok = uiv[0] == 0 && uiv[3] == 0xffffffffu &&
             usv[0] == 0 && usv[2] >= 32760 && usv[2] <= 32776 &&
             consume_error(GL_NO_ERROR);
    }
    expect_bool("indexed map integer query", ok, 8);

    reset_state();
    glDrawPixels(1, 1, GL_COLOR_INDEX, GL_FLOAT, indices);
    ok = consume_error(GL_INVALID_ENUM);
    glBegin(GL_TRIANGLES);
    glPixelZoom(2.0f, 2.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPixelTransferf(GL_RED_BIAS, 0.1f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, fv);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glGetPixelMapfv(GL_PIXEL_MAP_I_TO_R, fv);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColor3f(0.1f, 0.7f, 0.9f);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("indexed pixel validation", ok && consume_error(GL_NO_ERROR), 9);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
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
    uint8_t indices[1] = { 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL indexed pixel map probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_frame();
        setup_index_to_rgba_maps();
        indices[0] = all_passed() ? 1 : 2;
        draw_index_pixel(GL_UNSIGNED_BYTE, indices, pixel);
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL indexed pixel maps", all_passed() ? "all checks passed" : "indexed pixel-map check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

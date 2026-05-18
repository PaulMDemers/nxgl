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
    return d <= 10;
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

static GLfloat cf(uint8_t v)
{
    return (GLfloat)v / 255.0f;
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
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_POINT);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPointSize(1.0f);
    glLineWidth(1.0f);
    glLineStipple(1, 0xffff);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void clear_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(cf(r), cf(g), cf(b), 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void read_pixel(int x, int y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_point(float x, float y, uint8_t r, uint8_t g, uint8_t b)
{
    glColor3f(cf(r), cf(g), cf(b));
    glBegin(GL_POINTS);
    glVertex3f(x, y, 0.0f);
    glEnd();
}

static void draw_line(float x0, float y0, float x1, float y1,
                      uint8_t r, uint8_t g, uint8_t b)
{
    glColor3f(cf(r), cf(g), cf(b));
    glBegin(GL_LINES);
    glVertex3f(x0, y0, 0.0f);
    glVertex3f(x1, y1, 0.0f);
    glEnd();
}

static void draw_quad(float left, float top, float right, float bottom,
                      uint8_t r, uint8_t g, uint8_t b)
{
    glColor3f(cf(r), cf(g), cf(b));
    glBegin(GL_QUADS);
    glVertex3f(left, top, 0.0f);
    glVertex3f(right, top, 0.0f);
    glVertex3f(right, bottom, 0.0f);
    glVertex3f(left, bottom, 0.0f);
    glEnd();
}

static void window_from_world(float x, float y, int *wx, int *wy)
{
    *wx = 320 + (int)((x / 5.2f) * 320.0f);
    *wy = 240 + (int)((y / 5.2f) * 240.0f);
}

static void ones_stipple(GLubyte mask[128])
{
    memset(mask, 0xff, 128);
}

static void zeros_stipple(GLubyte mask[128])
{
    memset(mask, 0x00, 128);
}

static void run_probe(void)
{
    GLfloat point_size = 0.0f;
    GLfloat line_width = 0.0f;
    GLfloat fv[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint iv[4] = { 0, 0, 0, 0 };
    uint8_t p0[4] = { 0, 0, 0, 0 };
    uint8_t p1[4] = { 0, 0, 0, 0 };
    uint8_t p2[4] = { 0, 0, 0, 0 };
    GLubyte mask[128];
    GLubyte outmask[128];
    GLuint list;
    int sx;
    int sy;
    bool ok;

    reset_state();
    glGetFloatv(GL_POINT_SIZE, &point_size);
    glGetFloatv(GL_LINE_WIDTH, &line_width);
    glGetIntegerv(GL_POLYGON_MODE, iv);
    glGetFloatv(GL_POINT_SIZE_RANGE, fv);
    ok = point_size == 1.0f && line_width == 1.0f &&
         fv[0] == 1.0f && fv[1] == 64.0f &&
         iv[0] == (GLint)GL_FILL && iv[1] == (GLint)GL_FILL &&
         consume_error(GL_NO_ERROR);
    glGetFloatv(GL_LINE_WIDTH_RANGE, fv);
    ok = ok && fv[0] == 1.0f && fv[1] == 64.0f && consume_error(GL_NO_ERROR);
    expect_bool("default raster ranges", ok, 0);

    reset_state();
    clear_rgb(0, 0, 0);
    glPointSize(16.0f);
    draw_point(0.0f, 0.0f, 220, 35, 35);
    read_pixel(320, 240, p0);
    read_pixel(330, 240, p1);
    read_pixel(350, 240, p2);
    ok = pixel_rgb(p0, 220, 35, 35) && pixel_rgb(p1, 220, 35, 35) &&
         pixel_rgb(p2, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("wide point center coverage", ok, 1);

    reset_state();
    clear_rgb(0, 0, 0);
    glPointSize(16.0f);
    draw_point(-5.15f, 0.0f, 35, 220, 35);
    read_pixel(2, 240, p0);
    read_pixel(40, 240, p1);
    ok = pixel_rgb(p0, 35, 220, 35) && pixel_rgb(p1, 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("wide point clipped at edge", ok, 2);

    reset_state();
    clear_rgb(0, 0, 0);
    glLineWidth(8.0f);
    draw_line(-0.8f, 0.0f, 0.8f, 0.0f, 35, 120, 235);
    read_pixel(320, 240, p0);
    read_pixel(320, 243, p1);
    read_pixel(320, 250, p2);
    ok = pixel_rgb(p0, 35, 120, 235) && pixel_rgb(p1, 35, 120, 235) &&
         pixel_rgb(p2, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("wide line center coverage", ok, 3);

    reset_state();
    clear_rgb(0, 0, 0);
    glLineWidth(6.0f);
    draw_line(-8.0f, 0.35f, 0.35f, 0.35f, 220, 200, 35);
    read_pixel(4, 256, p0);
    read_pixel(320, 256, p1);
    ok = pixel_rgb(p0, 220, 200, 35) && pixel_rgb(p1, 220, 200, 35) &&
         consume_error(GL_NO_ERROR);
    expect_bool("wide line clipped at edge", ok, 4);

    reset_state();
    clear_rgb(0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glPointSize(12.0f);
    draw_quad(-0.55f, 0.55f, 0.55f, -0.55f, 230, 60, 180);
    window_from_world(-0.55f, 0.55f, &sx, &sy);
    read_pixel(sx, sy, p0);
    read_pixel(320, 240, p1);
    ok = pixel_rgb(p0, 230, 60, 180) && pixel_rgb(p1, 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("polygon point mode vertices", ok, 5);

    reset_state();
    clear_rgb(0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(6.0f);
    draw_quad(-0.55f, 0.55f, 0.55f, -0.55f, 50, 210, 210);
    read_pixel(320, 265, p0);
    read_pixel(320, 240, p1);
    ok = pixel_rgb(p0, 50, 210, 210) && pixel_rgb(p1, 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("polygon line mode edges", ok, 6);

    reset_state();
    clear_rgb(0, 0, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    draw_quad(-0.55f, 0.55f, 0.55f, -0.55f, 200, 120, 45);
    read_pixel(320, 240, p0);
    ok = pixel_rgb(p0, 200, 120, 45) && consume_error(GL_NO_ERROR);
    expect_bool("polygon fill mode interior", ok, 7);

    reset_state();
    clear_rgb(0, 0, 0);
    glLineWidth(8.0f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0000);
    draw_line(-0.7f, -0.15f, 0.7f, -0.15f, 230, 230, 230);
    read_pixel(320, 233, p0);
    glLineStipple(1, 0xffff);
    draw_line(-0.7f, 0.15f, 0.7f, 0.15f, 230, 230, 230);
    read_pixel(320, 247, p1);
    ok = pixel_rgb(p0, 0, 0, 0) && pixel_rgb(p1, 230, 230, 230) &&
         consume_error(GL_NO_ERROR);
    expect_bool("line stipple shadow extremes", ok, 8);

    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_POLYGON_STIPPLE);
    zeros_stipple(mask);
    glPolygonStipple(mask);
    draw_quad(-0.45f, 0.45f, 0.45f, -0.45f, 230, 230, 40);
    read_pixel(320, 240, p0);
    ones_stipple(mask);
    glPolygonStipple(mask);
    draw_quad(-0.45f, 0.45f, 0.45f, -0.45f, 230, 230, 40);
    read_pixel(320, 240, p1);
    ok = pixel_rgb(p0, 0, 0, 0) && pixel_rgb(p1, 230, 230, 40) &&
         consume_error(GL_NO_ERROR);
    expect_bool("polygon stipple shadow extremes", ok, 9);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPointSize(9.0f);
    glLineWidth(5.0f);
    glLineStipple(3, 0x0f0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonOffset(1.25f, -2.5f);
    ones_stipple(mask);
    glPolygonStipple(mask);
    glEndList();
    glCallList(list);
    glDeleteLists(list, 1);
    glGetFloatv(GL_POINT_SIZE, &fv[0]);
    glGetFloatv(GL_LINE_WIDTH, &fv[1]);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &iv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &iv[1]);
    glGetIntegerv(GL_POLYGON_MODE, &iv[2]);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &fv[2]);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &fv[3]);
    glGetPolygonStipple(outmask);
    ok = fv[0] == 9.0f && fv[1] == 5.0f &&
         iv[0] == 3 && iv[1] == 0x0f0f && iv[2] == (GLint)GL_LINE &&
         fv[2] > 1.24f && fv[2] < 1.26f && fv[3] < -2.49f && fv[3] > -2.51f &&
         outmask[0] == 0xff && outmask[127] == 0xff &&
         consume_error(GL_NO_ERROR);
    expect_bool("display-list raster state", ok, 10);

    reset_state();
    glPointSize(0.0f);
    ok = consume_error(GL_INVALID_VALUE);
    glLineWidth(0.0f);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glLineStipple(0, 0xffff);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPolygonMode(GL_TEXTURE_2D, GL_FILL);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glPolygonMode(GL_FRONT_AND_BACK, GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glPolygonStipple(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetPolygonStipple(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glBegin(GL_TRIANGLES);
    glPointSize(2.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLineWidth(2.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLineStipple(1, 0xffff);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    ones_stipple(mask);
    glPolygonStipple(mask);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glGetPolygonStipple(outmask);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPolygonOffset(0.0f, 0.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColor3f(0.2f, 0.6f, 0.9f);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("raster validation and begin guards", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
    for (int i = 0; i < 12; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL raster edge rules probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_rgb(4, 4, 10);
        draw_quad(-1.25f, 0.75f, 1.25f, -0.75f,
                  all_passed() ? 30 : 160,
                  all_passed() ? 120 : 20,
                  all_passed() ? 230 : 20);
        for (int i = 0; i < 12; ++i) {
            draw_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL raster edge rules", all_passed() ? "all checks passed" : "raster edge check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

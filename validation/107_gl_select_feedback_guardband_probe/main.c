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

static bool nearf(GLfloat actual, GLfloat expected, GLfloat tolerance)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= tolerance;
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
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    (void)glGetError();
}

static void draw_point(GLfloat x, GLfloat y, GLfloat z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void draw_line(GLfloat ax, GLfloat ay, GLfloat az, GLfloat bx, GLfloat by, GLfloat bz)
{
    glBegin(GL_LINES);
    glVertex3f(ax, ay, az);
    glVertex3f(bx, by, bz);
    glEnd();
}

static void draw_quad(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat x1, GLfloat y1, GLfloat z1)
{
    glBegin(GL_QUADS);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z0);
    glEnd();
}

static GLint feedback_point(GLfloat x, GLfloat y, GLfloat z, GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 64u);
    glFeedbackBuffer(64, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_point(x, y, z);
    return glRenderMode(GL_RENDER);
}

static GLint select_point(GLfloat x, GLfloat y, GLfloat z, GLuint name, GLuint *buffer)
{
    memset(buffer, 0, sizeof(GLuint) * 32u);
    glSelectBuffer(32, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(name);
    draw_point(x, y, z);
    return glRenderMode(GL_RENDER);
}

static GLint feedback_line(GLfloat ax, GLfloat ay, GLfloat az, GLfloat bx, GLfloat by, GLfloat bz, GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 96u);
    glFeedbackBuffer(96, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_line(ax, ay, az, bx, by, bz);
    return glRenderMode(GL_RENDER);
}

static bool polygon_feedback_vertices_in_bounds(const GLfloat *buffer, GLint total_count)
{
    GLint vertex_count;

    if (total_count < 5 || !nearf(buffer[0], (GLfloat)GL_POLYGON_TOKEN, 0.01f)) {
        return false;
    }
    vertex_count = (GLint)buffer[1];
    if (total_count != 2 + vertex_count * 3 || vertex_count < 3 || vertex_count > 8) {
        return false;
    }
    for (GLint i = 0; i < vertex_count; ++i) {
        GLfloat x = buffer[2 + i * 3];
        GLfloat y = buffer[3 + i * 3];
        GLfloat z = buffer[4 + i * 3];
        if (x < -1.0f || x > 641.0f || y < -1.0f || y > 481.0f || z < -0.01f || z > 1.01f) {
            return false;
        }
    }
    return true;
}

static void raster_at(GLint x, GLint y)
{
    GLfloat wx = ((GLfloat)x - 320.0f) * 5.2f / 320.0f;
    GLfloat wy = ((GLfloat)y - 240.0f) * 5.2f / 240.0f;
    glRasterPos3f(wx, wy, 0.0f);
}

static void run_probe(void)
{
    GLfloat fb[128];
    GLuint sb[64];
    GLuint list;
    GLint count;
    uint8_t pixel[4] = { 255, 64, 32, 255 };
    uint8_t bitmap[1] = { 0x80 };
    GLdouble clip_x_positive[4] = { 1.0, 0.0, 0.0, 0.0 };
    bool ok;

    reset_state();
    count = feedback_point(-5.20f, 0.0f, 0.0f, fb);
    ok = count == 3 && nearf(fb[0], (GLfloat)GL_POINT_TOKEN, 0.01f) &&
         fb[1] >= -0.5f && fb[1] <= 1.0f && nearf(fb[2], 240.0f, 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback left boundary point", ok, 0);

    reset_state();
    count = feedback_point(-5.22f, 0.0f, 0.0f, fb);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback left outside point", ok, 1);

    reset_state();
    count = feedback_point(5.19f, 0.0f, 0.0f, fb);
    ok = count == 3 && fb[1] >= 638.0f && fb[1] <= 640.0f && consume_error(GL_NO_ERROR);
    expect_bool("feedback right boundary point", ok, 2);

    reset_state();
    count = feedback_point(0.0f, 5.19f, 0.0f, fb);
    ok = count == 3 && fb[2] >= 478.0f && fb[2] <= 480.0f && consume_error(GL_NO_ERROR);
    expect_bool("feedback top boundary point", ok, 3);

    reset_state();
    count = select_point(-5.20f, 0.0f, 0.0f, 107, sb);
    ok = count == 1 && sb[0] == 1 && sb[3] == 107 && consume_error(GL_NO_ERROR);
    expect_bool("selection boundary point hit", ok, 4);

    reset_state();
    count = select_point(5.22f, 0.0f, 0.0f, 108, sb);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection outside point miss", ok, 5);

    reset_state();
    count = feedback_line(-6.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, fb);
    ok = count == 7 && nearf(fb[0], (GLfloat)GL_LINE_TOKEN, 0.01f) &&
         fb[1] >= -1.0f && fb[1] <= 2.0f && fb[4] >= 638.0f && fb[4] <= 641.0f &&
         nearf(fb[2], 240.0f, 2.0f) && nearf(fb[5], 240.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback line spans boundaries", ok, 6);

    reset_state();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(128, GL_3D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_quad(-6.0f, -0.4f, 0.0f, 6.0f, 0.4f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = polygon_feedback_vertices_in_bounds(fb, count) && consume_error(GL_NO_ERROR);
    expect_bool("feedback polygon clipped to viewport", ok, 7);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(64, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(200);
    draw_quad(-6.0f, -0.4f, 0.0f, -5.19f, 0.4f, 0.0f);
    glLoadName(201);
    draw_quad(5.21f, -0.4f, 0.0f, 6.0f, 0.4f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 1 && sb[0] == 1 && sb[3] == 200 && consume_error(GL_NO_ERROR);
    expect_bool("selection partial boundary polygon", ok, 8);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(64, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(300);
    glPushName(301);
    draw_point(20.0f, 0.0f, 0.0f);
    glPopName();
    glLoadName(302);
    draw_point(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 1 && sb[0] == 1 && sb[3] == 302 && consume_error(GL_NO_ERROR);
    expect_bool("selection name stack miss isolation", ok, 9);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPassThrough(2.5f);
    raster_at(320, 240);
    glBitmap(1, 1, 0.0f, 0.0f, 0.0f, 0.0f, bitmap);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glCopyPixels(320, 240, 1, 1, GL_COLOR);
    draw_point(0.0f, 0.0f, 0.0f);
    glEndList();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(128, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    count = glRenderMode(GL_RENDER);
    glDeleteLists(list, 1);
    ok = count == 14 &&
         nearf(fb[0], (GLfloat)GL_PASS_THROUGH_TOKEN, 0.01f) &&
         nearf(fb[2], (GLfloat)GL_BITMAP_TOKEN, 0.01f) &&
         nearf(fb[5], (GLfloat)GL_DRAW_PIXEL_TOKEN, 0.01f) &&
         nearf(fb[8], (GLfloat)GL_COPY_PIXEL_TOKEN, 0.01f) &&
         nearf(fb[11], (GLfloat)GL_POINT_TOKEN, 0.01f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("listed pixel geometry feedback order", ok, 10);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    count = select_point(0.0f, 0.0f, 0.0f, 400, sb);
    ok = count == 1 && sb[3] == 400 && consume_error(GL_NO_ERROR);
    count = select_point(-0.01f, 0.0f, 0.0f, 401, sb);
    ok = ok && count == 0 && consume_error(GL_NO_ERROR);
    glBegin(GL_POINTS);
    glSelectBuffer(64, sb);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glFeedbackBuffer(64, GL_2D, fb);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    expect_bool("clip boundary and begin guards", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_bar(float x, bool pass)
{
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
    debugPrint("NXGL select feedback guardband probe starting\n");

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
            draw_bar(-1.45f + (float)i * 0.26f, results[i]);
        }
        nxglSwapBuffers("NXGL select/feedback guardband", all_passed() ? "all checks passed" : "select feedback guardband failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

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
    return d <= 0.02f;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPixelZoom(1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    GLfloat buffer[128];
    GLubyte rgba[4] = { 255, 64, 32, 255 };
    GLubyte bits[4] = { 0x80, 0x00, 0x00, 0x00 };
    GLint count;
    GLfloat raster[4];
    GLuint list;
    bool ok;

    reset_state();
    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    count = glRenderMode(GL_RENDER);
    ok = count == 3 && nearf(buffer[0], (GLfloat)GL_DRAW_PIXEL_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("draw pixels token gl2d", ok, 0);

    reset_state();
    glFeedbackBuffer(128, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glCopyPixels(0, 0, 1, 1, GL_COLOR);
    count = glRenderMode(GL_RENDER);
    ok = count == 4 && nearf(buffer[0], (GLfloat)GL_COPY_PIXEL_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("copy pixels token gl3d", ok, 1);

    reset_state();
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glBitmap(1, 1, 0.0f, 0.0f, 3.0f, 4.0f, bits);
    count = glRenderMode(GL_RENDER);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = count == 3 && nearf(buffer[0], (GLfloat)GL_BITMAP_TOKEN) &&
         nearf(raster[0], buffer[1] + 3.0f) && nearf(raster[1], buffer[2] + 4.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("bitmap token and move", ok, 2);

    reset_state();
    glColor4f(0.2f, 0.4f, 0.6f, 0.8f);
    glFeedbackBuffer(128, GL_3D_COLOR, buffer);
    glRenderMode(GL_FEEDBACK);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    count = glRenderMode(GL_RENDER);
    ok = count == 8 && nearf(buffer[0], (GLfloat)GL_DRAW_PIXEL_TOKEN) &&
         nearf(buffer[4], 0.2f) && nearf(buffer[5], 0.4f) && nearf(buffer[6], 0.6f) && nearf(buffer[7], 0.8f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("draw pixel color payload", ok, 3);

    reset_state();
    glColor4f(0.3f, 0.5f, 0.7f, 1.0f);
    glTexCoord3f(0.25f, 0.5f, 0.75f);
    glFeedbackBuffer(128, GL_3D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    count = glRenderMode(GL_RENDER);
    ok = count == 12 && nearf(buffer[0], (GLfloat)GL_DRAW_PIXEL_TOKEN) &&
         nearf(buffer[8], 0.25f) && nearf(buffer[9], 0.5f) && nearf(buffer[10], 0.75f) && nearf(buffer[11], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("3d color tex payload", ok, 4);

    reset_state();
    glColor4f(0.4f, 0.6f, 0.8f, 1.0f);
    glTexCoord3f(0.125f, 0.25f, 0.5f);
    glFeedbackBuffer(128, GL_4D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glBitmap(1, 1, 0.0f, 0.0f, 0.0f, 0.0f, bits);
    count = glRenderMode(GL_RENDER);
    ok = count == 13 && nearf(buffer[0], (GLfloat)GL_BITMAP_TOKEN) &&
         nearf(buffer[4], 1.0f) && nearf(buffer[9], 0.125f) && nearf(buffer[10], 0.25f) &&
         nearf(buffer[11], 0.5f) && nearf(buffer[12], 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("4d bitmap payload", ok, 5);

    reset_state();
    glFeedbackBuffer(2, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    count = glRenderMode(GL_RENDER);
    ok = count == -1 && consume_error(GL_NO_ERROR);
    expect_bool("pixel feedback overflow", ok, 6);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBitmap(1, 1, 0.0f, 0.0f, 0.0f, 0.0f, bits);
    glCopyPixels(0, 0, 1, 1, GL_COLOR);
    glEndList();
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    count = glRenderMode(GL_RENDER);
    ok = count == 9 &&
         nearf(buffer[0], (GLfloat)GL_DRAW_PIXEL_TOKEN) &&
         nearf(buffer[3], (GLfloat)GL_BITMAP_TOKEN) &&
         nearf(buffer[6], (GLfloat)GL_COPY_PIXEL_TOKEN) &&
         consume_error(GL_NO_ERROR);
    expect_bool("pixel display list replay", ok, 7);

    reset_state();
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    ok = consume_error(GL_INVALID_VALUE);
    count = glRenderMode(GL_RENDER);
    ok = ok && count == 0 && consume_error(GL_NO_ERROR);
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glCopyPixels(0, 0, 1, 1, GL_TEXTURE_2D);
    ok = consume_error(GL_INVALID_ENUM);
    count = glRenderMode(GL_RENDER);
    ok = ok && count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("draw/copy validation", ok, 8);

    reset_state();
    glBitmap(-1, 1, 0.0f, 0.0f, 0.0f, 0.0f, bits);
    ok = consume_error(GL_INVALID_VALUE);
    glBitmap(1, 1, 0.0f, 0.0f, 0.0f, 0.0f, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("bitmap validation", ok, 9);
}

static void draw_bar(float x, bool pass)
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
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL feedback pixel probe starting\n");

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
        for (int i = 0; i < 10; ++i) {
            draw_bar(-2.65f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL feedback pixels", all_passed() ? "all checks passed" : "feedback pixel check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[8];

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.03f;
}

static bool neard(GLdouble a, GLdouble b)
{
    GLdouble d = a - b;
    if (d < 0.0) d = -d;
    return d <= 0.03;
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
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    const GLubyte texel[8] = { 255, 0, 0, 255, 0, 255, 0, 255 };
    GLfloat fv[4];
    GLdouble dv[4];
    GLint iv[4];
    GLuint tex;
    bool ok;

    reset_state();
    glTexCoord3f(0.25f, 0.50f, 0.75f);
    glGetFloatv(GL_CURRENT_TEXTURE_COORDS, fv);
    ok = nearf(fv[0], 0.25f) && nearf(fv[1], 0.50f) && nearf(fv[2], 0.75f) && nearf(fv[3], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("current texture coords query", ok, 0);

    reset_state();
    glColor4f(0.20f, 0.40f, 0.60f, 0.80f);
    glTexCoord3f(0.10f, 0.30f, 0.50f);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_COLOR, fv);
    glGetDoublev(GL_CURRENT_RASTER_TEXTURE_COORDS, dv);
    ok = nearf(fv[0], 0.20f) && nearf(fv[3], 0.80f) && neard(dv[0], 0.10) && neard(dv[1], 0.30) &&
         neard(dv[2], 0.50) && neard(dv[3], 1.0) && consume_error(GL_NO_ERROR);
    expect_bool("current raster payload queries", ok, 1);

    reset_state();
    glDepthRange(0.25f, 0.75f);
    glGetFloatv(GL_DEPTH_RANGE, fv);
    glGetBooleanv(GL_DEPTH_RANGE, (GLboolean *)iv);
    ok = nearf(fv[0], 0.25f) && nearf(fv[1], 0.75f) &&
         ((GLboolean *)iv)[0] == GL_TRUE && ((GLboolean *)iv)[1] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("depth range get conversions", ok, 2);

    reset_state();
    glLightfv(GL_LIGHT0, GL_POSITION, (const GLfloat[]){ 1.0f, 2.0f, 3.0f, 0.0f });
    glGetLightfv(GL_LIGHT0, GL_POSITION, fv);
    glGetLightiv(GL_LIGHT0, GL_POSITION, iv);
    ok = nearf(fv[0], 1.0f) && nearf(fv[2], 3.0f) && iv[1] == 2 && iv[3] == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("light get wrappers", ok, 3);

    reset_state();
    glMaterialfv(GL_FRONT, GL_SPECULAR, (const GLfloat[]){ 0.1f, 0.2f, 0.3f, 0.4f });
    glMateriali(GL_FRONT, GL_SHININESS, 48);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, fv);
    glGetMaterialiv(GL_FRONT, GL_SHININESS, iv);
    ok = nearf(fv[0], 0.1f) && nearf(fv[3], 0.4f) && iv[0] == 48 && consume_error(GL_NO_ERROR);
    expect_bool("material get wrappers", ok, 4);

    reset_state();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_1D, tex);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel);
    glGetTexLevelParameterfv(GL_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, fv);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPONENTS, iv);
    ok = nearf(fv[0], 2.0f) && iv[0] == (GLint)GL_RGBA && consume_error(GL_NO_ERROR);
    expect_bool("tex level get wrappers", ok, 5);

    reset_state();
    glClearColor(0.3f, 0.4f, 0.5f, 0.6f);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, fv);
    glGetIntegerv(GL_CURRENT_INDEX, iv);
    ok = nearf(fv[0], 0.3f) && nearf(fv[3], 0.6f) && iv[0] == 0 && consume_error(GL_NO_ERROR);
    expect_bool("legacy get aliases", ok, 6);

    glGetLightfv(GL_TEXTURE_2D, GL_POSITION, fv);
    ok = consume_error(GL_INVALID_ENUM);
    glGetMaterialfv(GL_TEXTURE_2D, GL_AMBIENT, fv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetTexLevelParameterfv(GL_TEXTURE_1D, -1, GL_TEXTURE_WIDTH, fv);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("get validation", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
    glRectf(x - 0.17f, -1.42f, x + 0.17f, -1.68f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL get surface probe starting\n");

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
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL get surface", all_passed() ? "all checks passed" : "get check failed");
        Sleep(16);
    }
}

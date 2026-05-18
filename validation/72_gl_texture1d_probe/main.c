#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool rgba_eq(const GLubyte *p, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    return p[0] == r && p[1] == g && p[2] == b && p[3] == a;
}

static void reset_state(void)
{
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    const GLubyte source[16] = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 0, 255
    };
    const GLubyte patch[8] = {
        20, 30, 40, 255,
        50, 60, 70, 255
    };
    const GLubyte skipped[20] = {
        1, 2, 3, 4,
        8, 9, 10, 255,
        11, 12, 13, 255,
        14, 15, 16, 255,
        17, 18, 19, 255
    };
    GLubyte out[32];
    GLint iv[4];
    GLuint tex;
    GLuint list;
    bool ok;

    reset_state();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_1D, tex);
    glEnable(GL_TEXTURE_1D);
    glGetIntegerv(GL_TEXTURE_BINDING_1D, iv);
    ok = tex != 0 && iv[0] == (GLint)tex && glIsEnabled(GL_TEXTURE_1D) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("texture1d binding state", ok, 0);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, &iv[0]);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_HEIGHT, &iv[1]);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_DEPTH, &iv[2]);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_INTERNAL_FORMAT, &iv[3]);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = iv[0] == 4 && iv[1] == 1 && iv[2] == 1 && iv[3] == (GLint)GL_RGBA &&
         memcmp(out, source, sizeof(source)) == 0 && consume_error(GL_NO_ERROR);
    expect_bool("texture1d upload query readback", ok, 1);

    glTexSubImage1D(GL_TEXTURE_1D, 0, 1, 2, GL_RGBA, GL_UNSIGNED_BYTE, patch);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = rgba_eq(out, 255, 0, 0, 255) &&
         rgba_eq(out + 4, 20, 30, 40, 255) &&
         rgba_eq(out + 8, 50, 60, 70, 255) &&
         rgba_eq(out + 12, 255, 255, 0, 255) &&
         consume_error(GL_NO_ERROR);
    expect_bool("texture1d subimage readback", ok, 2);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 5);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, skipped);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = rgba_eq(out, 8, 9, 10, 255) &&
         rgba_eq(out + 4, 11, 12, 13, 255) &&
         rgba_eq(out + 8, 14, 15, 16, 255) &&
         consume_error(GL_NO_ERROR);
    expect_bool("texture1d unpack stride", ok, 3);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    glTexImage1D(GL_TEXTURE_1D, 1, GL_RGBA, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    glTexImage1D(GL_TEXTURE_1D, 2, GL_RGBA, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glGetTexParameteriv(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, iv);
    ok = iv[0] == (GLint)GL_LINEAR_MIPMAP_NEAREST && consume_error(GL_NO_ERROR);
    expect_bool("texture1d mip chain state", ok, 4);

    glClearColor(0.25f, 0.50f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glCopyTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 0, 0, 4, 0);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = rgba_eq(out, 63, 127, 191, 255) && rgba_eq(out + 12, 63, 127, 191, 255) && consume_error(GL_NO_ERROR);
    expect_bool("copy tex image 1d", ok, 5);

    glClearColor(0.90f, 0.10f, 0.20f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glCopyTexSubImage1D(GL_TEXTURE_1D, 0, 1, 0, 0, 2);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = rgba_eq(out, 63, 127, 191, 255) &&
         rgba_eq(out + 4, 229, 25, 51, 255) &&
         rgba_eq(out + 8, 229, 25, 51, 255) &&
         consume_error(GL_NO_ERROR);
    expect_bool("copy tex subimage 1d", ok, 6);

    reset_state();
    glBindTexture(GL_TEXTURE_1D, tex);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_TEXTURE_1D);
    glEndList();
    glCallList(list);
    ok = glIsEnabled(GL_TEXTURE_1D) == GL_TRUE && consume_error(GL_NO_ERROR);
    glTexImage1D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glTexSubImage1D(GL_TEXTURE_2D, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, source);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("texture1d list and validation", ok, 7);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.2f, -1.42f, 0.0f);
    glVertex3f(x + 0.2f, -1.42f, 0.0f);
    glVertex3f(x + 0.2f, -1.68f, 0.0f);
    glVertex3f(x - 0.2f, -1.68f, 0.0f);
    glEnd();
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
    debugPrint("NXGL texture1D probe starting\n");

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

        glColor3f(0.2f, 0.7f, 0.9f);
        glBegin(GL_QUADS);
        glVertex3f(-2.30f, 0.86f, 0.0f);
        glVertex3f(2.30f, 0.86f, 0.0f);
        glVertex3f(2.30f, -0.26f, 0.0f);
        glVertex3f(-2.30f, -0.26f, 0.0f);
        glEnd();

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.1f + (float)i * 0.6f, results[i]);
        }

        nxglSwapBuffers("NXGL texture1D", all_passed() ? "texture1D checks passed" : "one or more texture1D checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

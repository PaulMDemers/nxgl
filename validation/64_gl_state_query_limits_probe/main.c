#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
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
    return d <= 0.001f;
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
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    GLint iv[8] = { 0 };
    GLfloat fv[8] = { 0.0f };
    GLdouble dv[8] = { 0.0 };
    GLboolean bv[8] = { GL_FALSE };
    bool ok;

    glGetFloatv(GL_POINT_SIZE_RANGE, fv);
    glGetFloatv(GL_POINT_SIZE_GRANULARITY, &fv[2]);
    glGetDoublev(GL_LINE_WIDTH_RANGE, dv);
    glGetDoublev(GL_LINE_WIDTH_GRANULARITY, &dv[2]);
    ok = nearf(fv[0], 1.0f) && nearf(fv[1], 64.0f) && nearf(fv[2], 1.0f) &&
         dv[0] == 1.0 && dv[1] == 64.0 && dv[2] == 1.0 && consume_error(GL_NO_ERROR);
    expect_bool("smooth size width ranges", ok, 0);

    memset(iv, 0, sizeof(iv));
    glGetIntegerv(GL_ALIASED_POINT_SIZE_RANGE, iv);
    glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, &iv[2]);
    glGetBooleanv(GL_ALIASED_LINE_WIDTH_RANGE, bv);
    ok = iv[0] == 1 && iv[1] == 64 && iv[2] == 1 && iv[3] == 64 &&
         bv[0] == GL_TRUE && bv[1] == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("aliased size width ranges", ok, 1);

    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &iv[0]);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &iv[1]);
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &iv[2]);
    glGetIntegerv(GL_MAX_NAME_STACK_DEPTH, &iv[3]);
    glGetIntegerv(GL_MAX_LIST_NESTING, &iv[4]);
    ok = iv[0] == 32 && iv[1] == 8 && iv[2] == 8 && iv[3] == 64 && iv[4] == 64 && consume_error(GL_NO_ERROR);
    expect_bool("stack limit queries", ok, 2);

    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &iv[0]);
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, &iv[1]);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &iv[2]);
    glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &iv[3]);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 16 && iv[3] == 16 && consume_error(GL_NO_ERROR);
    expect_bool("attrib stack limits", ok, 3);

    glGetIntegerv(GL_MAX_LIGHTS, &iv[0]);
    glGetIntegerv(GL_MAX_CLIP_PLANES, &iv[1]);
    glGetIntegerv(GL_MAX_EVAL_ORDER, &iv[2]);
    glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &iv[3]);
    glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &iv[4]);
    glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &iv[5]);
    ok = iv[0] == 8 && iv[1] == 6 && iv[2] == 8 && iv[3] == 256 && iv[4] == 1024 && iv[5] == 1024 &&
         consume_error(GL_NO_ERROR);
    expect_bool("misc implementation limits", ok, 4);

    glGetIntegerv(GL_INDEX_BITS, &iv[0]);
    glGetIntegerv(GL_ACCUM_RED_BITS, &iv[1]);
    glGetIntegerv(GL_ACCUM_GREEN_BITS, &iv[2]);
    glGetIntegerv(GL_ACCUM_BLUE_BITS, &iv[3]);
    glGetIntegerv(GL_ACCUM_ALPHA_BITS, &iv[4]);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 0 && iv[3] == 0 && iv[4] == 0 && consume_error(GL_NO_ERROR);
    expect_bool("index accum bit queries", ok, 5);

    glGetIntegerv(GL_PACK_SWAP_BYTES, &iv[0]);
    glGetIntegerv(GL_PACK_LSB_FIRST, &iv[1]);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &iv[2]);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &iv[3]);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &iv[4]);
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &iv[5]);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 0 && iv[3] == 0 && iv[4] == 0 && iv[5] == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("pack defaults", ok, 6);

    glGetIntegerv(GL_UNPACK_LSB_FIRST, &iv[0]);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &iv[1]);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &iv[2]);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &iv[3]);
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glGetIntegerv(GL_PACK_ALIGNMENT, &iv[4]);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &iv[5]);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 0 && iv[3] == 0 && iv[4] == 8 && iv[5] == 2 &&
         consume_error(GL_NO_ERROR);
    expect_bool("unpack defaults alignment", ok, 7);

    memset(iv, 0, sizeof(iv));
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, iv);
    glGetDoublev(GL_COMPRESSED_TEXTURE_FORMATS, dv);
    ok = iv[0] == (GLint)GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
         iv[1] == (GLint)GL_COMPRESSED_RGBA_S3TC_DXT1_EXT &&
         iv[2] == (GLint)GL_COMPRESSED_RGBA_S3TC_DXT3_EXT &&
         iv[3] == (GLint)GL_COMPRESSED_RGBA_S3TC_DXT5_EXT &&
         dv[3] == (GLdouble)GL_COMPRESSED_RGBA_S3TC_DXT5_EXT && consume_error(GL_NO_ERROR);
    expect_bool("compressed format vector", ok, 8);

    glDisable(GL_MULTISAMPLE);
    glGetIntegerv(GL_MULTISAMPLE, &iv[0]);
    glGetBooleanv(GL_MULTISAMPLE, &bv[0]);
    ok = iv[0] == GL_FALSE && bv[0] == GL_FALSE && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_TEXTURE_ENV, fv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetIntegerv(GL_POINT_SIZE_RANGE, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("long query validation", ok, 9);
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
    debugPrint("NXGL state query limits probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    reset_state();
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 10; ++i) {
            draw_bar(-2.65f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL state query limits", all_passed() ? "all checks passed" : "state query limits failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

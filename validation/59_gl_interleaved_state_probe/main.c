#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct C4ubV3f {
    uint8_t c[4];
    GLfloat v[3];
} C4ubV3f;

typedef struct T2fC4ubV3f {
    GLfloat t[2];
    uint8_t c[4];
    GLfloat v[3];
} T2fC4ubV3f;

typedef struct C4fN3fV3f {
    GLfloat c[4];
    GLfloat n[3];
    GLfloat v[3];
} C4fN3fV3f;

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

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b) && p[3] == 255;
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
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    GLint iv[4] = { 0, 0, 0, 0 };
    GLboolean edge = GL_FALSE;
    GLvoid *ptr = NULL;
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    glGetIntegerv(GL_CULL_FACE_MODE, &iv[0]);
    glGetIntegerv(GL_FRONT_FACE, &iv[1]);
    glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, &iv[2]);
    glGetBooleanv(GL_EDGE_FLAG, &edge);
    ok = iv[0] == (GLint)GL_BACK && iv[1] == (GLint)GL_CCW &&
         iv[2] == (GLint)GL_DONT_CARE && edge == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("default cull hint edge", ok, 0);

    glCullFace(GL_FRONT_AND_BACK);
    glFrontFace(GL_CW);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[0]);
    glGetIntegerv(GL_FRONT_FACE, &iv[1]);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[2]);
    glGetIntegerv(GL_FRONT_FACE, &iv[3]);
    ok = iv[0] == (GLint)GL_FRONT_AND_BACK && iv[1] == (GLint)GL_CW &&
         iv[2] == (GLint)GL_FRONT && iv[3] == (GLint)GL_CCW && consume_error(GL_NO_ERROR);
    expect_bool("cull front list", ok, 1);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_FOG_HINT, GL_FASTEST);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, &iv[0]);
    glGetIntegerv(GL_FOG_HINT, &iv[1]);
    glGetIntegerv(GL_LINE_SMOOTH_HINT, &iv[2]);
    glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &iv[3]);
    ok = iv[0] == (GLint)GL_NICEST && iv[1] == (GLint)GL_FASTEST &&
         iv[2] == (GLint)GL_NICEST && iv[3] == (GLint)GL_FASTEST && consume_error(GL_NO_ERROR);
    expect_bool("hint state list", ok, 2);

    edge = GL_FALSE;
    glEdgeFlag(GL_FALSE);
    glGetBooleanv(GL_EDGE_FLAG, &edge);
    ok = edge == GL_FALSE;
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEdgeFlag(GL_TRUE);
    glEndList();
    glCallList(list);
    glGetBooleanv(GL_EDGE_FLAG, &edge);
    ok = ok && edge == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("edge flag state list", ok, 3);

    {
        GLfloat verts[4][3] = {
            { -0.6f, 0.6f, 0.0f }, { 0.6f, 0.6f, 0.0f },
            { 0.6f, -0.6f, 0.0f }, { -0.6f, -0.6f, 0.0f }
        };
        glInterleavedArrays(GL_V3F, 0, verts);
        glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &iv[0]);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &iv[1]);
        glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
        ok = iv[0] == 3 && iv[1] == 12 && ptr == (GLvoid *)verts && glIsEnabled(GL_VERTEX_ARRAY) == GL_TRUE &&
             glIsEnabled(GL_COLOR_ARRAY) == GL_FALSE && consume_error(GL_NO_ERROR);
    }
    expect_bool("interleaved v3f pointers", ok, 4);

    {
        C4ubV3f quad[4] = {
            { { 20, 220, 40, 255 }, { -0.55f, 0.55f, 0.0f } },
            { { 20, 220, 40, 255 }, { 0.55f, 0.55f, 0.0f } },
            { { 20, 220, 40, 255 }, { 0.55f, -0.55f, 0.0f } },
            { { 20, 220, 40, 255 }, { -0.55f, -0.55f, 0.0f } }
        };
        reset_state();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glInterleavedArrays(GL_C4UB_V3F, 0, quad);
        glGetIntegerv(GL_COLOR_ARRAY_TYPE, &iv[0]);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &iv[1]);
        glDrawArrays(GL_QUADS, 0, 4);
        read_center(pixel);
        ok = iv[0] == (GLint)GL_UNSIGNED_BYTE && iv[1] == (GLint)sizeof(C4ubV3f) &&
             pixel_rgb(pixel, 20, 220, 40) && consume_error(GL_NO_ERROR);
    }
    expect_bool("c4ub v3f draw", ok, 5);

    {
        T2fC4ubV3f quad[1];
        glInterleavedArrays(GL_T2F_C4UB_V3F, 0, quad);
        glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &ptr);
        ok = ptr == (GLvoid *)quad;
        glGetPointerv(GL_COLOR_ARRAY_POINTER, &ptr);
        ok = ok && ptr == (GLvoid *)((uint8_t *)quad + 8);
        glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
        ok = ok && ptr == (GLvoid *)((uint8_t *)quad + 12);
        glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &iv[0]);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &iv[1]);
        ok = ok && iv[0] == 2 && iv[1] == (GLint)sizeof(T2fC4ubV3f) && consume_error(GL_NO_ERROR);
    }
    expect_bool("t2f c4ub offsets", ok, 6);

    {
        C4fN3fV3f quad[1];
        glInterleavedArrays(GL_C4F_N3F_V3F, 0, quad);
        glGetPointerv(GL_COLOR_ARRAY_POINTER, &ptr);
        ok = ptr == (GLvoid *)quad;
        glGetPointerv(GL_NORMAL_ARRAY_POINTER, &ptr);
        ok = ok && ptr == (GLvoid *)((uint8_t *)quad + 16);
        glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
        ok = ok && ptr == (GLvoid *)((uint8_t *)quad + 28);
        glGetIntegerv(GL_NORMAL_ARRAY_TYPE, &iv[0]);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &iv[1]);
        ok = ok && iv[0] == (GLint)GL_FLOAT && iv[1] == (GLint)sizeof(C4fN3fV3f) && consume_error(GL_NO_ERROR);
    }
    expect_bool("c4f n3f offsets", ok, 7);

    {
        uint8_t padded[4 * 32];
        glInterleavedArrays(GL_C4UB_V3F, 32, padded);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &iv[0]);
        glGetIntegerv(GL_COLOR_ARRAY_STRIDE, &iv[1]);
        glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
        ok = iv[0] == 32 && iv[1] == 32 && ptr == (GLvoid *)(padded + 4) && consume_error(GL_NO_ERROR);
    }
    expect_bool("explicit interleaved stride", ok, 8);

    glCullFace(GL_TEXTURE_2D);
    ok = consume_error(GL_INVALID_ENUM);
    glFrontFace(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glHint(GL_TEXTURE_2D, GL_FASTEST);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glHint(GL_FOG_HINT, GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glInterleavedArrays(GL_TEXTURE_2D, 0, NULL);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glInterleavedArrays(GL_V3F, -1, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glEdgeFlagv(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("validation", ok, 9);
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
    debugPrint("NXGL interleaved/state probe starting\n");

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
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        for (int i = 0; i < 10; ++i) {
            draw_bar(-2.65f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL interleaved state", all_passed() ? "all checks passed" : "interleaved/state check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

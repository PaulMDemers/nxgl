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
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static GLint feedback_point(GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 64u);
    glFeedbackBuffer(64, GL_3D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex4i(2, 2, 0, 2);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static void run_probe(void)
{
    GLfloat fv[64];
    GLfloat matrix[16];
    GLdouble dv[4] = { 0.0, 0.0, 0.0, 1.0 };
    GLshort sv[4] = { 2, 4, 6, 2 };
    GLint iv[4] = { 0, 1, 0, 1 };
    GLuint list;
    GLint count;
    bool ok;

    reset_state();
    glColor4ub(64, 128, 192, 255);
    glGetFloatv(GL_CURRENT_COLOR, fv);
    ok = nearf(fv[0], 64.0f / 255.0f) && nearf(fv[1], 128.0f / 255.0f) &&
         nearf(fv[2], 192.0f / 255.0f) && nearf(fv[3], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("color unsigned-byte wrapper", ok, 0);

    reset_state();
    glNormal3iv((const GLint[]){ 0, 0, 4 });
    glGetFloatv(GL_CURRENT_NORMAL, fv);
    ok = nearf(fv[0], 0.0f) && nearf(fv[1], 0.0f) && nearf(fv[2], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("normal integer-vector wrapper", ok, 1);

    reset_state();
    glColor3dv((const GLdouble[]){ 0.25, 0.50, 0.75 });
    glTexCoord4dv((const GLdouble[]){ 2.0, 4.0, 6.0, 2.0 });
    count = feedback_point(fv);
    ok = count == 12 && nearf(fv[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(fv[4], 0.25f) && nearf(fv[5], 0.50f) && nearf(fv[6], 0.75f) && nearf(fv[7], 1.0f) &&
         nearf(fv[8], 1.0f) && nearf(fv[9], 2.0f) && nearf(fv[10], 3.0f) && nearf(fv[11], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("texcoord and vertex wrappers", ok, 2);

    reset_state();
    memset(fv, 0, sizeof(fv));
    glFeedbackBuffer(64, GL_3D, fv);
    glRenderMode(GL_FEEDBACK);
    glRecti(-1, -1, 1, 1);
    count = glRenderMode(GL_RENDER);
    ok = count == 14 && nearf(fv[0], (GLfloat)GL_POLYGON_TOKEN) && nearf(fv[1], 4.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("rect wrapper emits quad", ok, 3);

    reset_state();
    glRasterPos4d(0.0, 0.0, 0.0, 2.0);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, (GLboolean *)iv);
    ok = ((GLboolean *)iv)[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("rasterpos typed wrappers", ok, 4);

    reset_state();
    glTranslated(1.0, 2.0, 3.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    ok = nearf(matrix[12], 1.0f) && nearf(matrix[13], 2.0f) && nearf(matrix[14], 3.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("double transform wrapper", ok, 5);

    reset_state();
    glLightiv(GL_LIGHT0, GL_POSITION, (const GLint[]){ 0, 0, 1, 0 });
    glMateriali(GL_FRONT, GL_SHININESS, 32);
    iv[0] = 0;
    iv[1] = 1;
    iv[2] = 0;
    iv[3] = 1;
    glFogiv(GL_FOG_COLOR, iv);
    glGetFloatv(GL_FOG_COLOR, fv);
    ok = nearf(fv[0], 0.0f) && nearf(fv[1], 1.0f) && nearf(fv[2], 0.0f) && nearf(fv[3], 1.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("light material fog integer wrappers", ok, 6);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glColor4us(65535, 0, 0, 32768);
    glTexCoord4sv(sv);
    glBegin(GL_POINTS);
    glVertex2dv(dv);
    glEnd();
    glEndList();
    memset(fv, 0, sizeof(fv));
    glFeedbackBuffer(64, GL_3D_COLOR_TEXTURE, fv);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    count = glRenderMode(GL_RENDER);
    ok = count == 12 && nearf(fv[4], 1.0f) && nearf(fv[5], 0.0f) && nearf(fv[6], 0.0f) &&
         nearf(fv[7], 32768.0f / 65535.0f) && nearf(fv[8], 1.0f) && nearf(fv[9], 2.0f) &&
         nearf(fv[10], 3.0f) && consume_error(GL_NO_ERROR);
    expect_bool("display-list wrapper replay", ok, 7);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.16f : 0.90f, pass ? 0.78f : 0.14f, pass ? 0.42f : 0.16f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -1.45f, 0.0f);
    glVertex3f(x + 0.18f, -1.45f, 0.0f);
    glVertex3f(x + 0.18f, -1.72f, 0.0f);
    glVertex3f(x - 0.18f, -1.72f, 0.0f);
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
    debugPrint("NXGL API wrapper probe starting\n");

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

        glColor3f(0.18f, 0.34f, 0.92f);
        glRectf(-2.2f, 0.85f, -0.18f, -0.32f);
        glColor3f(0.95f, 0.58f, 0.16f);
        glRectd(0.18, 0.85, 2.20, -0.32);

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.1f + (float)i * 0.6f, results[i]);
        }

        nxglSwapBuffers("NXGL API wrappers", all_passed() ? "typed wrappers passed" : "one or more wrapper checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[10];

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
    for (int unit = 0; unit < 2; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_Q);
        glTexCoord2f(0.0f, 0.0f);
    }
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static GLint feedback_point(GLfloat x, GLfloat y, GLfloat z, GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 64u);
    glFeedbackBuffer(64, GL_3D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static void run_probe(void)
{
    GLfloat s_plane[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    GLfloat t_plane[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat r_plane[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    GLfloat q_plane[4] = { 0.0f, 0.0f, 0.0f, 2.0f };
    GLfloat buffer[64];
    GLfloat fv[4];
    GLint iv[4];
    GLuint list;
    GLint count;
    bool ok;

    reset_state();
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, s_plane);
    glEnable(GL_TEXTURE_GEN_S);
    glGetTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, iv);
    glGetTexGenfv(GL_S, GL_OBJECT_PLANE, fv);
    ok = iv[0] == (GLint)GL_OBJECT_LINEAR && nearf(fv[0], 1.0f) && nearf(fv[1], 0.0f) &&
         glIsEnabled(GL_TEXTURE_GEN_S) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("texgen object state", ok, 0);

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, t_plane);
    glEnable(GL_TEXTURE_GEN_T);
    count = feedback_point(0.25f, 0.75f, 0.0f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.25f) && nearf(buffer[9], 0.75f) && consume_error(GL_NO_ERROR);
    expect_bool("object linear feedback", ok, 1);

    reset_state();
    glTranslatef(0.10f, 0.20f, 0.0f);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    count = feedback_point(0.25f, 0.25f, 0.0f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.35f) && nearf(buffer[9], 0.45f) && consume_error(GL_NO_ERROR);
    expect_bool("eye linear feedback", ok, 2);

    reset_state();
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, s_plane);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, t_plane);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_R, GL_OBJECT_PLANE, r_plane);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_Q, GL_OBJECT_PLANE, q_plane);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    count = feedback_point(0.50f, 0.25f, 0.75f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.25f) && nearf(buffer[9], 0.125f) &&
         nearf(buffer[10], 0.375f) && nearf(buffer[11], 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("object linear q divide", ok, 3);

    reset_state();
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glNormal3f(0.0f, 0.0f, 1.0f);
    count = feedback_point(0.0f, 0.0f, 0.0f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.5f) && nearf(buffer[9], 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("sphere map feedback", ok, 4);

    reset_state();
    glActiveTexture(GL_TEXTURE1);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, s_plane);
    glEnable(GL_TEXTURE_GEN_S);
    ok = glIsEnabled(GL_TEXTURE_GEN_S) == GL_TRUE && consume_error(GL_NO_ERROR);
    glActiveTexture(GL_TEXTURE0);
    ok = ok && glIsEnabled(GL_TEXTURE_GEN_S) == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("per unit texgen state", ok, 5);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, s_plane);
    glEnable(GL_TEXTURE_GEN_S);
    glEndList();
    glCallList(list);
    count = feedback_point(0.66f, 0.0f, 0.0f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.66f) && consume_error(GL_NO_ERROR);
    expect_bool("display list texgen state", ok, 6);

    reset_state();
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    ok = consume_error(GL_INVALID_ENUM);
    glTexGeni(GL_TEXTURE_2D, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glTexGeni(GL_S, GL_TEXTURE_ENV_MODE, GL_OBJECT_LINEAR);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("texgen validation", ok, 7);

    reset_state();
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeniv(GL_S, GL_OBJECT_PLANE, (GLint[]){ 0, 1, 0, 0 });
    glGetTexGeniv(GL_S, GL_OBJECT_PLANE, iv);
    ok = iv[0] == 0 && iv[1] == 1 && iv[2] == 0 && iv[3] == 0 && consume_error(GL_NO_ERROR);
    expect_bool("texgen integer plane", ok, 8);

    reset_state();
    count = feedback_point(0.40f, 0.60f, 0.0f, buffer);
    ok = count == 12 && nearf(buffer[8], 0.0f) && nearf(buffer[9], 0.0f) && consume_error(GL_NO_ERROR);
    expect_bool("texgen disabled leaves coords", ok, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.42f, 0.0f);
    glVertex3f(x + 0.16f, -1.42f, 0.0f);
    glVertex3f(x + 0.16f, -1.67f, 0.0f);
    glVertex3f(x - 0.16f, -1.67f, 0.0f);
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
    debugPrint("NXGL texgen probe starting\n");

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

        glColor3f(0.85f, 0.18f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(-2.35f, 0.92f, 0.0f);
        glVertex3f(-0.85f, 0.92f, 0.0f);
        glVertex3f(-0.85f, -0.20f, 0.0f);
        glVertex3f(-2.35f, -0.20f, 0.0f);
        glEnd();

        glColor3f(0.10f, 0.64f, 0.95f);
        glBegin(GL_QUADS);
        glVertex3f(-0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, -0.42f, 0.0f);
        glVertex3f(-0.55f, -0.42f, 0.0f);
        glEnd();

        glColor3f(0.96f, 0.80f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(0.85f, 0.92f, 0.0f);
        glVertex3f(2.35f, 0.92f, 0.0f);
        glVertex3f(2.35f, -0.20f, 0.0f);
        glVertex3f(0.85f, -0.20f, 0.0f);
        glEnd();

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL texgen", all_passed() ? "texgen checks passed" : "one or more texgen checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

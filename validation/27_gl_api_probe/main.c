#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static int pass_count;
static int fail_count;

static void expect_bool(const char *name, bool condition)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    if (condition) {
        ++pass_count;
    } else {
        ++fail_count;
    }
}

static void draw_bar(float y, bool pass)
{
    if (pass) {
        glColor3f(0.1f, 0.8f, 0.2f);
    } else {
        glColor3f(0.9f, 0.1f, 0.1f);
    }
    glBegin(GL_QUADS);
    glVertex3f(-2.8f, y + 0.10f, 0.0f);
    glVertex3f( 2.8f, y + 0.10f, 0.0f);
    glVertex3f( 2.8f, y - 0.10f, 0.0f);
    glVertex3f(-2.8f, y - 0.10f, 0.0f);
    glEnd();
}

static void run_probe(bool results[12])
{
    GLint value = 0;
    GLfloat fvalue = 0.0f;
    GLboolean bvalue = GL_FALSE;
    GLint viewport[4] = { 0, 0, 0, 0 };
    GLfloat matrix[16];
    GLuint tex[2] = { 0, 0 };
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);

    results[0] = vendor != NULL && renderer != NULL && version != NULL && glGetError() == GL_NO_ERROR;
    expect_bool("glGetString core strings", results[0]);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glTranslatef(1.0f, 2.0f, 3.0f);
    glGetFloatv(GL_PROJECTION_MATRIX, matrix);
    results[1] = matrix[12] == 1.0f && matrix[13] == 2.0f && matrix[14] == 3.0f;
    expect_bool("projection matrix stack transform", results[1]);

    glPopMatrix();
    glGetFloatv(GL_PROJECTION_MATRIX, matrix);
    results[2] = matrix[12] == 0.0f && matrix[13] == 0.0f && matrix[14] == 0.0f;
    expect_bool("projection matrix pop restores", results[2]);

    glViewport(11, 22, 333, 244);
    glGetIntegerv(GL_VIEWPORT, viewport);
    results[3] = viewport[0] == 11 && viewport[1] == 22 && viewport[2] == 333 && viewport[3] == 244;
    expect_bool("viewport query", results[3]);

    glEnable(GL_BLEND);
    results[4] = glIsEnabled(GL_BLEND) == GL_TRUE;
    expect_bool("blend enable query", results[4]);

    glDisable(GL_DEPTH_TEST);
    results[5] = glIsEnabled(GL_DEPTH_TEST) == GL_FALSE;
    expect_bool("depth disable query", results[5]);
    glEnable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &value);
    results[6] = value == GL_TEXTURE1;
    expect_bool("active texture query", results[6]);

    glClientActiveTexture(GL_TEXTURE2);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &value);
    results[7] = value == GL_TEXTURE2;
    expect_bool("client active texture query", results[7]);

    glGenTextures(2, tex);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
    results[8] = tex[0] != 0 && tex[1] != 0 && tex[0] != tex[1] && value == (GLint)tex[0];
    expect_bool("texture name allocation and bind", results[8]);

    glEnable(0xffffffffu);
    results[9] = glGetError() == GL_INVALID_ENUM && glGetError() == GL_NO_ERROR;
    expect_bool("error latch and clear", results[9]);

    results[10] = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
    glDisable(GL_MULTISAMPLE);
    results[10] = results[10] && glIsEnabled(GL_MULTISAMPLE) == GL_FALSE && glGetError() == GL_NO_ERROR;
    glEnable(GL_SAMPLE_COVERAGE);
    results[10] = results[10] && glIsEnabled(GL_SAMPLE_COVERAGE) == GL_TRUE && glGetError() == GL_NO_ERROR;
    expect_bool("multisample enables", results[10]);

    glSampleCoverage(-0.5f, GL_TRUE);
    glGetFloatv(GL_SAMPLE_COVERAGE_VALUE, &fvalue);
    results[11] = fvalue == 0.0f && glGetError() == GL_NO_ERROR;
    glGetBooleanv(GL_SAMPLE_COVERAGE_INVERT, &bvalue);
    results[11] = results[11] && bvalue == GL_TRUE && glGetError() == GL_NO_ERROR;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &value);
    results[11] = results[11] && value == 0 && glGetError() == GL_NO_ERROR;
    glGetIntegerv(GL_SAMPLES, &value);
    results[11] = results[11] && value == 0 && glGetError() == GL_NO_ERROR;
    expect_bool("sample coverage state", results[11]);

    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(2, tex);
}

int main(void)
{
    bool results[12];

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL API probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe(results);

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -6.0f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        for (int i = 0; i < 12; ++i) {
            draw_bar(2.35f - (float)i * 0.38f, results[i]);
        }

        nxglSwapBuffers("NXGL API probe", fail_count == 0 ? "all state/error checks passed" : "one or more checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static GLuint base_list;
static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void compile_quad_list(GLuint list, float r, float g, float b)
{
    glNewList(list, GL_COMPILE);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-0.45f,  0.45f, 0.0f);
    glVertex3f( 0.45f,  0.45f, 0.0f);
    glVertex3f( 0.45f, -0.45f, 0.0f);
    glVertex3f(-0.45f, -0.45f, 0.0f);
    glEnd();
    glEndList();
}

static void compile_transformed_list(GLuint list, float x, float y, float r, float g, float b)
{
    glNewList(list, GL_COMPILE);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-0.35f,  0.35f, 0.0f);
    glVertex3f( 0.35f,  0.35f, 0.0f);
    glVertex3f( 0.35f, -0.35f, 0.0f);
    glVertex3f(-0.35f, -0.35f, 0.0f);
    glEnd();
    glPopMatrix();
    glEndList();
}

static void run_static_probe(void)
{
    GLint value = 0;
    GLuint tmp;

    base_list = glGenLists(4);
    expect_bool("glGenLists returned contiguous range", base_list != 0, 0);

    compile_quad_list(base_list + 0, 0.9f, 0.1f, 0.1f);
    compile_quad_list(base_list + 1, 0.1f, 0.8f, 0.2f);
    compile_transformed_list(base_list + 2, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f);
    expect_bool("glNewList/glEndList compile", glGetError() == GL_NO_ERROR, 1);

    expect_bool("glIsList true for compiled list", glIsList(base_list + 1) == GL_TRUE, 2);

    glListBase(base_list);
    glGetIntegerv(GL_LIST_BASE, &value);
    expect_bool("glListBase query", value == (GLint)base_list && glGetError() == GL_NO_ERROR, 3);

    tmp = glGenLists(1);
    glNewList(tmp, GL_COMPILE_AND_EXECUTE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glEndList();
    expect_bool("GL_COMPILE_AND_EXECUTE accepts state list", tmp != 0 && glGetError() == GL_NO_ERROR, 4);

    glDeleteLists(tmp, 1);
    expect_bool("glDeleteLists clears list", glIsList(tmp) == GL_FALSE, 5);

    glCallList(250);
    expect_bool("missing list reports invalid value", glGetError() == GL_INVALID_VALUE, 6);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -2.05f, 0.0f);
    glVertex3f(x + 0.18f, -2.05f, 0.0f);
    glVertex3f(x + 0.18f, -2.35f, 0.0f);
    glVertex3f(x - 0.18f, -2.35f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

static void draw_scene(void)
{
    GLubyte ids[] = { 0, 1 };

    glPushMatrix();
    glTranslatef(-1.8f, 0.7f, 0.0f);
    glCallList(base_list + 0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.7f, 0.0f);
    glCallList(base_list + 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.8f, 0.7f, 0.0f);
    glCallList(base_list + 2);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.9f, -0.55f, 0.0f);
    glScalef(0.8f, 0.8f, 1.0f);
    glCallLists(2, GL_UNSIGNED_BYTE, ids);
    glPopMatrix();

    results[7] = glGetError() == GL_NO_ERROR;
    for (int i = 0; i < 8; ++i) {
        draw_result_bar(-2.45f + (float)i * 0.7f, results[i]);
    }
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL display list probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.5f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        draw_scene();
        nxglSwapBuffers("NXGL display lists", all_passed() ? "compile/call/delete passed" : "one or more list checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

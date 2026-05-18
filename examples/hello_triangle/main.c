#include <hal/video.h>
#include <windows.h>

#include "nxgl.h"

static void draw_triangle(void)
{
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.15f, 0.12f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.10f, 0.85f, 0.25f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glColor3f(0.12f, 0.35f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glEnd();
}

static void draw_quad(void)
{
    glBegin(GL_QUADS);
    glColor3f(0.95f, 0.75f, 0.12f);
    glVertex3f(-0.8f, 0.8f, 0.0f);
    glColor3f(0.95f, 0.35f, 0.12f);
    glVertex3f(0.8f, 0.8f, 0.0f);
    glColor3f(0.45f, 0.18f, 0.85f);
    glVertex3f(0.8f, -0.8f, 0.0f);
    glColor3f(0.12f, 0.65f, 0.95f);
    glVertex3f(-0.8f, -0.8f, 0.0f);
    glEnd();
}

static void reset_view(void)
{
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -0.75, 0.75, 1.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(void)
{
    float angle = 0.0f;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    if (nxglInit() != 0) {
        return 1;
    }

    glClearColor(0.015f, 0.018f, 0.028f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_TEXTURE_2D);

    for (;;) {
        reset_view();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushMatrix();
        glTranslatef(-1.4f, 0.0f, -5.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        draw_triangle();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(1.4f, 0.0f, -5.0f);
        glRotatef(angle * 0.7f, 1.0f, 1.0f, 0.0f);
        draw_quad();
        glPopMatrix();

        nxglSwapBuffers("NXGL hello triangle", "standalone nxgl.mk consumer");
        angle += 0.8f;
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

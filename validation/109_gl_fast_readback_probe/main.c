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

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool all_passed(void)
{
    for (int i = 0; i < 10; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

static uint32_t expected_shadow_bytes(void)
{
    return 640u * 480u * ((uint32_t)sizeof(uint32_t) + (uint32_t)sizeof(float) + (uint32_t)sizeof(uint8_t));
}

static void draw_quad(float x0, float y0, float x1, float y1, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x0, y0, 0.0f);
    glVertex3f(x1, y0, 0.0f);
    glVertex3f(x1, y1, 0.0f);
    glVertex3f(x0, y1, 0.0f);
    glEnd();
}

static void clear_rgb(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.20f, -1.38f, 0.0f);
    glVertex3f(x + 0.20f, -1.38f, 0.0f);
    glVertex3f(x + 0.20f, -1.63f, 0.0f);
    glVertex3f(x - 0.20f, -1.63f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    NxglPerfCounters counters;
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    uint8_t draw_pixel[4] = { 255, 0, 0, 255 };
    GLuint texture = 0;
    bool ok;

    nxglGetPerfCounters(&counters);
    expect_bool("fast init skips shadow alloc",
                counters.shadow_buffer_allocations == 0 &&
                counters.shadow_buffer_allocation_bytes == 0 &&
                consume_error(GL_NO_ERROR),
                0);

    clear_rgb(0.0f, 0.0f, 0.0f);
    draw_quad(-0.45f, -0.45f, 0.45f, 0.45f, 0.0f, 0.5f, 1.0f);
    nxglSwapBuffers("NXGL fast readback", "fast path warmup");
    nxglGetPerfCounters(&counters);
    expect_bool("fast render keeps shadow unallocated",
                counters.backend_batches == 1 &&
                counters.backend_vertices == 4 &&
                counters.shadow_buffer_allocations == 0 &&
                counters.shadow_primitives == 0 &&
                consume_error(GL_NO_ERROR),
                1);

    nxglResetPerfCounters();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.50f, -0.50f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.50f, 0.50f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.50f, -0.50f, 0.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(0.50f, 0.50f, 0.0f);
    glEnd();
    nxglSwapBuffers("NXGL fast readback", "fast strip warmup");
    nxglGetPerfCounters(&counters);
    expect_bool("fast strip stays native",
                counters.backend_batches == 1 &&
                counters.backend_vertices == 4 &&
                counters.shadow_buffer_allocations == 0 &&
                counters.shadow_primitives == 0 &&
                consume_error(GL_NO_ERROR),
                2);

    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("fast readpixels rejected", consume_error(GL_INVALID_OPERATION), 3);

    glRasterPos2f(0.0f, 0.0f);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, draw_pixel);
    expect_bool("fast drawpixels rejected", consume_error(GL_INVALID_OPERATION), 4);

    glRasterPos2f(0.0f, 0.0f);
    glCopyPixels(320, 240, 1, 1, GL_COLOR);
    expect_bool("fast copypixels rejected", consume_error(GL_INVALID_OPERATION), 5);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 1, 1, 0);
    expect_bool("fast copytex rejected", consume_error(GL_INVALID_OPERATION), 6);

    glClear(GL_ACCUM_BUFFER_BIT);
    expect_bool("fast accum clear rejected", consume_error(GL_INVALID_OPERATION), 7);

    nxglSetReadbackEnabled(GL_TRUE);
    clear_rgb(0.25f, 0.5f, 0.75f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    nxglGetPerfCounters(&counters);
    ok = counters.shadow_buffer_allocations == 3 &&
         counters.shadow_buffer_allocation_bytes == expected_shadow_bytes() &&
         pixel[0] > 40 && pixel[1] > 90 && pixel[2] > 140 &&
         consume_error(GL_NO_ERROR);
    expect_bool("readback enable allocates shadows", ok, 8);

    nxglSetReadbackEnabled(GL_FALSE);
    nxglSetReadbackEnabled(GL_TRUE);
    nxglGetPerfCounters(&counters);
    ok = counters.shadow_buffer_frees == 3 &&
         counters.shadow_buffer_free_bytes == expected_shadow_bytes() &&
         counters.shadow_buffer_allocations == 6 &&
         counters.shadow_buffer_allocation_bytes == expected_shadow_bytes() * 2u &&
         consume_error(GL_NO_ERROR);
    expect_bool("readback toggle frees reallocates", ok, 9);
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL fast readback probe starting\n");

    nxglSetDefaultReadbackEnabled(GL_TRUE);
    if (nxglInitFast() != 0) {
        debugPrint("nxglInitFast failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        clear_rgb(0.02f, 0.02f, 0.04f);
        draw_quad(-0.62f, -0.15f, 0.62f, 0.55f,
                  all_passed() ? 0.05f : 0.45f,
                  all_passed() ? 0.45f : 0.08f,
                  all_passed() ? 0.9f : 0.08f);
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.45f + (float)i * 0.55f, results[i]);
        }

        nxglSwapBuffers("NXGL fast readback", all_passed() ? "all checks passed" : "fast readback check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

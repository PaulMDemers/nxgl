#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef void (*PerfSceneFn)(void);

typedef struct PerfScene {
    const char *name;
    PerfSceneFn draw;
    bool needs_readback;
    bool expect_shadow;
} PerfScene;

static bool results[6];
static GLuint textures[3];
static uint8_t texture_pixels[3][4 * 4 * 4];

static const GLfloat cube_vertices[8][3] = {
    { -0.65f, -0.65f,  0.65f },
    {  0.65f, -0.65f,  0.65f },
    {  0.65f,  0.65f,  0.65f },
    { -0.65f,  0.65f,  0.65f },
    { -0.65f, -0.65f, -0.65f },
    {  0.65f, -0.65f, -0.65f },
    {  0.65f,  0.65f, -0.65f },
    { -0.65f,  0.65f, -0.65f }
};

static const GLfloat cube_texcoords[8][2] = {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f }
};

static const GLfloat cube_colors[8][3] = {
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f },
    { 0.8f, 0.8f, 0.8f },
    { 0.3f, 0.5f, 1.0f }
};

static const GLubyte cube_indices[24] = {
    0, 1, 2, 3,
    1, 5, 6, 2,
    5, 4, 7, 6,
    4, 0, 3, 7,
    3, 2, 6, 7,
    4, 5, 1, 0
};

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool all_passed(void)
{
    for (int i = 0; i < 6; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

static void clear_scene(void)
{
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void reset_state(void)
{
    for (int unit = 0; unit < 4; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -4.0f, 18.0f, 27.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    (void)glGetError();
}

static void upload_texture(GLuint texture, int which, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 16; ++i) {
        texture_pixels[which][i * 4 + 0] = (uint8_t)(r + (i % 4) * 12u);
        texture_pixels[which][i * 4 + 1] = (uint8_t)(g + (i / 4) * 12u);
        texture_pixels[which][i * 4 + 2] = b;
        texture_pixels[which][i * 4 + 3] = 255;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_pixels[which]);
}

static void setup_textures(void)
{
    glGenTextures(3, textures);
    upload_texture(textures[0], 0, 180, 20, 20);
    upload_texture(textures[1], 1, 20, 180, 20);
    upload_texture(textures[2], 2, 20, 20, 180);
}

static void draw_immediate_cube(void)
{
    glBegin(GL_QUADS);
    for (int i = 0; i < 24; ++i) {
        int v = cube_indices[i];
        glColor3fv(cube_colors[v]);
        glVertex3fv(cube_vertices[v]);
    }
    glEnd();
}

static void draw_array_textured_cube(void)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glColorPointer(3, GL_FLOAT, 0, cube_colors);
    glTexCoordPointer(2, GL_FLOAT, 0, cube_texcoords);
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, cube_indices);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void draw_indexed_shared_cube(void)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glColorPointer(3, GL_FLOAT, 0, cube_colors);
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, cube_indices);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void draw_multitexture_quads(void)
{
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    glActiveTexture(GL_TEXTURE0);

    glColor3f(0.55f, 0.55f, 0.55f);
    glBegin(GL_QUADS);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
    glVertex3f(-1.10f, -0.70f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glVertex3f(1.10f, -0.70f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
    glVertex3f(1.10f, 0.70f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
    glVertex3f(-1.10f, 0.70f, 0.0f);
    glEnd();
}

static void draw_lit_cube(void)
{
    static const GLfloat light_pos[4] = { 0.0f, 0.0f, 2.0f, 1.0f };
    static const GLfloat white[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    static const GLfloat ambient[4] = { 0.15f, 0.15f, 0.15f, 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);

    glBegin(GL_QUADS);
    for (int face = 0; face < 6; ++face) {
        if (face == 0) glNormal3f(0.0f, 0.0f, 1.0f);
        if (face == 1) glNormal3f(1.0f, 0.0f, 0.0f);
        if (face == 2) glNormal3f(0.0f, 0.0f, -1.0f);
        if (face == 3) glNormal3f(-1.0f, 0.0f, 0.0f);
        if (face == 4) glNormal3f(0.0f, 1.0f, 0.0f);
        if (face == 5) glNormal3f(0.0f, -1.0f, 0.0f);
        for (int corner = 0; corner < 4; ++corner) {
            int v = cube_indices[face * 4 + corner];
            glVertex3fv(cube_vertices[v]);
        }
    }
    glEnd();
}

static void draw_readback_quad(void)
{
    glColor3f(0.1f, 0.7f, 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(-0.70f, -0.70f, 0.0f);
    glVertex3f(0.70f, -0.70f, 0.0f);
    glVertex3f(0.70f, 0.70f, 0.0f);
    glVertex3f(-0.70f, 0.70f, 0.0f);
    glEnd();
}

static void print_counter_line(const char *name, const NxglPerfCounters *c)
{
    debugPrint("PERF %-18s frames=%u backend_batches=%u backend_vertices=%u command_blocks=%u flush=%u/%ums finish=%u/%ums\n",
               name,
               c->frames,
               c->backend_batches,
               c->backend_vertices,
               c->backend_command_blocks,
               c->backend_flush_calls,
               c->backend_flush_ms,
               c->backend_finish_calls,
               c->backend_finish_ms);
    debugPrint("PERF %-18s arrays=%u pos=%u norm=%u light=%u fog=%u clip_tests=%u clip_vertices=%u clipped=%u shadow_prims=%u shadow_fragments=%u read=%u\n",
               name,
               c->cpu_array_vertices,
               c->cpu_position_transforms,
               c->cpu_normal_transforms,
               c->cpu_lighting_vertices,
               c->cpu_fog_vertices,
               c->cpu_clip_tests,
               c->cpu_clip_vertices,
               c->cpu_clipped_primitives,
               c->shadow_primitives,
               c->shadow_fragments,
               c->read_pixels_calls);
    debugPrint("PERF %-18s shader=%u/%u render=%u/%u texstage=%u/%u texdisable=%u/%u tex_uploads=%u sub=%u\n",
               name,
               c->backend_shader_uploads,
               c->backend_shader_cache_hits,
               c->backend_render_state_uploads,
               c->backend_render_state_cache_hits,
               c->backend_texture_stage_uploads,
               c->backend_texture_stage_cache_hits,
               c->backend_texture_stage_disables,
               c->backend_texture_stage_disable_hits,
               c->texture_uploads,
               c->texture_sub_uploads);
}

static bool validate_scene_counters(const PerfScene *scene, const NxglPerfCounters *c)
{
    if (c->frames != 1 || c->backend_batches == 0 || c->backend_vertices == 0 ||
        c->backend_command_blocks == 0 || c->backend_flush_calls == 0 ||
        c->backend_finish_calls != 1) {
        return false;
    }
    if (!scene->needs_readback && (c->shadow_buffer_allocations != 0 ||
        c->shadow_primitives != 0)) {
        return false;
    }
    if (scene->expect_shadow && (c->shadow_primitives == 0 || c->read_pixels_calls == 0)) {
        return false;
    }
    return glGetError() == GL_NO_ERROR;
}

static void run_perf_scene(const PerfScene *scene, int slot)
{
    NxglPerfCounters counters;
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    bool ok;

    reset_state();
    nxglSetReadbackEnabled(scene->needs_readback ? GL_TRUE : GL_FALSE);
    nxglResetPerfCounters();
    clear_scene();
    scene->draw();
    if (scene->needs_readback) {
        glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    }
    nxglSwapBuffers("NXGL perf counters", scene->name);
    nxglGetPerfCounters(&counters);
    print_counter_line(scene->name, &counters);
    ok = validate_scene_counters(scene, &counters);
    expect_bool(scene->name, ok, slot);
}

static void draw_result_bars(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glColor3f(all_passed() ? 0.05f : 0.45f, all_passed() ? 0.45f : 0.08f, all_passed() ? 0.9f : 0.08f);
    glBegin(GL_QUADS);
    glVertex3f(-0.95f, -0.15f, 0.0f);
    glVertex3f(0.95f, -0.15f, 0.0f);
    glVertex3f(0.95f, 0.55f, 0.0f);
    glVertex3f(-0.95f, 0.55f, 0.0f);
    glEnd();

    for (int i = 0; i < 6; ++i) {
        float x = -1.25f + (float)i * 0.50f;
        glColor3f(results[i] ? 0.1f : 0.9f, results[i] ? 0.8f : 0.1f, 0.15f);
        glBegin(GL_QUADS);
        glVertex3f(x - 0.18f, -1.25f, 0.0f);
        glVertex3f(x + 0.18f, -1.25f, 0.0f);
        glVertex3f(x + 0.18f, -1.50f, 0.0f);
        glVertex3f(x - 0.18f, -1.50f, 0.0f);
        glEnd();
    }
}

int main(void)
{
    static const PerfScene scenes[] = {
        { "immediate_cube", draw_immediate_cube, false, false },
        { "array_textured_cube", draw_array_textured_cube, false, false },
        { "indexed_shared_cube", draw_indexed_shared_cube, false, false },
        { "multitexture_quad", draw_multitexture_quads, false, false },
        { "lit_cube", draw_lit_cube, false, false },
        { "readback_quad", draw_readback_quad, true, true }
    };

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL perf counter probe starting\n");

    nxglSetDefaultReadbackEnabled(GL_FALSE);
    if (nxglInitFast() != 0) {
        debugPrint("nxglInitFast failed\n");
        return 1;
    }

    setup_textures();
    memset(results, 0, sizeof(results));
    for (int i = 0; i < 6; ++i) {
        run_perf_scene(&scenes[i], i);
    }

    for (;;) {
        reset_state();
        nxglSetReadbackEnabled(GL_FALSE);
        clear_scene();
        draw_result_bars();
        nxglSwapBuffers("NXGL perf counters", all_passed() ? "all checks passed" : "perf counter check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}

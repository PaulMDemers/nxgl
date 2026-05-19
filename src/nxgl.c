#include "nxgl.h"

#include "nxgl_backend.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xboxkrnl/xboxkrnl.h>

typedef float Matrix[16];

#define SHADOW_MAX_WIDTH 640
#define SHADOW_MAX_HEIGHT 480
#define SHADOW_MAXRAM 0x03FFAFFF
#define NXGL_MAX_TEXTURE_LEVELS 13
#define NXGL_PIXEL_MAP_MAX 256
#define NXGL_MAX_EVAL_ORDER 8
#define NXGL_EVAL_MAP_COUNT 9
#define NXGL_MAX_CLIP_PLANES 6
#define NXGL_NO_BEGIN_MODE 0xffffffffu
#define NXGL_ATTRIB_STACK_MAX 16
#define NXGL_CLIENT_ATTRIB_STACK_MAX 16
#define NXGL_CLIPPED_POLYGON_MAX 32

typedef struct PixelMapState {
    GLsizei size;
    GLfloat values[NXGL_PIXEL_MAP_MAX];
} PixelMapState;

typedef struct EvalMap1 {
    bool defined;
    GLboolean enabled;
    GLenum target;
    GLint components;
    GLint order;
    GLfloat u1;
    GLfloat u2;
    GLfloat points[NXGL_MAX_EVAL_ORDER * 4];
} EvalMap1;

typedef struct EvalMap2 {
    bool defined;
    GLboolean enabled;
    GLenum target;
    GLint components;
    GLint uorder;
    GLint vorder;
    GLfloat u1;
    GLfloat u2;
    GLfloat v1;
    GLfloat v2;
    GLfloat points[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
} EvalMap2;

typedef struct TexGenState {
    GLboolean enabled;
    GLenum mode;
    GLfloat object_plane[4];
    GLfloat eye_plane[4];
} TexGenState;

typedef struct ClipPlaneState {
    GLboolean enabled;
    GLdouble equation[4];
} ClipPlaneState;

typedef struct ClientArray {
    bool enabled;
    GLint size;
    GLenum type;
    GLsizei stride;
    const uint8_t *pointer;
} ClientArray;

typedef struct TextureLevel {
    bool defined;
    bool compressed;
    GLint internal_format;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    uint8_t *rgba;
    uint8_t *compressed_data;
    GLsizei compressed_size;
} TextureLevel;

typedef struct TextureObject {
    bool allocated;
    GLint min_filter;
    GLint mag_filter;
    GLint wrap_s;
    GLint wrap_t;
    GLint wrap_r;
    GLfloat min_lod;
    GLfloat max_lod;
    GLfloat lod_bias;
    GLfloat priority;
    GLint base_level;
    GLint max_level;
    TextureLevel levels[NXGL_MAX_TEXTURE_LEVELS];
    TextureLevel levels_1d[NXGL_MAX_TEXTURE_LEVELS];
    TextureLevel levels_3d[NXGL_MAX_TEXTURE_LEVELS];
    TextureLevel cube_faces[6][NXGL_MAX_TEXTURE_LEVELS];
    NxglBackendTexture native;
    NxglBackendTexture native_1d;
    NxglBackendTexture native_3d;
    NxglBackendTexture native_cube;
} TextureObject;

typedef struct LightObject {
    bool enabled;
    NxglBackendColor ambient;
    NxglBackendColor diffuse;
    NxglBackendColor specular;
    GLfloat position[4];
    GLfloat spot_direction[3];
    GLfloat spot_exponent;
    GLfloat spot_cutoff;
    GLfloat constant_attenuation;
    GLfloat linear_attenuation;
    GLfloat quadratic_attenuation;
} LightObject;

typedef struct MaterialState {
    NxglBackendColor ambient;
    NxglBackendColor diffuse;
    NxglBackendColor specular;
    NxglBackendColor emission;
    GLfloat shininess;
} MaterialState;

typedef enum ListCommandType {
    LIST_CMD_CLEAR_COLOR,
    LIST_CMD_CLEAR_INDEX,
    LIST_CMD_CLEAR_ACCUM,
    LIST_CMD_CLEAR,
    LIST_CMD_ACCUM,
    LIST_CMD_CLEAR_DEPTH,
    LIST_CMD_DEPTH_RANGE,
    LIST_CMD_CLEAR_STENCIL,
    LIST_CMD_VIEWPORT,
    LIST_CMD_PASS_THROUGH,
    LIST_CMD_MATRIX_MODE,
    LIST_CMD_LOAD_IDENTITY,
    LIST_CMD_LOAD_MATRIX,
    LIST_CMD_MULT_MATRIX,
    LIST_CMD_PUSH_MATRIX,
    LIST_CMD_POP_MATRIX,
    LIST_CMD_TRANSLATE,
    LIST_CMD_SCALE,
    LIST_CMD_ROTATE,
    LIST_CMD_COLOR4,
    LIST_CMD_INDEX,
    LIST_CMD_NORMAL3,
    LIST_CMD_TEXCOORD2,
    LIST_CMD_TEXCOORD3,
    LIST_CMD_BEGIN,
    LIST_CMD_VERTEX3,
    LIST_CMD_END,
    LIST_CMD_POINT_SIZE,
    LIST_CMD_LINE_WIDTH,
    LIST_CMD_LINE_STIPPLE,
    LIST_CMD_POLYGON_STIPPLE,
    LIST_CMD_POLYGON_MODE,
    LIST_CMD_POLYGON_OFFSET,
    LIST_CMD_ALPHA_FUNC,
    LIST_CMD_COLOR_MASK,
    LIST_CMD_LOGIC_OP,
    LIST_CMD_CULL_FACE,
    LIST_CMD_FRONT_FACE,
    LIST_CMD_HINT,
    LIST_CMD_EDGE_FLAG,
    LIST_CMD_PIXEL_ZOOM,
    LIST_CMD_PIXEL_TRANSFER,
    LIST_CMD_PIXEL_MAP,
    LIST_CMD_BITMAP,
    LIST_CMD_DRAW_PIXELS,
    LIST_CMD_COPY_PIXELS,
    LIST_CMD_SHADE_MODEL,
    LIST_CMD_LIGHT_MODEL,
    LIST_CMD_LIGHT,
    LIST_CMD_FOG,
    LIST_CMD_CLIP_PLANE,
    LIST_CMD_RASTER_POS,
    LIST_CMD_ENABLE,
    LIST_CMD_DISABLE,
    LIST_CMD_DEPTH_FUNC,
    LIST_CMD_DEPTH_MASK,
    LIST_CMD_STENCIL_FUNC,
    LIST_CMD_STENCIL_OP,
    LIST_CMD_STENCIL_MASK,
    LIST_CMD_SCISSOR,
    LIST_CMD_BLEND_FUNC,
    LIST_CMD_DRAW_BUFFER,
    LIST_CMD_READ_BUFFER,
    LIST_CMD_ACTIVE_TEXTURE,
    LIST_CMD_BIND_TEXTURE,
    LIST_CMD_TEX_PARAMETER,
    LIST_CMD_TEX_PARAMETER_F,
    LIST_CMD_TEX_ENV,
    LIST_CMD_TEX_GEN,
    LIST_CMD_PUSH_ATTRIB,
    LIST_CMD_POP_ATTRIB,
    LIST_CMD_PUSH_CLIENT_ATTRIB,
    LIST_CMD_POP_CLIENT_ATTRIB
} ListCommandType;

typedef struct ListCommand {
    ListCommandType type;
    GLenum a;
    GLenum b;
    GLuint u;
    GLbitfield bits;
    GLfloat f[4];
    GLfloat extra[2];
    Matrix matrix;
    GLint i[4];
    uint8_t bytes[128];
    void *data;
    size_t data_size;
} ListCommand;

typedef struct DisplayList {
    bool allocated;
    ListCommand *commands;
    size_t count;
    size_t capacity;
} DisplayList;

typedef struct AttribSnapshot {
    GLbitfield mask;
    NxglBackendColor current_color;
    NxglBackendVec3 current_normal;
    GLfloat current_u[4];
    GLfloat current_v[4];
    GLfloat current_r[4];
    GLboolean current_edge_flag;
    GLfloat current_raster_position[4];
    bool current_raster_position_valid;
    ClipPlaneState clip_planes[NXGL_MAX_CLIP_PLANES];
    GLfloat point_size;
    GLfloat line_width;
    GLint line_stipple_factor;
    GLushort line_stipple_pattern;
    bool line_stipple_enabled;
    GLenum polygon_mode;
    uint8_t polygon_stipple_pattern[128];
    bool polygon_stipple_enabled;
    bool polygon_offset_point_enabled;
    bool polygon_offset_line_enabled;
    bool polygon_offset_fill_enabled;
    GLfloat polygon_offset_factor;
    GLfloat polygon_offset_units;
    bool cull_enabled;
    GLenum cull_face_mode;
    GLenum front_face_mode;
    GLfloat pixel_zoom_x;
    GLfloat pixel_zoom_y;
    GLfloat pixel_transfer_scale[4];
    GLfloat pixel_transfer_bias[4];
    PixelMapState pixel_maps[10];
    bool lighting_enabled;
    LightObject lights[8];
    GLenum shade_model;
    bool light_model_local_viewer;
    bool light_model_two_side;
    GLenum light_model_color_control;
    NxglBackendColor light_model_ambient;
    bool color_material_enabled;
    GLenum color_material_face;
    GLenum color_material_parameter;
    MaterialState material_state;
    MaterialState material_back_state;
    bool fog_enabled;
    GLenum fog_mode;
    GLenum fog_hint;
    NxglBackendColor fog_color;
    GLfloat fog_density;
    GLfloat fog_start;
    GLfloat fog_end;
    bool depth_test_enabled;
    bool depth_write_enabled;
    GLfloat depth_clear_value;
    GLfloat depth_range_near;
    GLfloat depth_range_far;
    GLenum depth_func;
    GLfloat accum_clear_value[4];
    bool stencil_test_enabled;
    GLint stencil_clear_value;
    GLenum stencil_func;
    GLint stencil_ref;
    GLuint stencil_value_mask;
    GLuint stencil_write_mask;
    GLenum stencil_fail;
    GLenum stencil_zfail;
    GLenum stencil_zpass;
    GLint viewport[4];
    GLint scissor_box[4];
    bool scissor_test_enabled;
    GLenum matrix_mode;
    bool normalize_enabled;
    bool rescale_normal_enabled;
    bool blend_enabled;
    GLenum blend_sfactor;
    GLenum blend_dfactor;
    bool alpha_test_enabled;
    GLenum alpha_test_func;
    GLfloat alpha_test_ref;
    GLboolean color_write_mask[4];
    bool color_logic_op_enabled;
    GLenum logic_op_mode;
    bool multisample_enabled;
    bool sample_alpha_to_coverage_enabled;
    bool sample_alpha_to_one_enabled;
    bool sample_coverage_enabled;
    GLclampf sample_coverage_value;
    GLboolean sample_coverage_invert;
    GLenum draw_buffer_mode;
    GLenum read_buffer_mode;
    GLenum perspective_correction_hint;
    GLenum point_smooth_hint;
    GLenum line_smooth_hint;
    GLenum polygon_smooth_hint;
    EvalMap1 eval_maps1[NXGL_EVAL_MAP_COUNT];
    EvalMap2 eval_maps2[NXGL_EVAL_MAP_COUNT];
    GLboolean auto_normal_enabled;
    GLint map_grid1_n;
    GLfloat map_grid1_u1;
    GLfloat map_grid1_u2;
    GLint map_grid2_un;
    GLint map_grid2_vn;
    GLfloat map_grid2_u1;
    GLfloat map_grid2_u2;
    GLfloat map_grid2_v1;
    GLfloat map_grid2_v2;
    GLuint list_base;
    GLenum active_texture;
    bool texture_1d_enabled[4];
    bool texture_2d_enabled[4];
    bool texture_3d_enabled[4];
    bool texture_cube_map_enabled[4];
    GLuint texture_binding_1d[4];
    GLuint texture_binding_2d[4];
    GLuint texture_binding_3d[4];
    GLuint texture_binding_cube_map[4];
    TexGenState texgen_state[4][4];
    GLenum texture_env_mode[4];
    NxglBackendColor texture_env_color[4];
    GLenum texture_combine_rgb[4];
    GLenum texture_combine_alpha[4];
    GLenum texture_source_rgb[4][3];
    GLenum texture_source_alpha[4][3];
    GLenum texture_operand_rgb[4][3];
    GLenum texture_operand_alpha[4][3];
    GLfloat texture_rgb_scale[4];
    GLfloat texture_alpha_scale[4];
} AttribSnapshot;

typedef struct ClientAttribSnapshot {
    GLbitfield mask;
    GLenum client_active_texture;
    ClientArray vertex_array;
    ClientArray color_array;
    ClientArray texcoord_array[4];
    ClientArray normal_array;
    GLint pack_alignment;
    GLint pack_row_length;
    GLint pack_skip_rows;
    GLint pack_skip_pixels;
    GLint pack_image_height;
    GLint pack_skip_images;
    GLboolean pack_swap_bytes;
    GLboolean pack_lsb_first;
    GLint unpack_alignment;
    GLint unpack_row_length;
    GLint unpack_skip_rows;
    GLint unpack_skip_pixels;
    GLint unpack_image_height;
    GLint unpack_skip_images;
    GLboolean unpack_swap_bytes;
    GLboolean unpack_lsb_first;
} ClientAttribSnapshot;

enum {
    M11 = 0, M12, M13, M14,
    M21, M22, M23, M24,
    M31, M32, M33, M34,
    M41, M42, M43, M44
};

static Matrix modelview;
static Matrix projection;
static Matrix texture_matrix;
static Matrix modelview_stack[32];
static Matrix projection_stack[8];
static Matrix texture_stack[8];
static Matrix modelview_inverse_cache;
static int modelview_stack_top;
static int projection_stack_top;
static int texture_stack_top;
static GLenum matrix_mode = GL_MODELVIEW;
static bool modelview_inverse_cache_valid;
static bool modelview_inverse_cache_invertible;
static NxglBackendColor current_color = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat current_index;
static NxglBackendVec3 current_normal = { 0.0f, 0.0f, 1.0f };
static float current_u[4];
static float current_v[4];
static float current_r[4];
static NxglBackendVertex pending[1024];
static int pending_count;
static GLenum begin_mode = NXGL_NO_BEGIN_MODE;
static GLfloat point_size = 1.0f;
static GLfloat line_width = 1.0f;
static GLint line_stipple_factor = 1;
static GLushort line_stipple_pattern = 0xffffu;
static bool line_stipple_enabled;
static GLenum polygon_mode = GL_FILL;
static uint8_t polygon_stipple_pattern[128];
static bool polygon_stipple_enabled;
static bool polygon_offset_point_enabled;
static bool polygon_offset_line_enabled;
static bool polygon_offset_fill_enabled;
static GLfloat polygon_offset_factor;
static GLfloat polygon_offset_units;
static GLenum shade_model = GL_SMOOTH;
static uint32_t clear_color = 0x00000000;
static GLfloat clear_color_value[4];
static GLfloat clear_index_value;
static GLint viewport[4] = { 0, 0, 640, 480 };
static GLint scissor_box[4] = { 0, 0, 640, 480 };
static GLenum last_error = GL_NO_ERROR;
static bool blend_enabled;
static bool depth_test_enabled = true;
static bool depth_write_enabled = true;
static bool cull_enabled;
static GLenum cull_face_mode = GL_BACK;
static GLenum front_face_mode = GL_CCW;
static GLboolean current_edge_flag = GL_TRUE;
static GLenum perspective_correction_hint = GL_DONT_CARE;
static GLenum point_smooth_hint = GL_DONT_CARE;
static GLenum line_smooth_hint = GL_DONT_CARE;
static GLenum polygon_smooth_hint = GL_DONT_CARE;
static bool scissor_test_enabled;
static bool stencil_test_enabled;
static bool alpha_test_enabled;
static GLenum alpha_test_func = GL_ALWAYS;
static GLfloat alpha_test_ref;
static GLboolean color_write_mask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
static bool color_logic_op_enabled;
static GLenum logic_op_mode = GL_COPY;
static bool lighting_enabled;
static bool light_model_local_viewer;
static bool light_model_two_side;
static GLenum light_model_color_control = GL_SINGLE_COLOR;
static NxglBackendColor light_model_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
static bool color_material_enabled;
static bool fog_enabled;
static bool normalize_enabled;
static bool rescale_normal_enabled;
static bool multisample_enabled = true;
static bool sample_alpha_to_coverage_enabled;
static bool sample_alpha_to_one_enabled;
static bool sample_coverage_enabled;
static GLclampf sample_coverage_value = 1.0f;
static GLboolean sample_coverage_invert;
static ClipPlaneState clip_planes[NXGL_MAX_CLIP_PLANES];
static bool texture_1d_enabled[4];
static bool texture_2d_enabled[4];
static bool texture_3d_enabled[4];
static bool texture_cube_map_enabled[4];
static TexGenState texgen_state[4][4];
static bool texgen_any_enabled;
static GLenum active_texture = GL_TEXTURE0;
static GLenum client_active_texture = GL_TEXTURE0;
static TextureObject texture_objects[16];
static GLuint texture_binding_1d[4];
static GLuint texture_binding_2d[4];
static GLuint texture_binding_3d[4];
static GLuint texture_binding_cube_map[4];
static GLenum texture_env_mode[4];
static NxglBackendColor texture_env_color[4];
static GLenum texture_combine_rgb[4];
static GLenum texture_combine_alpha[4];
static GLenum texture_source_rgb[4][3];
static GLenum texture_source_alpha[4][3];
static GLenum texture_operand_rgb[4][3];
static GLenum texture_operand_alpha[4][3];
static GLfloat texture_rgb_scale[4];
static GLfloat texture_alpha_scale[4];
static GLfloat pixel_zoom_x = 1.0f;
static GLfloat pixel_zoom_y = 1.0f;
static GLfloat pixel_transfer_scale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat pixel_transfer_bias[4];
static PixelMapState pixel_maps[10];
static EvalMap1 eval_maps1[NXGL_EVAL_MAP_COUNT];
static EvalMap2 eval_maps2[NXGL_EVAL_MAP_COUNT];
static GLboolean auto_normal_enabled;
static GLint map_grid1_n = 1;
static GLfloat map_grid1_u1;
static GLfloat map_grid1_u2 = 1.0f;
static GLint map_grid2_un = 1;
static GLint map_grid2_vn = 1;
static GLfloat map_grid2_u1;
static GLfloat map_grid2_u2 = 1.0f;
static GLfloat map_grid2_v1;
static GLfloat map_grid2_v2 = 1.0f;
static GLfloat depth_clear_value = 1.0f;
static GLfloat depth_range_near = 0.0f;
static GLfloat depth_range_far = 1.0f;
static GLenum depth_func = GL_LEQUAL;
static GLint stencil_clear_value;
static GLenum stencil_func = GL_ALWAYS;
static GLint stencil_ref;
static GLuint stencil_value_mask = 0xffffffffu;
static GLuint stencil_write_mask = 0xffffffffu;
static GLenum stencil_fail = GL_KEEP;
static GLenum stencil_zfail = GL_KEEP;
static GLenum stencil_zpass = GL_KEEP;
static GLenum blend_sfactor = GL_SRC_ALPHA;
static GLenum blend_dfactor = GL_ONE_MINUS_SRC_ALPHA;
static GLenum draw_buffer_mode = GL_BACK;
static GLenum read_buffer_mode = GL_BACK;
static GLenum color_material_face = GL_FRONT_AND_BACK;
static GLenum color_material_parameter = GL_AMBIENT_AND_DIFFUSE;
static GLenum fog_mode = GL_EXP;
static GLenum fog_hint = GL_DONT_CARE;
static NxglBackendColor fog_color = { 0.0f, 0.0f, 0.0f, 0.0f };
static GLfloat fog_density = 1.0f;
static GLfloat fog_start = 0.0f;
static GLfloat fog_end = 1.0f;
static GLint pack_alignment = 4;
static GLint pack_row_length;
static GLint pack_skip_rows;
static GLint pack_skip_pixels;
static GLint pack_image_height;
static GLint pack_skip_images;
static GLboolean pack_swap_bytes;
static GLboolean pack_lsb_first;
static GLint unpack_alignment = 4;
static GLint unpack_row_length;
static GLint unpack_skip_rows;
static GLint unpack_skip_pixels;
static GLint unpack_image_height;
static GLint unpack_skip_images;
static GLboolean unpack_swap_bytes;
static GLboolean unpack_lsb_first;
static uint32_t *shadow_color_buffer;
static float *shadow_depth_buffer;
static uint8_t *shadow_stencil_buffer;
static bool shadow_readback_enabled = true;
static float *accum_buffer;
static bool native_frame_started;
static GLfloat accum_clear_value[4];
static int shadow_width = 640;
static int shadow_height = 480;
static float camera_x;
static float camera_y;
static float camera_z = -6.0f;
static GLfloat current_raster_position[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static bool current_raster_position_valid = true;
static GLenum render_mode = GL_RENDER;
static GLuint *selection_buffer;
static GLsizei selection_buffer_size;
static GLsizei selection_write_count;
static GLint selection_hits;
static bool selection_overflow;
static GLuint name_stack[64];
static GLint name_stack_depth;
static GLfloat *feedback_buffer;
static GLsizei feedback_buffer_size;
static GLsizei feedback_write_count;
static bool feedback_overflow;
static GLenum feedback_type = GL_2D;
static LightObject lights[8];
static MaterialState material_state = {
    { 0.2f, 0.2f, 0.2f, 1.0f },
    { 0.8f, 0.8f, 0.8f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    0.0f
};
static MaterialState material_back_state = {
    { 0.2f, 0.2f, 0.2f, 1.0f },
    { 0.8f, 0.8f, 0.8f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    0.0f
};
static DisplayList display_lists[256];
static GLuint recording_list;
static GLenum recording_mode;
static GLuint list_base;
static bool replaying_list;
static ClientArray vertex_array = { false, 4, GL_FLOAT, 0, NULL };
static ClientArray color_array = { false, 4, GL_FLOAT, 0, NULL };
static ClientArray texcoord_array[4] = {
    { false, 4, GL_FLOAT, 0, NULL },
    { false, 4, GL_FLOAT, 0, NULL },
    { false, 4, GL_FLOAT, 0, NULL },
    { false, 4, GL_FLOAT, 0, NULL }
};
static ClientArray normal_array = { false, 3, GL_FLOAT, 0, NULL };
static AttribSnapshot attrib_stack[NXGL_ATTRIB_STACK_MAX];
static GLint attrib_stack_top;
static ClientAttribSnapshot client_attrib_stack[NXGL_CLIENT_ATTRIB_STACK_MAX];
static GLint client_attrib_stack_top;

static int source_components(GLenum format);
static int texgen_coord_index(GLenum coord);
static GLenum texgen_cap_from_index(int index);
static bool valid_texgen_mode(GLenum coord, GLenum mode);
static void init_texgen_state(void);
static void apply_texgen_to_coords(GLfloat obj[4], NxglBackendVec3 eye, NxglBackendVec3 normal, GLfloat *u, GLfloat *v, GLfloat *r, int unit);
static void capture_attrib_snapshot(AttribSnapshot *snapshot, GLbitfield mask);
static void restore_attrib_snapshot(const AttribSnapshot *snapshot);
static void capture_client_attrib_snapshot(ClientAttribSnapshot *snapshot, GLbitfield mask);
static void restore_client_attrib_snapshot(const ClientAttribSnapshot *snapshot);
static int eval_map_index(GLenum target, bool *map2);
static int eval_map_components(GLenum target);
static void init_eval_maps(void);
static void eval_map1_value(const EvalMap1 *map, GLfloat u, GLfloat out[4]);
static void eval_map2_value(const EvalMap2 *map, GLfloat u, GLfloat v, GLfloat out[4]);
static void apply_eval_map1(GLfloat u);
static void apply_eval_map2(GLfloat u, GLfloat v);
static int clip_plane_index(GLenum plane);
static bool invert_matrix(Matrix out, const Matrix in);
static void transform_clip_plane(GLdouble out[4], const GLdouble in[4]);
static bool point_inside_clip_planes(NxglBackendVec3 pos);
static bool primitive_rejected_by_clip_planes(const NxglBackendVertex *vertices, int count);
static GLfloat feedback_clip_plane_value(const NxglBackendVertex *vertex, int plane);
static void normalize_lower_feedback_clip_edges(NxglBackendVertex *vertices, int count);
static bool clip_planes_enabled(void);
static bool native_fast_fill_enabled(void);
static void shadow_fill_bounds(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d, bool quad, bool line);
static int mip_chain_end_level(GLsizei width, GLsizei height, GLsizei depth, GLint base_level, GLint max_level);
static bool texture_levels_complete(const TextureObject *texture, const TextureLevel *levels);
static TextureLevel *select_texture_level_for_lod(TextureObject *texture, TextureLevel *levels, GLfloat lambda);
static bool texture_complete(const TextureObject *texture);
static bool texture_1d_complete(const TextureObject *texture);
static bool is_mipmap_filter(GLint filter);
static bool copy_shadow_rgba(uint8_t *dst, GLint x, GLint y, GLsizei width, GLsizei height);
static bool valid_stencil_func(GLenum func);
static bool valid_stencil_op(GLenum op);
static bool valid_depth_func(GLenum func);
static bool valid_blend_src_factor(GLenum factor);
static bool valid_blend_dst_factor(GLenum factor);
static bool valid_logic_op(GLenum op);
static bool valid_hint_target(GLenum target);
static bool valid_hint_mode(GLenum mode);
static bool valid_draw_buffer(GLenum mode);
static bool valid_read_buffer(GLenum mode);
static GLfloat clamp_priority(GLfloat value);
static size_t source_element_size(GLenum type);
static bool ensure_accum_buffer(void);
static int get_component_count(GLenum pname);
static bool valid_pixel_type(GLenum type);
static bool convert_to_rgba(uint8_t *dst, const uint8_t *src, GLsizei width, GLsizei height, GLenum format, GLenum type);
static bool shadow_project(NxglBackendVec3 pos, int *sx, int *sy);
static size_t pixel_source_data_size(GLsizei width, GLsizei height, GLenum format, GLenum type);
static size_t bitmap_source_data_size(GLsizei width, GLsizei height);
static void update_texgen_enabled_cache(void);
static void sync_native_state(void);
static void ensure_native_frame_started(void);
static void apply_color_material(NxglBackendColor color);

static void set_error(GLenum error)
{
    if (last_error == GL_NO_ERROR) {
        last_error = error;
    }
}

static void capture_attrib_snapshot(AttribSnapshot *snapshot, GLbitfield mask)
{
    snapshot->mask = mask & GL_ALL_ATTRIB_BITS;
    snapshot->current_color = current_color;
    snapshot->current_normal = current_normal;
    memcpy(snapshot->current_u, current_u, sizeof(current_u));
    memcpy(snapshot->current_v, current_v, sizeof(current_v));
    memcpy(snapshot->current_r, current_r, sizeof(current_r));
    snapshot->current_edge_flag = current_edge_flag;
    memcpy(snapshot->current_raster_position, current_raster_position, sizeof(current_raster_position));
    snapshot->current_raster_position_valid = current_raster_position_valid;
    memcpy(snapshot->clip_planes, clip_planes, sizeof(clip_planes));
    snapshot->point_size = point_size;
    snapshot->line_width = line_width;
    snapshot->line_stipple_factor = line_stipple_factor;
    snapshot->line_stipple_pattern = line_stipple_pattern;
    snapshot->line_stipple_enabled = line_stipple_enabled;
    snapshot->polygon_mode = polygon_mode;
    memcpy(snapshot->polygon_stipple_pattern, polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
    snapshot->polygon_stipple_enabled = polygon_stipple_enabled;
    snapshot->polygon_offset_point_enabled = polygon_offset_point_enabled;
    snapshot->polygon_offset_line_enabled = polygon_offset_line_enabled;
    snapshot->polygon_offset_fill_enabled = polygon_offset_fill_enabled;
    snapshot->polygon_offset_factor = polygon_offset_factor;
    snapshot->polygon_offset_units = polygon_offset_units;
    snapshot->cull_enabled = cull_enabled;
    snapshot->cull_face_mode = cull_face_mode;
    snapshot->front_face_mode = front_face_mode;
    snapshot->pixel_zoom_x = pixel_zoom_x;
    snapshot->pixel_zoom_y = pixel_zoom_y;
    memcpy(snapshot->pixel_transfer_scale, pixel_transfer_scale, sizeof(pixel_transfer_scale));
    memcpy(snapshot->pixel_transfer_bias, pixel_transfer_bias, sizeof(pixel_transfer_bias));
    memcpy(snapshot->pixel_maps, pixel_maps, sizeof(pixel_maps));
    snapshot->lighting_enabled = lighting_enabled;
    memcpy(snapshot->lights, lights, sizeof(lights));
    snapshot->shade_model = shade_model;
    snapshot->light_model_local_viewer = light_model_local_viewer;
    snapshot->light_model_two_side = light_model_two_side;
    snapshot->light_model_color_control = light_model_color_control;
    snapshot->light_model_ambient = light_model_ambient;
    snapshot->color_material_enabled = color_material_enabled;
    snapshot->color_material_face = color_material_face;
    snapshot->color_material_parameter = color_material_parameter;
    snapshot->material_state = material_state;
    snapshot->material_back_state = material_back_state;
    snapshot->fog_enabled = fog_enabled;
    snapshot->fog_mode = fog_mode;
    snapshot->fog_hint = fog_hint;
    snapshot->fog_color = fog_color;
    snapshot->fog_density = fog_density;
    snapshot->fog_start = fog_start;
    snapshot->fog_end = fog_end;
    snapshot->depth_test_enabled = depth_test_enabled;
    snapshot->depth_write_enabled = depth_write_enabled;
    snapshot->depth_clear_value = depth_clear_value;
    snapshot->depth_range_near = depth_range_near;
    snapshot->depth_range_far = depth_range_far;
    snapshot->depth_func = depth_func;
    memcpy(snapshot->accum_clear_value, accum_clear_value, sizeof(accum_clear_value));
    snapshot->stencil_test_enabled = stencil_test_enabled;
    snapshot->stencil_clear_value = stencil_clear_value;
    snapshot->stencil_func = stencil_func;
    snapshot->stencil_ref = stencil_ref;
    snapshot->stencil_value_mask = stencil_value_mask;
    snapshot->stencil_write_mask = stencil_write_mask;
    snapshot->stencil_fail = stencil_fail;
    snapshot->stencil_zfail = stencil_zfail;
    snapshot->stencil_zpass = stencil_zpass;
    memcpy(snapshot->viewport, viewport, sizeof(viewport));
    memcpy(snapshot->scissor_box, scissor_box, sizeof(scissor_box));
    snapshot->scissor_test_enabled = scissor_test_enabled;
    snapshot->matrix_mode = matrix_mode;
    snapshot->normalize_enabled = normalize_enabled;
    snapshot->rescale_normal_enabled = rescale_normal_enabled;
    snapshot->blend_enabled = blend_enabled;
    snapshot->blend_sfactor = blend_sfactor;
    snapshot->blend_dfactor = blend_dfactor;
    snapshot->alpha_test_enabled = alpha_test_enabled;
    snapshot->alpha_test_func = alpha_test_func;
    snapshot->alpha_test_ref = alpha_test_ref;
    memcpy(snapshot->color_write_mask, color_write_mask, sizeof(color_write_mask));
    snapshot->color_logic_op_enabled = color_logic_op_enabled;
    snapshot->logic_op_mode = logic_op_mode;
    snapshot->multisample_enabled = multisample_enabled;
    snapshot->sample_alpha_to_coverage_enabled = sample_alpha_to_coverage_enabled;
    snapshot->sample_alpha_to_one_enabled = sample_alpha_to_one_enabled;
    snapshot->sample_coverage_enabled = sample_coverage_enabled;
    snapshot->sample_coverage_value = sample_coverage_value;
    snapshot->sample_coverage_invert = sample_coverage_invert;
    snapshot->draw_buffer_mode = draw_buffer_mode;
    snapshot->read_buffer_mode = read_buffer_mode;
    snapshot->perspective_correction_hint = perspective_correction_hint;
    snapshot->point_smooth_hint = point_smooth_hint;
    snapshot->line_smooth_hint = line_smooth_hint;
    snapshot->polygon_smooth_hint = polygon_smooth_hint;
    memcpy(snapshot->eval_maps1, eval_maps1, sizeof(eval_maps1));
    memcpy(snapshot->eval_maps2, eval_maps2, sizeof(eval_maps2));
    snapshot->auto_normal_enabled = auto_normal_enabled;
    snapshot->map_grid1_n = map_grid1_n;
    snapshot->map_grid1_u1 = map_grid1_u1;
    snapshot->map_grid1_u2 = map_grid1_u2;
    snapshot->map_grid2_un = map_grid2_un;
    snapshot->map_grid2_vn = map_grid2_vn;
    snapshot->map_grid2_u1 = map_grid2_u1;
    snapshot->map_grid2_u2 = map_grid2_u2;
    snapshot->map_grid2_v1 = map_grid2_v1;
    snapshot->map_grid2_v2 = map_grid2_v2;
    snapshot->list_base = list_base;
    snapshot->active_texture = active_texture;
    memcpy(snapshot->texture_1d_enabled, texture_1d_enabled, sizeof(texture_1d_enabled));
    memcpy(snapshot->texture_2d_enabled, texture_2d_enabled, sizeof(texture_2d_enabled));
    memcpy(snapshot->texture_3d_enabled, texture_3d_enabled, sizeof(texture_3d_enabled));
    memcpy(snapshot->texture_cube_map_enabled, texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
    memcpy(snapshot->texture_binding_1d, texture_binding_1d, sizeof(texture_binding_1d));
    memcpy(snapshot->texture_binding_2d, texture_binding_2d, sizeof(texture_binding_2d));
    memcpy(snapshot->texture_binding_3d, texture_binding_3d, sizeof(texture_binding_3d));
    memcpy(snapshot->texture_binding_cube_map, texture_binding_cube_map, sizeof(texture_binding_cube_map));
    memcpy(snapshot->texgen_state, texgen_state, sizeof(texgen_state));
    memcpy(snapshot->texture_env_mode, texture_env_mode, sizeof(texture_env_mode));
    memcpy(snapshot->texture_env_color, texture_env_color, sizeof(texture_env_color));
    memcpy(snapshot->texture_combine_rgb, texture_combine_rgb, sizeof(texture_combine_rgb));
    memcpy(snapshot->texture_combine_alpha, texture_combine_alpha, sizeof(texture_combine_alpha));
    memcpy(snapshot->texture_source_rgb, texture_source_rgb, sizeof(texture_source_rgb));
    memcpy(snapshot->texture_source_alpha, texture_source_alpha, sizeof(texture_source_alpha));
    memcpy(snapshot->texture_operand_rgb, texture_operand_rgb, sizeof(texture_operand_rgb));
    memcpy(snapshot->texture_operand_alpha, texture_operand_alpha, sizeof(texture_operand_alpha));
    memcpy(snapshot->texture_rgb_scale, texture_rgb_scale, sizeof(texture_rgb_scale));
    memcpy(snapshot->texture_alpha_scale, texture_alpha_scale, sizeof(texture_alpha_scale));
}

static void restore_attrib_snapshot(const AttribSnapshot *snapshot)
{
    GLbitfield mask = snapshot->mask;

    if ((mask & GL_CURRENT_BIT) != 0) {
        current_color = snapshot->current_color;
        current_normal = snapshot->current_normal;
        memcpy(current_u, snapshot->current_u, sizeof(current_u));
        memcpy(current_v, snapshot->current_v, sizeof(current_v));
        memcpy(current_r, snapshot->current_r, sizeof(current_r));
        current_edge_flag = snapshot->current_edge_flag;
        memcpy(current_raster_position, snapshot->current_raster_position, sizeof(current_raster_position));
        current_raster_position_valid = snapshot->current_raster_position_valid;
        apply_color_material(current_color);
    }
    if ((mask & GL_POINT_BIT) != 0) {
        point_size = snapshot->point_size;
    }
    if ((mask & GL_LINE_BIT) != 0) {
        line_width = snapshot->line_width;
        line_stipple_factor = snapshot->line_stipple_factor;
        line_stipple_pattern = snapshot->line_stipple_pattern;
        line_stipple_enabled = snapshot->line_stipple_enabled;
    }
    if ((mask & GL_POLYGON_BIT) != 0) {
        polygon_mode = snapshot->polygon_mode;
        polygon_offset_point_enabled = snapshot->polygon_offset_point_enabled;
        polygon_offset_line_enabled = snapshot->polygon_offset_line_enabled;
        polygon_offset_fill_enabled = snapshot->polygon_offset_fill_enabled;
        polygon_offset_factor = snapshot->polygon_offset_factor;
        polygon_offset_units = snapshot->polygon_offset_units;
        cull_enabled = snapshot->cull_enabled;
        cull_face_mode = snapshot->cull_face_mode;
        front_face_mode = snapshot->front_face_mode;
    }
    if ((mask & GL_POLYGON_STIPPLE_BIT) != 0) {
        memcpy(polygon_stipple_pattern, snapshot->polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
        polygon_stipple_enabled = snapshot->polygon_stipple_enabled;
    }
    if ((mask & GL_PIXEL_MODE_BIT) != 0) {
        pixel_zoom_x = snapshot->pixel_zoom_x;
        pixel_zoom_y = snapshot->pixel_zoom_y;
        memcpy(pixel_transfer_scale, snapshot->pixel_transfer_scale, sizeof(pixel_transfer_scale));
        memcpy(pixel_transfer_bias, snapshot->pixel_transfer_bias, sizeof(pixel_transfer_bias));
        memcpy(pixel_maps, snapshot->pixel_maps, sizeof(pixel_maps));
    }
    if ((mask & GL_LIGHTING_BIT) != 0) {
        lighting_enabled = snapshot->lighting_enabled;
        memcpy(lights, snapshot->lights, sizeof(lights));
        shade_model = snapshot->shade_model;
        light_model_local_viewer = snapshot->light_model_local_viewer;
        light_model_two_side = snapshot->light_model_two_side;
        light_model_color_control = snapshot->light_model_color_control;
        light_model_ambient = snapshot->light_model_ambient;
        color_material_enabled = snapshot->color_material_enabled;
        color_material_face = snapshot->color_material_face;
        color_material_parameter = snapshot->color_material_parameter;
        material_state = snapshot->material_state;
        material_back_state = snapshot->material_back_state;
    }
    if ((mask & GL_FOG_BIT) != 0) {
        fog_enabled = snapshot->fog_enabled;
        fog_mode = snapshot->fog_mode;
        fog_hint = snapshot->fog_hint;
        fog_color = snapshot->fog_color;
        fog_density = snapshot->fog_density;
        fog_start = snapshot->fog_start;
        fog_end = snapshot->fog_end;
    }
    if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
        depth_test_enabled = snapshot->depth_test_enabled;
        depth_write_enabled = snapshot->depth_write_enabled;
        depth_clear_value = snapshot->depth_clear_value;
        depth_range_near = snapshot->depth_range_near;
        depth_range_far = snapshot->depth_range_far;
        depth_func = snapshot->depth_func;
    }
    if ((mask & GL_ACCUM_BUFFER_BIT) != 0) {
        memcpy(accum_clear_value, snapshot->accum_clear_value, sizeof(accum_clear_value));
    }
    if ((mask & GL_STENCIL_BUFFER_BIT) != 0) {
        stencil_test_enabled = snapshot->stencil_test_enabled;
        stencil_clear_value = snapshot->stencil_clear_value;
        stencil_func = snapshot->stencil_func;
        stencil_ref = snapshot->stencil_ref;
        stencil_value_mask = snapshot->stencil_value_mask;
        stencil_write_mask = snapshot->stencil_write_mask;
        stencil_fail = snapshot->stencil_fail;
        stencil_zfail = snapshot->stencil_zfail;
        stencil_zpass = snapshot->stencil_zpass;
    }
    if ((mask & GL_VIEWPORT_BIT) != 0) {
        memcpy(viewport, snapshot->viewport, sizeof(viewport));
    }
    if ((mask & GL_SCISSOR_BIT) != 0) {
        memcpy(scissor_box, snapshot->scissor_box, sizeof(scissor_box));
        scissor_test_enabled = snapshot->scissor_test_enabled;
    }
    if ((mask & GL_TRANSFORM_BIT) != 0) {
        matrix_mode = snapshot->matrix_mode;
        normalize_enabled = snapshot->normalize_enabled;
        rescale_normal_enabled = snapshot->rescale_normal_enabled;
        memcpy(clip_planes, snapshot->clip_planes, sizeof(clip_planes));
    }
    if ((mask & GL_ENABLE_BIT) != 0) {
        blend_enabled = snapshot->blend_enabled;
        depth_test_enabled = snapshot->depth_test_enabled;
        cull_enabled = snapshot->cull_enabled;
        scissor_test_enabled = snapshot->scissor_test_enabled;
        stencil_test_enabled = snapshot->stencil_test_enabled;
        alpha_test_enabled = snapshot->alpha_test_enabled;
        color_logic_op_enabled = snapshot->color_logic_op_enabled;
        multisample_enabled = snapshot->multisample_enabled;
        sample_alpha_to_coverage_enabled = snapshot->sample_alpha_to_coverage_enabled;
        sample_alpha_to_one_enabled = snapshot->sample_alpha_to_one_enabled;
        sample_coverage_enabled = snapshot->sample_coverage_enabled;
        line_stipple_enabled = snapshot->line_stipple_enabled;
        polygon_stipple_enabled = snapshot->polygon_stipple_enabled;
        polygon_offset_point_enabled = snapshot->polygon_offset_point_enabled;
        polygon_offset_line_enabled = snapshot->polygon_offset_line_enabled;
        polygon_offset_fill_enabled = snapshot->polygon_offset_fill_enabled;
        lighting_enabled = snapshot->lighting_enabled;
        fog_enabled = snapshot->fog_enabled;
        color_material_enabled = snapshot->color_material_enabled;
        normalize_enabled = snapshot->normalize_enabled;
        rescale_normal_enabled = snapshot->rescale_normal_enabled;
        auto_normal_enabled = snapshot->auto_normal_enabled;
        for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
            clip_planes[i].enabled = snapshot->clip_planes[i].enabled;
        }
        memcpy(lights, snapshot->lights, sizeof(lights));
        memcpy(texture_1d_enabled, snapshot->texture_1d_enabled, sizeof(texture_1d_enabled));
        memcpy(texture_2d_enabled, snapshot->texture_2d_enabled, sizeof(texture_2d_enabled));
        memcpy(texture_3d_enabled, snapshot->texture_3d_enabled, sizeof(texture_3d_enabled));
        memcpy(texture_cube_map_enabled, snapshot->texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
        memcpy(texgen_state, snapshot->texgen_state, sizeof(texgen_state));
        update_texgen_enabled_cache();
        for (int i = 0; i < NXGL_EVAL_MAP_COUNT; ++i) {
            eval_maps1[i].enabled = snapshot->eval_maps1[i].enabled;
            eval_maps2[i].enabled = snapshot->eval_maps2[i].enabled;
        }
    }
    if ((mask & GL_COLOR_BUFFER_BIT) != 0) {
        blend_enabled = snapshot->blend_enabled;
        blend_sfactor = snapshot->blend_sfactor;
        blend_dfactor = snapshot->blend_dfactor;
        alpha_test_enabled = snapshot->alpha_test_enabled;
        alpha_test_func = snapshot->alpha_test_func;
        alpha_test_ref = snapshot->alpha_test_ref;
        memcpy(color_write_mask, snapshot->color_write_mask, sizeof(color_write_mask));
        color_logic_op_enabled = snapshot->color_logic_op_enabled;
        logic_op_mode = snapshot->logic_op_mode;
        multisample_enabled = snapshot->multisample_enabled;
        sample_alpha_to_coverage_enabled = snapshot->sample_alpha_to_coverage_enabled;
        sample_alpha_to_one_enabled = snapshot->sample_alpha_to_one_enabled;
        sample_coverage_enabled = snapshot->sample_coverage_enabled;
        sample_coverage_value = snapshot->sample_coverage_value;
        sample_coverage_invert = snapshot->sample_coverage_invert;
        draw_buffer_mode = snapshot->draw_buffer_mode;
        read_buffer_mode = snapshot->read_buffer_mode;
    }
    if ((mask & GL_HINT_BIT) != 0) {
        perspective_correction_hint = snapshot->perspective_correction_hint;
        point_smooth_hint = snapshot->point_smooth_hint;
        line_smooth_hint = snapshot->line_smooth_hint;
        polygon_smooth_hint = snapshot->polygon_smooth_hint;
        fog_hint = snapshot->fog_hint;
    }
    if ((mask & GL_EVAL_BIT) != 0) {
        memcpy(eval_maps1, snapshot->eval_maps1, sizeof(eval_maps1));
        memcpy(eval_maps2, snapshot->eval_maps2, sizeof(eval_maps2));
        auto_normal_enabled = snapshot->auto_normal_enabled;
        map_grid1_n = snapshot->map_grid1_n;
        map_grid1_u1 = snapshot->map_grid1_u1;
        map_grid1_u2 = snapshot->map_grid1_u2;
        map_grid2_un = snapshot->map_grid2_un;
        map_grid2_vn = snapshot->map_grid2_vn;
        map_grid2_u1 = snapshot->map_grid2_u1;
        map_grid2_u2 = snapshot->map_grid2_u2;
        map_grid2_v1 = snapshot->map_grid2_v1;
        map_grid2_v2 = snapshot->map_grid2_v2;
    }
    if ((mask & GL_LIST_BIT) != 0) {
        list_base = snapshot->list_base;
    }
    if ((mask & GL_TEXTURE_BIT) != 0) {
        active_texture = snapshot->active_texture;
        memcpy(texture_1d_enabled, snapshot->texture_1d_enabled, sizeof(texture_1d_enabled));
        memcpy(texture_2d_enabled, snapshot->texture_2d_enabled, sizeof(texture_2d_enabled));
        memcpy(texture_3d_enabled, snapshot->texture_3d_enabled, sizeof(texture_3d_enabled));
        memcpy(texture_cube_map_enabled, snapshot->texture_cube_map_enabled, sizeof(texture_cube_map_enabled));
        memcpy(texture_binding_1d, snapshot->texture_binding_1d, sizeof(texture_binding_1d));
        memcpy(texture_binding_2d, snapshot->texture_binding_2d, sizeof(texture_binding_2d));
        memcpy(texture_binding_3d, snapshot->texture_binding_3d, sizeof(texture_binding_3d));
        memcpy(texture_binding_cube_map, snapshot->texture_binding_cube_map, sizeof(texture_binding_cube_map));
        memcpy(texgen_state, snapshot->texgen_state, sizeof(texgen_state));
        update_texgen_enabled_cache();
        memcpy(texture_env_mode, snapshot->texture_env_mode, sizeof(texture_env_mode));
        memcpy(texture_env_color, snapshot->texture_env_color, sizeof(texture_env_color));
        memcpy(texture_combine_rgb, snapshot->texture_combine_rgb, sizeof(texture_combine_rgb));
        memcpy(texture_combine_alpha, snapshot->texture_combine_alpha, sizeof(texture_combine_alpha));
        memcpy(texture_source_rgb, snapshot->texture_source_rgb, sizeof(texture_source_rgb));
        memcpy(texture_source_alpha, snapshot->texture_source_alpha, sizeof(texture_source_alpha));
        memcpy(texture_operand_rgb, snapshot->texture_operand_rgb, sizeof(texture_operand_rgb));
        memcpy(texture_operand_alpha, snapshot->texture_operand_alpha, sizeof(texture_operand_alpha));
        memcpy(texture_rgb_scale, snapshot->texture_rgb_scale, sizeof(texture_rgb_scale));
        memcpy(texture_alpha_scale, snapshot->texture_alpha_scale, sizeof(texture_alpha_scale));
    }
    sync_native_state();
}

static void capture_client_attrib_snapshot(ClientAttribSnapshot *snapshot, GLbitfield mask)
{
    snapshot->mask = mask;
    snapshot->client_active_texture = client_active_texture;
    snapshot->vertex_array = vertex_array;
    snapshot->color_array = color_array;
    memcpy(snapshot->texcoord_array, texcoord_array, sizeof(texcoord_array));
    snapshot->normal_array = normal_array;
    snapshot->pack_alignment = pack_alignment;
    snapshot->pack_row_length = pack_row_length;
    snapshot->pack_skip_rows = pack_skip_rows;
    snapshot->pack_skip_pixels = pack_skip_pixels;
    snapshot->pack_image_height = pack_image_height;
    snapshot->pack_skip_images = pack_skip_images;
    snapshot->pack_swap_bytes = pack_swap_bytes;
    snapshot->pack_lsb_first = pack_lsb_first;
    snapshot->unpack_alignment = unpack_alignment;
    snapshot->unpack_row_length = unpack_row_length;
    snapshot->unpack_skip_rows = unpack_skip_rows;
    snapshot->unpack_skip_pixels = unpack_skip_pixels;
    snapshot->unpack_image_height = unpack_image_height;
    snapshot->unpack_skip_images = unpack_skip_images;
    snapshot->unpack_swap_bytes = unpack_swap_bytes;
    snapshot->unpack_lsb_first = unpack_lsb_first;
}

static void restore_client_attrib_snapshot(const ClientAttribSnapshot *snapshot)
{
    GLbitfield mask = snapshot->mask;

    if ((mask & GL_CLIENT_VERTEX_ARRAY_BIT) != 0) {
        client_active_texture = snapshot->client_active_texture;
        vertex_array = snapshot->vertex_array;
        color_array = snapshot->color_array;
        memcpy(texcoord_array, snapshot->texcoord_array, sizeof(texcoord_array));
        normal_array = snapshot->normal_array;
    }
    if ((mask & GL_CLIENT_PIXEL_STORE_BIT) != 0) {
        pack_alignment = snapshot->pack_alignment;
        pack_row_length = snapshot->pack_row_length;
        pack_skip_rows = snapshot->pack_skip_rows;
        pack_skip_pixels = snapshot->pack_skip_pixels;
        pack_image_height = snapshot->pack_image_height;
        pack_skip_images = snapshot->pack_skip_images;
        pack_swap_bytes = snapshot->pack_swap_bytes;
        pack_lsb_first = snapshot->pack_lsb_first;
        unpack_alignment = snapshot->unpack_alignment;
        unpack_row_length = snapshot->unpack_row_length;
        unpack_skip_rows = snapshot->unpack_skip_rows;
        unpack_skip_pixels = snapshot->unpack_skip_pixels;
        unpack_image_height = snapshot->unpack_image_height;
        unpack_skip_images = snapshot->unpack_skip_images;
        unpack_swap_bytes = snapshot->unpack_swap_bytes;
        unpack_lsb_first = snapshot->unpack_lsb_first;
    }
}

static void clear_display_list(DisplayList *list)
{
    for (size_t i = 0; i < list->count; ++i) {
        free(list->commands[i].data);
        list->commands[i].data = NULL;
        list->commands[i].data_size = 0;
    }
    free(list->commands);
    list->commands = NULL;
    list->count = 0;
    list->capacity = 0;
    list->allocated = false;
}

static bool is_recording(void)
{
    return recording_list != 0 && !replaying_list;
}

static bool compile_only(void)
{
    return is_recording() && recording_mode == GL_COMPILE;
}

static bool reject_inside_begin(void)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return true;
    }
    return false;
}

static void record_command(ListCommand command)
{
    DisplayList *list;
    ListCommand *grown;
    size_t new_capacity;

    if (!is_recording() || recording_list >= 256) {
        free(command.data);
        return;
    }

    list = &display_lists[recording_list];
    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        grown = (ListCommand *)realloc(list->commands, new_capacity * sizeof(ListCommand));
        if (grown == NULL) {
            free(command.data);
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        list->commands = grown;
        list->capacity = new_capacity;
    }
    list->commands[list->count++] = command;
}

static bool attach_command_data(ListCommand *command, const void *data, size_t size)
{
    if (size == 0) {
        return true;
    }
    command->data = malloc(size);
    if (command->data == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return false;
    }
    memcpy(command->data, data, size);
    command->data_size = size;
    return true;
}

static int texture_unit_index(GLenum texture)
{
    if (texture < GL_TEXTURE0 || texture > GL_TEXTURE3) {
        return -1;
    }
    return (int)(texture - GL_TEXTURE0);
}

static int pixel_map_index(GLenum map)
{
    if (map >= GL_PIXEL_MAP_I_TO_I && map <= GL_PIXEL_MAP_A_TO_A) {
        return (int)(map - GL_PIXEL_MAP_I_TO_I);
    }
    return -1;
}

static int eval_map_components(GLenum target)
{
    switch (target) {
    case GL_MAP1_INDEX:
    case GL_MAP2_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_1:
        return 1;
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_2:
        return 2;
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_VERTEX_3:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_VERTEX_3:
        return 3;
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_4:
        return 4;
    default:
        return 0;
    }
}

static int eval_map_index(GLenum target, bool *map2)
{
    if (map2 != NULL) {
        *map2 = false;
    }
    switch (target) {
    case GL_MAP1_COLOR_4: return 0;
    case GL_MAP1_NORMAL: return 1;
    case GL_MAP1_TEXTURE_COORD_1: return 2;
    case GL_MAP1_TEXTURE_COORD_2: return 3;
    case GL_MAP1_TEXTURE_COORD_3: return 4;
    case GL_MAP1_TEXTURE_COORD_4: return 5;
    case GL_MAP1_VERTEX_3: return 6;
    case GL_MAP1_VERTEX_4: return 7;
    case GL_MAP1_INDEX: return 8;
    case GL_MAP2_COLOR_4: if (map2 != NULL) *map2 = true; return 0;
    case GL_MAP2_NORMAL: if (map2 != NULL) *map2 = true; return 1;
    case GL_MAP2_TEXTURE_COORD_1: if (map2 != NULL) *map2 = true; return 2;
    case GL_MAP2_TEXTURE_COORD_2: if (map2 != NULL) *map2 = true; return 3;
    case GL_MAP2_TEXTURE_COORD_3: if (map2 != NULL) *map2 = true; return 4;
    case GL_MAP2_TEXTURE_COORD_4: if (map2 != NULL) *map2 = true; return 5;
    case GL_MAP2_VERTEX_3: if (map2 != NULL) *map2 = true; return 6;
    case GL_MAP2_VERTEX_4: if (map2 != NULL) *map2 = true; return 7;
    case GL_MAP2_INDEX: if (map2 != NULL) *map2 = true; return 8;
    default: return -1;
    }
}

static void init_eval_maps(void)
{
    static const GLenum map1_targets[NXGL_EVAL_MAP_COUNT] = {
        GL_MAP1_COLOR_4, GL_MAP1_NORMAL, GL_MAP1_TEXTURE_COORD_1, GL_MAP1_TEXTURE_COORD_2,
        GL_MAP1_TEXTURE_COORD_3, GL_MAP1_TEXTURE_COORD_4, GL_MAP1_VERTEX_3, GL_MAP1_VERTEX_4, GL_MAP1_INDEX
    };
    static const GLenum map2_targets[NXGL_EVAL_MAP_COUNT] = {
        GL_MAP2_COLOR_4, GL_MAP2_NORMAL, GL_MAP2_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_2,
        GL_MAP2_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_4, GL_MAP2_VERTEX_3, GL_MAP2_VERTEX_4, GL_MAP2_INDEX
    };

    memset(eval_maps1, 0, sizeof(eval_maps1));
    memset(eval_maps2, 0, sizeof(eval_maps2));
    for (int i = 0; i < NXGL_EVAL_MAP_COUNT; ++i) {
        eval_maps1[i].target = map1_targets[i];
        eval_maps1[i].components = eval_map_components(map1_targets[i]);
        eval_maps1[i].u2 = 1.0f;
        eval_maps2[i].target = map2_targets[i];
        eval_maps2[i].components = eval_map_components(map2_targets[i]);
        eval_maps2[i].u2 = 1.0f;
        eval_maps2[i].v2 = 1.0f;
    }
}

static GLenum pixel_map_size_pname(GLenum map)
{
    int index = pixel_map_index(map);
    return index >= 0 ? (GLenum)(GL_PIXEL_MAP_I_TO_I_SIZE + index) : 0;
}

static int pixel_map_index_from_size_pname(GLenum pname)
{
    if (pname >= GL_PIXEL_MAP_I_TO_I_SIZE && pname <= GL_PIXEL_MAP_A_TO_A_SIZE) {
        return (int)(pname - GL_PIXEL_MAP_I_TO_I_SIZE);
    }
    return -1;
}

static int active_texture_index(void)
{
    return texture_unit_index(active_texture);
}

static int client_texture_index(void)
{
    return texture_unit_index(client_active_texture);
}

static int texgen_coord_index(GLenum coord)
{
    switch (coord) {
    case GL_S: return 0;
    case GL_T: return 1;
    case GL_R: return 2;
    case GL_Q: return 3;
    default: return -1;
    }
}

static GLenum texgen_cap_from_index(int index)
{
    static const GLenum caps[4] = { GL_TEXTURE_GEN_S, GL_TEXTURE_GEN_T, GL_TEXTURE_GEN_R, GL_TEXTURE_GEN_Q };
    return index >= 0 && index < 4 ? caps[index] : 0;
}

static int texgen_index_from_cap(GLenum cap)
{
    switch (cap) {
    case GL_TEXTURE_GEN_S: return 0;
    case GL_TEXTURE_GEN_T: return 1;
    case GL_TEXTURE_GEN_R: return 2;
    case GL_TEXTURE_GEN_Q: return 3;
    default: return -1;
    }
}

static bool valid_texgen_mode(GLenum coord, GLenum mode)
{
    if (mode == GL_OBJECT_LINEAR || mode == GL_EYE_LINEAR) {
        return true;
    }
    if (mode == GL_SPHERE_MAP) {
        return coord == GL_S || coord == GL_T;
    }
    return false;
}

static void init_texgen_state(void)
{
    texgen_any_enabled = false;
    for (int unit = 0; unit < 4; ++unit) {
        for (int coord = 0; coord < 4; ++coord) {
            TexGenState *state = &texgen_state[unit][coord];
            memset(state, 0, sizeof(*state));
            state->mode = GL_EYE_LINEAR;
            state->object_plane[coord] = 1.0f;
            state->eye_plane[coord] = 1.0f;
        }
    }
}

static bool valid_texture_env_mode(GLint mode)
{
    return mode == GL_MODULATE ||
           mode == GL_DECAL ||
           mode == GL_BLEND ||
           mode == GL_REPLACE ||
           mode == GL_COMBINE ||
           mode == GL_ADD;
}

static bool valid_combine_mode(GLint mode)
{
    return mode == GL_REPLACE ||
           mode == GL_MODULATE ||
           mode == GL_ADD ||
           mode == GL_ADD_SIGNED ||
           mode == GL_INTERPOLATE ||
           mode == GL_SUBTRACT ||
           mode == GL_DOT3_RGB ||
           mode == GL_DOT3_RGBA;
}

static bool valid_combine_source(GLint source)
{
    return source == GL_TEXTURE ||
           source == GL_CONSTANT ||
           source == GL_PRIMARY_COLOR ||
           source == GL_PREVIOUS ||
           (source >= GL_TEXTURE0 && source <= GL_TEXTURE3);
}

static bool valid_combine_rgb_operand(GLint operand)
{
    return operand == GL_SRC_COLOR ||
           operand == GL_ONE_MINUS_SRC_COLOR ||
           operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_ALPHA;
}

static bool valid_combine_alpha_operand(GLint operand)
{
    return operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_ALPHA;
}

static bool valid_combine_scale(GLfloat scale)
{
    return scale == 1.0f || scale == 2.0f || scale == 4.0f;
}

static int light_index(GLenum light)
{
    if (light < GL_LIGHT0 || light > GL_LIGHT7) {
        return -1;
    }
    return (int)(light - GL_LIGHT0);
}

static void init_texture_object(TextureObject *texture)
{
    memset(texture, 0, sizeof(*texture));
    texture->min_filter = GL_NEAREST_MIPMAP_LINEAR;
    texture->mag_filter = GL_LINEAR;
    texture->wrap_s = GL_REPEAT;
    texture->wrap_t = GL_REPEAT;
    texture->wrap_r = GL_REPEAT;
    texture->min_lod = -1000.0f;
    texture->max_lod = 1000.0f;
    texture->lod_bias = 0.0f;
    texture->priority = 1.0f;
    texture->base_level = 0;
    texture->max_level = 1000;
}

static void init_light_object(LightObject *light, int index)
{
    light->enabled = false;
    light->ambient = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 1.0f };
    light->diffuse = (NxglBackendColor){ index == 0 ? 1.0f : 0.0f, index == 0 ? 1.0f : 0.0f, index == 0 ? 1.0f : 0.0f, 1.0f };
    light->specular = light->diffuse;
    light->position[0] = 0.0f;
    light->position[1] = 0.0f;
    light->position[2] = 1.0f;
    light->position[3] = 0.0f;
    light->spot_direction[0] = 0.0f;
    light->spot_direction[1] = 0.0f;
    light->spot_direction[2] = -1.0f;
    light->spot_exponent = 0.0f;
    light->spot_cutoff = 180.0f;
    light->constant_attenuation = 1.0f;
    light->linear_attenuation = 0.0f;
    light->quadratic_attenuation = 0.0f;
}

static int cube_face_index(GLenum target)
{
    if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
        return (int)(target - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    }
    return -1;
}

static bool valid_texture_binding_target(GLenum target)
{
    return target == GL_TEXTURE_1D || target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP;
}

static bool valid_texture_image_target(GLenum target)
{
    return target == GL_TEXTURE_1D || target == GL_TEXTURE_2D || target == GL_TEXTURE_3D || cube_face_index(target) >= 0;
}

static bool valid_texture_2d_image_target(GLenum target)
{
    return target == GL_TEXTURE_2D || cube_face_index(target) >= 0;
}

static bool valid_compressed_texture_format(GLenum format)
{
    return format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
           format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
}

static int compressed_block_size(GLenum format)
{
    if (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
        return 8;
    }
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
        return 16;
    }
    return 0;
}

static GLsizei compressed_image_size(GLsizei width, GLsizei height, GLenum format)
{
    int block_size = compressed_block_size(format);
    GLsizei block_w;
    GLsizei block_h;

    if (width <= 0 || height <= 0 || block_size == 0) {
        return 0;
    }
    block_w = (width + 3) / 4;
    block_h = (height + 3) / 4;
    return block_w * block_h * block_size;
}

static GLsizei compressed_image_size_3d(GLsizei width, GLsizei height, GLsizei depth, GLenum format)
{
    GLsizei slice_size;

    if (depth <= 0) {
        return 0;
    }
    slice_size = compressed_image_size(width, height, format);
    if (slice_size <= 0) {
        return 0;
    }
    return slice_size * depth;
}

static void destroy_texture_level_image(TextureLevel *level)
{
    free(level->rgba);
    free(level->compressed_data);
    level->rgba = NULL;
    level->compressed_data = NULL;
    level->defined = false;
    level->compressed = false;
    level->internal_format = 0;
    level->width = 0;
    level->height = 0;
    level->depth = 0;
    level->compressed_size = 0;
}

static void destroy_texture_level(TextureObject *texture, GLenum target, int level)
{
    int face = cube_face_index(target);

    if (target == GL_TEXTURE_1D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native_1d);
        memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    } else if (target == GL_TEXTURE_2D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native);
        memset(&texture->native, 0, sizeof(texture->native));
    } else if (target == GL_TEXTURE_3D && level == 0) {
        nxgl_backend_texture_destroy(&texture->native_3d);
        memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    }
    if (target == GL_TEXTURE_1D) {
        destroy_texture_level_image(&texture->levels_1d[level]);
    } else if (target == GL_TEXTURE_2D) {
        destroy_texture_level_image(&texture->levels[level]);
    } else if (target == GL_TEXTURE_3D) {
        destroy_texture_level_image(&texture->levels_3d[level]);
    } else if (face >= 0) {
        destroy_texture_level_image(&texture->cube_faces[face][level]);
    }
}

static void destroy_texture_image(TextureObject *texture)
{
    nxgl_backend_texture_destroy(&texture->native);
    memset(&texture->native, 0, sizeof(texture->native));
    nxgl_backend_texture_destroy(&texture->native_1d);
    memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    nxgl_backend_texture_destroy(&texture->native_3d);
    memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    nxgl_backend_texture_destroy(&texture->native_cube);
    memset(&texture->native_cube, 0, sizeof(texture->native_cube));
    for (int level = 0; level < NXGL_MAX_TEXTURE_LEVELS; ++level) {
        destroy_texture_level_image(&texture->levels[level]);
        destroy_texture_level_image(&texture->levels_1d[level]);
        destroy_texture_level_image(&texture->levels_3d[level]);
        for (int face = 0; face < 6; ++face) {
            destroy_texture_level_image(&texture->cube_faces[face][level]);
        }
    }
}

static TextureObject *bound_texture_object_for_target(GLenum target)
{
    int unit = active_texture_index();
    GLuint id = 0;

    if (unit < 0) {
        return NULL;
    }
    if (target == GL_TEXTURE_1D) {
        id = texture_binding_1d[unit];
    } else if (target == GL_TEXTURE_2D) {
        id = texture_binding_2d[unit];
    } else if (target == GL_TEXTURE_3D) {
        id = texture_binding_3d[unit];
    } else if (target == GL_TEXTURE_CUBE_MAP || cube_face_index(target) >= 0) {
        id = texture_binding_cube_map[unit];
    }

    if (id == 0 || id >= 16) {
        return NULL;
    }
    return &texture_objects[id];
}

static TextureObject *bound_texture_object(void)
{
    return bound_texture_object_for_target(GL_TEXTURE_2D);
}

static TextureLevel *texture_level_for_target(TextureObject *texture, GLenum target, GLint level)
{
    int face = cube_face_index(target);

    if (texture == NULL || level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        return NULL;
    }
    if (target == GL_TEXTURE_1D) {
        return &texture->levels_1d[level];
    }
    if (target == GL_TEXTURE_2D) {
        return &texture->levels[level];
    }
    if (target == GL_TEXTURE_3D) {
        return &texture->levels_3d[level];
    }
    if (face >= 0) {
        return &texture->cube_faces[face][level];
    }
    return NULL;
}

static TextureLevel *select_texture_level_for_lod(TextureObject *texture, TextureLevel *levels, GLfloat lambda)
{
    GLint base;
    GLint max;
    GLint end;
    GLint selected;
    GLfloat lod;

    if (texture == NULL || levels == NULL) {
        return NULL;
    }
    base = texture->base_level;
    max = texture->max_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return NULL;
    }
    if (max >= NXGL_MAX_TEXTURE_LEVELS) {
        max = NXGL_MAX_TEXTURE_LEVELS - 1;
    }
    if (!levels[base].defined) {
        return NULL;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return &levels[base];
    }
    if (!texture_levels_complete(texture, levels)) {
        return NULL;
    }

    lod = lambda + texture->lod_bias;
    if (lod < texture->min_lod) {
        lod = texture->min_lod;
    }
    if (lod > texture->max_lod) {
        lod = texture->max_lod;
    }
    if (lod < 0.0f) {
        lod = 0.0f;
    }

    selected = base + (GLint)floorf(lod + 0.5f);
    end = mip_chain_end_level(levels[base].width, levels[base].height, levels[base].depth, base, max);
    if (selected < base) {
        selected = base;
    }
    if (selected > end) {
        selected = end;
    }
    if (selected >= NXGL_MAX_TEXTURE_LEVELS || !levels[selected].defined) {
        return NULL;
    }
    return &levels[selected];
}

static bool cube_base_complete(const TextureObject *texture)
{
    GLint base;
    GLsizei size;
    GLint internal_format;

    if (texture == NULL) {
        return false;
    }
    base = texture->base_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS) {
        return false;
    }

    if (!texture->cube_faces[0][base].defined || texture->cube_faces[0][base].width != texture->cube_faces[0][base].height) {
        return false;
    }

    size = texture->cube_faces[0][base].width;
    internal_format = texture->cube_faces[0][base].internal_format;
    for (int face = 0; face < 6; ++face) {
        const TextureLevel *image = &texture->cube_faces[face][base];
        if (!image->defined ||
            image->rgba == NULL ||
            image->width != size ||
            image->height != size ||
            image->internal_format != internal_format) {
            return false;
        }
    }
    return true;
}

static bool cube_levels_complete(const TextureObject *texture)
{
    GLint base;
    GLint max;
    GLint end;
    GLsizei expected_size;
    GLint internal_format;

    if (texture == NULL) {
        return false;
    }
    base = texture->base_level;
    max = texture->max_level;
    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return false;
    }
    if (max >= NXGL_MAX_TEXTURE_LEVELS) {
        max = NXGL_MAX_TEXTURE_LEVELS - 1;
    }
    if (!cube_base_complete(texture)) {
        return false;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return true;
    }

    expected_size = texture->cube_faces[0][base].width;
    internal_format = texture->cube_faces[0][base].internal_format;
    end = mip_chain_end_level(expected_size, expected_size, 1, base, max);
    for (GLint level = base; level <= end; ++level) {
        for (int face = 0; face < 6; ++face) {
            const TextureLevel *image = &texture->cube_faces[face][level];
            if (!image->defined ||
                image->rgba == NULL ||
                image->width != expected_size ||
                image->height != expected_size ||
                image->depth != 1 ||
                image->internal_format != internal_format) {
                return false;
            }
        }
        expected_size = expected_size > 1 ? expected_size / 2 : 1;
    }
    return true;
}

static GLint select_cube_level_for_lod(TextureObject *texture, GLfloat lambda)
{
    TextureLevel *selected;

    if (texture == NULL || !cube_levels_complete(texture)) {
        return -1;
    }
    selected = select_texture_level_for_lod(texture, texture->cube_faces[0], lambda);
    if (selected == NULL) {
        return -1;
    }
    return (GLint)(selected - texture->cube_faces[0]);
}

static bool rebuild_native_cube_texture(TextureObject *texture)
{
    const uint8_t *faces[6];
    GLint level;

    nxgl_backend_texture_destroy(&texture->native_cube);
    memset(&texture->native_cube, 0, sizeof(texture->native_cube));
    level = select_cube_level_for_lod(texture, 0.0f);
    if (level < 0) {
        return true;
    }

    for (int face = 0; face < 6; ++face) {
        faces[face] = texture->cube_faces[face][level].rgba;
    }
    return nxgl_backend_texture_create_cube_rgba(&texture->native_cube, (uint16_t)texture->cube_faces[0][level].width, faces) == 0;
}

static NxglBackendCompressedTextureFormat native_compressed_format(GLenum format)
{
    if (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
        return NXGL_BACKEND_COMPRESSED_DXT1;
    }
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
        return NXGL_BACKEND_COMPRESSED_DXT3;
    }
    return NXGL_BACKEND_COMPRESSED_DXT5;
}

static bool rebuild_native_texture2d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native);
    memset(&texture->native, 0, sizeof(texture->native));

    image = select_texture_level_for_lod(texture, texture->levels, 0.0f);
    if (image == NULL) {
        return true;
    }
    if (image->compressed) {
        if (image->compressed_data == NULL || image->compressed_size <= 0) {
            return true;
        }
        return nxgl_backend_texture_create_compressed(&texture->native,
                                            (uint16_t)image->width,
                                            (uint16_t)image->height,
                                            native_compressed_format((GLenum)image->internal_format),
                                            image->compressed_data,
                                            (uint32_t)image->compressed_size) == 0;
    }
    if (image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba(&texture->native, (uint16_t)image->width, (uint16_t)image->height, image->rgba) == 0;
}

static bool rebuild_native_compressed_texture(TextureObject *texture)
{
    return rebuild_native_texture2d(texture);
}

static bool rebuild_native_texture1d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native_1d);
    memset(&texture->native_1d, 0, sizeof(texture->native_1d));
    if (texture->base_level != 0 || is_mipmap_filter(texture->min_filter)) {
        return true;
    }
    image = &texture->levels_1d[0];
    if (image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba(&texture->native_1d, (uint16_t)image->width, 1, image->rgba) == 0;
}

static bool rebuild_native_texture3d(TextureObject *texture)
{
    TextureLevel *image;

    if (texture == NULL) {
        return true;
    }
    nxgl_backend_texture_destroy(&texture->native_3d);
    memset(&texture->native_3d, 0, sizeof(texture->native_3d));
    image = select_texture_level_for_lod(texture, texture->levels_3d, 0.0f);
    if (image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return true;
    }
    return nxgl_backend_texture_create_rgba3d(&texture->native_3d,
                                    (uint16_t)image->width,
                                    (uint16_t)image->height,
                                    (uint16_t)image->depth,
                                    image->rgba) == 0;
}

static int type_size(GLenum type)
{
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        return 4;
    default:
        return 0;
    }
}

static int get_component_count(GLenum pname)
{
    switch (pname) {
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
    case GL_TRANSPOSE_PROJECTION_MATRIX:
    case GL_TRANSPOSE_TEXTURE_MATRIX:
    case GL_TRANSPOSE_COLOR_MATRIX:
        return 16;
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_COMPRESSED_TEXTURE_FORMATS:
    case GL_COLOR_WRITEMASK:
    case GL_CURRENT_COLOR:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_CLEAR_COLOR:
    case GL_CURRENT_RASTER_POSITION:
    case GL_CURRENT_RASTER_COLOR:
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
    case GL_FOG_COLOR:
    case GL_ACCUM_CLEAR_VALUE:
    case GL_LIGHT_MODEL_AMBIENT:
        return 4;
    case GL_CURRENT_NORMAL:
        return 3;
    case GL_DEPTH_RANGE:
    case GL_POLYGON_MODE:
    case GL_POINT_SIZE_RANGE:
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        return 2;
    default:
        return 1;
    }
}

static bool is_float_or_integer_type(GLenum type)
{
    return type_size(type) != 0;
}

static GLsizei effective_stride(const ClientArray *array)
{
    GLsizei stride = array->stride;
    if (stride == 0) {
        stride = array->size * type_size(array->type);
    }
    return stride;
}

static size_t aligned_row_bytes(size_t bytes, GLint alignment)
{
    size_t align = alignment > 0 ? (size_t)alignment : 1;
    return (bytes + align - 1) & ~(align - 1);
}

static GLsizei storage_row_pixels(GLsizei width, GLint row_length, GLint skip_pixels)
{
    GLsizei row_pixels = row_length > 0 ? row_length : width;
    GLsizei needed = skip_pixels + width;
    return row_pixels < needed ? needed : row_pixels;
}

static size_t pixel_store_row_stride(GLsizei width, GLint row_length, GLint skip_pixels, size_t pixel_bytes, GLint alignment)
{
    if (width <= 0 || pixel_bytes == 0) {
        return 0;
    }
    return aligned_row_bytes((size_t)storage_row_pixels(width, row_length, skip_pixels) * pixel_bytes, alignment);
}

static size_t pixel_store_data_size(GLsizei width, GLsizei height, GLint row_length, GLint skip_rows, GLint skip_pixels, size_t pixel_bytes, GLint alignment)
{
    size_t stride;

    if (width <= 0 || height <= 0 || pixel_bytes == 0) {
        return 0;
    }
    stride = pixel_store_row_stride(width, row_length, skip_pixels, pixel_bytes, alignment);
    return (size_t)(skip_rows + height - 1) * stride + (size_t)(skip_pixels + width) * pixel_bytes;
}

static GLsizei storage_image_rows(GLsizei height, GLint image_height, GLint skip_rows)
{
    GLsizei rows = image_height > 0 ? image_height : height;
    GLsizei needed = skip_rows + height;
    return rows < needed ? needed : rows;
}

static size_t pixel_store_slice_stride(GLsizei width, GLsizei height, GLint row_length, GLint skip_rows, GLint skip_pixels, GLint image_height, size_t pixel_bytes, GLint alignment)
{
    size_t row_stride;

    if (width <= 0 || height <= 0 || pixel_bytes == 0) {
        return 0;
    }
    row_stride = pixel_store_row_stride(width, row_length, skip_pixels, pixel_bytes, alignment);
    return row_stride * (size_t)storage_image_rows(height, image_height, skip_rows);
}

static size_t unpack_color_pixel_bytes(GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);

    if (comps == 0 || elem_size == 0) {
        return 0;
    }
    return type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
}

static size_t bitmap_unpack_row_stride(GLsizei width)
{
    GLsizei row_pixels;

    if (width <= 0) {
        return 0;
    }
    row_pixels = storage_row_pixels(width, unpack_row_length, unpack_skip_pixels);
    return aligned_row_bytes((size_t)((row_pixels + 7) / 8), unpack_alignment);
}

static size_t pixel_source_data_size(GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    size_t pixel_bytes;

    if (width <= 0 || height <= 0) {
        return 0;
    }
    if (format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA) {
        pixel_bytes = unpack_color_pixel_bytes(format, type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else if (format == GL_COLOR_INDEX) {
        pixel_bytes = (size_t)type_size(type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else if (format == GL_DEPTH_COMPONENT || format == GL_STENCIL_INDEX) {
        pixel_bytes = (size_t)type_size(type);
        if (pixel_bytes == 0) {
            return 0;
        }
    } else {
        return 0;
    }
    return pixel_store_data_size(width, height, unpack_row_length, unpack_skip_rows, unpack_skip_pixels, pixel_bytes, unpack_alignment);
}

static size_t bitmap_source_data_size(GLsizei width, GLsizei height)
{
    size_t stride;
    GLsizei final_bit;

    if (width <= 0 || height <= 0) {
        return 0;
    }
    stride = bitmap_unpack_row_stride(width);
    final_bit = unpack_skip_pixels + width;
    return (size_t)(unpack_skip_rows + height - 1) * stride + (size_t)((final_bit + 7) / 8);
}

static GLfloat eval_normalized(GLfloat value, GLfloat a, GLfloat b)
{
    if (a == b) {
        return 0.0f;
    }
    return (value - a) / (b - a);
}

static GLfloat eval_lerp(GLfloat a, GLfloat b, GLfloat t)
{
    return a + (b - a) * t;
}

static void eval_curve_points(GLfloat *work, GLint order, GLint components, GLfloat t)
{
    for (GLint level = order - 1; level > 0; --level) {
        for (GLint i = 0; i < level; ++i) {
            for (GLint c = 0; c < components; ++c) {
                work[i * components + c] = eval_lerp(work[i * components + c], work[(i + 1) * components + c], t);
            }
        }
    }
}

static void eval_map1_value(const EvalMap1 *map, GLfloat u, GLfloat out[4])
{
    GLfloat work[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat t;

    memset(out, 0, sizeof(GLfloat) * 4u);
    if (map == NULL || !map->defined || map->order <= 0 || map->components <= 0) {
        return;
    }
    memcpy(work, map->points, (size_t)map->order * (size_t)map->components * sizeof(GLfloat));
    t = eval_normalized(u, map->u1, map->u2);
    eval_curve_points(work, map->order, map->components, t);
    for (GLint c = 0; c < map->components; ++c) {
        out[c] = work[c];
    }
}

static void eval_map2_value(const EvalMap2 *map, GLfloat u, GLfloat v, GLfloat out[4])
{
    GLfloat vwork[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat uwork[NXGL_MAX_EVAL_ORDER * 4];
    GLfloat tu;
    GLfloat tv;

    memset(out, 0, sizeof(GLfloat) * 4u);
    if (map == NULL || !map->defined || map->uorder <= 0 || map->vorder <= 0 || map->components <= 0) {
        return;
    }
    tu = eval_normalized(u, map->u1, map->u2);
    tv = eval_normalized(v, map->v1, map->v2);
    for (GLint j = 0; j < map->vorder; ++j) {
        for (GLint i = 0; i < map->uorder; ++i) {
            for (GLint c = 0; c < map->components; ++c) {
                uwork[i * map->components + c] = map->points[((size_t)j * (size_t)map->uorder + (size_t)i) * 4u + (size_t)c];
            }
        }
        eval_curve_points(uwork, map->uorder, map->components, tu);
        for (GLint c = 0; c < map->components; ++c) {
            vwork[j * map->components + c] = uwork[c];
        }
    }
    eval_curve_points(vwork, map->vorder, map->components, tv);
    for (GLint c = 0; c < map->components; ++c) {
        out[c] = vwork[c];
    }
}

static void apply_eval_map1(GLfloat u)
{
    GLfloat value[4];
    int unit = active_texture_index();

    if (eval_maps1[0].enabled && eval_maps1[0].defined) {
        eval_map1_value(&eval_maps1[0], u, value);
        glColor4f(value[0], value[1], value[2], value[3]);
    }
    if (eval_maps1[1].enabled && eval_maps1[1].defined) {
        eval_map1_value(&eval_maps1[1], u, value);
        glNormal3f(value[0], value[1], value[2]);
    }
    for (int i = 2; i <= 5; ++i) {
        if (eval_maps1[i].enabled && eval_maps1[i].defined) {
            eval_map1_value(&eval_maps1[i], u, value);
            if (unit >= 0) {
                current_u[unit] = value[0];
                current_v[unit] = eval_maps1[i].components > 1 ? value[1] : 0.0f;
                current_r[unit] = eval_maps1[i].components > 2 ? value[2] : 0.0f;
            }
            break;
        }
    }
    if (eval_maps1[7].enabled && eval_maps1[7].defined) {
        eval_map1_value(&eval_maps1[7], u, value);
        if (value[3] != 0.0f) {
            glVertex3f(value[0] / value[3], value[1] / value[3], value[2] / value[3]);
        } else {
            glVertex3f(value[0], value[1], value[2]);
        }
    } else if (eval_maps1[6].enabled && eval_maps1[6].defined) {
        eval_map1_value(&eval_maps1[6], u, value);
        glVertex3f(value[0], value[1], value[2]);
    }
}

static void apply_eval_map2(GLfloat u, GLfloat v)
{
    GLfloat value[4];
    int unit = active_texture_index();

    if (eval_maps2[0].enabled && eval_maps2[0].defined) {
        eval_map2_value(&eval_maps2[0], u, v, value);
        glColor4f(value[0], value[1], value[2], value[3]);
    }
    if (eval_maps2[1].enabled && eval_maps2[1].defined) {
        eval_map2_value(&eval_maps2[1], u, v, value);
        glNormal3f(value[0], value[1], value[2]);
    }
    for (int i = 2; i <= 5; ++i) {
        if (eval_maps2[i].enabled && eval_maps2[i].defined) {
            eval_map2_value(&eval_maps2[i], u, v, value);
            if (unit >= 0) {
                current_u[unit] = value[0];
                current_v[unit] = eval_maps2[i].components > 1 ? value[1] : 0.0f;
                current_r[unit] = eval_maps2[i].components > 2 ? value[2] : 0.0f;
            }
            break;
        }
    }
    if (eval_maps2[7].enabled && eval_maps2[7].defined) {
        eval_map2_value(&eval_maps2[7], u, v, value);
        if (value[3] != 0.0f) {
            glVertex3f(value[0] / value[3], value[1] / value[3], value[2] / value[3]);
        } else {
            glVertex3f(value[0], value[1], value[2]);
        }
    } else if (eval_maps2[6].enabled && eval_maps2[6].defined) {
        eval_map2_value(&eval_maps2[6], u, v, value);
        glVertex3f(value[0], value[1], value[2]);
    }
}

static bool copy_shadow_rgba(uint8_t *dst, GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (dst == NULL) {
        return false;
    }
    if (shadow_color_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return false;
    }

    nxgl_backend_flush();
    for (GLsizei row = 0; row < height; ++row) {
        int src_y = shadow_height - 1 - (y + row);
        const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
        uint8_t *dst_row = dst + (size_t)row * (size_t)width * 4u;
        for (GLsizei col = 0; col < width; ++col) {
            uint32_t pixel = src_row[col];
            uint8_t *out = dst_row + (size_t)col * 4u;
            out[0] = (uint8_t)((pixel >> 16) & 0xff);
            out[1] = (uint8_t)((pixel >> 8) & 0xff);
            out[2] = (uint8_t)(pixel & 0xff);
            out[3] = 255;
        }
    }
    return true;
}

static float read_component(const uint8_t *ptr, GLenum type, bool normalize)
{
    switch (type) {
    case GL_BYTE:
        return normalize ? (float)(*(const int8_t *)ptr) / 127.0f : (float)(*(const int8_t *)ptr);
    case GL_UNSIGNED_BYTE:
        return normalize ? (float)(*(const uint8_t *)ptr) / 255.0f : (float)(*(const uint8_t *)ptr);
    case GL_SHORT:
        return normalize ? (float)(*(const int16_t *)ptr) / 32767.0f : (float)(*(const int16_t *)ptr);
    case GL_UNSIGNED_SHORT:
        return normalize ? (float)(*(const uint16_t *)ptr) / 65535.0f : (float)(*(const uint16_t *)ptr);
    case GL_INT:
        return (float)(*(const int32_t *)ptr);
    case GL_UNSIGNED_INT:
        return (float)(*(const uint32_t *)ptr);
    case GL_FLOAT:
        return *(const float *)ptr;
    default:
        return 0.0f;
    }
}

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static float clamp_accum_clear(float value)
{
    if (value < -1.0f) {
        return -1.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static GLfloat normalized_glbyte(GLbyte value)
{
    return value < 0 ? (GLfloat)value / 128.0f : (GLfloat)value / 127.0f;
}

static GLfloat normalized_glshort(GLshort value)
{
    return value < 0 ? (GLfloat)value / 32768.0f : (GLfloat)value / 32767.0f;
}

static GLfloat normalized_glint(GLint value)
{
    return value < 0 ? (GLfloat)value / 2147483648.0f : (GLfloat)value / 2147483647.0f;
}

static GLfloat normalized_glubyte(GLubyte value)
{
    return (GLfloat)value / 255.0f;
}

static GLfloat normalized_glushort(GLushort value)
{
    return (GLfloat)value / 65535.0f;
}

static GLfloat normalized_gluint(GLuint value)
{
    return (GLfloat)value / 4294967295.0f;
}

static GLfloat map_depth_range(GLfloat normalized)
{
    GLfloat clamped = clamp01(normalized);
    return clamp01(depth_range_near + (depth_range_far - depth_range_near) * clamped);
}

static GLfloat eye_depth_normalized(NxglBackendVec3 pos)
{
    return clamp01((-(pos.z + camera_z)) / 100.0f);
}

static void viewport_top_left_bounds(int *min_x, int *min_y, int *max_x, int *max_y)
{
    int x1 = viewport[0];
    int y1 = shadow_height - (viewport[1] + viewport[3]);
    int x2 = viewport[0] + viewport[2] - 1;
    int y2 = shadow_height - viewport[1] - 1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= shadow_width) x2 = shadow_width - 1;
    if (y2 >= shadow_height) y2 = shadow_height - 1;

    *min_x = x1;
    *min_y = y1;
    *max_x = x2;
    *max_y = y2;
}

static NxglBackendVec3 normalize_vec3(NxglBackendVec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.00001f) {
        NxglBackendVec3 fallback = { 0.0f, 0.0f, 1.0f };
        return fallback;
    }
    v.x /= len;
    v.y /= len;
    v.z /= len;
    return v;
}

static void apply_texgen_to_coords(GLfloat obj[4], NxglBackendVec3 eye, NxglBackendVec3 normal, GLfloat *u, GLfloat *v, GLfloat *r, int unit)
{
    GLfloat generated[4] = { *u, *v, *r, 1.0f };
    GLfloat eye_vec[4] = { eye.x, eye.y, eye.z, 1.0f };

    if (unit < 0 || unit >= 4) {
        return;
    }
    for (int coord = 0; coord < 4; ++coord) {
        TexGenState *state = &texgen_state[unit][coord];
        GLfloat value = generated[coord];

        if (!state->enabled) {
            continue;
        }
        if (state->mode == GL_OBJECT_LINEAR) {
            value = obj[0] * state->object_plane[0] +
                    obj[1] * state->object_plane[1] +
                    obj[2] * state->object_plane[2] +
                    obj[3] * state->object_plane[3];
        } else if (state->mode == GL_EYE_LINEAR) {
            value = eye_vec[0] * state->eye_plane[0] +
                    eye_vec[1] * state->eye_plane[1] +
                    eye_vec[2] * state->eye_plane[2] +
                    eye_vec[3] * state->eye_plane[3];
        } else if (state->mode == GL_SPHERE_MAP && coord < 2) {
            NxglBackendVec3 n = normalize_vec3(normal);
            value = coord == 0 ? n.x * 0.5f + 0.5f : n.y * 0.5f + 0.5f;
        }
        generated[coord] = value;
    }
    if (generated[3] != 0.0f) {
        *u = generated[0] / generated[3];
        *v = generated[1] / generated[3];
        *r = generated[2] / generated[3];
    } else {
        *u = generated[0];
        *v = generated[1];
        *r = generated[2];
    }
}

static float dot_vec3(NxglBackendVec3 a, NxglBackendVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static NxglBackendVec3 subtract_vec3(NxglBackendVec3 a, NxglBackendVec3 b)
{
    NxglBackendVec3 out = { a.x - b.x, a.y - b.y, a.z - b.z };
    return out;
}

static float length_vec3(NxglBackendVec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static NxglBackendColor multiply_color(NxglBackendColor a, NxglBackendColor b)
{
    NxglBackendColor out = { a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a };
    return out;
}

static NxglBackendColor add_color(NxglBackendColor a, NxglBackendColor b)
{
    NxglBackendColor out = { a.r + b.r, a.g + b.g, a.b + b.b, a.a };
    return out;
}

static NxglBackendColor scale_color(NxglBackendColor color, float scale)
{
    NxglBackendColor out = { color.r * scale, color.g * scale, color.b * scale, color.a };
    return out;
}

static NxglBackendColor clamp_color(NxglBackendColor color)
{
    color.r = clamp01(color.r);
    color.g = clamp01(color.g);
    color.b = clamp01(color.b);
    color.a = clamp01(color.a);
    return color;
}

static void apply_color_material(NxglBackendColor color)
{
    if (!color_material_enabled) {
        return;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_AMBIENT || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_state.ambient = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_DIFFUSE || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_state.diffuse = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_SPECULAR) {
        material_state.specular = color;
    }
    if ((color_material_face == GL_FRONT || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_EMISSION) {
        material_state.emission = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_AMBIENT || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_back_state.ambient = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        (color_material_parameter == GL_DIFFUSE || color_material_parameter == GL_AMBIENT_AND_DIFFUSE)) {
        material_back_state.diffuse = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_SPECULAR) {
        material_back_state.specular = color;
    }
    if ((color_material_face == GL_BACK || color_material_face == GL_FRONT_AND_BACK) &&
        color_material_parameter == GL_EMISSION) {
        material_back_state.emission = color;
    }
}

static NxglBackendColor lit_color_with_material(NxglBackendColor base_color, NxglBackendVec3 normal, NxglBackendVec3 eye, const MaterialState *material)
{
    NxglBackendColor result;
    NxglBackendColor specular_accum = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool any_light = false;

    if (!lighting_enabled) {
        return base_color;
    }

    if (normalize_enabled || rescale_normal_enabled) {
        normal = normalize_vec3(normal);
    }

    result = material->emission;
    result = add_color(result, multiply_color(material->ambient, light_model_ambient));
    result.a = material->diffuse.a;

    for (int i = 0; i < 8; ++i) {
        if (!lights[i].enabled) {
            continue;
        }
        any_light = true;
        float light_scale = 1.0f;
        NxglBackendVec3 light_dir = { lights[i].position[0], lights[i].position[1], lights[i].position[2] };
        if (lights[i].position[3] != 0.0f) {
            NxglBackendVec3 light_pos = light_dir;
            NxglBackendVec3 to_vertex = subtract_vec3(eye, light_pos);
            float distance = length_vec3(to_vertex);
            float attenuation = lights[i].constant_attenuation +
                                lights[i].linear_attenuation * distance +
                                lights[i].quadratic_attenuation * distance * distance;
            light_dir = subtract_vec3(light_pos, eye);
            if (attenuation > 0.00001f) {
                light_scale = 1.0f / attenuation;
            }
            if (lights[i].spot_cutoff != 180.0f) {
                NxglBackendVec3 spot_dir = normalize_vec3((NxglBackendVec3){
                    lights[i].spot_direction[0],
                    lights[i].spot_direction[1],
                    lights[i].spot_direction[2]
                });
                float spot_cos = dot_vec3(normalize_vec3(to_vertex), spot_dir);
                float cutoff_cos = cosf(lights[i].spot_cutoff * 0.017453292519943295f);
                if (spot_cos < cutoff_cos) {
                    light_scale = 0.0f;
                } else if (lights[i].spot_exponent > 0.0f) {
                    light_scale *= powf(spot_cos < 0.0f ? 0.0f : spot_cos, lights[i].spot_exponent);
                }
            }
        }
        light_dir = normalize_vec3(light_dir);
        float diffuse = dot_vec3(normal, light_dir);
        if (diffuse < 0.0f) {
            diffuse = 0.0f;
        }
        result = add_color(result, scale_color(multiply_color(material->ambient, lights[i].ambient), light_scale));
        result = add_color(result, scale_color(multiply_color(material->diffuse, lights[i].diffuse), diffuse * light_scale));
        if (material->shininess > 0.0f && diffuse > 0.0f) {
            NxglBackendVec3 view_dir = light_model_local_viewer ? normalize_vec3((NxglBackendVec3){ -eye.x, -eye.y, -eye.z }) : (NxglBackendVec3){ 0.0f, 0.0f, 1.0f };
            NxglBackendVec3 half_vec = normalize_vec3((NxglBackendVec3){ light_dir.x + view_dir.x, light_dir.y + view_dir.y, light_dir.z + view_dir.z });
            float spec_angle = dot_vec3(normal, half_vec);
            float spec = 0.0f;
            if (spec_angle > 0.0f) {
                spec = powf(spec_angle, material->shininess * 0.25f) * light_scale;
            }
            NxglBackendColor contribution = scale_color(multiply_color(material->specular, lights[i].specular), spec);
            if (light_model_color_control == GL_SEPARATE_SPECULAR_COLOR) {
                specular_accum = add_color(specular_accum, contribution);
            } else {
                result = add_color(result, contribution);
            }
        }
    }

    if (!any_light) {
        result = multiply_color(material->diffuse, base_color);
    }
    result.a = base_color.a;
    if (light_model_color_control == GL_SEPARATE_SPECULAR_COLOR) {
        result = add_color(clamp_color(result), specular_accum);
        result.a = base_color.a;
    }
    return clamp_color(result);
}

static NxglBackendColor lit_color(NxglBackendColor base_color, NxglBackendVec3 normal, NxglBackendVec3 eye)
{
    return lit_color_with_material(base_color, normal, eye, &material_state);
}

static float fog_factor(float depth)
{
    float factor;

    if (!fog_enabled) {
        return 1.0f;
    }
    if (depth < 0.0f) {
        depth = -depth;
    }

    if (fog_mode == GL_LINEAR) {
        float range = fog_end - fog_start;
        if (range == 0.0f) {
            return depth <= fog_start ? 1.0f : 0.0f;
        }
        factor = (fog_end - depth) / range;
    } else if (fog_mode == GL_EXP2) {
        float d = fog_density * depth;
        factor = expf(-(d * d));
    } else {
        factor = expf(-(fog_density * depth));
    }
    return clamp01(factor);
}

static NxglBackendColor apply_fog(NxglBackendColor color, NxglBackendVec3 pos)
{
    float factor;
    NxglBackendColor out;

    if (!fog_enabled) {
        return color;
    }

    factor = fog_factor(pos.z);
    out.r = color.r * factor + fog_color.r * (1.0f - factor);
    out.g = color.g * factor + fog_color.g * (1.0f - factor);
    out.b = color.b * factor + fog_color.b * (1.0f - factor);
    out.a = color.a;
    return clamp_color(out);
}

static void matrix_identity(Matrix out)
{
    memset(out, 0, sizeof(Matrix));
    out[M11] = 1.0f;
    out[M22] = 1.0f;
    out[M33] = 1.0f;
    out[M44] = 1.0f;
}

static void matrix_multiply(Matrix out, const Matrix a, const Matrix b)
{
    Matrix w;
    w[M11] = a[M11] * b[M11] + a[M12] * b[M21] + a[M13] * b[M31] + a[M14] * b[M41];
    w[M12] = a[M11] * b[M12] + a[M12] * b[M22] + a[M13] * b[M32] + a[M14] * b[M42];
    w[M13] = a[M11] * b[M13] + a[M12] * b[M23] + a[M13] * b[M33] + a[M14] * b[M43];
    w[M14] = a[M11] * b[M14] + a[M12] * b[M24] + a[M13] * b[M34] + a[M14] * b[M44];
    w[M21] = a[M21] * b[M11] + a[M22] * b[M21] + a[M23] * b[M31] + a[M24] * b[M41];
    w[M22] = a[M21] * b[M12] + a[M22] * b[M22] + a[M23] * b[M32] + a[M24] * b[M42];
    w[M23] = a[M21] * b[M13] + a[M22] * b[M23] + a[M23] * b[M33] + a[M24] * b[M43];
    w[M24] = a[M21] * b[M14] + a[M22] * b[M24] + a[M23] * b[M34] + a[M24] * b[M44];
    w[M31] = a[M31] * b[M11] + a[M32] * b[M21] + a[M33] * b[M31] + a[M34] * b[M41];
    w[M32] = a[M31] * b[M12] + a[M32] * b[M22] + a[M33] * b[M32] + a[M34] * b[M42];
    w[M33] = a[M31] * b[M13] + a[M32] * b[M23] + a[M33] * b[M33] + a[M34] * b[M43];
    w[M34] = a[M31] * b[M14] + a[M32] * b[M24] + a[M33] * b[M34] + a[M34] * b[M44];
    w[M41] = a[M41] * b[M11] + a[M42] * b[M21] + a[M43] * b[M31] + a[M44] * b[M41];
    w[M42] = a[M41] * b[M12] + a[M42] * b[M22] + a[M43] * b[M32] + a[M44] * b[M42];
    w[M43] = a[M41] * b[M13] + a[M42] * b[M23] + a[M43] * b[M33] + a[M44] * b[M43];
    w[M44] = a[M41] * b[M14] + a[M42] * b[M24] + a[M43] * b[M34] + a[M44] * b[M44];
    memcpy(out, w, sizeof(Matrix));
}

static void matrix_transpose(Matrix out, const Matrix in)
{
    Matrix w;
    w[M11] = in[M11];
    w[M12] = in[M21];
    w[M13] = in[M31];
    w[M14] = in[M41];
    w[M21] = in[M12];
    w[M22] = in[M22];
    w[M23] = in[M32];
    w[M24] = in[M42];
    w[M31] = in[M13];
    w[M32] = in[M23];
    w[M33] = in[M33];
    w[M34] = in[M43];
    w[M41] = in[M14];
    w[M42] = in[M24];
    w[M43] = in[M34];
    w[M44] = in[M44];
    memcpy(out, w, sizeof(Matrix));
}

static Matrix *current_matrix(void)
{
    if (matrix_mode == GL_PROJECTION) {
        return &projection;
    }
    if (matrix_mode == GL_TEXTURE) {
        return &texture_matrix;
    }
    return &modelview;
}

static void invalidate_modelview_inverse_cache(void)
{
    modelview_inverse_cache_valid = false;
}

static int clip_plane_index(GLenum plane)
{
    if (plane >= GL_CLIP_PLANE0 && plane <= GL_CLIP_PLANE5) {
        return (int)(plane - GL_CLIP_PLANE0);
    }
    return -1;
}

static bool invert_matrix(Matrix out, const Matrix in)
{
    Matrix inv;
    GLfloat det;

    inv[0] = in[5]  * in[10] * in[15] -
             in[5]  * in[11] * in[14] -
             in[9]  * in[6]  * in[15] +
             in[9]  * in[7]  * in[14] +
             in[13] * in[6]  * in[11] -
             in[13] * in[7]  * in[10];
    inv[4] = -in[4]  * in[10] * in[15] +
              in[4]  * in[11] * in[14] +
              in[8]  * in[6]  * in[15] -
              in[8]  * in[7]  * in[14] -
              in[12] * in[6]  * in[11] +
              in[12] * in[7]  * in[10];
    inv[8] = in[4]  * in[9] * in[15] -
             in[4]  * in[11] * in[13] -
             in[8]  * in[5] * in[15] +
             in[8]  * in[7] * in[13] +
             in[12] * in[5] * in[11] -
             in[12] * in[7] * in[9];
    inv[12] = -in[4]  * in[9] * in[14] +
               in[4]  * in[10] * in[13] +
               in[8]  * in[5] * in[14] -
               in[8]  * in[6] * in[13] -
               in[12] * in[5] * in[10] +
               in[12] * in[6] * in[9];
    inv[1] = -in[1]  * in[10] * in[15] +
              in[1]  * in[11] * in[14] +
              in[9]  * in[2] * in[15] -
              in[9]  * in[3] * in[14] -
              in[13] * in[2] * in[11] +
              in[13] * in[3] * in[10];
    inv[5] = in[0]  * in[10] * in[15] -
             in[0]  * in[11] * in[14] -
             in[8]  * in[2] * in[15] +
             in[8]  * in[3] * in[14] +
             in[12] * in[2] * in[11] -
             in[12] * in[3] * in[10];
    inv[9] = -in[0]  * in[9] * in[15] +
              in[0]  * in[11] * in[13] +
              in[8]  * in[1] * in[15] -
              in[8]  * in[3] * in[13] -
              in[12] * in[1] * in[11] +
              in[12] * in[3] * in[9];
    inv[13] = in[0]  * in[9] * in[14] -
              in[0]  * in[10] * in[13] -
              in[8]  * in[1] * in[14] +
              in[8]  * in[2] * in[13] +
              in[12] * in[1] * in[10] -
              in[12] * in[2] * in[9];
    inv[2] = in[1]  * in[6] * in[15] -
             in[1]  * in[7] * in[14] -
             in[5]  * in[2] * in[15] +
             in[5]  * in[3] * in[14] +
             in[13] * in[2] * in[7] -
             in[13] * in[3] * in[6];
    inv[6] = -in[0]  * in[6] * in[15] +
              in[0]  * in[7] * in[14] +
              in[4]  * in[2] * in[15] -
              in[4]  * in[3] * in[14] -
              in[12] * in[2] * in[7] +
              in[12] * in[3] * in[6];
    inv[10] = in[0]  * in[5] * in[15] -
              in[0]  * in[7] * in[13] -
              in[4]  * in[1] * in[15] +
              in[4]  * in[3] * in[13] +
              in[12] * in[1] * in[7] -
              in[12] * in[3] * in[5];
    inv[14] = -in[0]  * in[5] * in[14] +
               in[0]  * in[6] * in[13] +
               in[4]  * in[1] * in[14] -
               in[4]  * in[2] * in[13] -
               in[12] * in[1] * in[6] +
               in[12] * in[2] * in[5];
    inv[3] = -in[1] * in[6] * in[11] +
              in[1] * in[7] * in[10] +
              in[5] * in[2] * in[11] -
              in[5] * in[3] * in[10] -
              in[9] * in[2] * in[7] +
              in[9] * in[3] * in[6];
    inv[7] = in[0] * in[6] * in[11] -
             in[0] * in[7] * in[10] -
             in[4] * in[2] * in[11] +
             in[4] * in[3] * in[10] +
             in[8] * in[2] * in[7] -
             in[8] * in[3] * in[6];
    inv[11] = -in[0] * in[5] * in[11] +
               in[0] * in[7] * in[9] +
               in[4] * in[1] * in[11] -
               in[4] * in[3] * in[9] -
               in[8] * in[1] * in[7] +
               in[8] * in[3] * in[5];
    inv[15] = in[0] * in[5] * in[10] -
              in[0] * in[6] * in[9] -
              in[4] * in[1] * in[10] +
              in[4] * in[2] * in[9] +
              in[8] * in[1] * in[6] -
              in[8] * in[2] * in[5];

    det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];
    if (det == 0.0f) {
        return false;
    }
    det = 1.0f / det;
    for (int i = 0; i < 16; ++i) {
        out[i] = inv[i] * det;
    }
    return true;
}

static void transform_clip_plane(GLdouble out[4], const GLdouble in[4])
{
    Matrix inverse;

    if (!invert_matrix(inverse, modelview)) {
        memcpy(out, in, sizeof(GLdouble) * 4u);
        return;
    }
    out[0] = in[0] * inverse[M11] + in[1] * inverse[M12] + in[2] * inverse[M13] + in[3] * inverse[M14];
    out[1] = in[0] * inverse[M21] + in[1] * inverse[M22] + in[2] * inverse[M23] + in[3] * inverse[M24];
    out[2] = in[0] * inverse[M31] + in[1] * inverse[M32] + in[2] * inverse[M33] + in[3] * inverse[M34];
    out[3] = in[0] * inverse[M41] + in[1] * inverse[M42] + in[2] * inverse[M43] + in[3] * inverse[M44];
}

static bool point_inside_clip_planes(NxglBackendVec3 pos)
{
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        const GLdouble *p = clip_planes[i].equation;
        GLdouble d;

        if (!clip_planes[i].enabled) {
            continue;
        }
        d = (GLdouble)pos.x * p[0] + (GLdouble)pos.y * p[1] + (GLdouble)pos.z * p[2] + p[3];
        if (d < 0.0) {
            return false;
        }
    }
    return true;
}

static bool primitive_rejected_by_clip_planes(const NxglBackendVertex *vertices, int count)
{
    if (vertices == NULL || count <= 0) {
        return true;
    }
    for (int plane = 0; plane < NXGL_MAX_CLIP_PLANES; ++plane) {
        bool all_outside = true;
        const GLdouble *p = clip_planes[plane].equation;

        if (!clip_planes[plane].enabled) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            GLdouble d = (GLdouble)vertices[i].eye.x * p[0] +
                         (GLdouble)vertices[i].eye.y * p[1] +
                         (GLdouble)vertices[i].eye.z * p[2] +
                         p[3];
            if (d >= 0.0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return true;
        }
    }
    return false;
}

static bool clip_planes_enabled(void)
{
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        if (clip_planes[i].enabled) {
            return true;
        }
    }
    return false;
}

static bool texgen_enabled(void)
{
    return texgen_any_enabled;
}

static void update_texgen_enabled_cache(void)
{
    texgen_any_enabled = false;
    for (int unit = 0; unit < 4; ++unit) {
        for (int coord = 0; coord < 4; ++coord) {
            if (texgen_state[unit][coord].enabled) {
                texgen_any_enabled = true;
                return;
            }
        }
    }
}

static bool native_fast_fill_enabled(void)
{
    return !shadow_readback_enabled &&
           render_mode == GL_RENDER &&
           polygon_mode == GL_FILL &&
           !clip_planes_enabled();
}

static bool default_combine_sources(int unit)
{
    return texture_source_rgb[unit][0] == GL_TEXTURE &&
           texture_source_rgb[unit][1] == GL_PREVIOUS &&
           texture_source_rgb[unit][2] == GL_CONSTANT &&
           texture_operand_rgb[unit][0] == GL_SRC_COLOR &&
           texture_operand_rgb[unit][1] == GL_SRC_COLOR &&
           (texture_operand_rgb[unit][2] == GL_SRC_COLOR || texture_operand_rgb[unit][2] == GL_SRC_ALPHA) &&
           texture_rgb_scale[unit] == 1.0f;
}

static NxglBackendTextureEnvMode native_texture_env_mode_for_unit(int unit)
{
    GLenum mode = texture_env_mode[unit];

    if (mode == GL_COMBINE && default_combine_sources(unit)) {
        switch (texture_combine_rgb[unit]) {
        case GL_REPLACE:
            return NXGL_BACKEND_TEXENV_REPLACE;
        case GL_ADD:
            return NXGL_BACKEND_TEXENV_ADD;
        case GL_ADD_SIGNED:
            return NXGL_BACKEND_TEXENV_ADD_SIGNED;
        case GL_SUBTRACT:
            return NXGL_BACKEND_TEXENV_SUBTRACT;
        case GL_INTERPOLATE:
            return NXGL_BACKEND_TEXENV_INTERPOLATE;
        case GL_MODULATE:
        default:
            return NXGL_BACKEND_TEXENV_MODULATE;
        }
    }

    switch (mode) {
    case GL_REPLACE:
        return NXGL_BACKEND_TEXENV_REPLACE;
    case GL_DECAL:
        return NXGL_BACKEND_TEXENV_DECAL;
    case GL_BLEND:
        return NXGL_BACKEND_TEXENV_BLEND;
    case GL_ADD:
        return NXGL_BACKEND_TEXENV_ADD;
    case GL_MODULATE:
    default:
        return NXGL_BACKEND_TEXENV_MODULATE;
    }
}

static void sync_native_state(void)
{
    TextureObject *texture0 = NULL;
    TextureObject *texture1d0 = NULL;
    TextureObject *texture1 = NULL;
    TextureObject *texture1d1 = NULL;
    nxgl_backend_set_depth(depth_test_enabled, depth_write_enabled);
    nxgl_backend_set_cull(cull_enabled);
    nxgl_backend_set_cull_mode(cull_face_mode, front_face_mode);
    nxgl_backend_set_blend(blend_enabled);
    nxgl_backend_set_blend_func(blend_sfactor, blend_dfactor);
    nxgl_backend_set_scissor(scissor_test_enabled, scissor_box[0], shadow_height - (scissor_box[1] + scissor_box[3]), scissor_box[2], scissor_box[3]);
    nxgl_backend_set_texture_env(native_texture_env_mode_for_unit(0), texture_env_color[0]);
    if (texture_2d_enabled[0] &&
        texture_binding_2d[0] > 0 &&
        texture_binding_2d[0] < 16 &&
        texture_objects[texture_binding_2d[0]].allocated) {
        texture0 = &texture_objects[texture_binding_2d[0]];
    }
    if (texture_1d_enabled[0] &&
        texture_binding_1d[0] > 0 &&
        texture_binding_1d[0] < 16 &&
        texture_objects[texture_binding_1d[0]].allocated) {
        texture1d0 = &texture_objects[texture_binding_1d[0]];
    }
    if (texture_2d_enabled[1] &&
        texture_binding_2d[1] > 0 &&
        texture_binding_2d[1] < 16 &&
        texture_objects[texture_binding_2d[1]].allocated) {
        texture1 = &texture_objects[texture_binding_2d[1]];
    }
    if (texture_1d_enabled[1] &&
        texture_binding_1d[1] > 0 &&
        texture_binding_1d[1] < 16 &&
        texture_objects[texture_binding_1d[1]].allocated) {
        texture1d1 = &texture_objects[texture_binding_1d[1]];
    }
    /*
     * Real NV2A hardware rejects the current cube/3D native descriptors at the
     * texture format register. Keep those targets in the GL shadow renderer
     * until the exact hardware encoding is known.
     */
    if (texture0 != NULL &&
        texture0->native.addr != NULL &&
        texture_complete(texture0)) {
        nxgl_backend_bind_texture(&texture0->native);
    } else if (texture1d0 != NULL &&
        texture1d0->base_level == 0 &&
        texture1d0->native_1d.addr != NULL &&
        texture_1d_complete(texture1d0)) {
        nxgl_backend_bind_texture(&texture1d0->native_1d);
    } else {
        nxgl_backend_bind_texture(NULL);
    }
    if (texture1 != NULL &&
        texture1->native.addr != NULL &&
        texture_complete(texture1)) {
        nxgl_backend_bind_texture1(&texture1->native);
    } else if (texture1d1 != NULL &&
        texture1d1->base_level == 0 &&
        texture1d1->native_1d.addr != NULL &&
        texture_1d_complete(texture1d1)) {
        nxgl_backend_bind_texture1(&texture1d1->native_1d);
    } else {
        nxgl_backend_bind_texture1(NULL);
    }
}

static void ensure_native_frame_started(void)
{
    if (!native_frame_started) {
        nxgl_backend_begin_frame(blend_enabled);
        native_frame_started = true;
        sync_native_state();
    }
}

static void transform_point4(const Matrix m, GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat out[4])
{
    out[0] = x * m[M11] + y * m[M21] + z * m[M31] + w * m[M41];
    out[1] = x * m[M12] + y * m[M22] + z * m[M32] + w * m[M42];
    out[2] = x * m[M13] + y * m[M23] + z * m[M33] + w * m[M43];
    out[3] = x * m[M14] + y * m[M24] + z * m[M34] + w * m[M44];
}

static bool projection_is_identity(void)
{
    Matrix identity;

    matrix_identity(identity);
    for (int i = 0; i < 16; ++i) {
        if (fabsf(projection[i] - identity[i]) > 0.00001f) {
            return false;
        }
    }
    return true;
}

static bool projected_path_active(void)
{
    return !projection_is_identity();
}

static void update_projected_position_from_clip(NxglBackendVertex *vertex)
{
    GLfloat inv_w;
    GLfloat ndc_x;
    GLfloat ndc_y;
    GLfloat ndc_z;
    GLfloat native_z;

    if (!projected_path_active() || fabsf(vertex->clip_w) < 0.000001f) {
        return;
    }

    inv_w = 1.0f / vertex->clip_w;
    ndc_x = vertex->clip_x * inv_w;
    ndc_y = vertex->clip_y * inv_w;
    ndc_z = vertex->clip_z * inv_w;
    vertex->window_z = map_depth_range(clamp01(ndc_z * 0.5f + 0.5f));
    native_z = 5.2f + vertex->window_z * 2.0f;
    vertex->pos.x = ndc_x * native_z - camera_x;
    vertex->pos.y = ndc_y * native_z - camera_y;
    vertex->pos.z = -camera_z - native_z;
}

static NxglBackendVec3 transform_vertex(float x, float y, float z)
{
    NxglBackendVec3 out;
    out.x = x * modelview[M11] + y * modelview[M21] + z * modelview[M31] + modelview[M41];
    out.y = x * modelview[M12] + y * modelview[M22] + z * modelview[M32] + modelview[M42];
    out.z = x * modelview[M13] + y * modelview[M23] + z * modelview[M33] + modelview[M43];
    return out;
}

static NxglBackendVec3 transform_normal_to_eye(NxglBackendVec3 normal)
{
    NxglBackendVec3 out;

    if (!modelview_inverse_cache_valid) {
        modelview_inverse_cache_invertible = invert_matrix(modelview_inverse_cache, modelview);
        modelview_inverse_cache_valid = true;
    }

    if (!modelview_inverse_cache_invertible) {
        out.x = normal.x * modelview[M11] + normal.y * modelview[M21] + normal.z * modelview[M31];
        out.y = normal.x * modelview[M12] + normal.y * modelview[M22] + normal.z * modelview[M32];
        out.z = normal.x * modelview[M13] + normal.y * modelview[M23] + normal.z * modelview[M33];
        return out;
    }

    out.x = normal.x * modelview_inverse_cache[M11] + normal.y * modelview_inverse_cache[M21] + normal.z * modelview_inverse_cache[M31];
    out.y = normal.x * modelview_inverse_cache[M12] + normal.y * modelview_inverse_cache[M22] + normal.z * modelview_inverse_cache[M32];
    out.z = normal.x * modelview_inverse_cache[M13] + normal.y * modelview_inverse_cache[M23] + normal.z * modelview_inverse_cache[M33];
    return out;
}

static void init_vertex_position(NxglBackendVertex *out, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLfloat eye[4];
    GLfloat clip[4];

    if (w == 0.0f) {
        w = 1.0f;
    }
    transform_point4(modelview, x, y, z, w, eye);
    out->eye = (NxglBackendVec3){ eye[0], eye[1], eye[2] };
    transform_point4(projection, eye[0], eye[1], eye[2], eye[3], clip);
    out->clip_x = clip[0];
    out->clip_y = clip[1];
    out->clip_z = clip[2];
    out->clip_w = clip[3];

    if (!projected_path_active() || fabsf(clip[3]) < 0.000001f) {
        out->pos = out->eye;
        out->window_z = map_depth_range(eye_depth_normalized(out->eye));
        return;
    }

    update_projected_position_from_clip(out);
}

static NxglBackendVec3 read_array_normal(GLint index)
{
    NxglBackendVec3 normal = current_normal;
    const uint8_t *base;
    int component_size;
    GLsizei stride;

    if (normal_array.enabled && normal_array.pointer != NULL) {
        component_size = type_size(normal_array.type);
        stride = normal_array.stride == 0 ? 3 * component_size : normal_array.stride;
        base = normal_array.pointer + (index * stride);
        normal.x = read_component(base, normal_array.type, false);
        normal.y = read_component(base + component_size, normal_array.type, false);
        normal.z = read_component(base + component_size * 2, normal_array.type, false);
    }
    return normalize_vec3(normal);
}

static NxglBackendVertex read_array_vertex(GLint index)
{
    NxglBackendVertex out;
    NxglBackendVec3 normal;
    GLfloat obj[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const uint8_t *base;
    int component_size;
    GLsizei stride;

    init_vertex_position(&out, 0.0f, 0.0f, 0.0f, 1.0f);
    out.color = current_color;
    out.u = current_u[0];
    out.v = current_v[0];
    out.r = current_r[0];
    out.u1 = current_u[1];
    out.v1 = current_v[1];
    out.r1 = current_r[1];
    out.u2 = current_u[2];
    out.v2 = current_v[2];
    out.r2 = current_r[2];
    out.u3 = current_u[3];
    out.v3 = current_v[3];
    out.r3 = current_r[3];

    if (vertex_array.enabled && vertex_array.pointer != NULL) {
        component_size = type_size(vertex_array.type);
        stride = effective_stride(&vertex_array);
        base = vertex_array.pointer + (index * stride);
        float x = vertex_array.size > 0 ? read_component(base, vertex_array.type, false) : 0.0f;
        float y = vertex_array.size > 1 ? read_component(base + component_size, vertex_array.type, false) : 0.0f;
        float z = vertex_array.size > 2 ? read_component(base + component_size * 2, vertex_array.type, false) : 0.0f;
        obj[0] = x;
        obj[1] = y;
        obj[2] = z;
        obj[3] = vertex_array.size > 3 ? read_component(base + component_size * 3, vertex_array.type, false) : 1.0f;
        init_vertex_position(&out, x, y, z, obj[3]);
    }

    if (color_array.enabled && color_array.pointer != NULL) {
        component_size = type_size(color_array.type);
        stride = effective_stride(&color_array);
        base = color_array.pointer + (index * stride);
        out.color.r = color_array.size > 0 ? read_component(base, color_array.type, color_array.type != GL_FLOAT) : current_color.r;
        out.color.g = color_array.size > 1 ? read_component(base + component_size, color_array.type, color_array.type != GL_FLOAT) : current_color.g;
        out.color.b = color_array.size > 2 ? read_component(base + component_size * 2, color_array.type, color_array.type != GL_FLOAT) : current_color.b;
        out.color.a = color_array.size > 3 ? read_component(base + component_size * 3, color_array.type, color_array.type != GL_FLOAT) : current_color.a;
    }

    if (texcoord_array[0].enabled && texcoord_array[0].pointer != NULL) {
        component_size = type_size(texcoord_array[0].type);
        stride = effective_stride(&texcoord_array[0]);
        base = texcoord_array[0].pointer + (index * stride);
        out.u = texcoord_array[0].size > 0 ? read_component(base, texcoord_array[0].type, false) : current_u[0];
        out.v = texcoord_array[0].size > 1 ? read_component(base + component_size, texcoord_array[0].type, false) : current_v[0];
        out.r = texcoord_array[0].size > 2 ? read_component(base + component_size * 2, texcoord_array[0].type, false) : current_r[0];
    }
    if (texcoord_array[1].enabled && texcoord_array[1].pointer != NULL) {
        component_size = type_size(texcoord_array[1].type);
        stride = effective_stride(&texcoord_array[1]);
        base = texcoord_array[1].pointer + (index * stride);
        out.u1 = texcoord_array[1].size > 0 ? read_component(base, texcoord_array[1].type, false) : current_u[1];
        out.v1 = texcoord_array[1].size > 1 ? read_component(base + component_size, texcoord_array[1].type, false) : current_v[1];
        out.r1 = texcoord_array[1].size > 2 ? read_component(base + component_size * 2, texcoord_array[1].type, false) : current_r[1];
    }
    if (texcoord_array[2].enabled && texcoord_array[2].pointer != NULL) {
        component_size = type_size(texcoord_array[2].type);
        stride = effective_stride(&texcoord_array[2]);
        base = texcoord_array[2].pointer + (index * stride);
        out.u2 = texcoord_array[2].size > 0 ? read_component(base, texcoord_array[2].type, false) : current_u[2];
        out.v2 = texcoord_array[2].size > 1 ? read_component(base + component_size, texcoord_array[2].type, false) : current_v[2];
        out.r2 = texcoord_array[2].size > 2 ? read_component(base + component_size * 2, texcoord_array[2].type, false) : current_r[2];
    }
    if (texcoord_array[3].enabled && texcoord_array[3].pointer != NULL) {
        component_size = type_size(texcoord_array[3].type);
        stride = effective_stride(&texcoord_array[3]);
        base = texcoord_array[3].pointer + (index * stride);
        out.u3 = texcoord_array[3].size > 0 ? read_component(base, texcoord_array[3].type, false) : current_u[3];
        out.v3 = texcoord_array[3].size > 1 ? read_component(base + component_size, texcoord_array[3].type, false) : current_v[3];
        out.r3 = texcoord_array[3].size > 2 ? read_component(base + component_size * 2, texcoord_array[3].type, false) : current_r[3];
    }

    normal = transform_normal_to_eye(read_array_normal(index));
    out.base_color = out.color;
    out.normal = normal;
    apply_texgen_to_coords(obj, out.eye, normal, &out.u, &out.v, &out.r, 0);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u1, &out.v1, &out.r1, 1);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u2, &out.v2, &out.r2, 2);
    apply_texgen_to_coords(obj, out.eye, normal, &out.u3, &out.v3, &out.r3, 3);
    out.color = lit_color(out.color, normal, out.eye);
    out.color = apply_fog(out.color, out.eye);
    return out;
}

static bool primitive_intersects_viewport(const NxglBackendVertex *vertices, int count)
{
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int viewport_min_x;
    int viewport_min_y;
    int viewport_max_x;
    int viewport_max_y;

    if (count <= 0 || vertices == NULL) {
        return false;
    }
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (primitive_rejected_by_clip_planes(vertices, count)) {
        return false;
    }
    if (projected_path_active() && count == 1) {
        for (int plane = 0; plane < 6; ++plane) {
            if (feedback_clip_plane_value(&vertices[0], plane) < 0.0f) {
                return false;
            }
        }
    }
    for (int i = 0; i < count; ++i) {
        int sx = 0;
        int sy = 0;
        if (!shadow_project(vertices[i].pos, &sx, &sy)) {
            return false;
        }
        if (i == 0) {
            min_x = max_x = sx;
            min_y = max_y = sy;
        } else {
            if (sx < min_x) min_x = sx;
            if (sx > max_x) max_x = sx;
            if (sy < min_y) min_y = sy;
            if (sy > max_y) max_y = sy;
        }
    }
    viewport_top_left_bounds(&viewport_min_x, &viewport_min_y, &viewport_max_x, &viewport_max_y);
    return max_x >= viewport_min_x &&
           max_y >= viewport_min_y &&
           min_x <= viewport_max_x &&
           min_y <= viewport_max_y;
}

static NxglBackendVertex interpolate_vertex(const NxglBackendVertex *a, const NxglBackendVertex *b, GLfloat t)
{
    NxglBackendVertex out;

    out.pos.x = a->pos.x + (b->pos.x - a->pos.x) * t;
    out.pos.y = a->pos.y + (b->pos.y - a->pos.y) * t;
    out.pos.z = a->pos.z + (b->pos.z - a->pos.z) * t;
    out.eye.x = a->eye.x + (b->eye.x - a->eye.x) * t;
    out.eye.y = a->eye.y + (b->eye.y - a->eye.y) * t;
    out.eye.z = a->eye.z + (b->eye.z - a->eye.z) * t;
    out.color.r = a->color.r + (b->color.r - a->color.r) * t;
    out.color.g = a->color.g + (b->color.g - a->color.g) * t;
    out.color.b = a->color.b + (b->color.b - a->color.b) * t;
    out.color.a = a->color.a + (b->color.a - a->color.a) * t;
    out.base_color.r = a->base_color.r + (b->base_color.r - a->base_color.r) * t;
    out.base_color.g = a->base_color.g + (b->base_color.g - a->base_color.g) * t;
    out.base_color.b = a->base_color.b + (b->base_color.b - a->base_color.b) * t;
    out.base_color.a = a->base_color.a + (b->base_color.a - a->base_color.a) * t;
    out.normal.x = a->normal.x + (b->normal.x - a->normal.x) * t;
    out.normal.y = a->normal.y + (b->normal.y - a->normal.y) * t;
    out.normal.z = a->normal.z + (b->normal.z - a->normal.z) * t;
    out.u = a->u + (b->u - a->u) * t;
    out.v = a->v + (b->v - a->v) * t;
    out.r = a->r + (b->r - a->r) * t;
    out.u1 = a->u1 + (b->u1 - a->u1) * t;
    out.v1 = a->v1 + (b->v1 - a->v1) * t;
    out.r1 = a->r1 + (b->r1 - a->r1) * t;
    out.u2 = a->u2 + (b->u2 - a->u2) * t;
    out.v2 = a->v2 + (b->v2 - a->v2) * t;
    out.r2 = a->r2 + (b->r2 - a->r2) * t;
    out.u3 = a->u3 + (b->u3 - a->u3) * t;
    out.v3 = a->v3 + (b->v3 - a->v3) * t;
    out.r3 = a->r3 + (b->r3 - a->r3) * t;
    out.clip_x = a->clip_x + (b->clip_x - a->clip_x) * t;
    out.clip_y = a->clip_y + (b->clip_y - a->clip_y) * t;
    out.clip_z = a->clip_z + (b->clip_z - a->clip_z) * t;
    out.clip_w = a->clip_w + (b->clip_w - a->clip_w) * t;
    out.window_z = a->window_z + (b->window_z - a->window_z) * t;
    update_projected_position_from_clip(&out);
    return out;
}

static bool clip_line_interval(GLfloat f0, GLfloat f1, GLfloat *t0, GLfloat *t1)
{
    GLfloat delta = f1 - f0;
    GLfloat t;

    if (f0 < 0.0f && f1 < 0.0f) {
        return false;
    }
    if (delta > -0.000001f && delta < 0.000001f) {
        return f0 >= 0.0f;
    }

    t = -f0 / delta;
    if (f0 < 0.0f) {
        if (t > *t0) {
            *t0 = t;
        }
    } else if (f1 < 0.0f) {
        if (t < *t1) {
            *t1 = t;
        }
    }
    return *t0 <= *t1;
}

static bool clip_line_to_feedback_volume(const NxglBackendVertex *in_a, const NxglBackendVertex *in_b, NxglBackendVertex *out_a, NxglBackendVertex *out_b)
{
    GLfloat t0 = 0.0f;
    GLfloat t1 = 1.0f;

    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (projected_path_active()) {
        if (!clip_line_interval(in_a->clip_z + in_a->clip_w, in_b->clip_z + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_z, in_b->clip_w - in_b->clip_z, &t0, &t1) ||
            !clip_line_interval(in_a->clip_x + in_a->clip_w, in_b->clip_x + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_x, in_b->clip_w - in_b->clip_x, &t0, &t1) ||
            !clip_line_interval(in_a->clip_y + in_a->clip_w, in_b->clip_y + in_b->clip_w, &t0, &t1) ||
            !clip_line_interval(in_a->clip_w - in_a->clip_y, in_b->clip_w - in_b->clip_y, &t0, &t1)) {
            return false;
        }
    } else {
        GLfloat ax = in_a->pos.x + camera_x;
        GLfloat ay = in_a->pos.y + camera_y;
        GLfloat az = -(in_a->pos.z + camera_z);
        GLfloat bx = in_b->pos.x + camera_x;
        GLfloat by = in_b->pos.y + camera_y;
        GLfloat bz = -(in_b->pos.z + camera_z);
        if (!clip_line_interval(az - 0.1f, bz - 0.1f, &t0, &t1) ||
            !clip_line_interval(ax + az, bx + bz, &t0, &t1) ||
            !clip_line_interval(az - ax, bz - bx, &t0, &t1) ||
            !clip_line_interval(ay + az, by + bz, &t0, &t1) ||
            !clip_line_interval(az - ay, bz - by, &t0, &t1)) {
            return false;
        }
    }

    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        const GLdouble *plane = clip_planes[i].equation;
        GLfloat f0;
        GLfloat f1;

        if (!clip_planes[i].enabled) {
            continue;
        }
        f0 = (GLfloat)(plane[0] * in_a->eye.x + plane[1] * in_a->eye.y + plane[2] * in_a->eye.z + plane[3]);
        f1 = (GLfloat)(plane[0] * in_b->eye.x + plane[1] * in_b->eye.y + plane[2] * in_b->eye.z + plane[3]);
        if (!clip_line_interval(f0, f1, &t0, &t1)) {
            return false;
        }
    }

    if (t1 < t0) {
        return false;
    }
    {
        NxglBackendVertex clipped[2];
        clipped[0] = interpolate_vertex(in_a, in_b, t0);
        clipped[1] = interpolate_vertex(in_a, in_b, t1);
        normalize_lower_feedback_clip_edges(clipped, 2);
        *out_a = clipped[0];
        *out_b = clipped[1];
    }
    return true;
}

static void normalize_lower_feedback_clip_edge(NxglBackendVertex *vertex)
{
    GLfloat z;
    GLfloat y;

    if (vertex == NULL || projected_path_active()) {
        return;
    }

    z = -(vertex->pos.z + camera_z);
    y = vertex->pos.y + camera_y;
    if (z >= 0.0999f && fabsf(y + z) <= 0.002f) {
        vertex->pos.y = z - camera_y;
        vertex->eye.y = vertex->pos.y;
    }
}

static void normalize_lower_feedback_clip_edges(NxglBackendVertex *vertices, int count)
{
    bool touches_upper = false;

    if (vertices == NULL || count <= 0 || projected_path_active()) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        GLfloat z = -(vertices[i].pos.z + camera_z);
        GLfloat y = vertices[i].pos.y + camera_y;
        if (z >= 0.0999f && fabsf(z - y) <= 0.002f) {
            touches_upper = true;
            break;
        }
    }
    if (touches_upper) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        normalize_lower_feedback_clip_edge(&vertices[i]);
    }
}

static GLfloat feedback_clip_plane_value(const NxglBackendVertex *vertex, int plane)
{
    if (projected_path_active()) {
        switch (plane) {
        case 0:
            return vertex->clip_z + vertex->clip_w;
        case 1:
            return vertex->clip_w - vertex->clip_z;
        case 2:
            return vertex->clip_x + vertex->clip_w;
        case 3:
            return vertex->clip_w - vertex->clip_x;
        case 4:
            return vertex->clip_y + vertex->clip_w;
        case 5:
            return vertex->clip_w - vertex->clip_y;
        default: {
            int index = plane - 6;
            const GLdouble *p = clip_planes[index].equation;
            return (GLfloat)(p[0] * vertex->eye.x + p[1] * vertex->eye.y + p[2] * vertex->eye.z + p[3]);
        }
        }
    } else {
        GLfloat x = vertex->pos.x + camera_x;
        GLfloat y = vertex->pos.y + camera_y;
        GLfloat z = -(vertex->pos.z + camera_z);

        switch (plane) {
        case 0:
            return z - 0.1f;
        case 1:
            return x + z;
        case 2:
            return z - x;
        case 3:
            return y + z;
        case 4:
            return z - y;
        default: {
            int index = plane - 5;
            const GLdouble *p = clip_planes[index].equation;
            return (GLfloat)(p[0] * vertex->eye.x + p[1] * vertex->eye.y + p[2] * vertex->eye.z + p[3]);
        }
        }
    }
}

static bool clip_polygon_against_feedback_plane(const NxglBackendVertex *input, int input_count, NxglBackendVertex *output, int *output_count, int plane)
{
    NxglBackendVertex previous;
    GLfloat previous_value;
    bool previous_inside;
    int out_count = 0;

    if (input_count <= 0) {
        *output_count = 0;
        return false;
    }

    previous = input[input_count - 1];
    previous_value = feedback_clip_plane_value(&previous, plane);
    previous_inside = previous_value >= 0.0f;

    for (int i = 0; i < input_count; ++i) {
        NxglBackendVertex current = input[i];
        GLfloat current_value = feedback_clip_plane_value(&current, plane);
        bool current_inside = current_value >= 0.0f;

        if (current_inside != previous_inside) {
            GLfloat denom = previous_value - current_value;
            GLfloat t = denom > -0.000001f && denom < 0.000001f ? 0.0f : previous_value / denom;
            if (out_count >= NXGL_CLIPPED_POLYGON_MAX) {
                return false;
            }
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            output[out_count++] = interpolate_vertex(&previous, &current, t);
        }
        if (current_inside) {
            if (out_count >= NXGL_CLIPPED_POLYGON_MAX) {
                return false;
            }
            output[out_count++] = current;
        }

        previous = current;
        previous_value = current_value;
        previous_inside = current_inside;
    }

    *output_count = out_count;
    return out_count >= 3;
}

static bool clip_polygon_to_feedback_volume(const NxglBackendVertex *vertices, int count, NxglBackendVertex *output, int *output_count)
{
    NxglBackendVertex scratch_a[NXGL_CLIPPED_POLYGON_MAX];
    NxglBackendVertex scratch_b[NXGL_CLIPPED_POLYGON_MAX];
    NxglBackendVertex *in = scratch_a;
    NxglBackendVertex *out = scratch_b;
    int in_count = count;
    int out_count = 0;

    if (vertices == NULL || count < 3 || count > NXGL_CLIPPED_POLYGON_MAX || viewport[2] <= 0 || viewport[3] <= 0) {
        *output_count = 0;
        return false;
    }

    memcpy(scratch_a, vertices, (size_t)count * sizeof(NxglBackendVertex));
    int fixed_plane_count = projected_path_active() ? 6 : 5;
    for (int plane = 0; plane < fixed_plane_count; ++plane) {
        if (!clip_polygon_against_feedback_plane(in, in_count, out, &out_count, plane)) {
            *output_count = 0;
            return false;
        }
        NxglBackendVertex *tmp = in;
        in = out;
        out = tmp;
        in_count = out_count;
    }

    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        if (!clip_planes[i].enabled) {
            continue;
        }
        if (!clip_polygon_against_feedback_plane(in, in_count, out, &out_count, fixed_plane_count + i)) {
            *output_count = 0;
            return false;
        }
        NxglBackendVertex *tmp = in;
        in = out;
        out = tmp;
        in_count = out_count;
    }

    memcpy(output, in, (size_t)in_count * sizeof(NxglBackendVertex));
    *output_count = in_count;
    normalize_lower_feedback_clip_edges(output, in_count);
    return in_count >= 3;
}

static GLuint selection_depth_value(const NxglBackendVertex *vertex)
{
    float normalized = projected_path_active() ? vertex->window_z : map_depth_range(eye_depth_normalized(vertex->pos));
    return (GLuint)(normalized * 4294967295.0f);
}

static void record_selection_hit(const NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex clipped[2];
    NxglBackendVertex clipped_polygon[NXGL_CLIPPED_POLYGON_MAX];
    const NxglBackendVertex *depth_vertices = vertices;
    int depth_count = count;
    GLuint min_z;
    GLuint max_z;
    GLsizei needed;

    if (render_mode != GL_SELECT) {
        return;
    }
    if (count == 2) {
        if (vertices == NULL || !clip_line_to_feedback_volume(&vertices[0], &vertices[1], &clipped[0], &clipped[1])) {
            return;
        }
        depth_vertices = clipped;
        depth_count = 2;
    } else if (count >= 3) {
        if (!clip_polygon_to_feedback_volume(vertices, count, clipped_polygon, &depth_count)) {
            return;
        }
        depth_vertices = clipped_polygon;
    } else {
        if (!primitive_intersects_viewport(vertices, count)) {
            return;
        }
    }

    min_z = 0xffffffffu;
    max_z = 0u;
    for (int i = 0; i < depth_count; ++i) {
        GLuint z = selection_depth_value(&depth_vertices[i]);
        if (z < min_z) {
            min_z = z;
        }
        if (z > max_z) {
            max_z = z;
        }
    }

    needed = 3 + name_stack_depth;
    ++selection_hits;
    if (selection_buffer == NULL ||
        selection_buffer_size < 0 ||
        selection_write_count + needed > selection_buffer_size) {
        selection_overflow = true;
        return;
    }

    selection_buffer[selection_write_count++] = (GLuint)name_stack_depth;
    selection_buffer[selection_write_count++] = min_z;
    selection_buffer[selection_write_count++] = max_z;
    for (GLint i = 0; i < name_stack_depth; ++i) {
        selection_buffer[selection_write_count++] = name_stack[i];
    }
}

static void feedback_write(GLfloat value)
{
    if (feedback_buffer == NULL || feedback_buffer_size < 0 || feedback_overflow) {
        feedback_overflow = true;
        return;
    }
    if (feedback_write_count >= feedback_buffer_size) {
        feedback_overflow = true;
        return;
    }
    feedback_buffer[feedback_write_count++] = value;
}

static bool project_depth_for_window(NxglBackendVec3 pos, GLfloat *z_out)
{
    GLfloat z = -(pos.z + camera_z);

    if (z < 0.0999f || viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }
    if (z < 0.1f) {
        z = 0.1f;
    }
    *z_out = z;
    return true;
}

static void snap_feedback_viewport_edges(GLfloat *x)
{
    GLfloat left = (GLfloat)viewport[0];
    GLfloat right = (GLfloat)(viewport[0] + viewport[2]);

    if (*x >= left - 1.0f && *x <= left + 3.0f) {
        *x = left;
    } else if (*x >= right - 4.0f && *x <= right + 1.0f) {
        *x = right;
    }
}

static bool feedback_project_window(NxglBackendVec3 pos, GLfloat *sx, GLfloat *sy)
{
    GLfloat z;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    *sx = (GLfloat)viewport[0] + ((pos.x + camera_x) / z) * (GLfloat)viewport[2] * 0.5f + (GLfloat)viewport[2] * 0.5f;
    *sy = ((pos.y + camera_y) / z) * (GLfloat)shadow_height * 0.5f + (GLfloat)shadow_height * 0.5f;
    snap_feedback_viewport_edges(sx);
    return true;
}

static void feedback_write_vertex(const NxglBackendVertex *vertex)
{
    GLfloat x = vertex->pos.x;
    GLfloat y = vertex->pos.y;
    GLfloat z = projected_path_active() ? vertex->window_z : map_depth_range(eye_depth_normalized(vertex->pos));

    if (feedback_project_window(vertex->pos, &x, &y)) {
        /* x/y updated by helper */
    }

    feedback_write(x);
    feedback_write(y);
    if (feedback_type == GL_2D) {
        return;
    }
    feedback_write(z);
    if (feedback_type == GL_3D) {
        return;
    }
    if (feedback_type == GL_4D_COLOR_TEXTURE) {
        feedback_write(1.0f);
    }
    feedback_write(clamp01(vertex->color.r));
    feedback_write(clamp01(vertex->color.g));
    feedback_write(clamp01(vertex->color.b));
    feedback_write(clamp01(vertex->color.a));
    if (feedback_type == GL_3D_COLOR) {
        return;
    }
    feedback_write(vertex->u);
    feedback_write(vertex->v);
    feedback_write(vertex->r);
    feedback_write(1.0f);
    if (feedback_type == GL_3D_COLOR_TEXTURE) {
        return;
    }
}

static void feedback_write_raster_vertex(void)
{
    feedback_write(current_raster_position[0]);
    feedback_write(current_raster_position[1]);
    if (feedback_type == GL_2D) {
        return;
    }
    feedback_write(current_raster_position[2]);
    if (feedback_type == GL_3D) {
        return;
    }
    if (feedback_type == GL_4D_COLOR_TEXTURE) {
        feedback_write(current_raster_position[3]);
    }
    feedback_write(clamp01(current_color.r));
    feedback_write(clamp01(current_color.g));
    feedback_write(clamp01(current_color.b));
    feedback_write(clamp01(current_color.a));
    if (feedback_type == GL_3D_COLOR) {
        return;
    }
    feedback_write(current_u[0]);
    feedback_write(current_v[0]);
    feedback_write(current_r[0]);
    feedback_write(1.0f);
}

static void record_feedback_point(const NxglBackendVertex *vertex)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!primitive_intersects_viewport(vertex, 1)) {
        return;
    }
    feedback_write((GLfloat)GL_POINT_TOKEN);
    feedback_write_vertex(vertex);
}

static void record_feedback_line(const NxglBackendVertex *a, const NxglBackendVertex *b)
{
    NxglBackendVertex clipped_a;
    NxglBackendVertex clipped_b;

    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!clip_line_to_feedback_volume(a, b, &clipped_a, &clipped_b)) {
        return;
    }
    feedback_write((GLfloat)GL_LINE_TOKEN);
    feedback_write_vertex(&clipped_a);
    feedback_write_vertex(&clipped_b);
}

static void rotate_feedback_polygon_near_first(NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex scratch[NXGL_CLIPPED_POLYGON_MAX];
    int best = -1;
    GLfloat best_z = 1.0f;

    if (vertices == NULL || count <= 0 || count > NXGL_CLIPPED_POLYGON_MAX || projected_path_active()) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        GLfloat z = map_depth_range(eye_depth_normalized(vertices[i].pos));
        if (z < best_z) {
            best_z = z;
            best = i;
        }
    }
    if (best <= 0 || best_z > 0.01f) {
        return;
    }

    memcpy(scratch, vertices, (size_t)count * sizeof(NxglBackendVertex));
    for (int i = 0; i < count; ++i) {
        vertices[i] = scratch[(best + i) % count];
    }
}

static void trim_feedback_polygon_near_quad(NxglBackendVertex *vertices, int *count)
{
    if (vertices == NULL || count == NULL || *count <= 4 || projected_path_active()) {
        return;
    }
    if (map_depth_range(eye_depth_normalized(vertices[0].pos)) <= 0.01f) {
        *count = 4;
    }
}

static void record_feedback_polygon(const NxglBackendVertex *vertices, GLsizei count)
{
    NxglBackendVertex clipped[NXGL_CLIPPED_POLYGON_MAX];
    int clipped_count = 0;

    if (render_mode != GL_FEEDBACK) {
        return;
    }
    if (!clip_polygon_to_feedback_volume(vertices, count, clipped, &clipped_count)) {
        return;
    }
    rotate_feedback_polygon_near_first(clipped, clipped_count);
    trim_feedback_polygon_near_quad(clipped, &clipped_count);
    feedback_write((GLfloat)GL_POLYGON_TOKEN);
    feedback_write((GLfloat)clipped_count);
    for (GLsizei i = 0; i < clipped_count; ++i) {
        feedback_write_vertex(&clipped[i]);
    }
}

static void record_feedback_pass_through(GLfloat token)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    feedback_write((GLfloat)GL_PASS_THROUGH_TOKEN);
    feedback_write(token);
}

static void record_feedback_pixel_token(GLenum token)
{
    if (render_mode != GL_FEEDBACK) {
        return;
    }
    feedback_write((GLfloat)token);
    feedback_write_raster_vertex();
}

static bool project_window_for_winding(NxglBackendVec3 pos, GLfloat *sx, GLfloat *sy)
{
    GLfloat z;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    *sx = (GLfloat)viewport[0] + ((pos.x + camera_x) / z) * (GLfloat)viewport[2] * 0.5f + (GLfloat)viewport[2] * 0.5f;
    *sy = (GLfloat)viewport[1] + ((pos.y + camera_y) / z) * (GLfloat)viewport[3] * 0.5f + (GLfloat)viewport[3] * 0.5f;
    return true;
}

static bool polygon_culled(const NxglBackendVertex *vertices, int count)
{
    GLfloat xs[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat ys[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat area = 0.0f;
    bool front;

    if (!cull_enabled || count < 3) {
        return false;
    }
    if (cull_face_mode == GL_FRONT_AND_BACK) {
        return true;
    }
    if (count > NXGL_CLIPPED_POLYGON_MAX) {
        count = NXGL_CLIPPED_POLYGON_MAX;
    }
    for (int i = 0; i < count; ++i) {
        if (!project_window_for_winding(vertices[i].pos, &xs[i], &ys[i])) {
            return false;
        }
    }
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        area += xs[i] * ys[next] - xs[next] * ys[i];
    }
    if (fabsf(area) < 0.0001f) {
        return false;
    }
    front = front_face_mode == GL_CCW ? area > 0.0f : area < 0.0f;
    return cull_face_mode == GL_FRONT ? front : !front;
}

static bool polygon_front_facing(const NxglBackendVertex *vertices, int count)
{
    GLfloat xs[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat ys[NXGL_CLIPPED_POLYGON_MAX];
    GLfloat area = 0.0f;

    if (count < 3) {
        return true;
    }
    if (count > NXGL_CLIPPED_POLYGON_MAX) {
        count = NXGL_CLIPPED_POLYGON_MAX;
    }
    for (int i = 0; i < count; ++i) {
        if (!project_window_for_winding(vertices[i].pos, &xs[i], &ys[i])) {
            return true;
        }
    }
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        area += xs[i] * ys[next] - xs[next] * ys[i];
    }
    if (fabsf(area) < 0.0001f) {
        return true;
    }
    return front_face_mode == GL_CCW ? area > 0.0f : area < 0.0f;
}

static void apply_two_sided_lighting(NxglBackendVertex *vertices, int count)
{
    bool front;

    if (!lighting_enabled || !light_model_two_side || count < 3) {
        return;
    }
    front = polygon_front_facing(vertices, count);
    for (int i = 0; i < count; ++i) {
        NxglBackendVec3 normal = vertices[i].normal;
        const MaterialState *material = &material_state;
        if (!front) {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
            material = &material_back_state;
        }
        vertices[i].color = lit_color_with_material(vertices[i].base_color, normal, vertices[i].eye, material);
        vertices[i].color = apply_fog(vertices[i].color, vertices[i].eye);
    }
}

static void emit_clipped_fill_polygon(const NxglBackendVertex *vertices, int count)
{
    NxglBackendVertex clipped[NXGL_CLIPPED_POLYGON_MAX];
    int clipped_count = 0;

    if (!clip_polygon_to_feedback_volume(vertices, count, clipped, &clipped_count)) {
        return;
    }
    if (polygon_culled(clipped, clipped_count)) {
        return;
    }

    if (clipped_count == 4) {
        shadow_fill_bounds(clipped[0], clipped[1], clipped[2], clipped[3], true, false);
        for (int i = 1; i + 1 < clipped_count; ++i) {
            ensure_native_frame_started();
            nxgl_backend_push_triangle(clipped[0], clipped[i], clipped[i + 1]);
        }
        return;
    }

    for (int i = 1; i + 1 < clipped_count; ++i) {
        NxglBackendVertex a = clipped[0];
        NxglBackendVertex b = clipped[i];
        NxglBackendVertex c = clipped[i + 1];
        shadow_fill_bounds(a, b, c, c, false, false);
        ensure_native_frame_started();
        nxgl_backend_push_triangle(a, b, c);
    }
}

static void emit_array_triangle(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c)
{
    NxglBackendVertex vertices[3] = { a, b, c };
    if (primitive_rejected_by_clip_planes(vertices, 3)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 3);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 3);
        return;
    }
    emit_clipped_fill_polygon(vertices, 3);
}

static void emit_array_quad(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d)
{
    NxglBackendVertex vertices[4] = { a, b, c, d };
    if (primitive_rejected_by_clip_planes(vertices, 4)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 4);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 4);
        return;
    }
    emit_clipped_fill_polygon(vertices, 4);
}

static void emit_point_vertex(NxglBackendVertex v)
{
    float half = point_size * 0.0125f;
    NxglBackendVertex a = v;
    NxglBackendVertex b = v;
    NxglBackendVertex c = v;
    NxglBackendVertex d = v;

    if (primitive_rejected_by_clip_planes(&v, 1)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(&v, 1);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_point(&v);
        return;
    }
    if (!primitive_intersects_viewport(&v, 1)) {
        return;
    }

    a.pos.x -= half; a.pos.y += half;
    b.pos.x += half; b.pos.y += half;
    c.pos.x += half; c.pos.y -= half;
    d.pos.x -= half; d.pos.y -= half;
    {
        NxglBackendVertex vertices[4] = { a, b, c, d };
        emit_clipped_fill_polygon(vertices, 4);
    }
}

static void emit_degenerate_line_vertex(NxglBackendVertex v)
{
    GLfloat z;
    float half_x;
    float half_y;
    NxglBackendVertex a = v;
    NxglBackendVertex b = v;
    NxglBackendVertex c = v;
    NxglBackendVertex d = v;

    if (!project_depth_for_window(v.pos, &z)) {
        return;
    }

    half_x = line_width * z / (float)viewport[2];
    half_y = line_width * z / (float)viewport[3];
    a.pos.x -= half_x; a.pos.y += half_y;
    b.pos.x += half_x; b.pos.y += half_y;
    c.pos.x += half_x; c.pos.y -= half_y;
    d.pos.x -= half_x; d.pos.y -= half_y;
    shadow_fill_bounds(a, b, c, d, true, true);
    ensure_native_frame_started();
    nxgl_backend_push_quad(a, b, c, d);
}

static void emit_line_vertices(NxglBackendVertex a, NxglBackendVertex b)
{
    NxglBackendVertex clipped_a;
    NxglBackendVertex clipped_b;
    float dx;
    float dy;
    float len;
    float half = line_width * 0.01f;
    float nx;
    float ny;
    NxglBackendVertex v0;
    NxglBackendVertex v1;
    NxglBackendVertex v2;
    NxglBackendVertex v3;
    NxglBackendVertex vertices[2] = { a, b };

    if (primitive_rejected_by_clip_planes(vertices, 2)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 2);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_line(&a, &b);
        return;
    }

    if (!clip_line_to_feedback_volume(&a, &b, &clipped_a, &clipped_b)) {
        return;
    }

    dx = clipped_b.pos.x - clipped_a.pos.x;
    dy = clipped_b.pos.y - clipped_a.pos.y;
    len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) {
        emit_degenerate_line_vertex(clipped_a);
        return;
    }

    nx = -dy / len * half;
    ny = dx / len * half;
    v0 = clipped_a;
    v1 = clipped_b;
    v2 = clipped_b;
    v3 = clipped_a;
    v0.pos.x += nx; v0.pos.y += ny;
    v1.pos.x += nx; v1.pos.y += ny;
    v2.pos.x -= nx; v2.pos.y -= ny;
    v3.pos.x -= nx; v3.pos.y -= ny;
    shadow_fill_bounds(v0, v1, v2, v3, true, true);
    ensure_native_frame_started();
    nxgl_backend_push_quad(v0, v1, v2, v3);
}

static void emit_triangle_vertices(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c)
{
    NxglBackendVertex vertices[3];
    if (shade_model == GL_FLAT) {
        a.color = c.color;
        b.color = c.color;
    }
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    apply_two_sided_lighting(vertices, 3);
    if (shade_model == GL_FLAT) {
        vertices[0].color = vertices[2].color;
        vertices[1].color = vertices[2].color;
    }
    if (native_fast_fill_enabled()) {
        ensure_native_frame_started();
        nxgl_backend_push_triangle(vertices[0], vertices[1], vertices[2]);
        return;
    }
    if (primitive_rejected_by_clip_planes(vertices, 3)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 3);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 3);
        return;
    }
    if (polygon_mode == GL_POINT) {
        emit_point_vertex(a);
        emit_point_vertex(b);
        emit_point_vertex(c);
    } else if (polygon_mode == GL_LINE) {
        emit_line_vertices(a, b);
        emit_line_vertices(b, c);
        emit_line_vertices(c, a);
    } else {
        emit_clipped_fill_polygon(vertices, 3);
    }
}

static void emit_quad_vertices(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d)
{
    NxglBackendVertex vertices[4];
    if (shade_model == GL_FLAT) {
        a.color = d.color;
        b.color = d.color;
        c.color = d.color;
    }
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    vertices[3] = d;
    apply_two_sided_lighting(vertices, 4);
    if (shade_model == GL_FLAT) {
        vertices[0].color = vertices[3].color;
        vertices[1].color = vertices[3].color;
        vertices[2].color = vertices[3].color;
    }
    if (native_fast_fill_enabled()) {
        ensure_native_frame_started();
        nxgl_backend_push_quad(vertices[0], vertices[1], vertices[2], vertices[3]);
        return;
    }
    if (primitive_rejected_by_clip_planes(vertices, 4)) {
        return;
    }
    if (render_mode == GL_SELECT) {
        record_selection_hit(vertices, 4);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_polygon(vertices, 4);
        return;
    }
    if (polygon_mode == GL_POINT) {
        emit_point_vertex(a);
        emit_point_vertex(b);
        emit_point_vertex(c);
        emit_point_vertex(d);
    } else if (polygon_mode == GL_LINE) {
        emit_line_vertices(a, b);
        emit_line_vertices(b, c);
        emit_line_vertices(c, d);
        emit_line_vertices(d, a);
    } else {
        emit_clipped_fill_polygon(vertices, 4);
    }
}

static void emit_vertices(GLenum mode, const NxglBackendVertex *vertices, GLsizei count)
{
    if (mode == GL_TRIANGLES) {
        for (GLsizei i = 0; i + 2 < count; i += 3) {
            emit_triangle_vertices(vertices[i], vertices[i + 1], vertices[i + 2]);
        }
    } else if (mode == GL_QUADS) {
        for (GLsizei i = 0; i + 3 < count; i += 4) {
            emit_quad_vertices(vertices[i], vertices[i + 1], vertices[i + 2], vertices[i + 3]);
        }
    } else if (mode == GL_TRIANGLE_STRIP) {
        for (GLsizei i = 2; i < count; ++i) {
            if ((i & 1) == 0) {
                emit_triangle_vertices(vertices[i - 2], vertices[i - 1], vertices[i]);
            } else {
                emit_triangle_vertices(vertices[i - 1], vertices[i - 2], vertices[i]);
            }
        }
    } else if (mode == GL_TRIANGLE_FAN || mode == GL_POLYGON) {
        for (GLsizei i = 2; i < count; ++i) {
            emit_triangle_vertices(vertices[0], vertices[i - 1], vertices[i]);
        }
    } else if (mode == GL_QUAD_STRIP) {
        for (GLsizei i = 2; i + 1 < count; i += 2) {
            emit_quad_vertices(vertices[i - 2], vertices[i - 1], vertices[i + 1], vertices[i]);
        }
    } else if (mode == GL_POINTS) {
        for (GLsizei i = 0; i < count; ++i) {
            emit_point_vertex(vertices[i]);
        }
    } else if (mode == GL_LINES) {
        for (GLsizei i = 0; i + 1 < count; i += 2) {
            emit_line_vertices(vertices[i], vertices[i + 1]);
        }
    } else if (mode == GL_LINE_STRIP) {
        for (GLsizei i = 1; i < count; ++i) {
            emit_line_vertices(vertices[i - 1], vertices[i]);
        }
    } else if (mode == GL_LINE_LOOP) {
        for (GLsizei i = 1; i < count; ++i) {
            emit_line_vertices(vertices[i - 1], vertices[i]);
        }
        if (count > 2) {
            emit_line_vertices(vertices[count - 1], vertices[0]);
        }
    }
}

static void emit_array_vertices(GLenum mode, const GLint *indices, GLsizei count)
{
    NxglBackendVertex *vertices;

    if (count <= 0) {
        return;
    }
    vertices = (NxglBackendVertex *)malloc((size_t)count * sizeof(NxglBackendVertex));
    if (vertices == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        vertices[i] = read_array_vertex(indices[i]);
    }
    emit_vertices(mode, vertices, count);
    free(vertices);
}

static uint8_t channel(float value)
{
    if (value < 0.0f) {
        value = 0.0f;
    }
    if (value > 1.0f) {
        value = 1.0f;
    }
    return (uint8_t)(value * 255.0f);
}

static uint32_t color_to_u32(NxglBackendColor color)
{
    return ((uint32_t)channel(color.a) << 24) |
           ((uint32_t)channel(color.r) << 16) |
           ((uint32_t)channel(color.g) << 8) |
           (uint32_t)channel(color.b);
}

static bool compare_alpha(GLfloat alpha, GLenum func, GLfloat ref)
{
    switch (func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return alpha < ref;
    case GL_EQUAL:
        return fabsf(alpha - ref) < 0.0001f;
    case GL_LEQUAL:
        return alpha <= ref || fabsf(alpha - ref) < 0.0001f;
    case GL_GREATER:
        return alpha > ref;
    case GL_NOTEQUAL:
        return fabsf(alpha - ref) >= 0.0001f;
    case GL_GEQUAL:
        return alpha >= ref || fabsf(alpha - ref) < 0.0001f;
    case GL_ALWAYS:
    default:
        return true;
    }
}

static uint32_t apply_logic_op(uint32_t src, uint32_t dst)
{
    uint32_t alpha = src & 0xff000000u;
    src &= 0x00ffffffu;
    dst &= 0x00ffffffu;
    switch (logic_op_mode) {
    case GL_CLEAR:
        return alpha;
    case GL_AND:
        return alpha | (src & dst);
    case GL_AND_REVERSE:
        return alpha | (src & ~dst);
    case GL_COPY:
        return alpha | src;
    case GL_AND_INVERTED:
        return alpha | (~src & dst);
    case GL_NOOP:
        return alpha | dst;
    case GL_XOR:
        return alpha | (src ^ dst);
    case GL_OR:
        return alpha | (src | dst);
    case GL_NOR:
        return alpha | (~(src | dst) & 0x00ffffffu);
    case GL_EQUIV:
        return alpha | (~(src ^ dst) & 0x00ffffffu);
    case GL_INVERT:
        return alpha | (~dst & 0x00ffffffu);
    case GL_OR_REVERSE:
        return alpha | (src | (~dst & 0x00ffffffu));
    case GL_COPY_INVERTED:
        return alpha | (~src & 0x00ffffffu);
    case GL_OR_INVERTED:
        return alpha | ((~src & 0x00ffffffu) | dst);
    case GL_NAND:
        return alpha | (~(src & dst) & 0x00ffffffu);
    case GL_SET:
        return alpha | 0x00ffffffu;
    default:
        return alpha | src;
    }
}

static GLfloat component_from_u32(uint32_t color, int component)
{
    uint32_t shift = component == 0 ? 16u : (component == 1 ? 8u : 0u);
    return (GLfloat)((color >> shift) & 0xffu) / 255.0f;
}

static GLfloat alpha_from_u32(uint32_t color)
{
    return (GLfloat)((color >> 24) & 0xffu) / 255.0f;
}

static GLfloat blend_factor_component(GLenum factor, int component, NxglBackendColor src, uint32_t dst)
{
    GLfloat src_component = component == 0 ? src.r : (component == 1 ? src.g : src.b);
    GLfloat dst_component = component_from_u32(dst, component);
    GLfloat dst_alpha = alpha_from_u32(dst);

    switch (factor) {
    case GL_ZERO:
        return 0.0f;
    case GL_ONE:
        return 1.0f;
    case GL_SRC_COLOR:
        return src_component;
    case GL_ONE_MINUS_SRC_COLOR:
        return 1.0f - src_component;
    case GL_DST_COLOR:
        return dst_component;
    case GL_ONE_MINUS_DST_COLOR:
        return 1.0f - dst_component;
    case GL_SRC_ALPHA:
        return src.a;
    case GL_ONE_MINUS_SRC_ALPHA:
        return 1.0f - src.a;
    case GL_DST_ALPHA:
        return dst_alpha;
    case GL_ONE_MINUS_DST_ALPHA:
        return 1.0f - dst_alpha;
    case GL_SRC_ALPHA_SATURATE:
        return fminf(src.a, 1.0f - dst_alpha);
    default:
        return 1.0f;
    }
}

static uint32_t apply_blend(NxglBackendColor src, uint32_t dst)
{
    NxglBackendColor out;

    if (!blend_enabled) {
        return color_to_u32(src);
    }

    out.r = clamp01(src.r * blend_factor_component(blend_sfactor, 0, src, dst) +
                    component_from_u32(dst, 0) * blend_factor_component(blend_dfactor, 0, src, dst));
    out.g = clamp01(src.g * blend_factor_component(blend_sfactor, 1, src, dst) +
                    component_from_u32(dst, 1) * blend_factor_component(blend_dfactor, 1, src, dst));
    out.b = clamp01(src.b * blend_factor_component(blend_sfactor, 2, src, dst) +
                    component_from_u32(dst, 2) * blend_factor_component(blend_dfactor, 2, src, dst));
    out.a = src.a;
    return color_to_u32(out);
}

static uint32_t apply_color_write_mask(uint32_t src, uint32_t dst)
{
    uint32_t out = dst;
    if (color_write_mask[0]) {
        out = (out & 0x0000ffffu) | (src & 0x00ff0000u);
    }
    if (color_write_mask[1]) {
        out = (out & 0x00ff00ffu) | (src & 0x0000ff00u);
    }
    if (color_write_mask[2]) {
        out = (out & 0x00ffff00u) | (src & 0x000000ffu);
    }
    if (color_write_mask[3]) {
        out = (out & 0x00ffffffu) | (src & 0xff000000u);
    }
    return out;
}

static bool compare_depth(GLfloat incoming, GLfloat current)
{
    if (!depth_test_enabled) {
        return true;
    }
    switch (depth_func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return incoming < current;
    case GL_EQUAL:
        return incoming == current;
    case GL_LEQUAL:
        return incoming <= current;
    case GL_GREATER:
        return incoming > current;
    case GL_NOTEQUAL:
        return incoming != current;
    case GL_GEQUAL:
        return incoming >= current;
    case GL_ALWAYS:
        return true;
    default:
        return incoming <= current;
    }
}

static bool compare_stencil(uint8_t value)
{
    GLuint lhs = ((GLuint)stencil_ref) & stencil_value_mask;
    GLuint rhs = ((GLuint)value) & stencil_value_mask;

    if (!stencil_test_enabled) {
        return true;
    }

    switch (stencil_func) {
    case GL_NEVER:
        return false;
    case GL_LESS:
        return lhs < rhs;
    case GL_EQUAL:
        return lhs == rhs;
    case GL_LEQUAL:
        return lhs <= rhs;
    case GL_GREATER:
        return lhs > rhs;
    case GL_NOTEQUAL:
        return lhs != rhs;
    case GL_GEQUAL:
        return lhs >= rhs;
    case GL_ALWAYS:
    default:
        return true;
    }
}

static uint8_t apply_stencil_op(uint8_t current, GLenum op)
{
    switch (op) {
    case GL_KEEP:
        return current;
    case GL_ZERO:
        return 0;
    case GL_REPLACE:
        return (uint8_t)(stencil_ref & 0xff);
    case GL_INCR:
        return current == 0xff ? 0xff : (uint8_t)(current + 1u);
    case GL_DECR:
        return current == 0 ? 0 : (uint8_t)(current - 1u);
    case GL_INVERT:
        return (uint8_t)~current;
    default:
        return current;
    }
}

static void write_stencil_value(size_t index, uint8_t value)
{
    uint8_t mask = (uint8_t)(stencil_write_mask & 0xffu);
    uint8_t current;

    if (shadow_stencil_buffer == NULL) {
        return;
    }
    current = shadow_stencil_buffer[index];
    shadow_stencil_buffer[index] = (uint8_t)((current & (uint8_t)~mask) | (value & mask));
}

static bool shadow_clear_bounds(int *min_x, int *min_y, int *max_x, int *max_y)
{
    *min_x = 0;
    *min_y = 0;
    *max_x = shadow_width - 1;
    *max_y = shadow_height - 1;
    if (scissor_test_enabled) {
        int sx1 = scissor_box[0];
        int sy1 = shadow_height - (scissor_box[1] + scissor_box[3]);
        int sx2 = scissor_box[0] + scissor_box[2] - 1;
        int sy2 = shadow_height - scissor_box[1] - 1;
        if (sx1 < 0) sx1 = 0;
        if (sy1 < 0) sy1 = 0;
        if (sx2 >= shadow_width) sx2 = shadow_width - 1;
        if (sy2 >= shadow_height) sy2 = shadow_height - 1;
        if (*min_x < sx1) *min_x = sx1;
        if (*max_x > sx2) *max_x = sx2;
        if (*min_y < sy1) *min_y = sy1;
        if (*max_y > sy2) *max_y = sy2;
    }
    return *min_x <= *max_x && *min_y <= *max_y;
}

static void shadow_clear(uint32_t color)
{
    int min_x, min_y, max_x, max_y;
    if (!shadow_readback_enabled || shadow_color_buffer == NULL) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        uint32_t *row = shadow_color_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            row[x] = apply_color_write_mask(color, row[x]);
        }
    }
}

static void shadow_clear_depth(GLfloat depth)
{
    int min_x, min_y, max_x, max_y;

    if (!shadow_readback_enabled || shadow_depth_buffer == NULL || !depth_write_enabled) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        GLfloat *row = shadow_depth_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            row[x] = depth;
        }
    }
}

static void shadow_clear_stencil(uint8_t stencil)
{
    int min_x, min_y, max_x, max_y;
    uint8_t mask = (uint8_t)(stencil_write_mask & 0xffu);
    if (!shadow_readback_enabled || shadow_stencil_buffer == NULL) {
        return;
    }
    if (!shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y) || mask == 0u) {
        return;
    }
    for (int y = min_y; y <= max_y; ++y) {
        uint8_t *row = shadow_stencil_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            uint8_t current = row[x];
            row[x] = (uint8_t)((current & (uint8_t)~mask) | (stencil & mask));
        }
    }
}

static bool ensure_accum_buffer(void)
{
    size_t count;

    if (accum_buffer != NULL) {
        return true;
    }

    count = (size_t)shadow_width * (size_t)shadow_height * 4u;
    accum_buffer = (float *)malloc(count * sizeof(float));
    if (accum_buffer == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return false;
    }
    memset(accum_buffer, 0, count * sizeof(float));
    return true;
}

static bool shadow_project(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * (float)viewport[2] * 0.5f + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)x;
    *sy = shadow_height - 1 - (int)y_bottom;
    return true;
}

static bool shadow_project_wide_x(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;
    float cube_x_scale;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    cube_x_scale = (float)viewport[2] * (27.0f / 32.0f);
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * cube_x_scale + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)x;
    *sy = shadow_height - 1 - (int)y_bottom;
    return true;
}

static bool shadow_project_rounded(NxglBackendVec3 pos, int *sx, int *sy)
{
    GLfloat z;
    float x;
    float y_bottom;

    if (!project_depth_for_window(pos, &z)) {
        return false;
    }
    x = (float)viewport[0] + ((pos.x + camera_x) / z) * (float)viewport[2] * 0.5f + (float)viewport[2] * 0.5f;
    y_bottom = (float)viewport[1] + ((pos.y + camera_y) / z) * (float)viewport[3] * 0.5f + (float)viewport[3] * 0.5f;
    *sx = (int)floorf(x + 0.5f);
    *sy = shadow_height - 1 - (int)floorf(y_bottom + 0.5f);
    return true;
}

static bool cube_texture_enabled_for_shadow(void)
{
    for (int unit = 0; unit < 4; ++unit) {
        if (texture_cube_map_enabled[unit]) {
            return true;
        }
    }
    return false;
}

static GLfloat shadow_depth_value(const NxglBackendVertex *vertices, int count)
{
    GLfloat sum = 0.0f;

    if (count <= 0) {
        return 1.0f;
    }
    if (projected_path_active()) {
        for (int i = 0; i < count; ++i) {
            sum += vertices[i].window_z;
        }
        return clamp01(sum / (GLfloat)count);
    }
    for (int i = 0; i < count; ++i) {
        sum += -(vertices[i].pos.z + camera_z);
    }
    return map_depth_range(clamp01((sum / (GLfloat)count) / 100.0f));
}

static void set_raster_position_from_vertex4(float x, float y, float z, float w)
{
    NxglBackendVertex vertex;
    int sx = 0;
    int sy = 0;

    memset(&vertex, 0, sizeof(vertex));
    init_vertex_position(&vertex, x, y, z, w);
    if (!point_inside_clip_planes(vertex.eye)) {
        current_raster_position_valid = false;
        return;
    }
    if (projected_path_active()) {
        for (int plane = 0; plane < 6; ++plane) {
            if (feedback_clip_plane_value(&vertex, plane) < 0.0f) {
                current_raster_position_valid = false;
                return;
            }
        }
    }
    if (!shadow_project_rounded(vertex.pos, &sx, &sy)) {
        current_raster_position_valid = false;
        return;
    }

    current_raster_position[0] = (GLfloat)sx;
    current_raster_position[1] = (GLfloat)(shadow_height - 1 - sy);
    current_raster_position[2] = projected_path_active() ? vertex.window_z : map_depth_range(eye_depth_normalized(vertex.pos));
    current_raster_position[3] = projected_path_active() ? vertex.clip_w : 1.0f;
    current_raster_position_valid = sx >= viewport[0] &&
                                    sx < viewport[0] + viewport[2] &&
                                    current_raster_position[1] >= viewport[1] &&
                                    current_raster_position[1] < viewport[1] + viewport[3] &&
                                    sx >= 0 && sy >= 0 && sx < shadow_width && sy < shadow_height;
}

static void set_raster_position_from_vertex(float x, float y, float z)
{
    set_raster_position_from_vertex4(x, y, z, 1.0f);
}

static uint32_t pixel_from_source(const uint8_t *src, GLenum format)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (format == GL_RGBA || format == GL_RGB) {
        r = src[0];
        g = src[1];
        b = src[2];
    } else {
        b = src[0];
        g = src[1];
        r = src[2];
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static GLfloat map_component(GLfloat value, int map_index)
{
    PixelMapState *map;
    GLfloat position;
    int lo;
    int hi;
    GLfloat frac;

    if (map_index < 0 || map_index >= 10) {
        return value;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 1) {
        return map->values[0];
    }
    position = clamp01(value) * (GLfloat)(map->size - 1);
    lo = (int)floorf(position);
    hi = lo + 1;
    if (lo < 0) lo = 0;
    if (hi >= map->size) hi = map->size - 1;
    frac = position - (GLfloat)lo;
    return clamp01(map->values[lo] * (1.0f - frac) + map->values[hi] * frac);
}

static GLuint map_integer_value(GLuint value, int map_index, GLuint max_value)
{
    PixelMapState *map;
    GLuint index;

    if (map_index < 0 || map_index >= 10) {
        return value;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 0) {
        return value;
    }
    index = value;
    if (index >= (GLuint)map->size) {
        index = (GLuint)map->size - 1u;
    }
    return (GLuint)(clamp01(map->values[index]) * (GLfloat)max_value + 0.5f);
}

static GLfloat map_index_to_component(GLuint index, int map_index)
{
    PixelMapState *map;

    if (map_index < 0 || map_index >= 10) {
        return 0.0f;
    }
    map = &pixel_maps[map_index];
    if (map->size <= 0) {
        return 0.0f;
    }
    if (index >= (GLuint)map->size) {
        index = (GLuint)map->size - 1u;
    }
    return clamp01(map->values[index]);
}

static NxglBackendColor apply_pixel_transfer_rgba_color(const uint8_t rgba[4])
{
    GLfloat channels[4] = {
        (GLfloat)rgba[0] / 255.0f,
        (GLfloat)rgba[1] / 255.0f,
        (GLfloat)rgba[2] / 255.0f,
        (GLfloat)rgba[3] / 255.0f
    };
    channels[0] = map_component(clamp01(channels[0] * pixel_transfer_scale[0] + pixel_transfer_bias[0]), pixel_map_index(GL_PIXEL_MAP_R_TO_R));
    channels[1] = map_component(clamp01(channels[1] * pixel_transfer_scale[1] + pixel_transfer_bias[1]), pixel_map_index(GL_PIXEL_MAP_G_TO_G));
    channels[2] = map_component(clamp01(channels[2] * pixel_transfer_scale[2] + pixel_transfer_bias[2]), pixel_map_index(GL_PIXEL_MAP_B_TO_B));
    channels[3] = map_component(clamp01(channels[3] * pixel_transfer_scale[3] + pixel_transfer_bias[3]), pixel_map_index(GL_PIXEL_MAP_A_TO_A));
    return (NxglBackendColor){ channels[0], channels[1], channels[2], channels[3] };
}

static uint32_t apply_pixel_transfer_rgba(const uint8_t rgba[4])
{
    NxglBackendColor color = apply_pixel_transfer_rgba_color(rgba);
    return color_to_u32(color);
}

static uint32_t apply_pixel_transfer_u32(uint32_t pixel)
{
    uint8_t rgba[4] = {
        (uint8_t)((pixel >> 16) & 0xff),
        (uint8_t)((pixel >> 8) & 0xff),
        (uint8_t)(pixel & 0xff),
        (uint8_t)((pixel >> 24) & 0xff)
    };
    return apply_pixel_transfer_rgba(rgba);
}

static uint8_t apply_pixel_transfer_stencil(uint8_t stencil)
{
    return (uint8_t)map_integer_value(stencil, pixel_map_index(GL_PIXEL_MAP_S_TO_S), 255u);
}

static int pixel_zoom_replicate(GLfloat factor)
{
    GLfloat abs_factor = factor < 0.0f ? -factor : factor;
    int replicate = (int)(abs_factor + 0.0001f);
    return replicate < 1 ? 1 : replicate;
}

static bool pixel_inside_scissor(GLint x, GLint y)
{
    if (!scissor_test_enabled) {
        return true;
    }
    return x >= scissor_box[0] &&
           y >= scissor_box[1] &&
           x < scissor_box[0] + scissor_box[2] &&
           y < scissor_box[1] + scissor_box[3];
}

static void write_shadow_color_fragment(GLint x, GLint y, NxglBackendColor color, GLfloat depth)
{
    uint32_t *dst;
    uint32_t src;
    size_t index;
    bool stencil_pass;
    bool depth_pass;

    if (draw_buffer_mode == GL_NONE || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    if (alpha_test_enabled && !compare_alpha(color.a, alpha_test_func, alpha_test_ref)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    stencil_pass = compare_stencil(shadow_stencil_buffer != NULL ? shadow_stencil_buffer[index] : 0);
    depth_pass = shadow_depth_buffer == NULL || compare_depth(depth, shadow_depth_buffer[index]);
    if (!stencil_pass) {
        if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
            write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_fail));
        }
        return;
    }
    if (!depth_pass) {
        if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
            write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zfail));
        }
        return;
    }
    if (depth_test_enabled && depth_write_enabled && shadow_depth_buffer != NULL) {
        shadow_depth_buffer[index] = depth;
    }
    if (stencil_test_enabled && shadow_stencil_buffer != NULL) {
        write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zpass));
    }
    dst = shadow_color_buffer + index;
    src = color_logic_op_enabled ? apply_logic_op(color_to_u32(color), *dst) : apply_blend(color, *dst);
    *dst = apply_color_write_mask(src, *dst);
}

static void write_shadow_pixel(GLint x, GLint y, uint32_t color)
{
    NxglBackendColor src = {
        (GLfloat)((color >> 16) & 0xffu) / 255.0f,
        (GLfloat)((color >> 8) & 0xffu) / 255.0f,
        (GLfloat)(color & 0xffu) / 255.0f,
        alpha_from_u32(color)
    };
    write_shadow_color_fragment(x, y, src, current_raster_position[2]);
}

static bool valid_depth_pixel_type(GLenum type)
{
    return type == GL_FLOAT || type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT;
}

static bool valid_stencil_pixel_type(GLenum type)
{
    return type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT;
}

static GLfloat unpack_depth_pixel(const uint8_t *src, GLenum type)
{
    if (type == GL_FLOAT) {
        return clamp01(((const GLfloat *)src)[0]);
    }
    if (type == GL_UNSIGNED_BYTE) {
        return (GLfloat)src[0] / 255.0f;
    }
    if (type == GL_UNSIGNED_SHORT) {
        return (GLfloat)((const GLushort *)src)[0] / 65535.0f;
    }
    return (GLfloat)((const GLuint *)src)[0] / 4294967295.0f;
}

static uint8_t unpack_stencil_pixel(const uint8_t *src, GLenum type)
{
    if (type == GL_UNSIGNED_BYTE) {
        return src[0];
    }
    if (type == GL_UNSIGNED_SHORT) {
        return (uint8_t)(((const GLushort *)src)[0] & 0xffu);
    }
    return (uint8_t)(((const GLuint *)src)[0] & 0xffu);
}

static void write_shadow_depth_pixel(GLint x, GLint y, GLfloat depth)
{
    size_t index;

    if (shadow_depth_buffer == NULL || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    if (!depth_test_enabled || compare_depth(depth, shadow_depth_buffer[index])) {
        if (!depth_test_enabled || depth_write_enabled) {
            shadow_depth_buffer[index] = depth;
        }
    }
}

static void write_shadow_stencil_pixel(GLint x, GLint y, uint8_t stencil)
{
    size_t index;

    if (shadow_stencil_buffer == NULL || x < 0 || y < 0 || x >= shadow_width || y >= shadow_height || !pixel_inside_scissor(x, y)) {
        return;
    }
    index = (size_t)(shadow_height - 1 - y) * (size_t)shadow_width + (size_t)x;
    write_stencil_value(index, stencil);
}

static uint16_t dxt_read_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static uint8_t expand_bits_u8(uint32_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return (uint8_t)((value * 255u + max / 2u) / max);
}

static void decode_rgb565(uint16_t value, uint8_t out[4])
{
    out[0] = expand_bits_u8((value >> 11) & 0x1Fu, 5);
    out[1] = expand_bits_u8((value >> 5) & 0x3Fu, 6);
    out[2] = expand_bits_u8(value & 0x1Fu, 5);
    out[3] = 255;
}

static NxglBackendColor sample_dxt_color(const TextureLevel *image, int x, int y)
{
    int block_size = compressed_block_size((GLenum)image->internal_format);
    int blocks_w = (image->width + 3) / 4;
    int block_x = x / 4;
    int block_y = y / 4;
    int local = (y & 3) * 4 + (x & 3);
    const uint8_t *block = image->compressed_data + ((size_t)block_y * (size_t)blocks_w + (size_t)block_x) * (size_t)block_size;
    const uint8_t *color_block = block;
    uint8_t colors[4][4];
    uint16_t color0;
    uint16_t color1;
    uint32_t indices;
    int index;
    uint8_t alpha = 255;

    if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
        alpha = (uint8_t)((block[local / 2] >> ((local & 1) ? 4 : 0)) & 0x0F);
        alpha = (uint8_t)((alpha << 4) | alpha);
        color_block = block + 8;
    } else if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
        uint8_t alpha0 = block[0];
        uint8_t alpha1 = block[1];
        uint64_t alpha_bits = 0;
        int alpha_index;
        uint8_t alpha_values[8];
        for (int i = 0; i < 6; ++i) {
            alpha_bits |= (uint64_t)block[2 + i] << (8 * i);
        }
        alpha_index = (int)((alpha_bits >> (3 * local)) & 0x7u);
        alpha_values[0] = alpha0;
        alpha_values[1] = alpha1;
        if (alpha0 > alpha1) {
            alpha_values[2] = (uint8_t)((6 * alpha0 + alpha1) / 7);
            alpha_values[3] = (uint8_t)((5 * alpha0 + 2 * alpha1) / 7);
            alpha_values[4] = (uint8_t)((4 * alpha0 + 3 * alpha1) / 7);
            alpha_values[5] = (uint8_t)((3 * alpha0 + 4 * alpha1) / 7);
            alpha_values[6] = (uint8_t)((2 * alpha0 + 5 * alpha1) / 7);
            alpha_values[7] = (uint8_t)((alpha0 + 6 * alpha1) / 7);
        } else {
            alpha_values[2] = (uint8_t)((4 * alpha0 + alpha1) / 5);
            alpha_values[3] = (uint8_t)((3 * alpha0 + 2 * alpha1) / 5);
            alpha_values[4] = (uint8_t)((2 * alpha0 + 3 * alpha1) / 5);
            alpha_values[5] = (uint8_t)((alpha0 + 4 * alpha1) / 5);
            alpha_values[6] = 0;
            alpha_values[7] = 255;
        }
        alpha = alpha_values[alpha_index];
        color_block = block + 8;
    }

    color0 = dxt_read_u16(color_block);
    color1 = dxt_read_u16(color_block + 2);
    decode_rgb565(color0, colors[0]);
    decode_rgb565(color1, colors[1]);
    if (color0 > color1 || image->internal_format != GL_COMPRESSED_RGB_S3TC_DXT1_EXT) {
        for (int c = 0; c < 3; ++c) {
            colors[2][c] = (uint8_t)((2 * colors[0][c] + colors[1][c]) / 3);
            colors[3][c] = (uint8_t)((colors[0][c] + 2 * colors[1][c]) / 3);
        }
        colors[2][3] = 255;
        colors[3][3] = 255;
    } else {
        for (int c = 0; c < 3; ++c) {
            colors[2][c] = (uint8_t)((colors[0][c] + colors[1][c]) / 2);
            colors[3][c] = 0;
        }
        colors[2][3] = 255;
        colors[3][3] = 0;
    }

    indices = (uint32_t)color_block[4] |
              ((uint32_t)color_block[5] << 8) |
              ((uint32_t)color_block[6] << 16) |
              ((uint32_t)color_block[7] << 24);
    index = (int)((indices >> (2 * local)) & 0x3u);
    if (image->internal_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && color0 <= color1 && index == 3) {
        alpha = 0;
    }
    return (NxglBackendColor){
        (GLfloat)colors[index][0] / 255.0f,
        (GLfloat)colors[index][1] / 255.0f,
        (GLfloat)colors[index][2] / 255.0f,
        (GLfloat)alpha / 255.0f
    };
}

static NxglBackendColor sample_bound_texture_unit_color(int unit, GLfloat u, GLfloat v)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    int x;
    int y;

    if (unit < 0 || unit >= 4) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    if (texture_2d_enabled[unit] && texture_binding_2d[unit] > 0 && texture_binding_2d[unit] < 16) {
        texture = &texture_objects[texture_binding_2d[unit]];
        image = select_texture_level_for_lod(texture, texture->levels, 0.0f);
    } else if (texture_1d_enabled[unit] && texture_binding_1d[unit] > 0 && texture_binding_1d[unit] < 16) {
        texture = &texture_objects[texture_binding_1d[unit]];
        image = select_texture_level_for_lod(texture, texture->levels_1d, 0.0f);
        v = 0.0f;
    } else {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    if (!texture->allocated || image == NULL || !image->defined) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (texture->wrap_s == GL_REPEAT) {
        u = u - floorf(u);
    } else {
        u = clamp01(u);
    }
    if (texture->wrap_t == GL_REPEAT) {
        v = v - floorf(v);
    } else {
        v = clamp01(v);
    }

    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;

    if (image->compressed) {
        if (image->compressed_data == NULL) {
            return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        return sample_dxt_color(image, x, y);
    }
    if (image->rgba == NULL) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    pixel = image->rgba + ((size_t)y * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static NxglBackendColor sample_bound_cube_texture_unit_color(int unit, GLfloat s, GLfloat t, GLfloat r)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    GLfloat abs_s = fabsf(s);
    GLfloat abs_t = fabsf(t);
    GLfloat abs_r = fabsf(r);
    GLfloat ma;
    GLfloat sc;
    GLfloat tc;
    GLfloat u;
    GLfloat v;
    int face;
    int x;
    int y;

    if (unit < 0 || unit >= 4 || !texture_cube_map_enabled[unit] || texture_binding_cube_map[unit] == 0 || texture_binding_cube_map[unit] >= 16) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    texture = &texture_objects[texture_binding_cube_map[unit]];
    if (!texture->allocated || !cube_levels_complete(texture)) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (abs_s >= abs_t && abs_s >= abs_r) {
        ma = abs_s;
        if (s >= 0.0f) {
            face = 0;
            sc = -r;
            tc = -t;
        } else {
            face = 1;
            sc = r;
            tc = -t;
        }
    } else if (abs_t >= abs_s && abs_t >= abs_r) {
        ma = abs_t;
        if (t >= 0.0f) {
            face = 2;
            sc = s;
            tc = r;
        } else {
            face = 3;
            sc = s;
            tc = -r;
        }
    } else {
        ma = abs_r;
        if (r >= 0.0f) {
            face = 4;
            sc = s;
            tc = -t;
        } else {
            face = 5;
            sc = -s;
            tc = -t;
        }
    }

    if (ma <= 0.0f) {
        face = 4;
        u = 0.5f;
        v = 0.5f;
    } else {
        u = clamp01((sc / ma + 1.0f) * 0.5f);
        v = clamp01((tc / ma + 1.0f) * 0.5f);
    }

    {
        GLint level = select_cube_level_for_lod(texture, 0.0f);
        if (level < 0) {
            return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        image = &texture->cube_faces[face][level];
    }
    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;

    pixel = image->rgba + ((size_t)y * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static NxglBackendColor sample_bound_texture3d_unit_color(int unit, GLfloat u, GLfloat v, GLfloat r)
{
    TextureObject *texture;
    TextureLevel *image;
    const uint8_t *pixel;
    int x;
    int y;
    int z;

    if (unit < 0 || unit >= 4 || !texture_3d_enabled[unit] || texture_binding_3d[unit] == 0 || texture_binding_3d[unit] >= 16) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }
    texture = &texture_objects[texture_binding_3d[unit]];
    image = select_texture_level_for_lod(texture, texture->levels_3d, 0.0f);
    if (!texture->allocated || image == NULL || !image->defined || image->compressed || image->rgba == NULL) {
        return (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    }

    if (texture->wrap_s == GL_REPEAT) {
        u = u - floorf(u);
    } else {
        u = clamp01(u);
    }
    if (texture->wrap_t == GL_REPEAT) {
        v = v - floorf(v);
    } else {
        v = clamp01(v);
    }
    if (texture->wrap_r == GL_REPEAT) {
        r = r - floorf(r);
    } else {
        r = clamp01(r);
    }

    x = (int)(u * (GLfloat)(image->width - 1) + 0.5f);
    y = (int)(v * (GLfloat)(image->height - 1) + 0.5f);
    z = (int)(r * (GLfloat)(image->depth - 1) + 0.5f);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (z < 0) z = 0;
    if (x >= image->width) x = image->width - 1;
    if (y >= image->height) y = image->height - 1;
    if (z >= image->depth) z = image->depth - 1;

    pixel = image->rgba + (((size_t)z * (size_t)image->height + (size_t)y) * (size_t)image->width + (size_t)x) * 4u;
    return (NxglBackendColor){
        (GLfloat)pixel[0] / 255.0f,
        (GLfloat)pixel[1] / 255.0f,
        (GLfloat)pixel[2] / 255.0f,
        (GLfloat)pixel[3] / 255.0f
    };
}

static bool texture_unit_enabled_for_shadow(int unit)
{
    if (unit < 0 || unit >= 4) {
        return false;
    }
    return texture_2d_enabled[unit] ||
           texture_1d_enabled[unit] ||
           texture_3d_enabled[unit] ||
           texture_cube_map_enabled[unit];
}

static NxglBackendColor sample_bound_texture_color_for_unit(int unit, GLfloat u, GLfloat v, GLfloat r)
{
    if (texture_cube_map_enabled[unit]) {
        return sample_bound_cube_texture_unit_color(unit, u, v, r);
    }
    if (texture_3d_enabled[unit]) {
        return sample_bound_texture3d_unit_color(unit, u, v, r);
    }
    return sample_bound_texture_unit_color(unit, u, v);
}

static NxglBackendColor combine_source_color(int unit, GLenum source, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                    const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    if (source == GL_TEXTURE) {
        return texel;
    }
    if (source >= GL_TEXTURE0 && source <= GL_TEXTURE3) {
        int source_unit = (int)(source - GL_TEXTURE0);
        return sample_bound_texture_color_for_unit(source_unit, u[source_unit], v[source_unit], r[source_unit]);
    }
    if (source == GL_CONSTANT) {
        return texture_env_color[unit];
    }
    if (source == GL_PRIMARY_COLOR) {
        return primary;
    }
    return previous;
}

static GLfloat combine_operand_component(NxglBackendColor source, GLenum operand, int component)
{
    GLfloat color_component = component == 0 ? source.r : (component == 1 ? source.g : source.b);

    if (operand == GL_SRC_ALPHA) {
        return source.a;
    }
    if (operand == GL_ONE_MINUS_SRC_ALPHA) {
        return 1.0f - source.a;
    }
    if (operand == GL_ONE_MINUS_SRC_COLOR) {
        return 1.0f - color_component;
    }
    return color_component;
}

static GLfloat combine_arg_component(int unit, int arg, int component, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                     const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor source = combine_source_color(unit, texture_source_rgb[unit][arg], primary, previous, texel, u, v, r);
    return combine_operand_component(source, texture_operand_rgb[unit][arg], component);
}

static GLfloat combine_alpha_arg_component(int unit, int arg, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                           const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor source = combine_source_color(unit, texture_source_alpha[unit][arg], primary, previous, texel, u, v, r);

    if (texture_operand_alpha[unit][arg] == GL_ONE_MINUS_SRC_ALPHA) {
        return 1.0f - source.a;
    }
    return source.a;
}

static GLfloat combine_alpha_value(int unit, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                   const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    GLfloat arg0 = combine_alpha_arg_component(unit, 0, primary, previous, texel, u, v, r);
    GLfloat arg1 = combine_alpha_arg_component(unit, 1, primary, previous, texel, u, v, r);
    GLfloat arg2 = combine_alpha_arg_component(unit, 2, primary, previous, texel, u, v, r);
    GLfloat value;

    if (texture_combine_alpha[unit] == GL_REPLACE) {
        value = arg0;
    } else if (texture_combine_alpha[unit] == GL_ADD) {
        value = arg0 + arg1;
    } else if (texture_combine_alpha[unit] == GL_ADD_SIGNED) {
        value = arg0 + arg1 - 0.5f;
    } else if (texture_combine_alpha[unit] == GL_SUBTRACT) {
        value = arg0 - arg1;
    } else if (texture_combine_alpha[unit] == GL_INTERPOLATE) {
        value = arg0 * arg2 + arg1 * (1.0f - arg2);
    } else {
        value = arg0 * arg1;
    }
    return clamp01(value * texture_alpha_scale[unit]);
}

static NxglBackendColor combined_texture_env_color(int unit, NxglBackendColor primary, NxglBackendColor previous, NxglBackendColor texel,
                                          const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor out = previous;
    GLfloat arg[3];

    if (texture_combine_rgb[unit] == GL_DOT3_RGB || texture_combine_rgb[unit] == GL_DOT3_RGBA) {
        GLfloat dot = 0.0f;
        for (int c = 0; c < 3; ++c) {
            GLfloat a = combine_arg_component(unit, 0, c, primary, previous, texel, u, v, r) - 0.5f;
            GLfloat b = combine_arg_component(unit, 1, c, primary, previous, texel, u, v, r) - 0.5f;
            dot += a * b;
        }
        dot = clamp01(dot * 4.0f);
        out.r = dot;
        out.g = dot;
        out.b = dot;
        out.a = texture_combine_rgb[unit] == GL_DOT3_RGBA ? dot : combine_alpha_value(unit, primary, previous, texel, u, v, r);
        return out;
    }

    for (int c = 0; c < 3; ++c) {
        arg[0] = combine_arg_component(unit, 0, c, primary, previous, texel, u, v, r);
        arg[1] = combine_arg_component(unit, 1, c, primary, previous, texel, u, v, r);
        arg[2] = combine_arg_component(unit, 2, c, primary, previous, texel, u, v, r);

        if (texture_combine_rgb[unit] == GL_REPLACE) {
            if (c == 0) out.r = arg[0];
            if (c == 1) out.g = arg[0];
            if (c == 2) out.b = arg[0];
        } else if (texture_combine_rgb[unit] == GL_ADD) {
            GLfloat value = clamp01(arg[0] + arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_ADD_SIGNED) {
            GLfloat value = clamp01(arg[0] + arg[1] - 0.5f);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_SUBTRACT) {
            GLfloat value = clamp01(arg[0] - arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else if (texture_combine_rgb[unit] == GL_INTERPOLATE) {
            GLfloat value = clamp01(arg[0] * arg[2] + arg[1] * (1.0f - arg[2]));
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        } else {
            GLfloat value = clamp01(arg[0] * arg[1]);
            if (c == 0) out.r = value;
            if (c == 1) out.g = value;
            if (c == 2) out.b = value;
        }
    }

    out.r = clamp01(out.r * texture_rgb_scale[unit]);
    out.g = clamp01(out.g * texture_rgb_scale[unit]);
    out.b = clamp01(out.b * texture_rgb_scale[unit]);
    out.a = combine_alpha_value(unit, primary, previous, texel, u, v, r);
    return out;
}

static NxglBackendColor texture_env_color_for_shadow_unit(int unit, NxglBackendColor primary, NxglBackendColor previous,
                                                 const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor texel = sample_bound_texture_color_for_unit(unit, u[unit], v[unit], r[unit]);
    NxglBackendColor env = texture_env_color[unit];
    NxglBackendColor out = previous;

    if (!texture_unit_enabled_for_shadow(unit)) {
        return previous;
    }

    if (texture_env_mode[unit] == GL_COMBINE) {
        return combined_texture_env_color(unit, primary, previous, texel, u, v, r);
    }

    if (texture_env_mode[unit] == GL_REPLACE) {
        out = texel;
    } else if (texture_env_mode[unit] == GL_DECAL) {
        out.r = previous.r * (1.0f - texel.a) + texel.r * texel.a;
        out.g = previous.g * (1.0f - texel.a) + texel.g * texel.a;
        out.b = previous.b * (1.0f - texel.a) + texel.b * texel.a;
        out.a = previous.a;
    } else if (texture_env_mode[unit] == GL_BLEND) {
        out.r = previous.r * (1.0f - texel.r) + env.r * texel.r;
        out.g = previous.g * (1.0f - texel.g) + env.g * texel.g;
        out.b = previous.b * (1.0f - texel.b) + env.b * texel.b;
        out.a = previous.a * texel.a;
    } else if (texture_env_mode[unit] == GL_ADD) {
        out.r = clamp01(previous.r + texel.r);
        out.g = clamp01(previous.g + texel.g);
        out.b = clamp01(previous.b + texel.b);
        out.a = previous.a * texel.a;
    } else {
        out.r = previous.r * texel.r;
        out.g = previous.g * texel.g;
        out.b = previous.b * texel.b;
        out.a = previous.a * texel.a;
    }

    out.r = clamp01(out.r);
    out.g = clamp01(out.g);
    out.b = clamp01(out.b);
    out.a = clamp01(out.a);
    return out;
}

static void mark_crossbar_source_units(int unit, bool source_only[4])
{
    if (texture_env_mode[unit] != GL_COMBINE) {
        return;
    }
    for (int arg = 0; arg < 3; ++arg) {
        GLenum rgb_source = texture_source_rgb[unit][arg];
        GLenum alpha_source = texture_source_alpha[unit][arg];

        if (rgb_source >= GL_TEXTURE0 && rgb_source <= GL_TEXTURE3) {
            int source_unit = (int)(rgb_source - GL_TEXTURE0);
            if (source_unit > unit) {
                source_only[source_unit] = true;
            }
        }
        if (alpha_source >= GL_TEXTURE0 && alpha_source <= GL_TEXTURE3) {
            int source_unit = (int)(alpha_source - GL_TEXTURE0);
            if (source_unit > unit) {
                source_only[source_unit] = true;
            }
        }
    }
}

static NxglBackendColor texture_env_color_for_shadow(NxglBackendColor primary, const GLfloat u[4], const GLfloat v[4], const GLfloat r[4])
{
    NxglBackendColor color = primary;
    bool source_only[4] = { false, false, false, false };

    for (int unit = 0; unit < 4; ++unit) {
        if (source_only[unit]) {
            continue;
        }
        color = texture_env_color_for_shadow_unit(unit, primary, color, u, v, r);
        mark_crossbar_source_units(unit, source_only);
    }
    return color;
}

static bool shadow_stipple_allows(int x, int y, bool line)
{
    if (line && line_stipple_enabled) {
        int repeat = line_stipple_factor < 1 ? 1 : line_stipple_factor;
        int bit = ((x + y) / repeat) & 15;
        if (((line_stipple_pattern >> bit) & 1u) == 0u) {
            return false;
        }
    }
    if (!line && polygon_stipple_enabled) {
        int sx = x & 31;
        int sy = y & 31;
        uint8_t mask = polygon_stipple_pattern[sy * 4 + sx / 8];
        if ((mask & (1u << (sx & 7))) == 0u) {
            return false;
        }
    }
    return true;
}

static void shadow_fill_bounds(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d, bool quad, bool line)
{
    int xs[4];
    int ys[4];
    int count = quad ? 4 : 3;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    NxglBackendColor base_color = quad ? d.color : c.color;
    GLfloat sample_u[4] = {
        quad ? (a.u + b.u + c.u + d.u) * 0.25f : (a.u + b.u + c.u) / 3.0f,
        quad ? (a.u1 + b.u1 + c.u1 + d.u1) * 0.25f : (a.u1 + b.u1 + c.u1) / 3.0f,
        quad ? (a.u2 + b.u2 + c.u2 + d.u2) * 0.25f : (a.u2 + b.u2 + c.u2) / 3.0f,
        quad ? (a.u3 + b.u3 + c.u3 + d.u3) * 0.25f : (a.u3 + b.u3 + c.u3) / 3.0f
    };
    GLfloat sample_v[4] = {
        quad ? (a.v + b.v + c.v + d.v) * 0.25f : (a.v + b.v + c.v) / 3.0f,
        quad ? (a.v1 + b.v1 + c.v1 + d.v1) * 0.25f : (a.v1 + b.v1 + c.v1) / 3.0f,
        quad ? (a.v2 + b.v2 + c.v2 + d.v2) * 0.25f : (a.v2 + b.v2 + c.v2) / 3.0f,
        quad ? (a.v3 + b.v3 + c.v3 + d.v3) * 0.25f : (a.v3 + b.v3 + c.v3) / 3.0f
    };
    GLfloat sample_r[4] = {
        quad ? (a.r + b.r + c.r + d.r) * 0.25f : (a.r + b.r + c.r) / 3.0f,
        quad ? (a.r1 + b.r1 + c.r1 + d.r1) * 0.25f : (a.r1 + b.r1 + c.r1) / 3.0f,
        quad ? (a.r2 + b.r2 + c.r2 + d.r2) * 0.25f : (a.r2 + b.r2 + c.r2) / 3.0f,
        quad ? (a.r3 + b.r3 + c.r3 + d.r3) * 0.25f : (a.r3 + b.r3 + c.r3) / 3.0f
    };
    NxglBackendColor shaded;
    GLfloat depth;
    NxglBackendVertex vertices[4] = { a, b, c, d };
    bool cube_shadow = cube_texture_enabled_for_shadow();

    if (!shadow_readback_enabled) {
        return;
    }

    if (quad) {
        base_color = (NxglBackendColor){
            (a.color.r + b.color.r + c.color.r + d.color.r) * 0.25f,
            (a.color.g + b.color.g + c.color.g + d.color.g) * 0.25f,
            (a.color.b + b.color.b + c.color.b + d.color.b) * 0.25f,
            (a.color.a + b.color.a + c.color.a + d.color.a) * 0.25f
        };
        if (lighting_enabled && light_model_local_viewer) {
            NxglBackendVec3 center_eye = {
                (a.eye.x + b.eye.x + c.eye.x + d.eye.x) * 0.25f,
                (a.eye.y + b.eye.y + c.eye.y + d.eye.y) * 0.25f,
                (a.eye.z + b.eye.z + c.eye.z + d.eye.z) * 0.25f
            };
            NxglBackendVec3 center_normal = normalize_vec3((NxglBackendVec3){
                (a.normal.x + b.normal.x + c.normal.x + d.normal.x) * 0.25f,
                (a.normal.y + b.normal.y + c.normal.y + d.normal.y) * 0.25f,
                (a.normal.z + b.normal.z + c.normal.z + d.normal.z) * 0.25f
            });
            NxglBackendColor center_base = {
                (a.base_color.r + b.base_color.r + c.base_color.r + d.base_color.r) * 0.25f,
                (a.base_color.g + b.base_color.g + c.base_color.g + d.base_color.g) * 0.25f,
                (a.base_color.b + b.base_color.b + c.base_color.b + d.base_color.b) * 0.25f,
                (a.base_color.a + b.base_color.a + c.base_color.a + d.base_color.a) * 0.25f
            };
            base_color = lit_color(center_base, center_normal, center_eye);
        }
    } else {
        base_color = (NxglBackendColor){
            (a.color.r + b.color.r + c.color.r) / 3.0f,
            (a.color.g + b.color.g + c.color.g) / 3.0f,
            (a.color.b + b.color.b + c.color.b) / 3.0f,
            (a.color.a + b.color.a + c.color.a) / 3.0f
        };
    }
    shaded = texture_env_color_for_shadow(base_color, sample_u, sample_v, sample_r);

    if (shadow_color_buffer == NULL) {
        return;
    }
    if (alpha_test_enabled && !compare_alpha(shaded.a, alpha_test_func, alpha_test_ref)) {
        return;
    }
    depth = shadow_depth_value(vertices, count);
    for (int i = 0; i < count; ++i) {
        bool projected = cube_shadow ? shadow_project_wide_x(vertices[i].pos, &xs[i], &ys[i]) :
                                       shadow_project(vertices[i].pos, &xs[i], &ys[i]);
        if (!projected) {
            return;
        }
    }

    min_x = max_x = xs[0];
    min_y = max_y = ys[0];
    for (int i = 1; i < count; ++i) {
        if (xs[i] < min_x) min_x = xs[i];
        if (xs[i] > max_x) max_x = xs[i];
        if (ys[i] < min_y) min_y = ys[i];
        if (ys[i] > max_y) max_y = ys[i];
    }
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= shadow_width) max_x = shadow_width - 1;
    if (max_y >= shadow_height) max_y = shadow_height - 1;
    if (scissor_test_enabled) {
        int sx1 = scissor_box[0];
        int sy1 = shadow_height - (scissor_box[1] + scissor_box[3]);
        int sx2 = scissor_box[0] + scissor_box[2] - 1;
        int sy2 = shadow_height - scissor_box[1] - 1;
        if (sx1 < 0) sx1 = 0;
        if (sy1 < 0) sy1 = 0;
        if (sx2 >= shadow_width) sx2 = shadow_width - 1;
        if (sy2 >= shadow_height) sy2 = shadow_height - 1;
        if (min_x < sx1) min_x = sx1;
        if (max_x > sx2) max_x = sx2;
        if (min_y < sy1) min_y = sy1;
        if (max_y > sy2) max_y = sy2;
    }
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    for (int y = min_y; y <= max_y; ++y) {
        uint32_t *row = shadow_color_buffer + (size_t)y * (size_t)shadow_width;
        for (int x = min_x; x <= max_x; ++x) {
            size_t index = (size_t)y * (size_t)shadow_width + (size_t)x;
            bool stencil_pass = compare_stencil(shadow_stencil_buffer != NULL ? shadow_stencil_buffer[index] : 0);
            bool depth_pass = shadow_depth_buffer == NULL || compare_depth(depth, shadow_depth_buffer[index]);
            uint32_t dst = row[x];
            uint32_t blended = apply_blend(shaded, dst);
            uint32_t src = color_logic_op_enabled ? apply_logic_op(color_to_u32(shaded), dst) : blended;

            if (!shadow_stipple_allows(x, y, line)) {
                continue;
            }
            if (!stencil_pass) {
                if (stencil_test_enabled) {
                    write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_fail));
                }
                continue;
            }
            if (!depth_pass) {
                if (stencil_test_enabled) {
                    write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zfail));
                }
                continue;
            }
            if (depth_test_enabled && depth_write_enabled && shadow_depth_buffer != NULL) {
                shadow_depth_buffer[index] = depth;
            }
            if (stencil_test_enabled) {
                write_stencil_value(index, apply_stencil_op(shadow_stencil_buffer[index], stencil_zpass));
            }
            if (draw_buffer_mode != GL_NONE) {
                row[x] = apply_color_write_mask(src, dst);
            }
        }
    }
}

int nxglInit(void)
{
    int status;

    matrix_identity(modelview);
    matrix_identity(projection);
    matrix_identity(texture_matrix);
    matrix_mode = GL_MODELVIEW;
    modelview_stack_top = 0;
    projection_stack_top = 0;
    texture_stack_top = 0;
    attrib_stack_top = 0;
    client_attrib_stack_top = 0;
    current_color = (NxglBackendColor){ 1.0f, 1.0f, 1.0f, 1.0f };
    current_index = 0.0f;
    active_texture = GL_TEXTURE0;
    client_active_texture = GL_TEXTURE0;
    vertex_array = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    color_array = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    normal_array = (ClientArray){ false, 3, GL_FLOAT, 0, NULL };
    for (int i = 0; i < 4; ++i) {
        texcoord_array[i] = (ClientArray){ false, 4, GL_FLOAT, 0, NULL };
    }
    memset(texture_1d_enabled, 0, sizeof(texture_1d_enabled));
    memset(texture_2d_enabled, 0, sizeof(texture_2d_enabled));
    memset(texture_3d_enabled, 0, sizeof(texture_3d_enabled));
    memset(texture_cube_map_enabled, 0, sizeof(texture_cube_map_enabled));
    init_texgen_state();
    memset(texture_binding_1d, 0, sizeof(texture_binding_1d));
    memset(texture_binding_2d, 0, sizeof(texture_binding_2d));
    memset(texture_binding_3d, 0, sizeof(texture_binding_3d));
    memset(texture_binding_cube_map, 0, sizeof(texture_binding_cube_map));
    for (int i = 0; i < 4; ++i) {
        current_u[i] = 0.0f;
        current_v[i] = 0.0f;
        current_r[i] = 0.0f;
        texture_env_mode[i] = GL_MODULATE;
        texture_env_color[i] = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
        texture_combine_rgb[i] = GL_MODULATE;
        texture_combine_alpha[i] = GL_MODULATE;
        texture_source_rgb[i][0] = GL_TEXTURE;
        texture_source_rgb[i][1] = GL_PREVIOUS;
        texture_source_rgb[i][2] = GL_CONSTANT;
        texture_source_alpha[i][0] = GL_TEXTURE;
        texture_source_alpha[i][1] = GL_PREVIOUS;
        texture_source_alpha[i][2] = GL_CONSTANT;
        texture_operand_rgb[i][0] = GL_SRC_COLOR;
        texture_operand_rgb[i][1] = GL_SRC_COLOR;
        texture_operand_rgb[i][2] = GL_SRC_ALPHA;
        texture_operand_alpha[i][0] = GL_SRC_ALPHA;
        texture_operand_alpha[i][1] = GL_SRC_ALPHA;
        texture_operand_alpha[i][2] = GL_SRC_ALPHA;
        texture_rgb_scale[i] = 1.0f;
        texture_alpha_scale[i] = 1.0f;
    }
    for (int i = 0; i < 16; ++i) {
        init_texture_object(&texture_objects[i]);
    }
    for (int i = 0; i < 8; ++i) {
        init_light_object(&lights[i], i);
    }
    recording_list = 0;
    recording_mode = 0;
    list_base = 0;
    replaying_list = false;
    begin_mode = NXGL_NO_BEGIN_MODE;
    pending_count = 0;
    point_size = 1.0f;
    line_width = 1.0f;
    line_stipple_factor = 1;
    line_stipple_pattern = 0xffffu;
    line_stipple_enabled = false;
    polygon_mode = GL_FILL;
    memset(polygon_stipple_pattern, 0xff, sizeof(polygon_stipple_pattern));
    polygon_stipple_enabled = false;
    polygon_offset_point_enabled = false;
    polygon_offset_line_enabled = false;
    polygon_offset_fill_enabled = false;
    polygon_offset_factor = 0.0f;
    polygon_offset_units = 0.0f;
    shade_model = GL_SMOOTH;
    cull_enabled = false;
    cull_face_mode = GL_BACK;
    front_face_mode = GL_CCW;
    current_edge_flag = GL_TRUE;
    perspective_correction_hint = GL_DONT_CARE;
    point_smooth_hint = GL_DONT_CARE;
    line_smooth_hint = GL_DONT_CARE;
    polygon_smooth_hint = GL_DONT_CARE;
    fog_hint = GL_DONT_CARE;
    clear_color_value[0] = 0.0f;
    clear_color_value[1] = 0.0f;
    clear_color_value[2] = 0.0f;
    clear_color_value[3] = 0.0f;
    clear_index_value = 0.0f;
    clear_color = 0x00000000;
    current_normal = (NxglBackendVec3){ 0.0f, 0.0f, 1.0f };
    lighting_enabled = false;
    light_model_local_viewer = false;
    light_model_two_side = false;
    light_model_color_control = GL_SINGLE_COLOR;
    light_model_ambient = (NxglBackendColor){ 0.2f, 0.2f, 0.2f, 1.0f };
    color_material_enabled = false;
    fog_enabled = false;
    normalize_enabled = false;
    rescale_normal_enabled = false;
    for (int i = 0; i < NXGL_MAX_CLIP_PLANES; ++i) {
        clip_planes[i].enabled = GL_FALSE;
        clip_planes[i].equation[0] = 0.0;
        clip_planes[i].equation[1] = 0.0;
        clip_planes[i].equation[2] = 0.0;
        clip_planes[i].equation[3] = 0.0;
    }
    scissor_test_enabled = false;
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = 640;
    viewport[3] = 480;
    scissor_box[0] = 0;
    scissor_box[1] = 0;
    scissor_box[2] = 640;
    scissor_box[3] = 480;
    depth_clear_value = 1.0f;
    depth_range_near = 0.0f;
    depth_range_far = 1.0f;
    depth_func = GL_LEQUAL;
    depth_test_enabled = true;
    depth_write_enabled = true;
    stencil_test_enabled = false;
    stencil_clear_value = 0;
    stencil_func = GL_ALWAYS;
    stencil_ref = 0;
    stencil_value_mask = 0xffffffffu;
    stencil_write_mask = 0xffffffffu;
    stencil_fail = GL_KEEP;
    stencil_zfail = GL_KEEP;
    stencil_zpass = GL_KEEP;
    blend_enabled = false;
    draw_buffer_mode = GL_BACK;
    read_buffer_mode = GL_BACK;
    alpha_test_enabled = false;
    alpha_test_func = GL_ALWAYS;
    alpha_test_ref = 0.0f;
    color_write_mask[0] = GL_TRUE;
    color_write_mask[1] = GL_TRUE;
    color_write_mask[2] = GL_TRUE;
    color_write_mask[3] = GL_TRUE;
    color_logic_op_enabled = false;
    logic_op_mode = GL_COPY;
    multisample_enabled = true;
    sample_alpha_to_coverage_enabled = false;
    sample_alpha_to_one_enabled = false;
    sample_coverage_enabled = false;
    sample_coverage_value = 1.0f;
    sample_coverage_invert = GL_FALSE;
    fog_mode = GL_EXP;
    fog_color = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
    fog_density = 1.0f;
    fog_start = 0.0f;
    fog_end = 1.0f;
    accum_clear_value[0] = 0.0f;
    accum_clear_value[1] = 0.0f;
    accum_clear_value[2] = 0.0f;
    accum_clear_value[3] = 0.0f;
    pack_alignment = 4;
    pack_row_length = 0;
    pack_skip_rows = 0;
    pack_skip_pixels = 0;
    pack_image_height = 0;
    pack_skip_images = 0;
    pack_swap_bytes = GL_FALSE;
    pack_lsb_first = GL_FALSE;
    unpack_alignment = 4;
    unpack_row_length = 0;
    unpack_skip_rows = 0;
    unpack_skip_pixels = 0;
    unpack_image_height = 0;
    unpack_skip_images = 0;
    unpack_swap_bytes = GL_FALSE;
    unpack_lsb_first = GL_FALSE;
    pixel_zoom_x = 1.0f;
    pixel_zoom_y = 1.0f;
    for (int i = 0; i < 4; ++i) {
        pixel_transfer_scale[i] = 1.0f;
        pixel_transfer_bias[i] = 0.0f;
    }
    for (int i = 0; i < 10; ++i) {
        pixel_maps[i].size = 2;
        pixel_maps[i].values[0] = 0.0f;
        pixel_maps[i].values[1] = 1.0f;
    }
    pixel_maps[pixel_map_index(GL_PIXEL_MAP_I_TO_I)].size = 256;
    pixel_maps[pixel_map_index(GL_PIXEL_MAP_S_TO_S)].size = 256;
    for (int i = 0; i < 256; ++i) {
        GLfloat value = (GLfloat)i / 255.0f;
        pixel_maps[pixel_map_index(GL_PIXEL_MAP_I_TO_I)].values[i] = value;
        pixel_maps[pixel_map_index(GL_PIXEL_MAP_S_TO_S)].values[i] = value;
    }
    init_eval_maps();
    auto_normal_enabled = GL_FALSE;
    map_grid1_n = 1;
    map_grid1_u1 = 0.0f;
    map_grid1_u2 = 1.0f;
    map_grid2_un = 1;
    map_grid2_vn = 1;
    map_grid2_u1 = 0.0f;
    map_grid2_u2 = 1.0f;
    map_grid2_v1 = 0.0f;
    map_grid2_v2 = 1.0f;
    current_raster_position[0] = 0.0f;
    current_raster_position[1] = 0.0f;
    current_raster_position[2] = 0.0f;
    current_raster_position[3] = 1.0f;
    current_raster_position_valid = true;
    render_mode = GL_RENDER;
    shadow_readback_enabled = true;
    selection_buffer = NULL;
    selection_buffer_size = 0;
    selection_write_count = 0;
    selection_hits = 0;
    selection_overflow = false;
    name_stack_depth = 0;
    feedback_buffer = NULL;
    feedback_buffer_size = 0;
    feedback_write_count = 0;
    feedback_overflow = false;
    feedback_type = GL_2D;
    color_material_face = GL_FRONT_AND_BACK;
    color_material_parameter = GL_AMBIENT_AND_DIFFUSE;
    material_state = (MaterialState){
        { 0.2f, 0.2f, 0.2f, 1.0f },
        { 0.8f, 0.8f, 0.8f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        0.0f
    };
    material_back_state = material_state;
    camera_x = 0.0f;
    camera_y = 0.0f;
    camera_z = -6.0f;
    last_error = GL_NO_ERROR;
    native_frame_started = false;
    status = nxgl_backend_init();
    if (status != 0) {
        return status;
    }
    shadow_width = nxgl_backend_back_buffer_width();
    shadow_height = nxgl_backend_back_buffer_height();
    viewport[2] = shadow_width;
    viewport[3] = shadow_height;
    scissor_box[2] = shadow_width;
    scissor_box[3] = shadow_height;
    if (shadow_width > SHADOW_MAX_WIDTH || shadow_height > SHADOW_MAX_HEIGHT) {
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_color_buffer = (uint32_t *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(uint32_t),
                                                                    0,
                                                                    SHADOW_MAXRAM,
                                                                    0,
                                                                    PAGE_READWRITE);
    if (shadow_color_buffer == NULL) {
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_depth_buffer = (float *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(float),
                                                                0,
                                                                SHADOW_MAXRAM,
                                                                0,
                                                                PAGE_READWRITE);
    if (shadow_depth_buffer == NULL) {
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_stencil_buffer = (uint8_t *)MmAllocateContiguousMemoryEx((size_t)shadow_width * (size_t)shadow_height * sizeof(uint8_t),
                                                                    0,
                                                                    SHADOW_MAXRAM,
                                                                    0,
                                                                    PAGE_READWRITE);
    if (shadow_stencil_buffer == NULL) {
        MmFreeContiguousMemory(shadow_depth_buffer);
        shadow_depth_buffer = NULL;
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
        nxgl_backend_shutdown();
        return 1;
    }
    shadow_clear(clear_color);
    shadow_clear_depth(depth_clear_value);
    shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    return 0;
}

void nxglShutdown(void)
{
    if (shadow_stencil_buffer != NULL) {
        MmFreeContiguousMemory(shadow_stencil_buffer);
        shadow_stencil_buffer = NULL;
    }
    if (shadow_depth_buffer != NULL) {
        MmFreeContiguousMemory(shadow_depth_buffer);
        shadow_depth_buffer = NULL;
    }
    if (shadow_color_buffer != NULL) {
        MmFreeContiguousMemory(shadow_color_buffer);
        shadow_color_buffer = NULL;
    }
    free(accum_buffer);
    accum_buffer = NULL;
    for (int i = 1; i < 16; ++i) {
        if (texture_objects[i].allocated) {
            destroy_texture_image(&texture_objects[i]);
            texture_objects[i].allocated = false;
        }
    }
    for (int i = 1; i < 256; ++i) {
        clear_display_list(&display_lists[i]);
    }
    nxgl_backend_shutdown();
}

void nxglSetCamera(float x, float y, float z, float rx, float ry, float rz)
{
    camera_x = x;
    camera_y = y;
    camera_z = z;
    nxgl_backend_set_camera(x, y, z, rx, ry, rz);
}

void nxglSetReadbackEnabled(GLboolean enabled)
{
    bool next = enabled != GL_FALSE;
    if (shadow_readback_enabled == next) {
        return;
    }
    shadow_readback_enabled = next;
    if (shadow_readback_enabled) {
        shadow_clear(clear_color);
        shadow_clear_depth(depth_clear_value);
        shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    }
}

void nxglSwapBuffers(const char *title, const char *detail)
{
    ensure_native_frame_started();
    nxgl_backend_finish(title, detail);
    native_frame_started = false;
}

const GLubyte *glGetString(GLenum name)
{
    switch (name) {
    case GL_VENDOR:
        return (const GLubyte *)"nxdk";
    case GL_RENDERER:
        return (const GLubyte *)"NXGL NV2A fixed-function shim";
    case GL_VERSION:
        return (const GLubyte *)"1.1 NXGL draft";
    case GL_EXTENSIONS:
        return (const GLubyte *)"GL_ARB_multitexture GL_EXT_bgra GL_EXT_texture_edge_clamp";
    default:
        set_error(GL_INVALID_ENUM);
        return NULL;
    }
}

GLenum glGetError(void)
{
    GLenum error = last_error;
    last_error = GL_NO_ERROR;
    return error;
}

GLboolean glIsEnabled(GLenum cap)
{
    int unit = active_texture_index();
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    if (eval_index >= 0) {
        return map2 ? eval_maps2[eval_index].enabled : eval_maps1[eval_index].enabled;
    }
    if (texgen_coord >= 0) {
        return unit >= 0 ? texgen_state[unit][texgen_coord].enabled : GL_FALSE;
    }
    if (clip >= 0) {
        return clip_planes[clip].enabled;
    }
    if (light >= 0) {
        return lights[light].enabled ? GL_TRUE : GL_FALSE;
    }
    switch (cap) {
    case GL_BLEND:
        return blend_enabled ? GL_TRUE : GL_FALSE;
    case GL_LINE_STIPPLE:
        return line_stipple_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_STIPPLE:
        return polygon_stipple_enabled ? GL_TRUE : GL_FALSE;
    case GL_DEPTH_TEST:
        return depth_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_CULL_FACE:
        return cull_enabled ? GL_TRUE : GL_FALSE;
    case GL_SCISSOR_TEST:
        return scissor_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_STENCIL_TEST:
        return stencil_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_ALPHA_TEST:
        return alpha_test_enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_LOGIC_OP:
    case GL_INDEX_LOGIC_OP:
        return color_logic_op_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_POINT:
        return polygon_offset_point_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_LINE:
        return polygon_offset_line_enabled ? GL_TRUE : GL_FALSE;
    case GL_POLYGON_OFFSET_FILL:
        return polygon_offset_fill_enabled ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_1D:
        return unit >= 0 && texture_1d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_2D:
        return unit >= 0 && texture_2d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_3D:
        return unit >= 0 && texture_3d_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_CUBE_MAP:
        return unit >= 0 && texture_cube_map_enabled[unit] ? GL_TRUE : GL_FALSE;
    case GL_LIGHTING:
        return lighting_enabled ? GL_TRUE : GL_FALSE;
    case GL_FOG:
        return fog_enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_MATERIAL:
        return color_material_enabled ? GL_TRUE : GL_FALSE;
    case GL_NORMALIZE:
        return normalize_enabled ? GL_TRUE : GL_FALSE;
    case GL_RESCALE_NORMAL:
        return rescale_normal_enabled ? GL_TRUE : GL_FALSE;
    case GL_CURRENT_RASTER_POSITION_VALID:
        return current_raster_position_valid ? GL_TRUE : GL_FALSE;
    case GL_EDGE_FLAG:
        return current_edge_flag;
    case GL_VERTEX_ARRAY:
        return vertex_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_COLOR_ARRAY:
        return color_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_NORMAL_ARRAY:
        return normal_array.enabled ? GL_TRUE : GL_FALSE;
    case GL_TEXTURE_COORD_ARRAY:
        return unit >= 0 && texcoord_array[unit].enabled ? GL_TRUE : GL_FALSE;
    case GL_AUTO_NORMAL:
        return auto_normal_enabled;
    case GL_MULTISAMPLE:
        return multisample_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        return sample_alpha_to_coverage_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_ALPHA_TO_ONE:
        return sample_alpha_to_one_enabled ? GL_TRUE : GL_FALSE;
    case GL_SAMPLE_COVERAGE:
        return sample_coverage_enabled ? GL_TRUE : GL_FALSE;
    default:
        set_error(GL_INVALID_ENUM);
        return GL_FALSE;
    }
}

void glGetIntegerv(GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    int client_unit = client_texture_index();
    int map_index = pixel_map_index_from_size_pname(pname);

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if (map_index >= 0) {
        params[0] = pixel_maps[map_index].size;
        return;
    }

    switch (pname) {
    case GL_MATRIX_MODE:
        params[0] = (GLint)matrix_mode;
        break;
    case GL_MODELVIEW_STACK_DEPTH:
        params[0] = modelview_stack_top + 1;
        break;
    case GL_PROJECTION_STACK_DEPTH:
        params[0] = projection_stack_top + 1;
        break;
    case GL_TEXTURE_STACK_DEPTH:
        params[0] = texture_stack_top + 1;
        break;
    case GL_VIEWPORT:
        memcpy(params, viewport, sizeof(viewport));
        break;
    case GL_MAX_VIEWPORT_DIMS:
        params[0] = shadow_width;
        params[1] = shadow_height;
        break;
    case GL_SCISSOR_BOX:
        memcpy(params, scissor_box, sizeof(scissor_box));
        break;
    case GL_POLYGON_MODE:
        params[0] = (GLint)polygon_mode;
        params[1] = (GLint)polygon_mode;
        break;
    case GL_LINE_STIPPLE_PATTERN:
        params[0] = (GLint)line_stipple_pattern;
        break;
    case GL_LINE_STIPPLE_REPEAT:
        params[0] = line_stipple_factor;
        break;
    case GL_POINT_SIZE_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = 1;
        params[1] = 64;
        break;
    case GL_POINT_SIZE_GRANULARITY:
    case GL_LINE_WIDTH_GRANULARITY:
        params[0] = 1;
        break;
    case GL_CULL_FACE_MODE:
        params[0] = (GLint)cull_face_mode;
        break;
    case GL_FRONT_FACE:
        params[0] = (GLint)front_face_mode;
        break;
    case GL_PERSPECTIVE_CORRECTION_HINT:
        params[0] = (GLint)perspective_correction_hint;
        break;
    case GL_POINT_SMOOTH_HINT:
        params[0] = (GLint)point_smooth_hint;
        break;
    case GL_LINE_SMOOTH_HINT:
        params[0] = (GLint)line_smooth_hint;
        break;
    case GL_POLYGON_SMOOTH_HINT:
        params[0] = (GLint)polygon_smooth_hint;
        break;
    case GL_FOG_HINT:
        params[0] = (GLint)fog_hint;
        break;
    case GL_STENCIL_CLEAR_VALUE:
        params[0] = stencil_clear_value;
        break;
    case GL_STENCIL_FUNC:
        params[0] = (GLint)stencil_func;
        break;
    case GL_STENCIL_REF:
        params[0] = stencil_ref;
        break;
    case GL_STENCIL_VALUE_MASK:
        params[0] = (GLint)stencil_value_mask;
        break;
    case GL_STENCIL_WRITEMASK:
        params[0] = (GLint)stencil_write_mask;
        break;
    case GL_STENCIL_FAIL:
        params[0] = (GLint)stencil_fail;
        break;
    case GL_STENCIL_PASS_DEPTH_FAIL:
        params[0] = (GLint)stencil_zfail;
        break;
    case GL_STENCIL_PASS_DEPTH_PASS:
        params[0] = (GLint)stencil_zpass;
        break;
    case GL_STENCIL_BITS:
        params[0] = 8;
        break;
    case GL_DEPTH_BITS:
        params[0] = 24;
        break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
        params[0] = 8;
        break;
    case GL_SUBPIXEL_BITS:
        params[0] = 4;
        break;
    case GL_AUX_BUFFERS:
        params[0] = 0;
        break;
    case GL_DRAW_BUFFER:
        params[0] = (GLint)draw_buffer_mode;
        break;
    case GL_READ_BUFFER:
        params[0] = (GLint)read_buffer_mode;
        break;
    case GL_DOUBLEBUFFER:
    case GL_RGBA_MODE:
        params[0] = GL_TRUE;
        break;
    case GL_STEREO:
        params[0] = GL_FALSE;
        break;
    case GL_INDEX_MODE:
        params[0] = GL_FALSE;
        break;
    case GL_ALPHA_TEST_FUNC:
        params[0] = (GLint)alpha_test_func;
        break;
    case GL_COLOR_WRITEMASK:
        params[0] = color_write_mask[0];
        params[1] = color_write_mask[1];
        params[2] = color_write_mask[2];
        params[3] = color_write_mask[3];
        break;
    case GL_LOGIC_OP_MODE:
        params[0] = (GLint)logic_op_mode;
        break;
    case GL_SHADE_MODEL:
        params[0] = (GLint)shade_model;
        break;
    case GL_FOG_MODE:
        params[0] = (GLint)fog_mode;
        break;
    case GL_COLOR_MATERIAL_FACE:
        params[0] = (GLint)color_material_face;
        break;
    case GL_COLOR_MATERIAL_PARAMETER:
        params[0] = (GLint)color_material_parameter;
        break;
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
        params[0] = light_model_local_viewer ? GL_TRUE : GL_FALSE;
        break;
    case GL_LIGHT_MODEL_TWO_SIDE:
        params[0] = light_model_two_side ? GL_TRUE : GL_FALSE;
        break;
    case GL_LIGHT_MODEL_COLOR_CONTROL:
        params[0] = (GLint)light_model_color_control;
        break;
    case GL_DEPTH_FUNC:
        params[0] = (GLint)depth_func;
        break;
    case GL_DEPTH_RANGE:
        params[0] = (GLint)depth_range_near;
        params[1] = (GLint)depth_range_far;
        break;
    case GL_BLEND_SRC:
        params[0] = (GLint)blend_sfactor;
        break;
    case GL_BLEND_DST:
        params[0] = (GLint)blend_dfactor;
        break;
    case GL_DEPTH_WRITEMASK:
        params[0] = depth_write_enabled ? GL_TRUE : GL_FALSE;
        break;
    case GL_CURRENT_INDEX:
        params[0] = (GLint)current_index;
        break;
    case GL_INDEX_CLEAR_VALUE:
        params[0] = (GLint)clear_index_value;
        break;
    case GL_CURRENT_RASTER_INDEX:
    case GL_CURRENT_RASTER_DISTANCE:
        params[0] = 0;
        break;
    case GL_MAX_LIGHTS:
        params[0] = 8;
        break;
    case GL_MAX_CLIP_PLANES:
        params[0] = NXGL_MAX_CLIP_PLANES;
        break;
    case GL_MAX_EVAL_ORDER:
        params[0] = NXGL_MAX_EVAL_ORDER;
        break;
    case GL_MAX_LIST_NESTING:
        params[0] = 64;
        break;
    case GL_MAX_PIXEL_MAP_TABLE:
        params[0] = NXGL_PIXEL_MAP_MAX;
        break;
    case GL_MAX_ATTRIB_STACK_DEPTH:
        params[0] = NXGL_ATTRIB_STACK_MAX;
        break;
    case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
        params[0] = NXGL_CLIENT_ATTRIB_STACK_MAX;
        break;
    case GL_MAX_NAME_STACK_DEPTH:
        params[0] = (GLint)(sizeof(name_stack) / sizeof(name_stack[0]));
        break;
    case GL_ATTRIB_STACK_DEPTH:
        params[0] = attrib_stack_top;
        break;
    case GL_CLIENT_ATTRIB_STACK_DEPTH:
        params[0] = client_attrib_stack_top;
        break;
    case GL_MAX_TEXTURE_SIZE:
        params[0] = 4096;
        break;
    case GL_MAX_MODELVIEW_STACK_DEPTH:
        params[0] = 32;
        break;
    case GL_MAX_PROJECTION_STACK_DEPTH:
        params[0] = 8;
        break;
    case GL_MAX_TEXTURE_STACK_DEPTH:
        params[0] = 8;
        break;
    case GL_MAX_TEXTURE_UNITS:
        params[0] = 4;
        break;
    case GL_MAX_3D_TEXTURE_SIZE:
        params[0] = 256;
        break;
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_ELEMENTS_INDICES:
        params[0] = 1024;
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        params[0] = 4;
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        params[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        params[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        params[2] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        params[3] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    case GL_ACTIVE_TEXTURE:
        params[0] = (GLint)active_texture;
        break;
    case GL_CLIENT_ACTIVE_TEXTURE:
        params[0] = (GLint)client_active_texture;
        break;
    case GL_LIST_BASE:
        params[0] = (GLint)list_base;
        break;
    case GL_RENDER_MODE:
        params[0] = (GLint)render_mode;
        break;
    case GL_NAME_STACK_DEPTH:
        params[0] = name_stack_depth;
        break;
    case GL_SELECTION_BUFFER_SIZE:
        params[0] = selection_buffer_size;
        break;
    case GL_FEEDBACK_BUFFER_SIZE:
        params[0] = feedback_buffer_size;
        break;
    case GL_TEXTURE_BINDING_2D:
        params[0] = unit >= 0 ? (GLint)texture_binding_2d[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_3D:
        params[0] = unit >= 0 ? (GLint)texture_binding_3d[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_CUBE_MAP:
        params[0] = unit >= 0 ? (GLint)texture_binding_cube_map[unit] : 0;
        break;
    case GL_TEXTURE_BINDING_1D:
        params[0] = unit >= 0 ? (GLint)texture_binding_1d[unit] : 0;
        break;
    case GL_PACK_ALIGNMENT:
        params[0] = pack_alignment;
        break;
    case GL_PACK_ROW_LENGTH:
        params[0] = pack_row_length;
        break;
    case GL_PACK_SKIP_ROWS:
        params[0] = pack_skip_rows;
        break;
    case GL_PACK_SKIP_PIXELS:
        params[0] = pack_skip_pixels;
        break;
    case GL_PACK_IMAGE_HEIGHT:
        params[0] = pack_image_height;
        break;
    case GL_PACK_SKIP_IMAGES:
        params[0] = pack_skip_images;
        break;
    case GL_PACK_SWAP_BYTES:
        params[0] = pack_swap_bytes;
        break;
    case GL_PACK_LSB_FIRST:
        params[0] = pack_lsb_first;
        break;
    case GL_UNPACK_ALIGNMENT:
        params[0] = unpack_alignment;
        break;
    case GL_UNPACK_ROW_LENGTH:
        params[0] = unpack_row_length;
        break;
    case GL_UNPACK_SKIP_ROWS:
        params[0] = unpack_skip_rows;
        break;
    case GL_UNPACK_SKIP_PIXELS:
        params[0] = unpack_skip_pixels;
        break;
    case GL_UNPACK_IMAGE_HEIGHT:
        params[0] = unpack_image_height;
        break;
    case GL_UNPACK_SKIP_IMAGES:
        params[0] = unpack_skip_images;
        break;
    case GL_UNPACK_SWAP_BYTES:
        params[0] = unpack_swap_bytes;
        break;
    case GL_UNPACK_LSB_FIRST:
        params[0] = unpack_lsb_first;
        break;
    case GL_INDEX_BITS:
    case GL_ACCUM_RED_BITS:
    case GL_ACCUM_GREEN_BITS:
    case GL_ACCUM_BLUE_BITS:
    case GL_ACCUM_ALPHA_BITS:
        params[0] = 0;
        break;
    case GL_VERTEX_ARRAY_SIZE:
        params[0] = vertex_array.size;
        break;
    case GL_VERTEX_ARRAY_TYPE:
        params[0] = (GLint)vertex_array.type;
        break;
    case GL_VERTEX_ARRAY_STRIDE:
        params[0] = vertex_array.stride;
        break;
    case GL_COLOR_ARRAY_SIZE:
        params[0] = color_array.size;
        break;
    case GL_COLOR_ARRAY_TYPE:
        params[0] = (GLint)color_array.type;
        break;
    case GL_COLOR_ARRAY_STRIDE:
        params[0] = color_array.stride;
        break;
    case GL_TEXTURE_COORD_ARRAY_SIZE:
        params[0] = client_unit >= 0 ? texcoord_array[client_unit].size : 0;
        break;
    case GL_TEXTURE_COORD_ARRAY_TYPE:
        params[0] = client_unit >= 0 ? (GLint)texcoord_array[client_unit].type : 0;
        break;
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
        params[0] = client_unit >= 0 ? texcoord_array[client_unit].stride : 0;
        break;
    case GL_NORMAL_ARRAY_TYPE:
        params[0] = (GLint)normal_array.type;
        break;
    case GL_NORMAL_ARRAY_STRIDE:
        params[0] = normal_array.stride;
        break;
    case GL_BLEND:
    case GL_LINE_STIPPLE:
    case GL_POLYGON_STIPPLE:
    case GL_DEPTH_TEST:
    case GL_CULL_FACE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
    case GL_ALPHA_TEST:
    case GL_COLOR_LOGIC_OP:
    case GL_INDEX_LOGIC_OP:
    case GL_POLYGON_OFFSET_POINT:
    case GL_POLYGON_OFFSET_LINE:
    case GL_POLYGON_OFFSET_FILL:
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_LIGHTING:
    case GL_FOG:
    case GL_COLOR_MATERIAL:
    case GL_NORMALIZE:
    case GL_RESCALE_NORMAL:
    case GL_CURRENT_RASTER_POSITION_VALID:
    case GL_EDGE_FLAG:
    case GL_VERTEX_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_AUTO_NORMAL:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_VERTEX_4:
    case GL_MAP1_INDEX:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_VERTEX_4:
    case GL_MAP2_INDEX:
    case GL_MULTISAMPLE:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_ALPHA_TO_ONE:
    case GL_SAMPLE_COVERAGE:
        params[0] = glIsEnabled(pname);
        break;
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
        params[0] = 0;
        break;
    case GL_SAMPLE_COVERAGE_VALUE:
        params[0] = sample_coverage_value >= 0.5f ? 1 : 0;
        break;
    case GL_SAMPLE_COVERAGE_INVERT:
        params[0] = sample_coverage_invert ? GL_TRUE : GL_FALSE;
        break;
    case GL_CURRENT_COLOR:
        params[0] = (GLint)current_color.r;
        params[1] = (GLint)current_color.g;
        params[2] = (GLint)current_color.b;
        params[3] = (GLint)current_color.a;
        break;
    case GL_CURRENT_NORMAL:
        params[0] = (GLint)current_normal.x;
        params[1] = (GLint)current_normal.y;
        params[2] = (GLint)current_normal.z;
        break;
    case GL_CURRENT_TEXTURE_COORDS:
        params[0] = unit >= 0 ? (GLint)current_u[unit] : 0;
        params[1] = unit >= 0 ? (GLint)current_v[unit] : 0;
        params[2] = unit >= 0 ? (GLint)current_r[unit] : 0;
        params[3] = 1;
        break;
    case GL_CLEAR_COLOR:
        params[0] = (GLint)clear_color_value[0];
        params[1] = (GLint)clear_color_value[1];
        params[2] = (GLint)clear_color_value[2];
        params[3] = (GLint)clear_color_value[3];
        break;
    case GL_CURRENT_RASTER_POSITION:
        params[0] = (GLint)current_raster_position[0];
        params[1] = (GLint)current_raster_position[1];
        params[2] = (GLint)current_raster_position[2];
        params[3] = (GLint)current_raster_position[3];
        break;
    case GL_CURRENT_RASTER_COLOR:
        params[0] = (GLint)current_color.r;
        params[1] = (GLint)current_color.g;
        params[2] = (GLint)current_color.b;
        params[3] = (GLint)current_color.a;
        break;
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
        params[0] = (GLint)current_u[0];
        params[1] = (GLint)current_v[0];
        params[2] = (GLint)current_r[0];
        params[3] = 1;
        break;
    case GL_ACCUM_CLEAR_VALUE:
        params[0] = (GLint)accum_clear_value[0];
        params[1] = (GLint)accum_clear_value[1];
        params[2] = (GLint)accum_clear_value[2];
        params[3] = (GLint)accum_clear_value[3];
        break;
    case GL_DEPTH_CLEAR_VALUE:
        params[0] = (GLint)depth_clear_value;
        break;
    case GL_LIGHT_MODEL_AMBIENT:
        params[0] = (GLint)light_model_ambient.r;
        params[1] = (GLint)light_model_ambient.g;
        params[2] = (GLint)light_model_ambient.b;
        params[3] = (GLint)light_model_ambient.a;
        break;
    case GL_FOG_COLOR:
        params[0] = (GLint)fog_color.r;
        params[1] = (GLint)fog_color.g;
        params[2] = (GLint)fog_color.b;
        params[3] = (GLint)fog_color.a;
        break;
    case GL_FOG_DENSITY:
        params[0] = (GLint)fog_density;
        break;
    case GL_FOG_START:
        params[0] = (GLint)fog_start;
        break;
    case GL_FOG_END:
        params[0] = (GLint)fog_end;
        break;
    case GL_ALPHA_TEST_REF:
        params[0] = (GLint)alpha_test_ref;
        break;
    case GL_ZOOM_X:
        params[0] = (GLint)pixel_zoom_x;
        break;
    case GL_ZOOM_Y:
        params[0] = (GLint)pixel_zoom_y;
        break;
    case GL_RED_SCALE:
        params[0] = (GLint)pixel_transfer_scale[0];
        break;
    case GL_GREEN_SCALE:
        params[0] = (GLint)pixel_transfer_scale[1];
        break;
    case GL_BLUE_SCALE:
        params[0] = (GLint)pixel_transfer_scale[2];
        break;
    case GL_ALPHA_SCALE:
        params[0] = (GLint)pixel_transfer_scale[3];
        break;
    case GL_RED_BIAS:
        params[0] = (GLint)pixel_transfer_bias[0];
        break;
    case GL_GREEN_BIAS:
        params[0] = (GLint)pixel_transfer_bias[1];
        break;
    case GL_BLUE_BIAS:
        params[0] = (GLint)pixel_transfer_bias[2];
        break;
    case GL_ALPHA_BIAS:
        params[0] = (GLint)pixel_transfer_bias[3];
        break;
    case GL_POINT_SIZE:
        params[0] = (GLint)point_size;
        break;
    case GL_LINE_WIDTH:
        params[0] = (GLint)line_width;
        break;
    case GL_POLYGON_OFFSET_FACTOR:
        params[0] = (GLint)polygon_offset_factor;
        break;
    case GL_POLYGON_OFFSET_UNITS:
        params[0] = (GLint)polygon_offset_units;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
        break;
    }
}

void glGetFloatv(GLenum pname, GLfloat *params)
{
    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
    case GL_CURRENT_COLOR:
        params[0] = current_color.r;
        params[1] = current_color.g;
        params[2] = current_color.b;
        params[3] = current_color.a;
        break;
    case GL_CURRENT_TEXTURE_COORDS: {
        int unit = active_texture_index();
        if (unit < 0) {
            set_error(GL_INVALID_ENUM);
            params[0] = 0.0f;
            break;
        }
        params[0] = current_u[unit];
        params[1] = current_v[unit];
        params[2] = current_r[unit];
        params[3] = 1.0f;
        break;
    }
    case GL_CLEAR_COLOR:
        memcpy(params, clear_color_value, sizeof(clear_color_value));
        break;
    case GL_CURRENT_NORMAL:
        params[0] = current_normal.x;
        params[1] = current_normal.y;
        params[2] = current_normal.z;
        break;
    case GL_CURRENT_RASTER_POSITION:
        memcpy(params, current_raster_position, sizeof(current_raster_position));
        break;
    case GL_CURRENT_RASTER_COLOR:
        params[0] = current_color.r;
        params[1] = current_color.g;
        params[2] = current_color.b;
        params[3] = current_color.a;
        break;
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
        params[0] = current_u[0];
        params[1] = current_v[0];
        params[2] = current_r[0];
        params[3] = 1.0f;
        break;
    case GL_CURRENT_INDEX:
        params[0] = current_index;
        break;
    case GL_INDEX_CLEAR_VALUE:
        params[0] = clear_index_value;
        break;
    case GL_CURRENT_RASTER_INDEX:
    case GL_CURRENT_RASTER_DISTANCE:
        params[0] = 0.0f;
        break;
    case GL_ACCUM_CLEAR_VALUE:
        memcpy(params, accum_clear_value, sizeof(accum_clear_value));
        break;
    case GL_DEPTH_CLEAR_VALUE:
        params[0] = depth_clear_value;
        break;
    case GL_DEPTH_RANGE:
        params[0] = depth_range_near;
        params[1] = depth_range_far;
        break;
    case GL_LIGHT_MODEL_AMBIENT:
        params[0] = light_model_ambient.r;
        params[1] = light_model_ambient.g;
        params[2] = light_model_ambient.b;
        params[3] = light_model_ambient.a;
        break;
    case GL_FOG_COLOR:
        params[0] = fog_color.r;
        params[1] = fog_color.g;
        params[2] = fog_color.b;
        params[3] = fog_color.a;
        break;
    case GL_FOG_DENSITY:
        params[0] = fog_density;
        break;
    case GL_FOG_START:
        params[0] = fog_start;
        break;
    case GL_FOG_END:
        params[0] = fog_end;
        break;
    case GL_ALPHA_TEST_REF:
        params[0] = alpha_test_ref;
        break;
    case GL_SAMPLE_COVERAGE_VALUE:
        params[0] = sample_coverage_value;
        break;
    case GL_SAMPLE_COVERAGE_INVERT:
        params[0] = sample_coverage_invert ? 1.0f : 0.0f;
        break;
    case GL_ZOOM_X:
        params[0] = pixel_zoom_x;
        break;
    case GL_ZOOM_Y:
        params[0] = pixel_zoom_y;
        break;
    case GL_RED_SCALE:
        params[0] = pixel_transfer_scale[0];
        break;
    case GL_GREEN_SCALE:
        params[0] = pixel_transfer_scale[1];
        break;
    case GL_BLUE_SCALE:
        params[0] = pixel_transfer_scale[2];
        break;
    case GL_ALPHA_SCALE:
        params[0] = pixel_transfer_scale[3];
        break;
    case GL_RED_BIAS:
        params[0] = pixel_transfer_bias[0];
        break;
    case GL_GREEN_BIAS:
        params[0] = pixel_transfer_bias[1];
        break;
    case GL_BLUE_BIAS:
        params[0] = pixel_transfer_bias[2];
        break;
    case GL_ALPHA_BIAS:
        params[0] = pixel_transfer_bias[3];
        break;
    case GL_POINT_SIZE:
        params[0] = point_size;
        break;
    case GL_POINT_SIZE_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
        params[0] = 1.0f;
        params[1] = 64.0f;
        break;
    case GL_POINT_SIZE_GRANULARITY:
        params[0] = 1.0f;
        break;
    case GL_LINE_WIDTH:
        params[0] = line_width;
        break;
    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = 1.0f;
        params[1] = 64.0f;
        break;
    case GL_LINE_WIDTH_GRANULARITY:
        params[0] = 1.0f;
        break;
    case GL_SCISSOR_BOX:
        params[0] = (GLfloat)scissor_box[0];
        params[1] = (GLfloat)scissor_box[1];
        params[2] = (GLfloat)scissor_box[2];
        params[3] = (GLfloat)scissor_box[3];
        break;
    case GL_VIEWPORT:
        params[0] = (GLfloat)viewport[0];
        params[1] = (GLfloat)viewport[1];
        params[2] = (GLfloat)viewport[2];
        params[3] = (GLfloat)viewport[3];
        break;
    case GL_POLYGON_OFFSET_FACTOR:
        params[0] = polygon_offset_factor;
        break;
    case GL_POLYGON_OFFSET_UNITS:
        params[0] = polygon_offset_units;
        break;
    case GL_MODELVIEW_MATRIX:
        memcpy(params, modelview, sizeof(Matrix));
        break;
    case GL_PROJECTION_MATRIX:
        memcpy(params, projection, sizeof(Matrix));
        break;
    case GL_TEXTURE_MATRIX:
        memcpy(params, texture_matrix, sizeof(Matrix));
        break;
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
        matrix_transpose(params, modelview);
        break;
    case GL_TRANSPOSE_PROJECTION_MATRIX:
        matrix_transpose(params, projection);
        break;
    case GL_TRANSPOSE_TEXTURE_MATRIX:
        matrix_transpose(params, texture_matrix);
        break;
    case GL_TRANSPOSE_COLOR_MATRIX:
        matrix_identity(params);
        break;
    default: {
        GLint ints[16];
        GLenum before = last_error;
        int count;

        last_error = GL_NO_ERROR;
        glGetIntegerv(pname, ints);
        if (last_error == GL_NO_ERROR) {
            count = get_component_count(pname);
            for (int i = 0; i < count; ++i) {
                params[i] = (GLfloat)ints[i];
            }
            last_error = before;
            return;
        }

        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        break;
    }
    }
}

void glGetDoublev(GLenum pname, GLdouble *params)
{
    GLfloat floats[16];
    GLint ints[16];
    GLenum before;
    int count;

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    before = last_error;
    last_error = GL_NO_ERROR;
    glGetFloatv(pname, floats);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = (GLdouble)floats[i];
        }
        last_error = before;
        return;
    }

    last_error = GL_NO_ERROR;
    glGetIntegerv(pname, ints);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = (GLdouble)ints[i];
        }
        last_error = before;
        return;
    }

    last_error = before;
    set_error(GL_INVALID_ENUM);
    params[0] = 0.0;
}

void glGetBooleanv(GLenum pname, GLboolean *params)
{
    GLdouble values[16];
    GLenum before;
    int count;

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    before = last_error;
    last_error = GL_NO_ERROR;
    glGetDoublev(pname, values);
    if (last_error == GL_NO_ERROR) {
        count = get_component_count(pname);
        for (int i = 0; i < count; ++i) {
            params[i] = values[i] != 0.0 ? GL_TRUE : GL_FALSE;
        }
        last_error = before;
        return;
    }

    last_error = before;
    set_error(GL_INVALID_ENUM);
    params[0] = GL_FALSE;
}

void glGetPointerv(GLenum pname, GLvoid **params)
{
    int client_unit = client_texture_index();

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
    case GL_VERTEX_ARRAY_POINTER:
        *params = (GLvoid *)vertex_array.pointer;
        break;
    case GL_COLOR_ARRAY_POINTER:
        *params = (GLvoid *)color_array.pointer;
        break;
    case GL_NORMAL_ARRAY_POINTER:
        *params = (GLvoid *)normal_array.pointer;
        break;
    case GL_TEXTURE_COORD_ARRAY_POINTER:
        *params = client_unit >= 0 ? (GLvoid *)texcoord_array[client_unit].pointer : NULL;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        *params = NULL;
        break;
    }
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    ListCommand command = { LIST_CMD_VIEWPORT };

    if (reject_inside_begin()) {
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    record_command(command);
    if (compile_only()) {
        return;
    }

    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = width;
    viewport[3] = height;
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    ListCommand command = { LIST_CMD_SCISSOR };

    if (reject_inside_begin()) {
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    record_command(command);
    if (compile_only()) {
        return;
    }

    scissor_box[0] = x;
    scissor_box[1] = y;
    scissor_box[2] = width;
    scissor_box[3] = height;
    sync_native_state();
}

void glClearColor(float r, float g, float b, float a)
{
    ListCommand command = { LIST_CMD_CLEAR_COLOR };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = r;
    command.f[1] = g;
    command.f[2] = b;
    command.f[3] = a;
    record_command(command);
    if (compile_only()) {
        return;
    }

    clear_color_value[0] = clamp01(r);
    clear_color_value[1] = clamp01(g);
    clear_color_value[2] = clamp01(b);
    clear_color_value[3] = clamp01(a);
    clear_color = ((uint32_t)channel(a) << 24) |
                  ((uint32_t)channel(r) << 16) |
                  ((uint32_t)channel(g) << 8) |
                  (uint32_t)channel(b);
}

void glClearIndex(GLfloat c)
{
    ListCommand command = { LIST_CMD_CLEAR_INDEX };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = c;
    record_command(command);
    if (compile_only()) {
        return;
    }

    clear_index_value = c;
}

void glClearDepth(GLclampf depth)
{
    ListCommand command = { LIST_CMD_CLEAR_DEPTH };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = depth;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_clear_value = clamp01(depth);
}

void glDepthRange(GLclampf z_near, GLclampf z_far)
{
    ListCommand command = { LIST_CMD_DEPTH_RANGE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = z_near;
    command.f[1] = z_far;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_range_near = clamp01(z_near);
    depth_range_far = clamp01(z_far);
}

void glClearStencil(GLint s)
{
    ListCommand command = { LIST_CMD_CLEAR_STENCIL };
    if (reject_inside_begin()) {
        return;
    }
    command.u = (GLuint)s;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_clear_value = s;
}

void glClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    ListCommand command = { LIST_CMD_CLEAR_ACCUM };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = r;
    command.f[1] = g;
    command.f[2] = b;
    command.f[3] = a;
    record_command(command);
    if (compile_only()) {
        return;
    }
    accum_clear_value[0] = clamp_accum_clear(r);
    accum_clear_value[1] = clamp_accum_clear(g);
    accum_clear_value[2] = clamp_accum_clear(b);
    accum_clear_value[3] = clamp_accum_clear(a);
}

void glClear(uint32_t mask)
{
    ListCommand command = { LIST_CMD_CLEAR };
    const GLbitfield valid_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((mask & ~valid_mask) != 0u) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }

    ensure_native_frame_started();
    nxgl_backend_flush();
    {
        int min_x, min_y, max_x, max_y;
        if (shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
            int width = max_x - min_x + 1;
            int height = max_y - min_y + 1;
            if ((mask & GL_COLOR_BUFFER_BIT) != 0 && draw_buffer_mode != GL_NONE) {
                nxgl_backend_clear_color(clear_color,
                               color_write_mask[0] == GL_TRUE,
                               color_write_mask[1] == GL_TRUE,
                               color_write_mask[2] == GL_TRUE,
                               color_write_mask[3] == GL_TRUE,
                               min_x, min_y, width, height);
            }
            if (((mask & GL_DEPTH_BUFFER_BIT) != 0 && depth_write_enabled) ||
                ((mask & GL_STENCIL_BUFFER_BIT) != 0 && (stencil_write_mask & 0xffu) == 0xffu)) {
                nxgl_backend_clear_depth_stencil((mask & GL_DEPTH_BUFFER_BIT) != 0 && depth_write_enabled,
                                       depth_clear_value,
                                       (mask & GL_STENCIL_BUFFER_BIT) != 0 && (stencil_write_mask & 0xffu) == 0xffu,
                                       (uint8_t)(stencil_clear_value & 0xff),
                                       min_x, min_y, width, height);
            }
        }
    }
    if ((mask & GL_COLOR_BUFFER_BIT) != 0 && draw_buffer_mode != GL_NONE) {
        shadow_clear(clear_color);
    }
    if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
        shadow_clear_depth(depth_clear_value);
    }
    if ((mask & GL_STENCIL_BUFFER_BIT) != 0) {
        shadow_clear_stencil((uint8_t)(stencil_clear_value & 0xff));
    }
    if ((mask & GL_ACCUM_BUFFER_BIT) != 0 && ensure_accum_buffer()) {
        int min_x, min_y, max_x, max_y;
        if (shadow_clear_bounds(&min_x, &min_y, &max_x, &max_y)) {
            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    size_t index = ((size_t)y * (size_t)shadow_width + (size_t)x) * 4u;
                    accum_buffer[index + 0u] = accum_clear_value[0];
                    accum_buffer[index + 1u] = accum_clear_value[1];
                    accum_buffer[index + 2u] = accum_clear_value[2];
                    accum_buffer[index + 3u] = accum_clear_value[3];
                }
            }
        }
    }
    sync_native_state();
}

void glAccum(GLenum op, GLfloat value)
{
    ListCommand command = { LIST_CMD_ACCUM };
    size_t count;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (op != GL_ACCUM && op != GL_LOAD && op != GL_RETURN && op != GL_MULT && op != GL_ADD) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (shadow_color_buffer == NULL || !ensure_accum_buffer()) {
        return;
    }

    command.a = op;
    command.f[0] = value;
    record_command(command);
    if (compile_only()) {
        return;
    }

    nxgl_backend_flush();
    count = (size_t)shadow_width * (size_t)shadow_height;
    if (op == GL_ACCUM || op == GL_LOAD) {
        for (size_t i = 0; i < count; ++i) {
            uint32_t pixel = shadow_color_buffer[i];
            float r = (float)((pixel >> 16) & 0xff) / 255.0f;
            float g = (float)((pixel >> 8) & 0xff) / 255.0f;
            float b = (float)(pixel & 0xff) / 255.0f;
            float *dst = accum_buffer + i * 4u;

            if (op == GL_LOAD) {
                dst[0] = r * value;
                dst[1] = g * value;
                dst[2] = b * value;
                dst[3] = value;
            } else {
                dst[0] += r * value;
                dst[1] += g * value;
                dst[2] += b * value;
                dst[3] += value;
            }
        }
    } else if (op == GL_ADD) {
        for (size_t i = 0; i < count * 4u; ++i) {
            accum_buffer[i] += value;
        }
    } else if (op == GL_MULT) {
        for (size_t i = 0; i < count * 4u; ++i) {
            accum_buffer[i] *= value;
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            const float *src = accum_buffer + i * 4u;
            uint8_t r = channel(clamp01(src[0] * value));
            uint8_t g = channel(clamp01(src[1] * value));
            uint8_t b = channel(clamp01(src[2] * value));
            shadow_color_buffer[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

void glMatrixMode(uint32_t mode)
{
    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_MODELVIEW && mode != GL_PROJECTION && mode != GL_TEXTURE) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_MATRIX_MODE };
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }

    matrix_mode = mode;
}

void glLoadIdentity(void)
{
    ListCommand command = { LIST_CMD_LOAD_IDENTITY };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    matrix_identity(*current_matrix());
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

void glPushMatrix(void)
{
    ListCommand command = { LIST_CMD_PUSH_MATRIX };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (matrix_mode == GL_MODELVIEW) {
        if (modelview_stack_top >= 31) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(modelview_stack[modelview_stack_top++], modelview, sizeof(Matrix));
    } else if (matrix_mode == GL_PROJECTION) {
        if (projection_stack_top >= 7) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(projection_stack[projection_stack_top++], projection, sizeof(Matrix));
    } else {
        if (texture_stack_top >= 7) {
            set_error(GL_STACK_OVERFLOW);
            return;
        }
        memcpy(texture_stack[texture_stack_top++], texture_matrix, sizeof(Matrix));
    }
}

void glPopMatrix(void)
{
    ListCommand command = { LIST_CMD_POP_MATRIX };
    if (reject_inside_begin()) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (matrix_mode == GL_MODELVIEW) {
        if (modelview_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(modelview, modelview_stack[--modelview_stack_top], sizeof(Matrix));
        invalidate_modelview_inverse_cache();
    } else if (matrix_mode == GL_PROJECTION) {
        if (projection_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(projection, projection_stack[--projection_stack_top], sizeof(Matrix));
    } else {
        if (texture_stack_top <= 0) {
            set_error(GL_STACK_UNDERFLOW);
            return;
        }
        memcpy(texture_matrix, texture_stack[--texture_stack_top], sizeof(Matrix));
    }
}

static void load_current_matrix(const GLfloat *m)
{
    memcpy(*current_matrix(), m, sizeof(Matrix));
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

static void mult_current_matrix(const GLfloat *m)
{
    Matrix *dst = current_matrix();
    matrix_multiply(*dst, *dst, m);
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

static void premult_current_matrix(const GLfloat *m)
{
    Matrix *dst = current_matrix();
    matrix_multiply(*dst, m, *dst);
    if (matrix_mode == GL_MODELVIEW) {
        invalidate_modelview_inverse_cache();
    }
}

void glLoadMatrixf(const GLfloat *m)
{
    ListCommand command = { LIST_CMD_LOAD_MATRIX };

    if (reject_inside_begin()) {
        return;
    }
    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.matrix, m, sizeof(Matrix));
    record_command(command);
    if (compile_only()) {
        return;
    }

    load_current_matrix(m);
}

void glMultMatrixf(const GLfloat *m)
{
    ListCommand command = { LIST_CMD_MULT_MATRIX };

    if (reject_inside_begin()) {
        return;
    }
    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.matrix, m, sizeof(Matrix));
    record_command(command);
    if (compile_only()) {
        return;
    }

    mult_current_matrix(m);
}

void glLoadMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glLoadMatrixf(converted);
}

void glMultMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glMultMatrixf(converted);
}

void glLoadTransposeMatrixf(const GLfloat *m)
{
    Matrix transposed;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    matrix_transpose(transposed, m);
    glLoadMatrixf(transposed);
}

void glMultTransposeMatrixf(const GLfloat *m)
{
    Matrix transposed;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    matrix_transpose(transposed, m);
    glMultMatrixf(transposed);
}

void glLoadTransposeMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glLoadTransposeMatrixf(converted);
}

void glMultTransposeMatrixd(const GLdouble *m)
{
    Matrix converted;

    if (m == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < 16; ++i) {
        converted[i] = (GLfloat)m[i];
    }
    glMultTransposeMatrixf(converted);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far)
{
    Matrix m;

    if (right == left || top == bottom || z_far == z_near) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    matrix_identity(m);
    m[M11] = (GLfloat)(2.0 / (right - left));
    m[M22] = (GLfloat)(2.0 / (top - bottom));
    m[M33] = (GLfloat)(-2.0 / (z_far - z_near));
    m[M41] = (GLfloat)(-(right + left) / (right - left));
    m[M42] = (GLfloat)(-(top + bottom) / (top - bottom));
    m[M43] = (GLfloat)(-(z_far + z_near) / (z_far - z_near));
    glMultMatrixf(m);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far)
{
    Matrix m;

    if (right == left || top == bottom || z_far == z_near || z_near <= 0.0 || z_far <= 0.0) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    memset(m, 0, sizeof(m));
    m[M11] = (GLfloat)((2.0 * z_near) / (right - left));
    m[M22] = (GLfloat)((2.0 * z_near) / (top - bottom));
    m[M31] = (GLfloat)((right + left) / (right - left));
    m[M32] = (GLfloat)((top + bottom) / (top - bottom));
    m[M33] = (GLfloat)(-(z_far + z_near) / (z_far - z_near));
    m[M34] = -1.0f;
    m[M43] = (GLfloat)(-(2.0 * z_far * z_near) / (z_far - z_near));
    glMultMatrixf(m);
}

void glTranslatef(float x, float y, float z)
{
    ListCommand command = { LIST_CMD_TRANSLATE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix t;
    matrix_identity(t);
    t[M41] = x;
    t[M42] = y;
    t[M43] = z;
    mult_current_matrix(t);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    ListCommand command = { LIST_CMD_SCALE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix s;
    matrix_identity(s);
    s[M11] = x;
    s[M22] = y;
    s[M33] = z;
    premult_current_matrix(s);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRotatef(float angle_degrees, float x, float y, float z)
{
    ListCommand command = { LIST_CMD_ROTATE };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = angle_degrees;
    command.f[1] = x;
    command.f[2] = y;
    command.f[3] = z;
    record_command(command);
    if (compile_only()) {
        return;
    }

    Matrix r;
    float radians = angle_degrees * 0.017453292519943295f;
    matrix_identity(r);

    float axis_len = sqrtf(x * x + y * y + z * z);
    if (axis_len > 0.0f) {
        float inv_axis_len = 1.0f / axis_len;
        float s = sinf(radians);
        float c = cosf(radians);
        float one_minus_c = 1.0f - c;

        x *= inv_axis_len;
        y *= inv_axis_len;
        z *= inv_axis_len;

        r[M11] = x * x * one_minus_c + c;
        r[M12] = x * y * one_minus_c + z * s;
        r[M13] = x * z * one_minus_c - y * s;
        r[M21] = x * y * one_minus_c - z * s;
        r[M22] = y * y * one_minus_c + c;
        r[M23] = y * z * one_minus_c + x * s;
        r[M31] = x * z * one_minus_c + y * s;
        r[M32] = y * z * one_minus_c - x * s;
        r[M33] = z * z * one_minus_c + c;
    }

    premult_current_matrix(r);
}

void glRotated(GLdouble angle_degrees, GLdouble x, GLdouble y, GLdouble z)
{
    glRotatef((GLfloat)angle_degrees, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glColor3f(float r, float g, float b)
{
    glColor4f(r, g, b, 1.0f);
}

#define NXGL_DEFINE_COLOR3(suffix, type, convert) \
    void glColor3##suffix(type r, type g, type b) \
    { \
        glColor4f(convert(r), convert(g), convert(b), 1.0f); \
    } \
    void glColor3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glColor3##suffix(v[0], v[1], v[2]); \
    }

#define NXGL_DEFINE_COLOR4(suffix, type, convert) \
    void glColor4##suffix(type r, type g, type b, type a) \
    { \
        glColor4f(convert(r), convert(g), convert(b), convert(a)); \
    } \
    void glColor4##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glColor4##suffix(v[0], v[1], v[2], v[3]); \
    }

#define NXGL_FLOAT(value) ((GLfloat)(value))

NXGL_DEFINE_COLOR3(b, GLbyte, normalized_glbyte)
NXGL_DEFINE_COLOR3(d, GLdouble, NXGL_FLOAT)
NXGL_DEFINE_COLOR3(i, GLint, normalized_glint)
NXGL_DEFINE_COLOR3(s, GLshort, normalized_glshort)
NXGL_DEFINE_COLOR3(ub, GLubyte, normalized_glubyte)
NXGL_DEFINE_COLOR3(ui, GLuint, normalized_gluint)
NXGL_DEFINE_COLOR3(us, GLushort, normalized_glushort)
NXGL_DEFINE_COLOR4(b, GLbyte, normalized_glbyte)
NXGL_DEFINE_COLOR4(d, GLdouble, NXGL_FLOAT)
NXGL_DEFINE_COLOR4(i, GLint, normalized_glint)
NXGL_DEFINE_COLOR4(s, GLshort, normalized_glshort)
NXGL_DEFINE_COLOR4(ub, GLubyte, normalized_glubyte)
NXGL_DEFINE_COLOR4(ui, GLuint, normalized_gluint)
NXGL_DEFINE_COLOR4(us, GLushort, normalized_glushort)

#undef NXGL_FLOAT
#undef NXGL_DEFINE_COLOR4
#undef NXGL_DEFINE_COLOR3

void glColor3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glColor3f(v[0], v[1], v[2]);
}

void glColor4f(float r, float g, float b, float a)
{
    if (is_recording()) {
        ListCommand command = { LIST_CMD_COLOR4 };
        command.f[0] = r;
        command.f[1] = g;
        command.f[2] = b;
        command.f[3] = a;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    current_color = (NxglBackendColor){ r, g, b, a };
    apply_color_material(current_color);
}

void glColor4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glColor4f(v[0], v[1], v[2], v[3]);
}

void glIndexf(GLfloat c)
{
    ListCommand command = { LIST_CMD_INDEX };
    command.f[0] = c;
    record_command(command);
    if (compile_only()) {
        return;
    }

    current_index = c;
}

void glIndexfv(const GLfloat *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexf(c[0]);
}

void glIndexd(GLdouble c)
{
    glIndexf((GLfloat)c);
}

void glIndexdv(const GLdouble *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexd(c[0]);
}

void glIndexi(GLint c)
{
    glIndexf((GLfloat)c);
}

void glIndexiv(const GLint *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexi(c[0]);
}

void glIndexs(GLshort c)
{
    glIndexf((GLfloat)c);
}

void glIndexsv(const GLshort *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexs(c[0]);
}

void glIndexub(GLubyte c)
{
    glIndexf((GLfloat)c);
}

void glIndexubv(const GLubyte *c)
{
    if (c == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glIndexub(c[0]);
}

void glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    if (is_recording()) {
        ListCommand command = { LIST_CMD_NORMAL3 };
        command.f[0] = x;
        command.f[1] = y;
        command.f[2] = z;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }
    current_normal = (NxglBackendVec3){ x, y, z };
}

static void glNormal3f_normalized(GLfloat x, GLfloat y, GLfloat z)
{
    NxglBackendVec3 normal = normalize_vec3((NxglBackendVec3){ x, y, z });
    glNormal3f(normal.x, normal.y, normal.z);
}

void glNormal3d(GLdouble x, GLdouble y, GLdouble z)
{
    glNormal3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glNormal3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glNormal3d(v[0], v[1], v[2]);
}

#define NXGL_DEFINE_NORMAL3_INT(suffix, type) \
    void glNormal3##suffix(type x, type y, type z) \
    { \
        glNormal3f_normalized((GLfloat)x, (GLfloat)y, (GLfloat)z); \
    } \
    void glNormal3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glNormal3##suffix(v[0], v[1], v[2]); \
    }

NXGL_DEFINE_NORMAL3_INT(b, GLbyte)
NXGL_DEFINE_NORMAL3_INT(i, GLint)
NXGL_DEFINE_NORMAL3_INT(s, GLshort)

#undef NXGL_DEFINE_NORMAL3_INT

void glNormal3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glNormal3f(v[0], v[1], v[2]);
}

void glTexCoord1f(GLfloat s)
{
    glTexCoord3f(s, 0.0f, 0.0f);
}

void glTexCoord1fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord1f(v[0]);
}

void glTexCoord2f(float s, float t)
{
    int unit = active_texture_index();
    if (is_recording()) {
        ListCommand command = { LIST_CMD_TEXCOORD2 };
        command.f[0] = s;
        command.f[1] = t;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    if (unit >= 0) {
        current_u[unit] = s;
        current_v[unit] = t;
        current_r[unit] = 0.0f;
    }
}

void glTexCoord2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord2f(v[0], v[1]);
}

void glTexCoord3f(float s, float t, float r)
{
    int unit = active_texture_index();
    if (is_recording()) {
        ListCommand command = { LIST_CMD_TEXCOORD3 };
        command.f[0] = s;
        command.f[1] = t;
        command.f[2] = r;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    if (unit >= 0) {
        current_u[unit] = s;
        current_v[unit] = t;
        current_r[unit] = r;
    }
}

void glTexCoord3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord3f(v[0], v[1], v[2]);
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    if (q != 0.0f) {
        glTexCoord3f(s / q, t / q, r / q);
    } else {
        glTexCoord3f(s, t, r);
    }
}

void glTexCoord4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexCoord4f(v[0], v[1], v[2], v[3]);
}

#define NXGL_DEFINE_TEXCOORDS(suffix, type) \
    void glTexCoord1##suffix(type s) \
    { \
        glTexCoord1f((GLfloat)s); \
    } \
    void glTexCoord1##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord1##suffix(v[0]); \
    } \
    void glTexCoord2##suffix(type s, type t) \
    { \
        glTexCoord2f((GLfloat)s, (GLfloat)t); \
    } \
    void glTexCoord2##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord2##suffix(v[0], v[1]); \
    } \
    void glTexCoord3##suffix(type s, type t, type r) \
    { \
        glTexCoord3f((GLfloat)s, (GLfloat)t, (GLfloat)r); \
    } \
    void glTexCoord3##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord3##suffix(v[0], v[1], v[2]); \
    } \
    void glTexCoord4##suffix(type s, type t, type r, type q) \
    { \
        glTexCoord4f((GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q); \
    } \
    void glTexCoord4##suffix##v(const type *v) \
    { \
        if (v == NULL) { \
            set_error(GL_INVALID_VALUE); \
            return; \
        } \
        glTexCoord4##suffix(v[0], v[1], v[2], v[3]); \
    }

NXGL_DEFINE_TEXCOORDS(d, GLdouble)
NXGL_DEFINE_TEXCOORDS(i, GLint)
NXGL_DEFINE_TEXCOORDS(s, GLshort)

#undef NXGL_DEFINE_TEXCOORDS

void glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t)
{
    int unit = texture_unit_index(texture);

    if (unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    current_u[unit] = s;
    current_v[unit] = t;
    current_r[unit] = 0.0f;
}

void glMultiTexCoord3f(GLenum texture, GLfloat s, GLfloat t, GLfloat r)
{
    int unit = texture_unit_index(texture);

    if (unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    current_u[unit] = s;
    current_v[unit] = t;
    current_r[unit] = r;
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;
    ListCommand command = { LIST_CMD_TEX_GEN };

    if (coord_index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_GEN_MODE && !valid_texgen_mode(coord, (GLenum)((GLint)params[0]))) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pname != GL_TEXTURE_GEN_MODE && pname != GL_OBJECT_PLANE && pname != GL_EYE_PLANE) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = coord;
    command.b = pname;
    if (pname == GL_TEXTURE_GEN_MODE) {
        command.u = (GLuint)((GLint)params[0]);
    } else {
        command.f[0] = params[0];
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (unit < 0) {
        return;
    }

    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        state->mode = (GLenum)((GLint)params[0]);
    } else if (pname == GL_OBJECT_PLANE) {
        memcpy(state->object_plane, params, sizeof(state->object_plane));
    } else {
        memcpy(state->eye_plane, params, sizeof(state->eye_plane));
    }
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    GLfloat converted[4];
    if (params == NULL) {
        glTexGenfv(coord, pname, NULL);
        return;
    }
    converted[0] = (GLfloat)params[0];
    if (pname != GL_TEXTURE_GEN_MODE) {
        converted[1] = (GLfloat)params[1];
        converted[2] = (GLfloat)params[2];
        converted[3] = (GLfloat)params[3];
    }
    glTexGenfv(coord, pname, converted);
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    GLfloat params[4] = { param, 0.0f, 0.0f, 0.0f };
    glTexGenfv(coord, pname, params);
}

void glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    GLfloat params[4] = { (GLfloat)param, 0.0f, 0.0f, 0.0f };
    glTexGenfv(coord, pname, params);
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (coord_index < 0 || unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        params[0] = (GLfloat)state->mode;
    } else if (pname == GL_OBJECT_PLANE) {
        memcpy(params, state->object_plane, sizeof(state->object_plane));
    } else if (pname == GL_EYE_PLANE) {
        memcpy(params, state->eye_plane, sizeof(state->eye_plane));
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    int coord_index = texgen_coord_index(coord);
    TexGenState *state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (coord_index < 0 || unit < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    state = &texgen_state[unit][coord_index];
    if (pname == GL_TEXTURE_GEN_MODE) {
        params[0] = (GLint)state->mode;
    } else if (pname == GL_OBJECT_PLANE) {
        for (int i = 0; i < 4; ++i) params[i] = (GLint)state->object_plane[i];
    } else if (pname == GL_EYE_PLANE) {
        for (int i = 0; i < 4; ++i) params[i] = (GLint)state->eye_plane[i];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glBegin(uint32_t mode)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (mode != GL_POINTS &&
        mode != GL_LINES &&
        mode != GL_LINE_LOOP &&
        mode != GL_LINE_STRIP &&
        mode != GL_TRIANGLES &&
        mode != GL_TRIANGLE_STRIP &&
        mode != GL_TRIANGLE_FAN &&
        mode != GL_QUADS &&
        mode != GL_QUAD_STRIP &&
        mode != GL_POLYGON) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_BEGIN };
        command.a = mode;
        record_command(command);
    }
    begin_mode = mode;
    pending_count = 0;
    if (compile_only()) {
        return;
    }
}

void glVertex2d(GLdouble x, GLdouble y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2d(v[0], v[1]);
}

void glVertex2f(GLfloat x, GLfloat y)
{
    glVertex3f(x, y, 0.0f);
}

void glVertex2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2f(v[0], v[1]);
}

void glVertex2i(GLint x, GLint y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2i(v[0], v[1]);
}

void glVertex2s(GLshort x, GLshort y)
{
    glVertex3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glVertex2sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex2s(v[0], v[1]);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3d(v[0], v[1], v[2]);
}

void glVertex3f(float x, float y, float z)
{
    NxglBackendVertex vertex;
    GLfloat obj[4] = { x, y, z, 1.0f };
    GLfloat u0 = current_u[0];
    GLfloat v0 = current_v[0];
    GLfloat r0 = current_r[0];
    GLfloat u1 = current_u[1];
    GLfloat v1 = current_v[1];
    GLfloat r1 = current_r[1];
    GLfloat u2 = current_u[2];
    GLfloat v2 = current_v[2];
    GLfloat r2 = current_r[2];
    GLfloat u3 = current_u[3];
    GLfloat v3 = current_v[3];
    GLfloat r3 = current_r[3];

    if (begin_mode == NXGL_NO_BEGIN_MODE || pending_count >= (int)(sizeof(pending) / sizeof(pending[0]))) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_VERTEX3 };
        command.f[0] = x;
        command.f[1] = y;
        command.f[2] = z;
        record_command(command);
        if (compile_only()) {
            return;
        }
    }

    memset(&vertex, 0, sizeof(vertex));
    init_vertex_position(&vertex, x, y, z, 1.0f);
    vertex.base_color = current_color;
    if (!lighting_enabled && !fog_enabled && !texgen_enabled()) {
        vertex.normal = current_normal;
        vertex.color = current_color;
    } else {
        NxglBackendVec3 normal = transform_normal_to_eye(current_normal);
        vertex.normal = normal;
        apply_texgen_to_coords(obj, vertex.eye, normal, &u0, &v0, &r0, 0);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u1, &v1, &r1, 1);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u2, &v2, &r2, 2);
        apply_texgen_to_coords(obj, vertex.eye, normal, &u3, &v3, &r3, 3);
        vertex.color = lit_color(current_color, normal, vertex.eye);
        vertex.color = apply_fog(vertex.color, vertex.eye);
    }
    vertex.u = u0;
    vertex.v = v0;
    vertex.r = r0;
    vertex.u1 = u1;
    vertex.v1 = v1;
    vertex.r1 = r1;
    vertex.u2 = u2;
    vertex.v2 = v2;
    vertex.r2 = r2;
    vertex.u3 = u3;
    vertex.v3 = v3;
    vertex.r3 = r3;
    pending[pending_count++] = vertex;
}

void glVertex3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3f(v[0], v[1], v[2]);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3i(v[0], v[1], v[2]);
}

void glVertex3s(GLshort x, GLshort y, GLshort z)
{
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glVertex3sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex3s(v[0], v[1], v[2]);
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (w != 0.0f) {
        glVertex3f(x / w, y / w, z / w);
    } else {
        glVertex3f(x, y, z);
    }
}

void glVertex4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4f(v[0], v[1], v[2], v[3]);
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4d(v[0], v[1], v[2], v[3]);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4i(v[0], v[1], v[2], v[3]);
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glVertex4sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glVertex4s(v[0], v[1], v[2], v[3]);
}

void glEnd(void)
{
    uint32_t mode;

    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (is_recording()) {
        ListCommand command = { LIST_CMD_END };
        record_command(command);
    }
    if (compile_only()) {
        pending_count = 0;
        begin_mode = NXGL_NO_BEGIN_MODE;
        return;
    }

    mode = begin_mode;
    if (native_fast_fill_enabled() && !lighting_enabled && shade_model == GL_SMOOTH && mode == GL_QUADS) {
        ensure_native_frame_started();
        for (int i = 0; i + 3 < pending_count; i += 4) {
            nxgl_backend_push_quad(pending[i], pending[i + 1], pending[i + 2], pending[i + 3]);
        }
    } else {
        emit_vertices(mode, pending, pending_count);
    }
    pending_count = 0;
    begin_mode = NXGL_NO_BEGIN_MODE;
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, -y1);
    glVertex2f(x2, -y1);
    glVertex2f(x2, -y2);
    glVertex2f(x1, -y2);
    glEnd();
}

void glRectfv(const GLfloat *v1, const GLfloat *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRectf(v1[0], v1[1], v2[0], v2[1]);
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectdv(const GLdouble *v1, const GLdouble *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRectd(v1[0], v1[1], v2[0], v2[1]);
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectiv(const GLint *v1, const GLint *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRecti(v1[0], v1[1], v2[0], v2[1]);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2);
}

void glRectsv(const GLshort *v1, const GLshort *v2)
{
    if (v1 == NULL || v2 == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRects(v1[0], v1[1], v2[0], v2[1]);
}

void glPointSize(GLfloat size)
{
    ListCommand command = { LIST_CMD_POINT_SIZE };

    if (reject_inside_begin()) {
        return;
    }
    if (size <= 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = size;
    record_command(command);
    if (compile_only()) {
        return;
    }
    point_size = size;
}

void glLineWidth(GLfloat width)
{
    ListCommand command = { LIST_CMD_LINE_WIDTH };

    if (reject_inside_begin()) {
        return;
    }
    if (width <= 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.f[0] = width;
    record_command(command);
    if (compile_only()) {
        return;
    }
    line_width = width;
}

void glLineStipple(GLint factor, GLushort pattern)
{
    ListCommand command = { LIST_CMD_LINE_STIPPLE };

    if (reject_inside_begin()) {
        return;
    }
    if (factor < 1) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (factor > 256) {
        factor = 256;
    }
    command.u = (GLuint)pattern;
    command.f[0] = (GLfloat)factor;
    record_command(command);
    if (compile_only()) {
        return;
    }
    line_stipple_factor = factor;
    line_stipple_pattern = pattern;
}

void glPolygonStipple(const GLubyte *mask)
{
    ListCommand command = { LIST_CMD_POLYGON_STIPPLE };

    if (reject_inside_begin()) {
        return;
    }
    if (mask == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(command.bytes, mask, sizeof(command.bytes));
    record_command(command);
    if (compile_only()) {
        return;
    }
    memcpy(polygon_stipple_pattern, mask, sizeof(polygon_stipple_pattern));
}

void glGetPolygonStipple(GLubyte *mask)
{
    if (reject_inside_begin()) {
        return;
    }
    if (mask == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    memcpy(mask, polygon_stipple_pattern, sizeof(polygon_stipple_pattern));
}

void glPolygonMode(GLenum face, GLenum mode)
{
    ListCommand command = { LIST_CMD_POLYGON_MODE };

    if (reject_inside_begin()) {
        return;
    }
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = face;
    command.b = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    (void)face;
    polygon_mode = mode;
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    ListCommand command = { LIST_CMD_POLYGON_OFFSET };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = factor;
    command.f[1] = units;
    record_command(command);
    if (compile_only()) {
        return;
    }
    polygon_offset_factor = factor;
    polygon_offset_units = units;
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    ListCommand command = { LIST_CMD_ALPHA_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    command.f[0] = ref;
    record_command(command);
    if (compile_only()) {
        return;
    }
    alpha_test_func = func;
    alpha_test_ref = clamp01(ref);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    ListCommand command = { LIST_CMD_COLOR_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = red ? 1.0f : 0.0f;
    command.f[1] = green ? 1.0f : 0.0f;
    command.f[2] = blue ? 1.0f : 0.0f;
    command.f[3] = alpha ? 1.0f : 0.0f;
    record_command(command);
    if (compile_only()) {
        return;
    }
    color_write_mask[0] = red ? GL_TRUE : GL_FALSE;
    color_write_mask[1] = green ? GL_TRUE : GL_FALSE;
    color_write_mask[2] = blue ? GL_TRUE : GL_FALSE;
    color_write_mask[3] = alpha ? GL_TRUE : GL_FALSE;
}

void glLogicOp(GLenum opcode)
{
    ListCommand command = { LIST_CMD_LOGIC_OP };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_logic_op(opcode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = opcode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    logic_op_mode = opcode;
}

void glShadeModel(GLenum mode)
{
    ListCommand command = { LIST_CMD_SHADE_MODEL };

    if (mode != GL_FLAT && mode != GL_SMOOTH) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    shade_model = mode;
}

void glCullFace(GLenum mode)
{
    ListCommand command = { LIST_CMD_CULL_FACE };

    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    cull_face_mode = mode;
    sync_native_state();
}

void glFrontFace(GLenum mode)
{
    ListCommand command = { LIST_CMD_FRONT_FACE };

    if (reject_inside_begin()) {
        return;
    }
    if (mode != GL_CW && mode != GL_CCW) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    front_face_mode = mode;
    sync_native_state();
}

void glHint(GLenum target, GLenum mode)
{
    ListCommand command = { LIST_CMD_HINT };

    if (!valid_hint_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_hint_mode(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = target;
    command.b = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (target == GL_PERSPECTIVE_CORRECTION_HINT) {
        perspective_correction_hint = mode;
    } else if (target == GL_POINT_SMOOTH_HINT) {
        point_smooth_hint = mode;
    } else if (target == GL_LINE_SMOOTH_HINT) {
        line_smooth_hint = mode;
    } else if (target == GL_POLYGON_SMOOTH_HINT) {
        polygon_smooth_hint = mode;
    } else {
        fog_hint = mode;
    }
}

void glEdgeFlag(GLboolean flag)
{
    ListCommand command = { LIST_CMD_EDGE_FLAG };
    command.u = flag ? GL_TRUE : GL_FALSE;
    record_command(command);
    if (compile_only()) {
        return;
    }
    current_edge_flag = flag ? GL_TRUE : GL_FALSE;
}

void glEdgeFlagv(const GLboolean *flag)
{
    if (flag == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glEdgeFlag(*flag);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_LIGHT_MODEL };

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        command.a = pname;
        memcpy(command.f, params, sizeof(command.f));
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER) {
        glLightModeli(pname, params[0] != 0.0f ? 1 : 0);
    } else if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        glLightModeli(pname, params[0] != 0.0f ? 1 : 0);
    } else if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
        glLightModeli(pname, (GLint)params[0]);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glLightModelf(GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glLightModelfv(pname, values);
}

void glLightModeliv(GLenum pname, const GLint *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        GLfloat values[4] = {
            (GLfloat)params[0],
            (GLfloat)params[1],
            (GLfloat)params[2],
            (GLfloat)params[3]
        };
        glLightModelfv(pname, values);
    } else {
        glLightModeli(pname, params[0]);
    }
}

void glLightModeli(GLenum pname, GLint param)
{
    ListCommand command = { LIST_CMD_LIGHT_MODEL };

    if (reject_inside_begin()) {
        return;
    }
    if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
        if (param != GL_SINGLE_COLOR && param != GL_SEPARATE_SPECULAR_COLOR) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_color_control = (GLenum)param;
    } else if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER) {
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_local_viewer = param != 0;
    } else if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        command.a = pname;
        command.f[0] = (GLfloat)param;
        record_command(command);
        if (compile_only()) {
            return;
        }
        light_model_two_side = param != 0;
    } else if (pname == GL_LIGHT_MODEL_AMBIENT) {
        GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
        glLightModelfv(pname, values);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glLightfv(light, pname, values);
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glLightfv(light, pname, values);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_LIGHT };
    int index = light_index(light);

    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if (pname == GL_SPOT_EXPONENT) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname == GL_SPOT_CUTOFF) {
        if ((params[0] < 0.0f || params[0] > 90.0f) && params[0] != 180.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname == GL_CONSTANT_ATTENUATION ||
               pname == GL_LINEAR_ATTENUATION ||
               pname == GL_QUADRATIC_ATTENUATION) {
        if (params[0] < 0.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
    } else if (pname != GL_AMBIENT &&
               pname != GL_DIFFUSE &&
               pname != GL_SPECULAR &&
               pname != GL_POSITION &&
               pname != GL_SPOT_DIRECTION) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = light;
    command.b = pname;
    command.f[0] = params[0];
    if (pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION) {
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        command.f[1] = params[1];
        command.f[2] = params[2];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_AMBIENT) {
        lights[index].ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_DIFFUSE) {
        lights[index].diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_SPECULAR) {
        lights[index].specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if (pname == GL_POSITION) {
        lights[index].position[0] = params[0];
        lights[index].position[1] = params[1];
        lights[index].position[2] = params[2];
        lights[index].position[3] = params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        lights[index].spot_direction[0] = params[0];
        lights[index].spot_direction[1] = params[1];
        lights[index].spot_direction[2] = params[2];
    } else if (pname == GL_SPOT_EXPONENT) {
        lights[index].spot_exponent = params[0];
    } else if (pname == GL_SPOT_CUTOFF) {
        lights[index].spot_cutoff = params[0];
    } else if (pname == GL_CONSTANT_ATTENUATION) {
        lights[index].constant_attenuation = params[0];
    } else if (pname == GL_LINEAR_ATTENUATION) {
        lights[index].linear_attenuation = params[0];
    } else if (pname == GL_QUADRATIC_ATTENUATION) {
        lights[index].quadratic_attenuation = params[0];
    }
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else if (pname == GL_SPOT_DIRECTION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = 0.0f;
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glLightfv(light, pname, values);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    int index = light_index(light);

    if (reject_inside_begin()) {
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        return;
    }

    if (pname == GL_AMBIENT) {
        params[0] = lights[index].ambient.r;
        params[1] = lights[index].ambient.g;
        params[2] = lights[index].ambient.b;
        params[3] = lights[index].ambient.a;
    } else if (pname == GL_DIFFUSE) {
        params[0] = lights[index].diffuse.r;
        params[1] = lights[index].diffuse.g;
        params[2] = lights[index].diffuse.b;
        params[3] = lights[index].diffuse.a;
    } else if (pname == GL_SPECULAR) {
        params[0] = lights[index].specular.r;
        params[1] = lights[index].specular.g;
        params[2] = lights[index].specular.b;
        params[3] = lights[index].specular.a;
    } else if (pname == GL_POSITION) {
        memcpy(params, lights[index].position, sizeof(lights[index].position));
    } else if (pname == GL_SPOT_DIRECTION) {
        params[0] = lights[index].spot_direction[0];
        params[1] = lights[index].spot_direction[1];
        params[2] = lights[index].spot_direction[2];
    } else if (pname == GL_SPOT_EXPONENT) {
        params[0] = lights[index].spot_exponent;
    } else if (pname == GL_SPOT_CUTOFF) {
        params[0] = lights[index].spot_cutoff;
    } else if (pname == GL_CONSTANT_ATTENUATION) {
        params[0] = lights[index].constant_attenuation;
    } else if (pname == GL_LINEAR_ATTENUATION) {
        params[0] = lights[index].linear_attenuation;
    } else if (pname == GL_QUADRATIC_ATTENUATION) {
        params[0] = lights[index].quadratic_attenuation;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
    GLfloat values[4];
    GLenum before;
    int count = pname == GL_AMBIENT || pname == GL_DIFFUSE || pname == GL_SPECULAR || pname == GL_POSITION ? 4 :
                pname == GL_SPOT_DIRECTION ? 3 : 1;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetLightfv(light, pname, values);
    if (last_error == GL_NO_ERROR) {
        for (int i = 0; i < count; ++i) {
            params[i] = (GLint)values[i];
        }
        last_error = before;
    } else {
        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glMaterialfv(face, pname, values);
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glMaterialfv(face, pname, values);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT) {
        material_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_DIFFUSE) {
        material_state.diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT_AND_DIFFUSE) {
        material_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
        material_state.diffuse = material_state.ambient;
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_SPECULAR) {
        material_state.specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_EMISSION) {
        material_state.emission = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_FRONT || face == GL_FRONT_AND_BACK) && pname == GL_SHININESS) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        material_state.shininess = params[0];
    }

    if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT) {
        material_back_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_DIFFUSE) {
        material_back_state.diffuse = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_AMBIENT_AND_DIFFUSE) {
        material_back_state.ambient = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
        material_back_state.diffuse = material_back_state.ambient;
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_SPECULAR) {
        material_back_state.specular = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_EMISSION) {
        material_back_state.emission = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
    } else if ((face == GL_BACK || face == GL_FRONT_AND_BACK) && pname == GL_SHININESS) {
        if (params[0] < 0.0f || params[0] > 128.0f) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        material_back_state.shininess = params[0];
    }

    if (pname != GL_AMBIENT &&
        pname != GL_DIFFUSE &&
        pname != GL_AMBIENT_AND_DIFFUSE &&
        pname != GL_SPECULAR &&
        pname != GL_EMISSION &&
        pname != GL_SHININESS) {
        set_error(GL_INVALID_ENUM);
    }
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_AMBIENT ||
        pname == GL_DIFFUSE ||
        pname == GL_AMBIENT_AND_DIFFUSE ||
        pname == GL_SPECULAR ||
        pname == GL_EMISSION) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glMaterialfv(face, pname, values);
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    const MaterialState *material = &material_state;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
        return;
    }
    if (face == GL_BACK) {
        material = &material_back_state;
    }

    if (pname == GL_AMBIENT) {
        params[0] = material->ambient.r;
        params[1] = material->ambient.g;
        params[2] = material->ambient.b;
        params[3] = material->ambient.a;
    } else if (pname == GL_DIFFUSE) {
        params[0] = material->diffuse.r;
        params[1] = material->diffuse.g;
        params[2] = material->diffuse.b;
        params[3] = material->diffuse.a;
    } else if (pname == GL_SPECULAR) {
        params[0] = material->specular.r;
        params[1] = material->specular.g;
        params[2] = material->specular.b;
        params[3] = material->specular.a;
    } else if (pname == GL_EMISSION) {
        params[0] = material->emission.r;
        params[1] = material->emission.g;
        params[2] = material->emission.b;
        params[3] = material->emission.a;
    } else if (pname == GL_SHININESS) {
        params[0] = material->shininess;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
    GLfloat values[4];
    GLenum before;
    int count = pname == GL_SHININESS ? 1 : 4;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetMaterialfv(face, pname, values);
    if (last_error == GL_NO_ERROR) {
        for (int i = 0; i < count; ++i) {
            params[i] = (GLint)values[i];
        }
        last_error = before;
    } else {
        last_error = before;
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glColorMaterial(GLenum face, GLenum mode)
{
    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mode != GL_AMBIENT &&
        mode != GL_DIFFUSE &&
        mode != GL_AMBIENT_AND_DIFFUSE &&
        mode != GL_SPECULAR &&
        mode != GL_EMISSION) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    color_material_face = face;
    color_material_parameter = mode;
    apply_color_material(current_color);
}

void glFogf(GLenum pname, GLfloat param)
{
    GLfloat values[4] = { param, param, param, param };
    glFogfv(pname, values);
}

void glFogfv(GLenum pname, const GLfloat *params)
{
    ListCommand command = { LIST_CMD_FOG };

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname != GL_FOG_MODE &&
        pname != GL_FOG_COLOR &&
        pname != GL_FOG_DENSITY &&
        pname != GL_FOG_START &&
        pname != GL_FOG_END) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pname == GL_FOG_MODE) {
        GLenum mode = (GLenum)(GLint)params[0];
        if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
            set_error(GL_INVALID_ENUM);
            return;
        }
    } else if (pname == GL_FOG_DENSITY && params[0] < 0.0f) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    command.a = pname;
    command.f[0] = params[0];
    if (pname == GL_FOG_COLOR) {
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else {
        command.f[1] = params[0];
        command.f[2] = params[0];
        command.f[3] = params[0];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_FOG_MODE) {
        fog_mode = (GLenum)(GLint)params[0];
    } else if (pname == GL_FOG_COLOR) {
        fog_color = clamp_color((NxglBackendColor){ params[0], params[1], params[2], params[3] });
    } else if (pname == GL_FOG_DENSITY) {
        fog_density = params[0];
    } else if (pname == GL_FOG_START) {
        fog_start = params[0];
    } else if (pname == GL_FOG_END) {
        fog_end = params[0];
    }
}

void glFogi(GLenum pname, GLint param)
{
    GLfloat values[4] = { (GLfloat)param, (GLfloat)param, (GLfloat)param, (GLfloat)param };
    glFogfv(pname, values);
}

void glFogiv(GLenum pname, const GLint *params)
{
    GLfloat values[4];

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    values[0] = (GLfloat)params[0];
    if (pname == GL_FOG_COLOR) {
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
    } else {
        values[1] = values[0];
        values[2] = values[0];
        values[3] = values[0];
    }
    glFogfv(pname, values);
}

void glClipPlane(GLenum plane, const GLdouble *equation)
{
    int index = clip_plane_index(plane);
    ListCommand command = { LIST_CMD_CLIP_PLANE };
    GLdouble transformed[4];

    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (equation == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.a = plane;
    for (int i = 0; i < 4; ++i) {
        command.f[i] = (GLfloat)equation[i];
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    transform_clip_plane(transformed, equation);
    memcpy(clip_planes[index].equation, transformed, sizeof(transformed));
}

void glGetClipPlane(GLenum plane, GLdouble *equation)
{
    int index = clip_plane_index(plane);

    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (equation == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    memcpy(equation, clip_planes[index].equation, sizeof(clip_planes[index].equation));
}

void glPixelStorei(GLenum pname, GLint param)
{
    switch (pname) {
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        if (pname == GL_PACK_ALIGNMENT) {
            pack_alignment = param;
        } else {
            unpack_alignment = param;
        }
        return;
    case GL_PACK_ROW_LENGTH:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_row_length = param;
        return;
    case GL_PACK_SKIP_ROWS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_rows = param;
        return;
    case GL_PACK_SKIP_PIXELS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_pixels = param;
        return;
    case GL_PACK_IMAGE_HEIGHT:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_image_height = param;
        return;
    case GL_PACK_SKIP_IMAGES:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_skip_images = param;
        return;
    case GL_PACK_SWAP_BYTES:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_swap_bytes = (GLboolean)param;
        return;
    case GL_PACK_LSB_FIRST:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        pack_lsb_first = (GLboolean)param;
        return;
    case GL_UNPACK_ROW_LENGTH:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_row_length = param;
        return;
    case GL_UNPACK_SKIP_ROWS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_rows = param;
        return;
    case GL_UNPACK_SKIP_PIXELS:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_pixels = param;
        return;
    case GL_UNPACK_IMAGE_HEIGHT:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_image_height = param;
        return;
    case GL_UNPACK_SKIP_IMAGES:
        if (param < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_skip_images = param;
        return;
    case GL_UNPACK_SWAP_BYTES:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_swap_bytes = (GLboolean)param;
        return;
    case GL_UNPACK_LSB_FIRST:
        if (param != GL_FALSE && param != GL_TRUE) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        unpack_lsb_first = (GLboolean)param;
        return;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    ListCommand command = { LIST_CMD_PIXEL_ZOOM };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = xfactor;
    command.f[1] = yfactor;
    record_command(command);
    if (compile_only()) {
        return;
    }
    pixel_zoom_x = xfactor;
    pixel_zoom_y = yfactor;
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
    ListCommand command = { LIST_CMD_PIXEL_TRANSFER };
    int index = -1;
    bool scale = false;

    if (reject_inside_begin()) {
        return;
    }
    switch (pname) {
    case GL_RED_SCALE: index = 0; scale = true; break;
    case GL_GREEN_SCALE: index = 1; scale = true; break;
    case GL_BLUE_SCALE: index = 2; scale = true; break;
    case GL_ALPHA_SCALE: index = 3; scale = true; break;
    case GL_RED_BIAS: index = 0; break;
    case GL_GREEN_BIAS: index = 1; break;
    case GL_BLUE_BIAS: index = 2; break;
    case GL_ALPHA_BIAS: index = 3; break;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = pname;
    command.f[0] = param;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (scale) {
        pixel_transfer_scale[index] = param;
    } else {
        pixel_transfer_bias[index] = param;
    }
}

void glPixelTransferi(GLenum pname, GLint param)
{
    glPixelTransferf(pname, (GLfloat)param);
}

void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
    ListCommand command = { LIST_CMD_PIXEL_MAP };
    int index = pixel_map_index(map);

    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX || values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize <= 4) {
        command.a = map;
        command.u = (GLuint)mapsize;
        for (GLsizei i = 0; i < mapsize; ++i) {
            command.f[i] = values[i];
        }
        record_command(command);
    }
    if (compile_only()) {
        return;
    }
    pixel_maps[index].size = mapsize;
    for (GLsizei i = 0; i < mapsize; ++i) {
        pixel_maps[index].values[i] = values[i];
    }
}

void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
    GLfloat converted[NXGL_PIXEL_MAP_MAX];
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < mapsize; ++i) {
        converted[i] = (GLfloat)values[i] / 4294967295.0f;
    }
    glPixelMapfv(map, mapsize, converted);
}

void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
    GLfloat converted[NXGL_PIXEL_MAP_MAX];
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (mapsize < 1 || mapsize > NXGL_PIXEL_MAP_MAX) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < mapsize; ++i) {
        converted[i] = (GLfloat)values[i] / 65535.0f;
    }
    glPixelMapfv(map, mapsize, converted);
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        values[i] = pixel_maps[index].values[i];
    }
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        GLfloat value = clamp01(pixel_maps[index].values[i]);
        values[i] = value >= 1.0f ? 0xffffffffu : (GLuint)((double)value * 4294967295.0 + 0.5);
    }
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
    int index = pixel_map_index(map);
    if (reject_inside_begin()) {
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (values == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < pixel_maps[index].size; ++i) {
        values[i] = (GLushort)(clamp01(pixel_maps[index].values[i]) * 65535.0f + 0.5f);
    }
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);
    EvalMap1 *map;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (index < 0 || map2) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    map = &eval_maps1[index];
    if (u1 == u2 || stride < map->components || order <= 0 || order > NXGL_MAX_EVAL_ORDER || points == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map->defined = true;
    map->u1 = u1;
    map->u2 = u2;
    map->order = order;
    for (GLint i = 0; i < order; ++i) {
        for (GLint c = 0; c < map->components; ++c) {
            map->points[i * map->components + c] = points[i * stride + c];
        }
    }
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    GLfloat converted[NXGL_MAX_EVAL_ORDER * 4];
    int comps = eval_map_components(target);

    if (points == NULL) {
        glMap1f(target, (GLfloat)u1, (GLfloat)u2, stride, order, NULL);
        return;
    }
    if (comps <= 0 || order <= 0 || order > NXGL_MAX_EVAL_ORDER || stride < comps) {
        glMap1f(target, (GLfloat)u1, (GLfloat)u2, stride, order, NULL);
        return;
    }
    for (GLint i = 0; i < order; ++i) {
        for (GLint c = 0; c < comps; ++c) {
            converted[i * comps + c] = (GLfloat)points[i * stride + c];
        }
    }
    glMap1f(target, (GLfloat)u1, (GLfloat)u2, comps, order, converted);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);
    EvalMap2 *map;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (index < 0 || !map2) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    map = &eval_maps2[index];
    if (u1 == u2 || v1 == v2 || ustride < map->components || vstride < ustride * uorder ||
        uorder <= 0 || uorder > NXGL_MAX_EVAL_ORDER || vorder <= 0 || vorder > NXGL_MAX_EVAL_ORDER || points == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map->defined = true;
    map->u1 = u1;
    map->u2 = u2;
    map->v1 = v1;
    map->v2 = v2;
    map->uorder = uorder;
    map->vorder = vorder;
    for (GLint j = 0; j < vorder; ++j) {
        for (GLint i = 0; i < uorder; ++i) {
            for (GLint c = 0; c < map->components; ++c) {
                map->points[((size_t)j * (size_t)uorder + (size_t)i) * 4u + (size_t)c] = points[j * vstride + i * ustride + c];
            }
        }
    }
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    GLfloat converted[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int comps = eval_map_components(target);

    if (points == NULL) {
        glMap2f(target, (GLfloat)u1, (GLfloat)u2, ustride, uorder, (GLfloat)v1, (GLfloat)v2, vstride, vorder, NULL);
        return;
    }
    if (comps <= 0 || uorder <= 0 || uorder > NXGL_MAX_EVAL_ORDER || vorder <= 0 || vorder > NXGL_MAX_EVAL_ORDER ||
        ustride < comps || vstride < ustride * uorder) {
        glMap2f(target, (GLfloat)u1, (GLfloat)u2, ustride, uorder, (GLfloat)v1, (GLfloat)v2, vstride, vorder, NULL);
        return;
    }
    for (GLint j = 0; j < vorder; ++j) {
        for (GLint i = 0; i < uorder; ++i) {
            for (GLint c = 0; c < comps; ++c) {
                converted[((size_t)j * (size_t)uorder + (size_t)i) * (size_t)comps + (size_t)c] =
                    (GLfloat)points[j * vstride + i * ustride + c];
            }
        }
    }
    glMap2f(target, (GLfloat)u1, (GLfloat)u2, comps, uorder, (GLfloat)v1, (GLfloat)v2, comps * uorder, vorder, converted);
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!map2) {
        EvalMap1 *map = &eval_maps1[index];
        if (query == GL_DOMAIN) {
            v[0] = map->u1;
            v[1] = map->u2;
        } else if (query == GL_ORDER) {
            v[0] = (GLfloat)map->order;
        } else if (query == GL_COEFF) {
            memcpy(v, map->points, (size_t)map->order * (size_t)map->components * sizeof(GLfloat));
        } else {
            set_error(GL_INVALID_ENUM);
        }
    } else {
        EvalMap2 *map = &eval_maps2[index];
        if (query == GL_DOMAIN) {
            v[0] = map->u1;
            v[1] = map->u2;
            v[2] = map->v1;
            v[3] = map->v2;
        } else if (query == GL_ORDER) {
            v[0] = (GLfloat)map->uorder;
            v[1] = (GLfloat)map->vorder;
        } else if (query == GL_COEFF) {
            for (GLint j = 0; j < map->vorder; ++j) {
                for (GLint i = 0; i < map->uorder; ++i) {
                    for (GLint c = 0; c < map->components; ++c) {
                        *v++ = map->points[((size_t)j * (size_t)map->uorder + (size_t)i) * 4u + (size_t)c];
                    }
                }
            }
        } else {
            set_error(GL_INVALID_ENUM);
        }
    }
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
    GLfloat values[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int count = 1;
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glGetMapfv(target, query, values);
    if (glGetError() != GL_NO_ERROR) {
        return;
    }
    if (query == GL_DOMAIN) count = map2 ? 4 : 2;
    else if (query == GL_ORDER) count = map2 ? 2 : 1;
    else if (query == GL_COEFF) count = map2 ? eval_maps2[index].uorder * eval_maps2[index].vorder * eval_maps2[index].components
                                             : eval_maps1[index].order * eval_maps1[index].components;
    for (int i = 0; i < count; ++i) {
        v[i] = (GLint)values[i];
    }
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
    GLfloat values[NXGL_MAX_EVAL_ORDER * NXGL_MAX_EVAL_ORDER * 4];
    int count = 1;
    bool map2 = false;
    int index = eval_map_index(target, &map2);

    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (index < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glGetMapfv(target, query, values);
    if (glGetError() != GL_NO_ERROR) {
        return;
    }
    if (query == GL_DOMAIN) count = map2 ? 4 : 2;
    else if (query == GL_ORDER) count = map2 ? 2 : 1;
    else if (query == GL_COEFF) count = map2 ? eval_maps2[index].uorder * eval_maps2[index].vorder * eval_maps2[index].components
                                             : eval_maps1[index].order * eval_maps1[index].components;
    for (int i = 0; i < count; ++i) {
        v[i] = (GLdouble)values[i];
    }
}

void glEvalCoord1f(GLfloat u)
{
    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    apply_eval_map1(u);
}

void glEvalCoord1d(GLdouble u)
{
    glEvalCoord1f((GLfloat)u);
}

void glEvalCoord2f(GLfloat u, GLfloat v)
{
    if (begin_mode == NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    apply_eval_map2(u, v);
}

void glEvalCoord2d(GLdouble u, GLdouble v)
{
    glEvalCoord2f((GLfloat)u, (GLfloat)v);
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (un <= 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map_grid1_n = un;
    map_grid1_u1 = u1;
    map_grid1_u2 = u2;
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    glMapGrid1f(un, (GLfloat)u1, (GLfloat)u2);
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (un <= 0 || vn <= 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    map_grid2_un = un;
    map_grid2_vn = vn;
    map_grid2_u1 = u1;
    map_grid2_u2 = u2;
    map_grid2_v1 = v1;
    map_grid2_v2 = v2;
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    glMapGrid2f(un, (GLfloat)u1, (GLfloat)u2, vn, (GLfloat)v1, (GLfloat)v2);
}

void glEvalPoint1(GLint i)
{
    GLfloat u = map_grid1_n != 0 ? map_grid1_u1 + ((map_grid1_u2 - map_grid1_u1) * (GLfloat)i / (GLfloat)map_grid1_n) : map_grid1_u1;
    glEvalCoord1f(u);
}

void glEvalPoint2(GLint i, GLint j)
{
    GLfloat u = map_grid2_un != 0 ? map_grid2_u1 + ((map_grid2_u2 - map_grid2_u1) * (GLfloat)i / (GLfloat)map_grid2_un) : map_grid2_u1;
    GLfloat v = map_grid2_vn != 0 ? map_grid2_v1 + ((map_grid2_v2 - map_grid2_v1) * (GLfloat)j / (GLfloat)map_grid2_vn) : map_grid2_v1;
    glEvalCoord2f(u, v);
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    if (mode == GL_POINT) {
        glBegin(GL_POINTS);
        for (GLint i = i1; i <= i2; ++i) glEvalPoint1(i);
        glEnd();
    } else if (mode == GL_LINE) {
        glBegin(GL_LINE_STRIP);
        for (GLint i = i1; i <= i2; ++i) glEvalPoint1(i);
        glEnd();
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    if (mode == GL_POINT) {
        glBegin(GL_POINTS);
        for (GLint j = j1; j <= j2; ++j) {
            for (GLint i = i1; i <= i2; ++i) glEvalPoint2(i, j);
        }
        glEnd();
    } else if (mode == GL_LINE) {
        for (GLint j = j1; j <= j2; ++j) {
            glBegin(GL_LINE_STRIP);
            for (GLint i = i1; i <= i2; ++i) glEvalPoint2(i, j);
            glEnd();
        }
        for (GLint i = i1; i <= i2; ++i) {
            glBegin(GL_LINE_STRIP);
            for (GLint j = j1; j <= j2; ++j) glEvalPoint2(i, j);
            glEnd();
        }
    } else if (mode == GL_FILL) {
        for (GLint j = j1; j < j2; ++j) {
            glBegin(GL_QUAD_STRIP);
            for (GLint i = i1; i <= i2; ++i) {
                glEvalPoint2(i, j);
                glEvalPoint2(i, j + 1);
            }
            glEnd();
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    int comps = 1;
    int elem_size = 1;
    uint8_t *dst = (uint8_t *)pixels;
    size_t pixel_bytes;
    size_t out_stride;
    bool color_read = format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA;
    bool depth_read = format == GL_DEPTH_COMPONENT;
    bool stencil_read = format == GL_STENCIL_INDEX;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!color_read && !depth_read && !stencil_read) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (color_read && type != GL_UNSIGNED_BYTE) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (depth_read && type != GL_FLOAT && type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (stencil_read && type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL && width > 0 && height > 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_read && shadow_color_buffer == NULL) ||
        (depth_read && shadow_depth_buffer == NULL) ||
        (stencil_read && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    nxgl_backend_flush();

    if (color_read) {
        comps = source_components(format);
    }
    elem_size = color_read ? 1 : type_size(type);
    pixel_bytes = (size_t)comps * (size_t)elem_size;
    out_stride = pixel_store_row_stride(width, pack_row_length, pack_skip_pixels, pixel_bytes, pack_alignment);
    dst += (size_t)pack_skip_rows * out_stride + (size_t)pack_skip_pixels * pixel_bytes;
    for (GLsizei row = 0; row < height; ++row) {
        int src_y = shadow_height - 1 - (y + row);
        uint8_t *dst_row = dst + (size_t)row * out_stride;
        if (color_read) {
            const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                uint32_t pixel = src_row[col];
                uint8_t a = (uint8_t)((pixel >> 24) & 0xff);
                uint8_t r = (uint8_t)((pixel >> 16) & 0xff);
                uint8_t g = (uint8_t)((pixel >> 8) & 0xff);
                uint8_t b = (uint8_t)(pixel & 0xff);
                uint8_t *out = dst_row + (size_t)col * (size_t)comps;

                if (format == GL_RGBA) {
                    out[0] = r;
                    out[1] = g;
                    out[2] = b;
                    out[3] = a;
                } else if (format == GL_RGB) {
                    out[0] = r;
                    out[1] = g;
                    out[2] = b;
                } else if (format == GL_BGRA) {
                    out[0] = b;
                    out[1] = g;
                    out[2] = r;
                    out[3] = a;
                } else {
                    out[0] = b;
                    out[1] = g;
                    out[2] = r;
                }
            }
        } else if (depth_read) {
            const float *src_row = shadow_depth_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                GLfloat depth = clamp01(src_row[col]);
                uint8_t *out = dst_row + (size_t)col * (size_t)elem_size;

                if (type == GL_FLOAT) {
                    ((GLfloat *)out)[0] = depth;
                } else if (type == GL_UNSIGNED_BYTE) {
                    out[0] = (uint8_t)(depth * 255.0f + 0.5f);
                } else if (type == GL_UNSIGNED_SHORT) {
                    ((GLushort *)out)[0] = (GLushort)(depth * 65535.0f + 0.5f);
                } else {
                    ((GLuint *)out)[0] = (GLuint)(depth * 4294967295.0f + 0.5f);
                }
            }
        } else {
            const uint8_t *src_row = shadow_stencil_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            for (GLsizei col = 0; col < width; ++col) {
                GLuint stencil = src_row[col];
                uint8_t *out = dst_row + (size_t)col * (size_t)elem_size;

                if (type == GL_UNSIGNED_BYTE) {
                    out[0] = (uint8_t)stencil;
                } else if (type == GL_UNSIGNED_SHORT) {
                    ((GLushort *)out)[0] = (GLushort)stencil;
                } else {
                    ((GLuint *)out)[0] = stencil;
                }
            }
        }
    }
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
    glRasterPos3f(x, y, 0.0f);
}

void glRasterPos2fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2f(v[0], v[1]);
}

void glRasterPos2d(GLdouble x, GLdouble y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2d(v[0], v[1]);
}

void glRasterPos2i(GLint x, GLint y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2i(v[0], v[1]);
}

void glRasterPos2s(GLshort x, GLshort y)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, 0.0f);
}

void glRasterPos2sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos2s(v[0], v[1]);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    ListCommand command = { LIST_CMD_RASTER_POS };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    command.f[3] = 1.0f;
    record_command(command);
    if (compile_only()) {
        return;
    }
    set_raster_position_from_vertex(x, y, z);
}

void glRasterPos3fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3f(v[0], v[1], v[2]);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3d(v[0], v[1], v[2]);
}

void glRasterPos3i(GLint x, GLint y, GLint z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3i(v[0], v[1], v[2]);
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glRasterPos3sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos3s(v[0], v[1], v[2]);
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ListCommand command = { LIST_CMD_RASTER_POS };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.f[0] = x;
    command.f[1] = y;
    command.f[2] = z;
    command.f[3] = w;
    record_command(command);
    if (compile_only()) {
        return;
    }
    set_raster_position_from_vertex4(x, y, z, w);
}

void glRasterPos4fv(const GLfloat *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4f(v[0], v[1], v[2], v[3]);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4dv(const GLdouble *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4d(v[0], v[1], v[2], v[3]);
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4iv(const GLint *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4i(v[0], v[1], v[2], v[3]);
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    glRasterPos4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

void glRasterPos4sv(const GLshort *v)
{
    if (v == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glRasterPos4s(v[0], v[1], v[2], v[3]);
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    ListCommand command = { LIST_CMD_BITMAP };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    size_t src_stride;
    size_t data_size;
    uint32_t color;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (bitmap == NULL && width > 0 && height > 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (shadow_color_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)width;
    command.f[1] = (GLfloat)height;
    command.f[2] = xorig;
    command.f[3] = yorig;
    command.extra[0] = xmove;
    command.extra[1] = ymove;
    command.i[0] = unpack_alignment;
    command.i[1] = unpack_row_length;
    command.i[2] = unpack_skip_rows;
    command.i[3] = unpack_skip_pixels;
    command.u = (unpack_swap_bytes ? 1u : 0u) | (unpack_lsb_first ? 2u : 0u);
    data_size = bitmap_source_data_size(width, height);
    if (data_size != 0 && !attach_command_data(&command, bitmap, data_size)) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (!current_raster_position_valid) {
        return;
    }

    if (render_mode == GL_FEEDBACK && width > 0 && height > 0) {
        record_feedback_pixel_token(GL_BITMAP_TOKEN);
    } else if (width > 0 && height > 0) {
        src_stride = bitmap_unpack_row_stride(width);
        color = color_to_u32(current_color);
        for (GLsizei row = 0; row < height; ++row) {
            const GLubyte *src_row = bitmap + (size_t)unpack_skip_rows * src_stride + (size_t)row * src_stride;
            GLint dst_y = raster_y + row - (GLint)yorig;
            for (GLsizei col = 0; col < width; ++col) {
                GLsizei bit_index = unpack_skip_pixels + col;
                GLubyte mask = unpack_lsb_first ? (GLubyte)(1u << (bit_index & 7)) : (GLubyte)(0x80u >> (bit_index & 7));
                if ((src_row[bit_index >> 3] & mask) != 0) {
                    write_shadow_pixel(raster_x + col - (GLint)xorig, dst_y, color);
                }
            }
        }
    }

    current_raster_position[0] += xmove;
    current_raster_position[1] += ymove;
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ListCommand command = { LIST_CMD_DRAW_PIXELS };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    const uint8_t *src = (const uint8_t *)pixels;
    uint8_t *rgba = NULL;
    size_t data_size;
    int zoom_x = pixel_zoom_replicate(pixel_zoom_x);
    int zoom_y = pixel_zoom_replicate(pixel_zoom_y);
    bool color_draw = format == GL_RGB || format == GL_RGBA || format == GL_BGR || format == GL_BGRA || format == GL_COLOR_INDEX;
    bool depth_draw = format == GL_DEPTH_COMPONENT;
    bool stencil_draw = format == GL_STENCIL_INDEX;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!color_draw && !depth_draw && !stencil_draw) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if ((color_draw && !valid_pixel_type(type)) ||
        (depth_draw && !valid_depth_pixel_type(type)) ||
        (stencil_draw && !valid_stencil_pixel_type(type))) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_draw && shadow_color_buffer == NULL) ||
        (depth_draw && shadow_depth_buffer == NULL) ||
        (stencil_draw && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)width;
    command.f[1] = (GLfloat)height;
    command.a = format;
    command.b = type;
    command.i[0] = unpack_alignment;
    command.i[1] = unpack_row_length;
    command.i[2] = unpack_skip_rows;
    command.i[3] = unpack_skip_pixels;
    command.u = (unpack_swap_bytes ? 1u : 0u) | (unpack_lsb_first ? 2u : 0u);
    data_size = pixel_source_data_size(width, height, format, type);
    if (data_size != 0 && !attach_command_data(&command, pixels, data_size)) {
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (!current_raster_position_valid) {
        return;
    }
    if (pixel_zoom_x == 0.0f || pixel_zoom_y == 0.0f) {
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_pixel_token(GL_DRAW_PIXEL_TOKEN);
        return;
    }

    if (color_draw) {
        rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
        if (rgba == NULL) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (!convert_to_rgba(rgba, src, width, height, format, type)) {
            free(rgba);
            set_error(GL_INVALID_ENUM);
            return;
        }
    }

    for (GLsizei row = 0; row < height; ++row) {
        GLint base_y = pixel_zoom_y > 0.0f ? raster_y + row * zoom_y : raster_y - (row + 1) * zoom_y;
        size_t pixel_bytes = color_draw ? 0u : (size_t)type_size(type);
        size_t src_stride = color_draw ? 0u : pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
        const uint8_t *src_row = color_draw ? NULL : src + (size_t)unpack_skip_rows * src_stride + (size_t)unpack_skip_pixels * pixel_bytes + (size_t)row * src_stride;
        for (GLsizei col = 0; col < width; ++col) {
            GLint base_x = pixel_zoom_x > 0.0f ? raster_x + col * zoom_x : raster_x - (col + 1) * zoom_x;
            const uint8_t *src_pixel = color_draw ? rgba + ((size_t)row * (size_t)width + (size_t)col) * 4u
                                                  : src_row + (size_t)col * pixel_bytes;
            NxglBackendColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
            GLfloat depth = 0.0f;
            uint8_t stencil = 0;

            if (color_draw) {
                color = apply_pixel_transfer_rgba_color(src_pixel);
            } else if (depth_draw) {
                depth = unpack_depth_pixel(src_pixel, type);
            } else {
                stencil = apply_pixel_transfer_stencil(unpack_stencil_pixel(src_pixel, type));
            }
            for (int yy = 0; yy < zoom_y; ++yy) {
                for (int xx = 0; xx < zoom_x; ++xx) {
                    if (color_draw) {
                        write_shadow_color_fragment(base_x + xx, base_y + yy, color, current_raster_position[2]);
                    } else if (depth_draw) {
                        write_shadow_depth_pixel(base_x + xx, base_y + yy, depth);
                    } else {
                        write_shadow_stencil_pixel(base_x + xx, base_y + yy, stencil);
                    }
                }
            }
        }
    }
    free(rgba);
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    ListCommand command = { LIST_CMD_COPY_PIXELS };
    GLint raster_x = (GLint)current_raster_position[0];
    GLint raster_y = (GLint)current_raster_position[1];
    uint32_t *copy;
    float *depth_copy;
    uint8_t *stencil_copy;
    int zoom_x = pixel_zoom_replicate(pixel_zoom_x);
    int zoom_y = pixel_zoom_replicate(pixel_zoom_y);
    bool color_copy = type == GL_COLOR;
    bool depth_copy_mode = type == GL_DEPTH;
    bool stencil_copy_mode = type == GL_STENCIL;

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (!color_copy && !depth_copy_mode && !stencil_copy_mode) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width < 0 || height < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!shadow_readback_enabled) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((color_copy && shadow_color_buffer == NULL) ||
        (depth_copy_mode && shadow_depth_buffer == NULL) ||
        (stencil_copy_mode && shadow_stencil_buffer == NULL)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    command.f[0] = (GLfloat)x;
    command.f[1] = (GLfloat)y;
    command.f[2] = (GLfloat)width;
    command.f[3] = (GLfloat)height;
    command.a = type;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (!current_raster_position_valid || width == 0 || height == 0) {
        return;
    }
    if (pixel_zoom_x == 0.0f || pixel_zoom_y == 0.0f) {
        return;
    }
    if (x < 0 || y < 0 || x + width > shadow_width || y + height > shadow_height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        record_feedback_pixel_token(GL_COPY_PIXEL_TOKEN);
        return;
    }

    if (color_copy) {
        nxgl_backend_flush();
    }

    copy = NULL;
    depth_copy = NULL;
    stencil_copy = NULL;
    if (color_copy) {
        copy = (uint32_t *)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    } else if (depth_copy_mode) {
        depth_copy = (float *)malloc((size_t)width * (size_t)height * sizeof(float));
    } else {
        stencil_copy = (uint8_t *)malloc((size_t)width * (size_t)height);
    }
    if ((color_copy && copy == NULL) ||
        (depth_copy_mode && depth_copy == NULL) ||
        (stencil_copy_mode && stencil_copy == NULL)) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei row = 0; row < height; ++row) {
        GLint src_y = shadow_height - 1 - (y + row);
        if (color_copy) {
            const uint32_t *src_row = shadow_color_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(copy + (size_t)row * (size_t)width, src_row, (size_t)width * sizeof(uint32_t));
        } else if (depth_copy_mode) {
            const float *src_row = shadow_depth_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(depth_copy + (size_t)row * (size_t)width, src_row, (size_t)width * sizeof(float));
        } else {
            const uint8_t *src_row = shadow_stencil_buffer + (size_t)src_y * (size_t)shadow_width + (size_t)x;
            memcpy(stencil_copy + (size_t)row * (size_t)width, src_row, (size_t)width);
        }
    }
    for (GLsizei row = 0; row < height; ++row) {
        GLint base_y = pixel_zoom_y > 0.0f ? raster_y + row * zoom_y : raster_y - (row + 1) * zoom_y;
        for (GLsizei col = 0; col < width; ++col) {
            GLint base_x = pixel_zoom_x > 0.0f ? raster_x + col * zoom_x : raster_x - (col + 1) * zoom_x;
            size_t src_index = (size_t)row * (size_t)width + (size_t)col;
            uint32_t color = color_copy ? apply_pixel_transfer_u32(copy[src_index]) : 0;
            GLfloat depth = depth_copy_mode ? depth_copy[src_index] : 0.0f;
            uint8_t stencil = stencil_copy_mode ? apply_pixel_transfer_stencil(stencil_copy[src_index]) : 0;
            for (int yy = 0; yy < zoom_y; ++yy) {
                for (int xx = 0; xx < zoom_x; ++xx) {
                    if (color_copy) {
                        write_shadow_pixel(base_x + xx, base_y + yy, color);
                    } else if (depth_copy_mode) {
                        write_shadow_depth_pixel(base_x + xx, base_y + yy, depth);
                    } else {
                        write_shadow_stencil_pixel(base_x + xx, base_y + yy, stencil);
                    }
                }
            }
        }
    }
    free(copy);
    free(depth_copy);
    free(stencil_copy);
}

void glDrawBuffer(GLenum mode)
{
    ListCommand command = { LIST_CMD_DRAW_BUFFER };

    if (!valid_draw_buffer(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    draw_buffer_mode = mode;
}

void glReadBuffer(GLenum mode)
{
    ListCommand command = { LIST_CMD_READ_BUFFER };

    if (!valid_read_buffer(mode)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = mode;
    record_command(command);
    if (compile_only()) {
        return;
    }
    read_buffer_mode = mode;
}

void glSelectBuffer(GLsizei size, GLuint *buffer)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode == GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (size < 0 || buffer == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    selection_buffer_size = size;
    selection_buffer = buffer;
    selection_write_count = 0;
    selection_hits = 0;
    selection_overflow = false;
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode == GL_FEEDBACK) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (size < 0 || buffer == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_2D &&
        type != GL_3D &&
        type != GL_3D_COLOR &&
        type != GL_3D_COLOR_TEXTURE &&
        type != GL_4D_COLOR_TEXTURE) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    feedback_buffer_size = size;
    feedback_buffer = buffer;
    feedback_write_count = 0;
    feedback_overflow = false;
    feedback_type = type;
}

GLint glRenderMode(GLenum mode)
{
    GLint result = 0;

    if (mode != GL_RENDER && mode != GL_SELECT && mode != GL_FEEDBACK) {
        set_error(GL_INVALID_ENUM);
        return 0;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }
    if (mode == GL_SELECT && selection_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }
    if (mode == GL_FEEDBACK && feedback_buffer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return 0;
    }

    if (render_mode == GL_SELECT) {
        result = selection_overflow ? -1 : selection_hits;
    } else if (render_mode == GL_FEEDBACK) {
        result = feedback_overflow ? -1 : feedback_write_count;
    }

    render_mode = mode;
    if (mode == GL_SELECT) {
        selection_write_count = 0;
        selection_hits = 0;
        selection_overflow = false;
    } else if (mode == GL_FEEDBACK) {
        feedback_write_count = 0;
        feedback_overflow = false;
    }
    return result;
}

void glPassThrough(GLfloat token)
{
    ListCommand command = { LIST_CMD_PASS_THROUGH };
    if (reject_inside_begin()) {
        return;
    }
    command.f[0] = token;
    record_command(command);
    if (compile_only()) {
        return;
    }
    record_feedback_pass_through(token);
}

void glInitNames(void)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    name_stack_depth = 0;
}

void glPushName(GLuint name)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth >= (GLint)(sizeof(name_stack) / sizeof(name_stack[0]))) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    name_stack[name_stack_depth++] = name;
}

void glPopName(void)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    --name_stack_depth;
}

void glLoadName(GLuint name)
{
    if (reject_inside_begin()) {
        return;
    }
    if (render_mode != GL_SELECT) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (name_stack_depth <= 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    name_stack[name_stack_depth - 1] = name;
}

void glEnable(uint32_t cap)
{
    int unit = active_texture_index();
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    ListCommand command = { LIST_CMD_ENABLE };
    command.a = cap;

    if (reject_inside_begin()) {
        return;
    }
    if (eval_index >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (map2) {
            eval_maps2[eval_index].enabled = GL_TRUE;
        } else {
            eval_maps1[eval_index].enabled = GL_TRUE;
        }
    } else if (texgen_coord >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texgen_state[unit][texgen_coord].enabled = GL_TRUE;
            texgen_any_enabled = true;
        }
    } else if (clip >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        clip_planes[clip].enabled = GL_TRUE;
    } else if (light >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lights[light].enabled = true;
    } else if (cap == GL_BLEND) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        blend_enabled = true;
        sync_native_state();
    } else if (cap == GL_LINE_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        line_stipple_enabled = true;
    } else if (cap == GL_POLYGON_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        polygon_stipple_enabled = true;
    } else if (cap == GL_DEPTH_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        depth_test_enabled = true;
        sync_native_state();
    } else if (cap == GL_CULL_FACE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        cull_enabled = true;
        sync_native_state();
    } else if (cap == GL_SCISSOR_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        scissor_test_enabled = true;
        sync_native_state();
    } else if (cap == GL_STENCIL_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        stencil_test_enabled = true;
    } else if (cap == GL_ALPHA_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        alpha_test_enabled = true;
    } else if (cap == GL_COLOR_LOGIC_OP || cap == GL_INDEX_LOGIC_OP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_logic_op_enabled = true;
    } else if (cap == GL_POLYGON_OFFSET_POINT ||
               cap == GL_POLYGON_OFFSET_LINE ||
               cap == GL_POLYGON_OFFSET_FILL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_POLYGON_OFFSET_POINT) {
            polygon_offset_point_enabled = true;
        } else if (cap == GL_POLYGON_OFFSET_LINE) {
            polygon_offset_line_enabled = true;
        } else {
            polygon_offset_fill_enabled = true;
        }
    } else if (cap == GL_TEXTURE_1D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_1d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_2D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_2d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_3D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_3d_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_CUBE_MAP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_cube_map_enabled[unit] = true;
        }
        sync_native_state();
    } else if (cap == GL_LIGHTING) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lighting_enabled = true;
    } else if (cap == GL_FOG) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        fog_enabled = true;
    } else if (cap == GL_COLOR_MATERIAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_material_enabled = true;
        apply_color_material(current_color);
    } else if (cap == GL_NORMALIZE || cap == GL_RESCALE_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_NORMALIZE) {
            normalize_enabled = true;
        } else {
            rescale_normal_enabled = true;
        }
    } else if (cap == GL_AUTO_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        auto_normal_enabled = GL_TRUE;
    } else if (cap == GL_MULTISAMPLE ||
               cap == GL_SAMPLE_ALPHA_TO_COVERAGE ||
               cap == GL_SAMPLE_ALPHA_TO_ONE ||
               cap == GL_SAMPLE_COVERAGE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_MULTISAMPLE) {
            multisample_enabled = true;
        } else if (cap == GL_SAMPLE_ALPHA_TO_COVERAGE) {
            sample_alpha_to_coverage_enabled = true;
        } else if (cap == GL_SAMPLE_ALPHA_TO_ONE) {
            sample_alpha_to_one_enabled = true;
        } else {
            sample_coverage_enabled = true;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glDisable(uint32_t cap)
{
    int unit = active_texture_index();
    int light = light_index(cap);
    bool map2 = false;
    int eval_index = eval_map_index(cap, &map2);
    int texgen_coord = texgen_index_from_cap(cap);
    int clip = clip_plane_index(cap);
    ListCommand command = { LIST_CMD_DISABLE };
    command.a = cap;

    if (reject_inside_begin()) {
        return;
    }
    if (eval_index >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (map2) {
            eval_maps2[eval_index].enabled = GL_FALSE;
        } else {
            eval_maps1[eval_index].enabled = GL_FALSE;
        }
    } else if (texgen_coord >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texgen_state[unit][texgen_coord].enabled = GL_FALSE;
            update_texgen_enabled_cache();
        }
    } else if (clip >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        clip_planes[clip].enabled = GL_FALSE;
    } else if (light >= 0) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lights[light].enabled = false;
    } else if (cap == GL_BLEND) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        blend_enabled = false;
        sync_native_state();
    } else if (cap == GL_LINE_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        line_stipple_enabled = false;
    } else if (cap == GL_POLYGON_STIPPLE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        polygon_stipple_enabled = false;
    } else if (cap == GL_DEPTH_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        depth_test_enabled = false;
        sync_native_state();
    } else if (cap == GL_CULL_FACE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        cull_enabled = false;
        sync_native_state();
    } else if (cap == GL_SCISSOR_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        scissor_test_enabled = false;
        sync_native_state();
    } else if (cap == GL_STENCIL_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        stencil_test_enabled = false;
    } else if (cap == GL_ALPHA_TEST) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        alpha_test_enabled = false;
    } else if (cap == GL_COLOR_LOGIC_OP || cap == GL_INDEX_LOGIC_OP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_logic_op_enabled = false;
    } else if (cap == GL_POLYGON_OFFSET_POINT ||
               cap == GL_POLYGON_OFFSET_LINE ||
               cap == GL_POLYGON_OFFSET_FILL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_POLYGON_OFFSET_POINT) {
            polygon_offset_point_enabled = false;
        } else if (cap == GL_POLYGON_OFFSET_LINE) {
            polygon_offset_line_enabled = false;
        } else {
            polygon_offset_fill_enabled = false;
        }
    } else if (cap == GL_TEXTURE_1D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_1d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_2D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_2d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_3D) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_3d_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_TEXTURE_CUBE_MAP) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (unit >= 0) {
            texture_cube_map_enabled[unit] = false;
        }
        sync_native_state();
    } else if (cap == GL_LIGHTING) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        lighting_enabled = false;
    } else if (cap == GL_FOG) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        fog_enabled = false;
    } else if (cap == GL_COLOR_MATERIAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        color_material_enabled = false;
    } else if (cap == GL_NORMALIZE || cap == GL_RESCALE_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_NORMALIZE) {
            normalize_enabled = false;
        } else {
            rescale_normal_enabled = false;
        }
    } else if (cap == GL_AUTO_NORMAL) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        auto_normal_enabled = GL_FALSE;
    } else if (cap == GL_MULTISAMPLE ||
               cap == GL_SAMPLE_ALPHA_TO_COVERAGE ||
               cap == GL_SAMPLE_ALPHA_TO_ONE ||
               cap == GL_SAMPLE_COVERAGE) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        if (cap == GL_MULTISAMPLE) {
            multisample_enabled = false;
        } else if (cap == GL_SAMPLE_ALPHA_TO_COVERAGE) {
            sample_alpha_to_coverage_enabled = false;
        } else if (cap == GL_SAMPLE_ALPHA_TO_ONE) {
            sample_alpha_to_one_enabled = false;
        } else {
            sample_coverage_enabled = false;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glSampleCoverage(GLclampf value, GLboolean invert)
{
    if (reject_inside_begin()) {
        return;
    }
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    sample_coverage_value = value;
    sample_coverage_invert = invert ? GL_TRUE : GL_FALSE;
}

void glPushAttrib(GLbitfield mask)
{
    ListCommand command = { LIST_CMD_PUSH_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if ((mask & ~GL_ALL_ATTRIB_BITS) != 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (attrib_stack_top >= NXGL_ATTRIB_STACK_MAX) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    capture_attrib_snapshot(&attrib_stack[attrib_stack_top++], mask);
}

void glPopAttrib(void)
{
    ListCommand command = { LIST_CMD_POP_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (attrib_stack_top <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    restore_attrib_snapshot(&attrib_stack[--attrib_stack_top]);
}

void glPushClientAttrib(GLbitfield mask)
{
    ListCommand command = { LIST_CMD_PUSH_CLIENT_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    mask &= GL_CLIENT_ALL_ATTRIB_BITS;
    command.bits = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (client_attrib_stack_top >= NXGL_CLIENT_ATTRIB_STACK_MAX) {
        set_error(GL_STACK_OVERFLOW);
        return;
    }
    capture_client_attrib_snapshot(&client_attrib_stack[client_attrib_stack_top++], mask);
}

void glPopClientAttrib(void)
{
    ListCommand command = { LIST_CMD_POP_CLIENT_ATTRIB };

    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    record_command(command);
    if (compile_only()) {
        return;
    }
    if (client_attrib_stack_top <= 0) {
        set_error(GL_STACK_UNDERFLOW);
        return;
    }
    restore_client_attrib_snapshot(&client_attrib_stack[--client_attrib_stack_top]);
}

void glDepthFunc(GLenum func)
{
    ListCommand command = { LIST_CMD_DEPTH_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_depth_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    record_command(command);
    if (compile_only()) {
        return;
    }
    depth_func = func;
}

void glDepthMask(GLboolean flag)
{
    ListCommand command = { LIST_CMD_DEPTH_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.u = flag;
    record_command(command);
    if (compile_only()) {
        return;
    }

    depth_write_enabled = flag != GL_FALSE;
    sync_native_state();
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    ListCommand command = { LIST_CMD_STENCIL_FUNC };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_func(func)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = func;
    command.f[0] = (GLfloat)ref;
    command.u = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_func = func;
    stencil_ref = ref;
    stencil_value_mask = mask;
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    ListCommand command = { LIST_CMD_STENCIL_OP };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_stencil_op(fail) || !valid_stencil_op(zfail) || !valid_stencil_op(zpass)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    command.a = fail;
    command.b = zfail;
    command.u = zpass;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_fail = fail;
    stencil_zfail = zfail;
    stencil_zpass = zpass;
}

void glStencilMask(GLuint mask)
{
    ListCommand command = { LIST_CMD_STENCIL_MASK };
    if (reject_inside_begin()) {
        return;
    }
    command.u = mask;
    record_command(command);
    if (compile_only()) {
        return;
    }
    stencil_write_mask = mask;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (reject_inside_begin()) {
        return;
    }
    if (!valid_blend_src_factor(sfactor) || !valid_blend_dst_factor(dfactor)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_BLEND_FUNC };
    command.a = sfactor;
    command.b = dfactor;
    record_command(command);
    if (compile_only()) {
        return;
    }

    blend_sfactor = sfactor;
    blend_dfactor = dfactor;
    sync_native_state();
}

void glActiveTexture(GLenum texture)
{
    if (texture_unit_index(texture) < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    ListCommand command = { LIST_CMD_ACTIVE_TEXTURE };
    command.a = texture;
    record_command(command);
    if (compile_only()) {
        return;
    }

    active_texture = texture;
    sync_native_state();
}

void glClientActiveTexture(GLenum texture)
{
    if (texture_unit_index(texture) < 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    client_active_texture = texture;
}

void glEnableClientState(GLenum array)
{
    int unit = client_texture_index();

    if (array == GL_VERTEX_ARRAY) {
        vertex_array.enabled = true;
    } else if (array == GL_COLOR_ARRAY) {
        color_array.enabled = true;
    } else if (array == GL_NORMAL_ARRAY) {
        normal_array.enabled = true;
    } else if (array == GL_TEXTURE_COORD_ARRAY) {
        if (unit >= 0) {
            texcoord_array[unit].enabled = true;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glDisableClientState(GLenum array)
{
    int unit = client_texture_index();

    if (array == GL_VERTEX_ARRAY) {
        vertex_array.enabled = false;
    } else if (array == GL_COLOR_ARRAY) {
        color_array.enabled = false;
    } else if (array == GL_NORMAL_ARRAY) {
        normal_array.enabled = false;
    } else if (array == GL_TEXTURE_COORD_ARRAY) {
        if (unit >= 0) {
            texcoord_array[unit].enabled = false;
        }
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (size < 2 || size > 4 || !is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    vertex_array.size = size;
    vertex_array.type = type;
    vertex_array.stride = stride;
    vertex_array.pointer = (const uint8_t *)pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (size < 3 || size > 4 || !is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    color_array.size = size;
    color_array.type = type;
    color_array.stride = stride;
    color_array.pointer = (const uint8_t *)pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    int unit = client_texture_index();

    if (size < 1 || size > 4 || !is_float_or_integer_type(type) || stride < 0 || unit < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    texcoord_array[unit].size = size;
    texcoord_array[unit].type = type;
    texcoord_array[unit].stride = stride;
    texcoord_array[unit].pointer = (const uint8_t *)pointer;
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (!is_float_or_integer_type(type) || stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    normal_array.size = 3;
    normal_array.type = type;
    normal_array.stride = stride;
    normal_array.pointer = (const uint8_t *)pointer;
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    const uint8_t *base = (const uint8_t *)pointer;
    GLsizei natural_stride = 0;
    GLint vertex_size = 0;
    GLint tex_size = 0;
    bool has_color = false;
    bool has_normal = false;
    bool has_texcoord = false;
    GLenum color_type = GL_FLOAT;
    GLint color_size = 4;
    size_t color_offset = 0;
    size_t normal_offset = 0;
    size_t tex_offset = 0;
    size_t vertex_offset = 0;
    int unit = client_texture_index();

    if (stride < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (unit < 0) {
        unit = 0;
    }

    switch (format) {
    case GL_V2F:
        vertex_size = 2; vertex_offset = 0; natural_stride = 2 * 4;
        break;
    case GL_V3F:
        vertex_size = 3; vertex_offset = 0; natural_stride = 3 * 4;
        break;
    case GL_C4UB_V2F:
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 0;
        vertex_size = 2; vertex_offset = 4; natural_stride = 4 + 2 * 4;
        break;
    case GL_C4UB_V3F:
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 0;
        vertex_size = 3; vertex_offset = 4; natural_stride = 4 + 3 * 4;
        break;
    case GL_C3F_V3F:
        has_color = true; color_type = GL_FLOAT; color_size = 3; color_offset = 0;
        vertex_size = 3; vertex_offset = 3 * 4; natural_stride = 6 * 4;
        break;
    case GL_N3F_V3F:
        has_normal = true; normal_offset = 0;
        vertex_size = 3; vertex_offset = 3 * 4; natural_stride = 6 * 4;
        break;
    case GL_C4F_N3F_V3F:
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 0;
        has_normal = true; normal_offset = 4 * 4;
        vertex_size = 3; vertex_offset = 7 * 4; natural_stride = 10 * 4;
        break;
    case GL_T2F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        vertex_size = 3; vertex_offset = 2 * 4; natural_stride = 5 * 4;
        break;
    case GL_T4F_V4F:
        has_texcoord = true; tex_size = 4; tex_offset = 0;
        vertex_size = 4; vertex_offset = 4 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_C4UB_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_UNSIGNED_BYTE; color_size = 4; color_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 2 * 4 + 4; natural_stride = 2 * 4 + 4 + 3 * 4;
        break;
    case GL_T2F_C3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 3; color_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 5 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_N3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_normal = true; normal_offset = 2 * 4;
        vertex_size = 3; vertex_offset = 5 * 4; natural_stride = 8 * 4;
        break;
    case GL_T2F_C4F_N3F_V3F:
        has_texcoord = true; tex_size = 2; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 2 * 4;
        has_normal = true; normal_offset = 6 * 4;
        vertex_size = 3; vertex_offset = 9 * 4; natural_stride = 12 * 4;
        break;
    case GL_T4F_C4F_N3F_V4F:
        has_texcoord = true; tex_size = 4; tex_offset = 0;
        has_color = true; color_type = GL_FLOAT; color_size = 4; color_offset = 4 * 4;
        has_normal = true; normal_offset = 8 * 4;
        vertex_size = 4; vertex_offset = 11 * 4; natural_stride = 15 * 4;
        break;
    default:
        set_error(GL_INVALID_ENUM);
        return;
    }

    if (stride == 0) {
        stride = natural_stride;
    }

    vertex_array.enabled = true;
    vertex_array.size = vertex_size;
    vertex_array.type = GL_FLOAT;
    vertex_array.stride = stride;
    vertex_array.pointer = base != NULL ? base + vertex_offset : NULL;

    color_array.enabled = has_color;
    color_array.size = color_size;
    color_array.type = color_type;
    color_array.stride = stride;
    color_array.pointer = has_color && base != NULL ? base + color_offset : NULL;

    normal_array.enabled = has_normal;
    normal_array.size = 3;
    normal_array.type = GL_FLOAT;
    normal_array.stride = stride;
    normal_array.pointer = has_normal && base != NULL ? base + normal_offset : NULL;

    texcoord_array[unit].enabled = has_texcoord;
    texcoord_array[unit].size = has_texcoord ? tex_size : 4;
    texcoord_array[unit].type = GL_FLOAT;
    texcoord_array[unit].stride = stride;
    texcoord_array[unit].pointer = has_texcoord && base != NULL ? base + tex_offset : NULL;
}

void glArrayElement(GLint index)
{
    NxglBackendVertex vertex;

    if (begin_mode == NXGL_NO_BEGIN_MODE || index < 0 || pending_count >= (int)(sizeof(pending) / sizeof(pending[0]))) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    vertex = read_array_vertex(index);
    pending[pending_count++] = vertex;
}

static bool validate_array_draw(GLenum mode, GLsizei count)
{
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return false;
    }
    if (!vertex_array.enabled || vertex_array.pointer == NULL) {
        set_error(GL_INVALID_OPERATION);
        return false;
    }
    if (mode != GL_POINTS &&
        mode != GL_LINES &&
        mode != GL_LINE_LOOP &&
        mode != GL_LINE_STRIP &&
        mode != GL_TRIANGLES &&
        mode != GL_TRIANGLE_STRIP &&
        mode != GL_TRIANGLE_FAN &&
        mode != GL_QUADS &&
        mode != GL_QUAD_STRIP &&
        mode != GL_POLYGON) {
        set_error(GL_INVALID_ENUM);
        return false;
    }
    return true;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLint *indices;

    if (!validate_array_draw(mode, count)) {
        return;
    }
    if (first < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (count == 0) {
        return;
    }

    indices = (GLint *)malloc((size_t)count * sizeof(GLint));
    if (indices == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        indices[i] = first + i;
    }
    emit_array_vertices(mode, indices, count);
    free(indices);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    GLint *expanded;

    if (!validate_array_draw(mode, count)) {
        return;
    }
    if (indices == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (count == 0) {
        return;
    }

    expanded = (GLint *)malloc((size_t)count * sizeof(GLint));
    if (expanded == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    for (GLsizei i = 0; i < count; ++i) {
        if (type == GL_UNSIGNED_BYTE) {
            expanded[i] = ((const GLubyte *)indices)[i];
        } else if (type == GL_UNSIGNED_SHORT) {
            expanded[i] = ((const GLushort *)indices)[i];
        } else {
            expanded[i] = (GLint)((const GLuint *)indices)[i];
        }
    }
    emit_array_vertices(mode, expanded, count);
    free(expanded);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
    if (end < start) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glDrawElements(mode, count, type, indices);
}

static void execute_list_command(const ListCommand *command)
{
    switch (command->type) {
    case LIST_CMD_CLEAR_COLOR:
        glClearColor(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_CLEAR_INDEX:
        glClearIndex(command->f[0]);
        break;
    case LIST_CMD_CLEAR_ACCUM:
        glClearAccum(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_CLEAR_DEPTH:
        glClearDepth(command->f[0]);
        break;
    case LIST_CMD_DEPTH_RANGE:
        glDepthRange(command->f[0], command->f[1]);
        break;
    case LIST_CMD_CLEAR_STENCIL:
        glClearStencil((GLint)command->u);
        break;
    case LIST_CMD_VIEWPORT:
        glViewport((GLint)command->f[0], (GLint)command->f[1], (GLsizei)command->f[2], (GLsizei)command->f[3]);
        break;
    case LIST_CMD_PASS_THROUGH:
        glPassThrough(command->f[0]);
        break;
    case LIST_CMD_CLEAR:
        glClear(command->bits);
        break;
    case LIST_CMD_ACCUM:
        glAccum(command->a, command->f[0]);
        break;
    case LIST_CMD_MATRIX_MODE:
        glMatrixMode(command->a);
        break;
    case LIST_CMD_LOAD_IDENTITY:
        glLoadIdentity();
        break;
    case LIST_CMD_LOAD_MATRIX:
        glLoadMatrixf(command->matrix);
        break;
    case LIST_CMD_MULT_MATRIX:
        glMultMatrixf(command->matrix);
        break;
    case LIST_CMD_PUSH_MATRIX:
        glPushMatrix();
        break;
    case LIST_CMD_POP_MATRIX:
        glPopMatrix();
        break;
    case LIST_CMD_TRANSLATE:
        glTranslatef(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_SCALE:
        glScalef(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_ROTATE:
        glRotatef(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_COLOR4:
        glColor4f(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_INDEX:
        glIndexf(command->f[0]);
        break;
    case LIST_CMD_NORMAL3:
        glNormal3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_TEXCOORD2:
        glTexCoord2f(command->f[0], command->f[1]);
        break;
    case LIST_CMD_TEXCOORD3:
        glTexCoord3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_BEGIN:
        glBegin(command->a);
        break;
    case LIST_CMD_VERTEX3:
        glVertex3f(command->f[0], command->f[1], command->f[2]);
        break;
    case LIST_CMD_END:
        glEnd();
        break;
    case LIST_CMD_POINT_SIZE:
        glPointSize(command->f[0]);
        break;
    case LIST_CMD_LINE_WIDTH:
        glLineWidth(command->f[0]);
        break;
    case LIST_CMD_LINE_STIPPLE:
        glLineStipple((GLint)command->f[0], (GLushort)command->u);
        break;
    case LIST_CMD_POLYGON_STIPPLE:
        glPolygonStipple(command->bytes);
        break;
    case LIST_CMD_POLYGON_MODE:
        glPolygonMode(command->a, command->b);
        break;
    case LIST_CMD_POLYGON_OFFSET:
        glPolygonOffset(command->f[0], command->f[1]);
        break;
    case LIST_CMD_ALPHA_FUNC:
        glAlphaFunc(command->a, command->f[0]);
        break;
    case LIST_CMD_COLOR_MASK:
        glColorMask(command->f[0] != 0.0f, command->f[1] != 0.0f, command->f[2] != 0.0f, command->f[3] != 0.0f);
        break;
    case LIST_CMD_LOGIC_OP:
        glLogicOp(command->a);
        break;
    case LIST_CMD_CULL_FACE:
        glCullFace(command->a);
        break;
    case LIST_CMD_FRONT_FACE:
        glFrontFace(command->a);
        break;
    case LIST_CMD_HINT:
        glHint(command->a, command->b);
        break;
    case LIST_CMD_EDGE_FLAG:
        glEdgeFlag((GLboolean)command->u);
        break;
    case LIST_CMD_PIXEL_ZOOM:
        glPixelZoom(command->f[0], command->f[1]);
        break;
    case LIST_CMD_PIXEL_TRANSFER:
        glPixelTransferf(command->a, command->f[0]);
        break;
    case LIST_CMD_PIXEL_MAP:
        glPixelMapfv(command->a, (GLsizei)command->u, command->f);
        break;
    case LIST_CMD_BITMAP: {
        GLint saved_unpack_alignment = unpack_alignment;
        GLint saved_unpack_row_length = unpack_row_length;
        GLint saved_unpack_skip_rows = unpack_skip_rows;
        GLint saved_unpack_skip_pixels = unpack_skip_pixels;
        GLboolean saved_unpack_swap_bytes = unpack_swap_bytes;
        GLboolean saved_unpack_lsb_first = unpack_lsb_first;
        unpack_alignment = command->i[0];
        unpack_row_length = command->i[1];
        unpack_skip_rows = command->i[2];
        unpack_skip_pixels = command->i[3];
        unpack_swap_bytes = (command->u & 1u) ? GL_TRUE : GL_FALSE;
        unpack_lsb_first = (command->u & 2u) ? GL_TRUE : GL_FALSE;
        glBitmap((GLsizei)command->f[0], (GLsizei)command->f[1],
                 command->f[2], command->f[3],
                 command->extra[0], command->extra[1],
                 (const GLubyte *)command->data);
        unpack_alignment = saved_unpack_alignment;
        unpack_row_length = saved_unpack_row_length;
        unpack_skip_rows = saved_unpack_skip_rows;
        unpack_skip_pixels = saved_unpack_skip_pixels;
        unpack_swap_bytes = saved_unpack_swap_bytes;
        unpack_lsb_first = saved_unpack_lsb_first;
        break;
    }
    case LIST_CMD_DRAW_PIXELS: {
        GLint saved_unpack_alignment = unpack_alignment;
        GLint saved_unpack_row_length = unpack_row_length;
        GLint saved_unpack_skip_rows = unpack_skip_rows;
        GLint saved_unpack_skip_pixels = unpack_skip_pixels;
        GLboolean saved_unpack_swap_bytes = unpack_swap_bytes;
        GLboolean saved_unpack_lsb_first = unpack_lsb_first;
        unpack_alignment = command->i[0];
        unpack_row_length = command->i[1];
        unpack_skip_rows = command->i[2];
        unpack_skip_pixels = command->i[3];
        unpack_swap_bytes = (command->u & 1u) ? GL_TRUE : GL_FALSE;
        unpack_lsb_first = (command->u & 2u) ? GL_TRUE : GL_FALSE;
        glDrawPixels((GLsizei)command->f[0], (GLsizei)command->f[1],
                     command->a, command->b, command->data);
        unpack_alignment = saved_unpack_alignment;
        unpack_row_length = saved_unpack_row_length;
        unpack_skip_rows = saved_unpack_skip_rows;
        unpack_skip_pixels = saved_unpack_skip_pixels;
        unpack_swap_bytes = saved_unpack_swap_bytes;
        unpack_lsb_first = saved_unpack_lsb_first;
        break;
    }
    case LIST_CMD_COPY_PIXELS:
        glCopyPixels((GLint)command->f[0], (GLint)command->f[1],
                     (GLsizei)command->f[2], (GLsizei)command->f[3], command->a);
        break;
    case LIST_CMD_SHADE_MODEL:
        glShadeModel(command->a);
        break;
    case LIST_CMD_LIGHT_MODEL:
        glLightModelfv(command->a, command->f);
        break;
    case LIST_CMD_LIGHT:
        glLightfv(command->a, command->b, command->f);
        break;
    case LIST_CMD_FOG:
        glFogfv(command->a, command->f);
        break;
    case LIST_CMD_CLIP_PLANE: {
        GLdouble equation[4] = { command->f[0], command->f[1], command->f[2], command->f[3] };
        glClipPlane(command->a, equation);
        break;
    }
    case LIST_CMD_RASTER_POS:
        glRasterPos4f(command->f[0], command->f[1], command->f[2], command->f[3]);
        break;
    case LIST_CMD_ENABLE:
        glEnable(command->a);
        break;
    case LIST_CMD_DISABLE:
        glDisable(command->a);
        break;
    case LIST_CMD_DEPTH_FUNC:
        glDepthFunc(command->a);
        break;
    case LIST_CMD_DEPTH_MASK:
        glDepthMask((GLboolean)command->u);
        break;
    case LIST_CMD_STENCIL_FUNC:
        glStencilFunc(command->a, (GLint)command->f[0], command->u);
        break;
    case LIST_CMD_STENCIL_OP:
        glStencilOp(command->a, command->b, command->u);
        break;
    case LIST_CMD_STENCIL_MASK:
        glStencilMask(command->u);
        break;
    case LIST_CMD_SCISSOR:
        glScissor((GLint)command->f[0], (GLint)command->f[1], (GLsizei)command->f[2], (GLsizei)command->f[3]);
        break;
    case LIST_CMD_BLEND_FUNC:
        glBlendFunc(command->a, command->b);
        break;
    case LIST_CMD_DRAW_BUFFER:
        glDrawBuffer(command->a);
        break;
    case LIST_CMD_READ_BUFFER:
        glReadBuffer(command->a);
        break;
    case LIST_CMD_ACTIVE_TEXTURE:
        glActiveTexture(command->a);
        break;
    case LIST_CMD_BIND_TEXTURE:
        glBindTexture(command->a, command->u);
        break;
    case LIST_CMD_TEX_PARAMETER:
        glTexParameteri(command->a, command->b, (GLint)command->u);
        break;
    case LIST_CMD_TEX_PARAMETER_F:
        glTexParameterf(command->a, command->b, command->f[0]);
        break;
    case LIST_CMD_TEX_ENV:
        glTexEnvfv(command->a, command->b, command->f);
        break;
    case LIST_CMD_TEX_GEN:
        if (command->b == GL_TEXTURE_GEN_MODE) {
            glTexGeni(command->a, command->b, (GLint)command->u);
        } else {
            glTexGenfv(command->a, command->b, command->f);
        }
        break;
    case LIST_CMD_PUSH_ATTRIB:
        glPushAttrib(command->bits);
        break;
    case LIST_CMD_POP_ATTRIB:
        glPopAttrib();
        break;
    case LIST_CMD_PUSH_CLIENT_ATTRIB:
        glPushClientAttrib(command->bits);
        break;
    case LIST_CMD_POP_CLIENT_ATTRIB:
        glPopClientAttrib();
        break;
    }
}

GLuint glGenLists(GLsizei range)
{
    if (range < 0) {
        set_error(GL_INVALID_VALUE);
        return 0;
    }
    if (range == 0 || range >= 256) {
        return 0;
    }

    for (GLuint first = 1; first + (GLuint)range <= 256; ++first) {
        bool free_range = true;
        for (GLsizei i = 0; i < range; ++i) {
            if (display_lists[first + (GLuint)i].allocated) {
                free_range = false;
                break;
            }
        }
        if (free_range) {
            for (GLsizei i = 0; i < range; ++i) {
                clear_display_list(&display_lists[first + (GLuint)i]);
                display_lists[first + (GLuint)i].allocated = true;
            }
            return first;
        }
    }
    return 0;
}

GLboolean glIsList(GLuint list)
{
    return list > 0 && list < 256 && display_lists[list].allocated ? GL_TRUE : GL_FALSE;
}

void glNewList(GLuint list, GLenum mode)
{
    if (list == 0 || list >= 256 || (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE)) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (recording_list != 0 || begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    clear_display_list(&display_lists[list]);
    display_lists[list].allocated = true;
    recording_list = list;
    recording_mode = mode;
}

void glEndList(void)
{
    if (recording_list == 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    recording_list = 0;
    recording_mode = 0;
}

void glCallList(GLuint list)
{
    DisplayList *display_list;
    bool was_replaying;

    if (list == 0 || list >= 256 || !display_lists[list].allocated) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (begin_mode != NXGL_NO_BEGIN_MODE) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    display_list = &display_lists[list];
    was_replaying = replaying_list;
    replaying_list = true;
    for (size_t i = 0; i < display_list->count; ++i) {
        execute_list_command(&display_list->commands[i]);
    }
    replaying_list = was_replaying;
}

void glCallLists(GLsizei count, GLenum type, const GLvoid *lists)
{
    if (reject_inside_begin()) {
        return;
    }
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (lists == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint id;
        if (type == GL_UNSIGNED_BYTE) {
            id = list_base + ((const GLubyte *)lists)[i];
        } else if (type == GL_UNSIGNED_SHORT) {
            id = list_base + ((const GLushort *)lists)[i];
        } else {
            id = list_base + ((const GLuint *)lists)[i];
        }
        glCallList(id);
    }
}

void glDeleteLists(GLuint list, GLsizei range)
{
    if (range < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (list == 0 || range == 0) {
        return;
    }

    for (GLsizei i = 0; i < range; ++i) {
        GLuint id = list + (GLuint)i;
        if (id > 0 && id < 256) {
            clear_display_list(&display_lists[id]);
        }
    }
}

void glListBase(GLuint base)
{
    ListCommand command = { LIST_CMD_COLOR4 };
    (void)command;
    if (reject_inside_begin()) {
        return;
    }
    list_base = base;
}

void glGenTextures(int count, uint32_t *out_textures)
{
    if (out_textures == NULL || count <= 0) {
        if (count < 0) {
            set_error(GL_INVALID_VALUE);
        }
        return;
    }

    for (int i = 0; i < count; ++i) {
        out_textures[i] = 0;
        for (int id = 1; id < 16; ++id) {
            if (!texture_objects[id].allocated) {
                init_texture_object(&texture_objects[id]);
                texture_objects[id].allocated = true;
                out_textures[i] = (uint32_t)id;
                break;
            }
        }
    }
}

void glDeleteTextures(GLsizei count, const GLuint *delete_textures)
{
    if (count < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (delete_textures == NULL) {
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint id = delete_textures[i];
        if (id > 0 && id < 16) {
            destroy_texture_image(&texture_objects[id]);
            init_texture_object(&texture_objects[id]);
            for (int unit = 0; unit < 4; ++unit) {
                if (texture_binding_1d[unit] == id) {
                    texture_binding_1d[unit] = 0;
                }
                if (texture_binding_2d[unit] == id) {
                    texture_binding_2d[unit] = 0;
                }
                if (texture_binding_3d[unit] == id) {
                    texture_binding_3d[unit] = 0;
                }
                if (texture_binding_cube_map[unit] == id) {
                    texture_binding_cube_map[unit] = 0;
                }
            }
        }
    }
    sync_native_state();
}

GLboolean glIsTexture(GLuint texture)
{
    return texture > 0 && texture < 16 && texture_objects[texture].allocated ? GL_TRUE : GL_FALSE;
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
    if (n < 0) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (textures == NULL || priorities == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    for (GLsizei i = 0; i < n; ++i) {
        GLuint id = textures[i];
        if (id > 0 && id < 16 && texture_objects[id].allocated) {
            texture_objects[id].priority = clamp_priority(priorities[i]);
        }
    }
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    GLboolean all_resident = GL_TRUE;

    if (n < 0) {
        set_error(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    if (textures == NULL || residences == NULL) {
        set_error(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    for (GLsizei i = 0; i < n; ++i) {
        GLuint id = textures[i];
        GLboolean resident = id > 0 && id < 16 && texture_objects[id].allocated ? GL_TRUE : GL_FALSE;
        residences[i] = resident;
        if (!resident) {
            all_resident = GL_FALSE;
        }
    }
    return all_resident;
}

void glBindTexture(uint32_t target, uint32_t texture)
{
    int unit = active_texture_index();

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture >= 16) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (texture > 0 && !texture_objects[texture].allocated) {
        init_texture_object(&texture_objects[texture]);
        texture_objects[texture].allocated = true;
    }
    ListCommand command = { LIST_CMD_BIND_TEXTURE };
    command.a = target;
    command.u = texture;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (unit >= 0) {
        if (target == GL_TEXTURE_1D) {
            texture_binding_1d[unit] = texture;
        } else if (target == GL_TEXTURE_2D) {
            texture_binding_2d[unit] = texture;
        } else if (target == GL_TEXTURE_3D) {
            texture_binding_3d[unit] = texture;
        } else {
            texture_binding_cube_map[unit] = texture;
        }
    }
    sync_native_state();
}

static int combine_source_index(GLenum pname, bool *alpha)
{
    if (pname >= GL_SOURCE0_RGB && pname <= GL_SOURCE2_RGB) {
        *alpha = false;
        return (int)(pname - GL_SOURCE0_RGB);
    }
    if (pname >= GL_SOURCE0_ALPHA && pname <= GL_SOURCE2_ALPHA) {
        *alpha = true;
        return (int)(pname - GL_SOURCE0_ALPHA);
    }
    return -1;
}

static int combine_operand_index(GLenum pname, bool *alpha)
{
    if (pname >= GL_OPERAND0_RGB && pname <= GL_OPERAND2_RGB) {
        *alpha = false;
        return (int)(pname - GL_OPERAND0_RGB);
    }
    if (pname >= GL_OPERAND0_ALPHA && pname <= GL_OPERAND2_ALPHA) {
        *alpha = true;
        return (int)(pname - GL_OPERAND0_ALPHA);
    }
    return -1;
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    int unit = active_texture_index();
    ListCommand command = { LIST_CMD_TEX_ENV };
    bool alpha = false;
    int index = -1;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        if (!valid_texture_env_mode((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        command.f[0] = params[0];
        command.f[1] = params[1];
        command.f[2] = params[2];
        command.f[3] = params[3];
    } else if (pname == GL_COMBINE_RGB || pname == GL_COMBINE_ALPHA) {
        if (!valid_combine_mode((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        if (!valid_combine_source((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        if (alpha) {
            if (!valid_combine_alpha_operand((GLint)params[0])) {
                set_error(GL_INVALID_ENUM);
                return;
            }
        } else if (!valid_combine_rgb_operand((GLint)params[0])) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        command.f[0] = params[0];
    } else if (pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE) {
        if (!valid_combine_scale(params[0])) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        command.f[0] = params[0];
    } else {
        set_error(GL_INVALID_ENUM);
        return;
    }

    command.a = target;
    command.b = pname;
    record_command(command);
    if (compile_only()) {
        return;
    }

    if (pname == GL_TEXTURE_ENV_MODE) {
        texture_env_mode[unit] = (GLenum)((GLint)params[0]);
    } else {
        index = combine_source_index(pname, &alpha);
        if (index >= 0) {
            if (alpha) {
                texture_source_alpha[unit][index] = (GLenum)((GLint)params[0]);
            } else {
                texture_source_rgb[unit][index] = (GLenum)((GLint)params[0]);
            }
        } else {
            index = combine_operand_index(pname, &alpha);
            if (index >= 0) {
                if (alpha) {
                    texture_operand_alpha[unit][index] = (GLenum)((GLint)params[0]);
                } else {
                    texture_operand_rgb[unit][index] = (GLenum)((GLint)params[0]);
                }
            } else if (pname == GL_COMBINE_RGB) {
                texture_combine_rgb[unit] = (GLenum)((GLint)params[0]);
            } else if (pname == GL_COMBINE_ALPHA) {
                texture_combine_alpha[unit] = (GLenum)((GLint)params[0]);
            } else if (pname == GL_RGB_SCALE) {
                texture_rgb_scale[unit] = params[0];
            } else if (pname == GL_ALPHA_SCALE) {
                texture_alpha_scale[unit] = params[0];
            } else {
        texture_env_color[unit] = (NxglBackendColor){ params[0], params[1], params[2], params[3] };
            }
        }
    }
    if (unit == 0) {
        sync_native_state();
    }
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    GLfloat values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        values[0] = (GLfloat)params[0];
        glTexEnvfv(target, pname, values);
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        values[0] = (GLfloat)params[0];
        values[1] = (GLfloat)params[1];
        values[2] = (GLfloat)params[2];
        values[3] = (GLfloat)params[3];
        glTexEnvfv(target, pname, values);
    } else {
        values[0] = (GLfloat)params[0];
        glTexEnvfv(target, pname, values);
    }
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    GLfloat value = (GLfloat)param;

    if (pname == GL_TEXTURE_ENV_COLOR) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glTexEnvfv(target, pname, &value);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    if (pname == GL_TEXTURE_ENV_COLOR) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    glTexEnvfv(target, pname, &param);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
    int unit = active_texture_index();
    bool alpha = false;
    int index;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        params[0] = (GLint)texture_env_mode[unit];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        params[0] = (GLint)texture_env_color[unit].r;
        params[1] = (GLint)texture_env_color[unit].g;
        params[2] = (GLint)texture_env_color[unit].b;
        params[3] = (GLint)texture_env_color[unit].a;
    } else if (pname == GL_COMBINE_RGB) {
        params[0] = (GLint)texture_combine_rgb[unit];
    } else if (pname == GL_COMBINE_ALPHA) {
        params[0] = (GLint)texture_combine_alpha[unit];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        params[0] = (GLint)(alpha ? texture_source_alpha[unit][index] : texture_source_rgb[unit][index]);
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        params[0] = (GLint)(alpha ? texture_operand_alpha[unit][index] : texture_operand_rgb[unit][index]);
    } else if (pname == GL_RGB_SCALE) {
        params[0] = (GLint)texture_rgb_scale[unit];
    } else if (pname == GL_ALPHA_SCALE) {
        params[0] = (GLint)texture_alpha_scale[unit];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
    int unit = active_texture_index();
    bool alpha = false;
    int index;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (target != GL_TEXTURE_ENV) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (unit < 0) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (pname == GL_TEXTURE_ENV_MODE) {
        params[0] = (GLfloat)texture_env_mode[unit];
    } else if (pname == GL_TEXTURE_ENV_COLOR) {
        params[0] = texture_env_color[unit].r;
        params[1] = texture_env_color[unit].g;
        params[2] = texture_env_color[unit].b;
        params[3] = texture_env_color[unit].a;
    } else if (pname == GL_COMBINE_RGB) {
        params[0] = (GLfloat)texture_combine_rgb[unit];
    } else if (pname == GL_COMBINE_ALPHA) {
        params[0] = (GLfloat)texture_combine_alpha[unit];
    } else if ((index = combine_source_index(pname, &alpha)) >= 0) {
        params[0] = (GLfloat)(alpha ? texture_source_alpha[unit][index] : texture_source_rgb[unit][index]);
    } else if ((index = combine_operand_index(pname, &alpha)) >= 0) {
        params[0] = (GLfloat)(alpha ? texture_operand_alpha[unit][index] : texture_operand_rgb[unit][index]);
    } else if (pname == GL_RGB_SCALE) {
        params[0] = texture_rgb_scale[unit];
    } else if (pname == GL_ALPHA_SCALE) {
        params[0] = texture_alpha_scale[unit];
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

static bool valid_min_filter(GLint value)
{
    return value == GL_NEAREST ||
           value == GL_LINEAR ||
           value == GL_NEAREST_MIPMAP_NEAREST ||
           value == GL_LINEAR_MIPMAP_NEAREST ||
           value == GL_NEAREST_MIPMAP_LINEAR ||
           value == GL_LINEAR_MIPMAP_LINEAR;
}

static bool valid_mag_filter(GLint value)
{
    return value == GL_NEAREST || value == GL_LINEAR;
}

static bool valid_wrap(GLint value)
{
    return value == GL_CLAMP || value == GL_REPEAT || value == GL_CLAMP_TO_EDGE;
}

static bool valid_stencil_func(GLenum func)
{
    return func == GL_NEVER ||
           func == GL_LESS ||
           func == GL_EQUAL ||
           func == GL_LEQUAL ||
           func == GL_GREATER ||
           func == GL_NOTEQUAL ||
           func == GL_GEQUAL ||
           func == GL_ALWAYS;
}

static bool valid_stencil_op(GLenum op)
{
    return op == GL_KEEP ||
           op == GL_ZERO ||
           op == GL_REPLACE ||
           op == GL_INCR ||
           op == GL_DECR ||
           op == GL_INVERT;
}

static bool valid_depth_func(GLenum func)
{
    return valid_stencil_func(func);
}

static bool valid_blend_src_factor(GLenum factor)
{
    return factor == GL_ZERO ||
           factor == GL_ONE ||
           factor == GL_DST_COLOR ||
           factor == GL_ONE_MINUS_DST_COLOR ||
           factor == GL_SRC_ALPHA ||
           factor == GL_ONE_MINUS_SRC_ALPHA ||
           factor == GL_DST_ALPHA ||
           factor == GL_ONE_MINUS_DST_ALPHA ||
           factor == GL_SRC_ALPHA_SATURATE;
}

static bool valid_blend_dst_factor(GLenum factor)
{
    return factor == GL_ZERO ||
           factor == GL_ONE ||
           factor == GL_SRC_COLOR ||
           factor == GL_ONE_MINUS_SRC_COLOR ||
           factor == GL_SRC_ALPHA ||
           factor == GL_ONE_MINUS_SRC_ALPHA ||
           factor == GL_DST_ALPHA ||
           factor == GL_ONE_MINUS_DST_ALPHA;
}

static bool valid_logic_op(GLenum op)
{
    return op == GL_CLEAR ||
           op == GL_AND ||
           op == GL_AND_REVERSE ||
           op == GL_COPY ||
           op == GL_AND_INVERTED ||
           op == GL_NOOP ||
           op == GL_XOR ||
           op == GL_OR ||
           op == GL_NOR ||
           op == GL_EQUIV ||
           op == GL_INVERT ||
           op == GL_OR_REVERSE ||
           op == GL_COPY_INVERTED ||
           op == GL_OR_INVERTED ||
           op == GL_NAND ||
           op == GL_SET;
}

static bool valid_hint_target(GLenum target)
{
    return target == GL_PERSPECTIVE_CORRECTION_HINT ||
           target == GL_POINT_SMOOTH_HINT ||
           target == GL_LINE_SMOOTH_HINT ||
           target == GL_POLYGON_SMOOTH_HINT ||
           target == GL_FOG_HINT;
}

static bool valid_hint_mode(GLenum mode)
{
    return mode == GL_DONT_CARE ||
           mode == GL_FASTEST ||
           mode == GL_NICEST;
}

static bool valid_draw_buffer(GLenum mode)
{
    return mode == GL_NONE ||
           mode == GL_FRONT ||
           mode == GL_BACK ||
           mode == GL_FRONT_AND_BACK ||
           mode == GL_FRONT_LEFT ||
           mode == GL_FRONT_RIGHT ||
           mode == GL_BACK_LEFT ||
           mode == GL_BACK_RIGHT;
}

static bool valid_read_buffer(GLenum mode)
{
    return mode == GL_NONE ||
           mode == GL_FRONT ||
           mode == GL_BACK ||
           mode == GL_FRONT_LEFT ||
           mode == GL_FRONT_RIGHT ||
           mode == GL_BACK_LEFT ||
           mode == GL_BACK_RIGHT;
}

static bool is_mipmap_filter(GLint value)
{
    return value == GL_NEAREST_MIPMAP_NEAREST ||
           value == GL_LINEAR_MIPMAP_NEAREST ||
           value == GL_NEAREST_MIPMAP_LINEAR ||
           value == GL_LINEAR_MIPMAP_LINEAR;
}

static GLfloat clamp_priority(GLfloat value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static int mip_chain_end_level(GLsizei width, GLsizei height, GLsizei depth, GLint base_level, GLint max_level)
{
    int level = base_level;
    while ((width > 1 || height > 1 || depth > 1) && level + 1 < NXGL_MAX_TEXTURE_LEVELS && level < max_level) {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        depth = depth > 1 ? depth / 2 : 1;
        ++level;
    }
    return level;
}

static bool texture_levels_complete(const TextureObject *texture, const TextureLevel *levels)
{
    GLint base = texture->base_level;
    GLint max = texture->max_level;
    GLsizei expected_w;
    GLsizei expected_h;
    GLsizei expected_d;
    GLint end;

    if (base < 0 || base >= NXGL_MAX_TEXTURE_LEVELS || max < base) {
        return false;
    }
    if (!levels[base].defined) {
        return false;
    }
    if (!is_mipmap_filter(texture->min_filter)) {
        return true;
    }

    expected_w = levels[base].width;
    expected_h = levels[base].height;
    expected_d = levels[base].depth;
    end = mip_chain_end_level(expected_w, expected_h, expected_d, base, max);

    for (GLint level = base; level <= end; ++level) {
        if (!levels[level].defined ||
            levels[level].width != expected_w ||
            levels[level].height != expected_h ||
            levels[level].depth != expected_d ||
            levels[level].internal_format != levels[base].internal_format) {
            return false;
        }
        expected_w = expected_w > 1 ? expected_w / 2 : 1;
        expected_h = expected_h > 1 ? expected_h / 2 : 1;
        expected_d = expected_d > 1 ? expected_d / 2 : 1;
    }
    return true;
}

static bool texture_complete(const TextureObject *texture)
{
    return texture_levels_complete(texture, texture->levels);
}

static bool texture_1d_complete(const TextureObject *texture)
{
    return texture_levels_complete(texture, texture->levels_1d);
}

static int source_components(GLenum format)
{
    if (format == GL_COLOR_INDEX) {
        return 1;
    }
    if (format == GL_RGB || format == GL_BGR) {
        return 3;
    }
    if (format == GL_RGBA || format == GL_BGRA) {
        return 4;
    }
    return 0;
}

static size_t source_element_size(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
        return 1;
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        return 2;
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
        return 4;
    default:
        return 0;
    }
}

static bool valid_pixel_type(GLenum type)
{
    return source_element_size(type) != 0;
}

static uint8_t expand_bits(uint32_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return (uint8_t)((value * 255u + max / 2u) / max);
}

static uint32_t pack_bits(uint8_t value, int bits)
{
    uint32_t max = (1u << bits) - 1u;
    return ((uint32_t)value * max + 127u) / 255u;
}

static uint16_t read_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static uint32_t read_u32(const uint8_t *src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static void write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static bool unpack_pixel(const uint8_t *src, GLenum format, GLenum type, uint8_t *rgba)
{
    uint32_t v;

    if (format == GL_COLOR_INDEX) {
        GLuint index;

        if (type == GL_UNSIGNED_BYTE) {
            index = src[0];
        } else if (type == GL_UNSIGNED_SHORT) {
            index = read_u16(src);
        } else if (type == GL_UNSIGNED_INT) {
            index = read_u32(src);
        } else {
            return false;
        }
        index = map_integer_value(index, pixel_map_index(GL_PIXEL_MAP_I_TO_I), 255u);
        rgba[0] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_R)));
        rgba[1] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_G)));
        rgba[2] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_B)));
        rgba[3] = channel(map_index_to_component(index, pixel_map_index(GL_PIXEL_MAP_I_TO_A)));
        return true;
    }

    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGBA) {
            rgba[0] = src[0]; rgba[1] = src[1]; rgba[2] = src[2]; rgba[3] = src[3];
            return true;
        }
        if (format == GL_RGB) {
            rgba[0] = src[0]; rgba[1] = src[1]; rgba[2] = src[2]; rgba[3] = 255;
            return true;
        }
        if (format == GL_BGRA) {
            rgba[0] = src[2]; rgba[1] = src[1]; rgba[2] = src[0]; rgba[3] = src[3];
            return true;
        }
        if (format == GL_BGR) {
            rgba[0] = src[2]; rgba[1] = src[1]; rgba[2] = src[0]; rgba[3] = 255;
            return true;
        }
        return false;
    }

    rgba[0] = 0;
    rgba[1] = 0;
    rgba[2] = 0;
    rgba[3] = 255;
    if (type == GL_UNSIGNED_BYTE_3_3_2) {
        if (format != GL_RGB) return false;
        v = src[0];
        rgba[0] = expand_bits((v >> 5) & 0x7, 3);
        rgba[1] = expand_bits((v >> 2) & 0x7, 3);
        rgba[2] = expand_bits(v & 0x3, 2);
    } else if (type == GL_UNSIGNED_BYTE_2_3_3_REV) {
        if (format != GL_RGB) return false;
        v = src[0];
        rgba[0] = expand_bits(v & 0x7, 3);
        rgba[1] = expand_bits((v >> 3) & 0x7, 3);
        rgba[2] = expand_bits((v >> 6) & 0x3, 2);
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format != GL_RGB) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 11) & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x3f, 6);
        rgba[2] = expand_bits(v & 0x1f, 5);
    } else if (type == GL_UNSIGNED_SHORT_5_6_5_REV) {
        if (format != GL_RGB) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x3f, 6);
        rgba[2] = expand_bits((v >> 11) & 0x1f, 5);
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 12) & 0xf, 4);
        rgba[1] = expand_bits((v >> 8) & 0xf, 4);
        rgba[2] = expand_bits((v >> 4) & 0xf, 4);
        rgba[3] = expand_bits(v & 0xf, 4);
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0xf, 4);
        rgba[1] = expand_bits((v >> 4) & 0xf, 4);
        rgba[2] = expand_bits((v >> 8) & 0xf, 4);
        rgba[3] = expand_bits((v >> 12) & 0xf, 4);
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits((v >> 11) & 0x1f, 5);
        rgba[1] = expand_bits((v >> 6) & 0x1f, 5);
        rgba[2] = expand_bits((v >> 1) & 0x1f, 5);
        rgba[3] = (v & 0x1) ? 255 : 0;
    } else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u16(src);
        rgba[0] = expand_bits(v & 0x1f, 5);
        rgba[1] = expand_bits((v >> 5) & 0x1f, 5);
        rgba[2] = expand_bits((v >> 10) & 0x1f, 5);
        rgba[3] = (v & 0x8000) ? 255 : 0;
    } else if (type == GL_UNSIGNED_INT_8_8_8_8) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = (uint8_t)((v >> 24) & 0xff);
        rgba[1] = (uint8_t)((v >> 16) & 0xff);
        rgba[2] = (uint8_t)((v >> 8) & 0xff);
        rgba[3] = (uint8_t)(v & 0xff);
    } else if (type == GL_UNSIGNED_INT_8_8_8_8_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = (uint8_t)(v & 0xff);
        rgba[1] = (uint8_t)((v >> 8) & 0xff);
        rgba[2] = (uint8_t)((v >> 16) & 0xff);
        rgba[3] = (uint8_t)((v >> 24) & 0xff);
    } else if (type == GL_UNSIGNED_INT_10_10_10_2) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = expand_bits((v >> 22) & 0x3ff, 10);
        rgba[1] = expand_bits((v >> 12) & 0x3ff, 10);
        rgba[2] = expand_bits((v >> 2) & 0x3ff, 10);
        rgba[3] = expand_bits(v & 0x3, 2);
    } else if (type == GL_UNSIGNED_INT_2_10_10_10_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = read_u32(src);
        rgba[0] = expand_bits(v & 0x3ff, 10);
        rgba[1] = expand_bits((v >> 10) & 0x3ff, 10);
        rgba[2] = expand_bits((v >> 20) & 0x3ff, 10);
        rgba[3] = expand_bits((v >> 30) & 0x3, 2);
    } else {
        return false;
    }

    if (format == GL_BGRA || format == GL_BGR) {
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
    }
    return true;
}

static bool pack_pixel(uint8_t *dst, const uint8_t *rgba, GLenum format, GLenum type)
{
    uint8_t r = rgba[0];
    uint8_t g = rgba[1];
    uint8_t b = rgba[2];
    uint8_t a = rgba[3];
    uint32_t v;

    if (format == GL_BGRA || format == GL_BGR) {
        uint8_t tmp = r;
        r = b;
        b = tmp;
    }

    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGBA || format == GL_BGRA) {
            dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
            return true;
        }
        if (format == GL_RGB || format == GL_BGR) {
            dst[0] = r; dst[1] = g; dst[2] = b;
            return true;
        }
        return false;
    }

    if (type == GL_UNSIGNED_BYTE_3_3_2) {
        if (format != GL_RGB) return false;
        dst[0] = (uint8_t)((pack_bits(r, 3) << 5) | (pack_bits(g, 3) << 2) | pack_bits(b, 2));
    } else if (type == GL_UNSIGNED_BYTE_2_3_3_REV) {
        if (format != GL_RGB) return false;
        dst[0] = (uint8_t)(pack_bits(r, 3) | (pack_bits(g, 3) << 3) | (pack_bits(b, 2) << 6));
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format != GL_RGB) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 5) << 11) | (pack_bits(g, 6) << 5) | pack_bits(b, 5)));
    } else if (type == GL_UNSIGNED_SHORT_5_6_5_REV) {
        if (format != GL_RGB) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 5) | (pack_bits(g, 6) << 5) | (pack_bits(b, 5) << 11)));
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 4) << 12) | (pack_bits(g, 4) << 8) | (pack_bits(b, 4) << 4) | pack_bits(a, 4)));
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 4) | (pack_bits(g, 4) << 4) | (pack_bits(b, 4) << 8) | (pack_bits(a, 4) << 12)));
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)((pack_bits(r, 5) << 11) | (pack_bits(g, 5) << 6) | (pack_bits(b, 5) << 1) | (a >= 128 ? 1 : 0)));
    } else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u16(dst, (uint16_t)(pack_bits(r, 5) | (pack_bits(g, 5) << 5) | (pack_bits(b, 5) << 10) | (a >= 128 ? 0x8000 : 0)));
    } else if (type == GL_UNSIGNED_INT_8_8_8_8) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u32(dst, ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a);
    } else if (type == GL_UNSIGNED_INT_8_8_8_8_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        write_u32(dst, (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24));
    } else if (type == GL_UNSIGNED_INT_10_10_10_2) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = (pack_bits(r, 10) << 22) | (pack_bits(g, 10) << 12) | (pack_bits(b, 10) << 2) | pack_bits(a, 2);
        write_u32(dst, v);
    } else if (type == GL_UNSIGNED_INT_2_10_10_10_REV) {
        if (format != GL_RGBA && format != GL_BGRA) return false;
        v = pack_bits(r, 10) | (pack_bits(g, 10) << 10) | (pack_bits(b, 10) << 20) | (pack_bits(a, 2) << 30);
        write_u32(dst, v);
    } else {
        return false;
    }
    return true;
}

static bool convert_to_rgba(uint8_t *dst, const uint8_t *src, GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t src_stride;

    if (comps == 0 || elem_size == 0) {
        return false;
    }

    if (src == NULL) {
        memset(dst, 0, (size_t)width * (size_t)height * 4);
        return true;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    src_stride = pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
    src += (size_t)unpack_skip_rows * src_stride + (size_t)unpack_skip_pixels * pixel_bytes;
    for (GLsizei y = 0; y < height; ++y) {
        const uint8_t *src_row = src + (size_t)y * src_stride;
        uint8_t *dst_row = dst + (size_t)y * (size_t)width * 4;
        for (GLsizei x = 0; x < width; ++x) {
            const uint8_t *s = src_row + (size_t)x * pixel_bytes;
            uint8_t *d = dst_row + (size_t)x * 4;
            if (!unpack_pixel(s, format, type, d)) {
                return false;
            }
        }
    }
    return true;
}

static bool convert_to_rgba_3d(uint8_t *dst, const uint8_t *src, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type)
{
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t src_stride;
    size_t src_slice_stride;

    if (comps == 0 || elem_size == 0) {
        return false;
    }

    if (src == NULL) {
        memset(dst, 0, (size_t)width * (size_t)height * (size_t)depth * 4u);
        return true;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    src_stride = pixel_store_row_stride(width, unpack_row_length, unpack_skip_pixels, pixel_bytes, unpack_alignment);
    src_slice_stride = pixel_store_slice_stride(width, height, unpack_row_length, unpack_skip_rows, unpack_skip_pixels,
                                                unpack_image_height, pixel_bytes, unpack_alignment);
    src += (size_t)unpack_skip_images * src_slice_stride +
           (size_t)unpack_skip_rows * src_stride +
           (size_t)unpack_skip_pixels * pixel_bytes;
    for (GLsizei z = 0; z < depth; ++z) {
        for (GLsizei y = 0; y < height; ++y) {
            const uint8_t *src_row = src + (size_t)z * src_slice_stride + (size_t)y * src_stride;
            uint8_t *dst_row = dst + ((size_t)z * (size_t)height + (size_t)y) * (size_t)width * 4u;
            for (GLsizei x = 0; x < width; ++x) {
                const uint8_t *s = src_row + (size_t)x * pixel_bytes;
                uint8_t *d = dst_row + (size_t)x * 4u;
                if (!unpack_pixel(s, format, type, d)) {
                    return false;
                }
            }
        }
    }
    return true;
}

void glTexParameteri(uint32_t target, uint32_t pname, int value)
{
    TextureObject *texture;

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    ListCommand command = { LIST_CMD_TEX_PARAMETER };
    command.a = target;
    command.b = pname;
    command.u = (GLuint)value;

    if (pname == GL_TEXTURE_MIN_FILTER) {
        if (!valid_min_filter(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_filter = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_1D && !rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_3D && !rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        if (!valid_mag_filter(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->mag_filter = value;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_s = value;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_t = value;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        if (!valid_wrap(value)) {
            set_error(GL_INVALID_ENUM);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->wrap_r = value;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        if (value < 0 || value >= NXGL_MAX_TEXTURE_LEVELS) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->base_level = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_1D && !rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        if (target == GL_TEXTURE_3D && !rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        if (value < 0) {
            set_error(GL_INVALID_VALUE);
            return;
        }
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_level = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_lod = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_lod = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->lod_bias = (GLfloat)value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_PRIORITY) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->priority = clamp_priority((GLfloat)value);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat value)
{
    TextureObject *texture;
    ListCommand command = { LIST_CMD_TEX_PARAMETER_F };

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    command.a = target;
    command.b = pname;
    command.f[0] = value;

    if (pname == GL_TEXTURE_MIN_FILTER ||
        pname == GL_TEXTURE_MAG_FILTER ||
        pname == GL_TEXTURE_WRAP_S ||
        pname == GL_TEXTURE_WRAP_T ||
        pname == GL_TEXTURE_WRAP_R ||
        pname == GL_TEXTURE_BASE_LEVEL ||
        pname == GL_TEXTURE_MAX_LEVEL) {
        glTexParameteri(target, pname, (GLint)value);
        return;
    }

    if (pname == GL_TEXTURE_MIN_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->min_lod = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->max_lod = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->lod_bias = value;
        if (target == GL_TEXTURE_2D && !rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
        sync_native_state();
    } else if (pname == GL_TEXTURE_PRIORITY) {
        record_command(command);
        if (compile_only()) {
            return;
        }
        texture->priority = clamp_priority(value);
    } else {
        set_error(GL_INVALID_ENUM);
    }
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (pname == GL_TEXTURE_MIN_LOD ||
        pname == GL_TEXTURE_MAX_LOD ||
        pname == GL_TEXTURE_LOD_BIAS ||
        pname == GL_TEXTURE_PRIORITY) {
        glTexParameterf(target, pname, (GLfloat)params[0]);
    } else {
        glTexParameteri(target, pname, params[0]);
    }
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    glTexParameterf(target, pname, params[0]);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
    TextureObject *texture;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0;
        return;
    }

    if (pname == GL_TEXTURE_MIN_FILTER) {
        params[0] = texture->min_filter;
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        params[0] = texture->mag_filter;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        params[0] = texture->wrap_s;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        params[0] = texture->wrap_t;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        params[0] = texture->wrap_r;
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        params[0] = (GLint)texture->min_lod;
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        params[0] = (GLint)texture->max_lod;
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        params[0] = (GLint)texture->lod_bias;
    } else if (pname == GL_TEXTURE_PRIORITY) {
        params[0] = (GLint)texture->priority;
    } else if (pname == GL_TEXTURE_RESIDENT) {
        params[0] = texture->allocated ? GL_TRUE : GL_FALSE;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        params[0] = texture->base_level;
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        params[0] = texture->max_level;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
    TextureObject *texture;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_binding_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0.0f;
        return;
    }

    if (pname == GL_TEXTURE_MIN_FILTER) {
        params[0] = (GLfloat)texture->min_filter;
    } else if (pname == GL_TEXTURE_MAG_FILTER) {
        params[0] = (GLfloat)texture->mag_filter;
    } else if (pname == GL_TEXTURE_WRAP_S) {
        params[0] = (GLfloat)texture->wrap_s;
    } else if (pname == GL_TEXTURE_WRAP_T) {
        params[0] = (GLfloat)texture->wrap_t;
    } else if (pname == GL_TEXTURE_WRAP_R) {
        params[0] = (GLfloat)texture->wrap_r;
    } else if (pname == GL_TEXTURE_MIN_LOD) {
        params[0] = texture->min_lod;
    } else if (pname == GL_TEXTURE_MAX_LOD) {
        params[0] = texture->max_lod;
    } else if (pname == GL_TEXTURE_LOD_BIAS) {
        params[0] = texture->lod_bias;
    } else if (pname == GL_TEXTURE_PRIORITY) {
        params[0] = texture->priority;
    } else if (pname == GL_TEXTURE_RESIDENT) {
        params[0] = texture->allocated ? 1.0f : 0.0f;
    } else if (pname == GL_TEXTURE_BASE_LEVEL) {
        params[0] = (GLfloat)texture->base_level;
    } else if (pname == GL_TEXTURE_MAX_LEVEL) {
        params[0] = (GLfloat)texture->max_level;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0.0f;
    }
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    TextureObject *texture;
    TextureLevel *image;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        params[0] = 0;
        return;
    }

    texture = bound_texture_object_for_target(target);
    if (texture == NULL) {
        set_error(GL_INVALID_OPERATION);
        params[0] = 0;
        return;
    }
    image = texture_level_for_target(texture, target, level);

    if (pname == GL_TEXTURE_WIDTH) {
        params[0] = image != NULL && image->defined ? image->width : 0;
    } else if (pname == GL_TEXTURE_HEIGHT) {
        params[0] = image != NULL && image->defined ? image->height : 0;
    } else if (pname == GL_TEXTURE_DEPTH) {
        params[0] = image != NULL && image->defined ? image->depth : 0;
    } else if (pname == GL_TEXTURE_INTERNAL_FORMAT) {
        params[0] = image != NULL && image->defined ? image->internal_format : 0;
    } else if (pname == GL_TEXTURE_BORDER) {
        params[0] = 0;
    } else if (pname == GL_TEXTURE_COMPRESSED) {
        params[0] = image != NULL && image->defined && image->compressed ? GL_TRUE : GL_FALSE;
    } else if (pname == GL_TEXTURE_COMPRESSED_IMAGE_SIZE) {
        params[0] = image != NULL && image->defined && image->compressed ? image->compressed_size : 0;
    } else {
        set_error(GL_INVALID_ENUM);
        params[0] = 0;
    }
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    GLint value = 0;
    GLenum before;

    if (params == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    before = last_error;
    last_error = GL_NO_ERROR;
    glGetTexLevelParameteriv(target, level, pname, &value);
    if (last_error == GL_NO_ERROR) {
        params[0] = (GLfloat)value;
        last_error = before;
    } else {
        GLenum error = last_error;
        last_error = before;
        set_error(error);
        params[0] = 0.0f;
    }
}

void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (reject_inside_begin()) {
        return;
    }
    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(rgba, (const uint8_t *)pixels, width, 1, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexImage2D(uint32_t target, int level, int internalformat, int width, int height, int border, uint32_t format, uint32_t type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (reject_inside_begin()) {
        return;
    }
    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(rgba, (const uint8_t *)pixels, width, height, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    const uint8_t *rgba;
    int comps = source_components(format);
    size_t elem_size = source_element_size(type);
    size_t pixel_bytes;
    size_t dst_stride;
    size_t dst_slice_stride;

    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL || image == NULL || !image->defined) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (image->compressed || image->rgba == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (elem_size == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    pixel_bytes = type == GL_UNSIGNED_BYTE ? (size_t)comps : elem_size;
    dst_stride = pixel_store_row_stride(image->width, pack_row_length, pack_skip_pixels, pixel_bytes, pack_alignment);
    dst_slice_stride = target == GL_TEXTURE_3D ?
        pixel_store_slice_stride(image->width, image->height, pack_row_length, pack_skip_rows, pack_skip_pixels,
                                 pack_image_height, pixel_bytes, pack_alignment) :
        dst_stride * (size_t)image->height;
    rgba = image->rgba;
    pixels = (uint8_t *)pixels +
             (target == GL_TEXTURE_3D ? (size_t)pack_skip_images * dst_slice_stride : 0u) +
             (size_t)pack_skip_rows * dst_stride +
             (size_t)pack_skip_pixels * pixel_bytes;
    for (GLsizei z = 0; z < image->depth; ++z) {
        for (GLsizei y = 0; y < image->height; ++y) {
            uint8_t *dst_row = (uint8_t *)pixels + (size_t)z * dst_slice_stride + (size_t)y * dst_stride;
            const uint8_t *src_row = rgba + ((size_t)z * (size_t)image->height + (size_t)y) * (size_t)image->width * 4u;
            for (GLsizei x = 0; x < image->width; ++x) {
                uint8_t *dst = dst_row + (size_t)x * pixel_bytes;
                const uint8_t *src = src_row + (size_t)x * 4u;
                if (!pack_pixel(dst, src, format, type)) {
                    set_error(GL_INVALID_ENUM);
                    return;
                }
            }
        }
    }
}

void glGetCompressedTexImage(GLenum target, GLint level, GLvoid *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;

    if (!valid_texture_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (pixels == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL || image == NULL || !image->defined || !image->compressed || image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    memcpy(pixels, image->compressed_data, (size_t)image->compressed_size);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *converted;
    int comps;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        width < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * 4u);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(converted, (const uint8_t *)pixels, width, 1, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }
    memcpy(image->rgba + (size_t)xoffset * 4u, converted, (size_t)width * 4u);
    free(converted);

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *level_rgba;
    uint8_t *converted;
    int comps;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba(converted, (const uint8_t *)pixels, width, height, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }
    level_rgba = image->rgba;
    for (GLsizei y = 0; y < height; ++y) {
        uint8_t *dst = level_rgba + (((size_t)yoffset + (size_t)y) * (size_t)image->width + (size_t)xoffset) * 4;
        const uint8_t *src = converted + (size_t)y * (size_t)width * 4;
        memcpy(dst, src, (size_t)width * 4);
    }
    free(converted);

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (internalformat != GL_RGB && internalformat != GL_RGBA) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(rgba, x, y, width, height)) {
        free(rgba);
        return;
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = false;

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (internalformat != GL_RGB && internalformat != GL_RGBA) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(rgba, x, y, width, 1)) {
        free(rgba);
        return;
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, height)) {
        free(copy);
        return;
    }

    for (GLsizei row = 0; row < height; ++row) {
        uint8_t *dst = image->rgba + (((size_t)yoffset + (size_t)row) * (size_t)image->width + (size_t)xoffset) * 4u;
        const uint8_t *src = copy + (size_t)row * (size_t)width * 4u;
        memcpy(dst, src, (size_t)width * 4u);
    }
    free(copy);

    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_texture2d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    } else if (cube_face_index(target) >= 0 && level == 0) {
        if (!rebuild_native_cube_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        width < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, 1)) {
        free(copy);
        return;
    }
    memcpy(image->rgba + (size_t)xoffset * 4u, copy, (size_t)width * 4u);
    free(copy);

    if (level == 0) {
        if (!rebuild_native_texture1d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *rgba;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        depth <= 0 ||
        width > 256 ||
        height > 256 ||
        depth > 256) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_RGBA && format != GL_RGB && format != GL_BGRA && format != GL_BGR) ||
        (internalformat != GL_RGB && internalformat != GL_RGBA)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }

    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * (size_t)depth * 4u);
    if (rgba == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba_3d(rgba, (const uint8_t *)pixels, width, height, depth, format, type)) {
        free(rgba);
        set_error(GL_INVALID_ENUM);
        return;
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->rgba = rgba;
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->internal_format = internalformat;
    image->defined = true;
    image->compressed = false;

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *level_rgba;
    uint8_t *converted;
    int comps;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        depth < 0 ||
        pixels == NULL ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset + depth > image->depth) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (!valid_pixel_type(type)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    comps = source_components(format);
    if (comps == 0) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0 || depth == 0) {
        return;
    }

    converted = (uint8_t *)malloc((size_t)width * (size_t)height * (size_t)depth * 4u);
    if (converted == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!convert_to_rgba_3d(converted, (const uint8_t *)pixels, width, height, depth, format, type)) {
        free(converted);
        set_error(GL_INVALID_ENUM);
        return;
    }

    level_rgba = image->rgba;
    for (GLsizei z = 0; z < depth; ++z) {
        for (GLsizei y = 0; y < height; ++y) {
            uint8_t *dst = level_rgba + (((size_t)zoffset + (size_t)z) * (size_t)image->height * (size_t)image->width +
                                          ((size_t)yoffset + (size_t)y) * (size_t)image->width +
                                          (size_t)xoffset) * 4u;
            const uint8_t *src = converted + ((size_t)z * (size_t)height + (size_t)y) * (size_t)width * 4u;
            memcpy(dst, src, (size_t)width * 4u);
        }
    }
    free(converted);

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *copy;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        image == NULL ||
        !image->defined ||
        image->compressed ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset >= image->depth) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    copy = (uint8_t *)malloc((size_t)width * (size_t)height * 4u);
    if (copy == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (!copy_shadow_rgba(copy, x, y, width, height)) {
        free(copy);
        return;
    }

    for (GLsizei row = 0; row < height; ++row) {
        uint8_t *dst = image->rgba + ((size_t)zoffset * (size_t)image->height * (size_t)image->width +
                                      ((size_t)yoffset + (size_t)row) * (size_t)image->width +
                                      (size_t)xoffset) * 4u;
        const uint8_t *src = copy + (size_t)row * (size_t)width * 4u;
        memcpy(dst, src, (size_t)width * 4u);
    }
    free(copy);

    if (level == 0) {
        if (!rebuild_native_texture3d(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        width > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size(width, 1, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = 1;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    sync_native_state();
}

void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei src_blocks_w;
    GLsizei dst_block_x;

    if (target != GL_TEXTURE_1D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        width < 0 ||
        xoffset + width > image->width ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size(width, 1, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    src_blocks_w = (width + 3) / 4;
    dst_block_x = xoffset / 4;
    memcpy(image->compressed_data + (size_t)dst_block_x * (size_t)block_size,
           data,
           (size_t)src_blocks_w * (size_t)block_size);
    sync_native_state();
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        width > 4096 ||
        height > 4096) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (cube_face_index(target) >= 0 && width != height) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size(width, height, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = height;
    image->depth = 1;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_compressed_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei dst_blocks_w;
    GLsizei src_blocks_w;
    GLsizei src_blocks_h;
    GLsizei dst_block_x;
    GLsizei dst_block_y;

    if (!valid_texture_2d_image_target(target)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        yoffset < 0 ||
        width < 0 ||
        height < 0 ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        (yoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width) ||
        ((height & 3) != 0 && yoffset + height != image->height)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size(width, height, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    dst_blocks_w = (image->width + 3) / 4;
    src_blocks_w = (width + 3) / 4;
    src_blocks_h = (height + 3) / 4;
    dst_block_x = xoffset / 4;
    dst_block_y = yoffset / 4;

    for (GLsizei row = 0; row < src_blocks_h; ++row) {
        uint8_t *dst = image->compressed_data + ((size_t)(dst_block_y + row) * (size_t)dst_blocks_w + (size_t)dst_block_x) * (size_t)block_size;
        const uint8_t *src = (const uint8_t *)data + (size_t)row * (size_t)src_blocks_w * (size_t)block_size;
        memcpy(dst, src, (size_t)src_blocks_w * (size_t)block_size);
    }
    if (target == GL_TEXTURE_2D) {
        if (!rebuild_native_compressed_texture(texture)) {
            set_error(GL_OUT_OF_MEMORY);
            return;
        }
    }
    sync_native_state();
}

void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    uint8_t *compressed;
    GLsizei expected_size;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(internalformat)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (texture == NULL ||
        level < 0 ||
        level >= NXGL_MAX_TEXTURE_LEVELS ||
        border != 0 ||
        width <= 0 ||
        height <= 0 ||
        depth <= 0 ||
        width > 256 ||
        height > 256 ||
        depth > 256) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    expected_size = compressed_image_size_3d(width, height, depth, internalformat);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    compressed = (uint8_t *)malloc((size_t)imageSize);
    if (compressed == NULL) {
        set_error(GL_OUT_OF_MEMORY);
        return;
    }
    if (data != NULL) {
        memcpy(compressed, data, (size_t)imageSize);
    } else {
        memset(compressed, 0, (size_t)imageSize);
    }

    destroy_texture_level(texture, target, level);
    image = texture_level_for_target(texture, target, level);
    image->compressed_data = compressed;
    image->compressed_size = imageSize;
    image->width = width;
    image->height = height;
    image->depth = depth;
    image->internal_format = (GLint)internalformat;
    image->defined = true;
    image->compressed = true;
    sync_native_state();
}

void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    TextureObject *texture = bound_texture_object_for_target(target);
    TextureLevel *image;
    int block_size;
    GLsizei expected_size;
    GLsizei dst_slice_size;
    GLsizei src_slice_size;
    GLsizei dst_blocks_w;
    GLsizei src_blocks_w;
    GLsizei src_blocks_h;
    GLsizei dst_block_x;
    GLsizei dst_block_y;

    if (target != GL_TEXTURE_3D) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (!valid_compressed_texture_format(format)) {
        set_error(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || level >= NXGL_MAX_TEXTURE_LEVELS) {
        set_error(GL_INVALID_VALUE);
        return;
    }

    image = texture_level_for_target(texture, target, level);
    if (texture == NULL ||
        image == NULL ||
        !image->defined ||
        !image->compressed ||
        image->compressed_data == NULL) {
        set_error(GL_INVALID_OPERATION);
        return;
    }
    if (format != (GLenum)image->internal_format ||
        xoffset < 0 ||
        yoffset < 0 ||
        zoffset < 0 ||
        width < 0 ||
        height < 0 ||
        depth < 0 ||
        xoffset + width > image->width ||
        yoffset + height > image->height ||
        zoffset + depth > image->depth ||
        data == NULL) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if ((xoffset & 3) != 0 ||
        (yoffset & 3) != 0 ||
        ((width & 3) != 0 && xoffset + width != image->width) ||
        ((height & 3) != 0 && yoffset + height != image->height)) {
        set_error(GL_INVALID_OPERATION);
        return;
    }

    expected_size = compressed_image_size_3d(width, height, depth, format);
    if (imageSize != expected_size) {
        set_error(GL_INVALID_VALUE);
        return;
    }
    if (width == 0 || height == 0 || depth == 0) {
        return;
    }

    block_size = compressed_block_size(format);
    dst_blocks_w = (image->width + 3) / 4;
    src_blocks_w = (width + 3) / 4;
    src_blocks_h = (height + 3) / 4;
    dst_block_x = xoffset / 4;
    dst_block_y = yoffset / 4;
    dst_slice_size = compressed_image_size(image->width, image->height, format);
    src_slice_size = compressed_image_size(width, height, format);

    for (GLsizei slice = 0; slice < depth; ++slice) {
        for (GLsizei row = 0; row < src_blocks_h; ++row) {
            uint8_t *dst = image->compressed_data +
                           ((size_t)zoffset + (size_t)slice) * (size_t)dst_slice_size +
                           ((size_t)(dst_block_y + row) * (size_t)dst_blocks_w + (size_t)dst_block_x) * (size_t)block_size;
            const uint8_t *src = (const uint8_t *)data +
                                 (size_t)slice * (size_t)src_slice_size +
                                 (size_t)row * (size_t)src_blocks_w * (size_t)block_size;
            memcpy(dst, src, (size_t)src_blocks_w * (size_t)block_size);
        }
    }
    sync_native_state();
}

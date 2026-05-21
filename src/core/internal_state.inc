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
static NxglPerfCounters perf_counters;

static void perf_backend_triangle(void)
{
    ++perf_counters.backend_batches;
    perf_counters.backend_vertices += 3;
}

static void perf_backend_quad(void)
{
    ++perf_counters.backend_batches;
    perf_counters.backend_vertices += 4;
}

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


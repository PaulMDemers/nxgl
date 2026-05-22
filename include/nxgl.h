#ifndef NXGL_H
#define NXGL_H

#include <stdint.h>

typedef uint32_t GLenum;
typedef uint8_t GLboolean;
typedef uint32_t GLbitfield;
typedef int8_t GLbyte;
typedef int16_t GLshort;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef uint8_t GLubyte;
typedef uint16_t GLushort;
typedef uint32_t GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef void GLvoid;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_ACCUM_BUFFER_BIT 0x00000200
#define GL_CURRENT_BIT 0x00000001
#define GL_POINT_BIT 0x00000002
#define GL_LINE_BIT 0x00000004
#define GL_POLYGON_BIT 0x00000008
#define GL_POLYGON_STIPPLE_BIT 0x00000010
#define GL_PIXEL_MODE_BIT 0x00000020
#define GL_LIGHTING_BIT 0x00000040
#define GL_FOG_BIT 0x00000080
#define GL_VIEWPORT_BIT 0x00000800
#define GL_TRANSFORM_BIT 0x00001000
#define GL_ENABLE_BIT 0x00002000
#define GL_HINT_BIT 0x00008000
#define GL_EVAL_BIT 0x00010000
#define GL_LIST_BIT 0x00020000
#define GL_TEXTURE_BIT 0x00040000
#define GL_SCISSOR_BIT 0x00080000
#define GL_ALL_ATTRIB_BITS 0x000fffff
#define GL_CLIENT_PIXEL_STORE_BIT 0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT 0x00000002
#define GL_CLIENT_ALL_ATTRIB_BITS 0xffffffff
#define GL_POINTS 0x0000
#define GL_NONE 0
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702
#define GL_MODELVIEW 0x1700
#define GL_DEPTH_TEST 0x0B71
#define GL_DEPTH_RANGE 0x0B70
#define GL_CULL_FACE 0x0B44
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_BLEND 0x0BE2
#define GL_STENCIL_TEST 0x0B90
#define GL_SCISSOR_TEST 0x0C11
#define GL_CLIP_PLANE0 0x3000
#define GL_CLIP_PLANE1 0x3001
#define GL_CLIP_PLANE2 0x3002
#define GL_CLIP_PLANE3 0x3003
#define GL_CLIP_PLANE4 0x3004
#define GL_CLIP_PLANE5 0x3005
#define GL_TEXTURE_1D 0x0DE0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_BINDING_3D 0x806A
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_TEXTURE_GEN_S 0x0C60
#define GL_TEXTURE_GEN_T 0x0C61
#define GL_TEXTURE_GEN_R 0x0C62
#define GL_TEXTURE_GEN_Q 0x0C63
#define GL_OBJECT_LINEAR 0x2401
#define GL_EYE_LINEAR 0x2400
#define GL_SPHERE_MAP 0x2402
#define GL_S 0x2000
#define GL_T 0x2001
#define GL_R 0x2002
#define GL_Q 0x2003
#define GL_TEXTURE_GEN_MODE 0x2500
#define GL_OBJECT_PLANE 0x2501
#define GL_EYE_PLANE 0x2502
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TEXTURE_ENV_COLOR 0x2201
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
#define GL_REPLACE 0x1E01
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_COMBINE_ALPHA 0x8572
#define GL_RGB_SCALE 0x8573
#define GL_ADD_SIGNED 0x8574
#define GL_INTERPOLATE 0x8575
#define GL_CONSTANT 0x8576
#define GL_PRIMARY_COLOR 0x8577
#define GL_PREVIOUS 0x8578
#define GL_SOURCE0_RGB 0x8580
#define GL_SOURCE1_RGB 0x8581
#define GL_SOURCE2_RGB 0x8582
#define GL_SOURCE0_ALPHA 0x8588
#define GL_SOURCE1_ALPHA 0x8589
#define GL_SOURCE2_ALPHA 0x858A
#define GL_OPERAND0_RGB 0x8590
#define GL_OPERAND1_RGB 0x8591
#define GL_OPERAND2_RGB 0x8592
#define GL_OPERAND0_ALPHA 0x8598
#define GL_OPERAND1_ALPHA 0x8599
#define GL_OPERAND2_ALPHA 0x859A
#define GL_SUBTRACT 0x84E7
#define GL_DOT3_RGB 0x86AE
#define GL_DOT3_RGBA 0x86AF
#define GL_POINT_SIZE 0x0B11
#define GL_POINT_SIZE_RANGE 0x0B12
#define GL_POINT_SIZE_GRANULARITY 0x0B13
#define GL_SMOOTH_POINT_SIZE_RANGE GL_POINT_SIZE_RANGE
#define GL_SMOOTH_POINT_SIZE_GRANULARITY GL_POINT_SIZE_GRANULARITY
#define GL_LINE_WIDTH 0x0B21
#define GL_LINE_WIDTH_RANGE 0x0B22
#define GL_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_SMOOTH_LINE_WIDTH_RANGE GL_LINE_WIDTH_RANGE
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY GL_LINE_WIDTH_GRANULARITY
#define GL_POLYGON_MODE 0x0B40
#define GL_LINE_STIPPLE 0x0B24
#define GL_LINE_STIPPLE_PATTERN 0x0B25
#define GL_LINE_STIPPLE_REPEAT 0x0B26
#define GL_POLYGON_STIPPLE 0x0B42
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_LEFT 0x0400
#define GL_FRONT_RIGHT 0x0401
#define GL_BACK_LEFT 0x0402
#define GL_BACK_RIGHT 0x0403
#define GL_FRONT_AND_BACK 0x0408
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_POINT 0x1B00
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01
#define GL_EDGE_FLAG 0x0B43
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT 0x0C51
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_FOG_HINT 0x0C54
#define GL_ALPHA_TEST 0x0BC0
#define GL_ALPHA_TEST_FUNC 0x0BC1
#define GL_ALPHA_TEST_REF 0x0BC2
#define GL_FOG 0x0B60
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
#define GL_LIGHTING 0x0B50
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_LIGHT2 0x4002
#define GL_LIGHT3 0x4003
#define GL_LIGHT4 0x4004
#define GL_LIGHT5 0x4005
#define GL_LIGHT6 0x4006
#define GL_LIGHT7 0x4007
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE 0x0B52
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81F8
#define GL_SINGLE_COLOR 0x81F9
#define GL_SEPARATE_SPECULAR_COLOR 0x81FA
#define GL_SPOT_DIRECTION 0x1204
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_COLOR_MATERIAL 0x0B57
#define GL_COLOR_MATERIAL_FACE 0x0B55
#define GL_COLOR_MATERIAL_PARAMETER 0x0B56
#define GL_NORMALIZE 0x0BA1
#define GL_RESCALE_NORMAL 0x803A
#define GL_MULTISAMPLE 0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE 0x809F
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_COMPILE 0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301
#define GL_LIST_BASE 0x0B32
#define GL_MAX_LIST_NESTING 0x0B31
#define GL_ATTRIB_STACK_DEPTH 0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH 0x0BB1
#define GL_RENDER 0x1C00
#define GL_FEEDBACK 0x1C01
#define GL_SELECT 0x1C02
#define GL_RENDER_MODE 0x0C40
#define GL_NAME_STACK_DEPTH 0x0D70
#define GL_SELECTION_BUFFER_SIZE 0x0DF4
#define GL_FEEDBACK_BUFFER_SIZE 0x0DF1
#define GL_PASS_THROUGH_TOKEN 0x0700
#define GL_POINT_TOKEN 0x0701
#define GL_LINE_TOKEN 0x0702
#define GL_POLYGON_TOKEN 0x0703
#define GL_BITMAP_TOKEN 0x0704
#define GL_DRAW_PIXEL_TOKEN 0x0705
#define GL_COPY_PIXEL_TOKEN 0x0706
#define GL_LINE_RESET_TOKEN 0x0707
#define GL_ACCUM_CLEAR_VALUE 0x0B80
#define GL_ACCUM 0x0100
#define GL_LOAD 0x0101
#define GL_RETURN 0x0102
#define GL_MULT 0x0103
#define GL_ADD 0x0104
#define GL_2D 0x0600
#define GL_3D 0x0601
#define GL_3D_COLOR 0x0602
#define GL_3D_COLOR_TEXTURE 0x0603
#define GL_4D_COLOR_TEXTURE 0x0604
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_TRANSPOSE_MODELVIEW_MATRIX 0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX 0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX 0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX 0x84E6
#define GL_CURRENT_COLOR 0x0B00
#define GL_CURRENT_INDEX 0x0B01
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_CURRENT_RASTER_COLOR 0x0B04
#define GL_CURRENT_RASTER_INDEX 0x0B05
#define GL_CURRENT_RASTER_TEXTURE_COORDS 0x0B06
#define GL_CURRENT_RASTER_POSITION 0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID 0x0B08
#define GL_CURRENT_RASTER_DISTANCE 0x0B09
#define GL_SHADE_MODEL 0x0B54
#define GL_MATRIX_MODE 0x0BA0
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#define GL_TEXTURE_STACK_DEPTH 0x0BA5
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_AUX_BUFFERS 0x0C00
#define GL_DRAW_BUFFER 0x0C01
#define GL_READ_BUFFER 0x0C02
#define GL_DOUBLEBUFFER 0x0C32
#define GL_STEREO 0x0C33
#define GL_RGBA_MODE 0x0C31
#define GL_CLEAR_COLOR 0x0C22
#define GL_COLOR_CLEAR_VALUE GL_CLEAR_COLOR
#define GL_INDEX_CLEAR_VALUE 0x0C20
#define GL_INDEX_MODE 0x0C30
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_ALPHA_BITS 0x0D55
#define GL_DEPTH_BITS 0x0D56
#define GL_INDEX_BITS 0x0D51
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STENCIL_BITS 0x0D57
#define GL_ACCUM_RED_BITS 0x0D58
#define GL_ACCUM_GREEN_BITS 0x0D59
#define GL_ACCUM_BLUE_BITS 0x0D5A
#define GL_ACCUM_ALPHA_BITS 0x0D5B
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC 0x0BE1
#define GL_LOGIC_OP_MODE 0x0BF0
#define GL_INDEX_LOGIC_OP 0x0BF1
#define GL_LOGIC_OP GL_INDEX_LOGIC_OP
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_ALPHA_SCALE 0x0D1C
#define GL_RED_SCALE 0x0D14
#define GL_RED_BIAS 0x0D15
#define GL_ZOOM_X 0x0D16
#define GL_ZOOM_Y 0x0D17
#define GL_GREEN_SCALE 0x0D18
#define GL_GREEN_BIAS 0x0D19
#define GL_BLUE_SCALE 0x0D1A
#define GL_BLUE_BIAS 0x0D1B
#define GL_ALPHA_BIAS 0x0D1D
#define GL_PIXEL_MAP_I_TO_I 0x0C70
#define GL_PIXEL_MAP_S_TO_S 0x0C71
#define GL_PIXEL_MAP_I_TO_R 0x0C72
#define GL_PIXEL_MAP_I_TO_G 0x0C73
#define GL_PIXEL_MAP_I_TO_B 0x0C74
#define GL_PIXEL_MAP_I_TO_A 0x0C75
#define GL_PIXEL_MAP_R_TO_R 0x0C76
#define GL_PIXEL_MAP_G_TO_G 0x0C77
#define GL_PIXEL_MAP_B_TO_B 0x0C78
#define GL_PIXEL_MAP_A_TO_A 0x0C79
#define GL_PIXEL_MAP_I_TO_I_SIZE 0x0CB0
#define GL_PIXEL_MAP_S_TO_S_SIZE 0x0CB1
#define GL_PIXEL_MAP_I_TO_R_SIZE 0x0CB2
#define GL_PIXEL_MAP_I_TO_G_SIZE 0x0CB3
#define GL_PIXEL_MAP_I_TO_B_SIZE 0x0CB4
#define GL_PIXEL_MAP_I_TO_A_SIZE 0x0CB5
#define GL_PIXEL_MAP_R_TO_R_SIZE 0x0CB6
#define GL_PIXEL_MAP_G_TO_G_SIZE 0x0CB7
#define GL_PIXEL_MAP_B_TO_B_SIZE 0x0CB8
#define GL_PIXEL_MAP_A_TO_A_SIZE 0x0CB9
#define GL_MAX_LIGHTS 0x0D31
#define GL_MAX_CLIP_PLANES 0x0D32
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_PIXEL_MAP_TABLE 0x0D34
#define GL_MAX_ATTRIB_STACK_DEPTH 0x0D35
#define GL_MAX_MODELVIEW_STACK_DEPTH 0x0D36
#define GL_MAX_NAME_STACK_DEPTH 0x0D37
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH 0x0D39
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH 0x0D3B
#define GL_MAX_EVAL_ORDER 0x0D30
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_MAX_TEXTURE_UNITS 0x84E2
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE 0x84E1
#define GL_AUTO_NORMAL 0x0D80
#define GL_MAP1_COLOR_4 0x0D90
#define GL_MAP1_INDEX 0x0D91
#define GL_MAP1_NORMAL 0x0D92
#define GL_MAP1_TEXTURE_COORD_1 0x0D93
#define GL_MAP1_TEXTURE_COORD_2 0x0D94
#define GL_MAP1_TEXTURE_COORD_3 0x0D95
#define GL_MAP1_TEXTURE_COORD_4 0x0D96
#define GL_MAP1_VERTEX_3 0x0D97
#define GL_MAP1_VERTEX_4 0x0D98
#define GL_MAP2_COLOR_4 0x0DB0
#define GL_MAP2_INDEX 0x0DB1
#define GL_MAP2_NORMAL 0x0DB2
#define GL_MAP2_TEXTURE_COORD_1 0x0DB3
#define GL_MAP2_TEXTURE_COORD_2 0x0DB4
#define GL_MAP2_TEXTURE_COORD_3 0x0DB5
#define GL_MAP2_TEXTURE_COORD_4 0x0DB6
#define GL_MAP2_VERTEX_3 0x0DB7
#define GL_MAP2_VERTEX_4 0x0DB8
#define GL_COEFF 0x0A00
#define GL_ORDER 0x0A01
#define GL_DOMAIN 0x0A02
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE_BINDING_1D 0x8068
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_PACK_SWAP_BYTES 0x0D00
#define GL_PACK_LSB_FIRST 0x0D01
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_PACK_SKIP_IMAGES 0x806B
#define GL_PACK_IMAGE_HEIGHT 0x806C
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_UNPACK_SWAP_BYTES 0x0CF0
#define GL_UNPACK_LSB_FIRST 0x0CF1
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_IMAGES 0x806D
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_VERTEX_ARRAY_SIZE 0x807A
#define GL_VERTEX_ARRAY_TYPE 0x807B
#define GL_VERTEX_ARRAY_STRIDE 0x807C
#define GL_NORMAL_ARRAY_TYPE 0x807E
#define GL_NORMAL_ARRAY_STRIDE 0x807F
#define GL_COLOR_ARRAY_SIZE 0x8081
#define GL_COLOR_ARRAY_TYPE 0x8082
#define GL_COLOR_ARRAY_STRIDE 0x8083
#define GL_TEXTURE_COORD_ARRAY_SIZE 0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE 0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE 0x808A
#define GL_VERTEX_ARRAY_POINTER 0x808E
#define GL_NORMAL_ARRAY_POINTER 0x808F
#define GL_COLOR_ARRAY_POINTER 0x8090
#define GL_TEXTURE_COORD_ARRAY_POINTER 0x8092
#define GL_V2F 0x2A20
#define GL_V3F 0x2A21
#define GL_C4UB_V2F 0x2A22
#define GL_C4UB_V3F 0x2A23
#define GL_C3F_V3F 0x2A24
#define GL_N3F_V3F 0x2A25
#define GL_C4F_N3F_V3F 0x2A26
#define GL_T2F_V3F 0x2A27
#define GL_T4F_V4F 0x2A28
#define GL_T2F_C4UB_V3F 0x2A29
#define GL_T2F_C3F_V3F 0x2A2A
#define GL_T2F_N3F_V3F 0x2A2B
#define GL_T2F_C4F_N3F_V3F 0x2A2C
#define GL_T4F_C4F_N3F_V4F 0x2A2D
#define GL_COLOR_INDEX 0x1900
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_COLOR 0x1800
#define GL_STENCIL_INDEX 0x1901
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802
#define GL_BITMAP 0x1A00
#define GL_BGRA 0x80E1
#define GL_BGR 0x80E0
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_10_10_10_2 0x8036
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_TEXTURE_PRIORITY 0x8066
#define GL_TEXTURE_RESIDENT 0x8067
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_LOD_BIAS 0x8501
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_DEPTH 0x8071
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_TEXTURE_COMPONENTS GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_BORDER 0x1005
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE 0x86A0
#define GL_TEXTURE_COMPRESSED 0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_POSITION 0x1203
#define GL_EMISSION 0x1600
#define GL_SHININESS 0x1601
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_LINEAR 0x2601
#define GL_NEAREST 0x2600
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_CLAMP 0x2900
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_ONE 1
#define GL_ZERO 0
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_CLEAR 0x1500
#define GL_AND 0x1501
#define GL_AND_REVERSE 0x1502
#define GL_COPY 0x1503
#define GL_AND_INVERTED 0x1504
#define GL_NOOP 0x1505
#define GL_XOR 0x1506
#define GL_OR 0x1507
#define GL_NOR 0x1508
#define GL_EQUIV 0x1509
#define GL_INVERT 0x150A
#define GL_OR_REVERSE 0x150B
#define GL_COPY_INVERTED 0x150C
#define GL_OR_INVERTED 0x150D
#define GL_NAND 0x150E
#define GL_SET 0x150F

int nxglInit(void);
int nxglInitFast(void);
void nxglShutdown(void);
void nxglSetCamera(float x, float y, float z, float rx, float ry, float rz);
void nxglSetDefaultReadbackEnabled(GLboolean enabled);
void nxglSetReadbackEnabled(GLboolean enabled);
void nxglSwapBuffers(const char *title, const char *detail);

typedef struct NxglPerfCounters {
    uint32_t frames;
    uint32_t clears;
    uint32_t immediate_begins;
    uint32_t array_draws;
    uint32_t element_draws;
    uint32_t display_list_calls;
    uint32_t backend_batches;
    uint32_t backend_vertices;
    uint32_t shadow_buffer_allocations;
    uint32_t shadow_buffer_allocation_bytes;
    uint32_t shadow_buffer_frees;
    uint32_t shadow_buffer_free_bytes;
    uint32_t shadow_primitives;
    uint32_t shadow_fragments;
    uint32_t read_pixels_calls;
    uint32_t draw_pixels_calls;
    uint32_t copy_pixels_calls;
    uint32_t texture_uploads;
    uint32_t texture_sub_uploads;
    uint32_t compressed_texture_uploads;
    uint32_t compressed_texture_sub_uploads;
    uint32_t backend_shader_uploads;
    uint32_t backend_shader_cache_hits;
    uint32_t backend_render_state_uploads;
    uint32_t backend_render_state_cache_hits;
    uint32_t backend_texture_stage_uploads;
    uint32_t backend_texture_stage_cache_hits;
    uint32_t backend_texture_stage_disables;
    uint32_t backend_texture_stage_disable_hits;
    uint32_t cpu_array_vertices;
    uint32_t cpu_position_transforms;
    uint32_t cpu_normal_transforms;
    uint32_t cpu_lighting_vertices;
    uint32_t cpu_fog_vertices;
    uint32_t cpu_clip_tests;
    uint32_t cpu_clip_vertices;
    uint32_t cpu_clipped_primitives;
    uint32_t backend_command_blocks;
    uint32_t backend_flush_calls;
    uint32_t backend_flush_ms;
    uint32_t backend_finish_calls;
    uint32_t backend_finish_ms;
} NxglPerfCounters;

void nxglResetPerfCounters(void);
void nxglGetPerfCounters(NxglPerfCounters *counters);

const GLubyte *glGetString(GLenum name);
GLenum glGetError(void);
GLboolean glIsEnabled(GLenum cap);
void glGetIntegerv(GLenum pname, GLint *params);
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetDoublev(GLenum pname, GLdouble *params);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetPointerv(GLenum pname, GLvoid **params);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClearIndex(GLfloat c);
void glClearDepth(GLclampf depth);
void glDepthRange(GLclampf z_near, GLclampf z_far);
void glClearStencil(GLint s);
void glClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);
void glAccum(GLenum op, GLfloat value);
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glMultMatrixf(const GLfloat *m);
void glLoadMatrixf(const GLfloat *m);
void glMultMatrixd(const GLdouble *m);
void glLoadMatrixd(const GLdouble *m);
void glMultTransposeMatrixf(const GLfloat *m);
void glLoadTransposeMatrixf(const GLfloat *m);
void glMultTransposeMatrixd(const GLdouble *m);
void glLoadTransposeMatrixd(const GLdouble *m);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble z_near, GLdouble z_far);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glTranslated(GLdouble x, GLdouble y, GLdouble z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glScaled(GLdouble x, GLdouble y, GLdouble z);
void glRotatef(GLfloat angle_degrees, GLfloat x, GLfloat y, GLfloat z);
void glRotated(GLdouble angle_degrees, GLdouble x, GLdouble y, GLdouble z);
void glColor3b(GLbyte r, GLbyte g, GLbyte b);
void glColor3bv(const GLbyte *v);
void glColor3d(GLdouble r, GLdouble g, GLdouble b);
void glColor3dv(const GLdouble *v);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor3fv(const GLfloat *v);
void glColor3i(GLint r, GLint g, GLint b);
void glColor3iv(const GLint *v);
void glColor3s(GLshort r, GLshort g, GLshort b);
void glColor3sv(const GLshort *v);
void glColor3ub(GLubyte r, GLubyte g, GLubyte b);
void glColor3ubv(const GLubyte *v);
void glColor3ui(GLuint r, GLuint g, GLuint b);
void glColor3uiv(const GLuint *v);
void glColor3us(GLushort r, GLushort g, GLushort b);
void glColor3usv(const GLushort *v);
void glColor4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a);
void glColor4bv(const GLbyte *v);
void glColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a);
void glColor4dv(const GLdouble *v);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glColor4fv(const GLfloat *v);
void glColor4i(GLint r, GLint g, GLint b, GLint a);
void glColor4iv(const GLint *v);
void glColor4s(GLshort r, GLshort g, GLshort b, GLshort a);
void glColor4sv(const GLshort *v);
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void glColor4ubv(const GLubyte *v);
void glColor4ui(GLuint r, GLuint g, GLuint b, GLuint a);
void glColor4uiv(const GLuint *v);
void glColor4us(GLushort r, GLushort g, GLushort b, GLushort a);
void glColor4usv(const GLushort *v);
void glIndexd(GLdouble c);
void glIndexdv(const GLdouble *c);
void glIndexf(GLfloat c);
void glIndexfv(const GLfloat *c);
void glIndexi(GLint c);
void glIndexiv(const GLint *c);
void glIndexs(GLshort c);
void glIndexsv(const GLshort *c);
void glIndexub(GLubyte c);
void glIndexubv(const GLubyte *c);
void glNormal3b(GLbyte x, GLbyte y, GLbyte z);
void glNormal3bv(const GLbyte *v);
void glNormal3d(GLdouble x, GLdouble y, GLdouble z);
void glNormal3dv(const GLdouble *v);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void glNormal3fv(const GLfloat *v);
void glNormal3i(GLint x, GLint y, GLint z);
void glNormal3iv(const GLint *v);
void glNormal3s(GLshort x, GLshort y, GLshort z);
void glNormal3sv(const GLshort *v);
void glTexCoord1d(GLdouble s);
void glTexCoord1dv(const GLdouble *v);
void glTexCoord1f(GLfloat s);
void glTexCoord1fv(const GLfloat *v);
void glTexCoord1i(GLint s);
void glTexCoord1iv(const GLint *v);
void glTexCoord1s(GLshort s);
void glTexCoord1sv(const GLshort *v);
void glTexCoord2d(GLdouble s, GLdouble t);
void glTexCoord2dv(const GLdouble *v);
void glTexCoord2f(GLfloat s, GLfloat t);
void glTexCoord2fv(const GLfloat *v);
void glTexCoord2i(GLint s, GLint t);
void glTexCoord2iv(const GLint *v);
void glTexCoord2s(GLshort s, GLshort t);
void glTexCoord2sv(const GLshort *v);
void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
void glTexCoord3dv(const GLdouble *v);
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
void glTexCoord3fv(const GLfloat *v);
void glTexCoord3i(GLint s, GLint t, GLint r);
void glTexCoord3iv(const GLint *v);
void glTexCoord3s(GLshort s, GLshort t, GLshort r);
void glTexCoord3sv(const GLshort *v);
void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void glTexCoord4dv(const GLdouble *v);
void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glTexCoord4fv(const GLfloat *v);
void glTexCoord4i(GLint s, GLint t, GLint r, GLint q);
void glTexCoord4iv(const GLint *v);
void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
void glTexCoord4sv(const GLshort *v);
void glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t);
void glMultiTexCoord3f(GLenum texture, GLfloat s, GLfloat t, GLfloat r);
void glTexGeni(GLenum coord, GLenum pname, GLint param);
void glTexGenf(GLenum coord, GLenum pname, GLfloat param);
void glTexGeniv(GLenum coord, GLenum pname, const GLint *params);
void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params);
void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
void glBegin(GLenum mode);
void glVertex2d(GLdouble x, GLdouble y);
void glVertex2dv(const GLdouble *v);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex2fv(const GLfloat *v);
void glVertex2i(GLint x, GLint y);
void glVertex2iv(const GLint *v);
void glVertex2s(GLshort x, GLshort y);
void glVertex2sv(const GLshort *v);
void glVertex3d(GLdouble x, GLdouble y, GLdouble z);
void glVertex3dv(const GLdouble *v);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3fv(const GLfloat *v);
void glVertex3i(GLint x, GLint y, GLint z);
void glVertex3iv(const GLint *v);
void glVertex3s(GLshort x, GLshort y, GLshort z);
void glVertex3sv(const GLshort *v);
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glVertex4dv(const GLdouble *v);
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertex4fv(const GLfloat *v);
void glVertex4i(GLint x, GLint y, GLint z, GLint w);
void glVertex4iv(const GLint *v);
void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
void glVertex4sv(const GLshort *v);
void glEnd(void);
void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void glRectdv(const GLdouble *v1, const GLdouble *v2);
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glRectfv(const GLfloat *v1, const GLfloat *v2);
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
void glRectiv(const GLint *v1, const GLint *v2);
void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void glRectsv(const GLshort *v1, const GLshort *v2);
void glPointSize(GLfloat size);
void glLineWidth(GLfloat width);
void glLineStipple(GLint factor, GLushort pattern);
void glPolygonStipple(const GLubyte *mask);
void glGetPolygonStipple(GLubyte *mask);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glAlphaFunc(GLenum func, GLclampf ref);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glLogicOp(GLenum opcode);
void glShadeModel(GLenum mode);
void glCullFace(GLenum mode);
void glFrontFace(GLenum mode);
void glHint(GLenum target, GLenum mode);
void glEdgeFlag(GLboolean flag);
void glEdgeFlagv(const GLboolean *flag);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glLightModeli(GLenum pname, GLint param);
void glLightModeliv(GLenum pname, const GLint *params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLighti(GLenum light, GLenum pname, GLint param);
void glLightiv(GLenum light, GLenum pname, const GLint *params);
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glGetLightiv(GLenum light, GLenum pname, GLint *params);
void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void glMateriali(GLenum face, GLenum pname, GLint param);
void glMaterialiv(GLenum face, GLenum pname, const GLint *params);
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void glGetMaterialiv(GLenum face, GLenum pname, GLint *params);
void glColorMaterial(GLenum face, GLenum mode);
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, GLint param);
void glFogiv(GLenum pname, const GLint *params);
void glClipPlane(GLenum plane, const GLdouble *equation);
void glGetClipPlane(GLenum plane, GLdouble *equation);
void glPixelStorei(GLenum pname, GLint param);
void glPixelZoom(GLfloat xfactor, GLfloat yfactor);
void glPixelTransferf(GLenum pname, GLfloat param);
void glPixelTransferi(GLenum pname, GLint param);
void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values);
void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values);
void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values);
void glGetPixelMapfv(GLenum map, GLfloat *values);
void glGetPixelMapuiv(GLenum map, GLuint *values);
void glGetPixelMapusv(GLenum map, GLushort *values);
void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void glGetMapfv(GLenum target, GLenum query, GLfloat *v);
void glGetMapiv(GLenum target, GLenum query, GLint *v);
void glGetMapdv(GLenum target, GLenum query, GLdouble *v);
void glEvalCoord1f(GLfloat u);
void glEvalCoord1d(GLdouble u);
void glEvalCoord2f(GLfloat u, GLfloat v);
void glEvalCoord2d(GLdouble u, GLdouble v);
void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void glEvalPoint1(GLint i);
void glEvalPoint2(GLint i, GLint j);
void glEvalMesh1(GLenum mode, GLint i1, GLint i2);
void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glRasterPos2d(GLdouble x, GLdouble y);
void glRasterPos2dv(const GLdouble *v);
void glRasterPos2f(GLfloat x, GLfloat y);
void glRasterPos2fv(const GLfloat *v);
void glRasterPos2i(GLint x, GLint y);
void glRasterPos2iv(const GLint *v);
void glRasterPos2s(GLshort x, GLshort y);
void glRasterPos2sv(const GLshort *v);
void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
void glRasterPos3dv(const GLdouble *v);
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void glRasterPos3fv(const GLfloat *v);
void glRasterPos3i(GLint x, GLint y, GLint z);
void glRasterPos3iv(const GLint *v);
void glRasterPos3s(GLshort x, GLshort y, GLshort z);
void glRasterPos3sv(const GLshort *v);
void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glRasterPos4dv(const GLdouble *v);
void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glRasterPos4fv(const GLfloat *v);
void glRasterPos4i(GLint x, GLint y, GLint z, GLint w);
void glRasterPos4iv(const GLint *v);
void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
void glRasterPos4sv(const GLshort *v);
void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void glDrawBuffer(GLenum mode);
void glReadBuffer(GLenum mode);
void glSelectBuffer(GLsizei size, GLuint *buffer);
void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
GLint glRenderMode(GLenum mode);
void glPassThrough(GLfloat token);
void glInitNames(void);
void glPushName(GLuint name);
void glPopName(void);
void glLoadName(GLuint name);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glPushAttrib(GLbitfield mask);
void glPopAttrib(void);
void glPushClientAttrib(GLbitfield mask);
void glPopClientAttrib(void);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void glStencilMask(GLuint mask);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glSampleCoverage(GLclampf value, GLboolean invert);
void glActiveTexture(GLenum texture);
void glClientActiveTexture(GLenum texture);
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
void glArrayElement(GLint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
GLuint glGenLists(GLsizei range);
GLboolean glIsList(GLuint list);
void glNewList(GLuint list, GLenum mode);
void glEndList(void);
void glCallList(GLuint list);
void glCallLists(GLsizei count, GLenum type, const GLvoid *lists);
void glDeleteLists(GLuint list, GLsizei range);
void glListBase(GLuint base);
void glGenTextures(GLsizei count, GLuint *textures);
void glDeleteTextures(GLsizei count, const GLuint *textures);
GLboolean glIsTexture(GLuint texture);
void glBindTexture(GLenum target, GLuint texture);
void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);
GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnviv(GLenum target, GLenum pname, const GLint *params);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
void glTexParameteri(GLenum target, GLenum pname, GLint value);
void glTexParameterf(GLenum target, GLenum pname, GLfloat value);
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void glGetCompressedTexImage(GLenum target, GLint level, GLvoid *pixels);
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);

#endif

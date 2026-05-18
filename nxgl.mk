NXGL_DIR ?= $(dir $(lastword $(MAKEFILE_LIST)))
NXGL_DIR := $(patsubst %/,%,$(NXGL_DIR))

NXGL_SRCS := \
	$(NXGL_DIR)/src/nxgl.c \
	$(NXGL_DIR)/src/backend/nxgl_backend.c

NXGL_CFLAGS := \
	-I$(NXGL_DIR)/include \
	-I$(NXGL_DIR)/src/backend

NXGL_SHADER_OBJS := \
	$(NXGL_DIR)/src/common3d/ps.inl \
	$(NXGL_DIR)/src/common3d/vs.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_modulate_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_replace_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_decal_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_blend_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_add_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_subtract_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_add_signed_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_interpolate_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex_vs.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex2_vs.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex2_modulate_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_cube_ps.inl \
	$(NXGL_DIR)/src/backend/nxgl_tex3d_ps.inl

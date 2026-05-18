XBE_TITLE = xbgl_45_combine_probe
GEN_XISO = $(XBE_TITLE).iso

NXDK_DIR ?= $(CURDIR)/../../../.nxdk
NXGL_DIR ?= $(CURDIR)/../..

include $(NXGL_DIR)/nxgl.mk

SRCS = \
	$(NXGL_SRCS) \
	$(CURDIR)/main.c

SHADER_OBJS = $(NXGL_SHADER_OBJS)

CFLAGS += $(NXGL_CFLAGS)

include $(NXDK_DIR)/Makefile

NXGL_DIR := $(CURDIR)
include $(NXGL_DIR)/nxgl.mk

.PHONY: all print-vars

all:
	@printf 'NXGL source package ready. Include nxgl.mk from an nxdk app to build it.\n'

print-vars:
	@printf 'NXGL_SRCS=%s\n' "$(NXGL_SRCS)"
	@printf 'NXGL_CFLAGS=%s\n' "$(NXGL_CFLAGS)"
	@printf 'NXGL_SHADER_OBJS=%s\n' "$(NXGL_SHADER_OBJS)"

NXGL_DIR := $(CURDIR)
include $(NXGL_DIR)/nxgl.mk

.PHONY: all example validation validation-autorun print-vars

all:
	@printf 'NXGL source package ready. Include nxgl.mk from an nxdk app to build it.\n'

example:
	$(MAKE) -C examples/hello_triangle

validation:
	$(MAKE) -C validation

validation-autorun:
	$(MAKE) -C validation/autorun_suite

print-vars:
	@printf 'NXGL_SRCS=%s\n' "$(NXGL_SRCS)"
	@printf 'NXGL_CFLAGS=%s\n' "$(NXGL_CFLAGS)"
	@printf 'NXGL_SHADER_OBJS=%s\n' "$(NXGL_SHADER_OBJS)"

XBE_TITLE = xbgl_110_autorun_suite_v$(AUTORUN_VERSION)
GEN_XISO = $(XBE_TITLE).iso
.DEFAULT_GOAL := all

AUTORUN_VERSION = 23
AUTORUN_BUILD_SUFFIX = fast-readback
AUTORUN_BUILD_LABEL = 2026-05-16-v$(AUTORUN_VERSION)-$(AUTORUN_BUILD_SUFFIX)
AUTORUN_VERSION_HEADER = generated/autorun_version.h

NXDK_DIR ?= $(CURDIR)/../../../.nxdk
NXGL_DIR ?= $(CURDIR)/../..

include $(NXGL_DIR)/nxgl.mk

CFLAGS += -I$(CURDIR) $(NXGL_CFLAGS)
CFLAGS += -DAUTORUN_BUILD_LABEL=\"$(AUTORUN_BUILD_LABEL)\"
CFLAGS += -DAUTORUN_EXPECT_LINE_ACCEPTED=1

SHADER_OBJS = $(NXGL_SHADER_OBJS)

generated/generated_sources.mk generated/probe_table.inc: $(NXGL_DIR)/tools/generate_nxgl_autorun_suite.sh
	sh $(NXGL_DIR)/tools/generate_nxgl_autorun_suite.sh

$(AUTORUN_VERSION_HEADER): Makefile
	@mkdir -p generated
	@printf '#pragma once\n#define AUTORUN_VERSION %s\n#define AUTORUN_BUILD_LABEL "%s"\n#define AUTORUN_REPORT_E_PATH "E:\\\\nxgl_report_v%s.txt"\n#define AUTORUN_REPORT_D_PATH "D:\\\\nxgl_report_v%s.txt"\n' "$(AUTORUN_VERSION)" "$(AUTORUN_BUILD_LABEL)" "$(AUTORUN_VERSION)" "$(AUTORUN_VERSION)" > $@

$(CURDIR)/main.c: $(AUTORUN_VERSION_HEADER)

include generated/generated_sources.mk

SRCS = \
	$(NXGL_SRCS) \
	$(CURDIR)/main.c \
	$(AUTORUN_PROBE_SRCS)

include $(NXDK_DIR)/Makefile

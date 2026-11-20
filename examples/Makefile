ifndef DPP_ROOT
$(error DPP_ROOT is undefined)
endif

ifndef KMEM_ROOT
$(error KMEM_ROOT is undefined)
endif

ifndef BUILD_DIR
BUILD_DIR := .
endif

ifndef RADAR_BUILD_DIR
RADAR_BUILD_DIR := ../build-examples-mingw
endif

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
RADAR_SRC_DIR := $(abspath $(dir $(THIS_MAKEFILE))/..)
IMGUI_INC_DIR := $(RADAR_SRC_DIR)/deps/imgui
CUSTOM_CFLAGS := -MMD -MP -I$(IMGUI_INC_DIR)

include $(DPP_ROOT)/Makefile.inc
include $(KMEM_ROOT)/Makefile.inc

radar_path_exists = \
	$(if $(realpath $1),, \
	$(error ERROR: $1 does not exist, run `build-mingw-with-examples.sh` first.))

ifndef BUILD_CPP_OBJECT
$(error BUILD_CPP_OBJECT not defined, check DPP_ROOT)
endif

$(call radar_path_exists,$(RADAR_BUILD_DIR)/libradar.a)
$(call radar_path_exists,$(RADAR_BUILD_DIR)/libimgui.a)

ifndef LIBKMEM
$(error LIBKMEM not defined, check KMEM_ROOT)
endif

ifndef LIBKMEM_USER
$(error LIBKMEM_USER not defined, check KMEM_ROOT)
endif

RADAR_TEST_FAKE_GAME = fake-game
RADAR_TEST_FAKE_GAME_OBJECTS = $(BUILD_DIR)/$(RADAR_TEST_FAKE_GAME).o
RADAR_TEST_FAKE_GAME_TARGET = $(BUILD_DIR)/$(RADAR_TEST_FAKE_GAME).exe
CFLAGS_$(RADAR_TEST_FAKE_GAME).o = -DUSE_EA_STL=1

RADAR_TEST_DRIVER = radar-driver
RADAR_TEST_DRIVER_OBJECTS = $(BUILD_DIR)/$(RADAR_TEST_DRIVER).o
RADAR_TEST_DRIVER_TARGET = $(BUILD_DIR)/$(RADAR_TEST_DRIVER).sys
LDFLAGS_$(RADAR_TEST_DRIVER).sys = $(LIBKMEM)

RADAR_TEST_IMGUI = radar
RADAR_TEST_IMGUI_OBJECTS = $(BUILD_DIR)/$(RADAR_TEST_IMGUI).o
RADAR_TEST_IMGUI_TARGET = $(BUILD_DIR)/$(RADAR_TEST_IMGUI).exe
CFLAGS_$(RADAR_TEST_IMGUI).o = -I$(RADAR_SRC_DIR)
LDFLAGS_$(RADAR_TEST_IMGUI).exe = $(LIBKMEM_USER) \
	$(RADAR_BUILD_DIR)/libradar.a $(RADAR_BUILD_DIR)/libimgui.a \
	$(RADAR_BUILD_DIR)/deps/glfw/src/libglfw3.a -lopengl32 -lgdi32

RADAR_LIBS = $(RADAR_BUILD_DIR)/libradar.a $(RADAR_BUILD_DIR)/libimgui.a \
	$(RADAR_BUILD_DIR)/deps/glfw/src/libglfw3.a

all: $(RADAR_TEST_FAKE_GAME_TARGET) $(RADAR_TEST_DRIVER_TARGET) $(RADAR_TEST_IMGUI_TARGET)

$(BUILD_DIR)/%.o: %.cpp
	$(call BUILD_CPP_OBJECT,$<,$(notdir $@),$(BUILD_DIR))

$(RADAR_TEST_FAKE_GAME_TARGET): $(RADAR_TEST_FAKE_GAME_OBJECTS)
	$(call LINK_CPP_USER_TARGET,$(notdir $^),$(notdir $@),$(BUILD_DIR))

$(RADAR_TEST_DRIVER_TARGET): $(LIBKMEM) $(RADAR_TEST_DRIVER_OBJECTS)
	$(call LINK_CPP_KERNEL_TARGET,$(notdir $(RADAR_TEST_DRIVER_OBJECTS)),$(notdir $@),$(BUILD_DIR))

$(RADAR_TEST_IMGUI_TARGET): $(LIBKMEM_USER) $(RADAR_LIBS) $(RADAR_TEST_IMGUI_OBJECTS)
	$(call LINK_CPP_USER_TARGET,$(notdir $(RADAR_TEST_IMGUI_OBJECTS)),$(notdir $@),$(BUILD_DIR))

install: $(RADAR_TEST_FAKE_GAME_TARGET) $(RADAR_TEST_DRIVER_TARGET) $(RADAR_TEST_IMGUI_TARGET)
	$(INSTALL) -d '$(DESTDIR)/'
	$(CP) '$(RADAR_TEST_FAKE_GAME_TARGET)' $(DESTDIR)
	$(call INSTALL_EXEC_SIGN,$(RADAR_TEST_DRIVER_TARGET))
	$(CP) '$(RADAR_TEST_DRIVER).bat' $(DESTDIR)
	$(CP) '$(RADAR_TEST_IMGUI_TARGET)' $(DESTDIR)

.PHONY: all install
.DEFAULT_GOAL := all

-include $(BUILD_DIR)/*.d

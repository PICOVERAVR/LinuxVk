# make -d prints debug info
# makefile rules are specified here: https://www.gnu.org/software/make/manual/html_node/Rule-Syntax.html

# use all cores if possible
MAKEFLAGS += -j $(shell nproc)

CXX := clang++
LD := $(CXX)
DB := lldb

# directories to search for .cpp files
SRC_DIRS := gfx-support src imgui

# directories to search for .h files
INC_DIRS := $(SRC_DIRS) imgui/backends

# including some manually specified sources since we can't build for every single target imgui offers
MANUAL_SRCS := imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_vulkan.cpp

# all source files
SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp)) $(MANUAL_SRCS)

# directories to search for includes (which may not be all source directories)
INCS := $(foreach dir,$(INC_DIRS),-I$(dir))

# create object files and dependancy files in hidden dirs
OBJDIR := .obj
DEPDIR := .dep

LIBS := glfw3 assimp glm vulkan
LIB_CFLAGS := $(shell pkg-config --cflags $(LIBS))
LIB_LDFLAGS := $(shell pkg-config --libs $(LIBS))

# generate dependancy information, and stick it in depdir
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

CFLAGS := -Wall -Wextra -std=c++17 $(INCS) $(LIB_CFLAGS)
LDFLAGS := -lpthread $(LIB_LDFLAGS)

# if any word (delimited by whitespace) of SRCS (excluding suffix) matches the wildcard '%', put it in the object or dep directory
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))
DEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

# make hidden subdirectories
$(shell mkdir -p $(dir $(OBJS)) > /dev/null)
$(shell mkdir -p $(dir $(DEPS)) > /dev/null)

.PHONY: default clean spv
BINS := dbg opt small asan tsan

default: dbg

# tuned debug info, basic optimization
dbg: CFLAGS += -g$(DB) -Og

# check for memory leaks using clang's address sanitizer (much faster than Valgrind!)
asan: CFLAGS += -g$(DB) -Og -fsanitize=address -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address

# check for race conditions between threads using clang's thread sanitizer
tsan: CFLAGS += -g$(DB) -Og -fsanitize=thread
tsan: LDFLAGS += -fsanitize=thread

# fastest executable on current machine
opt: CFLAGS += -Ofast -march=native -ffast-math -flto=thin -DNDEBUG
opt: LDFLAGS += -flto=thin

# smallest executable on current machine
small: CFLAGS += -Oz -march=native -ffast-math -flto=thin -DNDEBUG
small: LDFLAGS += -flto=thin -s

# clean out intermediate files
clean:
	@rm -f $(BINS)
	@rm -rf .dep .obj .spv

# build shaders
spv:
	@cd shader && $(MAKE) -s

# link executable together using object files in OBJDIR
$(BINS): $(OBJS)
	@$(LD) -o $@ $(LDFLAGS) $^
	@echo linked $@

# if a dep file is available, include it as a dependancy
# when including the dep file, don't let the timestamp of the file determine if we remake the target since the dep
# is updated after the target is built
$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp | $(DEPDIR)/%.d
	@$(CXX) -c -o $@ $< $(CFLAGS) $(DEPFLAGS)
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d
	@echo built $(notdir $@)

# dep files are not deleted if make dies
.PRECIOUS: $(DEPDIR)/%.d

# empty dep for deps
$(DEPDIR)/%.d: ;

# read .d files if nothing else matches, ok if deps don't exist...?
-include $(DEPS)

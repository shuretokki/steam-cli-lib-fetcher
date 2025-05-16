# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I$(SRCDIR)/include -I$(64WINDOWS_DIR)/include -I$(64STATIC_DIR)/include -static-libgcc -static-libstdc++
LDFLAGS = -L$(64STATIC_DIR)/lib -lfmt -lws2_32

# Makefile settings
APPNAME = steamlibfetcher
APPDIR = steam-lib-fetcher
SRCDIR = src
OBJDIR = obj
EXT = .cpp
HOME =  C:/Users/Administrator
VCPKG_ROOT ?= $(HOME)/vcpkg/installed
64STATIC_DIR = $(VCPKG_ROOT)/x64-mingw-static
64DYNAMIC_DIR = $(VCPKG_ROOT)/x64-mingw-dynamic
64WINDOWS_DIR = $(VCPKG_ROOT)/x64-windows
OTHER_LIBS = # (e.g., -lotherlib1 -lotherlib2)

# Add other libraries to LDFLAGS
LDFLAGS += $(OTHER_LIBS)

# Support for debug and release builds
BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE),debug)
	CXXFLAGS += -O0 -g
else ifeq ($(BUILD_TYPE),release)
	CXXFLAGS += -O3
endif

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)
DEP = $(OBJ:%.o=%.d)
%.d: $(SRCDIR)/%$(EXT) | $(OBJDIR)
	@$(CC) $(CXXFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$(OBJDIR)/$@


		
# UNIX-based OS variables & settings
RM = rm
DELOBJ = $(OBJ)
# Windows OS variables & settings
DEL = del
EXE = .exe
WDELOBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)\%.o)

########################################################################
####################### Targets beginning here #########################
########################################################################

.PHONY: all
all: $(APPNAME)

# Builds the app
$(APPNAME): $(OBJ) | $(APPDIR)
	$(CXX) $(CXXFLAGS) -o $(APPDIR)/$@ $^ $(LDFLAGS)

# Create app directory
$(APPDIR):
	mkdir $(APPDIR)

# Create object directory
$(OBJDIR):
	mkdir $(OBJDIR)

# Build object files
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

# Generate dependency files
$(OBJDIR)/%.d: $(SRCDIR)/%$(EXT) | $(OBJDIR)
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@
	
.PHONY: run
run: $(APPNAME)
	./$(APPNAME)$(EXE)

.PHONY: run-wt
run-wt: $(APPNAME)
	wt -w 0 -d . powershell -Command .\\$(APPNAME)$(EXE)

# Creates the dependency rules
%.d: $(SRCDIR)/%$(EXT)
	@$(CC) $(CXXFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@

# Includes all .h files
-include $(DEP)

# Building rule for .o files
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

################### Cleaning rules for Unix-based OS ###################
.PHONY: clean
clean:
	$(RM) -rf $(DELOBJ) $(DEP) $(APPNAME)

.PHONY: cleandep
cleandep:
	$(RM) $(DEP)

#################### Cleaning rules for Windows OS #####################
.PHONY: cleanw
cleanw:
	$(DEL) $(subst /,\,$(OBJDIR)\*.o) $(subst /,\,$(OBJDIR)\*.d) $(APPNAME)$(EXE)

.PHONY: cleandepw
cleandepw:
	$(DEL) $(subst /,\,$(OBJDIR)\*.d)

.PHONY: help
help:
	@echo "Makefile for $(APPNAME)"
	@echo "Usage:"
	@echo "  make [all]		 		Build the application"
	@echo "  make run		   		Build and run the application"
	@echo "  make clean		 		Remove build artifacts"
	@echo "  make install	   			Install the application to $(BINDIR)"
	@echo "  make BUILD_TYPE=release  		Build with release optimizations"

	
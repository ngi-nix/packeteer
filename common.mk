# Always include top-level source directory
AM_CXXFLAGS = -I$(top_srcdir) -I$(top_builddir) @AM_CXXFLAGS@

# Set default compiler flags
AM_CXXFLAGS += -g -std=c++0x -Wc++0x-compat \
	-DMETA_CXX_MODE=META_CXX_MODE_CXX0X \
	-DTWINE_CXX_MODE=TWINE_CXX_MODE_CXX0X

# Dependencies XXX keep these in sync with packeteer.pc.in
AM_CXXFLAGS += $(META_CFLAGS) $(TWINE_CFLAGS)
# AM_LDFLAGS += 

# Build object files in subdirectories so we can support platform-dependent
# builds
AUTOMAKE_OPTIONS = subdir-objects

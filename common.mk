# Always include top-level source directory
AM_CXXFLAGS = -I$(top_srcdir) @AM_CXXFLAGS@

# Set default compiler flags
AM_CXXFLAGS += -g -std=c++0x -Wc++0x-compat

# Dependencies XXX keep these in sync with packetflinger.pc.in
AM_CXXFLAGS += $(META_CFLAGS) $(TWINE_CFLAGS)
# AM_LDFLAGS += 

# Build object files in subdirectories so we can support platform-dependent
# builds
AUTOMAKE_OPTIONS = subdir-objects

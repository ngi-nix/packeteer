# Always include top-level source directory
AM_CXXFLAGS = -I$(top_srcdir) @AM_CXXFLAGS@

# Set default compiler flags
AM_CXXFLAGS += -g --std=c++0x

# Set debugging-related options.
if DEBUG
AM_CXXFLAGS += -DDEBUG -O0
else
AM_CXXFLAGS += -DNDEBUG -O3
endif

# Dependencies XXX keep these in sync with packetflinger.pc.in
AM_CXXFLAGS += $(BOOST_CPPFLAGS)
# AM_LDFLAGS += $(BOOST_SIGNALS_LIB)

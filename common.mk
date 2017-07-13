# Ignore Xcode's setting since the SDK may contain older versions of Clang and libc++.
unexport SDKROOT

CC			?= gcc
CXX			?= g++
GCOVR		?= gcovr
MKDIR		?= mkdir
CLOC		?= cloc
GENGETOPT	?= gengetopt

BOOST_IOSTREAMS_LIB ?= -lboost_iostreams-mt

WARNING_FLAGS	?=
WARNING_FLAGS	+= \
	-Wall -Werror -Wno-unused -Wno-missing-braces -Wstrict-aliasing \
	-Wno-deprecated-declarations -Wno-sign-compare # for Bandit

LOCAL_CXXFLAGS		?= -std=c++14
OPT_FLAGS			?=
OPT_FLAGS			+= -fstrict-aliasing -fstrict-overflow -g
OPT_FLAGS_DEBUG		?= -O0
OPT_FLAGS_RELEASE	?= -march=native -O2 -ftree-vectorize -foptimize-sibling-calls
CPP_FLAGS_DEBUG		= -DSDSL_DEBUG
CPP_FLAGS_RELEASE	= -DNDEBUG

CFLAGS		= -c -std=c99 $(OPT_FLAGS) $(WARNING_FLAGS)
CXXFLAGS	= -c $(OPT_FLAGS) $(LOCAL_CXXFLAGS) $(WARNING_FLAGS)
CPPFLAGS	=	-DMODE_TI \
				-DHAVE_ATTRIBUTE_HOT \
				-DHAVE_ATTRIBUTE_PURE \
				-DHAVE_ATTRIBUTE_CONST \
				-I../include \
				-I../../lib/aho_corasick/src \
				-I../../lib/bandit \
				-I../../lib/range-v3/include \
				-I../../lib/sdsl/build/include \
				-I../../lib/sdsl/build/external/libdivsufsort/include \
				$(LOCAL_CPPFLAGS)

ifeq ($(shell uname -s),Linux)
	CPPFLAGS	+=	-I../../lib/libdispatch
endif

LDFLAGS		=	-L../../lib/sdsl/build/lib \
				-L../../lib/sdsl/build/external/libdivsufsort/lib \
				$(OPT_FLAGS) $(LOCAL_LDFLAGS) -ldivsufsort -ldivsufsort64 -lsdsl

ifeq ($(BUILD_STYLE),release)
	OPT_FLAGS	+= $(OPT_FLAGS_RELEASE)
	CPPFLAGS	+= $(CPP_FLAGS_RELEASE)
else
	OPT_FLAGS	+= $(OPT_FLAGS_DEBUG)
	CPPFLAGS	+= $(CPP_FLAGS_DEBUG)
endif

ifeq ($(EXTRA_WARNINGS),1)
	WARNING_FLAGS += -Wextra
endif


%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.c: %.ggo
	$(GENGETOPT) --input="$<"

%.cc: %.rl
	$(RAGEL) -G2 -o $@ $<

%.pdf: %.dot
	$(DOT) -Tpdf -o$@ $<

%.dot: %.rl
	$(RAGEL) -V -p -o $@ $<

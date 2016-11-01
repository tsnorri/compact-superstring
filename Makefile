include local.mk
include common.mk

PROJECT_DIR = tribble

DEPENDENCIES = lib/sdsl/build/lib/libsdsl.a
ifeq ($(shell uname -s),Linux)
	DEPENDENCIES    +=  lib/libdispatch/libdispatch-build/src/libdispatch.a
endif


.PHONY: all clean-all clean clean-dependencies dependencies cloc

all: dependencies
	$(MAKE) -C $(PROJECT_DIR)

clean-all: clean clean-dependencies

clean:
	$(MAKE) -C $(PROJECT_DIR) clean

clean-dependencies:
	cd lib/sdsl/build && ./clean.sh

dependencies: $(DEPENDENCIES)

cloc:
	$(CLOC) \
		$(PROJECT_DIR)/include

lib/sdsl/build/lib/libsdsl.a:
	cd lib/sdsl/build && ./clean.sh && \
	CC="$(CC)" \
	CXX="$(CXX)" \
	CFLAGS="$(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) $(OPT_FLAGS)" \
	CXXFLAGS="$(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(OPT_FLAGS)" \
	LDFLAGS="$(LOCAL_LDFLAGS) $(OPT_FLAGS)" \
	cmake ..
	$(MAKE) -C lib/sdsl/build VERBOSE=1

lib/libdispatch/libdispatch-build/src/libdispatch.a:
	rm -rf lib/libdispatch/libdispatch-build && \
	cd lib/libdispatch && \
	mkdir libdispatch-build && \
	cd libdispatch-build && \
	../configure --cc="$(CC)" --c++="$(CXX)" --release
	$(MAKE) -C lib/libdispatch/libdispatch-build VERBOSE=1

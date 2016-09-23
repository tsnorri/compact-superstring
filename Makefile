include local.mk
include common.mk

PROJECT_DIR = tribble

.PHONY: all clean-all clean clean-dependencies dependencies cloc

all: dependencies
	$(MAKE) -C $(PROJECT_DIR)

clean-all: clean clean-dependencies

clean:
	$(MAKE) -C $(PROJECT_DIR) clean

clean-dependencies:
	cd lib/sdsl/build && ./clean.sh

dependencies: lib/sdsl/build/lib/libsdsl.a

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

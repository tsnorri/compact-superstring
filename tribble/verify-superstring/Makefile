include ../../local.mk
include ../../common.mk

TARGET			=	verify-superstring

# FIXME: dispatch, pthread, pthread_workqueue and kqueue only for Linux.
LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB) \
					../src/libtribble.a \
					../../lib/libdispatch/libdispatch-build/src/libdispatch.a \
					-lkqueue \
					-lpthread \
					-lpthread_workqueue

# CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	cmdline.o \
					main.o \
					verify_superstring.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

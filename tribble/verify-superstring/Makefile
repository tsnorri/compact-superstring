include ../../local.mk
include ../../common.mk

TARGET			=	verify-superstring

LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB) ../src/libtribble.a
# CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	cmdline.o \
					main.o \
					verify_superstring.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

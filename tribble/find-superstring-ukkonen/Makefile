include ../../local.mk
include ../../common.mk

TARGET			=	find-superstring-ukkonen

LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB) ../src/libtribble.a
# CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	cmdline.o \
					find_superstring_ukkonen.o \
					main.o \
					process_strings.o \
					read_input.o


all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

main.o: cmdline.c

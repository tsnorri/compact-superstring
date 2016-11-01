include ../../local.mk
include ../../common.mk

TARGET			=	find-superstring

LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB) ../src/libtribble.a
# CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	check_non_unique_strings.o \
					cmdline.o \
					create_index.o \
					find_suffixes.o \
					find_superstring.o \
					main.o \
					superstring_callback.o \
					visualize.o \
					union_find.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

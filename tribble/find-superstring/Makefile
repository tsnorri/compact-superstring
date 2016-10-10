include ../../local.mk
include ../../common.mk

TARGET			=	find-superstring

LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB)
# CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	cmdline.o \
					create_index.o \
					find_suffixes.o \
					main.o \
					sort_strings.o \
					superstring_callback.o \
					vector_source.o \
					visualize.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

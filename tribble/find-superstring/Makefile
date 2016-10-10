include ../../local.mk
include ../../common.mk

TARGET			=	find-superstring

LDFLAGS			+=	$(BOOST_IOSTREAMS_LIB)
CPPFLAGS		+=	-DDEBUGGING_OUTPUT

OBJECTS			=	main.o \
					cmdline.o \
					create_index.o \
					find_suffixes.o \
					sort_strings.o \
					vector_source.o


all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

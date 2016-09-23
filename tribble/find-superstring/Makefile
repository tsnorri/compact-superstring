include ../../local.mk
include ../../common.mk

TARGET			=	find-superstring

LDFLAGS			+=	-lboost_iostreams-mt

OBJECTS			=	main.o \
					cmdline.o \
					create_index.o \
					find_superstring.o \
					vector_source.o


all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJECTS)

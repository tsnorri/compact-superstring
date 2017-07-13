include ../../local.mk
include ../../common.mk

OBJECTS		=	io.o \
				vector_source.o \
				union_find.o

TARGET		=	libtribble.a

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	ar cru $(TARGET) $(OBJECTS)

include ../../local.mk
include ../../common.mk

TARGET			=	gen-repetitive

OBJECTS			=	gen_repetitive.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

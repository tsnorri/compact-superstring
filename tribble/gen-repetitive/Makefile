all:
	g++ gen_repetitive.cpp --std=c++11 -o gen_repetitive



include ../../local.mk
include ../../common.mk

TARGET			=	gen-repetitive

OBJECTS			=	gen_repetitive.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(eval OS := $(shell uname))
TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++17 -g $(pkg-config --cflags glfw3)
INC+=-I../inc -I../deps/xmath.h/inc -I../deps/libpng -I../deps/opengametools/src -I../deps/zlib -I../deps/sha1
LIB+=-L../lib/$(TARGET)/ -L../ex_libs/$(TARGET)/png -L../ex_libs/$(TARGET)/zlib -L../deps/sha1/lib/$(TARGET)
LINK+=-lg

ifeq ($(OS),Darwin)
	LINK += -lm -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
	CXXFLAGS += -DGL_SILENCE_DEPRECATION
else
	CXXFLAGS += -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES
ifeq ($(TARGET),wasm32-unknown-emscripten)
	LINK +=-s USE_GLFW=3 -lglfw3 -lGL -lrt -lm -ldl
else
	LINK+=-lglfw -lGL -lrt -lm -ldl -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor
endif

endif

LINK+=-lpthread ../ex_libs/$(TARGET)/png/libpng16.a -lz
SRCS=$(wildcard *.cpp)

../lib/$(TARGET)/libg.a:
	make -C .. static

bin:
	mkdir -p $@

bin/%: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ $(LINK)

bin/%.html: %.cpp bin ../lib/$(TARGET)/libg.a
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) $< -o $@ --preload-file data $(LINK)

.PHONEY: all clean

all:
	@echo "Built all examples"

clean:
	rm -rf bin

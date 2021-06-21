#    ___          _        _    __   __
#   | _ \_ _ ___ (_)___ __| |_  \ \ / /_ _ _ _ ___
#   |  _/ '_/ _ \| / -_) _|  _|  \ V / _` | '_(_-<
#   |_| |_| \___// \___\__|\__|   \_/\__,_|_| /__/
#              |__/
PROJECT=wandernaught
TARGET=$(shell ${CC} -dumpmachine)
$(eval OS := $(shell uname))

G_DEPS=$(BUILD_PATH)/gitman_sources
SRC_OBJS=$(patsubst src/%.cpp,obj/$(TARGET)/%.cpp.o,$(wildcard src/*.c*))
INC+=-I$(BUILD_PATH)/inc -I$(BUILD_PATH)/gitman_sources/g.h/inc -I$(G_DEPS)/xmath.h/inc -I$(G_DEPS)/opengametools/src -I$(G_DEPS)/libpng
LIB+=-L$(BUILD_PATH)/gitman_sources/g.h/lib/$(TARGET) -L$(G_DEPS)/lib/$(TARGET)/ 
LIB+=
CXXFLAGS+=-Wall -g -std=c++11
LINK+=-lg -lm

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

LINK+=-lpthread

TARGET=$(shell $(CXX) -dumpmachine)

CXXFLAGS+=-std=c++11 -g
CXXFLAGS+=-D_XOPEN_SOURCE=500 -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES -DGL_SILENCE_DEPRECATION
INC+=-I./inc -Igitman_sources/xmath.h/inc -Igitman_sources/png -Igitman_sources/sha1 -Igitman_sources/opengametools/src
LIB+=-Ldeps/libpng -Ldeps/sha1/lib/$(TARGET)
LINK+=-lpng -s MAX_WEBGL_VERSION=2
SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp,out/$(TARGET)/%.cpp.o,$(wildcard src/*.cpp))
OBJS+=$(wildcard gitman_sources/png/*.o)
OBJS+=$(wildcard gitman_sources/zlib/*.o)

lib/$(TARGET):
	mkdir -p $@

out/$(TARGET):
	mkdir -p $@

gitman_sources:
	gitman install

out/$(TARGET)/%.cpp.o: src/%.cpp out/$(TARGET) lib/$(TARGET) deps
	$(CXX) $(CXXFLAGS) $(INC) $(LIB) -c $< -o $@

lib/$(TARGET)/libg.a: $(OBJS)
	$(AR) rcs $@ $^

.PHONEY: clean CLEAN static deps what
clean:
	rm -rf lib out ex_libs

CLEAN:
	rm -rf gitman_sources deps lib out ex_libs

what:
	@echo $(OBJS)

static: lib/$(TARGET)/libg.a
	@echo "Built libg.a"

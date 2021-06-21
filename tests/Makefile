BUILD_PATH=..
include ../vars.mk
INC+=
LIB+=
LINK+=

TESTS=$(patsubst %.cpp,bin/%,$(wildcard *.cpp))


bin:
	mkdir -p bin

bin/%: %.cpp bin
	$(CXX) $(CXXFLAGS) $(LIB) $(INC) $(LIBS) $< -o $@ $(LINK)

.PHONY: test clean 
test: $(TESTS)
	@./.test-runner.py

clean:
	rm -rf bin/



CPPFLAGS := -std=c++11
ifeq ($(SNIPPET),true)
	CFLAGS += -Wno-unused-function
endif
CPPFLAGS += -O3 -flto
BUILD_DIR := ./
HEADERS := $(wildcard ./*.hpp)
SOURCES := $(wildcard ./*.cpp)
OBJECTS := $(addprefix engel/, $(notdir $(SOURCES:.cpp=.o)))
engel: $(OBJECTS)
	@ printf "%8s %-40s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p build
	@ $(CC) $(CPPFLAGS) $^ -o $@.
engel/%.o: src/%.cpp $(HEADERS)
	@ printf "%8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p engel
	@ $(CC) -c $(CPPFLAGS) -o $@ $<

.PHONY: default

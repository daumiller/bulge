C_COMPILER = clang
C_FLAGS    = -std=c99

LIBRARY_TOOL  = ar
LIBRARY_FLAGS = rcs

RELEASE_OBJECTS = source/table-accessor.o \
                  source/entry-accessor.o \
                  source/path.o           \
                  source/filesystem.o
DEBUG_OBJECTS   = $(RELEASE_OBJECTS:.o=.debug.o)

RELEASE_LIBRARY = libbulge.a
DEBUG_LIBRARY   = libbulge-debug.a

# ===============================================

release: $(RELEASE_LIBRARY)

debug: $(DEBUG_LIBRARY)

all: release

# ===============================================

%.debug.o: %.c
	$(C_COMPILER) $(C_FLAGS) -g -c $^ -o $@

%.o: %.c
	$(C_COMPILER) $(C_FLAGS) -c $^ -o $@

$(RELEASE_LIBRARY): $(RELEASE_OBJECTS)
	$(LIBRARY_TOOL) $(LIBRARY_FLAGS) $@ $^

$(DEBUG_LIBRARY): $(DEBUG_OBJECTS)
	$(LIBRARY_TOOL) $(LIBRARY_FLAGS) $@ $^

# ===============================================

tests/filesystem: $(RELEASE_LIBRARY) tests/filesystem.c
	$(C_COMPILER) $(C_FLAGS) $^ -L./ -lbulge -o $@

tests/test-filesystem-debug: $(DEBUG_LIBRARY) test-filesystem.c
	$(C_COMPILER) $(C_FLAGS) -g $^ -L./ -lbulge-debug -o $@

tests: tests/filesystem

tests-debug: tests/filesystem-debug

# ===============================================

clean:
	rm -f $(RELEASE_OBJECTS)
	rm -f $(DEBUG_OBJECTS)
	rm -rf ./*.dSYM

veryclean: clean
	rm -f $(RELEASE_LIBRARY)
	rm -f $(DEBUG_LIBRARY)
	rm -f tests/filesystem
	rm -f tests/filesystem-debug

remake: veryclean all

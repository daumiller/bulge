# comment this out for big-endian builds
ENDIANNESS = -DBULGE_LITTLE_ENDIAN

# comment this out to build without string descriptions of error codes
ERROR_STRINGS = -DBULGE_ERROR_STRINGS

C_COMPILER = clang
C_FLAGS    = -std=c99 -I./ $(ENDIANNESS) $(ERROR_STRINGS)

LIBRARY_TOOL  = ar
LIBRARY_FLAGS = rcs

RELEASE_OBJECTS = source/table-accessor.o \
                  source/entry-accessor.o \
                  source/path.o           \
                  source/filesystem.o     \
                  source/path-separator.o \
                  source/utility.o        \
                  source/error.o
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

test/filesystem: $(RELEASE_LIBRARY) test/filesystem.c
	$(C_COMPILER) $(C_FLAGS) $^ -L./ -lbulge -o $@

test/filesystem-debug: $(DEBUG_LIBRARY) test/filesystem.c
	$(C_COMPILER) $(C_FLAGS) -g $^ -L./ -lbulge-debug -o $@

tests: test/filesystem

tests-debug: test/filesystem-debug

# ===============================================

clean:
	rm -f $(RELEASE_OBJECTS)
	rm -f $(DEBUG_OBJECTS)
	rm -rf ./*.dSYM

veryclean: clean
	rm -f $(RELEASE_LIBRARY)
	rm -f $(DEBUG_LIBRARY)
	rm -f test/filesystem
	rm -f test/filesystem-debug

remake: veryclean all

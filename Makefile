C_TOOL  = clang
C_FLAGS = -std=c99

LIBRARY_TOOL  = ar
LIBRARY_FLAGS = rcs

HEADER_FILES = storage-types.h  \
               table-accessor.h \
               entry-accessor.h \
               path.h           \
               filesystem.h

RELEASE_OBJECTS = table-accessor.o \
                  entry-accessor.o \
                  path.o           \
                  filesystem.o
DEBUG_OBJECTS   = $(RELEASE_OBJECTS:.o=.debug.o)

RELEASE_LIBRARY = libbulge.a
DEBUG_LIBRARY   = libbulge-debug.a

# ===============================================

release: $(RELEASE_LIBRARY)

debug: $(DEBUG_LIBRARY)

all: release

# ===============================================

%.debug.o: %.c
	$(C_TOOL) $(C_FLAGS) -g -c $^ -o $@

%.o: %.c
	$(C_TOOL) $(C_FLAGS) -c $^ -o $@

$(RELEASE_LIBRARY): $(RELEASE_OBJECTS)
	$(LIBRARY_TOOL) $(LIBRARY_FLAGS) $@ $^

$(DEBUG_LIBRARY): $(DEBUG_OBJECTS)
	$(LIBRARY_TOOL) $(LIBRARY_FLAGS) $@ $^

# ===============================================

test-filesystem: $(RELEASE_LIBRARY) test-filesystem.c
	$(C_TOOL) $(C_FLAGS) $^ -L./ -lbulge -o $@

test-filesystem-debug: $(DEBUG_LIBRARY) test-filesystem.c
	$(C_TOOL) $(C_FLAGS) -g $^ -L./ -lbulge-debug -o $@

test-release: test-filesystem

test-debug: test-filesystem-debug

# ===============================================

clean:
	rm -f $(RELEASE_OBJECTS)
	rm -f $(DEBUG_OBJECTS)
	rm -rf ./*.dSYM

veryclean: clean
	rm -f $(RELEASE_LIBRARY)
	rm -f $(DEBUG_LIBRARY)
	rm -f ./test-release
	rm -f ./test-debug

remake: veryclean all

# Makefile for bheap
# Ben Williams
# April 2025

CC=cl

CFLAGS=/Zi /EHsc /I.

#### HEADERS ####
DATASTRUCTURES_H = Datastructures/Datastructures.h Datastructures/block.h Datastructures/global_structures.h

MACHINERY_H = Machinery/bfree.h Machinery/bmalloc.h Machinery/block_operations.h Machinery/init.h

OTHER_H = Headers/headers.h Headers/macros.h Headers/parameters.h Headers/constants.h globals.h

# Combined header file dependencies
DEPS = $(DATASTRUCTURES_H) $(MACHINERY_H) $(OTHER_H)

#### OBJECTS ####
DATASTRUCTURES_O = Datastructures/block.obj

MACHINERY_O = Machinery/bfree.obj Machinery/bmalloc.obj Machinery/block_operations.obj Machinery/init.obj

OTHER_O = Testing/first_bmalloc_test.obj

# Object files to compile
OBJ = $(DATASTRUCTURES_O) $(MACHINERY_O) $(OTHER_O)

#### Default target ####
test.exe: $(OBJ)
	$(CC) $(CFLAGS) /Fetest.exe /Fo:. $^

#### Other Targets ####
.PHONY: clean test

clean:
	del /f *.exe *.obj *.pdb
	del /f Datastructures\*.obj
	del /f Machinery\*.obj


# test.exe: test.obj
# 	$(CC) $(CFLAGS) /Fe:test.exe $^

# test.obj: test.c
# 	$(CC) $(CFLAGS) /c /Fo:$@ $<

# test: test.exe


#### Compilation rules for individual files ####
Datastructures/block.obj: Datastructures/block.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

Machinery/bfree.obj: Machinery/bfree.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

Machinery/bmalloc.obj: Machinery/bmalloc.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

Machinery/block_operations.obj: Machinery/block_operations.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

Machinery/init.obj: Machinery/init.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

Testing/first_bmalloc_test.obj: Testing/first_bmalloc_test.c $(DEPS)
	$(CC) $(CFLAGS) /c /Fo:$@ $<

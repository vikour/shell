CC=gcc
DEBUG=-g
CFLAGS=-I include -c
LDFLAGS=
RUNNER=bin/shell
OBJECTS=build/shell.o build/inputModule.o build/history.o build/jobs_control.o

$(RUNNER): $(OBJECTS) build bin
	$(CC) $(OBJECTS) -o $(RUNNER) $(DEBUG) $(LDFLAGS)

build:
	@mkdir build
	@echo "Creating build dir..."

bin:
	@mkdir bin
	@echo "Creating bin dir..."

build/history.o: src/history.c include/history.h include/defs.h build
	@echo "Building build/history.o..."
	$(CC) $(CFLAGS) src/history.c -o build/history.o $(DEBUG)
	
build/inputModule.o: src/inputModule.c include/IOModule.h include/defs.h build
	@echo "Building build/inputModule.o..."
	$(CC) $(CFLAGS) src/inputModule.c -o build/inputModule.o $(DEBUG)

build/shell.o: src/shell.c include/history.h include/shell.h include/IOModule.h build include/jobs_control.h
	@echo "Building build/shell.o..."
	$(CC) $(CFLAGS) src/shell.c -o build/shell.o $(DEBUG)
	
build/jobs_control.o: src/jobs_control.c include/jobs_control.h include/defs.h
	@echo "Building build/jobs_control.o..."
	$(CC) $(CFLAGS) src/jobs_control.c -o build/jobs_control.o $(DEBUG)
	
clean:
	@echo "Cleaning..."
	@rm -rf build bin

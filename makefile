# Define variables
APP_NAME := ld_preload
SRC := .
SO_NAME := ld_preload.so

# Default target
.PHONY: all
all: build

# Build target: creates both an executable and a shared object
.PHONY: build
build:
	# Build executable
	go build -o $(APP_NAME) $(SRC)
	# Build shared object
	go build -o $(SO_NAME) -buildmode=c-shared $(SRC)

# Run target: implicitly calls build, then runs the application
.PHONY: run
run: build
	./$(APP_NAME)

# Clean target: removes the executable and shared object
.PHONY: clean
clean:
	rm -f $(APP_NAME) $(SO_NAME)

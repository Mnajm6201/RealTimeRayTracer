CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
TARGET = realtime_raytracer
SOURCES = raytracer.cpp

# Detect operating system
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

# macOS
ifeq ($(UNAME_S),Darwin)
    LIBS = -lglfw -lGLEW -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    INCLUDES = -I/opt/homebrew/include -I/usr/local/include
    LIBDIRS = -L/opt/homebrew/lib -L/usr/local/lib
    INSTALL_CMD = brew install glfw glew
    PLATFORM = macOS
endif

# Linux
ifeq ($(UNAME_S),Linux)
    LIBS = -lglfw -lGLEW -lGL -lX11 -lpthread -lXrandr -lXi -ldl
    INCLUDES = -I/usr/include
    LIBDIRS = -L/usr/lib -L/usr/local/lib
    INSTALL_CMD = sudo apt-get install libglfw3-dev libglew-dev libgl1-mesa-dev
    PLATFORM = Linux
endif

# Windows (MinGW/MSYS2)
ifeq ($(UNAME_S),Windows)
    TARGET = realtime_raytracer.exe
    LIBS = -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32
    INCLUDES = -I/mingw64/include -I/usr/local/include
    LIBDIRS = -L/mingw64/lib -L/usr/local/lib
    INSTALL_CMD = pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
    PLATFORM = Windows
endif

# Windows (MSYS environment)
ifneq (,$(findstring MSYS,$(UNAME_S)))
    TARGET = realtime_raytracer.exe
    LIBS = -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32
    INCLUDES = -I/mingw64/include
    LIBDIRS = -L/mingw64/lib
    INSTALL_CMD = pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
    PLATFORM = Windows-MSYS2
endif

# Build target
$(TARGET): $(SOURCES)
	@echo "Building for $(PLATFORM)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBDIRS) -o $(TARGET) $(SOURCES) $(LIBS)

# Platform-specific dependency installation
install_deps:
	@echo "Installing dependencies for $(PLATFORM)..."
	@echo "Run: $(INSTALL_CMD)"

# Detailed installation instructions
install_help:
	@echo "=== Cross-Platform Installation Guide ==="
	@echo ""
ifeq ($(UNAME_S),Darwin)
	@echo "macOS Setup:"
	@echo "1. Install Homebrew: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
	@echo "2. Install libraries: brew install glfw glew"
endif
ifeq ($(UNAME_S),Linux)
	@echo "Linux Setup:"
	@echo "Ubuntu/Debian: sudo apt-get update && sudo apt-get install libglfw3-dev libglew-dev libgl1-mesa-dev"
	@echo "Fedora: sudo dnf install glfw-devel glew-devel mesa-libGL-devel"
	@echo "Arch: sudo pacman -S glfw glew mesa"
	@echo "CentOS/RHEL: sudo yum install glfw-devel glew-devel mesa-libGL-devel"
endif
ifneq (,$(findstring Windows,$(PLATFORM)))
	@echo "Windows Setup Options:"
	@echo ""
	@echo "Option 1 - MSYS2/MinGW (Recommended):"
	@echo "1. Download MSYS2 from: https://www.msys2.org/"
	@echo "2. Install packages: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-glew"
	@echo "3. Use MinGW64 terminal to compile"
	@echo ""
	@echo "Option 2 - vcpkg:"
	@echo "1. git clone https://github.com/Microsoft/vcpkg.git"
	@echo "2. .\\vcpkg\\bootstrap-vcpkg.bat"
	@echo "3. .\\vcpkg\\vcpkg install glfw3 glew"
	@echo ""
	@echo "Option 3 - Manual:"
	@echo "1. Download GLFW from: https://www.glfw.org/download.html"
	@echo "2. Download GLEW from: http://glew.sourceforge.net/"
	@echo "3. Set up include/lib paths manually"
endif
	@echo ""
	@echo "After installation: make && make run"

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -f $(TARGET) realtime_raytracer realtime_raytracer.exe

# Run the program
run: $(TARGET)
	@echo "Starting Real-Time Ray Tracer..."
	./$(TARGET)

# Test compilation without running
test:
	@echo "Testing compilation for $(PLATFORM)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBDIRS) -c $(SOURCES)
	@echo "Compilation test successful!"
	rm -f *.o

# Show current platform and settings
info:
	@echo "=== Build Configuration ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Includes: $(INCLUDES)"
	@echo "Target: $(TARGET)"

# Help menu
help:
	@echo "Real-Time Ray Tracer - Cross-Platform Makefile"
	@echo "=============================================="
	@echo ""
	@echo "Commands:"
	@echo "  make              - Build the ray tracer"
	@echo "  make run          - Build and run"
	@echo "  make clean        - Remove build files"
	@echo "  make install_help - Show installation guide"
	@echo "  make test         - Test compilation only"
	@echo "  make info         - Show build configuration"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Quick Start:"
	@echo "1. make install_help  (follow instructions for your OS)"
	@echo "2. make && make run"

.PHONY: clean run help install_help install_deps test info
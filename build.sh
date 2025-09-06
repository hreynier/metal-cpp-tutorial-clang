#!/bin/bash

# Minimal Metal C++ Build Script
# Usage: ./build.sh [clean|run|help]

set -e  # Exit on any error

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE_NAME="minimal-metal-cpp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "Minimal Metal C++ Build Script"
    echo ""
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Options:"
    echo "  clean      Clean build directory and rebuild from scratch"
    echo "  run        Build and run the application"
    echo "  help       Show this help message"
    echo "  (no args)  Just build the project"
    echo ""
    echo "Examples:"
    echo "  $0              # Build the project"
    echo "  $0 clean        # Clean build and rebuild"
    echo "  $0 run          # Build and run"
    echo ""
}

clean_build() {
    print_status "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"/*
        print_success "Build directory cleaned"
    else
        print_warning "Build directory doesn't exist, creating it..."
    fi
}

ensure_submodules() {
    print_status "Checking git submodules..."
    if [ ! -f "$PROJECT_DIR/dependencies/glfw/CMakeLists.txt" ]; then
        print_warning "GLFW submodule not found, initializing submodules..."
        cd "$PROJECT_DIR"
        git submodule update --init --recursive
        print_success "Submodules initialized"
    else
        print_success "Submodules are ready"
    fi
}

build_project() {
    print_status "Building project..."

    # Create build directory if it doesn't exist
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure CMake if needed
    if [ ! -f "CMakeCache.txt" ]; then
        print_status "Configuring CMake..."
        cmake ..
    fi

    # Build with verbose output and parallel jobs
    NPROC=$(sysctl -n hw.ncpu 2>/dev/null || echo "4")
    print_status "Building with $NPROC parallel jobs..."
    cmake --build . --verbose -j"$NPROC"

    if [ -f "$EXECUTABLE_NAME" ]; then
        print_success "Build completed successfully!"
        print_status "Executable: $BUILD_DIR/$EXECUTABLE_NAME"
    else
        print_error "Build failed - executable not found!"
        exit 1
    fi
}

run_application() {
    cd "$BUILD_DIR"
    if [ -f "$EXECUTABLE_NAME" ]; then
        print_status "Running $EXECUTABLE_NAME..."
        echo "----------------------------------------"
        ./"$EXECUTABLE_NAME"
        echo "----------------------------------------"
        print_success "Application finished"
    else
        print_error "Executable not found! Please build first."
        exit 1
    fi
}

check_dependencies() {
    print_status "Checking dependencies..."

    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found! Please install CMake 3.28.0 or later."
        exit 1
    fi

    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_success "CMake found: $CMAKE_VERSION"

    # Check Xcode command line tools
    if ! xcode-select -p &> /dev/null; then
        print_error "Xcode command line tools not found! Please install with: xcode-select --install"
        exit 1
    fi

    print_success "Xcode command line tools found"
}

# Main script logic
case "${1:-build}" in
    "clean")
        print_status "Starting clean build..."
        check_dependencies
        ensure_submodules
        clean_build
        build_project
        print_success "Clean build completed!"
        ;;
    "run")
        print_status "Building and running application..."
        check_dependencies
        ensure_submodules
        build_project
        run_application
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    "build"|"")
        print_status "Building project..."
        check_dependencies
        ensure_submodules
        build_project
        ;;
    *)
        print_error "Unknown option: $1"
        show_help
        exit 1
        ;;
esac

print_success "Script completed successfully!"

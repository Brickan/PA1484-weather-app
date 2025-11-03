#!/bin/sh
# Linux/Unix launcher with auto-install for T4-S3 Simulator
# POSIX-compatible - works on all shells

set -e

# Store the project root directory
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"

# ============================================
# Show Menu
# ============================================
show_menu() {
    echo ""
    echo "======================================"
    echo "  T4-S3 Simulator - Menu"
    echo "======================================"
    echo "1. Build"
    echo "2. Run"
    echo "3. Install Requirements"
    echo "4. Exit"
    echo "======================================"
    printf "Enter your choice [1-4]: "
}

# ============================================
# Build
# ============================================
build_project() {
    cd "$PROJECT_ROOT/simulator"
    sh ./build.sh clean
    cd "$PROJECT_ROOT"
}

# ============================================
# Run
# ============================================
run_project() {
    cd "$PROJECT_ROOT/simulator"
    sh ./run.sh
    cd "$PROJECT_ROOT"
}

# ============================================
# Install Requirements
# ============================================
install_requirements() {
    echo "======================================"
    echo "  Installing Requirements"
    echo "======================================"
    echo ""

    # Detect package manager and install accordingly
    if command -v apt >/dev/null 2>&1; then
        echo "Detected: Debian/Ubuntu-based system (apt)"
        echo "Installing: build-essential cmake libsdl2-dev libsdl2-image-dev libcurl4-openssl-dev"
        echo ""

        # Check if running as root
        if [ "$(id -u)" -ne 0 ]; then
            echo "This requires sudo privileges..."
            sudo apt update
            sudo apt install -y build-essential cmake libsdl2-dev libsdl2-image-dev libcurl4-openssl-dev
        else
            apt update
            apt install -y build-essential cmake libsdl2-dev libsdl2-image-dev libcurl4-openssl-dev
        fi

    elif command -v dnf >/dev/null 2>&1; then
        echo "Detected: Fedora/RHEL-based system (dnf)"
        echo "Installing: gcc gcc-c++ cmake SDL2-devel SDL2_image-devel libcurl-devel"
        echo ""

        if [ "$(id -u)" -ne 0 ]; then
            sudo dnf install -y gcc gcc-c++ cmake SDL2-devel SDL2_image-devel libcurl-devel
        else
            dnf install -y gcc gcc-c++ cmake SDL2-devel SDL2_image-devel libcurl-devel
        fi

    elif command -v pacman >/dev/null 2>&1; then
        echo "Detected: Arch-based system (pacman)"
        echo "Installing: base-devel cmake sdl2 sdl2_image curl"
        echo ""

        if [ "$(id -u)" -ne 0 ]; then
            sudo pacman -Syu --needed --noconfirm base-devel cmake sdl2 sdl2_image curl
        else
            pacman -Syu --needed --noconfirm base-devel cmake sdl2 sdl2_image curl
        fi

    elif command -v zypper >/dev/null 2>&1; then
        echo "Detected: openSUSE-based system (zypper)"
        echo "Installing: devel_basis cmake libSDL2-devel libSDL2_image-devel libcurl-devel"
        echo ""

        if [ "$(id -u)" -ne 0 ]; then
            sudo zypper install -y -t pattern devel_basis
            sudo zypper install -y cmake libSDL2-devel libSDL2_image-devel libcurl-devel
        else
            zypper install -y -t pattern devel_basis
            zypper install -y cmake libSDL2-devel libSDL2_image-devel libcurl-devel
        fi

    elif command -v apk >/dev/null 2>&1; then
        echo "Detected: Alpine Linux (apk)"
        echo "Installing: build-base cmake sdl2-dev sdl2_image-dev curl-dev"
        echo ""

        if [ "$(id -u)" -ne 0 ]; then
            sudo apk add --no-cache build-base cmake sdl2-dev sdl2_image-dev curl-dev
        else
            apk add --no-cache build-base cmake sdl2-dev sdl2_image-dev curl-dev
        fi

    else
        echo "ERROR: No supported package manager found!"
        echo ""
        echo "Supported package managers:"
        echo "  - apt (Debian/Ubuntu)"
        echo "  - dnf (Fedora/RHEL)"
        echo "  - pacman (Arch)"
        echo "  - zypper (openSUSE)"
        echo "  - apk (Alpine)"
        echo ""
        echo "Please install manually:"
        echo "  - CMake"
        echo "  - GCC/G++ (build-essential)"
        echo "  - SDL2 development libraries"
        echo "  - SDL2_image development libraries"
        echo "  - libcurl development libraries"
        return 1
    fi

    echo ""
    echo "======================================"
    echo "  Installation Complete!"
    echo "======================================"
    echo "Verifying installation..."

    # Verify installations
    if command -v cmake >/dev/null 2>&1; then
        echo "✓ CMake: $(cmake --version | head -n1)"
    else
        echo "✗ CMake: Not found"
    fi

    if command -v gcc >/dev/null 2>&1; then
        echo "✓ GCC: $(gcc --version | head -n1)"
    else
        echo "✗ GCC: Not found"
    fi

    if command -v g++ >/dev/null 2>&1; then
        echo "✓ G++: $(g++ --version | head -n1)"
    else
        echo "✗ G++: Not found"
    fi

    if command -v sdl2-config >/dev/null 2>&1; then
        echo "✓ SDL2: $(sdl2-config --version)"
    else
        echo "✗ SDL2: Not found"
    fi

    echo "======================================"
}

# ============================================
# Command-line argument handling
# ============================================
if [ $# -gt 0 ]; then
    case "$1" in
        build)
            build_project "$2"
            exit 0
            ;;
        run)
            run_project
            exit 0
            ;;
        install)
            install_requirements
            exit 0
            ;;
        *)
            echo "Usage: $0 {build|run|install}"
            echo "   or: $0 (for interactive menu)"
            exit 1
            ;;
    esac
fi

# ============================================
# Interactive Menu Loop
# ============================================
while true; do
    show_menu
    read choice
    case $choice in
        1)
            build_project
            ;;
        2)
            run_project
            ;;
        3)
            install_requirements
            ;;
        4)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo "Invalid option. Please choose 1, 2, 3, or 4."
            ;;
    esac
done

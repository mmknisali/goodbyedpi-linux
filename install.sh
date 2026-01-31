#!/bin/bash
# GoodbyeDPI Linux Installation Script
# This script installs GoodbyeDPI and sets up the system

set -e # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script must be run as root"
        echo "Please run: sudo $0"
        exit 1
    fi
}

# Detect Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        VERSION=$VERSION_ID
    else
        print_error "Cannot detect Linux distribution"
        exit 1
    fi

    print_info "Detected: $PRETTY_NAME"
}

# Install dependencies based on distribution
install_dependencies() {
    print_info "Installing dependencies..."

    case $DISTRO in
    ubuntu | debian | linuxmint)
        apt-get update
        apt-get install -y \
            build-essential cmake pkg-config \
            libnetfilter-queue-dev libmnl-dev \
            libsystemd-dev iptables
        ;;

    fedora | rhel | centos)
        dnf install -y \
            gcc cmake pkgconfig \
            libnetfilter_queue-devel libmnl-devel \
            systemd-devel iptables
        ;;

    arch | manjaro)
        pacman -Sy --noconfirm \
            base-devel cmake \
            libnetfilter_queue libmnl \
            systemd iptables
        ;;

    *)
        print_error "Unsupported distribution: $DISTRO"
        print_warn "Please install dependencies manually:"
        print_warn "  - build tools (gcc, cmake, pkg-config)"
        print_warn "  - libnetfilter-queue development files"
        print_warn "  - libmnl development files"
        print_warn "  - iptables"
        exit 1
        ;;
    esac

    print_info "Dependencies installed successfully"
}

# Build the project
build_project() {
    print_info "Building GoodbyeDPI..."

    # Create build directory
    if [ -d "build" ]; then
        print_warn "Removing existing build directory"
        rm -rf build
    fi

    mkdir build
    cd build

    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DENABLE_SYSTEMD=ON

    # Build
    make -j$(nproc)

    print_info "Build completed successfully"
}

# Install the binary and files
install_files() {
    print_info "Installing GoodbyeDPI..."

    # Install from build directory
    cd build
    make install

    cd ..

    # Create config directory
    mkdir -p /etc/goodbyedpi

    # Install example config if it doesn't exist
    if [ ! -f /etc/goodbyedpi/goodbyedpi.conf ]; then
        if [ -f config/goodbyedpi.conf.example ]; then
            cp config/goodbyedpi.conf.example /etc/goodbyedpi/goodbyedpi.conf
            print_info "Installed configuration file to /etc/goodbyedpi/goodbyedpi.conf"
        fi
    else
        print_warn "Configuration file already exists, skipping"
    fi

    # Create log directory
    mkdir -p /var/log/goodbyedpi

    print_info "Installation completed successfully"
}

# Setup systemd service
setup_systemd() {
    print_info "Setting up systemd service..."

    # Reload systemd
    systemctl daemon-reload

    print_info "Systemd service installed"
    print_info "To enable on boot: systemctl enable goodbyedpi"
    print_info "To start now: systemctl start goodbyedpi"
}

# Load kernel modules
load_modules() {
    print_info "Loading required kernel modules..."

    # Load nfnetlink_queue module
    modprobe nfnetlink_queue 2>/dev/null || true

    # Make it load on boot
    if [ ! -f /etc/modules-load.d/goodbyedpi.conf ]; then
        echo "nfnetlink_queue" >/etc/modules-load.d/goodbyedpi.conf
        print_info "Configured kernel module to load on boot"
    fi
}

# Set capabilities
set_capabilities() {
    print_info "Setting capabilities..."

    # Set CAP_NET_ADMIN capability on binary
    setcap cap_net_admin,cap_net_raw=+eip /usr/bin/goodbyedpi || {
        print_warn "Failed to set capabilities, will need to run as root"
    }
}

# Print final instructions
print_instructions() {
    echo ""
    echo "======================================================================"
    echo "  GoodbyeDPI Linux Installation Complete!"
    echo "======================================================================"
    echo ""
    echo "Quick Start:"
    echo "  1. Edit configuration (optional):"
    echo "     nano /etc/goodbyedpi/goodbyedpi.conf"
    echo ""
    echo "  2. Start the service:"
    echo "     systemctl start goodbyedpi"
    echo ""
    echo "  3. Enable on boot (optional):"
    echo "     systemctl enable goodbyedpi"
    echo ""
    echo "  4. Check status:"
    echo "     systemctl status goodbyedpi"
    echo ""
    echo "Manual Usage:"
    echo "  sudo goodbyedpi -9           # Run with mode 9 (recommended)"
    echo "  sudo goodbyedpi -h           # Show all options"
    echo ""
    echo "Logs:"
    echo "  journalctl -u goodbyedpi -f  # View systemd logs"
    echo "  tail -f /var/log/goodbyedpi/goodbyedpi.log  # View file logs"
    echo ""
    echo "For more information, see: /usr/share/doc/goodbyedpi/README.md"
    echo "======================================================================"
}

# Main installation flow
main() {
    echo "======================================================================"
    echo "  GoodbyeDPI Linux Installation Script"
    echo "======================================================================"
    echo ""

    check_root
    detect_distro
    install_dependencies
    build_project
    install_files
    setup_systemd
    load_modules
    set_capabilities
    print_instructions
}

# Run main function
main

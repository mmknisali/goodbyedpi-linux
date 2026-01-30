#!/bin/bash

# GoodbyeDPI Linux Installation Script
# This script installs GoodbyeDPI Linux with proper dependencies and configuration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

# Detect distribution
detect_distribution() {
    if [[ -f /etc/arch-release ]]; then
        DISTRO="arch"
    elif [[ -f /etc/debian_version ]]; then
        DISTRO="debian"
    elif [[ -f /etc/fedora-release ]]; then
        DISTRO="fedora"
    else
        print_error "Unsupported distribution"
        exit 1
    fi
    
    print_status "Detected distribution: $DISTRO"
}

# Install dependencies for Arch Linux
install_dependencies_arch() {
    print_status "Installing dependencies for Arch Linux..."
    
    pacman -Syu --noconfirm
    pacman -S --noconfirm \
        cmake \
        make \
        gcc \
        pkg-config \
        libnetfilter_queue \
        libmnl \
        iptables-nft \
        systemd \
        git
    
    print_status "Dependencies installed successfully"
}

# Install dependencies for Debian/Ubuntu
install_dependencies_debian() {
    print_status "Installing dependencies for Debian/Ubuntu..."
    
    apt-get update
    apt-get install -y \
        cmake \
        make \
        gcc \
        pkg-config \
        libnetfilter-queue-dev \
        libmnl-dev \
        iptables \
        systemd \
        git
    
    print_status "Dependencies installed successfully"
}

# Install dependencies for Fedora
install_dependencies_fedora() {
    print_status "Installing dependencies for Fedora..."
    
    dnf update -y
    dnf install -y \
        cmake \
        make \
        gcc \
        pkg-config \
        libnetfilter_queue-devel \
        libmnl-devel \
        iptables \
        systemd \
        git
    
    print_status "Dependencies installed successfully"
}

# Build and install GoodbyeDPI
build_install_goodbyedpi() {
    print_status "Building GoodbyeDPI..."
    
    # Create build directory
    cd /tmp
    rm -rf goodbyedpi-linux
    git clone https://github.com/goodbyedpi-linux/goodbyedpi-linux.git
    cd goodbyedpi-linux
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    
    # Build
    make -j$(nproc)
    
    # Install
    make install
    
    print_status "GoodbyeDPI installed successfully"
}

# Setup configuration
setup_configuration() {
    print_status "Setting up configuration..."
    
    # Create configuration directory
    mkdir -p /etc/goodbyedpi
    
    # Copy default configuration
    cp /usr/local/etc/goodbyedpi/goodbyedpi.conf.example \
       /etc/goodbyedpi/goodbyedpi.conf
    
    # Create log directory
    mkdir -p /var/log
    
    # Set permissions
    chmod 644 /etc/goodbyedpi/goodbyedpi.conf
    
    print_status "Configuration setup completed"
}

# Setup systemd service
setup_systemd_service() {
    print_status "Setting up systemd service..."
    
    # Reload systemd daemon
    systemctl daemon-reload
    
    # Enable service (but don't start it yet)
    systemctl enable goodbyedpi
    
    print_status "Systemd service setup completed"
}

# Setup firewall rules (optional)
setup_firewall() {
    print_warning "Firewall rules need to be set up manually"
    print_warning "Add these rules to make GoodbyeDPI work:"
    echo ""
    echo "iptables -I OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0"
    echo "iptables -I OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0"
    echo "iptables -I INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0"
    echo "iptables -I INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0"
    echo ""
    print_warning "Make these rules persistent across reboots"
}

# Display completion message
show_completion() {
    print_status "GoodbyeDPI Linux installation completed!"
    echo ""
    echo "Next steps:"
    echo "1. Edit /etc/goodbyedpi/goodbyedpi.conf to configure settings"
    echo "2. Set up firewall rules manually (see above)"
    echo "3. Start the service: systemctl start goodbyedpi"
    echo "4. Check status: systemctl status goodbyedpi"
    echo ""
    echo "For more information, see: https://github.com/goodbyedpi-linux"
}

# Main installation flow
main() {
    print_status "Starting GoodbyeDPI Linux installation..."
    
    check_root
    detect_distribution
    
    # Install dependencies based on distribution
    case $DISTRO in
        arch)
            install_dependencies_arch
            ;;
        debian)
            install_dependencies_debian
            ;;
        fedora)
            install_dependencies_fedora
            ;;
    esac
    
    build_install_goodbyedpi
    setup_configuration
    setup_systemd_service
    setup_firewall
    show_completion
}

# Run main function
main "$@"
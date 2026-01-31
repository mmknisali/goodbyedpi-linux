#!/bin/bash
# GoodbyeDPI Docker Quick Start Script

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

banner() {
    echo "======================================================================"
    echo "  🚀 GoodbyeDPI Linux - Docker Quick Start"
    echo "======================================================================"
    echo ""
}

check_docker() {
    if ! command -v docker &>/dev/null; then
        print_error "Docker is not installed!"
        echo ""
        echo "Please install Docker first:"
        echo "  curl -fsSL https://get.docker.com -o get-docker.sh"
        echo "  sudo sh get-docker.sh"
        exit 1
    fi

    if ! docker ps &>/dev/null; then
        print_error "Docker daemon is not running or you don't have permission"
        echo ""
        echo "Try:"
        echo "  sudo systemctl start docker"
        echo "  sudo usermod -aG docker $USER"
        echo "  (then log out and back in)"
        exit 1
    fi

    print_info "Docker: ✓"
}

check_docker_compose() {
    if ! command -v docker-compose &>/dev/null; then
        print_warn "docker-compose not found, trying docker compose..."
        if ! docker compose version &>/dev/null; then
            print_error "Neither docker-compose nor docker compose found!"
            echo ""
            echo "Install docker-compose:"
            echo "  sudo apt install docker-compose"
            exit 1
        else
            COMPOSE_CMD="docker compose"
        fi
    else
        COMPOSE_CMD="docker-compose"
    fi

    print_info "Docker Compose: ✓"
}

check_files() {
    local missing=0

    if [ ! -f "Dockerfile" ]; then
        print_error "Dockerfile not found!"
        missing=1
    fi

    if [ ! -f "docker-compose.yml" ]; then
        print_error "docker-compose.yml not found!"
        missing=1
    fi

    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found!"
        missing=1
    fi

    if [ ! -d "src" ]; then
        print_error "src/ directory not found!"
        missing=1
    fi

    if [ $missing -eq 1 ]; then
        echo ""
        print_error "Missing required files. Are you in the project directory?"
        exit 1
    fi

    print_info "Project files: ✓"
}

check_kernel_module() {
    if ! lsmod | grep -q nfnetlink_queue; then
        print_warn "nfnetlink_queue module not loaded"
        print_info "Loading kernel module..."
        sudo modprobe nfnetlink_queue || {
            print_error "Failed to load kernel module"
            echo "This is required for GoodbyeDPI to work"
            exit 1
        }
    fi
    print_info "Kernel module: ✓"
}

build_image() {
    print_info "Building Docker image (this may take a few minutes)..."
    $COMPOSE_CMD build || {
        print_error "Build failed!"
        exit 1
    }
    print_info "Build complete: ✓"
}

start_container() {
    print_info "Starting GoodbyeDPI container..."
    $COMPOSE_CMD up -d || {
        print_error "Failed to start container!"
        exit 1
    }
    print_info "Container started: ✓"
}

show_status() {
    echo ""
    echo "======================================================================"
    echo "  Container Status"
    echo "======================================================================"
    $COMPOSE_CMD ps
    echo ""
}

show_logs() {
    echo "======================================================================"
    echo "  Live Logs (Ctrl+C to exit)"
    echo "======================================================================"
    echo ""
    $COMPOSE_CMD logs -f
}

show_instructions() {
    echo ""
    echo "======================================================================"
    echo "  🎉 GoodbyeDPI is Running!"
    echo "======================================================================"
    echo ""
    echo "Useful commands:"
    echo "  $COMPOSE_CMD logs -f          # View live logs"
    echo "  $COMPOSE_CMD down             # Stop container"
    echo "  $COMPOSE_CMD restart          # Restart container"
    echo "  $COMPOSE_CMD ps               # Check status"
    echo ""
    echo "Or use the Makefile:"
    echo "  make logs                     # View logs"
    echo "  make down                     # Stop"
    echo "  make restart                  # Restart"
    echo "  make help                     # See all commands"
    echo ""
    echo "To stop now: Ctrl+C (logs will continue in background)"
    echo "======================================================================"
    echo ""
}

main() {
    banner

    print_info "Performing pre-flight checks..."
    check_docker
    check_docker_compose
    check_files
    check_kernel_module

    echo ""
    print_info "All checks passed! ✓"
    echo ""

    # Ask user if they want to proceed
    read -p "Start GoodbyeDPI in Docker? (Y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Nn]$ ]]; then
        print_info "Cancelled by user"
        exit 0
    fi

    echo ""
    build_image
    echo ""
    start_container
    echo ""
    show_status
    show_instructions

    # Ask if user wants to see logs
    read -p "Show live logs now? (Y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        echo ""
        show_logs
    fi
}

main

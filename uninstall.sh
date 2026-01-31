#!/bin/bash
# GoodbyeDPI Linux Uninstallation Script

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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

# Check root
if [ "$EUID" -ne 0 ]; then
    print_error "This script must be run as root"
    echo "Please run: sudo $0"
    exit 1
fi

echo "======================================================================"
echo "  GoodbyeDPI Linux Uninstallation Script"
echo "======================================================================"
echo ""

# Stop service if running
print_info "Stopping GoodbyeDPI service..."
systemctl stop goodbyedpi 2>/dev/null || true
systemctl disable goodbyedpi 2>/dev/null || true

# Remove binary
print_info "Removing binary..."
rm -f /usr/bin/goodbyedpi
rm -f /usr/local/bin/goodbyedpi

# Remove systemd service
print_info "Removing systemd service..."
rm -f /lib/systemd/system/goodbyedpi.service
rm -f /usr/lib/systemd/system/goodbyedpi.service
systemctl daemon-reload

# Ask about configuration
read -p "Remove configuration files? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_info "Removing configuration..."
    rm -rf /etc/goodbyedpi
fi

# Ask about logs
read -p "Remove log files? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_info "Removing logs..."
    rm -rf /var/log/goodbyedpi
fi

# Remove PID file
rm -f /var/run/goodbyedpi.pid

# Remove kernel module config
print_info "Removing kernel module configuration..."
rm -f /etc/modules-load.d/goodbyedpi.conf

# Clean iptables rules
print_info "Cleaning iptables rules..."
iptables -D OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0 2>/dev/null || true
iptables -D OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0 2>/dev/null || true
iptables -D INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0 2>/dev/null || true
iptables -D INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0 2>/dev/null || true

print_info "GoodbyeDPI has been uninstalled"
echo ""

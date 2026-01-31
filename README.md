# GoodbyeDPI Linux

![Version](https://img.shields.io/badge/version-0.2.3-blue)
![License](https://img.shields.io/badge/license-Apache--2.0-green)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)

**GoodbyeDPI for Linux** - A powerful Deep Packet Inspection (DPI) bypass and circumvention utility for Linux systems, specifically optimized for Turkey's internet censorship.

## 🌟 Features

- **DPI Bypass**: Circumvent Deep Packet Inspection used for internet censorship
- **Multiple Evasion Techniques**:
  - HTTP/HTTPS packet fragmentation
  - Host header manipulation
  - TTL manipulation
  - TCP sequence/checksum obfuscation
  - Fake packet injection
- **DNS Redirection**: Optional DNS server override
- **Legacy Mode Support**: Compatible with original GoodbyeDPI modes (1, 2, 5, 6, 7, 9)
- **Daemon Mode**: Run as a background service
- **Systemd Integration**: Native systemd service support
- **Automatic Firewall Setup**: Configures iptables rules automatically

## 📋 Requirements

### Runtime Dependencies
- Linux kernel 3.x or later
- libnetfilter-queue (>= 1.0)
- libmnl (>= 1.0)
- iptables
- Root/sudo privileges

### Build Dependencies
- GCC or Clang
- CMake (>= 3.16)
- pkg-config
- Development headers:
  - libnetfilter-queue-dev
  - libmnl-dev
  - libsystemd-dev (optional, for systemd support)

## 🚀 Installation

### From Source

#### Ubuntu/Debian
```bash
# Install dependencies
sudo apt update
sudo apt install build-essential cmake pkg-config \
    libnetfilter-queue-dev libmnl-dev libsystemd-dev iptables

# Clone repository
git clone https://github.com/mmknisali/goodbyedpi-linux.git
cd goodbyedpi-linux

# Build
mkdir build && cd build
cmake ..
make

# Install
sudo make install
```

#### Fedora/RHEL/CentOS
```bash
# Install dependencies
sudo dnf install gcc cmake pkgconfig \
    libnetfilter_queue-devel libmnl-devel systemd-devel iptables

# Clone and build
git clone https://github.com/mmknisali/goodbyedpi-linux.git
cd goodbyedpi-linux
mkdir build && cd build
cmake ..
make
sudo make install
```

#### Arch Linux
```bash
# Install dependencies
sudo pacman -S base-devel cmake libnetfilter_queue libmnl systemd iptables

# Clone and build
git clone https://github.com/mmknisali/goodbyedpi-linux.git
cd goodbyedpi-linux
mkdir build && cd build
cmake ..
make
sudo make install
```

### Build Options

```bash
# Build with debug symbols
cmake -DENABLE_DEBUG=ON ..

# Build without systemd support
cmake -DENABLE_SYSTEMD=OFF ..

# Custom installation prefix
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
```

## 📖 Usage

### Basic Usage

```bash
# Run with default settings (requires root)
sudo goodbyedpi

# Run in daemon mode
sudo goodbyedpi -d

# Use legacy mode 9 (recommended for Turkey)
sudo goodbyedpi -9

# Enable verbose logging
sudo goodbyedpi -v -9
```

### Command Line Options

```
Options:
  -h, --help              Show help message
  -V, --version           Show version information
  -d, --daemon            Run as daemon
  -c, --config FILE       Load configuration from file
  -p, --pidfile FILE      PID file path (default: /var/run/goodbyedpi.pid)
  -l, --logfile FILE      Log file path
  -v, --verbose           Enable verbose output
  --debug                 Enable debug output
  --syslog                Use syslog for logging
  --queue-num NUM         NFQUEUE number (default: 0)

Fragmentation options:
  -f, --fragment-http SIZE    HTTP fragment size (1-65535)
  -e, --fragment-https SIZE   HTTPS fragment size (1-65535)
  --native-frag               Use native fragmentation
  --reverse-frag              Use reverse fragmentation

Header manipulation:
  --host-mixedcase          Mix case in Host header
  --additional-space         Add additional space
  --host-removespace        Remove space after Host:

DNS options:
  --dns-redirect-v4 ADDR    Redirect IPv4 DNS to ADDR
  --dns-redirect-v6 ADDR    Redirect IPv6 DNS to ADDR
  --dns-port PORT           DNS port (default: 53)

Legacy modes:
  -1                     Legacy mode 1 (compatible)
  -2                     Legacy mode 2 (HTTPS optimization)
  -5                     Modern mode 5 (auto-TTL)
  -6                     Modern mode 6 (wrong-seq)
  -7                     Modern mode 7 (wrong-chksum)
  -9                     Modern mode 9 (full features - RECOMMENDED)
```

### Legacy Mode Descriptions

- **Mode 1**: Basic compatibility mode with simple fragmentation
- **Mode 2**: Optimized for HTTPS with larger fragment size
- **Mode 5**: Auto-TTL mode with fake packets
- **Mode 6**: Wrong TCP sequence mode
- **Mode 7**: Wrong TCP checksum mode
- **Mode 9**: Full feature mode (recommended) - combines all techniques

## 🔧 Configuration

### Configuration File

Create `/etc/goodbyedpi/goodbyedpi.conf`:

```ini
# GoodbyeDPI Linux Configuration

[general]
daemon = true
verbose = false
pidfile = /var/run/goodbyedpi.pid
logfile = /var/log/goodbyedpi.log

[fragmentation]
http_fragment_size = 2
https_fragment_size = 2
native_fragmentation = true
reverse_fragmentation = true

[evasion]
host_mixedcase = true
host_removespace = true
wrong_sequence = true
wrong_checksum = true
fake_packet = true
block_quic = true

[dns]
redirect_ipv4 = false
redirect_ipv6 = false
# dns_server_v4 = 8.8.8.8
# dns_server_v6 = 2001:4860:4860::8888
```

### Systemd Service

Enable and start the service:

```bash
# Enable service to start on boot
sudo systemctl enable goodbyedpi

# Start service
sudo systemctl start goodbyedpi

# Check status
sudo systemctl status goodbyedpi

# View logs
sudo journalctl -u goodbyedpi -f
```

### Manual Firewall Setup

If you need to manually configure iptables (normally handled automatically):

```bash
# Add rules
sudo iptables -I OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0
sudo iptables -I OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0
sudo iptables -I INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0
sudo iptables -I INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0

# Remove rules
sudo iptables -D OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0
sudo iptables -D OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0
sudo iptables -D INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0
sudo iptables -D INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0
```

## 🐛 Troubleshooting

### Common Issues

**1. "Permission denied" error**
```bash
# Make sure you're running as root
sudo goodbyedpi
```

**2. "Failed to initialize netfilter queue"**
```bash
# Check if netfilter_queue module is loaded
sudo modprobe nfnetlink_queue

# Check if another instance is running
sudo killall goodbyedpi
```

**3. "Failed to add iptables rule"**
```bash
# Make sure iptables is installed
sudo apt install iptables  # Debian/Ubuntu
sudo dnf install iptables  # Fedora/RHEL

# Check if you have CAP_NET_ADMIN capability
sudo getcap $(which goodbyedpi)
```

**4. Internet not working after starting**
```bash
# Stop goodbyedpi
sudo systemctl stop goodbyedpi

# Or kill the process
sudo killall goodbyedpi

# Rules should be cleaned up automatically
# If not, manually remove:
sudo iptables -F  # WARNING: This flushes ALL rules
```

### Debug Mode

Run with debug logging:

```bash
sudo goodbyedpi --debug -v --logfile /tmp/goodbyedpi.log
```

Check the log file:

```bash
tail -f /tmp/goodbyedpi.log
```

## 📊 Performance

- Typical CPU usage: 1-5% (depends on traffic)
- Memory usage: ~5-10 MB
- Latency overhead: <1ms for most packets
- Handles thousands of packets per second

## 🔒 Security Considerations

1. **Root Privileges**: This tool requires root to manipulate packets
2. **Firewall Rules**: Automatically manages iptables rules
3. **Privacy**: All processing is done locally, no external connections
4. **Logging**: Be careful with verbose logging in production

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Original [GoodbyeDPI](https://github.com/ValdikSS/GoodbyeDPI) by ValdikSS
- The netfilter_queue developers
- The Linux networking community

## 🔗 Related Projects

- [zapret](https://github.com/bol-van/zapret) - Russian DPI bypass
- [byedpi](https://github.com/hufrea/byedpi) - Another DPI bypass tool
- [Green Tunnel](https://github.com/SadeghHayeri/GreenTunnel) - Anti-censorship utility

## 📧 Support

- **Issues**: [GitHub Issues](https://github.com/mmknisali/goodbyedpi-linux/issues)
- **Discussions**: [GitHub Discussions](https://github.com/mmknisali/goodbyedpi-linux/discussions)

## ⚠️ Disclaimer

This tool is for educational and research purposes. Users are responsible for complying with their local laws and regulations. The authors are not responsible for any misuse of this software.

---

**Made with ❤️ for a free and open internet**

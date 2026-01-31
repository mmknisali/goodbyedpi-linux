# GoodbyeDPI Linux - Docker Setup Guide

Complete guide for running GoodbyeDPI in a containerized environment without interfering with your host system.

## 🐳 Docker Setup Options

You have **three ways** to run GoodbyeDPI in Docker:

1. **Docker Compose** (Recommended - Easiest)
2. **Docker Run** (Manual control)
3. **Rootless Docker** (Most isolated, but complex)

---

## ⚡ Quick Start (Recommended)

### Option 1: Docker Compose

```bash
# 1. Build the container
docker-compose build

# 2. Start GoodbyeDPI
docker-compose up -d

# 3. Check logs
docker-compose logs -f

# 4. Stop GoodbyeDPI
docker-compose down
```

That's it! 🎉

---

## 📋 Detailed Instructions

### Prerequisites

1. **Docker installed**
```bash
# Check if Docker is installed
docker --version

# If not installed (Ubuntu/Debian):
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add your user to docker group (optional, to avoid sudo)
sudo usermod -aG docker $USER
# Log out and back in for this to take effect
```

2. **Docker Compose installed** (usually comes with Docker Desktop)
```bash
# Check version
docker-compose --version

# If not installed:
sudo apt install docker-compose  # Ubuntu/Debian
```

### Project Structure

```
goodbyedpi-linux/
├── Dockerfile                  # Container build instructions
├── docker-compose.yml          # Container orchestration
├── .dockerignore              # Files to exclude from build
├── CMakeLists.txt
├── src/
│   ├── main.c
│   ├── include/
│   └── ...
└── config/
    └── goodbyedpi.conf.example
```

---

## 🚀 Usage Methods

### Method 1: Docker Compose (Recommended)

**Advantages:**
- Simple configuration
- Easy to start/stop
- Automatic restart on failure
- Persistent logs

**Build and run:**
```bash
# Start in background
docker-compose up -d

# Start with logs visible
docker-compose up

# Stop
docker-compose down

# Restart
docker-compose restart

# View logs
docker-compose logs -f goodbyedpi

# Check status
docker-compose ps
```

**Custom configuration:**

Edit `docker-compose.yml` to change mode:
```yaml
command: ["-2", "-v"]  # Use mode 2 instead of 9
```

Or pass environment variables:
```yaml
environment:
  - GOODBYEDPI_MODE=2
```

### Method 2: Docker Run (Manual)

**Build the image:**
```bash
docker build -t goodbyedpi-linux:latest .
```

**Run the container:**
```bash
# Basic run with mode 9
docker run -d \
  --name goodbyedpi \
  --network host \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  --restart unless-stopped \
  goodbyedpi-linux:latest \
  -9 -v

# With custom configuration file
docker run -d \
  --name goodbyedpi \
  --network host \
  --cap-add NET_ADMIN \
  --cap-add NET_RAW \
  -v $(pwd)/config/goodbyedpi.conf.example:/etc/goodbyedpi/goodbyedpi.conf:ro \
  -v goodbyedpi-logs:/var/log/goodbyedpi \
  goodbyedpi-linux:latest \
  -c /etc/goodbyedpi/goodbyedpi.conf

# View logs
docker logs -f goodbyedpi

# Stop
docker stop goodbyedpi

# Remove
docker rm goodbyedpi
```

### Method 3: Privileged Mode (If cap-add doesn't work)

Some systems require full privileged mode:

**Docker Compose:**
```yaml
# In docker-compose.yml, uncomment:
privileged: true
```

**Docker Run:**
```bash
docker run -d \
  --name goodbyedpi \
  --network host \
  --privileged \
  goodbyedpi-linux:latest \
  -9 -v
```

---

## 🔧 Configuration

### Using Different Modes

**Mode 1 (Compatible):**
```bash
docker-compose run --rm goodbyedpi -1 -v
```

**Mode 9 (Full Features - Recommended for Turkey):**
```bash
docker-compose run --rm goodbyedpi -9 -v
```

**Custom Fragment Sizes:**
```bash
docker-compose run --rm goodbyedpi \
  --fragment-http 2 \
  --fragment-https 40 \
  --native-frag \
  -v
```

### Using Configuration File

1. Edit `config/goodbyedpi.conf.example`:
```bash
nano config/goodbyedpi.conf.example
```

2. Run with config:
```bash
docker-compose run --rm goodbyedpi -c /etc/goodbyedpi/goodbyedpi.conf
```

### Persistent Logs

Logs are stored in a Docker volume. To access them:

```bash
# View logs in real-time
docker-compose logs -f

# Access log files directly
docker exec goodbyedpi cat /var/log/goodbyedpi/goodbyedpi.log

# Copy logs to host
docker cp goodbyedpi:/var/log/goodbyedpi ./logs
```

---

## 🛠️ Troubleshooting

### Issue: "permission denied" or "operation not permitted"

**Solution 1:** Use privileged mode
```yaml
# In docker-compose.yml
privileged: true
```

**Solution 2:** Check Docker daemon is running
```bash
sudo systemctl status docker
sudo systemctl start docker
```

### Issue: "network not found" or connectivity problems

GoodbyeDPI **requires host networking** to intercept traffic.

**Verify host network:**
```bash
# Should show: "Network": "host"
docker inspect goodbyedpi | grep -A 10 NetworkSettings
```

**If using Docker Desktop (Windows/Mac):**

Docker Desktop doesn't fully support host networking. You have two options:

1. **Use Linux VM** (recommended):
   - Install VirtualBox or VMware
   - Run Ubuntu/Debian VM
   - Install Docker inside VM

2. **Use WSL2** (Windows only):
   ```bash
   # In WSL2 Ubuntu
   docker-compose up -d
   ```

### Issue: Container exits immediately

**Check logs:**
```bash
docker-compose logs goodbyedpi
```

**Common causes:**
- Missing NET_ADMIN capability → Add `--cap-add NET_ADMIN`
- Can't load kernel modules → Use privileged mode
- Another instance running → `sudo killall goodbyedpi`

### Issue: Internet stops working

**Stop the container:**
```bash
docker-compose down
```

**The container should clean up iptables rules automatically.**

If it doesn't:
```bash
# Manual cleanup
sudo iptables -D OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0 2>/dev/null
sudo iptables -D OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0 2>/dev/null
sudo iptables -D INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0 2>/dev/null
sudo iptables -D INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0 2>/dev/null
```

### Issue: "Failed to initialize netfilter queue"

**Load kernel module:**
```bash
# On host (not in container)
sudo modprobe nfnetlink_queue

# Make it persistent
echo "nfnetlink_queue" | sudo tee /etc/modules-load.d/goodbyedpi.conf
```

---

## 🔍 Monitoring and Debugging

### Real-time Monitoring

```bash
# View logs with timestamps
docker-compose logs -f --timestamps

# Check container status
docker-compose ps

# Check resource usage
docker stats goodbyedpi

# Check iptables rules (on host)
sudo iptables -L -n -v | grep NFQUEUE
```

### Debug Mode

```bash
# Run with debug logging
docker-compose run --rm goodbyedpi --debug -v

# Or modify docker-compose.yml:
command: ["--debug", "-v", "-9"]
```

### Interactive Shell in Container

```bash
# Access running container
docker exec -it goodbyedpi /bin/bash

# Check processes
ps aux | grep goodbyedpi

# Check network
ip addr
iptables -L -n
```

---

## 🎯 Testing

### Test if GoodbyeDPI is working

```bash
# 1. Start container
docker-compose up -d

# 2. Check it's running
docker-compose ps

# 3. Monitor logs
docker-compose logs -f

# 4. Try accessing a blocked site
curl -v https://www.blocked-site.com

# 5. Check statistics in logs
# Should see: "Packets processed: X, Packets modified: Y"
```

### Performance Testing

```bash
# Check CPU and memory usage
docker stats goodbyedpi

# Should be:
# - CPU: 1-5%
# - Memory: 5-10 MB
```

---

## 🔄 Updates and Maintenance

### Rebuild After Code Changes

```bash
# Rebuild image
docker-compose build --no-cache

# Restart with new image
docker-compose down
docker-compose up -d
```

### View Container Logs History

```bash
# All logs
docker-compose logs

# Last 100 lines
docker-compose logs --tail=100

# Logs since 1 hour ago
docker-compose logs --since 1h
```

### Backup Configuration

```bash
# Backup volume
docker run --rm \
  -v goodbyedpi-logs:/data \
  -v $(pwd):/backup \
  ubuntu tar czf /backup/goodbyedpi-logs.tar.gz /data
```

---

## 🚫 Limitations of Docker Setup

### What works:
✅ Packet interception and modification
✅ Traffic from the host machine
✅ All GoodbyeDPI features
✅ Easy start/stop without affecting system
✅ Clean isolation

### What doesn't work well:
❌ **Docker Desktop (Windows/Mac)** - Limited host networking
❌ **Rootless Docker** - Can't access raw sockets
❌ **Bridge networking** - Can't intercept host traffic

### Recommendation:
- **Linux host:** ✅ Full support
- **Windows:** Use WSL2 with Docker
- **macOS:** Use Linux VM (VirtualBox)

---

## 🔐 Security Notes

1. **Host Network Mode:** Container shares network with host
2. **NET_ADMIN Capability:** Container can modify network settings
3. **Privileged Mode:** If used, container has extended privileges
4. **Iptables:** Container modifies host iptables rules

**These are necessary for DPI bypass to work.**

To minimize risk:
- Only run when needed
- Use `docker-compose down` to stop
- Monitor logs for unexpected behavior
- Keep container image updated

---

## 📊 Example Docker Compose Configurations

### Minimal (Mode 9, Auto-restart)
```yaml
version: '3.8'
services:
  goodbyedpi:
    build: .
    network_mode: host
    cap_add: [NET_ADMIN, NET_RAW]
    restart: unless-stopped
    command: ["-9"]
```

### Verbose Logging
```yaml
version: '3.8'
services:
  goodbyedpi:
    build: .
    network_mode: host
    cap_add: [NET_ADMIN, NET_RAW]
    command: ["-9", "-v", "--debug"]
    volumes:
      - ./logs:/var/log/goodbyedpi
```

### Custom Configuration
```yaml
version: '3.8'
services:
  goodbyedpi:
    build: .
    network_mode: host
    cap_add: [NET_ADMIN, NET_RAW]
    volumes:
      - ./my-config.conf:/etc/goodbyedpi/goodbyedpi.conf:ro
    command: ["-c", "/etc/goodbyedpi/goodbyedpi.conf"]
```

---

## 🎓 Summary

**To run GoodbyeDPI in Docker:**

1. **Copy files** to your project directory
2. **Run:** `docker-compose up -d`
3. **Check:** `docker-compose logs -f`
4. **Stop:** `docker-compose down`

**The container is completely isolated and won't interfere with your system** - all network modifications are cleaned up when stopped!

**Need help?** Check the troubleshooting section or run with `--debug -v` for detailed logs.

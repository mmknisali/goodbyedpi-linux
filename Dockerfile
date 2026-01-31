FROM ubuntu:22.04

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies and runtime requirements
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libnetfilter-queue-dev \
    libmnl-dev \
    iptables \
    iproute2 \
    kmod \
    && rm -rf /var/lib/apt/lists/*

# Create directories
RUN mkdir -p /app/src /app/build /etc/goodbyedpi /var/log/goodbyedpi

# Set working directory
WORKDIR /app

# Copy source files
COPY src/ /app/src/
COPY CMakeLists.txt /app/
COPY config/goodbyedpi.conf.example /etc/goodbyedpi/goodbyedpi.conf

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DENABLE_SYSTEMD=OFF && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf build

# Create a non-root user (though we'll need root for network operations)
RUN useradd -r -s /bin/false goodbyedpi

# Expose no ports (we're intercepting traffic, not serving)
# But document that we need host networking
LABEL description="GoodbyeDPI Linux - DPI bypass utility"
LABEL version="0.2.3"

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pgrep goodbyedpi || exit 1

# Default command (mode 9 with verbose logging)
ENTRYPOINT ["/usr/bin/goodbyedpi"]
CMD ["-9", "-v", "--syslog"]

# STAGE 1: Build
FROM archlinux:latest AS builder

# Install build dependencies
RUN echo "Server = https://mirror.rackspace.com/archlinux/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm base-devel cmake libnetfilter_queue git

WORKDIR /build
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake .. && \
    make

# STAGE 2: Runner (Clean Image)
FROM archlinux:latest

# Install only runtime library
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm libnetfilter_queue && \
    pacman -Scc --noconfirm

# Copy the binary from builder
COPY --from=builder /build/build/goodbyedpi /usr/local/bin/goodbyedpi

# Entrypoint with Turkish ISP optimization
ENTRYPOINT ["/usr/local/bin/goodbyedpi"]
CMD ["-6", "-f", "1", "-k", "1", "-e", "1", "--not-fake-http", "--dns-addr", "1.1.1.1", "--dns-port", "53"]

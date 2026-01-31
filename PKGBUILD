# PKGBUILD for GoodbyeDPI Linux (Local Build)

pkgname=goodbyedpi-linux
pkgver=0.2.3rc3
pkgrel=1
pkgdesc="Linux DPI bypass and circumvention utility"
arch=('x86_64')
url="https://github.com/mmknisali/goodbyedpi-linux"
license=('Apache')
depends=('libnetfilter_queue' 'libmnl' 'iptables-nft' 'systemd')
makedepends=('cmake' 'make' 'gcc' 'pkg-config')
optdepends=('net-tools: for network interface detection')
backup=('etc/goodbyedpi/goodbyedpi.conf')

# We leave these empty because we are building from the local directory
source=()
sha256sums=()

prepare() {
    # $startdir points to the directory containing this PKGBUILD
    cd "$startdir"
    mkdir -p build
}

build() {
    cd "$startdir/build"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DENABLE_SYSTEMD=ON \
        -DENABLE_TESTING=OFF

    make -j$(nproc)
}

check() {
    cd "$startdir/build"

    if [[ -d ../tests ]]; then
        ctest --output-on-failure
    fi
}

package() {
    cd "$startdir/build"

    # Install main binary via Makefile
    make DESTDIR="$pkgdir/" install

    # Install systemd service
    install -Dm644 "$startdir/scripts/goodbyedpi.service" \
        "$pkgdir/usr/lib/systemd/system/goodbyedpi.service"

    # Install default configuration
    if [[ -f "$startdir/config/goodbyedpi.conf.example" ]]; then
        install -Dm644 "$startdir/config/goodbyedpi.conf.example" \
            "$pkgdir/etc/goodbyedpi/goodbyedpi.conf"
    fi

    # Install documentation
    install -Dm644 "$startdir/README.md" \
        "$pkgdir/usr/share/doc/$pkgname/README.md"

    # Install license
    if [[ -f "$startdir/LICENSE" ]]; then
        install -Dm644 "$startdir/LICENSE" \
            "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    fi

    # Install helper scripts
    install -Dm755 "$startdir/scripts/install.sh" \
        "$pkgdir/usr/share/$pkgname/install.sh"
    install -Dm755 "$startdir/scripts/uninstall.sh" \
        "$pkgdir/usr/share/$pkgname/uninstall.sh"

    # Create necessary runtime directories
    mkdir -p "$pkgdir/var/log"
    mkdir -p "$pkgdir/run"

    # Ensure correct permissions
    chmod 755 "$pkgdir/usr/bin/goodbyedpi"

    # Initialize an empty blacklist if it doesn't exist
    install -Dm644 /dev/null "$pkgdir/etc/goodbyedpi/blacklist.txt"
}

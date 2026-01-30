# PKGBUILD for GoodbyeDPI Linux

pkgname=goodbyedpi-linux
pkgver=0.2.3rc3
pkgrel=1
pkgdesc="Linux DPI bypass and circumvention utility"
arch=('x86_64')
url="https://github.com/mmknisali/goodbyedpi-linux/archive/refs/tags/v$pkgver.tar.gz"
license=('Apache')
depends=('libnetfilter_queue' 'libmnl' 'iptables-nft' 'systemd')
makedepends=('cmake' 'make' 'gcc' 'pkg-config')
optdepends=('net-tools: for network interface detection')
backup=('etc/goodbyedpi/goodbyedpi.conf')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

prepare() {
    cd "$srcdir/$pkgname-$pkgver"

    # Create build directory
    mkdir -p build
}

build() {
    cd "$srcdir/$pkgname-$pkgver/build"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DENABLE_SYSTEMD=ON \
        -DENABLE_TESTING=OFF

    make -j$(nproc)
}

check() {
    cd "$srcdir/$pkgname-$pkgver/build"

    # Run tests if enabled
    if [[ -d tests ]]; then
        ctest --output-on-failure
    fi
}

package() {
    cd "$srcdir/$pkgname-$pkgver/build"

    make DESTDIR="$pkgdir/" install

    # Install systemd service
    install -Dm644 "$srcdir/$pkgname-$pkgver/scripts/goodbyedpi.service" \
        "$pkgdir/usr/lib/systemd/system/goodbyedpi.service"

    # Install default configuration
    install -Dm644 "$srcdir/$pkgname-$pkgver/config/goodbyedpi.conf.example" \
        "$pkgdir/etc/goodbyedpi/goodbyedpi.conf"

    # Install documentation
    install -Dm644 "$srcdir/$pkgname-$pkgver/README.md" \
        "$pkgdir/usr/share/doc/$pkgname/README.md"

    # Install license
    install -Dm644 "$srcdir/$pkgname-$pkgver/LICENSE" \
        "$pkgdir/usr/share/licenses/$pkgname/LICENSE"

    # Install scripts
    install -Dm755 "$srcdir/$pkgname-$pkgver/scripts/install.sh" \
        "$pkgdir/usr/share/$pkgname/install.sh"
    install -Dm755 "$srcdir/$pkgname-$pkgver/scripts/uninstall.sh" \
        "$pkgdir/usr/share/$pkgname/uninstall.sh"

    # Create directories
    mkdir -p "$pkgdir/var/log"
    mkdir -p "$pkgdir/run"

    # Set permissions
    chmod 755 "$pkgdir/usr/bin/goodbyedpi"
    chmod 644 "$pkgdir/etc/goodbyedpi/goodbyedpi.conf"

    # Install default blacklist
    install -Dm644 /dev/null "$pkgdir/etc/goodbyedpi/blacklist.txt"
}

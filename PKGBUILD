pkgname=theme-manager
pkgver=1.0
pkgrel=1
pkgdesc="A GTK4 Theme Manager Application"
arch=('x86_64')
url="https://example.com"
license=('GPL')
depends=('gtk4')
source=("theme-manager.c" "favicon.svg" "theme-manager.desktop")
sha256sums=('SKIP' 'SKIP' 'SKIP')

build() {
    gcc $(pkg-config --cflags gtk4) -o theme-manager theme-manager.c $(pkg-config --libs gtk4)
}

package() {
    install -Dm755 "${srcdir}/theme-manager" "${pkgdir}/usr/bin/theme-manager"
    install -Dm644 "${srcdir}/favicon.svg" "${pkgdir}/usr/share/icons/hicolor/256x256/apps/theme-manager.svg"
    install -Dm644 "${srcdir}/theme-manager.desktop" "${pkgdir}/usr/share/applications/theme-manager.desktop"
}

clean() {
    rm -rf $srcdir $pkgdir
}

pkgname=theme-manager
pkgver=1.0
pkgrel=1
pkgdesc="A GTK4 Theme Manager Application"
arch=('x86_64')
url="https://example.com"
license=('GPL')
depends=('gtk4')
source=("theme-manager.c" "favicon.png")
sha256sums=('SKIP' 'SKIP')

package() {
    install -Dm755 "${srcdir}/theme-manager" "${pkgdir}/usr/bin/theme-manager"
    install -Dm644 "${srcdir}/favicon.png" "${pkgdir}/usr/share/icons/hicolor/256x256/apps/theme-manager.png"
    install -Dm644 "${srcdir}/theme-manager.desktop" "${pkgdir}/usr/share/applications/theme-manager.desktop"
}

EAPI=5

inherit flag-o-matic

SLOT="0"
DESCRIPTION="Bobcat (Brokken's Own Base Classes And Templates) library"
HOMEPAGE="http://bobcat.sourceforge.net/"
SRC_URI="mirror://sourceforge/${PN}/${P/-/_}.orig.tar.gz"

LICENSE="GPL-3"
KEYWORDS="~x86 ~amd64"
DEPEND="dev-util/icmake
	mail-filter/libmilter
	dev-libs/openssl
	x11-libs/libX11"

pkg_setup() {
	append-cxxflags -std=c++1y
	append-ldflags -pthread
}

src_compile() {
	./build libraries all strip || die
}

src_install() {
	./build install ${D} ${D}
}

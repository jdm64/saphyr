EAPI=5

inherit flag-o-matic

SLOT="0"
DESCRIPTION="Flex-style parser generator for C++"
HOMEPAGE="http://flexcpp.sourceforge.net/"
SRC_URI="mirror://sourceforge/flexcpp/${P/-/_}.orig.tar.gz"

LICENSE="GPL-3"
KEYWORDS="~x86 ~amd64"
DEPEND="dev-util/icmake
	dev-cpp/bobcat"

pkg_setup() {
	append-cxxflags -std=c++11
}

src_compile() {
	./build program strip || die
}

src_install() {
	mkdir -p ${D}/usr/bin/ ${D}/usr/share/flexc++
	cp tmp/bin/binary ${D}/usr/bin/flexc++
	cp skeletons/* ${D}/usr/share/flexc++/
}

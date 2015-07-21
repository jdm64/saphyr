EAPI=5

inherit git-r3

SLOT="0"
DESCRIPTION="A C-style compiler using LLVM"
HOMEPAGE="https://github.com/jdm64/saphyr"
EGIT_REPO_URI="https://github.com/jdm64/saphyr.git"

LICENSE="GPL-3"
KEYWORDS="~x86 ~amd64"
DEPEND="sys-devel/llvm
	sys-devel/flexc++
	sys-devel/bisonc++"

src_compile() {
	make
}

src_install() {
	mkdir -p ${D}/usr/bin
	cp saphyr ${D}/usr/bin/
}

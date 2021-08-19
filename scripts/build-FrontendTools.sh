#!/usr/bin/env bash

# exit on errors
set -e

BISON_VER="6.01.00-1"
BOBCAT_VER="4.08.02-2build1"
READ_VER="7.0-3"
TINFO_VER="6.1-1ubuntu1"
FLEX_VER="2.06.02-2"

AppImageToolBin=$(mktemp)
AppImageVer=13
wget "https://github.com/AppImage/AppImageKit/releases/download/$AppImageVer/appimagetool-x86_64.AppImage" -O "$AppImageToolBin"
chmod +x "$AppImageToolBin"

rm -rf FrontendTools
mkdir FrontendTools
cd FrontendTools

wget http://mirrors.kernel.org/ubuntu/pool/main/n/ncurses/libtinfo5_${TINFO_VER}_amd64.deb
ar x libtinfo5_${TINFO_VER}_amd64.deb
tar -xf data.tar.xz

wget http://mirrors.kernel.org/ubuntu/pool/main/r/readline/libreadline7_${READ_VER}_amd64.deb
ar x libreadline7_${READ_VER}_amd64.deb
tar -xf data.tar.xz

wget http://mirrors.kernel.org/ubuntu/pool/universe/b/bobcat/libbobcat4_${BOBCAT_VER}_amd64.deb
ar x libbobcat4_${BOBCAT_VER}_amd64.deb
tar -xf data.tar.xz

wget http://mirrors.kernel.org/ubuntu/pool/universe/b/bisonc++/bisonc++_${BISON_VER}_amd64.deb
ar x bisonc++_${BISON_VER}_amd64.deb
tar -xf data.tar.xz

wget http://mirrors.kernel.org/ubuntu/pool/universe/f/flexc++/flexc++_${FLEX_VER}_amd64.deb
ar x flexc++_${FLEX_VER}_amd64.deb
tar -xf data.tar.xz

mv lib/x86_64-linux-gnu/* usr/lib/
mv usr/lib/x86_64-linux-gnu/* usr/lib

patchelf --set-rpath '$ORIGIN/../lib' usr/bin/bisonc++
patchelf --set-rpath '$ORIGIN/../lib' usr/bin/flexc++
patchelf --set-rpath '$ORIGIN/../lib' usr/lib/libbobcat.so.4
patchelf --set-rpath '$ORIGIN/../lib' usr/lib/libreadline.so.7

sed -i -e 's#/usr/share#../share#g' usr/bin/bisonc++
sed -i -e 's#/usr/share#../share#g' usr/bin/flexc++

touch logo.png
rm -rf *.deb *.xz debian-binary etc lib usr/lib/x86_64-linux-gnu

# Build BisonC++

rm -f AppRun
ln -s usr/bin/bisonc++ AppRun

echo "
[Desktop Entry]
Name=bisonc++
Type=Application
Exec=bisonc++
Icon=logo
Terminal=true
Categories=Development;Building;ConsoleOnly;
" > bisonc++.desktop

cd ..

$AppImageToolBin -n --comp xz FrontendTools

# Build FlexC++

cd FrontendTools

rm -f AppRun
ln -s usr/bin/flexc++ AppRun

echo "
[Desktop Entry]
Name=flexc++
Type=Application
Exec=flexc++
Icon=logo
Terminal=true
Categories=Development;Building;ConsoleOnly;
" > flexc++.desktop

cd ..

$AppImageToolBin -n --comp xz FrontendTools

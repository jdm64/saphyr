#!/usr/bin/env bash

rm -rf DebPkg/usr
mkdir -p DebPkg/usr/bin

cp ../saphyr ../syfmt ./DebPkg/usr/bin/

dpkg-deb --build DebPkg

mv DebPkg.deb saphyr.deb

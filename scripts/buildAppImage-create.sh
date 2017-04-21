#!/usr/bin/env bash

AppImageToolBin=$(mktemp)

wget https://github.com/probonopd/AppImageKit/releases/download/8/AppRun-x86_64 -O AppImage/AppRun
wget https://github.com/probonopd/AppImageKit/releases/download/8/appimagetool-x86_64.AppImage -O $AppImageToolBin
chmod +x AppImage/AppRun
chmod +x $AppImageToolBin

$AppImageToolBin -n --comp xz AppImage

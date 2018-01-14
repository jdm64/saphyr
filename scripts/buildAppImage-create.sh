#!/usr/bin/env bash

AppImageToolBin=$(mktemp)

wget https://github.com/AppImage/AppImageKit/releases/download/10/AppRun-x86_64 -O AppImage/AppRun
wget https://github.com/AppImage/AppImageKit/releases/download/10/appimagetool-x86_64.AppImage -O $AppImageToolBin
chmod +x AppImage/AppRun
chmod +x $AppImageToolBin

$AppImageToolBin -n --comp xz AppImage

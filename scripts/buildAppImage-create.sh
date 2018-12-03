#!/usr/bin/env bash

AppImageToolBin=$(mktemp)
AppImageVer=11

wget https://github.com/AppImage/AppImageKit/releases/download/$AppImageVer/appimagetool-x86_64.AppImage -O $AppImageToolBin
chmod +x $AppImageToolBin

$AppImageToolBin -n --comp xz AppImage

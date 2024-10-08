#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DST=${SCRIPT_DIR}/../third_party/skia

if [ -e "${DST}/libskia.a" ]; then
    echo "Skia already built."
    exit 0
fi

mkdir -pv ${DST}/build
mkdir -pv ${DST}/include
cd ${DST}/build

if [ ! -e "skia-canvaskit-0.34.0" ]; then
    git clone 'https://chromium.googlesource.com/chromium/tools/depot_tools.git'
fi

export PATH="${PWD}/depot_tools:${PATH}"
wget https://github.com/google/skia/archive/refs/tags/canvaskit/0.34.0.tar.gz
tar -xvf 0.34.0.tar.gz
cd skia-canvaskit-0.34.0
python3 tools/git-sync-deps

bin/gn gen out/Static --args='is_official_build=true target_os="linux" target_cpu="x64" is_component_build=false skia_compile_modules=false skia_enable_tools=false skia_enable_pdf=false skia_enable_sksl=true skia_enable_skshaper=true skia_enable_svg=true skia_enable_gpu=true skia_use_gl=true skia_use_egl=true skia_use_angle=false skia_use_vulkan=false skia_use_dawn=false skia_use_metal=false skia_use_direct3d=false skia_use_expat=false skia_use_harfbuzz=true skia_use_icu=true skia_use_zlib=false skia_use_freetype=true skia_use_ffmpeg=false skia_use_sfml=false skia_use_sfntly=true skia_use_system_expat=false skia_use_system_harfbuzz=false skia_use_system_icu=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false skia_use_system_freetype2=false skia_enable_fontmgr_custom_directory=false text_tests_enabled=false skia_compile_sksl_tests=false'

ninja -C out/Static

cp out/Static/libskia.a ${DST}
cp -r include/* ${DST}/include
rm -rf ${DST}/build

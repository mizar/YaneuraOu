#!/bin/bash

cd `dirname $0`
cd ..

mkdir -p build/android/2018otafuku-kppt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
cp -r libs/* build/android/2018otafuku-kppt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT

mkdir -p build/android/2018otafuku-kpp_kkpt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
cp -r libs/* build/android/2018otafuku-kpp_kkpt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT

mkdir -p build/android/2018otafuku-material
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
cp -r libs/* build/android/2018otafuku-material
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL

mkdir -p build/android/2018tnk
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
ndk-build ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
cp -r libs/* build/android/2018tnk
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE

mkdir -p build/android/tnk-mate
ndk-build clean ENGINE_TARGET=MATE_ENGINE
ndk-build ENGINE_TARGET=MATE_ENGINE
cp -r libs/* build/android/tnk-mate
ndk-build clean ENGINE_TARGET=MATE_ENGINE

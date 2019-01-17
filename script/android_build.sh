#!/bin/bash

cd `dirname $0`
cd ..

mkdir -p build/android/2018otafuku-kppt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT 2>&1 | tee build/android/2018otafuku-kppt/2018otafuku-kppt.log
cp -r libs/* build/android/2018otafuku-kppt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT

mkdir -p build/android/2018otafuku-kpp_kkpt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT 2>&1 | tee build/android/2018otafuku-kpp_kkpt/2018otafuku-kpp_kkpt.log
cp -r libs/* build/android/2018otafuku-kpp_kkpt
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT

mkdir -p build/android/2018otafuku-material
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL 2>&1 | tee build/android/2018otafuku-material/2018otafuku-material.log
cp -r libs/* build/android/2018otafuku-material
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL

mkdir -p build/android/2018tnk
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
ndk-build ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE 2>&1 | tee build/android/2018tnk/2018tnk.log
cp -r libs/* build/android/2018tnk
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE

mkdir -p build/android/2018tnk-k-p
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE_K_P
ndk-build ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE_K_P 2>&1 | tee build/android/2018tnk-k-p/2018tnk-k-p.log
cp -r libs/* build/android/2018tnk-k-p
ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE_K_P

mkdir -p build/android/tnk-mate
ndk-build clean ENGINE_TARGET=MATE_ENGINE
ndk-build ENGINE_TARGET=MATE_ENGINE 2>&1 | tee build/android/tnk-mate/tnk-mate.log
cp -r libs/* build/android/tnk-mate
ndk-build clean ENGINE_TARGET=MATE_ENGINE

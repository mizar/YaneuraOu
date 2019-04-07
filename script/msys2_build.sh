#!/bin/bash
# -*- coding: utf-8 -*-
# MSYS2 (MinGW 64-bit) 上で Windows バイナリのビルド
# ビルド用パッケージの導入、パッケージの選択を促される画面ではそのままEnterキーを入力（全部をインストール）
# $ pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-clang mingw-w64-x86_64-openblas
# MSYS2パッケージの更新、更新出来る項目が無くなるまで繰り返し実行、場合によってはMinGWの再起動が必要
# $ pacman -Syuu
OS=Windows_NT
MAKE=mingw32-make
MAKEFILE=Makefile
JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`

cd `dirname $0`
cd ../source

COMPILER=clang++
BUILDDIR=../build/windows/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/2018tnk-k-p
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE_K_P
TARGET=YaneuraOu-2018-tnk-k-p-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-msys2-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018tnk-k-p
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE_K_P
TARGET=YaneuraOu-2018-tnk-k-p-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-msys2-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  nice ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

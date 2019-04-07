#!/bin/bash
# -*- coding: utf-8 -*-
# Ubuntu 上で Linux バイナリのビルド
MAKE=make
MAKEFILE=Makefile
JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`

cd `dirname $0`
cd ../source

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-linux-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kppt-linux-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-linux-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev libopenblas-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-linux-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-linux-clang
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-linux-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-linux-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-linux-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential libopenblas-dev
COMPILER=g++
BUILDDIR=../build/linux/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-linux-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-linux-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 sse41 sse2)
for BTG in ${TGTAIL[@]}; do
do
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}-${BTG}
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

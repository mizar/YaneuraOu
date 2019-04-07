#!/bin/bash
# -*- coding: utf-8 -*-
# Ubuntu 上で Windows バイナリのビルド (gcc)
OS=Windows_NT
MAKE=make
MAKEFILE=Makefile
JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`

cd `dirname $0`
cd ../source

# sudo apt install build-essential mingw-w64 libopenblas-dev
COMPILER64=x86_64-w64-mingw32-g++-posix
COMPILER32=i686-w64-mingw32-g++-posix
COMPILER=

BUILDDIR=../build/windows/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-mingw-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2 nosse)
for BTG in ${TGTAIL[@]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-mingw-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2 nosse)
for BTG in ${TGTAIL[@]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-mingw-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2 nosse)
for BTG in ${TGTAIL[@]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-mingw-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2 nosse)
for BTG in ${TGTAIL[@]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-mingw-gcc
TGTAIL=(avx512 avx2 sse42 tournament-avx512 tournament-avx2 tournament-sse42 evallearn-avx512 evallearn-avx2 evallearn-sse42 sse41 sse2 nosse)
for BTG in ${TGTAIL[@]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${BTG} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}-${BTG}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}-${BTG}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

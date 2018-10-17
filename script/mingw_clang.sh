#!/bin/bash
# -*- coding: utf-8 -*-
# Ubuntu 上で Windows バイナリのビルド (clang)
OS=Windows_NT
MAKE=make
MAKEFILE=Makefile
JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`

cd `dirname $0`
cd ../source

## Ubuntu 18.10 (cosmic)
# sudo apt install build-essential mingw-w64 debhelper cmake git clang clang-7 libomp-7-dev libopenblas-dev
# update-alternatives --set fakeroot /usr/bin/fakeroot-tcp
# update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix
# update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix
# update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
# update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
# git clone https://github.com/tpoechtrager/wclang
# pushd wclang
# debian/rules build
# fakeroot debian/rules binary
# popd
# sudo dpkg -i wclang_*.deb
COMPILER64=x86_64-w64-mingw32-clang++
COMPILER32=i686-w64-mingw32-clang++
COMPILER=

BUILDDIR=../build/windows/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-mingw-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2 [nosse]=-nosse)
for key in ${!TGTAIL[*]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-mingw-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2 [nosse]=-nosse)
for key in ${!TGTAIL[*]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-mingw-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [sse41]=-sse41 [sse2]=-sse2 [nosse]=-nosse)
for key in ${!TGTAIL[*]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-mingw-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2 [nosse]=-nosse)
for key in ${!TGTAIL[*]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

BUILDDIR=../build/windows/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-mingw-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [sse41]=-sse41 [sse2]=-sse2 [nosse]=-nosse)
for key in ${!TGTAIL[*]}; do
  if [ ${key} = 'nosse' ]; then COMPILER=${COMPILER32}; else COMPILER=${COMPILER64}; fi
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
  ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
  cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
  ${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

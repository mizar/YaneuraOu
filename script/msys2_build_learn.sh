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
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
done
${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}

COMPILER=clang++
BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-msys2-clang
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=clang++
BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-msys2-clang
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-msys2-gcc
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-msys2-gcc
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

COMPILER=g++
BUILDDIR=../build/windows/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-msys2-gcc
declare -A TGTAIL=([evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42)
for key in ${!TGTAIL[*]}; do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	nice -n 15 ${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} OS=${OS} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc.exe ${BUILDDIR}/${TARGET}${TGTAIL[$key]}.exe
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

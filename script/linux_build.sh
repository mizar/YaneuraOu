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
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kppt-linux-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-linux-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev libopenblas-dev
COMPILER=clang++
BUILDDIR=../build/linux/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-linux-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential clang libomp-7-dev
COMPILER=clang++
BUILDDIR=../build/linux/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-linux-clang
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-kppt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
TARGET=YaneuraOu-2018-otafuku-kppt-linux-gcc
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-kpp_kkpt
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
TARGET=YaneuraOu-2018-otafuku-kpp_kkpt-linux-gcc
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/2018otafuku-material
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
TARGET=YaneuraOu-2018-otafuku-material-linux-gcc
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential libopenblas-dev
COMPILER=g++
BUILDDIR=../build/linux/2018tnk
mkdir -p ${BUILDDIR}
EDITION=YANEURAOU_2018_TNK_ENGINE
TARGET=YaneuraOu-2018-tnk-linux-gcc
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [evallearn]=-evallearn-avx2 [evallearn-sse42]=-evallearn-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

# sudo apt install build-essential
COMPILER=g++
BUILDDIR=../build/linux/tnk-mate
mkdir -p ${BUILDDIR}
EDITION=MATE_ENGINE
TARGET=YaneuraOu-mate-linux-gcc
declare -A TGTAIL=([avx2]=-avx2 [sse42]=-sse42 [tournament]=-tournament-avx2 [tournament-sse42]=-tournament-sse42 [sse41]=-sse41 [sse2]=-sse2)
for key in ${!TGTAIL[*]}
do
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
	${MAKE} -f ${MAKEFILE} -j${JOBS} ${key} YANEURAOU_EDITION=${EDITION} COMPILER=${COMPILER} 2>&1 | tee $BUILDDIR/${TARGET}${TGTAIL[$key]}.log
	cp YaneuraOu-by-gcc ${BUILDDIR}/${TARGET}${TGTAIL[$key]}
	${MAKE} -f ${MAKEFILE} clean YANEURAOU_EDITION=${EDITION}
done

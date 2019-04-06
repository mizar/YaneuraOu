FROM ubuntu:18.10

WORKDIR /root

# Dockerfile: YaneuraOu Build Environment

# Using IIJ archive mirror
RUN sed -i.bak -e "s%http://archive.ubuntu.com/ubuntu/%http://ftp.iij.ad.jp/pub/linux/ubuntu/archive/%g" /etc/apt/sources.list

# apt install
RUN \
  apt-get update -y && \
  apt-get upgrade -y && \
  apt-get install -y software-properties-common && \
  add-apt-repository ppa:mati865/wclang && \
  apt-get update -y && \
  apt-get install -y build-essential clang clang-7 cmake curl debhelper git libomp-7-dev libopenblas-dev mingw-w64 openjdk-8-jdk p7zip-full unzip vim wclang && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/* && \
  update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix && \
  update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix && \
  update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix && \
  update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

# wclang install
#RUN git clone https://github.com/tpoechtrager/wclang && \
#  cd wclang && \
#  debian/rules build && \
#  fakeroot debian/rules binary && \
#  cd .. && \
#  dpkg -i wclang_*.deb && \
#  rm -rf wclang*

# Android-SDK install
# - https://developer.android.com/studio/#downloads
# - https://developer.android.com/studio/terms
# - Android SDK の再配布 (Android SDK を含む Docker Image の公開など) は禁じられている (see terms 3.4) ので注意
RUN curl -O https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip && \
  mkdir -p /usr/local/android-sdk && \
  unzip -d /usr/local/android-sdk sdk-tools-linux-4333796.zip && \
  rm -f sdk-tools-linux-4333796.zip && \
  yes | /usr/local/android-sdk/tools/bin/sdkmanager "ndk-bundle"

# Environment variables
ENV \
  JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 \
  ANDROID_HOME=/usr/local/android-sdk \
  PATH=/usr/local/android-sdk/platform-tools:/usr/local/android-sdk/tools:/usr/local/android-sdk/ndk-bundle:$PATH

# Copy build batches
COPY script/mingw_*.sh script/linux_build.sh script/android_build.sh script/sha256sums ./

# Modify path of build batches
RUN sed -i -e "s%cd \.\.%cd YaneuraOu%" *.sh

RUN \
  curl -O https://www.gnu.org/licenses/gpl.md && \
  curl -L -O https://github.com/HiraokaTakuya/apery/releases/download/WCSC28/apery_wcsc28.zip && \
  curl -L -O https://github.com/qhapaq-49/qhapaq-bin/releases/download/tagtest/qhwcsc28.7z && \
  curl -L -O https://www.qhapaq.org/static/media/bin/orqha.7z && \
  curl -L -O https://github.com/nodchip/tnk-/releases/download/wcsc28-2018-05-05/tnk-wcsc28-2018-05-05.7z && \
  curl -L -O https://github.com/yaneurao/YaneuraOu/releases/download/v4.73_book/standard_book.zip && \
  curl -L -O https://github.com/yaneurao/YaneuraOu/releases/download/v4.73_book/yaneura_book1_V101.zip && \
  curl -L -O https://github.com/yaneurao/YaneuraOu/releases/download/v4.73_book/yaneura_book3.zip && \
  unzip -d engine apery_wcsc28.zip && \
  7z x -oengine qhwcsc28.7z && \
  7z x -oengine orqha.7z && \
  7z x -oengine tnk-wcsc28-2018-05-05.7z && \
  unzip -d book standard_book.zip && \
  unzip -d book yaneura_book1_V101.zip && \
  unzip -d book yaneura_book3.zip && \
  sha256sum -c sha256sums

# Clone repositry
RUN git clone https://github.com/yaneurao/YaneuraOu.git && \
  cd YaneuraOu && \
  git remote add mizar https://github.com/mizar/YaneuraOu.git && \
  git remote add hakubishin- https://github.com/nodchip/hakubishin-.git && \
  git remote add tnk- https://github.com/nodchip/tnk-.git && \
  git remote add nnue https://github.com/ynasu87/nnue.git && \
  git remote add ai5 https://github.com/ai5/YaneuraOu.git && \
  git remote add kazu-nanoha https://github.com/kazu-nanoha/YaneuraOu.git && \
  git remote add YuriCat https://github.com/YuriCat/YaneuraOu.git && \
  git remote add tttak https://github.com/tttak/YaneuraOu.git && \
  git remote add qhapaq-49 https://github.com/qhapaq-49/YaneuraOu.git && \
  git remote add yuk-to https://github.com/yuk-to/YaneuraOu.git && \
  git remote add ishidakei https://github.com/ishidakei/YaneuraOu.git && \
  git fetch --all && \
  cd ..

# Makefiles overwrite
COPY source/Makefile YaneuraOu/source/
COPY jni/Android.mk jni/Application.mk YaneuraOu/jni/

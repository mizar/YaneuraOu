FROM ubuntu:18.10

WORKDIR /root

# Dockerfile: YaneuraOu Build Environment

# Using IIJ archive mirror
RUN sed -i.bak -e "s%http://archive.ubuntu.com/ubuntu/%http://ftp.iij.ad.jp/pub/linux/ubuntu/archive/%g" /etc/apt/sources.list

# apt install
RUN apt-get update -y && \
  apt-get upgrade -y && \
  apt-get install -y build-essential debhelper cmake mingw-w64 git clang openjdk-8-jdk unzip clang-7 libomp-7-dev libopenblas-dev vim && \
  apt-get clean && \
  update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix && \
  update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix && \
  update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix && \
  update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

# wclang install
RUN git clone https://github.com/tpoechtrager/wclang && \
  cd wclang && \
  debian/rules build && \
  fakeroot debian/rules binary && \
  cd .. && \
  dpkg -i wclang_*.deb && \
  rm -rf wclang*

# Android-SDK install
# - https://developer.android.com/studio/#downloads
# - https://developer.android.com/studio/terms
# - Android SDK の再配布 (Android SDK を含む Docker Image の公開など) は禁じられている (see terms 3.4) ので注意
RUN cd /usr/local && \
  curl https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip -O && \
  mkdir -p /usr/local/android-sdk && \
  unzip -d android-sdk sdk-tools-linux-4333796.zip && \
  rm -f sdk-tools-linux-4333796.zip && \
  yes | android-sdk/tools/bin/sdkmanager "ndk-bundle"

# Environment variables
ENV \
  JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 \
  ANDROID_HOME=/usr/local/android-sdk \
  PATH=/usr/local/android-sdk/platform-tools:/usr/local/android-sdk/tools:/usr/local/android-sdk/ndk-bundle:$PATH

# Copy build batches
COPY script/linux_build.sh script/android_build.sh script/mingw_gcc.sh script/mingw_clang.sh ./

# Modify path of build batches
RUN sed -i -e "s%cd \.\.%cd YaneuraOu%" *.sh

# Clone repositry
RUN git clone https://github.com/yaneurao/YaneuraOu.git && \
  cd YaneuraOu && \
  git remote add mizar http://github.com/mizar/YaneuraOu.git && \
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

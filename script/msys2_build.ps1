Set-Location (Join-Path $PSScriptRoot ..\source);
# msys2_shell.cmd -msys2 -defterm -no-start -l -c 'pacboy -Syuu --needed --noconfirm --noprogressbar toolchain:m clang:m openblas:m automake: autoconf: make: intltool: libtool: zip: unzip:';
@(
  @{
    COMPILER = 'clang++';
    TGCOMPILER = '-msys2-clang';
  };
  @{
    COMPILER = 'g++';
    TGCOMPILER = '-msys2-gcc';
  };
)|
ForEach-Object{@(
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = '2018otafuku-kppt';
    EDITION = 'YANEURAOU_2018_OTAFUKU_ENGINE_KPPT';
    TARGET = "YaneuraOu-2018-otafuku-kppt$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'evallearn-avx512';
      'evallearn-avx2';
      'evallearn-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = '2018otafuku-kpp_kkpt';
    EDITION = 'YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT';
    TARGET = "YaneuraOu-2018-otafuku-kpp_kkpt$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'evallearn-avx512';
      'evallearn-avx2';
      'evallearn-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = '2018otafuku-material';
    EDITION = 'YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL';
    TARGET = "YaneuraOu-2018-otafuku-material$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = '2018tnk';
    EDITION = 'YANEURAOU_2018_TNK_ENGINE';
    TARGET = "YaneuraOu-2018-tnk$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'evallearn-avx512';
      'evallearn-avx2';
      'evallearn-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = '2018tnk-k-p';
    EDITION = 'YANEURAOU_2018_TNK_ENGINE_K_P';
    TARGET = "YaneuraOu-2018-tnk-k-p$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'evallearn-avx512';
      'evallearn-avx2';
      'evallearn-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
  @{
    COMPILER = $_.COMPILER;
    BUILDDIR = 'tnk-mate';
    EDITION = 'MATE_ENGINE';
    TARGET = "YaneuraOu-tnk-mate$($_.TGCOMPILER)";
    TGTAIL = @(
      'avx512';
      'avx2';
      'sse42';
      'tournament-avx512';
      'tournament-avx2';
      'tournament-sse42';
      'sse41';
      'sse2';
      'nosse';
    );
  };
)}|
ForEach-Object{
  $OS = 'Windows_NT';
  $MAKE = 'mingw32-make';
  $MAKEFILE = 'Makefile';
  $Jobs = $env:NUMBER_OF_PROCESSORS;
  $Compiler = $_.COMPILER;
  $BuildDir = Join-Path '../build/windows/' $_.BUILDDIR;
  $Edition = $_.EDITION;
  $Target = $_.Target;
  if(-not (Test-Path $BuildDir)){
    New-Item $BuildDir -ItemType Directory -Force;
  }
  $_.TGTAIL|
  ForEach-Object{
    if ($_ -ne 'nosse') {
      Set-Item Env:MSYSTEM 'MINGW64';
    } else {
      Set-Item Env:MSYSTEM 'MINGW32';
    }
    msys2_shell.cmd -here -defterm -no-start -l -c "$MAKE -f $MAKEFILE clean YANEURAOU_EDITION=$Edition";
    $log = $null;
    msys2_shell.cmd -here -defterm -no-start -l -c "nice $MAKE -f $MAKEFILE -j$Jobs $_ YANEURAOU_EDITION=$Edition COMPILER=$Compiler OS=$OS 2>&1"|Tee-Object -Variable log
    $log|Out-File -Encoding utf8 -Force (Join-Path $BuildDir "$Target-$_.log");
    Copy-Item YaneuraOu-by-gcc.exe (Join-Path $BuildDir "$Target-$_.exe") -Force;
  };
  msys2_shell.cmd -here -defterm -no-start -l -c "$MAKE -f $MAKEFILE clean YANEURAOU_EDITION=$Edition";
};

$enc = ([Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes(@'
$ErrorActionPreference = 'Stop';
$ProgressPreference = 'SilentlyContinue';
Set-Location C:\;
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;
Invoke-WebRequest 'https://github.com/git-for-windows/git/releases/download/v2.19.1.windows.1/Git-2.19.1-64-bit.exe' -OutFile C:\Windows\Temp\Git-x64.exe;
Start-Process -FilePath C:\Windows\Temp\Git-x64.exe -ArgumentList /VERYSILENT, /NORESTART, /NOCANCEL, /SP- -NoNewWindow -PassThru -Wait;
Invoke-WebRequest 'https://www.7-zip.org/a/7z1900-x64.exe' -OutFile C:\Windows\Temp\7z-x64.exe;
Start-Process -FilePath C:\Windows\Temp\7z-x64.exe -ArgumentList /S -NoNewWindow -PassThru -Wait;
Invoke-WebRequest 'https://github.com/AdoptOpenJDK/openjdk8-binaries/releases/download/jdk8u202-b08/OpenJDK8U-jdk_x64_windows_hotspot_8u202b08.msi' -OutFile C:\Windows\Temp\OpenJDK8U-jdk_x64_windows.msi;
Start-Process -FilePath C:\Windows\System32\msiexec.exe -ArgumentList /i, C:\Windows\Temp\OpenJDK8U-jdk_x64_windows.msi, /passive -NoNewWindow -PassThru -Wait;
Invoke-WebRequest 'http://repo.msys2.org/distrib/msys2-x86_64-latest.tar.xz' -OutFile C:\Windows\Temp\msys2-x86_64-latest.tar.xz;
Start-Process -NoNewWindow -PassThru -Wait -FilePath 'C:\Program Files\7-Zip\7z.exe' -ArgumentList e, C:\Windows\Temp\msys2-x86_64-latest.tar.xz, '-oC:\Windows\Temp\';
Start-Process -NoNewWindow -PassThru -Wait -FilePath 'C:\Program Files\7-Zip\7z.exe' -ArgumentList x, C:\Windows\Temp\msys2-x86_64-latest.tar, '-oC:\';
Invoke-WebRequest 'https://dl.google.com/android/repository/sdk-tools-windows-4333796.zip' -OutFile C:\Windows\Temp\sdk-tools-windows.zip;
Expand-Archive -Path C:\Windows\Temp\sdk-tools-windows.zip -DestinationPath C:\Android\android-sdk;
Remove-Item @('C:\Windows\Temp\*', 'C:\Users\*\Appdata\Local\Temp\*') -Force -Recurse;
[Environment]::SetEnvironmentVariable('JAVA_HOME', 'C:\Program Files\AdoptOpenJDK\jdk-8.0.202.08', [EnvironmentVariableTarget]::Machine);
$env:PATH += ';C:\Program Files\Git\cmd;C:\msys64;C:\Android\android-sdk\ndk-bundle';
[Environment]::SetEnvironmentVariable('PATH', $env:PATH, [EnvironmentVariableTarget]::Machine);
'@)));

Out-File -InputObject @"
# escape=``
FROM mcr.microsoft.com/windows/servercore:1809
WORKDIR C:\
RUN ["powershell", "-ExecutionPolicy", "Bypass", "-EncodedCommand", "$enc"]
RUN ["C:\\msys64\\msys2_shell.cmd", "-msys2", "-defterm", "-no-start", "-lc", "exit"]
RUN ``
C:\msys64\usr\bin\bash.exe -lc 'pacman --needed --noconfirm --noprogressbar -Syuu'; ``
C:\msys64\usr\bin\bash.exe -lc 'pacman --needed --noconfirm --noprogressbar -Syu'; ``
C:\msys64\usr\bin\bash.exe -lc 'pacman --needed --noconfirm --noprogressbar -Su'; ``
C:\msys64\usr\bin\bash.exe -lc 'pacman --noconfirm -Scc';
RUN ``
C:\msys64\usr\bin\bash.exe -lc 'pacboy --needed --noconfirm --noprogressbar -S toolchain:m clang:m openblas:m automake: autoconf: make: intltool: libtool: zip: unzip:'; ``
C:\msys64\usr\bin\bash.exe -lc 'pacman --noconfirm -Scc';
RUN ["C:\\msys64\\usr\\bin\\bash.exe", "-lc 'yes | /C/Android/android-sdk/tools/bin/sdkmanager.bat ndk-bundle'"]
RUN git.exe clone https://github.com/mizar/YaneuraOu.git
WORKDIR C:\YaneuraOu\
"@ -FilePath (Join-Path $PSScriptRoot '../Dockerfile') -Encoding utf8 -Force;

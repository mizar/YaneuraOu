cd %~dp0\..
if not exist build mkdir build
if not exist build\android mkdir build\android

if not exist build\android mkdir build\android\2018otafuku-kppt
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
xcopy libs build\android\2018otafuku-kppt /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT

if not exist build\android mkdir build\android\2018otafuku-kpp_kkpt
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
xcopy libs build\android\2018otafuku-kpp_kkpt /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT

if not exist build\android mkdir build\android\2018otafuku-material
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
xcopy libs build\android\2018otafuku-material /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL

if not exist build\android mkdir build\android\2018tnk
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
call ndk-build ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
xcopy libs build\android\2018tnk /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE

if not exist build\android mkdir build\android\tnk-mate
call ndk-build clean ENGINE_TARGET=MATE_ENGINE
call ndk-build ENGINE_TARGET=MATE_ENGINE
xcopy libs build\android\tnk-mate /s /y
call ndk-build clean ENGINE_TARGET=MATE_ENGINE

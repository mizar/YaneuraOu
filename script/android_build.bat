cd %~dp0\..
if not exist build mkdir build
if not exist build\android mkdir build\android
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPPT
xcopy libs build\android /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT
xcopy libs build\android /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
call ndk-build ENGINE_TARGET=YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL
xcopy libs build\android /s /y
call ndk-build clean ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
call ndk-build ENGINE_TARGET=YANEURAOU_2018_TNK_ENGINE
xcopy libs build\android /s /y

Set-Location (Join-Path $PSScriptRoot ..);
@(
  @{
    Target = "YANEURAOU_2018_OTAFUKU_ENGINE_KPPT";
    Dir = ".\build\android\2018otafuku-kppt";
  };
  @{
    Target = "YANEURAOU_2018_OTAFUKU_ENGINE_KPP_KKPT";
    Dir = ".\build\android\2018otafuku-kpp_kkpt";
  };
  @{
    Target = "YANEURAOU_2018_TNK_ENGINE";
    Dir = ".\build\android\2018tnk";
  };
  @{
    Target = "YANEURAOU_2018_TNK_ENGINE_K_P";
    Dir = "build\android\2018tnk-k-p";
  };
  @{
    Target = "YANEURAOU_2018_OTAFUKU_ENGINE_MATERIAL";
    Dir = ".\build\android\2018otafuku-material";
  };
  @{
    Target = "MATE_ENGINE";
    Dir = "build\android\tnk-mate";
  };
)|
ForEach-Object{

$Target = $_.Target;
$Dir = $_.Dir;
$Jobs = $env:NUMBER_OF_PROCESSORS;

"`n# Build $Target to $Dir"|Out-Host;

if(-not (Test-Path $Dir)){
  "`n* Make Directory"|Out-Host;
  New-Item $Dir -ItemType Directory -Force;
}

"`n* Clean Build"|Out-Host;
ndk-build.cmd clean ENGINE_TARGET=$Target;

"`n* Build Binary"|Out-Host;
$log = $null;
ndk-build.cmd ENGINE_TARGET=$Target -j $Jobs|Tee-Object -Variable log;
$log|Out-File -Encoding utf8 -Force (Join-Path $Dir "build.log");

"`n* Copy Binary"|Out-Host;
Get-ChildItem .\libs -File -Recurse|
ForEach-Object{
  Copy-Item $_.PSPath -Destination $Dir -Force -PassThru;
};

"`n* Clean Build"|Out-Host;
ndk-build.cmd clean ENGINE_TARGET=$Target;

}

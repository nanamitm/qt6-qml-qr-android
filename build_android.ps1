param(
    [string]$QtVersion = "6.11.1",
    [string]$NdkVersion = "27.2.12479018",
    [string]$QtRoot = "C:\Qt",
    [string]$AndroidSdkRoot = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaHome = "C:\Program Files\Android\Android Studio\jbr",
    [string]$HostQtPath = "",
    [string]$NinjaPath = ""
)

$ErrorActionPreference = "Stop"

$buildDir = "build-android-debug"
$qtPrefix = Join-Path $QtRoot "$QtVersion\android_arm64_v8a"
$hostQtCandidates = @(
    (Join-Path $QtRoot "$QtVersion\mingw_64"),
    (Join-Path $QtRoot "$QtVersion\msvc2022_64"),
    (Join-Path $QtRoot "$QtVersion\llvm-mingw_64")
)

if (!$HostQtPath) {
    foreach ($candidate in $hostQtCandidates) {
        if (Test-Path "$candidate\lib\cmake\Qt6\Qt6Config.cmake") {
            $HostQtPath = $candidate
            break
        }
    }
}

$ninja = if ($NinjaPath) { $NinjaPath } elseif (Test-Path "$QtRoot\Tools\Ninja\ninja.exe") { "$QtRoot\Tools\Ninja\ninja.exe" } else { "ninja.exe" }
$env:JAVA_HOME = $JavaHome
$env:ANDROID_SDK_ROOT = $AndroidSdkRoot
$env:ANDROID_NDK_ROOT = "$env:ANDROID_SDK_ROOT\ndk\$NdkVersion"
$env:Path = "$env:JAVA_HOME\bin;$env:ANDROID_SDK_ROOT\platform-tools;$env:Path"

if (!(Test-Path "$qtPrefix\bin\qt-cmake.bat")) {
    throw "Qt Android kit not found: $qtPrefix"
}
if (!(Test-Path $env:ANDROID_NDK_ROOT)) {
    throw "Android NDK not found: $env:ANDROID_NDK_ROOT"
}

$configureArgs = @(
    "-S", ".",
    "-B", $buildDir,
    "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DQT_HOST_PATH=$HostQtPath",
    "-DANDROID_SDK_ROOT=$env:ANDROID_SDK_ROOT",
    "-DANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT"
)

& "$qtPrefix\bin\qt-cmake.bat" @configureArgs
& $ninja -C $buildDir

$debugApk = ".\$buildDir\android-build\build\outputs\apk\debug\android-build-debug.apk"
$resultApk = ".\$buildDir\QtQmlQrAndroid-debug.apk"

if (!(Test-Path $debugApk)) {
    throw "Debug APK was not generated: $debugApk"
}

Copy-Item -LiteralPath $debugApk -Destination $resultApk -Force
Write-Host "Debug APK: $resultApk"

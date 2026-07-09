param(
    [string]$QtVersion = "6.11.1",
    [string]$NdkVersion = "27.2.12479018",
    [string]$QtRoot = "C:\Qt",
    [string]$AndroidSdkRoot = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaHome = "C:\Program Files\Android\Android Studio\jbr",
    [string]$HostQtPath = "",
    [string]$CMakePath = "",
    [string]$NinjaPath = ""
)

$ErrorActionPreference = "Stop"

$buildDir = "build-android-release"
if (!(Test-Path ".\android-signing.properties")) {
    throw "android-signing.properties not found. Run .\create_android_keystore.ps1 first."
}

$signing = @{}
Get-Content ".\android-signing.properties" | ForEach-Object {
    if ($_ -match '^\s*([^#][^=]*)=(.*)$') {
        $signing[$matches[1].Trim()] = $matches[2]
    }
}

$storeFile = $signing.storeFile
if ([string]::IsNullOrWhiteSpace($storeFile)) {
    throw "storeFile is missing in android-signing.properties."
}
if (![System.IO.Path]::IsPathRooted($storeFile)) {
    $storeFile = Join-Path (Resolve-Path ".").Path $storeFile
}
$storeFile = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($storeFile)
if (!(Test-Path $storeFile)) {
    throw "Keystore file not found: $storeFile"
}

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

$cmake = if ($CMakePath) { $CMakePath } elseif (Test-Path "$QtRoot\Tools\CMake_64\bin\cmake.exe") { "$QtRoot\Tools\CMake_64\bin\cmake.exe" } else { "cmake.exe" }
$ninja = if ($NinjaPath) { $NinjaPath } elseif (Test-Path "$QtRoot\Tools\Ninja\ninja.exe") { "$QtRoot\Tools\Ninja\ninja.exe" } else { "ninja.exe" }
$env:JAVA_HOME = $JavaHome
$env:ANDROID_SDK_ROOT = $AndroidSdkRoot
$env:ANDROID_NDK_ROOT = "$env:ANDROID_SDK_ROOT\ndk\$NdkVersion"
$env:ANDROID_SIGNING_PROPERTIES = (Resolve-Path ".\android-signing.properties").Path
$env:ANDROID_storeFile = $storeFile
$env:ANDROID_storePassword = $signing.storePassword
$env:ANDROID_keyAlias = $signing.keyAlias
$env:ANDROID_keyPassword = $signing.keyPassword
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
    "-DCMAKE_BUILD_TYPE=Release",
    "-DQT_HOST_PATH=$HostQtPath",
    "-DANDROID_SDK_ROOT=$env:ANDROID_SDK_ROOT",
    "-DANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT"
)

& "$qtPrefix\bin\qt-cmake.bat" @configureArgs

$gradleReleaseApk = ".\$buildDir\android-build\build\outputs\apk\release\android-build-release.apk"
$releaseApk = ".\$buildDir\QtQmlQrAndroid-release.apk"

$stalePaths = @(
    "$buildDir\android-build\libs",
    "$buildDir\android-build\build\intermediates\merged_jni_libs",
    "$buildDir\android-build\build\intermediates\merged_native_libs",
    "$buildDir\android-build\build\intermediates\stripped_native_libs",
    $gradleReleaseApk,
    $releaseApk
)
foreach ($path in $stalePaths) {
    if (Test-Path $path) {
        Remove-Item -LiteralPath $path -Recurse -Force
    }
}

& $cmake --build $buildDir
$buildExitCode = $LASTEXITCODE

if (Test-Path $gradleReleaseApk) {
    Copy-Item -LiteralPath $gradleReleaseApk -Destination $releaseApk -Force
    Write-Host "Release APK: $releaseApk"
    exit 0
}

if ($buildExitCode -ne 0) {
    throw "Release build failed with exit code $buildExitCode."
}

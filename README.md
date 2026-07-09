# Qt 6 QML QR Scanner for Android

Qt 6 + QML + C++ で実装した Android 向け QR コードスキャナーです。

## 特徴

- `Qt Multimedia` を使ったカメラプレビュー
- `QML VideoOutput` ベースの全画面スキャナー UI
- `ZXing-C++` による QR コードデコード
- 実行時カメラ権限リクエスト
- 背面カメラ優先
- 重複読み取りのクールダウン
- 読み取り成功時の短いバイブ通知
- Android 向け署名付き release APK ビルドスクリプト付き

## フォルダー構成

```text
src/                       C++ 実装
qml/                       QML UI
android/package/           Android manifest と Gradle テンプレート
third_party/zxing-cpp/     ZXing-C++ のローカル同梱
create_android_keystore.ps1
build_android_release.ps1
```

GitHub に上げないローカル生成物:

```text
build/
build-*/
keystore/
android-signing.properties
```

## 必要な環境

- Qt 6.11.1 `android_arm64_v8a`
- Android SDK platform 36
- Android Build Tools 37.0.0
- Android NDK 27.2.12479018
- Java 17

## ZXing-C++ セットアップ

次のいずれかで解決できます。

1. `find_package(ZXing CONFIG)` で `ZXing::ZXing` を見つける
2. `third_party/zxing-cpp` を同梱する
3. `-DQR_APP_FETCH_ZXING=ON` で CMake に取得させる

この作業フォルダでは `third_party/zxing-cpp` を同梱しています。

## Android debug ビルド例

```powershell
C:\Qt\6.11.1\android_arm64_v8a\bin\qt-cmake.bat -S . -B build-android-arm64-debug -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:\Qt\Tools\Ninja\ninja.exe `
  -DQT_HOST_PATH=C:\Qt\6.11.1\msvc2022_64 `
  -DANDROID_SDK_ROOT=C:\Users\vt2v7\AppData\Local\Android\Sdk `
  -DANDROID_NDK_ROOT=C:\Users\vt2v7\AppData\Local\Android\Sdk\ndk\27.2.12479018

C:\Qt\Tools\Ninja\ninja.exe -C build-android-arm64-debug
```

## Android release APK

署名用 keystore は GitHub に含めず、ローカルだけで管理します。

keystore 作成:

```powershell
.\create_android_keystore.ps1
```

生成されるローカル秘密ファイル:

```text
keystore/qt-qr-scanner-release.jks
android-signing.properties
```

署名付き release APK:

```powershell
.\build_android_release.ps1
```

生成物:

```text
build-android-release/QtQmlQrAndroid-release.apk
```

`android-signing.properties.example` は公開用の雛形です。実際の `android-signing.properties` は `.gitignore` で除外しています。

## GitHub に上げる前の注意

- `third_party/zxing-cpp/.git` は残さず、通常フォルダとして管理する
- `keystore/` と `android-signing.properties` はコミットしない
- `build-*` や APK はコミットしない

## 実装メモ

- デコード処理は UI フリーズを避けるため別スレッドで実行しています
- `android/package/build.gradle` で release 署名設定を読み込みます
- 実機確認済みの構成は arm64-v8a です

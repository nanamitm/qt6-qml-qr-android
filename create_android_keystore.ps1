param(
    [string]$KeystorePath = ".\keystore\qt-qr-scanner-release.jks",
    [string]$Alias = "qtqrscanner",
    [string]$CommonName = "Qt QR Scanner",
    [string]$OrganizationalUnit = "Personal",
    [string]$Organization = "Codex",
    [string]$Country = "JP",
    [int]$ValidityDays = 10950,
    [string]$StorePassword = "",
    [string]$KeyPassword = ""
)

$ErrorActionPreference = "Stop"

$keytool = "C:\Program Files\Android\Android Studio\jbr\bin\keytool.exe"
if (!(Test-Path $keytool)) {
    throw "keytool not found: $keytool"
}

$keystoreFullPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($KeystorePath)
$keystoreDir = Split-Path -Parent $keystoreFullPath
New-Item -ItemType Directory -Force -Path $keystoreDir | Out-Null

if (Test-Path $keystoreFullPath) {
    throw "Keystore already exists: $keystoreFullPath"
}

if ([string]::IsNullOrWhiteSpace($StorePassword)) {
    $StorePassword = [Guid]::NewGuid().ToString("N") + "A1!"
}
if ([string]::IsNullOrWhiteSpace($KeyPassword)) {
    $KeyPassword = [Guid]::NewGuid().ToString("N") + "B2!"
}

$distinguishedName = "CN=$CommonName, OU=$OrganizationalUnit, O=$Organization, C=$Country"

& $keytool -genkeypair `
    -v `
    -keystore $keystoreFullPath `
    -storetype JKS `
    -alias $Alias `
    -keyalg RSA `
    -keysize 2048 `
    -validity $ValidityDays `
    -dname $distinguishedName `
    -storepass $StorePassword `
    -keypass $KeyPassword

@"
storeFile=$KeystorePath
storePassword=$StorePassword
keyAlias=$Alias
keyPassword=$KeyPassword
"@ | Set-Content -Encoding utf8NoBOM -Path ".\android-signing.properties"

Write-Host "Created keystore: $keystoreFullPath"
Write-Host "Created local signing properties: android-signing.properties"
Write-Host "Keep both files private. They are not for source control."

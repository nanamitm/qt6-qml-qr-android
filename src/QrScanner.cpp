#include "QrScanner.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QClipboard>
#include <QMetaObject>
#include <QPermission>
#include <QPermissions>
#include <QVariant>
#include <QDebug>
#include <QUrl>
#include <QtConcurrentRun>
#include <qcoreapplication_platform.h>

#include <Barcode.h>
#include <BarcodeFormat.h>
#include <ImageView.h>
#include <ReadBarcode.h>
#include <ReaderOptions.h>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

namespace {
constexpr qint64 kDecodeIntervalMs = 180;
constexpr int kFeedbackDurationMs = 35;
}

QrScanner::QrScanner(QObject* parent)
    : QObject(parent)
{
    m_decodeThrottle.invalidate();
    connect(&m_decodeWatcher, &QFutureWatcher<QString>::finished, this, [this]() {
        m_decodeInFlight = false;
        const QString text = m_decodeWatcher.result();
        if (!text.isEmpty())
            qInfo() << "Decoded QR text length" << text.size();
        if (!text.isEmpty() && shouldAcceptResult(text))
            emit decodeSucceeded(text);
    });
    connect(this, &QrScanner::decodeSucceeded, this, [this](const QString& text) {
        rememberAcceptedResult(text);
        setLastResult(text);
        setLastResultType(classifyResultType(text));
        setScanCount(m_scanCount + 1);
        setStatusText(QStringLiteral("QR code detected"));
        triggerScanFeedback();
    });
    connect(this, &QrScanner::errorOccurred, this, [this](const QString& message) {
        setStatusText(message);
    });
}

QObject* QrScanner::videoOutput() const
{
    return m_videoOutput;
}

void QrScanner::setVideoOutput(QObject* output)
{
    if (m_videoOutput == output)
        return;

    qInfo() << "QrScanner::setVideoOutput" << output;
    m_videoOutput = output;
    m_captureSession.setVideoOutput(output);
    attachVideoSink();
    emit videoOutputChanged();

    if (m_startRequested && !m_running)
        QMetaObject::invokeMethod(this, &QrScanner::start, Qt::QueuedConnection);
}

QString QrScanner::statusText() const
{
    return m_statusText;
}

QString QrScanner::lastResult() const
{
    return m_lastResult;
}

QString QrScanner::lastResultType() const
{
    return m_lastResultType;
}

QString QrScanner::primaryActionLabel() const
{
    return classifyPrimaryActionLabel(m_lastResult, m_lastResultType);
}

bool QrScanner::primaryActionAvailable() const
{
    return !m_lastResult.isEmpty() && !primaryActionLabel().isEmpty();
}

bool QrScanner::running() const
{
    return m_running;
}

bool QrScanner::hasResult() const
{
    return !m_lastResult.isEmpty();
}

int QrScanner::scanCount() const
{
    return m_scanCount;
}

int QrScanner::duplicateCooldownMs() const
{
    return m_duplicateCooldownMs;
}

void QrScanner::setDuplicateCooldownMs(int ms)
{
    const int sanitized = qMax(0, ms);
    if (m_duplicateCooldownMs == sanitized)
        return;

    m_duplicateCooldownMs = sanitized;
    emit duplicateCooldownMsChanged();
}

void QrScanner::start()
{
    if (m_running)
        return;

    m_startRequested = true;
    qInfo() << "QrScanner::start requested";

    if (!m_videoOutput) {
        qInfo() << "QrScanner::start waiting for video output";
        setStatusText(QStringLiteral("Waiting for camera preview..."));
        return;
    }

    auto startCamera = [this]() {
        logCameraDevices();
        const auto cameraDevice = chooseCameraDevice();
        if (cameraDevice.isNull()) {
            qWarning() << "No camera device is available";
            emit errorOccurred(QStringLiteral("No camera device is available"));
            return;
        }

        qInfo() << "Using camera device" << cameraDevice.description()
                << "id=" << cameraDevice.id()
                << "position=" << cameraDevice.position();

        m_camera = std::make_unique<QCamera>(cameraDevice);
        connectCameraSignals();
        m_captureSession.setCamera(m_camera.get());
        attachVideoSink();

        if (m_camera->isExposureModeSupported(QCamera::ExposureBarcode))
            m_camera->setExposureMode(QCamera::ExposureBarcode);
        if (m_camera->isFocusModeSupported(QCamera::FocusModeAutoNear))
            m_camera->setFocusMode(QCamera::FocusModeAutoNear);

        m_camera->start();
        m_running = true;
        m_decodeInFlight = false;
        m_decodeThrottle.restart();
        qInfo() << "Camera start issued. active=" << m_camera->isActive();
        setStatusText(QStringLiteral("Scanning for QR codes..."));
        emit runningChanged();
    };

    const auto status = qApp->checkPermission(QCameraPermission{});
    qInfo() << "Camera permission status" << status;
    if (status == Qt::PermissionStatus::Granted) {
        startCamera();
        return;
    }

    qApp->requestPermission(QCameraPermission{}, [this, startCamera](const QPermission& permission) {
        qInfo() << "Camera permission callback status" << permission.status();
        if (permission.status() != Qt::PermissionStatus::Granted) {
            emit errorOccurred(QStringLiteral("Camera permission was denied"));
            return;
        }

        startCamera();
    });
}

void QrScanner::stop()
{
    m_startRequested = false;
    if (!m_running)
        return;

    if (m_camera)
        m_camera->stop();
    m_camera.reset();
    m_captureSession.setCamera(nullptr);
    m_running = false;
    m_decodeInFlight = false;
    setStatusText(QStringLiteral("Scanner stopped"));
    emit runningChanged();
}

void QrScanner::clearLastResult()
{
    setLastResult(QString());
    setLastResultType(QString());
    setStatusText(QStringLiteral("Scanning for QR codes..."));
}

void QrScanner::triggerPrimaryAction()
{
    if (m_lastResult.isEmpty())
        return;

    if (openAssociatedContent(m_lastResult, m_lastResultType))
        return;

    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(m_lastResult);
        setStatusText(QStringLiteral("Result copied to clipboard"));
    } else {
        setStatusText(QStringLiteral("No app action is available for this result"));
    }
}

void QrScanner::processFrame(const QVideoFrame& frame)
{
    if (!m_running || m_decodeInFlight)
        return;
    if (m_decodeThrottle.isValid() && m_decodeThrottle.elapsed() < kDecodeIntervalMs)
        return;

    static int frameLogCount = 0;
    if (frameLogCount < 8) {
        ++frameLogCount;
        qInfo() << "processFrame" << frame.size() << frame.pixelFormat() << "valid=" << frame.isValid();
    }

    QVideoFrame clone(frame);
    if (!clone.isValid()) {
        qWarning() << "Received invalid video frame";
        return;
    }

    if (!clone.map(QVideoFrame::ReadOnly)) {
        qWarning() << "Failed to map video frame";
        return;
    }

    const QImage image = clone.toImage();
    clone.unmap();

    if (image.isNull()) {
        qWarning() << "Converted frame image is null";
        return;
    }

    m_decodeInFlight = true;
    m_decodeThrottle.restart();
    QImage frameImage = image;
    if (frameImage.width() > 960)
        frameImage = frameImage.scaledToWidth(960, Qt::SmoothTransformation);
    m_decodeWatcher.setFuture(QtConcurrent::run([this, image = std::move(frameImage)]() {
        return decodeImage(image);
    }));
}

void QrScanner::attachVideoSink()
{
    QVideoSink* sink = nullptr;
    if (m_videoOutput) {
        const QVariant sinkProperty = m_videoOutput->property("videoSink");
        if (sinkProperty.isValid())
            sink = qobject_cast<QVideoSink*>(sinkProperty.value<QObject*>());
    }
    if (!sink)
        sink = m_captureSession.videoSink();

    if (m_videoSink == sink)
        return;

    if (m_videoSink)
        disconnect(m_videoSink, nullptr, this, nullptr);

    m_videoSink = sink;
    qInfo() << "attachVideoSink output=" << m_videoOutput << "sink=" << m_videoSink;
    if (!m_videoSink) {
        qWarning() << "No video sink is available";
        return;
    }

    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &QrScanner::processFrame, Qt::UniqueConnection);
}

void QrScanner::setStatusText(const QString& text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void QrScanner::setLastResult(const QString& text)
{
    const bool hadResult = hasResult();
    if (m_lastResult == text)
        return;
    m_lastResult = text;
    emit lastResultChanged();
    emit primaryActionChanged();
    if (hadResult != hasResult())
        emit hasResultChanged();
}

void QrScanner::setLastResultType(const QString& type)
{
    if (m_lastResultType == type)
        return;

    m_lastResultType = type;
    emit lastResultTypeChanged();
    emit primaryActionChanged();
}

void QrScanner::setScanCount(int count)
{
    if (m_scanCount == count)
        return;

    m_scanCount = count;
    emit scanCountChanged();
}

QCameraDevice QrScanner::chooseCameraDevice() const
{
    const auto devices = QMediaDevices::videoInputs();
    for (const auto& device : devices) {
        if (device.position() == QCameraDevice::BackFace)
            return device;
    }
    return devices.isEmpty() ? QCameraDevice{} : devices.front();
}

QString QrScanner::decodeImage(const QImage& image) const
{
    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    const auto options = ZXing::ReaderOptions{}
                             .setFormats(ZXing::BarcodeFormat::QRCode)
                             .setTryRotate(true)
                             .setTryHarder(true)
                             .setMaxNumberOfSymbols(1);
    const ZXing::ImageView view(gray.constBits(), gray.width(), gray.height(), ZXing::ImageFormat::Lum);
    const auto result = ZXing::ReadBarcode(view, options);

    if (!result.isValid())
        return {};

    return QString::fromStdString(result.text());
}

QString QrScanner::classifyResultType(const QString& text) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QString upper = trimmed.left(64).toUpper();

    if (upper.startsWith(QStringLiteral("WIFI:")))
        return QStringLiteral("Wi-Fi settings");
    if (upper.startsWith(QStringLiteral("BEGIN:VCARD")) || upper.startsWith(QStringLiteral("MECARD:")))
        return QStringLiteral("Contact card");
    if (upper.startsWith(QStringLiteral("MAILTO:")) || upper.startsWith(QStringLiteral("MATMSG:")))
        return QStringLiteral("Email address");
    if (upper.startsWith(QStringLiteral("TEL:")))
        return QStringLiteral("Phone number");
    if (upper.startsWith(QStringLiteral("SMS:")) || upper.startsWith(QStringLiteral("SMSTO:")))
        return QStringLiteral("SMS message");
    if (upper.startsWith(QStringLiteral("GEO:")))
        return QStringLiteral("Map location");
    if (upper.startsWith(QStringLiteral("BEGIN:VEVENT")))
        return QStringLiteral("Calendar event");
    if (upper.startsWith(QStringLiteral("OTPAUTH:")))
        return QStringLiteral("Two-factor setup");

    const QUrl url(trimmed);
    if (url.isValid() && !url.scheme().isEmpty() &&
        (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))) {
        return QStringLiteral("Web link");
    }

    return QStringLiteral("Plain text");
}

QString QrScanner::classifyPrimaryActionLabel(const QString& text, const QString& type) const
{
    if (text.trimmed().isEmpty())
        return {};

    if (type == QStringLiteral("Web link"))
        return QStringLiteral("Open Link");
    if (type == QStringLiteral("Email address"))
        return QStringLiteral("Open Email");
    if (type == QStringLiteral("Phone number"))
        return QStringLiteral("Call");
    if (type == QStringLiteral("SMS message"))
        return QStringLiteral("Open SMS");
    if (type == QStringLiteral("Map location"))
        return QStringLiteral("Open Map");
    if (type == QStringLiteral("Wi-Fi settings"))
        return QStringLiteral("Open Wi-Fi");

    return QStringLiteral("Copy");
}

bool QrScanner::openAssociatedContent(const QString& text, const QString& type)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return false;

    if (type == QStringLiteral("Wi-Fi settings")) {
        if (openWifiSettings()) {
            setStatusText(QStringLiteral("Opened Wi-Fi settings"));
            return true;
        }
        return false;
    }

    const QUrl url(trimmed);
    if ((type == QStringLiteral("Web link") ||
         type == QStringLiteral("Email address") ||
         type == QStringLiteral("Phone number") ||
         type == QStringLiteral("SMS message") ||
         type == QStringLiteral("Map location")) && url.isValid() && !url.scheme().isEmpty()) {
        if (QDesktopServices::openUrl(url)) {
            setStatusText(QStringLiteral("Opened matching app"));
            return true;
        }
        return false;
    }

    return false;
}

bool QrScanner::openWifiSettings() const
{
#ifdef Q_OS_ANDROID
    using AndroidApp = QNativeInterface::QAndroidApplication;
    const QJniObject activity = AndroidApp::context();
    if (!activity.isValid())
        return false;

    const QJniObject action = QJniObject::fromString(QStringLiteral("android.settings.WIFI_SETTINGS"));
    QJniObject intent("android/content/Intent",
                      "(Ljava/lang/String;)V",
                      action.object<jstring>());
    if (!intent.isValid())
        return false;

    activity.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
    return true;
#else
    return false;
#endif
}

bool QrScanner::shouldAcceptResult(const QString& text) const
{
    if (text.isEmpty())
        return false;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool isDuplicate = text == m_lastAcceptedResult;
    const bool coolingDown = isDuplicate && (nowMs - m_lastAcceptedAtMs) < m_duplicateCooldownMs;
    return !coolingDown;
}

void QrScanner::rememberAcceptedResult(const QString& text)
{
    m_lastAcceptedResult = text;
    m_lastAcceptedAtMs = QDateTime::currentMSecsSinceEpoch();
}

void QrScanner::triggerScanFeedback() const
{
#ifdef Q_OS_ANDROID
    using AndroidApp = QNativeInterface::QAndroidApplication;
    const QJniObject context = AndroidApp::context();
    if (!context.isValid())
        return;

    const QJniObject serviceName = QJniObject::getStaticObjectField(
        "android/content/Context", "VIBRATOR_SERVICE", "Ljava/lang/String;");
    const QJniObject vibrator = context.callObjectMethod(
        "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;",
        serviceName.object<jstring>());
    if (!vibrator.isValid())
        return;

    if (QJniObject::isClassAvailable("android/os/VibrationEffect")) {
        const QJniObject effect = QJniObject::callStaticObjectMethod(
            "android/os/VibrationEffect",
            "createOneShot",
            "(JI)Landroid/os/VibrationEffect;",
            static_cast<jlong>(kFeedbackDurationMs),
            static_cast<jint>(-1));
        if (effect.isValid()) {
            vibrator.callMethod<void>(
                "vibrate",
                "(Landroid/os/VibrationEffect;)V",
                effect.object());
            return;
        }
    }

    vibrator.callMethod<void>("vibrate", "(J)V", static_cast<jlong>(kFeedbackDurationMs));
#endif
}

void QrScanner::logCameraDevices() const
{
    const auto devices = QMediaDevices::videoInputs();
    qInfo() << "Available camera count" << devices.size();
    for (const auto& device : devices) {
        qInfo() << "Camera device" << device.description()
                << "id=" << device.id()
                << "position=" << device.position();
    }
}

void QrScanner::connectCameraSignals()
{
    if (!m_camera)
        return;

    connect(m_camera.get(), &QCamera::activeChanged, this, [this](bool active) {
        qInfo() << "QCamera activeChanged" << active;
    });
    connect(m_camera.get(), &QCamera::errorChanged, this, [this]() {
        qWarning() << "QCamera errorChanged" << m_camera->error() << m_camera->errorString();
        if (m_camera->error() != QCamera::NoError)
            emit errorOccurred(m_camera->errorString());
    });
    connect(m_camera.get(), &QCamera::cameraDeviceChanged, this, [this]() {
        qInfo() << "QCamera cameraDeviceChanged" << m_camera->cameraDevice().description();
    });
}

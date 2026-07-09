#pragma once

#include <QCamera>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QObject>
#include <QPointer>
#include <QVideoFrame>
#include <QVideoSink>
#include <QStringView>
#include <QtGlobal>
#include <memory>

class QrScanner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastResult READ lastResult NOTIFY lastResultChanged)
    Q_PROPERTY(QString lastResultType READ lastResultType NOTIFY lastResultTypeChanged)
    Q_PROPERTY(QString primaryActionLabel READ primaryActionLabel NOTIFY primaryActionChanged)
    Q_PROPERTY(bool primaryActionAvailable READ primaryActionAvailable NOTIFY primaryActionChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY hasResultChanged)
    Q_PROPERTY(int scanCount READ scanCount NOTIFY scanCountChanged)
    Q_PROPERTY(int duplicateCooldownMs READ duplicateCooldownMs WRITE setDuplicateCooldownMs NOTIFY duplicateCooldownMsChanged)

public:
    explicit QrScanner(QObject* parent = nullptr);

    QObject* videoOutput() const;
    void setVideoOutput(QObject* output);

    QString statusText() const;
    QString lastResult() const;
    QString lastResultType() const;
    QString primaryActionLabel() const;
    bool primaryActionAvailable() const;
    bool running() const;
    bool hasResult() const;
    int scanCount() const;
    int duplicateCooldownMs() const;
    void setDuplicateCooldownMs(int ms);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clearLastResult();
    Q_INVOKABLE void triggerPrimaryAction();

signals:
    void videoOutputChanged();
    void statusTextChanged();
    void lastResultChanged();
    void lastResultTypeChanged();
    void primaryActionChanged();
    void runningChanged();
    void hasResultChanged();
    void scanCountChanged();
    void duplicateCooldownMsChanged();
    void decodeSucceeded(const QString& text);
    void errorOccurred(const QString& message);

private slots:
    void processFrame(const QVideoFrame& frame);

private:
    void attachVideoSink();
    void setStatusText(const QString& text);
    void setLastResult(const QString& text);
    void setLastResultType(const QString& type);
    void setScanCount(int count);
    QCameraDevice chooseCameraDevice() const;
    QString decodeImage(const QImage& image) const;
    QString classifyResultType(const QString& text) const;
    QString classifyPrimaryActionLabel(const QString& text, const QString& type) const;
    bool openAssociatedContent(const QString& text, const QString& type);
    bool openWifiSettings() const;
    bool shouldAcceptResult(const QString& text) const;
    void rememberAcceptedResult(const QString& text);
    void triggerScanFeedback() const;
    void logCameraDevices() const;
    void connectCameraSignals();

    QObject* m_videoOutput = nullptr;
    QMediaCaptureSession m_captureSession;
    std::unique_ptr<QCamera> m_camera;
    QPointer<QVideoSink> m_videoSink;
    QString m_statusText;
    QString m_lastResult;
    QString m_lastResultType;
    QString m_lastAcceptedResult;
    bool m_running = false;
    bool m_decodeInFlight = false;
    int m_scanCount = 0;
    int m_duplicateCooldownMs = 1800;
    qint64 m_lastAcceptedAtMs = 0;
    QElapsedTimer m_decodeThrottle;
    bool m_startRequested = false;
    QFutureWatcher<QString> m_decodeWatcher;
};

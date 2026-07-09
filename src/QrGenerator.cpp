#include "QrGenerator.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QPainter>

#include <cstdint>
#include <exception>
#include <string>

#include <qrcodegen.hpp>

QrGenerator::QrGenerator(QObject* parent)
    : QObject(parent)
{
    setStatusText(QStringLiteral("Enter text to create a QR code."));
}

QString QrGenerator::inputText() const
{
    return m_inputText;
}

void QrGenerator::setInputText(const QString& text)
{
    if (m_inputText == text)
        return;

    const bool oldCanGenerate = canGenerate();
    m_inputText = text;
    emit inputTextChanged();
    if (oldCanGenerate != canGenerate())
        emit canGenerateChanged();
}

QString QrGenerator::generatedImageUrl() const
{
    return m_generatedImageUrl;
}

QString QrGenerator::statusText() const
{
    return m_statusText;
}

bool QrGenerator::hasGenerated() const
{
    return !m_generatedImageUrl.isEmpty();
}

bool QrGenerator::canGenerate() const
{
    return !m_inputText.trimmed().isEmpty();
}

void QrGenerator::generate()
{
    if (!canGenerate()) {
        setStatusText(QStringLiteral("Enter text before generating a QR code."));
        return;
    }

    try {
        setGeneratedImageUrl(renderQrDataUrl(m_inputText));
        setStatusText(QStringLiteral("QR code generated."));
    } catch (const std::exception&) {
        setGeneratedImageUrl(QString());
        setStatusText(QStringLiteral("Could not generate a QR code for this text."));
    }
}

void QrGenerator::clear()
{
    const bool oldCanGenerate = canGenerate();
    m_inputText.clear();
    emit inputTextChanged();
    if (oldCanGenerate != canGenerate())
        emit canGenerateChanged();

    setGeneratedImageUrl(QString());
    setStatusText(QStringLiteral("Enter text to create a QR code."));
}

void QrGenerator::setGeneratedImageUrl(const QString& url)
{
    const bool hadGenerated = hasGenerated();
    if (m_generatedImageUrl == url)
        return;

    m_generatedImageUrl = url;
    emit generatedImageUrlChanged();
    if (hadGenerated != hasGenerated())
        emit hasGeneratedChanged();
}

void QrGenerator::setStatusText(const QString& text)
{
    if (m_statusText == text)
        return;

    m_statusText = text;
    emit statusTextChanged();
}

QString QrGenerator::renderQrDataUrl(const QString& text) const
{
    const std::string utf8 = text.toUtf8().toStdString();
    const auto qr = qrcodegen::QrCode::encodeText(utf8.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);

    constexpr int scale = 8;
    constexpr int borderModules = 4;
    const int qrSize = qr.getSize();
    const int imageSize = (qrSize + borderModules * 2) * scale;

    QImage image(imageSize, imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < qrSize; ++y) {
        for (int x = 0; x < qrSize; ++x) {
            if (!qr.getModule(x, y))
                continue;

            painter.drawRect((x + borderModules) * scale,
                             (y + borderModules) * scale,
                             scale,
                             scale);
        }
    }

    painter.end();

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    return QStringLiteral("data:image/png;base64,%1")
        .arg(QString::fromLatin1(pngBytes.toBase64()));
}

#pragma once

#include <QObject>
#include <QString>

class QrGenerator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputText READ inputText WRITE setInputText NOTIFY inputTextChanged)
    Q_PROPERTY(QString generatedImageUrl READ generatedImageUrl NOTIFY generatedImageUrlChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool hasGenerated READ hasGenerated NOTIFY hasGeneratedChanged)
    Q_PROPERTY(bool canGenerate READ canGenerate NOTIFY canGenerateChanged)

public:
    explicit QrGenerator(QObject* parent = nullptr);

    QString inputText() const;
    void setInputText(const QString& text);

    QString generatedImageUrl() const;
    QString statusText() const;
    bool hasGenerated() const;
    bool canGenerate() const;

    Q_INVOKABLE void generate();
    Q_INVOKABLE void clear();

signals:
    void inputTextChanged();
    void generatedImageUrlChanged();
    void statusTextChanged();
    void hasGeneratedChanged();
    void canGenerateChanged();

private:
    void setGeneratedImageUrl(const QString& url);
    void setStatusText(const QString& text);
    QString renderQrDataUrl(const QString& text) const;

    QString m_inputText;
    QString m_generatedImageUrl;
    QString m_statusText;
};

#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QCoreApplication>

class ConverterBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList outputFormats READ outputFormats NOTIFY outputFormatsChanged)
    Q_PROPERTY(QStringList audioCodecs READ audioCodecs NOTIFY audioCodecsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool progressKnown READ progressKnown NOTIFY progressChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)

public:
    explicit ConverterBackend(QObject *parent = nullptr);

    QStringList outputFormats() const;
    QStringList audioCodecs() const;
    bool busy() const;
    double progress() const;
    bool progressKnown() const;
    QString logText() const;

    Q_INVOKABLE void detectCapabilities();
    Q_INVOKABLE void convert(const QString &inputFile,
                             const QString &outputFile,
                             const QString &codec,
                             int bitrateKbps,
                             int sampleRate,
                             int channels,
                             const QString &extraArgs);
    Q_INVOKABLE void cancel();

signals:
    void outputFormatsChanged();
    void audioCodecsChanged();
    void busyChanged();
    void progressChanged();
    void logTextChanged();
    void conversionFinished(bool success, const QString &message);

private slots:
    void onFfmpegReadyRead();
    void onFfmpegFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProbeFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void appendLog(const QString &line);
    QString ffmpegProgram() const;
    QString ffprobeProgram() const;
    QString pathFromUrlString(const QString &value) const;
    double probeDurationSeconds(const QString &inputPath) const;
    void updateProgressFromOutput(const QString &text);
    static double parseClockToSeconds(const QString &clockText);

    QStringList m_outputFormats;
    QStringList m_audioCodecs;
    bool m_busy = false;
    double m_progress = 0.0;
    double m_totalDurationSeconds = -1.0;
    QString m_stderrBuffer;
    QString m_logText;
    QProcess m_ffmpeg;
    QProcess m_probe;
};

#include "converterbackend.h"

#include <algorithm>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

ConverterBackend::ConverterBackend(QObject *parent) : QObject(parent)
{
    connect(&m_ffmpeg, &QProcess::readyReadStandardError, this, &ConverterBackend::onFfmpegReadyRead);
    connect(&m_ffmpeg, &QProcess::readyReadStandardOutput, this, &ConverterBackend::onFfmpegReadyRead);
    connect(&m_ffmpeg, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &ConverterBackend::onFfmpegFinished);

    connect(&m_probe, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &ConverterBackend::onProbeFinished);
}

QStringList ConverterBackend::outputFormats() const
{
    return m_outputFormats;
}

QStringList ConverterBackend::audioCodecs() const
{
    return m_audioCodecs;
}

bool ConverterBackend::busy() const
{
    return m_busy;
}

double ConverterBackend::progress() const
{
    return m_progress;
}

bool ConverterBackend::progressKnown() const
{
    return m_totalDurationSeconds > 0.0;
}

QString ConverterBackend::logText() const
{
    return m_logText;
}

void ConverterBackend::appendLog(const QString &line)
{
    m_logText += line;
    if (!line.endsWith('\n')) {
        m_logText += '\n';
    }
    emit logTextChanged();
}

QString ConverterBackend::ffmpegProgram() const
{
    return QStringLiteral("ffmpeg");
}

QString ConverterBackend::ffprobeProgram() const
{
    return QStringLiteral("ffprobe");
}

QString ConverterBackend::pathFromUrlString(const QString &value) const
{
    const QUrl url(value);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return value;
}

double ConverterBackend::parseClockToSeconds(const QString &clockText)
{
    const QStringList parts = clockText.split(':');
    if (parts.size() != 3) {
        return -1.0;
    }
    bool okH = false;
    bool okM = false;
    bool okS = false;
    const double h = parts[0].toDouble(&okH);
    const double m = parts[1].toDouble(&okM);
    const double s = parts[2].toDouble(&okS);
    if (!okH || !okM || !okS) {
        return -1.0;
    }
    return h * 3600.0 + m * 60.0 + s;
}

double ConverterBackend::probeDurationSeconds(const QString &inputPath) const
{
    QProcess ffprobe;
    QStringList args;
    args << "-v" << "error"
         << "-show_entries" << "format=duration"
         << "-of" << "default=noprint_wrappers=1:nokey=1"
         << inputPath;
    ffprobe.start(ffprobeProgram(), args);
    if (!ffprobe.waitForFinished(5000)) {
        return -1.0;
    }
    if (ffprobe.exitStatus() != QProcess::NormalExit || ffprobe.exitCode() != 0) {
        return -1.0;
    }
    bool ok = false;
    const double duration = QString::fromUtf8(ffprobe.readAllStandardOutput()).trimmed().toDouble(&ok);
    if (!ok || duration <= 0.0) {
        return -1.0;
    }
    return duration;
}

void ConverterBackend::updateProgressFromOutput(const QString &text)
{
    static const QRegularExpression outTimeMsRx("out_time_ms=(\\d+)");
    static const QRegularExpression timeRx("time=(\\d{2}:\\d{2}:\\d{2}(?:\\.\\d+)?)");

    const auto msMatch = outTimeMsRx.match(text);
    if (msMatch.hasMatch()) {
        bool ok = false;
        const qint64 micros = msMatch.captured(1).toLongLong(&ok);
        if (ok && micros >= 0 && m_totalDurationSeconds > 0.0) {
            const double seconds = static_cast<double>(micros) / 1000000.0;
            const double ratio = std::clamp(seconds / m_totalDurationSeconds, 0.0, 1.0);
            if (!qFuzzyCompare(ratio, m_progress)) {
                m_progress = ratio;
                emit progressChanged();
            }
        }
        return;
    }

    const auto timeMatch = timeRx.match(text);
    if (timeMatch.hasMatch() && m_totalDurationSeconds > 0.0) {
        const double seconds = parseClockToSeconds(timeMatch.captured(1));
        if (seconds >= 0.0) {
            const double ratio = std::clamp(seconds / m_totalDurationSeconds, 0.0, 1.0);
            if (!qFuzzyCompare(ratio, m_progress)) {
                m_progress = ratio;
                emit progressChanged();
            }
        }
    }
}

void ConverterBackend::detectCapabilities()
{
    if (m_probe.state() != QProcess::NotRunning || m_busy) {
        return;
    }

    m_outputFormats.clear();
    m_audioCodecs.clear();
    emit outputFormatsChanged();
    emit audioCodecsChanged();

    appendLog("Detecting ffmpeg capabilities...");
    m_probe.start(ffmpegProgram(), QStringList() << "-hide_banner" << "-formats" << "-codecs");
}

void ConverterBackend::onProbeFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        appendLog("Could not query ffmpeg capabilities. Ensure ffmpeg is installed and in PATH.");
        emit conversionFinished(false, "ffmpeg not found or failed.");
        return;
    }

    const QString out = QString::fromUtf8(m_probe.readAllStandardOutput());
    const QString err = QString::fromUtf8(m_probe.readAllStandardError());
    const QString content = out + "\n" + err;

    QStringList formats;
    QStringList codecs;
    const QStringList lines = content.split('\n');
    bool inFormats = false;
    bool inCodecs = false;

    const QRegularExpression formatLine("^\\s*([D\\.][E\\.])\\s+([\\w,]+)\\s+");
    const QRegularExpression codecLine("^\\s*([D\\.][E\\.])([VAS\\.][FS\\.][SD\\.][TL\\.][SP\\.])\\s+([\\w_]+)\\s+");

    for (const QString &line : lines) {
        if (line.contains("File formats:")) {
            inFormats = true;
            inCodecs = false;
            continue;
        }
        if (line.contains("Codecs:")) {
            inFormats = false;
            inCodecs = true;
            continue;
        }

        if (inFormats) {
            const auto m = formatLine.match(line);
            if (m.hasMatch()) {
                const QString flags = m.captured(1);
                if (flags.contains('E')) {
                    const QStringList names = m.captured(2).split(',');
                    for (const QString &name : names) {
                        const QString trimmed = name.trimmed();
                        if (!trimmed.isEmpty() && !formats.contains(trimmed)) {
                            formats.append(trimmed);
                        }
                    }
                }
            }
        } else if (inCodecs) {
            const auto m = codecLine.match(line);
            if (m.hasMatch()) {
                const QString decodeEncode = m.captured(1);
                const QString mediaFlags = m.captured(2);
                const QString codecName = m.captured(3).trimmed();
                if (decodeEncode.contains('E') && mediaFlags.startsWith('A')) {
                    if (!codecName.isEmpty() && !codecs.contains(codecName)) {
                        codecs.append(codecName);
                    }
                }
            }
        }
    }

    if (formats.isEmpty()) {
        formats = {"mp3", "aac", "wav", "flac", "ogg", "opus", "m4a"};
    }
    if (codecs.isEmpty()) {
        codecs = {"libmp3lame", "aac", "pcm_s16le", "flac", "libopus", "libvorbis"};
    }

    std::sort(formats.begin(), formats.end());
    std::sort(codecs.begin(), codecs.end());
    m_outputFormats = formats;
    m_audioCodecs = codecs;
    emit outputFormatsChanged();
    emit audioCodecsChanged();
    appendLog(QString("Capabilities loaded: %1 formats, %2 audio codecs.")
                  .arg(m_outputFormats.size())
                  .arg(m_audioCodecs.size()));
}

void ConverterBackend::convert(const QString &inputFile,
                               const QString &outputFile,
                               const QString &codec,
                               int bitrateKbps,
                               int sampleRate,
                               int channels,
                               const QString &extraArgs)
{
    if (m_busy) {
        appendLog("Conversion is already running.");
        return;
    }

    const QString inPath = pathFromUrlString(inputFile);
    const QString outPath = pathFromUrlString(outputFile);
    if (!QFileInfo::exists(inPath)) {
        emit conversionFinished(false, "Input file does not exist.");
        return;
    }
    if (outPath.isEmpty()) {
        emit conversionFinished(false, "Output path is empty.");
        return;
    }

    QStringList args;
    args << "-hide_banner" << "-y" << "-nostats" << "-progress" << "pipe:2";
    args << "-i" << inPath;

    if (!codec.trimmed().isEmpty()) {
        args << "-c:a" << codec.trimmed();
    }
    if (bitrateKbps > 0) {
        args << "-b:a" << QString::number(bitrateKbps) + "k";
    }
    if (sampleRate > 0) {
        args << "-ar" << QString::number(sampleRate);
    }
    if (channels > 0) {
        args << "-ac" << QString::number(channels);
    }
    if (!extraArgs.trimmed().isEmpty()) {
        args << QProcess::splitCommand(extraArgs.trimmed());
    }

    args << outPath;

    m_totalDurationSeconds = probeDurationSeconds(inPath);
    m_progress = 0.0;
    m_stderrBuffer.clear();
    emit progressChanged();

    m_busy = true;
    emit busyChanged();
    appendLog(QString("Starting conversion:\nffmpeg %1").arg(args.join(' ')));
    m_ffmpeg.start(ffmpegProgram(), args);
}

void ConverterBackend::cancel()
{
    if (m_busy && m_ffmpeg.state() != QProcess::NotRunning) {
        appendLog("Cancelling conversion...");
        m_ffmpeg.kill();
    }
}

void ConverterBackend::onFfmpegReadyRead()
{
    const QString out = QString::fromUtf8(m_ffmpeg.readAllStandardOutput());
    const QString err = QString::fromUtf8(m_ffmpeg.readAllStandardError());
    if (!out.isEmpty()) {
        appendLog(out);
    }
    if (!err.isEmpty()) {
        m_stderrBuffer += err;
        const QStringList lines = m_stderrBuffer.split('\n');
        if (!m_stderrBuffer.endsWith('\n')) {
            m_stderrBuffer = lines.last();
            for (int i = 0; i < lines.size() - 1; ++i) {
                updateProgressFromOutput(lines.at(i));
            }
        } else {
            m_stderrBuffer.clear();
            for (const QString &line : lines) {
                updateProgressFromOutput(line);
            }
        }
        appendLog(err);
    }
}

void ConverterBackend::onFfmpegFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
    m_busy = false;
    emit busyChanged();
    if (ok) {
        m_progress = 1.0;
        emit progressChanged();
    }

    if (ok) {
        appendLog("Conversion completed successfully.");
        emit conversionFinished(true, "Conversion completed.");
    } else {
        appendLog(QString("Conversion failed (exit code %1).").arg(exitCode));
        emit conversionFinished(false, "Conversion failed. Check logs.");
    }
}

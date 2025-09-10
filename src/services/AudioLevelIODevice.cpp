#include "AudioLevelIODevice.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>

AudioLevelIODevice::AudioLevelIODevice(QFile* outputFile, QObject* parent)
    : QIODevice(parent)
    , m_outputFile(outputFile)
    , m_sampleRate(16000)
    , m_channelCount(1)
    , m_bytesPerSample(2)
    , m_currentLevel(0.0)
{
    // Set up the device as write-only (we're recording)
    setOpenMode(QIODevice::WriteOnly);
}

AudioLevelIODevice::~AudioLevelIODevice() {
    if (isOpen()) {
        close();
    }
}

bool AudioLevelIODevice::isSequential() const {
    return m_outputFile ? m_outputFile->isSequential() : true;
}

bool AudioLevelIODevice::open(OpenMode mode) {
    if (!m_outputFile) {
        setErrorString("No output file set");
        return false;
    }
    
    // We only support write mode for recording
    if (!(mode & QIODevice::WriteOnly)) {
        setErrorString("AudioLevelIODevice only supports write mode");
        return false;
    }
    
    if (m_outputFile->isOpen() || m_outputFile->open(mode)) {
        return QIODevice::open(mode);
    }
    
    setErrorString(m_outputFile->errorString());
    return false;
}

void AudioLevelIODevice::close() {
    if (m_outputFile && m_outputFile->isOpen()) {
        m_outputFile->close();
    }
    QIODevice::close();
}

qint64 AudioLevelIODevice::size() const {
    return m_outputFile ? m_outputFile->size() : 0;
}

qint64 AudioLevelIODevice::pos() const {
    return m_outputFile ? m_outputFile->pos() : 0;
}

bool AudioLevelIODevice::seek(qint64 pos) {
    if (m_outputFile && m_outputFile->seek(pos)) {
        return QIODevice::seek(pos);
    }
    return false;
}

bool AudioLevelIODevice::atEnd() const {
    return m_outputFile ? m_outputFile->atEnd() : true;
}

double AudioLevelIODevice::getCurrentLevel() const {
    QMutexLocker locker(&m_levelMutex);
    return m_currentLevel;
}

QByteArray AudioLevelIODevice::getLastAudioData() const {
    QMutexLocker locker(&m_levelMutex);
    return m_lastAudioData;
}

void AudioLevelIODevice::setAudioFormat(int sampleRate, int channelCount, int bytesPerSample) {
    QMutexLocker locker(&m_levelMutex);
    m_sampleRate = sampleRate;
    m_channelCount = channelCount;
    m_bytesPerSample = bytesPerSample;
    
    qDebug() << "AudioLevelIODevice format set:" 
             << "sampleRate=" << m_sampleRate
             << "channelCount=" << m_channelCount  
             << "bytesPerSample=" << m_bytesPerSample;
}

qint64 AudioLevelIODevice::readData(char* data, qint64 maxlen) {
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    
    // We don't support reading in recording mode
    setErrorString("Read not supported in write-only mode");
    return -1;
}

qint64 AudioLevelIODevice::writeData(const char* data, qint64 len) {
    if (!m_outputFile) {
        setErrorString("No output file available");
        return -1;
    }
    
    // Write data to the underlying file first
    qint64 bytesWritten = m_outputFile->write(data, len);
    
    if (bytesWritten > 0) {
        // Update audio level monitoring with the actual data being written
        updateLevel(data, bytesWritten);
    } else if (bytesWritten < 0) {
        setErrorString(m_outputFile->errorString());
    }
    
    return bytesWritten;
}

double AudioLevelIODevice::calculateRMSLevel(const char* data, qint64 len) const {
    if (!data || len <= 0 || m_bytesPerSample <= 0) {
        return 0.0;
    }
    
    // Calculate number of samples
    qint64 sampleCount = len / m_bytesPerSample;
    if (sampleCount == 0) {
        return 0.0;
    }
    
    double sum = 0.0;
    
    if (m_bytesPerSample == 2) {
        // 16-bit signed integer samples
        const int16_t* samples = reinterpret_cast<const int16_t*>(data);
        for (qint64 i = 0; i < sampleCount; ++i) {
            double sample = static_cast<double>(samples[i]) / 32768.0; // Normalize to [-1, 1]
            sum += sample * sample;
        }
    } else if (m_bytesPerSample == 4) {
        // 32-bit signed integer samples  
        const int32_t* samples = reinterpret_cast<const int32_t*>(data);
        for (qint64 i = 0; i < sampleCount; ++i) {
            double sample = static_cast<double>(samples[i]) / 2147483648.0; // Normalize to [-1, 1]
            sum += sample * sample;
        }
    } else if (m_bytesPerSample == 1) {
        // 8-bit unsigned samples
        const uint8_t* samples = reinterpret_cast<const uint8_t*>(data);
        for (qint64 i = 0; i < sampleCount; ++i) {
            double sample = (static_cast<double>(samples[i]) - 128.0) / 128.0; // Normalize to [-1, 1]
            sum += sample * sample;
        }
    } else {
        qWarning() << "Unsupported bytes per sample:" << m_bytesPerSample;
        return 0.0;
    }
    
    // Calculate RMS and clamp to [0, 1]
    double rms = qSqrt(sum / sampleCount);
    return qBound(0.0, rms, 1.0);
}

void AudioLevelIODevice::updateLevel(const char* data, qint64 len) {
    QMutexLocker locker(&m_levelMutex);
    
    // Calculate new level from the actual audio data
    double newLevel = calculateRMSLevel(data, len);
    m_currentLevel = newLevel;
    
    // Store a copy of the audio data (limited size for memory efficiency)
    qint64 dataSize = qMin(len, static_cast<qint64>(MAX_AUDIO_DATA_SIZE));
    m_lastAudioData = QByteArray(data, static_cast<int>(dataSize));
    
    // Emit signals (Qt will queue these if called from different thread)
    emit levelChanged(newLevel);
    emit audioDataReady(m_lastAudioData);
}

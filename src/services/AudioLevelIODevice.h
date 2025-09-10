#pragma once

#include <QIODevice>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>

/**
 * @brief AudioLevelIODevice - Wraps a QFile to monitor audio levels during recording
 * 
 * This class acts as a proxy between QAudioSource and the output file, allowing us to
 * calculate real-time audio levels from the actual audio stream being recorded.
 * It forwards all write operations to the underlying file while computing RMS levels.
 */
class AudioLevelIODevice : public QIODevice {
    Q_OBJECT

public:
    explicit AudioLevelIODevice(QFile* outputFile, QObject* parent = nullptr);
    ~AudioLevelIODevice() override;

    // QIODevice interface
    bool isSequential() const override;
    bool open(OpenMode mode) override;
    void close() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;

    // Audio level monitoring
    double getCurrentLevel() const;
    QByteArray getLastAudioData() const;
    void setAudioFormat(int sampleRate, int channelCount, int bytesPerSample);

signals:
    void levelChanged(double level);
    void audioDataReady(const QByteArray& data);

protected:
    // QIODevice interface - main data flow
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QFile* m_outputFile;
    mutable QMutex m_levelMutex;
    
    // Audio format parameters
    int m_sampleRate;
    int m_channelCount;
    int m_bytesPerSample;
    
    // Level monitoring
    double m_currentLevel;
    QByteArray m_lastAudioData;
    
    // Level calculation
    double calculateRMSLevel(const char* data, qint64 len) const;
    void updateLevel(const char* data, qint64 len);
    
    // Constants
    static constexpr int MAX_AUDIO_DATA_SIZE = 4096;
};

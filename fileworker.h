#ifndef FILEWORKER_H
#define FILEWORKER_H
#include <QObject>
#include <QStringList>
#include <atomic>
#include <mutex>
#include <condition_variable>

class FileWorker : public QObject {
    Q_OBJECT
public:
    explicit FileWorker(QObject* parent = nullptr);

    void pause();
    void resume();
    void cancel();

public slots:
    void processFiles(const QString& inputDir, const QString& outputDir,
        const QStringList& masks, bool deleteOriginal,
        int collisionAction, uint64_t xorKey);

signals:
    void progressUpdated(int percent);
    void logMessage(const QString& msg);
    void finished();

private:
    std::atomic<bool> m_isPaused{ false };
    std::atomic<bool> m_isCancelled{ false };
    std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;

    bool processSingleFile(const QString& inPath, const QString& outPath, uint64_t key,
        qint64 totalBytesToProcess, qint64& currentProcessedBytes);
};

#endif 
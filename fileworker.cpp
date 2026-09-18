#include "fileworker.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <fstream>
#include <vector>


FileWorker::FileWorker(QObject* parent) : QObject(parent) {}

void FileWorker::pause() { m_isPaused = true; }
void FileWorker::resume() { m_isPaused = false; m_pauseCv.notify_one(); }
void FileWorker::cancel() { m_isCancelled = true; resume(); }

void FileWorker::processFiles(const QString& inputDir, const QString& outputDir,
    const QStringList& masks, bool deleteOriginal,
    int collisionAction, uint64_t xorKey)
{
    m_isCancelled = false;
    m_isPaused = false;

    QDir inDir(inputDir);
    if (!inDir.exists()) 
    {
        emit logMessage("Ошибка: Входная директория не существует.");
        emit finished();
        return;
    }

    QDir outDir(outputDir);
    if (!outDir.exists()) outDir.mkpath(".");

     
    inDir.setNameFilters(masks);
    inDir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList fileList=inDir.entryInfoList();

    if (fileList.isEmpty()) 
    {
        emit logMessage("Файлы по заданным маскам не найдены.");
        emit finished();
        return;
    }

     
    qint64 totalBytes=0;
    for (const QFileInfo& fi : fileList) totalBytes+=fi.size();
    qint64 processedBytes=0;

    for (const QFileInfo& fileInfo : fileList) 
    {
        if (m_isCancelled) break;

        QString inPath = fileInfo.absoluteFilePath();
        QString outFileName = fileInfo.fileName();
        QString outPath = outDir.absoluteFilePath(outFileName);

         
        if (QFile::exists(outPath)) 
        {
            if (collisionAction==1) 
            { 
                int counter=1;
                QString baseName = fileInfo.completeBaseName();
                QString ext = fileInfo.suffix();
                if (!ext.isEmpty()) ext="."+ext;

                do 
                {
                    outPath = outDir.absoluteFilePath(baseName+"_"+QString::number(counter)+ext);
                    counter++;
                } while (QFile::exists(outPath));
            }
            else 
            {
                QFile::remove(outPath);  
            }
        }

        emit logMessage("Обработка файла: "+outFileName);
        bool success=processSingleFile(inPath, outPath, xorKey, totalBytes, processedBytes);
        if (success && deleteOriginal && !m_isCancelled) {
            QFile::remove(inPath);
            emit logMessage("Удален оригинал: "+inPath);
        }
    }
    if (m_isCancelled) emit logMessage("Операция прервана пользователем.");
    else emit logMessage("Обработка успешно завершена!");
    emit progressUpdated(100);
    emit finished();
}

bool FileWorker::processSingleFile(const QString& inPath, const QString& outPath, uint64_t key,
    qint64 totalBytesToProcess, qint64& currentProcessedBytes)
{
     
    std::ifstream inFile(inPath.toStdWString().c_str(), std::ios::binary);
    std::ofstream outFile(outPath.toStdWString().c_str(), std::ios::binary);
    if (!inFile.is_open() || !outFile.is_open()) 
    {
        emit logMessage("Ошибка открытия файлов для " + inPath);
        return false;
    }

    const size_t chunkSize=1024*1024*4;  
    std::vector<char> buffer(chunkSize);
    while (inFile) 
    {
        if (m_isCancelled) return false;

        if (m_isPaused) 
        {
            emit logMessage("Пауза...");
            std::unique_lock<std::mutex> lock(m_pauseMutex);
            m_pauseCv.wait(lock, [this]() { return !m_isPaused || m_isCancelled; });
            if (m_isCancelled) return false;
            emit logMessage("Продолжение...");
        }
        inFile.read(buffer.data(), chunkSize);
        std::streamsize bytesRead = inFile.gcount();
        if (bytesRead==0) break;
        size_t blocks = bytesRead/8;
        size_t remainder = bytesRead%8;

        uint64_t* data64 = reinterpret_cast<uint64_t*>(buffer.data());
        for (size_t i=0;i<blocks;++i) 
        {
            data64[i]^=key;
        }

        if (remainder>0) 
        {
            char* data8=buffer.data()+blocks*8;
            char* key8=reinterpret_cast<char*>(&key);
            for (size_t i=0;i<remainder;++i) 
            {
                data8[i]^=key8[i];
            }
        }

        outFile.write(buffer.data(), bytesRead);

        currentProcessedBytes+=bytesRead;
        int percent=static_cast<int>((currentProcessedBytes*100)/totalBytesToProcess);
        emit progressUpdated(percent);
    }
    return true;
}
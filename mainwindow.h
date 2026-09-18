#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QCloseEvent>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include "fileworker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent=nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onResumeClicked();
    void onTimerToggled(bool checked);
    void onTimerTick();
    void onWorkerFinished();

    void selectInputDir();
    void selectOutputDir();

private:
    void setupUi();

 
    QLineEdit* leInputDir, * leOutputDir, * leMasks, * leHexKey;
    QCheckBox* cbDeleteOriginal, * cbTimerMode;
    QComboBox* comboCollision;
    QSpinBox* spinTimerInterval;
    QPushButton* btnStart, * btnPause, * btnResume;
    QProgressBar* progressBar;
    QTextEdit* textLog;

 
    QThread* m_thread;
    FileWorker* m_worker;
    QTimer* m_timer;
    bool m_isProcessing{ false };
};

#endif 
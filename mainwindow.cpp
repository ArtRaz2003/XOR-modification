#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi(); 

    m_thread = new QThread(this);
    m_worker = new FileWorker();
    m_worker->moveToThread(m_thread);

    m_timer = new QTimer(this);
    connect(m_worker,&FileWorker::progressUpdated,progressBar,&QProgressBar::setValue);
    connect(m_worker,&FileWorker::logMessage,textLog,&QTextEdit::append);
    connect(m_worker,&FileWorker::finished,this,&MainWindow::onWorkerFinished);
    connect(m_thread,&QThread::finished,m_worker,&QObject::deleteLater);

    connect(btnStart,&QPushButton::clicked,this,&MainWindow::onStartClicked);
    connect(btnPause,&QPushButton::clicked,this,&MainWindow::onPauseClicked);
    connect(btnResume,&QPushButton::clicked,this,&MainWindow::onResumeClicked);
    connect(cbTimerMode,&QCheckBox::toggled,this,&MainWindow::onTimerToggled);
    connect(m_timer,&QTimer::timeout,this,&MainWindow::onTimerTick);
    m_thread->start();
}

MainWindow::~MainWindow() 
{
    m_thread->quit();
    m_thread->wait();
}

void MainWindow::closeEvent(QCloseEvent* event) 
{
    if (m_isProcessing) 
    {
        m_worker->cancel();
        m_thread->quit();
        m_thread->wait();  
    }
    event->accept();
}

void MainWindow::onStartClicked() 
{
    if (m_isProcessing) return;
    QString inDir=leInputDir->text();
    QString outDir= leOutputDir->text();
    QStringList masks=leMasks->text().split(",",Qt::SkipEmptyParts);
    for (auto& m : masks) m=m.trimmed();

    bool ok;
    uint64_t hexKey=leHexKey->text().toULongLong(&ok,16);
    if (!ok) 
    {
        QMessageBox::warning(this,"Ошибка","Некорректный Hex ключ!");
        return;
    }

    m_isProcessing=true;
    btnStart->setEnabled(false);
    btnPause->setEnabled(true);
    progressBar->setValue(0);
    textLog->append("--- Запуск операции ---");
    QMetaObject::invokeMethod(m_worker, [=]() {
        m_worker->processFiles(inDir,outDir,masks,cbDeleteOriginal->isChecked(),
            comboCollision->currentIndex(),hexKey);
        });
}

void MainWindow::onWorkerFinished() 
{
    m_isProcessing=false;
    btnPause->setEnabled(false);
    btnResume->setEnabled(false);
    if (!cbTimerMode->isChecked()) 
    {
        btnStart->setEnabled(true);
    }
}

void MainWindow::onPauseClicked() 
{
    m_worker->pause();
    btnPause->setEnabled(false);
    btnResume->setEnabled(true);
}

void MainWindow::onResumeClicked() 
{
    m_worker->resume();
    btnResume->setEnabled(false);
    btnPause->setEnabled(true);
}

void MainWindow::onTimerToggled(bool checked) 
{
    btnStart->setEnabled(!checked);  
    if (checked) 
    {
        m_timer->start(spinTimerInterval->value()*1000);
        textLog->append(QString("Таймер запущен. Интервал: %1 сек.").arg(spinTimerInterval->value()));
    }
    else 
    {
        m_timer->stop();
        textLog->append("Таймер остановлен.");
    }
}

void MainWindow::onTimerTick() {
    if (!m_isProcessing) {  
        onStartClicked();
    }
}


void MainWindow::selectInputDir() 
{
    QString dir=QFileDialog::getExistingDirectory(this,"Выбор входной директории");
    if (!dir.isEmpty()) leInputDir->setText(dir);
}

void MainWindow::selectOutputDir() 
{
    QString dir=QFileDialog::getExistingDirectory(this,"Выбор выходной директории");
    if (!dir.isEmpty()) leOutputDir->setText(dir);
}

void MainWindow::setupUi() 
{
    QWidget* centralWidget=new QWidget(this);
    QVBoxLayout* mainLayout=new QVBoxLayout(centralWidget);
    QFormLayout* formLayout=new QFormLayout();
    QHBoxLayout* inLayout=new QHBoxLayout();
    leInputDir=new QLineEdit("C:/Test/In");
    QPushButton* btnIn= new QPushButton("...");
    inLayout->addWidget(leInputDir); inLayout->addWidget(btnIn);
    connect(btnIn,&QPushButton::clicked,this,&MainWindow::selectInputDir);

    QHBoxLayout* outLayout=new QHBoxLayout();
    leOutputDir=new QLineEdit("C:/Test/Out");
    QPushButton* btnOut=new QPushButton("...");
    outLayout->addWidget(leOutputDir); outLayout->addWidget(btnOut);
    connect(btnOut,&QPushButton::clicked,this,&MainWindow::selectOutputDir);

    formLayout->addRow("Входная директория:",inLayout);
    formLayout->addRow("Выходная директория:",outLayout);
    leMasks=new QLineEdit("*.txt, *.bin");
    formLayout->addRow("Маски файлов (через запятую):",leMasks);

    leHexKey=new QLineEdit("1234567890ABCDEF");
    QRegularExpression rx("^[0-9A-Fa-f]{1,16}$");
    leHexKey->setValidator(new QRegularExpressionValidator(rx,this));
    formLayout->addRow("Hex-ключ (8 байт):",leHexKey);
    comboCollision=new QComboBox();
    comboCollision->addItems({ "Перезаписать", "Модифицировать имя (добавить счетчик)" });
    formLayout->addRow("Действие при совпадении имен:",comboCollision);
    cbDeleteOriginal=new QCheckBox("Удалять исходные файлы после успешной обработки");
    formLayout->addRow("",cbDeleteOriginal);


    QHBoxLayout* timerLayout=new QHBoxLayout();
    cbTimerMode=new QCheckBox("Работа по таймеру");
    spinTimerInterval=new QSpinBox();
    spinTimerInterval->setRange(1,3600);
    spinTimerInterval->setValue(10);
    spinTimerInterval->setSuffix(" сек");
    timerLayout->addWidget(cbTimerMode);
    timerLayout->addWidget(spinTimerInterval);
    timerLayout->addStretch();
    formLayout->addRow("Автоматизация:",timerLayout);

    mainLayout->addLayout(formLayout);

    QHBoxLayout* btnLayout=new QHBoxLayout();
    btnStart=new QPushButton("Запуск");
    btnPause=new QPushButton("Пауза"); btnPause->setEnabled(false);
    btnResume=new QPushButton("Возобновить"); btnResume->setEnabled(false);
    btnLayout->addWidget(btnStart); btnLayout->addWidget(btnPause); btnLayout->addWidget(btnResume);
    mainLayout->addLayout(btnLayout);

    progressBar=new QProgressBar();
    progressBar->setValue(0);
    mainLayout->addWidget(progressBar);
    textLog=new QTextEdit();
    textLog->setReadOnly(true);
    mainLayout->addWidget(textLog);
    setCentralWidget(centralWidget);
    setWindowTitle("Бинарный процессор файлов (XOR)");
    resize(600,450);
}
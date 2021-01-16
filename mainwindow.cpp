#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hpgldownloader.h"

#include <QLabel>
#include <QApplication>
#include <QIntValidator>
#include <QLineEdit>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QTextCodec>
#include <QSettings>
#include <QDir>
#include <QSharedMemory>
#include <QMessageBox>

static const char blankString[] = QT_TRANSLATE_NOOP("MainWindow", "N/A");
// 변수에 tr 사용 위해 QT_TRANSLATE_NOOP매크로 정의

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow),
      m_intValidator(new QIntValidator(0, 4000000, this)), // intVal 유효성 검사
      m_serial(new QSerialPort(this)),
      m_status(new QLabel)
{
    QCoreApplication::setOrganizationName(QString("gentlist"));
    QCoreApplication::setApplicationName(QString("bespokePlotter"));

    m_ui->setupUi(this);
    m_ui->baudRateBox->setInsertPolicy(QComboBox::NoInsert);

    connect(m_ui->applyButton, &QPushButton::clicked,
            this, &MainWindow::apply);
    connect(m_ui->comPortBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::showPortInfo);
    connect(m_ui->baudRateBox,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::checkCustomBaudRatePolicy);
    connect(m_ui->comPortBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::checkCustomDevicePathPolicy);
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    m_ui->statusBar->addWidget(m_status);
    fillPortsParameters();
    fillPortsInfo();
    updateSettings();
    registerProtocol();
    QSettings();
    initActionsConnections(); // 초기화
    apply();

    m_hpglDownloader = nullptr;
    connect(m_ui->startButton, SIGNAL(released()),this, SLOT(startSearch()));

    if (openSerialPort()){
        if (m_serial->isOpen()){
            qDebug() << "openSerialPort() true";
            qDebug() << "m_serial->isOpen() true";
            startSearch();
        } else {
            qDebug() << "m_serial->isOpen() false";
        }
    } else{
        qDebug() << "openSerialPort() false";
    }


}

//QString appName("HKEY_CURRENT_USER\\Software\\Classes\\MasterFitPlotter");
//QString path("HKEY_CURRENT_USER\\Software\\Classes\\MasterFitPlotter\\shell\\open\\command");
//QSettings setUrlProtocol(appName, QSettings::NativeFormat);
//QSettings setPath(path, QSettings::NativeFormat);
//const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
//const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\") + urlScheme;

// ========================================================================================================================
// App 실행 시 URL Scheme 등록 Protocol
bool MainWindow::registerProtocol()
{

   #ifdef Q_OS_WIN
    const QString urlScheme = "MasterFitPlotter";
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\") + urlScheme;
    QScopedPointer<QSettings> reg(new QSettings(regPath, QSettings::NativeFormat));

   reg->setValue(QStringLiteral("Default"), "MasterFit Plotter");
   reg->setValue(QStringLiteral("URL Protocol"), QString());
   reg->beginGroup(QStringLiteral("shell"));
   reg->beginGroup(QStringLiteral("open"));
   reg->beginGroup(QStringLiteral("command"));
   reg->setValue(QStringLiteral("Default"), appPath + QLatin1String(" %1"));

// main argument에 "masterfitplotter://" 스트링 포함 시 argument[1]값 inputUrl setText
   for(int i = 0; i < QApplication::arguments().count(); i++)
   {
      QString arg = QApplication::arguments().at(i);
      if(arg.contains("masterfitplotter://"))
      {
        QString arg1 = QApplication::arguments().at(1);
        QStringList list = arg1.split("masterfitplotter://");
        QString arg1Split = list.at(1);
        m_ui->inputUrl->setText(arg1Split);
      }
   }


   return true;

   #elif defined(Q_OS_UNIX)
   //TODO
   Logger::getInstance()->Info(tr("Cannot integrate with web browser - unsupported system"));
   return false;
   #endif
   return false;
}
// ========================================================================================================================
// url 입력 시 다운로드 이벤트 발생

void MainWindow::startSearch()
{
    qDebug() << "startSearch()";
    QUrl inputUrl = m_ui->inputUrl->text();
    m_hpglDownloader = new HpglDownloader(inputUrl,this);
    connect(m_hpglDownloader,SIGNAL(signalDownloaded()),this,SLOT(textDownloaded()));
}


// ========================================================================================================================
// s3 url에서 다운로드

void MainWindow::textDownloaded()
{
    QString data (m_hpglDownloader->downloadedData());
    qDebug("below is HPGL string data");
    qDebug() << data;
    const QByteArray requestData = data.toUtf8();
    m_serial->write(requestData);
    if(m_serial->waitForBytesWritten(100000)){
        qDebug("serial write done");
//        MainWindow::close();
//        qDebug("close");
    };

}


// ========================================================================================================================
// 소멸자
MainWindow::~MainWindow()
{
    delete m_ui;
}

// ========================================================================================================================
// 포트 연결 정보 setText

void MainWindow::showPortInfo(int idx)
{
    if (idx == -1)
        return;

    const QStringList list = m_ui->comPortBox->itemData(idx).toStringList();
    m_ui->descriptionLabel->setText(tr("Description: %1").arg(list.count() > 1 ? list.at(1) : tr(blankString)));
    m_ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(list.count() > 2 ? list.at(2) : tr(blankString)));
    m_ui->serialNumberLabel->setText(tr("Serial number: %1").arg(list.count() > 3 ? list.at(3) : tr(blankString)));
    m_ui->locationLabel->setText(tr("Location: %1").arg(list.count() > 4 ? list.at(4) : tr(blankString)));
    m_ui->vidLabel->setText(tr("Vendor Identifier: %1").arg(list.count() > 5 ? list.at(5) : tr(blankString)));
    m_ui->pidLabel->setText(tr("Product Identifier: %1").arg(list.count() > 6 ? list.at(6) : tr(blankString)));
}

// ========================================================================================================================
// 변경사항 적용

void MainWindow::apply()
{
    updateSettings();
    qDebug() << "apply()";
    showStatusMessage(tr("현재 선택된 포트 : %1").arg(m_currentSettings.name));
//    hide();
}

// ========================================================================================================================
// custom baudRate check

void MainWindow::checkCustomBaudRatePolicy(int idx)
{
    const bool isCustomBaudRate = !m_ui->baudRateBox->itemData(idx).isValid();
    m_ui->baudRateBox->setEditable(isCustomBaudRate);
    if (isCustomBaudRate) {
        m_ui->baudRateBox->clearEditText();
        QLineEdit *edit = m_ui->baudRateBox->lineEdit();
        edit->setValidator(m_intValidator);
    }
}

// ========================================================================================================================
// custom devicePath check

void MainWindow::checkCustomDevicePathPolicy(int idx)
{
    const bool isCustomPath = !m_ui->comPortBox->itemData(idx).isValid();
    m_ui->comPortBox->setEditable(isCustomPath);
    if (isCustomPath)
        m_ui->comPortBox->clearEditText();
}


// ========================================================================================================================
// fillPortsParameters

void MainWindow::fillPortsParameters()
{
    qDebug() << "fillPortsParameters()";
    m_ui->baudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    m_ui->baudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    m_ui->baudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    m_ui->baudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    m_ui->baudRateBox->addItem(tr("Custom"));

    m_ui->dataBitsBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
    m_ui->dataBitsBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
    m_ui->dataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_ui->dataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
    m_ui->dataBitsBox->setCurrentIndex(3);

    m_ui->parityBox->addItem(tr("None"), QSerialPort::NoParity);
    m_ui->parityBox->addItem(tr("Even"), QSerialPort::EvenParity);
    m_ui->parityBox->addItem(tr("Odd"), QSerialPort::OddParity);
    m_ui->parityBox->addItem(tr("Mark"), QSerialPort::MarkParity);
    m_ui->parityBox->addItem(tr("Space"), QSerialPort::SpaceParity);

    m_ui->stopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    m_ui->stopBitsBox->addItem(tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    m_ui->stopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    m_ui->flowControlBox->addItem(tr("None"), QSerialPort::NoFlowControl);
    m_ui->flowControlBox->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
    m_ui->flowControlBox->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);
}

// ========================================================================================================================
// fillPortsInfo

void MainWindow::fillPortsInfo()
{
    m_ui->parityBox->addItem(tr("TTTTT"), QSerialPort::SpaceParity);
    m_ui->comPortBox->clear();
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

        m_ui->comPortBox->addItem(list.first(), list);
    }

    m_ui->comPortBox->addItem(tr("Custom"));
}

// ========================================================================================================================
// updateSettings

void MainWindow::updateSettings()
{
    m_currentSettings.name = m_ui->comPortBox->currentText();

    if (m_ui->baudRateBox->currentIndex() == 4) {
        m_currentSettings.baudRate = m_ui->baudRateBox->currentText().toInt();
    } else {
        m_currentSettings.baudRate = static_cast<QSerialPort::BaudRate>(
                    m_ui->baudRateBox->itemData(m_ui->baudRateBox->currentIndex()).toInt());
    }
    m_currentSettings.stringBaudRate = QString::number(m_currentSettings.baudRate);

    m_currentSettings.dataBits = static_cast<QSerialPort::DataBits>(
                m_ui->dataBitsBox->itemData(m_ui->dataBitsBox->currentIndex()).toInt());
    m_currentSettings.stringDataBits = m_ui->dataBitsBox->currentText();

    m_currentSettings.parity = static_cast<QSerialPort::Parity>(
                m_ui->parityBox->itemData(m_ui->parityBox->currentIndex()).toInt());
    m_currentSettings.stringParity = m_ui->parityBox->currentText();

    m_currentSettings.stopBits = static_cast<QSerialPort::StopBits>(
                m_ui->stopBitsBox->itemData(m_ui->stopBitsBox->currentIndex()).toInt());
    m_currentSettings.stringStopBits = m_ui->stopBitsBox->currentText();

    m_currentSettings.flowControl = static_cast<QSerialPort::FlowControl>(
                m_ui->flowControlBox->itemData(m_ui->flowControlBox->currentIndex()).toInt());
    m_currentSettings.stringFlowControl = m_ui->flowControlBox->currentText();

    m_currentSettings.localEchoEnabled = m_ui->localEchoCheckBox->isChecked();
    qDebug() << "현재 선택된 포트 "+m_currentSettings.name;
}

// ========================================================================================================================
// openSerialPort


bool MainWindow::openSerialPort()
{
    qDebug() << "openSerialPort()";
    Settings p = m_currentSettings;
    m_serial->setPortName(p.name);
    qDebug() << p.name;
    m_serial->setBaudRate(p.baudRate);
    qDebug() << p.baudRate;
    m_serial->setDataBits(p.dataBits);
    qDebug() << p.dataBits;
    m_serial->setParity(p.parity);
    qDebug() << p.parity;
    m_serial->setStopBits(p.stopBits);
    qDebug() << p.stopBits;
    m_serial->setFlowControl(p.flowControl);
    qDebug() << p.flowControl;
if(m_serial->open(QIODevice::ReadWrite)) {
        m_ui->applyButton->setEnabled(false);
        m_ui->connButton->setEnabled(false);
        m_ui->disconnButton->setEnabled(true);
        showStatusMessage(tr("%1 포트에 연결되었습니다. 설정 정보: %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        return true;
    } else {

        QMessageBox::critical(this, tr("Error"), m_serial->errorString());
        showStatusMessage(tr("Open error"));
       return false;
    }
}

// ========================================================================================================================
// closeSerialPort

void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    qDebug() << "close port";

    m_ui->applyButton->setEnabled(true);
    m_ui->connButton->setEnabled(true);
    m_ui->disconnButton->setEnabled(false);
    showStatusMessage(tr("연결이 해제되었습니다."));
}

// ========================================================================================================================
// handleError

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

// ========================================================================================================================
// initActionsConnections

void MainWindow::initActionsConnections()
{
    qDebug() << "initActionsConnections";
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->connButton, &QPushButton::clicked, this, &MainWindow::openSerialPort);
    connect(m_ui->disconnButton, &QPushButton::clicked, this, &MainWindow::closeSerialPort);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}
// ========================================================================================================================
// about()

void MainWindow::about()
{

    QMessageBox::about(this, tr("BespokePlotter"),
                       tr("Gentlist CEO email  :  rlion1000@gmail.com"));
}




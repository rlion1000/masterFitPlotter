#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hpgldownloader.h"

#include <QLabel>
#include <QIntValidator>
#include <QLineEdit>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QTextCodec>
#include <QSettings>




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
    QSettings();
    initActionsConnections(); // 초기화

    m_hpglDownloader = nullptr;
    connect(m_ui->startButton, SIGNAL(released()),this, SLOT(startSearch()));
}

QString appName("HKEY_CURRENT_USER\\Software\\Classes\\BespokePlotter");
QString path("HKEY_CURRENT_USER\\Software\\Classes\\BespokePlotter\\shell\\open\\command");
QSettings setUrlProtocol(appName, QSettings::NativeFormat);
QSettings setPath(path, QSettings::NativeFormat);

void MainWindow::about()
{

    QMessageBox::about(this, tr("BespokePlotter"),
                       tr("<b>비스포크 플로터 프로그램을 사용해주셔서 감사합니다.</b>"
                          "웹 서버에 저장된 파일을 불러오기 위해선 사용자 PC에 OpenSSL이 설치되어 있어야 합니다."
                          "https://slproweb.com/products/Win32OpenSSL.html"
                          "비스포크 매니저 혹은 상기 경로에서 다운로드 및 설치 이후 사용해주시기 바랍니다"));
}

void MainWindow::QSettings()
{
    if(setPath.value("Default").toString() == NULL)
    {
    qDebug() << "reg NULL";
    qDebug() << "레지스트리 등록";
    setUrlProtocol.setValue("URL Protocol",QString(""));
    setPath.setValue("Default","C:\\Program Files\\BespokePlotterDemo\\BespokePlotter.exe");
    qDebug() << "레지스트리 등록 완료";
    qDebug() << setPath.value("Default").toString();
    }
    else
    {
    qDebug() << "reg NOT NULL";
    qDebug() << "등록된 레지스트리 있음";
    qDebug() << setPath.value("Default").toString();
    }
}


void MainWindow::startSearch()
{
    QUrl inputUrl = m_ui->inputUrl->text();
    qDebug() << "1";
    qDebug() << this;
    m_hpglDownloader = new HpglDownloader(inputUrl,this);
    qDebug() << "2";
    qDebug() << m_hpglDownloader;
    qDebug() << "3";
    qDebug() << SIGNAL(signalDownloaded());
    qDebug() << "4";
    qDebug() << SLOT(textDownloaded());


    connect(m_hpglDownloader,SIGNAL(signalDownloaded()),this,SLOT(textDownloaded()));

}

void MainWindow::textDownloaded()
{
//    qDebug() << "5";
//    qDebug() << m_hpglDownloader->downloadedData();

    QString data (m_hpglDownloader->downloadedData());
//    qDebug() << QSslSocket :: sslLibraryBuildVersionString ()  ;
//    qDebug() << QSslSocket::supportsSsl();
//    qDebug() << "6";
    //    qDebug() << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << data;
    const QByteArray requestData = data.toUtf8();
//    qDebug() << "7";
//    qDebug() << requestData;
    m_serial->write(requestData);
    qDebug("job is done");
}

//void MainWindow::writeData(const QByteArray &data)
//{
//    m_serial->write(data);
//}


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


void MainWindow::openSerialPort()
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
        m_ui->connButton->setEnabled(false);
        m_ui->disconnButton->setEnabled(true);
        showStatusMessage(tr("%1 포트에 연결되었습니다. 설정 정보: %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());
        showStatusMessage(tr("Open error"));
    }
}


// ========================================================================================================================
// closeSerialPort

void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    qDebug() << "close port";


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





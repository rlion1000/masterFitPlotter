#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QDesktopServices>


QT_BEGIN_NAMESPACE
class QLabel;
class QIntValidator;


namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class HpglDownloader;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
        bool localEchoEnabled;
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void incomingURL(QString path);
private slots:
//    void handleURL(const QUrl &url);
    void showPortInfo(int idx);
    void apply();
    void checkCustomBaudRatePolicy(int idx);
    void checkCustomDevicePathPolicy(int idx);

    bool openSerialPort();
    void closeSerialPort();
    void handleError(QSerialPort::SerialPortError error);
//    void writeData(const QByteArray &data);

    void startSearch();
    void textDownloaded();

private:
    bool registerProtocol();
//    void QSettings();
    void checkApplicationArguments();
    void fillPortsParameters();
    void fillPortsInfo();
    void updateSettings();
    void about();

private:
    void initActionsConnections();

private:
    void showStatusMessage(const QString &message);
    Ui::MainWindow *m_ui = nullptr;
    QIntValidator *m_intValidator = nullptr;
    QSerialPort *m_serial = nullptr;
    QLabel *m_status = nullptr;
    Settings m_currentSettings;
    Settings m_settings;

    HpglDownloader * m_hpglDownloader;
};
#endif // MAINWINDOW_H

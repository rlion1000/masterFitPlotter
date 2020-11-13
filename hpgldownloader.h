#ifndef HPGLDOWNLOADER_H
#define HPGLDOWNLOADER_H


#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


class HpglDownloader : public QObject
{
    Q_OBJECT

public:
    explicit HpglDownloader(QUrl hpglHpgl, QObject *parent = 0);
    virtual ~HpglDownloader();
    QByteArray downloadedData() const;


signals:
    void signalDownloaded();

private slots:
    void fileDownloaded(QNetworkReply* pReply);

private:
    QNetworkAccessManager m_WebCtrl;
    QByteArray m_DownloadedData;

};

#endif // HpglDOWNLOADER_H

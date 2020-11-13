#include "hpgldownloader.h"
#include <QAuthenticator>


HpglDownloader::HpglDownloader(QUrl url, QObject *parent) :
    QObject(parent)
{
    connect(&m_WebCtrl, SIGNAL (finished(QNetworkReply*)), this, SLOT (fileDownloaded(QNetworkReply*)) );
    QNetworkRequest request(url);
    m_WebCtrl.get(request);
}

HpglDownloader::~HpglDownloader()
{
}

void HpglDownloader::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();
    emit signalDownloaded();
}

QByteArray HpglDownloader::downloadedData() const {
    return m_DownloadedData;
}

#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QSharedMemory>
#include <QMessageBox>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QSharedMemory shared("MasterFitPlotter");
        if(!shared.create(512,QSharedMemory::ReadWrite))
        {
            QMessageBox::information(&w,QObject::tr("알림"),QObject::tr("프로그램이 중복 실행되어 종료됩니다."),QMessageBox::Ok);
            qDebug() << "exit(0)";
            exit(0);
        }
//    기존에 실행된 프로그램 있으면 경고 메시지 출력 후 프로그램 종료
    w.show();
    return a.exec();
}

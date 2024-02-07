#include "ctrlbar.h"
#include "ui_ctrlbar.h"
#include <QDebug>
CtrlBar::CtrlBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CtrlBar)
{
    ui->setupUi(this);
    // set play icon
    QIcon icon_play(":/ctrl/icon/play.png");
    ui->playBtn->setIcon(icon_play);

    // set stop icon
    QIcon icon_stop(":/ctrl/icon/stop.svg");
    ui->stopBtn->setIcon(icon_stop);

    // // set pause icon
    // QIcon icon_pause(":/ctrl/icon/pause.svg");
    // ui->

}

CtrlBar::~CtrlBar()
{
    delete ui;
}

void CtrlBar::on_playBtn_clicked()
{
    qDebug() << "on_playBtn_clicked";
}


void CtrlBar::on_stopBtn_clicked()
{
    qDebug() << "on_stopBtn_clicked";
}


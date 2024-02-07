#include "playlistwindow.h"
#include "ui_playlistwindow.h"

PlayListWindow::PlayListWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlayListWindow)
{
    ui->setupUi(this);
}

PlayListWindow::~PlayListWindow()
{
    delete ui;
}

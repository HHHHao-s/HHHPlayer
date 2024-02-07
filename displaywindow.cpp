#include "displaywindow.h"
#include "ui_displaywindow.h"

DisplayWindow::DisplayWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DisplayWindow)
{
    ui->setupUi(this);
}

DisplayWindow::~DisplayWindow()
{
    delete ui;
}

#include "titlebar.h"
#include "ui_titlebar.h"

titleBar::titleBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::titleBar)
{
    ui->setupUi(this);
}

titleBar::~titleBar()
{
    delete ui;
}

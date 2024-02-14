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

void DisplayWindow::enqueueFrame(std::shared_ptr<Frame> f)
{
	queue_.push(f);
    update();
}

void DisplayWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    auto f = queue_.tryPop();
    if (!f.get()) {
		return;
    }

    // 假设你有一个RGB24格式的数据
    uchar* rgb_data = (uchar*)f->frame_->data;
    int width = f->frame_->width;
    int height = f->frame_->height;

    // 创建一个QImage
    QImage image(rgb_data, width, height, QImage::Format_RGB888);

    // 使用QPainter来渲染这个QImage
    painter.drawImage(0, 0, image);
}

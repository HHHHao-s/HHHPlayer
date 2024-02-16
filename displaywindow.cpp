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
    if (!queue_) {
        queue_ = new BufferQueue<std::shared_ptr<Frame>>();
    }
    queue_->push(f);
    update();
}

void DisplayWindow::paintEvent(QPaintEvent* event)
{
    if (!queue_) {
		return;
	}

    QPainter painter(this);
    
    if (paused_) {
        updates_++;
		return;
	}

    auto f = queue_->tryPop();
    if (!f.get()) {
		return;
    }
  
    double diff = get_cur_time_() - f->pts_;

    

    while (diff < 0) {
        // current time is less than the frame pts, sleep for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        diff = get_cur_time_() - f->pts_;
    }
    if (diff > 0.040) {
        // too late
        // drop the frame
        /*painter.drawImage(0, 0, image_);*/
        while (!queue_->empty() && diff>0.04) {
            LOG_INFO("droped frame %d ", droped_frames_);
			f = queue_->pop();
            diff = get_cur_time_() - f->pts_;
		}
        
        
    }
    uchar* rgb_data = (uchar*)f->frame_->data[0];
    int width = f->frame_->width;
    int height = f->frame_->height;

    // 创建一个QImage
    image_ = QImage(rgb_data, width, height, QImage::Format_RGB888);

    // 使用QPainter来渲染这个QImage
    painter.drawImage(0, 0, image_);
}

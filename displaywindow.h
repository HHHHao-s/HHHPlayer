#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QWidget>
#include <QPainter>
#include "bufferqueue.h"

namespace Ui {
class DisplayWindow;
}

class DisplayWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayWindow(QWidget *parent = nullptr);
    ~DisplayWindow();
    
    void enqueueFrame(std::shared_ptr<Frame> f);

    void setGetCurTime(std::function<double()> get_cur_time) {
		get_cur_time_ = get_cur_time;
	}
    void stop() {
        queue_->abort();
        delete queue_;
        queue_ = nullptr;
    }
    void pause() {
		paused_ = true;
	}
    void play() {
        paused_ = false;
    }
    void pauseOrPlay() {
        if (paused_) {
            paused_ = false;
        }
        else {
            paused_ = true;
        }
    }

protected:
    void paintEvent(QPaintEvent *event) override;

private:

    BufferQueue<std::shared_ptr<Frame>>* queue_{nullptr};

    Ui::DisplayWindow *ui;

    std::function<double()> get_cur_time_{nullptr};

    QImage image_;

    int droped_frames_{0};

    bool paused_{false};

    int updates_{0};

};

#endif // DISPLAYWINDOW_H

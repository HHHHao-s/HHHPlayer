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

protected:
    void paintEvent(QPaintEvent *event) override;

private:

    BufferQueue<std::shared_ptr<Frame>> queue_;

    Ui::DisplayWindow *ui;
};

#endif // DISPLAYWINDOW_H

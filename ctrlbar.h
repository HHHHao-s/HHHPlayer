#ifndef CTRLBAR_H
#define CTRLBAR_H

#include <QWidget>

namespace Ui {
class CtrlBar;
}

class CtrlBar : public QWidget
{
    Q_OBJECT

public:
    explicit CtrlBar(QWidget *parent = nullptr);
    ~CtrlBar();

signals:
    // 调用的时候会自动将信号传递给槽函数
    void SigPlayOrPause();

    void SigStop();

private slots:
    void on_playBtn_clicked();

    void on_stopBtn_clicked();

private:
    Ui::CtrlBar *ui;
};

#endif // CTRLBAR_H

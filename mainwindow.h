#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qsignalmapper.h>
#include "mediaplayer.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int messageLoop(void* arg);

private:
    Ui::MainWindow *ui;
    int InitSignalsAndSlots();
    void OnPlayOrPause();
    MediaPlayer* mp_;

};
#endif // MAINWINDOW_H

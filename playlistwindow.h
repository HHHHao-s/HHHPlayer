#ifndef PLAYLISTWINDOW_H
#define PLAYLISTWINDOW_H

#include <QWidget>

namespace Ui {
class PlayListWindow;
}

class PlayListWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlayListWindow(QWidget *parent = nullptr);
    ~PlayListWindow();

private:
    Ui::PlayListWindow *ui;
};

#endif // PLAYLISTWINDOW_H

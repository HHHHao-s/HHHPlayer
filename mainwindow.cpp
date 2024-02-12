#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "logger.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // init the signal and slot
InitSignalsAndSlots();
}

int MainWindow::InitSignalsAndSlots() {
    connect(ui->ctrlBarWindow, &CtrlBar::SigPlayOrPause, this, &MainWindow::OnPlayOrPause);
    connect(ui->ctrlBarWindow, &CtrlBar::SigStop, this, &MainWindow::OnStop);
    return 0;
}

void MainWindow::OnPlayOrPause() {
	LOG_INFO("OnPlayOrPause");
    int ret = 0;
    if (!mp_) {
        mp_ = new MediaPlayer();
        ret = mp_->create(std::bind(&MainWindow::messageLoop, this, std::placeholders::_1));
        if (ret < 0) {
            LOG_ERROR("create media player failed");
            delete mp_;
            mp_ = nullptr;
            return;
        }
        mp_->setDataSource(ROOT_DIR  "/data/sync.mp4");
        ret = mp_->prepareAsync();
        if(ret<0){
			LOG_ERROR("prepareAsync failed");
            delete mp_;
            mp_ = nullptr;
			return;
		}

    }
    return;
}

void MainWindow::OnStop() {
	LOG_INFO("OnStop");
	if (mp_) {
		mp_->stop();
        mp_->destroy();
		delete mp_;
		mp_ = nullptr;
	}
	return;
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::messageLoop(void *arg) {
	MediaPlayer *mp = (MediaPlayer *)arg;
	int ret = 0;
	while (1) {
		Msg msg;
        ret = mp->blockGetMsg(msg);
        if (ret < 0) {
            LOG_ERROR("blockGetMsg failed");
            continue;
		}
        switch (msg.what_) {
            case MSG_NULL:
				break;
            case MSG_PREPARED:
                doPrepared();
                break;
            default:
				break;
        }
	}
	return 0;
}


int MainWindow::doPrepared() {
	LOG_INFO("doPrepared");
	if (mp_) {
		mp_->start();
	}
	return 0;
}
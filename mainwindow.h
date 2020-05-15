#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QQueue>
#include <QMutex>

#include "DecodecThread.h"
#include "AudioPlayer.h"
#include "VideoDownloadDlg.h"
#include "ScreenCaptureDlg.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
	void setCentralImage(QImage image);
	void initUiAndControl();
	void addImage(QImage image);
	void getOutImage(QImage &tempImage);

	//1表示 添加  2表示 取出
	void operateImageQueue(int saveOrGetOut, QImage &image);

public slots:
    void onShowFlag();
    void onGotOneDecodecPic(QImage srcImage, const QString strID);
	void onStart();
	void onStop();
	void onUpdateMediaSource();

private:
    Ui::MainWindow *ui;
	QTimer m_pTimer;
	QWidget *m_pCentralWidget;
	QLabel *m_pCentralCanvas;

	QQueue<QImage> m_pImagesQueue;
	QMutex m_pMutext;

	DecodecThread *m_pDecodecThread;
	AudioPlayer *m_pAudioPlayer;
	VideoDownloadDlg *m_pSourceChooseDlg;
};

#endif // MAINWINDOW_H

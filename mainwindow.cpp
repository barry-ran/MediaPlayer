#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ScreenCaptureDlg.h"

#include <QAction>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	m_pSourceChooseDlg = new VideoDownloadDlg(this);
	m_pCentralCanvas = new QLabel(this);
	m_pCentralWidget = new QWidget(this);
	m_pCentralWidget->setStyleSheet("background:blue;");
	this->setCentralWidget(m_pCentralWidget);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(m_pCentralCanvas);
	m_pCentralWidget->setLayout(layout);

	initUiAndControl();

	m_pTimer.setInterval(30);
	m_pDecodecThread = new DecodecThread(this);
	connect(&m_pTimer, SIGNAL(timeout()), this, SLOT(onShowFlag()));
	connect(m_pDecodecThread, SIGNAL(onSigDecodecOnePicture(QImage, const QString)), this, SLOT(onGotOneDecodecPic(QImage, const QString)));

	m_pAudioPlayer = new AudioPlayer(this);
	connect(m_pDecodecThread, SIGNAL(onSigDecodecOneAudio(uint8_t*, int, const QString)),
		m_pAudioPlayer, SLOT(onGetOneAudioBuffer(uint8_t*, int, const QString)));
	connect(m_pDecodecThread, SIGNAL(onSigOutAudioParams(int, int, int)), 
		m_pAudioPlayer, SLOT(onInitSDLAudio(int, int, int)));
}

MainWindow::~MainWindow()
{
	m_pDecodecThread->UpdateFormatContext("");

    delete ui;
}

void MainWindow::setCentralImage(QImage image)
{
	QImage adjustSizeImg = image.scaled(m_pCentralCanvas->size(), Qt::IgnoreAspectRatio);
	m_pCentralCanvas->setPixmap(QPixmap::fromImage(adjustSizeImg));
}

void MainWindow::initUiAndControl()
{
	QAction *start = new QAction("START", this);
	QAction *stop = new QAction("STOP", this);
	QAction *chooseSource = new QAction("ChooseSource", this);
	QAction *capture = new QAction("CAPTURE", this);

	this->menuBar()->addAction(start);
	this->menuBar()->addAction(stop);
	this->menuBar()->addAction(chooseSource);
	this->menuBar()->addAction(capture);

	connect(start, SIGNAL(triggered()), this, SLOT(onStart()));
	connect(stop, SIGNAL(triggered()), this, SLOT(onStop()));
	connect(chooseSource, SIGNAL(triggered()), m_pSourceChooseDlg, SLOT(show()));

	connect(capture, &QAction::triggered, this, [&]() {
		ScreenCaptureDlg dlg;
		dlg.exec();
	});
	connect(m_pSourceChooseDlg, SIGNAL(onSigDownload()), this, SLOT(onUpdateMediaSource()));
}

void MainWindow::addImage(QImage image)
{
	QImage pImage;
	pImage = image.copy();
	if (!pImage.isNull())
	{
		m_pImagesQueue.enqueue(pImage);
	}
}

void MainWindow::getOutImage(QImage & tempImage)
{
	if (m_pImagesQueue.count())
	{
		QImage pImage = m_pImagesQueue.dequeue();
		if (!pImage.isNull())
		{
			tempImage = pImage.copy();
		}
	}
}

void MainWindow::operateImageQueue(int saveOrGetOut, QImage & image)
{
	QMutexLocker locker(&m_pMutext);
	switch (saveOrGetOut)
	{
	case 1:
		addImage(image);
		break;
	case 2:
		getOutImage(image);
		break;
	case 3:
		if (m_pImagesQueue.count())
		{
			m_pImagesQueue.clear();
		}
		break;
	default:
		break;
	}
}

void MainWindow::onShowFlag()
{
	QImage image;
	operateImageQueue(2, image);
	if (!image.isNull())
	{
		setCentralImage(image);
	}
}

void MainWindow::onGotOneDecodecPic(QImage srcImage, const QString strID)
{
	if (m_pImagesQueue.count() > 4000)
	{
		return;
	}
	operateImageQueue(1, srcImage);
}

void MainWindow::onStart()
{
	m_pTimer.start();
}

void MainWindow::onStop()
{
	m_pTimer.stop();
}

void MainWindow::onUpdateMediaSource()
{
	m_pTimer.stop();

	QString newSource = m_pSourceChooseDlg->getFilePath();
	QByteArray ba = newSource.toLatin1();

	QImage image;
	operateImageQueue(3, image);
	m_pDecodecThread->UpdateFormatContext(ba.data());

	m_pTimer.start();
	m_pSourceChooseDlg->setVisible(false);
}

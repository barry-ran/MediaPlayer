#include "VideoDownloadDlg.h"
#include <QCoreApplication>
#include <QFileDialog>

VideoDownloadDlg::VideoDownloadDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	m_pBeginTime = QDateTime::currentDateTime();
	m_pEndTime = m_pBeginTime;

	setBeginTime(m_pBeginTime);
	setEndTime(m_pEndTime);

	connect(ui.button_ok, SIGNAL(clicked()), this, SIGNAL(onSigDownload()));
	connect(ui.button_cancel, SIGNAL(clicked()), this, SLOT(hide()));
	connect(ui.button_scan, SIGNAL(clicked()), this, SLOT(onFileDialog()));

	this->setWindowTitle(QString::fromLocal8Bit("选择视频来源"));
}

VideoDownloadDlg::~VideoDownloadDlg()
{

}

void VideoDownloadDlg::setBeginTime(const QDateTime & dateTime)
{
	ui.timeEdit_begin->setDateTime(dateTime);
}

const QDateTime & VideoDownloadDlg::getBeginTime()
{
	m_pBeginTime = ui.timeEdit_begin->dateTime();
	return m_pBeginTime;
}

void VideoDownloadDlg::setEndTime(const QDateTime & dateTime)
{
	ui.timeEdit_end->setDateTime(dateTime);
}

const QDateTime & VideoDownloadDlg::getEndTime()
{
	m_pEndTime = ui.timeEdit_end->dateTime();
	return m_pEndTime;
}

void VideoDownloadDlg::setFilePath(const QString & path)
{
	ui.lineEdit_path->setText(path);
}

const QString & VideoDownloadDlg::getFilePath()
{
	m_pCurrFilePath = ui.lineEdit_path->text();
	return m_pCurrFilePath;
}

void VideoDownloadDlg::onFileDialog()
{
	m_pCurrFilePath = getFilePath();
	if (m_pCurrFilePath.isEmpty())
	{
		m_pCurrFilePath = QApplication::applicationFilePath();
	}

	QString filePath = QFileDialog::getSaveFileName(this, tr("选择路径"), m_pCurrFilePath, 0, 0);
	if (!filePath.isEmpty())
	{
		setFilePath(filePath);
	}
}

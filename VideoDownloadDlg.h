#pragma once

#include <QDialog>
#include <QDateTime>
#include "ui_VideoDownloadDlg.h"

class VideoDownloadDlg : public QDialog
{
	Q_OBJECT

public:
	VideoDownloadDlg(QWidget *parent = Q_NULLPTR);
	~VideoDownloadDlg();

public:
	void setBeginTime(const QDateTime &dateTime);
	const QDateTime& getBeginTime();

	void setEndTime(const QDateTime &dateTime);
	const QDateTime& getEndTime();

	void setFilePath(const QString &path);
	const QString& getFilePath();

public slots:
    void onFileDialog();
	
signals:
	void onSigDownload();

private:
	Ui::VideoDownloadDlg ui;
	QString m_pCurrFilePath;
	QDateTime m_pBeginTime;
	QDateTime m_pEndTime;
};

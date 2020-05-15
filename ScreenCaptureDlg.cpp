﻿#include "ScreenCaptureDlg.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QScreen>
#include <QPixmap>

ScreenCaptureDlg::ScreenCaptureDlg(QWidget *parent)
    : QDialog(parent)
    , m_pStatus(0)
    , m_pIsPressed(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
	this->setModal(true);

	InitUI();
    InitConnect(); 
}

ScreenCaptureDlg::~ScreenCaptureDlg()
{

}

void ScreenCaptureDlg::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	int x, y; 
	int alpha = 1;
	int space = 4;
	QPoint topRectP, bottomRectP, leftRectP, rightRectP;
	int w = qAbs(m_pStartPoint.x() - m_pEndPoint.x());
	int h = qAbs(m_pStartPoint.y() - m_pEndPoint.y());
	if (m_pStartPoint.x() <= m_pEndPoint.x() && m_pStartPoint.y() <= m_pEndPoint.y())
	{
		x = m_pStartPoint.x();
		y = m_pStartPoint.y();

		topRectP.rx() = m_pStartPoint.x() + w / 2 - space / 2;
		topRectP.ry() = m_pStartPoint.y() - space / 2;
		bottomRectP.rx() = topRectP.rx();
		bottomRectP.ry() = m_pEndPoint.y() - space / 2;
		leftRectP.rx() = m_pStartPoint.x() - space / 2;
		leftRectP.ry() = m_pStartPoint.y() + h / 2 - space / 2;
		rightRectP.rx() = m_pEndPoint.x() - space / 2;
		rightRectP.ry() = leftRectP.ry();
	}
	else if (m_pStartPoint.x() < m_pEndPoint.x() && m_pStartPoint.y() > m_pEndPoint.y())
	{
		x = m_pStartPoint.x();
		y = m_pEndPoint.y();

		topRectP.rx() = m_pEndPoint.x() - w / 2 - space / 2;
		topRectP.ry() = m_pEndPoint.y() - space / 2;
		bottomRectP.rx() = topRectP.rx();
		bottomRectP.ry() = m_pStartPoint.y() - space / 2;
		leftRectP.rx() = m_pStartPoint.x() - space / 2;
		leftRectP.ry() = m_pStartPoint.y() - h / 2 - space / 2;
		rightRectP.rx() = m_pEndPoint.x() - space / 2;
		rightRectP.ry() = leftRectP.ry();
	}
	else if (m_pStartPoint.x() > m_pEndPoint.x() && m_pStartPoint.y() <= m_pEndPoint.y())
	{
		x = m_pEndPoint.x();
		y = m_pStartPoint.y();

		topRectP.rx() = m_pStartPoint.x() - w / 2 - space / 2;
		topRectP.ry() = m_pStartPoint.y() - space / 2;
		bottomRectP.rx() = topRectP.rx();
		bottomRectP.ry() = m_pEndPoint.y() - space / 2;
		leftRectP.rx() = m_pEndPoint.x() - space / 2;
		leftRectP.ry() = m_pEndPoint.y() - h / 2 - space / 2;
		rightRectP.rx() = m_pStartPoint.x() - space / 2;
		rightRectP.ry() = leftRectP.ry();
	}
	else
	{
		x = m_pEndPoint.x();
		y = m_pEndPoint.y();

		topRectP.rx() = m_pEndPoint.x() + w / 2 - space / 2;
		topRectP.ry() = m_pEndPoint.y() - space / 2;
		bottomRectP.rx() = topRectP.rx();
		bottomRectP.ry() = m_pStartPoint.y() - space / 2;
		leftRectP.rx() = m_pEndPoint.x() - space / 2;
		leftRectP.ry() = m_pEndPoint.y() + h / 2 - space / 2;
		rightRectP.rx() = m_pStartPoint.x() - space / 2;
		rightRectP.ry() = leftRectP.ry();
	}


    QPainter painter(this);
	QPen pen;
	pen.setColor(QColor("#FF8C05"));
	pen.setWidth(4);

	if (m_pIsPressed)
	{
		pen.setWidth(1);
		//pen.setStyle(Qt::DotLine);
	}

    painter.setPen(pen);
    painter.drawPixmap(0, 0, *bgScreen);

    if (w != 0 && h != 0) {
         painter.drawPixmap(x, y, m_pFullScreen.copy(x, y, w, h));
    }
    painter.drawRect(x, y, w, h);

	if (m_pIsPressed)
	{
		painter.setBrush(QBrush(QColor("#FF8C05")));
		painter.drawRect(m_pStartPoint.x() - space / 2, m_pStartPoint.y() - space / 2, space, space);
		painter.drawRect(m_pEndPoint.x() - space / 2, m_pEndPoint.y() - space / 2, space, space);
		painter.drawRect(m_pStartPoint.x() - space / 2, m_pEndPoint.y() - space / 2, space, space);
		painter.drawRect(m_pEndPoint.x() - space / 2, m_pStartPoint.y() - space / 2, space, space);

		painter.drawRect(topRectP.rx(), topRectP.ry(), space, space);
		painter.drawRect(bottomRectP.rx(), bottomRectP.ry(), space, space);
		painter.drawRect(leftRectP.rx(), leftRectP.ry(), space, space);
		painter.drawRect(rightRectP.rx(), rightRectP.ry(), space, space);
	}

	int tipsX = 0;
	int tipsY = 0;
	if (x == 0 && y == 0)
	{
		tipsX = x + 2;
		tipsY = y + 2;
	}
	else
	{
		tipsX = x;
		tipsY = y - 25 - 2;
	}

	painter.setPen(QPen(QColor("#262626")));
	painter.setBrush(QBrush(QColor("#262626")));
	painter.drawRect(tipsX, tipsY, 150, 25);

	QFont font;
	font.setFamily(QString("Microsoft YaHei"));
	font.setPixelSize(12);
	painter.setFont(font);
	painter.setPen(QPen("#FF8C05"));
	painter.drawText(QRect(tipsX + 15, tipsY, 216, 25), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("请选取需要直播的区域"));

	if (m_pIsPressed)
	{
		painter.setPen(QPen(QColor("#262626")));
		painter.setBrush(QBrush(QColor("#262626")));
		painter.drawRect(m_pEndPoint.x(), m_pEndPoint.y() + 12, 72, 18);

		painter.setFont(font);
		painter.setPen(QPen("#FF8C05"));
		painter.drawText(QRect(m_pEndPoint.x(), m_pEndPoint.y() + 12, 72, 18), Qt::AlignCenter, QStringLiteral("%1 x %2").arg(w).arg(h));
	}
}

void ScreenCaptureDlg::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pIsPressed = true;
		m_pStatus = 1;
        m_pStartPoint = event->pos();
		m_pEndPoint = m_pStartPoint;
         qDebug()<<"Pressed pos: "<<event->pos();
         update();
    }
}

void ScreenCaptureDlg::mouseReleaseEvent(QMouseEvent *event)
{
	m_pIsPressed = false;
	m_pStatus = 2;
	qDebug() << "Release pos: " << event->pos();
	update();
	this->accept();
}

void ScreenCaptureDlg::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pIsPressed)
    {
		m_pEndPoint = event->pos();
        update();
    }
}

void ScreenCaptureDlg::showEvent(QShowEvent *event)
{
     QScreen *screen = QApplication::primaryScreen();
     m_pFullScreen = screen->grabWindow(QApplication::desktop()->winId(), 0, 0, width(), height());

     QPixmap pix(width(), height());
     pix.fill(QColor(0, 0, 0, 150));
     bgScreen = new QPixmap(m_pFullScreen);
     QPainter p(bgScreen);
     p.drawPixmap(0, 0, pix);

}

void ScreenCaptureDlg::InitUI()
{
    QWidget *desktop = QApplication::desktop()->screen();
    resize(desktop->width(), desktop->height());

	m_pStartPoint = QPoint(desktop->x(), desktop->y());
	m_pEndPoint = QPoint(desktop->x() + desktop->width(), desktop->y() + desktop->height());

	setCursor(QCursor(Qt::CrossCursor));

    this->setMouseTracking(true);
}

void ScreenCaptureDlg::InitConnect()
{

}

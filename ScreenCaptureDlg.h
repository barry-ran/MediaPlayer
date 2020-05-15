#ifndef SCREENCAPTUREDLG_H
#define SCREENCAPTUREDLG_H

#include <QDialog>

class ScreenCaptureDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ScreenCaptureDlg(QWidget *parent = 0);
    virtual ~ScreenCaptureDlg(void);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void showEvent(QShowEvent *event);

private:
    void InitUI();
    void InitConnect();

private:
    int m_pStatus;
    QPoint m_pStartPoint;
    QPoint m_pEndPoint;
    bool m_pIsPressed;
    QPixmap m_pFullScreen;
    QPixmap *bgScreen;
};

#endif // SCREENCAPTUREDLG_H

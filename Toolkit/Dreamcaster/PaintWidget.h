#ifndef PAINT_WIDGET_H
#define PAINT_WIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

//#include "dreamcaster.h"
/**	\class PaintWidget.
	\brief Class, inherited from QWidget. It is possible to draw on this widget outside paintEvent event.

	Class have QPixmap pointer.	Drawing on widget is implemented by drawing on this pixmap.
	
	paintEvent is redefined to draw data contained in pxmp on widget.
*/
//extern DreamCaster * dcast;

class PaintWidget : public QWidget
{
	Q_OBJECT

public:
	PaintWidget(QPixmap *a_pxmp, QWidget *parent);
	~PaintWidget();
	QPixmap * GetPixmap(){ return m_pxmp; }
	void SetPixmap(QPixmap *pxmp){ m_pxmp = pxmp; }
	void SetHiglightedIndices(int *inds_x, int *inds_y, unsigned int count);
	void SetHighlightStyle(const QColor &color, float penWidth);
	void RemoveHighlights();
	//virtual int heightForWidth ( int w ) const
	//{
	//	return ratio*w;
	//}

protected:
	void paintEvent(QPaintEvent *event);
	void mouseReleaseEvent ( QMouseEvent * event );
	void mousePressEvent ( QMouseEvent * event );
	void mouseMoveEvent ( QMouseEvent * event );
public:
	int lastX, lastY;
	int lastMoveX, lastMoveY;
	//int lastMoveX, lastMoveY;
	double scaleCoef;
	//int ratio;
signals:
	void mouseReleaseEventSignal();
	void mouseReleaseEventSignal(int x, int y);
	void mouseMoveEventSignal();
	void mousePressEventSignal();
	void ChangedSignal(double & scale, double & offsetX, double & offsetY);
public slots:
	virtual void UpdateSlot(double & scale, double & offsetX, double & offsetY);

private:
	void checkOffset();
	void checkScale();
	QPixmap *m_pxmp;
	double m_scale;
	double m_offset[2];
	float m_maxScale;
	int *m_highlightX;
	int *m_highlightY;
	int highlightCount;
	float highlightPenWidth;
	QColor highlightColor;
	int m_lastMoveX, m_lastMoveY;
};


#endif//PAINT_WIDGET_H

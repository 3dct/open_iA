#ifndef STABILITY_WIDGET_H
#define STABILITY_WIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

/**	\class StabilityWidget.
	\brief Class, inherited from QWidget. 
	Represents stability by 3 axes.
*/
class StabilityWidget : public QWidget
{
	Q_OBJECT

public:
	StabilityWidget(QWidget *parent);
	~StabilityWidget();
protected:
	void paintEvent(QPaintEvent *event);
	void mouseReleaseEvent ( QMouseEvent * event );
public:
	unsigned int lastX, lastY;
	unsigned int countX(){return m_countX;}
	unsigned int countY(){return m_countY;}
	unsigned int countZ(){return m_countZ;}
	QColor **colsXY,*colsZ, colArrowX, colArrowY, colArrowZ;
	void SetCount(int count);
private:
	QPainter painter;
	unsigned int m_countX, m_countY, m_countZ;
	float m_pix_size;
	float stepPixSize;
	float spanAngleZ;
	QWidget * m_parent;
signals:
	void mouseReleaseEventSignal();
};
//STABILITY_WIDGET_H

#endif

/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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

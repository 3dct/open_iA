/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include <QWidget>

class MouseEvent;

//! Class, inherited from QWidget, representing stability by 3 axes.
class iAStabilityWidget : public QWidget
{
	Q_OBJECT

public:
	iAStabilityWidget(QWidget *parent);
	~iAStabilityWidget();
protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseReleaseEvent ( QMouseEvent * event ) override;
public:
	unsigned int countX(){return m_countX;}
	unsigned int countY(){return m_countY;}
	unsigned int countZ(){return m_countZ;}
	void SetCount(int count);

	unsigned int m_lastX, m_lastY;
	QColor **m_colsXY, *m_colsZ, m_colArrowX, m_colArrowY, m_colArrowZ;
private:
	unsigned int m_countX, m_countY, m_countZ;
	float m_pix_size;
	float m_stepPixSize;
	float m_spanAngleZ;
	QWidget * m_parent;
signals:
	void mouseReleaseEventSignal();
};

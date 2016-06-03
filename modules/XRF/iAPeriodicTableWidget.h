/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAPERIODICTABLEWIDGET_H
#define IAPERIODICTABLEWIDGET_H

#include "iAElementSelectionListener.h"

#include <QMap>
#include <QWidget>

class QColor;
class QPainter;
class QPaintEvent;

#include <utility> // for std::pair

class iAPeriodicTableWidget: public QWidget
{
	Q_OBJECT
public:
	iAPeriodicTableWidget(QWidget *parent);
	void setConcentration(QString const & elementName, double percentage, QColor const & color);
	void setListener(QSharedPointer<iAElementSelectionListener> listener);
	int GetCurrentElement() const;
protected:
	void paintEvent(QPaintEvent *);
	void mouseMoveEvent(QMouseEvent *);
private:
	void drawElement(QPainter& painter, QPoint const & elemPos, int elemWidth, int elemHeight, int elemIdx);
	QColor getPenColor(QPainter const & painter);
	int getElementFromMousePos(QPoint pos);

	QMap<QString, std::pair<double, QColor> > m_concentration;
	QVector<std::pair<QPoint, int> > m_elementPositions;
	int m_elementWidth, m_elementHeight;
	QSharedPointer<iAElementSelectionListener> m_listener;
	int m_currentElemIdx;
};

#endif /* IAPERIODICTABLEWIDGET_H */

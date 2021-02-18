/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <QVector>

class iAChartWithFunctionsWidget;
class iASimpleSlicerWidget;

class QLabel;
class QGridLayout;
class QResizeEvent;

class iAHistogramStackGrid : public QWidget
{
public:
	iAHistogramStackGrid(
		QWidget *parent,
		QVector<iAChartWithFunctionsWidget*> const & histograms,
		QVector<iASimpleSlicerWidget*> const & slicers,
		QVector<QLabel*> const & labels,
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
		Qt::WindowFlags f = 0);
#else
		Qt::WindowFlags f = Qt::WindowFlags());
#endif

	void adjustStretch() { adjustStretch(size().width()); }
protected:
	void resizeEvent(QResizeEvent* event);
private:
	void adjustStretch(int w);
	QGridLayout *m_gridLayout;
	int m_spacing = 1;
};
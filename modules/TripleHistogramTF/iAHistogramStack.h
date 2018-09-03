/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include <QWidget>

#include "iATripleModalityWidget.h"
//#include "mdichild.h"
class MdiChild;
class iADiagramFctWidget;
class iASimpleSlicerWidget;

class iAHistogramStackGrid : public QWidget
{
public:
	iAHistogramStackGrid(QWidget *parent, iADiagramFctWidget *histograms[3], iASimpleSlicerWidget *slicers[3], QLabel *labels[3], Qt::WindowFlags f = 0);
	void adjustStretch() { adjustStretch(size().width()); }
protected:
	void resizeEvent(QResizeEvent* event);
private:
	void adjustStretch(int w);
	QGridLayout *m_gridLayout;
};

class iAHistogramStack : public iATripleModalityWidget
{
public:
	iAHistogramStack(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);

	// OVERRIDES
	void initialize() override;
	void setModalityLabel(QString label, int index) override;

private:
	QSplitter *m_splitter;
	iAHistogramStackGrid *m_grid;
	QLabel *m_modalityLabels[3] = { nullptr, nullptr, nullptr };

};
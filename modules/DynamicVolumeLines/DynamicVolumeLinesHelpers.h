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

#include <charts/qcustomplot.h>
#include "iAColorTheme.h"

#include <itkImageBase.h>
#include <itkImage.h>
#include <itkImageIOBase.h>

#include <math.h>

typedef itk::ImageBase< DIM > ImageBaseType;
typedef ImageBaseType::Pointer ImagePointer;
typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot< unsigned int, double> FunctionalBoxPlot;

struct icData
{
	icData(double i, itk::Index<DIM> coord ) : 
		intensity(i), x(coord[0]), y(coord[1]), z(coord[2]) {}
	
	double intensity;
	unsigned int x;
	unsigned int y;
	unsigned int z;
};

enum PathID
{
	P_HILBERT,
	P_SCAN_LINE
};

const QStringList pathNames = QStringList()\
<< "Hilbert"\
<< "Scan Line";

typedef QMap<QString, PathID> MapPathNames2PathID;
static MapPathNames2PathID fill_PathNameToId()
{
	MapPathNames2PathID m;
	m[pathNames.at(0)] = P_HILBERT;
	m[pathNames.at(1)] = P_SCAN_LINE;

	return m;
}
const MapPathNames2PathID PathNameToId = fill_PathNameToId();

const double golden_ratio = 0.618033988749895;

inline void updateLegendAndGraphVisibility(QCPPlottableLegendItem *ptliU, QCustomPlot *plotP,
	int legendPItemIdx,  float alpha, bool visibility)
{
	QCPGraph *g = qobject_cast<QCPGraph*>(ptliU->plottable());
	QColor c = ptliU->textColor();
	c.setAlphaF(alpha);
	ptliU->setTextColor(c);
	plotP->legend->item(legendPItemIdx)->setTextColor(c);
	g->setVisible(visibility);
	plotP->graph(legendPItemIdx)->setVisible(visibility);
}

inline QPen getDatasetPen(int datasetIdx, int datasetCnt, int penWidth, QString themeName)
{
	auto theme = iAColorThemeManager::GetInstance().GetTheme(themeName);
	QPen datasetPen; datasetPen.setWidth(penWidth);
	QColor datasetColor;
	if (datasetCnt <= theme->size())
	{
		datasetColor = theme->GetColor(datasetIdx);
	}
	else
	{
		// https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
		float h = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		h += golden_ratio;
		h = fmodf(h, 1.0);
		datasetColor = QColor::fromHsvF(h, 0.95, 0.95, 1.0);
	}
	datasetPen.setColor(datasetColor);
	return datasetPen;
}

inline void showGraphandAddToLegend(QCustomPlot *nonlinearPlot, QCustomPlot *linearPlot, int graphIdx)
{
	nonlinearPlot->graph(graphIdx)->setVisible(true);
	nonlinearPlot->graph(graphIdx)->addToLegend();
	linearPlot->graph(graphIdx)->setVisible(true);
	linearPlot->graph(graphIdx)->addToLegend();
}

inline void hideGraphandRemoveFromLegend(QCustomPlot *nonlinearPlot, QCustomPlot *linearPlot, int graphIdx)
{
	nonlinearPlot->graph(graphIdx)->setVisible(false);
	nonlinearPlot->graph(graphIdx)->removeFromLegend();
	linearPlot->graph(graphIdx)->setVisible(false);
	linearPlot->graph(graphIdx)->removeFromLegend();
}

inline void switchFBPMode(QString FBPMode, QCustomPlot *nonlinearPlot, QCustomPlot *linearPlot, 
	int datasetsCnt, QSlider *sl_FBPTransparency)
{
	if (FBPMode == "only")
	{
		for (int i = 0; i < nonlinearPlot->graphCount(); ++i)
		{
			sl_FBPTransparency->hide();
			if (i >= datasetsCnt)
			{
				nonlinearPlot->graph(i)->setVisible(true);
				linearPlot->graph(i)->setVisible(true);
				if (nonlinearPlot->graph(i)->name() != "Third Quartile")
				{
					nonlinearPlot->graph(i)->addToLegend();
					linearPlot->graph(i)->addToLegend();
				}
			}
			else
			{
				hideGraphandRemoveFromLegend(nonlinearPlot, linearPlot, i);
			}
		}
	}
	else
	{
		sl_FBPTransparency->show();
		for (int i = 0; i < nonlinearPlot->graphCount(); ++i)
		{
			nonlinearPlot->graph(i)->removeFromLegend();
			linearPlot->graph(i)->removeFromLegend();
			if (i < datasetsCnt)
			{
				showGraphandAddToLegend(nonlinearPlot, linearPlot, i);
			}
			else
			{
				nonlinearPlot->graph(i)->setLayer("background");
				nonlinearPlot->graph(i)->setVisible(true);
				linearPlot->graph(i)->setLayer("background");
				linearPlot->graph(i)->setVisible(true);
				if (nonlinearPlot->graph(i)->name() != "Third Quartile")
				{
					nonlinearPlot->graph(i)->addToLegend();
					linearPlot->graph(i)->addToLegend();
				}
			}
		}
	}
}

inline void switchLevelOfDetail(bool histVisMode, QCheckBox *cb_showFBP, QComboBox *cb_FBPView,
	QSlider *sl_FBPTransparency, QCustomPlot *nonlinearPlot, QCustomPlot *linearPlot, iAScalingWidget *scalingWidget)
{
	cb_showFBP->setEnabled(!histVisMode);
	cb_FBPView->setEnabled(!histVisMode);
	sl_FBPTransparency->setEnabled(!histVisMode);
	nonlinearPlot->legend->setVisible(!histVisMode);
	linearPlot->legend->setVisible(!histVisMode);
	scalingWidget->setHistVisMode(histVisMode);
	for (int i = 0; i < nonlinearPlot->itemCount(); ++i)
	{
		if (nonlinearPlot->item(i)->objectName() == "histRect")
		{
			nonlinearPlot->item(i)->setVisible(histVisMode);
			linearPlot->item(i)->setVisible(histVisMode);
		}
	}
}

inline void setPlotVisibility(QToolButton *tb, QCustomPlot *qcp)
{
	qcp->isVisible() ? 
		tb->setIcon(QIcon(":/images/add.png")) :
		tb->setIcon(QIcon(":/images/minus.png"));
	qcp->setVisible(!qcp->isVisible());
	qcp->update();
}
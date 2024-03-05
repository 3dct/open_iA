// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAScalingWidget.h"

#include <qcustomplot.h>
#include <defines.h>   // for DIM
#include <iAColorTheme.h>
#include <iAThemeHelper.h>

#include <itkImageBase.h>
#include <itkImage.h>

#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QToolButton>

#include <cassert>

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot< unsigned int, double> FunctionalBoxPlot;

struct icData
{
	icData(double i, itk::Index<DIM> coord ) :
		intensity(i), x(static_cast<unsigned int>(coord[0])), y(static_cast<unsigned int>(coord[1])), z(static_cast<unsigned int>(coord[2])) {}

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
	m[pathNames.at(1)] = P_HILBERT;
	m[pathNames.at(0)] = P_SCAN_LINE;

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

inline QPen getDatasetPen(qsizetype datasetIdx, qsizetype datasetCnt, int penWidth, QString themeName)
{
	auto theme = iAColorThemeManager::instance().theme(themeName);
	QPen datasetPen; datasetPen.setWidth(penWidth);
	QColor datasetColor;
	assert(datasetCnt > 0);
	if (static_cast<size_t>(datasetCnt) <= theme->size())
	{
		datasetColor = theme->color(datasetIdx);
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
	qsizetype datasetsCnt, QSlider *sl_FBPTransparency)
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
	tb->setIcon(iAThemeHelper::icon(qcp->isVisible() ? "plus":"minus"));
	qcp->setVisible(!qcp->isVisible());
	qcp->update();
}

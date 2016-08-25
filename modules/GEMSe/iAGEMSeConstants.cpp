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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iAGEMSeConstants.h"

#include <QBrush>
#include <QPen>

const QColor DefaultColors::AllDataChartColor        (215, 215, 215, 255);
const QColor DefaultColors::SubtreeHighlightColor[MaxSelectedClusters] = {
	QColor(170, 170, 170, 255),
	QColor(210, 210, 210, 255),
	QColor(180, 180, 180, 255),
	QColor(220, 220, 220, 255),
	QColor(190, 190, 190, 255),
	QColor(230, 230, 230, 255),
	QColor(200, 200, 200, 255),
	QColor(240, 240, 240, 255)
};
const QColor DefaultColors::ClusterChartColor[MaxSelectedClusters] = {
	QColor(255,  50,  50, 255),
	QColor(255, 130, 130, 255),
	QColor(255,  70,  70, 255),
	QColor(255, 150, 150, 255),
	QColor(255,  90,  90, 255),
	QColor(255, 170, 170, 255),
	QColor(255, 110, 110, 255),
	QColor(255, 190, 190, 255)
};
const QColor DefaultColors::ImageChartColor          (255, 204,  51, 255);
const QColor DefaultColors::FilteredChartColor       (185, 185, 185, 255);
const QColor DefaultColors::FilteredClusterChartColor(155,   0,   0, 255);
const QColor DefaultColors::ChartMarkerColor         (235, 184,  31, 255);
const QColor DefaultColors::ChartSliderColor         (128, 128, 128, 255);
const QColor DefaultColors::CaptionBGColor           (205, 205, 205, 255);
const QColor DefaultColors::CaptionFontColor         (  0,   0,   0, 255);
const QColor DefaultColors::DifferenceColor          (255,   0,   0, 255);

const QColor DefaultColors::BackgroundHateColor      (255, 221, 221, 255);
const QColor DefaultColors::BackgroundLikeColor      (221, 255, 221, 255);

const QString DefaultColors::BackgroundColorText("white");

const QPen DefaultColors::ClusterSelectPen[MaxSelectedClusters] = {
	QPen(DefaultColors::ClusterChartColor[0], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[1], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[2], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[3], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[4], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[5], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[6], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin),
	QPen(DefaultColors::ClusterChartColor[7], 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin)
};

const QPen DefaultColors::ImageSelectPen(DefaultColors::ImageChartColor, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin);
const QPen DefaultColors::TriangleButtonPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
const QPen DefaultColors::ChartMarkerPen(DefaultColors::ChartMarkerColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
const QPen DefaultColors::ChartSliderPen(DefaultColors::ChartSliderColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

const QBrush DefaultColors::TriangleButtonSelectedBrush(Qt::black, Qt::SolidPattern);
const QBrush DefaultColors::ChartMarkerBrush(DefaultColors::ImageChartColor, Qt::SolidPattern);
const QBrush DefaultColors::ChartSliderBrush(DefaultColors::ChartSliderColor, Qt::SolidPattern);
const QBrush DefaultColors::CaptionBrush(DefaultColors::CaptionBGColor, Qt::SolidPattern);



const QString Output::NameSeparator(": ");
const QString Output::ValueSeparator(",");
const QString Output::OptionalParamSeparator(" ");
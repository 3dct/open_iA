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
#pragma once

#include <QColor>
#include <QPen>
#include <QBrush>

// TODO: make configurable?

// COMMON

//! maximum number of preview widgets allowed at one time 
const int MaxPreviewWidgets = 24;

const int FontSize = 10;

// IMAGE PREVIEW

const double SLICER_BACKGROUND_COLOR[3] = {1.0, 1.0, 1.0};

// EXAMPLE VIEW

//! example widget will show NUM_PREVIEWS_GRID_WIDTH x NUM_PREVIEWS_GRID_HEIGHT example images
//const int NUM_PREVIEWS_GRID_WIDTH  = 3;
//const int NUM_PREVIEWS_GRID_HEIGHT = 3;

//! spacing in example view
const int ExampleViewSpacing     =   3;

//! standard width/height of preview widget (modified by aspect ratio)
const int ExamplePreviewWidth    = 150;
const int ExamplePreviewHeight   = 150;

// TREE VIEW

//! padding for the whole tree
const int TreePadding            =   5;

//! padding between clusters
const int TreeClusterPadding     =   5;

//! indent for a level in tree
const int TreeLevelIndent        =  15;
//! preview width/height in tree
const int TreePreviewSize        =  75;

const int TreeInfoRegionWidth    =  40;
//! @{
//! size of expand button 
const int TreeButtonWidth  =  15;
const int TreeButtonHeight =  20;
//! @}

//! @{cluster highlight region: padding values
const int HighlightPaddingLeft   =   2;
const int HighlightPaddingTop    =   2;
const int HighlightPaddingBottom =   2;
const int HighlightPaddingRight  =   2;

const int TreeClusterShrinkedHeight = 16;

// CHARTS

//! height & width of markers
const int MarkerTriangleHeight   =  8;
const int MarkerTriangleWidthHalf=  4;

//! default histogram count for Continuous parameters
const size_t HistogramBinCount   = 20;

//! the maximum number of concurrently selected clusters:
const int MaxSelectedClusters = 8;

//! the height of the good/bad markings
const int ChartColoringHeight = 5;

//! the spacing between charts
const int ChartSpacing = 5;

// CAMERA VIEW

//! size in pixels of slicer caption (TODO: adapt to font height?)
const int CaptionHeight = 18;

//! spacing inside the camera view
const int CameraSpacing   = ExampleViewSpacing;


// FAVORITE VIEW
const int FavoriteBarWidth = 80;
const int FavoriteWidth    = FavoriteBarWidth - (2 * ExampleViewSpacing);

//! Attributes available for charts/histograms:
typedef int AttributeID;

const int MeasureCount = 5;

enum DerivedOutput
{
	objectCount,
	duration,
	diceMetric,
	kappa,
	overallAcc,
	precision,
	recall,
	DerivedOutputCount
};


struct DefaultColors
{
	static const QColor AllDataChartColor;
	static const QColor SubtreeHighlightColor[MaxSelectedClusters];
	static const QColor ClusterChartColor[MaxSelectedClusters];
	static const QColor ImageChartColor;
	static const QColor FilteredChartColor;
	static const QColor FilteredClusterChartColor;
	static const QColor ChartMarkerColor;
	static const QColor ChartSliderColor;
	static const QColor CaptionBGColor;
	static const QColor CaptionFontColor;
	static const QColor DifferenceColor;
	static const QColor BackgroundLikeColor;
	static const QColor BackgroundHateColor;

	static const QString BackgroundColorText;

	static const QPen ClusterSelectPen[MaxSelectedClusters];
	static const QPen ImageSelectPen;
	static const QPen TriangleButtonPen;
	static const QPen ChartMarkerPen;
	static const QPen ChartSliderPen;

	static const QBrush TriangleButtonSelectedBrush;
	static const QBrush ChartMarkerBrush;
	static const QBrush ChartSliderBrush;

	static const QBrush CaptionBrush;
};

struct Output
{
	static const QString NameSeparator;
	static const QString ValueSeparator;
	static const QString OptionalParamSeparator;
};


const QString SMPFileVersion("v6");
const QString SMPFileFormatVersion("SamplingParameters File " + SMPFileVersion);

const QString AttributeSplitString("\t");
const QString ValueSplitString(" ");
const QString CategoricalValueSplitString(",");

// Attribute Types:
const QString ParameterStr("Parameter");
const QString DerivedOutputStr("Derived Output");

// Value Types:
const QString ContinuousStr("Continuous");
const QString DiscreteStr("Discrete");
const QString CategoricalStr("Categorical");

const QString LinearStr("Linear");
const QString LogarithmicStr("Logarithmic");

const QString UnknownStr("Unknown");

enum iARepresentativeType
{
	Difference,
	AverageEntropy,
	AverageLabel,
	LabelDistribution
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QBrush>
#include <QColor>
#include <QPen>

// TODO: make configurable?

// COMMON

//! maximum number of preview widgets allowed at one time
const int MaxPreviewWidgets = 24;

const int FontSize = 10;

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
	static const QColor SlicerBackgroundColor;

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

enum iARepresentativeType
{
	Difference,
	AverageEntropy,
	AverageLabel,
	LabelDistribution,
	Correctness,
};

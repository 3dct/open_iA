/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

//! Settings for vtkRenderer, and helpers defined in iARenderer.
class iARenderSettings
{
public:
	bool
		ShowSlicers,			//! TODO: VOLUME: move to iAVolumeSettings?
		ShowSlicePlanes,        //!< whether a colored plane is shown for each currently visible slicer)
		ShowHelpers,            //!< whether axes cube and origin indicator are shown
		ShowRPosition,          //!< whether red position cube indicator is shown
		ParallelProjection,     //!< true - use parallel projection, false - use perspective projection
		UseStyleBGColor,        //!< true - use background color from style (bright/dark), false - use BackgroundTop/BackgroundBottom
		UseFXAA,                //!< whether to use FXAA anti-aliasing, if supported
		UseDepthPeeling,        //!< whether to use depth peeling (improves depth ordering in rendering of multiple objects), if false, alpha blending is used
		UseSSAO;                //!< whether to use Screen Space Ambient Occlusion (SSAO) - darkens some pixels to improve depth perception.
	QString BackgroundTop,      //!< top color used in background gradient
		BackgroundBottom;       //!< bottom color used in background gradient
	float PlaneOpacity;         //!< opacity of the slice planes enabled via ShowSlicePlanes
	int DepthPeels,             //!< number of depth peels to use (if enabled via UseDepthPeeling). The more the higher quality, but also slower rendering
		MultiSamples;           //!< number of multi
	double OcclusionRatio;      //!< In case of use of depth peeling technique for rendering translucent material, define the threshold under which the algorithm stops to iterate over peel layers (see <a href="https://vtk.org/doc/nightly/html/classvtkRenderer.html">vtkRenderer documentation</a>

	iARenderSettings() :
		ShowSlicers(false),
		ShowSlicePlanes(false),
		ShowHelpers(true),
		ShowRPosition(true),
		ParallelProjection(false),
		UseStyleBGColor(false),
		UseFXAA(true),
		UseDepthPeeling(true),
		UseSSAO(false),
		BackgroundTop("#7FAAFF"),
		BackgroundBottom("#FFFFFF"),
		PlaneOpacity(1.0),
		DepthPeels(4),
		MultiSamples(0),
		OcclusionRatio(0.0)
	{}
};

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

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QColor>

class iAChannelData;

class vtkAbstractTransform;
class vtkActor;
class vtkImageActor;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkLookupTable;
class vtkMarchingContourFilter;
class vtkPolyDataMapper;
class vtkPiecewiseFunction;
class vtkRenderer;
class vtkScalarsToColors;


class open_iA_Core_API iAChannelSlicerData
{
public:
	iAChannelSlicerData(iAChannelData const & chData, int mode);
	void update(iAChannelData const & chData);
	void setResliceAxesOrigin(double x, double y, double z);
	void resliceAxesOrigin(double * origin);
	//! Get lookup table (combined color transfer function + piecewise function for opacity)
	//vtkScalarsToColors* getLookupTable();
	//! Get color transfer function (only the colors, fully opaque)
	vtkScalarsToColors* cTF();

	void updateMapper();
	QColor getColor() const;  //!< get "color" of this channel (TODO: used only for pie charts in XRF module -> move there?)
	void updateResliceAxesDirectionCosines(int mode);
	void setTransform(vtkAbstractTransform * transform);
	void updateReslicer();
	void updateLUT();
	QString const & name() const;
	vtkImageData * input() const;  // TODO: returned vtkImageData should be const
	vtkImageData * output() const; // TODO: returned vtkImageData should be const
	vtkImageReslice * reslicer() const; // check if that can be kept private somehow
	double const * actorPosition() const;
	void setActorPosition(double x, double y, double z);
	void setActorOpacity(double opacity);
	void setMovable(bool movable);
	void setInterpolate(bool interpolate);
	void setEnabled(vtkRenderer* ren, bool enable);
	void setSlabNumberOfSlices(int slices);
	void setSlabMode(int mode);

	// TODO: contour functionality should be moved into separate class:
	// {
	void setContours(int numberOfContours, double contourMin, double contourMax);
	void setContours(int numberOfContours, double const * contourValues);
	void setShowContours(vtkRenderer* ren, bool show);
	void setContourLineParams(double lineWidth, bool dashed = false);
	void setContoursColor(double * rgb);
	void setContoursOpacity(double opacity);
	// }
private:
	//! @{ disable copy construction/assignment
	iAChannelSlicerData(iAChannelSlicerData const & other) =delete;
	iAChannelSlicerData& operator=(iAChannelSlicerData const & other) = delete;
	//! @}

	void assign(vtkSmartPointer<vtkImageData> imageData, QColor const & col);
	void setupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	vtkSmartPointer<vtkImageActor>  m_imageActor;
	vtkSmartPointer<vtkImageReslice> m_reslicer;
	vtkSmartPointer<vtkImageMapToColors> m_colormapper;
	vtkSmartPointer<vtkLookupTable> m_lut;   //! used for combining given ctf and otf into a single transfer function for both color and opacity accepted by imageMapToColors
	vtkScalarsToColors *            m_cTF;   //! the color transfer function - should be const (as soon as VTK supports it)
	vtkPiecewiseFunction *          m_oTF;   //! the opacity function - nullptr if not used - should be const (as soon as VTK supports it)
	QString                         m_name;  //! name of the channel
	QColor                          m_color; //! color of this channel (TODO: used only for pie charts in XRF module -> move there?)

	//! @{ for contours / iso lines
	void initContours();    // TODO: contour functionality should be moved into separate class
	vtkSmartPointer<vtkMarchingContourFilter> m_contourFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_contourMapper;
	vtkSmartPointer<vtkActor>       m_contourActor;
	//! @}
};


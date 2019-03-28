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

class iAChannelVisualizationData;

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
class vtkScalarsToColors;


class open_iA_Core_API iAChannelSlicerData
{
public:
	iAChannelSlicerData();
	~iAChannelSlicerData();
	void init(iAChannelVisualizationData * chData, int mode);
	void reInit(iAChannelVisualizationData * chData);
	void setResliceAxesOrigin(double x, double y, double z);
	//! Get lookup table (combined color transfer function + piecewise function for opacity)
	//vtkScalarsToColors* getLookupTable();
	//! Get color transfer function (only the colors, fully opaque)
	vtkScalarsToColors* getColorTransferFunction();

	bool isInitialized() const;
	void updateMapper();
	QColor getColor() const;  //! get "color" of this channel (TODO: used only for pie charts in XRF module -> move there?)
	void updateResliceAxesDirectionCosines(int mode);
	void setTransform(vtkAbstractTransform * transform);
	void updateReslicer();
	void updateLUT();

	QString getName() const;

	vtkImageActor*                imageActor;
	vtkSmartPointer<vtkImageData> image;
	vtkImageReslice*              reslicer;

	// TODO: contour functionality should be moved into separate class:
	// {
	vtkSmartPointer<vtkMarchingContourFilter> cFilter;
	vtkSmartPointer<vtkPolyDataMapper>        cMapper;
	vtkSmartPointer<vtkActor>                 cActor;
	void setContours(int num, const double * contourVals);
	void setContoursColor(double * rgb);
	void setContoursOpacity(double opacity);
	void setShowContours(bool show);
	void setContourLineParams(double lineWidth, bool dashed = false);
	// }
private:
	iAChannelSlicerData(iAChannelSlicerData const & other);

	void initContours();    // TODO: contour functionality should be moved into separate class

	iAChannelSlicerData& operator=(iAChannelSlicerData const & other);
	void assign(vtkSmartPointer<vtkImageData> imageData, QColor const & col);
	void setupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	vtkImageMapToColors*            m_colormapper;
	bool                            m_isInitialized;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkScalarsToColors*             m_ctf;
	vtkPiecewiseFunction*           m_otf;
	QString                         m_name;
	QColor                          m_color;  //! color of this channel (TODO: used only for pie charts in XRF module -> move there?)
};


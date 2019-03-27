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
#include "vtkScalarsToColors.h"
#include <QColor>


class iAChannelVisualizationData; 
class vtkTransform;
class vtkImageActor;
class vtkImageData;
class vtkImageReslice;
class vtkMarchingContourFilter;
class vtkPolyDataMapper;
class vtkActor; 
class vtkPiecewiseFunction;
class vtkImageMapToColors;
class vtkLookupTable;


class open_iA_Core_API iAChannelSlicerData
{
public:
	iAChannelSlicerData();
	~iAChannelSlicerData();
	void Init(iAChannelVisualizationData * chData, int mode);
	void ReInit(iAChannelVisualizationData * chData);
	void SetResliceAxesOrigin(double x, double y, double z);
	vtkScalarsToColors* GetLookupTable();

	bool isInitialized();
	void updateMapper();
	QColor GetColor() const;
	void UpdateResliceAxesDirectionCosines(int mode);
	void assignTransform(vtkTransform * transform);
	void updateReslicer();
	void UpdateLUT();

	vtkImageActor*						imageActor;
	vtkSmartPointer<vtkImageData>		image;
	vtkImageReslice*					reslicer;

	QString GetName() const;

	// TODO: contour functionality should be moved into separate class:
	// {
	vtkSmartPointer<vtkMarchingContourFilter>	cFilter;
	vtkSmartPointer<vtkPolyDataMapper>			cMapper;
	vtkSmartPointer<vtkActor>					cActor;
	void SetContours(int num, const double * contourVals);
	void SetContoursColor(double * rgb);
	void SetContoursOpacity(double opacity);
	void SetShowContours(bool show);
	void SetContourLineParams(double lineWidth, bool dashed = false);
	// }
private:
	iAChannelSlicerData(iAChannelSlicerData const & other);

	void InitContours();	// TODO: contour functionality should be moved into separate class

	iAChannelSlicerData& operator=(iAChannelSlicerData const & other);
	void Assign(vtkSmartPointer<vtkImageData> imageData, QColor const & col);
	void SetupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	vtkImageMapToColors*				colormapper;
	bool								m_isInitialized;
	QColor								color;
	vtkSmartPointer<vtkLookupTable>		m_lut;

	vtkScalarsToColors*					m_ctf;
	vtkPiecewiseFunction*				m_otf;
	QString                             m_name;
};


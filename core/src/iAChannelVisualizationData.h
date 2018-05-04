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

#include "open_iA_Core_export.h"

#include <QColor>

#include <vtkSmartPointer.h>

class QWidget;
class QString;

class vtkImageActor;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkPiecewiseFunction;
class vtkScalarsToColors;
class vtkTransform;
class vtkLookupTable;
class vtkMarchingContourFilter;
class vtkPolyDataMapper;
class vtkActor;

class iAChannelVisualizationData;

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
	void UpdateResliceAxesDirectionCosines( int mode );
	void assignTransform( vtkTransform * transform );
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
	void SetContours( int num, const double * contourVals );
	void SetContoursColor( double * rgb );
	void SetContoursOpacity( double opacity );
	void SetShowContours( bool show );
	void SetContourLineParams( double lineWidth, bool dashed = false );
	// }
private:
	iAChannelSlicerData(iAChannelSlicerData const & other);

	void InitContours();	// TODO: contour functionality should be moved into separate class

	iAChannelSlicerData& operator=(iAChannelSlicerData const & other);
	void Assign(vtkSmartPointer<vtkImageData> imageData, QColor const & col);
	void SetupOutput( vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf );

	vtkImageMapToColors*				colormapper;
	bool								m_isInitialized;
	QColor								color;
	vtkSmartPointer<vtkLookupTable>		m_lut;

	vtkScalarsToColors*					m_ctf;
	vtkPiecewiseFunction*				m_otf;
	QString                             m_name;};


class open_iA_Core_API iAChannelVisualizationData
{
public:
	static const size_t Maximum3DChannels = 3;

	iAChannelVisualizationData();
	virtual ~iAChannelVisualizationData();

	virtual void Reset();

	void SetOpacity(double opacity);
	double GetOpacity() const;

	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	bool Uses3D() const;
	void Set3D(bool enabled);

	void SetImage(vtkSmartPointer<vtkImageData> image);
	void SetColorTF(vtkScalarsToColors* cTF);
	void SetOpacityTF(vtkPiecewiseFunction* oTF);

	void SetName(QString name);
	QString GetName() const;

	// check if this can be somehow refactored (not needed for each kind of channel):
	// begin
	void SetColor(QColor const & col);
	QColor GetColor() const;

	bool IsSimilarityRenderingEnabled() const;
	void SetSimilarityRenderingEnabled(bool enabled);
	// end

	vtkSmartPointer<vtkImageData> GetImage();
	vtkPiecewiseFunction * GetOTF();
	vtkScalarsToColors* GetCTF();
private:
	bool enabled;
	double opacity;
	bool threeD;
	QColor color;
	bool similarityRenderingEnabled;
	vtkSmartPointer<vtkImageData>       image;
	vtkPiecewiseFunction*               piecewiseFunction;
	vtkScalarsToColors*                 colorTransferFunction;
	QString                             m_name;
};

void open_iA_Core_API ResetChannel(iAChannelVisualizationData* chData, vtkSmartPointer<vtkImageData> image, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

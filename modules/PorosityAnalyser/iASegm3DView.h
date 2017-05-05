/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#ifndef IA_SEGM_3D_VIEW_H
#define IA_SEGM_3D_VIEW_H


#include "ui_Segm3DView.h"
#include <iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

class iAFast3DMagicLensWidget;
class iARenderer;
class iARendererManager;
class iAVolumeRenderer;

class vtkActor;
class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkScalarBarWidget;
class vtkTransform;

typedef iAQTtoUIConnector<QDockWidget, Ui_Segm3DView>   Segm3DViewContainer;

class iASegm3DViewData
{
public:
	iASegm3DViewData( double * rangeExt, QWidget * parent );
	~iASegm3DViewData();
	void removeObserver();
	void SetDataToVisualize( vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf );
	void SetPolyData( vtkPolyData * polyData );
	void UpdateRange();
	iARenderer * GetRenderer();
	iAFast3DMagicLensWidget * GetWidget();
	void ShowVolume( bool visible );
	void ShowSurface( bool visible );
	void ShowWireframe( bool visible );
	void SetSensitivity( double sensitivity );
protected:
	void LoadAndApplySettings();
	void UpdateColorCoding();
protected:
	iARenderer * m_renderer;
	QSharedPointer<iAVolumeRenderer> m_volumeRenderer;
	bool m_rendInitialized;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	vtkRenderer * m_observedRenderer;
	unsigned long m_tag;
	iAFast3DMagicLensWidget * m_wgt;
	vtkSmartPointer<vtkScalarBarWidget> scalarBarWgt;
	vtkSmartPointer<vtkScalarBarWidget> volScalarBarWgt;
	double m_sensitivity;
	vtkSmartPointer<vtkPolyDataMapper> m_wireMapper;
	vtkSmartPointer<vtkActor> m_wireActor;
	double * m_rangeExt;
	vtkSmartPointer<vtkLookupTable> m_lut;
};

class iASegm3DView : public Segm3DViewContainer
{
	Q_OBJECT
public:
	iASegm3DView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iASegm3DView();
	void SetDataToVisualize( QList<vtkImageData*> imgData, 
		QList<vtkPolyData*> polyData,
		QList<vtkPiecewiseFunction*> otf,
		QList<vtkColorTransferFunction*> ctf,
		QStringList slicerNames );
	void SetPolyData( QList<vtkPolyData*> polyData );
	void ShowVolume( bool visible );
	void ShowSurface( bool visible );
	void SetSensitivity( double sensitivity );
	void ShowWireframe( bool visible );

protected:
	QList<iASegm3DViewData*> m_data;
	QHBoxLayout * m_layout;
	QScopedPointer<iARendererManager> m_renMgr;
	double m_range;
	QList<QWidget *> m_containerList;
};

#endif

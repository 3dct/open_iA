// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_Segm3DView.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <memory>

class iAPolyData;
class iAPolyDataRenderer;
class iAFast3DMagicLensWidget;
class iARenderer;
class iARendererImpl;
class iARendererViewSync;
class iATransferFunctionPtrs;
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
	std::shared_ptr<iAPolyData> m_polyData;
	std::shared_ptr<iAVolumeRenderer> m_volumeRenderer;
	std::shared_ptr<iAPolyDataRenderer> m_polyDataRenderer;
	std::shared_ptr<iATransferFunctionPtrs> m_tf;
	vtkSmartPointer<vtkTransform> m_axesTransform;
	vtkRenderer * m_observedRenderer;
	unsigned long m_tag;
	iAFast3DMagicLensWidget * m_wgt;
	iARendererImpl* m_renderer;
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
	iASegm3DView(QWidget* parent = nullptr);
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
	QScopedPointer<iARendererViewSync> m_renMgr;
	double m_range;
	QList<QWidget *> m_containerList;
};

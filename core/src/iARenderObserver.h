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
#pragma once

#include "open_iA_Core_export.h"

#include <QObject>
#include <QFile>

#include <string>
#include <sstream>

#include <vtkCommand.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPicker.h> 
#include <vtkTextMapper.h>
#include <vtkActor2D.h>
#include <vtkImageReslice.h>
#include <vtkPlane.h>

#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>

#include <vtkTransform.h>
#include <vtkLineSource.h>
#include <vtkProbeFilter.h>
#include <vtkCellLocator.h>
#include <vtkDoubleArray.h>

#include <vtkWorldPointPicker.h>

/**
 * \brief	observes the mouse moving
 * 
 * This class servers the iARenderer class to observe mouse movement and to extract coordinates
 * and the corresponding data "below" the mouse pointer. 
 */
class open_iA_Core_API RenderObserver : public QObject, public vtkCommand
{
	Q_OBJECT

	enum Axis { X_AXIS = 1, Y_AXIS, Z_AXIS };

public:
	RenderObserver(vtkRenderer* pRen,
						 vtkRenderer* pLabelRen,
						 vtkRenderWindowInteractor* pIren,
						 vtkPicker* pPicker,
						 vtkTransform* pTrans,
						 vtkImageData* pImageData,
						 vtkPlane* plane1,
						 vtkPlane* plane2,
						 vtkPlane* plane3,
						 vtkCellLocator * cellLocator
						 );
	~RenderObserver();

	static RenderObserver *New(vtkRenderer* pRen,
									 vtkRenderer* pLabelRen,
									 vtkRenderWindowInteractor* pIren,
									 vtkPicker* pPicker,
									 vtkTransform* pTrans,
									 vtkImageData* pImageData,
									 vtkPlane* plane1,
									 vtkPlane* plane2,
									 vtkPlane* plane3,
									 vtkCellLocator *cellLocator);
	void ReInitialize( vtkRenderer* pRen,
		vtkRenderer* pLabelRen,
		vtkRenderWindowInteractor* pIren,
		vtkPicker* pPicker,
		vtkTransform* pTrans,
		vtkImageData* pImageData,
		vtkPlane* plane1,
		vtkPlane* plane2,
		vtkPlane* plane3,
		vtkCellLocator *cellLocator );
	
	void AddListener(vtkCommand* listener);

	int GetMode();
	vtkCellLocator * GetCellLocator();
	vtkRenderWindowInteractor* GetInteractor();
	vtkImageData* GetImageData();
	vtkRenderer* GetLabelRenderer();
	vtkPicker* GetPicker();
	vtkWorldPointPicker* GetWorldPicker();
	void PickWithWorldPicker();
protected:
	vtkRenderer* m_pRen, *m_pLabelRen;
	vtkRenderWindowInteractor* m_pIren;
	vtkPicker* m_pPicker;
	vtkWorldPointPicker *m_pWorldPicker;
	vtkTransform* m_pTrans;
	vtkImageData* m_pImageData;
	
	vtkLineSource* m_pLine;
	vtkProbeFilter* m_pProbe;

	vtkPlane* m_pPlane1;
	vtkPlane* m_pPlane2;
	vtkPlane* m_pPlane3;

	vtkCellLocator * m_pcellLocator;
	
private:
	bool rotate;
	int mode, pos[3];
	double speed;
	double scale;
	std::vector<vtkCommand*> m_listener;

	virtual void Execute(vtkObject *caller, unsigned long, void*);
	void PickVolume(double point[3]);
	void SetAxis(Axis axis, double point[3]);
	void CheckPos(int dim);

Q_SIGNALS:
	void Clicked(int x, int y, int z);
};

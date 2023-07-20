// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALinePointers.h"

#include <vtkType.h>
#include <vtkSmartPointer.h>

class vtkPolyLine;
class vtkCellArray;
class vtkPolyData;
class vtkPoints;
class vtkActor;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkImageData;

//! Represents a "raw" profile function drawn over an image on a given renderer.
class iASlicerProfile
{
public:
	iASlicerProfile();
	void setVisibility ( bool isVisible );
	void point ( vtkIdType id, double pos_out[3] );
	void addToRenderer (vtkRenderer * ren);
	bool updatePosition( double posY, vtkImageData * imgData );

	static const int ZCoord = 0;

protected:
	iALineSource                       m_profileLine;
	iALinePointers                     m_zeroLine;
	//plot
	vtkSmartPointer<vtkPolyLine>       m_plotPolyLine;
	vtkSmartPointer<vtkCellArray>      m_plotCells;
	vtkSmartPointer<vtkPolyData>       m_plotPolyData;
	vtkSmartPointer<vtkPoints>         m_plotPoints;
	vtkSmartPointer<vtkActor>          m_plotActor;
	vtkSmartPointer<vtkActor>          m_plotActorHalo;
	vtkSmartPointer<vtkPolyDataMapper> m_plotMapper;
	float                              m_plotScaleFactor;
};

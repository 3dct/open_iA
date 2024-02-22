// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALinePointers.h"

#include <vtkLineSource.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

class vtkActor;
class vtkCellArray;
class vtkImageData;
class vtkPolyData;
class vtkPolyLine;
class vtkPoints;
class vtkPolyDataMapper;
class vtkRenderer;

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
	iAvtkSourcePoly<vtkLineSource>     m_profileLine;
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

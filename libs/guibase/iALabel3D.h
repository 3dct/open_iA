// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QImage>

class vtkActor;
class vtkArrowSource;
class vtkCamera;
class vtkCellArray;
class vtkFollower;
class vtkImageData;
class vtkLineSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkQuad;
class vtkRenderer;
class vtkTubeFilter;

//! Shows a text label attached to a point in 3D.
class iAguibase_API iALabel3D final
{
//methods
public:
	iALabel3D(bool showLine = true);
	~iALabel3D();

	void AttachActorsToRenderers(
		vtkRenderer * ren,
		vtkRenderer * labelRen,
		vtkCamera * cam ) const;
	void DetachActorsToRenderers(vtkRenderer * ren, vtkRenderer * labelRen);
	void SetVisible(bool isVisible);
	void SetLabeledPoint(double labeledPnt[3], double centerPnt[3]);
	void Update();	//make label up to date after Qt image is changed

	void UpdateImageData();	//copy image from Qt to VTK
	void SetupLabelQuad();  //setup VTK textured quad corresponding to Qt image

	void SetShowLine(bool showLine);
	void SetScale(double scale);
	void SetTubeRadius(double tubeRadius);
	void SetDisplacement(double displacement);

	QImage qImage;
	vtkFollower * follower;

private:
	void UpdateLabelPositioning();

	// labeled point and center of the scene
	double m_labeledPnt[3];
	double m_centerPnt[3];

	vtkPoints * m_pnts;
	vtkQuad * m_quad;
	vtkCellArray * m_quadCell;
	vtkPolyData * m_polyData;
	vtkImageData * m_imageData;
	vtkPolyDataMapper * m_mapper;
	vtkLineSource * m_lineSource;
	vtkTubeFilter *m_lineTubeFilter;
	vtkPolyDataMapper * m_lineMapper;
	vtkActor * m_lineActor;

	//appearance
	bool m_showLine;
	double m_scale;
	double m_tubeRadius;
	double m_displacement;
};

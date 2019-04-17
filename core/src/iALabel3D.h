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

#include <QImage>

class vtkTubeFilter;
class vtkArrowSource;
class vtkSphereSource;
class vtkFollower;
class iARegularPolygonSourceEx;
class vtkPoints;
class vtkRenderer;
class vtkCamera;
class vtkUnsignedCharArray;
class vtkActor;
class vtkPolyDataMapper;
class vtkQuad;
class vtkCellArray;
class vtkTexture;
class vtkPolyLine;
class vtkMath;
class vtkPolyData;
class vtkImageData;
class vtkDoubleArray;
class vtkDataArray;
class vtkLineSource;

struct open_iA_Core_API iALabel3D
{
//methods
public:
	iALabel3D(bool showLine = true);
	~iALabel3D();

	virtual void AttachActorsToRenderers(
		vtkRenderer * ren,
		vtkRenderer * labelRen,
		vtkCamera * cam ) const;
	virtual void DetachActorsToRenderers(vtkRenderer * ren, vtkRenderer * labelRen);
	void SetVisible(bool isVisible);
	void SetLabeledPoint(double labeledPnt[3], double centerPnt[3]);
	virtual void Update();	//make label up to date after Qt image is changed

	void UpdateImageData();	//copy image from Qt to VTK
	void SetupLabelQuad();  //setup VTK textured quad corresponding to Qt image

	void SetShowLine(bool showLine);
	void SetScale(double scale);
	void SetTubeRadius(double tubeRadius);
	void SetDisplacement(double displacement);
private:
	void UpdateLabelPositioning();

//properties
public:
	QImage qImage;
	vtkFollower * follower;//vtkActor * follower;
private:
	//labeled point and center of the scene
	double m_labeledPnt[3];
	double m_centerPnt[3];

	bool m_showLine;

	//vtk classes
	vtkPoints * m_pnts;
	vtkQuad * m_quad;
	vtkCellArray * m_quadCell;
	vtkPolyData * m_polyData;
	vtkImageData * m_imageData;
	vtkTexture * m_texture;
	vtkPolyDataMapper * m_mapper;
	vtkLineSource * m_lineSource;
	vtkTubeFilter *m_lineTubeFilter;
	vtkPolyDataMapper * m_lineMapper;
	vtkActor * m_lineActor;
	//appearance
	double m_scale;
	double m_tubeRadius;
	double m_displacement;
};

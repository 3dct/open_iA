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
#include "iA3DEllipseObjectVis.h"

#include "iACsvConfig.h"

#include "iAConsole.h"

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkEllipsoidSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>
#include <vtkUnsignedCharArray.h>

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include "QVTKOpenGLWidget.h"
#else
#include "QVTKWidget2.h"
#endif


iA3DEllipseObjectVis::iA3DEllipseObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, int phiRes, int thetaRes ):
	iA3DObjectVis( widget, objectTable, columnMapping ),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_colors(vtkSmartPointer<vtkUnsignedCharArray>::New())
{
	// maybe use vtkParametricFunctionSource with vtkParametricEllipsoid?
	auto fullPolySource = vtkSmartPointer<vtkAppendPolyData>::New();
	for (vtkIdType row = 0; row < objectTable->GetNumberOfRows(); ++row)
	{
		double cx = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterX)).ToDouble();
		double cy = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterY)).ToDouble();
		double cz = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterZ)).ToDouble();
		double dx = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionX)).ToDouble();
		double dy = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionY)).ToDouble();
		double dz = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionZ)).ToDouble();
		auto ellipsoidSrc = vtkSmartPointer<vtkEllipsoidSource>::New();
		ellipsoidSrc->SetThetaResolution(thetaRes);
		ellipsoidSrc->SetPhiResolution(phiRes);
		ellipsoidSrc->SetCenter(cx, cy, cz);
		ellipsoidSrc->SetXRadius(dx);
		ellipsoidSrc->SetYRadius(dy);
		ellipsoidSrc->SetZRadius(dz);
		ellipsoidSrc->Update();
		fullPolySource->AddInputData(ellipsoidSrc->GetOutput());
	}
	fullPolySource->Update();
	DEBUG_LOG(QString("Full poly source number of points: %1").arg(fullPolySource->GetOutput()->GetNumberOfPoints()));
	m_mapper->SetInputConnection(fullPolySource->GetOutputPort());
}

void iA3DEllipseObjectVis::show()
{
	auto actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(m_mapper);
	vtkRenderWindow* renWin = m_widget->GetRenderWindow();
	renWin->GetRenderers()->GetFirstRenderer()->AddActor(actor);
	renWin->GetRenderers()->GetFirstRenderer()->ResetCamera();
}

void iA3DEllipseObjectVis::renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem)
{}

void iA3DEllipseObjectVis::renderSingle(int labelID, int classID, QColor const & classColors, QStandardItem* activeClassItem)
{}

void iA3DEllipseObjectVis::multiClassRendering(QList<QColor> const & colors, QStandardItem* rootItem, double alpha)
{}

void iA3DEllipseObjectVis::renderOrientationDistribution(vtkImageData* oi)
{}

void iA3DEllipseObjectVis::renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range)
{}
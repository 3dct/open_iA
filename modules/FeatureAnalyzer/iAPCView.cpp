/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAPCView.h"

#include <iAQtVTKBindings.h>
#include <iAVtkWidget.h>

#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPlot.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>

#include <QTableWidget>

iAPCView::iAPCView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: PCViewConnector( parent, f ),
	m_view( vtkSmartPointer<vtkContextView>::New() ),
	m_chart( vtkSmartPointer<vtkChartParallelCoordinates>::New() )
{
	CREATE_OLDVTKWIDGET(m_widget);
	QHBoxLayout *layoutHB = new QHBoxLayout( this );
	layoutHB->setMargin( 0 );
	layoutHB->addWidget( m_widget );
	PCContainer->setLayout( layoutHB );

	m_view->GetScene()->AddItem( m_chart );
#if VTK_MAJOR_VERSION < 9
	m_view->SetInteractor( m_widget->GetInteractor() );
#else
	m_view->SetInteractor( m_widget->interactor() );
#endif
	m_view->GetRenderWindow()->LineSmoothingOn();
	m_view->GetRenderWindow()->PointSmoothingOn();
	m_view->GetRenderWindow()->PolygonSmoothingOn();

#if VTK_MAJOR_VERSION < 9
	m_widget->SetRenderWindow( m_view->GetRenderWindow() );
#else
	m_widget->setRenderWindow( m_view->GetRenderWindow() );
#endif
}

iAPCView::~iAPCView()
{}

void iAPCView::SetData( const QTableWidget * newData )
{
	//Init PC
	vtkSmartPointer<vtkTable> matrixInputTable = vtkSmartPointer<vtkTable>::New();
	matrixInputTable->DeepCopy( convertQTableWidgetToVTKTable(newData) );
	m_chart->GetPlot( 0 )->SetInputData( matrixInputTable );
	ChartModified();
}

void iAPCView::ChartModified()
{
	m_chart->Update();
	m_view->Render();
}

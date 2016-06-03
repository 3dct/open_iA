/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_charts.h"

#include "mdichild.h"

#include <vtkChart.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

#define VTK_CREATE(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

dlg_charts::dlg_charts(QWidget *parent, int numberOfCharts) : QDockWidget (parent)
{
	setupUi(this);
	for(int i=0; i<numberOfCharts; i++)
	{
		widgets.push_back(new QVTKWidget());
		this->horizontalLayout->addWidget(widgets.at(i));

		contextViews.push_back(vtkSmartPointer<vtkContextView>::New());
		charts.push_back(vtkSmartPointer<vtkChartXY>::New());
  
		contextViews.at(i)->SetRenderWindow(widgets.at(i)->GetRenderWindow());
		contextViews.at(i)->GetScene()->AddItem(charts.at(i));
	}
	this->activeChild = parent;

	// Create a table with some points in it...
	VTK_CREATE(vtkTable, table);
	VTK_CREATE(vtkFloatArray, arrX);
	arrX->SetName("1");
	table->AddColumn(arrX);
	VTK_CREATE(vtkFloatArray, arrC);
	arrC->SetName("2");
	table->AddColumn(arrC);
	VTK_CREATE(vtkFloatArray, arrS);
	arrS->SetName("3");
	table->AddColumn(arrS);
	VTK_CREATE(vtkFloatArray, arr1);
	arr1->SetName("4");
	table->AddColumn(arr1);
	VTK_CREATE(vtkFloatArray, arr2);
	arr2->SetName("5");
	table->AddColumn(arr2);
	VTK_CREATE(vtkFloatArray, arr3);
	arr3->SetName("6");
	table->AddColumn(arr3);
	VTK_CREATE(vtkFloatArray, arr4);
	arr4->SetName("7");
	table->AddColumn(arr4);
	VTK_CREATE(vtkFloatArray, arr5);
	arr5->SetName("8");
	table->AddColumn(arr5);
	VTK_CREATE(vtkFloatArray, arr6);
	arr6->SetName("9");
	table->AddColumn(arr6);
	VTK_CREATE(vtkFloatArray, arr7);
	arr7->SetName("10");
	table->AddColumn(arr7);

  // Make a timer object - need to get some frame rates/render times
  VTK_CREATE(vtkTimerLog, timer);

  // Test charting with a few more points...
  srand (time(NULL));
  int numPoints = 1000;
  int max = 1000;
  float delta = 0.1;
  float inc = 7.0 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
	  table->SetValue(i, 0, rand()%max);
	  table->SetValue(i, 1, (rand()%100));
	  table->SetValue(i, 2, rand()%max);
	  table->SetValue(i, 3, (rand()%max)/max);
	  table->SetValue(i, 4, rand()%max);
	  table->SetValue(i, 5, (rand()%max)/max);
	  table->SetValue(i, 6, rand()%max);
	  table->SetValue(i, 7, (rand()%max)/max);
	  table->SetValue(i, 8, rand()%max);
	  table->SetValue(i, 9, (rand()%max)/max);
  }
  int alpha = 200;
  float width = 1.0;

  if(numberOfCharts != 1)
  {
	  vtkPlot *line;
	  for(int i=0; i<numberOfCharts; i++)
	  {
		  line = charts.at(i)->AddPlot(vtkChart::POINTS);
		  line->SetColor(100, 100, 100, 100);
		  line->SetWidth(1.0);
	  }
  }
}

dlg_charts::~dlg_charts()
{
}

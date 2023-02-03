// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompVisOptions.h"
#include "iACsvDataStorage.h"

//Qt
#include <QGridLayout>

//CompVis
class iAMainWindow;
class iACsvDataStorage;
class iAComp3DWidget;

//iAobjectvis
class iAQVTKWidget;

//vtk
class vtkRenderer;
class vtkTable;


class iAComp3DView
{
public:

	/*** Initialization ***/

	iAComp3DView(iAMainWindow* parent, iACsvDataStorage* dataStorage);

	//fills gridlayout with qdockwidgets that will render the individual datasets
	void constructGridLayout(QGridLayout* layout);

	/*** Update ***/
	void update3DViews(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);
	void reset3DViews();

private:

	iACsvDataStorage* m_dataStorage;

	std::vector<iAComp3DWidget*>* m_dockWidgets;

};

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
 
#include "pch.h"
#include "dlg_eventExplorer.h"

#include "dlg_trackingGraph.h"
#include "iAConsole.h"
#include "iAFeatureTracking.h"
#include "iAVolumeStack.h"
#include "mdichild.h"
// vtk
#include <vtkAxis.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkStringArray.h>

#include <sstream>

#define VTK_CREATE(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

QString toqstr(vtkVariant const & var)
{
	ostringstream oss;
	oss << var;
	return QString(oss.str().c_str());
}

dlg_eventExplorer::dlg_eventExplorer(QWidget *parent, int numberOfCharts, int numberOfEventTypes, iAVolumeStack* volumeStack, dlg_trackingGraph* trackingGraph, std::vector<iAFeatureTracking*> trackedFeaturesForwards, std::vector<iAFeatureTracking*> trackedFeaturesBackwards) : QDockWidget(parent)
{
	setupUi(this);

	this->m_numberOfCharts = numberOfCharts;
	this->m_numberOfEventTypes = numberOfEventTypes;
	this->m_volumeStack = volumeStack;
	this->m_trackingGraph = trackingGraph;
	this->m_trackedFeaturesForwards = trackedFeaturesForwards;
	this->m_trackedFeaturesBackwards = trackedFeaturesBackwards;

	m_propertyXId = 0;
	m_propertyYId = 7;

	m_rgb[0][0] = 218; m_rgb[0][1] = 181; m_rgb[0][2] = 214;
	m_rgb[1][0] = 205; m_rgb[1][1] = 221; m_rgb[1][2] = 112;
	m_rgb[2][0] = 135; m_rgb[2][1] = 216; m_rgb[2][2] = 219;
	m_rgb[3][0] = 139; m_rgb[3][1] = 224; m_rgb[3][2] = 164;
	m_rgb[4][0] = 228; m_rgb[4][1] = 179; m_rgb[4][2] = 111;

	for (int c = 0; c < 3; c++)
	{
		for (int r = 0; r < 5; r++)
		{
			DEBUG_LOG(QString("rgb[%1][%2] = %3").arg(r).arg(c).arg(m_rgb[r][c]));
		}
	}

	this->comboBoxX->addItem(" Volume ");
	this->comboBoxX->addItem(" Dimension X ");
	this->comboBoxX->addItem(" Dimension Y ");
	this->comboBoxX->addItem(" Dimension Z ");
	this->comboBoxX->addItem(" Shape Factor ");
	this->comboBoxX->addItem(" Probability ");
	this->comboBoxX->addItem(" Uncertainty ");
	this->comboBoxX->addItem(" Volume Overlap ");
	this->comboBoxX->addItem(" Dataset Id ");
	this->comboBoxX->addItem(" Correspondence Id ");
	this->comboBoxX->addItem(" Event Type");
	this->comboBoxX->setCurrentIndex(m_propertyXId);

	this->comboBoxY->addItem(" Volume ");
	this->comboBoxY->addItem(" Dimension X ");
	this->comboBoxY->addItem(" Dimension Y ");
	this->comboBoxY->addItem(" Dimension Z ");
	this->comboBoxY->addItem(" Shape Factor ");
	this->comboBoxY->addItem(" Probability ");
	this->comboBoxY->addItem(" Uncertainty ");
	this->comboBoxY->addItem(" Volume Overlap ");
	this->comboBoxY->addItem(" Dataset Id ");
	this->comboBoxY->addItem(" Correspondence Id ");
	this->comboBoxY->addItem(" Event Type");
	this->comboBoxY->setCurrentIndex(m_propertyYId);

	connect(comboBoxX, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxXSelectionChanged(int)));
	connect(comboBoxY, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxYSelectionChanged(int)));

	creationCheckBox->setChecked(true);
	continuationCheckBox->setChecked(true);
	splitCheckBox->setChecked(true);
	mergeCheckBox->setChecked(true);
	dissipationCheckBox->setChecked(true);

	connect(creationSlider, SIGNAL (sliderMoved(int)), this, SLOT(updateOpacityCreation(int)));
	connect(continuationSlider, SIGNAL (sliderMoved(int)), this, SLOT(updateOpacityContinuation(int)));
	connect(splitSlider, SIGNAL (sliderMoved(int)), this, SLOT(updateOpacitySplit(int)));
	connect(mergeSlider, SIGNAL (sliderMoved(int)), this, SLOT(updateOpacityMerge(int)));
	connect(dissipationSlider, SIGNAL (sliderMoved(int)), this, SLOT(updateOpacityDissipation(int)));

	connect(gridOpacitySlider, SIGNAL(sliderMoved(int)), this, SLOT(updateOpacityGrid(int)));

	connect(creationCheckBox, SIGNAL (stateChanged(int)), this, SLOT(updateCheckBoxCreation(int)));
	connect(continuationCheckBox, SIGNAL (stateChanged(int)), this, SLOT(updateCheckBoxContinuation(int)));
	connect(splitCheckBox, SIGNAL (stateChanged(int)), this, SLOT(updateCheckBoxSplit(int)));
	connect(mergeCheckBox, SIGNAL (stateChanged(int)), this, SLOT(updateCheckBoxMerge(int)));
	connect(dissipationCheckBox, SIGNAL (stateChanged(int)), this, SLOT(updateCheckBoxDissipation(int)));

	connect(logXCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateCheckBoxLogX(int)));
	connect(logYCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateCheckBoxLogY(int)));

	m_chartConnections = vtkEventQtSlotConnect::New();

	for(int i=0; i<numberOfCharts; i++)
	{
		m_widgets.push_back(new QVTKWidget());
		this->horizontalLayout->addWidget(m_widgets.at(i));

		m_contextViews.push_back(vtkSmartPointer<vtkContextView>::New());
		m_charts.push_back(vtkSmartPointer<vtkChartXY>::New());
  
		m_contextViews.at(i)->SetRenderWindow(m_widgets.at(i)->GetRenderWindow());
		m_contextViews.at(i)->GetScene()->AddItem(m_charts.at(i));

		m_chartConnections->Connect(m_charts.at(i),
			vtkCommand::SelectionChangedEvent,
			this,
			SLOT(chartMouseButtonCallBack(vtkObject*)));
	}

	this->m_activeChild = parent;

	int tableId=0;
	for(int i=0; i<numberOfCharts; i++)
	{
		m_tables.push_back(vtkSmartPointer<vtkTable>::New());

		VTK_CREATE(vtkFloatArray, arrX);
		arrX->SetName("x");
		m_tables.at(tableId)->AddColumn(arrX);
		VTK_CREATE(vtkFloatArray, arrVol);
		arrVol->SetName("Creation[Volume]");
		m_tables.at(tableId)->AddColumn(arrVol);
		VTK_CREATE(vtkFloatArray, arrDimX);
		arrDimX->SetName("Creation[Dimension X]");
		m_tables.at(tableId)->AddColumn(arrDimX);
		VTK_CREATE(vtkFloatArray, arrDimY);
		arrDimY->SetName("Creation[Dimension Y]");
		m_tables.at(tableId)->AddColumn(arrDimY);
		VTK_CREATE(vtkFloatArray, arrDimZ);
		arrDimZ->SetName("Creation[Dimension Z]");
		m_tables.at(tableId)->AddColumn(arrDimZ);
		VTK_CREATE(vtkFloatArray, arrShape);
		arrShape->SetName("Creation[Shape factor]");
		m_tables.at(tableId)->AddColumn(arrShape);
		VTK_CREATE(vtkFloatArray, arrProbability);
		arrProbability->SetName("Creation[Probability]");
		m_tables.at(tableId)->AddColumn(arrProbability);
		VTK_CREATE(vtkFloatArray, arrUncertainty);
		arrUncertainty->SetName("Creation[Uncertainty]");
		m_tables.at(tableId)->AddColumn(arrUncertainty);
		VTK_CREATE(vtkFloatArray, arrOverlap);
		arrOverlap->SetName("Creation[Overlap]");
		m_tables.at(tableId)->AddColumn(arrOverlap);
		VTK_CREATE(vtkFloatArray, arrDatasetId);
		arrDatasetId->SetName("Creation[Dataset Id]");
		m_tables.at(tableId)->AddColumn(arrDatasetId);
		VTK_CREATE(vtkFloatArray, arrCorrId);
		arrCorrId->SetName("Creation[Correspondence Id]");
		m_tables.at(tableId)->AddColumn(arrCorrId);
		VTK_CREATE(vtkFloatArray, arrEvent);
		arrEvent->SetName("Creation[Event Type]");
		m_tables.at(tableId)->AddColumn(arrEvent);
		tableId++;
	}

	for(int i=0; i<numberOfCharts; i++)
	{
		m_tables.push_back(vtkSmartPointer<vtkTable>::New());

		VTK_CREATE(vtkFloatArray, arrX);
		arrX->SetName("x");
		m_tables.at(tableId)->AddColumn(arrX);
		VTK_CREATE(vtkFloatArray, arrVol);
		arrVol->SetName("Continuation[Volume]");
		m_tables.at(tableId)->AddColumn(arrVol);
		VTK_CREATE(vtkFloatArray, arrDimX);
		arrDimX->SetName("Continuation[Dimension X]");
		m_tables.at(tableId)->AddColumn(arrDimX);
		VTK_CREATE(vtkFloatArray, arrDimY);
		arrDimY->SetName("Continuation[Dimension Y]");
		m_tables.at(tableId)->AddColumn(arrDimY);
		VTK_CREATE(vtkFloatArray, arrDimZ);
		arrDimZ->SetName("Continuation[Dimension Z]");
		m_tables.at(tableId)->AddColumn(arrDimZ);
		VTK_CREATE(vtkFloatArray, arrShape);
		arrShape->SetName("Continuation[Shape factor]");
		m_tables.at(tableId)->AddColumn(arrShape);
		VTK_CREATE(vtkFloatArray, arrProbability);
		arrProbability->SetName("Continuation[Probability]");
		m_tables.at(tableId)->AddColumn(arrProbability);
		VTK_CREATE(vtkFloatArray, arrUncertainty);
		arrUncertainty->SetName("Continuation[Uncertainty]");
		m_tables.at(tableId)->AddColumn(arrUncertainty);
		VTK_CREATE(vtkFloatArray, arrOverlap);
		arrOverlap->SetName("Continuation[Overlap]");
		m_tables.at(tableId)->AddColumn(arrOverlap);
		VTK_CREATE(vtkFloatArray, arrDatasetId);
		arrDatasetId->SetName("Continuation[Dataset Id]");
		m_tables.at(tableId)->AddColumn(arrDatasetId);
		VTK_CREATE(vtkFloatArray, arrCorrId);
		arrCorrId->SetName("Continuation[Correspondence Id]");
		m_tables.at(tableId)->AddColumn(arrCorrId);
		VTK_CREATE(vtkFloatArray, arrEvent);
		arrEvent->SetName("Continuation[Event Type]");
		m_tables.at(tableId)->AddColumn(arrEvent);
		tableId++;
	}

	for(int i=0; i<numberOfCharts; i++)
	{
		m_tables.push_back(vtkSmartPointer<vtkTable>::New());

		VTK_CREATE(vtkFloatArray, arrX);
		arrX->SetName("x");
		m_tables.at(tableId)->AddColumn(arrX);
		VTK_CREATE(vtkFloatArray, arrVol);
		arrVol->SetName("Split[Volume]");
		m_tables.at(tableId)->AddColumn(arrVol);
		VTK_CREATE(vtkFloatArray, arrDimX);
		arrDimX->SetName("Split[Dimension X]");
		m_tables.at(tableId)->AddColumn(arrDimX);
		VTK_CREATE(vtkFloatArray, arrDimY);
		arrDimY->SetName("Split[Dimension Y]");
		m_tables.at(tableId)->AddColumn(arrDimY);
		VTK_CREATE(vtkFloatArray, arrDimZ);
		arrDimZ->SetName("Split[Dimension Z]");
		m_tables.at(tableId)->AddColumn(arrDimZ);
		VTK_CREATE(vtkFloatArray, arrShape);
		arrShape->SetName("Split[Shape factor]");
		m_tables.at(tableId)->AddColumn(arrShape);
		VTK_CREATE(vtkFloatArray, arrProbability);
		arrProbability->SetName("Split[Probability]");
		m_tables.at(tableId)->AddColumn(arrProbability);
		VTK_CREATE(vtkFloatArray, arrUncertainty);
		arrUncertainty->SetName("Split[Uncertainty]");
		m_tables.at(tableId)->AddColumn(arrUncertainty);
		VTK_CREATE(vtkFloatArray, arrOverlap);
		arrOverlap->SetName("Split[Overlap]");
		m_tables.at(tableId)->AddColumn(arrOverlap);
		VTK_CREATE(vtkFloatArray, arrDatasetId);
		arrDatasetId->SetName("Split[Dataset Id]");
		m_tables.at(tableId)->AddColumn(arrDatasetId);
		VTK_CREATE(vtkFloatArray, arrCorrId);
		arrCorrId->SetName("Split[Correspondence Id]");
		m_tables.at(tableId)->AddColumn(arrCorrId);
		VTK_CREATE(vtkFloatArray, arrEvent);
		arrEvent->SetName("Split[Event Type]");
		m_tables.at(tableId)->AddColumn(arrEvent);
		tableId++;
	}

	for(int i=0; i<numberOfCharts; i++)
	{
		m_tables.push_back(vtkSmartPointer<vtkTable>::New());

		VTK_CREATE(vtkFloatArray, arrX);
		arrX->SetName("x");
		m_tables.at(tableId)->AddColumn(arrX);
		VTK_CREATE(vtkFloatArray, arrVol);
		arrVol->SetName("Merge[Volume]");
		m_tables.at(tableId)->AddColumn(arrVol);
		VTK_CREATE(vtkFloatArray, arrDimX);
		arrDimX->SetName("Merge[Dimension X]");
		m_tables.at(tableId)->AddColumn(arrDimX);
		VTK_CREATE(vtkFloatArray, arrDimY);
		arrDimY->SetName("Merge[Dimension Y]");
		m_tables.at(tableId)->AddColumn(arrDimY);
		VTK_CREATE(vtkFloatArray, arrDimZ);
		arrDimZ->SetName("Merge[Dimension Z]");
		m_tables.at(tableId)->AddColumn(arrDimZ);
		VTK_CREATE(vtkFloatArray, arrShape);
		arrShape->SetName("Merge[Shape factor]");
		m_tables.at(tableId)->AddColumn(arrShape);
		VTK_CREATE(vtkFloatArray, arrProbability);
		arrProbability->SetName("Merge[Probability]");
		m_tables.at(tableId)->AddColumn(arrProbability);
		VTK_CREATE(vtkFloatArray, arrUncertainty);
		arrUncertainty->SetName("Merge[Uncertainty]");
		m_tables.at(tableId)->AddColumn(arrUncertainty);
		VTK_CREATE(vtkFloatArray, arrOverlap);
		arrOverlap->SetName("Merge[Overlap]");
		m_tables.at(tableId)->AddColumn(arrOverlap);
		VTK_CREATE(vtkFloatArray, arrDatasetId);
		arrDatasetId->SetName("Merge[Dataset Id]");
		m_tables.at(tableId)->AddColumn(arrDatasetId);
		VTK_CREATE(vtkFloatArray, arrCorrId);
		arrCorrId->SetName("Merge[Correspondence Id]");
		m_tables.at(tableId)->AddColumn(arrCorrId);
		VTK_CREATE(vtkFloatArray, arrEvent);
		arrEvent->SetName("Merge[Event Type]");
		m_tables.at(tableId)->AddColumn(arrEvent);
		tableId++;
	}

	for(int i=0; i<numberOfCharts; i++)
	{
		m_tables.push_back(vtkSmartPointer<vtkTable>::New());

		VTK_CREATE(vtkFloatArray, arrX);
		arrX->SetName("x");
		m_tables.at(tableId)->AddColumn(arrX);
		VTK_CREATE(vtkFloatArray, arrVol);
		arrVol->SetName("Dissipation[Volume]");
		m_tables.at(tableId)->AddColumn(arrVol);
		VTK_CREATE(vtkFloatArray, arrDimX);
		arrDimX->SetName("Dissipation[Dimension X]");
		m_tables.at(tableId)->AddColumn(arrDimX);
		VTK_CREATE(vtkFloatArray, arrDimY);
		arrDimY->SetName("Dissipation[Dimension Y]");
		m_tables.at(tableId)->AddColumn(arrDimY);
		VTK_CREATE(vtkFloatArray, arrDimZ);
		arrDimZ->SetName("Dissipation[Dimension Z]");
		m_tables.at(tableId)->AddColumn(arrDimZ);
		VTK_CREATE(vtkFloatArray, arrShape);
		arrShape->SetName("Dissipation[Shape factor]");
		m_tables.at(tableId)->AddColumn(arrShape);
		VTK_CREATE(vtkFloatArray, arrProbability);
		arrProbability->SetName("Dissipation[Probability]");
		m_tables.at(tableId)->AddColumn(arrProbability);
		VTK_CREATE(vtkFloatArray, arrUncertainty);
		arrUncertainty->SetName("Dissipation[Uncertainty]");
		m_tables.at(tableId)->AddColumn(arrUncertainty);
		VTK_CREATE(vtkFloatArray, arrOverlap);
		arrOverlap->SetName("Dissipation[Overlap]");
		m_tables.at(tableId)->AddColumn(arrOverlap);
		VTK_CREATE(vtkFloatArray, arrDatasetId);
		arrDatasetId->SetName("Dissipation[Dataset Id]");
		m_tables.at(tableId)->AddColumn(arrDatasetId);
		VTK_CREATE(vtkFloatArray, arrCorrId);
		arrCorrId->SetName("Dissipation[Correspondence Id]");
		m_tables.at(tableId)->AddColumn(arrCorrId);
		VTK_CREATE(vtkFloatArray, arrEvent);
		arrEvent->SetName("Dissipation[Event Type]");
		m_tables.at(tableId)->AddColumn(arrEvent);
		tableId++;
	}
	iAFeatureTracking *ftF;
	iAFeatureTracking *ftB;

	for (int t = 0; t < trackedFeaturesForwards.size(); t++)
	{
		ftF = trackedFeaturesForwards.at(t);
		ftB = trackedFeaturesBackwards.at(t);

		vtkTable *u = ftB->getU();
		vtkTable *v = ftF->getV();

		DEBUG_LOG(QString("%1:   %2 rows in u, %3 rows in v").arg(t).arg(u->GetNumberOfRows()).arg(v->GetNumberOfRows()));

		/*for (int i = 1; i <= numberOfRows; i++)
		{
			vector<iAFeatureTrackingCorrespondence> correspondences;
			if (t > 0)
				correspondences = ft->FromUtoV(i);
			else
				correspondences = ft->FromVtoU(i);
			//correspondences = ft->FromUtoV(i);

			for (iAFeatureTrackingCorrespondence c : correspondences)
			{
				if ()
			}
		}*/

		int numberOfRows;
		
		if (t > 0)
			numberOfRows = u->GetNumberOfRows();
		else
			numberOfRows = v->GetNumberOfRows();

		DEBUG_LOG(QString("%1 rows\n").arg(numberOfRows));

		for (int i = 0; i < numberOfRows; i++) //ft->getNumberOfEventsInV()
		{
			//cout << "i: " << i << "   " << v->GetValue(i, 4) << ", " << v->GetValue(i, 5) << ", " << v->GetValue(i, 6) << ", " << v->GetValue(i, 7) << ", " << endl;

			vector<iAFeatureTrackingCorrespondence> correspondences;
			if (t > 0)
				correspondences = ftB->FromUtoV(i+1);
			else
				correspondences = ftB->FromVtoU(i+1);
			//correspondences = ftF->FromUtoV(i);

			for (vector<iAFeatureTrackingCorrespondence>::iterator c = correspondences.begin(); c != correspondences.end(); c++)
			{
				DEBUG_LOG(QString("i: %1   c->id: %2, event: %3, overlap: %4, volumeRatio: %5   %6, %7, %8, %9")
					.arg(i).arg(c->id).arg(c->featureEvent).arg(c->overlap).arg(c->volumeRatio)
					.arg(toqstr(v->GetValue(i, 4))).arg(toqstr(v->GetValue(i, 5)))
					.arg(toqstr(v->GetValue(i, 6))).arg(toqstr(v->GetValue(i, 7))));

				vtkSmartPointer<vtkVariantArray> arr = vtkSmartPointer<vtkVariantArray>::New();
				arr->SetNumberOfValues(12);

				if (t > 0) //t > 0 
				{
					switch (c->featureEvent)
					{
					case 0:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i+1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 4)->InsertNextRow(arr);
						break;
					case 1:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i+1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 1)->InsertNextRow(arr);
						break;
					case 2:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i+1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 3)->InsertNextRow(arr);
						break;
					case 3:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 2)->InsertNextRow(arr);
						break;
					case 4:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 0)->InsertNextRow(arr);  //(4 + numberOfEventTypes * t)
						break;
					default:
						break;
					}
				}
				else if (c->isTakenForCurrentIteration)
				{
					switch (c->featureEvent)
					{
					case 0:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 0)->InsertNextRow(arr);
						break;
					case 1:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 1)->InsertNextRow(arr);
						break;
					case 2:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 3)->InsertNextRow(arr);
						break;
					case 3:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 2)->InsertNextRow(arr);
						break;
					case 4:
						arr->SetValue(0, i+1);
						arr->SetValue(1, v->GetValue(i, 4));
						arr->SetValue(2, v->GetValue(i, 5));
						arr->SetValue(3, v->GetValue(i, 6));
						arr->SetValue(4, v->GetValue(i, 7));
						arr->SetValue(5, v->GetValue(i, 4));
						arr->SetValue(6, c->likelyhood);
						arr->SetValue(7, 1 - c->likelyhood);
						arr->SetValue(8, c->overlap);
						arr->SetValue(9, i + 1);
						arr->SetValue(10, c->id);
						arr->SetValue(11, c->featureEvent);
						m_tables.at(t + numberOfCharts * 4)->InsertNextRow(arr);  //(4 + numberOfEventTypes * t)
						break;
					default:
						break;
					}
				}
				break; //only show the best correspondence
			}
		}
	}



	float width = 1.0;

	vtkPlot *plot;
	for(int i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 0), 1, 6);
		plot->SetColor(218,181,214, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for(int i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 1), 1, 6);
		plot->SetColor(205,221,112, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for(int i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 2), 1, 6);
		plot->SetColor(135,216,219, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for(int i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 3), 1, 6);
		plot->SetColor(139,224,164, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for(int i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 4), 1, 6);
		plot->SetColor(228,179,111, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	  
	for(int i=0; i<numberOfEventTypes; i++)
	{
		m_plotPositionInVector[i]=i;
	}

	m_numberOfActivePlots = numberOfEventTypes;
 
	for(int i=0; i<numberOfCharts; i++)
	{
		m_charts.at(i)->GetAxis(0)->SetTitle("Uncertainty");
		m_charts.at(i)->GetAxis(1)->SetTitle("Volume");
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, 1.0);
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, 1.0);
		m_charts.at(i)->Update();
	}
}


dlg_eventExplorer::~dlg_eventExplorer()
{
	//TODO
}

void dlg_eventExplorer::updateOpacityCreation(int v)
{
	for (int i = (m_numberOfCharts * 0); i<(m_numberOfCharts * 1); i++)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacityContinuation(int v)
{
	for (int i = (m_numberOfCharts * 1); i<(m_numberOfCharts * 2); i++)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacitySplit(int v)
{
	for (int i = (m_numberOfCharts * 2); i<(m_numberOfCharts * 3); i++)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacityMerge(int v)
{
	for (int i = (m_numberOfCharts * 3); i<(m_numberOfCharts * 4); i++)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacityDissipation(int v)
{
	for (int i = (m_numberOfCharts * 4); i<(m_numberOfCharts * 5); i++)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacityGrid(int v)
{
	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateCheckBoxCreation(int c)
{
	DEBUG_LOG(QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));

	if(!creationCheckBox->isChecked())
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[0]);
		}
		
		for(int i=0; i<m_numberOfEventTypes; i++)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[0])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[0] = -1;

		m_numberOfActivePlots--;

		creationSlider->setValue(0);
		updateOpacityCreation(0);
	}
	else
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 0));
			m_charts.at(i)->Update();
			
			m_plotPositionInVector[0]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		creationSlider->setValue(255);
		updateOpacityCreation(255);
	}
	creationCheckBox->update();
	creationSlider->update();
	DEBUG_LOG(QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxContinuation(int c)
{
	DEBUG_LOG(QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!continuationCheckBox->isChecked())
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[1]);
		}
		
		for(int i=0; i<m_numberOfEventTypes; i++)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[1])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[1] = -1;

		m_numberOfActivePlots--;

		continuationSlider->setValue(0);
		updateOpacityContinuation(0);
	}
	else
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 1));
			
			m_plotPositionInVector[1]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		continuationSlider->setValue(255);
		updateOpacityContinuation(255);
	}
	continuationCheckBox->update();
	continuationSlider->update();
	DEBUG_LOG(QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxSplit(int c)
{
	DEBUG_LOG(QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!splitCheckBox->isChecked())
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[2]);
		}
		
		for(int i=0; i<m_numberOfEventTypes; i++)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[2])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[2] = -1;

		m_numberOfActivePlots--;

		splitSlider->setValue(0);
		updateOpacitySplit(0);
	}
	else
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 2));
			
			m_plotPositionInVector[2]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		splitSlider->setValue(255);
		updateOpacitySplit(255);
	}
	splitCheckBox->update();
	splitSlider->update();
	DEBUG_LOG(QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxMerge(int c)
{
	DEBUG_LOG(QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!mergeCheckBox->isChecked())
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[3]);
		}
		
		for(int i=0; i<m_numberOfEventTypes; i++)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[3])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[3] = -1;

		m_numberOfActivePlots--;

		mergeSlider->setValue(0);
		updateOpacityMerge(0);
	}
	else
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 3));
			
			m_plotPositionInVector[3]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		mergeSlider->setValue(255);
		updateOpacityMerge(255);
	}
	mergeCheckBox->update();
	mergeSlider->update();
	DEBUG_LOG(QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxDissipation(int c)
{
	DEBUG_LOG(QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!dissipationCheckBox->isChecked())
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[4]);
		}

		for(int i=0; i<m_numberOfEventTypes; i++)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[4])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[4] = -1;

		m_numberOfActivePlots--;

		dissipationSlider->setValue(0);
		updateOpacityDissipation(0);
	}
	else
	{
		for(int i=0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i+m_numberOfCharts*4));
			
			m_plotPositionInVector[4]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		dissipationSlider->setValue(255);
		updateOpacityDissipation(255);
	}
	dissipationCheckBox->update();
	dissipationSlider->update();
	DEBUG_LOG(QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxLogX(int c)
{
	if (!logXCheckBox->isChecked())
	{
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->LogScaleOff();
		}
	}
	else
	{
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->LogScaleOn();
		}
	}
}

void dlg_eventExplorer::updateCheckBoxLogY(int c)
{
	if (!logYCheckBox->isChecked())
	{
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::LEFT)->LogScaleOff();
		}
	}
	else
	{
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::LEFT)->LogScaleOn();
		}
	}
}

void dlg_eventExplorer::comboBoxXSelectionChanged(int s)
{
	vtkStdString title;

	switch(s)
	{
	case 0:
		title = "Volume";
		break;
	case 1:
		title = "Dimension X";
		break;
	case 2:
		title = "Dimension Y";
		break;
	case 3:
		title = "Dimension Z";
		break;
	case 4:
		title = "Shape factor";
		break;
	case 5:
		title = "Probability";
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->SetRange(0.0, 1.0);
		}
		break;
	case 6:
		title = "Uncertainty";
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->SetRange(0.0, 1.0);
		}
		break;
	case 7:
		title = "Volume Overlap";
		for (int i = 0; i<m_numberOfCharts; i++)
		{
			m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->SetRange(0.0, 1.0);
		}
		break;
	}

	m_propertyXId = s;

	for(int i=0; i<m_numberOfCharts*m_numberOfEventTypes; i++)
	{
		m_plots.at(i)->SetInputData(m_tables.at(i), m_propertyXId+1, m_propertyYId+1);
	}

	for(int i=0; i<m_numberOfCharts; i++)
	{
		m_charts.at(i)->GetAxis(1)->SetTitle(title);
	}
}

void dlg_eventExplorer::comboBoxYSelectionChanged(int s)
{
	vtkStdString title;

	switch(s)
	{
		case 0:
			title="Volume";
			break;
		case 1:
			title="Dimension X";
			break;
		case 2:
			title="Dimension Y";
			break;
		case 3:
			title="Dimension Z";
			break;
		case 4:
			title="Shape factor";
			break;
		case 5:
			title="Probability";
			for (int i = 0; i<m_numberOfCharts; i++)
			{
				m_charts.at(i)->GetAxis(vtkAxis::LEFT)->SetRange(0.0, 1.0);
			}
			break;
		case 6:
			title = "Uncertainty";
			for (int i = 0; i<m_numberOfCharts; i++)
			{
				m_charts.at(i)->GetAxis(vtkAxis::LEFT)->SetRange(0.0, 1.0);
			}
			break;
		case 7:
			title = "Volume Overlap";
			for (int i = 0; i<m_numberOfCharts; i++)
			{
				m_charts.at(i)->GetAxis(vtkAxis::LEFT)->SetRange(0.0, 1.0);
			}
			break;
	}

	m_propertyYId = s;

	for(int i=0; i<m_numberOfCharts*m_numberOfEventTypes; i++)
	{
		m_plots.at(i)->SetInputData(m_tables.at(i), m_propertyXId+1, m_propertyYId+1);
	}

	for(int i=0; i<m_numberOfCharts; i++)
	{
		m_charts.at(i)->GetAxis(0)->SetTitle(title);
	}
}

void dlg_eventExplorer::chartMouseButtonCallBack(vtkObject * obj)
{
	//clear graph TODO
	m_graph = vtkMutableDirectedGraph::New();
	m_labels = vtkStringArray::New();
	m_labels->SetName("Label");
	m_nodeLayer = vtkIntArray::New();
	m_nodeLayer->SetName("Layer");
	m_colorR = vtkIntArray::New();
	m_colorR->SetName("ColorR");
	m_colorG = vtkIntArray::New();
	m_colorG->SetName("ColorG");
	m_colorB = vtkIntArray::New();
	m_colorB->SetName("ColorB");
	m_trackingUncertainty = vtkDoubleArray::New();
	m_trackingUncertainty->SetName("Uncertainty");
	m_nodes.clear();
	m_visitedNodes.clear();
	m_nodesToLayers.clear();
	m_graphToTableId.clear();
	m_tableToGraphId.clear();
	for (int i = 0; i < m_numberOfCharts; i++)
	{
		m_nodes.push_back(vector<int>());
	}

	DEBUG_LOG("\n\nSELECTION");

	vtkColorTransferFunction *cTF;
	vtkPiecewiseFunction *oTF;

	for (int i = 0; i < m_numberOfCharts; i++)
	{
		DEBUG_LOG(QString("\nChart[%1]").arg(i));

		cTF = m_volumeStack->getColorTransferFunction(i);
		oTF = m_volumeStack->getPiecewiseFunction(i);

		cTF->RemoveAllPoints();
		oTF->RemoveAllPoints();

		cTF->AddRGBPoint(0, 0.0, 0.0, 0.0);
		oTF->AddPoint(0, 0.0);
		cTF->AddRGBPoint(m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(), 0.0, 0.0, 0.0);
		oTF->AddPoint(m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(), 0.0);

		for (int c = 1; c < m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(); c++)
		{
			cTF = m_volumeStack->getColorTransferFunction(i);
			cTF->AddRGBPoint(c - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
			cTF->AddRGBPoint(c, 0.0, 0.0, 0.0, 0.5, 1.0);
			cTF->AddRGBPoint(c + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
			oTF = m_volumeStack->getPiecewiseFunction(i);
			oTF->AddPoint(c - 0.5, 0.0, 0.5, 1.0);
			oTF->AddPoint(c, (double)gridOpacitySlider->value() / 255.0 / 100, 0.5, 1.0);
			oTF->AddPoint(c + 0.3, 0.0, 0.5, 1.0);
		}

		int currentPlot = 0;
		for (int j = 0; j < m_numberOfEventTypes; j++)
		{
			if (m_plotPositionInVector[j] >= 0)
			{
				vtkIdTypeArray *ids = m_charts.at(i)->GetPlot(currentPlot)->GetSelection();

				if (ids != 0)
				{
					DEBUG_LOG(QString("  Plot[%1] is active. %2 selected.").arg(j).arg(ids->GetNumberOfTuples()));

					for (int k = 0; k < m_numberOfEventTypes; k++)
					{
						//if(currentPlot==plotPositionInVector[k])
						if (currentPlot == k)
						{
							switch (k)
							{
							case 0:
								DEBUG_LOG("   Creation Events");
								//plots.at(numberOfCharts * 0)->GetColor(rgb);
								break;
							case 1:
								DEBUG_LOG("   Continuation Events");
								//plots.at(numberOfCharts * 1)->GetColor(rgb);
								break;
							case 2:
								DEBUG_LOG("   Split Events");
								//plots.at(numberOfCharts * 2)->GetColor(rgb);
								break;
							case 3:
								DEBUG_LOG("   Merge Events");
								//plots.at(numberOfCharts * 3)->GetColor(rgb);
								break;
							case 4:
								DEBUG_LOG("   Dissipation Events");
								//plots.at(numberOfCharts * 4)->GetColor(rgb);
								break;
							}

							for (int l = 0; l < ids->GetNumberOfTuples(); l++)
							{
								double id = m_tables.at(i + m_numberOfCharts * k)->GetRow(ids->GetValue(l))->GetValue(0).ToDouble();
								DEBUG_LOG(QString("    %1 --> table id: %2").arg(ids->GetValue(l)).arg(id));

								buildGraph(id, i, k, m_tables.at(i + m_numberOfCharts * k)->GetRow(ids->GetValue(l))->GetValue(7).ToDouble());
							}
						}
					}
					currentPlot++;
				}
			}
			else
			{
				DEBUG_LOG(QString("  Plot[%1] is deactivated").arg(j));
			}
		}
		DEBUG_LOG(QString("   cTF range: %1, %2").arg(cTF->GetRange()[0]).arg(cTF->GetRange()[1]));
	}
	m_trackingGraph->updateGraph(m_graph, this->m_volumeStack->getNumberOfVolumes(), m_nodesToLayers, m_graphToTableId);
}

void dlg_eventExplorer::buildGraph(int id, int layer, int eventType, double uncertainty)
{
	vtkColorTransferFunction *cTF;
	vtkPiecewiseFunction *oTF;

	if (m_tableToGraphId[layer].find(id) == m_tableToGraphId[layer].end())
	{
		vtkIdType vId = m_graph->AddVertex();
		m_labels->InsertValue(vId, "[" + std::to_string((long long)id) + "]" + " (" + std::to_string((long long)uncertainty) + ")");
		m_nodeLayer->InsertValue(vId, layer);
		m_colorR->InsertValue(vId, m_rgb[eventType][0]);
		m_colorG->InsertValue(vId, m_rgb[eventType][1]);
		m_colorB->InsertValue(vId, m_rgb[eventType][2]);
		m_trackingUncertainty->InsertValue(vId, uncertainty);

		m_graphToTableId[layer][vId] = id;
		m_tableToGraphId[layer][id] = vId;
		m_nodesToLayers[vId] = layer;

		cTF = m_volumeStack->getColorTransferFunction(layer);
		cTF->AddRGBPoint(id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
		cTF->AddRGBPoint(id, (double)m_rgb[eventType][0] / 255.0, (double)m_rgb[eventType][1] / 255.0, (double)m_rgb[eventType][2] / 255.0, 0.5, 1.0);
		cTF->AddRGBPoint(id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
		oTF = m_volumeStack->getPiecewiseFunction(layer);
		oTF->AddPoint(id - 0.5, 0.0, 0.5, 1.0);
		oTF->AddPoint(id, (double)creationSlider->value() / 255.0, 0.5, 1.0);
		oTF->AddPoint(id + 0.3, 0.0, 0.5, 1.0);
	}

	buildSubGraph(id, layer);

	m_graph->GetVertexData()->AddArray(m_labels);
	m_graph->GetVertexData()->AddArray(m_nodeLayer);
	m_graph->GetVertexData()->AddArray(m_colorR);
	m_graph->GetVertexData()->AddArray(m_colorG);
	m_graph->GetVertexData()->AddArray(m_colorB);
	m_graph->GetVertexData()->AddArray(m_trackingUncertainty);
}

void dlg_eventExplorer::buildSubGraph(int id, int layer)
{
	vtkColorTransferFunction* cTF;
	vtkPiecewiseFunction* oTF;

	if (m_visitedNodes.find(id) == m_visitedNodes.end())
	{
		m_visitedNodes[id] = true;

		// search backwards
		if (layer > 0)
		{
			iAFeatureTracking *ftB = m_trackedFeaturesBackwards.at(layer);
			iAFeatureTracking *ftF = m_trackedFeaturesForwards.at(layer - 1);
			int newVertexId;

			vector<iAFeatureTrackingCorrespondence> correspondences;
			correspondences = ftB->FromUtoV(id);

			for (auto c : correspondences)
			{
				if (c.id > 0 && c.isTakenForCurrentIteration)
				{
					if (m_tableToGraphId[layer - 1].find(c.id) == m_tableToGraphId[layer - 1].end())
					{
						int featureEvent = 0;
						//if (trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).size() > 0)
						//	featureEvent = trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).at(0).featureEvent;
						if (m_trackedFeaturesForwards.at(layer - 1)->FromVtoU(c.id).size() > 0)
							featureEvent = m_trackedFeaturesForwards.at(layer - 1)->FromVtoU(c.id).at(0).featureEvent;
						else
							featureEvent = 0;

						newVertexId = m_graph->AddVertex();

						m_labels->InsertValue(newVertexId, std::to_string(c.id) + " (" + std::to_string((long long)(1 - c.likelyhood)) + ")");
						m_nodeLayer->InsertValue(newVertexId, layer - 1);
						m_colorR->InsertValue(newVertexId, m_rgb[featureEvent][0]);
						m_colorG->InsertValue(newVertexId, m_rgb[featureEvent][1]);
						m_colorB->InsertValue(newVertexId, m_rgb[featureEvent][2]);
						m_trackingUncertainty->InsertValue(newVertexId, 1 - c.likelyhood);

						m_graphToTableId[layer - 1][newVertexId] = c.id;
						m_tableToGraphId[layer - 1][c.id] = newVertexId;

						m_nodesToLayers[newVertexId] = layer - 1;

						cTF = m_volumeStack->getColorTransferFunction(layer - 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, (double)m_rgb[featureEvent][0] / 255.0, (double)m_rgb[featureEvent][1] / 255.0, (double)m_rgb[featureEvent][2] / 255.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);

						oTF = m_volumeStack->getPiecewiseFunction(layer - 1);
						oTF->AddPoint(c.id - 0.5, 0.0, 0.5, 1.0);
						oTF->AddPoint(c.id, (double)creationSlider->value() / 255.0, 0.5, 1.0);
						oTF->AddPoint(c.id + 0.3, 0.0, 0.5, 1.0);

						m_graph->AddEdge(m_tableToGraphId[layer][id], newVertexId);

						buildSubGraph(c.id, layer - 1);
						//buildSubGraph(c.id, layer + 1);
					}
					else
					{
						//TODO: only add edges which are not existing
						m_graph->AddEdge(m_tableToGraphId[layer][id], m_tableToGraphId[layer - 1][c.id]);
						DEBUG_LOG(QString("Edge [%1][%2] --> [%3][%4]").arg(id).arg(layer).arg(c.id).arg(layer - 1));
						
						/*if (g->GetEdgeId(tableToGraphId[layer][id], tableToGraphId[layer - 1][c.id]) != -1 || g->GetEdgeId(tableToGraphId[layer - 1][c.id], tableToGraphId[layer][id]) != -1)
						{
							g->AddEdge(tableToGraphId[layer][id], tableToGraphId[layer - 1][c.id]);
							cout << "Edge [" << id << "][" << layer << "] --> [" << c.id << "][" << layer - 1 << "]" << endl;
						}*/
					}
				}
				//break;
			}
		}

		// search forwards
		if (layer < m_numberOfCharts - 1)
		{
			iAFeatureTracking *ftB = m_trackedFeaturesBackwards.at(layer + 1);
			iAFeatureTracking *ftF = m_trackedFeaturesForwards.at(layer + 1);
			int newVertexId;

			vector<iAFeatureTrackingCorrespondence> correspondences;
			if (layer > 0)
				correspondences = ftF->FromVtoU(id);
			else
				correspondences = ftF->FromUtoV(id);

			for (auto c : correspondences)
			{
				if (c.id > 0 && c.isTakenForCurrentIteration)
				{
					if(m_tableToGraphId[layer + 1].find(c.id) == m_tableToGraphId[layer + 1].end())
					{
						int featureEvent = 0;
						//if (trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).size() > 0)
						//	featureEvent = trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).at(0).featureEvent;
						if (m_trackedFeaturesForwards.at(layer + 1)->FromUtoV(c.id).size() > 0)
							m_trackedFeaturesForwards.at(layer + 1)->FromUtoV(c.id).at(0).featureEvent;
						else
							featureEvent = 0;

						newVertexId = m_graph->AddVertex();
						m_labels->InsertValue(newVertexId, std::to_string(c.id) +" (" + std::to_string((long long)(1 - c.likelyhood)) + ")");
						m_nodeLayer->InsertValue(newVertexId, layer + 1);
						m_colorR->InsertValue(newVertexId, m_rgb[featureEvent][0]);
						m_colorG->InsertValue(newVertexId, m_rgb[featureEvent][1]);
						m_colorB->InsertValue(newVertexId, m_rgb[featureEvent][2]);
						m_trackingUncertainty->InsertValue(newVertexId, 1 - c.likelyhood);

						m_graphToTableId[layer + 1][newVertexId] = c.id;
						m_tableToGraphId[layer + 1][c.id] = newVertexId;

						m_nodesToLayers[newVertexId] = layer + 1;

						cTF = m_volumeStack->getColorTransferFunction(layer + 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, (double)m_rgb[featureEvent][0] / 255.0, (double)m_rgb[featureEvent][1] / 255.0, (double)m_rgb[featureEvent][2] / 255.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
						oTF = m_volumeStack->getPiecewiseFunction(layer + 1);
						oTF->AddPoint(c.id - 0.5, 0.0, 0.5, 1.0);
						oTF->AddPoint(c.id, (double)creationSlider->value() / 255.0, 0.5, 1.0);
						oTF->AddPoint(c.id + 0.3, 0.0, 0.5, 1.0);

						m_graph->AddEdge(m_tableToGraphId[layer][id], newVertexId);

						//buildSubGraph(c.id, layer - 1);
						buildSubGraph(c.id, layer + 1);
					}
					else
					{
						//TODO: only add edges which are not existing
						m_graph->AddEdge(m_tableToGraphId[layer][id], m_tableToGraphId[layer + 1][c.id]);
						DEBUG_LOG(QString("Edge [%1][%2] --> [%3][%4]").arg(id).arg(layer).arg(c.id).arg(layer + 1));
							
						/*if (g->GetEdgeId(tableToGraphId[layer][id], tableToGraphId[layer + 1][c.id]) != -1 || g->GetEdgeId(tableToGraphId[layer + 1][c.id], tableToGraphId[layer][id]) != -1)
						{
							g->AddEdge(tableToGraphId[layer][id], tableToGraphId[layer + 1][c.id]);
							cout << "Edge [" << id << "][" << layer << "] --> [" << c.id << "][" << layer + 1 << "]" << endl;
						}*/
					}
				}
				//break;
			}
		}
	}
}

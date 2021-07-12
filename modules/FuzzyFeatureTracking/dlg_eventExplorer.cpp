/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "dlg_eventExplorer.h"

#include "dlg_trackingGraph.h"
#include "iAFeatureTracking.h"

#include <iALog.h>
#include <iAVolumeStack.h>
#include <iAVtkWidget.h>
#include <iAMdiChild.h>

#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPen.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlot.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <sstream>

namespace
{
	const QStringList EventTypes = QStringList()
		<< "Creation"
		<< "Continuation"
		<< "Split"
		<< "Merge"
		<< "Dissipation";
	
	const QStringList AvailableProperties = QStringList()
		<< " Volume "
		<< " Dimension X "
		<< " Dimension Y "
		<< " Dimension Z "
		<< " Shape Factor "
		<< " Probability "
		<< " Uncertainty "
		<< " Volume Overlap "
		<< " Dataset Id "
		<< " Correspondence Id "
		<< " Event Type";
}

#define VTK_CREATE(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

QString toqstr(vtkVariant const & var)
{
	std::ostringstream oss;
	oss << var;
	return QString(oss.str().c_str());
}

dlg_eventExplorer::dlg_eventExplorer(QWidget *parent, size_t numberOfCharts, int numberOfEventTypes, iAVolumeStack* volumeStack, dlg_trackingGraph* trackingGraph, std::vector<iAFeatureTracking*> trackedFeaturesForwards, std::vector<iAFeatureTracking*> trackedFeaturesBackwards) : QDockWidget(parent)
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
			LOG(lvlInfo, QString("rgb[%1][%2] = %3").arg(r).arg(c).arg(m_rgb[r][c]));
		}
	}

	this->comboBoxX->addItems(AvailableProperties);
	this->comboBoxX->setCurrentIndex(m_propertyXId);

	this->comboBoxY->addItems(AvailableProperties);
	this->comboBoxY->setCurrentIndex(m_propertyYId);

	connect(comboBoxX, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_eventExplorer::comboBoxXSelectionChanged);
	connect(comboBoxY, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_eventExplorer::comboBoxYSelectionChanged);

	creationCheckBox->setChecked(true);
	continuationCheckBox->setChecked(true);
	splitCheckBox->setChecked(true);
	mergeCheckBox->setChecked(true);
	dissipationCheckBox->setChecked(true);

	connect(creationSlider,     &QSlider::sliderMoved, [this](int v) { updateOpacity(v, Creation); });
	connect(continuationSlider, &QSlider::sliderMoved, [this](int v) { updateOpacity(v, Continuation); });
	connect(splitSlider,        &QSlider::sliderMoved, [this](int v) { updateOpacity(v, Bifurcation); });
	connect(mergeSlider,        &QSlider::sliderMoved, [this](int v) { updateOpacity(v, Amalgamation); });
	connect(dissipationSlider,  &QSlider::sliderMoved, [this](int v) { updateOpacity(v, Dissipation); });

	connect(gridOpacitySlider, &QSlider::sliderMoved, this, &dlg_eventExplorer::updateOpacityGrid);

	connect(creationCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxCreation);
	connect(continuationCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxContinuation);
	connect(splitCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxSplit);
	connect(mergeCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxMerge);
	connect(dissipationCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxDissipation);

	connect(logXCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxLogX);
	connect(logYCheckBox, &QCheckBox::stateChanged, this, &dlg_eventExplorer::updateCheckBoxLogY);

	m_chartConnections = vtkEventQtSlotConnect::New();

	for (size_t i=0; i<numberOfCharts; i++)
	{
		iAVtkWidget* vtkWidget = new iAVtkWidget();
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		vtkWidget->setRenderWindow(renWin);
		m_widgets.push_back(vtkWidget);

		this->horizontalLayout->addWidget(m_widgets.at(i));

		m_contextViews.push_back(vtkSmartPointer<vtkContextView>::New());
		m_charts.push_back(vtkSmartPointer<vtkChartXY>::New());

#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_contextViews.at(i)->SetRenderWindow(m_widgets.at(i)->GetRenderWindow());
#else
		m_contextViews.at(i)->SetRenderWindow(m_widgets.at(i)->renderWindow());
#endif
		m_contextViews.at(i)->GetScene()->AddItem(m_charts.at(i));

		m_chartConnections->Connect(m_charts.at(i),
			vtkCommand::SelectionChangedEvent,
			this,
			SLOT(chartMouseButtonCallBack(vtkObject*)));
	}
	int tableId=0;

	for (QString eventName : EventTypes)
	{
		for (size_t i = 0; i < numberOfCharts; i++)
		{
			m_tables.push_back(vtkSmartPointer<vtkTable>::New());

			VTK_CREATE(vtkFloatArray, arrX);
			arrX->SetName("x");
			m_tables.at(tableId)->AddColumn(arrX);
			for (QString propName : AvailableProperties)
			{
				VTK_CREATE(vtkFloatArray, arrProp);
				auto arrName = QString("%1[%2]").arg(eventName).arg(propName).toStdString();
				arrProp->SetName(arrName.c_str());
				m_tables.at(tableId)->AddColumn(arrProp);
			}
			tableId++;
		}
	}

	iAFeatureTracking *ftF;
	iAFeatureTracking *ftB;

	for (size_t t = 0; t < trackedFeaturesForwards.size(); ++t)
	{
		ftF = trackedFeaturesForwards.at(t);
		ftB = trackedFeaturesBackwards.at(t);

		auto u = ftB->getU();
		auto v = ftF->getV();

		LOG(lvlInfo, QString("%1:   %2 rows in u, %3 rows in v").arg(t).arg(u->GetNumberOfRows()).arg(v->GetNumberOfRows()));

		/*for (int i = 1; i <= numberOfRows; i++)
		{
			std::vector<iAFeatureTrackingCorrespondence> correspondences;
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

		int numberOfRows = (t > 0) ? u->GetNumberOfRows() : v->GetNumberOfRows();

		LOG(lvlInfo, QString("%1 rows\n").arg(numberOfRows));

		for (int i = 0; i < numberOfRows; ++i) //ft->getNumberOfEventsInV()
		{
			//cout << "i: " << i << "   " << v->GetValue(i, 4) << ", " << v->GetValue(i, 5) << ", " << v->GetValue(i, 6) << ", " << v->GetValue(i, 7) << ", " << endl;

			auto correspondences = (t > 0) ? ftB->FromUtoV(i + 1): ftB->FromVtoU(i + 1);

			for (auto c = correspondences.begin(); c != correspondences.end();
				/* ++c */  // currently only one loop iteration is performed anyway (see break at end) -> unreachable code warning if uncommented
			)
			{
				LOG(lvlInfo, QString("i: %1   c->id: %2, event: %3, overlap: %4, volumeRatio: %5   %6, %7, %8, %9")
					.arg(i).arg(c->id).arg(c->featureEvent).arg(c->overlap).arg(c->volumeRatio)
					.arg(toqstr(v->GetValue(i, 4))).arg(toqstr(v->GetValue(i, 5)))
					.arg(toqstr(v->GetValue(i, 6))).arg(toqstr(v->GetValue(i, 7))));

				vtkSmartPointer<vtkVariantArray> arr = vtkSmartPointer<vtkVariantArray>::New();
				arr->SetNumberOfValues(12);
				
				int newEventType = -1;
				if (t > 0)
				{
					switch (c->featureEvent)
					{
						case 0: newEventType =  4; break;
						case 1: newEventType =  1; break;
						case 2: newEventType =  3; break;
						case 3: newEventType =  2; break;
						case 4: newEventType =  0; break;
						default:newEventType = -1; break;
					}
				}
				else if (c->isTakenForCurrentIteration)
				{
					switch (c->featureEvent)
					{
						case 0: newEventType = 0; break;
						case 1: newEventType = 1; break;
						case 2: newEventType = 3; break;
						case 3: newEventType = 2; break;
						case 4: newEventType = 4; break;
						default:newEventType = -1;break;
					}
				}
				assert(newEventType >= 0 && newEventType < EventTypes.size());
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
				m_tables.at(t + numberOfCharts * newEventType)->InsertNextRow(arr);
				break; //only show the best correspondence
			}
		}
	}



	float width = 1.0;

	vtkPlot *plot;
	for(size_t i=0; i<numberOfCharts; i++)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 0), 1, 6);
		plot->SetColor(218,181,214, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for (size_t i=0; i<numberOfCharts; ++i)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 1), 1, 6);
		plot->SetColor(205,221,112, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for (size_t i=0; i<numberOfCharts; ++i)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 2), 1, 6);
		plot->SetColor(135,216,219, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for (size_t i=0; i<numberOfCharts; ++i)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 3), 1, 6);
		plot->SetColor(139,224,164, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}
	for (size_t i=0; i<numberOfCharts; ++i)
	{
		plot = m_charts.at(i)->AddPlot(vtkChart::POINTS);
		plot->SetInputData(m_tables.at(i + numberOfCharts * 4), 1, 6);
		plot->SetColor(228,179,111, 255);
		plot->SetWidth(width);
		plot->SetTooltipLabelFormat("");
		m_plots.push_back(plot);
	}

	for (int i=0; i<numberOfEventTypes; ++i)
	{
		m_plotPositionInVector[i] = i;
	}

	m_numberOfActivePlots = numberOfEventTypes;

	for (size_t i=0; i<numberOfCharts; ++i)
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

void dlg_eventExplorer::updateOpacity(int v, int eventType)
{
	for (size_t i = (m_numberOfCharts * eventType); i < (m_numberOfCharts * (eventType + 1) ); ++i)
	{
		m_plots.at(i)->GetPen()->SetOpacity(v);
	}
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->Update();
	}
}

void dlg_eventExplorer::updateOpacityGrid(int v)
{
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
		m_charts.at(i)->Update();
	}
}

// BEGIN CODE DUPLICATION
// {
void dlg_eventExplorer::updateCheckBoxCreation(int /*c*/)
{
	LOG(lvlInfo, QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));

	if (!creationCheckBox->isChecked())
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[0]);
		}

		for (int i=0; i<m_numberOfEventTypes; ++i)
		{
			if (m_plotPositionInVector[i] > m_plotPositionInVector[0])
			{
				m_plotPositionInVector[i]--;
			}
		}

		m_plotPositionInVector[0] = -1;

		m_numberOfActivePlots--;

		creationSlider->setValue(0);
		updateOpacity(0, Creation);
	}
	else
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 0));
			m_charts.at(i)->Update();

			m_plotPositionInVector[0]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		creationSlider->setValue(255);
		updateOpacity(255, Creation);
	}
	creationCheckBox->update();
	creationSlider->update();
	LOG(lvlInfo, QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxContinuation(int /*c*/)
{
	LOG(lvlInfo, QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!continuationCheckBox->isChecked())
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[1]);
		}

		for (int i=0; i<m_numberOfEventTypes; ++i)
		{
			if (m_plotPositionInVector[i] > m_plotPositionInVector[1])
			{
				--m_plotPositionInVector[i];
			}
		}

		m_plotPositionInVector[1] = -1;

		--m_numberOfActivePlots;

		continuationSlider->setValue(0);
		updateOpacity(0, Continuation);
	}
	else
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 1));

			m_plotPositionInVector[1]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		continuationSlider->setValue(255);
		updateOpacity(255, Continuation);
	}
	continuationCheckBox->update();
	continuationSlider->update();
	LOG(lvlInfo, QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxSplit(int /*c*/)
{
	LOG(lvlInfo, QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if(!splitCheckBox->isChecked())
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[2]);
		}

		for (int i=0; i<m_numberOfEventTypes; ++i)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[2])
			{
				--m_plotPositionInVector[i];
			}
		}

		m_plotPositionInVector[2] = -1;

		--m_numberOfActivePlots;

		splitSlider->setValue(0);
		updateOpacity(0, Bifurcation);
	}
	else
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 2));

			m_plotPositionInVector[2]=m_numberOfActivePlots;
		}

		m_numberOfActivePlots++;

		splitSlider->setValue(255);
		updateOpacity(255, Bifurcation);
	}
	splitCheckBox->update();
	splitSlider->update();
	LOG(lvlInfo, QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxMerge(int /*c*/)
{
	LOG(lvlInfo, QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if (!mergeCheckBox->isChecked())
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[3]);
		}

		for (int i=0; i<m_numberOfEventTypes; ++i)
		{
			if(m_plotPositionInVector[i] > m_plotPositionInVector[3])
			{
				--m_plotPositionInVector[i];
			}
		}

		m_plotPositionInVector[3] = -1;

		m_numberOfActivePlots--;

		mergeSlider->setValue(0);
		updateOpacity(0, Amalgamation);
	}
	else
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i + m_numberOfCharts * 3));

			m_plotPositionInVector[3]=m_numberOfActivePlots;
		}

		++m_numberOfActivePlots;

		mergeSlider->setValue(255);
		updateOpacity(255, Amalgamation);
	}
	mergeCheckBox->update();
	mergeSlider->update();
	LOG(lvlInfo, QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}

void dlg_eventExplorer::updateCheckBoxDissipation(int /*c*/)
{
	LOG(lvlInfo, QString("BEFORE   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
	if (!dissipationCheckBox->isChecked())
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[4]);
		}

		for (int i=0; i<m_numberOfEventTypes; ++i)
		{
			if (m_plotPositionInVector[i] > m_plotPositionInVector[4])
			{
				--m_plotPositionInVector[i];
			}
		}

		m_plotPositionInVector[4] = -1;

		--m_numberOfActivePlots;

		dissipationSlider->setValue(0);
		updateOpacity(0, Dissipation);
	}
	else
	{
		for (size_t i=0; i<m_numberOfCharts; ++i)
		{
			m_charts.at(i)->AddPlot(m_plots.at(i+m_numberOfCharts*4));

			m_plotPositionInVector[4]=m_numberOfActivePlots;
		}

		++m_numberOfActivePlots;

		dissipationSlider->setValue(255);
		updateOpacity(255, Dissipation);
	}
	dissipationCheckBox->update();
	dissipationSlider->update();
	LOG(lvlInfo, QString("AFTER   %1 %2 %3 %4 %5   -   %6")
		.arg(m_plotPositionInVector[0]).arg(m_plotPositionInVector[1]).arg(m_plotPositionInVector[2])
		.arg(m_plotPositionInVector[3]).arg(m_plotPositionInVector[4]).arg(m_numberOfActivePlots));
}
// END CODE DUPLICATION

void dlg_eventExplorer::updateChartLogScale(int axis, bool logScale)
{
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(axis)->SetLogScale(logScale);
	}
}

void dlg_eventExplorer::updateCheckBoxLogX(int /*c*/)
{
	updateChartLogScale(vtkAxis::BOTTOM, logXCheckBox->isChecked());
}

void dlg_eventExplorer::updateCheckBoxLogY(int /*c*/)
{
	updateChartLogScale(vtkAxis::LEFT, logYCheckBox->isChecked());
}

void dlg_eventExplorer::updateChartData(int axis, int s)
{
	if (s >= 5 && s <= 7)
	{
		for (size_t i = 0; i < m_numberOfCharts; ++i)
		{
			m_charts.at(i)->GetAxis(axis)->SetRange(0.0, 1.0);
		}
	}
	for (size_t i = 0; i < m_numberOfCharts * m_numberOfEventTypes; ++i)
	{
		m_plots.at(i)->SetInputData(m_tables.at(i), m_propertyXId + 1, m_propertyYId + 1);
	}
	vtkStdString title = AvailableProperties[s].toStdString();
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(axis)->SetTitle(title);
	}
}

void dlg_eventExplorer::comboBoxXSelectionChanged(int s)
{
	m_propertyXId = s;
	updateChartData(vtkAxis::BOTTOM, s);
}

void dlg_eventExplorer::comboBoxYSelectionChanged(int s)
{
	m_propertyYId = s;
	updateChartData(vtkAxis::LEFT, s);
}

void dlg_eventExplorer::chartMouseButtonCallBack(vtkObject * /*obj*/)
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
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_nodes.push_back(std::vector<int>());
	}

	LOG(lvlInfo, "\n\nSELECTION");

	vtkColorTransferFunction *cTF;
	vtkPiecewiseFunction *oTF;

	if (m_numberOfCharts > std::numeric_limits<int>::max())
	{
		LOG(lvlError, QString("Number of charts (%1) larger than supported (%2)!").arg(m_numberOfCharts).arg(std::numeric_limits<int>::max()));
	}
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		LOG(lvlInfo, QString("\nChart[%1]").arg(i));

		cTF = m_volumeStack->colorTF(i);
		oTF = m_volumeStack->opacityTF(i);

		cTF->RemoveAllPoints();
		oTF->RemoveAllPoints();

		cTF->AddRGBPoint(0, 0.0, 0.0, 0.0);
		oTF->AddPoint(0, 0.0);
		cTF->AddRGBPoint(m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(), 0.0, 0.0, 0.0);
		oTF->AddPoint(m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(), 0.0);

		for (size_t c = 1; c < m_trackedFeaturesBackwards.at(i)->getNumberOfEventsInV(); ++c)
		{
			cTF = m_volumeStack->colorTF(i);
			cTF->AddRGBPoint(c - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
			cTF->AddRGBPoint(c, 0.0, 0.0, 0.0, 0.5, 1.0);
			cTF->AddRGBPoint(c + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
			oTF = m_volumeStack->opacityTF(i);
			oTF->AddPoint(c - 0.5, 0.0, 0.5, 1.0);
			oTF->AddPoint(c, (double)gridOpacitySlider->value() / 255.0 / 100, 0.5, 1.0);
			oTF->AddPoint(c + 0.3, 0.0, 0.5, 1.0);
		}

		int currentPlot = 0;
		for (int j = 0; j < m_numberOfEventTypes; ++j)
		{
			if (m_plotPositionInVector[j] >= 0)
			{
				vtkIdTypeArray *ids = m_charts.at(i)->GetPlot(currentPlot)->GetSelection();

				if (ids != 0)
				{
					LOG(lvlInfo, QString("  Plot[%1] is active. %2 selected.").arg(j).arg(ids->GetNumberOfTuples()));

					for (int k = 0; k < m_numberOfEventTypes; ++k)
					{
						//if(currentPlot==plotPositionInVector[k])
						if (currentPlot == k)
						{
							LOG(lvlInfo, QString("   %1 Events").arg(EventTypes[k]));
							//plots.at(numberOfCharts * k)->GetColor(rgb);
							for (int l = 0; l < ids->GetNumberOfTuples(); ++l)
							{
								double id = m_tables.at(i + m_numberOfCharts * k)->GetRow(ids->GetValue(l))->GetValue(0).ToDouble();
								LOG(lvlInfo, QString("    %1 --> table id: %2").arg(ids->GetValue(l)).arg(id));

								buildGraph(id, static_cast<int>(i), k, m_tables.at(i + m_numberOfCharts * k)->GetRow(ids->GetValue(l))->GetValue(7).ToDouble());
							}
						}
					}
					++currentPlot;
				}
			}
			else
			{
				LOG(lvlInfo, QString("  Plot[%1] is deactivated").arg(j));
			}
		}
		LOG(lvlInfo, QString("   cTF range: %1, %2").arg(cTF->GetRange()[0]).arg(cTF->GetRange()[1]));
	}
	m_trackingGraph->updateGraph(m_graph, this->m_volumeStack->numberOfVolumes(), m_nodesToLayers, m_graphToTableId);
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

		cTF = m_volumeStack->colorTF(layer);
		cTF->AddRGBPoint(id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
		cTF->AddRGBPoint(id, (double)m_rgb[eventType][0] / 255.0, (double)m_rgb[eventType][1] / 255.0, (double)m_rgb[eventType][2] / 255.0, 0.5, 1.0);
		cTF->AddRGBPoint(id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
		oTF = m_volumeStack->opacityTF(layer);
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
			//iAFeatureTracking *ftF = m_trackedFeaturesForwards.at(layer - 1);
			int newVertexId;

			std::vector<iAFeatureTrackingCorrespondence> correspondences;
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
						{
							featureEvent = m_trackedFeaturesForwards.at(layer - 1)->FromVtoU(c.id).at(0).featureEvent;
						}
						else
						{
							featureEvent = 0;
						}

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

						cTF = m_volumeStack->colorTF(layer - 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, (double)m_rgb[featureEvent][0] / 255.0, (double)m_rgb[featureEvent][1] / 255.0, (double)m_rgb[featureEvent][2] / 255.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);

						oTF = m_volumeStack->opacityTF(layer - 1);
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
						LOG(lvlInfo, QString("Edge [%1][%2] --> [%3][%4]").arg(id).arg(layer).arg(c.id).arg(layer - 1));

						/*if (g->GetEdgeId(tableToGraphId[layer][id], tableToGraphId[layer - 1][c.id]) != -1 || g->GetEdgeId(tableToGraphId[layer - 1][c.id], tableToGraphId[layer][id]) != -1)
						{
							g->AddEdge(tableToGraphId[layer][id], tableToGraphId[layer - 1][c.id]);
							std::cout << "Edge [" << id << "][" << layer << "] --> [" << c.id << "][" << layer - 1 << "]" << std::endl;
						}*/
					}
				}
				//break;
			}
		}

		// search forwards
		assert(layer > 0);
		if (static_cast<size_t>(layer) < m_numberOfCharts - 1)
		{
			//iAFeatureTracking *ftB = m_trackedFeaturesBackwards.at(layer + 1);
			iAFeatureTracking *ftF = m_trackedFeaturesForwards.at(layer + 1);
			int newVertexId;

			std::vector<iAFeatureTrackingCorrespondence> correspondences;
			if (layer > 0)
			{
				correspondences = ftF->FromVtoU(id);
			}
			else
			{
				correspondences = ftF->FromUtoV(id);
			}

			for (auto c : correspondences)
			{
				if (c.id > 0 && c.isTakenForCurrentIteration)
				{
					if (m_tableToGraphId[layer + 1].find(c.id) == m_tableToGraphId[layer + 1].end())
					{
						int featureEvent = 0;
						//if (trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).size() > 0)
						//	featureEvent = trackedFeaturesBackwards.at(layer - 1)->FromUtoV(c.id).at(0).featureEvent;
						if (m_trackedFeaturesForwards.at(layer + 1)->FromUtoV(c.id).size() > 0)
						{
							featureEvent = m_trackedFeaturesForwards.at(layer + 1)->FromUtoV(c.id).at(0).featureEvent;
						}

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

						cTF = m_volumeStack->colorTF(layer + 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, (double)m_rgb[featureEvent][0] / 255.0, (double)m_rgb[featureEvent][1] / 255.0, (double)m_rgb[featureEvent][2] / 255.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
						oTF = m_volumeStack->opacityTF(layer + 1);
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
						LOG(lvlInfo, QString("Edge [%1][%2] --> [%3][%4]").arg(id).arg(layer).arg(c.id).arg(layer + 1));

						/*if (g->GetEdgeId(tableToGraphId[layer][id], tableToGraphId[layer + 1][c.id]) != -1 || g->GetEdgeId(tableToGraphId[layer + 1][c.id], tableToGraphId[layer][id]) != -1)
						{
							g->AddEdge(tableToGraphId[layer][id], tableToGraphId[layer + 1][c.id]);
							std::cout << "Edge [" << id << "][" << layer << "] --> [" << c.id << "][" << layer + 1 << "]" << std::endl;
						}*/
					}
				}
				//break;
			}
		}
	}
}

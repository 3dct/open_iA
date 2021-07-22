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
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPen.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlot.h>
#include <vtkPlotPoints.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <array>
#include <sstream>

namespace
{
	const QStringList EventNames = QStringList()
		<< "Creation"
		<< "Continuation"
		<< "Split"
		<< "Merge"
		<< "Dissipation";
	const std::array<QColor, 5> EventColors =
	{	// use color theme instead?
		QColor(218, 181, 214),
		QColor(205, 221, 112),
		QColor(135, 216, 219),
		QColor(139, 224, 164),
		QColor(228, 179, 111)
	};
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

QString toqstr(vtkVariant const & var)
{
	std::ostringstream oss;
	oss << var;
	return QString(oss.str().c_str());
}

void dlg_eventExplorer::addPlot(int eventID, size_t chartID)
{
	const float width = 1.0;
	vtkPlot* plot = m_charts.at(chartID)->AddPlot(vtkChart::POINTS);
	plot->SetInputData(m_tables.at(chartID + m_numberOfCharts * eventID), m_propertyXId, m_propertyYId);
	QColor c = EventColors[eventID];
	plot->SetColor(static_cast<unsigned char>(c.red()), static_cast<unsigned char>(c.green()),
		static_cast<unsigned char>(c.blue()), static_cast<unsigned char>(c.alpha()));
	plot->SetWidth(width);
	plot->SetTooltipLabelFormat("");
	dynamic_cast<vtkPlotPoints*>(plot)->SetMarkerStyle(vtkPlotPoints::CIRCLE);
	m_plots[chartID + m_numberOfCharts * eventID] = plot;
}

dlg_eventExplorer::dlg_eventExplorer(QWidget *parent, size_t numberOfCharts, int numberOfEventTypes, iAVolumeStack* volumeStack, dlg_trackingGraph* trackingGraph, std::vector<iAFeatureTracking*> trackedFeaturesForwards, std::vector<iAFeatureTracking*> trackedFeaturesBackwards) : QDockWidget(parent)
{
	setupUi(this);

	m_slider.push_back(creationSlider);
	m_slider.push_back(continuationSlider);
	m_slider.push_back(splitSlider);
	m_slider.push_back(mergeSlider);
	m_slider.push_back(dissipationSlider);

	this->m_numberOfCharts = numberOfCharts;
	this->m_numberOfEventTypes = numberOfEventTypes;
	this->m_volumeStack = volumeStack;
	this->m_trackingGraph = trackingGraph;
	this->m_trackedFeaturesForwards = trackedFeaturesForwards;
	this->m_trackedFeaturesBackwards = trackedFeaturesBackwards;

	m_propertyXId = 0;
	m_propertyYId = 7;

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

	connect(creationSlider,     &QSlider::sliderMoved, [this](int v) { setOpacity(Creation, v); });
	connect(continuationSlider, &QSlider::sliderMoved, [this](int v) { setOpacity(Continuation, v); });
	connect(splitSlider,        &QSlider::sliderMoved, [this](int v) { setOpacity(Bifurcation, v); });
	connect(mergeSlider,        &QSlider::sliderMoved, [this](int v) { setOpacity(Amalgamation, v); });
	connect(dissipationSlider,  &QSlider::sliderMoved, [this](int v) { setOpacity(Dissipation, v); });

	connect(gridOpacitySlider, &QSlider::sliderMoved, this, &dlg_eventExplorer::setGridOpacity);

	connect(creationCheckBox, &QCheckBox::stateChanged,     [this](int c) { updateCheckBox(Creation, c == Qt::Checked); });
	connect(continuationCheckBox, &QCheckBox::stateChanged, [this](int c) { updateCheckBox(Continuation, c == Qt::Checked); });
	connect(splitCheckBox, &QCheckBox::stateChanged,        [this](int c) { updateCheckBox(Bifurcation, c == Qt::Checked); });
	connect(mergeCheckBox, &QCheckBox::stateChanged,        [this](int c) { updateCheckBox(Amalgamation, c == Qt::Checked); });
	connect(dissipationCheckBox, &QCheckBox::stateChanged,  [this](int c) { updateCheckBox(Dissipation, c == Qt::Checked); });

	connect(logXCheckBox, &QCheckBox::stateChanged, [this](int c) { setChartLogScale(vtkAxis::BOTTOM, c == Qt::Checked); });
	connect(logYCheckBox, &QCheckBox::stateChanged, [this](int c) { setChartLogScale(vtkAxis::LEFT, c == Qt::Checked); });

	m_chartConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();

	for (size_t i=0; i<numberOfCharts; i++)
	{
		iAVtkWidget* vtkWidget = new iAVtkWidget();
		vtkWidget->setFormat(QVTKOpenGLNativeWidget::defaultFormat());
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		vtkWidget->SetRenderWindow(renWin);
#else
		vtkWidget->setRenderWindow(renWin);
#endif
		m_widgets.push_back(vtkWidget);
		this->horizontalLayout->addWidget(m_widgets.at(i));
		m_contextViews.push_back(vtkSmartPointer<vtkContextView>::New());
		m_charts.push_back(vtkSmartPointer<vtkChartXY>::New());
		m_contextViews.at(i)->SetRenderWindow(renWin);
		m_contextViews.at(i)->GetScene()->AddItem(m_charts.at(i));
		m_chartConnections->Connect(m_charts.at(i),
			vtkCommand::SelectionChangedEvent,
			this,
			SLOT(chartSelectionChanged(vtkObject*)));
	}
	int tableId=0;

	for (QString eventName : EventNames)
	{
		for (size_t i = 0; i < numberOfCharts; i++)
		{
			m_tables.push_back(vtkSmartPointer<vtkTable>::New());

			auto arrX = vtkSmartPointer<vtkFloatArray>::New();
			arrX->SetName("x");
			m_tables.at(tableId)->AddColumn(arrX);
			for (QString propName : AvailableProperties)
			{
				auto arrProp = vtkSmartPointer<vtkFloatArray>::New();
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
				assert(newEventType >= 0 && newEventType < EventNames.size());
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

	m_plots.resize(EventColors.size() * numberOfCharts);
	for (size_t eventID = 0; eventID < EventColors.size(); ++eventID)
	{
		for (size_t i = 0; i < numberOfCharts; i++)
		{
			addPlot(eventID, i);
		}
	}

	for (int i=0; i<numberOfEventTypes; ++i)
	{
		m_plotPositionInVector[i] = i;
	}

	m_numberOfActivePlots = numberOfEventTypes;

	for (size_t i=0; i<numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->SetTitle(AvailableProperties[m_propertyYId].toStdString().c_str());
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->SetTitle(AvailableProperties[m_propertyXId].toStdString().c_str());
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, 1.0);
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, 1.0);
	}
	updateCharts();
}


dlg_eventExplorer::~dlg_eventExplorer()
{
	//TODO
}

void dlg_eventExplorer::setOpacity(int eventType, int value)
{
	for (size_t i = (m_numberOfCharts * eventType); i < (m_numberOfCharts * (eventType + 1) ); ++i)
	{
		m_plots[i]->GetPen()->SetOpacity(value);
	}
	updateCharts();
}

void dlg_eventExplorer::updateCharts()
{
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_widgets.at(i)->GetRenderWindow()->Render();
#else
		m_widgets.at(i)->renderWindow()->Render();
#endif
	}
}

void dlg_eventExplorer::setGridOpacity(int v)
{
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(vtkAxis::BOTTOM)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
		m_charts.at(i)->GetAxis(vtkAxis::LEFT)->GetGridPen()->SetColorF(0.5, 0.5, 0.5, v/255.0);
	}
	updateCharts();
}

void dlg_eventExplorer::updateCheckBox(int eventType, int checked)
{
	LOG(lvlDebug,
		QString("BEFORE   %1 %2 %3 %4 %5   -   %6 (%7)")
			.arg(m_plotPositionInVector[0])
			.arg(m_plotPositionInVector[1])
			.arg(m_plotPositionInVector[2])
			.arg(m_plotPositionInVector[3])
			.arg(m_plotPositionInVector[4])
			.arg(m_numberOfActivePlots)
			.arg(EventNames[eventType]));
	if (!checked)
	{
		for (size_t i = 0; i < m_numberOfCharts; ++i)
		{
			m_charts.at(i)->RemovePlot(m_plotPositionInVector[eventType]);
			m_plots[i + m_numberOfCharts * eventType] = nullptr;
		}
		for (int i = 0; i < m_numberOfEventTypes; ++i)
		{
			if (m_plotPositionInVector[i] > m_plotPositionInVector[eventType])
			{
				--m_plotPositionInVector[i];
			}
		}
		m_plotPositionInVector[eventType] = -1;
		--m_numberOfActivePlots;
		m_slider[eventType]->setValue(0);
		m_slider[eventType]->setDisabled(true);
	}
	else
	{
		m_plotPositionInVector[eventType] = m_numberOfActivePlots;
		for (size_t i = 0; i < m_numberOfCharts; ++i)
		{
			addPlot(eventType, i);
		}
		++m_numberOfActivePlots;
		m_slider[eventType]->setValue(255);
		m_slider[eventType]->setDisabled(false);
	}
	m_slider[eventType]->update();
	LOG(lvlDebug,
		QString("AFTER   %1 %2 %3 %4 %5   -   %6 (%7)")
			.arg(m_plotPositionInVector[0])
			.arg(m_plotPositionInVector[1])
			.arg(m_plotPositionInVector[2])
			.arg(m_plotPositionInVector[3])
			.arg(m_plotPositionInVector[4])
			.arg(m_numberOfActivePlots)
			.arg(EventNames[eventType]));
}

void dlg_eventExplorer::setChartLogScale(int axis, bool logScale)
{
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(axis)->SetLogScale(logScale);
	}
	updateCharts();
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
		if (m_plots[i])
		{
			m_plots[i]->SetInputData(m_tables.at(i), m_propertyXId, m_propertyYId);
		}
	}
	vtkStdString title = AvailableProperties[s].toStdString();
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		m_charts.at(i)->GetAxis(axis)->SetTitle(title);
	}
	updateCharts();
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

void dlg_eventExplorer::chartSelectionChanged(vtkObject* /*obj*/)
{
	//clear graph TODO
	m_graph = vtkSmartPointer<vtkMutableDirectedGraph>::New();
	m_labels = vtkSmartPointer<vtkStringArray>::New();
	m_labels->SetName("Label");
	m_nodeLayer = vtkSmartPointer<vtkIntArray>::New();
	m_nodeLayer->SetName("Layer");
	m_colorR = vtkSmartPointer<vtkIntArray>::New();
	m_colorR->SetName("ColorR");
	m_colorG = vtkSmartPointer<vtkIntArray>::New();
	m_colorG->SetName("ColorG");
	m_colorB = vtkSmartPointer<vtkIntArray>::New();
	m_colorB->SetName("ColorB");
	m_trackingUncertainty = vtkSmartPointer<vtkDoubleArray>::New();
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

	if (m_numberOfCharts > std::numeric_limits<int>::max())
	{
		LOG(lvlError, QString("Number of charts (%1) larger than supported (%2)!").arg(m_numberOfCharts).arg(std::numeric_limits<int>::max()));
	}
	for (size_t i = 0; i < m_numberOfCharts; ++i)
	{
		LOG(lvlInfo, QString("\nChart[%1]").arg(i));

		auto cTF = m_volumeStack->colorTF(i);
		auto oTF = m_volumeStack->opacityTF(i);

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
							LOG(lvlInfo, QString("   %1 Events").arg(EventNames[k]));
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
	m_trackingGraph->updateGraph(m_graph, this->m_volumeStack->numberOfVolumes());
}

void dlg_eventExplorer::buildGraph(int id, int layer, int eventType, double uncertainty)
{
	if (m_tableToGraphId[layer].find(id) == m_tableToGraphId[layer].end())
	{
		vtkIdType vId = m_graph->AddVertex();
		m_labels->InsertValue(vId, "[" + std::to_string((long long)id) + "]" + " (" + std::to_string((long long)uncertainty) + ")");
		m_nodeLayer->InsertValue(vId, layer);
		m_colorR->InsertValue(vId, EventColors[eventType].red());
		m_colorG->InsertValue(vId, EventColors[eventType].green());
		m_colorB->InsertValue(vId, EventColors[eventType].blue());
		m_trackingUncertainty->InsertValue(vId, uncertainty);

		m_graphToTableId[layer][vId] = id;
		m_tableToGraphId[layer][id] = vId;
		m_nodesToLayers[vId] = layer;

		auto cTF = m_volumeStack->colorTF(layer);
		cTF->AddRGBPoint(id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
		cTF->AddRGBPoint(id, EventColors[eventType].redF(), EventColors[eventType].greenF(), EventColors[eventType].blueF(), 0.5, 1.0);
		cTF->AddRGBPoint(id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
		auto oTF = m_volumeStack->opacityTF(layer);
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
						m_colorR->InsertValue(newVertexId, EventColors[featureEvent].red());
						m_colorG->InsertValue(newVertexId, EventColors[featureEvent].green());
						m_colorB->InsertValue(newVertexId, EventColors[featureEvent].blue());
						m_trackingUncertainty->InsertValue(newVertexId, 1 - c.likelyhood);

						m_graphToTableId[layer - 1][newVertexId] = c.id;
						m_tableToGraphId[layer - 1][c.id] = newVertexId;

						m_nodesToLayers[newVertexId] = layer - 1;

						auto cTF = m_volumeStack->colorTF(layer - 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, EventColors[featureEvent].redF(), EventColors[featureEvent].greenF(), EventColors[featureEvent].blueF(), 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);

						auto oTF = m_volumeStack->opacityTF(layer - 1);
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
		//assert(layer > 0);		// not sure whether this check is required?
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

						m_colorR->InsertValue(newVertexId, EventColors[featureEvent].red());
						m_colorG->InsertValue(newVertexId, EventColors[featureEvent].green());
						m_colorB->InsertValue(newVertexId, EventColors[featureEvent].blue());
						m_trackingUncertainty->InsertValue(newVertexId, 1 - c.likelyhood);

						m_graphToTableId[layer + 1][newVertexId] = c.id;
						m_tableToGraphId[layer + 1][c.id] = newVertexId;

						m_nodesToLayers[newVertexId] = layer + 1;

						auto cTF = m_volumeStack->colorTF(layer + 1);
						cTF->AddRGBPoint(c.id - 0.5, 0.0, 0.0, 0.0, 0.5, 1.0);
						cTF->AddRGBPoint(c.id, EventColors[featureEvent].redF(), EventColors[featureEvent].greenF(), EventColors[featureEvent].blueF(), 0.5, 1.0);
						cTF->AddRGBPoint(c.id + 0.3, 0.0, 0.0, 0.0, 0.5, 1.0);
						auto oTF = m_volumeStack->opacityTF(layer + 1);
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

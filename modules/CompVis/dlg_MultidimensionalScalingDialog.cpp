/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_MultidimensionalScalingDialog.h"

//QT
#include "qbuttongroup.h"
#include "qstringlist.h"
#include <QHeaderView>
#include "qstring.h"

dlg_MultidimensionalScalingDialog::dlg_MultidimensionalScalingDialog(QWidget* parent, QList<csvFileData>* data, iAMultidimensionalScaling* mds) :
	QDialog(parent),
	m_data(data),
	m_weights(mds->getWeights()),
	m_mds(mds)
{
	setupUi(this);

	setupWeigthTable();
	setupProximityBox();
	setupDistanceBox();
	connectSignals();
}

void dlg_MultidimensionalScalingDialog::setupWeigthTable()
{
	QStringList elems = *(m_data->at(0).header);
	elems.removeFirst(); //neglect label row
	int amountElems = elems.size(); 
	
	QList<QTableWidgetItem*>* tableItems = new QList<QTableWidgetItem*>();

	//init tableWidget
	tableWeightsWidget->setRowCount(amountElems);
	tableWeightsWidget->setColumnCount(2);

	//insert labels
	QStringList header;
	header << "Attributes"<< "Weigth in Percentage (%)";
	tableWeightsWidget->setHorizontalHeaderLabels(header);
	tableWeightsWidget->verticalHeader()->setVisible(false);
	tableWeightsWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	//calculate weight
	//double sumOfWeights = 100;
	//std::vector<double>* indivWeight;
	if (m_weights->size() == 0)
	{
		m_weights = new std::vector<double>(amountElems, 100 / (amountElems)); 
	}
	
	//start with second argument --> neglect label
	for (int ind = 0; ind < amountElems; ind++)
	{
		QTableWidgetItem* item = new QTableWidgetItem(elems.at(ind));
		item->setFlags(item->flags() ^ Qt::ItemIsEditable);
		tableItems->append(item);
		tableWeightsWidget->setItem(ind, 0, item);

		QTableWidgetItem* weight = new QTableWidgetItem(QString::number(m_weights->at(ind)));
		tableWeightsWidget->setItem(ind, 1, weight);
	}

	connect(tableWeightsWidget, &QTableWidget::cellChanged, this, &dlg_MultidimensionalScalingDialog::onCellChanged);
}

void dlg_MultidimensionalScalingDialog::onCellChanged(int row, int column)
{
	double inputWeight = tableWeightsWidget->item(row, column)->text().toDouble();

	if(inputWeight <= 0)
	{ //weights are not allowed to be completely 0, since arccosine distance is only delivering 0 as value
		m_weights->at(row) = 0.01;
	}
	else
	{
		m_weights->at(row) = inputWeight;
	}

}

void dlg_MultidimensionalScalingDialog::setupProximityBox()
{
	int amount = static_cast<int>(MDS::ProximityMetric::NumberOfProximityMetrics);
	
	m_proxiGroup = new QButtonGroup(box_Proximity);
	m_proxiGroup->setExclusive(true);

	for (int i = 0; i < amount; i++)
	{
		QCheckBox* dynamic = new QCheckBox(MDS::proximityMetric_to_string(i));
		layoutProximityMetric->addWidget(dynamic);
		m_proxiGroup->addButton(dynamic);
	
		//check the first checkbox
		if (i == 0)
		{
			dynamic->setChecked(true);
		}	
	}
}

void dlg_MultidimensionalScalingDialog::setupDistanceBox()
{
	int amount = static_cast<int>(MDS::DistanceMetric::NumberOfDistanceMetrics);

	m_disGroup = new QButtonGroup(box_Distance);
	m_disGroup->setExclusive(true);

	for (int i = 0; i < amount; i++)
	{
		QCheckBox* dynamic = new QCheckBox(MDS::distanceMetric_to_string(i));
		layoutDistanceMetric->addWidget(dynamic);
		m_disGroup->addButton(dynamic);

		//check the first checkbox
		if (i == 0)
		{
			dynamic->setChecked(true);
		}
	}
}

void dlg_MultidimensionalScalingDialog::connectSignals()
{
	connect(Button_StartMDS, &QPushButton::clicked, this, &dlg_MultidimensionalScalingDialog::okBtnClicked);
}

void dlg_MultidimensionalScalingDialog::okBtnClicked()
{
	MDS::ProximityMetric proxiName = MDS::string_to_proximityMetric(m_proxiGroup->checkedButton()->text());
	m_mds->setProximityMetric(proxiName);

	MDS::DistanceMetric disName = MDS::string_to_distanceMetric(m_disGroup->checkedButton()->text());
	m_mds->setDistanceMetric(disName);

	m_mds->startMDS(m_weights);

	accept();
}

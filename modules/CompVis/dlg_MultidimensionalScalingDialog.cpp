#include "dlg_MultidimensionalScalingDialog.h"

//Debug
#include "iAConsole.h"

//QT
#include "qbuttongroup.h"
#include "qstringlist.h"
#include <QHeaderView>
#include "qstring.h"

dlg_MultidimensionalScalingDialog::dlg_MultidimensionalScalingDialog(
	QList<csvFileData>* data, iAMultidimensionalScaling* mds, QWidget* parent /* = 0,*/, Qt::WindowFlags f /* f = 0*/) :
	QDialog(parent, f),
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
	int amountElems = m_data->at(0).header->size()-1; //neglect label row
	QStringList* elems = m_data->at(0).header;
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
	double sumOfWeights = 100;
	std::vector<double>* indivWeight;
	if (m_weights->size() == 0)
	{
		m_weights = new std::vector<double>(amountElems, 100 / (amountElems)); 
	}
	
	//start with second argument --> neglect label
	for (int ind = 1; ind <= amountElems; ind++)
	{
		QTableWidgetItem* item = new QTableWidgetItem(elems->at(ind));
		item->setFlags(item->flags() ^ Qt::ItemIsEditable);
		tableItems->append(item);
		tableWeightsWidget->setItem(ind-1, 0, item);

		QTableWidgetItem* weight = new QTableWidgetItem(QString::number(m_weights->at(ind - 1)));
		tableWeightsWidget->setItem(ind-1, 1, weight);
	}

	connect(tableWeightsWidget, &QTableWidget::cellChanged, this, &dlg_MultidimensionalScalingDialog::onCellChanged);
}

void dlg_MultidimensionalScalingDialog::onCellChanged(int row, int column)
{
	m_weights->at(row) =  tableWeightsWidget->item(row, column)->text().toDouble();
}

void dlg_MultidimensionalScalingDialog::setupProximityBox()
{
	int amount = static_cast<int>(ProximityMetric::NumberOfProximityMetrics);
	
	m_proxiGroup = new QButtonGroup(box_Proximity);
	m_proxiGroup->setExclusive(true);

	for (int i = 0; i < amount; i++)
	{
		QCheckBox* dynamic = new QCheckBox(proximityMetric_to_string(i));
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
	int amount = static_cast<int>(DistanceMetric::NumberOfDistanceMetrics);

	m_disGroup = new QButtonGroup(box_Distance);
	m_disGroup->setExclusive(true);

	for (int i = 0; i < amount; i++)
	{
		QCheckBox* dynamic = new QCheckBox(distanceMetric_to_string(i));
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
	connect(Box_StartMDS, &QDialogButtonBox::accepted, this, &dlg_MultidimensionalScalingDialog::okBtnClicked);
}

void dlg_MultidimensionalScalingDialog::okBtnClicked()
{
	ProximityMetric proxiName = string_to_proximityMetric(m_proxiGroup->checkedButton()->text());
	m_mds->setProximityMetric(proxiName);

	DistanceMetric disName = string_to_distanceMetric(m_disGroup->checkedButton()->text());
	m_mds->setDistanceMetric(disName);

	m_mds->startMDS(m_weights);

	accept();
}

//TODO
void dlg_MultidimensionalScalingDialog::checkWeightValues()
{

}


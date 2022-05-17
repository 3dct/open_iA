#include "iAComp3DView.h"

//CompVis
#include "iACompVisOptions.h"
#include "iAComp3DWidget.h"

//iA
#include "iAMainWindow.h"
#include "iAQVTKWidget.h"
#include "iACsvIO.h"
#include "iACsvConfig.h"

// Qt
#include <QGridLayout>

#include "vtkRenderer.h"
#include "vtkTable.h"

iAComp3DView::iAComp3DView(iAMainWindow* parent, iACsvDataStorage* dataStorage) : 
	m_dataStorage(dataStorage), 
	m_dockWidgets(new std::vector<iAComp3DWidget*>())
{
	std::vector<vtkSmartPointer<vtkTable>>* objectTables = m_dataStorage->getObjectTables();
	std::vector<iACsvIO*>* ios = m_dataStorage->getIOs();
	std::vector<const iACsvConfig*>* csvConfigs = m_dataStorage->getCsvConfigs();

	for (int i = 0; i < static_cast<int>(ios->size()); i++)
	{
		iAComp3DWidget* widget = new iAComp3DWidget(parent, objectTables->at(i), ios->at(i), csvConfigs->at(i));
		widget->setWindowTitle(iACompVisOptions::getLabel(m_dataStorage->getDatasetNames()->at(i)));
		m_dockWidgets->push_back(widget);
	}
}

 void iAComp3DView::constructGridLayout(QGridLayout* layout)
{
	 int row = 0;
	int col = 0;
	 for (int i = 0; i < static_cast<int>(m_dockWidgets->size()); i++)
	 {
		 if ((i == (static_cast<int>(m_dockWidgets->size())-1)) && ((i%2) == 0))
		 {
			 layout->addWidget(m_dockWidgets->at(i), row, col , 1, col);
		 }
		 else
		 {
			 layout->addWidget(m_dockWidgets->at(i), row, col);
		 }

		if ((i+1) % 2 == 0)
		 { //uneven
			col = 0;
			 row += 1;
		 }
		 else
		 { //even
			 col = 1;
		 }
	 }
}

 void iAComp3DView::update3DViews(
	csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	 QStringList allDatasetNames = *m_dataStorage->getDatasetNames();
	 QStringList names = QStringList();

	 int indexInSelectedData = 0;
	 std::vector<double> allObjectLables = selectedData->at(0);

	 for (std::map<int, std::vector<double>>::const_iterator it = pickStatistic->begin(); it != pickStatistic->end();
		  ++it)
	 {
		 int selectedDataInd = it->first;
		 int numberOfObjectsPicked = it->second.at(1);

		 std::vector<size_t> selectedIds;
		 for (int objectId = 0; objectId < numberOfObjectsPicked; objectId++)
		 {
			 size_t labelId = allObjectLables.at(indexInSelectedData);
			 
			 selectedIds.push_back(labelId);
				 
				 
			 indexInSelectedData++;
		 }

		 iAComp3DWidget* currWidget = m_dockWidgets->at(selectedDataInd);
		 currWidget->drawSelection(selectedIds);
	 }

}

 void iAComp3DView::reset3DViews()
{
	 for (int i = 0; i < static_cast<int>(m_dockWidgets->size()); i++)
	 {
		 m_dockWidgets->at(i)->resetWidget();
	 }
 }
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
#include "iASimilarityModuleInterface.h"

#include "dlg_commoninput.h"
#include "iASimilarity.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QSettings>
#include <QMessageBox>

void iASimilarityModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSimilarity = getMenuWithTitle(filtersMenu, QString("Similarity"));
	QAction * actionCalculateSimilarityMetrics = new QAction(QApplication::translate("MainWindow", "Similarity Metrics", 0), m_mainWnd);
	menuSimilarity->addAction(actionCalculateSimilarityMetrics);
	connect(actionCalculateSimilarityMetrics, SIGNAL(triggered()), this, SLOT(calc_similarity_metrics()));
}

void iASimilarityModuleInterface::calc_similarity_metrics()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;	

	QString filterName = "Similarity Metrics";
	m_childClosed = false;
	MdiChild *active_child = m_mdiChild;
	connect(active_child, SIGNAL(closed()), this, SLOT(childClosed()));
	
	MdiChild *nonActive_child = GetSecondNonActiveChild();
	if (!nonActive_child)
		return;	
	connect(nonActive_child, SIGNAL(closed()), this, SLOT(childClosed()));

	if (*active_child->getImagePointer()->GetSpacing()    != *nonActive_child->getImagePointer()->GetSpacing()    ||
		*active_child->getImagePointer()->GetOrigin()     != *nonActive_child->getImagePointer()->GetOrigin()     ||
		*active_child->getImagePointer()->GetDimensions() != *nonActive_child->getImagePointer()->GetDimensions() ||
		*active_child->getImagePointer()->GetExtent()     != *nonActive_child->getImagePointer()->GetExtent()     ||
		 active_child->getImagePointer()->GetScalarType() !=  nonActive_child->getImagePointer()->GetScalarType())
	{
		QMessageBox::warning(m_mainWnd, "Similarity Metrics", "The images do not have the same size, dimension, origin, spacing, or image type!");
		return;
	}
		
	//set filter description
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml("<p>NOTE: Normailze the images before calculating the simialrity metrics!</p>"
		"<p>Define the region of interest by setting the extents of the red box (in the slicers). "
		"Select the metrics you want to calculate. See the notes below.</p>"
		"<p>Make sure the images have the same size, origin, spacing, and image type.</p>"
		"<p>General information on similarity metrics: https://itk.org/Doxygen/html/ImageSimilarityMetricsPage.html </p>"
		"<p> Mean Squares Metric: The optimal value of the metric is zero. Poor matches between images A and B result in large "
		"values of the metric. This metric relies on the assumption that intensity representing the same homologous point "
		"must be the same in both images. https://itk.org/Doxygen/html/classitk_1_1MeanSquaresImageToImageMetric.html </p>"
		"<p> Normalized Correlation Metric: Note the −1 factor in the metric computation. This factor is used to make the " 
		"metric be optimal when its minimum is reached.The optimal value of the metric is then minus one.Misalignment "
		"between the images results in small measure values. "
		"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageToImageMetric.html </p>"
		"<p>More Information on Mutual Information is given in the 'ITK Software Guide' in the sections '3.10.4 Mutual "
		"Information Metric' (pp. 262-264) and '5.3.2 Information Theory' (pp. 462-471) </p>");

	QSettings settings;
	smMeanSquares = settings.value("Filters/Similarity/smMeanSquares").toBool();
	smNormalizedCorrelation = settings.value("Filters/Similarity/msNormalizedCorrelation").toBool();
	smMutualInformation = settings.value("Filters/Similarity/smMutualInformation").toBool();
	smMIHistogramBins = settings.value("Filters/Similarity/smMIHistogramBins", 255).toInt();

	//set parameters
	QStringList inList = (QStringList() << tr("*IndexX") << tr("*IndexY") << tr("*IndexZ")
		<< tr("*SizeX") << tr("*SizeY") << tr("*SizeZ") 
		<< tr("$Means Squares") << tr("$Normalized Correlation")
		<< tr("$Mutual Information (MI)") << tr("*    MI Histogram Bins") );
	QList<QVariant> inPara = (QList<QVariant>()
		<< tr("%1").arg(eiIndexX) << tr("%1").arg(eiIndexY) << tr("%1").arg(eiIndexZ)
		<< tr("%1").arg(m_childData.imgData->GetExtent()[1] + 1)
		<< tr("%1").arg(m_childData.imgData->GetExtent()[3] + 1)
		<< tr("%1").arg(m_childData.imgData->GetExtent()[5] + 1) 
		<< (smMeanSquares ? tr("true") : tr("false"))
		<< (smNormalizedCorrelation ? tr("true") : tr("false"))
		<< (smMutualInformation ? tr("true") : tr("false"))
		<< tr("%1").arg(smMIHistogramBins));

	dlg_commoninput dlg(m_mainWnd, "Similarity Metrics", inList, inPara, fDescr);
	dlg.connectMdiChild(active_child);
	dlg.connectMdiChild(nonActive_child);
	dlg.setModal(false);
	dlg.show();
	active_child->activate(MdiChild::cs_ROI);
	active_child->setROI(eiIndexX, eiIndexY, eiIndexZ,
		m_childData.imgData->GetExtent()[1] + 1,
		m_childData.imgData->GetExtent()[3] + 1,
		m_childData.imgData->GetExtent()[5] + 1);
	active_child->showROI();

	nonActive_child->activate(MdiChild::cs_ROI);
	nonActive_child->setROI(eiIndexX, eiIndexY, eiIndexZ,
		m_childData.imgData->GetExtent()[1] + 1,
		m_childData.imgData->GetExtent()[3] + 1,
		m_childData.imgData->GetExtent()[5] + 1);
	nonActive_child->showROI();

	int result = dlg.exec();
	if (!m_mainWnd->isVisible() || m_childClosed)	// main window or mdi child was closed in the meantime
		return;	

	//active_child->hideROI();
	//active_child->deactivate();
	//nonActive_child->hideROI();
	//nonActive_child->deactivate();

	if (result != QDialog::Accepted)
		return;

	eiIndexX = dlg.getSpinBoxValues()[0];
	eiIndexY = dlg.getSpinBoxValues()[1];
	eiIndexZ = dlg.getSpinBoxValues()[2];
	eiSizeX = dlg.getSpinBoxValues()[3];
	eiSizeY = dlg.getSpinBoxValues()[4];
	eiSizeZ = dlg.getSpinBoxValues()[5];
	smMeanSquares = dlg.getCheckValues()[6];
	smNormalizedCorrelation = dlg.getCheckValues()[7];
	smMutualInformation = dlg.getCheckValues()[8];
	smMIHistogramBins = dlg.getSpinBoxValues()[9];

	settings.setValue("Filters/Similarity/smMeanSquares", smMeanSquares);
	settings.setValue("Filters/Similarity/msNormalizedCorrelation", smNormalizedCorrelation);
	settings.setValue("Filters/Similarity/smMutualInformation", smMutualInformation);
	settings.setValue("Filters/Similarity/smMIHistogramBins", smMIHistogramBins);

	//execute
	iASimilarity * thread = new iASimilarity(filterName, SIMILARITY_METRICS,
		active_child->getImagePointer(), NULL, active_child->getLogger(), active_child);
	active_child->connectThreadSignalsToChildSlots(thread);
	thread->setSMParameters(eiIndexX, eiIndexY, eiIndexZ, eiSizeX, eiSizeY, eiSizeZ,
		smMeanSquares, smNormalizedCorrelation, smMutualInformation, smMIHistogramBins,
		nonActive_child->getImagePointer(), active_child->windowTitle(), nonActive_child->windowTitle());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
	active_child->logs->show();
}

void iASimilarityModuleInterface::childClosed()
{
	m_childClosed = true;
}
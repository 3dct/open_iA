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

void iASimilarityModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSimilarity = getMenuWithTitle(filtersMenu, QString("Similarity"));
	QAction * actionCalculateSimilarityMetrics = new QAction(QApplication::translate("MainWindow", "Calculate Similarity Metrics", 0), m_mainWnd);
	menuSimilarity->addAction(actionCalculateSimilarityMetrics);
	connect(actionCalculateSimilarityMetrics, SIGNAL(triggered()), this, SLOT(calc_similarity_metrics()));
}

void iASimilarityModuleInterface::calc_similarity_metrics()
{
	//set filter description
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml("<p>NOTE: Normalize the images before calculating the similarity metrics!</p>"
		"<p><a href=\"https://itk.org/Doxygen/html/ImageSimilarityMetricsPage.html\">General information on ITK similarity metrics</a> </p>"
		"<p> <a href=\"https://itk.org/Doxygen/html/classitk_1_1MeanSquaresImageToImageMetric.html\">"
		"Mean Squares Metric</a>: The optimal value of the metric is zero. Poor matches between images A and B result in large "
		"values of the metric. This metric relies on the assumption that intensity representing the same homologous point "
		"must be the same in both images.</p>"
		"<p> <a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageToImageMetric.html\">"
		"Normalized Correlation Metric</a>: Note the −1 factor in the metric computation. This factor is used to make the "
		"metric be optimal when its minimum is reached.The optimal value of the metric is then minus one. Misalignment "
		"between the images results in small measure values.</p>"
		"<p>More Information on Mutual Information is given in the "
		"<a href=\"https://itk.org/ItkSoftwareGuide.pdf\">ITK Software Guide</a> in the sections '3.10.4 Mutual "
		"Information Metric' (pp. 262-264) and '5.3.2 Information Theory' (pp. 462-471) </p>");

	QSettings settings;
	smMeanSquares = settings.value("Filters/Similarity/smMeanSquares").toBool();
	smNormalizedCorrelation = settings.value("Filters/Similarity/msNormalizedCorrelation").toBool();
	smMutualInformation = settings.value("Filters/Similarity/smMutualInformation").toBool();
	smMIHistogramBins = settings.value("Filters/Similarity/smMIHistogramBins").toInt();

	//set parameters
	QStringList inList = (QStringList() << tr("$Means Squares") << tr("$Normalized Correlation") 
		<< tr("$Mutual Information (MI)") << tr("*    MI Histogram Bins") );
	QList<QVariant> inPara = (QList<QVariant>()
		<< (smMeanSquares ? tr("true") : tr("false"))
		<< (smNormalizedCorrelation ? tr("true") : tr("false"))
		<< (smMutualInformation ? tr("true") : tr("false"))
		<< tr("%1").arg(smMIHistogramBins));
	dlg_commoninput *dlg = new dlg_commoninput(m_mainWnd, "Similarity Metrics", inList, inPara, fDescr);
	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}
	smMeanSquares = dlg->getCheckValue(0);
	smNormalizedCorrelation = dlg->getCheckValue(1);
	smMutualInformation = dlg->getCheckValue(2);
	smMIHistogramBins = dlg->getIntValue(3);

	settings.setValue("Filters/Similarity/smMeanSquares", smMeanSquares);
	settings.setValue("Filters/Similarity/msNormalizedCorrelation", smNormalizedCorrelation);
	settings.setValue("Filters/Similarity/smMutualInformation", smMutualInformation);
	settings.setValue("Filters/Similarity/smMIHistogramBins", smMIHistogramBins);

	MdiChild *child2 = GetSecondNonActiveChild();
	if (!child2)
		return;

	//prepare (in active mdiChild)
	QString filterName = "Image Similarity Metrics";
	m_mdiChild = m_mainWnd->activeMdiChild();
	m_mdiChild->addStatusMsg(filterName);

	//execute
	iASimilarity * thread = new iASimilarity(filterName, SIMILARITY_METRICS,
		m_mdiChild->getImagePointer(), NULL, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->setSMParameters(smMeanSquares, smNormalizedCorrelation, smMutualInformation, 
		smMIHistogramBins, child2->getImageData());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}
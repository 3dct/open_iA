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
#include "iASegmentationModuleInterface.h"

#include "iAFuzzyCMeans.h"
#include "iASVMImageFilter.h"
#include "iAThresholding.h"
#include "iAWatershedSegmentation.h"

#include "dlg_commoninput.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilterRegistry.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <itkLabelOverlapMeasuresImageFilter.h>

#include <vtkImageData.h>

#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSettings>
#include <QCheckBox>


void iASegmentationModuleInterface::Initialize()
{
	REGISTER_FILTER(iABinaryThreshold);
	REGISTER_FILTER(iAOtsuThreshold);
	REGISTER_FILTER(iAOtsuMultipleThreshold);
	REGISTER_FILTER(iAMaximumDistance);
	REGISTER_FILTER(iARatsThreshold);
	REGISTER_FILTER(iAAdaptiveOtsuThreshold);

	REGISTER_FILTER(iAWatershed);
	REGISTER_FILTER(iAMorphologicalWatershed);

	REGISTER_FILTER(iAFCMFilter);
	REGISTER_FILTER(iAKFCMFilter);
	REGISTER_FILTER(iAMSKFCMFilter);

	REGISTER_FILTER(iASVMImageFilter);
	REGISTER_FILTER(iAKMeans);
	
	if (!m_mainWnd)
		return;

	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSegmentation = getMenuWithTitle(filtersMenu, QString("Segmentation"));
	QAction * actionSegmMetric = new QAction(QApplication::translate("MainWindow", "Quality Metrics to Console", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuSegmentation, actionSegmMetric, true);
	connect(actionSegmMetric, SIGNAL(triggered()), this, SLOT(CalculateSegmentationMetrics()));
}


template <typename ImagePixelType>
void CalculateSegmentationMetrics_template(iAConnector & groundTruthCon, iAConnector & segmentedCon, ImagePixelType)
{
	typedef itk::Image<ImagePixelType, 3> ImageType;
	typedef typename ImageType::Pointer ImagePointer;
	typedef itk::LabelOverlapMeasuresImageFilter<ImageType > FilterType;
	typename FilterType::Pointer filter = FilterType::New();
	ImagePointer groundTruthPtr = dynamic_cast<ImageType*>(groundTruthCon.GetITKImage());
	ImagePointer segmentedPtr = dynamic_cast<ImageType*>(segmentedCon.GetITKImage());
	assert(groundTruthPtr);
	assert(segmentedPtr);
	filter->SetSourceImage(groundTruthPtr);
	filter->SetTargetImage(segmentedPtr);
	filter->Update();

	DEBUG_LOG("************ All Labels *************");
	DEBUG_LOG(" \t Total \t Union (jaccard) \t Mean (dice) \t Volume sim. \t False negative \t False positive");
	DEBUG_LOG(QString(" \t %1 \t %2 \t  %3 \t  %4 \t  %5 \t  %6")
		.arg(filter->GetTotalOverlap())
		.arg(filter->GetUnionOverlap())
		.arg(filter->GetMeanOverlap())
		.arg(filter->GetVolumeSimilarity())
		.arg(filter->GetFalseNegativeError())
		.arg(filter->GetFalsePositiveError()));

	DEBUG_LOG("************ Individual Labels *************");
	DEBUG_LOG("Label \t Target \t Union (jaccard) \t Mean (dice) \t Volume sim. \t False negative \t False positive");

	typename FilterType::MapType labelMap = filter->GetLabelSetMeasures();
	typename FilterType::MapType::const_iterator it;
	for (it = labelMap.begin(); it != labelMap.end(); ++it)
	{
		if ((*it).first == 0)
		{
			continue;
		}
		int label = (*it).first;
		DEBUG_LOG(QString(" \t %1 \t %2 \t  %3 \t  %4 \t  %5 \t  %6")
			.arg(label)
			.arg(filter->GetTargetOverlap(label))
			.arg(filter->GetUnionOverlap(label))
			.arg(filter->GetMeanOverlap(label))
			.arg(filter->GetVolumeSimilarity(label))
			.arg(filter->GetFalseNegativeError(label))
			.arg(filter->GetFalsePositiveError(label)));
	}
}

bool iASegmentationModuleInterface::CalculateSegmentationMetrics()
{
	QList<MdiChild *> mdiwindows = m_mainWnd->MdiChildList();
	if (mdiwindows.size() < 2) {
		QMessageBox::warning(m_mainWnd, tr("Segmentation Quality Metric"),
			tr("This operation requires at least two datasets to be loaded, "
				"one ground truth and the segmentation result to be evaluated. "
				"Currently there are %1 windows open.")
			.arg(mdiwindows.size()));
		return false;
	}
	QStringList inList = (QStringList()
		<< tr("+Ground Truth")
		<< tr("+Segmented image"));
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(
		"<p><font size=+1>Calculate segmentation quality metrics.</font></p>"
		"<p>Select which image to use as ground truth</p>");
	QList<QVariant> inPara;
	QStringList list;
	QString::SplitBehavior behavior = QString::SplitBehavior::SkipEmptyParts;
	for (MdiChild* mdiChild: mdiwindows)
	{
		QString fileName = mdiChild->currentFile();
		if (!fileName.isEmpty())
		{
			list << fileName.split("/", behavior).last();
		}
		else
		{
			list << mdiChild->windowTitle();
		}
	}
	inPara << list << list;
	dlg_commoninput dlg(m_mainWnd, "Segmentation Quality Metric", inList, inPara, fDescr, true);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	if (dlg.getComboBoxIndex(0) == dlg.getComboBoxIndex(1))
	{
		QMessageBox::warning(m_mainWnd, tr("Segmentation Quality Metric"),
			tr("Same file selected for both ground truth and segmented image!"));
	}
	vtkImageData * groundTruthVTK = mdiwindows[dlg.getComboBoxIndex(0)]->getImageData();
	vtkImageData * segmentedVTK = mdiwindows[dlg.getComboBoxIndex(1)]->getImageData();
	iAConnector groundTruthCon;
	groundTruthCon.SetImage(groundTruthVTK);
	iAConnector segmentedCon;
	segmentedCon.SetImage(segmentedVTK);
	if (groundTruthCon.GetITKScalarPixelType() != segmentedCon.GetITKScalarPixelType())
	{
		// TODO: cast image!
		QMessageBox::warning(m_mainWnd, tr("Segmentation Quality Metric"),
			tr("The label images need to have the same type; ground truth type: %1; segmented type: %2")
			.arg(groundTruthCon.GetITKScalarPixelType())
			.arg(segmentedCon.GetITKScalarPixelType()));
		return false;
	}
	try
	{
		switch (groundTruthCon.GetITKScalarPixelType()) // This filter handles all types
		{

		case itk::ImageIOBase::UCHAR:  CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<unsigned char>(0)); break;
		case itk::ImageIOBase::CHAR:   CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<char>(0)); break;
		case itk::ImageIOBase::USHORT: CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<unsigned short>(0)); break;
		case itk::ImageIOBase::SHORT:  CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<short>(0)); break;
		case itk::ImageIOBase::UINT:   CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<unsigned int>(0)); break;
		case itk::ImageIOBase::INT:    CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<int>(0)); break;
		case itk::ImageIOBase::ULONG:  CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<unsigned long>(0)); break;
		case itk::ImageIOBase::LONG:   CalculateSegmentationMetrics_template(groundTruthCon, segmentedCon, static_cast<long>(0)); break;
		case itk::ImageIOBase::FLOAT:
		case itk::ImageIOBase::DOUBLE:
		case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
		default:
			DEBUG_LOG("Unknown/Invalid component type.");
			return false;
		}
	}
	catch (itk::ExceptionObject &e)
	{
		DEBUG_LOG(QString("Segmentation Metric calculation terminated unexpectedly: %1.").arg(e.what()));
		return false;
	}
	return true;
}

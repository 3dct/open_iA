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
#include "iAThresholding.h"
#include "iAWatershedSegmentation.h"

#include "dlg_commoninput.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilterRunner.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <itkLabelOverlapMeasuresImageFilter.h>

#include <vtkImageAccumulate.h>
#include <vtkImageData.h>

#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSettings>
#include <QCheckBox>

void iASegmentationModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSegmentation = getMenuWithTitle(filtersMenu, QString( "Segmentation" ) );
	QMenu * menuGlobalThresholding = getMenuWithTitle(menuSegmentation, QString("Global Thresholding"));
	QMenu * menuLocalThresholding = getMenuWithTitle(menuSegmentation, QString("Local Thresholding"));
	QMenu * menuWatershed = getMenuWithTitle( menuSegmentation, QString( "Based on Watershed" ) );
	QMenu * menuFuzzyCMeans = getMenuWithTitle( menuSegmentation, QString( "Fuzzy C-Means" ) );

	menuSegmentation->addAction(menuGlobalThresholding->menuAction() );
	menuSegmentation->addAction(menuLocalThresholding->menuAction());
	menuSegmentation->addAction(menuWatershed->menuAction() );
	menuSegmentation->addAction(menuFuzzyCMeans->menuAction() );

	// global thresholding
	QAction * actionBinary_threshold_filter = new QAction(QApplication::translate("MainWindow", "Binary threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuGlobalThresholding, actionBinary_threshold_filter);
	connect(actionBinary_threshold_filter, SIGNAL(triggered()), this, SLOT(binary_threshold()));

	QAction * actionOtsu_threshold_filter = new QAction(QApplication::translate("MainWindow", "Otsu threshold filter", 0), m_mainWnd );
	AddActionToMenuAlphabeticallySorted(menuGlobalThresholding, actionOtsu_threshold_filter );
	connect( actionOtsu_threshold_filter, SIGNAL( triggered() ), this, SLOT( otsu_Threshold_Filter() ) );

	QAction * actionOtsu_Multiple_Threshold_Filter = new QAction(QApplication::translate("MainWindow", "Otsu multiple threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuGlobalThresholding, actionOtsu_Multiple_Threshold_Filter );
	connect( actionOtsu_Multiple_Threshold_Filter, SIGNAL( triggered() ), this, SLOT( otsu_Multiple_Threshold_Filter() ) );

	QAction * actionMaximum_distance_filter = new QAction(QApplication::translate("MainWindow", "Maximum distance filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuGlobalThresholding, actionMaximum_distance_filter );
	connect( actionMaximum_distance_filter, SIGNAL( triggered() ), this, SLOT( maximum_Distance_Filter() ) );

	QAction * actionRats_threshold_filter = new QAction(QApplication::translate("MainWindow", "Rats threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuGlobalThresholding, actionRats_threshold_filter);
	connect(actionRats_threshold_filter, SIGNAL(triggered()), this, SLOT(rats_Threshold_Filter()));

	QAction * actionAdaptive_otsu_threshold_filter = new QAction(QApplication::translate("MainWindow", "Adaptive Otsu threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuLocalThresholding, actionAdaptive_otsu_threshold_filter );
	connect( actionAdaptive_otsu_threshold_filter, SIGNAL( triggered() ), this, SLOT(adaptive_Otsu_Threshold_Filter() ) );

	// watershed-based
	QAction * actionWatershed = new QAction(QApplication::translate("MainWindow", "Watershed Segmentation Filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuWatershed, actionWatershed );
	connect( actionWatershed, SIGNAL( triggered() ), this, SLOT( watershed_seg() ) );

	QAction * actionMorphologicalWatershed = new QAction(QApplication::translate("MainWindow", "Morphological Watershed Segmentation Filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuWatershed, actionMorphologicalWatershed );
	connect( actionMorphologicalWatershed, SIGNAL( triggered() ), this, SLOT( morph_watershed_seg() ) );

	QAction * actionSegmMetric = new QAction(QApplication::translate("MainWindow", "Quality Metrics to Console", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuSegmentation, actionSegmMetric, true);
	connect(actionSegmMetric, SIGNAL(triggered()), this, SLOT(CalculateSegmentationMetrics()));
	
	// fuzzy c-means
	QAction * actionFuzzyCMeans = new QAction(QApplication::translate("MainWindow", "Fuzzy C-Means", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuFuzzyCMeans, actionFuzzyCMeans );
	connect( actionFuzzyCMeans, SIGNAL( triggered() ), this, SLOT( fcm_seg() ) );
	
	QAction * actionKFCM = new QAction(QApplication::translate("MainWindow", "Kernelized FCM", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuFuzzyCMeans, actionKFCM);
	connect(actionKFCM, SIGNAL( triggered() ), this, SLOT( kfcm_seg() ) );

	QAction * actionMSKFCM = new QAction(QApplication::translate("MainWindow", "MSKFCM", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuFuzzyCMeans, actionMSKFCM);
	connect(actionMSKFCM, SIGNAL(triggered()), this, SLOT(mskfcm_seg()));
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
	QList<QMdiSubWindow *> mdiwindows = m_mainWnd->MdiChildList();
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
	for (int i = 0; i<mdiwindows.size(); ++i)
	{
		MdiChild* mdiChild = qobject_cast<MdiChild *>(mdiwindows[i]->widget());
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
	vtkImageData * groundTruthVTK = qobject_cast<MdiChild *>(mdiwindows[dlg.getComboBoxIndex(0)]->widget())->getImageData();
	vtkImageData * segmentedVTK = qobject_cast<MdiChild *>(mdiwindows[dlg.getComboBoxIndex(1)]->widget())->getImageData();
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

void iASegmentationModuleInterface::binary_threshold()
{
	RunFilter(iABinaryThreshold::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::otsu_Threshold_Filter()
{
	RunFilter(iAOtsuThreshold::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::adaptive_Otsu_Threshold_Filter()
{
	RunFilter(iAAdaptiveOtsuThreshold::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::rats_Threshold_Filter()
{
	RunFilter(iARatsThreshold::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::otsu_Multiple_Threshold_Filter()
{
	RunFilter(iAOtsuMultipleThreshold::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::maximum_Distance_Filter()
{
	RunFilter(iAMaximumDistance::Create(), m_mainWnd);
}

void iASegmentationModuleInterface::watershed_seg()
{
	QStringList inList = (QStringList()
		<< tr( "#Level" )
		<< tr( "#Threshold" ));
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( wsLevel )
		<< tr( "%1" ).arg( wsThreshold );

	dlg_commoninput dlg( m_mainWnd, "Watershed segmentation", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;

	wsLevel = dlg.getDblValue(0);
	wsThreshold = dlg.getDblValue(1);

	QString filterName = "Watershed segmentation";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAWatershedSegmentation* thread = new iAWatershedSegmentation( filterName, WATERSHED,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setWParameters( wsLevel, wsThreshold );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::morph_watershed_seg()
{
	QSettings settings;
	mwsLevel = settings.value( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsLevel" ).toDouble();
	mwsMarkWSLines = settings.value( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsMarkWSLines" ).toBool();
	mwsFullyConnected = settings.value( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsFullyConnected" ).toBool();
	QTextDocument *fDescr = new QTextDocument( 0 );
	fDescr->setHtml(
		"<p><font size=+1>Calculates the Morphological Watershed Transformation.</font></p>"
		"<p>For further details see: http://www.insight-journal.org/browse/publication/92Select <br>"
		"Note 1: As input image use e.g., a gradient magnitude image.<br>"
		"Note 2: Mark WS Lines label whatershed lines with 0, background with 1. )</p>"
		);
	QStringList inList = ( QStringList()
		<< tr( "#Level" )
		<< tr( "$Mark WS Lines" )
		<< tr( "$Fully Connected" ) );
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( mwsLevel )
		<< tr( "%1" ).arg( mwsMarkWSLines )
		<< tr( "%1" ).arg( mwsFullyConnected );
	dlg_commoninput dlg( m_mainWnd, "Morphological Watershed Segmentation", inList, inPara, fDescr );
	if ( dlg.exec() != QDialog::Accepted )
		return;
		
	mwsLevel = dlg.getDblValue(0);
	mwsMarkWSLines = dlg.getCheckValue(1);
	mwsFullyConnected = dlg.getCheckValue(2);
	
	settings.setValue( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsLevel", mwsLevel );
	settings.setValue( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsMarkWSLines", mwsMarkWSLines );
	settings.setValue( "Filters/Segmentation/MorphologicalWatershedSegmentation/mwsFullyConnected", mwsFullyConnected );

	QString filterName = "Morphological watershed segmentation";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAWatershedSegmentation* thread = new iAWatershedSegmentation( filterName, MORPH_WATERSHED,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setMWSParameters( mwsLevel, mwsMarkWSLines, mwsFullyConnected );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::FuzzyCMeansFinished()
{
	iAFilterRunner* thread = dynamic_cast<iAFilterRunner*>(sender());
	iAProbabilitySource* probSource = m_probSources[thread];
	if (!thread || !probSource)
	{
		DEBUG_LOG("Invalid FCM finished call!");
		return;
	}
	auto & probs = probSource->Probabilities();
	for (int p = 0; p < probs.size(); ++p)
	{
		m_mdiChild->GetModalities()->Add(QSharedPointer<iAModality>(
			new iAModality(QString("FCM Prob. %1").arg(p), "", -1, probs[p], 0)));
	}
	m_probSources.remove(thread);
}

void iASegmentationModuleInterface::StartFCMThread(QSharedPointer<iAFilter> filter)
{
	iAFilterRunner* thread = RunFilter(filter, m_mainWnd);
	if (!thread)
		return;

	m_probSources.insert(thread, dynamic_cast<iAProbabilitySource*>(filter.data()));
	m_mdiChild = qobject_cast<MdiChild*>(thread->parent());
	connect(thread, SIGNAL(workDone()), this, SLOT(FuzzyCMeansFinished()));
}

void iASegmentationModuleInterface::fcm_seg()
{
	StartFCMThread(iAFCMFilter::Create());
}

void iASegmentationModuleInterface::kfcm_seg()
{
	StartFCMThread(iAKFCMFilter::Create());
}

void iASegmentationModuleInterface::mskfcm_seg()
{
	StartFCMThread(iAMSKFCMFilter::Create());
}

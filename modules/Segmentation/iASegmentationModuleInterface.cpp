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
#include "iAMaximumDistance.h"
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

	// local thresholding
	QAction * actionAdaptive_otsu_threshold_filter = new QAction(QApplication::translate("MainWindow", "Adaptive Otsu threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuLocalThresholding, actionAdaptive_otsu_threshold_filter );
	connect( actionAdaptive_otsu_threshold_filter, SIGNAL( triggered() ), this, SLOT(adaptive_Otsu_Threshold_Filter() ) );

	QAction * actionRats_threshold_filter = new QAction(QApplication::translate("MainWindow", "Rats threshold filter", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuLocalThresholding, actionRats_threshold_filter );
	connect( actionRats_threshold_filter, SIGNAL( triggered() ), this, SLOT( rats_Threshold_Filter() ) );

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
	QSettings settings;
	btlower = settings.value("Filters/Segmentation/BinaryThresholding/btlower").toDouble();
	btupper = settings.value("Filters/Segmentation/BinaryThresholding/btupper").toDouble();
	btoutside = settings.value("Filters/Segmentation/BinaryThresholding/btoutside").toDouble();
	btinside = settings.value("Filters/Segmentation/BinaryThresholding/btinside").toDouble();

	QStringList inList = (QStringList()
		<< tr("#Lower Threshold")
		<< tr("#Upper Threshold")
		<< tr("#Outside Value")
		<< tr("#Inside Value"));
	QList<QVariant> inPara; inPara
		<< tr("%1").arg(btlower)
		<< tr("%1").arg(btupper)
		<< tr("%1").arg(btoutside)
		<< tr("%1").arg(btinside);
	dlg_commoninput dlg(m_mainWnd, "Binary Threshold", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	btlower = dlg.getDblValue(0);
	btupper = dlg.getDblValue(1);
	btoutside = dlg.getDblValue(2);
	btinside = dlg.getDblValue(3);

	settings.setValue("Filters/Segmentation/BinaryThresholding/btlower", btlower);
	settings.setValue("Filters/Segmentation/BinaryThresholding/btupper", btupper);
	settings.setValue("Filters/Segmentation/BinaryThresholding/btoutside", btoutside);
	settings.setValue("Filters/Segmentation/BinaryThresholding/btinside", btinside);

	QString filterName = "Binary threshold";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	iAThresholding * thread = new iAThresholding(filterName, BINARY_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->setBTParameters(btlower, btupper, btoutside, btinside);
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}

void iASegmentationModuleInterface::otsu_Threshold_Filter()
{
	QSettings settings;
	otBins = settings.value( "Filters/Segmentation/Otsu/otBins" ).toDouble();
	otoutside = settings.value( "Filters/Segmentation/Otsu/otoutside" ).toDouble();
	otinside = settings.value( "Filters/Segmentation/Otsu/otinside" ).toDouble();
	otremovepeaks = settings.value( "Filters/Segmentation/Otsu/otremovepeaks" ).toBool();

	QStringList inList = (QStringList()
		<< tr( "#Number of Histogram Bins" )
		<< tr( "#Outside Value" )
		<< tr( "#Inside Value" )
		<< tr( "$Remove Peaks" ));
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( otBins )
		<< tr( "%1" ).arg( otoutside )
		<< tr( "%1" ).arg( otinside )
		<< (otremovepeaks ? tr( "true" ) : tr( "false" ));
	dlg_commoninput dlg( m_mainWnd, "Otsu Threshold", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;

	otBins = dlg.getDblValue(0);
	otoutside = dlg.getDblValue(1);
	otinside = dlg.getDblValue(2);
	otremovepeaks = dlg.getCheckValue(3);

	settings.setValue( "Filters/Segmentation/Otsu/otBins", otBins );
	settings.setValue( "Filters/Segmentation/Otsu/otoutside", otoutside );
	settings.setValue( "Filters/Segmentation/Otsu/otinside", otinside );
	settings.setValue( "Filters/Segmentation/Otsu/otremovepeaks", otremovepeaks );

	QString filterName = "Otsu threshold";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAThresholding* thread = new iAThresholding( filterName, OTSU_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setOTParameters( otBins, otoutside, otinside, otremovepeaks );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::maximum_Distance_Filter()
{
	QStringList inList = (QStringList()
		<< tr( "#Number of Intensity" )
		<< tr( "#Low Intensity" )
		<< tr( "$Use Low Intensity" ));
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( mdfbins )
		<< tr( "%1" ).arg( mdfli )
		<< tr( "%1" ).arg( mdfuli );
	dlg_commoninput dlg( m_mainWnd, "Maximum Distance Filter", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;

	mdfbins = dlg.getDblValue(0);
	mdfli = dlg.getDblValue(1);
	mdfuli = dlg.getCheckValue(2);

	QString filterName = "Maximum distance";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAMaximumDistance* thread = new iAMaximumDistance( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setMDFParameters( mdfli, mdfbins, mdfuli );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
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

void iASegmentationModuleInterface::adaptive_Otsu_Threshold_Filter()
{
	QStringList inList = (QStringList()
		<< tr( "#Number of Histogram Bins" )
		<< tr( "#Outside Value" )
		<< tr( "#Inside Value" )
		<< tr( "#Radius" )
		<< tr( "#Samples" )
		<< tr( "#Levels" )
		<< tr( "#Control Points" ));
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( aotBins )
		<< tr( "%1" ).arg( aotOutside )
		<< tr( "%1" ).arg( aotInside )
		<< tr( "%1" ).arg( aotRadius )
		<< tr( "%1" ).arg( aotSamples )
		<< tr( "%1" ).arg( aotLevels )
		<< tr( "%1" ).arg( aotControlpoints );
	dlg_commoninput dlg( m_mainWnd, "Adaptive otsu threshold", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;

	aotBins = dlg.getDblValue(0);
	aotOutside = dlg.getDblValue(1);
	aotInside = dlg.getDblValue(2);
	aotRadius = dlg.getDblValue(3);
	aotSamples = dlg.getDblValue(4);
	aotLevels = dlg.getDblValue(5);
	aotControlpoints = dlg.getDblValue(6);

	QString filterName = "Adaptive otsu threshold";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAThresholding* thread = new iAThresholding( filterName, ADAPTIVE_OTSU_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setAOTParameters( aotRadius, aotSamples, aotLevels, aotControlpoints, aotBins, aotOutside, aotInside );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::rats_Threshold_Filter()
{
	QStringList inList = (QStringList()
		<< tr( "#Power" )
		<< tr( "#Outside Value" )
		<< tr( "#Inside Value" ));
	QList<QVariant> inPara; inPara
		<< tr( "%1" ).arg( rtPow )
		<< tr( "%1" ).arg( rtOutside )
		<< tr( "%1" ).arg( rtInside );
	dlg_commoninput dlg( m_mainWnd, "Rats Threshold", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	
	rtPow = dlg.getDblValue(0);
	rtOutside = dlg.getDblValue(1);
	rtInside = dlg.getDblValue(2);

	QString filterName = "Rats threshold filter";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAThresholding* thread = new iAThresholding( filterName, RATS_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setRTParameters( rtPow, rtOutside, rtInside );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::otsu_Multiple_Threshold_Filter()
{
	QStringList inList = ( QStringList()
		<< tr( "#Number of Histogram Bins" )
		<< tr( "#Number of Thresholds" )
		<< tr( "$Valley Emphasis" ) );
	QList<QVariant> inPara;	inPara
		<< tr( "%1" ).arg( omtBins )
		<< tr( "%1" ).arg( omtThreshs )
		<< ( omtVe ? tr( "true" ) : tr( "false" ) );
	dlg_commoninput dlg( m_mainWnd, "Otsu Multiple Thresholds", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;

	omtBins = dlg.getDblValue(0);
	omtThreshs = dlg.getDblValue(1);
	omtVe = dlg.getCheckValue(2);

	QString filterName = "Otsu multiple threshold";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	iAThresholding* thread = new iAThresholding( filterName, OTSU_MULTIPLE_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setOMTParameters( omtBins, omtThreshs, omtVe );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iASegmentationModuleInterface::FuzzyCMeansFinished()
{
	if (!m_probSource)
	{
		DEBUG_LOG("Fuzzy not set!");
		return;
	}
	auto & probs = m_probSource->Probabilities();
	for (int p = 0; p < probs.size(); ++p)
	{
		m_mdiChild->GetModalities()->Add(QSharedPointer<iAModality>(
			new iAModality(QString("FCM Prob. %1").arg(p), "", -1, probs[p], 0)));
	}
}

void iASegmentationModuleInterface::StartFCMThread(QSharedPointer<iAFilter> filter)
{
	m_probSource = dynamic_cast<iAProbabilitySource*>(filter.data());
	iAFilterRunner* thread = RunFilter(filter, m_mainWnd);
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

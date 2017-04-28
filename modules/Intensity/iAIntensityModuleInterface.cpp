/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAIntensityModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAIntensity.h"
#include "iASubtractImageFilter.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QSettings>

void iAIntensityModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuIntensity = getMenuWithTitle(filtersMenu, QString( "Intensity" ) );
	QAction * actionDifference = new QAction(QApplication::translate("MainWindow", "Difference", 0), m_mainWnd );
	QAction * actionSubtractImage_Filter = new QAction(QApplication::translate("MainWindow", "Subtract Images", 0), m_mainWnd );
	QAction * actionInvertIntensity = new QAction(QApplication::translate("MainWindow", "Invert Intensities", 0), m_mainWnd );
	QAction * actionMaskImage = new QAction(QApplication::translate("MainWindow", "Mask Image", 0), m_mainWnd );
	QAction * actionIntensityWindowing = new QAction(QApplication::translate("MainWindow", "Intensity Windowing", 0), m_mainWnd );
	QAction * actionNormalizeImage = new QAction( QApplication::translate( "MainWindow", "Normalize Image", 0 ), m_mainWnd );
	QAction * actionHistogramMatch = new QAction( QApplication::translate( "MainWindow", "Histogram Match", 0 ), m_mainWnd );
	menuIntensity->addAction( actionDifference );
	menuIntensity->addAction( actionSubtractImage_Filter );
	menuIntensity->addAction( actionInvertIntensity );
	menuIntensity->addAction( actionMaskImage );
	menuIntensity->addAction( actionIntensityWindowing );
	menuIntensity->addAction( actionNormalizeImage );
	menuIntensity->addAction( actionHistogramMatch );
	connect( actionDifference, SIGNAL( triggered() ), this, SLOT( difference_Image_Filter() ) );
	connect( actionSubtractImage_Filter, SIGNAL( triggered() ), this, SLOT( subtractimage_Filter() ) );
	connect( actionInvertIntensity, SIGNAL( triggered() ), this, SLOT( invert_intensity() ) );
	connect( actionMaskImage, SIGNAL( triggered() ), this, SLOT( mask() ) );
	connect( actionIntensityWindowing, SIGNAL( triggered() ), this, SLOT( intensity_windowing() ) );
	connect( actionNormalizeImage, SIGNAL( triggered() ), this, SLOT( normalize_image() ) );
	connect( actionHistogramMatch, SIGNAL( triggered() ), this, SLOT( histogram_match() ) );
}

void iAIntensityModuleInterface::difference_Image_Filter()
{
	//set parameters
	QStringList inList = (QStringList()
		<< tr( "#Difference threshold" ) << tr( "#Tolerance radius" ));
	QList<QVariant> inPara;
	inPara << tr( "%1" ).arg( difDifferenceThreshold ) << tr( "%1" ).arg( difToleranceRadius );

	dlg_commoninput dlg( m_mainWnd, "Difference Image Filter", 2, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;

	difDifferenceThreshold = dlg.getValues()[0];
	difToleranceRadius = dlg.getValues()[1];

	MdiChild *child2 = GetSecondNonActiveChild();
	if (!child2)
	{
		return;
	}
	//prepare
	QString filterName = "Difference Image between " + child2->windowTitle() + " and";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, DIFFERENCE_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDIFParameters( difDifferenceThreshold, difToleranceRadius, child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::subtractimage_Filter()
{
	//set parameters
	MdiChild *child2 = GetSecondNonActiveChild();
	if (!child2)
	{
		return;
	}
	//prepare
	QString filterName = "Subtract Image " + child2->windowTitle() + " from";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iASubtractImageFilter* thread = new iASubtractImageFilter( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setSIParameters( child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::invert_intensity()
{
	//prepare
	QString filterName = "Invert Intensity";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity* thread = new iAIntensity( filterName, INVERT_INTENSITY,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::mask()
{
	MdiChild *child2 = GetSecondNonActiveChild();
	if ( !child2 )
	{
		return;
	}
	//prepare
	QString filterName = "Mask Image " + child2->windowTitle();
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, MASK_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDIFParameters( 0, 0, child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::intensity_windowing()
{
	//set parameters
	QStringList inList = ( QStringList()
		 << tr( "#WindowMinimum" ) << tr( "#WindowMaximum" ) << tr( "#OutputMinimum" ) << tr( "#OutputMaximum" ) );
	QList<QVariant> inPara;
	inPara << tr( "%1" ).arg( windowMinimum ) << tr( "%1" ).arg( windowMaximum ) << tr( "%1" ).arg( outputMinimum ) << tr( "%1" ).arg( outputMaximum );
	dlg_commoninput dlg( m_mainWnd, "Intensity Windowing Image Filter", 4, inList, inPara, NULL );
	if ( dlg.exec() != QDialog::Accepted )
		return;

	windowMinimum = dlg.getValues()[0];
	windowMaximum = dlg.getValues()[1];
	outputMinimum = dlg.getValues()[2];
	outputMaximum = dlg.getValues()[3];

	//prepare
	QString filterName = "Intensity Windowing";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, INTENSITY_WINDOWING,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setWIIFParameters( windowMinimum, windowMaximum, outputMinimum, outputMaximum );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::normalize_image()
{
	//set filter description
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml(
		"<p>Normalizes an image by setting its mean to zero and variance to one."
		"The Normalize Image Filter shifts and scales an image so that the pixels in the image "
		"have a zero mean and unit variance. NB: since this filter normalizes the data to lie within -1 to "
		"1, integral types will produce an image that DOES NOT HAVE a unit variance.</p>"
		"<p>https://itk.org/Doxygen/html/classitk_1_1NormalizeImageFilter.html</p>");

	QStringList inList; QList<QVariant> inPara; 	
	dlg_commoninput dlg( m_mainWnd, "Normalize Image Filter", 0, inList, inPara, fDescr );
	if ( dlg.exec() != QDialog::Accepted )
		return;

	//prepare
	QString filterName = "Normalize Image";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, NORMALIZE_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::histogram_match()
{	
	//set filter description
	QTextDocument *fDescr = new QTextDocument();
	fDescr->setHtml(
		"<p>Normalizes the grayscale values between two images by histogram matching."
		"The Histogram Matching Image Filter normalizes the grayscale values of a source "
		"image based on the grayscale values of a reference image. This filter uses a histogram "
		"matching technique where the histograms of the two images are matched only at a specified "
		"number of quantile values.</p>"
		"<p>This filter was originally designed to normalize MR images of the same MR protocol and "
		"same body part. The algorithm works best if background pixels are excluded from both the "
		"source and reference histograms. A simple background exclusion method is to exclude all "
		"pixels whose grayscale values are smaller than the mean grayscale value. "
		"ThresholdAtMeanIntensityOn() switches on this simple background exclusion method.</p>"
		"<p>SetNumberOfHistogramLevels() sets the number of bins used when creating histograms of the "
		"source and reference images. SetNumberOfMatchPoints() governs the number of quantile values "
		"to be matched.</p>"
		"<p>This filter assumes that both the source and reference are of the same type and that the "
		"input and output image type have the same number of dimension and have scalar pixel types.</p>"
		"<p>https://itk.org/Doxygen/html/classitk_1_1HistogramMatchingImageFilter.html</p>" );

	QSettings settings;
	hmHistogramLevels = settings.value( "Filters/Intensity/hmHistogramLevels" ).toInt();
	hmMatchPoints = settings.value( "Filters/Intensity/hmMatchPoints" ).toInt();
	hmThresholdAtMeanIntensity = settings.value( "Filters/Intensity/hmThresholdAtMeanIntensity" ).toBool();
	
	//set parameters
	QStringList inList = ( QStringList()
		<< tr( "#Histogram Levels" ) << tr( "#Match Points" ) << tr( "$ThresholdAtMeanIntensity" ) );
	QList<QVariant> inPara;
	inPara << tr( "%1" ).arg( hmHistogramLevels ) << tr( "%1" ).arg( hmMatchPoints ) << tr( "%1" ).arg( hmThresholdAtMeanIntensity);
	dlg_commoninput dlg( m_mainWnd, "Histogram Matching Image Filter", 3, inList, inPara, fDescr );
	if ( dlg.exec() != QDialog::Accepted )
		return;

	hmHistogramLevels = dlg.getValues()[0]; hmMatchPoints = dlg.getValues()[1]; hmThresholdAtMeanIntensity = dlg.getCheckValues()[2];

	settings.setValue( "Filters/Intensity/hmHistogramLevels", hmHistogramLevels );
	settings.setValue( "Filters/Intensity/hmMatchPoints", hmMatchPoints );
	settings.setValue( "Filters/Intensity/hmThresholdAtMeanIntensity", hmThresholdAtMeanIntensity );

	MdiChild *child2 = GetSecondNonActiveChild();
	if ( !child2 )
		return;
	
	//prepare
	QString filterName = "Histogram Macth";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, HISTOGRAM_MATCH,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setHMFParameters( hmHistogramLevels, hmMatchPoints, hmThresholdAtMeanIntensity, child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
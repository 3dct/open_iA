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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"

#include "iADragFilterWidget.h"
#include "PorosityAnalyserHelpers.h"

#include <QtWidgets>
#include <QStringList>

const QStringList customMimeType = QStringList()\
<< "application/x-dnditemdatafilter"\
<< "application/x-dnditemdatadataset";

const int rowStartOffset = 25;
const int maxColumns = 5;
const int maxRows = 7;
const int iconHeight = 60;
const int iconWidth = 60;
const int columnGutter = iconWidth + 7;
const int rowGutter = iconHeight + 10;
const int nbOfNPFilters = 5;
const int nbOfPFilters = 5;
const int nbOfRGFilters = 3;
const int nbOfSMFilters = 5;

iADragFilterWidget::iADragFilterWidget( QString datasetDir, QStringList datasetList, int d_f_switch, QWidget *parent )
	: QFrame( parent ), m_d_f_switch( d_f_switch ), m_datasetDir( datasetDir ), m_datasetList( datasetList )
{
	setStyleSheet( "border: none" );
	setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	setAcceptDrops( true );
	setFixedSize( maxColumns * columnGutter, (maxRows-1) * rowGutter + maxRows * rowStartOffset );
	QFont font( "Arial", 11, QFont::Black, false );

	if ( m_d_f_switch == 1 )	// Datasets
	{
		for ( int i = 0; i < m_datasetList.size(); ++i )
		{
			QLabel *dataset = new QLabel( this );
			dataset->setObjectName( "dataset_" + m_datasetList[i] );
			dataset->setPixmap( mergeOnTop( QPixmap( ":/images/dataset2.png" ), m_datasetList[i] ) );
			dataset->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
			dataset->setToolTip( generateDatasetTooltip( m_datasetList[i] ) );
			dataset->move( 0, ( i + 1 ) * 10 + i*iconHeight );
			dataset->show();
			dataset->setAttribute( Qt::WA_DeleteOnClose );
		}
	}
	else if ( m_d_f_switch == 0 )	// Algorithms
	{
		int	xIdx = 0, yIdx = 0, yGutterIdx = 1;

		// Smoothing Group Label
		QLabel *smFilters = new QLabel( this );
		smFilters->setFont( font );
		smFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		smFilters->setText( "Smoothing filters" );

		smFilters->move( xIdx, yIdx++ );
		smFilters->show();

		QFrame *separatorSM = new QFrame( this );
		separatorSM->setGeometry( QRect( xIdx, yIdx * rowStartOffset - 5, nbOfSMFilters * columnGutter, 1 ) );
		separatorSM->setFrameShape( QFrame::HLine );
		separatorSM->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
									"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorSM->setFrameShadow( QFrame::Sunken );
		separatorSM->show();

		// Smoothing
		QLabel *gadIcon = new QLabel( this );
		gadIcon->setObjectName( "Gradient Anisotropic Diffusion Smoothing" );
		gadIcon->setPixmap( QPixmap( ":/images/smooth_GAD.png" ) );
		gadIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		gadIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		gadIcon->show();
		gadIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *cadIcon = new QLabel( this );
		cadIcon->setObjectName( "Curvature Anisotropic Diffusion Smoothing" );
		cadIcon->setPixmap( QPixmap( ":/images/smooth_CAD.png" ) );
		cadIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		cadIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		cadIcon->show();
		cadIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *gaussIcon = new QLabel( this );
		gaussIcon->setObjectName( "Recursive Gaussian Smoothing" );
		gaussIcon->setPixmap( QPixmap( ":/images/smooth_Gauss.png" ) );
		gaussIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		gaussIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		gaussIcon->show();
		gaussIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *bilIcon = new QLabel( this );
		bilIcon->setObjectName( "Bilateral Smoothing" );
		bilIcon->setPixmap( QPixmap( ":/images/smooth_Bilateral.png" ) );
		bilIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		bilIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		bilIcon->show();
		bilIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *medIcon = new QLabel( this );
		medIcon->setObjectName( "Median Smoothing" );
		medIcon->setPixmap( QPixmap( ":/images/smooth_Median.png" ) );
		medIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		medIcon->setToolTip( "<p>The <b>Median</b> smoothing filter Computes an image where a given pixel is the median "
							 "value of the the pixels in a neighborhood about the corresponding input pixel. A median filter "
							 "is one of the family of nonlinear filters.It is used to smooth an image without being biased "
							 "by outliers or shot noise.</p>"
							 "<p>http://www.itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html</p>" );
		medIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		medIcon->show();
		medIcon->setAttribute( Qt::WA_DeleteOnClose );

		xIdx = 0;

		// Non-parametric Group Label
		QLabel *npSegmFilters = new QLabel( this );
		npSegmFilters->setFont( font );
		npSegmFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		npSegmFilters->setText( "Non-parametric segmentation filters" );
		npSegmFilters->move( columnGutter * xIdx, ++yIdx * rowStartOffset + yGutterIdx * rowGutter );
		npSegmFilters->show();

		QFrame *separatorNP = new QFrame( this );
		separatorNP->setGeometry( QRect( columnGutter * xIdx, ++yIdx * rowStartOffset + yGutterIdx * rowGutter - 5, nbOfPFilters * columnGutter, 1 ) );
		separatorNP->setFrameShape( QFrame::HLine );
		separatorNP->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
									"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorNP->setFrameShadow( QFrame::Sunken );
		separatorNP->show();

		// Non-parametric
		QLabel *otsuIcon = new QLabel( this );
		otsuIcon->setObjectName( "Otsu Threshold" );
		otsuIcon->setPixmap( QPixmap( ":/images/segment_Otsu.png" ) );
		otsuIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
		otsuIcon->setToolTip( "<p>The <b>Otsu Threshold</b> filter creates a binary thresholded image that "
							  "separates an image into foreground and background components.The filter computes the threshold "
							  "using the OtsuThresholdCalculator and applies that theshold to the input image using the BinaryThresholdImageFilter.</p>"
							  "<p>http ://hdl.handle.net/10380/3279 or http://www.insight-journal.org/browse/publication/811 </p>" );
		otsuIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		otsuIcon->show();
		otsuIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *isoDataIcon = new QLabel( this );
		isoDataIcon->setObjectName( "IsoData Threshold" );
		isoDataIcon->setPixmap( QPixmap( ":/images/segment_IsoData.png" ) );
		isoDataIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		isoDataIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		isoDataIcon->show();
		isoDataIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *intermodesIcon = new QLabel( this );
		intermodesIcon->setObjectName( "Intermodes Threshold" );
		intermodesIcon->setPixmap( QPixmap( ":/images/segment_Intermodes.png" ) );
		intermodesIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		intermodesIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		intermodesIcon->show();
		intermodesIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *maxEntropyIcon = new QLabel( this );
		maxEntropyIcon->setObjectName( "MaxEntropy Threshold" );
		maxEntropyIcon->setPixmap( QPixmap( ":/images/segment_MaxEntropy.png" ) );
		maxEntropyIcon->setStyleSheet( "QToolTip{ color: black; background - color: #ffffe1; border: 0px solid white; }" );
		maxEntropyIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		maxEntropyIcon->show();
		maxEntropyIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *minimumIcon = new QLabel( this );
		minimumIcon->setObjectName( "Minimum Threshold" );
		minimumIcon->setPixmap( QPixmap( ":/images/segment_Minimum.png" ) );
		minimumIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		minimumIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		minimumIcon->show();
		minimumIcon->setAttribute( Qt::WA_DeleteOnClose );

		xIdx = 0;

		// Non-parametric (new row)
		QLabel *momentsIcon = new QLabel( this );
		momentsIcon->setObjectName( "Moments Threshold" );
		momentsIcon->setPixmap( QPixmap( ":/images/segment_Moments.png" ) );
		momentsIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		momentsIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		momentsIcon->show();
		momentsIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *renyiIcon = new QLabel( this );
		renyiIcon->setObjectName( "Renyi Threshold" );
		renyiIcon->setPixmap( QPixmap( ":/images/segment_Renyi.png" ) );
		renyiIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		renyiIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		renyiIcon->show();
		renyiIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *shanbhagngIcon = new QLabel( this );
		shanbhagngIcon->setObjectName( "Shanbhag Threshold" );
		shanbhagngIcon->setPixmap( QPixmap( ":/images/segment_Shanbhagng.png" ) );
		shanbhagngIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		shanbhagngIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		shanbhagngIcon->show();
		shanbhagngIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *yenIcon = new QLabel( this );
		yenIcon->setObjectName( "Yen Threshold" );
		yenIcon->setPixmap( QPixmap( ":/images/segment_Yen.png" ) );
		yenIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		yenIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		yenIcon->show();
		yenIcon->setAttribute( Qt::WA_DeleteOnClose );

		xIdx = 0;

		// Parametric Group Label
		QLabel *pSegmFilters = new QLabel( this );
		pSegmFilters->setFont( font );
		pSegmFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		pSegmFilters->setText( "Parametric segmentation filters" );
		pSegmFilters->move( columnGutter * xIdx, ++yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		pSegmFilters->show();

		QFrame *separatorP = new QFrame( this );
		separatorP->setGeometry( QRect( columnGutter * xIdx, ++yIdx * rowStartOffset + rowGutter * yGutterIdx - 5, nbOfPFilters * columnGutter, 1 ) );
		separatorP->setFrameShape( QFrame::HLine );
		separatorP->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
								   "border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorP->setFrameShadow( QFrame::Sunken );
		separatorP->show();

		// Parametric
		QLabel *binaryIcon = new QLabel( this );
		binaryIcon->setObjectName( "Binary Threshold" );
		binaryIcon->setPixmap( QPixmap( ":/images/segment_Binary.png" ) );
		binaryIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		binaryIcon->setToolTip( "<p>The <b>Binary Threshold</b> filter produces an output image whose pixels are either one of two "
								"values (OutsideValue or InsideValue), depending on whether the corresponding input image "
								"pixels lie between the two thresholds (LowerThreshold and UpperThreshold). Values equal to "
								"either threshold is considered to be between the thresholds.</p>"
								"<p>http://www.itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html</p>" );
		binaryIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		binaryIcon->show();
		binaryIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *ratsIcon = new QLabel( this );
		ratsIcon->setObjectName( "Rats Threshold" );
		ratsIcon->setPixmap( QPixmap( ":/images/segment_Rats.png" ) );
		ratsIcon->setObjectName( "Rats Threshold" );
		ratsIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		ratsIcon->setToolTip( "<p>The <b>Robust Automatic Threshold (RATS)</b> filter takes two inputs : the image to be thresholded "
							  "and a image of gradient magnitude of that image. The threshold is computed as the mean of the "
							  "pixel values in the input image weighted by the pixel values in the gradient image. The threshold "
							  "computed that way should be the mean pixel value where the intensity change the most.</p>"
							  "<p>Lehmann G.http ://hdl.handle.net/1926/370 http://www.insight-journal.org/browse/publication/134 </p>" );
		ratsIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		ratsIcon->show();
		ratsIcon->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *multiOtsu = new QLabel( this );
		multiOtsu->setObjectName( "Multiple Otsu" );
		multiOtsu->setPixmap( QPixmap( ":/images/segment_MultipleOtsu.png" ) );
		multiOtsu->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		multiOtsu->setToolTip( "<p>The <b>Multiple Otsu Threshol</b> filter creates a labeled image that separates the input image into various "
							   "classes.The filter computes the thresholds using the OtsuMultipleThresholdsCalculator and applies those thesholds "
							   "to the input image using the ThresholdLabelerImageFilter.The NumberOfHistogramBins and NumberOfThresholds can be set "
							   "for the Calculator.</p><p>This filter also includes an option to use the valley emphasis algorithm from <i>H.F.Ng, "
							   "Automatic thresholding for defect detection, Pattern Recognition Letters, ( 27 ) : 1644 - 1649, 2006</i>. The valley "
							   "emphasis algorithm is particularly effective when the object to be thresholded is small.</p>" );
		multiOtsu->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		multiOtsu->show();
		multiOtsu->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *mWaterB = new QLabel( this );
		mWaterB->setObjectName( "Morph Watershed Beucher" );
		mWaterB->setPixmap( QPixmap( ":/images/segment_MW_B.png" ) );
		mWaterB->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		mWaterB->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		mWaterB->show();
		mWaterB->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *mWaterM = new QLabel( this );
		mWaterM->setObjectName( "Morph Watershed Meyer" );
		mWaterM->setPixmap( QPixmap( ":/images/segment_MW_M.png" ) );
		mWaterM->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		mWaterM->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		mWaterM->show();
		mWaterM->setAttribute( Qt::WA_DeleteOnClose );

		//// Region Growing Group Label
		//QLabel *rgSegmFilters = new QLabel( this );
		//rgSegmFilters->setFont( font );
		//rgSegmFilters->setStyleSheet( "border: none" );
		//rgSegmFilters->setText( "Region growing segmentation filters" );
		//rgSegmFilters->move( 0, 2 * rowStartOffset + rowGutter * 2 );
		//rgSegmFilters->show();

		//QFrame *separatorRG = new QFrame( this );
		//separatorRG->setGeometry( QRect( 0, 3 * rowStartOffset + rowGutter * 2 - 5, nbOfRGFilters * columnGutter, 1 ) );
		//separatorRG->setFrameShape( QFrame::HLine );
		//separatorRG->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
		//							"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		//separatorRG->setFrameShadow( QFrame::Sunken );
		//separatorRG->show();

		xIdx = 0;

		QLabel *isoX = new QLabel( this );
		isoX->setObjectName( "IsoX Threshold" );
		isoX->setPixmap( QPixmap( ":/images/segment_IsoX.png" ) );
		isoX->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		isoX->setToolTip( "<p>The <b>IsoX Threshold</b> lies between the air/pore peak and the first material peak of the gray value "
						  "histogram.</p><p>An isoX value of 50% means that from this point the distance to the air/pore peak is the same as "
						  "the distance to the material 1 peak.</p>" );
		isoX->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		isoX->show();
		isoX->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *fhwIcon = new QLabel( this );
		fhwIcon->setObjectName( "Fhw Threshold" );
		fhwIcon->setPixmap( QPixmap( ":/images/segment_FHW.png" ) );
		fhwIcon->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		fhwIcon->setToolTip( "<p>The <b>FHW Threshold</b> is calculated based on the Iso50 value (the threshold which lies between the air/pore "
							 "peak and the first material peak) and the FHW weight factor." );
		fhwIcon->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter *yGutterIdx );
		fhwIcon->show();
		fhwIcon->setAttribute( Qt::WA_DeleteOnClose );

		// Region Growing
		QLabel *confiRegCrow = new QLabel( this );
		confiRegCrow->setObjectName( "Confidence Connected" );
		confiRegCrow->setPixmap( QPixmap( ":/images/segment_Confidence.png" ) );
		confiRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		confiRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		confiRegCrow->show();
		confiRegCrow->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *connRegCrow = new QLabel( this );
		connRegCrow->setObjectName( "Connected Threshold" );
		connRegCrow->setPixmap( QPixmap( ":/images/segment_Connected.png" ) );
		connRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		connRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		connRegCrow->show();
		connRegCrow->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *neighRegCrow = new QLabel( this );
		neighRegCrow->setObjectName( "Neighborhood Connected" );
		neighRegCrow->setPixmap( QPixmap( ":/images/segment_Neighborhood.png" ) );
		neighRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		neighRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		neighRegCrow->show();
		neighRegCrow->setAttribute( Qt::WA_DeleteOnClose );

		xIdx = 0;

		// Surrounding Group Label
		QLabel *surroundingFilters = new QLabel( this );
		surroundingFilters->setFont( font );
		surroundingFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		surroundingFilters->setText( "Surrounding filters" );
		surroundingFilters->move( columnGutter * xIdx, ++yIdx * rowStartOffset + ++yGutterIdx * rowGutter );
		surroundingFilters->show();

		QFrame *separatorSUR = new QFrame( this );
		separatorSUR->setGeometry( QRect( columnGutter * xIdx, ++yIdx * rowStartOffset + yGutterIdx * rowGutter - 5, 2 * columnGutter, 1 ) );
		separatorSUR->setFrameShape( QFrame::HLine );
		separatorSUR->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
									"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorSUR->setFrameShadow( QFrame::Sunken );
		separatorSUR->show();

		// Surrounding
		QLabel *createSurrounding = new QLabel( this );
		createSurrounding->setObjectName( "Create Surrounding" );
		createSurrounding->setPixmap( QPixmap( ":/images/createSurrounding.png" ) );	
		createSurrounding->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		createSurrounding->setToolTip( "" );
		createSurrounding->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		createSurrounding->show();
		createSurrounding->setAttribute( Qt::WA_DeleteOnClose );

		QLabel *removeSurrounding = new QLabel( this );
		removeSurrounding->setObjectName( "Remove Surrounding" );
		removeSurrounding->setPixmap( QPixmap( ":/images/removeSurrounding.png" ) );	
		removeSurrounding->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		removeSurrounding->setToolTip( "" );
		removeSurrounding->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		removeSurrounding->show();
		removeSurrounding->setAttribute( Qt::WA_DeleteOnClose );
	}
}

QPixmap iADragFilterWidget::mergeOnTop( const QPixmap& pix, QString txt )
{	
	txt.truncate( 7 );
	txt.append( "..." );
	QPainter p;
	int strWidth = p.fontMetrics().width( txt );
	int strHeight = p.fontMetrics().height();
	QPixmap res( pix.width(), pix.height() );
	p.begin( &res );
	p.drawPixmap( 0, 0, pix );
	p.drawText( QRect( 0, pix.height() - strHeight - 3, strWidth, strHeight ), 0, txt );
	p.end();
	return res;
}

void iADragFilterWidget::dragEnterEvent( QDragEnterEvent *event )
{
	if ( event->mimeData()->hasFormat( customMimeType[m_d_f_switch] ) )
	{
		if ( event->source() == this )
		{
			event->setDropAction( Qt::MoveAction );
			event->accept();
		}
		else
			event->acceptProposedAction();
	}
	else
		event->ignore();
}

void iADragFilterWidget::dragMoveEvent( QDragMoveEvent *event )
{
	if ( event->mimeData()->hasFormat( customMimeType[m_d_f_switch] ) )
	{
		if ( event->source() == this )
		{
			event->setDropAction( Qt::MoveAction );
			event->accept();
		}
		else
			event->acceptProposedAction();
	}
	else
		event->ignore();
}

void iADragFilterWidget::dropEvent( QDropEvent *event )
{
	if ( event->mimeData()->hasFormat( customMimeType[m_d_f_switch] ) )
	{
		if ( event->source() == this )
		{
			event->setDropAction( Qt::CopyAction );
			event->accept();
		}
		else
			event->acceptProposedAction();
	}
	else
		event->ignore();
}

void iADragFilterWidget::mousePressEvent( QMouseEvent *event )
{
	QLabel *child = static_cast<QLabel*>( childAt( event->pos() ) );
	if ( !child )
		return;

	if ( !child->pixmap() )
		return;

	QPixmap pixmap = *child->pixmap();
	QString filtername = child->objectName();
	QString description = child->toolTip();
	QByteArray itemData;
	QDataStream dataStream( &itemData, QIODevice::WriteOnly );
	dataStream << pixmap << -1 << filtername << description;

	QMimeData *mimeData = new QMimeData;
	mimeData->setData( customMimeType[filtername.startsWith( "dataset_" )], itemData );

	QDrag *drag = new QDrag( this );
	drag->setMimeData( mimeData );
	drag->setPixmap( pixmap );
	drag->setHotSpot( event->pos() - child->pos() );

	QPixmap tempPixmap = pixmap;
	QPainter painter;
	painter.begin( &tempPixmap );
	painter.fillRect( pixmap.rect(), QColor( 127, 127, 127, 127 ) );
	painter.end();

	child->setPixmap( tempPixmap );

	if ( drag->exec( Qt::CopyAction | Qt::MoveAction, Qt::CopyAction ) == Qt::MoveAction )
		child->close();
	else
	{
		child->show();
		child->setPixmap( pixmap );
	}
}

QString iADragFilterWidget::generateDatasetTooltip( QString dataset )
{
	QString datasetSlicesDir = m_datasetDir + "/" + QFileInfo( dataset ).baseName();
	int sliceNumber = ( QDir( getMaskSliceDirNameAbsolute( datasetSlicesDir ) ).count() - 2 ) * 0.5;
	QString sliceName = getSliceFilename( datasetSlicesDir, sliceNumber );
	QPixmap tmpPic( sliceName );
	QPixmap scaledTmpPic;
	QString datasetToolTipStr;
	QStringList datasetInfo = getDatasetInfo( m_datasetDir, dataset );

	if ( tmpPic.width() > 500 || tmpPic.height() > 500 )
		scaledTmpPic = tmpPic.scaled( tmpPic.width() / 2, tmpPic.height() / 2, Qt::KeepAspectRatio );
	else
		scaledTmpPic = tmpPic;

	if ( !scaledTmpPic.isNull() )
	{
		datasetToolTipStr.append( tr( "<center><br><img src=\"%1\" width=%2 height=%3></center><br>" )
								  .arg( sliceName ).arg( scaledTmpPic.width() ).arg( scaledTmpPic.height() ) );
	}
	else
		datasetToolTipStr.append( "<center><br>No dataset preview available.</center><br>" );

	if ( datasetInfo[2].section( ' ', 0, 0 ) == "DimSize(microns)" )
	{
		int zDim = datasetInfo[2].section( ' ', -1, -1 ).section( ':', 1, 1 ).toInt() / 2;
		datasetToolTipStr.append( tr( "<left>XY-Slice No. %4<br></left>" ).arg( zDim ) );
	}
	for ( int k = 0; k < datasetInfo.size(); ++k )
		datasetToolTipStr.append( tr( "%1<br>" ).arg( datasetInfo[k] ) );

	return datasetToolTipStr;
}

void iADragFilterWidget::updateDatasetTooltip( QStringList filesToUpdateList )
{
	foreach( QString s, filesToUpdateList )
		this->findChild<QLabel*>( QString("dataset_").append(s) )->setToolTip(generateDatasetTooltip( s ) );
}

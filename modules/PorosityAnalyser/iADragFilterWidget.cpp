/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iADragFilterWidget.h"

#include "PorosityAnalyserHelpers.h"

#include <QtWidgets>
#include <QStringList>

const QStringList customMimeType = QStringList()\
<< "application/x-dnditemdatafilter"\
<< "application/x-dnditemdatadataset";

const int rowStartOffset = 25;
const int maxColumns = 5;
const int maxRows = 8;
const int iconHeight = 60;
const int iconWidth = 60;
const int columnGutter = iconWidth + 7;
const int rowGutter = iconHeight + 10;

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
			m_labelList.append( dataset );
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
		separatorSM->setGeometry( QRect( xIdx, yIdx * rowStartOffset - 5, maxColumns * columnGutter, 1 ) );
		separatorSM->setFrameShape( QFrame::HLine );
		separatorSM->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
									"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorSM->setFrameShadow( QFrame::Sunken );
		separatorSM->show();

		// Smoothing
		QLabel *gad = new QLabel( this );
		gad->setObjectName( "Gradient Anisotropic Diffusion Smoothing" );
		gad->setPixmap( QPixmap( ":/images/smooth_GAD.png" ) );
		gad->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		gad->setToolTip( "<p>The <b>GradientAnisotropicDiffusion</b> smoothing filter reduce noise (or unwanted detail) "
							 "in images while preserving specific image features. For many applications, there is an assumption "
							 "that light-dark transitions (edges) are interesting. Standard isotropic diffusion methods move and "
							 "blur light-dark boundaries. Anisotropic diffusion methods are formulated to specifically preserve edges.</p>"
							 "<p><b>TimeStep: </b>appropriate time steps for solving this type of p.d.e.depend on the dimensionality of "
							 "the image and the order of the equation.Stable values for most 2D and 3D functions are 0.125 and 0.0625, "
							 "respectively, when the pixel spacing is unity or is turned off.In general, you should keep the time step "
							 "below (PixelSpacing)/2^(N + 1), where N is the number of image dimensions.</p>"
							 "<p><b>Conductance Parameter: </b>The conductance parameter controls the sensitivity of the conductance "
							 "term in the basic anisotropic diffusion equation. It affects the conductance term in different ways "
							 "depending on the particular variation on the basic equation. As a general rule, the lower the value, the "
							 "more strongly the diffusion equation preserves image features (such as high gradients or curvature). A high "
							 "value for conductance will cause the filter to diffuse image features more readily. Typical values range from "
							 "0.5 to 2.0 for data like the Visible Human color data."
							 "<p>https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionImageFilter.html</p>" );
		gad->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		gad->show();
		gad->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( gad );

		QLabel *cad = new QLabel( this );
		cad->setObjectName( "Curvature Anisotropic Diffusion Smoothing" );
		cad->setPixmap( QPixmap( ":/images/smooth_CAD.png" ) );
		cad->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		cad->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		cad->show();
		cad->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( cad );

		QLabel *gauss = new QLabel( this );
		gauss->setObjectName( "Recursive Gaussian Smoothing" );
		gauss->setPixmap( QPixmap( ":/images/smooth_Gauss.png" ) );
		gauss->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		gauss->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		gauss->show();
		gauss->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( gauss );

		QLabel *bil = new QLabel( this );
		bil->setObjectName( "Bilateral Smoothing" );
		bil->setPixmap( QPixmap( ":/images/smooth_Bilateral.png" ) );
		bil->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		bil->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		bil->show();
		bil->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( bil );

		QLabel *med = new QLabel( this );
		med->setObjectName( "Median Smoothing" );
		med->setPixmap( QPixmap( ":/images/smooth_Median.png" ) );
		med->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		med->setToolTip( "<p>The <b>Median</b> smoothing filter Computes an image where a given pixel is the median "
							 "value of the the pixels in a neighborhood about the corresponding input pixel. A median filter "
							 "is one of the family of nonlinear filters.It is used to smooth an image without being biased "
							 "by outliers or shot noise.</p>"
							 "<p>https://itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html</p>" );
		med->move( columnGutter * xIdx++, yIdx * rowStartOffset );
		med->show();
		med->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( med );

		xIdx = 0;

		// Non-parametric Group Label
		QLabel *npSegmFilters = new QLabel( this );
		npSegmFilters->setFont( font );
		npSegmFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		npSegmFilters->setText( "Non-parametric segmentation filters" );
		npSegmFilters->move( columnGutter * xIdx, ++yIdx * rowStartOffset + yGutterIdx * rowGutter );
		npSegmFilters->show();

		QFrame *separatorNP = new QFrame( this );
		separatorNP->setGeometry( QRect( columnGutter * xIdx, ++yIdx * rowStartOffset + yGutterIdx * rowGutter - 5, maxColumns * columnGutter, 1 ) );
		separatorNP->setFrameShape( QFrame::HLine );
		separatorNP->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
									"border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorNP->setFrameShadow( QFrame::Sunken );
		separatorNP->show();

		// Non-parametric
		QLabel *otsu = new QLabel( this );
		otsu->setObjectName( "Otsu Threshold" );
		otsu->setPixmap( QPixmap( ":/images/segment_Otsu.png" ) );
		otsu->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
		otsu->setToolTip( "<p>The <b>Otsu Threshold</b> filter creates a binary thresholded image that "
							  "separates an image into foreground and background components.The filter computes the threshold "
							  "using the OtsuThresholdCalculator and applies that theshold to the input image using the BinaryThresholdImageFilter.</p>"
							  "<p>http ://hdl.handle.net/10380/3279 or http://www.insight-journal.org/browse/publication/811 </p>" );
		otsu->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		otsu->show();
		otsu->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( otsu );

		QLabel *isoData = new QLabel( this );
		isoData->setObjectName( "IsoData Threshold" );
		isoData->setPixmap( QPixmap( ":/images/segment_IsoData.png" ) );
		isoData->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		isoData->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		isoData->show();
		isoData->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( isoData );

		QLabel *intermodes = new QLabel( this );
		intermodes->setObjectName( "Intermodes Threshold" );
		intermodes->setPixmap( QPixmap( ":/images/segment_Intermodes.png" ) );
		intermodes->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		intermodes->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		intermodes->show();
		intermodes->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( intermodes );

		QLabel *maxEntropy = new QLabel( this );
		maxEntropy->setObjectName( "MaxEntropy Threshold" );
		maxEntropy->setPixmap( QPixmap( ":/images/segment_MaxEntropy.png" ) );
		maxEntropy->setStyleSheet( "QToolTip{ color: black; background - color: #ffffe1; border: 0px solid white; }" );
		maxEntropy->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		maxEntropy->show();
		maxEntropy->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( maxEntropy );

		QLabel *minimum = new QLabel( this );
		minimum->setObjectName( "Minimum Threshold" );
		minimum->setPixmap( QPixmap( ":/images/segment_Minimum.png" ) );
		minimum->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		minimum->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		minimum->show();
		minimum->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( minimum );

		xIdx = 0;

		// Non-parametric (new row)
		QLabel *moments = new QLabel( this );
		moments->setObjectName( "Moments Threshold" );
		moments->setPixmap( QPixmap( ":/images/segment_Moments.png" ) );
		moments->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		moments->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		moments->show();
		moments->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( moments );

		QLabel *renyi = new QLabel( this );
		renyi->setObjectName( "Renyi Threshold" );
		renyi->setPixmap( QPixmap( ":/images/segment_Renyi.png" ) );
		renyi->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		renyi->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		renyi->show();
		renyi->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( renyi );

		QLabel *shanbhagng = new QLabel( this );
		shanbhagng->setObjectName( "Shanbhag Threshold" );
		shanbhagng->setPixmap( QPixmap( ":/images/segment_Shanbhagng.png" ) );
		shanbhagng->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		shanbhagng->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		shanbhagng->show();
		shanbhagng->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( shanbhagng );

		QLabel *yen = new QLabel( this );
		yen->setObjectName( "Yen Threshold" );
		yen->setPixmap( QPixmap( ":/images/segment_Yen.png" ) );
		yen->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		yen->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		yen->show();
		yen->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( yen );

		QLabel *huang = new QLabel( this );
		huang->setObjectName( "Huang Threshold" );
		huang->setPixmap( QPixmap( ":/images/segment_Huang.png" ) );
		huang->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		huang->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		huang->show();
		huang->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( huang );

		xIdx = 0;

		// Non-parametric (new row)
		QLabel *li = new QLabel( this );
		li->setObjectName( "Li Threshold" );
		li->setPixmap( QPixmap( ":/images/segment_Li.png" ) );
		li->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		li->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		li->show();
		li->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( li );

		QLabel *KittlerIllingworth = new QLabel( this );
		KittlerIllingworth->setObjectName( "KittlerIllingworth Threshold" );
		KittlerIllingworth->setPixmap( QPixmap( ":/images/segment_KittlerIllingworth.png" ) );
		KittlerIllingworth->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		KittlerIllingworth->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		KittlerIllingworth->show();
		KittlerIllingworth->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( KittlerIllingworth );

		QLabel *Triangle = new QLabel( this );
		Triangle->setObjectName( "Triangle Threshold" );
		Triangle->setPixmap( QPixmap( ":/images/segment_Triangle.png" ) );
		Triangle->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		Triangle->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		Triangle->show();
		Triangle->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( Triangle );

		xIdx = 0;

		// Parametric Group Label
		QLabel *pSegmFilters = new QLabel( this );
		pSegmFilters->setFont( font );
		pSegmFilters->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		pSegmFilters->setText( "Parametric segmentation filters" );
		pSegmFilters->move( columnGutter * xIdx, ++yIdx * rowStartOffset + rowGutter * ++yGutterIdx );
		pSegmFilters->show();

		QFrame *separatorP = new QFrame( this );
		separatorP->setGeometry( QRect( columnGutter * xIdx, ++yIdx * rowStartOffset + rowGutter * yGutterIdx - 5, maxColumns * columnGutter, 1 ) );
		separatorP->setFrameShape( QFrame::HLine );
		separatorP->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
								   "border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
		separatorP->setFrameShadow( QFrame::Sunken );
		separatorP->show();

		// Parametric
		QLabel *binary = new QLabel( this );
		binary->setObjectName( "Binary Threshold" );
		binary->setPixmap( QPixmap( ":/images/segment_Binary.png" ) );
		binary->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		binary->setToolTip( "<p>The <b>Binary Threshold</b> filter produces an output image whose pixels are either one of two "
								"values (OutsideValue or InsideValue), depending on whether the corresponding input image "
								"pixels lie between the two thresholds (LowerThreshold and UpperThreshold). Values equal to "
								"either threshold is considered to be between the thresholds.</p>"
								"<p>https://itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html</p>" );
		binary->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		binary->show();
		binary->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( binary );

		QLabel *general = new QLabel(this);
		general->setObjectName("General Threshold");
		general->setPixmap(QPixmap(":/images/segment_Binary.png"));
		general->setStyleSheet("QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }");
		general->setToolTip("<p>The <b>Binary Threshold</b> filter produces an output image whose pixels are either one of two "
			"values (OutsideValue or InsideValue), depending on whether the corresponding input image "
			"pixels lie between the two thresholds (LowerThreshold and UpperThreshold). Values equal to "
			"either threshold is considered to be between the thresholds.</p>"
			"<p>https://itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html</p>");
		general->move(columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx);
		general->show();
		general->setAttribute(Qt::WA_DeleteOnClose);
		m_labelList.append(general);

		QLabel *rats = new QLabel( this );
		rats->setObjectName( "Rats Threshold" );
		rats->setPixmap( QPixmap( ":/images/segment_Rats.png" ) );
		rats->setObjectName( "Rats Threshold" );
		rats->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		rats->setToolTip( "<p>The <b>Robust Automatic Threshold (RATS)</b> filter takes two inputs : the image to be thresholded "
							  "and a image of gradient magnitude of that image. The threshold is computed as the mean of the "
							  "pixel values in the input image weighted by the pixel values in the gradient image. The threshold "
							  "computed that way should be the mean pixel value where the intensity change the most.</p>"
							  "<p>Lehmann G.http ://hdl.handle.net/1926/370 http://www.insight-journal.org/browse/publication/134 </p>" );
		rats->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		rats->show();
		rats->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( rats );

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
		m_labelList.append( multiOtsu );

		QLabel *mWaterB = new QLabel( this );
		mWaterB->setObjectName( "Morph Watershed Beucher" );
		mWaterB->setPixmap( QPixmap( ":/images/segment_MW_B.png" ) );
		mWaterB->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		mWaterB->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		mWaterB->show();
		mWaterB->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( mWaterB );

		QLabel *mWaterM = new QLabel( this );
		mWaterM->setObjectName( "Morph Watershed Meyer" );
		mWaterM->setPixmap( QPixmap( ":/images/segment_MW_M.png" ) );
		mWaterM->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		mWaterM->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		mWaterM->show();
		mWaterM->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( mWaterM );

		//// Region Growing Group Label
		//QLabel *rgSegmFilters = new QLabel( this );
		//rgSegmFilters->setFont( font );
		//rgSegmFilters->setStyleSheet( "border: none" );
		//rgSegmFilters->setText( "Region growing segmentation filters" );
		//rgSegmFilters->move( 0, 2 * rowStartOffset + rowGutter * 2 );
		//rgSegmFilters->show();

		//QFrame *separatorRG = new QFrame( this );
		//separatorRG->setGeometry( QRect( 0, 3 * rowStartOffset + rowGutter * 2 - 5, maxColumns * columnGutter, 1 ) );
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
		m_labelList.append( isoX );

		QLabel *fhw = new QLabel( this );
		fhw->setObjectName( "Fhw Threshold" );
		fhw->setPixmap( QPixmap( ":/images/segment_FHW.png" ) );
		fhw->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		fhw->setToolTip( "<p>The <b>FHW Threshold</b> is calculated based on the Iso50 value (the threshold which lies between the air/pore "
							 "peak and the first material peak) and the FHW weight factor." );
		fhw->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter *yGutterIdx );
		fhw->show();
		fhw->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( fhw );

		// Region Growing
		QLabel *confiRegCrow = new QLabel( this );
		confiRegCrow->setObjectName( "Confidence Connected" );
		confiRegCrow->setPixmap( QPixmap( ":/images/segment_Confidence.png" ) );
		confiRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		confiRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		confiRegCrow->show();
		confiRegCrow->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( confiRegCrow );

		QLabel *connRegCrow = new QLabel( this );
		connRegCrow->setObjectName( "Connected Threshold" );
		connRegCrow->setPixmap( QPixmap( ":/images/segment_Connected.png" ) );
		connRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		connRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		connRegCrow->show();
		connRegCrow->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( connRegCrow );

		QLabel *neighRegCrow = new QLabel( this );
		neighRegCrow->setObjectName( "Neighborhood Connected" );
		neighRegCrow->setPixmap( QPixmap( ":/images/segment_Neighborhood.png" ) );
		neighRegCrow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		neighRegCrow->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		neighRegCrow->show();
		neighRegCrow->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( neighRegCrow );

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
		m_labelList.append( createSurrounding );

		QLabel *removeSurrounding = new QLabel( this );
		removeSurrounding->setObjectName( "Remove Surrounding" );
		removeSurrounding->setPixmap( QPixmap( ":/images/removeSurrounding.png" ) );	
		removeSurrounding->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 0px solid white; }" );
		removeSurrounding->setToolTip( "" );
		removeSurrounding->move( columnGutter * xIdx++, yIdx * rowStartOffset + rowGutter * yGutterIdx );
		removeSurrounding->show();
		removeSurrounding->setAttribute( Qt::WA_DeleteOnClose );
		m_labelList.append( removeSurrounding );
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

QLabel* iADragFilterWidget::getLabel( QString name )
{
	for ( int i = 0; i < m_labelList.length(); ++i )
	{
		if ( m_labelList[i]->objectName() == name )
			return ( m_labelList[i] );
	}
	return 0;
}
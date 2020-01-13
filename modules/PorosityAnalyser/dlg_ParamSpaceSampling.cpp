/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "dlg_ParamSpaceSampling.h"

#include "PorosityAnalyserHelpers.h"

#include <mdichild.h>

#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkMedianImageFilter.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QErrorMessage>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpacerItem>
#include <QSpinBox>


namespace
{
	int detect_peak(const double* data, /* the data */ int data_count, /* row count of data */
		int* emi_peaks, /* emission peaks will be put here */ int* num_emi_peaks, /* number of emission peaks found */
		int max_emi_peaks, /* maximum number of emission peaks */ int* absop_peaks, /* absorption peaks will be put here */
		int* num_absop_peaks, /* number of absorption peaks found */ int max_absop_peaks, /* maximum number of absorption peaks */
		double delta, /* delta used for distinguishing peaks */ int emi_first /* should we search emission peak first of absorption peak first? */)
	{
		//Copyright 2011, 2013 Hong Xu.All rights reserved.

		//	Redistribution and use in source and binary forms, with or without
		//	modification, are permitted provided that the following conditions are met :

		//1. Redistributions of source code must retain the above copyright notice,
		//	this list of conditions and the following disclaimer.

		//	2. Redistributions in binary form must reproduce the above copyright
		//	notice, this list of conditions and the following disclaimer in the
		//	documentation and / or other materials provided with the distribution.

		//	THIS SOFTWARE IS PROVIDED BY HONG XU ``AS IS'' AND ANY EXPRESS OR IMPLIED
		//	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
		//	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO
		//	EVENT SHALL HONG XU OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
		//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES( INCLUDING, BUT
		//	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
		//	DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
		//	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
		//	( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
		//	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
		//http://billauer.co.il/peakdet.html


		// Detect peaks
		int     i;
		double  mx;
		double  mn;
		int     mx_pos = 0;
		int     mn_pos = 0;
		int     is_detecting_emi = emi_first;

		mx = data[0];
		mn = data[0];
		*num_emi_peaks = 0;
		*num_absop_peaks = 0;

		for (i = 1; i < data_count; ++i)
		{
			if (data[i] > mx)
			{
				mx_pos = i;
				mx = data[i];
			}
			if (data[i] < mn)
			{
				mn_pos = i;
				mn = data[i];
			}

			if (is_detecting_emi &&
				data[i] < mx - delta)
			{
				if (*num_emi_peaks >= max_emi_peaks) /* not enough spaces */
					return 1;

				emi_peaks[*num_emi_peaks] = mx_pos;
				++(*num_emi_peaks);

				is_detecting_emi = 0;

				i = mx_pos - 1;

				mn = data[mx_pos];
				mn_pos = mx_pos;
			}
			else if ((!is_detecting_emi) &&
				data[i] > mn + delta)
			{
				if (*num_absop_peaks >= max_absop_peaks)
					return 2;

				absop_peaks[*num_absop_peaks] = mn_pos;
				++(*num_absop_peaks);

				is_detecting_emi = 1;

				i = mn_pos - 1;

				mx = data[mn_pos];
				mx_pos = mn_pos;
			}
		}
		return 0;
	}

	enum ContainerSize
	{
		WIDTH = 530, HEIGHT = 600
	};
}

dlg_ParamSpaceSampling::dlg_ParamSpaceSampling( QWidget *parent, QString winTitle, int n, QStringList inList,
	QList<QVariant> inPara, QTextDocument * fDescr, QString datasetDir, QString datasetName, QStringList datasetInfo,
	QVector<double> keyData, QVector<double> valueData, QString filterName, bool modal ) : QDialog( parent ),
	m_keyData( keyData ), m_valueData( valueData ), m_datasetDir( datasetDir ), m_datasetName( datasetName ),
	m_datasetInfo( datasetInfo ), m_description( fDescr ), m_filterName( filterName ), m_inPara( inPara )
{
	//initialize a instance of error message dialog box
	QErrorMessage* eMessage = new QErrorMessage( this );
	this->setModal( modal );

	//check whether the input parameters are correct
	//if the input parameter are not correct show error message
	//if the input parameters are correct, then initialize the dialog class
	if (winTitle.isEmpty())
	{
		eMessage->showMessage("No window title entered. Please give a window title");
	}
	else if (inList.size() != n)
	{
		eMessage->showMessage("The number of strings in the string list and the number of parameters entered does not match.");
	}
	else
	{
		//setup the ui dialog class widget as this
		setupUi( this );

		m_numPara = n;

		//set the window title
		this->setWindowTitle( winTitle );

		// TODO: better way to set up init values -> registry
		m_delta = 6; m_sigma = 300; m_isoX = 50;

		createDatasetPreview();
		createDatasetInfo();
		createHistoSpinBoxes();
		computeSmoothHisto( );
		computePeaks( m_smoothKey, m_smoothValue );
		createHistoPlot();
		createHistoPeaks();
		createDescription();

		// Generates a scrollable container for the widgets with a grid layout
		auto scrollArea = new QScrollArea( this );
		scrollArea->setObjectName( "scrollArea" );
		m_container = new QWidget( scrollArea );
		m_container->setObjectName( "container" );
		auto containerLayout = new QGridLayout( m_container );
		containerLayout->setObjectName( "containerLayout" );

		for ( int i = 0; i < m_numPara; i++ )
		{
			QString nameStr(inList[i]);

			if ( !nameStr.contains( QRegExp( "[$#+*^?]" ) ) )
			{
				eMessage->showMessage( QString( "Unknown widget prefix '" )
					.append( inList[i][0] )
					.append( "' for label \"" )
					.append(nameStr.remove( 0, 1 ) ).append( "\"" ) );
			}
			else
			{
				nameStr.remove(0, 1);
			}

			QString tempStr = nameStr;

			QLabel *label = new QLabel(m_container);
			label->setObjectName( tempStr.append( "Label" ) );
			m_widgetList.insert( i, tempStr );
			label->setText(nameStr);
			containerLayout->addWidget( label, i, 0, 1, 1 );

			QWidget *newWidget = nullptr;

			tempStr = nameStr;
			switch ( inList[i].at( 0 ).toLatin1() )
			{
				case '$':
				{
					newWidget = new QCheckBox(m_container);
					newWidget->setObjectName( tempStr.append( "CheckBox" ) );
				}
					break;
				case '#':
				{
					newWidget = new QLineEdit(m_container);
					newWidget->setObjectName( tempStr.append( "LineEdit" ) );
				}
					break;
				case '+':
				{
					tempStr = nameStr;
					newWidget = new QComboBox(m_container);
					newWidget->setObjectName( tempStr.append( "ComboBox" ) );

				}
					break;
				case '*':
				{
					tempStr = nameStr;
					newWidget = new QSpinBox(m_container);
					newWidget->setObjectName( tempStr.append( "SpinBox" ) );
					( (QSpinBox*) newWidget )->setRange( 0, 65536 );
				}
					break;
				case '^':	// alexander 14.10.2012
				{
					tempStr = nameStr;
					newWidget = new QDoubleSpinBox(m_container);
					newWidget->setObjectName( tempStr.append( "QDoubleSpinBox" ) );
					( (QDoubleSpinBox*) newWidget )->setSingleStep( 0.001 );
					( (QDoubleSpinBox*) newWidget )->setDecimals( 6 );
					( (QDoubleSpinBox*) newWidget )->setRange( -999999, 999999 );
				}
					break;
				case '?':
				{
					label->setStyleSheet( "background-color : lightGray" );
					QFont font = label->font();
					font.setBold( true );
					font.setPointSize( 11 );
					label->setFont( font );
					continue;
				}
					break;
			}

			m_widgetList.insert( i, tempStr );
			containerLayout->addWidget( newWidget, i, 1, 1, 1 );
		}

		// Controls the containers width and sets the correct width for the widgets
		containerLayout->setColumnMinimumWidth( 1, WIDTH + 30 - containerLayout->minimumSize().width() );
		m_container->setMaximumWidth( WIDTH );
		m_container->setLayout( containerLayout );

		// Set scrollbar if needed
		if (containerLayout->minimumSize().height() > HEIGHT)
		{
			scrollArea->setMinimumHeight(HEIGHT);
		}
		else
		{
			scrollArea->setMinimumHeight(containerLayout->minimumSize().height());
		}

		if (containerLayout->minimumSize().width() > WIDTH)
		{
			scrollArea->setMinimumWidth(WIDTH + 20);
		}
		else
		{
			scrollArea->setMinimumWidth(containerLayout->minimumSize().width());
		}

		// Add the container to the scrollarea 
		scrollArea->setWidget(m_container);

		// Make scrollArea widgets backround transparent
		QPalette pal = scrollArea->palette();
		pal.setColor( scrollArea->backgroundRole(), Qt::transparent );
		scrollArea->setPalette( pal );

		// Add the scrollarea to the gridlayout
		gridLayout->addWidget( scrollArea, 5, 0, 1, 1 );

		// Adds units to linedit labels and spinbox
		addUnits();

		// Sets init line edit values 
		updateLineEdits();

		// Sets init line edit values
		updateValues( m_inPara );

		// Add the ok and cancel button to the gridlayout
		gridLayout->addWidget( buttonBox, 6, 0, 1, 2, Qt::AlignRight );
	}
}

void dlg_ParamSpaceSampling::updateValues( QList<QVariant> inPara )
{
	QObjectList children = m_container->children();

	int paramIdx = 0;
	for ( int i = 0; i < children.size(); i++ )
	{
		QLineEdit *lineEdit = dynamic_cast<QLineEdit*>( children.at( i ) );
		if (lineEdit != nullptr)
		{
			lineEdit->setText(inPara[paramIdx++].toString());
		}

		QComboBox *comboBox = dynamic_cast<QComboBox*>( children.at( i ) );

		if ( comboBox != nullptr )
		{
			comboBox->addItems( inPara[paramIdx++].toStringList() );
		}
		QCheckBox *checkBox = dynamic_cast<QCheckBox*>( children.at( i ) );
		if ( checkBox != nullptr )
		{
			if (inPara[paramIdx] == tr("true"))
			{
				checkBox->setChecked(true);
			}
			else if (inPara[paramIdx] == tr("false"))
			{
				checkBox->setChecked(false);
			}
			else
			{
				checkBox->setChecked(inPara[paramIdx] != 0);
			}
			paramIdx++;

		}

		QSpinBox *spinBox = dynamic_cast<QSpinBox*>( children.at( i ) );
		if (spinBox != nullptr)
		{
			spinBox->setValue(inPara[paramIdx++].toDouble());
		}

		QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox*>( children.at( i ) );
		if (doubleSpinBox != nullptr)
		{
			doubleSpinBox->setValue(inPara[paramIdx++].toDouble());
		}
	}
}

double dlg_ParamSpaceSampling::getValue(int index) const
{
	QLineEdit *t = m_container->findChild<QLineEdit*>( m_widgetList[index] );
	return t ? t->text().toDouble() : 0.0;
}

void dlg_ParamSpaceSampling::createDatasetPreview()
{
	// Load and scale dataset preview (png image)
	QWidget * datasetSliceContainer = new QWidget( this );
	QHBoxLayout * dcHBLayout = new QHBoxLayout;
	QLabel * datasetSlice = new QLabel( this );
	QString datasetSlicesDir = m_datasetDir + "/" + QFileInfo( m_datasetName ).baseName();
	int sliceNumber = ( QDir( getMaskSliceDirNameAbsolute( datasetSlicesDir ) ).count() - 2 ) * 0.5;
	QString sliceName = getSliceFilename( datasetSlicesDir, sliceNumber );
	QPixmap tmpPic( sliceName );
	QPixmap scaledTmpPic;
	scaledTmpPic = tmpPic.scaled( 500, 300, Qt::KeepAspectRatio );
	datasetSliceContainer->setMinimumSize( scaledTmpPic.width(), scaledTmpPic.height() + 30 );
	datasetSlice->setPixmap( scaledTmpPic );
	dcHBLayout->addWidget( datasetSlice );
	datasetSliceContainer->setLayout( dcHBLayout );
	gridLayout->setRowMinimumHeight( 0, datasetSliceContainer->height() );
	gridLayout->addWidget( datasetSliceContainer, 0, 0, 1, 1, Qt::AlignCenter );
}

void dlg_ParamSpaceSampling::createDatasetInfo()
{
	// Dataset info
	QTextEdit * textEdit = new QTextEdit();
	textEdit->setMaximumHeight( 200 );
	//textEdit->setStyleSheet( "QTextEdit { background-color: #ffffe1}" );
	QString text;
	//text.append( "\n" );
	for (int i = 0; i < m_datasetInfo.size(); i++)
	{
		text.append("<p style = line-height:50 % >" + m_datasetInfo[i] + "</p>");
	}
	textEdit->setHtml( text );
	gridLayout->setColumnMinimumWidth( 1, 250 );
	gridLayout->addWidget( textEdit, 0, 1, 1, 1 );
}

void dlg_ParamSpaceSampling::createHistoSpinBoxes()
{
	//TODO: Set SpinBox initial values from registry, see: QSettings settings( organisationName, applicationName );

	// histogram buttons
	auto histoBtnContainer = new QWidget( this );
	auto histogramBtnContainer_HBLayout = new QHBoxLayout();

	auto sbDelta_Label = new QLabel( histoBtnContainer );
	sbDelta_Label->setText( "Cutoff bin hight:" );
	sbDelta_Label->setToolTip( "<p>Histogram peak detection</p><p>A point is considered as a maximum peak if it has the maximal"
							   "value, and was preceded (to the left) by a value lower than the 'cutoff bin hight' (DELTA).</p>"
							   "<p>A too small value generats many local maxima, a value too high only a few or none.</p>" );
	auto sbDelta = new QSpinBox( histoBtnContainer );
	sbDelta->setMinimum( 1 );
	sbDelta->setMaximum( 999 );
	sbDelta->setValue( m_delta );
	sbDelta->setToolTip( "<p>Histogram peak detection</p><p>A point is considered as a maximum peak if it has the maximal"
						 "value, and was preceded (to the left) by a value lower than the 'cutoff bin hight' (DELTA).</p>"
						 "<p>A too small value generats many local maxima, a value too high only a few or none.</p>" );

	auto sbSigma_Label = new QLabel( histoBtnContainer );
	sbSigma_Label->setText( "Sigma (smoothing):" );
	sbSigma_Label->setToolTip( "<p>Set / Get the Sigma, measured in world coordinates, of the Gaussian kernel.The default"
							   "is 1.0.An exception will be generated if the Sigma value is less than or equal to zero.</p>" );
	auto sbSigma = new QSpinBox( histoBtnContainer );
	sbSigma->setMinimum( 1 );
	sbSigma->setMaximum( 3000 );
	sbSigma->setValue( m_sigma );
	sbSigma->setToolTip( "<p>Set / Get the Sigma, measured in world coordinates, of the Gaussian kernel.The default"
						 "is 1.0.An exception will be generated if the Sigma value is less than or equal to zero.</p>" );

	auto sbIsoX_Label = new QLabel( histoBtnContainer );
	sbIsoX_Label->setText( "IsoX:" );
	sbIsoX_Label->setToolTip( "<p>X % distance between Air peak and Mareial 1 peak.</p>" );
	m_sbIsoX = new QSpinBox( histoBtnContainer );
	m_sbIsoX->setMinimum( 1 );
	m_sbIsoX->setMaximum( 99 );
	m_sbIsoX->setValue( m_isoX );
	m_sbIsoX->setToolTip( "<p>X % distance between Air peak and Mareial 1 peak.</p>" );

	auto cbSHLine_Label = new QLabel( histoBtnContainer );
	cbSHLine_Label->setText( "Histogram peak line:" );
	cbSHLine_Label->setToolTip( "<p>Shows the smoothed histogram peak line.</p>" );
	m_cbSHLine = new QCheckBox( histoBtnContainer );
	m_cbSHLine->setToolTip( "<p>Shows the smoothed histogram peak line.</p>" );

	QObject::connect( sbDelta, SIGNAL( valueChanged( int ) ), this, SLOT( updateHistoPeaks( int ) ) );
	QObject::connect( sbSigma, SIGNAL( valueChanged( int ) ), this, SLOT( updateHistoSmooth( int ) ) );
	QObject::connect( m_sbIsoX, SIGNAL( valueChanged( int ) ), this, SLOT( updateIsoXPeak( int ) ) );
	QObject::connect( m_cbSHLine, SIGNAL( stateChanged( int ) ), this, SLOT( updateSHLine( int ) ) );

	histogramBtnContainer_HBLayout->addWidget( sbDelta_Label );
	histogramBtnContainer_HBLayout->addWidget( sbDelta );
	histogramBtnContainer_HBLayout->addWidget( sbSigma_Label );
	histogramBtnContainer_HBLayout->addWidget( sbSigma );
	QSpacerItem* hs = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
	histogramBtnContainer_HBLayout->addItem( hs );
	histogramBtnContainer_HBLayout->addWidget( sbIsoX_Label );
	histogramBtnContainer_HBLayout->addWidget( m_sbIsoX );
	histogramBtnContainer_HBLayout->addWidget( cbSHLine_Label );
	histogramBtnContainer_HBLayout->addWidget( m_cbSHLine );
	histoBtnContainer->setLayout( histogramBtnContainer_HBLayout );
	gridLayout->addWidget( histoBtnContainer, 2, 0, 1, 2 );
}

void dlg_ParamSpaceSampling::createDescription()
{
	// Add separator
	QFrame * separator = new QFrame( this );
	separator->setGeometry( QRect( 320, 150, 118, 3 ) );
	separator->setFrameShape( QFrame::HLine );
	separator->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
							  "border-bottom-style: solid; border-left-style: none; border-color: darkGray; " );
	separator->setFrameShadow( QFrame::Sunken );
	gridLayout->addWidget( separator, 3, 0, 1, 2 );

	// Generates the description in the CommonInput dialog 
	if ( m_description )
	{
		auto info = new QTextEdit();
		info->setDocument( m_description );
		info->setFontPointSize( 20 );
		info->setReadOnly( true );
		info->setMaximumHeight( 150 );
		info->setStyleSheet( "QTextEdit {background-color: #ffffe1; margin-left:10px; font: 10pt; }" );
		gridLayout->addWidget( info, 4, 0, 1, 2 );
	}
}

void dlg_ParamSpaceSampling::updateHistoPeaks( int value )
{
	m_delta = value;
	computePeaks( m_smoothKey, m_smoothValue );
	createHistoPeaks();
	updateLineEdits();
	updateValues( m_inPara );
}

void dlg_ParamSpaceSampling::updateHistoSmooth( int value )
{
	m_sigma = value;
	computeSmoothHisto();
	computePeaks( m_smoothKey, m_smoothValue );
	createHistoPeaks();
	updateLineEdits();
	updateValues( m_inPara );
}

void dlg_ParamSpaceSampling::updateIsoXPeak( int percent )
{
	m_isoX = percent;
	createHistoPeaks();
	updateLineEdits();
	updateValues( m_inPara );
}

void dlg_ParamSpaceSampling::updateLineEdits()
{
	if ( m_filterName == "IsoXThreshold" )
	{
		// Set init line edit values  
		m_inPara[1] = m_isoXGrayValue;
		m_inPara[2] = m_isoXGrayValue;
		m_inPara[3] = 1;
	}

	if ( m_filterName == "FhwThreshold" )
	{
		// Disable isoX spin box and set color to gray
		m_sbIsoX->setStyleSheet( "QSpinBox { color: rgb(120, 120, 120); background: lightGray; }"

							   "QSpinBox::up-arrow{ background-color: transparent; border-left: 5px solid none; "
							   "border-right: 5px solid none; border-bottom: 5px solid rgb(150, 150, 150); width: 0px; height: 0px; }"

							   "QSpinBox::down-arrow{ background-color: transparent; border-left: 5px solid none; "
							   "border-right: 5px solid none; border-top: 5px solid rgb(150, 150, 150); width: 0px; height: 0px; } " );
		m_sbIsoX->setEnabled(false);

		// Set init line edit values 
		m_inPara[1] = m_airPoreGrayValue;
		m_inPara[2] = m_airPoreGrayValue;
		m_inPara[3] = 1;
		m_inPara[4] = 73;
		m_inPara[5] = 73;
		m_inPara[6] = 1;
	}
}

void dlg_ParamSpaceSampling::addUnits()
{
	m_sbIsoX->setSuffix( "%" );
	
	if ( m_filterName == "IsoXThreshold" )
	{
		m_sbIsoX->setSuffix( "%" );

		QObjectList children = m_container->children();

		for ( int i = 0; i < children.size(); i++ )
		{
			QLabel *label = dynamic_cast<QLabel*>( children.at( i ) );
			if ( label != nullptr )
			{
				if ( label->objectName().startsWith( "IsoX Start" ) ||
					 label->objectName().startsWith( "IsoX End" ) )
				{
					QString str( label->text() );
					str.append( " (%)" );
					label->setText( str );
				}
			}
		}
	}

	if ( m_filterName == "FhwThreshold" )
	{
		QObjectList children = m_container->children();

		for ( int i = 0; i < children.size(); i++ )
		{
			QLineEdit *lineEdit = dynamic_cast<QLineEdit*>( children.at( i ) );
			if ( lineEdit != nullptr )
			{
				if ( lineEdit->objectName().startsWith( "Iso50" ) )
				{
					//qDebug() << lineEdit->objectName();
					lineEdit->setStyleSheet( "QLineEdit { color: rgb(120, 120, 120); background: lightGray; }" );
					lineEdit->setEnabled( false );
				}
			}

			QLabel *label = dynamic_cast<QLabel*>( children.at( i ) );
			if ( label != nullptr )
			{
				if ( label->objectName().startsWith( "FhwWeight Start" ) ||
					 label->objectName().startsWith( "FhwWeight End" ) ||
					 label->objectName().startsWith( "Air/Pore Start" ) ||
					 label->objectName().startsWith( "Air/Pore End" ) )
				{
					QString str( label->text() );
					str.append( " (%)" );
					label->setText( str );
				}
			}
		}
	}
}

void dlg_ParamSpaceSampling::updateSHLine( int state )
{
	// Show smooth histogram line if state is true
	if ( state )
	{
		createSHLine();
	}
	else
	{
		for ( int i = 0; i < m_histoPlot->graphCount(); ++i )
		{
			if ( m_histoPlot->graph( i )->objectName() == "SHG" )
			{
				m_histoPlot->removeGraph( i );
				m_histoPlot->replot();
			}

		}
	}
}

void dlg_ParamSpaceSampling::computePeaks( QVector<double> smoothKey, QVector<double> smoothValue )
{
	// Compute histogram peaks
	double delta = m_delta;
	int length_ValueDataVector = smoothKey.length();
	m_data[0] = (double*) malloc( sizeof( double ) * smoothKey.length() );
	m_data[1] = (double*) malloc( sizeof( double ) * smoothValue.length() );

	for ( int i = 0; i < m_keyData.length(); ++i )
	{
		m_data[0][i] = smoothKey.at( i );
		m_data[1][i] = smoothValue.at( i );
	}

	detect_peak( m_data[1], length_ValueDataVector, m_emi_peaks, &m_emi_count, MAX_PEAK,
				 m_absorp_peaks, &m_absorp_count, MAX_PEAK, delta, 1 );
}

void dlg_ParamSpaceSampling::computeSmoothHisto()
{
	// Compute smoothed histogram from original histogram data
	typedef long long	PixelType1D;
	typedef itk::Image< PixelType1D, 1 > ImageType1D;
	typedef itk::ImageRegionIterator< ImageType1D>       IteratorType;
	ImageType1D::Pointer image1D = ImageType1D::New();
	ImageType1D::SizeType size1D;
	ImageType1D::IndexType start1D;
	ImageType1D::RegionType region1D;
	size1D.Fill( m_keyData.length() );
	start1D.Fill( 0 );
	region1D.SetSize( size1D );
	region1D.SetIndex( start1D );
	image1D->SetRegions( region1D );
	image1D->Allocate();
	IteratorType image1DIt( image1D, image1D->GetRequestedRegion() );
	int idx = -1;
	for ( image1DIt.GoToBegin(); !image1DIt.IsAtEnd(); ++image1DIt )
		image1DIt.Set( m_valueData[++idx] );
	
	// RecursiveGaussianImageFilter produced huge peak at the start and
	// enf of the data range, which caused problems for the peak detection
	// DiscreteGaussianImageFilter, works but the delta parameter is hard
	// to set then, therefore median filter 

	typedef itk::MedianImageFilter< ImageType1D, ImageType1D >  SmoothingFilterType;
	SmoothingFilterType::Pointer smoothingFilter = SmoothingFilterType::New();
	smoothingFilter->SetRadius( m_sigma );
	smoothingFilter->SetInput( image1D );
	smoothingFilter->Update();
	QVector<double> smoothValue;
	itk::ImageRegionConstIterator<ImageType1D> smooth1DImgIt( smoothingFilter->GetOutput(), smoothingFilter->GetOutput()->GetLargestPossibleRegion() );
	for (smooth1DImgIt.GoToBegin(); !smooth1DImgIt.IsAtEnd(); ++smooth1DImgIt)
	{
		smoothValue.append(smooth1DImgIt.Get());
	}

	m_smoothKey = m_keyData;
	m_smoothValue = smoothValue;
}

void dlg_ParamSpaceSampling::createSHLine()
{
	QCPGraph * smoothHistogramGraph = m_histoPlot->addGraph();
	smoothHistogramGraph->setObjectName( "SHG" );
	smoothHistogramGraph->setData( m_smoothKey, m_smoothValue );
	smoothHistogramGraph->setLineStyle( QCPGraph::lsLine );
	smoothHistogramGraph->setPen( QPen( QColor( "#ff0000" ), 1.0 ) );
	m_histoPlot->replot();
}

void dlg_ParamSpaceSampling::createHistoPlot()
{
	// Get highest frequency from original histogram data
	Q_ASSERT( m_valueData.size() > 0 );
	m_highestFreq = std::numeric_limits<double>::min(); // everything is >= this
	for (double v : m_valueData)
	{
		m_highestFreq = qMax(m_highestFreq, v);
	}

	// Create histogram plot
	m_histoPlot = new QCustomPlot( this );
	m_histoPlot->setMinimumHeight( 300 );
	m_histoPlot->xAxis->setLabel( "Gray value" );
	m_histoPlot->yAxis->setLabel( "Frequency" );
	m_histoPlot->xAxis->setRange( 0, 65535 );
	m_histoPlot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );

	gridLayout->setRowMinimumHeight( 0, m_histoPlot->height() );
	gridLayout->addWidget( m_histoPlot, 1, 0, 1, 2 );

	// main histogram graph
	QCPGraph * histoMainGraph = m_histoPlot->addGraph();
	histoMainGraph->setData( m_keyData, m_valueData );
	histoMainGraph->setLineStyle( QCPGraph::lsStepLeft );
	histoMainGraph->setPen( QPen( QColor( "#525252" ), 1.8 ) );
	histoMainGraph->setBrush( QBrush( QColor( "#525252" ) ) );
}

void dlg_ParamSpaceSampling::createHistoPeaks()
{
	// Remove all grapghs
	while ( m_histoPlot->graphCount() > 1 )
		m_histoPlot->removeGraph( m_histoPlot->graphCount() - 1 );
	m_histoPlot->clearItems();

	// In case of a flat peak: we take the middle of this flat peak
	QVector<int> peak_middle;
	for ( int i = 0; i < m_emi_count; ++i )
	{
		double peak_start = m_emi_peaks[i];
		double peak_runner = 0;
		double peak_end = m_emi_peaks[i];
		while ( m_smoothValue[m_emi_peaks[i] + peak_runner] == m_smoothValue[m_emi_peaks[i]] )
			peak_runner++;
		peak_end = peak_start + peak_runner - 1;
		peak_middle << floor( ( peak_end - peak_start ) / 2 + peak_start );
	}

	// Create peak graphs
	for ( int i = 0; i < m_emi_count; ++i )
	{
		QCPGraph* m_peakGraph = m_histoPlot->addGraph();
		QVector<double> pX, pY;
		pX << m_data[0][peak_middle[i]];
		m_emi_count > 1 ? pY << m_valueData[peak_middle[1]] : pY << m_valueData[peak_middle[0]];
		m_peakGraph->setData( pX, pY );
		m_peakGraph->setLineStyle( QCPGraph::lsImpulse );
		QCPItemText *textLabel = new QCPItemText( m_histoPlot );
		textLabel->setPositionAlignment( Qt::AlignTop | Qt::AlignHCenter );
		textLabel->position->setType( QCPItemPosition::ptPlotCoords );
		textLabel->position->setCoords( pX[0], pY[0] / 2 );
		textLabel->setFont( QFont( font().family(), 9 ) );
		textLabel->setPen( QPen( Qt::black ) );
		textLabel->setBrush( QBrush( QColor( 255, 255, 255, 180 ), Qt::BrushStyle::SolidPattern ) );
		
		if ( i == 0 )
		{
			m_airPoreGrayValue = pX[0];
			textLabel->setText( tr( " Air/Pore\n %1 " ).arg( m_airPoreGrayValue ) );
			m_peakGraph->setPen( QPen( QColor( Qt::blue ), 0.7, Qt::DashLine ) );
		}
		else
		{
			textLabel->setText( tr( " Material %1\n %2 " ).arg( i ).arg( pX[0] ) );
			m_peakGraph->setPen( QPen( QColor( Qt::black ), 0.7, Qt::DashLine ) );
		}
	}
	// Create IsoX peak graph
	if ( m_emi_count > 1 )
	{
		// iso X value between air/pore and material 1
		m_isoXGrayValue =
			floor( m_data[0][peak_middle[0]] +
			abs( m_data[0][peak_middle[1]] - m_data[0][peak_middle[0]] ) *
			( m_isoX / 100.0 ) );	

		QVector<double> pX, pY;
		pX << m_isoXGrayValue;
		pY << m_valueData[peak_middle[1]];	//1st material peak
		QCPGraph * peakGraph = m_histoPlot->addGraph();
		peakGraph->setData( pX, pY );
		peakGraph->setPen( QPen( QColor( Qt::gray ), 0.7, Qt::DashLine ) );
		peakGraph->setLineStyle( QCPGraph::lsImpulse );
		QCPItemText *textLabel = new QCPItemText( m_histoPlot );
		textLabel->setPositionAlignment( Qt::AlignTop | Qt::AlignHCenter );
		textLabel->position->setType( QCPItemPosition::ptPlotCoords );
		textLabel->position->setCoords( pX[0], pY[0] / 2 );
		textLabel->setText( tr( " Iso %1%\n %2 " ).arg( m_isoX ).arg( pX[0] ) );
		textLabel->setFont( QFont( font().family(), 9 ) );
		textLabel->setPen( QPen( Qt::black ) );
		textLabel->setBrush( QBrush( QColor( 255, 255, 255, 180 ), Qt::BrushStyle::SolidPattern ) );
	}

	if (m_cbSHLine->checkState())
	{
		m_cbSHLine->setCheckState(Qt::Unchecked);
	}

	// Scale to 1st material peak's height
	m_histoPlot->yAxis->setRange( 0, m_emi_count > 1 ? m_valueData[peak_middle[1]] + 20 : m_valueData[peak_middle[0]] + 20 );
	//m_histoPlot->rescaleAxes();
	m_histoPlot->replot();
}

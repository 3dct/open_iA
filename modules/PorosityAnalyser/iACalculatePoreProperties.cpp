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
#include "iACalculatePoreProperties.h"

#include "iACSVToQTableWidgetConverter.h"

#include <defines.h>
#include <io/iAITKIO.h>

#include <itkConnectedComponentImageFilter.h>
#include <itkImageBase.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>

#include <vtkIOStream.h>
#include <vtkMath.h>

#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QTableWidget>

typedef itk::ImageBase< DIM > ImageBaseType;
typedef ImageBaseType::Pointer ImagePointer;
typedef itk::ImageIOBase::IOComponentType ScalarPixelType;
typedef itk::Image<char, DIM>  InputImageType;
typedef itk::Image<long, DIM>  LabeledImageType;
typedef itk::ConnectedComponentImageFilter<InputImageType, LabeledImageType> ConnectedComponentImageFilterType;
typedef itk::LabelGeometryImageFilter< LabeledImageType > LabelGeometryImageFilterType;

iACalculatePoreProperties::iACalculatePoreProperties( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : PorePropertiesConnector( parent, f )
{
	for( int i = 0; i < numThreads; i++ )
		m_calcThread[i] = 0;

	//connect( tbOpenCSV, SIGNAL( clicked() ), this, SLOT( browseCSV() ) );
	//connect( pbRunCalculations, SIGNAL( clicked() ), this, SLOT( CalculatePoreProperties() ) );
	//LoadSettings();
}

iACalculatePoreProperties::~iACalculatePoreProperties()
{
	SaveSettings();
}

void iACalculatePoreProperties::CalculatePoreProperties()
{
	m_masks.clear();
	iACSVToQTableWidgetConverter::loadCSVFile( m_masksCSVPath, &m_masks );
	int rowCnt = m_masks.rowCount();
	int * rowRange = new int[numThreads + 1];
	for( int i = 0; i <= numThreads; i++ )
		rowRange[i] = rowCnt * ( i / (double)numThreads );
	for( int i = 0; i < numThreads; i++ )
	{
		m_splitMasks[i].clear();
		m_splitMasks[i].setRowCount( 0 );
		m_splitMasks[i].setColumnCount( m_masks.columnCount() );
		for( int r = rowRange[i]; r < rowRange[i + 1]; r++ )
		{
			int lr = m_splitMasks[i].rowCount();
			m_splitMasks[i].insertRow( lr );
			for( int c = 0; c < m_masks.columnCount(); c++ )
			{
				m_splitMasks[i].setItem( lr, c, new QTableWidgetItem( m_masks.item( r, c )->text() ) );
			}
		}
	}
	for( int i = 0; i < numThreads; i++ )
	{
		if( m_calcThread[i] )
		{
			m_calcThread[i]->terminate();
			delete m_calcThread[i];
		}
		m_calcThread[i] = new iACalculatePorePropertiesThread( &m_splitMasks[i], this );
		//(connect( m_calcThread[i], SIGNAL( totalProgress( int ) ), this, SLOT( totalProgressSlot( int ) ) );
		m_calcThread[i]->start();
	}
	delete [] rowRange;
}

void iACalculatePoreProperties::totalProgressSlot( int progress )
{
	totalProgressBar->setValue( progress );
}

void iACalculatePoreProperties::browseCSV()
{
	QString csvFile = QFileDialog::getOpenFileName( this, tr( "Masks File (CSV)" ), "", tr( "CSV Files (*.csv *.CSV)" ) );
	csvFilename->setText( csvFile );
}

void iACalculatePoreProperties::LoadSettings()
{
	QSettings settings( organisationName, applicationName );
	csvFilename->setText( settings.value( "PorosityAnalyser/PorePropertiesComputation/masksCSV", "" ).toString() );
}

void iACalculatePoreProperties::SaveSettings()
{
	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/PorePropertiesComputation/masksCSV", csvFilename->text() );
}

void iACalculatePoreProperties::SetMasksCSVPath(QString masksCSVPath)
{
	m_masksCSVPath = masksCSVPath;
}

void iACalculatePorePropertiesThread::run()
{
	try
	{
		for ( int i = 0; i < m_masks->rowCount(); ++i )
		{
			QString masksName = m_masks->item( i, 0 )->text();
			QFile f( m_masks->item( i, 0 )->text().append( ".csv" ) );
			if ( f.exists() )
			{
				//val += progIncr;
				//emit totalProgress( (int)val );
				continue;
			}

			ScalarPixelType pixelType;
			ImagePointer image = iAITKIO::readFile( masksName, pixelType, true );
			InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );
	
			//label image
			ConnectedComponentImageFilterType::Pointer connectedComponents = ConnectedComponentImageFilterType::New();
			connectedComponents->SetInput( input );
			connectedComponents->FullyConnectedOn();
			connectedComponents->Update();
			
			//Save labeled image
			QString labeledMaskName = masksName;
			labeledMaskName.insert(masksName.lastIndexOf("."), "_labeled");
			iAITKIO::writeFile(labeledMaskName, connectedComponents->GetOutput(), itk::ImageIOBase::LONG, true);

			// Writing pore csv file 
			double spacing = image->GetSpacing()[0];
			ofstream fout( getLocalEncodingFileName(masksName.append( ".csv" )).c_str(), std::ofstream::out );

			// Header of pore csv file
			fout << "Spacing" << ',' << spacing << '\n'
				 << "Voids\n"
				 << '\n'
				 << '\n'
				 << "Label Id" << ','
				 << "X1" << ','
				 << "Y1" << ','
				 << "Z1" << ','
				 << "X2" << ','
				 << "Y2" << ','
				 << "Z2" << ','
				 << "a11" << ','
				 << "a22" << ','
				 << "a33" << ','
				 << "a12" << ','
				 << "a13" << ','
				 << "a23" << ','
				 << "DimX" << ','
				 << "DimY" << ','
				 << "DimZ" << ','
				 << "phi" << ','
				 << "theta" << ','
				 << "Xm" << ','
				 << "Ym" << ','
				 << "Zm" << ','
				 << "Volume" << ','
				 << "Roundness" << ','
				 << "FeretDiam" << ','
				 << "Flatness" << ','
				 << "VoxDimX" << ','
				 << "VoxDimY" << ','
				 << "VoxDimZ" << ','
				 << "MajorLength" << ','
				 << "MinorLength" << ','
				 << '\n';

			// Initalisation of itk::LabelGeometryImageFilter for calculating pore parameters
			LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
			labelGeometryImageFilter->SetInput( connectedComponents->GetOutput() );
			// These generate optional outputs.
			//labelGeometryImageFilter->CalculatePixelIndicesOn();
			//labelGeometryImageFilter->CalculateOrientedBoundingBoxOn();
			//labelGeometryImageFilter->CalculateOrientedLabelRegionsOn();
			//labelGeometryImageFilter->CalculateOrientedIntensityRegionsOn();
			labelGeometryImageFilter->Update();

			// Initalisation of itk::LabelImageToShapeLabelMapFilter for calculating other pore parameters
			typedef unsigned long LabelType;
			typedef itk::ShapeLabelObject<LabelType, DIM>	ShapeLabelObjectType;
			typedef itk::LabelMap<ShapeLabelObjectType>	LabelMapType;
			typedef itk::LabelImageToShapeLabelMapFilter<LabeledImageType, LabelMapType> I2LType;
			I2LType::Pointer i2l = I2LType::New();
			i2l->SetInput( connectedComponents->GetOutput() );
			i2l->SetComputePerimeter( false );
			i2l->SetComputeFeretDiameter( true );	//Debug changed to false 
			i2l->Update();

			LabelMapType *labelMap = i2l->GetOutput();
			LabelGeometryImageFilterType::LabelsType allLabels = labelGeometryImageFilter->GetLabels();
			LabelGeometryImageFilterType::LabelsType::iterator allLabelsIt;

			// Pore Characteristics calculation 
			for ( allLabelsIt = allLabels.begin(); allLabelsIt != allLabels.end(); allLabelsIt++ )
			{
				LabelGeometryImageFilterType::LabelPixelType labelValue = *allLabelsIt;
				if ( labelValue == 0 )	// label 0 = backround
					continue;

				std::vector<double> eigenvalue( 3 );
				std::vector<double> eigenvector( 3 );
				std::vector<double> centroid( 3 );
				int dimX, dimY, dimZ;
				double x1, x2, y1, y2, z1, z2, xm, ym, zm, phi, theta, a11, a22, a33, a12, a13, a23,
					majorlength, minorlength, half_length, dx, dy, dz;

				// Calculating start and and point of the pores's major principal axis
				eigenvalue = labelGeometryImageFilter->GetEigenvalues( labelValue );
				auto maxEigenvalue = std::max_element( std::begin( eigenvalue ), std::end( eigenvalue ) );
				int maxEigenvaluePos = std::distance( std::begin( eigenvalue ), maxEigenvalue );

				eigenvector[0] = labelGeometryImageFilter->GetEigenvectors( labelValue )[0][maxEigenvaluePos];
				eigenvector[1] = labelGeometryImageFilter->GetEigenvectors( labelValue )[1][maxEigenvaluePos];
				eigenvector[2] = labelGeometryImageFilter->GetEigenvectors( labelValue )[2][maxEigenvaluePos];
				centroid[0] = labelGeometryImageFilter->GetCentroid( labelValue )[0];
				centroid[1] = labelGeometryImageFilter->GetCentroid( labelValue )[1];
				centroid[2] = labelGeometryImageFilter->GetCentroid( labelValue )[2];
				half_length = labelGeometryImageFilter->GetMajorAxisLength( labelValue ) / 2.0;
				x1 = centroid[0] + half_length * eigenvector[0];
				y1 = centroid[1] + half_length * eigenvector[1];
				z1 = centroid[2] + half_length * eigenvector[2];
				x2 = centroid[0] - half_length * eigenvector[0];
				y2 = centroid[1] - half_length * eigenvector[1];
				z2 = centroid[2] - half_length * eigenvector[2];

				// Preparing orientation and tensor calculation
				dx = x1 - x2;
				dy = y1 - y2;
				dz = z1 - z2;
				xm = ( x1 + x2 ) / 2.0f;
				ym = ( y1 + y2 ) / 2.0f;
				zm = ( z1 + z2 ) / 2.0f;

				if ( dz < 0 )
				{
					dx = x2 - x1;
					dy = y2 - y1;
					dz = z2 - z1;
				}

				phi = asin( dy / sqrt( dx*dx + dy*dy ) );
				theta = acos( dz / sqrt( dx*dx + dy*dy + dz*dz ) );
				a11 = cos( phi )*cos( phi )*sin( theta )*sin( theta );
				a22 = sin( phi )*sin( phi )*sin( theta )*sin( theta );
				a33 = cos( theta )*cos( theta );
				a12 = cos( phi )*sin( theta )*sin( theta )*sin( phi );
				a13 = cos( phi )*sin( theta )*cos( theta );
				a23 = sin( phi )*sin( theta )*cos( theta );

				phi = ( phi*180.0f ) / vtkMath::Pi();
				theta = ( theta*180.0f ) / vtkMath::Pi();

				// Locating the phi value to quadrant
				if ( dx < 0 )
					phi = 180.0 - phi;

				if ( phi < 0.0 )
					phi = phi + 360.0;

				if ( dx == 0 && dy == 0 )
				{
					phi = 0.0;
					theta = 0.0;
					a11 = 0.0;
					a22 = 0.0;
					a12 = 0.0;
					a13 = 0.0;
					a23 = 0.0;
				}

				majorlength = labelGeometryImageFilter->GetMajorAxisLength( labelValue );
				minorlength = labelGeometryImageFilter->GetMinorAxisLength( labelValue );
				dimX = abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[0] - labelGeometryImageFilter->GetBoundingBox( labelValue )[1] ) + 1;
				dimY = abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[2] - labelGeometryImageFilter->GetBoundingBox( labelValue )[3] ) + 1;
				dimZ = abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[4] - labelGeometryImageFilter->GetBoundingBox( labelValue )[5] ) + 1;

				// Calculation of other pore characteristics and writing the csv file 
				ShapeLabelObjectType *labelObject = labelMap->GetNthLabelObject( labelValue - 1 ); // debug -1 delated	// labelMap index contaions first pore at 0 

				/* The equivalent radius is a radius of a circle with the same area as the object.
				The feret diameter is the diameter of circumscribing circle. So this measure has a maximum of 1.0 when the object is a perfect circle.
				http://public.kitware.com/pipermail/insight-developers/2011-April/018466.html */

				if (labelObject->GetFeretDiameter() == 0)
				{
					labelObject->SetRoundness(0.0);
				}
				else
				{
					labelObject->SetRoundness(labelObject->GetEquivalentSphericalRadius() 
						/ (labelObject->GetFeretDiameter() / 2.0));
				}

				fout << labelValue << ','
					 << x1 * spacing << ',' 	// unit = microns
					 << y1 * spacing << ',' 	// unit = microns
					 << z1 * spacing << ',' 	// unit = microns
					 << x2 * spacing << ','		// unit = microns
					 << y2 * spacing << ','		// unit = microns
					 << z2 * spacing << ','		// unit = microns
					 << a11 << ','
					 << a22 << ','
					 << a33 << ','
					 << a12 << ','
					 << a13 << ','
					 << a23 << ','
					 << dimX * spacing << ','	// unit = microns
					 << dimY * spacing << ','	// unit = microns
					 << dimZ * spacing << ','	// unit = microns
					 << phi << ','				// unit = °
					 << theta << ','			// unit = °
					 << xm * spacing << ',' 	// unit = microns
					 << ym * spacing << ',' 	// unit = microns
					 << zm * spacing << ',' 	// unit = microns
					 << labelGeometryImageFilter->GetVolume( labelValue ) * pow( spacing, 3.0 ) << ','	// unit = microns^3
					 << labelObject->GetRoundness() << ','
					 << labelObject->GetFeretDiameter() << ','	// unit = microns
					 << labelObject->GetFlatness() << ','
					 << dimX << ','		// unit = voxels
					 << dimY << ','		// unit = voxels
					 << dimZ << ','		// unit = voxels
					 << majorlength * spacing << ',' 	// unit = microns
					 << minorlength * spacing << ',' 	// unit = microns
					 << '\n';
			}
			fout.close();
		}
	}
	catch( itk::ExceptionObject &excep )
	{
		QString tolog = tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() );
		qDebug() << tolog;
	}
}

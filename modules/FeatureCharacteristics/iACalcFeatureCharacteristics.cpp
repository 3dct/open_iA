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
#include "iACalcFeatureCharacteristics.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iATypedCallHelper.h"
#include "iAProgressToQtSignal.h"

#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLabelGeometryImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

template<class T> int calcFeatureCharacteristics_template( iAConnector *image, iAProgressToQtSignal & progress,  QString pathCSV, bool feretDiameter )
{
	// Cast iamge to type long
	typedef itk::Image< T, DIM > InputImageType;
	typename InputImageType::Pointer inputImage;
	inputImage = dynamic_cast<InputImageType *>( image->GetITKImage() );
	typedef itk::Image<long, DIM> LongImageType;
	typename LongImageType::Pointer longImage;
	typedef itk::CastImageFilter<InputImageType, LongImageType> CIFType;
	typename CIFType::Pointer castfilter = CIFType::New();
	castfilter->SetInput( inputImage );
	castfilter->Update();
	longImage = castfilter->GetOutput();

	// Writing pore csv file 
	double spacing = longImage->GetSpacing()[0];
	ofstream fout( pathCSV.toStdString(), std::ofstream::out );

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
		//<< "Shape factor" << ','
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
	typedef itk::LabelGeometryImageFilter<LongImageType> LabelGeometryImageFilterType;
	LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
	labelGeometryImageFilter->SetInput( longImage );

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
	typedef itk::LabelImageToShapeLabelMapFilter<LongImageType, LabelMapType> I2LType;
	I2LType::Pointer i2l = I2LType::New();
	i2l->SetInput( longImage );
	i2l->SetComputePerimeter( false );
	i2l->SetComputeFeretDiameter( feretDiameter );
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

		phi = ( phi*180.0f ) / M_PI;
		theta = ( theta*180.0f ) / M_PI;

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
		ShapeLabelObjectType *labelObject = labelMap->GetNthLabelObject( labelValue -1); // debug -1 delated	// labelMap index contaions first pore at 0 

		/* The equivalent radius is a radius of a circle with the same area as the object.
		The feret diameter is the diameter of circumscribing circle. So this measure has a maximum of 1.0 when the object is a perfect circle.
		http://public.kitware.com/pipermail/insight-developers/2011-April/018466.html */
		
		if ( labelObject->GetFeretDiameter() == 0 )
			labelObject->SetRoundness( 0.0 );
		else
			labelObject->SetRoundness( labelObject->GetEquivalentSphericalRadius() / ( labelObject->GetFeretDiameter() / 2.0 ) );

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
			//<< poresPtr->operator[]( it->first ).getShapeFactor() << ','	//no that correct -> see roundness
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

		progress.emitProgress(round(labelValue * 100 / allLabels.size()));
	}
	fout.close();
	return EXIT_SUCCESS;
}

iACalcFeatureCharacteristics::iACalcFeatureCharacteristics( QString fn,
	vtkImageData* i, vtkPolyData* p, iALogger* logger, MdiChild* parent, QString path,
	bool calculateFeretDiameter)
:
	iAAlgorithm( fn, i, p, logger, parent ),
	image(i),
	pathCSV(path),
	m_calculateFeretDiameter(calculateFeretDiameter),
	m_mdiChild(parent)
{}

void iACalcFeatureCharacteristics::run()
{
	calcFeatureCharacteristics();
}

void iACalcFeatureCharacteristics::calcFeatureCharacteristics()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );
	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		iAProgressToQtSignal progress;
		connect(&progress, SIGNAL(progress(int)), m_mdiChild, SLOT(updateProgressBar(int)));

		ITK_TYPED_CALL( calcFeatureCharacteristics_template, itkType,
			getConnector(), progress, pathCSV, m_calculateFeretDiameter);
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}

	addMsg( tr( "%1   Feature csv file created in: %2" )
			.arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( pathCSV ) );

	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" )
			.arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );
}

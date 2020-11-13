/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iACalcFeatureCharacteristics.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>
#include <io/iAFileUtils.h>

#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLabelGeometryImageFilter.h>

#include <vtkImageData.h>
#include <vtkMath.h>

template<class T> void calcFeatureCharacteristics_template(iAConnector *image, iAProgress* progress,
	QString pathCSV, bool feretDiameter, bool calculateAdvancedChars, bool calculateRoundness)
{
	// Cast image to type long
	typedef itk::Image< T, DIM > InputImageType;
	typename InputImageType::Pointer inputImage;
	inputImage = dynamic_cast<InputImageType *>( image->itkImage() );
	typedef itk::Image<long, DIM> LongImageType;
	typename LongImageType::Pointer longImage;
	typedef itk::CastImageFilter<InputImageType, LongImageType> CIFType;
	typename CIFType::Pointer castfilter = CIFType::New();
	castfilter->SetInput( inputImage );
	castfilter->Update();
	longImage = castfilter->GetOutput();

	// Writing pore csv file
	double spacing = longImage->GetSpacing()[0];
	std::ofstream fout( getLocalEncodingFileName(pathCSV), std::ofstream::out );

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
		<< "MinorLength" << ',';

		if (calculateAdvancedChars)
		{
			fout << "Elongation" << ','
				<< "Perimeter" << ','
				<< "EquivalentSphericalRadius" << ','
				<< "MiddleAxisLength" << ","
				//<< "Sphericity" << ","
				//<< "Surface " << ",";
				/*<< "RadiusManually" << ","*/
				<< "RatioAxisLongToAxisMiddle" << ","
				<< "RatioMiddleToSmallest" << ","
				<< "Dir2_X1" << "," << "Dir2_Y1" << "," << "Dir2_Z1" << ","
				<< "Dir2_X2" << "," << "Dir_Y2" << "," << "Dir2_Z2" << ",";
		}
		fout << '\n';

	// Initalisation of itk::LabelGeometryImageFilter for calculating pore parameters
	typedef itk::LabelGeometryImageFilter<LongImageType> LabelGeometryImageFilterType;
	typename LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
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
	typename I2LType::Pointer i2l = I2LType::New();
	i2l->SetInput( longImage );
	i2l->SetComputePerimeter(calculateAdvancedChars);
	i2l->SetComputeFeretDiameter(feretDiameter);
	i2l->Update();

	LabelMapType *labelMap = i2l->GetOutput();
	typename LabelGeometryImageFilterType::LabelsType allLabels = labelGeometryImageFilter->GetLabels();
	typename LabelGeometryImageFilterType::LabelsType::iterator allLabelsIt;

	// Pore Characteristics calculation
	for ( allLabelsIt = allLabels.begin(); allLabelsIt != allLabels.end(); allLabelsIt++ )
	{
		typename LabelGeometryImageFilterType::LabelPixelType labelValue = *allLabelsIt;
		if ( labelValue == 0 )	// label 0 = background
		{
			continue;
		}

		std::vector<double> eigenvalue( 3 );
		std::vector<double> eigenvector( 3 );
		std::vector<double> centroid( 3 );
		int dimX, dimY, dimZ;
		double x1, x2, y1, y2, z1, z2, xm, ym, zm, phi, theta, a11, a22, a33, a12, a13, a23,
			majorlength, minorlength, half_length, dx, dy, dz;

		// Calculating start and and point of the pore's major principal axis
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
		{
			phi = 180.0 - phi;
		}
		if ( phi < 0.0 )
		{
			phi = phi + 360.0;
		}
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

		// apparently the "roundness" delivered by the filter (GetRoundness), is not really reliable
		// (values up to 2 when it should only produce values up to 1)
		// So we use the computation as proposed in
		// http://public.kitware.com/pipermail/insight-developers/2011-April/018466.html:
		// The equivalent radius is a radius of a circle with the same area as the object.
		// The feret diameter is the diameter of circumscribing circle.
		// So this measure has a maximum of 1.0 when the object is a perfect circle:
		double roundness = (labelObject->GetFeretDiameter() > 0) ?
			labelObject->GetEquivalentSphericalRadius() / (labelObject->GetFeretDiameter() / 2.0) :
			(calculateRoundness ?
				labelObject->GetRoundness() :
				0.0);

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
			<< labelGeometryImageFilter->GetVolume(labelValue)* pow(spacing, 3.0) << ','	// unit = microns^3
			<< roundness << ','
			<< labelObject->GetFeretDiameter() << ','	// unit = microns
			<< labelObject->GetFlatness() << ','
			<< dimX << ','		// unit = voxels
			<< dimY << ','		// unit = voxels
			<< dimZ << ','		// unit = voxels
			<< majorlength * spacing << ',' 	// unit = microns
			<< minorlength * spacing << ','; 	// unit = microns

		if (calculateAdvancedChars)
		{
			//double sphericity = std::pow(vtkMath::Pi(), 1.0 / 3.0) * std::pow(6.0 * labelGeometryImageFilter->GetVolume(labelValue) * pow(spacing, 3.0), 2.0 / 3.0) / perimeter;
			//double surface = 4.0 * vtkMath::Pi() *std::pow(equivSphericalRadius/**spacing*/,2.0);
			//double sphericalRadiusManually = std::pow((6.0 / vtkMath::Pi() * labelGeometryImageFilter->GetVolume(labelValue) * pow(spacing, 3.0)), 1 / 3);
				//std::pow(labelGeometryImageFilter->GetVolume(labelValue) * pow(spacing, 3.0) / (4.0 / 3.0 * vtkMath::Pi()), 1.0/3.0);  // Vsphere =  4/3*pI*r^3
			double elongation = labelGeometryImageFilter->GetElongation(labelValue);
			double perimeter = labelObject->GetPerimeter();
			double secondAxisLengh = 4 * sqrt(eigenvalue[1]); //second prinzipal axis
			double equivSphericalRadius = labelObject->GetEquivalentSphericalRadius();
			double ratioLongestToMiddle = majorlength / secondAxisLengh;
			double ratioMiddleToSmallest = secondAxisLengh / minorlength;

			std::vector<double> eigenvector_middle(3);
			int EWPos = 1; //should be lambda2, lambda1 < lambda2 < lambda3

			//represents second principal axis
			eigenvector_middle[0] = labelGeometryImageFilter->GetEigenvectors(labelValue)[0][EWPos];
			eigenvector_middle[1] = labelGeometryImageFilter->GetEigenvectors(labelValue)[1][EWPos];
			eigenvector_middle[2] = labelGeometryImageFilter->GetEigenvectors(labelValue)[2][EWPos];

			double half_axis2 =/* minorlength*/ secondAxisLengh / 2.0;

			//p1 and px2 vector obtained by second eigenvector
			double p_x1 = centroid[0] + half_axis2 * eigenvector_middle[0];
			double p_y1 = centroid[1] + half_axis2 * eigenvector_middle[1];
			double p_z1 = centroid[2] + half_axis2 * eigenvector_middle[2];
			double p_x2 = centroid[0] - half_axis2 * eigenvector_middle[0];
			double p_y2 = centroid[1] - half_axis2 * eigenvector_middle[1];
			double p_z2 = centroid[2] - half_axis2 * eigenvector_middle[2];

			fout << elongation << ','
				<< perimeter/**spacing*/ << ','
				<< equivSphericalRadius/**spacing */ << ','
				<< secondAxisLengh * spacing << ","
				<< ratioLongestToMiddle << ","
				<< ratioMiddleToSmallest << ",";
			fout << p_x1*spacing << "," << p_y1*spacing << "," << p_z1*spacing << ","
				 << p_x2*spacing << "," << p_y2*spacing << "," << p_z2*spacing << ",";

		}
		fout << '\n';

		progress->emitProgress(labelValue * 100.0 / allLabels.size());
	}

	fout.close();
}

iACalcFeatureCharacteristics::iACalcFeatureCharacteristics():
	iAFilter("Calculate Feature Characteristics", "Feature Characteristics",
		"Compute characteristics of the objects in a labelled dataset.<br/>"
		"This filter takes a labelled image as input, and writes a table of the "
		"characteristics of each of the features (=objects) in this image to  csv file with the given <em>Output CSV filename</em>."
		"If you need a precise diameter, enable <em>Calculate Feret Diameter</em> "
		"(but note that this increases computation time significantly!). "
		"Note that the Feret Diameter is also required to compute an accurate roundness. "
		"If you disable the Feret Diameter, an inaccurate roundness is provided (which can go over 1). "
		"If you want to disable this roundness, disable <em>Calculate roundness</em>, "
		"this will set roundness to 0 if no feret diameter available .<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1LabelGeometryImageFilter.html\">"
		"Label Geometry Image Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1LabelImageToShapeLabelMapFilter.html\">"
		"Label Image to Shape Label Map Filter </a> "
		"in the ITK documentation.", 1, 0)
{
	addParameter("Output CSV filename", iAValueType::FileNameSave, ".csv");
	addParameter("Calculate Feret Diameter", iAValueType::Boolean, false);
	addParameter("Calculate roundness", iAValueType::Boolean, false);
	addParameter("Calculate advanced void parameters", iAValueType::Boolean, false);
}

IAFILTER_CREATE(iACalcFeatureCharacteristics)

void iACalcFeatureCharacteristics::performWork(QMap<QString, QVariant> const & parameters)
{
	QString pathCSV = parameters["Output CSV filename"].toString();
	ITK_TYPED_CALL(calcFeatureCharacteristics_template, inputPixelType(), input()[0], progress(), pathCSV,
		parameters["Calculate Feret Diameter"].toBool(), parameters["Calculate advanced void parameters"].toBool(), parameters["Calculate roundness"].toBool());
	addMsg(QString("Feature csv file created in: %1").arg(pathCSV));
}

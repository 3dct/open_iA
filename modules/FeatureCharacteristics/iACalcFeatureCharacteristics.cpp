// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iAFilterDefault.h>

IAFILTER_DEFAULT_CLASS(iACalcFeatureCharacteristics);

#include <defines.h>          // for DIM
#include <iALog.h>
#include <iAProgress.h>

// base
#include <iAImageData.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLabelGeometryImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>

#include <numbers>

template<class T> void calcFeatureCharacteristics(itk::ImageBase<3>* itkImg, iAProgress* progress,
	QString pathCSV, bool feretDiameter, bool calculateAdvancedChars, bool calculateRoundness)
{
	// Cast image to type long
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image<long, DIM> LongImageType;
	typename InputImageType::Pointer inputImage = dynamic_cast<InputImageType *>(itkImg);
	auto castfilter = itk::CastImageFilter<InputImageType, LongImageType>::New();
	castfilter->SetInput( inputImage );
	castfilter->Update();
	typename LongImageType::Pointer longImage = castfilter->GetOutput();

	// Writing pore csv file
	double spacing = longImage->GetSpacing()[0];
	std::ofstream fout( pathCSV.toStdString());

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
	auto labelGeometryImageFilter = itk::LabelGeometryImageFilter<LongImageType>::New();
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
	auto allLabels = labelGeometryImageFilter->GetLabels();

	// Pore Characteristics calculation
	for (auto labelValue: allLabels)
	{
		if ( labelValue == 0 )	// label 0 = background
		{
			continue;
		}
		// Calculating start and and point of the pore's major principal axis
		auto eigenvalue = labelGeometryImageFilter->GetEigenvalues(labelValue);
		auto maxEigenvalue = std::max_element( std::begin( eigenvalue ), std::end( eigenvalue ) );
		auto maxEigenvaluePos = std::distance( std::begin( eigenvalue ), maxEigenvalue );

		std::vector<double> eigenvector(3);
		eigenvector[0] = labelGeometryImageFilter->GetEigenvectors( labelValue )[0][maxEigenvaluePos];
		eigenvector[1] = labelGeometryImageFilter->GetEigenvectors( labelValue )[1][maxEigenvaluePos];
		eigenvector[2] = labelGeometryImageFilter->GetEigenvectors( labelValue )[2][maxEigenvaluePos];
		std::vector<double> centroid(3);
		centroid[0] = labelGeometryImageFilter->GetCentroid( labelValue )[0];
		centroid[1] = labelGeometryImageFilter->GetCentroid( labelValue )[1];
		centroid[2] = labelGeometryImageFilter->GetCentroid( labelValue )[2];

		double half_length = labelGeometryImageFilter->GetMajorAxisLength( labelValue ) / 2.0;

		double x1 = centroid[0] + half_length * eigenvector[0];
		double y1 = centroid[1] + half_length * eigenvector[1];
		double z1 = centroid[2] + half_length * eigenvector[2];
		double x2 = centroid[0] - half_length * eigenvector[0];
		double y2 = centroid[1] - half_length * eigenvector[1];
		double z2 = centroid[2] - half_length * eigenvector[2];

		// Preparing orientation and tensor calculation
		double dx = x1 - x2;
		double dy = y1 - y2;
		double dz = z1 - z2;
		double xm = ( x1 + x2 ) / 2.0f;
		double ym = ( y1 + y2 ) / 2.0f;
		double zm = ( z1 + z2 ) / 2.0f;

		if ( dz < 0 )
		{
			dx = x2 - x1;
			dy = y2 - y1;
			dz = z2 - z1;
		}

		double phi = std::asin( dy / std::sqrt( dx*dx + dy*dy ) );
		double theta = std::acos( dz / std::sqrt( dx*dx + dy*dy + dz*dz ) );
		double a11 = std::cos( phi )*std::cos( phi )*std::sin( theta )*std::sin( theta );
		double a22 = std::sin( phi )*std::sin( phi )*std::sin( theta )*std::sin( theta );
		double a33 = std::cos( theta )*std::cos( theta );
		double a12 = std::cos( phi )*std::sin( theta )*std::sin( theta )*std::sin( phi );
		double a13 = std::cos( phi )*std::sin( theta )*std::cos( theta );
		double a23 = std::sin( phi )*std::sin( theta )*std::cos( theta );

		phi = ( phi*180.0f ) / std::numbers::pi;
		theta = ( theta*180.0f ) / std::numbers::pi;

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
		double majorlength = labelGeometryImageFilter->GetMajorAxisLength( labelValue );
		double minorlength = labelGeometryImageFilter->GetMinorAxisLength( labelValue );
		auto dimX = std::abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[0] - labelGeometryImageFilter->GetBoundingBox( labelValue )[1] ) + 1;
		auto dimY = std::abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[2] - labelGeometryImageFilter->GetBoundingBox( labelValue )[3] ) + 1;
		auto dimZ = std::abs( labelGeometryImageFilter->GetBoundingBox( labelValue )[4] - labelGeometryImageFilter->GetBoundingBox( labelValue )[5] ) + 1;

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
			<< labelGeometryImageFilter->GetVolume(labelValue)* std::pow(spacing, 3.0) << ','	// unit = microns^3
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
			//double sphericity = std::pow(std::numbers::pi, 1.0 / 3.0) * std::pow(6.0 * labelGeometryImageFilter->GetVolume(labelValue) * std::pow(spacing, 3.0), 2.0 / 3.0) / perimeter;
			//double surface = 4.0 * std::numbers::pi *std::pow(equivSphericalRadius/**spacing*/,2.0);
			//double sphericalRadiusManually = std::pow((6.0 / std::numbers::pi * labelGeometryImageFilter->GetVolume(labelValue) * std::pow(spacing, 3.0)), 1 / 3);
				//std::pow(labelGeometryImageFilter->GetVolume(labelValue) * std::pow(spacing, 3.0) / (4.0 / 3.0 * std::numbers::pi), 1.0/3.0);  // Vsphere =  4/3*pI*r^3
			double elongation = labelGeometryImageFilter->GetElongation(labelValue);
			double perimeter = labelObject->GetPerimeter();
			double secondAxisLengh = 4 * std::sqrt(eigenvalue[1]); //second prinzipal axis
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

void iACalcFeatureCharacteristics::performWork(QVariantMap const & parameters)
{
	QString pathCSV = parameters["Output CSV filename"].toString();
	ITK_TYPED_CALL(calcFeatureCharacteristics, inputScalarType(), imageInput(0)->itkImage(), progress(), pathCSV,
		parameters["Calculate Feret Diameter"].toBool(), parameters["Calculate advanced void parameters"].toBool(), parameters["Calculate roundness"].toBool());
	LOG(lvlInfo, QString("Feature csv file created in: %1").arg(pathCSV));
}

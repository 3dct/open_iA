// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iAFilterDefault.h>

IAFILTER_DEFAULT_CLASS(iACalcFeatureCharacteristics);

#include <defines.h>          // for DIM
#include <iALog.h>
#include <iAProgress.h>
#include <iAVec3.h>

// base
#include <iAImageData.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkCastImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
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
	double spc = longImage->GetSpacing()[0];
	std::ofstream fout( pathCSV.toStdString());

	// Header of pore csv file
	fout << "Spacing," << spc << '\n'
		<< "Voids\n\n\n"
		<< "Label Id,"
		<< "X1,Y1,Z1,"
		<< "X2,Y2,Z2,"
		<< "a11,a22,a33,a12,a13,a23,"
		<< "DimX,DimY,DimZ,"
		<< "phi,theta,"
		<< "Xm,Ym,Zm,"
		<< "Volume,"
		<< "Roundness,"
		<< "FeretDiam,"
		<< "Flatness,"
		<< "VoxDimX,"
		<< "VoxDimY,"
		<< "VoxDimZ,"
		<< "MajorLength,"
		<< "MinorLength,";

		if (calculateAdvancedChars)
		{
			fout << "Elongation,"
				<< "Perimeter,"
				<< "EquivalentSphericalRadius,"
				<< "MiddleAxisLength,"
				<< "RatioAxisLongToAxisMiddle,"
				<< "RatioMiddleToSmallest,"
				<< "Dir2_X1,Dir2_Y1,Dir2_Z1,"
				<< "Dir2_X2,Dir2_Y2,Dir2_Z2,";
		}
		fout << '\n';
	typedef unsigned long LabelType;
	typedef itk::ShapeLabelObject<LabelType, DIM>	ShapeLabelObjectType;
	typedef itk::LabelMap<ShapeLabelObjectType>	LabelMapType;
	typedef itk::LabelImageToShapeLabelMapFilter<LongImageType, LabelMapType> I2LType;
	typename I2LType::Pointer i2l = I2LType::New();
	i2l->SetInput( longImage );
	i2l->SetComputePerimeter(calculateAdvancedChars);
	i2l->SetComputeFeretDiameter(feretDiameter);
	i2l->SetComputeOrientedBoundingBox(true);
	progress->observe(i2l);
	progress->setStatus("Computing feature maps");
	i2l->Update();

	LabelMapType *labelMap = i2l->GetOutput();
	progress->setStatus("Computing individual characteristics");
	for (itk::SizeValueType labelValue = 0; labelValue < labelMap->GetNumberOfLabelObjects(); ++labelValue)
	{
		ShapeLabelObjectType* labelObject = labelMap->GetNthLabelObject(labelValue);
		iAVec3d centroid(labelObject->GetCentroid().data());
		auto const& bb = labelObject->GetBoundingBox();
		auto const& obbsize = labelObject->GetOrientedBoundingBoxSize();
		//labelObject->Get
		double majorlength = obbsize[2];
		double minorlength = obbsize[1];
		auto bbsize = bb.GetSize().data();
		double half_length = majorlength / 2.0;
		auto const & eigenvectors = labelObject->GetPrincipalAxes();
		auto const& eigenvalues = labelObject->GetPrincipalMoments();
		const auto maxEVPos = 2;
		// inverse direction to keep results comparable to results from before with LabelGeometry filter:
		iAVec3d majDirEV(-eigenvectors[maxEVPos][0], -eigenvectors[maxEVPos][1], -eigenvectors[maxEVPos][2]);
		LOG(lvlDebug, QString("%1 - ").arg(labelValue) +
			QString("vec: %1, %2, %3; %4, %5, %6; %7, %8, %9")
			.arg(eigenvectors[0][0]).arg(eigenvectors[0][1]).arg(eigenvectors[0][2])
			.arg(eigenvectors[1][0]).arg(eigenvectors[1][1]).arg(eigenvectors[1][2])
			.arg(eigenvectors[2][0]).arg(eigenvectors[2][1]).arg(eigenvectors[2][2]) +
			QString("majDirEV: %1; val: %2, %3, %4").arg(majDirEV.toString())
			.arg(eigenvalues[0]).arg(eigenvalues[1]).arg(eigenvalues[2]));
		auto pt1 = centroid + half_length * majDirEV.normalized();
		auto pt2 = centroid - half_length * majDirEV.normalized();
		auto dPt = pt1 - pt2;
		if ( dPt.z() < 0 )
		{
			dPt = pt2 - pt1;
		}
		double phi   = std::asin(dPt.y() / std::sqrt(dPt.x() * dPt.x() + dPt.y() * dPt.y()) );
		double theta = std::acos(dPt.z() / std::sqrt(dPt.x() * dPt.x() + dPt.y() * dPt.y() + dPt.z() * dPt.z()) );
		double a11 = std::cos( phi )*std::cos( phi )*std::sin( theta )*std::sin( theta );
		double a22 = std::sin( phi )*std::sin( phi )*std::sin( theta )*std::sin( theta );
		double a33 = std::cos( theta )*std::cos( theta );
		double a12 = std::cos( phi )*std::sin( theta )*std::sin( theta )*std::sin( phi );
		double a13 = std::cos( phi )*std::sin( theta )*std::cos( theta );
		double a23 = std::sin( phi )*std::sin( theta )*std::cos( theta );

		phi = ( phi*180.0f ) / std::numbers::pi;
		theta = ( theta*180.0f ) / std::numbers::pi;

		// Locating the phi value to quadrant
		if ( dPt.x() < 0 )
		{
			phi = 180.0 - phi;
		}
		if ( phi < 0.0 )
		{
			phi = phi + 360.0;
		}
		if ( dPt.x() == 0 && dPt.y() == 0)
		{
			phi = 0.0;
			theta = 0.0;
			a11 = 0.0;
			a22 = 0.0;
			a12 = 0.0;
			a13 = 0.0;
			a23 = 0.0;
		}

		// Calculation of other pore characteristics and writing the csv file

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

		fout << labelValue+1 << ','
			<< pt1.x() << ',' << pt1.y() << ',' << pt1.z() << ','     // physical units
			<< pt2.x() << ',' << pt2.y() << ',' << pt2.z() << ','     // physical units
			<< a11 << ','<< a22 << ','<< a33 << ','
			<< a12 << ','<< a13 << ','<< a23 << ','
			<< bbsize[0] * spc << ','              // physical units
			<< bbsize[1] * spc << ','              // physical units
			<< bbsize[2] * spc << ','              // physical units
			<< phi << ',' << theta << ','              // unit = °
			<< centroid.x() << ',' << centroid.y() << ',' << centroid.z() << ','     // physical units
			<< labelObject->GetPhysicalSize() << ','   // physical units
			<< roundness << ','
			<< labelObject->GetFeretDiameter() << ','  // physical units
			<< labelObject->GetFlatness() << ','
			<< bbsize[0] << ','	<< bbsize[1] << ',' << bbsize[2] << ','	// unit = voxels
			<< majorlength << ',' << minorlength << ',';                // physical units

		if (calculateAdvancedChars)
		{
			double elongation = labelObject->GetElongation();
			double perimeter = labelObject->GetPerimeter();
			double secondAxisLengh = 4 * std::sqrt(eigenvalues[1]); //second principal axis
			double equivSphericalRadius = labelObject->GetEquivalentSphericalRadius();
			double ratioLongestToMiddle = majorlength / secondAxisLengh;
			double ratioMiddleToSmallest = secondAxisLengh / minorlength;

			std::vector<double> eigenvector_middle(3);
			int EWPos = 1; //should be lambda2, lambda1 < lambda2 < lambda3

			//represents second principal axis
			eigenvector_middle[0] = eigenvectors[0][EWPos];
			eigenvector_middle[1] = eigenvectors[1][EWPos];
			eigenvector_middle[2] = eigenvectors[2][EWPos];

			double half_axis2 =/* minorlength*/ secondAxisLengh / 2.0;

			//p1 and px2 vector obtained by second eigenvector
			double p_x1 = centroid[0] + half_axis2 * eigenvector_middle[0];
			double p_y1 = centroid[1] + half_axis2 * eigenvector_middle[1];
			double p_z1 = centroid[2] + half_axis2 * eigenvector_middle[2];
			double p_x2 = centroid[0] - half_axis2 * eigenvector_middle[0];
			double p_y2 = centroid[1] - half_axis2 * eigenvector_middle[1];
			double p_z2 = centroid[2] - half_axis2 * eigenvector_middle[2];

			fout << elongation << ','
				<< perimeter << ','
				<< equivSphericalRadius << ','
				<< secondAxisLengh * spc << ","
				<< ratioLongestToMiddle << ","
				<< ratioMiddleToSmallest << ",";
			fout << p_x1*spc << "," << p_y1*spc << "," << p_z1*spc << ","
				 << p_x2*spc << "," << p_y2*spc << "," << p_z2*spc << ",";
		}
		fout << '\n';

		progress->emitProgress(labelValue * 100.0 / labelMap->GetNumberOfLabelObjects());
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

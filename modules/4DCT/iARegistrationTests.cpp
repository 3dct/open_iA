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
#include "iARegistrationTests.h"
// iA
#include "ImageFilterDrawTokens.h"
// itk
#include <itkImageToVTKImageFilter.h>
#include <itkMesh.h>
#include <itkTransformMeshFilter.h>
#include <itkVTKImageToImageFilter.h>

#include <vtkMath.h>

#define RAD_TO_DEG 57.295779513082323

void ReadCSV(std::string file, FiberDataType& fibersInfo) {
	//cout << "Reading csv-file" << endl;
	const int numRows = 14;
	const int pastFirstNRows = 5;
	ifstream stream(file);
	std::string line;
	for (int i = 0; i < pastFirstNRows; i++) {
		getline(stream, line);
	}
	while (getline(stream, line)) {
		std::stringstream sStream(line);
		std::string cell[numRows];
		int successfulReads = 0;
		for (int i = 0; i < numRows; i++) {
			if (std::getline(sStream, cell[i], ','))
				successfulReads++;
		}
		if (successfulReads != numRows) continue;

		FibersDatas fd;
		fd.id = atoi(cell[0].c_str());
		fd.startPoint[0] = atof(cell[1].c_str());
		fd.startPoint[1] = atof(cell[2].c_str());
		fd.startPoint[2] = atof(cell[3].c_str());
		fd.endPoint[0] = atof(cell[4].c_str());
		fd.endPoint[1] = atof(cell[5].c_str());
		fd.endPoint[2] = atof(cell[6].c_str());
		fd.straightLength = atof(cell[7].c_str());
		fd.curvedLength = atof(cell[8].c_str());
		fd.diameter = atof(cell[9].c_str());
		fd.surfaceArea = atof(cell[10].c_str());
		fd.volume = atof(cell[11].c_str());
		fd.isSeparated = (atoi(cell[12].c_str()) == 1);
		fd.isCurved = (atoi(cell[13].c_str()) == 1);
		fibersInfo.push_back(fd);
	}
	//cout << "Reading is finished. Found " << fibersInfo.size() << " rows with fiber information in the file" << endl;
}

inline double AngleBetweenVectors(const double vec1[3], const double vec2[3]) {
	return acos(vtkMath::Dot(vec1, vec2) / (vtkMath::Norm(vec1) * vtkMath::Norm(vec2))) * RAD_TO_DEG;
}

void FindSpecialFibers(FiberDataType& fibersInfo)
{
	for(auto fiber = begin(fibersInfo); fiber != end(fibersInfo); )
	{
		double dir[3];
		dir[0] = fiber->endPoint[0] - fiber->startPoint[0];
		dir[1] = fiber->endPoint[1] - fiber->startPoint[1];
		dir[2] = fiber->endPoint[2] - fiber->startPoint[2];

		vtkMath::Normalize(dir);

		double z_axis[3];
		z_axis[0] = 0;
		z_axis[1] = 0;
		z_axis[2] = 1;

		double angle = AngleBetweenVectors(dir, z_axis);
		if(angle > 90) angle = 180 - angle;

		if(angle < 50)
		{			
			fiber = fibersInfo.erase(fiber);	// remove not special fiber
		}
		else
		{
			fiber++;
		}
	}
}

inline bool IsTheSameFiber(FibersDatas const & fib1, FibersDatas const & fib2, double* transform, double tolerance)
{
	double dist[2];
	dist[0] = std::sqrt (
				std::pow(fib2.startPoint[0] - transform[0] - fib1.startPoint[0], 2) +
				std::pow(fib2.startPoint[1] - transform[1] - fib1.startPoint[1], 2) +
				std::pow(fib2.startPoint[2] - transform[2] - fib1.startPoint[2], 2) );
	dist[1] = std::sqrt (
				std::pow(fib2.endPoint[0] - transform[0] - fib1.endPoint[0], 2) +
				std::pow(fib2.endPoint[1] - transform[1] - fib1.endPoint[1], 2) +
				std::pow(fib2.endPoint[2] - transform[2] - fib1.endPoint[2], 2) );

	return (dist[0] <= tolerance && dist[1] <= tolerance);
}

template<class TPointSet>
int Registration(TPointSet* fixedPointSet, TPointSet* movingPointSet, RegistrationTransformType* transform)
{
	// set up the metric

	typedef itk::EuclideanDistancePointMetric<TPointSet, TPointSet>	MetricType;

	typedef typename MetricType::TransformType                 TransformBaseType;
	typedef typename TransformBaseType::ParametersType         ParametersType;
	typedef typename TransformBaseType::JacobianType           JacobianType;

	typename MetricType::Pointer  metric = MetricType::New();

	// set up the transform

	//typedef itk::TranslationTransform<double, 3>	TransformType;
	//typedef itk::AffineTransform<double, 6>		TransformType;
	//TransformType::Pointer transform = TransformType::New();

	// Optimizer Type
	typedef itk::LevenbergMarquardtOptimizer OptimizerType;

	OptimizerType::Pointer optimizer = OptimizerType::New();
	optimizer->SetUseCostFunctionGradient(false);

	// Registration Method
	typedef itk::PointSetToPointSetRegistrationMethod<TPointSet, TPointSet> RegistrationType;
	typename RegistrationType::Pointer registration = RegistrationType::New();

	// Scale the translation components of the Transform in the Optimizer
	OptimizerType::ScalesType scales(transform->GetNumberOfParameters());
	scales.Fill(1);

	unsigned long	numberOfIterations = 300;
	double			gradientTolerance = 1e-3;	// convergence criterion
	double			valueTolerance = 1e-3;		// convergence criterion
	double			epsilonFunction = 1e-3;		// convergence criterion

	optimizer->SetScales(scales);
	optimizer->SetNumberOfIterations(numberOfIterations);
	optimizer->SetValueTolerance(valueTolerance);
	optimizer->SetGradientTolerance(gradientTolerance);
	optimizer->SetEpsilonFunction(epsilonFunction);

	// Start from an Identity transform (in a normal case, the user
	// can probably provide a better guess than the identity...
	transform->SetIdentity();

	registration->SetInitialTransformParameters(transform->GetParameters());

	//------------------------------------------------------
	// Connect all the components required for Registration
	//------------------------------------------------------
	registration->SetMetric(metric);
	registration->SetOptimizer(optimizer);
	registration->SetTransform(transform);
	registration->SetFixedPointSet(fixedPointSet);
	registration->SetMovingPointSet(movingPointSet);

	// Connect an observer
	//CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
	//optimizer->AddObserver(itk::IterationEvent(), observer);

	try
	{
		registration->Update();
	}
	catch (itk::ExceptionObject & e)
	{
		std::cout << e << std::endl;
		return EXIT_FAILURE;
	}

	typedef itk::Mesh<float, 3> MeshType;
	MeshType::Pointer mesh = MeshType::New();
	mesh->SetPoints(movingPointSet->GetPoints());

	typedef itk::TransformMeshFilter<MeshType, MeshType, RegistrationTransformType> FilterType;
	FilterType::Pointer filter = FilterType::New();
	filter->SetInput(mesh);
	filter->SetTransform(transform);
	filter->Update();

	movingPointSet->SetPoints(filter->GetOutput()->GetPoints());

	return EXIT_SUCCESS;
}

int RegistrationTest(vtkImageData* image, int region[6], std::string outputFilePath, std::vector<Endpoint>& ep1, std::vector<Endpoint>& ep2)
{
	std::cout << "PointSet to PointSet registration started" << std::endl;

	int dims[3];
	int extent[6];
	image->GetDimensions(dims);
	image->GetExtent(extent);

	// load point information

	FiberDataType fibersInfo[2];
	ReadCSV("D:\\work\\Datasets\\4DXCT_Downsampled\\roi0\\FCP\\roi0-FCPRev3.csv", fibersInfo[0]);
	ReadCSV("D:\\work\\Datasets\\4DXCT_Downsampled\\roi1\\FCP\\roi1-FCPRev3.csv", fibersInfo[1]);

	cout << "procedure of finding special fibers started" << endl;

	FindSpecialFibers(fibersInfo[0]);
	FindSpecialFibers(fibersInfo[1]);

	cout << "procedure of finding special fibers finished" << endl;

	typedef itk::PointSet<float, 3> PointSetType;
	typedef PointSetType::PointType PointType;
	typedef PointSetType::PointsContainer PointsContainer;

	PointSetType::Pointer fixedPointSet = PointSetType::New();
	PointSetType::Pointer movingPointSet = PointSetType::New();
	PointsContainer::Pointer fixedPointContainer = fixedPointSet->GetPoints();
	PointsContainer::Pointer movingPointContainer = movingPointSet->GetPoints();

	unsigned int pointId = 0;
	int threshold = 5;
	for (auto iter = begin(fibersInfo[0]); iter != end(fibersInfo[0]); iter++)
	//for (auto iter = begin(ep1); iter != end(ep1); iter++)
	{
		PointType p1, p2;
		p1[0] = iter->startPoint[0] / 2;
		p1[1] = iter->startPoint[1] / 2;
		p1[2] = iter->startPoint[2] / 2;
		p2[0] = iter->endPoint[0] / 2;
		p2[1] = iter->endPoint[1] / 2;
		p2[2] = iter->endPoint[2] / 2;
		//p1[0] = iter->GetPosition()[0];
		//p1[1] = iter->GetPosition()[1];
		//p1[2] = iter->GetPosition()[2];
		//p1[3] = iter->GetDirection()[0];
		//p1[4] = iter->GetDirection()[1];
		//p1[5] = iter->GetDirection()[2];
		
		if( extent[0] + threshold <= p1[0] && p1[0] <= extent[1] - threshold &&
			extent[2] + threshold <= p1[1] && p1[1] <= extent[3] - threshold &&
			extent[4] + threshold <= p1[2] && p1[2] <= extent[5] - threshold ) {
			fixedPointContainer->InsertElement(pointId++, p1);
		}
		if( extent[0] + threshold <= p2[0] && p2[0] <= extent[1] - threshold &&
			extent[2] + threshold <= p2[1] && p2[1] <= extent[3] - threshold &&
			extent[4] + threshold <= p2[2] && p2[2] <= extent[5] - threshold ) {
			fixedPointContainer->InsertElement(pointId++, p2);
		}
	}
	fixedPointSet->SetPoints(fixedPointContainer);

	pointId = 0;
	for (auto iter = begin(fibersInfo[1]); iter != end(fibersInfo[1]); iter++)
	//for (auto iter = begin(ep2); iter != end(ep2); iter++)
	{
		PointType p1, p2;
		p1[0] = iter->startPoint[0] / 2;
		p1[1] = iter->startPoint[1] / 2;
		p1[2] = iter->startPoint[2] / 2;
		p2[0] = iter->endPoint[0] / 2;
		p2[1] = iter->endPoint[1] / 2;
		p2[2] = iter->endPoint[2] / 2;
		//p1[0] = iter->GetPosition()[0];
		//p1[1] = iter->GetPosition()[1];
		//p1[2] = iter->GetPosition()[2];
		//p1[3] = iter->GetDirection()[0];
		//p1[4] = iter->GetDirection()[1];
		//p1[5] = iter->GetDirection()[2];

		if( extent[0] + threshold <= p1[0] && p1[0] <= extent[1] - threshold &&
			extent[2] + threshold <= p1[1] && p1[1] <= extent[3] - threshold &&
			extent[4] + threshold <= p1[2] && p1[2] <= extent[5] - threshold ) {

			if( region[0] <= p1[0] && p1[0] <= region[1] &&
				region[2] <= p1[1] && p1[1] <= region[3] &&
				region[4] <= p1[2] && p1[2] <= region[5] )
			{
				movingPointContainer->InsertElement(pointId++, p1);
			}
		}
		if( extent[0] + threshold <= p2[0] && p2[0] <= extent[1] - threshold &&
			extent[2] + threshold <= p2[1] && p2[1] <= extent[3] - threshold &&
			extent[4] + threshold <= p2[2] && p2[2] <= extent[5] - threshold ) {

			if( region[0] <= p2[0] && p2[0] <= region[1] &&
				region[2] <= p2[1] && p2[1] <= region[3] &&
				region[4] <= p2[2] && p2[2] <= region[5] )
			{
				movingPointContainer->InsertElement(pointId++, p2);
			}
		}
	}
	movingPointSet->SetPoints(movingPointContainer);

	RegistrationTransformType::Pointer transform = RegistrationTransformType::New();
	Registration<PointSetType>(fixedPointSet, movingPointSet, transform);
	//std::cout << "Solution = " << transform->GetParameters() << std::endl;

	double trans[3];
	trans[0] = transform->GetParameters()[0];
	trans[1] = transform->GetParameters()[1];
	trans[2] = transform->GetParameters()[2];

	int matchedFibers = 0;
	for(auto fib1 = begin(fibersInfo[0]); fib1 != end(fibersInfo[0]); fib1++)
	{
		for(auto fib2 = begin(fibersInfo[1]); fib2 != end(fibersInfo[1]); fib2++)
		{
			if(IsTheSameFiber(*fib1, *fib2, trans, 4)) matchedFibers++;
		}
	}
	cout << "Founded " << matchedFibers << " matched fibers" << endl;

	vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();

	img->SetDimensions(dims);
	img->SetSpacing(image->GetSpacing());
	img->AllocateScalars(VTK_INT, 1);
	void * buffer = img->GetScalarPointer();
	memset(buffer, 0, sizeof(int) * dims[0] * dims[1] * dims[2]);

	const int pointSize = 5;

	memset(buffer, 0, sizeof(int) * dims[0] * dims[1] * dims[2]);

	unsigned int num = movingPointSet->GetNumberOfPoints();
	std::cout << "Number of points = " << num << endl;
	for(int i = 0; i < num; i++)
	{
		for (int x = -pointSize; x <= pointSize; x++)
		{
			for (int y = -pointSize; y <= pointSize; y++)
			{
				for (int z = -pointSize; z <= pointSize; z++)
				{
					PointType p;
					//movingPointSet->GetPoint(i, &p);
					fixedPointSet->GetPoint(i, &p);
					//int curPos[3] = { p[0] + x + transform->GetParameters()[0], p[1] + y + transform->GetParameters()[1], p[2] + z + transform->GetParameters()[2] };
					int curPos[3] = {
						static_cast<int>(p[0] + x),
						static_cast<int>(p[1] + y),
						static_cast<int>(p[2] + z)
					};

					if (curPos[0] <= extent[0] || curPos[0] >= extent[1] ||
						curPos[1] <= extent[2] || curPos[1] >= extent[3] ||
						curPos[2] <= extent[4] || curPos[2] >= extent[5]) continue;

					int * mapVal = static_cast<int *>(img->GetScalarPointer(curPos));
					mapVal[0] = 255;
				}
			}
		}
	}

	//typedef itk::Image<int, 3> TImage;

	//typedef itk::VTKImageToImageFilter<TImage> VTKImageToImageType;
	//VTKImageToImageType::Pointer vtkImageToImageFilter = VTKImageToImageType::New();
	//vtkImageToImageFilter->SetInput(image);
	//vtkImageToImageFilter->Update();

	//typedef itk::ImageFilterDrawTokens<TImage> ImageFilterDrawTokensType;
	//std::vector<TImage::IndexType> points;
	//TImage::IndexType ind;
	//ind[0] = 10;
	//ind[1] = 10;
	//ind[2] = 10;
	//points.push_back(ind);
	//ImageFilterDrawTokensType::Pointer drawTokens = ImageFilterDrawTokensType::New();
	//drawTokens->SetPoints(points);
	//drawTokens->SetInput(vtkImageToImageFilter->GetOutput());
	//drawTokens->SetRadius(5);
	//drawTokens->Update();

	//typedef itk::ImageToVTKImageFilter<TImage>       ConnectorType;
	//ConnectorType::Pointer connector = ConnectorType::New();
	//connector->SetInput(drawTokens->GetOutput());
	//connector->Update();

	//vtkImageData* img = connector->GetOutput();

	vtkSmartPointer<vtkMetaImageWriter> writer = vtkSmartPointer<vtkMetaImageWriter>::New();
	writer->SetInputData(img);
	writer->SetFileName("D:\\work\\Datasets\\4DXCT_Downsampled\\output\\test\\output_roi0_special_endpoints.mhd");
	//writer->SetFileName(outputFilePath.c_str());
	writer->SetCompression(false);
	writer->Write();

	std::cout << "Registration is finished" << endl;

	return 1;
}

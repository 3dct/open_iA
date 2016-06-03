/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkWindows.h"
#include <fstream>
#include "itkNormalizedCorrelationImageFilter.h"
#include "itkAnnulusOperator.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkResampleImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkSimpleFilterWatcher.h"
#include "itkImageKernelOperator.h"

//forward decl
int itkNormalizedCorrelationImageFilterTest(int ac, char** av); 

//entry point
int main(int argc, char*argv)
{
	std::cout << "Performing NCC test" << std::endl;
	
	return itkNormalizedCorrelationImageFilterTest(argc, argv);
}

int itkNormalizedCorrelationImageFilterTest(int ac, char** av)
{
	std::string file(av[0]);
	std::string templateFile(av[1]);

	const unsigned int Dimension = 3;
	typedef unsigned char PixelType;
	typedef float         CorrelationPixelType;

	typedef itk::Image<PixelType, Dimension>            InputImageType;
	typedef itk::Image<CorrelationPixelType, Dimension> CorrelationImageType;
	
	itk::ImageFileReader<InputImageType>::Pointer input	= itk::ImageFileReader<InputImageType>::New();
	input->SetFileName(file); 
	try{
		input->Update();
	}
	catch (itk::ExceptionObject &e){

		std::cerr << "Error reading file. Please make sure networkdrive: NanotomData2 is mapped " << std::endl;
		std::cerr << e.GetDescription() << std::endl; 
	}
	// create a mask
	itk::ImageFileReader<CorrelationImageType>::Pointer mask = itk::ImageFileReader<CorrelationImageType>::New();
	mask->SetFileName(templateFile);
	mask->Update(); 

	itk::Size<3> radius = mask->GetOutput()->GetLargestPossibleRegion().GetSize();
	radius[0] = (radius[0] - 1) / 2;
	radius[1] = (radius[1] - 1) / 2;
	radius[2] = (radius[2] - 1) / 2;

	itk::ImageKernelOperator<float, 3> kernelOperator;
	kernelOperator.SetImageKernel(mask->GetOutput());
	kernelOperator.CreateToRadius(radius);

	// Create a filter
	typedef itk::NormalizedCorrelationImageFilter<InputImageType, CorrelationImageType, CorrelationImageType> FilterType;
	FilterType::Pointer filter = FilterType::New();
	//itk::SimpleFilterWatcher watcher(filter, "Normalized correlation");
	filter->SetInput(input->GetOutput());
	filter->SetTemplate(kernelOperator);

	//find and erase.mhd extension
	int dotpos = file.find_last_of("."); 
	file.erase(dotpos, 4); 

	//erase /Input/ and replace by /Output_fileName
	int slashpos = file.find_last_of("/"); 
	file = file.erase(0, slashpos+1); 

	std::stringstream ss; 
	ss << "Correlation_Output_" << file << ".mhd"; 
	
	// Generate output image
	itk::ImageFileWriter<CorrelationImageType>::Pointer writer;
	writer = itk::ImageFileWriter<CorrelationImageType>::New();
	writer->SetInput(filter->GetOutput());
	writer->SetFileName(ss.str());

	try
	{
		writer->Update();
	}
	catch (itk::ExceptionObject& e)
	{
		std::cerr << "\nException occured: " << e.GetDescription();
		return -1;
	}
	catch (...)
	{
		std::cerr << "Some other exception occurred" << std::endl;
		return -2;
	}

	std::cout << "Finished NCC test" << std::endl; 
	
	return EXIT_SUCCESS;
}

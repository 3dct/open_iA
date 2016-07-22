#include "DensityMap.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImage.h"

void DensityMap::calculate(std::string inputFile, std::string densityFile, int* size)
{
	typedef itk::Image<unsigned short, 3> InputImageType;
	typedef itk::Image<double, 3> DensityImageType;

	InputImageType::Pointer inputImage;
	{
		typedef itk::ImageFileReader<InputImageType> ReaderType;
		ReaderType::Pointer reader = ReaderType::New();
		reader->SetFileName(inputFile);
		reader->Update();
		inputImage = reader->GetOutput();
	}

	InputImageType::SizeType inputSize = inputImage->GetLargestPossibleRegion().GetSize();
	double cellSize[3] = { (double)inputSize[0] / size[0], (double)inputSize[1] / size[1], (double)inputSize[2] / size[2] };

	DensityImageType::Pointer densityImage;
	{
		densityImage = DensityImageType::New();
		DensityImageType::IndexType index; index.Fill(0);
		DensityImageType::SizeType imgSize; imgSize[0] = size[0]; imgSize[1] = size[1]; imgSize[2] = size[2];
		DensityImageType::RegionType region(index, imgSize);
		densityImage->SetRegions(region);
		DensityImageType::SpacingType newSpacing;
		InputImageType::SpacingType oldSpacing = inputImage->GetSpacing();
		newSpacing[0] = oldSpacing[0] * cellSize[0];
		newSpacing[1] = oldSpacing[1] * cellSize[1];
		newSpacing[2] = oldSpacing[2] * cellSize[2];
		densityImage->SetSpacing(newSpacing);
		densityImage->Allocate();
		densityImage->FillBuffer(0);
	}

	InputImageType::IndexType ind;
	DensityImageType::IndexType densityInd;
	double weight = 1. / (cellSize[0] * cellSize[1] * cellSize[2]);
	for(ind[0] = 0; ind[0] < inputSize[0]; ind[0]++)
	{
		for(ind[1] = 0; ind[1] < inputSize[1]; ind[1]++)
		{
			for(ind[2] = 0; ind[2] < inputSize[2]; ind[2]++)
			{ 
				InputImageType::PixelType val = inputImage->GetPixel(ind);
				densityInd[0] = ind[0] / cellSize[0];
				densityInd[1] = ind[1] / cellSize[1];
				densityInd[2] = ind[2] / cellSize[2];
				if(val > 0)
				{
					densityImage->SetPixel(densityInd, densityImage->GetPixel(densityInd) + weight);
					//densityImage->SetPixel(densityInd, densityImage->GetPixel(densityInd) + 1);
				}
			}
		}	
	}

	{
		typedef itk::ImageFileWriter<DensityImageType> WriterType;
		WriterType::Pointer writer = WriterType::New();
		writer->SetInput(densityImage);
		writer->SetFileName(densityFile);
		writer->Update();
	}
}

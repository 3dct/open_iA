#ifndef __itkImageFilter_hxx
#define __itkImageFilter_hxx

#include "FilterLabelImage.h"
// itk
#include <itkImageAlgorithm.h>
#include <itkObjectFactory.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>

namespace itk
{

	template<class TImage>
	void FilterLabelImage<TImage>::GenerateData()
	{
		typename TImage::ConstPointer input = this->GetInput();
		typename TImage::Pointer output = this->GetOutput();

		this->AllocateOutputs();

		//ImageAlgorithm::Copy(input.GetPointer(), output.GetPointer(), output->GetRequestedRegion(),
		//	output->GetRequestedRegion());

		typedef typename TImage::PixelType PixelType;

		PixelType* buffer = output->GetBufferPointer();
		typename TImage::SizeType region = output->GetLargestPossibleRegion().GetSize();
		for (int x = 0; x < region[0]; x++)
		{
			for (int y = 0; y < region[1]; y++)
			{
				for (int z = 0; z < region[2]; z++)
				{
					typename TImage::IndexType pixel = { x, y, z };
					PixelType val = input->GetPixel(pixel);
					if ( val == 0 || std::find(m_list.begin(), m_list.end(), val) == m_list.end() ) {
						output->SetPixel(pixel, 0);
					}
					else {
						output->SetPixel(pixel, 255);
					}
				}
			}
		}
	}

}// end namespace


#endif

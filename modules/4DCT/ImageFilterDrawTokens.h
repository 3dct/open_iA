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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IMAGEFILTERDRAWTOKENS_H
#define IMAGEFILTERDRAWTOKENS_H
// itk
#include <itkImageToImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
// std
#include <vector>

namespace itk
{
template<class TImage>
class ImageFilterDrawTokens : public ImageToImageFilter<TImage, TImage>
{
public:
	/** Standard class typedefs. */
	typedef ImageFilterDrawTokens				Self;
	typedef ImageToImageFilter<TImage, TImage>	Superclass;
	typedef SmartPointer<Self>					Pointer;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);

	/** Run-time type information (and related methods). */
	itkTypeMacro(ImageFilterDrawTokens, ImageToImageFilter);

	itkSetMacro(Radius, unsigned int);
	itkSetMacro(Points, std::vector < typename TImage::IndexType > );

protected:
	ImageFilterDrawTokens() { };
	~ImageFilterDrawTokens() { };

	/** Does the real work. */

	virtual void GenerateData();

private:
	ImageFilterDrawTokens(const Self&);	//purposely not implemented
	void operator =(const Self&);		//purposely not implemented

	unsigned int m_Radius;
	std::vector<typename TImage::IndexType> m_Points;
};

template<class TImage>
void ImageFilterDrawTokens<TImage>::GenerateData()
{
	typename TImage::ConstPointer input = this->GetInput();
	typename TImage::Pointer output = this->GetOutput();

	output->SetRegions(input->GetLargestPossibleRegion());
	output->Allocate();

	itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());
	itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());

	while(!outputIterator.IsAtEnd())
	{
		outputIterator.Set(inputIterator.Get());

		inputIterator++;
		outputIterator++;
	}

	for(auto point = begin(m_Points); point != end(m_Points); point++)
	{
		for(int x = m_Radius + 1; x < m_Radius; x++)
		{
			for(int y = m_Radius + 1; y < m_Radius; y++)
			{
				for(int z = m_Radius + 1; z < m_Radius; z++)
				{
					typename TImage::IndexType index;
					index[0] = (* point)[0] + x;
					index[1] = (* point)[1] + y;
					index[2] = (* point)[2] + z;

					if(input->GetLargestPossibleRegion().IsInside(index))
					{
						outputIterator.Set(255);
					}
				}
			}
		}
	}
}

} // namespace ITK

#endif // IMAGEFILTERDRAWTOKENS_H

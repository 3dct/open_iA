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
#ifndef __itkVectorImageDuplicator_txx
#define __itkVectorImageDuplicator_txx

#include "itkVectorImageDuplicator.h"


namespace itk
{

/** Constructor */
template<class TInputVectorImage>
VectorImageDuplicator<TInputVectorImage>
::VectorImageDuplicator()
{
  m_InputImage = nullptr;
  m_Output = nullptr;
  m_InternalImageTime = 0;
}

/** */
template<class TInputVectorImage>
void
VectorImageDuplicator<TInputVectorImage>
::Update(void)
{
  if(!m_InputImage )
    {
    itkExceptionMacro(<<"Input image has not been connected");
    return;
    }

  // Update only if the input image has been modified
  unsigned long t, t1, t2;
  t1 = m_InputImage->GetPipelineMTime();
  t2 = m_InputImage->GetMTime();
  t = (t1 > t2 ? t1 : t2);

  if(t == m_InternalImageTime)
    {
    return; // No need to update
    }

  // Cache the timestamp
  m_InternalImageTime = t;

  // Allocate the image
  m_Output = ImageType::New();
  m_Output->SetVectorLength(m_InputImage->GetVectorLength());
  m_Output->SetOrigin(m_InputImage->GetOrigin());
  m_Output->SetSpacing(m_InputImage->GetSpacing());
  m_Output->SetDirection(m_InputImage->GetDirection());
  m_Output->SetLargestPossibleRegion(m_InputImage->GetLargestPossibleRegion());
  m_Output->SetRequestedRegion(m_InputImage->GetRequestedRegion());
  m_Output->SetBufferedRegion(m_InputImage->GetBufferedRegion());
  m_Output->Allocate();

  // Do the copy
  unsigned long size = 1;
  for(unsigned int i=0;i<itkGetStaticConstMacro(ImageDimension);i++)
    {
    size *= m_InputImage->GetBufferedRegion().GetSize()[i];
    }

  memcpy( m_Output->GetBufferPointer(), m_InputImage->GetBufferPointer(),
          size * sizeof(InternalPixelType) * m_InputImage->GetVectorLength() );
}

template<class TInputVectorImage>
void
VectorImageDuplicator<TInputVectorImage>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf(os,indent);
  os << indent << "Input Image: " << m_InputImage << std::endl;
  os << indent << "Output Image: " << m_Output << std::endl;
  os << indent << "Internal Image Time: " << m_InternalImageTime << std::endl;
}

} // end namespace itk

#endif

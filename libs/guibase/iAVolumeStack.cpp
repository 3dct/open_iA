// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeStack.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

iAVolumeStack::~iAVolumeStack()
{
	while(!m_colorTFVector.empty())
	{
		vtkColorTransferFunction* ctf = m_colorTFVector.back();
		ctf->Delete();
		m_colorTFVector.pop_back();
	}
	while(!m_opacityTFVector.empty())
	{
		vtkPiecewiseFunction* pwf= m_opacityTFVector.back();
		pwf->Delete();
		m_opacityTFVector.pop_back();
	}
}

void iAVolumeStack::addVolume(vtkImageData* volume)
{
	vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
	image->DeepCopy(volume);
	m_volumes.push_back(image);
}

vtkImageData* iAVolumeStack::volume(size_t i)
{
	return m_volumes.at(i);
}

size_t iAVolumeStack::numberOfVolumes()
{
	return m_volumes.size();
}

void iAVolumeStack::addVolumeAt(vtkImageData* volume, size_t i)
{
	vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
	image->DeepCopy(volume);
	m_volumes.at(i)=image;
}

void iAVolumeStack::addColorTransferFunction(vtkColorTransferFunction* instance)
{
	vtkColorTransferFunction* ctf = vtkColorTransferFunction::New();
	ctf->DeepCopy((vtkScalarsToColors*)instance);
	m_colorTFVector.push_back(ctf);
}

void iAVolumeStack::addPiecewiseFunction(vtkPiecewiseFunction* instance)
{
	vtkPiecewiseFunction* pwf = vtkPiecewiseFunction::New();
	pwf->DeepCopy((vtkDataObject*)instance);
	m_opacityTFVector.push_back(pwf);
}

vtkColorTransferFunction* iAVolumeStack::colorTF(size_t i)
{
	return m_colorTFVector[i];
}

vtkPiecewiseFunction* iAVolumeStack::opacityTF(size_t i)
{
	return m_opacityTFVector[i];
}

void iAVolumeStack::addFileName(QString fileName)
{
	m_fileNameArray.push_back(fileName);
}

QString iAVolumeStack::fileName(size_t i)
{
	return m_fileNameArray.at(i);
}

std::vector<vtkSmartPointer<vtkImageData> > * iAVolumeStack::volumes()
{
	return &m_volumes;
}

std::vector<QString> * iAVolumeStack::fileNames()
{
	return &m_fileNameArray;
}

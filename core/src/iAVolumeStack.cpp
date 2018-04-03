/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAVolumeStack.h"

//#include <vtkDataSet.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
//#include <vtkStructuredData.h> // Needed for inline methods


iAVolumeStack::iAVolumeStack()
{
	id = 0;
}

iAVolumeStack::~iAVolumeStack()
{
	// clear vectors
	while(!colorTransferVector.empty()) {
		vtkColorTransferFunction* ctf = colorTransferVector.back();
		ctf->Delete();
		colorTransferVector.pop_back();
	}
	while(!piecewiseVector.empty()) {
		vtkPiecewiseFunction* pwf= piecewiseVector.back();
		pwf->Delete();
		piecewiseVector.pop_back();
	}
}

void iAVolumeStack::addVolume(vtkImageData* volume)
{
	vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
	image->DeepCopy(volume);
	volumes.push_back(image);
}

vtkImageData* iAVolumeStack::getVolume(int i)
{
	return volumes.at(i);
}

size_t iAVolumeStack::getNumberOfVolumes()
{
	return volumes.size();
}

void iAVolumeStack::addVolumeAt(vtkImageData* volume, int i)
{
	vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
	image->DeepCopy(volume);
	volumes.at(i)=image;
}

void iAVolumeStack::addColorTransferFunction(vtkColorTransferFunction* instance)
{
	vtkColorTransferFunction* ctf = vtkColorTransferFunction::New();
	ctf->DeepCopy((vtkScalarsToColors*)instance);
	colorTransferVector.push_back(ctf);
}

void iAVolumeStack::addPiecewiseFunction(vtkPiecewiseFunction* instance)
{
	vtkPiecewiseFunction* pwf = vtkPiecewiseFunction::New();
	pwf->DeepCopy((vtkDataObject*)instance);
	piecewiseVector.push_back(pwf);
}

/*void iAVolumeStack::addColorTransferFunctionAt(vtkColorTransferFunction* colorTransferFunction, int i)
{
	vtkColorTransferFunction* ctf = vtkColorTransferFunction::New();
	ctf->DeepCopy(colorTransferFunction);
	colorTransferVector.at(i)=ctf;
}
void iAVolumeStack::addPiecewiseFunctionAt(vtkPiecewiseFunction* pieceWiseFunction, int i)
{
	vtkPiecewiseFunction* pwf = vtkPiecewiseFunction::New();
	pwf->DeepCopy(pieceWiseFunction);
	piecewiseVector.at(i)=pwf;
}*/
vtkColorTransferFunction* iAVolumeStack::getColorTransferFunction(int i)
{
	return colorTransferVector[i];
}

vtkPiecewiseFunction* iAVolumeStack::getPiecewiseFunction(int i)
{
	return piecewiseVector[i];
}

void iAVolumeStack::addFileName(QString fileName) {
	fileNameArray.push_back(fileName);
}

QString iAVolumeStack::getFileName(int i) {
	return fileNameArray.at(i);
}

std::vector<vtkSmartPointer<vtkImageData> > * iAVolumeStack::GetVolumes()
{ 
	return &volumes;
}

std::vector<QString> * iAVolumeStack::GetFileNames()
{
	return &fileNameArray;
}

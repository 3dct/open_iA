/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iANModalPCAModalityReducer.h"

#include <defines.h> // for DIM
#include "iAModality.h"
#include "iATypedCallHelper.h"

#include <vtkImageData.h>

#include <itkImagePCAShapeModelEstimator.h>

// Input modalities (volumes) must have the exact same dimensions
QList<QSharedPointer<iAModality>> iANModalPCAModalityReducer::reduce(QList<QSharedPointer<iAModality>> modalities) {

	// TODO: assert if all modalities have the same dimensions

	// Set up connectors
	std::vector<iAConnector> connectors(modalities.size());
	for (int i = 0; i < modalities.size(); i++) {
		connectors[i] = iAConnector();
		connectors[i].setImage(modalities[i]->image());
	}

	// Go!
	ITK_TYPED_CALL(itkPCA, connectors[0].itkScalarPixelType(), connectors);

	// Set up output list
	modalities = QList<QSharedPointer<iAModality>>();
	for (int i = 0; i < connectors.size(); i++) {
		auto name = "Principal Component " + QString::number(i);
		auto mod = new iAModality(name, "", -1, connectors[i].vtkImage(), iAModality::NoRenderer);
		


		//QSharedPointer<iAVolumeRenderer> renderer(new iAVolumeRenderer(mod->transfer().data(), mod->image()));
		//mod->setRenderer(renderer);

		//m_mdiChild->modalitiesDockWidget()->addModality(...);

		modalities.append(QSharedPointer<iAModality>(mod));
	}

	// Ready to output :)
	// - length of output list <= maxOutputLength()
	auto output = modalities;
	assert(output.size() <= maxOutputLength());
	return output;
}

template<class T>
void iANModalPCAModalityReducer::itkPCA(std::vector<iAConnector> &c) {
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType> PCASMEType;

	int inputSize = c.size();
	int outputSize = std::min((int)c.size(), maxOutputLength());
	
	auto pca = PCASMEType::New();
	pca->SetNumberOfTrainingImages(inputSize);
	pca->SetNumberOfPrincipalComponentsRequired(outputSize);
	for (int i = 0; i < inputSize; i++) {
		pca->SetInput(i, dynamic_cast<ImageType *>(c[i].itkImage()));
	}

	pca->Update();

	c.resize(outputSize);
	for (int i = 0; i < outputSize; i++) {
		c[i].setImage(pca->GetOutput(i));
	}
}
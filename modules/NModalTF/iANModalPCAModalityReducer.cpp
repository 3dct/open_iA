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

#ifndef NDEBUG
#include "iAToolsITK.h"
#endif

#include <vtkImageData.h>

#include <itkImagePCAShapeModelEstimator.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>

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
	//ITK_TYPED_CALL(itkPCA, connectors[0].itkScalarPixelType(), connectors);
	ITK_TYPED_CALL(ownPCA, connectors[0].itkScalarPixelType(), connectors);

	// Set up output list
	modalities = QList<QSharedPointer<iAModality>>();
	for (int i = 0; i < connectors.size(); i++) {
		auto name = "Principal Component " + QString::number(i);

		auto imageData = vtkSmartPointer<vtkImageData>::New();
		imageData->DeepCopy(connectors[i].vtkImage());

		auto mod = new iAModality(name, "", -1, imageData, iAModality::NoRenderer);

#ifndef NDEBUG
		storeImage2(connectors[i].itkImage(), "pca_output_" + QString::number(i) + ".mhd", true);
#endif

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

#ifndef NDEBUG
		storeImage2(c[i].itkImage(), "pca_input_itk_" + QString::number(i) + ".mhd", true);
		storeImage2(dynamic_cast<ImageType *>(c[i].itkImage()), "pca_input_itkcast_" + QString::number(i) + ".mhd", true);
#endif

		pca->SetInput(i, dynamic_cast<ImageType *>(c[i].itkImage()));
	}

	pca->Update();

	// Debug. TODO: remove
	PCASMEType::VectorOfDoubleType eigenValues = pca->GetEigenValues();
	double sv_mean = sqrt(eigenValues[0]);
	printf("sv_mean = %d\n", sv_mean);

	auto count = pca->GetNumberOfOutputs();

	// TODO uncomment
	//c.resize(outputSize);
	//for (int i = 0; i < outputSize; i++) {
	c.resize(count);
	for (int i = 0; i < count; i++) {

#ifndef NDEBUG
		storeImage2(pca->GetOutput(i), "pca_output_before_conversion_" + QString::number(i) + ".mhd", true);
#endif

		c[i].setImage(pca->GetOutput(i));
	}
}

#ifndef NDEBUG
#define DEBUG_LOG_MATRIX(matrix, string) \
	{ QString str = string; \
	str += "\n"; \
	for (int i = 0; i < numInputs; i++) { \
		auto row = matrix.get_row(i); \
		for (int j = 0; j < row.size(); j++) { \
			str += QString::number(row[j]) + "     "; \
		} \
		str += "\n"; \
	} \
	DEBUG_LOG(str); }
#else
#define DEBUG_LOG_MATRIX(matrix)
#endif

template<class T>
void iANModalPCAModalityReducer::ownPCA(std::vector<iAConnector> &c) {
	typedef itk::Image<T, DIM> ImageType;

	assert(c.size() > 0);

	int numInputs = c.size();
	int numOutputs = std::min((int)c.size(), maxOutputLength());

	auto itkImg0 = c[0].itkImage();

	auto size = itkImg0->GetBufferedRegion().GetSize();
	unsigned int numVoxels = 1;
	for (unsigned int dim_i = 0; dim_i < DIM; dim_i++) {
		numVoxels *= size[dim_i];
	}

	// Set up iterators
	vnl_matrix<double> inputs(numInputs, numVoxels);
	for (int row_i = 0; row_i < numInputs; row_i++) {
		auto input = dynamic_cast<const ImageType *>(c[row_i].itkImage());
		auto iterator = itk::ImageRegionConstIterator<ImageType>(input, input->GetBufferedRegion());
		iterator.GoToBegin();

		for (int col_i = 0; col_i < numVoxels; col_i++) {
			inputs[row_i][col_i] = iterator.Get();
			++iterator;
		}

#ifndef NDEBUG
		storeImage2(c[row_i].itkImage(), "pca_input_itk_" + QString::number(row_i) + ".mhd", true);
		storeImage2(dynamic_cast<ImageType *>(c[row_i].itkImage()), "pca_input_itkcast_" + QString::number(row_i) + ".mhd", true);
#endif
	}

	

	// Calculate means
	vnl_vector<double> means;
	means.set_size(numVoxels);
	means.fill(0);
	for (unsigned int img_i = 0; img_i < numInputs; img_i++) {
		//auto ite = iterators[img_i];
		for (unsigned int i = 0; i < numVoxels; i++) {
			//means[i] += ite.Get();
			//++ite;
			means[i] = inputs[img_i][i];
		}
	}
	means /= numInputs;

	// Calculate inner product (lower triangle) (for covariance matrix)
	vnl_matrix<double> innerProd;
	innerProd.set_size(numInputs, numInputs);
	innerProd.fill(0);
	for (unsigned int ix = 0; ix < numInputs; ix++) {
		for (unsigned int iy = 0; iy < ix; iy++) {
			//auto itex = iterators[ix];
			//auto itey = iterators[iy];
			for (unsigned int i = 0; i < numVoxels; i++) {
				//auto mx = itex.Get() - means[i];
				//auto my = itey.Get() - means[i];
				auto mx = inputs[ix][i] - means[i];
				auto my = inputs[iy][i] - means[i];
				innerProd[ix][iy] += (mx * my); // Product takes place!
				//++itex;
				//++itey;
			}
		}
	}

	// Fill upper triangle (make symmetric)
	for (unsigned int ix = 0; ix < (numInputs - 1); ix++) {
		for (unsigned int iy = ix + 1; iy < numInputs; iy++) {
			innerProd[ix][iy] = innerProd[iy][ix];
		}
	}
	if (numInputs - 1 != 0) {
		innerProd /= (numInputs - 1);
	} else {
		innerProd.fill(0);
	}

	// Solve eigenproblem
	vnl_matrix<double> eye(numInputs, numInputs); // (eye)dentity matrix
	eye.set_identity();
	DEBUG_LOG_MATRIX(innerProd, "Inner product");
	DEBUG_LOG_MATRIX(eye, "Identity");
	vnl_generalized_eigensystem evecs_evals_innerProd(innerProd, eye);
	auto evecs_innerProd = evecs_evals_innerProd.V;
	evecs_innerProd.fliplr(); // Flipped because VNL sorts eigenvectors in ascending order
	if (numInputs != numOutputs) evecs_innerProd.extract(numInputs, numOutputs); // Keep only 'numOutputs' columns
	//auto evals_innerProd = evecs_evals_innerProd.D.diagonal();

	DEBUG_LOG_MATRIX(evecs_innerProd, "Eigenvectors");
	
	vnl_matrix<double> reconstructed(numVoxels, numOutputs);
	reconstructed.fill(0);

	// Transform images to principal components
	for (unsigned int img_i = 0; img_i < numInputs; img_i++) {
		//auto ite = iterators[img_i];
		for (unsigned int i = 0; i < numVoxels; i++) {
			//auto vox = ite.Get();
			auto vox = inputs[img_i][i];
			for (unsigned int vec_i = 0; vec_i < numOutputs; vec_i++) {
				auto evec_elem = evecs_innerProd[img_i][vec_i];
				reconstructed[i][vec_i] += (vox * evec_elem);
			}
			//++ite;
		}
	}

	for (int vec_i = 0; vec_i < numOutputs; vec_i++) {
		double max_val = -DBL_MAX;
		double min_val = DBL_MAX;
		auto col = reconstructed.get_column(vec_i);
		for (int i = 0; i < numVoxels; i++) {
			auto rec = col[i];
			max_val = max_val > rec ? max_val : rec;
			min_val = min_val < rec ? min_val : rec;
		}
		col = (col - min_val) / (max_val - min_val) * 65536.0;
		reconstructed.set_column(vec_i, col);
	}

	// Create outputs
	/*std::vector<ImageType::Pointer> outputs(numOutputs);
	for (int i = 0; i < numOutputs; i++) {
		outputs[i] = ImageType::New();

		ImageType::RegionType region;
		region.SetSize(itkImg0->GetLargestPossibleRegion().GetSize());
		region.SetIndex(itkImg0->GetLargestPossibleRegion().GetIndex());

		//outputs[i]->SetRegions(itkImg0->GetLargestPossibleRegion());
		outputs[i]->SetRegions(region);
		outputs[i]->Allocate();
	}*/

	// Reshape reconstructed vectors into image
	c.resize(numOutputs);
	for (unsigned int out_i = 0; out_i < numOutputs; out_i++) {
		auto recvec = reconstructed.get_column(out_i);

		auto output = ImageType::New();
		ImageType::RegionType region;
		region.SetSize(itkImg0->GetLargestPossibleRegion().GetSize());
		region.SetIndex(itkImg0->GetLargestPossibleRegion().GetIndex());
		output->SetRegions(region);
		output->Allocate();
		auto ite = itk::ImageRegionIterator<ImageType>(output, region);

		unsigned int i = 0;
		ite.GoToBegin();
		while (!ite.IsAtEnd()) {

			double rec = recvec[i];
			ImageType::PixelType rec_cast = static_cast<typename ImageType::PixelType>(rec);
			ite.Set(rec_cast);

			++ite;
			++i;
		}

#ifndef NDEBUG
		storeImage2(output, "pca_output_before_conversion_" + QString::number(out_i) + ".mhd", true);
#endif

		c[out_i].setImage(output);
	}
}

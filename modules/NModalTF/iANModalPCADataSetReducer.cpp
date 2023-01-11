/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iANModalPCADataSetReducer.h"

#include <iADataSet.h>
#include <iAPerformanceHelper.h>
#include <iATypedCallHelper.h>

#include <defines.h>  // for DIM
#include <iALog.h>

#ifndef NDEBUG
#include <iAPerformanceHelper.h>  // TODO
#include <iAToolsITK.h>
#endif

#include <itkImagePCAShapeModelEstimator.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

#include <vtkImageData.h>

// Input modalities (volumes) must have the exact same dimensions
QList<std::shared_ptr<iAImageData>> iANModalPCADataSetReducer::reduce(
	const QList<std::shared_ptr<iAImageData>>& modalities_in)
{
	// TODO: assert if all modalities have the same dimensions

	// Set up connectors
	std::vector<iAConnector> connectors(modalities_in.size());
	for (int i = 0; i < modalities_in.size(); i++)
	{
		connectors[i] = iAConnector();
		connectors[i].setImage(modalities_in[i]->vtkImage());
	}

	// Go!
	//ITK_TYPED_CALL(itkPCA, connectors[0].itkScalarType(), connectors);
	ITK_TYPED_CALL(ownPCA, connectors[0].itkScalarType(), connectors);

	// Set up output list
	auto modalities = QList<std::shared_ptr<iAImageData>>();
	for (size_t i = 0; i < connectors.size(); i++)
	{
		auto name = "Principal Component " + QString::number(i);

		auto imageData = vtkSmartPointer<vtkImageData>::New();
		imageData->DeepCopy(connectors[i].vtkImage());

		auto dataSet = std::make_shared<iAImageData>(imageData);
		dataSet->setMetaData(iADataSet::NameKey, name);

#ifndef NDEBUG
		//storeImage(connectors[i].itkImage(), "pca_output_" + QString::number(i) + ".mhd", true);
#endif

		//QSharedPointer<iAVolumeRenderer> renderer(new iAVolumeRenderer(mod->transfer().data(), mod->image()));
		//mod->setRenderer(renderer);

		//m_mdiChild->dataDockWidget()->addModality(...);

		modalities.append(dataSet);
	}

	// Ready to output :)
	// - length of output list <= maxOutputLength()
	auto output = modalities;
	assert(output.size() <= maxOutputLength());
	return output;
}

template <class T>
void iANModalPCADataSetReducer::itkPCA(std::vector<iAConnector>& c)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType> PCASMEType;

	int inputSize = c.size();
	int outputSize = std::min((int)c.size(), maxOutputLength());

	auto pca = PCASMEType::New();
	pca->SetNumberOfTrainingImages(inputSize);
	pca->SetNumberOfPrincipalComponentsRequired(outputSize);
	for (int i = 0; i < inputSize; i++)
	{
#ifndef NDEBUG
		//storeImage2(c[i].itkImage(), "pca_input_itk_" + QString::number(i) + ".mhd", true);
		//storeImage2(dynamic_cast<ImageType *>(c[i].itkImage()), "pca_input_itkcast_" + QString::number(i) + ".mhd", true);
#endif

		pca->SetInput(i, dynamic_cast<ImageType*>(c[i].itkImage()));
	}

	pca->Update();

	// Debug. TODO: remove
	typename PCASMEType::VectorOfDoubleType eigenValues = pca->GetEigenValues();
	double sv_mean = std::sqrt(eigenValues[0]);
	printf("sv_mean = %d\n", sv_mean);

	auto count = pca->GetNumberOfOutputs();

	// TODO uncomment
	//c.resize(outputSize);
	//for (int i = 0; i < outputSize; i++) {
	c.resize(count);
	for (int i = 0; i < count; i++)
	{
#ifndef NDEBUG
		//storeImage2(pca->GetOutput(i), "pca_output_before_conversion_" + QString::number(i) + ".mhd", true);
		storeImage2(pca->GetOutput(i), "pca_output_" + QString::number(i) + ".mhd", true);
#endif

		c[i].setImage(pca->GetOutput(i));
	}
}

#ifndef NDEBUG
#define DEBUG_LOG_MATRIX(matrix, string)                  \
	{                                                     \
		QString str = string;                             \
		str += "\n";                                      \
		for (int i = 0; i < numInputs; i++)               \
		{                                                 \
			auto row = matrix.get_row(i);                 \
			for (int j = 0; j < row.size(); j++)          \
			{                                             \
				str += QString::number(row[j]) + "     "; \
			}                                             \
			str += "\n";                                  \
		}                                                 \
		LOG(lvlDebug, str);                               \
	}
#define DEBUG_LOG_VECTOR(vector, string)                 \
	{                                                    \
		QString str = string;                            \
		str += "\n";                                     \
		for (int i = 0; i < vector.size(); i++)          \
		{                                                \
			str += QString::number(vector[i]) + "     "; \
		}                                                \
		str += "\n";                                     \
		LOG(lvlDebug, str);                              \
	}
#else
#define DEBUG_LOG_MATRIX(matrix, string)
#define DEBUG_LOG_VECTOR(vector, string)
#endif

template <class T>
void iANModalPCADataSetReducer::ownPCA(std::vector<iAConnector>& c)
{
	typedef itk::Image<T, DIM> ImageType;

	assert(c.size() > 0);

	iATimeGuard tg("Perform PCA");

	size_t numInputs = c.size();
	size_t numOutputs = std::min((int)c.size(), maxOutputLength());

	auto itkImg0 = c[0].itkImage();

	auto size = itkImg0->GetBufferedRegion().GetSize();
	size_t numVoxels = 1;
	for (unsigned int dim_i = 0; dim_i < DIM; dim_i++)
	{
		numVoxels *= size[dim_i];
	}
	if (numVoxels >= std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Input image (number of voxels: %1) exceeds size that can be handled "
			"(current voxel number maximum: %2)!").arg(numVoxels).arg(std::numeric_limits<int>::max()));
	}
	if (numInputs >= std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Number of input images (%1) exceeds size that can be handled "
			"(current limit: %2)!").arg(numInputs).arg(std::numeric_limits<int>::max()));
	}

	// Set up input matrix
	vnl_matrix<double> inputs(numInputs, numVoxels);
	for (size_t row_i = 0; row_i < numInputs; row_i++)
	{
		auto input = dynamic_cast<const ImageType*>(c[row_i].itkImage());
		auto iterator = itk::ImageRegionConstIterator<ImageType>(input, input->GetBufferedRegion());
		iterator.GoToBegin();

		for (size_t col_i = 0; col_i < numVoxels; col_i++)
		{
			inputs[row_i][col_i] = iterator.Get();
			++iterator;
		}

#ifndef NDEBUG
		//storeImage(c[row_i].itkImage(), "pca_input_itk_" + QString::number(row_i) + ".mhd", true);
		//storeImage(dynamic_cast<ImageType *>(c[row_i].itkImage()), "pca_input_itkcast_" + QString::number(row_i) + ".mhd", true);
#endif
	}

#ifndef NDEBUG
	int numThreads;
#pragma omp parallel
	{
#pragma omp single
		numThreads = omp_get_num_threads();
	}
	LOG(lvlDebug, QString::number(numThreads) + " threads available\n");
#endif

	// Calculate means
	vnl_vector<double> means;
	means.set_size(numInputs);

	// Calculate inner product (lower triangle) (for covariance matrix)
	vnl_matrix<double> innerProd;
	innerProd.set_size(numInputs, numInputs);
	innerProd.fill(0);

	// Initialize the reconstructed matrix (with zeros)
	vnl_matrix<double> reconstructed(numOutputs, numVoxels);

	// Eigenvectors
	vnl_matrix<double> evecs_innerProd;

	// Normalize row-wise (i.e. image-wise) to range 0..1
	double max_val = -DBL_MAX;
	double min_val = DBL_MAX;

#pragma omp parallel
	{
		// Calculate means
		for (size_t img_i = 0; img_i < numInputs; img_i++)
		{
			double mean = 0;
			// TODO: Use omp reduction?
#pragma omp for nowait
			for (int i = 0; i < static_cast<int>(numVoxels); i++)
			{
				//means[img_i] += inputs[img_i][i];
				mean += inputs[img_i][i];
			}
#pragma omp single
			means[img_i] = 0;
#pragma omp barrier
#pragma omp atomic
			means[img_i] += mean;
#pragma omp barrier
#pragma omp single
			means[img_i] /= numVoxels;
		}

#ifndef NDEBUG
#pragma omp single
		DEBUG_LOG_VECTOR(means, "Means");
#endif

		// Calculate inner product (lower triangle) (for covariance matrix)
		for (size_t ix = 0; ix < numInputs; ix++)
		{
			for (size_t iy = 0; iy <= ix; iy++)
			{
				double innerProd_thread = 0;
#pragma omp for nowait
				for (int i = 0; i < static_cast<int>(numVoxels); i++)
				{
					auto mx = inputs[ix][i] - means[ix];
					auto my = inputs[iy][i] - means[iy];
					//innerProd[ix][iy] += (mx * my); // Product takes place!
					innerProd_thread += (mx * my);
				}
#pragma omp single
				innerProd[ix][iy] = 0;
#pragma omp barrier
#pragma omp atomic
				innerProd[ix][iy] += innerProd_thread;
			}
		}

		// Fill upper triangle (make symmetric)
#pragma omp for
		for (int ix = 0; ix < static_cast<int>(numInputs - 1); ix++)
		{
			for (size_t iy = ix + 1; iy < numInputs; iy++)
			{
				innerProd[ix][iy] = innerProd[iy][ix];
			}
		}

#ifndef NDEBUG
#pragma omp single
		DEBUG_LOG_MATRIX(innerProd, "Inner product");
#endif

#pragma omp single
		{
			// Make covariance matrix (divide by N-1)
			if (numInputs - 1 != 0)
			{
				innerProd /= (numVoxels - 1);
			}
			else
			{
				innerProd.fill(0);
			}

#ifndef NDEBUG
			DEBUG_LOG_MATRIX(innerProd, "Covariance matrix");
#endif

			// Solve eigenproblem
			vnl_matrix<double> eye(numInputs, numInputs);  // (eye)dentity matrix
			eye.set_identity();
			//DEBUG_LOG_MATRIX(eye, "Identity");
			vnl_generalized_eigensystem evecs_evals_innerProd(innerProd, eye);
			evecs_innerProd = evecs_evals_innerProd.V;
			evecs_innerProd.fliplr();  // Flipped because VNL sorts eigenvectors in ascending order
			if (numInputs != numOutputs)
				evecs_innerProd.extract(numInputs, numOutputs);  // Keep only 'numOutputs' columns
				//auto evals_innerProd = evecs_evals_innerProd.D.diagonal();

#ifndef NDEBUG
			DEBUG_LOG_MATRIX(evecs_innerProd, "Eigenvectors");
			DEBUG_LOG_VECTOR(evecs_evals_innerProd.D.diagonal(), "Eigenvalues");
#endif
		}

		// Initialize the reconstructed matrix (with zeros)
		for (size_t row_i = 0; row_i < numOutputs; row_i++)
		{
#pragma omp for nowait
			for (int col_i = 0; col_i < static_cast<int>(numVoxels); col_i++)
			{
				reconstructed[row_i][col_i] = 0;
			}
		}

#pragma omp barrier

		// Transform images to principal components
		for (size_t row_i = 0; row_i < numInputs; row_i++)
		{
			for (size_t vec_i = 0; vec_i < numOutputs; vec_i++)
			{
				auto evec_elem = evecs_innerProd[row_i][vec_i];
				//double reconstructed_thread = 0;
#pragma omp for nowait
				for (int col_i = 0; col_i < static_cast<int>(numVoxels); col_i++)
				{
					auto voxel_value = inputs[row_i][col_i];
					reconstructed[vec_i][col_i] += (voxel_value * evec_elem);
				}
			}
		}

#pragma omp barrier

		// Normalize row-wise (i.e. image-wise) to range 0..1
		for (size_t vec_i = 0; vec_i < numOutputs; vec_i++)
		{
			double max_thread = -DBL_MAX;
			double min_thread = DBL_MAX;

#pragma omp for
			for (int i = 0; i < static_cast<int>(numVoxels); i++)
			{
				auto rec = reconstructed[vec_i][i];
				//max_val = max_val > rec ? max_val : rec;
				//min_val = min_val < rec ? min_val : rec;
				max_thread = max_thread > rec ? max_thread : rec;
				min_thread = min_thread < rec ? min_thread : rec;
			}
			// Implicit barrier

#pragma omp critical  // Unfortunatelly, no atomic max :(
			max_val = max_thread > max_val ? max_thread : max_val;
#pragma omp critical  // Unfortunatelly, no atomic min :(
			min_val = min_thread < min_val ? min_thread : min_val;

#pragma omp barrier

#pragma omp for nowait
			for (int i = 0; i < static_cast<int>(numVoxels); i++)
			{
				auto old = reconstructed[vec_i][i];
				reconstructed[vec_i][i] = (old - min_val) / (max_val - min_val) * 65535.0;
			}
		}

	}  // end of parallel block

	// Reshape reconstructed vectors into image
	c.resize(numOutputs);
	for (size_t out_i = 0; out_i < numOutputs; out_i++)
	{
		auto recvec = reconstructed.get_row(out_i);

		auto output = ImageType::New();
		typename ImageType::RegionType region;
		region.SetSize(itkImg0->GetLargestPossibleRegion().GetSize());
		region.SetIndex(itkImg0->GetLargestPossibleRegion().GetIndex());
		output->SetRegions(region);
		output->SetSpacing(itkImg0->GetSpacing());
		output->Allocate();
		auto ite = itk::ImageRegionIterator<ImageType>(output, region);

		unsigned int i = 0;
		ite.GoToBegin();
		while (!ite.IsAtEnd())
		{
			double rec = recvec[i];
			typename ImageType::PixelType rec_cast = static_cast<typename ImageType::PixelType>(rec);
			ite.Set(rec_cast);

			++ite;
			++i;
		}

#ifndef NDEBUG
		//storeImage(output, "pca_output_before_conversion_" + QString::number(out_i) + ".mhd", true);
		storeImage(output, "pca_output_" + QString::number(out_i) + ".mhd", true);
#endif

		c[out_i].setImage(output);
	}
}

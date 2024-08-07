// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iANModalPCADataSetReducer.h"

#include <iAImageData.h>
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

// Input datasets must have the exact same dimensions
QList<std::shared_ptr<iAImageData>> iANModalPCADataSetReducer::reduce(
	const QList<std::shared_ptr<iAImageData>>& dataSets_in)
{
	// TODO: assert if all datasets have the same dimensions

	// Set up connectors
	std::vector<iAConnector> connectors(dataSets_in.size());
	for (int i = 0; i < dataSets_in.size(); i++)
	{
		connectors[i] = iAConnector();
		connectors[i].setImage(dataSets_in[i]->vtkImage());
	}

	// Go!
	//ITK_TYPED_CALL(itkPCA, connectors[0].itkScalarType(), connectors);
	ITK_TYPED_CALL(ownPCA, connectors[0].itkScalarType(), connectors);

	// Set up output list
	auto dataSets_out = QList<std::shared_ptr<iAImageData>>();
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

		//std::shared_ptr<iAVolumeRenderer> renderer(new iAVolumeRenderer(renderer, mod->image(), mod->transfer().data()));

		//m_mdiChild->dataDockWidget()->addModality(...);

		dataSets_out.append(dataSet);
	}
	assert(dataSets_out.size() <= maxOutputLength());
	return dataSets_out;
}

template <class T>
void iANModalPCADataSetReducer::itkPCA(std::vector<iAConnector>& c)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType> PCASMEType;

	uint inputSize = static_cast<uint>(c.size());
	uint outputSize = std::min(static_cast<uint>(c.size()), maxOutputLength());

	auto pca = PCASMEType::New();
	pca->SetNumberOfTrainingImages(inputSize);
	pca->SetNumberOfPrincipalComponentsRequired(outputSize);
	for (uint i = 0; i < inputSize; i++)
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
void DebugLogMatrix(vnl_matrix<double> const & matrix, QString const & string)
{
	QString str = string;
	str += "\n";
	for (unsigned int i = 0; i < matrix.rows(); i++)
	{
		auto row = matrix.get_row(i);
		for (size_t j = 0; j < row.size(); j++)
		{
			str += QString::number(row[j]) + "     ";
		}
		str += "\n";
	}
	LOG(lvlDebug, str);
}

void DebugLogVector(vnl_vector<double> const & vector, QString const & string)
{
	QString str = string;
	str += "\n";
	for (size_t i = 0; i < vector.size(); i++)
	{
		str += QString::number(vector[i]) + "     ";
	}
	str += "\n";
	LOG(lvlDebug, str);
}
#else
void DebugLogMatrix(vnl_matrix<double> const& /*matrix*/, QString const& /*string*/) {}
void DebugLogVector(vnl_vector<double> const& /*vector*/, QString const& /*string*/) {}
#endif

template <class T>
void iANModalPCADataSetReducer::ownPCA(std::vector<iAConnector>& c)
{
	typedef itk::Image<T, DIM> ImageType;

	assert(c.size() > 0);

	iATimeGuard tg("Perform PCA");

	auto itkImg0 = c[0].itkImage();

	auto size = itkImg0->GetBufferedRegion().GetSize();
	size_t numVoxels64 = 1;
	for (unsigned int dim_i = 0; dim_i < DIM; dim_i++)
	{
		numVoxels64 *= size[dim_i];
	}
	if (numVoxels64 >= std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Input image (number of voxels: %1) exceeds size that can be handled "
			"(current voxel number maximum: %2)!").arg(numVoxels64).arg(std::numeric_limits<int>::max()));
	}
	uint numVoxels = static_cast<uint>(numVoxels64);
	if (c.size() >= std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("Number of input images (%1) exceeds size that can be handled "
			"(current limit: %2)!").arg(c.size()).arg(std::numeric_limits<int>::max()));
	}
	uint numInputs = static_cast<uint>(c.size());
	uint numOutputs = std::min(static_cast<uint>(c.size()), maxOutputLength());

	// Set up input matrix
	vnl_matrix<double> inputs(numInputs, numVoxels);
	for (uint row_i = 0; row_i < numInputs; row_i++)
	{
		auto input = dynamic_cast<const ImageType*>(c[row_i].itkImage());
		auto iterator = itk::ImageRegionConstIterator<ImageType>(input, input->GetBufferedRegion());
		iterator.GoToBegin();

		for (uint col_i = 0; col_i < numVoxels; col_i++)
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
		for (uint img_i = 0; img_i < numInputs; img_i++)
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
		DebugLogVector(means, "Means");
#endif

		// Calculate inner product (lower triangle) (for covariance matrix)
		for (uint ix = 0; ix < numInputs; ix++)
		{
			for (uint iy = 0; iy <= ix; iy++)
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
			for (uint iy = ix + 1; iy < numInputs; iy++)
			{
				innerProd[ix][iy] = innerProd[iy][ix];
			}
		}

#ifndef NDEBUG
#pragma omp single
		DebugLogMatrix(innerProd, "Inner product");
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
			DebugLogMatrix(innerProd, "Covariance matrix");
#endif

			// Solve eigenproblem
			vnl_matrix<double> eye(numInputs, numInputs);  // (eye)dentity matrix
			eye.set_identity();
			//DebugLogMatrix(eye, "Identity");
			vnl_generalized_eigensystem evecs_evals_innerProd(innerProd, eye);
			evecs_innerProd = evecs_evals_innerProd.V;
			evecs_innerProd.fliplr();  // Flipped because VNL sorts eigenvectors in ascending order
			if (numInputs != numOutputs)
				evecs_innerProd.extract(numInputs, numOutputs);  // Keep only 'numOutputs' columns
				//auto evals_innerProd = evecs_evals_innerProd.D.diagonal();

#ifndef NDEBUG
			DebugLogMatrix(evecs_innerProd, "Eigenvectors");
			DebugLogVector(evecs_evals_innerProd.D.diagonal(), "Eigenvalues");
#endif
		}

		// Initialize the reconstructed matrix (with zeros)
		for (uint row_i = 0; row_i < numOutputs; row_i++)
		{
#pragma omp for nowait
			for (int col_i = 0; col_i < static_cast<int>(numVoxels); col_i++)
			{
				reconstructed[row_i][col_i] = 0;
			}
		}

#pragma omp barrier

		// Transform images to principal components
		for (uint row_i = 0; row_i < numInputs; row_i++)
		{
			for (uint vec_i = 0; vec_i < numOutputs; vec_i++)
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
		for (uint vec_i = 0; vec_i < numOutputs; vec_i++)
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
	for (uint out_i = 0; out_i < numOutputs; out_i++)
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

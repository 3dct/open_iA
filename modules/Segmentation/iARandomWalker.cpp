// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGraphWeights.h"
#include "iAImageGraph.h"
#include "iANormalizerImpl.h"
#include "iASeedType.h"
#include "iAVectorArrayImpl.h"
#include "iAVectorDistanceImpl.h"

#include <defines.h>     // for DIM
#include <iAFilterDefault.h>
#ifndef NDEBUG
#include <iAMathUtility.h>    // for dblApproxEqual used in assert
#endif
#include <iAImageData.h>
#include <iAToolsVTK.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <vtkImageData.h>

#ifdef USE_EIGEN

#include <Eigen/Core>
#pragma GCC diagnostic push
#if __clang_major__ > 12 || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include <Eigen/Sparse>
#pragma GCC diagnostic pop
typedef Eigen::SparseMatrix<double, Eigen::ColMajor> MatrixType;
typedef Eigen::VectorXd VectorType;

#else // using VNL

#include <vnl/vnl_vector.h>
#include <vnl/vnl_sparse_matrix.h>
#include <vnl/vnl_sparse_matrix_linear_system.h>
#include <vnl/algo/vnl_sparse_lu.h>
#include <vnl/algo/vnl_lsqr.h>
#include <vnl/algo/vnl_conjugate_gradient.h>
//<vnl/vnl_conjugate_gradient.h>
typedef vnl_sparse_matrix<double> MatrixType;
typedef vnl_vector<double> VectorType;

#endif

#include <QFile>
#include <QSet>
#include <QTextStream>

IAFILTER_DEFAULT_CLASS(iARandomWalker);
IAFILTER_DEFAULT_CLASS(iAExtendedRandomWalker);

IAFILTER_DEFAULT_CLASS(iAMaximumDecisionRule);
IAFILTER_DEFAULT_CLASS(iALabelImageToSeeds);

namespace
{
	typedef itk::Image<unsigned char, DIM> LabelImageType;
	typedef QMap<iAVertexIndexType, iAVertexIndexType> IndexMap;


	template <class T>
	void CreateLabelImage(
		int const dim[3],
		double const spacing[3],
		QVector<iAITKIO::ImagePointer> const & probabilityImages,
		int labelCount,
		iAITKIO::ImagePointer& labelImgP)
	{
		typedef itk::Image<T, DIM> ProbImageType;
		// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)
		labelImgP = allocateImage(dim, spacing, iAITKIO::ScalarType::UCHAR);
		LabelImageType* labelImg = dynamic_cast<LabelImageType*>(labelImgP.GetPointer());
		QVector<ProbImageType*> probImgs;
		for (int i = 0; i < labelCount; ++i)
		{
			probImgs.push_back(dynamic_cast<ProbImageType*>(probabilityImages[i].GetPointer()));
		}
		// TODO: parallelize? Use/Unify with FCM labelling filter?
		for (int x = 0; x < dim[0]; ++x)
		{
			for (int y = 0; y < dim[1]; ++y)
			{
				for (int z = 0; z < dim[2]; ++z)
				{
					double maxProb = 0;
					int maxProbLabel = -1;
					typename ProbImageType::IndexType idx;
					idx[0] = x;
					idx[1] = y;
					idx[2] = z;
					for (int l = 0; l < labelCount; ++l)
					{
						double prob = probImgs[l]->GetPixel(idx);
						if (prob >= maxProb)
						{
							maxProb = prob;
							maxProbLabel = l;
						}
					}
					labelImg->SetPixel(idx, maxProbLabel);
				}
			}
		}
	}

	template <class T>
	void SetIndexMapValues(iAITKIO::ImagePointer image,
		VectorType const & values,
		IndexMap const & indexMap,
		iAImageCoordConverter const & conv)
	{

		typedef itk::Image<T, DIM> ProbImageType;

		ProbImageType* pImg = dynamic_cast<ProbImageType*>(image.GetPointer());
		for (IndexMap::const_iterator it = indexMap.begin(); it != indexMap.end(); ++it)
		{
#ifdef USE_EIGEN
			double imgVal = values.coeff(it.value());
#else
			double imgVal = values[it.value()];
#endif
			iAImageCoordinate coord = conv.coordinatesFromIndex(it.key());
			typename ProbImageType::IndexType pixelIndex;
			pixelIndex[0] = coord.x;
			pixelIndex[1] = coord.y;
			pixelIndex[2] = coord.z;
			if (imgVal < 0 || imgVal > 1 || qIsInf(imgVal) || qIsNaN(imgVal))
			{
				/*
				addMsg(QString("Invalid pixel value at (%1, %2, %3): %4")
					.arg(pixelIndex[0]).arg(pixelIndex[1]).arg(pixelIndex[2]).arg(imgVal));
				*/
				imgVal = 0;
			}
			pImg->SetPixel(pixelIndex,	imgVal);
		}
	}

	void CreateLaplacianPart(MatrixType & output,
		IndexMap const & rowIndices,
		IndexMap const & colIndices,
		iAImageGraph & imageGraph,
		std::shared_ptr<iAGraphWeights const> finalWeight,
		QVector<double> const & vertexWeightSum,
		iAVertexIndexType vertexCount,
		bool noSameIndices = false)
	{
#ifdef USE_EIGEN
		output.reserve(Eigen::VectorXi::Constant(rowIndices.size(), 7));
#endif
		// edge weights:
		for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.edgeCount(); ++edgeIdx)
		{
			iAEdgeType const & edge = imageGraph.edge(edgeIdx);
			if (rowIndices.contains(edge.first) && colIndices.contains(edge.second))
			{
				iAVertexIndexType newRowIdx = rowIndices[edge.first];
				iAVertexIndexType newColIdx = colIndices[edge.second];
#ifdef USE_EIGEN
				output.insert(newRowIdx, newColIdx) = -finalWeight->GetWeight(edgeIdx);
#else
				output(newRowIdx, newColIdx) = -finalWeight->GetWeight(edgeIdx);
#endif
			}
			if (rowIndices.contains(edge.second) && colIndices.contains(edge.first))
			{
				iAVertexIndexType newRowIdx = rowIndices[edge.second];
				iAVertexIndexType newColIdx = colIndices[edge.first];
#ifdef USE_EIGEN
				output.insert(newRowIdx, newColIdx) = -finalWeight->GetWeight(edgeIdx);
#else
				output(newRowIdx, newColIdx) = -finalWeight->GetWeight(edgeIdx);
#endif
			}
		}
		if (noSameIndices)
		{ // optimization: if rowIndices and colIndices don't share any elements, there is no diagonal entry
			return;
		}
		// sum of incident edge weights at diagonal:
		for (iAVertexIndexType vertexIdx = 0; vertexIdx < vertexCount; ++vertexIdx)
		{
			if (rowIndices.contains(vertexIdx) && colIndices.contains(vertexIdx))
			{
				iAVertexIndexType newRowIdx = rowIndices[vertexIdx];
				iAVertexIndexType newColIdx = colIndices[vertexIdx];
#ifdef USE_EIGEN
				output.insert(newRowIdx, newColIdx) = vertexWeightSum[vertexIdx];
#else
				output(newRowIdx, newColIdx) = vertexWeightSum[vertexIdx];
#endif
			}
		}
	}

	struct iARWInputChannel
	{
		std::shared_ptr<iAVectorArray const> image;
		std::shared_ptr<iAVectorDistance const> distanceFunc;
		std::shared_ptr<iANormalizer> normalizeFunc;
		double weight;
	};

	void AddCommonRWParameters(iAFilter* filter)
	{
		QStringList distanceFunctions;
		for (int i = 0; i < dmCount; ++i)
			distanceFunctions << GetDistanceMeasureNames()[i];
		QStringList normalizeFunctions;
		for (int j = 0; j < nmCount; j++)
			normalizeFunctions << GetNormalizerNames()[j];
		filter->addParameter("Beta", iAValueType::Continuous, 100);
		filter->addParameter("Distance Function", iAValueType::Categorical, distanceFunctions);
		filter->addParameter("Normalizer", iAValueType::Categorical, normalizeFunctions);
	}
	QString CommonRWParameterDescription("The <em>Distance Function</em> "
		"determines how the distance between two data points is calculated."
		"The <em>Normalizer</em> determines how these distances (used as weights "
		"are normalized; <em>Beta</em> is a parameter to the Gaussian Normalizer. ");
}


iARandomWalker::iARandomWalker() :
	iAFilter("Random Walker", "Segmentation/Graph-based",
		"Computes the Random Walker segmentation.<br/>" +
		CommonRWParameterDescription +
		"As <em>Seeds</em>, specify text with one seed point per line in the following format:"
		"<pre>x y z label</pre>"
		"where x, y and z are the coordinates (set z = 0 for 2D images) and label is the index of the label "
		"for this seed point. Label indices should start at 0 and be contiguous (so if you have N different "
		"labels, you should use label indices 0..N - 1 and make sure that there is at least one seed per label).<br/>"
		"For more information see "
		"<a href=\"http://leogrady.net/publications/\">Leo Grady's website "
		"(inventor of the algorithm)</a>")
{
	AddCommonRWParameters(this);
	addParameter("Seeds", iAValueType::Text, "");
}

void iARandomWalker::performWork(QVariantMap const & parameters)
{
	int const * dim = imageInput(0)->vtkImage()->GetDimensions();
	double const * spc = imageInput(0)->vtkImage()->GetSpacing();
	QVector<iARWInputChannel> inputChannels;
	iARWInputChannel inputChannel;
	auto vtkPixelAccess = std::make_shared<iAvtkPixelVectorArray>(dim);
	for (size_t i = 0; i < inputCount(); ++i)
	{
		vtkPixelAccess->AddImage(imageInput(i)->vtkImage());
	}
	inputChannel.image = vtkPixelAccess;
	inputChannel.distanceFunc = GetDistanceMeasure(parameters["Distance Function"].toString());
	inputChannel.normalizeFunc = CreateNormalizer(parameters["Normalizer"].toString(), parameters["Beta"].toDouble());
	inputChannel.weight = 1.0;
	inputChannels.push_back(inputChannel);
	iAVertexIndexType vertexCount = static_cast<iAVertexIndexType>(dim[0]) * dim[1] * dim[2];
	iAImageGraph imageGraph(dim[0], dim[1], dim[2], iAImageCoordinate::ColRowDepMajor);
	iASeedsPointer seeds = ExtractSeedVector(parameters["Seeds"].toString(), dim[0], dim[1], dim[2]);
	int minLabel = std::numeric_limits<int>::max(),
		maxLabel = std::numeric_limits<int>::lowest();
	for (int i = 0; i<seeds->size(); ++i)
	{
		int label = seeds->at(i).second;
		if (label < minLabel)
		{
			minLabel = label;
		}
		if (label > maxLabel)
		{
			maxLabel = label;
		}
	}
	if (inputChannels.size() == 0)
	{
		addMsg("Input Channels must not be empty!");
		return;
	}
	if (seeds->size() == 0)
	{
		addMsg("Seeds must not be empty!");
		return;
	}
	if (minLabel != 0)
	{
		addMsg("Labels must start at 0");
		return;
	}

	IndexMap seedMap;
	QSet<int> labelSet;
	for (iAVertexIndexType seedIdx = 0; seedIdx < static_cast<iAVertexIndexType>(seeds->size()); ++seedIdx)
	{
		seedMap.insert(imageGraph.converter().indexFromCoordinates(seeds->at(seedIdx).first), seedIdx);
		labelSet.insert(seeds->at(seedIdx).second);
	}
	int labelCount = labelSet.size();
	if (maxLabel != labelCount - 1)
	{
		addMsg("Labels must be consecutive from 0 .. maxLabel !");
		return;
	}

	QVector<std::shared_ptr<iAGraphWeights> > graphWeights(inputChannels.size());
	QVector<double> weightsForChannels(inputChannels.size());
	for (int i = 0; i<inputChannels.size(); ++i)
	{
		auto currentMeasureWeight =
			CalculateGraphWeights(imageGraph, *inputChannels[i].image, *inputChannels[i].distanceFunc);
		graphWeights[i] = currentMeasureWeight;
		weightsForChannels[i] = inputChannels[i].weight;
	}

	for (int i = 0; i<inputChannels.size(); ++i)
	{
		graphWeights[i]->Normalize(inputChannels[i].normalizeFunc);
	}

	auto finalWeight = CombineGraphWeights(graphWeights, weightsForChannels);

	QVector<double> vertexWeightSum(vertexCount);
	for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.edgeCount(); ++edgeIdx)
	{
		iAEdgeType const & edge = imageGraph.edge(edgeIdx);
		vertexWeightSum[edge.first] += finalWeight->GetWeight(edgeIdx);
		vertexWeightSum[edge.second] += finalWeight->GetWeight(edgeIdx);
	}

	IndexMap unlabeledMap;
	for (iAVertexIndexType vertexIdx = 0, newIdx = 0;
		vertexIdx < vertexCount; ++vertexIdx)
	{
		if (!seedMap.contains(vertexIdx)) {
			unlabeledMap.insert(vertexIdx, newIdx);
			++newIdx;
		}
	}
	int seedCount = seedMap.size();

	MatrixType A(vertexCount - seedCount, vertexCount - seedCount);
	CreateLaplacianPart(A, unlabeledMap, unlabeledMap, imageGraph, finalWeight, vertexWeightSum, vertexCount);
#ifdef USE_EIGEN
	A.makeCompressed();
#endif

	MatrixType BT(vertexCount - seedCount, seedCount);
	CreateLaplacianPart(BT, unlabeledMap, seedMap, imageGraph, finalWeight, vertexWeightSum, vertexCount, true);
	BT = -BT;
#ifdef USE_EIGEN
	BT.makeCompressed();
#endif

	// Conjugate Gradient: very fast and small memory usage,
	// but apparently not ideal for Random Walker (just for Extended RW)!
	// Eigen::ConjugateGradient<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;
	// solver.setMaxIterations(parameters["Maximum Iterations"].toUInt());

#ifdef USE_EIGEN
	Eigen::SparseLU<Eigen::SparseMatrix<double, Eigen::ColMajor>, Eigen::COLAMDOrdering<int> > solver;
	solver.analyzePattern(A);
	std::string error = solver.lastErrorMessage();
	if (error != "")
	{
		addMsg(QString(error.c_str()));
		return;
	}
	solver.factorize(A);
	error = solver.lastErrorMessage();
	if (error != "")
	{
		addMsg(QString(error.c_str()));
		return;
	}
#else
	vnl_sparse_lu linear_solver(A, vnl_sparse_lu::quiet);
#endif

	// BiCGSTAB: uses a bit more memory, but is a bit faster!
	// Eigen::BiCGSTAB<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;
	// solver.compute(A);
	QVector<iAITKIO::ImagePointer> probImgs;
	for (int i = 0; i<labelCount; ++i)
	{
		VectorType boundary(seedCount);
		for (iAVertexIndexType seedIdx = 0; seedIdx < static_cast<iAVertexIndexType>(seeds->size()); ++seedIdx)
		{
			boundary[seedIdx] = seeds->at(seedIdx).second == i;
		}
		VectorType b(vertexCount - seedCount);
#ifdef USE_EIGEN
		b = BT * boundary;
#else
		BT.mult(boundary, b);
#endif
		VectorType x(vertexCount - seedCount);
#ifdef USE_EIGEN

		x = solver.solve(b);
#else
		linear_solver.solve(b, &x);
#endif
		// put values into probability image
		iAITKIO::ImagePointer pImg = allocateImage(dim, spc, iAITKIO::ScalarType::DOUBLE);
		ITK_TYPED_CALL(SetIndexMapValues, inputScalarType(), pImg, x, unlabeledMap, imageGraph.converter());
		ITK_TYPED_CALL(SetIndexMapValues, inputScalarType(), pImg, boundary, seedMap, imageGraph.converter());
		probImgs.push_back(pImg);
	}
	iAITKIO::ImagePointer labelImg;
	ITK_TYPED_CALL(CreateLabelImage, inputScalarType(), dim, spc, probImgs, labelCount, labelImg );
	addOutput(std::make_shared<iAImageData>(labelImg));
	setOutputName(0u, "Label Image");
	for (int i = 0; i < labelCount; ++i)
	{
		addOutput(std::make_shared<iAImageData>(probImgs[i]));
		setOutputName(static_cast<unsigned int>(1+i), QString("Probability image label %1").arg(i));
	}
}


// iAExtendedRandomWalker

iAExtendedRandomWalker::iAExtendedRandomWalker() :
	iAFilter("Extended Random Walker", "Segmentation/Graph-based",
		"Computes the Extended Random Walker segmentation.<br/>"
		"This requires a prior, a multi-channel image containing for each "
		"voxel and label the probability that the voxel belongs to that "
		"label.<br/>"
		"Every channel in the given prior is considered to "
		"be a prior probability image for one label, the number of target "
		"labels is thus derived from the number of specified priors.<br/>"
		+ CommonRWParameterDescription +
		"The <em>Gamma</em> parameter determines the weight of the prior model "
		"in comparison to the weights from the image gradients (thus, the higher "
		"gamma, the closer will the resulting values be to the original prior). "
		"<em>Maximum iterations</em> limits the number of iterations done in the "
		"internally used iterative linear equation solver.<br/>"
		"For more information see "
		"<a href=\"http://leogrady.net/publications/\">Leo Grady's website "
		"(inventor of the algorithm)</a>", 2)
{
	AddCommonRWParameters(this);
	addParameter("Maximum Iterations", iAValueType::Discrete, 100);
	addParameter("Gamma", iAValueType::Continuous, 1);
}

void iAExtendedRandomWalker::performWork(QVariantMap const & parameters)
{
	int const * dim = imageInput(0)->vtkImage()->GetDimensions();
	double const * spc = imageInput(0)->vtkImage()->GetSpacing();
	QVector<iARWInputChannel> inputChannels;
	iARWInputChannel inputChannel;
	auto vtkPixelAccess = std::make_shared<iAvtkPixelVectorArray>(dim);
	for (int i = 0; static_cast<unsigned int>(i) < firstInputChannels(); ++i)
	{
		vtkPixelAccess->AddImage(imageInput(i)->vtkImage());
	}
	inputChannel.image = vtkPixelAccess;
	inputChannel.distanceFunc = GetDistanceMeasure(parameters["Distance Function"].toString());
	inputChannel.normalizeFunc = CreateNormalizer(parameters["Normalizer"].toString(), parameters["Beta"].toDouble());
	inputChannel.weight = 1.0;
	inputChannels.push_back(inputChannel);
	iAVertexIndexType vertexCount = static_cast<iAVertexIndexType>(dim[0]) * dim[1] * dim[2];
	iAImageGraph imageGraph(dim[0], dim[1], dim[2], iAImageCoordinate::ColRowDepMajor);

	std::vector<iAImageData const*> priorModel;
	for (size_t p = firstInputChannels(); p < inputCount(); ++p)
	{
		priorModel.push_back(imageInput(p));
	}

	if (inputChannels.empty())
	{
		addMsg("Input Channels must not be empty!");
		return;
	}

	QVector<std::shared_ptr<iAGraphWeights> > graphWeights(inputChannels.size());
	QVector<double> weightsForChannels(inputChannels.size());
	for (int i = 0; i<inputChannels.size(); ++i)
	{
		auto currentMeasureWeight =
			CalculateGraphWeights(imageGraph, *inputChannels[i].image, *inputChannels[i].distanceFunc);
		graphWeights[i] = currentMeasureWeight;
		weightsForChannels[i] = inputChannels[i].weight;
	}

	for (int i = 0; i<inputChannels.size(); ++i)
	{
		graphWeights[i]->Normalize(inputChannels[i].normalizeFunc);
	}

	auto finalWeight =
		CombineGraphWeights(graphWeights, weightsForChannels);

	QVector<double> vertexWeightSum(vertexCount);
	for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.edgeCount(); ++edgeIdx)
	{
		iAEdgeType const & edge = imageGraph.edge(edgeIdx);
		vertexWeightSum[edge.first] += finalWeight->GetWeight(edgeIdx);
		vertexWeightSum[edge.second] += finalWeight->GetWeight(edgeIdx);
	}
	// perf.time("ERW: vertex weight sums");

	//bool priorNormalized = true;
	// add priors into vertexWeightSum:
	// if my thinking is correct it should be enough to add the weight factor to each entry,
	// since for one voxel, the probabilities for all labels should add up to 1!
	int labelCount = priorModel.size();
	for (iAVoxelIndexType voxelIdx = 0; static_cast<unsigned int>(voxelIdx) < vertexCount; ++voxelIdx)
	{
		double sum = 0;

		//PriorModelImageType::IndexType idx;
		iAImageCoordinate coord = imageGraph.converter().coordinatesFromIndex(voxelIdx);
		/*
		idx[0] = coord.x;
		idx[1] = coord.y;
		idx[2] = coord.z;
		*/
		for (int labelIdx = 0; labelIdx < labelCount; ++labelIdx)
		{
			//sum += (*m_priorModel)[labelIdx]->GetPixel(idx);
			double value = priorModel[labelIdx]->vtkImage()->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
			sum += value;
		}
		assert (dblApproxEqual(sum, 1.0, 1e-6) );
		//if (std::abs(sum-1.0) >= EPSILON)
		//{
		//priorNormalized = false;
		//DebugOut() << "Prior Model not normalized at (x="<<coord.x<<", y="<<coord.y<<", z="<<coord.z<<"): "<< sum << std::endl;
		//}
		vertexWeightSum[voxelIdx] += (parameters["Gamma"].toDouble() * sum);
	}
	//if (!priorNormalized)
	//{
	//	DebugOut() << "Prior Model not normalized." << std::endl;
	//}

	IndexMap fullMap;
	for(iAVertexIndexType vertexIdx=0;
	vertexIdx < vertexCount; ++vertexIdx)
	{
		fullMap.insert(vertexIdx, vertexIdx);
	}

	MatrixType A(vertexCount,  vertexCount);
	CreateLaplacianPart(A, fullMap, fullMap, imageGraph, finalWeight, vertexWeightSum, vertexCount);
#ifdef USE_EIGEN
	A.makeCompressed();
	/*
	Sparse LU (with both eigen and VNL):
	- very slow
	- uses lots and lots of memory!
	*/
	// Conjugate Gradient: very fast and small memory usage!
	Eigen::ConjugateGradient<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;

	// TODO:
	//    - check if matrix is positive-definite
	//    - find a good maximum number of iterations!
	solver.setMaxIterations(parameters["Maximum Iterations"].toUInt());

	// BiCGSTAB seems to be a bit faster than Conjugate Gradient, but using more memory
	//Eigen::BiCGSTAB<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;
	solver.compute(A);
	//#else
	// in case sparse LU should be used, do this:
	//	vnl_sparse_lu linear_solver(A, vnl_sparse_lu::quiet);
#endif

	// perf.time("ERW: solver compute done");

	QVector<iAITKIO::ImagePointer> probImgs;
	for (int i=0; i<labelCount; ++i)
	{
		VectorType priorForLabel(vertexCount);
		// fill from image
		for (iAVoxelIndexType voxelIdx = 0; static_cast<unsigned int>(voxelIdx) < vertexCount; ++ voxelIdx)
		{
			//PriorModelImageType::IndexType idx;
			iAImageCoordinate coord = imageGraph.converter().coordinatesFromIndex(voxelIdx);
			/*
			idx[0] = coord.x;
			idx[1] = coord.y;
			idx[2] = coord.z;
			*/
			priorForLabel[voxelIdx] = priorModel[i]->vtkImage()->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
		}

		VectorType x(vertexCount);
#ifdef USE_EIGEN
		x = solver.solve(priorForLabel);
#else
		vnl_sparse_matrix_linear_system<double> problem(A, priorForLabel);
		vnl_lsqr solver(problem);
		/*int returnCode =*/ solver.minimize(x);
		// in case sparse LU should be used, do this instead:
		//linear_solver.solve(priorForLabel, &x);
#endif
		// put values into probability image
		iAITKIO::ImagePointer pImg = allocateImage(dim, spc, iAITKIO::ScalarType::DOUBLE);
		ITK_TYPED_CALL(SetIndexMapValues, inputScalarType(), pImg, x, fullMap, imageGraph.converter());
		probImgs.push_back(pImg);
	}
	// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)

	iAITKIO::ImagePointer labelImg;
	ITK_TYPED_CALL(CreateLabelImage, inputScalarType(), dim, spc, probImgs, labelCount, labelImg);
	addOutput(std::make_shared<iAImageData>(labelImg));
	setOutputName(0u, "Label Image");
	for (int i = 0; i < labelCount; ++i)
	{
		addOutput(std::make_shared<iAImageData>(probImgs[i]));
		setOutputName(static_cast<unsigned int>(1 + i), QString("Probability image label %1").arg(i));
	}
}


iAMaximumDecisionRule::iAMaximumDecisionRule() :
	iAFilter("Maximum Decision Rule", "Segmentation",
		"Assign each pixel the label with maximum probability.<br/>"
		"Applies the maximum decision rule to a multichannel input which "
		"represents a probability distribution over labels. <br/>"
		"Output is an unsigned char (256 classes possible)")
{
}

void iAMaximumDecisionRule::performWork(QVariantMap const & /*parameters*/)
{
	if (inputCount() <= 1)
	{
		throw std::invalid_argument("Input has to have at least two channels!");
	}
	int const * dim = imageInput(0)->vtkImage()->GetDimensions();
	double const * spc = imageInput(0)->vtkImage()->GetSpacing();
	QVector<iAITKIO::ImagePointer> probImgs;
	for (size_t i = 0; i < inputCount(); ++i)
	{
		probImgs.push_back(imageInput(i)->itkImage());
	}
	iAITKIO::ImagePointer labelImg;
	ITK_TYPED_CALL(CreateLabelImage, inputScalarType(), dim, spc, probImgs, inputCount(), labelImg);
	addOutput(std::make_shared<iAImageData>(labelImg));
}


iALabelImageToSeeds::iALabelImageToSeeds() :
	iAFilter("Label Image to Seeds", "Segmentation",
		"Create a text file containing position and label value for all non-zero voxels in a label image.<br/>"
		"The input to this filter needs to be a label image (i.e., integer), "
		"the output will be a text file containing entries like this: <br/>"
		"x y z label<br/>"
		"This text file can be used as input for segmentation algorithms such as"
		"Random Walker or (Probabilistic) Support Vector Machines.", 1, 0)
{
	addParameter("File name", iAValueType::FileNameSave, ".txt");
}

void iALabelImageToSeeds::performWork(QVariantMap const& parameters)
{
	QString const& fileName = parameters["File name"].toString();
	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
	{
		LOG(lvlError, QString("Couldn't open file %1").arg(fileName));
		return;
	}
	QTextStream out(&f);
	auto img = imageInput(0)->vtkImage();
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		auto pixelValue = img->GetScalarComponentAsDouble(x, y, z, 0);
		if (pixelValue != 0)
		{
			out << x << " " << y << " " << z << " " << pixelValue << Qt::endl;
		}
	}
	f.close();
}

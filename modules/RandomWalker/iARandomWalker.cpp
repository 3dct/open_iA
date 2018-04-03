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
#include "iARandomWalker.h"

#include "defines.h"     // for DIM
#include "iAConnector.h"
#include "iAGraphWeights.h"
#include "iAImageGraph.h"
#include "iAMathUtility.h"
#include "iANormalizerImpl.h"
#include "iASeedType.h"
#include "iATypedCallHelper.h"
#include "iAToolsITK.h"
#include "iAVectorArrayImpl.h"
#include "iAVectorDistanceImpl.h"

#include <vtkImageData.h>

#include <QSet>

#ifdef USE_EIGEN

#include <Eigen/Core>
#include <Eigen/Sparse>
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


namespace
{
	typedef itk::Image<int, DIM> LabelImageType;
	typedef itk::Image<double, DIM> ProbImageType;
	typedef QMap<iAVertexIndexType, iAVertexIndexType> IndexMap;

	iAITKIO::ImagePointer CreateLabelImage(
		int const dim[3],
		double const spacing[3],
		QVector<iAITKIO::ImagePointer> const & probabilityImages,
		int labelCount)
	{
		// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)
		iAITKIO::ImagePointer labelImgP = AllocateImage(dim, spacing, itk::ImageIOBase::INT);
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
					ProbImageType::IndexType idx;
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
		return labelImgP;
	}

	void SetIndexMapValues(iAITKIO::ImagePointer image,
		VectorType const & values,
		IndexMap const & indexMap,
		iAImageCoordConverter const & conv)
	{
		ProbImageType* pImg = dynamic_cast<ProbImageType*>(image.GetPointer());
		for (IndexMap::const_iterator it = indexMap.begin(); it != indexMap.end(); ++it)
		{
#ifdef USE_EIGEN
			double imgVal = values.coeff(it.value());
#else
			double imgVal = values[it.value()];
#endif
			iAImageCoordinate coord = conv.GetCoordinatesFromIndex(it.key());
			ProbImageType::IndexType pixelIndex;
			pixelIndex[0] = coord.x;
			pixelIndex[1] = coord.y;
			pixelIndex[2] = coord.z;
			if (imgVal < 0 || imgVal > 1 || isInf(imgVal) || isNaN(imgVal))
			{
				/*
				AddMsg(QString("Invalid pixel value at (%1, %2, %3): %4")
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
		QSharedPointer<iAGraphWeights const> finalWeight,
		QVector<double> const & vertexWeightSum,
		int vertexCount,
		bool noSameIndices = false)
	{
#ifdef USE_EIGEN
		output.reserve(Eigen::VectorXi::Constant(rowIndices.size(), 7));
#endif
		// edge weights:
		for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.GetEdgeCount(); ++edgeIdx)
		{
			iAEdgeType const & edge = imageGraph.GetEdge(edgeIdx);
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
		QSharedPointer<iAVectorArray const> image;
		QSharedPointer<iAVectorDistance const> distanceFunc;
		QSharedPointer<iANormalizer> normalizeFunc;
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
		filter->AddParameter("Beta", Continuous, 100);
		filter->AddParameter("Distance Function", Categorical, distanceFunctions);
		filter->AddParameter("Normalizer", Categorical, normalizeFunctions);
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
	AddParameter("Seeds", Text, "");
}

IAFILTER_CREATE(iARandomWalker)

void iARandomWalker::PerformWork(QMap<QString, QVariant> const & parameters)
{
	int const * dim = Input()[0]->GetVTKImage()->GetDimensions();
	double const * spc = Input()[0]->GetVTKImage()->GetSpacing();
	QVector<iARWInputChannel> inputChannels;
	iARWInputChannel input;
	auto vtkPixelAccess = QSharedPointer<iAvtkPixelVectorArray>(new iAvtkPixelVectorArray(dim));
	for (int i = 0; i < Input().size(); ++i)
	{
		vtkPixelAccess->AddImage(Input()[i]->GetVTKImage());
	}
	input.image = vtkPixelAccess;
	input.distanceFunc = GetDistanceMeasure(parameters["Distance Function"].toString());
	input.normalizeFunc = CreateNormalizer(parameters["Normalizer"].toString(), parameters["Beta"].toDouble());
	input.weight = 1.0;
	inputChannels.push_back(input);
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
		AddMsg("Input Channels must not be empty!");
		return;
	}
	if (seeds->size() == 0)
	{
		AddMsg("Seeds must not be empty!");
		return;
	}
	if (minLabel != 0)
	{
		AddMsg("Labels must start at 0");
		return;
	}

	IndexMap seedMap;
	QSet<int> labelSet;
	for (iAVertexIndexType seedIdx = 0; seedIdx < seeds->size(); ++seedIdx)
	{
		seedMap.insert(imageGraph.GetConverter().GetIndexFromCoordinates(seeds->at(seedIdx).first), seedIdx);
		labelSet.insert(seeds->at(seedIdx).second);
	}
	int labelCount = labelSet.size();
	if (maxLabel != labelCount - 1)
	{
		AddMsg("Labels must be consecutive from 0 .. maxLabel !");
		return;
	}

	QVector<QSharedPointer<iAGraphWeights> > graphWeights(inputChannels.size());
	QVector<double> weightsForChannels(inputChannels.size());
	for (int i = 0; i<inputChannels.size(); ++i)
	{
		QSharedPointer<iAGraphWeights> currentMeasureWeight =
			CalculateGraphWeights(imageGraph, *inputChannels[i].image, *inputChannels[i].distanceFunc);
		graphWeights[i] = currentMeasureWeight;
		weightsForChannels[i] = inputChannels[i].weight;
	}

	for (int i = 0; i<inputChannels.size(); ++i)
	{
		graphWeights[i]->Normalize(inputChannels[i].normalizeFunc);
	}

	QSharedPointer<const iAGraphWeights > finalWeight =
		CombineGraphWeights(graphWeights, weightsForChannels);

	QVector<double> vertexWeightSum(vertexCount);
	for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.GetEdgeCount(); ++edgeIdx)
	{
		iAEdgeType const & edge = imageGraph.GetEdge(edgeIdx);
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
		AddMsg(QString(error.c_str()));
		return;
	}
	solver.factorize(A);
	error = solver.lastErrorMessage();
	if (error != "")
	{
		AddMsg(QString(error.c_str()));
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
		for (iAVertexIndexType seedIdx = 0; seedIdx < seeds->size(); ++seedIdx)
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
		iAITKIO::ImagePointer pImg = AllocateImage(dim, spc, itk::ImageIOBase::DOUBLE);
		SetIndexMapValues(pImg, x, unlabeledMap, imageGraph.GetConverter());
		SetIndexMapValues(pImg, boundary, seedMap, imageGraph.GetConverter());
		probImgs.push_back(pImg);
	}
	auto labelImg = CreateLabelImage(dim, spc, probImgs, labelCount);
	AddOutput(labelImg);
	for (int i=0; i<labelCount; ++i)
		AddOutput(probImgs[i]);
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
	AddParameter("Maximum Iterations", Discrete, 100);
	AddParameter("Gamma", Continuous, 1);
}

IAFILTER_CREATE(iAExtendedRandomWalker)


const double EPSILON = 1e-6;

void iAExtendedRandomWalker::PerformWork(QMap<QString, QVariant> const & parameters)
{
	int const * dim = Input()[0]->GetVTKImage()->GetDimensions();
	double const * spc = Input()[0]->GetVTKImage()->GetSpacing();
	QVector<iARWInputChannel> inputChannels;
	iARWInputChannel input;
	auto vtkPixelAccess = QSharedPointer<iAvtkPixelVectorArray>(new iAvtkPixelVectorArray(dim));
	for (int i = 0; i < FirstInputChannels(); ++i)
	{
		vtkPixelAccess->AddImage(Input()[i]->GetVTKImage());
	}
	input.image = vtkPixelAccess;
	input.distanceFunc = GetDistanceMeasure(parameters["Distance Function"].toString());
	input.normalizeFunc = CreateNormalizer(parameters["Normalizer"].toString(), parameters["Beta"].toDouble());
	input.weight = 1.0;
	inputChannels.push_back(input);
	iAVertexIndexType vertexCount = static_cast<iAVertexIndexType>(dim[0]) * dim[1] * dim[2];
	iAImageGraph imageGraph(dim[0], dim[1], dim[2], iAImageCoordinate::ColRowDepMajor);

	QVector<iAConnector*> priorModel;
	for (int p = FirstInputChannels(); p < Input().size(); ++p)
	{
		priorModel.push_back(Input()[p]);
	}

	if (inputChannels.size() == 0)
	{
		AddMsg("Input Channels must not be empty!");
		return;
	}

	QVector<QSharedPointer<iAGraphWeights> > graphWeights(inputChannels.size());
	QVector<double> weightsForChannels(inputChannels.size());
	for (int i = 0; i<inputChannels.size(); ++i)
	{
		QSharedPointer<iAGraphWeights> currentMeasureWeight =
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
	for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph.GetEdgeCount(); ++edgeIdx)
	{
		iAEdgeType const & edge = imageGraph.GetEdge(edgeIdx);
		vertexWeightSum[edge.first] += finalWeight->GetWeight(edgeIdx);
		vertexWeightSum[edge.second] += finalWeight->GetWeight(edgeIdx);
	}
	// perf.time("ERW: vertex weight sums");

	//bool priorNormalized = true;
	// add priors into vertexWeightSum:
	// if my thinking is correct it should be enough to add the weight factor to each entry,
	// since for one voxel, the probabilities for all labels should add up to 1!
	int labelCount = priorModel.size();
	for (iAVoxelIndexType voxelIdx = 0; voxelIdx < vertexCount; ++voxelIdx)
	{
		double sum = 0;

		//PriorModelImageType::IndexType idx;
		iAImageCoordinate coord = imageGraph.GetConverter().GetCoordinatesFromIndex(voxelIdx);
		/*
		idx[0] = coord.x;
		idx[1] = coord.y;
		idx[2] = coord.z;
		*/
		for (int labelIdx = 0; labelIdx < labelCount; ++labelIdx)
		{
			//sum += (*m_priorModel)[labelIdx]->GetPixel(idx);
			double value = priorModel[labelIdx]->GetVTKImage()->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
			sum += value;
		}
		assert (std::abs(sum-1.0) < EPSILON);
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
	for(iAVertexIndexType vertexIdx=0, newIdx=0;
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
		for (iAVoxelIndexType voxelIdx = 0; voxelIdx < vertexCount; ++ voxelIdx)
		{
			//PriorModelImageType::IndexType idx;
			iAImageCoordinate coord = imageGraph.GetConverter().GetCoordinatesFromIndex(voxelIdx);
			/*
			idx[0] = coord.x;
			idx[1] = coord.y;
			idx[2] = coord.z;
			*/
			priorForLabel[voxelIdx] = priorModel[i]->GetVTKImage()->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
		}

		VectorType x(vertexCount);
#ifdef USE_EIGEN
		x = solver.solve(priorForLabel);
#else
		vnl_sparse_matrix_linear_system<double> problem(A, priorForLabel);
		vnl_lsqr solver(problem);
		int returnCode = solver.minimize(x);
		// in case sparse LU should be used, do this instead:
		//linear_solver.solve(priorForLabel, &x);
#endif
		// put values into probability image
		iAITKIO::ImagePointer pImg = AllocateImage(dim, spc, itk::ImageIOBase::DOUBLE);
		SetIndexMapValues(pImg, x, fullMap, imageGraph.GetConverter());
		probImgs.push_back(pImg);
	}
	// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)
	auto labelImg = CreateLabelImage(dim, spc, probImgs, labelCount);
	AddOutput(labelImg);
	for (int i=0; i<labelCount; ++i)
		AddOutput(probImgs[i]);
}


iAMaximumDecisionRule::iAMaximumDecisionRule() :
	iAFilter("Maximum Decision Rule", "Segmentation",
		"Assign each pixel the label with maximum probability.<br/>"
		"Applies the maximum decision rule to a multichannel input which "
		"represents a probability distribution over labels.")
{
}

IAFILTER_CREATE(iAMaximumDecisionRule)

void iAMaximumDecisionRule::PerformWork(QMap<QString, QVariant> const & parameters)
{
	if (Input().size() <= 1)
	{
		throw std::invalid_argument("Input has to have at least two channels!");
		return;
	}
	int const * dim = Input()[0]->GetVTKImage()->GetDimensions();
	double const * spc = Input()[0]->GetVTKImage()->GetSpacing();
	QVector<iAITKIO::ImagePointer> probImgs;
	for (int i = 0; i < Input().size(); ++i)
	{
		probImgs.push_back(Input()[i]->GetITKImage());
	}
	auto labelImg = CreateLabelImage(dim, spc, probImgs, Input().size());
	AddOutput(labelImg);
}

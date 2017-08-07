/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAConsole.h"
#include "iAGraphWeights.h"
#include "iAImageGraph.h"
#include "iAMathUtility.h"
#include "iAVectorDistance.h"
#include "iAToolsITK.h"

#include <itkImage.h>

#include <vtkImageData.h>

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

#include <QMap>
#include <QMessageBox>
#include <QSet>

namespace
{
	
	// TODO: TEST!

	bool SeedsContainIdx(QVector<std::pair<iAImageCoordinate, iALabelType> > const & seeds,
		iAVertexIndexType vertexIdx, iAImageCoordConverter const & conv)
	{
		for (iAVertexIndexType i = 0; i < seeds.size(); ++i)
		{
			if (conv.GetIndexFromCoordinates(seeds[i].first) == vertexIdx)
			{
				return true;
			}
		}
		return false;
	}

	typedef QMap<iAVertexIndexType, iAVertexIndexType> IndexMap;

	void CreateLaplacianPart( MatrixType & output,
		IndexMap const & rowIndices,
		IndexMap const & colIndices,
		QSharedPointer<iAImageGraph> imageGraph,
		QSharedPointer<iAGraphWeights const> finalWeight,
		QVector<double> const & vertexWeightSum,
		int vertexCount,
		bool noSameIndices = false)
	{
#ifdef USE_EIGEN
		output.reserve(Eigen::VectorXi::Constant(rowIndices.size(), 7) );
#endif
		// edge weights:
		for (iAEdgeIndexType edgeIdx = 0; edgeIdx < imageGraph->GetEdgeCount(); ++edgeIdx)
		{
			iAEdgeType const & edge = imageGraph->GetEdge(edgeIdx);
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

	void SetIndexMapValues(iAITKIO::ImagePointer image,
		VectorType const & values,
		IndexMap const & indexMap,
		iAImageCoordConverter const & conv)
	{
		ProbabilityImageType* pImg = dynamic_cast<ProbabilityImageType*>(image.GetPointer());
		for (IndexMap::const_iterator it = indexMap.begin(); it != indexMap.end(); ++it)
		{
#ifdef USE_EIGEN
			double imgVal = values.coeff(it.value());
#else
			double imgVal = values[it.value()];
#endif
			iAImageCoordinate coord = conv.GetCoordinatesFromIndex(it.key());
			ProbabilityImageType::IndexType pixelIndex;
			pixelIndex[0] = coord.x;
			pixelIndex[1] = coord.y;
			pixelIndex[2] = coord.z;
			if (imgVal < 0 || imgVal > 1 || isInf(imgVal) || isNaN(imgVal))
			{
				/*
				DebugOut() << "Invalid pixel value at ("
					<<   "x=" << pixelIndex[0]
					<< ", y=" << pixelIndex[1]
					<< ", z=" << pixelIndex[2]
					<< "):  " << imgVal
					<< std::endl;
				*/
				imgVal = 0;
			}
			pImg->SetPixel(pixelIndex,	imgVal);
		}
	}


	iAITKIO::ImagePointer CreateProbabilityImage(
		iAImageCoordConverter const & conv,
		double const spacing[3])
	{
		int size[3];
		size[0] = conv.GetWidth(); size[1] = conv.GetHeight(); size[2] = conv.GetDepth();
		return AllocateImage(size, spacing, itk::ImageIOBase::IOComponentType::DOUBLE);
	}

	iAITKIO::ImagePointer CreateLabelImage(
		iAImageCoordConverter const & conv,
		double const  spacing[3])
	{

		int size[3];
		size[0] = conv.GetWidth(); size[1] = conv.GetHeight(); size[2] = conv.GetDepth();
		return AllocateImage(size, spacing, itk::ImageIOBase::IOComponentType::INT);
	}
	
	iAITKIO::ImagePointer CreateLabelImage(
		iAImageCoordConverter const & conv,
		double const spacing[3],
		QVector<iAITKIO::ImagePointer> const & probabilityImages,
		int labelCount)
	{
		// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)
		iAITKIO::ImagePointer labelImgP = CreateLabelImage(conv, spacing);
		LabelImageType* labelImg = dynamic_cast<LabelImageType*>(labelImgP.GetPointer());
		QVector<ProbabilityImageType*> probImgs;
		for (int i = 0; i < labelCount; ++i)
		{
			probImgs.push_back(dynamic_cast<ProbabilityImageType*>(probabilityImages[i].GetPointer()));
		}
		for (int x=0; x<conv.GetWidth(); ++x)
		{
			for (int y=0; y<conv.GetHeight(); ++y)
			{
				for (int z=0; z<conv.GetDepth(); ++z)
				{
					double maxProb =0;
					int maxProbLabel = -1;
					ProbabilityImageType::IndexType idx;
					idx[0] = x;
					idx[1] = y;
					idx[2] = z;
					for (int l=0; l<labelCount; ++l)
					{
						double prob = probImgs[l]->GetPixel(idx);
						if (prob >= maxProb)
						{
							maxProb = prob;
							maxProbLabel = l;
						}
					}
					labelImg->SetPixel(idx, maxProbLabel );
				}
			}
		}
		return labelImgP;
	}
}

iARandomWalker::iARandomWalker(
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		double const spacing[3],
		QSharedPointer<QVector<iARWInputChannel> > inputChannels,
		SeedVector const & seeds
):
	m_imageGraph(new iAImageGraph(width, height, depth, iAImageCoordinate::ColRowDepMajor)),
	m_inputChannels(inputChannels),
	m_vertexCount(width*height*depth),
	m_minLabel(std::numeric_limits<iALabelType>::max()),
	m_maxLabel(std::numeric_limits<iALabelType>::lowest()),
	m_seeds(seeds)
{
	for(int i=0; i<3; ++i) m_spacing[i] = spacing[i];
	assert((*m_inputChannels)[0].image->size() == width*height*depth);
	for (int i=0; i<seeds.size(); ++i)
	{
		int label = seeds[i].second;
		if (label < m_minLabel)
		{
			m_minLabel = label;
		}
		if (label > m_maxLabel)
		{
			m_maxLabel = label;
		}
	}
}

void iARandomWalker::run()
{
	if (m_inputChannels->size() == 0)
	{
		QMessageBox::warning(0, "Random Walker", "Input Channels must not be empty!");
		return;
	}
	if (m_seeds.size() == 0)
	{
		QMessageBox::warning(0, "Random Walker", "Seeds must not be empty!");
		return;
	}

	if (m_minLabel != 0)
	{
		QMessageBox::warning(0, "Random Walker", "Labels must start at 0");
		return;
	}

	// iATimeGuard perf("RW Start");

	IndexMap seedMap;
	QSet<int> labelSet;
	for (iAVertexIndexType seedIdx=0; seedIdx < m_seeds.size(); ++seedIdx)
	{
		seedMap.insert(m_imageGraph->GetConverter().GetIndexFromCoordinates(m_seeds[seedIdx].first), seedIdx);
		labelSet.insert(m_seeds[seedIdx].second);
	}
	int labelCount = labelSet.size();
	if (m_maxLabel != labelCount-1)
	{
		QMessageBox::warning(0, "Random Walker", "Labels must be consecutive from 0 .. maxLabel !");
		return;
	}
	//perf.time("RW: Labels set up");

	QVector<QSharedPointer<iAGraphWeights> > graphWeights(m_inputChannels->size());
	QVector<double> weightsForChannels(m_inputChannels->size());
	for (int i=0; i<m_inputChannels->size(); ++i)
	{
		QSharedPointer<iAGraphWeights> currentMeasureWeight =
			CalculateGraphWeights(*m_imageGraph, *(*m_inputChannels)[i].image, *(*m_inputChannels)[i].distanceFunc);
		graphWeights[i] = currentMeasureWeight;
		weightsForChannels[i] = (*m_inputChannels)[i].weight;
	}
	//perf.time("RW: weights calculated");

	for (int i=0; i<m_inputChannels->size(); ++i)
	{
		graphWeights[i]->Normalize((*m_inputChannels)[i].normalizeFunc);
	}
	// perf.time("RW: weights normalized");

	QSharedPointer<const iAGraphWeights > finalWeight =
		CombineGraphWeights(graphWeights, weightsForChannels);
	// perf.time("RW: weights combined");

	QVector<double> vertexWeightSum(m_vertexCount);
	for (iAEdgeIndexType edgeIdx = 0; edgeIdx < m_imageGraph->GetEdgeCount(); ++edgeIdx)
	{
		iAEdgeType const & edge = m_imageGraph->GetEdge(edgeIdx);
		vertexWeightSum[edge.first]  += finalWeight->GetWeight(edgeIdx);
		vertexWeightSum[edge.second] += finalWeight->GetWeight(edgeIdx);
	}
	// perf.time("RW: vertex weight sum");

	IndexMap unlabeledMap;
	for(iAVertexIndexType vertexIdx=0, newIdx=0;
		vertexIdx < m_vertexCount; ++vertexIdx)
	{
		if(!seedMap.contains(vertexIdx)) {
			unlabeledMap.insert(vertexIdx, newIdx);
			++newIdx;
		}
	}
	int seedCount  = seedMap.size();
	
	MatrixType A(m_vertexCount-seedCount,  m_vertexCount-seedCount);
	CreateLaplacianPart(A, unlabeledMap, unlabeledMap, m_imageGraph, finalWeight, vertexWeightSum, m_vertexCount);
#ifdef USE_EIGEN
	A.makeCompressed();
#endif
	// perf.time("RW: Laplacian generated");
	
	MatrixType BT(m_vertexCount-seedCount, seedCount);
	CreateLaplacianPart(BT, unlabeledMap, seedMap, m_imageGraph, finalWeight, vertexWeightSum, m_vertexCount, true);
	BT = -BT;
#ifdef USE_EIGEN
	BT.makeCompressed();
#endif
	// perf.time("RW: Other Laplacian generated");

	// Conjugate Gradient: very fast and small memory usage!
	// Eigen::ConjugateGradient<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;

#ifdef USE_EIGEN
	Eigen::SparseLU<Eigen::SparseMatrix<double, Eigen::ColMajor>, Eigen::COLAMDOrdering<int> > solver;
	solver.analyzePattern(A);
	std::string error = solver.lastErrorMessage();
	if (error != "")
	{
		DEBUG_LOG(QString(error.c_str()));
		return;
	}
	solver.factorize(A);
	error = solver.lastErrorMessage();
	if (error != "")
	{
		DEBUG_LOG(QString(error.c_str()));
		return;
	}
#else
	vnl_sparse_lu linear_solver(A, vnl_sparse_lu::quiet);
#endif

	// BiCGSTAB: uses a bit more memory, but is a bit faster!
	// Eigen::BiCGSTAB<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;
	// solver.compute(A);
	// perf.time("RW: Solver compute executed");

	m_result = QSharedPointer<iARWResult>(new iARWResult);
	for (int i=0; i<labelCount; ++i)
	{
		VectorType boundary(seedCount);
		for (iAVertexIndexType seedIdx = 0; seedIdx < m_seeds.size(); ++seedIdx)
		{
			boundary[seedIdx] = m_seeds[seedIdx].second == i;
		}
		VectorType b(m_vertexCount-seedCount);
#ifdef USE_EIGEN
		b = BT * boundary;
#else
		BT.mult(boundary, b);
#endif
		VectorType x(m_vertexCount-seedCount);
#ifdef USE_EIGEN

		x = solver.solve(b);
#else
		linear_solver.solve(b,&x);
#endif
		// perf.time("RW: part solved");

		// put values into probability image
		iAITKIO::ImagePointer pImg = CreateProbabilityImage(m_imageGraph->GetConverter(), m_spacing);
		SetIndexMapValues(pImg, x, unlabeledMap, m_imageGraph->GetConverter());
		SetIndexMapValues(pImg, boundary, seedMap, m_imageGraph->GetConverter());

		m_result->probabilityImages.push_back(pImg);
	}
	m_result->labelledImage = CreateLabelImage(m_imageGraph->GetConverter(), m_spacing, m_result->probabilityImages, labelCount);
}


iAExtendedRandomWalker::iAExtendedRandomWalker(
	iAVoxelIndexType width,
	iAVoxelIndexType height,
	iAVoxelIndexType depth,
	double const spacing[3],
	QSharedPointer<QVector<iARWInputChannel> > inputChannels,
	QSharedPointer<QVector<PriorModelImagePointer> > priorModel,
	double priorModelWeight,
	int maxIterations
):
	m_imageGraph(new iAImageGraph(width, height, depth, iAImageCoordinate::ColRowDepMajor)),
	m_inputChannels(inputChannels),
	m_vertexCount(width*height*depth),
	m_labelCount(0),
	m_priorModel(priorModel),
	m_priorModelWeight(priorModelWeight),
	m_maxIterations(maxIterations)
{
	for(int i=0; i<3; ++i) m_spacing[i] = spacing[i];
	int inputChannelSize = (*m_inputChannels)[0].image->size();
	assert(inputChannelSize == width*height*depth);
	m_labelCount = m_priorModel->size();
	assert(m_labelCount > 0);
	assert(m_priorModelWeight != 0);
	/*
	PriorModelImageType::RegionType reg = (*m_priorModel)[0]->GetLargestPossibleRegion();
	int priorWidth = reg.GetSize()[0];
	int priorHeight = reg.GetSize()[1];
	int priorDepth = reg.GetSize()[2];
	*/
	int * extent = (*m_priorModel)[0]->GetExtent();
	int priorWidth = extent[1] - extent[0] + 1;
	int priorHeight = extent[3] - extent[2] + 1;
	int priorDepth = extent[5] - extent[4] + 1;
	assert(width == priorWidth && height == priorHeight && depth == priorDepth);
}


iAExtendedRandomWalker::iAExtendedRandomWalker(
	iAVoxelIndexType width,
	iAVoxelIndexType height,
	iAVoxelIndexType depth,
	double const spacing[3],
	QSharedPointer<QVector<iARWInputChannel> > inputChannels,
	QSharedPointer<QVector<PriorModelImagePointer> > priorModel,
	double priorModelWeight,
	int maxIterations,
	vtkImageData* i,
	vtkPolyData* p,
	iALogger* logger,
	QObject* parent
):
	m_imageGraph(new iAImageGraph(width, height, depth, iAImageCoordinate::ColRowDepMajor)),
	m_inputChannels(inputChannels),
	m_vertexCount(width*height*depth),
	m_labelCount(0),
	m_priorModel(priorModel),
	m_priorModelWeight(priorModelWeight),
	m_maxIterations(maxIterations)
{
	for(int i=0; i<3; ++i) m_spacing[i] = spacing[i];
	assert((*m_inputChannels)[0].image->size() == width*height*depth);
	m_labelCount = m_priorModel->size();
	assert(m_labelCount > 0);
	assert(m_priorModelWeight != 0);
	/*
	PriorModelImageType::RegionType reg = (*m_priorModel)[0]->GetLargestPossibleRegion();
	int priorWidth = reg.GetSize()[0];
	int priorHeight = reg.GetSize()[1];
	int priorDepth = reg.GetSize()[2];
	*/
	int * extent = (*m_priorModel)[0]->GetExtent();
	int priorWidth = extent[1] - extent[0] + 1;
	int priorHeight = extent[3] - extent[2] + 1;
	int priorDepth = extent[5] - extent[4] + 1;
	assert(width == priorWidth && height == priorHeight && depth == priorDepth);
}

const double EPSILON = 1e-6;
		
void iAExtendedRandomWalker::run()
{
	if (m_inputChannels->size() == 0)
	{
		m_errorMsg = "Input Channels must not be empty!";
		return;
	}
	// iATimeGuard perf("ERW Start");

	try {
		QVector<QSharedPointer<iAGraphWeights> > graphWeights(m_inputChannels->size());
		QVector<double> weightsForChannels(m_inputChannels->size());
		for (int i=0; i<m_inputChannels->size(); ++i)
		{
			QSharedPointer<iAGraphWeights> currentMeasureWeight =
				CalculateGraphWeights(*m_imageGraph, *(*m_inputChannels)[i].image, *(*m_inputChannels)[i].distanceFunc);
			graphWeights[i] = currentMeasureWeight;
			weightsForChannels[i] = (*m_inputChannels)[i].weight;
		}
		// perf.time("ERW: weights calculated");

		for (int i=0; i<m_inputChannels->size(); ++i)
		{
			graphWeights[i]->Normalize((*m_inputChannels)[i].normalizeFunc);
		}
		// perf.time("ERW: weights normalized");

		QSharedPointer<iAGraphWeights const> finalWeight =
			CombineGraphWeights(graphWeights, weightsForChannels);
		// perf.time("ERW: weights combined");

		QVector<double> vertexWeightSum(m_vertexCount);
		for (iAEdgeIndexType edgeIdx = 0; edgeIdx < m_imageGraph->GetEdgeCount(); ++edgeIdx)
		{
			iAEdgeType const & edge = m_imageGraph->GetEdge(edgeIdx);
			vertexWeightSum[edge.first]  += finalWeight->GetWeight(edgeIdx);
			vertexWeightSum[edge.second] += finalWeight->GetWeight(edgeIdx);
		}
		// perf.time("ERW: vertex weight sums");
	
		//bool priorNormalized = true;
		// add priors into vertexWeightSum:
		// if my thinking is correct it should be enough to add the weight factor to each entry,
		// since for one voxel, the probabilities for all labels should add up to 1!
		for (iAVoxelIndexType voxelIdx = 0; voxelIdx < m_vertexCount; ++ voxelIdx)
		{
			double sum = 0;
		
			//PriorModelImageType::IndexType idx;
			iAImageCoordinate coord = m_imageGraph->GetConverter().GetCoordinatesFromIndex(voxelIdx);
			/*
			idx[0] = coord.x;
			idx[1] = coord.y;
			idx[2] = coord.z;
			*/
			for (int labelIdx = 0; labelIdx < m_priorModel->size(); ++labelIdx)
			{
				//sum += (*m_priorModel)[labelIdx]->GetPixel(idx);
				sum += (*m_priorModel)[labelIdx]->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
			}
			assert (std::abs(sum-1.0) < EPSILON);
			if (std::abs(sum-1.0) >= EPSILON)
			{
				//priorNormalized = false;
				//DebugOut() << "Prior Model not normalized at (x="<<coord.x<<", y="<<coord.y<<", z="<<coord.z<<"): "<< sum << std::endl;
			}
			vertexWeightSum[voxelIdx] += (m_priorModelWeight * sum);
		}
		//if (!priorNormalized)
		//{
		//	DebugOut() << "Prior Model not normalized." << std::endl;
		//}
		// perf.time("ERW: priors added to vertex weight");

		IndexMap fullMap;
		for(iAVertexIndexType vertexIdx=0, newIdx=0;
			vertexIdx < m_vertexCount; ++vertexIdx)
		{
			fullMap.insert(vertexIdx, vertexIdx);
		}

		MatrixType A(m_vertexCount,  m_vertexCount);
		CreateLaplacianPart(A, fullMap, fullMap, m_imageGraph, finalWeight, vertexWeightSum, m_vertexCount);
	#ifdef USE_EIGEN
		A.makeCompressed();
	#endif
		// perf.time("ERW: Laplacian generated");

		/*
		Sparse LU (with both eigen and VNL):
		  - very slow
		  - uses lots and lots of memory!
		*/
	
	#ifdef USE_EIGEN
		// Conjugate Gradient: very fast and small memory usage!
		Eigen::ConjugateGradient<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;

		// TODO:
		//    - check if matrix is positive-definite
		//    - find a good maximum number of iterations!
		solver.setMaxIterations(m_maxIterations);

		// BiCGSTAB seems to be a bit faster than Conjugate Gradient, but using more memory
		//Eigen::BiCGSTAB<Eigen::SparseMatrix<double, Eigen::ColMajor> > solver;
		solver.compute(A);
	//#else
		// in case sparse LU should be used, do this:
	//	vnl_sparse_lu linear_solver(A, vnl_sparse_lu::quiet);
	#endif

		// perf.time("ERW: solver compute done");

		m_result = QSharedPointer<iARWResult>(new iARWResult);

		for (int i=0; i<m_labelCount; ++i)
		{
			VectorType priorForLabel(m_vertexCount);
			// fill from image
			for (iAVoxelIndexType voxelIdx = 0; voxelIdx < m_vertexCount; ++ voxelIdx)
			{
				//PriorModelImageType::IndexType idx;
				iAImageCoordinate coord = m_imageGraph->GetConverter().GetCoordinatesFromIndex(voxelIdx);
				/*
				idx[0] = coord.x;
				idx[1] = coord.y;
				idx[2] = coord.z;
				*/
				priorForLabel[voxelIdx] = (*m_priorModel)[i]->GetScalarComponentAsDouble(coord.x, coord.y, coord.z, 0);
			}

			VectorType x(m_vertexCount);
	#ifdef USE_EIGEN
			x = solver.solve(priorForLabel);
	#else
			vnl_sparse_matrix_linear_system<double> problem(A, priorForLabel);
			vnl_lsqr solver(problem);
			int returnCode = solver.minimize(x);
			// in case sparse LU should be used, do this instead:
			//linear_solver.solve(priorForLabel, &x);
	#endif
			// perf.time("ERW: part solved");
			// put values into probability image
			iAITKIO::ImagePointer pImg = CreateProbabilityImage(m_imageGraph->GetConverter(), m_spacing);
			SetIndexMapValues(pImg, x, fullMap, m_imageGraph->GetConverter());
			m_result->probabilityImages.push_back(pImg);
		}
		// create labelled image (as value at k = arg l max(p_l^k) for each pixel k)
		m_result->labelledImage = CreateLabelImage(m_imageGraph->GetConverter(), m_spacing, m_result->probabilityImages, m_labelCount);
	}
	catch (std::bad_alloc&)
	{
		m_errorMsg = "Not enough memory for this dataset!";
	}
}

QSharedPointer<iARWResult> iAExtendedRandomWalker::GetResult()
{
	return m_result;
}

QSharedPointer<iARWResult> iARandomWalker::GetResult()
{
	return m_result;
}


QString const & iAExtendedRandomWalker::GetError() const
{
	return m_errorMsg;
}

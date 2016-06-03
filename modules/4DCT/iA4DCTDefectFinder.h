/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA4DCTDEFECTFINDER_H
#define IA4DCTDEFECTFINDER_H
// iA
#include "iAvec3.h"
#include "iAFiberCharacteristics.h"
#include "iAConsole.h"
//#include "iAFilterLabelImageFilter.h"
#include "itkLabelGeometryImageFilter2.h"
#include "iA4DCTDefects.h"
// Qt
#include <QString>
#include <QStringList>
#include <QList>
// itk
#include <itkImage.h>
#include <itkImageFileReader.h>
//#include <itkLabelGeometryImageFilter.h>
// std
#include <array>
#include <vector>
#include <string>

#define RAD_TO_DEG 57.295779513082320876798154814105

using namespace iA4DCTDefects;

//template<typename TPixel, unsigned int VImageDimension = 3>
template< typename TLabelImage, typename TIntensityImage = TLabelImage >
class iA4DCTDefectFinder
{
private:
	//typedef itk::Image<TPixel, VImageDimension>		TIntensityImage;
	//typedef typename TIntensityImage::Pointer				ImagePointer;
	typedef std::vector<FiberCharacteristics>		FiberInfo;

public:
						iA4DCTDefectFinder();
						~iA4DCTDefectFinder();

	// this functions have to be called before running the algorithm
	void				setLabeledImg(QString path);
	void				setIntensityImg(QString path);
	void				setFiberInfo(QString path);

	// make sure that you set up the labeled image, intensity image and fiber info.
	void				run();

	VectorDataType		getCracks();
	VectorDataType		getBreakages();
	VectorDataType		getPullouts();
	VectorDataType		getDebondings();

private:
	std::vector<FiberCharacteristics>	getNeighboringFibers(iAVec3 pos, double maxDist, FiberInfo fibers);

	typename TLabelImage::Pointer		m_labldImg;
	typename TIntensityImage::Pointer	m_intenImg;
	FiberInfo							m_fiberInfo;

	// parameters
	double				m_scale;
	double				m_pulloutDist;
	double				m_breakageDist;
	double				m_angleTolerance;
	double				m_defFibAngleTolerance;
	double				m_fiberWidthRatio[2];
	double				m_pulloutLengthRatio = 1.5;
	double				m_debondingAngle;
	double				m_debondingLengthRatio;
	double				m_debondingWidthRatio[2];
	double				m_neighboringFiberMaxDistance;
	double				m_bigDefectThreshold;

	VectorDataType		m_cracks;
	VectorDataType		m_breakages;
	VectorDataType		m_pullouts;
	VectorDataType		m_debondings;
};

inline float distBetweenFiberAndPoint(FiberCharacteristics* fiber, iAVec3 point)
{
	iAVec3 startPoint(fiber->startPoint[0], fiber->startPoint[1], fiber->startPoint[2]);
	iAVec3 endPoint(fiber->endPoint[0], fiber->endPoint[1], fiber->endPoint[2]);
	float dist1 = (startPoint - point).length();
	float dist2 = (endPoint - point).length();
	return dist1 < dist2 ? dist1 : dist2;
}

template< typename TLabelImage, typename TIntensityImage >
std::vector<FiberCharacteristics> iA4DCTDefectFinder<TLabelImage, TIntensityImage>::getNeighboringFibers(iAVec3 pos, double maxDist, FiberInfo fibers)
{
	FiberInfo neighbors;
	for (auto f : fibers) {
		double dist = distBetweenFiberAndPoint(&f, pos);
		if (dist < maxDist) {
			neighbors.push_back(f);
		}
	}
	return neighbors;
}

template< typename TLabelImage, typename TIntensityImage >
VectorDataType iA4DCTDefectFinder<TLabelImage, TIntensityImage>::getCracks()
{
	return m_cracks;
}

template< typename TLabelImage, typename TIntensityImage >
VectorDataType iA4DCTDefectFinder<TLabelImage, TIntensityImage>::getBreakages()
{
	return m_breakages;
}

template< typename TLabelImage, typename TIntensityImage >
VectorDataType iA4DCTDefectFinder<TLabelImage, TIntensityImage>::getPullouts()
{
	return m_pullouts;
}

template< typename TLabelImage, typename TIntensityImage >
VectorDataType iA4DCTDefectFinder<TLabelImage, TIntensityImage>::getDebondings()
{
	return m_debondings;
}

template< typename TLabelImage, typename TIntensityImage >
iA4DCTDefectFinder<TLabelImage, TIntensityImage>::iA4DCTDefectFinder()
{
	// default parameters
	m_scale							= 2.;
	m_pulloutDist					= 15;
	m_breakageDist					= 12;
	m_angleTolerance				= 25.;
	m_defFibAngleTolerance			= 20.;
	m_fiberWidthRatio[0]			= 0.5;
	m_fiberWidthRatio[1]			= 2.;
	m_pulloutLengthRatio			= 1.5;
	m_debondingAngle				= 45.;
	m_debondingLengthRatio			= 4.;
	m_debondingWidthRatio[0]		= 0.333;
	m_debondingWidthRatio[1]		= 3.;
	m_neighboringFiberMaxDistance	= 15;
	m_bigDefectThreshold			= 14;
}

template< typename TLabelImage, typename TIntensityImage >
iA4DCTDefectFinder<TLabelImage, TIntensityImage>::~iA4DCTDefectFinder()
{ /* no implementation */ }

template< typename TLabelImage, typename TIntensityImage >
void iA4DCTDefectFinder<TLabelImage, TIntensityImage>::run()
{
	if (m_labldImg.IsNull() || m_intenImg.IsNull()) {
		DEBUG_LOG("Wrong imput parameters\n");
		return;
	}

	// applying the label geometry image filter
	typedef itk::LabelGeometryImageFilter2<TLabelImage, TIntensityImage> LabelGeometryImageFilterType;
	typename LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
	labelGeometryImageFilter->SetInput(m_labldImg);
	labelGeometryImageFilter->SetIntensityInput(m_intenImg);

	// These generate optional outputs.
	labelGeometryImageFilter->CalculatePixelIndicesOn();
	labelGeometryImageFilter->CalculateOrientedBoundingBoxOn();
	labelGeometryImageFilter->CalculateOrientedLabelRegionsOn();

	labelGeometryImageFilter->Update();

	typename LabelGeometryImageFilterType::LabelsType allLabels;
	labelGeometryImageFilter->GetLabels( &allLabels );
	typename LabelGeometryImageFilterType::LabelsType::iterator allLabelsIt;

	DEBUG_LOG("Number of labels: " + std::to_string(labelGeometryImageFilter->GetNumberOfLabels()) + "\n\n");

	for (allLabelsIt = allLabels.begin(); allLabelsIt != allLabels.end(); allLabelsIt++)
	{
		typename LabelGeometryImageFilterType::LabelPixelType labelValue = *allLabelsIt;
		typename LabelGeometryImageFilterType::BoundingBoxVerticesType oobb =
			labelGeometryImageFilter->GetOrientedBoundingBoxVertices(labelValue);

		// axis of oobb
		iAVec3 oobbO(oobb[0][0], oobb[0][1], oobb[0][2]);						// center
		iAVec3 oobbX = iAVec3(oobb[1][0], oobb[1][1], oobb[1][2]) - oobbO;		// x-axis
		iAVec3 oobbY = iAVec3(oobb[2][0], oobb[2][1], oobb[2][2]) - oobbO;		// y-axis
		iAVec3 oobbZ = iAVec3(oobb[4][0], oobb[4][1], oobb[4][2]) - oobbO;		// z-axis

		// lengths of axises of oobb
		iAVec3 oobbSize;
		{
			typename LabelGeometryImageFilterType::LabelPointType orientedSize =
				labelGeometryImageFilter->GetOrientedBoundingBoxSize(labelValue);
			oobbSize[0] = orientedSize[0];
			oobbSize[1] = orientedSize[1];
			oobbSize[2] = orientedSize[2];
		}

		// sorted lengths of axises
		double oobbSizeSorted[3] = { oobbSize[0], oobbSize[1], oobbSize[2] };
		std::sort(std::begin(oobbSizeSorted), std::end(oobbSizeSorted), std::greater<double>());	// sort in descending order

		// the longest axis
		iAVec3 longestAxis[3];
		if (oobbSize[0] > oobbSize[1] && oobbSize[0] > oobbSize[2]) {
			longestAxis[0] = oobbX;
			if (oobbSize[1] > oobbSize[2]) {
				longestAxis[1] = oobbY;
				longestAxis[2] = oobbZ;
			} else {
				longestAxis[1] = oobbZ;
				longestAxis[2] = oobbY;
			}
		} else if (oobbSize[1] > oobbSize[2]) {
			longestAxis[0] = oobbY;
			if (oobbSize[0] > oobbSize[2]) {
				longestAxis[1] = oobbX;
				longestAxis[2] = oobbZ;
			} else {
				longestAxis[1] = oobbZ;
				longestAxis[2] = oobbX;
			}
		} else {
			longestAxis[0] = oobbZ;
			if (oobbSize[0] > oobbSize[1]) {
				longestAxis[1] = oobbX;
				longestAxis[2] = oobbY;
			} else {
				longestAxis[1] = oobbY;
				longestAxis[2] = oobbX;
			}
		}

		// angle between the force direction and a defect
		iAVec3 upVec(0, 0, 1);
		double angleBtwForceAndDefect = iAVec3::angle(longestAxis[0], upVec) * RAD_TO_DEG;
		if (angleBtwForceAndDefect > 90) angleBtwForceAndDefect -= 90;

		// extreme point of defect
		std::array<iAVec3, 2> defectExtremePoints;
		defectExtremePoints[0] = oobbO + longestAxis[1] / 2 + longestAxis[2] / 2;
		defectExtremePoints[1] = defectExtremePoints[0] + longestAxis[0];

		// is defect big
		bool isBigDefect = (defectExtremePoints[1] - defectExtremePoints[0]).length() > m_bigDefectThreshold;

		// weighted center of defect
		iAVec3 weightedCentroid;
		{
			typename LabelGeometryImageFilterType::LabelPointType wCentroid = labelGeometryImageFilter->GetWeightedCentroid(labelValue);
			weightedCentroid[0] = wCentroid[0];
			weightedCentroid[1] = wCentroid[1];
			weightedCentroid[2] = wCentroid[2];
		}

		// neighboring fibers
		FiberInfo neighboringFibers;
		{
			FiberInfo neighbor1, neighbor2;
			neighbor1 = getNeighboringFibers(defectExtremePoints[0], m_neighboringFiberMaxDistance, m_fiberInfo);
			neighbor2 = getNeighboringFibers(defectExtremePoints[1], m_neighboringFiberMaxDistance, m_fiberInfo);
			neighboringFibers.insert(neighboringFibers.end(), neighbor1.begin(), neighbor1.end());
			neighboringFibers.insert(neighboringFibers.end(), neighbor2.begin(), neighbor2.end());
		}

		bool hasBottomFiber = false;
		bool hasUpperFiber = false;
		for (auto f : neighboringFibers) {
			iAVec3 start(f.startPoint[0], f.startPoint[1], f.startPoint[2]);
			iAVec3 end(f.endPoint[0], f.endPoint[1], f.endPoint[2]);
			double distToStart = (weightedCentroid - start).length();
			double distToEnd = (weightedCentroid - end).length();
			iAVec3 dir;
			if (distToStart < distToEnd) {
				dir = end - start;
			} else {
				dir = start - end;
			}
			if (dir[2] > 0) {
				hasUpperFiber = true;
			} else {
				hasBottomFiber = true;
			}
			if (hasUpperFiber && hasBottomFiber)
				break;
		}

		// the closes fiber and distance to it
		FiberCharacteristics closestFiber;
		double distanceToClosestFiber = std::numeric_limits<double>::max();
		for (auto f : neighboringFibers) {
			iAVec3 startPoint(f.startPoint[0], f.startPoint[1], f.startPoint[2]);
			iAVec3 endPoint(f.endPoint[0], f.endPoint[1], f.endPoint[2]);

			double dist;
			double dist1 = (startPoint - weightedCentroid).length();
			double dist2 = (endPoint - weightedCentroid).length();
			dist = dist1 < dist2 ? dist1 : dist2;

			if (distanceToClosestFiber > dist) {
				distanceToClosestFiber = dist;
				closestFiber = f;
			}
		}
		
		// ratios of defect
		double lengthRatio = oobbSizeSorted[0] / oobbSizeSorted[1];
		double widthRatio = oobbSizeSorted[1] / oobbSizeSorted[2];

		// is defect has an extended shape
		bool isExtended = (widthRatio >= m_fiberWidthRatio[0] &&	widthRatio <= m_fiberWidthRatio[1]);

		// minimum angle between any neighboring fiber and defect
		double angleBetweenFiberAndVoid = 360;
		{
			for (auto f : neighboringFibers) {
				iAVec3 fDir(f.endPoint[0] - f.startPoint[0], f.endPoint[1] - f.startPoint[1], f.endPoint[2] - f.startPoint[2]);
				double ang = iAVec3::angle(fDir, longestAxis[0]) * RAD_TO_DEG;
				ang = ang > 90 ? ang - 90 : ang;
				if (angleBetweenFiberAndVoid > ang) {
					angleBetweenFiberAndVoid = ang;
				}
			}
		}

		// is the defect has a neighboring fiber around
		bool hasNeighboringFiber = neighboringFibers.size() > 0;

		// cracks
		if (angleBtwForceAndDefect > m_angleTolerance)
		{
			m_cracks.push_back(labelValue);
		}
		
		// pull-outs
		if (hasNeighboringFiber)
		{
			if (isExtended)
			{
				if (angleBtwForceAndDefect < m_angleTolerance &&
					angleBetweenFiberAndVoid < 10)
				{
					m_pullouts.push_back(labelValue);
				}
			}
			else
			{
				if (!isBigDefect && distanceToClosestFiber < 10)
				{
					m_pullouts.push_back(labelValue);
				}
			}			
		}

		// breakages
		if (distanceToClosestFiber < m_breakageDist &&
			neighboringFibers.size() >= 2 &&
			hasBottomFiber &&
			hasUpperFiber)
		{
			m_breakages.push_back(labelValue);
		}

		// debondings
		if (angleBtwForceAndDefect >= m_debondingAngle &&
			lengthRatio > m_debondingLengthRatio &&
			isExtended)
		{
			m_debondings.push_back(labelValue);
		}
	}

	DEBUG_LOG("Founded:\n");
	DEBUG_LOG("\t" + std::to_string(m_cracks.size()) + " matrix cracks\n");
	DEBUG_LOG("\t" + std::to_string(m_pullouts.size()) + " fiber pull-outs\n");
	DEBUG_LOG("\t" + std::to_string(m_breakages.size()) + " breakages\n");
	DEBUG_LOG("\t" + std::to_string(m_debondings.size()) + " debondings\n");
}

template< typename TLabelImage, typename TIntensityImage >
void iA4DCTDefectFinder<TLabelImage, TIntensityImage>::setLabeledImg(QString path)
{
	// read image
	typedef itk::ImageFileReader<TLabelImage> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();
	
	reader->SetFileName(path.toStdString());
	reader->Update();
	
	m_labldImg = reader->GetOutput();
}

template< typename TLabelImage, typename TIntensityImage >
void iA4DCTDefectFinder<TLabelImage, TIntensityImage>::setIntensityImg(QString path)
{
	// read image
	typedef itk::ImageFileReader<TIntensityImage> ReaderType;
	typename ReaderType::Pointer reader = ReaderType::New();

	reader->SetFileName(path.toStdString());
	reader->Update();

	m_intenImg = reader->GetOutput();
}

template< typename TLabelImage, typename TIntensityImage >
void iA4DCTDefectFinder<TLabelImage, TIntensityImage>::setFiberInfo(QString path)
{	
	// read fiber info from file
	m_fiberInfo = FiberCharacteristics::ReadFromCSV(path.toStdString(), 2);
	DEBUG_LOG("Founded " + std::to_string(m_fiberInfo.size()) + " fibers\n");
}

#endif // IA4DCTDEFECTFINDER_H

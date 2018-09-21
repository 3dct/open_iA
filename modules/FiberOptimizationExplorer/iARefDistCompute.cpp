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
#include "iARefDistCompute.h"

#include "iAFiberCharData.h"

#include "iACsvConfig.h"
#include "iAPerformanceHelper.h"

#include "iAConsole.h"
#include "iAvec3.h"
#include "charts/iASPLOMData.h"

#include <vtkMath.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <array>
#include <random>

typedef iAVec3T<double> Vec3D;

namespace
{
	double l2dist(double const * const pt1, double const * const pt2, int count)
	{
		double sqDistSum = 0;
		for (int i = 0; i < count; ++i)
			sqDistSum += std::pow(pt1[i] - pt2[i], 2);
		return std::sqrt(sqDistSum);
	}

	void setPoints(vtkVariantArray* fiber, QMap<uint, uint> const & mapping, double points[3][3])
	{
		points[0][0] = fiber->GetValue(mapping[iACsvConfig::StartX]).ToDouble();
		points[0][1] = fiber->GetValue(mapping[iACsvConfig::StartY]).ToDouble();
		points[0][2] = fiber->GetValue(mapping[iACsvConfig::StartZ]).ToDouble();
		points[1][0] = fiber->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
		points[1][1] = fiber->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
		points[1][2] = fiber->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
		points[2][0] = fiber->GetValue(mapping[iACsvConfig::EndX]).ToDouble();
		points[2][1] = fiber->GetValue(mapping[iACsvConfig::EndY]).ToDouble();
		points[2][2] = fiber->GetValue(mapping[iACsvConfig::EndZ]).ToDouble();
	}

	// great points about floating point equals: https://stackoverflow.com/a/41405501/671366
	template<typename T1, typename T2>
	static bool isApproxEqual(T1 a, T2 b, T1 tolerance = std::numeric_limits<T1>::epsilon())
	{
		T1 diff = std::fabs(a - b);
		if (diff <= tolerance)
			return true;

		if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
			return true;

		return false;
	}

	Vec3D perpendicularVector(Vec3D const & vectorIn)
	{
		if (!isApproxEqual(vectorIn[0], 0.0) && !isApproxEqual(-vectorIn[0], vectorIn[1]))
			return Vec3D(vectorIn[2], vectorIn[2], -vectorIn[0] - vectorIn[1]);
		else
			return Vec3D(-vectorIn[1] - vectorIn[2], vectorIn[0], vectorIn[0]);
	}

	Vec3D fromSpherical(double phi, double theta, double radius)
	{
		return Vec3D(
			radius * std::sin(phi) * std::cos(theta),
			radius * std::sin(phi) * std::sin(theta),
			radius * std::cos(phi));
	}

	const int CylinderSamplePoints = 200;

	void samplePoints(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, std::vector<Vec3D > & result, size_t numSamples)
	{
		std::default_random_engine generator; // deterministic, will always produce the same "random" numbers; might be exchanged for another generator to check the spread we still get
		std::uniform_real_distribution<double> radiusRnd(0, 1);
		std::uniform_real_distribution<double> posRnd(0, 1);

		Vec3D fiberStart(fiberInfo->GetValue(mapping[iACsvConfig::StartX]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::StartY]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::StartZ]).ToDouble());
		Vec3D fiberEnd(fiberInfo->GetValue(mapping[iACsvConfig::EndX]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::EndY]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::EndZ]).ToDouble());
		Vec3D fiberDir = fiberEnd - fiberStart;
		double fiberRadius = fiberInfo->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2;
		/*
		DEBUG_LOG(QString("Sampling fiber (%1, %2, %3) - (%4, %5, %6), radius = %7")
		.arg(fiberStart[0]).arg(fiberStart[1]).arg(fiberStart[2])
		.arg(fiberEnd[0]).arg(fiberEnd[1]).arg(fiberEnd[2]).arg(fiberRadius));
		*/

		Vec3D perpDir = perpendicularVector(fiberDir).normalized();
		Vec3D perpDir2 = crossProduct(fiberDir, perpDir).normalized();
		std::vector<Vec3D> perpDirs;
		perpDirs.push_back(Vec3D(perpDir));
		perpDirs.push_back(Vec3D(perpDir2));
		perpDirs.push_back(-Vec3D(perpDir));
		perpDirs.push_back(-Vec3D(perpDir2));
		for (size_t a = 0; a < 4; ++a)
		{
			perpDirs.push_back((perpDirs[a] + perpDirs[(a + 1) % 4]).normalized());
		}

		std::uniform_int_distribution<int> angleRnd(0, perpDirs.size() - 1);
		/*
		DEBUG_LOG(QString("Normal Vectors: (%1, %2, %3), (%4, %5, %6)")
		.arg(perpDir[0]).arg(perpDir[1]).arg(perpDir[2])
		.arg(perpDir2[0]).arg(perpDir2[1]).arg(perpDir2[2]));
		*/
		result.resize(numSamples);

		for (int i = 0; i < numSamples; ++i)
		{
			int angleIdx = angleRnd(generator);
			double radius = fiberRadius * std::sqrt(radiusRnd(generator));
			double t = posRnd(generator);
			result[i] = fiberStart + fiberDir * t + perpDirs[angleIdx] * radius;
			//DEBUG_LOG(QString("    Sampled point: (%1, %2, %3)").arg(result[i][0]).arg(result[i][1]).arg(result[i][2]));
		}
	}

	//linePnt - point the line passes through
	//lineDir - unit vector in direction of line, either direction works
	//pnt - the point to find nearest on line for
	Vec3D nearestPointOnLine(Vec3D const & linePoint, Vec3D const & lineDir, Vec3D const & point, double & dist)
	{
		auto normLineDir = lineDir.normalized();
		auto vecToPoint = point - linePoint;
		dist = dotProduct(vecToPoint, normLineDir);
		return linePoint + normLineDir * dist;
	}

	bool pointContainedInFiber(Vec3D const & point, vtkVariantArray* fiber, QMap<uint, uint> const & mapping)
	{
		Vec3D start(fiber->GetValue(mapping[iACsvConfig::StartX]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::StartY]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::StartZ]).ToDouble());
		Vec3D end(fiber->GetValue(mapping[iACsvConfig::EndX]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::EndY]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::EndZ]).ToDouble());
		Vec3D dir = end - start;
		double dist;
		Vec3D ptOnLine = nearestPointOnLine(start, dir, point, dist);
		if (dist > 0 && dist < dir.length())  // check whether point is between start and end
		{
			double radius = fiber->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2.0;
			double distance = (ptOnLine - point).length();
			return distance < radius;
		}
		return false;
	}

	double getOverlap(vtkVariantArray* fiber1, QMap<uint, uint> const & mapping, vtkVariantArray* fiber2)
	{
		// leave out pi in volume, as we only need relation of the volumes!
		double fiber1Vol = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble() + std::pow(fiber1->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2, 2);
		double fiber2Vol = fiber2->GetValue(mapping[iACsvConfig::Length]).ToDouble() + std::pow(fiber2->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2, 2);
		// TODO: also map fiber volume (currently not mapped!
		vtkVariantArray* shorterFiber = (fiber1Vol < fiber2Vol) ? fiber1 : fiber2;
		vtkVariantArray* longerFiber = (fiber1Vol > fiber2Vol) ? fiber1 : fiber2;
		std::vector<Vec3D > sampledPoints;
		samplePoints(shorterFiber, mapping, sampledPoints, CylinderSamplePoints);
		size_t containedPoints = 0;
		for (Vec3D pt : sampledPoints)
		{
			if (pointContainedInFiber(pt, longerFiber, mapping))
				++containedPoints;
		}
		double distance = static_cast<double>(containedPoints) / CylinderSamplePoints;
		distance *= (fiber1Vol < fiber2Vol) ? fiber1Vol / fiber2Vol : fiber2Vol / fiber1Vol;
		return distance;
	}

	// currently: L2 norm (euclidean distance). other measures?
	double getDistance(vtkVariantArray* fiber1, QMap<uint, uint> const & mapping, vtkVariantArray* fiber2,
		int distanceMeasure, double diagonalLength, double maxLength)
	{
		double distance = 0;
		switch (distanceMeasure)
		{
		default:
		case 0: // mid-point, angle, length
		{
			const int Dist1ValueCount = 6;
			double val1[Dist1ValueCount], val2[Dist1ValueCount];
			val1[0] = fiber1->GetValue(mapping[iACsvConfig::Phi]).ToDouble();
			val1[1] = fiber1->GetValue(mapping[iACsvConfig::Theta]).ToDouble();
			val1[2] = fiber1->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
			val1[3] = fiber1->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
			val1[4] = fiber1->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
			val1[5] = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			val2[0] = fiber2->GetValue(mapping[iACsvConfig::Phi]).ToDouble();
			val2[1] = fiber2->GetValue(mapping[iACsvConfig::Theta]).ToDouble();
			val2[2] = fiber2->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
			val2[3] = fiber2->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
			val2[4] = fiber2->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
			val2[5] = fiber2->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			// TODO: opposite direction treatment! -> phi/theta reversed?
			double radius = 1; //< radius doesn't matter here, we're only interested in the direction
			Vec3D dir1 = fromSpherical(val1[0], val1[1], radius);
			Vec3D dir2 = fromSpherical(val2[0], val2[1], radius);
			double fiberAngle = angle(dir1, dir2);
			if (fiberAngle > vtkMath::Pi() / 2) // if angle larger than 90°
			{
				val1[0] += (val1[0] < vtkMath::Pi()) ? vtkMath::Pi() : -vtkMath::Pi();
				val1[1] += (val1[1] < 0) ? vtkMath::Pi() / 2 : -vtkMath::Pi() / 2;
				// just to check if now angles are ok...
				dir1 = fromSpherical(val1[0], val1[1], radius);
				dir2 = fromSpherical(val2[0], val2[1], radius);
				fiberAngle = angle(dir1, dir2);
				if (fiberAngle > vtkMath::Pi() / 2) // still larger than 90° ? Then my calculations are wrong!
				{
					DEBUG_LOG(QString("Wrong angle computation: phi1=%1, theta1=%2, phi2=%3, theta2=%4")
						.arg(val1[0]).arg(val1[1]).arg(val2[0]).arg(val2[1]))
				}
			}

			distance =
				(std::abs(val2[0] - val1[0]) / (vtkMath::Pi() / 2)) +  // phi diff.
				(std::abs(val2[1] - val1[1]) / (vtkMath::Pi() / 4)) +  // theta diff.
				(l2dist(val1 + 2, val2 + 2, 3) / diagonalLength) +  // center diff.
				(std::abs(val2[5] - val1[5]) / maxLength);
			break;
		}
		case 1: // start/end/center
		{
			double points1[3][3];
			double points2[3][3];
			setPoints(fiber1, mapping, points1);
			setPoints(fiber2, mapping, points2);
			double fiber1Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();
			double fiber2Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			double dist1StartTo2Start = l2dist(points1[0], points2[0], 3);
			double dist1StartTo2End = l2dist(points1[0], points2[2], 3);
			double dist1EndTo2Start = l2dist(points1[2], points2[0], 3);
			double dist1EndTo2End = l2dist(points1[2], points2[2], 3);

			distance = l2dist(points1[1], points2[1], 3);
			// switch start and end of second fiber if distance from start of first to end of second is smaller:
			if (dist1StartTo2Start > dist1StartTo2End && dist1EndTo2End > dist1EndTo2Start)
				distance += dist1StartTo2End + dist1EndTo2Start;
			else
				distance += dist1StartTo2Start + dist1EndTo2End;
			distance /= (3 * diagonalLength);

			break;
		}
		case 2: // distances between all 9 pairs of the 3 points of each fiber:
		{

			double points1[3][3];
			double points2[3][3];
			setPoints(fiber1, mapping, points1);
			setPoints(fiber2, mapping, points2);
			double fiber1Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();
			double fiber2Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			for (int i = 0; i<3; ++i)
				for (int j = 0; j<3; ++j)
					distance += l2dist(points1[i], points2[j], 3);
			distance /= fiber1Len;
			break;
		}
		case 3: // overlap between the cylinder volumes, sampled through CylinderSamplePoints from the shorter fiber
		{
			// 1. sample points on the cylinder
			//    -> regular?
			//        - n places along the fiber axis (e.g. split length into 5 equal segments)
			//        - at each place, take m points along the fiber surface (i.e. split 360° by m = x, one point for each segment of angle width x)
			//        - at surface? -> at distance r from current middle point, in direction of angle x
			//    -> random? -> probably simplest, something like https://stackoverflow.com/a/9266704/671366:
			//        - one random variable for point along fiber axis (0..1, where 0=start point, 1=end point)
			//        - one random variable for direction from axis (0..360°)
			//            -> for now: only 4 positions (0, 90, 180, 270)°, which makes it much easier to handle (no rotation around custom axis, just cross product/inversion of direction!
			//        - one random variable for distance from center (0.. fiber radius); make sure to use sqrt of random variable to avoid clustering points in center (http://mathworld.wolfram.com/DiskPointPicking.html)
			//    - pseudorandom?
			//        --> no idea at the moment
			distance = 1 - getOverlap(fiber1, mapping, fiber2);
			break;
		}
		}
		return distance;
	}

	void getBestMatches(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, vtkTable* reference,
		std::vector<std::vector<iAFiberDistance> > & bestMatches, double diagonalLength, double maxLength)
	{
		size_t refFiberCount = reference->GetNumberOfRows();
		bestMatches.resize(iARefDistCompute::DistanceMetricCount);
		for (int d = 0; d<iARefDistCompute::DistanceMetricCount; ++d)
		{
			std::vector<iAFiberDistance> distances;
			if (d < 3)
			{
				distances.resize(refFiberCount);
				for (size_t fiberID = 0; fiberID < refFiberCount; ++fiberID)
				{
					distances[fiberID].index = fiberID;
					double curDistance = getDistance(fiberInfo, mapping, reference->GetRow(fiberID), d, diagonalLength, maxLength);
					distances[fiberID].distance = curDistance;
				}
			}
			else
			{ // compute overlap measures only for the best-matching fibers according to metric 2:
				distances.resize(bestMatches[1].size());
				for (size_t bestMatchID = 0; bestMatchID < bestMatches[1].size(); ++bestMatchID)
				{
					distances[bestMatchID].index = bestMatches[1][bestMatchID].index;
					double curDistance = getDistance(fiberInfo, mapping, reference->GetRow(distances[bestMatchID].index), d, diagonalLength, maxLength);
					distances[bestMatchID].distance = curDistance;
				}
			}
			std::sort(distances.begin(), distances.end());
			std::copy(distances.begin(), distances.begin() + iARefDistCompute::MaxNumberOfCloseFibers, std::back_inserter(bestMatches[d]));
		}
	}
}

int iARefDistCompute::MaxNumberOfCloseFibers = 25;

iARefDistCompute::iARefDistCompute(std::vector<iAFiberCharData> & results, iASPLOMData & splomData, int referenceID):
	m_splomData(splomData),
	m_resultData(results),
	m_referenceID(referenceID)
{}

void iARefDistCompute::run()
{
	iAPerformanceHelper perfRefComp;
	perfRefComp.start("Reference Distance computation");
	// "register" other datasets to reference:
	auto & ref = m_resultData[m_referenceID];
	auto const & mapping = *ref.m_outputMapping.data();
	/*
	std::vector<std::vector<double> > sampledPoints;
	samplePoints(m_resultData[0].m_resultTable->GetRow(0), mapping, sampledPoints);
	m_sampleData = vtkSmartPointer<vtkPolyData>::New();
	auto points = vtkSmartPointer<vtkPoints>::New();
	for (size_t s = 0; s < sampledPoints.size(); ++s)
	{
	double pt[3];
	for (int i = 0; i < 3; ++i)
	pt[i] = sampledPoints[s][i];
	points->InsertNextPoint(pt);
	}
	m_sampleData->SetPoints(points);
	auto vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexFilter->SetInputData(m_sampleData);
	vertexFilter->Update();

	// For color:
	auto polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->DeepCopy(vertexFilter->GetOutput());
	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents(3);
	colors->SetName ("Colors");
	unsigned char blue[3] = {0, 0, 255};
	for (size_t s = 0; s < sampledPoints.size(); ++s)
	#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
	colors->InsertNextTupleValue(blue);
	#else
	colors->InsertNextTypedTuple(blue);
	#endif
	polydata->GetPointData()->SetScalars(colors);

	m_sampleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_sampleActor = vtkSmartPointer<vtkActor>::New();
	m_sampleMapper->SetInputData(polydata);
	m_sampleActor->SetMapper(m_sampleMapper);
	m_sampleMapper->Update();
	m_sampleActor->GetProperty()->SetPointSize(2);
	m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_sampleActor);
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();

	return;
	*/
	double const * cxr = m_splomData.paramRange(mapping[iACsvConfig::CenterX]),
		*cyr = m_splomData.paramRange(mapping[iACsvConfig::CenterY]),
		*czr = m_splomData.paramRange(mapping[iACsvConfig::CenterZ]);
	double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
	double diagLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
	double const * lengthRange = m_splomData.paramRange(mapping[iACsvConfig::Length]);
	double maxLength = lengthRange[1] - lengthRange[0];

	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		m_progress.EmitProgress(static_cast<int>(100.0 * resultID / m_resultData.size()));
		auto & d = m_resultData[resultID];
		if (resultID == m_referenceID)
			continue;
		size_t fiberCount = d.m_resultTable->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// find the best-matching fibers in reference & compute difference:
			getBestMatches(d.m_resultTable->GetRow(fiberID),
				mapping, ref.m_resultTable,
				d.refDiffFiber[fiberID].dist, maxLength, diagLength);
		}
	}
	perfRefComp.stop();
	std::array<size_t, iAFiberCharData::FiberValueCount> diffCols = {
		mapping[iACsvConfig::StartX],  mapping[iACsvConfig::StartY],  mapping[iACsvConfig::StartZ],
		mapping[iACsvConfig::EndX],    mapping[iACsvConfig::EndY],    mapping[iACsvConfig::EndZ],
		mapping[iACsvConfig::CenterX], mapping[iACsvConfig::CenterY], mapping[iACsvConfig::CenterZ],
		mapping[iACsvConfig::Phi], mapping[iACsvConfig::Theta],
		mapping[iACsvConfig::Length],
		mapping[iACsvConfig::Diameter]
	};
	iAPerformanceHelper perfDistComp;
	perfDistComp.start("Distance computation");
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		auto& d = m_resultData[resultID];
		if (resultID == m_referenceID)
			continue;
		size_t fiberCount = d.m_resultTable->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			size_t timeStepCount = d.m_timeValues.size();
			auto & timeSteps = d.refDiffFiber[fiberID].timeStep;
			timeSteps.resize(timeStepCount);
			for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
			{
				// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
				auto & diffs = timeSteps[timeStep].diff;
				diffs.resize(iAFiberCharData::FiberValueCount+DistanceMetricCount);
				for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
				{
					diffs[diffID] = d.m_timeValues[timeStep][fiberID][diffID]
						- m_resultData[m_referenceID].m_resultTable->GetValue(d.refDiffFiber[fiberID].dist[0][0].index, diffCols[diffID]).ToDouble();
				}
				for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
				{
					// double dist = getDistance(d.m_timeValues[timeStep][fiberID], )
					double dist = 0; // TODO compute distance measure from vector instead of vtkTable!
					diffs[iAFiberCharData::FiberValueCount + distID] = dist;
				}
			}
		}
	}
	perfDistComp.stop();
	size_t splomID = 0;
	iAPerformanceHelper perfSPLOMUpdate;
	perfSPLOMUpdate.start("SPLOM Update");
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		iAFiberCharData& d = m_resultData[resultID];
		size_t fiberCount = d.m_resultTable->GetNumberOfRows();
		if (resultID == m_referenceID)
		{
			splomID += fiberCount;
			continue;
		}
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			auto & diffData = d.refDiffFiber[fiberID];
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				size_t tableColumnID = m_splomData.numParams() - (iAFiberCharData::FiberValueCount + DistanceMetricCount + EndColumns) + diffID;
				m_splomData.data()[tableColumnID][splomID] = diffData.timeStep[d.m_timeValues.size()-1].diff[diffID];
				//d.m_resultTable->SetValue(fiberID, tableColumnID, d.m_timeRefDiff[fiberID][d.m_timeValues.size()-1][diffID]);
			}
			for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
			{
				double dist = diffData.dist[distID][0].distance;
				size_t tableColumnID = m_splomData.numParams() - (DistanceMetricCount + EndColumns) + distID;
				m_splomData.data()[tableColumnID][splomID] = dist;
				//d.m_resultTable->SetValue(fiberID, tableColumnID, dist);
			}
			++splomID;
		}
	}
	perfSPLOMUpdate.stop();
}

iAProgress* iARefDistCompute::progress()
{
	return &m_progress;
}

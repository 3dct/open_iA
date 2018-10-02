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

	enum {
		PtStart = 0,
		PtCenter,
		PtEnd
	};

	struct iAFiberData
	{
		double phi, theta, length, diameter;
		Vec3D pts[3];
		iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping)
		{
			pts[PtStart] = Vec3D(
				table->GetValue(fiberID, mapping[iACsvConfig::StartX]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::StartY]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::StartZ]).ToDouble());
			pts[PtCenter] = Vec3D(
				table->GetValue(fiberID, mapping[iACsvConfig::CenterX]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::CenterY]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::CenterZ]).ToDouble());
			pts[PtEnd] = Vec3D(
				table->GetValue(fiberID, mapping[iACsvConfig::EndX]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::EndY]).ToDouble(),
				table->GetValue(fiberID, mapping[iACsvConfig::EndZ]).ToDouble());
			phi = table->GetValue(fiberID, mapping[iACsvConfig::Phi]).ToDouble();
			theta = table->GetValue(fiberID, mapping[iACsvConfig::Theta]).ToDouble();
			length = table->GetValue(fiberID, mapping[iACsvConfig::Length]).ToDouble();
			diameter = table->GetValue(fiberID, mapping[iACsvConfig::Diameter]).ToDouble();
		}
		iAFiberData(std::vector<double> const & data)
		{
			pts[PtStart] = Vec3D(data[0], data[1], data[2]);
			pts[PtEnd] = Vec3D(data[3], data[4], data[5]);
			pts[PtCenter] = Vec3D(data[6], data[7], data[8]);
			phi = data[9];
			theta = data[10];
			length = data[11];
			diameter = data[12];
		}
	};

	const int CylinderSamplePoints = 200;

	void samplePoints(iAFiberData const & fiber, std::vector<Vec3D > & result, size_t numSamples)
	{
		std::default_random_engine generator; // deterministic, will always produce the same "random" numbers; might be exchanged for another generator to check the spread we still get
		std::uniform_real_distribution<double> radiusRnd(0, 1);
		std::uniform_real_distribution<double> posRnd(0, 1);

		Vec3D fiberDir = fiber.pts[PtEnd] - fiber.pts[PtStart];
		double fiberRadius = fiber.diameter / 2;
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
			result[i] = fiber.pts[PtStart] + fiberDir * t + perpDirs[angleIdx] * radius;
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

	bool pointContainedInFiber(Vec3D const & point, iAFiberData const & fiber)
	{
		Vec3D dir = fiber.pts[PtEnd] - fiber.pts[PtStart];
		double dist;
		Vec3D ptOnLine = nearestPointOnLine(fiber.pts[PtStart], dir, point, dist);
		if (dist > 0 && dist < dir.length())  // check whether point is between start and end
		{
			double radius = fiber.diameter / 2.0;
			double distance = (ptOnLine - point).length();
			return distance < radius;
		}
		return false;
	}

	double getOverlap(iAFiberData const & fiber1, iAFiberData const & fiber2)
	{
		// leave out pi in volume, as we only need relation of the volumes!
		double fiber1Vol = fiber1.length + std::pow(fiber1.diameter / 2, 2);
		double fiber2Vol = fiber2.length + std::pow(fiber2.diameter / 2, 2);
		// TODO: also map fiber volume (currently not mapped!
		iAFiberData const & shorterFiber = (fiber1Vol < fiber2Vol) ? fiber1 : fiber2;
		iAFiberData const & longerFiber = (fiber1Vol > fiber2Vol) ? fiber1 : fiber2;
		std::vector<Vec3D > sampledPoints;
		samplePoints(shorterFiber, sampledPoints, CylinderSamplePoints);
		size_t containedPoints = 0;
		for (Vec3D pt : sampledPoints)
		{
			if (pointContainedInFiber(pt, longerFiber))
				++containedPoints;
		}
		double distance = static_cast<double>(containedPoints) / CylinderSamplePoints;
		distance *= (fiber1Vol < fiber2Vol) ? fiber1Vol / fiber2Vol : fiber2Vol / fiber1Vol;
		return distance;
	}

	// currently: L2 norm (euclidean distance). other measures?
	double getDistance(iAFiberData const & fiber1, iAFiberData const & fiber2,
		int distanceMeasure, double diagonalLength, double maxLength)
	{
		double distance = 0;
		switch (distanceMeasure)
		{
		default:
		case 0: // mid-point, angle, length
		{
			distance =
				(std::abs(fiber2.phi - fiber1.phi) / (vtkMath::Pi() / 2)) +  // phi diff.
				(std::abs(fiber2.theta - fiber1.theta) / (vtkMath::Pi() / 4)) +  // theta diff.
				((fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length() / diagonalLength) +  // center diff.
				(std::abs(fiber2.length - fiber1.length) / maxLength);
			break;
		}
		case 1: // start/end/center
		{
			double dist1StartTo2Start = (fiber2.pts[PtStart] - fiber1.pts[PtStart]).length();
			double dist1StartTo2End   = (fiber2.pts[PtEnd]   - fiber1.pts[PtStart]).length();
			double dist1EndTo2Start   = (fiber2.pts[PtStart] - fiber1.pts[PtEnd])  .length();
			double dist1EndTo2End     = (fiber2.pts[PtEnd]   - fiber1.pts[PtEnd])  .length();

			distance = (fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length();
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
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					distance += (fiber2.pts[j] - fiber1.pts[i]).length();
			distance /= fiber1.length;
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
			distance = 1 - getOverlap(fiber1, fiber2);
			break;
		}
		}
		return distance;
	}

	void getBestMatches(iAFiberData const & fiber, QMap<uint, uint> const & mapping, vtkTable* reference,
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
					iAFiberData refFiber(reference, fiberID, mapping);
					distances[fiberID].index = fiberID;
					double curDistance = getDistance(fiber, refFiber, d, diagonalLength, maxLength);
					distances[fiberID].distance = curDistance;
				}
			}
			else
			{ // compute overlap measures only for the best-matching fibers according to metric 2:
				distances.resize(bestMatches[1].size());
				for (size_t bestMatchID = 0; bestMatchID < bestMatches[1].size(); ++bestMatchID)
				{

					iAFiberData refFiber(reference, distances[bestMatchID].index, mapping);
					distances[bestMatchID].index = bestMatches[1][bestMatchID].index;
					double curDistance = getDistance(fiber, refFiber, d, diagonalLength, maxLength);
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
	auto const & mapping = *ref.mapping.data();
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
		size_t fiberCount = d.table->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// find the best-matching fibers in reference & compute difference:
			iAFiberData fiber(ref.table, fiberID, mapping);
			getBestMatches(fiber, mapping, ref.table,
				d.refDiffFiber[fiberID].dist, diagLength, maxLength);
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
		size_t fiberCount = d.table->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			size_t timeStepCount = d.timeValues.size();
			auto & timeSteps = d.refDiffFiber[fiberID].timeStep;
			timeSteps.resize(timeStepCount);
			for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
			{
				// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
				auto & diffs = timeSteps[timeStep].diff;
				diffs.resize(iAFiberCharData::FiberValueCount+DistanceMetricCount);
				for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
				{
					diffs[diffID] = d.timeValues[timeStep][fiberID][diffID]
						- m_resultData[m_referenceID].table->GetValue(d.refDiffFiber[fiberID].dist[0][0].index, diffCols[diffID]).ToDouble();
				}
				for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
				{
					iAFiberData refFiber(ref.table, d.refDiffFiber[fiberID].dist[0][0].index, mapping);
					iAFiberData fiber(d.timeValues[timeStep][fiberID]);
					double dist = getDistance(fiber, refFiber, distID, diagLength, maxLength);
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
		if (resultID == m_referenceID)
		{
			splomID += d.fiberCount;
			continue;
		}
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			auto & diffData = d.refDiffFiber[fiberID];
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				size_t tableColumnID = m_splomData.numParams() - (iAFiberCharData::FiberValueCount + DistanceMetricCount + EndColumns) + diffID;
				m_splomData.data()[tableColumnID][splomID] = diffData.timeStep[d.timeValues.size()-1].diff[diffID];
				//d.table->SetValue(fiberID, tableColumnID, d.timeRefDiff[fiberID][d.m_timeValues.size()-1][diffID]);
			}
			for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
			{
				double dist = diffData.dist[distID][0].distance;
				size_t tableColumnID = m_splomData.numParams() - (DistanceMetricCount + EndColumns) + distID;
				m_splomData.data()[tableColumnID][splomID] = dist;
				//d.table->SetValue(fiberID, tableColumnID, dist);
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

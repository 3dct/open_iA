/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFiberData.h"

#include "iACsvConfig.h"

#include "iAConsole.h"

#include <vtkMath.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <random>


iAFiberData::iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping, std::vector<iAVec3f> curvedPts):
	pts(3),
	curvedPoints(curvedPts)
{
	pts[PtStart] = iAVec3f(
		table->GetValue(fiberID, mapping[iACsvConfig::StartX]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::StartY]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::StartZ]).ToDouble());
	pts[PtCenter] = iAVec3f(
		table->GetValue(fiberID, mapping[iACsvConfig::CenterX]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::CenterY]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::CenterZ]).ToDouble());
	pts[PtEnd] = iAVec3f(
		table->GetValue(fiberID, mapping[iACsvConfig::EndX]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::EndY]).ToDouble(),
		table->GetValue(fiberID, mapping[iACsvConfig::EndZ]).ToDouble());
	phi = table->GetValue(fiberID, mapping[iACsvConfig::Phi]).ToDouble();
	theta = table->GetValue(fiberID, mapping[iACsvConfig::Theta]).ToDouble();
	length = table->GetValue(fiberID, mapping[iACsvConfig::Length]).ToDouble();
	diameter = table->GetValue(fiberID, mapping[iACsvConfig::Diameter]).ToDouble();
}

iAFiberData::iAFiberData(std::vector<double> const & data) :
	pts(3)
{
	pts[PtStart] = iAVec3f(data[0], data[1], data[2]);
	pts[PtEnd] = iAVec3f(data[3], data[4], data[5]);
	pts[PtCenter] = iAVec3f(data[6], data[7], data[8]);
	phi = data[9];
	theta = data[10];
	length = data[11];
	diameter = data[12];
}

iAFiberData::iAFiberData() :
	pts(3)
{}

iAFiberData iAFiberData::getOrientationCorrected(iAFiberData const & source, iAFiberData const & other)
{
	double dist1StartTo2Start = (other.pts[PtStart] - source.pts[PtStart]).length();
	double dist1StartTo2End = (other.pts[PtEnd] - source.pts[PtStart]).length();
	double dist1EndTo2Start = (other.pts[PtStart] - source.pts[PtEnd]).length();
	double dist1EndTo2End = (other.pts[PtEnd] - source.pts[PtEnd]).length();
	// switch start and end of second fiber if distance from start of first to end of second is smaller:
	if (dist1StartTo2Start > dist1StartTo2End && dist1EndTo2End > dist1EndTo2Start)
	{
		iAFiberData result;
		result.pts[PtStart] = source.pts[PtEnd];
		result.pts[PtCenter] = source.pts[PtCenter];
		result.pts[PtEnd] = source.pts[PtStart];
		result.diameter = source.diameter;
		result.length = source.length;
		iAVec3f dir = result.pts[PtStart] - source.pts[PtEnd];
		if (dir.z() < 0)
		{
			dir = result.pts[PtEnd] - source.pts[PtStart];
		}
		if (dir.x() == 0 && dir.y() == 0)
		{
			result.phi = 0.0;
			result.theta = 0.0;
		}
		else
		{
			result.phi = asin(dir.y() / sqrt(dir.x()*dir.x() + dir.y() * dir.y()));
			result.theta = acos(dir.z() / sqrt(dir.x()*dir.x() + dir.y() * dir.y() + dir.z() * dir.z()));
			result.phi = vtkMath::DegreesFromRadians(result.phi);
			result.theta = vtkMath::DegreesFromRadians(result.theta);
			// locate the phi value to quadrant
			if (dir.x() < 0)
			{
				result.phi = 180.0 - result.phi;
			}
			if (result.phi < 0.0)
			{
				result.phi = result.phi + 360.0;
			}
		}
		//result.phi = source.phi;
		//result.theta = source.theta;
		return result;
	}
	else
	{
		return source;
	}
}



namespace
{

	// great points about floating point equals: https://stackoverflow.com/a/41405501/671366
	template<typename T1, typename T2>
	static bool isApproxEqual(T1 a, T2 b, T1 tolerance = std::numeric_limits<T1>::epsilon())
	{
		T1 diff = std::fabs(a - b);
		if (diff <= tolerance)
		{
			return true;
		}

		if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
		{
			return true;
		}

		return false;
	}

	iAVec3f perpendicularVector(iAVec3f const & vectorIn)
	{
		if (!isApproxEqual(vectorIn[0], 0.0) && !isApproxEqual(-vectorIn[0], vectorIn[1]))
		{
			return iAVec3f(vectorIn[2], vectorIn[2], -vectorIn[0] - vectorIn[1]);
		}
		else
		{
			return iAVec3f(-vectorIn[1] - vectorIn[2], vectorIn[0], vectorIn[0]);
		}
	}
	/*
	iAVec3f fromSpherical(double phi, double theta, double radius)
	{
		return iAVec3f(
			radius * std::sin(vtkMath::RadiansFromDegrees(phi)) * std::cos(vtkMath::RadiansFromDegrees(theta)),
			radius * std::sin(vtkMath::RadiansFromDegrees(phi)) * std::sin(vtkMath::RadiansFromDegrees(theta)),
			radius * std::cos(vtkMath::RadiansFromDegrees(phi)));
	}
	*/

	//linePnt - point the line passes through
	//lineDir - unit vector in direction of line, either direction works
	//pnt - the point to find nearest on line for
	iAVec3f nearestPointOnLine(iAVec3f const & linePoint, iAVec3f const & lineDir, iAVec3f const & point, double & dist)
	{
		auto normLineDir = lineDir.normalized();
		auto vecToPoint = point - linePoint;
		dist = dotProduct(vecToPoint, normLineDir);
		return linePoint + normLineDir * dist;
	}

	bool pointContainedInLineSegment(iAVec3f const & start, iAVec3f const & dir, double radius, iAVec3f const & point)
	{
		double dist;
		iAVec3f ptOnLine = nearestPointOnLine(start, dir, point, dist);
		if (dist > 0 && dist < dir.length())  // check whether point is between start and end
		{
			double distance = (ptOnLine - point).length();
			return distance < radius;
		}
		return false;
	}

	bool pointContainedInFiber(iAVec3f const & point, iAFiberData const & fiber)
	{
		if (fiber.curvedPoints.empty())
		{
			iAVec3f dir = fiber.pts[PtEnd] - fiber.pts[PtStart];
			return pointContainedInLineSegment(fiber.pts[PtStart], dir, fiber.diameter / 2.0, point);
		}
		else
		{
			for (size_t i=0; i<fiber.curvedPoints.size()-1; ++i)
			{
				iAVec3f dir = (fiber.curvedPoints[i+1] - fiber.curvedPoints[i]);
				iAVec3f start(fiber.curvedPoints[i]);
				if (pointContainedInLineSegment(start, dir, fiber.diameter / 2.0, point))
				{
					return true;
				}
			}
			return false;
		}
	}

	//! determine the distance from the given point to the closest point on the given line segment
	//! @param
	double distanceToLineSegment(iAVec3f const & point, iAVec3f const & lineStart, iAVec3f const & lineEnd)
	{
		double dist;
		iAVec3f lineDir = lineEnd-lineStart;
		iAVec3f closestPointOnLine = nearestPointOnLine(lineStart, lineDir, point, dist);
		if (dist > 0 && dist < lineDir.length())
		{
			return (point - closestPointOnLine).length();
		}
		else
		{
			return std::min( (point-lineStart).length(), (point-lineEnd).length() );
		}
	}

	double getOverlap(iAFiberData const & fiber1, iAFiberData const & fiber2, bool volRelation, bool shortFiberDet)
	{
		// leave out pi in volume, as we only need relation of the volumes!
		double fiber1Vol = fiber1.length + std::pow(fiber1.diameter / 2, 2);
		double fiber2Vol = fiber2.length + std::pow(fiber2.diameter / 2, 2);
		// TODO: also map pre-computed fiber volume (currently not mapped)!
		iAFiberData const & shorterFiber = (!shortFiberDet || fiber1Vol < fiber2Vol) ? fiber1 : fiber2;
		iAFiberData const & longerFiber  = (!shortFiberDet || fiber1Vol < fiber2Vol) ? fiber2 : fiber1;
		std::vector<iAVec3f> sampledPoints;
		samplePoints(shorterFiber, sampledPoints, DefaultSamplePoints);
		size_t containedPoints = 0;
		for (iAVec3f pt : sampledPoints)
		{
			if (pointContainedInFiber(pt, longerFiber))
			{
				++containedPoints;
			}
		}
		double similarity = static_cast<double>(containedPoints) / sampledPoints.size();
		if (volRelation)
		{
			similarity *= (fiber1Vol < fiber2Vol) ? fiber1Vol / fiber2Vol : fiber2Vol / fiber1Vol;
		}
		return similarity;
	}

	//! computes the Euclidean distance between two vectors in R^cnt
	double dist(double* vec1, double* vec2, size_t cnt)
	{
		double sqdiffsum = 0;
		for (size_t cur = 0; cur < cnt; ++cur)
		{
			sqdiffsum += std::pow(vec2[cur] - vec1[cur], 2);
		}
		return sqrt(sqdiffsum);
	}

	void sampleSegmentPoints(iAVec3f const & start, iAVec3f const & dir, double radius, std::vector<iAVec3f> & result, size_t numSamples)
	{
		std::random_device r;
		std::default_random_engine generator(r());
		std::uniform_real_distribution<double> radiusRnd(0, 1);
		std::uniform_real_distribution<double> posRnd(0, 1);
		/*
		DEBUG_LOG(QString("Sampling fiber (%1, %2, %3) - (%4, %5, %6), radius = %7")
		.arg(fiberStart[0]).arg(fiberStart[1]).arg(fiberStart[2])
		.arg(fiberEnd[0]).arg(fiberEnd[1]).arg(fiberEnd[2]).arg(fiberRadius));
		*/

		iAVec3f perpDir = perpendicularVector(dir).normalized();
		iAVec3f perpDir2 = crossProduct(dir, perpDir).normalized();
		std::vector<iAVec3f> perpDirs;
		perpDirs.push_back(iAVec3f(perpDir));
		perpDirs.push_back(iAVec3f(perpDir2));
		perpDirs.push_back(-iAVec3f(perpDir));
		perpDirs.push_back(-iAVec3f(perpDir2));
		for (size_t a = 0; a < 4; ++a)
		{
			perpDirs.push_back((perpDirs[a] + perpDirs[(a + 1) % 4]).normalized());
		}

		std::uniform_int_distribution<size_t> angleRnd(0, perpDirs.size() - 1);
		/*
		DEBUG_LOG(QString("Normal Vectors: (%1, %2, %3), (%4, %5, %6)")
		.arg(perpDir[0]).arg(perpDir[1]).arg(perpDir[2])
		.arg(perpDir2[0]).arg(perpDir2[1]).arg(perpDir2[2]));
		*/

		for (size_t i = 0; i < numSamples; ++i)
		{
			size_t angleIdx = angleRnd(generator);
			double newRadius = radius * std::sqrt(radiusRnd(generator));
			double t = posRnd(generator);
			result.push_back(start + dir * t + perpDirs[angleIdx] * newRadius);
			//DEBUG_LOG(QString("    Sampled point: (%1, %2, %3)").arg(result[i][0]).arg(result[i][1]).arg(result[i][2]));
		}
	}

	double getPtToSegDistance(iAFiberData const & f1, iAFiberData const & f2, int measure)
	{
		double sumVal = 0;
		double minVal = std::numeric_limits<double>::max();
		double maxVal = 0;

		std::vector<iAVec3f> const & f1pts = (f1.curvedPoints.empty()) ? f1.pts : f1.curvedPoints;

		for (iAVec3f const & f1pt: f1pts)
		{
			double curDist;
			if (f2.curvedPoints.empty())
			{
				curDist = distanceToLineSegment(f1pt, f2.pts[PtStart], f2.pts[PtEnd]);
			}
			else
			{
				// find segment with minimal distance to this point:
				curDist = std::numeric_limits<double>::max();
				for (size_t j=0; j<f2.curvedPoints.size()-1; ++j)
				{
					double dist = distanceToLineSegment(f1pt, f2.curvedPoints[j], f2.curvedPoints[j+1]);
					if (dist < curDist)
					{
						curDist = dist;
					}
				}
			}
			sumVal += curDist;
			if (curDist < minVal)
			{
				minVal = curDist;
			}
			if (curDist > maxVal)
			{
				maxVal = curDist;
			}
		}
		double avgVal = sumVal / f1.curvedPoints.size();
		// which one to use?
		switch (measure)
		{
		case 0: return minVal;
		case 1: return maxVal;
		case 2: return sumVal;
		case 3: return avgVal;
		default: return 0;
		}
	}
}

void samplePoints(iAFiberData const & fiber, std::vector<iAVec3f> & result, size_t numSamples)
{
	result.reserve(numSamples);

	// TODO: iterate over curved fiber segments
	if (fiber.curvedPoints.empty())
	{
		iAVec3f dir = fiber.pts[PtEnd] - fiber.pts[PtStart];
		sampleSegmentPoints(fiber.pts[PtStart], dir, fiber.diameter / 2.0, result, numSamples );
	}
	else
	{
		// TODO: make sure the implementation delivers exactly numSamples points;
		//       different sampling altogether:
		//           - measure segment lengths
		//           - for each point:
		//               - first determine position along center line (random between 0 and full curved length)
		//               - determine final point from going randomly 0 to radius in arbitrary direction from this position
		double curvedLength = 0;
		for (size_t i=0; i<fiber.curvedPoints.size()-1; ++i)
		{
			curvedLength += (fiber.curvedPoints[i+1] - fiber.curvedPoints[i]).length();
		}
		for (size_t i=0; i<fiber.curvedPoints.size()-1; ++i)
		{
			iAVec3f dir = (fiber.curvedPoints[i+1] - fiber.curvedPoints[i]);
			iAVec3f start(fiber.curvedPoints[i]);
			sampleSegmentPoints(start, dir, fiber.diameter / 2.0, result,
				// spread number of samples according to length ratio, make sure 1 point per segment
				std::max(static_cast<size_t>(1), static_cast<size_t>(numSamples * dir.length() / curvedLength)));
		}
	}
}

double getDissimilarity(iAFiberData const & fiber1raw, iAFiberData const & fiber2,
	int measureID, double diagonalLength, double maxLength)
{
	double dissimilarity = 0;
	switch (measureID)
	{
	default:
	case 0: // Euclidean distance R^6 (midpoint, angle, length)
	{
		iAFiberData fiber1 = iAFiberData::getOrientationCorrected(fiber1raw, fiber2);
		double vec1[6], vec2[6];
		for (int i=0; i<3; ++i)
		{
			vec1[i] = fiber1.pts[PtCenter].data()[i];
			vec2[i] = fiber2.pts[PtCenter].data()[i];
		}
		vec1[3] = fiber1.length;
		vec2[3] = fiber2.length;
		vec1[4] = fiber1.phi;
		vec2[4] = fiber2.phi;
		vec1[5] = fiber1.theta;
		vec2[5] = fiber2.theta;
		dissimilarity = dist(vec1, vec2, 6);
		break;
	}
	case 1: // weighted mid-point, angle, length
	{
		iAFiberData fiber1 = iAFiberData::getOrientationCorrected(fiber1raw, fiber2);
		/*
		iAVec3f dir1 = fromSpherical(fiber1.phi, fiber1.theta, 1);
		iAVec3f dir2 = fromSpherical(fiber2.phi, fiber2.theta, 1);
		double fiberAngle = angle(dir1, dir2);
		if (fiberAngle > vtkMath::Pi() / 2) // still larger than 90° ? Then my calculations are wrong!
		{
			DEBUG_LOG(QString("Wrong angle computation: phi1=%1, theta1=%2, phi2=%3, theta2=%4 => angle=%5")
				.arg(fiber1.phi).arg(fiber1.theta).arg(fiber2.phi).arg(fiber2.theta).arg(fiberAngle));
		}
		*/
		dissimilarity = 0.25 * (
			(std::abs(fiber2.phi - fiber1.phi) / 180) +  // phi diff.
			(std::abs(fiber2.theta - fiber1.theta) / 90) +  // theta diff.
			((fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length() / diagonalLength) +  // center diff.
			(std::abs(fiber2.length - fiber1.length) / maxLength)
		);
		break;
	}
	case 2: // start/end/center distance
	{
		iAFiberData fiber1 = iAFiberData::getOrientationCorrected(fiber1raw, fiber2);
		dissimilarity =
			(fiber2.pts[PtStart] - fiber1.pts[PtStart]).length() +
			(fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length() +
			(fiber2.pts[PtEnd] - fiber1.pts[PtEnd]).length();
		dissimilarity /= (3 * diagonalLength);

		break;
	}
	case 3: // distances between all 9 pairs of the 3 points of each fiber:
	{
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				dissimilarity += (fiber2.pts[j] - fiber1raw.pts[i]).length();
			}
		}
		dissimilarity /= (fiber1raw.length != 0.0) ? fiber1raw.length : 1;
		break;
	}
	case 4: // Fiber fragment distance:
	{
		auto aimaj = (fiber1raw.pts[PtStart] - fiber2.pts[PtStart]);
		auto bimbj = (fiber1raw.pts[PtEnd]   - fiber2.pts[PtEnd]);
		double dist1 = std::sqrt((aimaj*aimaj).sum() + (bimbj*bimbj).sum() + (aimaj*bimbj).sum());
		auto aimbj = (fiber1raw.pts[PtStart] - fiber2.pts[PtEnd]);
		auto bimaj = (fiber1raw.pts[PtEnd]   - fiber2.pts[PtStart]);
		double dist2 = std::sqrt((aimbj*aimbj).sum() + (bimaj*bimaj).sum() + (aimbj*bimaj).sum());
		dissimilarity = std::min(dist1, dist2);
		break;
	}
	case 5: // overlap between the cylinder volumes, sampled through CylinderSamplePoints from the shorter fiber
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
		dissimilarity = 1 - getOverlap(fiber1raw, fiber2, false, true);
		break;
	}
	case 6:
		dissimilarity = 1 - getOverlap(fiber1raw, fiber2, true, true);
		break;
	case 7:
		dissimilarity = 1 - getOverlap(fiber1raw, fiber2, true, false);
		break;

	// one-sided distance:
	case 8:
		dissimilarity = getPtToSegDistance(fiber1raw, fiber2, 0);
		break;
	case 9:
		dissimilarity = getPtToSegDistance(fiber1raw, fiber2, 1);
		break;
	case 10:
		dissimilarity = getPtToSegDistance(fiber1raw, fiber2, 2);
		break;
	case 11:
		dissimilarity = getPtToSegDistance(fiber1raw, fiber2, 3);
		break;

	// measure in both directions and take minimum:
	case 12:
		dissimilarity = std::min(getPtToSegDistance(fiber1raw, fiber2, 0), getPtToSegDistance(fiber2, fiber1raw, 0));
		break;
	case 13:
		dissimilarity = std::min(getPtToSegDistance(fiber1raw, fiber2, 1), getPtToSegDistance(fiber2, fiber1raw, 1));
		break;
	case 14:
		dissimilarity = std::min(getPtToSegDistance(fiber1raw, fiber2, 2), getPtToSegDistance(fiber2, fiber1raw, 2));
		break;
	case 15:
		dissimilarity = std::min(getPtToSegDistance(fiber1raw, fiber2, 3), getPtToSegDistance(fiber2, fiber1raw, 3));
		break;

	// measure in both directions and take maximum:
	case 16:
		dissimilarity = std::max(getPtToSegDistance(fiber1raw, fiber2, 0), getPtToSegDistance(fiber2, fiber1raw, 0));
		break;
	case 17:
		dissimilarity = std::max(getPtToSegDistance(fiber1raw, fiber2, 1), getPtToSegDistance(fiber2, fiber1raw, 1));
		break;
	case 18:
		dissimilarity = std::max(getPtToSegDistance(fiber1raw, fiber2, 2), getPtToSegDistance(fiber2, fiber1raw, 2));
		break;
	case 19:
		dissimilarity = std::max(getPtToSegDistance(fiber1raw, fiber2, 3), getPtToSegDistance(fiber2, fiber1raw, 3));
		break;
	}
	if (std::isinf(dissimilarity) || std::isnan(dissimilarity))
	{
		dissimilarity = 0;
	}
	return dissimilarity;
}

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
#include "iAFiberData.h"

#include "iACsvConfig.h"

#include "iAConsole.h"

#include <vtkMath.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <random>


iAFiberData::iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping)
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
iAFiberData::iAFiberData(std::vector<double> const & data)
{
	pts[PtStart] = Vec3D(data[0], data[1], data[2]);
	pts[PtEnd] = Vec3D(data[3], data[4], data[5]);
	pts[PtCenter] = Vec3D(data[6], data[7], data[8]);
	phi = data[9];
	theta = data[10];
	length = data[11];
	diameter = data[12];
}
iAFiberData::iAFiberData()
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
		Vec3D dir = result.pts[PtStart] - source.pts[PtEnd];
		if (dir.z() < 0)
		dir = result.pts[PtEnd] - source.pts[PtStart];
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
		return source;
}



namespace
{

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
			radius * std::sin(vtkMath::RadiansFromDegrees(phi)) * std::cos(vtkMath::RadiansFromDegrees(theta)),
			radius * std::sin(vtkMath::RadiansFromDegrees(phi)) * std::sin(vtkMath::RadiansFromDegrees(theta)),
			radius * std::cos(vtkMath::RadiansFromDegrees(phi)));
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

	double getOverlap(iAFiberData const & fiber1, iAFiberData const & fiber2, bool volRelation, bool shortFiberDet)
	{
		// leave out pi in volume, as we only need relation of the volumes!
		double fiber1Vol = fiber1.length + std::pow(fiber1.diameter / 2, 2);
		double fiber2Vol = fiber2.length + std::pow(fiber2.diameter / 2, 2);
		// TODO: also map fiber volume (currently not mapped!
		iAFiberData const & shorterFiber = (!shortFiberDet || fiber1Vol < fiber2Vol) ? fiber1 : fiber2;
		iAFiberData const & longerFiber  = (!shortFiberDet || fiber1Vol < fiber2Vol) ? fiber2 : fiber1;
		std::vector<Vec3D > sampledPoints;
		samplePoints(shorterFiber, sampledPoints, DefaultSamplePoints);
		size_t containedPoints = 0;
		for (Vec3D pt : sampledPoints)
		{
			if (pointContainedInFiber(pt, longerFiber))
				++containedPoints;
		}
		double distance = static_cast<double>(containedPoints) / DefaultSamplePoints;
		if (volRelation)
			distance *= (fiber1Vol < fiber2Vol) ? fiber1Vol / fiber2Vol : fiber2Vol / fiber1Vol;
		return distance;
	}
}

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

// currently: L2 norm (euclidean distance). other measures?
double getDistance(iAFiberData const & fiber1raw, iAFiberData const & fiber2,
	int distanceMeasure, double diagonalLength, double maxLength)
{
	double distance = 0;
	switch (distanceMeasure)
	{
	default:
	case 0: // mid-point, angle, length
	{
		iAFiberData fiber1 = iAFiberData::getOrientationCorrected(fiber1raw, fiber2);
		/*
		Vec3D dir1 = fromSpherical(fiber1.phi, fiber1.theta, 1);
		Vec3D dir2 = fromSpherical(fiber2.phi, fiber2.theta, 1);
		double fiberAngle = angle(dir1, dir2);
		if (fiberAngle > vtkMath::Pi() / 2) // still larger than 90° ? Then my calculations are wrong!
		{
			DEBUG_LOG(QString("Wrong angle computation: phi1=%1, theta1=%2, phi2=%3, theta2=%4 => angle=%5")
				.arg(fiber1.phi).arg(fiber1.theta).arg(fiber2.phi).arg(fiber2.theta).arg(fiberAngle));
		}
		*/
		distance =
			(std::abs(fiber2.phi - fiber1.phi) / 180) +  // phi diff.
			(std::abs(fiber2.theta - fiber1.theta) / 90) +  // theta diff.
			((fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length() / diagonalLength) +  // center diff.
			(std::abs(fiber2.length - fiber1.length) / maxLength);
		break;
	}
	case 1: // start/end/center
	{
		iAFiberData fiber1 = iAFiberData::getOrientationCorrected(fiber1raw, fiber2);
		distance =
			(fiber2.pts[PtStart] - fiber1.pts[PtStart]).length() +
			(fiber2.pts[PtCenter] - fiber1.pts[PtCenter]).length() +
			(fiber2.pts[PtEnd] - fiber1.pts[PtEnd]).length();
		distance /= (3 * diagonalLength);

		break;
	}
	case 2: // distances between all 9 pairs of the 3 points of each fiber:
	{
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				distance += (fiber2.pts[j] - fiber1raw.pts[i]).length();
		distance /= (fiber1raw.length != 0.0) ? fiber1raw.length : 1;
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
		distance = 1 - getOverlap(fiber1raw, fiber2, false, true);
		break;
	}
	case 4:
		distance = 1 - getOverlap(fiber1raw, fiber2, true, true);
		break;
	case 5:
		distance = 1 - getOverlap(fiber1raw, fiber2, true, false);
		break;
	}
	if (std::isinf(distance))
		distance = 0;
	return distance;
}

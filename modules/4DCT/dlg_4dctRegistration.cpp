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
 
#include "pch.h"
#include "dlg_4dctRegistration.h"
// iA
#include "iAFiberCharacteristics.h"
// vtk
#include <vtkMath.h>
// std
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#define RAD_TO_DEG 57.295779513082320876798154814105

inline static void CopyVector(const double a[3], double b[3]) {
	for (int i = 0; i < 3; ++i)
		b[i] = a[i];
}

inline static double Angle(const double a[3], const double b[3]) {
	double cross[3];
	vtkMath::Cross(a, b, cross);
	return atan2(vtkMath::Norm(cross), vtkMath::Dot(a, b)) * RAD_TO_DEG;
}

struct FiberDifference
{
	FiberDifference(FiberCharacteristics* f1, FiberCharacteristics* f2, std::vector<CorrespondingPoints> points)
	{
		this->f1 = f1;
		this->f2 = f2;

		if (points.size() < 1)
		{
			Difference = std::numeric_limits<float>::max();
			return;
		}

		// finding the closest point
		CorrespondingPoints closest = points[0];
		float minDist = std::sqrt(vtkMath::Distance2BetweenPoints(f1->startPoint, closest.p1)) + 
			std::sqrt(vtkMath::Distance2BetweenPoints(f1->endPoint, closest.p1));
		for (auto p : points)
		{
			float dist = 
				std::sqrt(vtkMath::Distance2BetweenPoints(f1->startPoint, p.p1)) +
				std::sqrt(vtkMath::Distance2BetweenPoints(f1->endPoint, p.p1));

			if (dist < minDist)
			{
				closest = p;
				minDist = dist;
			}
		}

		// calculate difference function
		double upPoint1[3], downPoint1[3], upPoint2[3], downPoint2[3];
		if (f1->startPoint[1] > f1->endPoint[1]) 
		{
			CopyVector(f1->startPoint, upPoint1);
			CopyVector(f1->endPoint, downPoint1);
		}
		else 
		{
			CopyVector(f1->endPoint, upPoint1);
			CopyVector(f1->startPoint, downPoint1);
		}

		if (f2->startPoint[1] > f2->endPoint[1])
		{
			CopyVector(f2->startPoint, upPoint2);
			CopyVector(f2->endPoint, downPoint2);
		}
		else
		{
			CopyVector(f2->endPoint, upPoint2);
			CopyVector(f2->startPoint, downPoint2);
		}

		double upDist[2];
		upDist[0] = std::sqrt(vtkMath::Distance2BetweenPoints(upPoint1, closest.p1));
		upDist[1] = std::sqrt(vtkMath::Distance2BetweenPoints(upPoint2, closest.p2));

		double downDist[2];
		downDist[0] = std::sqrt(vtkMath::Distance2BetweenPoints(downPoint1, closest.p1));
		downDist[1] = std::sqrt(vtkMath::Distance2BetweenPoints(downPoint2, closest.p2));


		//double cross[3];
		//vtkMath::Cross(upPoint1, upPoint2, cross);
		//double angle1 = atan2(vtkMath::Norm(cross), vtkMath::Dot(upPoint1, upPoint2)) * RAD_TO_DEG;
		//vtkMath::Cross(downPoint1, downPoint2, cross);
		//double angle2 = atan2(vtkMath::Norm(cross), vtkMath::Dot(downPoint1, downPoint2)) * RAD_TO_DEG;


		double dirToUpPoint1[3], dirToUpPoint2[3], dirToDownPoint1[3], dirToDownPoint2[3];
		vtkMath::Subtract(upPoint1,		closest.p1, dirToUpPoint1);
		vtkMath::Subtract(downPoint1,	closest.p1, dirToDownPoint1);
		vtkMath::Subtract(upPoint2,		closest.p2, dirToUpPoint2);
		vtkMath::Subtract(downPoint2,	closest.p2, dirToDownPoint2);

		double angleBetweenDirsToUps	= Angle(dirToUpPoint1,		dirToUpPoint2);
		double angleBetweenDirsToDowns	= Angle(dirToDownPoint1,	dirToDownPoint2);

		Difference =
			3.0  * std::pow(upDist[0] - upDist[1], 2) +
			3.0  * std::pow(downDist[0] - downDist[1], 2) +
			//std::pow(angle1, 2) +
			//std::pow(angle2, 2) +
			0.1  * std::pow(angleBetweenDirsToUps, 2) +
			0.1  * std::pow(angleBetweenDirsToDowns, 2) +
			10.0 * std::pow(f1->diameter - f2->diameter, 2) +
			1.0  * std::pow(f1->curvedLength - f2->curvedLength, 2) +
			10.0 * std::pow(f1->surfaceArea - f2->surfaceArea, 2) +
			10.0 * std::pow(f1->volume - f2->volume, 2) +
			10.0 * std::pow(f1->straightLength - f2->straightLength, 2);
	}
	bool operator < (const FiberDifference& another) const
	{
		return this->Difference < another.Difference;
	}
	FiberCharacteristics* f1, *f2;
	float Difference;
};

dlg_4dctRegistration::dlg_4dctRegistration()
{
	setupUi(this);
	//dlg_4dctRegistration::LAST_OBJ = this;

	connect(selectButton, SIGNAL(clicked()), this, SLOT(onSelectButtonClick()));
	connect(registerButton, SIGNAL(clicked()), this, SLOT(onRegisterButtonClick()));
}

dlg_4dctRegistration::~dlg_4dctRegistration()
{
	//LAST_OBJ = nullptr;
}

void dlg_4dctRegistration::onSelectButtonClick()
{

}

void dlg_4dctRegistration::onRegisterButtonClick()
{
	std::string fiberPath[2];
	fiberPath[0] = "C:\\data\\iAnalyse\\datasets\\roi1\\FCP\\roi1-FCPRev3.csv";
	fiberPath[1] = "C:\\data\\iAnalyse\\datasets\\roi2\\FCP\\roi2-FCPRev3.csv";
	std::vector<FiberCharacteristics> fibers[2];
	fibers[0] = FiberCharacteristics::ReadFromCSV(fiberPath[0], 2);
	fibers[1] = FiberCharacteristics::ReadFromCSV(fiberPath[1], 2);

	// fiber filtering
	const double MIN_LENGTH{ 100 };
	for (int i = 0; i < 2; i++)
	{
		for (auto fiber = begin(fibers[i]); fiber != end(fibers[i]);)
		{
			if (fiber->straightLength < MIN_LENGTH)
				fiber = fibers[i].erase(fiber);
			else
				fiber++;
		}
	}

	// test filling in points
	CorrespondingPoints cp;
	cp.p1[0] = 1089.; cp.p1[1] = 196.; cp.p1[2] = 1965.;
	cp.p2[0] = 1028.; cp.p2[1] = 192.; cp.p2[2] = 2186.;
	m_points.push_back(cp);

	// registration here...
	
	//we need to remove all cropped fibers

	std::size_t numOfCombinations = fibers[0].size() * fibers[1].size();
	unsigned int numOfIterations = 1;
	unsigned long elementsPerIter = ceil((double)numOfCombinations / numOfIterations);

	std::map<unsigned int, unsigned int> correspondingFibers;

	for (unsigned int i = 0; i < numOfIterations; i++)
	{
		std::vector<FiberDifference> diffs;
		for (auto f1 = begin(fibers[0]); f1 != end(fibers[0]); f1++)
		{
			for (auto f2 = begin(fibers[1]); f2 != end(fibers[1]); f2++)
			{
				diffs.push_back(FiberDifference(&*f1, &*f2, m_points));
			}
		}
		std::sort(begin(diffs), end(diffs));
		//std::reverse(begin(diffs), end(diffs));

		for (int j = 0; j < 100; j++)
		{
			std::cout << diffs[j].f1->id << ' ' << diffs[j].f2->id << std::endl;
		}

		for (unsigned int j = 0; j < elementsPerIter; j++)
		{
			if (diffs.empty()) 
				break;

			FiberDifference fd = diffs.back();
			diffs.pop_back();

			/*CorrespondingPoints cp;

			cp.p1[0] = fd.f1->startPoint[0];
			cp.p1[0] = fd.f1->startPoint[1];
			cp.p1[0] = fd.f1->startPoint[2];
			cp.p2[0] = fd.f2->startPoint[0];
			cp.p2[0] = fd.f2->startPoint[1];
			cp.p2[0] = fd.f2->startPoint[2];

			cp.p1[0] = fd.f1->endPoint[0];
			cp.p1[0] = fd.f1->endPoint[1];
			cp.p1[0] = fd.f1->endPoint[2];
			cp.p2[0] = fd.f2->endPoint[0];
			cp.p2[0] = fd.f2->endPoint[1];
			cp.p2[0] = fd.f2->endPoint[2];
			
			m_points.push_back(cp);*/

			correspondingFibers.insert(std::pair<unsigned int, unsigned int>(fd.f1->id, fd.f2->id));
		}
	}
}

void dlg_4dctRegistration::SetPoint(double pos[3])
{
	if (m_currentPoinIsFirst)
	{
		CorrespondingPoints cp;

		cp.p1[0] = pos[0];
		cp.p1[1] = pos[1];
		cp.p1[2] = pos[2];

		m_points.push_back(cp);
	}
	else
	{
		CorrespondingPoints cp = m_points[m_points.size() - 1];
		m_points.pop_back();

		cp.p2[0] = pos[0];
		cp.p2[1] = pos[1];
		cp.p2[2] = pos[2];

		m_points.push_back(cp);
	}

	m_currentPoinIsFirst = !m_currentPoinIsFirst;
}

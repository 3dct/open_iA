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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#ifndef IAREGISTRATIONTESTS_H
#define IAREGISTRATIONTESTS_H
// vtk
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkMetaImageWriter.h>
// itk
#include <itkPointSet.h>
#include <itkEuclideanDistancePointMetric.h>
#include <itkTranslationTransform.h>
#include <itkLevenbergMarquardtOptimizer.h>
#include <itkPointSetToPointSetRegistrationMethod.h>
#include <itkAffineTransform.h>
// std
#include <string>
#include <vector>
// iA
#include "iAEndpointsExtractor.h"

struct FibersDatas {
	int		id;
	float	startPoint[3];
	float	endPoint[3];
	float	straightLength;
	float	curvedLength;
	float	diameter;
	float	surfaceArea;
	float	volume;
	bool	isSeparated;
	bool	isCurved;
};

typedef std::vector<FibersDatas> FiberDataType;

class CommandIterationUpdate : public itk::Command
{
public:
	typedef  CommandIterationUpdate   Self;
	typedef  itk::Command             Superclass;
	typedef itk::SmartPointer<Self>   Pointer;
	itkNewMacro(Self);

protected:
	CommandIterationUpdate() {};

public:

	typedef itk::LevenbergMarquardtOptimizer     OptimizerType;
	typedef const OptimizerType *                OptimizerPointer;

	void Execute(itk::Object *caller, const itk::EventObject & event)
	{
		Execute((const itk::Object *)caller, event);
	}

	void Execute(const itk::Object * object, const itk::EventObject & event)
	{
		OptimizerPointer optimizer =
			dynamic_cast<OptimizerPointer>(object);

		if (!itk::IterationEvent().CheckEvent(&event))
		{
			return;
		}

		std::cout << "Value = " << optimizer->GetCachedValue() << std::endl;
		std::cout << "Position = " << optimizer->GetCachedCurrentPosition();
		std::cout << std::endl << std::endl;

	}

};

typedef itk::TranslationTransform<double, 3>	RegistrationTransformType;

inline bool		IsTheSameFiber(FibersDatas* fib1, FibersDatas* fib2, double* transform, double tolerance);
void			ReadCSV(std::string file, FiberDataType& fibersInfo);
void			FindSpecialFibers(FiberDataType& fibersInfo);
template<class TPointSet>
int				Registration(TPointSet* fixedPointSet, TPointSet* movingPointSet, RegistrationTransformType* transfrom);
int				RegistrationTest(vtkImageData* image, int region[6], std::string outputFilePath, std::vector<Endpoint>& ep1, std::vector<Endpoint>& ep2);

#endif // IAREGISTRATIONTESTS_H
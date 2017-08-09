/************************************  SVMERW  ****************************************
* **  Command line tool for segmenting multi-modal 3D data with a Support Vector   ** *
* **  Machine (SVM) and the Extended Random Walker (ERW)                           ** *
***************************************************************************************
* Copyright (C) 2017 Bernhard Fröhler                                                 *
***************************************************************************************
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
***************************************************************************************
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*       Stelzhamerstraße 23, 4600 Wels / Austria, Email: bernhard.froehler@fh-wels.at *
**************************************************************************************/
#pragma once

#include "iAAlgorithm.h"
#include "iASeedType.h"

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>
#include <QVector>

struct iAImageCoordinate;

class vtkImageData;

/*
typedef double TrainingValueType;
typedef std::pair<int, QVector<TrainingValueType> > iATrainingValue;
typedef QSharedPointer<QVector<iATrainingValue> > iATrainingValuesPointer;
*/

class iASVMImageFilter: public iAAlgorithm
{
	Q_OBJECT
public:
	typedef vtkSmartPointer<vtkImageData>	ImagePointer;
	typedef QSharedPointer<QVector<ImagePointer> > ImagesPointer;

	iASVMImageFilter(vtkImageData* i, iALogger* l);
	void AddInput(ImagePointer input);
	void SetParameters(int kernel, double c, double gamma, int degree, double coef0);
	void SetSeeds(iASeedsPointer seeds);
	// void SetTrainingValues(iATrainingValuesPointer trainingValues);
	void performWork();
	ImagesPointer GetResult();
private:
	//! @{
	//! Input
	double m_c;
	double m_gamma;
	double m_coef0;
	int m_degree;
	int m_kernel;
	iASeedsPointer m_seeds;
	// iATrainingValuesPointer m_trainingValues;
	QVector<ImagePointer> m_input;
	//! @}
	//! @{
	//! Output
	ImagesPointer m_probabilities;
	QString m_error;
	//! @}
};

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
 
#ifndef SVM_IMAGE_FILTER_H
#define SVM_IMAGE_FILTER_H

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>
#include <QVector>

struct iAImageCoordinate;
class iAModalityList;

class vtkImageData;

class SVMImageFilter
{
public:
	typedef QVector<vtkSmartPointer<vtkImageData> >	ProbabilityImagesType;
	typedef QSharedPointer<ProbabilityImagesType> ProbabilityImagesPointer;
	typedef QVector<QList<iAImageCoordinate> > SeedsType;
	typedef QSharedPointer<SeedsType> SeedsPointer;
	typedef QSharedPointer<iAModalityList const> ModalitiesPointer;

	SVMImageFilter(double c, double gamma,
		ModalitiesPointer modalities,
		SeedsPointer seeds,
		int channelCount); // TODO: replace by bool mask to allow selective enabling/disabling of channels
	void Run();
	ProbabilityImagesPointer GetResult();
private:
	//! @{
	//! Input
	double m_c;
	double m_gamma;
	SeedsPointer m_seeds;
	ModalitiesPointer m_modalities;
	int m_channelCount;
	//! @}
	//! @{
	//! Output
	ProbabilityImagesPointer m_probabilities;
	QString m_error;
	//! @}
};

#endif // SVM_IMAGE_FILTER_H
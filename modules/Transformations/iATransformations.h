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
#pragma once

#include "iAFilter.h"

IAFILTER_DEFAULT_CLASS(iARotate);
IAFILTER_DEFAULT_CLASS(iAPermuteAxes);

#include "iAAlgorithm.h"

#include "iAConnector.h"

class iATransformations: public iAAlgorithm
{
	Q_OBJECT
public:
	typedef enum { Unknown = 0, Flip, Translation, PermuteAxes }		TransformationType;
	typedef enum { FlipAxesNone = 0, FlipAxesX = 1, FlipAxesY = 2, FlipAxesZ = 4 }	FlipAxesType;

	iATransformations( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0);

	TransformationType getTransformationType() const;
	void setTransformationType(TransformationType transType);
	FlipAxesType getFlipAxes() const;
	void setFlipAxes(FlipAxesType axes);
	void setFlipAxes(const QChar & axes);
	const qreal * getTranslation() const;
	void setTranslation(qreal tx, qreal ty, qreal tz);
protected:
	virtual void performWork();
private:
	const static int Dim = iAConnector::ImageBaseType::ImageDimension;
	qreal m_rotAngle;
	qreal m_rotCenterCoord[Dim];
	qreal m_translation[Dim];
	TransformationType m_transType;
	FlipAxesType m_flipAxesType;	
};

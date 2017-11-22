
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
#include "pch.h"
#include "iAMapperImpl.h"

#include "iAMathUtility.h"


iAMapper::~iAMapper() {}

bool iAMapper::equals(QSharedPointer<iAMapper> other) const
{
	return false;
}



iALinearMapper::iALinearMapper(double yZoom, double yMin, double yMax, int height)
{
	iALinearMapper::update(yZoom, yMax, yMin, height);
}

double iALinearMapper::SrcToDest(double y) const
{
	return (y - yMin) * yScaleFactor;
}

double iALinearMapper::DestToSrc(double y) const
{
	return y / yScaleFactor + yMin;
}

bool iALinearMapper::equals(QSharedPointer<iAMapper> other) const
{
	iALinearMapper* linearOther = dynamic_cast<iALinearMapper*>(other.data());
	return (linearOther != 0 && yScaleFactor == linearOther->yScaleFactor);
}

QSharedPointer<iAMapper> iALinearMapper::clone()
{
	return QSharedPointer<iAMapper>(new iALinearMapper(*this));
}

void iALinearMapper::update(double yZoom, double yMax, double yMin, int height)
{
	this->yMin = yMin;
	if (yMax)
		yScaleFactor = (double)(height - 1) / (yMax - yMin) *yZoom;
	else
		yScaleFactor = 1;
}

iALinearMapper::iALinearMapper(iALinearMapper const & other) :
	yScaleFactor(other.yScaleFactor),
	yMin(other.yMin)
{}



iALogarithmicMapper::iALogarithmicMapper(double yZoom, double yMax, double yMinValueBiggerThanZero, int height)
{
	iALogarithmicMapper::update(yZoom, yMax, yMinValueBiggerThanZero, height);
}

double iALogarithmicMapper::SrcToDest(double y) const
{
	if (y <= 0)
		return 0;

	double yLog = LogFunc(y);

	yLog = clamp(yMinLog, yMaxLog, yLog);

	return mapValue(
		yMinLog, yMaxLog,
		0.0, static_cast<double>(height * yZoom),
		yLog
	);
}

double iALogarithmicMapper::DestToSrc(double y) const
{
	double yLog = mapValue(
		0.0, static_cast<double>(height * yZoom),
		yMinLog, yMaxLog,
		y
	);
	return std::pow(LogBase, yLog);
}
bool iALogarithmicMapper::equals(QSharedPointer<iAMapper> other) const
{
	iALogarithmicMapper* logOther = dynamic_cast<iALogarithmicMapper*>(other.data());
	return (logOther && yZoom == logOther->yZoom &&
		yMaxLog == logOther->yMaxLog && yMinLog == logOther->yMinLog &&
		height == logOther->height);
}

QSharedPointer<iAMapper> iALogarithmicMapper::clone()
{
	return QSharedPointer<iAMapper>(new iALogarithmicMapper(*this));
}

void iALogarithmicMapper::update(double yZoom, double yMax, double yMinValueBiggerThanZero, int height)
{
	this->yZoom = yZoom;
	yMaxLog = LogFunc(yMax);
	yMinLog = LogFunc(yMinValueBiggerThanZero) - 1;
	this->height = height;
}

iALogarithmicMapper::iALogarithmicMapper(iALogarithmicMapper const & other) :
	yZoom(other.yZoom),
	yMaxLog(other.yMaxLog),
	yMinLog(other.yMinLog),
	height(other.height)
{}

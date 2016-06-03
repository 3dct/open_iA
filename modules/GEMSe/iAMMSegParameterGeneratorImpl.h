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
 
#ifndef IA_MMSEG_PARAMETER_GENERATOR_IMPL_H
#define IA_MMSEG_PARAMETER_GENERATOR_IMPL_H

#include "iAMMSegParameterGenerator.h"

class iARandomParameterGenerator: public iAMMSegParameterGenerator
{
	virtual QString GetName() const;
	virtual ParameterListPointer GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange);
};

class iALatinHypercubeParameterGenerator: public iAMMSegParameterGenerator
{
	virtual QString GetName() const;
	virtual ParameterListPointer GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange);
};

//! as all parameter values are supposed to be equally spaced,
//! and the number of values equally distributed among all parameters,
//! this algorithm will typically give less than the specified amount of samples
class iACartesianGridParameterGenerator : public iAMMSegParameterGenerator
{
	virtual QString GetName() const;
	virtual ParameterListPointer GetParameterSets(QSharedPointer<iAMMSegParameterRange> parameterRange);
};

QVector<QSharedPointer<iAMMSegParameterGenerator> > & GetParameterGenerators();

#endif // IA_MMSEG_PARAMETER_GENERATOR_IMPL_H
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
 
#ifndef IA_MMSEG_PARAMETER_RANGE_H
#define IA_MMSEG_PARAMETER_RANGE_H

#include "iAValueType.h"
#include "iAGEMSeConstants.h" // for AttributeID

#include <QSharedPointer>
#include <QVector>


class iANameMapper;
class iASpectraDistance;

class QTextStream;

struct iAMMSegModalityParamRange
{
	double weightFrom, weightTo;
	int pcaDimMin, pcaDimMax;
	QVector<QSharedPointer<iASpectraDistance> > distanceFuncs;
};


typedef std::vector<std::pair<double, double> > MinMaxPairs;

// TODO: generically for multiple parameters?
class iAMMSegParameterRange
{
public:
	iAMMSegParameterRange();
	static QSharedPointer<iAMMSegParameterRange> Create(QTextStream& in);
	static QString GetDistanceFuncsStr(QVector<QSharedPointer<iASpectraDistance> > const & funcList);
	bool Store(QTextStream& out);
	bool CoversWholeRange(AttributeID id, double min, double max) const;
	bool IsLogScale(AttributeID id) const;
	double GetMin(AttributeID id) const;
	double GetMax(AttributeID id) const;
	iAValueType GetRangeType(AttributeID id) const;
	QString GetName(AttributeID attribID) const;
	QSharedPointer<iANameMapper> GetNameMapper(AttributeID attribID) const;
	int GetAttributeCount() const;
	int GetInputParameterCount() const;
	int GetDerivedOutputCount() const;
	int GetFirstDerivedOutputIndex() const;
	void AdaptDurationMinMax(double curDur);
	void AdaptObjCountMinMax(int curCnt);

	double erw_beta_From, erw_beta_To;				bool erw_beta_logScale;
	double erw_gamma_From, erw_gamma_To;			bool erw_gamma_logScale;
	int erw_maxIter_From, erw_maxIter_To;			bool erw_maxIter_logScale;
	
	bool weightLogScale;

	double svm_C_From, svm_C_To;                bool svm_C_logScale;
	double svm_gamma_From, svm_gamma_To;		bool svm_gamma_logScale;
	int svm_channels_From, svm_channels_To;

	MinMaxPairs measureMinMax;

	QVector<iAMMSegModalityParamRange>	modalityParamRange;
	/* TBD:
		- (Normalization?)
		- (Prior Model)
	*/
	int objCountMin, objCountMax;
	double durationMin, durationMax;
	// TODO: move elsewhere?
	long sampleCount;
};

#endif // !IA_MMSEG_PARAMETER_RANGE_H

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
 
#ifndef IA_MMSEG_PARAMETER_H
#define IA_MMSEG_PARAMETER_H

#include <QSharedPointer>
#include <QVector>
#include "SVMImageFilter.h"

class iASpectraDistance;
class iAMMSegParameterRange;

class iAMMSegModalityParameter
{
public:
	double weight;
	int distanceFuncIdx;
	int pcaDim;
};

class iAMMSegParameter
{
public:
	iAMMSegParameter(QSharedPointer<iAMMSegParameterRange> range);
	iAMMSegParameter(double erw_beta, double erw_gamma, int erw_maxIter,
		QVector<iAMMSegModalityParameter> modalityParams,
		double svm_c, double svm_gamma, int svm_channels,
		QSharedPointer<iAMMSegParameterRange> range);

	static QSharedPointer<iAMMSegParameter> Create(QString const & Descriptor,
		QSharedPointer<iAMMSegParameterRange> paramRange);
	QString GetDescriptor() const;

	void setParam(int paramIdx, double value);

	void setID(int id);
	int id() const;
	double erw_beta() const;
	double erw_gamma() const;
	int erw_maxIter() const;
	double svm_c() const;
	double svm_gamma() const;
	int svm_channels() const;
	int pcaDim(int modalityIdx) const;
	int distanceFuncIdx(int modalityIdx) const;
	QSharedPointer<iASpectraDistance> distanceFunc(int modalityIdx) const;
	double modalityWeight(int modalityIdx) const;
	int modalityCount();
private:
	int m_id;
	// for ERW:
	double m_erw_gamma;
	double m_erw_beta;
	int m_erw_maxIter;
	// for SVM:
	double m_svm_c;
	double m_svm_gamma;
	double m_svm_channels;
	// for each channel:
	QVector<iAMMSegModalityParameter> m_modalityParams;

	QSharedPointer<iAMMSegParameterRange> m_parameterRanges;
};

#endif // IA_MMSEG_PARAMETER_H
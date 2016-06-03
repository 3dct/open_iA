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
#include "iAMMSegParameter.h"

#include "iASpectraDistanceImpl.h"
#include "iAMMSegParameterRange.h"
#include "iAConsole.h"

#include <QStringList>


iAMMSegParameter::iAMMSegParameter(QSharedPointer<iAMMSegParameterRange> range) :
	m_id(-1),
	m_erw_beta(0),
	m_erw_gamma(0),
	m_erw_maxIter(0),
	m_parameterRanges(range),
	m_svm_c(0),
	m_svm_gamma(0),
	m_svm_channels(0)
{
}

iAMMSegParameter::iAMMSegParameter(double erw_beta, double erw_gamma, int erw_maxIter,
	QVector<iAMMSegModalityParameter> modalityParams,
	double svm_c, double svm_gamma, int svm_channels,
	QSharedPointer<iAMMSegParameterRange> range)
:
	m_id(-1),
	m_erw_beta(erw_beta),
	m_erw_gamma(erw_gamma),
	m_erw_maxIter(erw_maxIter),
	m_modalityParams(modalityParams),
	m_parameterRanges(range),
	m_svm_c(svm_c),
	m_svm_gamma(svm_gamma),
	m_svm_channels(svm_channels)
{}


QString iAMMSegParameter::GetDescriptor() const
{
	const QString SEPARATOR = " ";
	QString result = QString::number(id())+SEPARATOR+
		QString::number(erw_beta())+SEPARATOR+
		QString::number(erw_gamma())+SEPARATOR+
		QString::number(erw_maxIter())+SEPARATOR+
		QString::number(svm_c())+SEPARATOR+
		QString::number(svm_gamma())+SEPARATOR+
		QString::number(svm_channels());
	for (int i=0; i<m_modalityParams.size(); ++i)
	{
		result += SEPARATOR + 
			QString::number(m_modalityParams[i].pcaDim)+SEPARATOR+
			QString::number(m_modalityParams[i].weight)+SEPARATOR+
			distanceFunc(i)->GetShortName();
	}
	return result;
}

int getShortNameIdx(QVector<QSharedPointer<iASpectraDistance> > const & vec, QString const & shortName)
{
	for (int i=0; i<vec.size(); ++i)
	{
		if (vec[i]->GetShortName() == shortName)
			return i;
	}
	return -1;
}

QSharedPointer<iAMMSegParameter> iAMMSegParameter::Create(QString const & descriptor, QSharedPointer<iAMMSegParameterRange> paramRange)
{
	if (!paramRange) return QSharedPointer<iAMMSegParameter>();
	QStringList tokens = descriptor.split(" ");
	bool ok;
	int cur = 0;
	int id = tokens[cur++].toInt(&ok);                   if (!ok) { DEBUG_LOG("Invalid ID!\n"); return QSharedPointer<iAMMSegParameter>();  }
	double erw_beta = tokens[cur++].toDouble(&ok);       if (!ok) { DEBUG_LOG("Invalid ERW beta!\n");  return QSharedPointer<iAMMSegParameter>(); }
	double erw_gamma = tokens[cur++].toDouble(&ok);      if (!ok) { DEBUG_LOG("Invalid ERW gamma!\n");  return QSharedPointer<iAMMSegParameter>(); }
	double erw_maxIter = tokens[cur++].toInt(&ok);       if (!ok) { DEBUG_LOG("Invalid ERW maximum iterations!\n");  return QSharedPointer<iAMMSegParameter>(); }
	double svm_C = tokens[cur++].toDouble(&ok);          if (!ok) { DEBUG_LOG("Invalid SVM C!\n");  return QSharedPointer<iAMMSegParameter>(); }
	double svm_gamma = tokens[cur++].toDouble(&ok);      if (!ok) { DEBUG_LOG("Invalid SVM gamma!\n");  return QSharedPointer<iAMMSegParameter>(); }
	double svm_channels = tokens[cur++].toInt(&ok);      if (!ok) { DEBUG_LOG("Invalid SVM channels!\n");  return QSharedPointer<iAMMSegParameter>(); }

	QVector<iAMMSegModalityParameter> modParams;
	for (int i=0; i<paramRange->modalityParamRange.size(); ++i)
	{
		iAMMSegModalityParameter modP;
		modP.pcaDim = tokens[cur++].toInt(&ok);     if (!ok) { DEBUG_LOG("Invalid PCA dim.!\n");  return QSharedPointer<iAMMSegParameter>(); }
		modP.weight = tokens[cur++].toDouble(&ok);  if (!ok) { DEBUG_LOG("Invalid weight!\n");  return QSharedPointer<iAMMSegParameter>(); }
		modP.distanceFuncIdx = getShortNameIdx(paramRange->modalityParamRange[i].distanceFuncs, tokens[cur++]);
		if (modP.distanceFuncIdx == -1)
		{
			DEBUG_LOG(QString("Invalid distance function string: %1\n").arg(tokens[cur-1]));
			return QSharedPointer<iAMMSegParameter>();
		}
		modParams.push_back(modP);
	}
	
	QSharedPointer<iAMMSegParameter> result(new iAMMSegParameter(
		erw_beta, erw_gamma, erw_maxIter,
		modParams,
		svm_C, svm_gamma, svm_channels,
		paramRange));
	result->setID(id);
	return result;
}

int iAMMSegParameter::modalityCount()
{
	return m_modalityParams.size();
}

void iAMMSegParameter::setID(int id){
	m_id = id;
}

int iAMMSegParameter::id() const
{
	return m_id;
}
double iAMMSegParameter::erw_beta() const
{
	return m_erw_beta;
}
double iAMMSegParameter::erw_gamma() const
{
	return m_erw_gamma;
}
int iAMMSegParameter::erw_maxIter() const
{
	return m_erw_maxIter;
}
double iAMMSegParameter::svm_c() const
{
	return m_svm_c;
}
double iAMMSegParameter::svm_gamma() const
{
	return m_svm_gamma;
}
int iAMMSegParameter::svm_channels() const
{
	return m_svm_channels;
}



int iAMMSegParameter::pcaDim(int modalityIdx) const
{
	return m_modalityParams[modalityIdx].pcaDim;
}

int iAMMSegParameter::distanceFuncIdx(int modalityIdx) const
{
	return m_modalityParams[modalityIdx].distanceFuncIdx;
}

QSharedPointer<iASpectraDistance> iAMMSegParameter::distanceFunc(int modalityIdx) const
{
	return m_parameterRanges->modalityParamRange[modalityIdx].distanceFuncs[m_modalityParams[modalityIdx].distanceFuncIdx];
}

double iAMMSegParameter::modalityWeight(int modalityIdx) const
{
	return m_modalityParams[modalityIdx].weight;
}

void iAMMSegParameter::setParam(int paramIdx, double value)
{
	switch (paramIdx)
	{
	case erwBeta:         m_erw_beta     = value; break;
	case erwGamma:        m_erw_gamma    = value; break;
	case erwMaxIter:      m_erw_maxIter  = value; break;
	case svmC:            m_svm_c        = value; break;
	case svmGamma:        m_svm_gamma    = value; break;
	case svmChannelCount: m_svm_channels = value; break;
	default: {
		int modIdx = (paramIdx - NonModalityParamCount) / ModalityParamCount;
		int paramId = (paramIdx - NonModalityParamCount) % ModalityParamCount;
		if (m_modalityParams.size() <= modIdx)
		{
			m_modalityParams.resize(modIdx + 1);
		}
		switch (paramId)
		{
			case weight: m_modalityParams[modIdx].weight = value;
			case distance: m_modalityParams[modIdx].distanceFuncIdx = static_cast<int>(value);
			case pca:  m_modalityParams[modIdx].pcaDim = static_cast<int>(value);
		}
	}

	}
}

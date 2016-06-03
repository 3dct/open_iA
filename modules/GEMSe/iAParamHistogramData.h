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
 
#ifndef IA_PARAM_HISTOGRAM_DATA
#define IA_PARAM_HISTOGRAM_DATA

#include "iAAbstractDiagramData.h"
#include "iAGEMSeConstants.h"

#include <QSharedPointer>

#include <cstddef> // for size_t

class iAAttributeFilter;
class iAImageClusterNode;

class iAParamHistogramData: public iAAbstractDiagramRangedData
{
public:
	// TODO: extract creation?
	static QSharedPointer<iAParamHistogramData> Create(iAImageClusterNode const * ,
		AttributeID attribID,
		iAValueType rangeType,
		double min, double max,
		bool log);
	static QSharedPointer<iAParamHistogramData> Create(iAImageClusterNode const * ,
		AttributeID attribID,
		iAValueType rangeType,
		double min, double max,
		bool log,
		iAAttributeFilter const & attributeFilter);
	virtual ~iAParamHistogramData();
	virtual DataType const * GetData() const;
	virtual size_t GetNumBin() const;
	virtual double GetSpacing() const;
	virtual double * GetDataRange();
	virtual double GetDataRange(int idx) const;
	virtual DataType GetMaxValue() const;
	virtual double GetBinStart(int binNr) const;
	double mapValueToBin(double value) const;
	double mapBinToValue(double bin) const;
	iAValueType GetRangeType() const;
	bool IsLogarithmic() const;
private:
	iAParamHistogramData(size_t numBin, double min, double max, bool log, iAValueType rangeType);
	static void VisitNode(iAImageClusterNode const * node, QSharedPointer<iAParamHistogramData> data, AttributeID attribID);
	static void VisitNode(iAImageClusterNode const * node, QSharedPointer<iAParamHistogramData> data, AttributeID attribID, iAAttributeFilter const & attributeFilter);
	static void CountNodeBin(iAImageClusterNode const * node, QSharedPointer<iAParamHistogramData> data, AttributeID attribID);
	DataType * m_data;
	size_t m_numBin;
	double m_dataRange[2];
	DataType m_maxValue;
	DataType m_spacing;
	iAValueType m_rangeType;
	bool m_log;
};


/*
class MyFakeData: public iAAbstractDiagramRangedData
{
public:
	const int FAKE_NUM_BIN = 20;
	MyFakeData():
		m_data(new DataType[FAKE_NUM_BIN])
	{
		const double MU    = 10.0;
		const double SIGMA = 5.0;
		const double PI = 3.14159265;
		for (int i=0; i<FAKE_NUM_BIN; ++i)
		{
			m_data[i] = ( 1.0 / (SIGMA * sqrt(2*PI)) )
				* exp(-1.0/2 * pow((i - MU)/ SIGMA, 2) );
		}
		m_range[0] = 0;
		m_range[1] = FAKE_NUM_BIN;
	}
	~MyFakeData()
	{
		delete [] m_data;
	}
	virtual DataType const * GetData() const 
	{
		return m_data;
	}
	virtual size_t GetNumBin() const
	{
		return FAKE_NUM_BIN;
	}
	virtual double GetSpacing(int component) const 
	{
		return 1;
	}
	virtual double * GetDataRange()
	{
		return m_range;
	}
	virtual double GetDataRange(int idx) const
	{
		return m_range[idx];
	}
	virtual DataType GetMaxValue() const
	{
		return m_data[10];
	}
private:
	DataType* m_data;
	double m_range[2];
};
*/


#endif // IA_PARAM_HISTOGRAM_DATA

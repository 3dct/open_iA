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

#include "iAAbstractDiagramData.h"
#include "iAGEMSeConstants.h"

#include <QSharedPointer>

#include <cstddef> // for size_t

class iAChartFilter;
class iAChartAttributeMapper;
class iAImageTreeNode;
class iAImageTreeLeaf;

class iAParamHistogramData: public iAAbstractDiagramRangedData
{
public:
	// TODO: extract creation?
	static QSharedPointer<iAParamHistogramData> Create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double min, double max,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		int numBin);
	static QSharedPointer<iAParamHistogramData> Create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double min, double max,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter,
		int numBin);
	iAParamHistogramData(size_t numBin,
		double min, double max, bool log,
		iAValueType rangeType);
	void Reset();
	virtual ~iAParamHistogramData();
	virtual DataType const * GetData() const;
	virtual size_t GetNumBin() const;
	virtual double GetSpacing() const;
	virtual double * GetDataRange();
	virtual double GetDataRange(int idx) const;
	virtual DataType GetMaxValue() const;
	virtual double GetBinStart(int binNr) const;
	double MapValueToBin(double value) const;
	double MapBinToValue(double bin) const;
	iAValueType GetRangeType() const;
	bool IsLogarithmic() const;
	virtual double GetMinX() const;
	virtual double GetMaxX() const;
	void SetMinX(double x);
	void SetMaxX(double x);
	void AddValue(double value);
private:
	static void VisitNode(iAImageTreeNode const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);
	static void VisitNode(iAImageTreeNode const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter);
	static void CountNodeBin(iAImageTreeLeaf const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);
	DataType * m_data;
	size_t m_numBin;
	double m_dataRange[2];
	DataType m_maxValue;
	DataType m_spacing;
	iAValueType m_rangeType;
	bool m_log;

	double m_minX, m_maxX;
};

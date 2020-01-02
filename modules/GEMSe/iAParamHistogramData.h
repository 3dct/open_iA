/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAGEMSeConstants.h"

#include <charts/iAPlotData.h>

#include <QSharedPointer>

#include <cstddef> // for size_t

class iAChartFilter;
class iAChartAttributeMapper;
class iAImageTreeNode;
class iAImageTreeLeaf;

class iAParamHistogramData: public iAPlotData
{
public:
	// TODO: extract creation?
	static QSharedPointer<iAParamHistogramData> create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double min, double max,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		int numBin);
	static QSharedPointer<iAParamHistogramData> create(iAImageTreeNode const * ,
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
	void reset();
	virtual ~iAParamHistogramData();
	DataType const * rawData() const override;
	size_t numBin() const override;
	double spacing() const override;
	double const * xBounds() const override;
	DataType const * yBounds() const override;
	double binStart(size_t binNr) const override;
	double mapValueToBin(double value) const;
	double mapBinToValue(double bin) const;
	iAValueType valueType() const override;
	bool isLogarithmic() const;
	double minX() const override;
	double maxX() const override;
	void setMinX(double x);
	void setMaxX(double x);
	void addValue(double value);
private:
	static void visitNode(iAImageTreeNode const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);
	static void visitNode(iAImageTreeNode const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter);
	static void countNodeBin(iAImageTreeLeaf const * node,
		QSharedPointer<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);

	DataType * m_data;
	size_t m_numBin;
	double m_xBounds[2];
	DataType m_yBounds[2];
	DataType m_spacing;
	iAValueType m_rangeType;
	bool m_log;
	double m_minX, m_maxX;
};

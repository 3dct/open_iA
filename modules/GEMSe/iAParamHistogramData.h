/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iAHistogramData.h>

#include <QSharedPointer>

#include <cstddef> // for size_t

class iAChartFilter;
class iAChartAttributeMapper;
class iAImageTreeNode;
class iAImageTreeLeaf;

// TODO: overlap with iAHistogramData / iAMapper?
class iAParamHistogramData : public iAHistogramData
{
public:
	// TODO: extract creation?
	static QSharedPointer<iAParamHistogramData> create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double minX, double maxX,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		int numBin);
	static QSharedPointer<iAParamHistogramData> create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double minX, double maxX,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter,
		int numBin);
	iAParamHistogramData(size_t numBin,
		double minX, double maxX, bool log,
		iAValueType type);
	void reset();
	double xValue(size_t idx) const override;
	double mapValueToBin(double value) const;
	double mapBinToValue(double bin) const;
	bool isLogarithmic() const;
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
	
	bool m_log;
};

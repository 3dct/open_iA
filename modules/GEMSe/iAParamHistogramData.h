// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGEMSeConstants.h"

#include <iAHistogramData.h>

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
	static std::shared_ptr<iAParamHistogramData> create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double minX, double maxX,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		size_t numBin);
	static std::shared_ptr<iAParamHistogramData> create(iAImageTreeNode const * ,
		int chartID,
		iAValueType rangeType,
		double minX, double maxX,
		bool log,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter,
		size_t numBin);
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
		std::shared_ptr<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);
	static void visitNode(iAImageTreeNode const * node,
		std::shared_ptr<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap,
		iAChartFilter const & attributeFilter);
	static void countNodeBin(iAImageTreeLeaf const * node,
		std::shared_ptr<iAParamHistogramData> data,
		int chartID,
		iAChartAttributeMapper const & chartAttrMap);

	bool m_log;
};

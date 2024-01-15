// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>

#include <utility>

class iAChartAttributeMapper
{
public:
	int GetChartID(int datasetID, int attributeID) const;
	QList<int> GetDatasetIDs(int chartID) const;
	int GetAttributeID(int chartID, int datasetID) const;
	int GetAttribCount(int datasetID) const;

	void Add(int datasetID, int attributeID, int chartID);
	void Clear();
private:
	//! map from chartID to (map from datasetID to attributeID)
	QMap<int, QMap<int, int> > m_chartAttributeMap;

	//! map from (datasetID, attributeID) to chartID
	QMap<std::pair<int, int>, int>  m_attributeChartMap;
	//! map from datasetID to attribute count
	QMap<int, int> m_datasetAttribCount;
};

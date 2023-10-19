// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFiberData.h"
#include "iAFiberResult.h"

#include <iAProgress.h>

#include <QDataStream>
#include <QThread>

#include <memory>
#include <vector>

class iAFiberResultsCollection;

class vtkTable;

class QFile;


static const QDataStream::Version CacheFileQtDataStreamVersion(QDataStream::Qt_5_6);

class iARefDistCompute : public QThread
{
	Q_OBJECT
public:
	//! type for containers - but since we mix QVector and std::vector usages, it doesn't really help!
	typedef int ContainerSizeType;
	static ContainerSizeType MaxNumberOfCloseFibers;
	iARefDistCompute(std::shared_ptr<iAFiberResultsCollection> data, size_t referenceID);
	bool setMeasuresToCompute(std::vector<std::pair<int, bool>> const& measuresToCompute, int optimizationMeasure, int bestMeasure);
	void run() override;
	iAProgress* progress();
	size_t referenceID() const;
	size_t columnsBefore() const;
	size_t columnsAdded() const;
private:
	bool readResultRefComparison(QFile& file, size_t resultID, bool& first);
	void writeResultRefComparison(QFile& cacheFile, size_t resultID);
	bool readAverageMeasures(QFile& cacheFile);
	void writeAverageMeasures(QFile& cacheFile);

	iAProgress m_progress;
	std::shared_ptr<iAFiberResultsCollection> m_data;
	size_t m_referenceID;
	std::vector<std::pair<int, bool>> m_measuresToCompute;  //!< index of measure to compute along with flag whether to use optimized computation
	size_t m_columnsBefore;
	int m_optimizationMeasureIdx,
		m_bestMeasure;

	//! @{ internal computation caches:
	double m_diagonalLength, m_maxLength;
	//! @}
};

void getBestMatches(iAFiberData const& fiber,
	QMap<uint, uint> const& mapping,
	vtkTable* refTable,
	QVector<QVector<iAFiberSimilarity> >& bestMatches,
	std::map<size_t, std::vector<iAVec3f> > const& refCurveInfo,
	double diagonalLength, double maxLength,
	std::vector<std::pair<int, bool>>& measuresToCompute, int optimizationMeasureIdx);

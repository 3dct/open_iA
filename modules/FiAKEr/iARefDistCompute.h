/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAProgress.h"

#include <QSharedPointer>
#include <QThread>

#include <vector>

class iAFiberResultsCollection;
class iAFiberAndRefData;

class vtkTable;

class QFile;

class iARefDistCompute : public QThread
{
	Q_OBJECT
public:
	typedef int ContainerSizeType;
	//static const int SimilarityMeasureCount = 20;
	//static const int BestSimilarityMeasure = 7;
	//static const int OverlapMeasureCount = 3;
	//static const int OverlapMeasureStart = SimilarityMeasureCount-OverlapMeasureCount;
	//static const int EndColumns = 2;
	//static const int BestMeasureWithoutOverlap = 2;
	static ContainerSizeType MaxNumberOfCloseFibers;
	iARefDistCompute(QSharedPointer<iAFiberResultsCollection> data, size_t referenceID);
	bool setMeasuresToCompute(std::vector<std::pair<int, bool>> const& measuresToCompute, int optimizationMeasure, int bestMeasure);
	void run() override;
	iAProgress* progress();
	size_t referenceID() const;
	size_t columnsBefore() const;
	size_t columnsAdded() const;
private:
	bool readResultRefComparison(QFile& file, size_t resultID);
	void writeResultRefComparison(QFile& cacheFile, size_t resultID);
	bool readAverageMeasures(QFile& cacheFile);
	void writeAverageMeasures(QFile& cacheFile);
	void getBestMatches(iAFiberAndRefData& fiberRef);

	iAProgress m_progress;
	QSharedPointer<iAFiberResultsCollection> m_data;
	size_t m_referenceID;
	std::vector<std::pair<int, bool>> m_measuresToCompute;  //!< index of measure to compute along with flag whether to use optimized computation
	size_t m_columnsBefore;
	int m_optimizationMeasureIdx,
		m_bestMeasure;

	//! @{ internal computation caches:
	double m_diagonalLength, m_maxLength;
	//! @}
};

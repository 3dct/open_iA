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

#include "iAProgress.h"

#include <QSharedPointer>
#include <QThread>

#include <vector>

class iAFiberResultsCollection;

class QFile;

class iARefDistCompute : public QThread
{
	Q_OBJECT
public:
	static const int SimilarityMeasureCount = 20;
	static const int BestSimilarityMeasure = 7;
	static const int OverlapMeasureCount = 3;
	static const int OverlapMeasureStart = SimilarityMeasureCount-OverlapMeasureCount;
	static const int EndColumns = 2;
	static const int BestMeasureWithoutOverlap = 2;
	static size_t MaxNumberOfCloseFibers;
	iARefDistCompute(QSharedPointer<iAFiberResultsCollection> data, int referenceID);
	void run() override;
	iAProgress* progress();
	size_t referenceID() const;
	static QStringList getSimilarityMeasureNames();
private:
	bool readFromCache(QFile& cacheFile);
	void writeToCache(QFile& cacheFile);

	iAProgress m_progress;
	QSharedPointer<iAFiberResultsCollection> m_data;
	int m_referenceID;
};

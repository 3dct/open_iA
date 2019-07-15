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

#include "iAAbortListener.h"
#include "iADurationEstimator.h"

#include <iAPerformanceHelper.h>

#include <QSharedPointer>
#include <QVector>
#include <QThread>

class iAImageTree;
class iAImageTreeNode;
class iASingleResult;

class iAImageClusterer: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:

	iAImageClusterer(int labelCount, QString const & outputDirectory);
	void AddImage(QSharedPointer<iASingleResult> singleResult);

	QSharedPointer<iAImageTree > GetResult();

	void abort() override;
	bool IsAborted();
	double elapsed() const override;
	double estimatedTimeRemaining() const override;
signals:
	void Progress(int);
	void Status(QString const &);
private:
	void run();
	QVector<QSharedPointer<iAImageTreeNode> > m_images;
	QSharedPointer<iAImageTree> m_tree;
	int m_labelCount;
	bool m_aborted;
	iAPerformanceTimer m_perfTimer;
	int m_remainingNodes;
	int m_currImage;
	iAPerformanceTimer::DurationType m_imageDistCalcDuration;
	QString m_outputDirectory;
};

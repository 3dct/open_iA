// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAbortListener.h"
#include "iADurationEstimator.h"

#include <iAPerformanceHelper.h>

#include <QSharedPointer>
#include <QVector>
#include <QThread>

class iAImageTree;
class iAImageTreeNode;
class iAProgress;
class iASingleResult;

class iAImageClusterer: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:

	iAImageClusterer(int labelCount, QString const & outputDirectory, iAProgress* progress);
	void AddImage(QSharedPointer<iASingleResult> singleResult);

	QSharedPointer<iAImageTree > GetResult();

	void abort() override;
	bool IsAborted();
	double elapsed() const override;
	double estimatedTimeRemaining(double percent) const override;
private:
	void run() override;
	QVector<QSharedPointer<iAImageTreeNode> > m_images;
	QSharedPointer<iAImageTree> m_tree;
	int m_labelCount;
	bool m_aborted;
	iAPerformanceTimer m_perfTimer;
	int m_remainingNodes;
	int m_currImage;
	iAPerformanceTimer::DurationType m_imageDistCalcDuration;
	QString m_outputDirectory;
	iAProgress* m_progress;
};

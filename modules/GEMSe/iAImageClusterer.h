/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_IMAGE_CLUSTERER_H
#define IA_IMAGE_CLUSTERER_H

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iAPerformanceHelper.h"

#include <QSharedPointer>
#include <QVector>
#include <QThread>

class iAImageTree;
class iAImageClusterNode;
class iASingleResult;

class iAImageClusterer: public QThread, public iADurationEstimator, public iAAbortListener
{
	Q_OBJECT
public:

	iAImageClusterer(int labelCount, QString const & outputDirectory);
	void AddImage(QSharedPointer<iASingleResult> singleResult);

	QSharedPointer<iAImageTree > GetResult();

	void Abort();
	bool IsAborted();
	virtual double elapsed() const;
	virtual double estimatedTimeRemaining() const;
signals:
	void Progress(int);
	void Status(QString const &);
private:
	void run();
	QVector<QSharedPointer<iAImageClusterNode> > m_images;
	QSharedPointer<iAImageTree> m_tree;
	int m_labelCount;
	bool m_aborted;
	iAPerformanceTimer m_perfTimer;
	int m_remainingNodes;
	int m_currImage;
	iAPerformanceTimer::DurationType m_imageDistCalcDuration;
	QString m_outputDirectory;
};

#endif
// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAProgress.h>

#include <QSharedPointer>
#include <QThread>
#include <QVector>

class iAAccumulatedXRFData;
class iAElementSpectralInfo;
class iAElementConcentrations;
class iAXRFData;

class iADecompositionCalculator: public QThread
{
	Q_OBJECT
public:
	iADecompositionCalculator(
		QSharedPointer<iAElementConcentrations> data,
		iAXRFData const * xrfData,
		iAAccumulatedXRFData const * accumulatedXRF);
	void AddElement(iAElementSpectralInfo* element);
	int ElementCount() const;
	void Stop();
	virtual void run();
	iAProgress* progress();
private:
	QSharedPointer<iAElementConcentrations> m_data;
	iAXRFData const * m_xrfData;
	iAAccumulatedXRFData const * m_accumulatedXRF;
	QVector<iAElementSpectralInfo*> m_elements;
	bool m_stopped;
	iAProgress m_progress;
signals:
	void success();
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAProgress.h>

#include <QThread>
#include <QVector>

#include <memory>

class iAAccumulatedXRFData;
class iAElementSpectralInfo;
class iAElementConcentrations;
class iAXRFData;

class iADecompositionCalculator: public QThread
{
	Q_OBJECT
public:
	iADecompositionCalculator(
		std::shared_ptr<iAElementConcentrations> data,
		iAXRFData const * xrfData,
		iAAccumulatedXRFData const * accumulatedXRF);
	void AddElement(iAElementSpectralInfo* element);
	int ElementCount() const;
	void Stop();
	virtual void run();
	iAProgress* progress();
private:
	std::shared_ptr<iAElementConcentrations> m_data;
	iAXRFData const * m_xrfData;
	iAAccumulatedXRFData const * m_accumulatedXRF;
	QVector<iAElementSpectralInfo*> m_elements;
	bool m_stopped;
	iAProgress m_progress;
signals:
	void success();
};

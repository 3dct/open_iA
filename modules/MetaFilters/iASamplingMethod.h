// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include <iAAttributes.h>

#include <QSharedPointer>
#include <QVector>

typedef QVector<QVariant> iAParameterSet;
typedef QVector<iAParameterSet> iAParameterSets;
typedef QSharedPointer<iAParameterSets> iAParameterSetsPointer;

class MetaFilters_API iASamplingMethod
{
public:
	virtual ~iASamplingMethod();
	virtual QString name() const =0;
	virtual bool supportsSamplesPerParameter() const;
	virtual void setSamplesPerParameter(std::vector<int> samplesPerParameter);
	virtual int sampleCount() const;
	virtual void setSampleCount(int sampleCount, QSharedPointer<iAAttributes> parameters);
	virtual iAParameterSetsPointer parameterSets(QSharedPointer<iAAttributes> parameters) =0;
private:
	int m_sampleCount;
};

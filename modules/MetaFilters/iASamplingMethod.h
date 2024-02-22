// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include <iAAttributes.h>

#include <QVector>

#include <memory>

typedef QVector<QVariant> iAParameterSet;
typedef QVector<iAParameterSet> iAParameterSets;
typedef std::shared_ptr<iAParameterSets> iAParameterSetsPointer;

class MetaFilters_API iASamplingMethod
{
public:
	virtual ~iASamplingMethod();
	virtual QString name() const =0;
	virtual bool supportsSamplesPerParameter() const;
	virtual void setSamplesPerParameter(std::vector<int> samplesPerParameter);
	virtual int sampleCount() const;
	virtual void setSampleCount(int sampleCount, std::shared_ptr<iAAttributes> parameters);
	virtual iAParameterSetsPointer parameterSets(std::shared_ptr<iAAttributes> parameters) =0;
private:
	int m_sampleCount;
};

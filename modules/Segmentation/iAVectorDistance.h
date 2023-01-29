// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVectorType.h"

#include <QSharedPointer>

//! abstract base class for the distance between two vectors of same length
class Segmentation_API iAVectorDistance
{
public:
	static double EPSILON;
	virtual ~iAVectorDistance();
	virtual char const * GetShortName() const =0;
	virtual char const * name() const =0;
	virtual double GetDistance(QSharedPointer<iAVectorType const> spec1, QSharedPointer<iAVectorType const> spec2) const = 0;
	virtual bool isSymmetric() const;
};

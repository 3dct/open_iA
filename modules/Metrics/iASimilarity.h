// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAutoRegistration.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>

class iASimilarity : public iAFilter, private iAAutoRegistration<iAFilter, iASimilarity, iAFilterRegistry>
{
public:
	iASimilarity();
	void adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets) override;

private:
	void performWork(QVariantMap const& parameters) override;
};

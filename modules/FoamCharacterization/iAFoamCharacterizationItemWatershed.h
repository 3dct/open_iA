// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationItem.h"

#include <itkImage.h>

class QFile;

class iAFoamCharacterizationItemWatershed : public iAFoamCharacterizationItem
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationItemWatershed(iAFoamCharacterizationTable* _pTable);
	explicit iAFoamCharacterizationItemWatershed(iAFoamCharacterizationItemWatershed* _pWatershed);

	std::shared_ptr<iADataSet> executeFloat(itk::ImageBase<3>* img);
	std::shared_ptr<iADataSet> executeUnsignedShort(itk::ImageBase<3>* img);

	int itemMask() const;

	double level() const;
	double threshold() const;

	void setItemMask(const int& _iItemMask);
	void setLevel(const double& _dLevel);
	void setThreshold(const double& _dThreshold);

	void dialog() override;
	std::shared_ptr<iADataSet> execute(std::shared_ptr<iADataSet> dataSet) override;
	void open(QFile* _pFileOpen) override;
	void save(QFile* _pFileSave) override;

private:
	double m_dLevel = 0.4;
	double m_dThreshold = 0.1;

	int m_iItemMask = -1;
};

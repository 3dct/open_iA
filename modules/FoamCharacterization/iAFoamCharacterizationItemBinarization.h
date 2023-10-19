// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationItem.h"

class QFile;

class iAFoamCharacterizationItemBinarization : public iAFoamCharacterizationItem
{
	Q_OBJECT

public:
	enum EItemFilterType { iftBinarization, iftOtzu};

public:
	explicit iAFoamCharacterizationItemBinarization(iAFoamCharacterizationTable* _pTable);
	explicit iAFoamCharacterizationItemBinarization(iAFoamCharacterizationItemBinarization* _pBinarization);

	std::shared_ptr<iADataSet> imageDataMask();

	EItemFilterType itemFilterType() const;

	bool isMask() const;

	unsigned short lowerThreshold() const;

	unsigned int otzuHistogramBins() const;

	void setIsMask(const bool& _bIsMask);
	void setLowerThreshold(const unsigned short& _usLowerThreshold);
	void setOtzuHistogramBins(const unsigned int& _uiOtzuHistogramBins);
	void setUpperThreshold(const unsigned short& _usUpperThreshold);

	unsigned short upperThreshold() const;

	void setItemFilterType(const EItemFilterType& _eItemFilterType);

	void dialog() override;
	std::shared_ptr<iADataSet> execute(std::shared_ptr<iADataSet> input) override;
	void open(QFile* _pFileOpen) override;
	void save(QFile* _pFileSave) override;

private:
	bool m_bIsMask = false;
	std::shared_ptr<iADataSet> m_mask;

	EItemFilterType m_eItemFilterType = iftOtzu;

	unsigned short m_usLowerThreshold = 0;
	unsigned short m_usUpperThreshold = 65535;
	unsigned int m_uiOtzuHistogramBins = 500;

	std::shared_ptr<iADataSet> executeBinarization(std::shared_ptr<iADataSet> dataSet);
	std::shared_ptr<iADataSet> executeOtzu(std::shared_ptr<iADataSet> dataSet);

	QString itemFilterTypeString() const;

protected:
	virtual void setItemText() override;
};

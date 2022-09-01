/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAFoamCharacterizationItem.h"

#include <vtkSmartPointer.h>

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

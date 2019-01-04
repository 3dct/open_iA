/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
	explicit iAFoamCharacterizationItemBinarization(iAFoamCharacterizationTable* _pTable , vtkImageData* _pImageData);
	explicit iAFoamCharacterizationItemBinarization(iAFoamCharacterizationItemBinarization* _pBinarization);

	vtkImageData* imageDataMask();

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

	virtual void dialog() override;
	virtual void execute() override;
	virtual void open(QFile* _pFileOpen) override;
	virtual void save(QFile* _pFileSave) override;

private:
	bool m_bIsMask = false;
		
	EItemFilterType m_eItemFilterType = iftOtzu;

	unsigned short m_usLowerThreshold = 0;
	unsigned short m_usUpperThreshold = 65535;
	unsigned int m_uiOtzuHistogramBins = 500;

	vtkSmartPointer<vtkImageData> m_pImageDataMask;

	void executeBinarization();
	void executeOtzu();

	QString itemFilterTypeString() const;

protected:
	virtual void setItemText() override;
};

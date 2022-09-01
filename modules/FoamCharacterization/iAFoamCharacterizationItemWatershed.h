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

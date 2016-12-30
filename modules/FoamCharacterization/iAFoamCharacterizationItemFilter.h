/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

class QFile;

class vtkImageData;

class iAFoamCharacterizationItemFilter : public iAFoamCharacterizationItem
{
	public:
		enum EItemFilterType { iftAnisotropic, iftGauss , iftMedian, iftNonLocalMeans};

	public:
		explicit iAFoamCharacterizationItemFilter(vtkImageData* _pImageData);
		explicit iAFoamCharacterizationItemFilter(iAFoamCharacterizationItemFilter* _pFilter);

		double anisotropicConductance() const;
		int anisotropicIteration() const;
		double anisotropicTimeStep() const;

		double gaussianVariance() const;

		EItemFilterType itemFilterType() const;

		int medianBoxRadius() const;

		void setAnisotropicConductance(const double& _dAnisotropicConductance);
		void setAnisotropicIteration(const int& _iAnisotropicIteration);
		void setAnisotropicTimeStep(const double& _dAnisotropicTimeStep);
		void setGaussianVariance(const double& _dGaussianVariance);
		void setItemFilterType(const EItemFilterType& _eItemFilterType);
		void setMedianBoxRadius(const int& _iBoxRadius);

		virtual void dialog() override;
		virtual void execute() override;
		virtual void open(QFile* _pFileOpen) override;
		virtual void save(QFile* _pFileSave) override;

	private:
		double m_dAnisotropicConductance = 1.0;
		double m_dAnisotropicTimeStep= 0.1;
		double m_dGaussianVariance = 4.0;

		EItemFilterType m_eItemFilterType = iftGauss;

		int m_iAnisotropicIteration = 2;
		int m_iMedianBoxRadius = 2;

		void executeAnisotropic();
		void executeGaussian();
		void executeMedian();
		void executeNonLocalMeans();

		QString itemFilterTypeString() const;

	protected:
		virtual void setItemText() override;
};

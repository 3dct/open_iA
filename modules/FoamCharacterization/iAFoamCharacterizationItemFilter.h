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

#include <QRunnable>

class QFile;

class vtkImageData;

#include <itkPatchBasedDenoisingBaseImageFilter.h>

class iAFoamCharacterizationItemFilter : public iAFoamCharacterizationItem
{
	Q_OBJECT

	//typedef itk::PatchBasedDenoisingBaseImageFilter
		//<itk::Image<unsigned short, 3>, itk::Image<unsigned short, 3>>::NoiseModelType ENoiseModelType;
		
	class QtRunnableMedian : public QRunnable
	{
		public:
			explicit QtRunnableMedian ( iAFoamCharacterizationItemFilter* _pItemFilter
									  , unsigned short* _pDataRead
									  , unsigned short* _pDataWrite
									  , const unsigned int& _uiNi
									  , const unsigned int& _uiNj
									  , const unsigned int& _uiNk
									  , const unsigned int& _uiStrideJ
									  , const unsigned int& _uiStrideK
									  , const unsigned int& _uiK1
									  , const unsigned int& _uiK2
									  ) : QRunnable()
										, m_pItemFilter (_pItemFilter)
										, m_pDataRead (_pDataRead)
										, m_pDataWrite (_pDataWrite)
										, m_uiNi(_uiNi)
				                        , m_uiNj(_uiNj)
				                        , m_uiNk(_uiNk)
										, m_uiStrideJ(_uiStrideJ)
										, m_uiStrideK(_uiStrideK)
										, m_uiK1(_uiK1)
										, m_uiK2 (_uiK2)
			{
				setAutoDelete(false);
			}

			virtual void run() override
			{
				m_pItemFilter->executeMedianFX ( m_pDataRead, m_pDataWrite
					                           , m_uiNi, m_uiNj, m_uiNk, m_uiStrideJ, m_uiStrideK, m_uiK1, m_uiK2
										       );
			}

		private:
			iAFoamCharacterizationItemFilter* m_pItemFilter = nullptr;

			unsigned short* m_pDataRead = nullptr;
			unsigned short* m_pDataWrite = nullptr;

			unsigned int m_uiNi = 0;
			unsigned int m_uiNj = 0;
			unsigned int m_uiNk = 0;

			unsigned int m_uiStrideJ = 0;
			unsigned int m_uiStrideK = 0;

			unsigned int m_uiK1 = 0;
			unsigned int m_uiK2 = 0;
	};

	public:
		enum EItemFilterType { iftAnisotropic, iftGauss, iftMedian, iftNonLocalMeans };

	public:
		explicit iAFoamCharacterizationItemFilter(iAFoamCharacterizationTable* _pTable, vtkImageData* _pImageData);
		explicit iAFoamCharacterizationItemFilter(iAFoamCharacterizationItemFilter* _pFilter);

		double anisotropicConductance() const;
		unsigned int anisotropicIteration() const;
		double anisotropicTimeStep() const;

		void executeMedianFX ( unsigned short* _pDataRead, unsigned short* _pDataWrite
							 , const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
							 , const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
							 , const unsigned int& _uiK1, const unsigned int& _uiK2
							 );

		bool gaussianImageSpacing() const;
		double gaussianVariance() const;

		EItemFilterType itemFilterType() const;

		unsigned int medianRadius() const;
		unsigned int nonLocalMeansIteration() const;
		unsigned int nonLocalMeansRadius() const;

		void setAnisotropicConductance(const double& _dAnisotropicConductance);
		void setAnisotropicIteration(const unsigned int& _uiAnisotropicIteration);
		void setAnisotropicTimeStep(const double& _dAnisotropicTimeStep);
		void setGaussianImageSpacing(const bool& _bGaussianImageSpacing);
		void setGaussianVariance(const double& _dGaussianVariance);
		void setItemFilterType(const EItemFilterType& _eItemFilterType);
		void setMedianRadius(const unsigned int& _uiMedianRadius);
		void setNonLocalMeansIteration(const unsigned int& _uiNonLocalMeansIteration);
		void setNonLocalMeansRadius(const unsigned int& _uiNonLocalMeansRadius);

		virtual void dialog() override;
		virtual void execute() override;
		virtual void open(QFile* _pFileOpen) override;
		virtual void save(QFile* _pFileSave) override;

	private:
		bool m_bGaussianImageSpacing = true;

		double m_dAnisotropicConductance = 1.0;
		double m_dAnisotropicTimeStep = 0.1;
		double m_dGaussianVariance = 1.0;

		EItemFilterType m_eItemFilterType = iftMedian;

		//ENoiseModelType m_nmtNonLocalMeans = ENoiseModelType::POISSON;

		int m_uiMedianFXSlice = 0;

		unsigned int m_uiAnisotropicIteration = 2;
		unsigned int m_uiMedianRadius = 2;
		unsigned int m_uiNonLocalMeansIteration = 1;
		unsigned int m_uiNonLocalMeansRadius = 2;

		void executeAnisotropic();
		void executeGaussian();
		void executeMedian();
		void executeMedianFX();

		void executeMedianFX1(unsigned short* _pDataRead, unsigned short* _pDataWrite
			, const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
			, const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
			, const unsigned int& _uiK1, const unsigned int& _uiK2
		);

		void executeMedianFXSlice(unsigned short* _pDataRead, unsigned short* _pDataWrite
			, const unsigned int& _uiNi, const unsigned int& _uiNj, const unsigned int& _uiNk
			, const unsigned int& _uiStrideJ, const unsigned int& _uiStrideK
			, const unsigned int& _uiK
		);

		unsigned short executeMedianFXValue(unsigned short* _pDataRead
			, const unsigned int& _i1, const unsigned int& _i2
			, const unsigned int& _j1, const unsigned int& _j2
			, const unsigned int& _k1, const unsigned int& _k2
			, const unsigned int& _uiStrideJ
			, const unsigned int& _uiStrideK
			, const unsigned int& _uiBoxSize
		);

		void executeNonLocalMeans();

		QString itemFilterTypeString() const;

	protected:
		virtual void setItemText() override;
};

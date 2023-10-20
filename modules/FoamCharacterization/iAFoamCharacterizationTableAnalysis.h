// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkFixedArray.h>
#include <itkIndex.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <QTableView>

class iAFoamCharacterizationTableAnalysis : public QTableView
{
	Q_OBJECT

	class CTableAnalysisRow
	{
	public:
		CTableAnalysisRow();

		void set(const long& _lLabel
			, const double& _dCenterX, const double& _dCenterY, const double& _dCenterZ
			, const double& _dVolume, const double& _dDiameter
			, const itk::FixedArray<itk::Index<3>::IndexValueType, 6> _faBoundingBox);

		long label() const;

		double centerX() const;
		double centerY() const;
		double centerZ() const;

		double volume() const;
		double diameter() const;

		double* boundingBox();

	private:
		long m_lLabel = 0;

		double m_dCenterX = 0.0;
		double m_dCenterY = 0.0;
		double m_dCenterZ = 0.0;

		double m_dVolume = 0.0;
		double m_dDiameter = 0.0;

		double m_pBoundingBox[6];
	};

public:
	explicit iAFoamCharacterizationTableAnalysis(QWidget* _pParent = nullptr);

	void setRowCount(const int& _iRowCount);

	void setRow ( const int& _iRow, const long& _lLabel
				, const double& _dCenterX, const double& _dCenterY, const double& _dCenterZ
				, const double& _dVolume, const double& _dDiameter
				, const itk::FixedArray<itk::Index<3>::IndexValueType, 6> _faBoundingBox
				);

private:
	QVector<CTableAnalysisRow> m_vData;
};

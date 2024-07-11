// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QTableView>

class iAFoamCharacterizationTableAnalysis : public QTableView
{
	Q_OBJECT

	class CTableAnalysisRow
	{
	public:
		CTableAnalysisRow();

		void set(const long& lLabel
			, const double& dCenterX, const double& dCenterY, const double& dCenterZ
			, const double& dVolume, const double& dDiameter);

		long label() const;

		double centerX() const;
		double centerY() const;
		double centerZ() const;

		double volume() const;
		double diameter() const;

	private:
		long m_lLabel = 0;

		double m_dCenterX = 0.0;
		double m_dCenterY = 0.0;
		double m_dCenterZ = 0.0;

		double m_dVolume = 0.0;
		double m_dDiameter = 0.0;
	};

public:
	explicit iAFoamCharacterizationTableAnalysis(QWidget* _pParent = nullptr);

	void setRowCount(const int& _iRowCount);

	void setRow ( const int& iRow, const long& lLabel
				, const double& centerX, const double& dCenterY, const double& dCenterZ
				, const double& volume, const double& dDiameter
				, std::array<int64_t, 3> bbOrigin, std::array<uint64_t, 3> bbSize
				);

private:
	QVector<CTableAnalysisRow> m_vData;
};

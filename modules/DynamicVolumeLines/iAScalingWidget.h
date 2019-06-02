/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <charts/qcustomplot.h>
#include <qthelper/iAQGLWidget.h>

#include <vtkSmartPointer.h>

class vtkLookupTable;

class iAScalingWidget : public iAQGLWidget
{
	Q_OBJECT

public:
	iAScalingWidget(QWidget* parent = 0);

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	void setNonlinearScalingVec(const QVector<double> &nls, const QVector<double> &impfv);
	void setAxes(QCPAxis *nla, QCPAxis *la);
	void setCursorPos(double lcp, double nlcp);
	void setRange(double lowerIdx, double upperIdx, double nonlinearLowerRest, 
		double nonlinearUpperRest, double linearLowerRest, double linearUpperRest);
	void setBkgrdThrRanges(const QList<QCPRange> &bkgrdRangeList);
	void setSel(QCPDataSelection sel);
	void setHistVisMode(bool histVisMode);

	void setOverviewRange(double lowerIdx, double upperIdx, double nonlinearLowerRest,
		double nonlinearUpperRest, double linearLowerRest, double linearUpperRest, 
		const QVector<double> &histBinImpFunctAvgVec, const QVector<double> &linearHistBinBoarderVec);

protected:
	void initializeGL() override;
	void paintGL() override;

private:
	QCPAxis *m_nonlinearAxis;
	QCPAxis *m_linearAxis;
	QVector<double> m_nonlinearScalingVec;
	QVector<double> m_impFunctVec;
	vtkSmartPointer<vtkLookupTable> m_lut;
	double m_linearBarCursorPos, m_nonlinearBarCursorPos, m_nonlinearLowerIdx,
		m_nonlinearUpperIdx, m_nonlinearLowerRest, m_nonlinearUpperRest,
		m_linearLowerRest, m_linearUpperRest, m_prevNonlinearBarStartPosX, 
		m_prevLinearBarStartPosX;
	QList<QCPRange> m_bkgrdRangeList;
	QCPDataSelection m_sel;
	bool m_histVisMode;

	QVector<double> m_histBinImpFunctAvgVec;
	QVector<double> m_linearHistBinBoarderVec;
};

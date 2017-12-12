/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QOpenGLWidget>

#include <qcustomplot.h>

#include <vtkSmartPointer.h>

class vtkLookupTable;

class iAScalingWidget : public QOpenGLWidget
{
	Q_OBJECT

public:
	iAScalingWidget(QWidget* parent = 0);

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

	void setNonlinearScalingVector(QVector<double> nls, QVector<double> impfv);
	void setNonlinearAxis(QCPAxis *nla);
	void setCursorPositions(double lcp, double nlcp);
	void setRange(double lowerIdx, double upperIdx, double nonlinearLowerRest, 
		double nonlinearUpperRest, double linearLowerRest, double linearUpperRest);
	void setBkgrdThrRanges(QList<QCPRange> bkgrdRangeList);
	void setSelection(QCPDataSelection sel);

protected:
	virtual void initializeGL();
	virtual void paintGL();

private:
	QCPAxis *m_nonlinearAxis;
	QVector<double> m_nonlinearScalingVec;
	QVector<double> m_impFunctVec;
	vtkSmartPointer<vtkLookupTable> m_lut;
	double m_linearBarCursorPos, m_nonlinearBarCursorPos, m_nonlinearLowerIdx,
		m_nonlinearUpperIdx, m_nonlinearLowerRest, m_nonlinearUpperRest,
		m_linearLowerRest, m_linearUpperRest, m_prevNonlinearBarStartPosX, 
		m_prevLinearBarStartPosX;
	QList<QCPRange> m_bkgrdRangeList;
	QCPDataSelection m_sel;
};
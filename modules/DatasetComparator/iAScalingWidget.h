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

#include <QWidget>

#include <qcustomplot.h>

#include <vtkSmartPointer.h>

class vtkLookupTable;

class iAScalingWidget : public QWidget
{
    Q_OBJECT

public:
	iAScalingWidget(QWidget* parent = 0);

	void setNonlinearScalingVector(QVector<double> nls, QVector<double> impfv);
	void setNonlinearAxis(QCPAxis *nla);
	void setCursorPositions(double, double);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
	QCPAxis *m_nonlinearAxis;
	QVector<double> m_nonlinearScalingVec;
	QVector<double> m_impFunctVec;
	vtkSmartPointer<vtkLookupTable> m_lut;
	QStaticText m_normalText;
	QStaticText m_nonlinearText;
	double m_linearBarCursorPos, m_nonlinearBarCursorPos;
};

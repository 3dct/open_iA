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

#include <QMap>
#include <QWidget>

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>

class iALinearColorGradientBar : public QWidget
{
	Q_OBJECT

public:
	iALinearColorGradientBar(QWidget *parent, QString colormapName,
		bool modifiable, bool flipColormap = false);
	vtkSmartPointer<vtkLookupTable> getLut();

public slots:
	void compLevelRangeChanged(QVector<double> range);

signals:
	void colorMapChanged(vtkSmartPointer<vtkLookupTable> lut);

protected:
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	bool event(QEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent*) override;
	void paintEvent(QPaintEvent *e) override;

private:
	QMap<double, QColor> m_colormap;
	vtkSmartPointer<vtkLookupTable> m_lut;
	bool m_modifiable, m_showSelection;
	QVector<double> m_compLevelRange;
};
// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

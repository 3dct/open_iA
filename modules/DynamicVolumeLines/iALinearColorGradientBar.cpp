// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iALinearColorGradientBar.h"
#include "iALUT.h"

#include <QPainter>
#include <QInputDialog>
#include <QStringList>
#include <QToolTip>
#include <QEvent>
#include <QHelpEvent>

iALinearColorGradientBar::iALinearColorGradientBar(QWidget *parent, QString colormapName,
	bool modifiable, bool flipColormap) :
	QWidget(parent),
	m_modifiable(modifiable),
	m_compLevelRange(2)
{
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	int colorCnt = iALUT::BuildLUT(m_lut, 0.0, 1.0, colormapName);
	QColor color;
	for (double i = 0.0; i <= 1.0; i += 1.0 / (colorCnt-1))
	{
		double c[3];
		m_lut->GetColor((flipColormap ? 1-i : i), c);
		color.setRgbF(c[0], c[1], c[2]);
		m_colormap.insert(i, color);
	}
}

vtkSmartPointer<vtkLookupTable> iALinearColorGradientBar::getLut()
{
	return m_lut;
}

QSize iALinearColorGradientBar::sizeHint() const
{
	return QSize(100, 20);
}

QSize iALinearColorGradientBar::minimumSizeHint() const
{
	return QSize(100, 20);
}

void iALinearColorGradientBar::compLevelRangeChanged(QVector<double> range)
{
	m_compLevelRange = range;
	update();
}

bool iALinearColorGradientBar::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip && m_modifiable)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		QToolTip::showText(helpEvent->globalPos(), "Double-click to change the colormap of the Histogram Heatmap.");
		return true;
	}
	return QWidget::event(event);
}

void iALinearColorGradientBar::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
	if (!m_modifiable)
	{
		return;
	}

	bool ok;
	QString colormapName = QInputDialog::getItem(this, tr("Change Histogram Colormap"),
		tr(""), iALUT::colorMapNames(), 0, false, &ok);

	if (!ok)
	{
		return;
	}

	m_colormap.clear();
	int colorCnt = iALUT::BuildLUT(m_lut, 0.0, 1.0, colormapName);
	QColor color;
	for (double i = 0.0; i <= 1.0; i += 1.0 / (colorCnt - 1))
	{
		double c[3];
		m_lut->GetColor(i, c);
		color.setRgbF(c[0], c[1], c[2]);
		m_colormap.insert(i, color);
	}
	emit colorMapChanged(m_lut);
	update();
}

void iALinearColorGradientBar::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);
	QPainter painter(this);
	QLinearGradient grad(0.0, 0.0, width(), 0.0);
	for (auto it = m_colormap.begin(); it != m_colormap.end(); ++it)
	{
		grad.setColorAt(it.key(), it.value());
	}
	painter.fillRect(0, 0, width(), height(), grad);
	if ((m_compLevelRange[1] - m_compLevelRange[0]) > 0)
	{
		painter.setPen(QColor(255, 0, 0));
		painter.drawRect(m_compLevelRange[0] * width(), 0.0,
			(m_compLevelRange[1] - m_compLevelRange[0]) * width() - 1.0, height() - 1.0);
	}
}

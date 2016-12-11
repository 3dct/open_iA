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

#include "iABoneThicknessChartBar.h"

#include <vtkDoubleArray.h>

#include <QMouseEvent>
#include <QPainter>

#include "iABoneThickness.h"
#include "iABoneThicknessTable.h"

iABoneThicknessChartBar::iABoneThicknessChartBar(QWidget* _pParent) : QWidget(_pParent)
{
	setCursor(Qt::PointingHandCursor);

	m_cBrush = palette().color(QPalette::Base);
	m_cPen1 = palette().color(QPalette::WindowText);
	m_cPen2 = palette().color(QPalette::Window);

	m_pImage.reset(new QImage(width(), height(), QImage::Format_RGB32));

	m_iMarginX = logicalDpiX() / 10;
	m_iMarginY = logicalDpiY() / 10;

	m_iTickX = logicalDpiX() / 30;
	m_iTickY = logicalDpiY() / 30;

	m_foTitle.setBold(true);
	m_foTitle.setItalic(true);
	m_foTitle.setFamily("Times new roman");
	m_foTitle.setPointSize((logicalDpiX() + logicalDpiY()) / 12);

	draw();
}

void iABoneThicknessChartBar::draw()
{
	m_pImage->fill(m_cBrush);

	const int iHeight(height());
	const int iWidth(width());

	std::unique_ptr<QPainter> pPainter(new QPainter(m_pImage.get()));

	QVector<QString> vAxisYString;
	vAxisYString.push_back(QString("%1").arg(m_dAxisY1));
	vAxisYString.push_back(QString("%1").arg(m_dThickness1));
	vAxisYString.push_back(QString("%1").arg(m_dThicknessMean));
	vAxisYString.push_back(QString("%1").arg(m_dThickness2));
	vAxisYString.push_back(QString("%1").arg(m_dAxisY2));

	const QFontMetrics fomAxis(m_foAxis);
	const int iAxisH(fomAxis.height());

	int iAxisW(0);
	for (auto& pAxisYString : vAxisYString)
	{
		iAxisW = qMax(iAxisW, fomAxis.width(pAxisYString));
	}
	
	const QFontMetrics fomTitle(m_foTitle);
	const int iTitle (fomTitle.height());

	const int iRectH(iHeight - 2 * m_iMarginY - m_iTickY - iTitle - iAxisH);
	const int iRectW(iWidth - 2 * m_iMarginX - m_iTickX - iAxisW);
	const int iRectX(m_iMarginX + m_iTickX + iAxisW);
	const int iRectY(m_iMarginY + iTitle);

	pPainter->setPen(m_cPen1);

	pPainter->setFont(m_foTitle);
	pPainter->drawText(iRectX, 0, iRectW, iRectY, Qt::AlignCenter, m_sTitle);

	if (m_daThickness)
	{
		m_iAxisX1 = iRectX + m_iMarginX;
		m_iAxisX2 = iRectX + iRectW - m_iMarginX;

		const int iTickXY1(iRectY + iRectH);
		const int iTickXY2(iTickXY1 + m_iTickY);

		pPainter->drawLine(m_iAxisX1, iTickXY1, m_iAxisX1, iTickXY2);
		pPainter->drawLine(m_iAxisX2, iTickXY1, m_iAxisX2, iTickXY2);

		const int iFlagAxisX(Qt::AlignTop | Qt::AlignHCenter | Qt::TextDontClip);

		pPainter->setFont(m_foAxis);
		pPainter->drawText(m_iAxisX2, iTickXY2, 0, 0, iFlagAxisX, QString("%1").arg(m_dAxisX2));

		m_iAxisY1 = iRectY + iRectH - m_iMarginY;
		m_iAxisY2 = iRectY + m_iMarginY;

		const int iTickYX1(iRectX - m_iTickX);
		const int iTickYX2(iTickYX1 + m_iTickX);

		pPainter->drawLine(iTickYX1, m_iAxisY1, iTickYX2, m_iAxisY1);
		const int iThickness1(valueToScreenY(m_dThickness1));
		pPainter->drawLine(iTickYX1, iThickness1, iTickYX2, iThickness1);
		const int iThicknessMean(valueToScreenY(m_dThicknessMean));
		pPainter->drawLine(iTickYX1, iThicknessMean, iTickYX2, iThicknessMean);
		const int iThickness2(valueToScreenY(m_dThickness2));
		pPainter->drawLine(iTickYX1, iThickness2, iTickYX2, iThickness2);
		pPainter->drawLine(iTickYX1, m_iAxisY2, iTickYX2, m_iAxisY2);

		const int iFlagAxisY(Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip);

		pPainter->drawText(iTickYX1, iThickness1, 0, 0, iFlagAxisY, vAxisYString.at(1));
		pPainter->drawText(iTickYX1, iThicknessMean, 0, 0, iFlagAxisY, vAxisYString.at(2));
		pPainter->drawText(iTickYX1, iThickness2, 0, 0, iFlagAxisY, vAxisYString.at(3));

		pPainter->setPen(m_cPen2);
		pPainter->drawLine(iRectX, iThickness1, iRectX + iRectW, iThickness1);
		pPainter->drawLine(iRectX, iThicknessMean, iRectX + iRectW, iThicknessMean);
		pPainter->drawLine(iRectX, iThickness2, iRectX + iRectW, iThickness2);
	}

	pPainter->setBrush(Qt::NoBrush);
	pPainter->setPen(m_cPen1);
	pPainter->drawRect(iRectX, iRectY, iRectW, iRectH);

	drawData(pPainter.get());
}

void iABoneThicknessChartBar::drawData(QPainter* _pPainter)
{
	if (m_daThickness)
	{
		_pPainter->setPen(m_cPen1);

		const int iAxisXW(m_iAxisX2 - m_iAxisX1);

		const vtkIdType idThickness(m_daThickness->GetNumberOfTuples());

		if (iAxisXW > 2 * idThickness)
		{
			vtkIdType ii(1);
			for (vtkIdType i(0); i < idThickness; ++i, ++ii)
			{
				const int iRectX((vtkIdType)iAxisXW * i / idThickness);
				const int iRectW((vtkIdType)iAxisXW * ii / idThickness - iRectX);

				const int iRectY(valueToScreenY(m_daThickness->GetValue(i)));
				const int iRectH(m_iAxisY1 - iRectY);

				_pPainter->setBrush((m_idSelected != i) ? m_cBar1 : m_cBar2);
				_pPainter->drawRect(m_iAxisX1 + iRectX, iRectY, iRectW, iRectH);
			}
		}
		else
		{
			for (vtkIdType i(0); i < idThickness; ++i)
			{
				const int iRectX((vtkIdType)iAxisXW * i / idThickness);
				const int iRectY(valueToScreenY(m_daThickness->GetValue(i)));
				const int iRectH(m_iAxisY1 - iRectY);

				const int iX(m_iAxisX1 + iRectX);

				_pPainter->setPen((m_idSelected != i) ? m_cBar1 : m_cBar2);
				_pPainter->drawLine(iX, iRectY, iX, iRectY + iRectH);
			}
		}
	}
}

QSize iABoneThicknessChartBar::minimumSizeHint() const
{
	return QSize(2 * logicalDpiX(), 2 * logicalDpiY());
}

void iABoneThicknessChartBar::mousePressEvent(QMouseEvent* e)
{
	if (m_daThickness)
	{
		const vtkIdType idSelected (selected(e->x(), e->y()));

		if ((idSelected == m_pBoneThicknessTable->selected()) || (idSelected < 0))
		{
			setSelected(idSelected);

			m_pBoneThickness->setSelected(idSelected);
		}
		else
		{
			m_pBoneThicknessTable->setSelected(idSelected);
		}
	}

	QWidget::mousePressEvent(e);
}

void iABoneThicknessChartBar::paintEvent(QPaintEvent* e)
{
	QWidget::paintEvent(e);

	std::unique_ptr<QPainter> pPainter(new QPainter(this));
	pPainter->drawImage(0, 0, *m_pImage);
}

void iABoneThicknessChartBar::resizeEvent(QResizeEvent* e)
{
	const int iHeight(height());
	const int iWidth(width());

	if ((iHeight) && (iWidth))
	{
		*m_pImage = m_pImage->scaled(iWidth, iHeight);
		draw();
	}

	QWidget::resizeEvent(e);
}

double iABoneThicknessChartBar::screenToValueX(const int& _iValueX) const
{
	return m_dAxisX1 + (m_dAxisX2 - m_dAxisX1) * (double)(_iValueX - m_iAxisX1) / (double)(m_iAxisX2 - m_iAxisX1);
}

double iABoneThicknessChartBar::screenToValueY(const int& _iValueY) const
{
	return m_dAxisY1 + (m_dAxisY2 - m_dAxisY1) * (double)(_iValueY - m_iAxisY1) / (double)(m_iAxisY2 - m_iAxisY1);
}

int iABoneThicknessChartBar::selected(const int& _iX, const int& _iY) const
{
	const double dX(screenToValueX(_iX));
	const double dY(screenToValueY(_iY));

	const vtkIdType idSelected(floor(dX));

	if ((dY > m_dAxisY1) && (dY < m_daThickness->GetValue(idSelected)))
	{
		return idSelected;
	}
	else
	{
		return -1;
	}
}

void iABoneThicknessChartBar::set(iABoneThickness* _pBoneThickness, iABoneThicknessTable* _pBoneThicknessTable)
{
	m_pBoneThickness = _pBoneThickness;
	m_pBoneThicknessTable = _pBoneThicknessTable;
}

void iABoneThicknessChartBar::setData(vtkDoubleArray* _daThickness)
{
	m_daThickness = _daThickness;

	const vtkIdType idThickness(m_daThickness->GetNumberOfTuples());

	m_dThicknessMean = 0.0;

	for (vtkIdType i (0) ; i < idThickness; ++i)
	{
		m_dThicknessMean += m_daThickness->GetValue(i);
	}
	
	const vtkIdType dIdThickness((double)idThickness);
	m_dThicknessMean /= dIdThickness;
	
	double pRange[2];
	_daThickness->GetRange(pRange);

	m_dThickness1 = pRange[0];
	m_dThickness2 = pRange[1];

	m_dAxisX2 = dIdThickness;
	m_dAxisY2 = (m_dThickness2 < FloatTolerance) ? 1.0 : 1.1 * m_dThickness2;

	draw();
	update();
}

void iABoneThicknessChartBar::setSelected(const vtkIdType& _idSelected)
{
	m_idSelected = _idSelected;

	draw();
	update();
}

int iABoneThicknessChartBar::valueToScreenX(const double& _dValueX) const
{
	return m_iAxisX1 + (int) ((double) (m_iAxisX2 - m_iAxisX1) * (_dValueX - m_dAxisX1) / (m_dAxisX2 - m_dAxisX1));
}

int iABoneThicknessChartBar::valueToScreenY(const double& _dValueY) const
{
	return m_iAxisY1 + (int) ((double) (m_iAxisY2 - m_iAxisY1) * (_dValueY - m_dAxisY1) / (m_dAxisY2 - m_dAxisY1));
}

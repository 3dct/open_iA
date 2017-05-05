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
// iA

#include <QWidget>

#include <memory>

#include <vtkType.h>

class vtkDoubleArray;

class iABoneThickness;
class iABoneThicknessTable;

class iABoneThicknessChartBar : public QWidget
{
		Q_OBJECT

		#define FloatTolerance 0.00001

	public:
		explicit iABoneThicknessChartBar(QWidget* _pParent = nullptr);

		void set(iABoneThickness* _pBoneThickness, iABoneThicknessTable* _pBoneThicknessTable);

		void setData(vtkDoubleArray* _daThickness);

		void setSelected(const vtkIdType& _idSelected);

	private:
		const QString m_sTitle = "Bone thickness";

		std::unique_ptr<QImage> m_pImage = nullptr;

		double m_dAxisX1 = 0.0;
		double m_dAxisX2 = 0.0;
		double m_dAxisY1 = 0.0;
		double m_dAxisY2 = 0.0;
		double m_dThicknessMean = 0.0;
		double m_dThickness1 = 0.0;
		double m_dThickness2 = 0.0;

		iABoneThickness* m_pBoneThickness = nullptr;
		iABoneThicknessTable* m_pBoneThicknessTable = nullptr;

		int m_iMarginX = 0;
		int m_iMarginY = 0;
		int m_iAxisX1 = 0;
		int m_iAxisX2 = 0;
		int m_iAxisY1 = 0;
		int m_iAxisY2 = 0;
		int m_iTickX = 0;
		int m_iTickY = 0;

		QColor m_cBar1 = Qt::red;
		QColor m_cBar2 = Qt::green;
		QColor m_cBrush = Qt::white;
		QColor m_cPen1 = Qt::black;
		QColor m_cPen2 = Qt::gray;

		QFont m_foAxis;
		QFont m_foTitle;

		vtkDoubleArray* m_daThickness = nullptr;

		vtkIdType m_idSelected = -1;

		void draw();
		void drawData(QPainter* _pPainter);

		double screenToValueX(const int& _iValueX) const;
		double screenToValueY(const int& _iValueY) const;

		int selected(const int& _iX, const int& _iY) const;

		int valueToScreenX(const double& _dValueX) const;
		int valueToScreenY(const double& _dValueY) const;

protected:
		virtual QSize minimumSizeHint() const override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void paintEvent(QPaintEvent* e) override;
		virtual void resizeEvent(QResizeEvent* e) override;
};

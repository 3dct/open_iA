// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkType.h>    // for vtkIdType

#include <QWidget>

#include <memory>

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

	vtkIdType selected(const int& _iX, const int& _iY) const;

	int valueToScreenX(const double& _dValueX) const;
	int valueToScreenY(const double& _dValueY) const;

protected:
	QSize minimumSizeHint() const override;
	void mousePressEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
};

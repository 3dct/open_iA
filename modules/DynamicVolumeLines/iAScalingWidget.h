// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <qcustomplot.h>

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include <vtkSmartPointer.h>

class vtkLookupTable;

class iAScalingWidget : public QOpenGLWidget, public QOpenGLFunctions
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

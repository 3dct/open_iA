// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QVector>

class vtkActorCollection;
class vtkDoubleArray;
class vtkIdList;
class vtkLineSource;
class vtkPointLocator;
class vtkPoints;
class vtkPolyData;

class iARenderer;

class iABoneThicknessChartBar;
class iABoneThicknessTable;

class iABoneThickness
{
	#define FloatTolerance 0.00001

public:
	iABoneThickness();
	double axisXMax() const;
	double axisXMin() const;
	double axisYMax() const;
	double axisYMin() const;
	double axisZMax() const;
	double axisZMin() const;

	double meanThickness() const;
	double stdThickness() const;
	double meanSurfaceDistance() const;
	double stdSurfaceDistance() const;

	void calculate();

	vtkDoubleArray* thickness();

	void open(const QString& _sFilename);

	double rangeMax() const;
	double rangeMin() const;
	double rangeX() const;
	double rangeY() const;
	double rangeZ() const;

	void save(const QString& _sFilename) const;

	vtkIdType selected() const;

	void set(iARenderer* _iARenderer, vtkPolyData* _pPolyData, iABoneThicknessChartBar* _pBoneThicknessChartBar, iABoneThicknessTable* _pBoneThicknessTable);
	void setChart(iABoneThicknessChartBar* _pBoneThicknessChartBar);
	void setShowThickness(const bool& _bShowThickness);
	void setShowThicknessLines(const bool& _bShowThicknessLines);
	void setSelected(const vtkIdType& _idSelected);
	void setSphereRadius(const double& _dSphereRadius);
	void setTable(iABoneThicknessTable* _pBoneThicknessTable);
	void setThicknessMaximum(const double& _dThicknessMaximum);
	void setSurfaceDistanceMaximum(const double& _dSurfaceDistanceMaximum);
	void setTransparency(const bool& _bTransparency);
	void setWindow();
	void setWindowSpheres();
	void setWindowThicknessLines();

	bool showThickness() const;

	double sphereOpacity() const;
	double sphereRadius() const;
	double surfaceOpacity() const;
	double thicknessMaximum() const;
	double surfaceDistanceMaximum() const;

private:
	double m_pColorNormal[3];
	double m_pColorSelected[3];
	double m_pColorMark[3];

	bool m_bShowThickness = true;
	bool m_bShowThicknessLines = true;

	double m_dRangeMax = 1.0;
	double m_dRangeMin = 0.0;
	double m_dSphereOpacity = 1.0;
	double m_dSphereRadius = 0.5;
	double m_dSurfaceOpacity = 1.0;
	double m_dThicknessMaximum = 0.0;
	double m_dSurfaceDistanceMaximum = 0.0;
	double m_dThicknessMean = 0.0;
	double m_dThicknessSTD = 0.0;
	double m_dSurfaceDistanceMean = 0.0;
	double m_dSurfaceDistanceSTD = 0.0;

	double m_pBound[6];
	double m_pRange[3];

	vtkIdType m_idSelected = -1;

	vtkPolyData* m_pPolyData = nullptr;

	vtkSmartPointer<vtkDoubleArray> m_daDistance;
	vtkSmartPointer<vtkDoubleArray> m_daThickness;

	vtkSmartPointer<vtkPoints> m_pPoints;
	QVector<vtkSmartPointer<vtkLineSource>> m_pThLines;
	QVector<vtkSmartPointer<vtkLineSource>> m_pDaLines;

	vtkSmartPointer<vtkActorCollection> m_pSpheres;
	vtkSmartPointer<vtkActorCollection> m_pThicknessLines;
	vtkSmartPointer<vtkActorCollection> m_pDistanceLines;

	iARenderer* m_iARenderer = nullptr;

	bool getNormalFromPCA(vtkIdList* _pIdList, double* _pNormal);
	void getNormalInPoint(vtkPointLocator* _pPointLocator, double* _pPoint, double* _pNormal);
	void getSphereColor(const vtkIdType& _id, const double& _dRadius, double* pColor);

	void setSphereOpacity(const double& _dSphereOpacity);
	void setSurfaceOpacity(const double& _dSurfaceOpacity);

	void setResults(const int& _iPoint, const double& _dThickness, const double& _dSurfaceDistance);
	void setTranslucent();
};

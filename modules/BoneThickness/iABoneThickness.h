/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
	void setTransparency(const bool& _bTransparency);
	void setWindow();
	void setWindowSpheres();
	void setWindowThicknessLines();

	bool showThickness() const;

	double sphereOpacity() const;
	double sphereRadius() const;
	double surfaceOpacity() const;
	double thicknessMaximum() const;

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

	double m_pBound[6];
	double m_pRange[3];

	vtkIdType m_idSelected = -1;

	vtkPolyData* m_pPolyData = nullptr;

	vtkSmartPointer<vtkDoubleArray> m_daDistance;
	vtkSmartPointer<vtkDoubleArray> m_daThickness;

	vtkSmartPointer<vtkPoints> m_pPoints;
	QVector<vtkSmartPointer<vtkLineSource>> m_pLines;

	vtkSmartPointer<vtkActorCollection> m_pSpheres;
	vtkSmartPointer<vtkActorCollection> m_pThicknessLines;

	iARenderer* m_iARenderer = nullptr;

	bool getNormalFromPCA(vtkIdList* _pIdList, double* _pNormal);
	void getNormalInPoint(vtkPointLocator* _pPointLocator, double* _pPoint, double* _pNormal);
	void getSphereColor(const vtkIdType& _id, const double& _dRadius, double* pColor);

	void setSphereOpacity(const double& _dSphereOpacity);
	void setSurfaceOpacity(const double& _dSurfaceOpacity);

	void setThickness(const int& _iPoint, const double& _dThickness);
	void setTranslucent();
};

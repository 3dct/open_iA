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

#pragma once
// iA

#include <vtkObject.h>

#include <QVector>

#include <vtkActorCollection.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <iARenderer.h>

#include "iABoneThicknessTable.h"

class iABoneThickness : public vtkObject
{
	public:
		enum EMethod { eCentroid, ePCA, ePlaneX, ePlaneY, ePlaneZ };

	public:
		static iABoneThickness* New();
		vtkTypeMacro(iABoneThickness, vtkObject);

		double axisXMax() const;
		double axisXMin() const;
		double axisYMax() const;
		double axisYMin() const;
		double axisZMax() const;
		double axisZMin() const;

		void calculate();

		double calculateSphereRadius() const;
		double calculateThicknessMaximum() const;
		
		void deSelect();

		EMethod method() const;

		void open(const QString& _sFilename);

		double rangeMax() const;
		double rangeMin() const;
		double rangeX() const;
		double rangeY() const;
		double rangeZ() const;

		void save(const QString& _sFilename) const;

		void set(iARenderer* _iARenderer, vtkPolyData* _pPolyData, iABoneThicknessTable* _pBoneThicknessTable);
		void setCalculateSphereRadius(const double& _dCalculateSphereRadius);
		void setCalculateThicknessMaximum(const double& _dCalculateThicknessMaximum);
		void setMethod(const EMethod& _eMethod);
		void setShowThickness(const bool& _bShowThickness);
		void setShowThicknessLines(const bool& _bShowThicknessLines);
		void setSphereOpacity(const double& _dSphereOpacity);
		void setSphereSelected(const vtkIdType& _idSphereSelected, iABoneThicknessTable* _pBoneThicknessTable = nullptr);
		void setSurfaceOpacity(const double& _dSurfaceOpacity);
		void setTable(iABoneThicknessTable* _iABoneThicknessTable);
		void setTransparency(const bool& _bTransparency);
		void setWindow();
		void setWindowSpheres();
		void setWindowThicknessLines();

		bool showThickness() const;

		double sphereOpacity() const;
		double surfaceOpacity() const;

	private:
		bool m_bShowThickness = true;
		bool m_bShowThicknessLines = true;

		double m_dCalculateSphereRadius = 0.5;
		double m_dCalculateThicknessMaximum = 1.0;
		double m_dRangeMax = 1.0;
		double m_dRangeMin = 0.0;
		double m_dSphereOpacity = 1.0;
		double m_dSurfaceOpacity = 1.0;

		double m_pBound[6] = { 0.0 , 0.0 , 0.0 , 0.0 , 0.0 , 0.0 };
		double m_pRange[3] = { 0.0 , 0.0 , 0.0 };

		vtkIdType m_idSphereSelected = -1;

		vtkPolyData* m_pPolyData = nullptr;

		EMethod m_eMethod = ePCA;

		QVector<double> m_vCalculateDistance;
		QVector<double> m_vCalculateThickness;

		vtkSmartPointer<vtkPoints> m_pPoints = nullptr;
		QVector<vtkSmartPointer<vtkLineSource>> m_pLines;

		vtkSmartPointer<vtkActorCollection> m_pSpheres = nullptr;
		vtkSmartPointer<vtkActorCollection> m_pThicknessLines = nullptr;

		iARenderer* m_iARenderer = nullptr;

		void addNormalsInPoint(vtkPoints* _pPointNormals);
		void findPoints(QVector<vtkSmartPointer<vtkPoints>>& _vPoints);
		bool getCenterFromPoints(vtkPoints* _pPoints, double* _pCenter);
		void getClosestPoints(vtkIdList* _idListClosest);
		void getConnectedPoints(const vtkIdType& _idPoint, vtkPoints* _pPoints);
		bool getNormalFromPCA(vtkPoints* _pPoints, double* _pNormal);
		bool getNormalFromPoints(vtkPoints* _pPoints, double* _pNormal);

		void setCalculateThickness(const int& _iPoint, const double& _dThickness);
		void setTranslucent();
};

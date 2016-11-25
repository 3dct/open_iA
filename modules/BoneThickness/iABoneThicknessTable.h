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
#include <QTableView>

#include <QString>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

class iARenderer;

class iABoneThicknessTable : public QTableView
{
	Q_OBJECT

  public:
	explicit iABoneThicknessTable(iARenderer* _iARenderer, QWidget* _pParent = nullptr);

	QVector<double>* distance();

	QVector<vtkSmartPointer<vtkLineSource>>* lines();

	void open(const QString& _sFilename);

	vtkSmartPointer<vtkPoints> point();

	void setSphereRadius(const double& _dSphereRadius);
	void setTable();
	void setWindow();

	double sphereRadius() const;

	QVector<double>* thickness();

  private:
	double m_dSphereRadius = 0.5;

	vtkIdType m_idSphereSelected = -1;

	QVector<double> m_vDistance;
	QVector<double> m_vThickness;

	vtkActor* m_pSurface = nullptr;

	vtkSmartPointer<vtkPoints> m_pPoints = nullptr;
	QVector<vtkSmartPointer<vtkLineSource>> m_pLines;

	vtkSmartPointer<vtkActorCollection> m_pSpheres = nullptr;
	vtkSmartPointer<vtkActorCollection> m_pThicknessLines = nullptr;

	iARenderer* m_iARenderer = nullptr;

protected:
	virtual void selectionChanged(const QItemSelection& _itemSelected, const QItemSelection& _itemDeselected) override;
};

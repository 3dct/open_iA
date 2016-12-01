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

#include <QMouseEvent>
#include <QString>

#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPropPicker.h>
#include <vtkRenderWindowInteractor.h>
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

	void save(const QString& _sFilename);

	void setShowThickness(const bool& _bShowThickness);
	void setShowThicknessLines(const bool& _bShowThicknessLines);
	void setSphereOpacity(const double& _dSphereOpacity);
	void setSphereRadius(const double& _dSphereRadius);
	void setSphereSelected(const vtkIdType& _idSphereSelected);
	void setSurfaceOpacity(const double& _dSurfaceOpacity);
	void setTable();
	void setTransparency(const bool& _bTransparency);
	void setThickness(const int& _iPoint, const double& _dThickness);
	void setThicknessMaximum(const double& _dThicknessMaximum);

	void setWindow();
	void setWindowSpheres();
	void setWindowThicknessLines();

	bool showThickness() const;
		
	double sphereOpacity() const;
	double sphereRadius() const;

	double surfaceOpacity() const;

	double thicknessMaximum() const;

  private:
	  bool m_bShowThickness = true;
	  bool m_bShowThicknessLines = false;

	  double m_dSphereOpacity = 1.0;
	  double m_dSphereRadius = 0.5;
	  double m_dSurfaceOpacity = 1.0;
	  double m_dThicknessMaximum = 0.0;

	vtkIdType m_idSphereSelected = -1;

	QVector<double> m_vDistance;
	QVector<double> m_vThickness;

	vtkSmartPointer<vtkPoints> m_pPoints = nullptr;
	QVector<vtkSmartPointer<vtkLineSource>> m_pLines;

	vtkSmartPointer<vtkActorCollection> m_pSpheres = nullptr;
	vtkSmartPointer<vtkActorCollection> m_pThicknessLines = nullptr;

	iARenderer* m_iARenderer = nullptr;

	void setTranslucent();

    protected:
	    virtual void mousePressEvent(QMouseEvent* e) override;
	    virtual void selectionChanged(const QItemSelection& _itemSelected, const QItemSelection& _itemDeselected) override;
};

class iABoneThicknessMouseInteractor : public vtkInteractorStyleTrackballCamera
{
public:
	static iABoneThicknessMouseInteractor* New();
	vtkTypeMacro(iABoneThicknessMouseInteractor, vtkInteractorStyleTrackballCamera);

	iABoneThicknessMouseInteractor()
	{

	}

	~iABoneThicknessMouseInteractor()
	{

	}

	void setSphereCollection(vtkActorCollection* _pSpheres)
	{
		m_pSpheres = _pSpheres;
	}

	void setBoneThicknessTable(iABoneThicknessTable* _pBoneThicknessTable)
	{
		m_pBoneThicknessTable = _pBoneThicknessTable;
	}

	virtual void OnLeftButtonDown() override
	{
		const int* pClickPos(GetInteractor()->GetEventPosition());

		vtkSmartPointer<vtkPropPicker>  pPicker(vtkSmartPointer<vtkPropPicker>::New());
		pPicker->Pick(pClickPos[0], pClickPos[1], 0, GetDefaultRenderer());

		vtkActor* pPickedActor(pPicker->GetActor());

		if (pPickedActor)
		{
			const vtkIdType idSpheresSize(m_pSpheres->GetNumberOfItems());

			m_pSpheres->InitTraversal();

			for (vtkIdType i(0); i < idSpheresSize; ++i)
			{
				if (pPickedActor == m_pSpheres->GetNextActor())
				{
					m_pBoneThicknessTable->setSphereSelected(i);
					break;
				}
			}

		}

		vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
	}

private:
	iABoneThicknessTable* m_pBoneThicknessTable = nullptr;
	vtkActorCollection* m_pSpheres = nullptr;
};

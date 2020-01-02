/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include <QMap>
#include <QObject>

#include <vector>

class vtkPolyData;
class vtkRenderWindow;
class vtkTextActor;
class vtkUnsignedCharArray;

class iASelectionProvider
{
public:
	virtual std::vector<std::vector<size_t> > & selection() =0;
protected:
	~iASelectionProvider();
};


class iASelectionInteractorStyle : public QObject, public vtkInteractorStyleTrackballCamera
{
	Q_OBJECT
public:
	enum InteractionMode
	{
		imNavigate,
		imSelect
	};
	enum SelectionMode
	{
		smDrag,
		smClick
	};
	iASelectionInteractorStyle();
	static iASelectionInteractorStyle* New();
	vtkTypeMacro(iASelectionInteractorStyle, vtkInteractorStyleTrackballCamera);

	void OnChar() override;
	void OnLeftButtonDown() override;
	void OnMouseMove() override;
	void OnLeftButtonUp() override;

	void setSelectionProvider(iASelectionProvider * selectionProvider);
	void addInput(size_t resultID, vtkSmartPointer<vtkPolyData> points, vtkSmartPointer<vtkActor> actor);
	void removeInput(size_t resultID);
	void assignToRenderWindow(vtkSmartPointer<vtkRenderWindow> renWin);
	void setSelectionMode(SelectionMode mode);
	void setRenderer(vtkRenderer* renderer);
signals:
	void selectionChanged();
private:
	void pick();
	void redrawRubberBand();

	QMap<int, std::pair<vtkSmartPointer<vtkPolyData>, vtkSmartPointer<vtkActor> > > m_input;
	iASelectionProvider * m_selectionProvider;
	vtkSmartPointer<vtkTextActor> m_showModeActor;
	vtkSmartPointer<vtkRenderWindow> m_renWin;
	InteractionMode m_interactionMode;
	SelectionMode m_selectionMode;
	vtkRenderer* m_cellRenderer;

	int m_startPos[2];
	int m_endPos[2];
	bool m_moving;
	vtkSmartPointer<vtkUnsignedCharArray> m_pixelArray;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include <QMap>
#include <QObject>

#include <vector>

class vtkActor2D;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkRenderWindow;
class vtkTextActor;

class iASelectionProvider
{
public:
	virtual std::vector<std::vector<size_t> > & selection() =0;
protected:
	virtual ~iASelectionProvider();
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
	//! disable default constructor.
	iASelectionInteractorStyle();

	//! @{ disable copying.
	void operator=(const iASelectionInteractorStyle&) = delete;
	iASelectionInteractorStyle(const iASelectionInteractorStyle&) = delete;
	//! @}

	void pick();
	void updateModeLabel();
	void updateSelectionRect();

	QMap<size_t, std::pair<vtkSmartPointer<vtkPolyData>, vtkSmartPointer<vtkActor> > > m_input;
	iASelectionProvider * m_selectionProvider;
	vtkSmartPointer<vtkTextActor> m_showModeActor;
	vtkSmartPointer<vtkRenderWindow> m_renWin;

	vtkSmartPointer<vtkPolyData> m_selRectPolyData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_selRectMapper;
	vtkSmartPointer<vtkActor2D> m_selRectActor;

	InteractionMode m_interactionMode;
	SelectionMode m_selectionMode;
	vtkRenderer* m_cellRenderer;

	int m_startPos[2];
	int m_endPos[2];
	bool m_moving;
};

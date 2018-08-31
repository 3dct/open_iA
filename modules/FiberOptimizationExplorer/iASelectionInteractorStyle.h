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

#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkSmartPointer.h>

#include <QMap>
#include <QObject>

#include <vector>

class vtkPolyData;
class vtkRenderWindow;

class iASelectionProvider
{
public:
	virtual std::vector<std::vector<size_t> > & selection() =0;
};


class iASelectionInteractorStyle : public QObject, public vtkInteractorStyleRubberBandPick
{
	Q_OBJECT
public:
	iASelectionInteractorStyle();
	static iASelectionInteractorStyle* New();
	vtkTypeMacro(iASelectionInteractorStyle, vtkInteractorStyleRubberBandPick);
	void setSelectionProvider(iASelectionProvider * selectionProvider);
	void Pick() override;
	void addInput(size_t resultID, vtkSmartPointer<vtkPolyData> points);
	void removeInput(size_t resultID);
	void assignToRenderWindow(vtkSmartPointer<vtkRenderWindow> renWin);
signals:
	void selectionChanged();
private:
	QMap<int, vtkSmartPointer<vtkPolyData> > m_resultPoints;
	iASelectionProvider * m_selectionProvider;

};

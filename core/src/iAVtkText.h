/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <vtkObject.h>
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkTextMapper;
class vtkActor2D;

//! Wraps the vtk classes required to display a text.
class iAVtkText : public vtkObject {

public:
	static iAVtkText* New();

	//! Set the text to a fixed position in the scene.
	void setPosition(double x, double y);

	//! Add the text to the scene.
	void addToScene(vtkRenderer* renderer);

	//! Hide or show the text.
	void show(bool show);

	//! determine whether text is currently shown
	bool isShown() const;

	//! Set the text.
	void setText(const char* text);

	//! Set the size of the text
	void setFontSize(int fontSize);

private:
	vtkSmartPointer<vtkTextMapper> m_textMapper;
	vtkSmartPointer<vtkActor2D> m_actor;
private:
	iAVtkText(const iAVtkText&) = delete;
	void operator=(const iAVtkText&) = delete;
protected:
	iAVtkText();
};

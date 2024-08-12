// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkObject.h>
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkTextMapper;
class vtkActor2D;

//! Wraps the vtk classes required to display a text at a fixed position of the screen (in display coordinates).
class iAVtkText : public vtkObject {

public:
	static iAVtkText* New();

	//! Set the text to a fixed position in the scene (display coordinates).
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

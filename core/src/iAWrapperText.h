/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "vtkObject.h"

class vtkRenderer;
class vtkTextMapper;
class vtkTextProperty;
class vtkActor2D;


class iAWrapperText : public vtkObject {

	public:
		static iAWrapperText *New();

		// Just pass the current window size and the text will center itself.
		void SetParentWindowSize(int cxWin, int cyWin);

		// Add the to the scene.
		void AddToScene(vtkRenderer *pParentRenderer);

		// Hide or show the text.
		void Show(int bShow);

		// Set the text.
		void SetText(const char *pszText);

		// Set the position of the text.
		void SetPosition(int nPosition);

		// Get a pointer to the internal text mapper.
		vtkTextMapper* GetTextMapper() { return m_TextMapper; }
		vtkActor2D* GetActor() { return m_Actor; }

		enum {
			POS_CENTER,
			POS_UPPER_LEFT,
			POS_UPPER_RIGHT,
			POS_LOWER_RIGHT,
			POS_LOWER_LEFT
		};

	private:

		// Combine a text mapper with an actor.
		vtkTextMapper *m_TextMapper;
		vtkActor2D    *m_Actor;

		int m_cxWin, m_cyWin;
		int m_nPositionType;

	private:
		iAWrapperText(const iAWrapperText&); // Not implemented
		void operator=(const iAWrapperText&); // Not implemented

	protected:
		iAWrapperText();
		~iAWrapperText();
};

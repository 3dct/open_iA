/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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

#include "open_iA_Core_export.h"

#include <QVector>

//forward declaration
class iARenderer;
class vtkObject;
class vtkCamera;

// the class is singleton
class open_iA_Core_API iARendererManager
{
public:
// 	static iARendererManager&	getInstance () {
// 		static iARendererManager instance;
// 		return instance;
// 	}
				iARendererManager();
	void		addToBundle(iARenderer* renderer);
	bool		removeFromBundle(iARenderer* renderer);
	void		removeAll();

private:
// 							iARendererManager();
// 							iARendererManager(const iARendererManager&);
// 	iARendererManager&		operator=(const iARendererManager&);

	void					redrawOtherRenderers(vtkObject* caller,
												 long unsigned int eventId,
												 void* callData);

	QVector<iARenderer*>	m_renderers;
	int						m_isRedrawn;
	vtkCamera*				m_commonCamera;
};

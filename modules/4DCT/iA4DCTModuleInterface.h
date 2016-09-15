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
 
#ifndef IA4DCTMODULEINTERFACE_H
#define IA4DCTMODULEINTERFACE_H
// iA
#include "iAFiberCharacteristics.h"
#include "iAModuleInterface.h"
#include "iAThresholding.h"
#include "iAvec3.h"

class vtkImageData;
class dlg_densityMap;
class iA4DCTMainWin;

class iA4DCTModuleInterface : public iAModuleInterface
{
	Q_OBJECT

public:
						iA4DCTModuleInterface();
						~iA4DCTModuleInterface();
	void				Initialize();

private slots:
	void				openProj();
	void				newProj();
	void				saveProj();
	//void				enableDensityMap();

	virtual	iAModuleAttachmentToChild*
						CreateAttachment(MainWindow* mainWnd, iAChildData childData);

private:
	//float				distBetweenFiberAndPoint(FiberCharacteristics* f, iAVec3 p);

	vtkImageData*		m_fracture;
	int					m_labeImageChildInd;
	dlg_densityMap*		m_densityMap;
};

#endif // IA4DCTMODULEINTERFACE_H

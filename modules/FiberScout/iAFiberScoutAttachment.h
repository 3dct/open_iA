/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAModuleAttachmentToChild.h"
#include "iABlobManager.h"

#include <QList>

class dlg_FiberScout;
class iABlobCluster;

class vtkOpenGLRenderer;
class vtkTable;

class iAFiberScoutAttachment : public iAModuleAttachmentToChild
{
	Q_OBJECT
public:
	iAFiberScoutAttachment(MainWindow* mainWnd, iAChildData childData);
	~iAFiberScoutAttachment();
	void init(int filterID, vtkSmartPointer<vtkTable> csvtbl);
	void enableBlobVisualization();
	void disableBlobVisualization();
	bool FiberScout_Options(int idx);
private:
	bool blobVisEnabled;
	iABlobManager m_blobManager;
	QList<iABlobCluster*> blobList;
	vtkSmartPointer<vtkOpenGLRenderer> blobRen;
	dlg_FiberScout * imgFS;
private slots:
	void rendererSetCamera();
};
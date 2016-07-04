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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_MODALITY_EXPLORER_ATTACHMENT_H
#define IA_MODALITY_EXPLORER_ATTACHMENT_H

#include "iAModuleAttachmentToChild.h"

#include <QSharedPointer>
#include <vtkSmartPointer.h>

class dlg_modalities;
class iAModalityList;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class iAModalityExplorerAttachment : public iAModuleAttachmentToChild
{
	Q_OBJECT
public:
	static iAModalityExplorerAttachment* create(MainWindow * mainWnd, iAChildData childData);
	void SetModalities(QSharedPointer<iAModalityList> modList);
	dlg_modalities* GetModalitiesDlg();
	void ChangeImage(vtkSmartPointer<vtkImageData> img, std::string const & caption);
	int GetCurrentModality() const;
	void SetCurrentModality(int modality);
	bool LoadModalities();
private slots:
	void ChangeImage(vtkSmartPointer<vtkImageData> img);
	void MagicLensToggled(bool isOn);
	void RenderSettingsChanged();
	void preferencesChanged();
	void ChangeModality(int);
	void ChangeMagicLensOpacity(int);
private:
	iAModalityExplorerAttachment(MainWindow * mainWnd, iAChildData childData);
};

#endif // IA_MODALITY_EXPLORER_ATTACHMENT_H

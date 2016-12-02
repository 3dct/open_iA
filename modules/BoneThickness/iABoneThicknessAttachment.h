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
#pragma once

#include "iAModuleAttachmentToChild.h"

#include "iABoneThicknessTable.h"

//class dlg_BoneThickness;

class iABoneThicknessAttachment : public iAModuleAttachmentToChild
{
	Q_OBJECT

		enum EMethod {eCentroid, ePCA, ePlaneX, ePlaneY, ePlaneZ};

	public:
		iABoneThicknessAttachment(MainWindow* _pMainWnd, iAChildData _iaChildData);

	private:
		EMethod m_eMethod = ePCA;

		double m_pBound[6] = { 0.0 , 0.0 , 0.0 , 0.0 , 0.0 , 0.0 };
		double m_pRange[3] = { 0.0 , 0.0 , 0.0 };

		iABoneThicknessTable* m_pBoneThicknessTable = nullptr;

		void addNormalsInPoint(vtkPoints* _pPointNormals);
		void calculate();
		bool getCenterFromPoints(vtkPoints* _pPoints, double* _pCenter);
		bool getNormalFromPCA(vtkPoints* _pPoints, double* _pNormal);
		bool getNormalFromPoints(vtkPoints* _pPoints, double* _pNormal);

	private slots:
		void slotComboBoxMethod(const int& _iIndex);
	    void slotDoubleSpinBoxSphereRadius(const double&);
		void slotDoubleSpinBoxThicknessMaximum(const double&);
		void slotPushButtonOpen();
		void slotPushButtonSave();
		void slotCheckBoxShowLines(const bool& _bChecked);
		void slotCheckBoxShowThickness(const bool& _bChecked);
		void slotCheckBoxTransparency(const bool& _bChecked);
};

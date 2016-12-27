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

#include <QTableWidget>

class QDropEvent;
class vtkImageData;

class iAFoamCharacterizationTable : public QTableWidget
{
		Q_OBJECT

	public:
		explicit iAFoamCharacterizationTable(vtkImageData* _pImageData, QWidget* _pParent = nullptr);

		void addBinarization();
		void addFilter();
		void addWatershed();

		void clear();

		void execute();

		void open(const QString& _sFilename);
		void save(const QString& _sFilename);

	private:
		int m_iRowDrag = -1;
		int m_iRowDrop = -1;

		int m_iCountBinarization = 0;
		int m_iCountFilter = 0;
		int m_iCountWatershed = 0;

		vtkImageData* m_pImageData = nullptr;

	protected:
		virtual void dropEvent(QDropEvent* e) override;
		virtual void keyPressEvent(QKeyEvent* e) override;
		virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void resizeEvent(QResizeEvent*) override;
};

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

#include <QObject>
#include <QTableWidgetItem>

class QFile;

class vtkImageData;

class iAFoamCharacterizationTable;

class iAFoamCharacterizationItem : public QObject, public QTableWidgetItem
{
		Q_OBJECT

	public:
		enum EItemType { itBinarization, itDistanceTransform, itFilter, itWatershed};

	public:
		explicit iAFoamCharacterizationItem ( iAFoamCharacterizationTable* _pTable
										    , vtkImageData* _pImageData, const EItemType& _eItemType
											);

		explicit iAFoamCharacterizationItem(iAFoamCharacterizationItem* _pItem);
		virtual ~iAFoamCharacterizationItem();

		double executeTime() const;
		bool executing() const;

		QString executeTimeString() const;

		vtkImageData* imageData() const;

		QIcon itemButtonIcon() const;

		bool itemEnabled() const;

		EItemType itemType() const;
		QString itemTypeStr() const;

		bool modified() const;

		QString name() const;

		int progress() const;

		void reset();
		void setItemEnabled(const bool& _bEnabled);
		void setModified(const bool& _bModified);
		void setName(const QString& _sName);

		iAFoamCharacterizationTable* table();

		virtual void dialog() = 0;
		virtual void execute() = 0;
		virtual void open(QFile* _pFileOpen) = 0;
		virtual void save(QFile* _pFileSave) = 0;

	private:
		bool m_bExecuting = false;

		QColor m_cItemIcon = Qt::black;

		int m_iProgress = 0;

		void setItemIcon();
		void setItemIconColor();

	protected:
		bool m_bItemEnabled = true;
		bool m_bModified = false;

		double m_dExecuteTime = 0.0;

		iAFoamCharacterizationTable* m_pTable = nullptr;

		EItemType m_eItemType = itFilter;

		QString m_sName;

		vtkImageData* m_pImageData = nullptr;

		QString fileRead(QFile* _pFileOpen);
		void fileWrite(QFile* _pFileSave, const QString& _sText);

		void setExecuting(const bool& _bExecuting);
		void setProgress(const unsigned int& _uiProgress);

		virtual void setItemText();

	protected slots:
		void slotObserver(const int&);
};

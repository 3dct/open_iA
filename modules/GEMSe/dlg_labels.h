/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "ui_labels.h"

#include "iALabelInfo.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QList>

class iAColorTheme;
class MdiChild;

class QStandardItem;
class QStandardItemModel;

class iAvtkImageData;
class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;

class iALabelOverlayThread;

class iAImageCoordinate;

typedef iAQTtoUIConnector<QDockWidget, Ui_labels> dlg_labelUI;

class dlg_labels : public dlg_labelUI, public iALabelInfo
{
	Q_OBJECT
public:
	dlg_labels(MdiChild* mdiChild, iAColorTheme const * theme);
	int curLabelRow() const;
	int seedCount(int labelIdx) const;
	bool load(QString const & filename);
	bool store(QString const & filename, bool extendedFormat);
	void setColorTheme(iAColorTheme const *);
	//! @{ implementing iALabelInfo methods
	int count() const override;
	QString name(int idx) const override;
	QColor color(int idx) const override;
	//! @}
public slots:
	void RendererClicked(int, int, int);
	void SlicerClicked(int, int, int);
	void SlicerRightClicked(int, int, int);
	void Add();
	void Remove();
	void Store();
	void Load();
	void storeImage();
	void Sample();
	void Clear();
	QString const & fileName();
private:
	void addSeed(int, int, int);
	void removeSeed(QStandardItem* item, int x, int y, int z);
	void addSeedItem(int label, int x, int y, int z);
	int addLabelItem(QString const & labelText);
	void reInitChannelTF();
	void updateChannel();

	QStandardItemModel* m_itemModel;
	iAColorTheme const * m_colorTheme;
	int m_maxColor;
	QString m_fileName;

	// for label overlay:
	vtkSmartPointer<iAvtkImageData> m_labelOverlayImg;
	vtkSmartPointer<vtkLookupTable> m_labelOverlayLUT;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOverlayOTF;
	MdiChild* m_mdiChild;
	bool m_newOverlay;
	uint m_labelChannelID;
};

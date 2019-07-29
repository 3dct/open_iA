/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "Labelling_export.h"

#include "ui_labels.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QList>

enum iASlicerMode;
class iAColorTheme;
class MdiChild;

class QStandardItem;
class QStandardItemModel;

class iAvtkImageData;
class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;

struct iAImageCoordinate;

typedef iAQTtoUIConnector<QDockWidget, Ui_labels> dlg_labelUI;

class Labelling_API dlg_labels : public dlg_labelUI
{
	Q_OBJECT
public:
	dlg_labels(MdiChild* mdiChild);
	int curLabelRow() const;
	int seedCount(int labelIdx) const;
	bool load(QString const & filename);
	bool store(QString const & filename, bool extendedFormat);
	int count() const;
	QString name(int idx) const;
	QColor color(int idx) const;

	// TEMPORARY
	QStandardItemModel* m_itemModel; // TODO: make private

public slots:
	void rendererClicked(int, int, int, iASlicerMode);
	void slicerClicked(int, int, int, iASlicerMode);
	void slicerDragged(int, int, int, iASlicerMode);
	void slicerRightClicked(int, int, int, iASlicerMode);
	void add();
	void remove();
	void storeLabels();
	void loadLabels();
	void storeImage();
	void sample();
	void clear();
	void colorThemeChanged(QString const & newThemeName);
	QString const & fileName();
	void opacityChanged(int newValue);
private:
	void addSingleSeed(int, int, int, int);
	void addSeed(int, int, int, iASlicerMode);
	void removeSeed(QStandardItem* item, int x, int y, int z);
	void addSeedItem(int label, int x, int y, int z);
	int addLabelItem(QString const & labelText);
	void reInitChannelTF();
	void recolorItems();
	void updateChannel();

	iAColorTheme const * m_colorTheme;
	int m_maxColor;
	QString m_fileName;

	// for label overlay:
	vtkSmartPointer<iAvtkImageData> m_labelOverlayImg;
	vtkSmartPointer<vtkLookupTable> m_labelColorTF;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOpacityTF;
	MdiChild* m_mdiChild;
	bool m_newOverlay;
	uint m_labelChannelID;
};

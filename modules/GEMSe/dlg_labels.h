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

#include "ui_labels.h"
#include <iAQTtoUIConnector.h>
typedef iAQTtoUIConnector<QDockWidget, Ui_labels> dlg_labelUI;

#include "iALabelInfo.h"
#include <vtkSmartPointer.h>

#include "SVMImageFilter.h" // for seeds input type

#include <QList>

class iAColorTheme;
class MdiChild;

class QStandardItemModel;

class vtkImageData;
class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;

class LabelOverlayThread;

struct iAImageCoordinate;

class dlg_labels : public dlg_labelUI, public iALabelInfo
{
	Q_OBJECT
public:
	dlg_labels(MdiChild* mdiChild, iAColorTheme const * theme);
	int GetCurLabelRow() const;
	int GetSeedCount(int labelIdx) const;
	QList<iAImageCoordinate> GetSeeds(int labelIdx) const;
	bool Load(QString const & filename);
	bool Store(QString const & filename);
	void SetColorTheme(iAColorTheme const *);
	virtual int count() const;
	virtual QString GetName(int idx) const;
	virtual QColor GetColor(int idx) const;
	bool AreSeedsAvailable() const;
	SVMImageFilter::SeedsPointer GetSeeds() const;
public slots:
	void RendererClicked(int, int, int);
	void SlicerClicked(int, int, int);
	void Add();
	void Remove();
	void Store();
	void Load();
	void StoreImage();
	void LabelOverlayReady();
	QString const & GetFileName();
private:
	void AddSeed(int, int, int);
	void RebuildLabelOverlayLUT();
	void UpdateOverlay();
	void SetOverlayPixels(int label, int value);
	void AddSeedItem(int label, int x, int y, int z);
	int AddLabelItem(QString const & labelText);
	vtkSmartPointer<vtkImageData> drawImage();
	void StartOverlayCreation();
	void ReColorExistingLabels();

	QStandardItemModel* m_itemModel;
	iAColorTheme const * m_colorTheme;
	int m_maxLabel;
	QString m_fileName;

	// for label overlay:
	vtkSmartPointer<vtkImageData> m_labelOverlayImg;
	vtkSmartPointer<vtkLookupTable> m_labelOverlayLUT;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOverlayOTF; // TODO: check why this is required - manual exploration also doesn't use it!
	MdiChild* m_mdiChild;
	LabelOverlayThread* m_labelOverlayThread;
	bool m_newOverlay;
signals:
	void SeedsAvailable();
};
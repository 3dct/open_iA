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

#include "iAImageTree.h"
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QWidget>

class iAImagePreviewWidget;
class iAPreviewWidgetPool;
class iATriangleButton;

class vtkCamera;

class QLabel;
class QToolButton;
class QHBoxLayout;
class QVBoxLayout;

class iAImageNodeWidget: public QWidget
{
	Q_OBJECT
public:
	iAImageNodeWidget(
		QWidget* parent,
		QSharedPointer<iAImageClusterNode > node,
		iAPreviewWidgetPool * previewPool,
		bool shrinkAuto,
		int representativeType);
	void Cleanup();
	bool IsExpanded() const;
	bool IsShrinked() const;
	void Layout(int x, int y, int width, int height);
	QSharedPointer<iAImageClusterNode> GetClusterNode();
	bool UpdateShrinkStatus();
	void ToggleButton();
	void ExpandNode();
	void SetAutoShrink(bool newAutoShrink);
	bool IsAutoShrinked() const;
	void UpdateRepresentative();
	void SetRepresentativeType(int representativeType);
protected:
	virtual void paintEvent(QPaintEvent * );
	virtual void mouseReleaseEvent(QMouseEvent * ev);
signals:
	void Expand(bool expand);
	void Clicked();
	void ImageClicked();
	void Updated();
private slots:
	void ExpandButtonClicked();
private:
	void ReturnPreview();
	bool CreatePreview();

	void SetShrinkedLayout();
	void SetLargeLayout();

	bool m_shrinkedAuto;
	bool m_shrinkStatus;
	QSharedPointer<iAImageClusterNode > m_cluster;
	iAImagePreviewWidget * m_imageView;
	iATriangleButton* m_expandButton;
	QLabel* m_infoLabel;
	QVBoxLayout* m_leftLayout;
	QHBoxLayout* m_mainLayout;
	iAPreviewWidgetPool * m_previewPool;
	int m_representativeType;
};

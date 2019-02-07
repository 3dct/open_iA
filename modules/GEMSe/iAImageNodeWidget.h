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

#include "iAImageTree.h"
#include "iAImageTreeNode.h"		// for LabelImagePointer

#include <iASlicerMode.h>

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
		QSharedPointer<iAImageTreeNode > node,
		iAPreviewWidgetPool * previewPool,
		bool shrinkAuto,
		int representativeType);
	void Cleanup();
	bool IsExpanded() const;
	bool IsShrinked() const;
	void Layout(int x, int y, int width, int height);
	QSharedPointer<iAImageTreeNode> GetClusterNode();
	bool UpdateShrinkStatus(LabelImagePointer refImg);
	void ToggleButton();
	void ExpandNode();
	void SetAutoShrink(bool newAutoShrink, LabelImagePointer refImg);
	bool IsAutoShrinked() const;
	bool UpdateRepresentative(LabelImagePointer refImg);
	bool SetRepresentativeType(int representativeType, LabelImagePointer refImg);
protected:
	virtual void paintEvent(QPaintEvent * );
	virtual void mouseReleaseEvent(QMouseEvent * ev);
signals:
	void Expand(bool expand);
	void Clicked();
	void ImageClicked();
	void ImageRightClicked();
	void Updated();
private slots:
	void ExpandButtonClicked();
private:
	void ReturnPreview();
	bool CreatePreview(LabelImagePointer refImg);

	void SetShrinkedLayout();
	void SetLargeLayout();

	bool m_shrinkedAuto;
	bool m_shrinkStatus;
	QSharedPointer<iAImageTreeNode > m_cluster;
	iAImagePreviewWidget * m_imageView;
	iATriangleButton* m_expandButton;
	QLabel* m_infoLabel;
	QVBoxLayout* m_leftLayout;
	QHBoxLayout* m_mainLayout;
	iAPreviewWidgetPool * m_previewPool;
	int m_representativeType;
};

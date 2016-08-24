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

#include "iASlicerMode.h"

#include "iAITKIO.h" // TODO: replace?
typedef iAITKIO::ImagePointer ClusterImageType;

#include <QGridLayout>
#include <QSharedPointer>
#include <QVector>
#include <QWidget>

class iAAttributeFilter;
class iAImageClusterLeaf;
class iAImageClusterNode;
class iAImagePreviewWidget;
class iAPreviewWidgetPool;

class ExampleGrid;

class vtkCamera;

class iAExampleImageWidget: public QWidget
{
	Q_OBJECT
public:
	//! aspectRatio = height/width
	iAExampleImageWidget(QWidget* parent, double aspectRatio, iAPreviewWidgetPool* previewPool, ClusterImageType nullImage);
	void SetSelectedNode(QSharedPointer<iAImageClusterNode> node);
	void SetSelectedImage(iAImageClusterLeaf * leaf);
	void FilterUpdated();
public slots:
	void AdaptLayout();
	void ImageUpdated();
signals:
	void Selected(iAImageClusterLeaf *);
	void Hovered(iAImageClusterLeaf *);
	void ViewUpdated();
protected:
	virtual void resizeEvent(QResizeEvent *);
private:
	void UpdateImages();
	QVector<iAImageClusterLeaf *> m_nodes;
	QGridLayout* m_layout;
	int m_width;
	int m_height;
	QSharedPointer<iAImageClusterNode> m_rootNode;
	double m_aspectRatio;
	iAPreviewWidgetPool * m_previewPool;
	ExampleGrid* m_gridWidget;
	ClusterImageType m_nullImage;
private slots:
	void ImageClicked();
	void ImageHovered();
};

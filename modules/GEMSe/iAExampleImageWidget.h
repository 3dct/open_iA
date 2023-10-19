// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>
#include <iAITKIO.h> // TODO: replace?

#include <QGridLayout>
#include <QVector>
#include <QWidget>

#include <memory>

class iAImageTreeLeaf;
class iAImageTreeNode;
class iAImagePreviewWidget;
class iAPreviewWidgetPool;

class ExampleGrid;

class vtkCamera;

typedef iAITKIO::ImagePointer ClusterImageType;

class iAExampleImageWidget: public QWidget
{
	Q_OBJECT
public:
	//! aspectRatio = height/width
	iAExampleImageWidget(double aspectRatio, iAPreviewWidgetPool* previewPool, ClusterImageType nullImage);
	void SetSelectedNode(std::shared_ptr<iAImageTreeNode> node);
	void SetSelectedImage(iAImageTreeLeaf * leaf);
	void FilterUpdated();
public slots:
	void AdaptLayout();
	void ImageUpdated();
private slots:
	void UpdateImages();
signals:
	void Selected(iAImageTreeLeaf *);
	void AlternateSelected(iAImageTreeNode *);
	void Hovered(iAImageTreeLeaf *);
	void ViewUpdated();
protected:
	virtual void resizeEvent(QResizeEvent *);
private:
	QVector<iAImageTreeLeaf *> m_nodes;
	QGridLayout* m_layout;
	int m_width;
	int m_height;
	std::shared_ptr<iAImageTreeNode> m_rootNode;
	double m_aspectRatio;
	iAPreviewWidgetPool * m_previewPool;
	ExampleGrid* m_gridWidget;
	ClusterImageType m_nullImage;
private slots:
	void ImageClicked();
	void ImageRightClicked();
	void ImageHovered();
};

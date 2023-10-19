// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTree.h"
#include "iAImageTreeNode.h"		// for LabelImagePointer

#include <iASlicerMode.h>

#include <QWidget>

#include <memory>

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
		std::shared_ptr<iAImageTreeNode > node,
		iAPreviewWidgetPool * previewPool,
		bool shrinkAuto,
		int representativeType);
	void Cleanup();
	bool IsExpanded() const;
	bool IsShrinked() const;
	void Layout(int x, int y, int width, int height);
	std::shared_ptr<iAImageTreeNode> GetClusterNode();
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
	void clicked();
	void ImageClicked();
	void ImageRightClicked();
	void updated();
private slots:
	void ExpandButtonClicked();
private:
	void ReturnPreview();
	bool CreatePreview(LabelImagePointer refImg);

	void SetShrinkedLayout();
	void SetLargeLayout();

	bool m_shrinkedAuto;
	bool m_shrinkStatus;
	std::shared_ptr<iAImageTreeNode > m_cluster;
	iAImagePreviewWidget * m_imageView;
	iATriangleButton* m_expandButton;
	QLabel* m_infoLabel;
	QVBoxLayout* m_leftLayout;
	QHBoxLayout* m_mainLayout;
	iAPreviewWidgetPool * m_previewPool;
	int m_representativeType;
};

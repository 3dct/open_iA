// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageNodeWidget.h"

#include <iASlicerMode.h>

#include <QMap>
#include <QRect>
#include <QWidget>

#include <memory>

class iAImageTree;
class iAPreviewWidgetPool;
class iATreeHighlight;

class vtkCamera;

class iATreeHighlight
{
public:
	iATreeHighlight() {}
	iATreeHighlight(QRect const & region, QColor const & color): m_region(region), m_color(color)
	{}
	QRect  m_region;
	QColor m_color;
};

class iAImageTreeView: public QWidget
{
	Q_OBJECT
public:

	iAImageTreeView(
		QWidget* parent,
		std::shared_ptr<iAImageTree > tree,
		iAPreviewWidgetPool * previewPool,
		int representativeType);
	std::shared_ptr<iAImageTree> const GetTree() const;
	void AddSelectedNode(std::shared_ptr<iAImageTreeNode> node, bool clear);
	void EnableSubtreeHighlight(bool enable);
	void FilterUpdated();
	bool JumpToNode(iAImageTreeNode const *, int stepLimit=0);
	QVector<std::shared_ptr<iAImageTreeNode> > const CurrentSelection() const;
	void UpdateAutoShrink(iAImageTreeNode* node, bool wasSelected);
	void UpdateSubtreeHighlight();
	void SetAutoShrink(bool enabled);
	bool GetAutoShrink();
	void SetIconSize(int iconSize);
	bool SetRepresentativeType(int representativeType, LabelImagePointer refImg);
	int  GetRepresentativeType() const;
	void freeMemory(std::shared_ptr<iAImageTreeNode> node, bool overrideFree);
	void SetRefImg(LabelImagePointer refImg);
signals:
	void clicked(std::shared_ptr<iAImageTreeNode >);
	void ImageClicked(std::shared_ptr<iAImageTreeNode >);
	void ImageRightClicked(iAImageTreeNode *);
	void Expanded(std::shared_ptr<iAImageTreeNode >);
	void JumpedTo(std::shared_ptr<iAImageTreeNode >);
	void ViewUpdated();
	void SelectionChanged();
protected:
	void paintEvent(QPaintEvent * ) override;
private slots:
	void ExpandNodeSlot(bool expand);
	void NodeClicked();
	void NodeImageClicked();
	void NodeImageRightClicked();
private:
	void AddNode(std::shared_ptr<iAImageTreeNode > node, bool shrinked);
	//! return true if expand was successful
	bool ExpandNode(iAImageNodeWidget* nodeWidget, bool expand, bool shrinked);
	void UpdateLayout();
	int LayoutNode(std::shared_ptr<iAImageTreeNode > node, int nodeNumber, int level, int & shrinked);

	void UpdateRepresentative(std::shared_ptr<iAImageTreeNode > node);
	//! collapse a node and all its subclusters:
	void CollapseNode(std::shared_ptr<iAImageTreeNode > node, bool & selectionChanged);

	int getNodeWithIdx(std::shared_ptr<iAImageTreeNode > node, int currentNr,
		int level, int searchedFor, std::shared_ptr<iAImageTreeNode > &result);

	void InsertNodeHighlight(iAImageTreeNode* node, QColor const & color);
	int GetExpandedChildren(iAImageTreeNode* node,
		int curLevel, int & maxLevel, int & shrinked);

	bool m_highlightSubtree;
	std::shared_ptr<iAImageTree> m_imageTree;
	// ToDo: extract to widget for a single cluster node:
	QMap<iAImageTreeNode*, iAImageNodeWidget* > m_nodeWidgets;
	//! number of currently shown nodes:
	int m_nodesShown;
	int m_maxLevel;
	//! whether to automatically shrink clusters:
	bool m_autoShrink;
	//! currently selected cluster
	QVector<std::shared_ptr<iAImageTreeNode> > m_selectedNode;
	QVector<iATreeHighlight> m_highlights;
	iAPreviewWidgetPool * m_previewPool;
	int m_iconSize;
	int m_representativeType;
	LabelImagePointer m_refImg;
};

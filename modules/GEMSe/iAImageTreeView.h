// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageNodeWidget.h"

#include <iASlicerMode.h>

#include <QMap>
#include <QRect>
#include <QSharedPointer>
#include <QWidget>

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
		QSharedPointer<iAImageTree > tree,
		iAPreviewWidgetPool * previewPool,
		int representativeType);
	QSharedPointer<iAImageTree> const GetTree() const;
	void AddSelectedNode(QSharedPointer<iAImageTreeNode> node, bool clear);
	void EnableSubtreeHighlight(bool enable);
	void FilterUpdated();
	bool JumpToNode(iAImageTreeNode const *, int stepLimit=0);
	QVector<QSharedPointer<iAImageTreeNode> > const CurrentSelection() const;
	void UpdateAutoShrink(iAImageTreeNode* node, bool wasSelected);
	void UpdateSubtreeHighlight();
	void SetAutoShrink(bool enabled);
	bool GetAutoShrink();
	void SetIconSize(int iconSize);
	bool SetRepresentativeType(int representativeType, LabelImagePointer refImg);
	int  GetRepresentativeType() const;
	void freeMemory(QSharedPointer<iAImageTreeNode> node, bool overrideFree);
	void SetRefImg(LabelImagePointer refImg);
signals:
	void clicked(QSharedPointer<iAImageTreeNode >);
	void ImageClicked(QSharedPointer<iAImageTreeNode >);
	void ImageRightClicked(iAImageTreeNode *);
	void Expanded(QSharedPointer<iAImageTreeNode >);
	void JumpedTo(QSharedPointer<iAImageTreeNode >);
	void ViewUpdated();
	void SelectionChanged();
protected:
	virtual void paintEvent(QPaintEvent * );
private slots:
	void ExpandNodeSlot(bool expand);
	void NodeClicked();
	void NodeImageClicked();
	void NodeImageRightClicked();
private:
	void AddNode(QSharedPointer<iAImageTreeNode > node, bool shrinked);
	//! return true if expand was successful
	bool ExpandNode(iAImageNodeWidget* nodeWidget, bool expand, bool shrinked);
	void UpdateLayout();
	int LayoutNode(QSharedPointer<iAImageTreeNode > node, int nodeNumber, int level, int & shrinked);

	void UpdateRepresentative(QSharedPointer<iAImageTreeNode > node);
	//! collapse a node and all its subclusters:
	void CollapseNode(QSharedPointer<iAImageTreeNode > node, bool & selectionChanged);

	int getNodeWithIdx(QSharedPointer<iAImageTreeNode > node, int currentNr,
		int level, int searchedFor, QSharedPointer<iAImageTreeNode > &result);

	void InsertNodeHighlight(iAImageTreeNode* node, QColor const & color);
	int GetExpandedChildren(iAImageTreeNode* node,
		int curLevel, int & maxLevel, int & shrinked);

	bool m_highlightSubtree;
	QSharedPointer<iAImageTree> m_imageTree;
	// ToDo: extract to widget for a single cluster node:
	QMap<iAImageTreeNode*, iAImageNodeWidget* > m_nodeWidgets;
	//! number of currently shown nodes:
	int m_nodesShown;
	int m_maxLevel;
	//! whether to automatically shrink clusters:
	bool m_autoShrink;
	//! currently selected cluster
	QVector<QSharedPointer<iAImageTreeNode> > m_selectedNode;
	QVector<iATreeHighlight> m_highlights;
	iAPreviewWidgetPool * m_previewPool;
	int m_iconSize;
	int m_representativeType;
	LabelImagePointer m_refImg;
};

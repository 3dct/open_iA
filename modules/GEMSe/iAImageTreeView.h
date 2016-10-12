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

#include "iAImageNodeWidget.h"
#include "iASlicerMode.h"

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
	void SetRepresentativeType(int representativeType);
signals:
	void Clicked(QSharedPointer<iAImageTreeNode >);
	void ImageClicked(QSharedPointer<iAImageTreeNode >);
	void Expanded(QSharedPointer<iAImageTreeNode >);
	void JumpedTo(QSharedPointer<iAImageTreeNode >);
	void ViewUpdated();
	void SelectionChanged();
protected:
	virtual void paintEvent(QPaintEvent * );
private slots:
	void ExpandNode(bool expand);
	void NodeClicked();
	void NodeImageClicked();
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
};

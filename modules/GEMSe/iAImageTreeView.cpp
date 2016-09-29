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
#include "pch.h"
#include "iAImageTreeView.h"

#include "iAConsole.h"
#include "iAImageNodeWidget.h"
#include "iAImageTreeNode.h"
#include "iAPreviewWidgetPool.h"
#include "iAGEMSeConstants.h"

#include <QMouseEvent>
#include <QPainter>

// tree utility functions:
// {
void FindNode(iAImageTreeNode const * searched, QList<QSharedPointer<iAImageTreeNode> > & path, QSharedPointer<iAImageTreeNode> curCluster, bool & found)
{
	path.push_back(curCluster);
	if (curCluster.data() != searched)
	{
		for (int i=0; i<curCluster->GetChildCount() && !found; ++i)
		{
			FindNode(searched, path, curCluster->GetChild(i), found);
		}
		if (!found)
		{
			path.removeLast();
		}
	}
	else
	{
		found = true;
	}
}

QSharedPointer<iAImageTreeNode> GetSibling(QSharedPointer<iAImageTreeNode> node)
{
	QSharedPointer<iAImageTreeNode> parent(node->GetParent());
	for (int i=0; i<parent->GetChildCount(); ++i)
	{
		if (parent->GetChild(i) != node)
		{
			return parent->GetChild(i);
		}
	}
	return QSharedPointer<iAImageTreeNode>();
}

// }


iAImageTreeView::iAImageTreeView(
	QWidget* parent,
	QSharedPointer<iAImageTree > tree,
	iAPreviewWidgetPool * previewPool,
	int representativeType)
:
	QWidget(parent),
	m_imageTree(tree),
	m_highlightSubtree(false),
	m_previewPool(previewPool),
	m_autoShrink(true),
	m_iconSize(TreePreviewSize),
	m_representativeType(representativeType)
{
	AddNode(m_imageTree->m_root, false);
	UpdateLayout();
}


QSharedPointer<iAImageTree > const iAImageTreeView::GetTree() const
{
	return m_imageTree;
}


void iAImageTreeView::AddSelectedNode(QSharedPointer<iAImageTreeNode> node, bool clear)
{
	if (clear)
	{
		m_selectedNode.clear();
	}
	if (m_selectedNode.contains(node))
	{
		m_selectedNode.remove(m_selectedNode.indexOf(node));
	}
	else
	{
		// emulate circular buffer
		if (m_selectedNode.size() == MaxSelectedClusters)
		{
			m_selectedNode.removeLast();
		}
		m_selectedNode.insert(0, node);
	}
	UpdateSubtreeHighlight();
	update();
	emit SelectionChanged();
}


void iAImageTreeView::EnableSubtreeHighlight(bool enable)
{
	m_highlightSubtree = enable;
}


int iAImageTreeView::GetExpandedChildren(iAImageTreeNode* node, int curLevel, int & maxLevel, int & shrinked)
{
	assert(m_nodeWidgets.contains(node));
	if (!m_nodeWidgets.contains(node))
	{
		return 0;
	}
	if (curLevel > maxLevel)
	{
		maxLevel = curLevel;
	}
	if (m_nodeWidgets[node]->IsShrinked())
	{
		shrinked++;
	}
	if (node->GetChildCount() == 0 ||
		!m_nodeWidgets[node]->IsExpanded())
	{
		return 1;
	}
	else
	{
		int sum = 1;
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			sum += GetExpandedChildren(node->GetChild(i).data(), curLevel+1, maxLevel, shrinked);
		}
		return sum;
	}
}


void iAImageTreeView::InsertNodeHighlight(iAImageTreeNode* node, QColor const & color)
{
	QRect g(m_nodeWidgets[node]->geometry());
	int left = g.left() - HighlightPaddingLeft;
	int top = g.top() - HighlightPaddingTop;
	int levelDiff = 0;
	int shrinkedNodes = 0;
	int expandedChildren = GetExpandedChildren(node, 0, levelDiff, shrinkedNodes);
	QRect highlightRegion(
		left,
		top,
		(levelDiff)*TreeLevelIndent + m_iconSize + TreeInfoRegionWidth + HighlightPaddingLeft+HighlightPaddingRight,
		(expandedChildren-shrinkedNodes) * (m_iconSize +TreeClusterPadding) +
		(shrinkedNodes) * (TreeClusterShrinkedHeight+TreeClusterPadding)
		- TreeClusterPadding + HighlightPaddingTop + HighlightPaddingBottom );
	//DebugOut () << "Highlighted region: x=" << highlightRegion.left() << "; y=" << highlightRegion.top() <<
	//	"; w=" << highlightRegion.width() << "; height=" << highlightRegion.height() << std::endl;
	m_highlights.push_back(iATreeHighlight(
		highlightRegion,
		color
	));
}


void iAImageTreeView::UpdateSubtreeHighlight()
{
	m_highlights.clear();
	if (!m_highlightSubtree)
	{
		return;
	}
	for (int i=0; i<m_selectedNode.size(); ++i)
	{
		// TODO ensure correct order ?
		QSharedPointer<iAImageTreeNode> node = m_selectedNode[i];
		if (!m_nodeWidgets.contains(node.data()))
		{
			DEBUG_LOG("ERROR in UpdateSubtreeHighlight: widget for selected node doesn't exist!");
			m_selectedNode.remove(m_selectedNode.indexOf(node));
			if (m_selectedNode.empty())
			{
				m_selectedNode.push_back(m_imageTree->m_root);
			}
			return;
		}
		InsertNodeHighlight(node.data(), DefaultColors::SubtreeHighlightColor[i]);

	}
	QList<iAImageTreeNode*> nodes = m_nodeWidgets.keys();
	for (int i=0; i<nodes.size(); ++i)
	{
		if (nodes[i]->GetAttitude() != iAImageTreeNode::NoPreference)
		{
			InsertNodeHighlight(nodes[i],
				nodes[i]-> GetAttitude() == iAImageTreeNode::Liked
					? DefaultColors::BackgroundLikeColor
					: DefaultColors::BackgroundHateColor );
		}
	}
}

void iAImageTreeView::SetAutoShrink(bool enabled)
{
	m_autoShrink = enabled;
}

bool iAImageTreeView::GetAutoShrink()
{
	return m_autoShrink;
}


void iAImageTreeView::SetIconSize(int iconSize)
{
	m_iconSize = iconSize;
	UpdateLayout();
}

void iAImageTreeView::UpdateLayout()
{
	m_maxLevel = 0;
	int shrinkedNodes = 0;
	m_nodesShown = LayoutNode(m_imageTree->m_root, 0, 0, shrinkedNodes);

	int requiredHeight = TreePadding + (m_nodesShown-shrinkedNodes) * (TreeClusterPadding+m_iconSize) +
							shrinkedNodes * (TreeClusterShrinkedHeight+TreeClusterPadding);
	setMinimumHeight(requiredHeight);

	UpdateSubtreeHighlight();
	update();
}


int iAImageTreeView::LayoutNode(QSharedPointer<iAImageTreeNode > node, int nodeNumber, int level, int & shrinkedNodes)
{
	if (level > m_maxLevel)
	{
		m_maxLevel = level;
	}
	if (!m_nodeWidgets.contains(node.data()))
	{
		DEBUG_LOG("ERROR in LayoutNode: widget for current child node doesn't exist.");
		return nodeNumber;
	}
	iAImageNodeWidget* nodeWidget = m_nodeWidgets[node.data()];
	if (!nodeWidget) {
		DEBUG_LOG("ERROR in LayoutNode: widget for current child node is NULL.");
		return nodeNumber;
	}
	nodeWidget->UpdateShrinkStatus();
	
	int left = TreePadding + level * TreeLevelIndent;
	int top = TreePadding + (nodeNumber-shrinkedNodes) * (TreeClusterPadding+m_iconSize) +
							shrinkedNodes * (TreeClusterShrinkedHeight+TreeClusterPadding);
	nodeWidget->Layout(
		left,
		top,
		m_iconSize+TreeInfoRegionWidth, // all remaining horizontal space for width
		nodeWidget->IsShrinked() ? TreeClusterShrinkedHeight : m_iconSize);

	//int nodeNumBefore = nodeNumber;
	nodeNumber++;
	if (nodeWidget->IsShrinked())
	{
		shrinkedNodes++;
	}

	if (nodeWidget->IsExpanded())
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			nodeNumber = LayoutNode(node->GetChild(i), nodeNumber, level+1, shrinkedNodes);
		}
	}
	return nodeNumber;
}



void iAImageTreeView::UpdateRepresentative(QSharedPointer<iAImageTreeNode > node)
{
	if (!m_nodeWidgets.contains(node.data()))
	{
		DEBUG_LOG("ERROR in UpdateRepresentative: widget for current child node doesn't exist.");
		return;
	}
	iAImageNodeWidget* nodeWidget = m_nodeWidgets[node.data()];
	if (!nodeWidget)
	{
		DEBUG_LOG("ERROR in UpdateRepresentative: widget for current child node is NULL.");
		return;
	}
	if (!nodeWidget->IsShrinked())
	{
		nodeWidget->UpdateRepresentative();
	}
	if (nodeWidget->IsExpanded())
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			UpdateRepresentative(node->GetChild(i));
		}
	}
}


void iAImageTreeView::AddNode(QSharedPointer<iAImageTreeNode > node, bool shrinked)
{
	iAImageNodeWidget* nodeWidget = new iAImageNodeWidget(this, node, m_previewPool, shrinked, m_representativeType);
	connect(nodeWidget, SIGNAL(Expand(bool)), this, SLOT(ExpandNode(bool)));
	connect(nodeWidget, SIGNAL(Clicked()), this, SLOT(NodeClicked()));
	connect(nodeWidget, SIGNAL(ImageClicked()), this, SLOT(NodeImageClicked()));
	connect(nodeWidget, SIGNAL(Updated()), this, SIGNAL(ViewUpdated()));
	// connect(rootNodeWidget, SIGNAL(remove()), this, SLOT(removeNode()));
	m_nodeWidgets.insert(node.data(), nodeWidget);
	nodeWidget->show();
}


void iAImageTreeView::CollapseNode(QSharedPointer<iAImageTreeNode > node, bool & selectionChanged)
{
	for (int i=0; i<node->GetChildCount(); ++i)
	{
		QSharedPointer<iAImageTreeNode> child = node->GetChild(i);
		if (!m_nodeWidgets.contains(child.data()))
		{
			DEBUG_LOG("ERROR in CollapseNode: widget for expanded child doesn't exist.");
			return;
		}
		iAImageNodeWidget* childWidget = m_nodeWidgets[child.data()];
		if (childWidget->IsExpanded())
		{
			CollapseNode(child, selectionChanged);
		}
		// if currently selected node is collapsed, select parent node:
		if (m_selectedNode.contains(child))
		{
			m_selectedNode.remove(m_selectedNode.indexOf(child));
			if (!m_selectedNode.contains(node))
			{
				m_selectedNode.push_back(node);
			}
			selectionChanged = true;
		}
		assert (childWidget);
		m_nodeWidgets.remove(child.data());
		childWidget->Cleanup();
		delete childWidget;
	}
}


void iAImageTreeView::ExpandNode(bool expand)
{
	QObject* obj = sender();
	iAImageNodeWidget* nodeWidget = dynamic_cast<iAImageNodeWidget*>(obj);

	// shrink parent node:
	QSharedPointer<iAImageTreeNode> parent = nodeWidget->GetClusterNode()->GetParent();
	if (parent)
	{
		if (m_autoShrink)
			m_nodeWidgets[parent.data()]->SetAutoShrink(true);
		QSharedPointer<iAImageTreeNode> sibling =
			GetSibling(nodeWidget->GetClusterNode());
		if (sibling && m_autoShrink)
		{
			m_nodeWidgets[sibling.data()]->SetAutoShrink(true);
		}
	}
	bool layoutNeedsUpdate = false;
	if (expand && nodeWidget->IsAutoShrinked())
	{
		layoutNeedsUpdate = true;
		nodeWidget->SetAutoShrink(false);
	}
	layoutNeedsUpdate |= ExpandNode(nodeWidget, expand, false);
	if (layoutNeedsUpdate)
	{
		UpdateLayout();
	}
	if (expand)
	{
		emit Expanded(nodeWidget->GetClusterNode());
	}
}

bool iAImageTreeView::ExpandNode(iAImageNodeWidget* nodeWidget, bool expand, bool shrinked)
{
	assert(nodeWidget);
	QSharedPointer<iAImageTreeNode> node = nodeWidget->GetClusterNode();
	if (expand)
	{
		if (m_previewPool->Capacity() < 2)
		{
			//DebugOut() << "Not enough free slicers available; please either collapse other nodes or shrink the example view! expanded=" << (nodeWidget->IsExpanded()?"true":"false") << std::endl;
			nodeWidget->ToggleButton();
			return false;
		}
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			AddNode(node->GetChild(i), shrinked);
		}
	}
	else
	{
		bool selChanged = false;
		CollapseNode(node, selChanged);
		if (selChanged)
		{
			emit SelectionChanged();
		}
	}
	return true;
}

void iAImageTreeView::JumpToNode(iAImageTreeNode const * cluster, int stepLimit)
{
	// Find cluster + parent path
	QList<QSharedPointer<iAImageTreeNode> > path;
	QSharedPointer<iAImageTreeNode> curCluster = m_imageTree->m_root;
	bool found = false;
	FindNode(cluster, path, curCluster, found);

	if (!found)
	{
		DEBUG_LOG("JumpToNode: Couldn't find given cluster!");
	}
	
	//QList<iAImageTreeNode const *> path(pathStack.toList());

	int steps = 0;
	int curIdx = 0;
	for (int i=0; i<path.size()-1 && (stepLimit == 0 || steps < stepLimit); ++i)
	{
		QSharedPointer<iAImageTreeNode> pathPtr = path[i];
		//DebugOut() << "cur Cluster ID: "<<  << "; path ID: " << pathID.toStdString() << std::endl;
		iAImageNodeWidget* widget = m_nodeWidgets[curCluster.data()];
		if (!widget->IsExpanded())
		{
			//ExpandNode(widget, true);
			widget->ToggleButton();
			ExpandNode(widget, true, true);
			// todo: check if node really expanded?
			++steps;
		}
		QSharedPointer<iAImageTreeNode>  nextPtr = path[i+1];
		for (int c=0; c<curCluster->GetChildCount(); ++c)
		{
			if (curCluster->GetChild(c) == nextPtr)
			{
				// it's this child, but expansion failed
				if (!m_nodeWidgets.contains(curCluster->GetChild(c).data()))
				{
					break;
				}
				curCluster = curCluster->GetChild(c);
				curIdx = i;
			}
		}
	}
	m_nodeWidgets[curCluster.data()]->SetAutoShrink(false);
	QSharedPointer<iAImageTreeNode> parent = path[curIdx];
	QSharedPointer<iAImageTreeNode> sibling = GetSibling(curCluster);
	m_nodeWidgets[parent.data()]->SetAutoShrink(false);
	m_nodeWidgets[sibling.data()]->SetAutoShrink(false);
	// get parent, expand
	// get sibling, expand
	UpdateLayout();
	emit JumpedTo(curCluster);
}

void iAImageTreeView::paintEvent(QPaintEvent * e)
{
	QWidget::paintEvent(e);
	QPainter p(this);
	QRect g(geometry());
	p.fillRect(g, Qt::white);
	for (int i=0; i<m_highlights.size(); ++i)
	{
		p.fillRect(m_highlights[i].m_region, m_highlights[i].m_color);
	}
	for (int j=0; j<m_selectedNode.size(); ++j)
	{
		QSharedPointer<iAImageTreeNode> node = m_selectedNode[j];
		p.setPen(DefaultColors::ClusterSelectPen[j]);
		QRect sel(m_nodeWidgets[node.data()]->geometry());
		if (m_nodeWidgets[node.data()]->IsShrinked())
		{
			sel.setHeight(TreeClusterShrinkedHeight);
		}
		//DebugOut() << "Selected cluster rect: x=" << sel.left() << "; y=" << sel.top() <<
		//	"; w=" << sel.width() << "; height=" << sel.height() << std::endl; 
		sel.adjust(-1, -1, +1, +1);
		p.drawRect(sel);
	}
}


void iAImageTreeView::NodeClicked()
{
	QObject* obj = sender();
	iAImageNodeWidget* nodeWidget = dynamic_cast<iAImageNodeWidget*>(obj);
	emit Clicked(nodeWidget->GetClusterNode());
}


void iAImageTreeView::NodeImageClicked()
{
	QObject* obj = sender();
	iAImageNodeWidget* nodeWidget = dynamic_cast<iAImageNodeWidget*>(obj);
	emit ImageClicked(nodeWidget->GetClusterNode());
}


void iAImageTreeView::FilterUpdated()
{
	UpdateRepresentative(m_imageTree->m_root);
	UpdateLayout(); // filter also affects layout - clusters with no elements after filter are displayed shrinked

}


QVector<QSharedPointer<iAImageTreeNode> > const iAImageTreeView::CurrentSelection() const
{
	return m_selectedNode;
}


void iAImageTreeView::UpdateAutoShrink(iAImageTreeNode* node, bool wasSelected)
{
	if (!m_nodeWidgets.contains(node))
	{
		return;
	}
	bool shrink = wasSelected && !m_nodeWidgets[node]->IsAutoShrinked() &&
		(m_nodeWidgets[node]->GetClusterNode()->GetAttitude() != iAImageTreeNode::Liked);
	if (m_nodeWidgets[node]->IsShrinked() != shrink)
	{
		m_nodeWidgets[node]->SetAutoShrink(shrink);
		UpdateLayout();
	}
}


void iAImageTreeView::SetRepresentativeType(int representativeType)
{
	m_representativeType = representativeType;
	for (iAImageTreeNode* key: m_nodeWidgets.keys())
	{
		m_nodeWidgets[key]->SetRepresentativeType(representativeType);
	}
}
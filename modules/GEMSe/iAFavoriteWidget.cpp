// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFavoriteWidget.h"

#include "iAImagePreviewWidget.h"
#include "iAImageTree.h"
#include "iAPreviewWidgetPool.h"
#include "iAQtCaptionWidget.h"
#include "iAGEMSeConstants.h"

#include <iAQWidgetHelper.h>
#include <iALog.h>

#include <QVBoxLayout>

typedef QVBoxLayout LikeLayoutType;


iAFavoriteWidget::iAFavoriteWidget(iAPreviewWidgetPool* previewPool) :
	m_previewPool(previewPool)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setLayout(createLayout<QHBoxLayout>(0));
	layout()->setAlignment(Qt::AlignTop | Qt::AlignCenter);

	QWidget* likes = new QWidget();
	m_likeLayout =  new LikeLayoutType();
	m_likeLayout->setSpacing(ExampleViewSpacing);
	m_likeLayout->setContentsMargins(ExampleViewSpacing, ExampleViewSpacing, ExampleViewSpacing, ExampleViewSpacing);
	m_likeLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	likes->setLayout(m_likeLayout);
	likes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	likes->setStyleSheet("background-color: #DFD;");
	layout()->addWidget(likes);
}


bool iAFavoriteWidget::HasAnyFavorite() const
{
	return m_favorites.size() > 0;
}


bool iAFavoriteWidget::ToggleLike(iAImageTreeNode * node)
{
	if (!node)
	{
		LOG(lvlError, "Favorites: ToggleLike called for nullptr node.\n");
		return false;
	}
	if (node->GetAttitude() == iAImageTreeNode::Liked)
	{
		node->SetAttitude(iAImageTreeNode::NoPreference);
		Remove(node);
		return false;
	}
	else
	{
		node->SetAttitude(iAImageTreeNode::Liked);
		Add(node);
		return true;
	}
}

bool iAFavoriteWidget::ToggleHate(iAImageTreeNode * node)
{
	if (!node)
	{
		LOG(lvlError, "Favorites: ToggleHate called for nullptr node.\n");
		return false;
	}
	if (node->GetAttitude() == iAImageTreeNode::Hated)
	{
		node->SetAttitude(iAImageTreeNode::NoPreference);
		return false;
	}
	else
	{
		if (node->GetAttitude() == iAImageTreeNode::Liked)
		{
			int idx = GetIndexForNode(node);
			if (idx == -1)
			{
				LOG(lvlError, "Favorites: node not found in favorite list.\n");
				return false;
			}
			iAImagePreviewWidget * widget = m_favorites[idx].widget;
			if (!widget)
			{
				LOG(lvlError, "Favorites: remove called for unset widget.\n");
				return false;
			}
			m_likeLayout->removeWidget(widget);
		}
		node->SetAttitude(iAImageTreeNode::Hated);
		return true;
	}
}


void iAFavoriteWidget::Add(iAImageTreeNode * node)
{
	if (!node || node->GetAttitude() != iAImageTreeNode::Liked)
	{
		return;
	}
	iAImagePreviewWidget * widget = m_previewPool->getWidget(this);
	if (!widget)
	{
		LOG(lvlError, "FavoriteView: No more slicer widgets available.\n");
		return;
	}
	widget->setFixedSize(FavoriteWidth, FavoriteWidth);
	widget->setImage(node->GetRepresentativeImage(iARepresentativeType::Difference,
		LabelImagePointer()), false, true);
	connect(widget, &iAImagePreviewWidget::clicked, this, &iAFavoriteWidget::FavoriteClicked);
	connect(widget, &iAImagePreviewWidget::rightClicked, this, &iAFavoriteWidget::FavoriteRightClicked);
	connect(widget, &iAImagePreviewWidget::updated, this, &iAFavoriteWidget::ViewUpdated);
	m_favorites.push_back(FavoriteData(node, widget));
	dynamic_cast<LikeLayoutType*>(m_likeLayout)->insertWidget(0, widget);
}


void iAFavoriteWidget::Remove(iAImageTreeNode const * node)
{
	if (!node)
	{
		LOG(lvlError, "Favorites: remove called for nullptr node\n");
		return;
	}
	int idx = GetIndexForNode(node);
	if (idx == -1)
	{
		LOG(lvlError, "Favorites: node not found in favorite list\n");
		return;
	}
	iAImagePreviewWidget * widget = m_favorites[idx].widget;
	if (!widget)
	{
		LOG(lvlError, "Favorites: remove called for unset widget\n");
		return;
	}
	m_favorites[idx].node = nullptr;
	m_favorites[idx].widget = nullptr;
	m_favorites.remove(idx);
	disconnect(widget, &iAImagePreviewWidget::clicked, this, &iAFavoriteWidget::FavoriteClicked);
	disconnect(widget, &iAImagePreviewWidget::rightClicked, this, &iAFavoriteWidget::FavoriteRightClicked);
	disconnect(widget, &iAImagePreviewWidget::updated, this, &iAFavoriteWidget::ViewUpdated);
	m_previewPool->returnWidget(widget);
}


void iAFavoriteWidget::FavoriteClicked()
{
	iAImagePreviewWidget * widget = dynamic_cast<iAImagePreviewWidget*>(sender());
	if (!widget)
	{
		LOG(lvlError, "FavoriteClicked: Invalid sender!\n");
	}
	iAImageTreeNode * node = GetNodeForWidget(widget);
	if (!node)
	{
		LOG(lvlError, "FavoriteClicked: Node not found!\n");
	}
	emit clicked(node);
}


void iAFavoriteWidget::FavoriteRightClicked()
{
	iAImagePreviewWidget * widget = dynamic_cast<iAImagePreviewWidget*>(sender());
	if (!widget)
	{
		LOG(lvlError, "FavoriteClicked: Invalid sender!\n");
	}
	iAImageTreeNode * node = GetNodeForWidget(widget);
	if (!node)
	{
		LOG(lvlError, "FavoriteClicked: Node not found!\n");
	}
	emit rightClicked(node);
}


int iAFavoriteWidget::GetIndexForNode(iAImageTreeNode const* node)
{
	for (int i=0; i<m_favorites.size(); ++i)
	{
		if (m_favorites[i].node == node)
		{
			return i;
		}
	}
	return -1;
}


iAImageTreeNode * iAFavoriteWidget::GetNodeForWidget(iAImagePreviewWidget* widget)
{
	for (FavoriteData const & fav: m_favorites)
	{
		if (fav.widget == widget)
		{
			return fav.node;
		}
	}
	return nullptr;
}


QVector<iAImageTreeNode const *> iAFavoriteWidget::GetFavorites(iAImageTreeNode::Attitude att) const
{
	QVector<iAImageTreeNode const *> result;
	for (FavoriteData const & fav : m_favorites)
	{
		if (fav.node->GetAttitude() == att)
		{
			result.push_back(fav.node);
		}
	}
	return result;
}

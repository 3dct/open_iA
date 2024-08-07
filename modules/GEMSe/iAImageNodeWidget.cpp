// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageNodeWidget.h"

#include "iAImagePreviewWidget.h"
#include "iAImageTreeNode.h"
#include "iAPreviewWidgetPool.h"
#include "iAGEMSeConstants.h"
#include "iATriangleButton.h"

#include <iALog.h>

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

iAImageNodeWidget::iAImageNodeWidget(QWidget* parent,
	std::shared_ptr<iAImageTreeNode> treeNode,
	iAPreviewWidgetPool * previewPool,
	bool shrinkAuto,
	int representativeType)
:
	QWidget(parent),
	m_shrinkedAuto(shrinkAuto),
	m_shrinkStatus(shrinkAuto || treeNode->GetFilteredSize() == 0),
	m_cluster(treeNode),
	m_imageView(nullptr),
	m_expandButton(nullptr),
	m_infoLabel(new QLabel(this)),
	m_previewPool(previewPool),
	m_representativeType(representativeType)
{
	setStyleSheet("background-color: transparent;");
	m_infoLabel->setStyleSheet("background-color: transparent;");
	m_infoLabel->setWordWrap(true);
	m_infoLabel->setFixedWidth(TreeInfoRegionWidth);
	QFont f(m_infoLabel->font());
	f.setPointSize(FontSize);
	m_infoLabel->setFont(f);

	m_mainLayout = new QHBoxLayout();
	QWidget * leftContainer(new QWidget());
	leftContainer->setFixedWidth(TreeInfoRegionWidth);
	m_leftLayout = new QVBoxLayout();
	m_leftLayout->setSpacing(0);
	m_leftLayout->setContentsMargins(0, 0, 0, 0);

	m_leftLayout->addWidget(m_infoLabel);
	m_leftLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

	if (!treeNode->IsLeaf())
	{
		m_expandButton = new iATriangleButton();
		m_expandButton->setFixedSize(TreeButtonWidth, TreeButtonHeight);
		m_expandButton->setContentsMargins(QMargins(0, 0, 0, 0));
		m_leftLayout->addWidget(m_expandButton);
		connect(m_expandButton, &iATriangleButton::clicked, this, &iAImageNodeWidget::ExpandButtonClicked);
	}
	leftContainer->setLayout(m_leftLayout);

	m_mainLayout->addWidget(leftContainer);
	m_mainLayout->setSpacing(0);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_mainLayout);
	if (!m_shrinkStatus)
	{
		CreatePreview(LabelImagePointer());
	}
	else
	{
		SetShrinkedLayout();
	}
}


bool iAImageNodeWidget::CreatePreview(LabelImagePointer refImg)
{
	m_imageView = m_previewPool->getWidget(this);
	if (!m_imageView)
	{
		return false;
	}
	UpdateRepresentative(refImg);
	connect(m_imageView, &iAImagePreviewWidget::clicked, this, &iAImageNodeWidget::ImageClicked);
	connect(m_imageView, &iAImagePreviewWidget::rightClicked, this, &iAImageNodeWidget::ImageRightClicked);
	connect(m_imageView, &iAImagePreviewWidget::updated, this, &iAImageNodeWidget::updated);
	m_mainLayout->addWidget(m_imageView);
	return true;
}

void iAImageNodeWidget::ReturnPreview()
{
	m_imageView->hide();
	m_mainLayout->removeWidget(m_imageView);
	disconnect(m_imageView, &iAImagePreviewWidget::clicked, this, &iAImageNodeWidget::ImageClicked);
	disconnect(m_imageView, &iAImagePreviewWidget::rightClicked, this, &iAImageNodeWidget::ImageRightClicked);
	disconnect(m_imageView, &iAImagePreviewWidget::updated, this, &iAImageNodeWidget::updated);
	m_previewPool->returnWidget(m_imageView);
	m_imageView = nullptr;
	m_cluster->DiscardDetails();
}

void iAImageNodeWidget::Cleanup()
{
	hide();
	if (!m_shrinkStatus)
	{
		ReturnPreview();
	}
}

void iAImageNodeWidget::Layout(int x, int y, int width, int height)
{
	if (m_expandButton)
	{
		m_expandButton->setVisible(
			IsExpanded() || GetClusterNode()->GetFilteredSize() > 0
		);
	}
	resize(width, height);
	if (m_imageView)
	{
		m_imageView->setFixedSize(width-TreeInfoRegionWidth, height);
	}
	move(x, y);
}

std::shared_ptr<iAImageTreeNode > iAImageNodeWidget::GetClusterNode()
{
	return m_cluster;
}

bool iAImageNodeWidget::IsExpanded() const
{
	return (!m_expandButton)? false : m_expandButton->IsExpanded();
}

bool iAImageNodeWidget::IsShrinked() const
{
	return m_shrinkedAuto || (m_cluster->GetFilteredSize() == 0);
}

void iAImageNodeWidget::ToggleButton()
{
	m_expandButton->Toggle();
}

void iAImageNodeWidget::ExpandNode()
{
	ToggleButton();
	ExpandButtonClicked();
}

void iAImageNodeWidget::ExpandButtonClicked()
{
	if (m_cluster->GetChildCount() == 0)
	{
		return;
	}
	if (m_cluster->GetDistance() == 0)
	{
		LOG(lvlWarn, "Cluster only holds exactly equal results, skipping expansion!");
		return;
	}
	emit Expand(IsExpanded());
}

void iAImageNodeWidget::paintEvent(QPaintEvent * e)
{
	if (m_cluster->GetFilteredSize() != m_cluster->GetClusterSize())
	{
		m_infoLabel->setText(QString::number(m_cluster->GetFilteredSize())+(IsShrinked()?"":"\n")+"("+QString::number(m_cluster->GetClusterSize())+")");
	}
	else
	{
		m_infoLabel->setText(QString::number(m_cluster->GetClusterSize()));
	}
	QWidget::paintEvent(e);
	/*
	QPainter p(this);
	QRect g(geometry());
	p.fillRect(g, QColor(230, 230, 230));
	*/
}

void iAImageNodeWidget::mouseReleaseEvent(QMouseEvent * ev)
{
	QWidget::mouseReleaseEvent(ev);
	if (ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}

void iAImageNodeWidget::SetShrinkedLayout()
{
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	setFixedHeight(TreeClusterShrinkedHeight);
	m_infoLabel->setWordWrap(false);
	m_infoLabel->setFixedWidth(TreePreviewSize);
	if (m_expandButton)
	{
		m_leftLayout->removeWidget(m_expandButton);
		m_mainLayout->addWidget(m_expandButton);
	}
}

void iAImageNodeWidget::SetLargeLayout()
{
	layout()->setSizeConstraint(QLayout::SetNoConstraint);
	setMinimumHeight(TreePreviewSize);							// TODO: get icon size from tree view!
	setMinimumWidth(TreePreviewSize+TreeInfoRegionWidth);
	m_infoLabel->setWordWrap(true);
	m_infoLabel->setFixedWidth(TreeInfoRegionWidth);
	if (m_expandButton)
	{
		m_mainLayout->removeWidget(m_expandButton);
		m_leftLayout->addWidget(m_expandButton);
	}
}

bool iAImageNodeWidget::UpdateShrinkStatus(LabelImagePointer refImg)
{
	bool newShrink = IsShrinked();
	if (m_shrinkStatus == newShrink)
	{ // only act if something has changed
		return true;
	}
	m_shrinkStatus = newShrink;
	if (newShrink)
	{
		ReturnPreview();
		SetShrinkedLayout();
	}
	else
	{
		if (!CreatePreview(refImg))
		{
			m_shrinkStatus = true;
			m_shrinkedAuto = true;
			return false;
		}
		SetLargeLayout();
	}
	return true;
}

void iAImageNodeWidget::SetAutoShrink(bool newAutoShrink, LabelImagePointer refImg)
{
	if (newAutoShrink == m_shrinkedAuto)
		return;
	if (GetClusterNode()->GetAttitude() == iAImageTreeNode::Liked)
	{
		return;
	}
	m_shrinkedAuto = newAutoShrink;
	UpdateShrinkStatus(refImg);
}

bool iAImageNodeWidget::IsAutoShrinked() const
{
	return m_shrinkedAuto;
}

bool iAImageNodeWidget::UpdateRepresentative(LabelImagePointer refImg)
{
	if (!m_imageView)
	{
		return true;
	}
	if (!m_cluster->GetRepresentativeImage(m_representativeType, refImg))
	{
		return false;
	}
	m_imageView->setImage(m_cluster->GetRepresentativeImage(m_representativeType, refImg), false,
		m_cluster->IsLeaf() || m_representativeType == Difference || m_representativeType == AverageLabel);
	m_imageView->update();
	return true;
}


bool iAImageNodeWidget::SetRepresentativeType(int representativeType, LabelImagePointer refImg)
{
	int oldRepresentativeType = m_representativeType;
	m_representativeType = representativeType;
	bool result = UpdateRepresentative(refImg);
	if (!result)
	{
		m_representativeType = oldRepresentativeType;
	}
	return result;
}

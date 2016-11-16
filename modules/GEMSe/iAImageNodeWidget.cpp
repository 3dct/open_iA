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
#include "iAImageNodeWidget.h"

#include "iAConsole.h"
#include "iAImagePreviewWidget.h"
#include "iAImageTreeNode.h"
#include "iAPreviewWidgetPool.h"
#include "iAGEMSeConstants.h"
#include "iATriangleButton.h"

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

iAImageNodeWidget::iAImageNodeWidget(QWidget* parent,
	QSharedPointer<iAImageTreeNode> treeNode,
	iAPreviewWidgetPool * previewPool,
	bool shrinkAuto,
	int representativeType)
:
	QWidget(parent),
	m_cluster(treeNode),
	m_shrinkedAuto(shrinkAuto),
	m_shrinkStatus(shrinkAuto || treeNode->GetFilteredSize() == 0),
	m_infoLabel(new QLabel(this)),
	m_expandButton(0),
	m_previewPool(previewPool),
	m_imageView(0),
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
	m_leftLayout->setMargin(0);
	
	m_leftLayout->addWidget(m_infoLabel);
	m_leftLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

	if (!treeNode->IsLeaf())
	{
		m_expandButton = new iATriangleButton();
		m_expandButton->setFixedSize(TreeButtonWidth, TreeButtonHeight);
		m_expandButton->setContentsMargins(QMargins(0, 0, 0, 0));
		m_leftLayout->addWidget(m_expandButton);
		connect(m_expandButton, SIGNAL(Clicked()), this, SLOT(ExpandButtonClicked()));
	}
	leftContainer->setLayout(m_leftLayout);

	m_mainLayout->addWidget(leftContainer);
	m_mainLayout->setSpacing(0);
	m_mainLayout->setMargin(0);
	setLayout(m_mainLayout);
	if (!m_shrinkStatus)
	{
		CreatePreview();
	}
	else
	{
		SetShrinkedLayout();
	}
}


bool iAImageNodeWidget::CreatePreview()
{
	m_imageView = m_previewPool->GetWidget(this);
	if (!m_imageView)
	{
		return false;
	}
	UpdateRepresentative();
	connect(m_imageView, SIGNAL(Clicked()), this, SIGNAL(ImageClicked()));
	connect(m_imageView, SIGNAL(Updated()), this, SIGNAL(Updated()) );
	m_mainLayout->addWidget(m_imageView);
	return true;
}

void iAImageNodeWidget::ReturnPreview()
{
	m_imageView->hide();
	m_mainLayout->removeWidget(m_imageView);
	disconnect(m_imageView, SIGNAL(Clicked()), this, SIGNAL(ImageClicked()));
	disconnect(m_imageView, SIGNAL(Updated()),   this, SIGNAL(Updated()) );
	m_previewPool->ReturnWidget(m_imageView);
	m_imageView = 0;
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

QSharedPointer<iAImageTreeNode > iAImageNodeWidget::GetClusterNode()
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
	QPainter p(this);
	QRect g(geometry());
	/*
	p.fillRect(g, QColor(230, 230, 230));
	*/

}

void iAImageNodeWidget::mouseReleaseEvent(QMouseEvent * ev)
{
	QWidget::mouseReleaseEvent(ev);
	if (ev->button() == Qt::LeftButton)
	{
		emit Clicked();
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

bool iAImageNodeWidget::UpdateShrinkStatus()
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
		if (!CreatePreview())
		{
			m_shrinkStatus = true;
			m_shrinkedAuto = true;
			return false;
		}
		SetLargeLayout();
	}
	return true;
}

void iAImageNodeWidget::SetAutoShrink(bool newAutoShrink)
{
	if (newAutoShrink == m_shrinkedAuto)
		return;
	if (GetClusterNode()->GetAttitude() == iAImageTreeNode::Liked)
	{
		return;
	}
	m_shrinkedAuto = newAutoShrink;
	UpdateShrinkStatus();
}

bool iAImageNodeWidget::IsAutoShrinked() const
{
	return m_shrinkedAuto;
}

bool iAImageNodeWidget::UpdateRepresentative()
{
	if (!m_imageView)
	{
		return true;
	}
	if (!m_cluster->GetRepresentativeImage(m_representativeType))
	{
		return false;
	}
	m_imageView->SetImage(m_cluster->GetRepresentativeImage(m_representativeType), false,
		m_cluster->IsLeaf() || m_representativeType == Difference || m_representativeType == AverageLabel);
	m_imageView->update();
	return true;
}


bool iAImageNodeWidget::SetRepresentativeType(int representativeType)
{
	int oldRepresentativeType = m_representativeType;
	m_representativeType = representativeType;
	bool result = UpdateRepresentative();
	if (!result)
	{
		m_representativeType = oldRepresentativeType;
	}
	return result;
}

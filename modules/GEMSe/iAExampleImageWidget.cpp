// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAExampleImageWidget.h"

#include "iAImageTreeLeaf.h"
#include "iAImagePreviewWidget.h"
#include "iAQtCaptionWidget.h"
#include "iAPreviewWidgetPool.h"
#include "iAGEMSeConstants.h"

#include <iALog.h>
#include <iAMathUtility.h>
#include <iAQWidgetHelper.h>

#include <QGridLayout>
#include <QPainter>
#include <QPushButton>
#include <QTimerEvent>

namespace
{
	const int NoImageSelected = -1;
}

class ExampleGrid: public QWidget
{
public:
	ExampleGrid():
		m_selectedIndex(NoImageSelected)
	{}
	QVector<iAImagePreviewWidget*> m_previews;
	qsizetype m_selectedIndex;
protected:
	void paintEvent(QPaintEvent * /*e*/) override
	{
		if (m_selectedIndex != NoImageSelected)
		{
			QPainter painter(this);
			painter.setPen(DefaultColors::ImageSelectPen);
			QRect sel(m_previews[m_selectedIndex]->geometry());
			sel.adjust(-1, -1, +1, +1);
			painter.drawRect(sel);
		}
	}
};


iAExampleImageWidget::iAExampleImageWidget(double aspectRatio, iAPreviewWidgetPool * previewPool, ClusterImageType nullImage):
	m_layout(createLayout<QGridLayout>(ExampleViewSpacing, ExampleViewSpacing)),
	m_width(-1),
	m_height(1),
	m_aspectRatio(aspectRatio),
	m_previewPool(previewPool),
	m_gridWidget(new ExampleGrid),
	m_nullImage(nullImage)
{
	m_gridWidget->setLayout(m_layout);

	QWidget* container = new QWidget();
	QPushButton* refreshButton = new QPushButton(">");
	refreshButton->setFixedWidth(30);
	connect(refreshButton, &QPushButton::clicked, this, &iAExampleImageWidget::UpdateImages);
	m_gridWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	container->setLayout(new QHBoxLayout());
	container->layout()->addWidget(m_gridWidget);
	container->layout()->addWidget(refreshButton);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	SetCaptionedContent(this, "Examples", container);

	AdaptLayout();
}


void iAExampleImageWidget::AdaptLayout()
{
	double aspectRatio  = 1;
	if (m_gridWidget->m_previews.size() > 0)
	{
		aspectRatio = clamp(0.5, 2.0, m_gridWidget->m_previews[0]->aspectRatio());
	}
	int widthFromHeight = (geometry().width() / static_cast<double>(geometry().height()) ) * aspectRatio;
	int newWidth = clamp(1, 12, widthFromHeight);
	if (newWidth == m_width)
	{
		return;
	}
	m_width = newWidth;

	// remove existing widgets from layout
	for (int i=0; i<m_gridWidget->m_previews.size(); ++i)
	{
		disconnect(m_gridWidget->m_previews[i], &iAImagePreviewWidget::clicked, this, &iAExampleImageWidget::ImageClicked);
		disconnect(m_gridWidget->m_previews[i], &iAImagePreviewWidget::rightClicked, this, &iAExampleImageWidget::ImageRightClicked);
		disconnect(m_gridWidget->m_previews[i], &iAImagePreviewWidget::mouseHover, this, &iAExampleImageWidget::ImageHovered);
		disconnect(m_gridWidget->m_previews[i], &iAImagePreviewWidget::updated, this, &iAExampleImageWidget::ImageUpdated);
		m_layout->removeWidget(m_gridWidget->m_previews[i]);
		m_previewPool->returnWidget(m_gridWidget->m_previews[i]);
	}
	m_gridWidget->m_previews.clear();
	// get new widgets:
	for (int i=0; i<m_width*m_height; ++i)
	{
		iAImagePreviewWidget * imgWidget = m_previewPool->getWidget(this);
		if (!imgWidget)
		{
			return;
		}
		imgWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		assert(imgWidget);
		m_gridWidget->m_previews.push_back(imgWidget);
	}
	// arrange slicers in calculated layout:
	for (int x=0; x<m_width; ++x)
	{
		for (int y=0; y<m_height; ++y)
		{
			int idx = y*m_width+x;
			iAImagePreviewWidget * imgWidget = m_gridWidget->m_previews[idx];
			imgWidget->show();
			m_layout->addWidget(imgWidget, y, x);
			connect(imgWidget, &iAImagePreviewWidget::clicked, this, &iAExampleImageWidget::ImageClicked);
			connect(imgWidget, &iAImagePreviewWidget::rightClicked, this, &iAExampleImageWidget::ImageRightClicked);
			connect(imgWidget, &iAImagePreviewWidget::mouseHover, this, &iAExampleImageWidget::ImageHovered);
			connect(imgWidget, &iAImagePreviewWidget::updated, this, &iAExampleImageWidget::ImageUpdated );
		}
	}
	UpdateImages();
}

void iAExampleImageWidget::SetSelectedNode(std::shared_ptr<iAImageTreeNode> node)
{
	m_rootNode = node;
	UpdateImages();
}

void iAExampleImageWidget::UpdateImages()
{
	int numOfImages = m_width*m_height;
	if (!m_rootNode || numOfImages == 0)
	{
		return;
	}
	if (m_rootNode->GetDistance() == 0)
	{
		numOfImages = 1;
	}
	m_gridWidget->m_selectedIndex = NoImageSelected;
	m_nodes.clear();

	m_rootNode->GetExampleImages(m_nodes, numOfImages);

	if (m_nodes.size() > m_rootNode->GetFilteredSize() )
	{
		LOG(lvlError, QString("Found more images (%1) than there are images in the cluster (%2)\n")
			.arg(m_nodes.size())
			.arg(m_rootNode->GetFilteredSize()));
	}
	for (int y=0; y<m_height; ++y)
	{
		for (int x=0; x<m_width; ++x)
		{
			int idx = y*m_width+x;
			if (idx < m_nodes.size())
			{
				m_gridWidget->m_previews[idx]->setImage(m_nodes[idx]->GetLargeImage(), false, true);
			}
			else
			{
				m_gridWidget->m_previews[idx]->setImage(m_nullImage, true, true);
			}
		}
	}
	update();
}

// TODO: find way to reuse code among the following two methods:
void iAExampleImageWidget::ImageClicked()
{
	iAImagePreviewWidget* imgWdgt = dynamic_cast<iAImagePreviewWidget*>(sender());
	assert(imgWdgt);
	if (!imgWdgt)
	{
		LOG(lvlError, "ExampleWidget click: sender not an image widget!\n");
		return;
	}
	auto idx = m_gridWidget->m_previews.indexOf(imgWdgt);
	assert(idx != -1);
	if (idx == -1)
	{
		LOG(lvlError, "ExampleWidget click: didn't find originating image widget!\n");
		// something wrong...
		return;
	}
	if (idx < m_nodes.size())
	{
		m_gridWidget->m_selectedIndex = idx;
		emit Selected(m_nodes[idx]);
		update();
	}
}

void iAExampleImageWidget::ImageRightClicked()
{
	iAImagePreviewWidget* imgWdgt = dynamic_cast<iAImagePreviewWidget*>(sender());
	assert(imgWdgt);
	if (!imgWdgt)
	{
		LOG(lvlError, "ExampleWidget click: sender not an image widget!\n");
		return;
	}
	auto idx = m_gridWidget->m_previews.indexOf(imgWdgt);
	assert(idx != -1);
	if (idx == -1)
	{
		LOG(lvlError, "ExampleWidget click: didn't find originating image widget!\n");
		// something wrong...
		return;
	}
	if (idx < m_nodes.size())
	{
		m_gridWidget->m_selectedIndex = idx;
		emit AlternateSelected(m_nodes[idx]);
		//update();
	}
}

void iAExampleImageWidget::SetSelectedImage(iAImageTreeLeaf * leaf)
{
	auto idx = m_nodes.indexOf(leaf);
	if (idx == -1)
	{
		//DebugOut() << "ExampleWidget: Requested Image not currently shown!" << std::endl;
		return;
	}
	m_gridWidget->m_selectedIndex = idx;
	update();
}

void iAExampleImageWidget::ImageHovered()
{
	iAImagePreviewWidget* imgWdgt = dynamic_cast<iAImagePreviewWidget*>(sender());
	assert(imgWdgt);
	if (!imgWdgt)
	{
		// something wrong...
		return;
	}
	auto idx = m_gridWidget->m_previews.indexOf(imgWdgt);
	assert(idx != -1);
	if (idx == -1)
	{
		// something wrong...
		return;
	}
	if (idx < m_nodes.size())
	{
		emit Hovered(m_nodes[idx]);
	}
}


void iAExampleImageWidget::resizeEvent(QResizeEvent * re)
{
	QWidget::resizeEvent(re);
	AdaptLayout();
}


void iAExampleImageWidget::FilterUpdated()
{
	UpdateImages();
}


void iAExampleImageWidget::ImageUpdated()
{
	emit ViewUpdated();
}

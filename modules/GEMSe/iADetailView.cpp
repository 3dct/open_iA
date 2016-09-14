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
#include "iADetailView.h"

#include "iAAttributes.h"
#include "iAConsole.h"
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAImagePreviewWidget.h"
#include "iALabelInfo.h"
#include "iANameMapper.h"
#include "iAAttributeDescriptor.h"
#include "iASlicerWidget.h"

#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextEdit>

iADetailView::iADetailView(
		iAImagePreviewWidget* prevWdgt,
		ClusterImageType nullImage,
		QSharedPointer<iAModalityList> modalities,
		iALabelInfo const & labelInfo,
		int representativeType) :
	m_previewWidget(prevWdgt),
	m_showingClusterRepresentative(true),
	m_pbLike(new QPushButton("")),
	m_pbHate(new QPushButton("")),
	m_pbGoto(new QPushButton("")),
	m_nullImage(nullImage),
	m_modalities(modalities),
	m_magicLensData(0),
	m_magicLensCurrentModality(0),
	m_representativeType(representativeType)
{
	m_pbLike->setContentsMargins(0, 0, 0, 0);
	m_pbHate->setContentsMargins(0, 0, 0, 0);
	m_pbGoto->setContentsMargins(0, 0, 0, 0);

	m_pbGoto->setStyleSheet("qproperty-icon: url(:/images/GEMSe_goto.png); background-color: "+DefaultColors::BackgroundColorText+"; border:none;");

	QHBoxLayout* buttonLay = new QHBoxLayout();
	buttonLay->setSpacing(0);
	buttonLay->setContentsMargins(0, 0, 0, 0);
	buttonLay->addWidget(m_pbLike);
	buttonLay->addWidget(m_pbHate);
	buttonLay->addWidget(m_pbGoto);
	
	QWidget * buttonBar = new QWidget();
	buttonBar->setLayout(buttonLay);
	buttonBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	prevWdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVBoxLayout* lay = new QVBoxLayout();
	lay->setSpacing(1);
	lay->setMargin(1);
	lay->addWidget(prevWdgt);
	lay->addWidget(buttonBar);

	QWidget * imgStuffWidget = new QWidget();
	imgStuffWidget->setLayout(lay);
	imgStuffWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	QWidget * detailWidget = new QWidget();
	QVBoxLayout* detailWidgetLayout = new QVBoxLayout();
	detailWidget->setLayout(detailWidgetLayout);
	
	QSplitter * detailSplitter = new QSplitter();
	detailSplitter->setOrientation(Qt::Vertical);
	detailWidgetLayout->addWidget(detailSplitter);

	m_labelItemModel = new QStandardItemModel();
	QListView* lvLegend = new QListView();
	lvLegend->setModel(m_labelItemModel);
	detailSplitter->addWidget(lvLegend);
	SetLabelInfo(labelInfo);
	int height = m_labelItemModel->rowCount() * lvLegend->sizeHintForRow(0);
	lvLegend->setMinimumHeight(height);
	
	m_detailText = new QTextEdit();
	m_detailText->setReadOnly(true);
	m_detailText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	detailSplitter->addWidget(m_detailText);
	
	QFont f(m_detailText->font());
	f.setPointSize(FontSize);
	m_detailText->setFont(f);
	lvLegend->setFont(f);
	
	lvLegend->setStyleSheet("border: none; outline:none;");
	m_detailText->setStyleSheet("border: none; outline:none;");

	QSplitter * horzSplitter = new QSplitter();
	
	QHBoxLayout* mainLay = new QHBoxLayout();
	mainLay->setSpacing(1);
	mainLay->setMargin(1);
	mainLay->addWidget(horzSplitter);

	horzSplitter->addWidget(imgStuffWidget);
	horzSplitter->addWidget(detailWidget);
	horzSplitter->setStretchFactor(0, 2);
	horzSplitter->setStretchFactor(0, 1);
		
	QWidget * mainWdgt(this);
	mainWdgt->setLayout(mainLay);
	QRect geom(geometry());
	geom.adjust(+1, +1, -1, -1);
	mainWdgt->setGeometry(geom);

	// prevWdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(m_pbLike, SIGNAL(clicked()), this, SIGNAL(Like()));
	connect(m_pbHate, SIGNAL(clicked()), this, SIGNAL(Hate()));
	connect(m_pbGoto, SIGNAL(clicked()), this, SIGNAL(GoToCluster()));
	connect(m_previewWidget, SIGNAL(Updated()), this, SIGNAL(ViewUpdated()));

	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(DblClicked()), this, SLOT(DblClicked()));
	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeModality(int)));
	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));
}

#include "iAChannelVisualizationData.h"
#include "iAModality.h"
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

vtkSmartPointer<vtkColorTransferFunction> GetDefaultCTF(vtkSmartPointer<vtkImageData> imageData)
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	double const * scalarRange = imageData->GetScalarRange();
	ctf->AddRGBPoint (scalarRange[0], 0.0, 0.0, 0.0 );
	ctf->AddRGBPoint (scalarRange[1], 1.0, 1.0, 1.0 );
	ctf->Build();
	return ctf;

}
vtkSmartPointer<vtkPiecewiseFunction> GetDefaultOTF(vtkSmartPointer<vtkImageData> imageData)
{
	vtkSmartPointer<vtkPiecewiseFunction> otf = vtkPiecewiseFunction::New();
	double const * scalarRange = imageData->GetScalarRange();
	otf->AddPoint(scalarRange[0], 1);
	otf->AddPoint(scalarRange[1], 1);
	return otf;
}

#include "iAModalityTransfer.h"

void iADetailView::DblClicked()
{
	iASlicer* slicer = m_previewWidget->GetSlicer();
	iAChannelID id = ch_SE_DetailView;

	if (m_magicLensData)
	{
		slicer->SetMagicLensEnabled(false);
		delete m_magicLensData;
		m_magicLensData = 0;
		return;
	}

	QSharedPointer<iAModality> mod = m_modalities->Get(m_magicLensCurrentModality);
	vtkSmartPointer<vtkImageData> imageData = mod->GetImage();

	m_magicLensData = new iAChannelVisualizationData();
	QColor color(255, 0, 0);
	m_magicLensData->SetColor(color);
	m_ctf = (mod->GetName() == "Ground Truth") ?
		m_previewWidget->GetCTF().GetPointer() :
		mod->GetTransfer()->GetColorFunction();
	m_otf = (mod->GetName() == "Ground Truth") ?
		GetDefaultOTF(imageData).GetPointer() :
		mod->GetTransfer()->GetOpacityFunction();
	ResetChannel(m_magicLensData, imageData, m_ctf, m_otf);
	slicer->SetMagicLensCaption(mod->GetName().toStdString());
	slicer->initializeChannel(id, m_magicLensData);
	m_magicLensData->SetEnabled(false);
	slicer->widget()->SetMagicLensFrameWidth(4.0);

	int sliceNr = m_previewWidget->GetSliceNumber();
	switch(slicer->GetMode())
	{
		case YZ: slicer->enableChannel(id, false, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->enableChannel(id, false, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->enableChannel(id, false, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}
	switch(slicer->GetMode())
	{
		case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}

	slicer->SetMagicLensInput(id);
	slicer->SetMagicLensEnabled(true);
	slicer->update();
}

void iADetailView::ChangeModality(int )
{
	if (!m_magicLensData)
	{
		return;
	}
	iASlicer* slicer = m_previewWidget->GetSlicer();
	iAChannelID id = ch_SE_DetailView;
	// TODO: refactor to remove duplication between here and DblClicked above
	m_magicLensCurrentModality = (m_magicLensCurrentModality + 1) % m_modalities->size();
	QSharedPointer<iAModality> mod = m_modalities->Get(m_magicLensCurrentModality);
	vtkSmartPointer<vtkImageData> imageData = mod->GetImage();
	m_ctf = (mod->GetName() == "Ground Truth") ?
		m_previewWidget->GetCTF().GetPointer() :
		mod->GetTransfer()->GetColorFunction();
	m_otf = (mod->GetName() == "Ground Truth") ?
		GetDefaultOTF(imageData).GetPointer() :
		mod->GetTransfer()->GetOpacityFunction();
	ResetChannel(m_magicLensData, imageData, m_ctf, m_otf);
	slicer->SetMagicLensCaption(mod->GetName().toStdString());
	slicer->reInitializeChannel(id, m_magicLensData);
	int sliceNr = m_previewWidget->GetSliceNumber();
	switch(slicer->GetMode())
	{
		case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}
	slicer->SetMagicLensInput(id);
	slicer->SetMagicLensEnabled(true);
	slicer->update();
}

void iADetailView::ChangeMagicLensOpacity(int chg)
{
	//iASlicer* slicer = m_previewWidget->GetSlicer();
	iASlicerWidget * sliceWidget = dynamic_cast<iASlicerWidget *>(sender());
	if (!sliceWidget)
	{
		DEBUG_LOG("Invalid slice widget sender!");
		return;
	}
	sliceWidget->SetMagicLensOpacity(sliceWidget->GetMagicLensOpacity() + (chg*0.05));
}

void iADetailView::SetSliceNumber(int sliceNr)
{
	if (!m_magicLensData)
	{
		return;
	}
	iAChannelID id = ch_SE_DetailView;
	iASlicer* slicer = m_previewWidget->GetSlicer();
	vtkSmartPointer<vtkImageData> imageData = m_modalities->Get(m_magicLensCurrentModality)->GetImage();
	switch(slicer->GetMode())
	{
		case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}
	slicer->update();
}

void iADetailView::SetSlicerMode(int mode,int sliceNr)
{
	if (!m_magicLensData)
	{
		return;
	}
	iAChannelID id = ch_SE_DetailView;
	iASlicer* slicer = m_previewWidget->GetSlicer();
	vtkSmartPointer<vtkImageData> imageData = m_modalities->Get(m_magicLensCurrentModality)->GetImage();
	switch(mode)
	{
		case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}
	slicer->update();
}

void iADetailView::paintEvent(QPaintEvent * )
{
	QPainter p(this);
	if (m_showingClusterRepresentative)
	{		// the latest selected item is always the one with index 0!
		p.setPen(DefaultColors::ClusterSelectPen[0]);
	}
	else
	{
		p.setPen(DefaultColors::ImageSelectPen);
	}
	QRect sel(geometry());
	p.drawRect(sel);
}

int iADetailView::GetSliceNumber() const
{
	return m_previewWidget->GetSliceNumber();
}


void iADetailView::UpdateLikeHate(bool isLike, bool isHate)
{
	assert(!isLike || !isHate);
	if (isLike)	m_pbLike->setStyleSheet("qproperty-icon: url(:/images/GEMSe_like2.png); background-color: "+DefaultColors::BackgroundColorText+"; border:none;");
	else        m_pbLike->setStyleSheet("qproperty-icon: url(:/images/GEMSe_like.png); background-color: "+DefaultColors::BackgroundColorText+"; border:none;");
	if (isHate) m_pbHate->setStyleSheet("qproperty-icon: url(:/images/GEMSe_hate2.png); background-color: "+DefaultColors::BackgroundColorText+"; border:none;");
	else        m_pbHate->setStyleSheet("qproperty-icon: url(:/images/GEMSe_hate.png); background-color: "+DefaultColors::BackgroundColorText+"; border:none;");
}

QString attrValueStr(double value, QSharedPointer<iAAttributes> attributes, int id)
{
	switch(attributes->at(id)->GetValueType())
	{
		case Discrete:    return QString::number(static_cast<int>(value)); break;
		case Categorical: return attributes->at(id)->GetNameMapper()->GetName(static_cast<int>(value)); break;
		default:          return QString::number(value); break;
	}
}

void iADetailView::SetNode(iAImageClusterNode const * node)
{
	m_node = node;
	SetImage();
	m_showingClusterRepresentative = !node->IsLeaf();
	m_pbGoto->setVisible(node->IsLeaf());
	update();
	UpdateLikeHate(node->GetAttitude() == iAImageClusterNode::Liked, node->GetAttitude() == iAImageClusterNode::Hated);

	m_detailText->clear();
	m_detailText->append(QString("ID: ") + QString::number(node->GetID()));

	//m_detailText->setMinimumWidth(m_detailText->document()->textWidth()+m_detailText->contentsMargins().left() + m_detailText->contentsMargins().right());
	m_detailText->setMinimumWidth(50);
	if (node->IsLeaf())
	{
		iAImageClusterLeaf* leaf = (iAImageClusterLeaf*)node;
		QSharedPointer<iAAttributes> attributes = leaf->GetAttributes();
		for (int attributeID = 0; attributeID < attributes->size(); ++attributeID)
		{
			double value = node->GetAttribute(attributeID);
			QString valueStr = attrValueStr(value, attributes, attributeID);

			m_detailText->append(attributes->at(attributeID)->GetName() + " = " + valueStr);
		}
	}
	else
	{
		/*
		TODO: MULTIP find a way to get attributes here!
			- either all attributes (-> need to be passed in from the outside)
			- or only the ones actually in this cluster (collection might be complex & time-consuming)
		for (int chartID=0; chartID<m_chartAttributes->size(); ++chartID)
		{
								// v mapper!
			if (m_attributes->at(chartID)->GetValueType() != Categorical)
			{
				double min, max;
				GetClusterMinMax(node, id, min, max);
				if (min == max)
				{
					continue;
				}
				m_detailText->append(m_attributes->at(id)->GetName() + ": [" +
					attrValueStr(min, m_attributes, id) + ".." + attrValueStr(max, m_attributes, id) + "]");
			}
		}
		*/
		m_detailText->append("Maximum distance: "+QString::number(node->GetDistance()));
	}
}

bool iADetailView::IsShowingCluster() const
{
	return m_showingClusterRepresentative;
}

#include "iASlicerData.h"

void iADetailView::SetMagicLensOpacity(double opacity)
{
	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->SetMagicLensOpacity(opacity);
}


void iADetailView::SetLabelInfo(iALabelInfo const & labelInfo)
{
	m_labelItemModel->clear();
	for (int i=0; i<labelInfo.count(); ++i)
	{
		QStandardItem* item = new QStandardItem(labelInfo.GetName(i));
		item->setFlags(item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
		item->setData(labelInfo.GetColor(i), Qt::DecorationRole);
		m_labelItemModel->appendRow(item);
	}
	QStandardItem* diffItem = new QStandardItem("Difference");
	diffItem->setFlags(diffItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
	diffItem->setData(DefaultColors::DifferenceColor, Qt::DecorationRole);
	m_labelItemModel->appendRow(diffItem);
}


void iADetailView::SetRepresentativeType(int representativeType)
{
	m_representativeType = representativeType;
	SetImage();
}


void iADetailView::SetImage()
{
	m_previewWidget->SetImage(m_node->GetRepresentativeImage(m_representativeType) ?
		m_node->GetRepresentativeImage(m_representativeType) : m_nullImage,
		!m_node->GetRepresentativeImage(m_representativeType),
		m_node->IsLeaf() || m_representativeType == Difference || m_representativeType == AverageLabel);
}

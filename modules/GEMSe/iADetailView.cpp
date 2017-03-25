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
#include "iAAttributeDescriptor.h"
#include "iAChannelVisualizationData.h"
#include "iAConsole.h"
#include "iAGEMSeConstants.h"
#include "iAImageCoordinate.h"
#include "iAImageTree.h" // for GetClusterMinMax
#include "iAImageTreeLeaf.h"
#include "iAImagePreviewWidget.h"
#include "iALabelInfo.h"
#include "iALabelOverlayThread.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iANameMapper.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "iAVtkDraw.h"
#include "iAToolsVTK.h"

#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageWriter.h>
#include <vtkPiecewiseFunction.h>

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
		iAColorTheme const * colorTheme,
		int representativeType) :
	m_previewWidget(prevWdgt),
	m_showingClusterRepresentative(true),
	m_pbLike(new QPushButton("")),
	m_pbHate(new QPushButton("")),
	m_pbGoto(new QPushButton("")),
	m_nullImage(nullImage),
	m_modalities(modalities),
	m_magicLensCurrentModality(0),
	m_magicLensCurrentComponent(0),
	m_representativeType(representativeType), 
	m_nextChannelID(0),
	m_magicLensEnabled(false),
	m_magicLensCount(1),
	m_labelItemModel(new QStandardItemModel())
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

	m_lvLegend = new QListView();
	m_lvLegend->setModel(m_labelItemModel);
	detailSplitter->addWidget(m_lvLegend);
	SetLabelInfo(labelInfo, colorTheme);
	int height = m_labelItemModel->rowCount() * m_lvLegend->sizeHintForRow(0);
	m_lvLegend->setMinimumHeight(height);
	
	m_detailText = new QTextEdit();
	m_detailText->setReadOnly(true);
	m_detailText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	detailSplitter->addWidget(m_detailText);
	
	QFont f(m_detailText->font());
	f.setPointSize(FontSize);
	m_detailText->setFont(f);
	m_lvLegend->setFont(f);
	
	m_lvLegend->setStyleSheet("border: none; outline:none;");
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
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(oslicerPos(int, int, int, int)), this, SIGNAL(SlicerHover(int, int, int, int)));
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(clicked(int, int, int)), this, SLOT(SlicerClicked(int, int, int)));
}

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

void iADetailView::DblClicked()
{
	m_magicLensEnabled = !m_magicLensEnabled;
	if (m_magicLensEnabled)
	{
		ChangeModality(0);
	}
	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->SetMagicLensEnabled(m_magicLensEnabled);
}


void iADetailView::ChangeModality(int offset)
{
	iAChannelID removedID = static_cast<iAChannelID>(ch_Concentration0 + m_nextChannelID - m_magicLensCount);

	iAChannelID id = static_cast<iAChannelID>(ch_Concentration0 + m_nextChannelID);
	m_nextChannelID = (m_nextChannelID + 1) % 8;
	// TOOD: refactor to remove duplication between here and MdiChild::ChangeModality!
	m_magicLensCurrentComponent = (m_magicLensCurrentComponent + offset);
	if (m_magicLensCurrentComponent < 0 || m_magicLensCurrentComponent >= m_modalities->Get(m_magicLensCurrentModality)->ComponentCount())
	{
		m_magicLensCurrentComponent = 0;
		m_magicLensCurrentModality = (m_magicLensCurrentModality + offset + m_modalities->size()) % m_modalities->size();
	}
	iAChannelVisualizationData magicLensData;
	iASlicer* slicer = m_previewWidget->GetSlicer();
	QSharedPointer<iAModality> mod = m_modalities->Get(m_magicLensCurrentModality);
	vtkSmartPointer<vtkImageData> imageData = mod->GetComponent(m_magicLensCurrentComponent);
	m_ctf = (mod->GetName() == "Ground Truth") ?
		m_previewWidget->GetCTF().GetPointer() :
		mod->GetTransfer()->GetColorFunction();
	m_otf = (mod->GetName() == "Ground Truth") ?
		GetDefaultOTF(imageData).GetPointer() :
		mod->GetTransfer()->GetOpacityFunction();
	ResetChannel(&magicLensData, imageData, m_ctf, m_otf);
	QString name(mod->GetImageName(m_magicLensCurrentComponent));
	magicLensData.SetName(name);
	slicer->removeChannel(removedID);
	slicer->initializeChannel(id, &magicLensData);
	int sliceNr = m_previewWidget->GetSliceNumber();
	switch(slicer->GetMode())
	{
		case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * imageData->GetSpacing()[0], 0, 0); break;
		case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[2]); break;
		case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * imageData->GetSpacing()[1], 0); break;
	}
	slicer->SetMagicLensInput(id);
	slicer->update();
}

void iADetailView::ChangeMagicLensOpacity(int chg)
{
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
	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->update();
}

void iADetailView::SetSlicerMode(int mode,int sliceNr)
{
}

void iADetailView::UpdateMagicLensColors()
{
	m_previewWidget->GetSlicer()->GetSlicerData()->updateChannelMappers();
	m_previewWidget->GetSlicer()->UpdateMagicLensColors();
	m_previewWidget->GetSlicer()->update();

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

void iADetailView::SetNode(iAImageTreeNode const * node,
	QSharedPointer<iAAttributes> allAttributes,
	iAChartAttributeMapper const & mapper)
{
	m_node = node;
	SetImage();
	m_showingClusterRepresentative = !node->IsLeaf();
	m_pbGoto->setVisible(node->IsLeaf());
	update();
	UpdateLikeHate(node->GetAttitude() == iAImageTreeNode::Liked, node->GetAttitude() == iAImageTreeNode::Hated);
	m_detailText->clear();
	m_detailText->setMinimumWidth(50);
	if (node->IsLeaf())
	{
		iAImageTreeLeaf* leaf = (iAImageTreeLeaf*)node;
		m_detailText->append(QString("ID: %1-%2").arg(leaf->GetDatasetID()).arg(node->GetID()));
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
		QStringList idList;
		VisitLeafs(node, [&](iAImageTreeLeaf const * leaf)
		{
			idList << QString("%1-%2").arg(leaf->GetDatasetID()).arg(leaf->GetID());
		});
		m_detailText->append(QString("ID: %1 (%2)").arg(node->GetID()).arg(idList.join(",")));
		if (allAttributes)
		{
			for (int chartID = 0; chartID < allAttributes->size(); ++chartID)
			{
				if (allAttributes->at(chartID)->GetValueType() != Categorical)
				{
					double min,	max;
					GetClusterMinMax(node, chartID, min, max, mapper);
					if (min != std::numeric_limits<double>::max() ||
						max != std::numeric_limits<double>::lowest())
					{
						QString minStr = attrValueStr(min, allAttributes, chartID);
						QString maxStr = attrValueStr(max, allAttributes, chartID);
						QString rangeStr = (minStr == maxStr) ? minStr : "[" + minStr + ".." + maxStr + "]";
						m_detailText->append(allAttributes->at(chartID)->GetName() + ": " + rangeStr);
					}
				}
				// else { } // collect list of all categorical values used!
			}
		}
		if (node->IsLeaf() || node->GetChildCount() > 0)
			m_detailText->append("Maximum distance: "+QString::number(node->GetDistance()));
	}
}

bool iADetailView::IsShowingCluster() const
{
	return m_showingClusterRepresentative;
}

void iADetailView::SetMagicLensOpacity(double opacity)
{
	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->SetMagicLensOpacity(opacity);
}


void iADetailView::SetLabelInfo(iALabelInfo const & labelInfo, iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	m_labelCount = labelInfo.count();
	m_labelItemModel->clear();
	for (int i=0; i<m_labelCount; ++i)
	{
		QStandardItem* item = new QStandardItem(labelInfo.GetName(i));
		item->setData(labelInfo.GetColor(i), Qt::DecorationRole);
		m_labelItemModel->appendRow(item);
		item = new QStandardItem(labelInfo.GetName(i));
	}
	QStandardItem* diffItem = new QStandardItem("Difference");
	diffItem->setFlags(diffItem->flags() & ~Qt::ItemIsSelectable);
	diffItem->setData(DefaultColors::DifferenceColor, Qt::DecorationRole);
	m_labelItemModel->appendRow(diffItem);

	/* TODO: check this
	m_resultFilterOverlayLUT = BuildLabelOverlayLUT(m_labelCount, m_colorTheme);
	m_resultFilterOverlayOTF = BuildLabelOverlayOTF(m_labelCount);
	ResetChannel(m_resultFilterChannel.data(), m_resultFilterOverlayImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
	slicer->reinitializeChannel(id, m_resultFilterChannel.data());
	*/
}


void iADetailView::SetRepresentativeType(int representativeType)
{
	m_representativeType = representativeType;
	SetImage();
}


int iADetailView::GetRepresentativeType()
{
	return m_representativeType;
}


void iADetailView::SetImage()
{
	ClusterImageType img = m_node->GetRepresentativeImage(m_representativeType);
	m_spacing[0] = img->GetSpacing()[0]; m_spacing[1] = img->GetSpacing()[1]; m_spacing[2] = img->GetSpacing()[2];
	itk::ImageRegion<3> region = img->GetLargestPossibleRegion();
	itk::Size<3> size = region.GetSize();
	itk::Index<3> idx = region.GetIndex();
	m_dimensions[0] = size[0]; m_dimensions[1] = size[1]; m_dimensions[2] = size[2];
	m_previewWidget->SetImage(img ?
		img : m_nullImage,
		!img,
		m_node->IsLeaf() || m_representativeType == Difference || m_representativeType == AverageLabel);
}

void iADetailView::SetMagicLensCount(int count)
{
	m_magicLensCount = count;
	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->SetMagicLensCount(count);
}

QString iADetailView::GetLabelNames() const
{
	QStringList labelNames;
	for (int i = 0; i < m_labelItemModel->rowCount()-1; ++i)
	{
		labelNames.append(m_labelItemModel->item(i, 0)->text());
	}
	return labelNames.join(",");
}

namespace
{
	// TODO: unify with dlg_labels
	QStandardItem* GetCoordinateItem(int x, int y, int z)
	{
		QStandardItem* item = new QStandardItem("(" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ")");
		item->setData(x, Qt::UserRole + 1);
		item->setData(y, Qt::UserRole + 2);
		item->setData(z, Qt::UserRole + 3);
		return item;
	}
}

void iADetailView::SlicerClicked(int x, int y, int z)
{
	DEBUG_LOG(QString("Slicer clicked: %1, %2, %3").arg(x).arg(y).arg(z));
	int label = GetCurLabelRow();
	if (label == -1 || label == m_labelItemModel->rowCount()-1)
	{
		return;
	}
	if (!m_resultFilterOverlayImg)
	{
		m_resultFilterOverlayImg = AllocateImage(VTK_INT, m_dimensions, m_spacing);
		clearImage(m_resultFilterOverlayImg, m_labelCount);
		m_resultFilterOverlayLUT = BuildLabelOverlayLUT(m_labelCount, m_colorTheme);
		m_resultFilterOverlayOTF = BuildLabelOverlayOTF(m_labelCount);
	}
	drawPixel(m_resultFilterOverlayImg, x, y, z, label);
	vtkMetaImageWriter *metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName("test.mhd");
	metaImageWriter->SetInputData(m_resultFilterOverlayImg);
	metaImageWriter->SetCompression(false);
	metaImageWriter->Write();
	metaImageWriter->Delete();
	// for being able to undo:
	m_resultFilter.append(QPair<iAImageCoordinate, int>(iAImageCoordinate(x, y, z), label));
	iAChannelID id = static_cast<iAChannelID>(ch_LabelOverlay);
	iASlicer* slicer = m_previewWidget->GetSlicer();
	if (!m_resultFilterChannel)
	{
		m_resultFilterChannel = QSharedPointer<iAChannelVisualizationData>(new iAChannelVisualizationData);
		m_resultFilterChannel->SetName("Result Filter");
		ResetChannel(m_resultFilterChannel.data(), m_resultFilterOverlayImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		slicer->initializeChannel(id, m_resultFilterChannel.data());
	}
	else
	{
		ResetChannel(m_resultFilterChannel.data(), m_resultFilterOverlayImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		slicer->reInitializeChannel(id, m_resultFilterChannel.data());
	}
	int sliceNr = m_previewWidget->GetSliceNumber();
	switch (slicer->GetMode())
	{
	case YZ: slicer->GetSlicerData()->enableChannel(id, true, static_cast<double>(sliceNr) * m_spacing[0], 0, 0); break;
	case XY: slicer->GetSlicerData()->enableChannel(id, true, 0, 0, static_cast<double>(sliceNr) * m_spacing[2]); break;
	case XZ: slicer->GetSlicerData()->enableChannel(id, true, 0, static_cast<double>(sliceNr) * m_spacing[1], 0); break;
	}
	iAChannelSlicerData* slicerChannel = slicer->GetChannel(id);
	slicerChannel->updateMapper();
	slicerChannel->updateReslicer();
	slicer->update();
}


int iADetailView::GetCurLabelRow() const
{
	QModelIndexList indices = m_lvLegend->selectionModel()->selectedIndexes();
	if (indices.size() <= 0)
	{
		return -1;
	}
	QStandardItem* item = m_labelItemModel->itemFromIndex(indices[0]);
	while (item->parent() != nullptr)
	{
		item = item->parent();
	}
	return item->row();
}

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
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFakeTreeNode.h"
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
#include "iAToolsITK.h"
#include "iAToolsVTK.h"

#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageWriter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkObjectFactory.h>

#include <QLabel>
#include <QListView>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextEdit>

iADetailView::iADetailView(
		iAImagePreviewWidget* prevWdgt,
		iAImagePreviewWidget* compareWdgt,
		ClusterImageType nullImage,
		QSharedPointer<iAModalityList> modalities,
		iALabelInfo const & labelInfo,
		iAColorTheme const * colorTheme,
		int representativeType,
		QWidget* comparisonDetailsWidget) :
	m_previewWidget(prevWdgt),
	m_compareWidget(compareWdgt),
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
	m_labelItemModel(new QStandardItemModel()),
	m_MouseButtonDown(false),
	m_resultFilterTriggerThread(0),
	m_correctnessUncertaintyOverlayEnabled(false),
	m_cmpDetailsWidget(comparisonDetailsWidget)
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
	buttonBar->setFixedHeight(25);

	prevWdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QWidget* topSpacer = new QWidget();
	topSpacer->setFixedHeight(18);

	QVBoxLayout* lay = new QVBoxLayout();
	lay->setSpacing(1);
	lay->setMargin(1);
	lay->addWidget(topSpacer);
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

	QPushButton* resetResultFilterButton = new QPushButton("Reset Result Filter");
	resetResultFilterButton->setMinimumHeight(10);
	resetResultFilterButton->setMaximumHeight(25);
	detailSplitter->addWidget(resetResultFilterButton);
	
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

	m_compareWidget->SetImage(m_nullImage, true, false);

	QWidget* buttonBarStandin = new QWidget();
	buttonBarStandin->setFixedHeight(25);

	QVBoxLayout* comparisonLayout = new QVBoxLayout();
	comparisonLayout->setSpacing(0);
	comparisonLayout->setContentsMargins(0, 0, 0, 0);
	comparisonLayout->addWidget(m_compareWidget);
	comparisonLayout->addWidget(buttonBarStandin);

	QWidget *comparisonContainer = new QWidget();
	comparisonContainer->setLayout(comparisonLayout);

	QTabWidget * tw = new QTabWidget();
	tw->setContentsMargins(0, 0, 0, 0);
	tw->addTab(detailWidget, "Details");
	tw->addTab(comparisonContainer, "Comparison");

	QSplitter * horzSplitter = new QSplitter();
	
	QHBoxLayout* mainLay = new QHBoxLayout();
	mainLay->setSpacing(1);
	mainLay->setMargin(1);
	mainLay->addWidget(horzSplitter);

	horzSplitter->addWidget(imgStuffWidget);
	horzSplitter->addWidget(tw);
	horzSplitter->setStretchFactor(0, 2);
	horzSplitter->setStretchFactor(1, 1);
		
	QWidget * mainWdgt(this);
	mainWdgt->setLayout(mainLay);
	QRect geom(geometry());
	geom.adjust(+1, +1, -1, -1);
	mainWdgt->setGeometry(geom);

	m_cmpDetailsWidget->layout()->addWidget(m_cmpDetailsLabel = new QLabel());
	m_cmpDetailsLabel->setWordWrap(true);

	// prevWdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(resetResultFilterButton, SIGNAL(clicked()), this, SLOT(ResetResultFilter()));
	connect(m_pbLike, SIGNAL(clicked()), this, SIGNAL(Like()));
	connect(m_pbHate, SIGNAL(clicked()), this, SIGNAL(Hate()));
	connect(m_pbGoto, SIGNAL(clicked()), this, SIGNAL(GoToCluster()));
	connect(m_compareWidget, SIGNAL(Updated()), this, SIGNAL(ViewUpdated()));
	connect(m_previewWidget, SIGNAL(Updated()), this, SIGNAL(ViewUpdated()));

	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(DblClicked()), this, SLOT(DblClicked()));
	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeModality(int)));
	connect(m_previewWidget->GetSlicer()->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(oslicerPos(int, int, int, int)), this, SIGNAL(SlicerHover(int, int, int, int)));
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(oslicerPos(int, int, int, int)), this, SLOT(SlicerMouseMove(int, int, int, int)));
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(clicked(int, int, int)), this, SLOT(SlicerClicked(int, int, int)));
	connect(m_previewWidget->GetSlicer()->GetSlicerData(), SIGNAL(released(int, int, int)), this, SLOT(SlicerReleased(int, int, int)));
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
	// TOOD: refactor to remove duplication between here and MdiChild::ChangeModality!
	m_magicLensCurrentComponent = (m_magicLensCurrentComponent + offset);
	if (m_magicLensCurrentComponent < 0 || m_magicLensCurrentComponent >= m_modalities->Get(m_magicLensCurrentModality)->ComponentCount())
	{
		m_magicLensCurrentComponent = 0;
		m_magicLensCurrentModality = (m_magicLensCurrentModality + offset + m_modalities->size()) % m_modalities->size();
	}
	QSharedPointer<iAModality> mod = m_modalities->Get(m_magicLensCurrentModality);
	vtkSmartPointer<vtkImageData> imageData = mod->GetComponent(m_magicLensCurrentComponent);
	vtkColorTransferFunction* ctf = (mod->GetName() == "Ground Truth") ?
		m_previewWidget->GetCTF().GetPointer() :
		mod->GetTransfer()->GetColorFunction();
	vtkPiecewiseFunction* otf = (mod->GetName() == "Ground Truth") ?
		GetDefaultOTF(imageData).GetPointer() :
		mod->GetTransfer()->GetOpacityFunction();
	QString name(mod->GetImageName(m_magicLensCurrentComponent));
	AddMagicLensInput(imageData, ctf, otf, name);
}

void iADetailView::AddMagicLensInput(vtkSmartPointer<vtkImageData> img, vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, QString const & name)
{
	iAChannelID removedID = static_cast<iAChannelID>(ch_Concentration0 + m_nextChannelID - m_magicLensCount);

	iAChannelID id = static_cast<iAChannelID>(ch_Concentration0 + m_nextChannelID);
	m_nextChannelID = (m_nextChannelID + 1) % 8;
	iAChannelVisualizationData magicLensData;
	ResetChannel(&magicLensData, img, ctf, otf);
	magicLensData.SetName(name);

	iASlicer* slicer = m_previewWidget->GetSlicer();
	slicer->removeChannel(removedID);
	slicer->initializeChannel(id, &magicLensData);
	int sliceNr = m_previewWidget->GetSliceNumber();
	switch (slicer->GetMode())
	{
	case YZ: slicer->setResliceChannelAxesOrigin(id, static_cast<double>(sliceNr) * img->GetSpacing()[0], 0, 0); break;
	case XY: slicer->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sliceNr) * img->GetSpacing()[2]); break;
	case XZ: slicer->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sliceNr) * img->GetSpacing()[1], 0); break;
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


int iADetailView::GetSliceNumber() const
{
	return m_previewWidget->GetSliceNumber();
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
	UpdateComparisonNumbers();
}


void iADetailView::SetCompareNode(iAImageTreeNode const * node)
{
	m_compareNode = node;
	ClusterImageType img = m_compareNode->GetRepresentativeImage(m_representativeType, m_refImg);
	if (!img)
	{
		return;
	}
	auto pixelType = GetITKScalarPixelType(img);
	m_compareWidget->SetImage(img ?
		img : m_nullImage,
		!img,
		(pixelType != itk::ImageIOBase::FLOAT && pixelType != itk::ImageIOBase::DOUBLE));

	iAConnector con;
	con.SetImage(img);
	vtkSmartPointer<vtkImageData> vtkImg = con.GetVTKImage();
	// determine CTF/OTF from image / settings?
	vtkColorTransferFunction* ctf = m_compareWidget->GetCTF().GetPointer();
	vtkPiecewiseFunction* otf = GetDefaultOTF(vtkImg);
	QString name;
	if (dynamic_cast<iAFakeTreeNode const*>(node))
	{
		name = dynamic_cast<iAFakeTreeNode const*>(node)->GetName();
	}
	else
	{
		name = QString("Ensemble member %1").arg(node->GetID());
	}
	AddMagicLensInput(vtkImg, ctf, otf, name);

	UpdateComparisonNumbers();
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

	if (m_resultFilterImg)
	{
		m_resultFilterOverlayLUT = BuildLabelOverlayLUT(m_labelCount, m_colorTheme);
		m_resultFilterOverlayOTF = BuildLabelOverlayOTF(m_labelCount);
		ResetChannel(m_resultFilterChannel.data(), m_resultFilterImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		iASlicer* slicer = m_previewWidget->GetSlicer();
		slicer->reInitializeChannel(ch_LabelOverlay, m_resultFilterChannel.data());
		slicer->update();
	}
}


void iADetailView::SetRepresentativeType(int representativeType)
{
	m_representativeType = representativeType;
	SetImage();
}


void iADetailView::SetCorrectnessUncertaintyOverlay(bool enabled)
{
	m_correctnessUncertaintyOverlayEnabled = enabled;
	if (enabled)
	{
		m_previewWidget->AddNoMapperChannel(m_node->GetCorrectnessEntropyImage(m_refImg));
	}
	else
	{
		m_previewWidget->RemoveChannel();
	}
}


int iADetailView::GetRepresentativeType()
{
	return m_representativeType;
}


void iADetailView::SetRefImg(LabelImagePointer refImg)
{
	m_refImg = refImg;
}


void iADetailView::SetImage()
{
	ClusterImageType img = m_node->GetRepresentativeImage(m_representativeType, m_refImg);
	if (!img)
	{
		return;
	}
	// {
	// initialization of m_spacing / m_dimension -> does it need to happen every time?
	m_spacing[0] = img->GetSpacing()[0]; m_spacing[1] = img->GetSpacing()[1]; m_spacing[2] = img->GetSpacing()[2];
	itk::ImageRegion<3> region = img->GetLargestPossibleRegion();
	itk::Size<3> size = region.GetSize();
	itk::Index<3> idx = region.GetIndex();
	m_dimensions[0] = size[0]; m_dimensions[1] = size[1]; m_dimensions[2] = size[2];
	// }
	m_previewWidget->SetImage(img ?
		img : m_nullImage,
		!img,
		(m_node->IsLeaf() && m_representativeType != AverageEntropy) || m_representativeType == Difference || m_representativeType == AverageLabel);
	if (m_correctnessUncertaintyOverlayEnabled)
	{
		m_previewWidget->RemoveChannel();
		vtkSmartPointer<vtkImageData> img = m_node->GetCorrectnessEntropyImage(m_refImg);
		if (img)
		{
			m_previewWidget->AddNoMapperChannel(img);
		}
	}
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


class iAvtkImageData: public vtkImageData
{
public:
	static iAvtkImageData *New();
	vtkTypeMacro(iAvtkImageData, vtkImageData);
	void SetScalarRange(int min, int max)
	{
		ScalarRangeComputeTime.Modified();
		ScalarRange[0] = min;
		ScalarRange[1] = max;
	}
};

vtkStandardNewMacro(iAvtkImageData);


class iATimedEvent: public QThread
{
public:
	iATimedEvent(int delayMS) :
		m_waitTimeMS(0),
		m_delayMS(delayMS)
	{}
	void restart()
	{
		QMutexLocker locker(&mutex);
		m_waitTimeMS = 0;
	}
	void run()
	{
		const int WaitTime = 10;
		while (m_waitTimeMS < m_delayMS)
		{
			msleep(WaitTime);
			QMutexLocker locker(&mutex);
			m_waitTimeMS += WaitTime;
		}
	}
private:
	QMutex mutex;
	int m_waitTimeMS;
	int m_delayMS;
};


void iADetailView::SlicerClicked(int x, int y, int z)
{
	AddResultFilterPixel(x, y, z);
	if (!m_resultFilterTriggerThread)
	{
		m_resultFilterTriggerThread = new iATimedEvent(5000);
		connect(m_resultFilterTriggerThread, SIGNAL(finished()), this, SLOT(TriggerResultFilterUpdate()));
		m_resultFilterTriggerThread->start();
	}
	m_MouseButtonDown = true;
}


void iADetailView::TriggerResultFilterUpdate()
{
	m_resultFilterTriggerThread = 0;
	emit ResultFilterUpdate();
}


void iADetailView::SlicerReleased(int x, int y, int z)
{
	m_MouseButtonDown = false;
}


void iADetailView::SlicerMouseMove(int x, int y, int z, int c)
{
	if (m_MouseButtonDown)
	{
		AddResultFilterPixel(x, y, z);
		if (!m_resultFilterTriggerThread)
			DEBUG_LOG("Result Filter Trigger not yet started....");
		else
			m_resultFilterTriggerThread->restart();
	}
}


void iADetailView::AddResultFilterPixel(int x, int y, int z)
{
	if (x == m_lastResultFilterX && y == m_lastResultFilterY && z == m_lastResultFilterZ)
	{
		return;
	}
	int label = GetCurLabelRow();
	if (label == -1 || label == m_labelItemModel->rowCount() - 1)
	{
		return;
	}
	if (!m_resultFilterImg)
	{
		m_resultFilterImg = vtkSmartPointer<iAvtkImageData>::New();
		m_resultFilterImg->SetDimensions(m_dimensions);
		m_resultFilterImg->AllocateScalars(VTK_INT, 1);
		m_resultFilterImg->SetSpacing(m_spacing);
		clearImage(m_resultFilterImg, m_labelCount);
		m_resultFilterOverlayLUT = BuildLabelOverlayLUT(m_labelCount, m_colorTheme);
		m_resultFilterOverlayOTF = BuildLabelOverlayOTF(m_labelCount);
	}
	drawPixel(m_resultFilterImg, x, y, z, label);
	m_resultFilterImg->Modified();
	m_resultFilterImg->SetScalarRange(0, m_labelCount);
	m_resultFilter.append(QPair<iAImageCoordinate, int>(iAImageCoordinate(x, y, z), label));

	iAChannelID id = static_cast<iAChannelID>(ch_LabelOverlay);
	iASlicer* slicer = m_previewWidget->GetSlicer();
	if (!m_resultFilterChannel)
	{
		m_resultFilterChannel = QSharedPointer<iAChannelVisualizationData>(new iAChannelVisualizationData);
		m_resultFilterChannel->SetName("Result Filter");
		ResetChannel(m_resultFilterChannel.data(), m_resultFilterImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		slicer->initializeChannel(id, m_resultFilterChannel.data());

		int sliceNr = m_previewWidget->GetSliceNumber();
		switch (slicer->GetMode())
		{
		case YZ: slicer->GetSlicerData()->enableChannel(id, true, static_cast<double>(sliceNr) * m_spacing[0], 0, 0); break;
		case XY: slicer->GetSlicerData()->enableChannel(id, true, 0, 0, static_cast<double>(sliceNr) * m_spacing[2]); break;
		case XZ: slicer->GetSlicerData()->enableChannel(id, true, 0, static_cast<double>(sliceNr) * m_spacing[1], 0); break;
		}
	}
	slicer->update();
}


void iADetailView::ResetResultFilter()
{
	if (m_resultFilterImg)
	{
		clearImage(m_resultFilterImg, m_labelCount);
		m_resultFilterImg->Modified();
		iASlicer* slicer = m_previewWidget->GetSlicer();
		slicer->update();
		m_resultFilter.clear();
		emit ResultFilterUpdate();
	}
}


iAResultFilter const & iADetailView::GetResultFilter() const
{
	return m_resultFilter;
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


#include "itkLabelOverlapMeasuresImageFilter.h"

void iADetailView::UpdateComparisonNumbers()
{
	/* calculate (if both integer types)
		- dice metric between selected 
		- equal pixels
		- 
	*/
	if (m_compareWidget->Empty())
	{
		return;
	}
	vtkImageData* left = m_previewWidget->GetImage();
	vtkImageData* right = m_compareWidget->GetImage();

	if (left->GetScalarType() != VTK_INT)
	{
		m_cmpDetailsLabel->setText("Left image is not of int type!");
		return;
	}
	if (right->GetScalarType() != VTK_INT)
	{
		m_cmpDetailsLabel->setText("Right image is not of int type!");
		return;
	}
	iAConnector leftCon;  leftCon.SetImage(left);  auto itkLeft = dynamic_cast<LabelImageType*>(leftCon.GetITKImage());
	iAConnector rightCon; rightCon.SetImage(right); auto itkRight = dynamic_cast<LabelImageType*>(rightCon.GetITKImage());

	if (!itkLeft)
	{
		m_cmpDetailsLabel->setText("Left itk image is not of int type!");
		return;
	}
	if (!itkRight)
	{
		m_cmpDetailsLabel->setText("Right itk image is not of int type!");
		return;
	}

	auto measureFilter = itk::LabelOverlapMeasuresImageFilter<LabelImageType>::New();
	measureFilter->SetSourceImage(itkLeft);
	measureFilter->SetTargetImage(itkRight);
	measureFilter->Update();

	const int UndecidedValue = m_labelCount;
	int* dims = left->GetDimensions();
	int* rightDims = right->GetDimensions();
	if (dims[0] != rightDims[0] || dims[1] != rightDims[1] || dims[2] != rightDims[2])
	{
		m_cmpDetailsLabel->setText(QString("Image dimensions don't match (%1, %2, %3) != (%4, %5, %6)!")
			.arg(dims[0]).arg(dims[1]).arg(dims[2])
			.arg(rightDims[0]).arg(rightDims[1]).arg(rightDims[2]));
		return;
	}
	long matching = 0;
	long leftUndecided = 0;
	long leftOnlyUndecided = 0;
	long rightUndecided = 0;
	long rightOnlyUndecided = 0;
	for (int x = 0; x < dims[0]; ++x)
	{
		for (int y = 0; y < dims[1]; ++y)
		{
			for (int z = 0; z < dims[2]; ++z)
			{
				int leftVal = left->GetScalarComponentAsDouble(x, y, z, 0);
				int rightVal = right->GetScalarComponentAsDouble(x, y, z, 0);
				if (leftVal == UndecidedValue)
				{
					leftUndecided++;
					if (rightVal != UndecidedValue)
					{
						leftOnlyUndecided++;
					}
				}
				if (rightVal == UndecidedValue)
				{
					rightUndecided++;
					if (leftVal != UndecidedValue)
					{
						rightOnlyUndecided++;
					}
				}
			}
		}
	}

	QString text;
	text += QString("Union Overlap (Jaccard): %1\n").arg(measureFilter->GetUnionOverlap());
	text += QString("Mean Overlap (Dice): %1\n").arg(measureFilter->GetMeanOverlap());
	text += QString("Total Overlap: %1\n").arg(measureFilter->GetTotalOverlap());
	text += QString("False Negatives: %1\n").arg(measureFilter->GetFalseNegativeError());
	text += QString("False Positives: %1\n").arg(measureFilter->GetFalsePositiveError());
	text += QString("Volume Similarity: %1\n").arg(measureFilter->GetVolumeSimilarity());

	//for (int i = 0; i < m_labelCount; ++i)
	//{
		//text += 		measureFilter->GetTargetOverlap
	//}
	
	text += QString("Undecided (left/right/only left/only right): %1/%2/%3/%4")
		.arg(leftUndecided)
		.arg(rightUndecided)
		.arg(leftOnlyUndecided)
		.arg(rightOnlyUndecided);
		
	m_cmpDetailsLabel->setText(text);
}

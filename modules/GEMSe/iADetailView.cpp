// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADetailView.h"

#include "iAAttributes.h"
#include "iAFakeTreeNode.h"
#include "iAGEMSeConstants.h"
#include "iAImageCoordinate.h"
#include "iAImageTree.h" // for GetClusterMinMax
#include "iAImageTreeLeaf.h"
#include "iAImagePreviewWidget.h"
#include "iALabelInfo.h"

#include <iAAttributeDescriptor.h>
#include <iAChannelData.h>
#include <iAConnector.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iALUT.h>
#include <iATransferFunction.h>
#include <iANameMapper.h>
#include <iAVtkDraw.h>
#include <iAToolsITK.h>
#include <iAToolsVTK.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

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
#include <QThread>

iADetailView::iADetailView(iAImagePreviewWidget* prevWdgt, iAImagePreviewWidget* compareWdgt,
	ClusterImageType nullImage, std::vector<std::shared_ptr<iADataSet>> const & dataSets,
	std::vector<iATransferFunction*> const& transfer, iALabelInfo const& labelInfo, iAColorTheme const* colorTheme,
	int representativeType, QWidget* comparisonDetailsWidget) :
	m_node(nullptr),
	m_compareNode(nullptr),
	m_previewWidget(prevWdgt),
	m_compareWidget(compareWdgt),
	m_pbLike(new QPushButton("")),
	m_pbHate(new QPushButton("")),
	m_pbGoto(new QPushButton("")),
	m_detailText(new QTextEdit()),
	m_lvLegend(new QListView()),
	m_cmpDetailsWidget(comparisonDetailsWidget),
	m_cmpDetailsLabel(new QLabel()),
	m_labelItemModel(new QStandardItemModel()),
	m_showingClusterRepresentative(true),
	m_dataSets(dataSets),
	m_transfer(transfer),
	m_nullImage(nullImage),
	m_representativeType(representativeType),
	m_magicLensDataSetIdx(0),
	m_magicLensEnabled(false),
	m_magicLensCount(1),
	m_colorTheme(nullptr),
	m_nextChannelID(0),
	m_resultFilterTriggerThread(nullptr),
	m_MouseButtonDown(false),
	m_correctnessUncertaintyOverlayEnabled(false)
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
	lay->setContentsMargins(1, 1, 1, 1);
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

	m_lvLegend->setModel(m_labelItemModel);
	detailSplitter->addWidget(m_lvLegend);
	SetLabelInfo(labelInfo, colorTheme);
	int height = m_labelItemModel->rowCount() * m_lvLegend->sizeHintForRow(0);
	m_lvLegend->setMinimumHeight(height);

	QPushButton* resetResultFilterButton = new QPushButton("Reset Result Filter");
	resetResultFilterButton->setMinimumHeight(10);
	resetResultFilterButton->setMaximumHeight(25);
	detailSplitter->addWidget(resetResultFilterButton);

	m_detailText->setReadOnly(true);
	m_detailText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	detailSplitter->addWidget(m_detailText);

	QFont f(m_detailText->font());
	f.setPointSize(FontSize);
	m_detailText->setFont(f);
	m_lvLegend->setFont(f);

	m_lvLegend->setStyleSheet("border: none; outline:none;");
	m_detailText->setStyleSheet("border: none; outline:none;");

	m_compareWidget->setImage(m_nullImage, true, false);

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
	mainLay->setContentsMargins(1, 1, 1, 1);
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

	m_cmpDetailsWidget->layout()->addWidget(m_cmpDetailsLabel);
	m_cmpDetailsLabel->setWordWrap(true);

	// prevWdgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	connect(resetResultFilterButton, &QPushButton::clicked, this, &iADetailView::ResetResultFilter);
	connect(m_pbLike, &QPushButton::clicked, this, &iADetailView::Like);
	connect(m_pbHate, &QPushButton::clicked, this, &iADetailView::Hate);
	connect(m_pbGoto, &QPushButton::clicked, this, &iADetailView::GoToCluster);
	connect(m_compareWidget, &iAImagePreviewWidget::updated, this, &iADetailView::ViewUpdated);
	connect(m_previewWidget, &iAImagePreviewWidget::updated, this, &iADetailView::ViewUpdated);

	connect(m_previewWidget->slicer(), &iASlicer::dblClicked, this, &iADetailView::dblClicked);
	connect(m_previewWidget->slicer(), &iASlicer::shiftMouseWheel, this, &iADetailView::changeDataSet);
	connect(m_previewWidget->slicer(), &iASlicer::altMouseWheel, this, &iADetailView::changeMagicLensOpacity);
	connect(m_previewWidget->slicer(), &iASlicer::mouseMoved, this, &iADetailView::SlicerHover);
	connect(m_previewWidget->slicer(), &iASlicer::mouseMoved, this, &iADetailView::SlicerMouseMove);
	connect(m_previewWidget->slicer(), &iASlicer::leftClicked, this, &iADetailView::SlicerClicked);
	connect(m_previewWidget->slicer(), &iASlicer::leftReleased, this, &iADetailView::SlicerReleased);
}


vtkSmartPointer<vtkColorTransferFunction> GetDefaultCTF(vtkSmartPointer<vtkImageData> imageData)
{
	auto ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
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


void iADetailView::dblClicked()
{
	m_magicLensEnabled = !m_magicLensEnabled;
	if (m_magicLensEnabled)
	{
		changeDataSet(0);
	}
	iASlicer* slicer = m_previewWidget->slicer();
	slicer->setMagicLensEnabled(m_magicLensEnabled);
}


void iADetailView::changeDataSet(int offset)
{
	// TOOD: refactor to remove duplication between here and iAMdiChild::changeModality!
	m_magicLensDataSetIdx = (m_magicLensDataSetIdx + offset + m_dataSets.size()) % m_dataSets.size();
	auto ds = m_dataSets[m_magicLensDataSetIdx];
	auto imageData = dynamic_cast<iAImageData*>(ds.get())->vtkImage();
	vtkColorTransferFunction* ctf = (ds->name() == "Ground Truth") ?
		m_previewWidget->colorTF().GetPointer() :
		m_transfer[m_magicLensDataSetIdx]->colorTF();
	vtkPiecewiseFunction* otf = (ds->name() == "Ground Truth") ?
		GetDefaultOTF(imageData).GetPointer() :
		m_transfer[m_magicLensDataSetIdx]->opacityTF();
	QString name(ds->name());
	AddMagicLensInput(imageData, ctf, otf, name);
}

void iADetailView::AddMagicLensInput(vtkSmartPointer<vtkImageData> img, vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, QString const & name)
{
	uint removedID = (m_nextChannelID - m_magicLensCount);

	uint id = m_nextChannelID;
	m_nextChannelID = (m_nextChannelID + 1) % 8;
	iAChannelData magicLensData(name, img, ctf, otf);
	iASlicer* slicer = m_previewWidget->slicer();
	slicer->removeChannel(removedID);
	slicer->addChannel(id, magicLensData, false);
	slicer->setMagicLensInput(id);
	slicer->update();
}


void iADetailView::changeMagicLensOpacity(int chg)
{
	m_previewWidget->slicer()->setMagicLensOpacity(m_previewWidget->slicer()->magicLensOpacity() + (chg*0.05));
}


void iADetailView::setSliceNumber(int /*sliceNr*/)
{
	iASlicer* slicer = m_previewWidget->slicer();
	slicer->update();
}


int iADetailView::sliceNumber() const
{
	return m_previewWidget->sliceNumber();
}


void iADetailView::UpdateMagicLensColors()
{
	m_previewWidget->slicer()->updateChannelMappers();
	m_previewWidget->slicer()->updateMagicLensColors();
	m_previewWidget->slicer()->update();

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


QString attrValueStr(double value, std::shared_ptr<iAAttributes> attributes, int id)
{
	switch(attributes->at(id)->valueType())
	{
		case iAValueType::Discrete:    return QString::number(static_cast<int>(value));
		case iAValueType::Categorical: return attributes->at(id)->nameMapper()->name(static_cast<int>(value));
		default:                       return QString::number(value);
	}
}


void iADetailView::SetNode(iAImageTreeNode const * node,
	std::shared_ptr<iAAttributes> allAttributes,
	iAChartAttributeMapper const & mapper)
{
	m_node = node;
	setImage();
	m_showingClusterRepresentative = !node->IsLeaf();
	m_pbGoto->setVisible(node->IsLeaf());
	update();
	UpdateLikeHate(node->GetAttitude() == iAImageTreeNode::Liked, node->GetAttitude() == iAImageTreeNode::Hated);
	m_detailText->clear();
	m_detailText->setMinimumWidth(50);
	if (node->IsLeaf())
	{
		auto leaf = dynamic_cast<iAImageTreeLeaf const *>(node);
		assert(node);
		m_detailText->append(QString("ID: %1-%2").arg(leaf->GetDatasetID()).arg(node->GetID()));
		std::shared_ptr<iAAttributes> attributes = leaf->GetAttributes();
		for (int attributeID = 0; attributeID < attributes->size(); ++attributeID)
		{
			double value = node->GetAttribute(attributeID);
			QString valueStr = attrValueStr(value, attributes, attributeID);

			m_detailText->append(attributes->at(attributeID)->name() + " = " + valueStr);
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
				if (allAttributes->at(chartID)->valueType() != iAValueType::Categorical)
				{
					double min,	max;
					GetClusterMinMax(node, chartID, min, max, mapper);
					if (min != std::numeric_limits<double>::max() ||
						max != std::numeric_limits<double>::lowest())
					{
						QString minStr = attrValueStr(min, allAttributes, chartID);
						QString maxStr = attrValueStr(max, allAttributes, chartID);
						QString rangeStr = (minStr == maxStr) ? minStr : "[" + minStr + ".." + maxStr + "]";
						m_detailText->append(allAttributes->at(chartID)->name() + ": " + rangeStr);
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
	auto pixelType = itkScalarType(img);
	m_compareWidget->setImage(img ?
		img : m_nullImage,
		!img,
		(pixelType != iAITKIO::ScalarType::FLOAT && pixelType != iAITKIO::ScalarType::DOUBLE));

	iAConnector con;
	con.setImage(img);
	vtkSmartPointer<vtkImageData> vtkImg = con.vtkImage();
	// determine CTF/OTF from image / settings?
	vtkColorTransferFunction* ctf = m_compareWidget->colorTF().GetPointer();
	vtkPiecewiseFunction* otf = GetDefaultOTF(vtkImg);
	QString name;
	if (dynamic_cast<iAFakeTreeNode const*>(node))
	{
		name = dynamic_cast<iAFakeTreeNode const*>(node)->name();
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
	iASlicer* slicer = m_previewWidget->slicer();
	slicer->setMagicLensOpacity(opacity);
}

namespace
{
	const uint ResultFilterChannelID = 9;
}

void iADetailView::SetLabelInfo(iALabelInfo const & labelInfo, iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	m_labelCount = labelInfo.count();
	m_labelItemModel->clear();
	for (int i=0; i<m_labelCount; ++i)
	{
		QStandardItem* item = new QStandardItem(labelInfo.name(i));
		item->setData(labelInfo.color(i), Qt::DecorationRole);
		m_labelItemModel->appendRow(item);
		item = new QStandardItem(labelInfo.name(i));
	}
	QStandardItem* diffItem = new QStandardItem("Difference");
	diffItem->setFlags(diffItem->flags() & ~Qt::ItemIsSelectable);
	diffItem->setData(DefaultColors::DifferenceColor, Qt::DecorationRole);
	m_labelItemModel->appendRow(diffItem);

	if (m_resultFilterImg)
	{
		m_resultFilterOverlayLUT = iALUT::BuildLabelColorTF(m_labelCount, m_colorTheme);
		m_resultFilterOverlayOTF = iALUT::BuildLabelOpacityTF(m_labelCount);
		m_resultFilterChannel->setData(m_resultFilterImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		iASlicer* slicer = m_previewWidget->slicer();
		slicer->updateChannel(ResultFilterChannelID, *m_resultFilterChannel.get());
		slicer->update();
	}
}


void iADetailView::SetRepresentativeType(int representativeType)
{
	m_representativeType = representativeType;
	setImage();
}


void iADetailView::SetCorrectnessUncertaintyOverlay(bool enabled)
{
	if (!m_refImg)
	{
		LOG(lvlError, "Reference image must be set!");
		return;
	}
	m_correctnessUncertaintyOverlayEnabled = enabled;
	if (enabled)
	{
		m_previewWidget->addNoMapperChannel(m_node->GetCorrectnessEntropyImage(m_refImg));
	}
	else
	{
		m_previewWidget->removeChannel();
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


void iADetailView::setImage()
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
	m_dimensions[0] = static_cast<int>(size[0]);
	m_dimensions[1] = static_cast<int>(size[1]);
	m_dimensions[2] = static_cast<int>(size[2]);
	// }
	m_previewWidget->setImage(img ?
		img : m_nullImage,
		!img,
		(m_node->IsLeaf() && m_representativeType != AverageEntropy) || m_representativeType == Difference || m_representativeType == AverageLabel);
	if (m_correctnessUncertaintyOverlayEnabled)
	{
		m_previewWidget->removeChannel();
		vtkSmartPointer<vtkImageData> entropyImg = m_node->GetCorrectnessEntropyImage(m_refImg);
		if (entropyImg)
		{
			m_previewWidget->addNoMapperChannel(entropyImg);
		}
	}
}

void iADetailView::setMagicLensCount(int count)
{
	m_magicLensCount = count;
	iASlicer* slicer = m_previewWidget->slicer();
	slicer->setMagicLensCount(count);
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
	void run() override
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


void iADetailView::SlicerClicked(double x, double y, double z)
{
	AddResultFilterPixel(x, y, z);
	if (!m_resultFilterTriggerThread)
	{
		m_resultFilterTriggerThread = new iATimedEvent(5000);
		connect(m_resultFilterTriggerThread, &iATimedEvent::finished, this, &iADetailView::TriggerResultFilterUpdate);
		m_resultFilterTriggerThread->start();
	}
	m_MouseButtonDown = true;
}


void iADetailView::TriggerResultFilterUpdate()
{
	m_resultFilterTriggerThread = nullptr;
	emit ResultFilterUpdate();
}


void iADetailView::SlicerReleased(double /*x*/, double /*y*/, double /*z*/)
{
	m_MouseButtonDown = false;
}


void iADetailView::SlicerMouseMove(double x, double y, double z, int /*c*/)
{
	if (m_MouseButtonDown)
	{
		AddResultFilterPixel(x, y, z);
		if (!m_resultFilterTriggerThread)
		{
			m_MouseButtonDown = false;
			//LOG(lvlError, "Result Filter Trigger not yet started....");
		}
		else
		{
			m_resultFilterTriggerThread->restart();
		}
	}
}


void iADetailView::AddResultFilterPixel(double x, double y, double z)
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
		m_resultFilterImg = allocateiAImage(VTK_INT, m_dimensions, m_spacing, 1);
		clearImage(m_resultFilterImg, 0);
		m_resultFilterOverlayLUT = iALUT::BuildLabelColorTF(m_labelCount, m_colorTheme);
		m_resultFilterOverlayOTF = iALUT::BuildLabelOpacityTF(m_labelCount);
	}
	double worldCoord[3] = { x, y, z };
	auto vxCoord = mapWorldCoordsToIndex(m_resultFilterImg, worldCoord);
	drawPixel(m_resultFilterImg, vxCoord[0], vxCoord[1], vxCoord[2], label + 1);
	m_resultFilterImg->Modified();
	m_resultFilterImg->SetScalarRange(0, m_labelCount);
	m_resultFilter.append(QPair<iAImageCoordinate, int>(iAImageCoordinate(vxCoord[0], vxCoord[1], vxCoord[2]), label));

	iASlicer* slicer = m_previewWidget->slicer();
	if (!m_resultFilterChannel)
	{
		m_resultFilterChannel = std::make_shared<iAChannelData>("Result Filter", m_resultFilterImg, m_resultFilterOverlayLUT, m_resultFilterOverlayOTF);
		slicer->addChannel(ResultFilterChannelID, *m_resultFilterChannel.get(), true);
	}
	slicer->update();
}


void iADetailView::ResetResultFilter()
{
	if (m_resultFilterImg)
	{
		clearImage(m_resultFilterImg, 0);
		m_resultFilterImg->Modified();
		iASlicer* slicer = m_previewWidget->slicer();
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


#include <itkLabelOverlapMeasuresImageFilter.h>

void iADetailView::UpdateComparisonNumbers()
{
	/* calculate (if both integer types)
		- dice metric between selected
		- equal pixels
		-
	*/
	if (m_compareWidget->empty())
	{
		return;
	}
	vtkImageData* left = m_previewWidget->image();
	vtkImageData* right = m_compareWidget->image();

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
	iAConnector leftCon;  leftCon.setImage(left);  auto itkLeft = dynamic_cast<LabelImageType*>(leftCon.itkImage());
	iAConnector rightCon; rightCon.setImage(right); auto itkRight = dynamic_cast<LabelImageType*>(rightCon.itkImage());

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

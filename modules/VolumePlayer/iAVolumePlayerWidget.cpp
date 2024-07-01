// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumePlayerWidget.h"

#include "ui_VolumePlayer.h"

#include "iAChannelData.h"
#include "iADataSetRenderer.h"
#include "iAImageData.h"
#include "iAMathUtility.h"
#include "iAMdiChild.h"
#include "iAParameterDlg.h"
#include "iASlicer.h"
#include "iATransferFunction.h"
#include "iAVolumeViewer.h"

#include <iALog.h>
#include <iAMathUtility.h>

#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QAction>
#include <QCheckBox>

#include <cassert>

const QString iAVolumePlayerTool::Name("VolumePlayer");

iAVolumePlayerTool::iAVolumePlayerTool(iAMainWindow* wnd, iAMdiChild* child):
	iATool(wnd, child)
{
	std::vector<iAVolumeViewer*> volumeViewers;
	for (auto ds : child->dataSetMap())
	{
		if (ds.second->type() == iADataSetType::Volume)
		{
			volumeViewers.push_back(dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(ds.first)));
		}
	}
	// hide all but first dataset:
	for (size_t v = 1; v < volumeViewers.size(); ++v)
	{
		volumeViewers[v]->renderer()->setVisible(false);
	}
	m_volumePlayer = new iAVolumePlayerWidget(child, volumeViewers);
	child->splitDockWidget(child->dataInfoDockWidget(), m_volumePlayer, Qt::Horizontal);
}

namespace
{
	const float TimerMinFPS = 0.25f;
	const float TimerMaxFPS = 20.0f;
	const int MilliSecondsPerSecond = 1000;
	const int BlendSteps = 100;
	const int NumberOfColumns = 5;
	const auto NoStep = -1;
	const auto NoVolIdx = std::numeric_limits<size_t>::max();
}

iAVolumePlayerWidget::iAVolumePlayerWidget(iAMdiChild *child, std::vector<iAVolumeViewer*> const& volumes)
	: QDockWidget(child),
	m_volumeViewers(volumes),
	m_ui(std::make_unique<Ui_VolumePlayer>()),
	m_prevStep(NoStep),
	m_prevVolIdx{ NoVolIdx, NoVolIdx },
	m_child(child)
{
	m_ui->setupUi(this);
	m_isBlendingOn = m_ui->blending->isChecked();
	setSpeed();

	m_ui->volumeSlider->setMaximum(static_cast<int>(m_volumeViewers.size() - 1));
	m_ui->volumeSlider->setMinimum(0);

	connect(m_ui->volumeSlider, &QSlider::valueChanged, this, &iAVolumePlayerWidget::sliderChanged);
	connect(m_ui->tbNext, &QToolButton::clicked, this, &iAVolumePlayerWidget::nextVolume);
	connect(m_ui->tbPrev, &QToolButton::clicked, this, &iAVolumePlayerWidget::previousVolume);
	connect(m_ui->tbPlay, &QToolButton::clicked, this, [this] {	m_timer.start(); });
	connect(m_ui->tbPause,&QToolButton::clicked, this, [this] { m_timer.stop(); });
	connect(m_ui->tbStop, &QToolButton::clicked, this, &iAVolumePlayerWidget::stopVolume);
	connect(m_ui->speedSlider, &QSlider::valueChanged, this, &iAVolumePlayerWidget::setSpeed);
	connect(m_ui->sbSpeed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &iAVolumePlayerWidget::speedEdited);
	connect(m_ui->dataTable, &QTableWidget::cellClicked, this, &iAVolumePlayerWidget::setChecked);
	//connect(m_ui->dataTable, &QTableWidget::cellDoubleClicked, this, &iAVolumePlayerWidget::updateView);
	connect(m_ui->tbApplyForAll, &QToolButton::clicked, this, &iAVolumePlayerWidget::applyForAll);
	connect(m_ui->dataTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &iAVolumePlayerWidget::selectAll);
	connect(&m_timer, &QTimer::timeout, this, &iAVolumePlayerWidget::nextVolume);
	connect(m_ui->blending, &QCheckBox::stateChanged, this, &iAVolumePlayerWidget::blendingStateChanged);

	m_ui->sbSpeed->setMinimum(TimerMinFPS);
	m_ui->sbSpeed->setMaximum(TimerMaxFPS);

	m_ui->dataTable->setShowGrid(true);
	m_ui->dataTable->setRowCount(static_cast<int>(m_volumeViewers.size()));
	m_ui->dataTable->setColumnCount(NumberOfColumns);
	auto headers = QStringList() <<"Select All"<<"Dim"<<"Spacing"<<"Filename";
	m_ui->dataTable->setHorizontalHeaderLabels(headers);

	int countNumber=0;
	m_checkColumn=countNumber++;
	m_dimColumn= countNumber++;
	m_spacColumn=countNumber++;
	m_fileColumn=countNumber++;
	m_sortColumn=countNumber++;

	for (size_t i=0; i< m_volumeViewers.size(); i++)
	{
		auto cb = new QCheckBox(this);
		cb->setObjectName(QString("check%1").arg(i));
		cb->setChecked(true);
		m_checkBoxes.push_back(cb);
		m_ui->dataTable->setCellWidget(static_cast<int>(i), m_checkColumn, cb);
		connect(cb, &QCheckBox::stateChanged, this, &iAVolumePlayerWidget::enableVolume);
		m_ui->dataTable->setRowHeight(static_cast<int>(i), 17);
		auto const dim = m_volumeViewers[i]->volume()->vtkImage()->GetDimensions();
		m_ui->dataTable->setItem(static_cast<int>(i), m_dimColumn, new QTableWidgetItem(QString("%1, %2, %3").arg(dim[0]).arg(dim[1]).arg(dim[2])));
		auto const spc = m_volumeViewers[i]->volume()->vtkImage()->GetSpacing();
		m_ui->dataTable->setItem(static_cast<int>(i), m_spacColumn,new QTableWidgetItem(QString("%1, %2, %3")	.arg(spc[0]).arg(spc[1]).arg(spc[2])));
		m_ui->dataTable->setItem(static_cast<int>(i), m_fileColumn,new QTableWidgetItem(m_volumeViewers[i]->volume()->metaData(iADataSet::FileNameKey).toString()));
		m_ui->dataTable->setItem(static_cast<int>(i), m_sortColumn,new QTableWidgetItem(QString("%1").arg(0)));
	}
	m_old_r=1;

	QFont font;
	font.setBold(true);
	m_ui->dataTable->horizontalHeaderItem(m_checkColumn)->setFont(font);
	m_ui->dataTable->setColumnHidden(m_sortColumn, true);

	//Contextmenu start
	m_contextDimensions = new QAction(tr("Dimensions"), this );
	connect( m_contextDimensions, &QAction::triggered, this, &iAVolumePlayerWidget::dimensionsActive);
	m_contextDimensions->setCheckable(true);
	m_contextDimensions->setChecked(true);
	m_contextSpacing = new QAction(tr("Spacing"), this );
	connect( m_contextSpacing, &QAction::triggered, this, &iAVolumePlayerWidget::spacingActive);
	m_contextSpacing->setCheckable(true);
	m_contextSpacing->setChecked(true);
	m_contextFileName = new QAction(tr("Filename"), this );
	connect( m_contextFileName, &QAction::triggered, this, &iAVolumePlayerWidget::fileNameActive);
	m_contextFileName->setCheckable(true);
	m_contextFileName->setChecked(true);
	setContextMenuPolicy( Qt::ActionsContextMenu );
	addAction( m_contextDimensions );
	addAction( m_contextSpacing );
	addAction( m_contextFileName );
	//Contextmenu end

	m_dimShow=m_spacShow=m_fileShow=true;
	m_ui->dataTable->resizeColumnsToContents();
}

iAVolumePlayerWidget::~iAVolumePlayerWidget() = default;

void iAVolumePlayerWidget::applyForAll()
{
	assert(!m_isBlendingOn);
	auto currentIdx = listToVolumeIndex(m_ui->volumeSlider->value());
	auto tf = m_volumeViewers[currentIdx]->transfer();
	for (size_t i = 0; i < m_volumeViewers.size(); ++i)
	{
		if (i != currentIdx)
		{
			m_volumeViewers[i]->transfer()->colorTF()->DeepCopy(tf->colorTF());
			m_volumeViewers[i]->transfer()->opacityTF()->DeepCopy(tf->opacityTF());
		}
	}
}

void iAVolumePlayerWidget::nextVolume()
{
	int index = m_ui->volumeSlider->value();
	index = (index + 1) % (m_ui->volumeSlider->maximum() + 1);
	m_ui->volumeSlider->setValue(index);    // triggers update
}

void iAVolumePlayerWidget::previousVolume()
{
	int index = m_ui->volumeSlider->value() - 1;
	if (index < 0)
	{
		index = m_ui->volumeSlider->maximum();
	}
	m_ui->volumeSlider->setValue(index);    // triggers update
}

void iAVolumePlayerWidget::sliderChanged()
{
	int step = m_ui->volumeSlider->value();
	//if (m_isBlendingOn)
	{
		//float listIdx = static_cast<float>(step / BlendSteps);
		//QSet<size_t> tohide;
		//if (m_prevVolIdx.first != NoVolIdx && m_prevVolIdx.second != NoVolIdx &&
		//	m_prevVolIdx.first != std::floor(listToVolumeIndex(listIdx)) || m_prevVolIdx.second != std::ceil(listToVolumeIndex(listIdx)))
		//{
		//	tohide.insert(m_prevVolIdx.first);
		//	tohide.insert(m_prevVolIdx.second);
		//}
		//if (step % BlendSteps == 0)
		//{
		//	tohide.insert(m_prevVolIdx.first);  // possibly not accurate, does this consider each case (automatic/manual switching?)
		//}
		//auto fadeOutVolIdx = listToVolumeIndex(std::floor(listIdx));
		//auto fadeInVolIdx = listToVolumeIndex(static_cast<int>(std::trunc(listIdx + 1)) % getNumberOfCheckedVolumes());
		//auto alpha = frac(listIdx);
		//QVector<size_t> toshow;
		//m_prevVolIdx.first = fadeOutVolIdx;
		//m_prevVolIdx.second = fadeInVolIdx;
		//
		//for (auto h : tohide)
		//{
		//	if (!toshow.contains(h))
		//	{
		//		m_volumeViewers[h]->renderer()->setVisible(false);
		//	}
		//}
		//for (auto s : toshow)
		//{
		//	m_volumeViewers[s]->renderer()->setVisible(true);
		//}
		//// actual blending: modify TF - store original somewhere...
		//for ()
		//m_volumeViewers[fadeOutVolIdx]->transfer()->opacityTF

	}
	//else
	{
		if (m_prevVolIdx.first != NoVolIdx)
		{
			m_volumeViewers[m_prevVolIdx.first]->renderer()->setVisible(false);
			m_volumeViewers[m_prevVolIdx.first]->showInSlicers(false);
		}
		auto volIdx = listToVolumeIndex(step);
		m_volumeViewers[volIdx]->renderer()->setVisible(true);
		m_child->updateRenderer();    // maybe do update in viewer?
		m_volumeViewers[volIdx]->showInSlicers(true);
		m_child->updateSlicers();     // maybe do update in viewer?
		m_prevVolIdx.first = volIdx;
	}
}

void iAVolumePlayerWidget::stopVolume()
{
	m_timer.stop();
	m_ui->volumeSlider->setValue(m_ui->volumeSlider->minimum());
}

void iAVolumePlayerWidget::setSpeed()
{
	m_timer.setInterval((int)((1/ currentSpeed()) * MilliSecondsPerSecond));
	QSignalBlocker block(m_ui->sbSpeed);
	m_ui->sbSpeed->setValue(currentSpeed());
}

void iAVolumePlayerWidget::speedEdited(double newSpeed)
{
	m_ui->speedSlider->setValue(mapValue(TimerMinFPS, TimerMaxFPS, m_ui->speedSlider->minimum(), m_ui->speedSlider->maximum(), static_cast<float>(newSpeed)));
}

void iAVolumePlayerWidget::setChecked(int r, int /*c*/)
{
	for (int i=1; i<NumberOfColumns;i++)
	{
		m_ui->dataTable->item(m_old_r,i)->setBackground(Qt::white);
		m_ui->dataTable->item(r,i)->setBackground(Qt::lightGray);
	}
	m_old_r=r;
}

void iAVolumePlayerWidget::selectAll(int c)
{
	if (c != m_checkColumn)
	{
		return;
	}
	for (size_t i=0; i< m_volumeViewers.size();i++)
	{
		m_checkBoxes[i]->setChecked(true);
	}
}

void iAVolumePlayerWidget::dimensionsActive()
{
	if (m_ui->dataTable->columnWidth(m_dimColumn)==0)
	{
		m_ui->dataTable->resizeColumnToContents(m_dimColumn);
		m_contextDimensions->setChecked(true);
	}
	else
	{
		m_ui->dataTable->setColumnWidth(m_dimColumn, 0);
		m_contextDimensions->setChecked(false);
	}
}

void iAVolumePlayerWidget::spacingActive()
{
	if (m_ui->dataTable->columnWidth(m_spacColumn)==0)
	{
		m_ui->dataTable->resizeColumnToContents(m_spacColumn);
		m_contextSpacing->setChecked(true);
	}
	else
	{
		m_ui->dataTable->setColumnWidth(m_spacColumn, 0);
		m_contextSpacing->setChecked(false);
	}
}

void iAVolumePlayerWidget::fileNameActive()
{
	if (m_ui->dataTable->columnWidth(m_fileColumn)==0)
	{
		m_ui->dataTable->resizeColumnToContents(m_fileColumn);
		m_contextFileName->setChecked(true);
	}
	else
	{
		m_ui->dataTable->setColumnWidth(m_fileColumn, 0);
		m_contextFileName->setChecked(false);
	}
}

float iAVolumePlayerWidget::currentSpeed() const
{
	return mapValue(m_ui->speedSlider->minimum(), m_ui->speedSlider->maximum(), TimerMinFPS, TimerMaxFPS, m_ui->speedSlider->value());
}

void iAVolumePlayerWidget::adjustSliderMax()
{
	m_ui->volumeSlider->setMaximum((getNumberOfCheckedVolumes() - 1) * (m_isBlendingOn ? BlendSteps : 1));
}

void iAVolumePlayerWidget::blendingStateChanged(int state)
{
	m_isBlendingOn = state;
	int oldVal = m_ui->volumeSlider->value();
	adjustSliderMax();
	m_ui->volumeSlider->setValue(m_isBlendingOn
		? oldVal * BlendSteps
		: std::floor(static_cast<double>(oldVal + 1) / BlendSteps + 0.5));
	m_ui->tbApplyForAll->setEnabled(!m_isBlendingOn);
}

void iAVolumePlayerWidget::enableVolume(int state)
{
	Q_UNUSED(state);
	int numOfCheckedVolumes = getNumberOfCheckedVolumes();
	for(size_t i = 0; i < m_checkBoxes.size(); ++i)
	{	// make sure one volume remains visible (disable last checked checkbox)
		m_checkBoxes[i]->setEnabled(numOfCheckedVolumes > 1 || !m_checkBoxes[i]->isChecked());
	}
	//int oldVal = m_ui->volumeSlider->value();
	adjustSliderMax();
	// TODO: keep current value (i.e. keep same value as before if checked is after current position, increase accordingly if before
	m_ui->volumeSlider->setValue(m_ui->volumeSlider->minimum());
}

size_t iAVolumePlayerWidget::listToVolumeIndex(int listIndex)
{
	int checkedVolumes = 0;
	for (size_t volIdx = 0; volIdx < m_checkBoxes.size(); ++volIdx)
	{
		if (m_checkBoxes[volIdx]->isChecked())
		{
			++checkedVolumes;
			if (checkedVolumes > listIndex)
			{
				return volIdx;
			}
		}
	}
	return NoVolIdx;
}

int iAVolumePlayerWidget::getNumberOfCheckedVolumes()
{
	int num = 0;
	for (size_t i = 0; i < m_checkBoxes.size(); ++i)
	{
		if (m_checkBoxes[i]->isChecked())
		{
			++num;
		}
	}
	return num;
}


/*
void iAVolumePlayerWidget::enableMultiChannelVisualization()
{
	if (!m_mdiChild)
	{
		return;
	}

	for (int i = 0; i < CHANNELS_COUNT; i++)
	{
		m_mdiChild->setChannelRenderingEnabled(m_channelID[i], true);
	}
}

void iAVolumePlayerWidget::disableMultiChannelVisualization()
{
	if (!m_mdiChild)
	{
		return;
	}

	for (int i = 0; i < CHANNELS_COUNT; i++)
	{
		m_mdiChild->setChannelRenderingEnabled(m_channelID[i], false);
	}
}

void iAVolumePlayerWidget::setMultiChannelVisualization(int volumeIndex1, int volumeIndex2, double blendingCoeff)
{
	if (!m_mdiChild)
	{
		return;
	}

	int volumeIndex[] = { volumeIndex1, volumeIndex2 };
	double opacity[] = { 1., blendingCoeff };

	if (!m_multiChannelIsInitialized)
	{
		for (int i = 0; i < CHANNELS_COUNT; i++)
		{
			m_channelID.push_back(m_mdiChild->createChannel());
		}
	}
	for(int i = 0; i < CHANNELS_COUNT; i++)
	{
		iAChannelData* chData = m_mdiChild->channelData(m_channelID[i]);
		vtkImageData* imageData = m_volumeStack->volume(volumeIndex[i]);
		vtkColorTransferFunction* ctf = m_volumeStack->colorTF(volumeIndex[i]);
		if(!m_multiChannelIsInitialized)
		{
			m_otf[i] = vtkSmartPointer<vtkPiecewiseFunction>::New();
		}
		m_otf[i]->ShallowCopy(m_volumeStack->opacityTF(volumeIndex[i]));

		for(int j = 0; j < m_otf[i]->GetSize(); j++)
		{
			double val[4];
			m_otf[i]->GetNodeValue(j, val);
			val[1] *= opacity[i];
			m_otf[i]->SetNodeValue(j, val);
		}

		chData->setData(imageData, ctf, m_otf[i]);

		if(!m_multiChannelIsInitialized)
		{
			m_mdiChild->initChannelRenderer(m_channelID[i], true);
			// TODO NEWIO: VOLUME: rewrite!
			// m_mdiChild->renderer()->showMainVolumeWithChannels(false);
		}
		m_mdiChild->updateChannelOpacity(m_channelID[i], opacity[i]);
	}

	if (!m_multiChannelIsInitialized)
	{
		m_multiChannelIsInitialized = true;
	}

//	m_mdiChild->renderer()->updateChannelImages();
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		m_mdiChild->slicer(i)->updateChannelMappers();
	}
	m_mdiChild->updateViews();
}

void iAVolumePlayerWidget::updateMultiChannelVisualization()
{
	double sliderVal = static_cast<double>(volumeSlider->value()) / DIVISIONS_PER_VOLUME;
	double blend = sliderVal - std::floor(sliderVal);
	if (volumeSlider->value() == volumeSlider->maximum())
	{
		setMultiChannelVisualization(sliderIndexToVolumeIndex(std::floor(sliderVal) - 1), sliderIndexToVolumeIndex(std::floor(sliderVal)), 1);
	}
	else
	{
		setMultiChannelVisualization(sliderIndexToVolumeIndex(std::floor(sliderVal)), sliderIndexToVolumeIndex(std::floor(sliderVal) + 1), blend);
	}
}
*/

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "dlg_volumePlayer.h"

#include "dlg_commoninput.h"
#include "iAChannelID.h"
#include "iAChannelVisualizationData.h"
#include "iARenderer.h"
#include "iASlicerData.h"
#include "iAVolumeStack.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QCheckBox>


const float TIMER_MIN_SPEED = 0.2f;	// frames per second
const float TIMER_MAX_SPEED = 50.0f;	// frames per second
const int SECONDS_TO_MILISECONDS = 1000;
const int DIVISIONS_PER_VOLUME = 200;

dlg_volumePlayer::dlg_volumePlayer(QWidget *parent, iAVolumeStack* volumeStack)
	: QDockWidget(parent),
	m_mask(0),
	m_volumeStack(volumeStack),
	m_multiChannelIsInitialized(false)
{
	setupUi(this);
	m_isBlendingOn = blending->isChecked();
	m_mdiChild = dynamic_cast<MdiChild*>(parent);
	m_numberOfVolumes=m_volumeStack->getNumberOfVolumes();
	for (int i = 0; i < m_numberOfVolumes; i++) {
		showVolume(i);
	}

	setSpeed();

	volumeSlider->setMaximum(m_numberOfVolumes-1);
	volumeSlider->setMinimum(0);

	connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged()));
	connect(nextVolumeButton, SIGNAL(clicked()),this, SLOT(nextVolume()));
	connect(previousVolumeButton, SIGNAL(clicked()),this, SLOT(previousVolume()));
	connect(playVolumeButton, SIGNAL(clicked()),this, SLOT(playVolume()));
	connect(pauseVolumeButton, SIGNAL(clicked()),this, SLOT(pauseVolume()));
	connect(stopVolumeButton, SIGNAL(clicked()),this, SLOT(stopVolume()));
	connect(speedSlider, SIGNAL(valueChanged(int)), this, SLOT(setSpeed()));
	connect(setMaxSpeedButton, SIGNAL(clicked()), this, SLOT(editMaxSpeed()));
	connect(dataTable, SIGNAL(cellClicked(int, int)), this, SLOT(setChecked(int, int)));
	connect(dataTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(updateView(int, int)));
	connect(applyForAllButton, SIGNAL(clicked()),this, SLOT(applyForAll()));
	connect(dataTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(selectAll(int)));
	connect(this, SIGNAL(setAllSelected(int)), this, SLOT(selectAll(int)));
	connect(this, SIGNAL(update(int, bool)), m_mdiChild, SLOT(updateVolumePlayerView(int, bool)));
	connect(&m_timer, SIGNAL(timeout()), this, SLOT(nextVolume()));
	connect(blending, SIGNAL(stateChanged(int)), this, SLOT(blendingStateChanged(int)));

	dataTable->setShowGrid(true);
	dataTable->setRowCount(m_numberOfVolumes);
	m_numberOfColumns=5;
	m_numberOfRows=m_numberOfVolumes;
	dataTable->setColumnCount(m_numberOfColumns);

	m_list<<"Select All"<<"Dim"<<"Spacing"<<"Filename";
	dataTable->setHorizontalHeaderLabels(m_list);

	int countNumber=0;
	m_checkColumn=countNumber++;
	m_dimColumn= countNumber++;
	m_spacColumn=countNumber++;
	m_fileColumn=countNumber++;
	m_sortColumn=countNumber++;

	for (int i=0; i<m_numberOfVolumes; i++) {
		m_tempStr=QString("%1").arg(i);
		m_newWidget = new QCheckBox(this);
		m_newWidget->setObjectName(m_tempStr.append("check"));
		m_checkBoxes.push_back((QCheckBox*)m_newWidget);
		dataTable->setCellWidget(i,m_checkColumn,m_newWidget);
		connect(m_newWidget, SIGNAL(stateChanged(int)), this, SLOT(enableVolume(int)));
		m_widgetList.insert(i, m_tempStr);
		dataTable->setRowHeight(i, 17);
		dataTable->setItem(i,m_dimColumn,new QTableWidgetItem(QString("%1").arg(volumeStack->getVolume(i)->GetDimensions()[0]).append(QString(", %1").arg(volumeStack->getVolume(i)->GetDimensions()[1]).append(QString(", %1").arg(volumeStack->getVolume(i)->GetDimensions()[2])))));
		dataTable->setItem(i,m_spacColumn,new QTableWidgetItem(QString("%1").arg(volumeStack->getVolume(i)->GetSpacing()[0]).append(QString(", %1").arg(volumeStack->getVolume(i)->GetSpacing()[1]).append(QString(", %1").arg(volumeStack->getVolume(i)->GetSpacing()[2])))));
		dataTable->setItem(i,m_fileColumn,new QTableWidgetItem(volumeStack->getFileName(i)));
		dataTable->setItem(i,m_sortColumn,new QTableWidgetItem(QString("%1").arg(0)));
	}
	m_old_r=1;

	QFont font;
	font.setBold(true);
	dataTable->horizontalHeaderItem(m_checkColumn)->setFont(font);
	dataTable->setColumnHidden(m_sortColumn, true);

	//Contextmenu start

	m_contextDimensions = new QAction(tr("Dimensions"), this );
	connect( m_contextDimensions, SIGNAL(activated()), this, SLOT(dimensionsActive()) );
	m_contextDimensions->setCheckable(true);
	m_contextDimensions->setChecked(true);
	m_contextSpacing = new QAction(tr("Spacing"), this );
	connect( m_contextSpacing, SIGNAL(activated()), this, SLOT(spacingActive()) );
	m_contextSpacing->setCheckable(true);
	m_contextSpacing->setChecked(true);
	m_contextFileName = new QAction(tr("Filename"), this );
	connect( m_contextFileName, SIGNAL(activated()), this, SLOT(fileNameActive()) );
	m_contextFileName->setCheckable(true);
	m_contextFileName->setChecked(true);
	setContextMenuPolicy( Qt::ActionsContextMenu );
	addAction( m_contextDimensions );
	addAction( m_contextSpacing );
	addAction( m_contextFileName );

	m_dimShow=m_spacShow=m_fileShow=true;
	dataTable->resizeColumnsToContents();

	//Contextmenu end

	emit setAllSelected(0);
}

dlg_volumePlayer::~dlg_volumePlayer() {

}

void dlg_volumePlayer::applyForAll()
{
	emit update(sliderIndexToVolumeIndex(volumeSlider->value()), true);
}

void dlg_volumePlayer::nextVolume()
{
	int index = volumeSlider->value();
	index = (index + 1) % (volumeSlider->maximum() + 1);
	volumeSlider->setValue(index);
}

void dlg_volumePlayer::previousVolume()
{
	int index = volumeSlider->value();
	if (--index < 0) {
		index=volumeSlider->maximum();
	}
	volumeSlider->setValue(index);
}

void dlg_volumePlayer::sliderChanged()
{
	if(m_isBlendingOn) {
		updateMultiChannelVisualization();
	} else {
		emit update(sliderIndexToVolumeIndex(volumeSlider->value()));
	}
}

void dlg_volumePlayer::playVolume()
{
	this->m_timer.start();
}

void dlg_volumePlayer::pauseVolume()
{
	this->m_timer.stop();
}

void dlg_volumePlayer::stopVolume()
{
	this->m_timer.stop();
	volumeSlider->setValue(volumeSlider->minimum());
}

void dlg_volumePlayer::setSpeed()
{
	float speed = getCurrentSpeed();
	m_timer.setInterval((int)((1/speed) * SECONDS_TO_MILISECONDS));
	this->speedValue->setText(QString::number(speed, 'f', 2));
}

void dlg_volumePlayer::updateView(int r, int c) {
	emit update(r);
}

void dlg_volumePlayer::editMaxSpeed() {
	QStringList inList		= (QStringList() << tr("#Speed (one step/msec)"));
	QList<QVariant> inPara	= (QList<QVariant>() << tr("%1").arg(getCurrentSpeed()));

	dlg_commoninput dlg(m_mdiChild, "Set speed", inList, inPara, NULL);

	if (dlg.exec() == QDialog::Accepted){
		float speed = (float)dlg.getDblValue(0);
		m_timer.setInterval((int)((1/speed) * SECONDS_TO_MILISECONDS));
		this->speedValue->setText(QString::number(speed, 'f', 2));
	}
}

void dlg_volumePlayer::setChecked(int r, int c) {

	for (int i=1; i<m_numberOfColumns;i++) {
		dataTable->item(m_old_r,i)->setBackgroundColor(Qt::white);
		dataTable->item(r,i)->setBackgroundColor(Qt::lightGray);
	}

	m_old_r=r;
}

QList<int> dlg_volumePlayer::getCheckedList() {

	m_outCheckList.clear();
	for (int i = 0; i < m_numberOfVolumes; i++)
	{
		// find the child widget with the name in the leList
		QCheckBox *t = this->findChild<QCheckBox*>(m_widgetList[i]);

		if (t != 0)
		{
			//get the text from the child widget and insert is to outValueList
			m_outCheckList.insert(i,t->checkState());
		}
		else
			m_outCheckList.insert(i, 0);
	}
	return (m_outCheckList);
}



void dlg_volumePlayer::selectAll(int c) {
	if (c==m_checkColumn) {
		m_areAllChecked=true;
		for (int i=0; i<m_numberOfVolumes;i++) {
			QCheckBox *t = this->findChild<QCheckBox*>(m_widgetList[i]);
			if (t->checkState()>=1) {
			}
			else {
				m_areAllChecked=false;
			}
		}
		for (int i=0; i<m_numberOfVolumes;i++) {
			QCheckBox *t = this->findChild<QCheckBox*>(m_widgetList[i]);
			if (m_areAllChecked) {
				t->setChecked(false);
			}
			else {
				t->setChecked(true);
			}
		}
	}
}

void dlg_volumePlayer::dimensionsActive() {
	if (dataTable->columnWidth(m_dimColumn)==0) {
		dataTable->resizeColumnToContents(m_dimColumn);
		m_contextDimensions->setChecked(true);
	}
	else {
		dataTable->setColumnWidth(m_dimColumn, 0);
		m_contextDimensions->setChecked(false);
	}
}

void dlg_volumePlayer::spacingActive() {
	if (dataTable->columnWidth(m_spacColumn)==0) {
		dataTable->resizeColumnToContents(m_spacColumn);
		m_contextSpacing->setChecked(true);
	}
	else {
		dataTable->setColumnWidth(m_spacColumn, 0);
		m_contextSpacing->setChecked(false);
	}
}

void dlg_volumePlayer::fileNameActive() {
	if (dataTable->columnWidth(m_fileColumn)==0) {
		dataTable->resizeColumnToContents(m_fileColumn);
		m_contextFileName->setChecked(true);
	}
	else {
		dataTable->setColumnWidth(m_fileColumn, 0);
		m_contextFileName->setChecked(false);
	}
}

float dlg_volumePlayer::getCurrentSpeed() {
	float speed = TIMER_MIN_SPEED + (TIMER_MAX_SPEED - TIMER_MIN_SPEED) * ((float)speedSlider->value() / speedSlider->maximum());
	return speed;
}

void dlg_volumePlayer::blendingStateChanged(int state)
{
	m_isBlendingOn = state;


	int oldVal = volumeSlider->value();
	if (m_isBlendingOn) {
		// set slider parameters
		volumeSlider->setMaximum((getNumberOfCheckedVolumes() - 1) * DIVISIONS_PER_VOLUME);
		volumeSlider->setValue(oldVal * DIVISIONS_PER_VOLUME);

		// set multi channel
		updateMultiChannelVisualization();
		enableMultiChannelVisualization();

		// setup gui
		applyForAllButton->setEnabled(false);
	} else {
		// set slider parameters
		volumeSlider->setMaximum(getNumberOfCheckedVolumes() - 1);
		volumeSlider->setValue(floor((double)(oldVal + 1) / DIVISIONS_PER_VOLUME + 0.5));

		// set multi channel
		disableMultiChannelVisualization();
		emit update(sliderIndexToVolumeIndex(volumeSlider->value()));

		// setup gui
		applyForAllButton->setEnabled(true);
	}
}

void dlg_volumePlayer::enableVolume(int state) {
	QCheckBox* check = static_cast<QCheckBox*>(QObject::sender());
	if(check == NULL) {
		return;
	}

	for(int i = 0; i < m_checkBoxes.size(); i++) {
		if (check == m_checkBoxes[i]) {
			switch(state) {
			case Qt::Checked:
				showVolume(i);
				break;
			case Qt::PartiallyChecked:
				break;
			case Qt::Unchecked:
				hideVolume(i);
				break;
			default:
				break;
			}
		}
	}

	int numOfCheckedVolumes = getNumberOfCheckedVolumes();
	for(int i = 0; i < m_checkBoxes.size(); i++) {
		if(numOfCheckedVolumes <= 1 && m_checkBoxes[i]->isChecked()) {
			m_checkBoxes[i]->setEnabled(false);
		} else {
			m_checkBoxes[i]->setEnabled(true);
		}
	}

	// setup slider
	int oldVal = volumeSlider->value();
	if(m_isBlendingOn) {
		volumeSlider->setMaximum((getNumberOfCheckedVolumes() - 1) * DIVISIONS_PER_VOLUME);
	} else {
		volumeSlider->setMaximum(getNumberOfCheckedVolumes() - 1);
	}
	volumeSlider->setValue(volumeSlider->minimum());
}

int dlg_volumePlayer::volumeIndexToSlicerIndex(int volumeIndex) {
	int slicerIndex = -1;

	if(((m_mask>>volumeIndex)&1) == 0) return -1;

	for(int i = 0; i <= volumeIndex; i++) {
		if(((m_mask>>i)&1) == 1) ++slicerIndex;
	}
	return slicerIndex;
}

int dlg_volumePlayer::sliderIndexToVolumeIndex(int slicerIndex) {
	int passedVolumes = 0;
	for(int i = 0; i <= m_numberOfVolumes; i++) {
		if(((m_mask>>i)&1) == 1) {
			passedVolumes++;
			if(passedVolumes > slicerIndex) {
				return i;
			}
		}
	}
	return -1;
}

void dlg_volumePlayer::showVolume(int volumeIndex) {
	m_mask = m_mask|(1<<volumeIndex);
}

void dlg_volumePlayer::hideVolume(int volumeIndex) {
	m_mask = m_mask&~(1<<volumeIndex);
}

int dlg_volumePlayer::getNumberOfCheckedVolumes() {
	int num = 0;
	for(int i = 0; i < m_numberOfVolumes; i++) {
		if(volumeIsShown(i)) num++;
	}
	return num;
}

bool dlg_volumePlayer::volumeIsShown (int volumeIndex) {
	return (((m_mask>>volumeIndex)&1) == 1);
}

void dlg_volumePlayer::enableMultiChannelVisualization() {
	if (m_mdiChild == NULL) {
		return;
	}

	for(int i = 0; i < CHANNELS_COUNT; i++)
		m_mdiChild->SetChannelRenderingEnabled(static_cast<iAChannelID>(ch_VolumePlayer0 + i), true);
}

void dlg_volumePlayer::disableMultiChannelVisualization() {
	if (m_mdiChild == NULL) {
		return;
	}

	for(int i = 0; i < CHANNELS_COUNT; i++)
		m_mdiChild->SetChannelRenderingEnabled(static_cast<iAChannelID>(ch_VolumePlayer0 + i), false);
}

void dlg_volumePlayer::setMultiChannelVisualization(int volumeIndex1, int volumeIndex2, double blendingCoeff) {
	if (m_mdiChild == NULL) {
		return;
	}

	int volumeIndex[] = { volumeIndex1, volumeIndex2 };
	double opacity[] = { 1., blendingCoeff };

	for(int i = 0; i < CHANNELS_COUNT; i++) {
		iAChannelID id = static_cast<iAChannelID>(ch_VolumePlayer0 + i);

		iAChannelVisualizationData* chData;
		if(!m_multiChannelIsInitialized) {
			chData = new iAChannelVisualizationData();
			m_mdiChild->InsertChannelData(id, chData);
		} else {
			chData = m_mdiChild->GetChannelData(id);
		}

		vtkImageData* imageData = m_volumeStack->getVolume(volumeIndex[i]);
		vtkColorTransferFunction* ctf = m_volumeStack->getColorTransferFunction(volumeIndex[i]);
		if(!m_multiChannelIsInitialized) {
			m_otf[i] = vtkSmartPointer<vtkPiecewiseFunction>::New();
		}
		m_otf[i]->ShallowCopy(m_volumeStack->getPiecewiseFunction(volumeIndex[i]));

		for(int j = 0; j < m_otf[i]->GetSize(); j++) {
			double val[4];
			m_otf[i]->GetNodeValue(j, val);
			val[1] *= opacity[i];
			m_otf[i]->SetNodeValue(j, val);
		}

		ResetChannel(chData, imageData, ctf, m_otf[i]);

		if(!m_multiChannelIsInitialized) {
			m_mdiChild->InitChannelRenderer(id, true);
			// TODO: VOLUME: rewrite!
			// m_mdiChild->getRenderer()->showMainVolumeWithChannels(false);
		}

		m_mdiChild->UpdateChannelSlicerOpacity(id, opacity[i]);
	}

	if(!m_multiChannelIsInitialized) m_multiChannelIsInitialized = true;

//	m_mdiChild->getRenderer()->updateChannelImages();
	m_mdiChild->getSlicerDataXY()->updateChannelMappers();
	m_mdiChild->getSlicerDataXZ()->updateChannelMappers();
	m_mdiChild->getSlicerDataYZ()->updateChannelMappers();
	m_mdiChild->updateViews();
}

void dlg_volumePlayer::updateMultiChannelVisualization() {
	double sliderVal = (double)volumeSlider->value() / DIVISIONS_PER_VOLUME;
	double blend = sliderVal - floor(sliderVal);
	if (volumeSlider->value() == volumeSlider->maximum()) {
		setMultiChannelVisualization(sliderIndexToVolumeIndex(floor(sliderVal) - 1), sliderIndexToVolumeIndex(floor(sliderVal)), 1);
	} else {
		setMultiChannelVisualization(sliderIndexToVolumeIndex(floor(sliderVal)), sliderIndexToVolumeIndex(floor(sliderVal) + 1), blend);
	}
}

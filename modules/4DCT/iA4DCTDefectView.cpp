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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iA4DCTDefectView.h"
// iA
#include "FilterLabelImage.h"
#include "iAConsole.h"
#include "iASlicer.h"
#include "iAChannelVisualizationData.h"
// vtk
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkMetaImageReader.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

const iAChannelID ChanID[4] = { ch_DefectView0, ch_DefectView1, ch_DefectView2, ch_DefectView3 };

const QColor CrackColor(91, 155, 213);		// blue
const QColor DebondingColor(0, 176, 80);	// green
const QColor PulloutColor(255, 192, 0);		// yellow
const QColor BreakageColor(255, 0, 0);		// red
const QColor PurpleCol(112, 48, 162);		// purple
const QColor Background(0, 0, 0);			// black

const int AmoutOfDefects = 4;
enum DefectID { Crack, Pullout, Debonding, Breakage };

iA4DCTDefectView::iA4DCTDefectView(QWidget* parent /*= 0*/)
	: QDockWidget(parent),
	//m_slicer(new iASlicer(slicerWidget, iASlicerMode::XY, slicerWidget)),
	m_channel(new iAChanData(Background, BreakageColor, ChanID[3])),
	m_transform(vtkSmartPointer<vtkTransform>::New()),
	m_colorTF(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_showPullouts(false),
	m_showCracks(false),
	m_showBreakages(false),
	m_showDebondings(false)
{
	setupUi(this);
	m_slicer = new iASlicer(slicerWidget, iASlicerMode::XY, slicerWidget);

	connect(rbXY, SIGNAL(toggled(bool)), this, SLOT(toggleXY(bool)));
	connect(rbXZ, SIGNAL(toggled(bool)), this, SLOT(toggleXZ(bool)));
	connect(rbYZ, SIGNAL(toggled(bool)), this, SLOT(toggleYZ(bool)));
	connect(verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setSliceScrollBar(int)));
	connect(cbPullouts, SIGNAL(stateChanged(int)), this, SLOT(enablePullouts(int)));
	connect(cbCracks, SIGNAL(stateChanged(int)), this, SLOT(enableCracks(int)));
	connect(cbDebondings, SIGNAL(stateChanged(int)), this, SLOT(enableDebondings(int)));
	connect(cbBreakages, SIGNAL(stateChanged(int)), this, SLOT(enableBrekages(int)));

	//radioButton_XZ->setChecked(true);
}

iA4DCTDefectView::~iA4DCTDefectView()
{
	delete m_slicer;
}

void iA4DCTDefectView::initializeSlicer(QString path)
{
	m_intensityImg = loadImage(path);

	// setup color transfer function
	m_colorTF->RemoveAllPoints();
	m_colorTF->AddRGBPoint(m_intensityImg->GetScalarRange()[0], 0., 0., 0.);
	m_colorTF->AddRGBPoint(m_intensityImg->GetScalarRange()[1], 1., 1., 1.);

	// initialize slicer
	m_slicer->setup(false, false, 0, 0, 0, false);
	m_slicer->initializeData(m_intensityImg, m_transform, m_colorTF);
	m_slicer->initializeWidget(m_intensityImg);
	m_slicer->setSliceNumber(58);
	m_slicer->ChangeMode(iASlicerMode::XZ);
	m_slicer->update();
}

void iA4DCTDefectView::setDefects(QString labeledImg, QString pullouts, QString cracks, QString breakages, QString debondings)
{
	m_labeledImg = loadImage(labeledImg);

	iA4DCTDefects::VectorDataType pulloutList, crackList, breakageList, debondingList;
	pulloutList		= iA4DCTDefects::load(pullouts);
	debondingList	= iA4DCTDefects::load(debondings);
	breakageList	= iA4DCTDefects::load(breakages);
	crackList		= iA4DCTDefects::load(cracks);

	QVector<iA4DCTDefects::VectorDataType> defects(AmoutOfDefects);
	defects[DefectID::Pullout]		= pulloutList;
	defects[DefectID::Debonding]	= debondingList;
	defects[DefectID::Breakage]		= breakageList;
	defects[DefectID::Crack]		= crackList;

	switch (m_labeledImg->GetScalarType()) {
	case VTK_UNSIGNED_SHORT:
		prepareLabeledImage<unsigned short>(m_labeledImg.GetPointer(), defects);
		break;
	default:
		break;
	}

	m_channel->imgData->DeepCopy(m_labeledImg);
	initializeTransferFunction(m_channel.data());
	m_slicer->initializeChannel(m_channel->id, m_channel->visData.data());
	m_slicer->setChannelOpacity(m_channel->id, 1.);
	m_slicer->enableChannel(m_channel->id, true);
	m_slicer->update();
}

//void iADefectView::initializeSlicer(vtkImageData* image)
//{
//	// setup color transfer function
//	m_colorTF->RemoveAllPoints();
//	m_colorTF->AddRGBPoint(image->GetScalarRange()[0], 0., 0., 0.);
//	m_colorTF->AddRGBPoint(image->GetScalarRange()[1], 1., 1., 1.);
//
//	// initialize slicer
//	m_slicer->setup(false, false, 0, 0, 0, false);
//	m_slicer->initializeData(image, m_transform, m_colorTF);
//	m_slicer->initializeWidget(image);
//	m_slicer->setSliceNumber(58);
//	m_slicer->ChangeMode(iASlicerMode::XZ);
//	m_slicer->update();
//}

void iA4DCTDefectView::initializeChannel(iAChanData* ch, vtkImageData* image)
{
	//typedef itk::Image<unsigned short, 3> TImage;
	//typedef itk::FilterLabelImage<TImage> FilterLabelImageType;
	//FilterLabelImageType::Pointer filter = FilterLabelImageType::New();
	//filter->SetInput()

	ch->imgData->DeepCopy(image);
	ch->InitTFs();
	m_slicer->initializeChannel(ch->id, ch->visData.data());
	m_slicer->setChannelOpacity(ch->id, 1.);
	m_slicer->enableChannel(ch->id, true);
	m_slicer->update();
}

void iA4DCTDefectView::directionChanged(iASlicerMode mode)
{
	m_slicer->ChangeMode(mode);
	m_slicer->update();
}

vtkImageData* iA4DCTDefectView::loadImage(QString path)
{
	vtkSmartPointer<vtkMetaImageReader> reader = vtkMetaImageReader::New();
	reader->SetFileName(path.toStdString().c_str());
	reader->Update();
	return reader->GetOutput();
}

void iA4DCTDefectView::enableDefect(bool& defect, int state)
{
	if (state == Qt::Checked) {
		defect = true;
	} else {
		defect = false;
	}
}

void iA4DCTDefectView::updateChannelTF()
{
	initializeTransferFunction(m_channel.data());
	m_slicer->reInitializeChannel(m_channel->id, m_channel->visData.data());
	m_slicer->update();
}

void iA4DCTDefectView::setSliceScrollBar(int sn)
{
	m_slicer->setSliceNumber(sn);
}

void iA4DCTDefectView::toggleXY(bool checked)
{
	if (checked)
		directionChanged(iASlicerMode::XY);
}

void iA4DCTDefectView::toggleXZ(bool checked)
{
	if (checked)
		directionChanged(iASlicerMode::XZ);
}

void iA4DCTDefectView::toggleYZ(bool checked)
{
	if (checked)
		directionChanged(iASlicerMode::YZ);
}

void iA4DCTDefectView::enablePullouts(int state)
{
	enableDefect(m_showPullouts, state);
	updateChannelTF();
}

void iA4DCTDefectView::enableCracks(int state)
{
	enableDefect(m_showCracks, state);
	updateChannelTF();
}

void iA4DCTDefectView::enableDebondings(int state)
{
	enableDefect(m_showDebondings, state);
	updateChannelTF();
}

void iA4DCTDefectView::enableBrekages(int state)
{
	enableDefect(m_showBreakages, state);
	updateChannelTF();
}

void iA4DCTDefectView::initializeTransferFunction(iAChanData* ch)
{
	vtkColorTransferFunction* tf = ch->tf;
	vtkPiecewiseFunction* otf = ch->otf;

	tf->RemoveAllPoints();
	otf->RemoveAllPoints();
	double rangeMin = ch->imgData->GetScalarRange()[0];
	double rangeMax = ch->imgData->GetScalarRange()[1];

	const int NumberOfDefects = 4; // pull-outs, cracks, debondigns, breakages
	unsigned int maxVal = 1 << 4;
	typedef std::bitset<NumberOfDefects> TIntensity;
	QVector<TIntensity> map;
	for (int i = 0; i < maxVal; i++) {
		TIntensity intens(i);
		if (!m_showCracks) {
			intens[0] = 0;
		}
		if (!m_showPullouts) {
			intens[1] = 0;
		}
		if (!m_showBreakages) {
			intens[2] = 0;
		}
		if (!m_showDebondings) {
			intens[3] = 0;
		}
		map.push_back(intens);
	}

	for (int i = 0; i < maxVal; i++) {
		switch (map[i].to_ulong()) {
		case 1 << DefectID::Pullout:
			tf->AddRGBPoint(i, PulloutColor.redF(), PulloutColor.greenF(), PulloutColor.blueF());
			break;
		case 1 << DefectID::Crack:
			tf->AddRGBPoint(i, CrackColor.redF(), CrackColor.greenF(), CrackColor.blueF());
			break;
		case 1 << DefectID::Debonding:
			tf->AddRGBPoint(i, DebondingColor.redF(), DebondingColor.greenF(), DebondingColor.blueF());
			break;
		case 1 << DefectID::Breakage:
			tf->AddRGBPoint(i, BreakageColor.redF(), BreakageColor.greenF(), BreakageColor.blueF());
			break;
		default:
			tf->AddRGBPoint(i, Background.redF(), Background.greenF(), Background.blueF());
			break;
		}
	}

	otf->AddPoint(rangeMin, 0.0);
	otf->AddPoint(rangeMin + 0.1, 1.0);
	//ch->visData->SetColor(ch->cols[0]);
	ResetChannel(ch->visData.data(), ch->imgData, ch->tf, ch->otf);
}

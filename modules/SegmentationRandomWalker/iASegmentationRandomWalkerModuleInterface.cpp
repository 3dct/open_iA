/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iASegmentationRandomWalkerModuleInterface.h"

#include "dlg_RWSeeds.h"
#include "dlg_commoninput.h"
#include "iAConnector.h"
#include "iAImageTypes.h"
#include "iAitkRandomWalker.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iATypedCallHelper.h"
#include "iAVectorArrayImpl.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QMdiSubWindow>
#include <QMessageBox>
#include <QTextDocument>


void iASegmentationRandomWalkerModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSegm = getMenuWithTitle(filtersMenu, QString( "Segmentation" ) );
	QMenu * menuGraphSegm = getMenuWithTitle(menuSegm, QString("Graph-Based"));
	QAction * actionSegmRW = new QAction( m_mainWnd );
	QAction * actionSegmERW = new QAction( m_mainWnd );
	actionSegmRW->setText( QApplication::translate( "MainWindow", "Random Walker (from Seed Points)", 0 ) );
	actionSegmERW->setText( QApplication::translate( "MainWindow", "Extended Random Walker (Prior Model)", 0 ) );
	menuGraphSegm->addAction(actionSegmRW);
	menuGraphSegm->addAction(actionSegmERW);
	connect(actionSegmRW, SIGNAL(triggered()), this, SLOT(CalculateRW()));
	connect(actionSegmERW, SIGNAL(triggered()), this, SLOT(CalculateERW()));
}


struct RWParams
{
	RWParams() :
		m_inputChannels(new QVector<iARWInputChannel>()),
		m_maxIter(1000),
		m_seeds(new SeedVector)
	{}
	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	int m_maxIter;
	int m_size[3];
	double m_spacing[3];
	QSharedPointer<SeedVector> m_seeds;
};


template <typename ImagePixelType>
void CalculateRW_template(
	iAConnector* connector,
	RWParams const & params
	)
{
	typedef itk::Image<ImagePixelType, 3> TInputImage;
	typedef iAitkRandomWalker<TInputImage> TRandomWalker;
	TRandomWalker* randomWalker = new TRandomWalker();
	randomWalker->SetInput(params.m_inputChannels);
	randomWalker->SetParams(params.m_maxIter, params.m_size, params.m_spacing, params.m_seeds);
	randomWalker->Calculate();
	if (randomWalker->Success() )
	{
		connector->SetImage( randomWalker->GetLabelImage() );
		connector->Modified();
	}
}

struct ERWParams
{
	ERWParams():
		m_inputChannels(new QVector<iARWInputChannel>()),
		m_priorModel(new QVector<PriorModelImagePointer>()),
		m_maxIter(1000),
		m_gamma(1)
	{}
	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	int m_maxIter;
	int m_size[3];
	double m_spacing[3];
	QSharedPointer<QVector<PriorModelImagePointer> > m_priorModel;
	double m_gamma;
};


template <typename ImagePixelType>
void CalculateERW_template(
	iAConnector* connector,
	ERWParams const & params
	)
{
	typedef itk::Image<ImagePixelType, 3> TInputImage;
	typedef iAitkExtendedRandomWalker<TInputImage> TExtendedRandomWalker;
	TExtendedRandomWalker* randomWalker = new TExtendedRandomWalker();
	randomWalker->SetInput(params.m_inputChannels);
	randomWalker->SetParams(params.m_maxIter, params.m_size, params.m_spacing, params.m_priorModel, params.m_gamma);
	randomWalker->Calculate();
	if (randomWalker->Success() )
	{
		connector->SetImage( randomWalker->GetLabelImage() );
		connector->Modified();
	}
}

class iAERWFilter: public iAAlgorithm
{
public:
	iAERWFilter(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0):
		iAAlgorithm(fn, i, p, logger, parent)
	{}
	void SetParams(ERWParams const & params)
	{
		m_params = params;
	}
private:
	ERWParams m_params;

	virtual void performWork()
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(CalculateERW_template, itkType, getConnector(), m_params);
	}
};

class iARWFilter: public iAAlgorithm
{
public:
	iARWFilter(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0):
		iAAlgorithm(fn, i, p, logger, parent)
	{}
	void SetParams(RWParams const & params  )
	{
		m_params = params;
	}
private:
	RWParams m_params;

	virtual void performWork()
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(CalculateRW_template, itkType, getConnector(), m_params);
	}
};

bool iASegmentationRandomWalkerModuleInterface::CalculateERW()
{
	double DefaultBeta = 1;
	double DefaultWeight = 1;
	int originalFileIdx = 0;
	int labelCnt = 2;

	QList<QMdiSubWindow *> mdiwindows = m_mainWnd->MdiChildList();
	int fileCnt = mdiwindows.size();
	QStringList windowList;
	QString::SplitBehavior behavior = QString::SplitBehavior::SkipEmptyParts;
	for (int i = 0; i<fileCnt; ++i)
	{
		MdiChild* mdiChild = qobject_cast<MdiChild *>(mdiwindows[i]->widget());
		int modCnt = mdiChild->GetModalities()->size();
		QString fileName = mdiChild->currentFile();
		if (!fileName.isEmpty())
		{
			windowList << fileName.split("/", behavior).last();
		}
		else
		{
			windowList << mdiChild->windowTitle();
		}
	}

	if (fileCnt > 3)	// if more than 3 files are open, there's either more than one input or more than two labels
	{
		QStringList inParaDescr = (QStringList()
			<< tr("+Original Image")
			<< tr("*Number of Labels"));
		QList<QVariant> inParaValue;
		inParaValue << windowList << labelCnt;
		QTextDocument *fDescr = new QTextDocument(0);
		fDescr->setHtml(
			"<p><font size=+1>Calculate Extended Random Walker.</font></p>"
			"<p>Select which image to use as input, and how many labels you want to segment "
			"(for each of which you need to select a prior probability image in the next step).</p>");
		dlg_commoninput dlg(m_mainWnd, "Extended Random Walker Segmentation", inParaDescr, inParaValue, fDescr, true);
		if (dlg.exec() != QDialog::Accepted)
		{
			return false;
		}
		originalFileIdx = dlg.getSpinBoxValues()[0];
		labelCnt = dlg.getSpinBoxValues()[1];
	}
	else
	{
		MdiChild* activeChild = m_mainWnd->activeMdiChild();
		for (int i = 0; i < fileCnt; ++i)
		{
			if (qobject_cast<MdiChild *>(mdiwindows[i]->widget()) == activeChild)
			{
				originalFileIdx = i;
				break;
			}
		}
	}
	if ( labelCnt+1 > fileCnt) {
		QMessageBox::warning(m_mainWnd, tr("Extended Random Walker Segmentation"),
			tr("Too few datasets loaded (%1). For the chosen task, there need to be at least %2 images loaded: One original image and %3 label priors.")
			.arg(fileCnt)
			.arg(labelCnt+1)
			.arg(labelCnt));
		return false;
	}
	windowList.removeAt(originalFileIdx);
	
	QStringList normalizerModes;
	for (int n = 0; n < nmCount; ++n)
	{
		normalizerModes << GetNormalizerNames()[n];
	}
	QStringList distanceFuncList;
	for (int d = 0; d < GetDistanceMeasureCount(); ++d)
	{
		distanceFuncList << GetDistanceMeasureNames()[d];
	}
	QStringList inParaDescr = QStringList();
	QList<QVariant> inParaValue;
	MdiChild* origMdiChild = qobject_cast<MdiChild *>(mdiwindows[originalFileIdx]->widget());
	int modalityCnt = origMdiChild->GetModalities()->size();
	for (int m = 0; m < modalityCnt; ++m)
	{
		inParaDescr
			<< QString("+Modality %1 Normalizer").arg(m)
			<< QString("^Modality %1 Beta (only Gaussian Norm.)").arg(m)
			<< QString("+Modality %1 Distance Function").arg(m)
			<< QString("^Modality %1 Weight").arg(m);
		inParaValue
			<< normalizerModes
			<< DefaultBeta
			<< distanceFuncList
			<< DefaultWeight;
	}
	for (int p = 0; p < labelCnt; ++p)
	{
		inParaDescr << QString("+Label %1 Prior").arg(p);
		inParaValue << windowList;
	}
	inParaDescr << tr("^Gamma (Weight of Priors)")
		<< tr("*Maximum Iterations (for linear solver)");
	ERWParams params;
	inParaValue
		<< params.m_gamma
		<< params.m_maxIter;
	
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(
		"<p><font size=+1>Calculate Extended Random Walker.</font></p>"
		"<p>Select which image to use as original image All other open windows will be considered as prior model.</p>");

	dlg_commoninput dlg(m_mainWnd, "Extended Random Walker Segmentation", inParaDescr, inParaValue, fDescr, true);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	auto img = origMdiChild->GetModality(0)->GetImage();
	int const * size = img->GetDimensions();
	double const * spacing = img->GetSpacing();
	std::copy(size, size + 3, params.m_size);
	std::copy(spacing, spacing + 3, params.m_spacing);

	for (int m=0; m<modalityCnt; ++m)
	{
		QString normFuncName = dlg.getComboBoxIndices()    [m*4  ];
		double beta          = dlg.getDoubleSpinBoxValues()[m*4+1];
		QString distFuncName = dlg.getComboBoxValues()     [m*4+2];
		double weight        = dlg.getDoubleSpinBoxValues()[m*4+3];
		iARWInputChannel input;
	
		auto vtkPixelAccess = QSharedPointer<iAvtkPixelVectorArray>(new iAvtkPixelVectorArray(size[0], size[1], size[2]));
		for (int c = 0; c < origMdiChild->GetModality(m)->ComponentCount(); ++c)
		{
			vtkPixelAccess->AddImage(origMdiChild->GetModality(m)->GetComponent(c));
		}
		input.image = vtkPixelAccess;
		input.distanceFunc = GetDistanceMeasure(distFuncName);
		input.normalizeFunc = CreateNormalizer(normFuncName, beta);
		input.weight = weight;
		params.m_inputChannels->push_back(input);
	}
	QSet<int> usedFileIDs;
	for (int p = 0; p < labelCnt; ++p)
	{
		int curFileID = dlg.getComboBoxIndices()[modalityCnt*4 + p];
		if (usedFileIDs.contains(curFileID))
		{
			QMessageBox::warning(m_mainWnd, tr("Extended Random Walker Segmentation"),
				tr("Same dataset used as prior for more than one label (%1), this does not make sense.").arg(dlg.getComboBoxValues()[modalityCnt * 4 + p]));
			return false;
		}
		usedFileIDs.insert(curFileID);
		params.m_priorModel->push_back(qobject_cast<MdiChild *>(mdiwindows[p]->widget())->GetModality(0)->GetImage());
	}
	params.m_gamma = dlg.getDoubleSpinBoxValues()[modalityCnt*4 + labelCnt];
	params.m_maxIter = dlg.getDoubleSpinBoxValues()[modalityCnt*4 + labelCnt+1];

	QString filterName = "Extended Random Walker";
	PrepareResultChild(originalFileIdx, filterName);
	iAERWFilter * erw = new iAERWFilter(filterName,
		m_childData.imgData, NULL, m_mdiChild->getLogger(), m_mdiChild);
	erw->SetParams(params);
	m_mdiChild->connectThreadSignalsToChildSlots(erw);
	erw->start();
	return true;
}


SeedVector ExtractSeedVector(QString const & seedString, int width, int height, int depth)
{
	QStringList lines = seedString.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
	SeedVector result;
	QString parseErrors;
	bool numberOK;
	// convert seed string to vector:
	QRegExp rx("(\\ |\\,|\\.|\\:|\\;|\\t)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'
	for (int lineNumber = 0; lineNumber < lines.size(); ++lineNumber)
	{
		QString line = lines[lineNumber];
		QStringList query = line.split(rx);
		int x=0,
			y=0,
			z=0;
		if (query.size() < 2)
		{
			parseErrors.append(QString("Line %1: Invalid coordinate number %2.\n").arg(lineNumber).arg(query.size()));
			continue;
		}
		x = query[0].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid x-coordinate: %2.\n").arg(lineNumber).arg(query[0]));
			continue;
		}
		y = query[1].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid y-coordinate: %2.\n").arg(lineNumber).arg(query[1]));
			continue;
		}
		z = query[2].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid z-coordinate: %2.\n").arg(lineNumber).arg(query[2]));
			continue;
		}
		int label = query[3].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid label: %2.\n").arg(lineNumber).arg(query[3]));
			continue;
		}
		if (x < 0 || x > width ||
		    y < 0 || y > height ||
			z < 0 || z > depth)
		{
			parseErrors.append(QString("Line %1: Coordinate outside of image: %2, %3, %4.\n").arg(lineNumber).arg(x).arg(y).arg(z));
		}
		else
		{
			result.push_back(std::make_pair(iAImageCoordinate(x, y, z), label));
		}
	}
	if (parseErrors.size() > 0)
	{
		QMessageBox::warning(0, "Random Walker", "Error(s) in seed file: \n"+parseErrors);
	}
	return result;
}


bool iASegmentationRandomWalkerModuleInterface::CalculateRW()
{
	dlg_RWSeeds dlgRWSeeds(m_mainWnd);
	if (dlgRWSeeds.exec() != QDialog::Accepted)
	{
		return false;
	}

	QString seeds = dlgRWSeeds.GetSeeds();
	double beta = dlgRWSeeds.GetBeta();

	QString filterName = "Random Walker";
	PrepareResultChild(filterName);
	iARWFilter * thread = new iARWFilter(filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	int extent[6];
	m_childData.imgData->GetExtent(extent);
	SeedVector seedVector = ExtractSeedVector(seeds, extent[1]-extent[0]+1, extent[3]-extent[2]+1, extent[5]-extent[4]+1);
	if (seedVector.size() < 2)
	{
		QMessageBox::warning(0, "Random Walker", "You must specify at least two seed points.");
		return false;
	}
	RWParams params;
	thread->SetParams(params);
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	return true;
}

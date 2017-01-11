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
#include "iASegmentationRandomWalkerModuleInterface.h"

#include "dlg_RWSeeds.h"
#include "dlg_commoninput.h"
#include "iAConnector.h"
#include "iAitkRandomWalker.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "iATypedCallHelper.h"

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
	actionSegmRW->setText( QApplication::translate( "MainWindow", "Random Walker", 0 ) );
	actionSegmERW->setText( QApplication::translate( "MainWindow", "Extended Random Walker", 0 ) );
	menuGraphSegm->addAction(actionSegmRW);
	menuGraphSegm->addAction(actionSegmERW);
	connect(actionSegmRW, SIGNAL(triggered()), this, SLOT(CalculateRW()));
	connect(actionSegmERW, SIGNAL(triggered()), this, SLOT(CalculateERW()));
}

/*
template <typename PixelType>
PriorModelImageType::Pointer CastToDouble(iAConnector::ImageBaseType* img, PixelType)
{
	typedef itk::Image<PixelType, 3> InputImageType;
	typedef itk::CastImageFilter<InputImageType, PriorModelImageType > CastType;
	typename CastType::Pointer caster = CastType::New();
	caster->SetInput(dynamic_cast<itk::Image<PixelType, 3>* >(img));
	caster->Update();
	return caster->GetOutput();
}
*/


template <typename ImagePixelType>
void CalculateRW_template(
	iAConnector* connector,
	SeedVector seeds,
	double beta
	)
{
	typedef itk::Image<ImagePixelType, 3> TInputImage;
	typedef iAitkRandomWalker<TInputImage> TRandomWalker;
	TRandomWalker* randomWalker = new TRandomWalker();
	randomWalker->SetInput(dynamic_cast< TInputImage* >(connector->GetITKImage()), seeds, beta);
	randomWalker->Calculate();
	if (randomWalker->Success() )
	{
		connector->SetImage( randomWalker->GetLabelImage() );
		connector->Modified();
	}
}


template <typename ImagePixelType>
void CalculateERW_template(
	iAConnector** connectors,
	int connectorCount
	)
{
	typedef itk::Image<ImagePixelType, 3> TInputImage;
	typedef iAitkExtendedRandomWalker<TInputImage> TExtendedRandomWalker;
	TExtendedRandomWalker* randomWalker = new TExtendedRandomWalker();
	randomWalker->SetInput(dynamic_cast< TInputImage* >(connectors[0]->GetITKImage()));
	/*
	for (int i=1; i<connectorCount; ++i)
	{
		typename PriorModelImageType::Pointer priorLabelModel;
		switch (connectors[i]->GetITKScalarPixelType()) // This filter handles all types
		{
			case itk::ImageIOBase::UCHAR:  priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<unsigned char>(0)); break;
			case itk::ImageIOBase::CHAR:   priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<char>(0)); break;
			case itk::ImageIOBase::USHORT: priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<unsigned short>(0)); break;
			case itk::ImageIOBase::SHORT:  priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<short>(0)); break;
			case itk::ImageIOBase::UINT:   priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<unsigned int>(0)); break;
			case itk::ImageIOBase::INT:    priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<int>(0)); break;
			case itk::ImageIOBase::ULONG:  priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<unsigned long>(0)); break;
			case itk::ImageIOBase::LONG:   priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<long>(0)); break;
			case itk::ImageIOBase::FLOAT:  priorLabelModel = CastToDouble(connectors[i]->GetITKImage(), static_cast<float>(0)); break;
			case itk::ImageIOBase::DOUBLE: priorLabelModel = dynamic_cast<PriorModelImageType* >(connectors[i]->GetITKImage());
			case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
			default: ;
//				addMsg(tr("%1  Unknown/Invalid image type")
//					.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
		}
		randomWalker->AddPriorModel(priorLabelModel);
	}
	*/
	randomWalker->Calculate();
	if (randomWalker->Success() )
	{
		connectors[0]->SetImage( randomWalker->GetLabelImage() );
		connectors[0]->Modified();
	}
}

class iAERWFilter: public iAAlgorithm
{
public:
	iAERWFilter(QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0):
		iAAlgorithm(fn, fid, i, p, logger, parent),
		m_priorCount(0)
	{}
	void SetPriors(QVector<vtkSmartPointer<vtkImageData> > priors)
	{
		m_priorCount = priors.size();
	}
private:
	int m_priorCount;

	virtual void run()
	{
		addMsg(tr("%1  %2 started.")
			.arg(QLocale().toString(Start(), QLocale::ShortFormat))
			.arg(getFilterName()));
		getConnector()->SetImage(getVtkImageData());
		getConnector()->Modified();
		try
		{
			iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
			ITK_TYPED_CALL(CalculateERW_template, itkType,
				getConnectorArray(), m_priorCount + 1);
		}
		catch( itk::ExceptionObject &e)
		{
			addMsg(tr("%1  %2 terminated unexpectedly (%3). Elapsed time: %4 ms")
				.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
				.arg(getFilterName())
				.arg(e.what())
				.arg(Stop()));
		}
		addMsg(tr("%1  %2 finished. Elapsed time: %3 ms")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		emit startUpdate();
	}
};

class iARWFilter: public iAAlgorithm
{
public:
	iARWFilter(QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0):
		iAAlgorithm(fn, fid, i, p, logger, parent),
		m_beta(0)
	{}
	void SetParams(SeedVector const & seeds, double beta)
	{
		m_seeds = seeds;
		m_beta = beta;
	}
private:
	SeedVector m_seeds;
	double m_beta;

	virtual void run()
	{
		addMsg(tr("%1  %2 started.")
			.arg(QLocale().toString(Start(), QLocale::ShortFormat))
			.arg(getFilterName()));
		getConnector()->SetImage(getVtkImageData());
		getConnector()->Modified();

		try
		{
			iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
			ITK_TYPED_CALL(CalculateRW_template, itkType,
				getConnector(), m_seeds, m_beta);
		}
		catch( itk::ExceptionObject &e)
		{
			addMsg(tr("%1  %2 terminated unexpectedly (%3). Elapsed time: %4 ms")
				.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
				.arg(getFilterName())
				.arg(e.what())
				.arg(Stop()));
		}
		addMsg(tr("%1  %2 finished. Elapsed time: %3 ms")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		emit startUpdate();
	}
};

bool iASegmentationRandomWalkerModuleInterface::CalculateERW()
{
	
	QList<QMdiSubWindow *> mdiwindows = m_mainWnd->MdiChildList();
	if (mdiwindows.size() < 3) {
		QMessageBox::warning(m_mainWnd, tr("Extended Random Walker Segmentation"),
			tr("This operation requires at least three datasets to be loaded: "
			"The original image data, and one probability image for each label to segment "
			"(a probabilistic pixelwise classification, which is used as a prior model). "
			"Currently there are %1 windows open.")
			.arg(mdiwindows.size()));
		return false;
	}
	QStringList inList = (QStringList()
		<< tr("+Original Image"));
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(
		"<p><font size=+1>Calculate Extended Random Walker.</font></p>"
		"<p>Select which image to use as original image All other open windows will be considered as prior model.</p>");
	QList<QVariant> inPara;
	QStringList list;
	QString::SplitBehavior behavior = QString::SplitBehavior::SkipEmptyParts;
	for (int i=0; i<mdiwindows.size(); ++i)
	{
		MdiChild* mdiChild = qobject_cast<MdiChild *>(mdiwindows[i]->widget());
		QString fileName = mdiChild->currentFile();
		if (!fileName.isEmpty())
		{
			list << fileName.split("/", behavior).last();
		}
		else
		{
			list << mdiChild->windowTitle();
		}
	}
	inPara	<< list;
	dlg_commoninput dlg(m_mainWnd, "Extended Random Walker Segmentation", 1, inList, inPara, fDescr, true);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	QList<int> fileIndices = dlg.getComboBoxIndices();
	QVector<vtkSmartPointer<vtkImageData> > priorImg(mdiwindows.size()-1);
	int currInsert = 0;
	for (int i=0; i<mdiwindows.size(); ++i)
	{
		if (i == fileIndices[0])
		{
			continue;
		}
		else
		{
			priorImg[currInsert++] = qobject_cast<MdiChild *>(mdiwindows[i]->widget())->getImagePointer();
		}
	}

	QString filterName = "Extended Random Walker";
	PrepareResultChild(fileIndices[0], filterName);
	iAERWFilter * thread = new iAERWFilter(filterName, EXTENDED_RANDOM_WALKER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	thread->SetPriors(priorImg);
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
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
	iARWFilter * thread = new iARWFilter(filterName, RANDOM_WALKER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	int extent[6];
	m_childData.imgData->GetExtent(extent);
	SeedVector seedVector = ExtractSeedVector(seeds, extent[1]-extent[0]+1, extent[3]-extent[2]+1, extent[5]-extent[4]+1);
	if (seedVector.size() < 2)
	{
		QMessageBox::warning(0, "Random Walker", "You must specify at least two seed points.");
		return false;
	}
	thread->SetParams(seedVector, beta);
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	return true;
}

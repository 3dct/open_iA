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
#include "dlg_GEMSeControl.h"

#include "dlg_labels.h"
#include "dlg_modalities.h"
#include "dlg_priors.h"
#include "dlg_samplingSettings.h"
#include "dlg_progress.h"
#include "dlg_GEMSe.h"
#include "iACharacteristics.h"
#include "iAColorTheme.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAMMSegParameterRange.h"
#include "iAMMSegSampler.h"
#include "iAModality.h"
#include "iAImageClusterer.h"
#include "iASamplingResults.h"
#include "iASEAFile.h"
#include "mdichild.h"
#include "ui_assistant.h"

#include <itkVTKImageToImageFilter.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

dlg_GEMSeControl::dlg_GEMSeControl(QWidget *parentWidget,
	dlg_GEMSe* dlgGEMSe,
	dlg_modalities* dlgModalities,
	dlg_priors* dlgPriors,
	dlg_labels* dlgLabels,
	QString const & defaultThemeName
):
	dlg_GEMSeControlUI(parentWidget),
	m_dlgSamplingSettings(0),
	m_dlgProgress(0),
	m_dlgGEMSe(dlgGEMSe),
	m_dlgModalities(dlgModalities),
	m_dlgPriors(dlgPriors),
	m_dlgModalitySPLOM(0),
	m_dlgLabels(dlgLabels)
{
	for (QString themeName : iAColorThemeManager::GetInstance().GetAvailableThemes())
	{
		cbColorThemes->addItem(themeName);
		if (themeName == defaultThemeName)
		{
			cbColorThemes->setCurrentText(themeName);
		}
	}

	connect(pbSample,         SIGNAL(clicked()), this, SLOT(StartSampling()));
	connect(pbSamplingLoad,   SIGNAL(clicked()), this, SLOT(LoadSampling()));
	connect(pbSamplingStore,  SIGNAL(clicked()), this, SLOT(StoreSampling()));
	connect(pbClusteringCalc, SIGNAL(clicked()), this, SLOT(CalculateClustering()));
	connect(pbClusteringLoad, SIGNAL(clicked()), this, SLOT(LoadClustering()));
	connect(pbClusteringStore,SIGNAL(clicked()), this, SLOT(StoreClustering()));
	connect(pbCalcCharac,     SIGNAL(clicked()), this, SLOT(CalcCharacteristics()));
	connect(pbRefImgComp,     SIGNAL(clicked()), this, SLOT(CalcRefImgComp()));
	connect(pbAllStore,       SIGNAL(clicked()), this, SLOT(StoreAll()));
	connect(pbHelp,           SIGNAL(clicked()), this, SLOT(Help()));

	connect(pbModalitySPLOM,  SIGNAL(clicked()), this, SLOT(ModalitySPLOM()));

	connect(m_dlgModalities,  SIGNAL(ModalityAvailable()), this, SLOT(DataAvailable()));
	connect(m_dlgLabels,      SIGNAL(SeedsAvailable()), this, SLOT(DataAvailable()));
	connect(m_dlgModalities,  SIGNAL(ShowImage(vtkSmartPointer<vtkImageData>)), this, SLOT(ShowImage(vtkSmartPointer<vtkImageData>)));

	connect(pbResetFilters,   SIGNAL(clicked()), this, SLOT(ResetFilters()));

	connect(slMagicLensOpacity, SIGNAL(valueChanged(int)), this, SLOT(SetMagicLensOpacity(int)));
	connect(sbClusterViewPreviewSize, SIGNAL(valueChanged(int)), this, SLOT(SetIconSize(int)));
	connect(cbColorThemes, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(SetColorTheme(const QString &)));
	connect(cbRepresentative, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(SetRepresentative(const QString &)));
	
	DataAvailable();
}


void dlg_GEMSeControl::StartSampling()
{
	if (!m_dlgModalities->GetModalities()->size())
	{
		DEBUG_LOG("No data available.\n");
		return;
	}
	if (!m_dlgLabels->AreSeedsAvailable())
	{
		QMessageBox::warning(this, "Segmentation Explorer", "No seeds for SVM available. Sampling requires seeds!");
		return;
	}
	if (m_dlgSamplingSettings || m_sampler)
	{
		QMessageBox::warning(this, "Segmentation Explorer", "Another sampler still running / dialog is still open...");
		return;
	}
	m_dlgSamplingSettings = new dlg_samplingSettings(this, m_dlgModalities->GetModalities());
	if (m_dlgSamplingSettings->exec() == QDialog::Accepted)
	{
		// get parameter ranges
		QSharedPointer<iAMMSegParameterRange> range = m_dlgSamplingSettings->GetRange();
		bool distFuncDefined = true;
		for (int i=0; i<range->modalityParamRange.size(); ++i)
		{
			if (range->modalityParamRange[i].distanceFuncs.empty())
			{
				distFuncDefined = false;
				break;
			}
		}
		if (!distFuncDefined)
		{
			QMessageBox::warning(this, "Segmentation Explorer", "You need to specify at least one distance function per modality");
			delete m_dlgSamplingSettings;
			m_dlgSamplingSettings = 0;
			return;
		}
		m_outputFolder = m_dlgSamplingSettings->GetOutputFolder();
		QDir outputFolder(m_outputFolder);
		outputFolder.mkpath(".");
		
		m_sampler = QSharedPointer<iAMMSegSampler>(new iAMMSegSampler(
			m_dlgModalities->GetModalities(),
			range,
			m_dlgSamplingSettings->GetGenerator(),
			m_dlgLabels->GetSeeds(),
			m_outputFolder,
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			m_dlgSamplingSettings->DoStoreProbabilities()) );
		m_dlgProgress = new dlg_progress(this, m_sampler, m_sampler, "Sampling Progress");
		MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
		mdiChild->splitDockWidget(this, m_dlgProgress, Qt::Vertical);
		connect(m_sampler.data(), SIGNAL(finished()), this, SLOT(SamplingFinished()) );
		connect(m_sampler.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)) );
		connect(m_sampler.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)) );
		
		// trigger parameter set creation & sampling (in foreground with progress bar for now)
		m_sampler->start();
	}
	delete m_dlgSamplingSettings;
	m_dlgSamplingSettings = 0;
}

#include <iAQTtoUIConnector.h>
typedef iAQTtoUIConnector<QDockWidget, Ui_assistant> dlg_assistantUI;

void dlg_GEMSeControl::Help()
{
	dlg_assistantUI* assistant = new dlg_assistantUI();
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	mdiChild->tabifyDockWidget(this, assistant);
}

void dlg_GEMSeControl::LoadSampling()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		QString(), // TODO get directory of current file
		tr("Sampling data file (*.smp );;" ) );
	if (!fileName.isEmpty())
	{
		LoadSampling(fileName);
	}
}

bool dlg_GEMSeControl::LoadSampling(QString const & fileName)
{
	if (fileName.isEmpty())
	{
		DEBUG_LOG("No filename given, not loading.\n");
		return false;
	}
	m_samplingResults = iASamplingResults::Load(fileName);
	if (m_samplingResults)
	{
		pbSamplingStore->setEnabled(true);
		pbClusteringCalc->setEnabled(true);
		pbClusteringLoad->setEnabled(true);
		pbCalcCharac->setEnabled(true);
		pbAllStore->setEnabled(true);
		pbResetFilters->setEnabled(true);
		QFileInfo fi(fileName);
		m_outputFolder = fi.absolutePath();
	}
	else
	{
		DEBUG_LOG("Loading Sampling failed.\n");
		return false;
	}
	return true;
}

void dlg_GEMSeControl::SamplingFinished()
{
	// retrieve results from sampler
	m_samplingResults = m_sampler->GetResults();
	delete m_dlgProgress;
	m_dlgProgress = 0;

	if (m_sampler->IsAborted())
	{
		DEBUG_LOG("Since Sampling was aborted, we're skipping clustering\n");
		m_sampler.clear();
		return;
	}
	m_sampler.clear();

	CalculateClustering();

	pbSamplingStore->setEnabled(true);
	pbClusteringCalc->setEnabled(true);
	pbClusteringLoad->setEnabled(true);
	pbCalcCharac->setEnabled(true);
	pbAllStore->setEnabled(true);
	pbResetFilters->setEnabled(true);
}

void dlg_GEMSeControl::LoadClustering()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		QString(), // TODO get directory of current file
		tr("Clustering filt(*.clt );;" ) );
	if (!fileName.isEmpty())
	{
		m_cltFile = fileName;
		LoadClustering(fileName);
	}
}

bool dlg_GEMSeControl::LoadClustering(QString const & fileName)
{
	assert(m_samplingResults);
	if (!m_samplingResults || fileName.isEmpty())
	{
		DEBUG_LOG("No Clustering available!\n");
		return false;
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	vtkSmartPointer<vtkImageData> originalImage = mdiChild->getImageData();
	QSharedPointer<iAImageTree> tree = iAImageTree::Create(fileName, m_samplingResults->GetResults(), m_dlgLabels->count());
	if (!tree)
	{
		DEBUG_LOG("Loading Clustering failed!\n");
		return false;
	}
	m_dlgGEMSe->SetTree(
		tree,
		originalImage,
		m_samplingResults->GetAttributes(),
		m_dlgModalities->GetModalities(),
		*m_dlgLabels
	);
	pbClusteringStore->setEnabled(true);
	m_cltFile = fileName;
	return true;
}

void dlg_GEMSeControl::CalculateClustering()
{
	assert(m_samplingResults);
	if (!m_samplingResults || m_samplingResults->size() == 0)
	{
		DEBUG_LOG("No Sampling Results available!\n");
		return;
	}
	assert( !m_dlgProgress );
	if (m_dlgProgress)
	{
		DEBUG_LOG("Other operation still running?\n");
		return;
	}
	QString dir = m_outputFolder+"/representatives";
	QDir qdir;
	if (!qdir.mkpath(dir))
	{
		DEBUG_LOG("Can't create representative directory!\n");
	}
	if (dir.isEmpty())
	{
		dir = QFileDialog::getExistingDirectory(this, tr("Output Directory"), QString());
		if (dir.isEmpty())
		{
			return;
		}
	}
	m_clusterer = QSharedPointer<iAImageClusterer>(new iAImageClusterer(m_dlgLabels->count(), dir));
	m_dlgProgress = new dlg_progress(this, m_clusterer, m_clusterer, "Clustering Progress");
	for (int i=0; i<m_samplingResults->size(); ++i)
	{
		m_clusterer->AddImage(m_samplingResults->Get(i));
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	mdiChild->splitDockWidget(this, m_dlgProgress, Qt::Vertical);
	connect(m_clusterer.data(), SIGNAL(finished()), this, SLOT(ClusteringFinished()) );
	connect(m_clusterer.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)) );
	connect(m_clusterer.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)) );
	m_clusterer->start();
}

void dlg_GEMSeControl::ClusteringFinished()
{
	delete m_dlgProgress;
	m_dlgProgress = 0;
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	vtkSmartPointer<vtkImageData> originalImage = mdiChild->getImageData();

	QSharedPointer<iAImageTree> tree = m_clusterer->GetResult();
	pbClusteringStore->setEnabled(true);
	assert(m_dlgGEMSe);
	if (!m_dlgGEMSe)
	{
		DEBUG_LOG("Segmentation Explorer not set!\n");
		return;
	}
	if (m_clusterer->IsAborted() || !m_clusterer->GetResult())
	{
		DEBUG_LOG("Clusterer aborted / missing Clustering Result!\n");
		return;
	}
	if (!m_outputFolder.isEmpty())
	{
		m_cltFile = m_outputFolder+"/"+iASEAFile::DefaultCLTFileName;
	
		m_samplingResults->Store(
			m_outputFolder + "/" + iASEAFile::DefaultSMPFileName,
			m_outputFolder + "/" + iASEAFile::DefaultSPSFileName,
			m_outputFolder + "/" + iASEAFile::DefaultCHRFileName);
		m_clusterer->GetResult()->Store(m_cltFile);

		if (m_dlgLabels->GetFileName().isEmpty())
		{
			m_dlgLabels->Store(iASEAFile::DefaultSeedFileName);
		}

		if (m_dlgModalities->GetModalities()->GetFileName().isEmpty())
		{
			m_dlgModalities->Store(m_outputFolder+"/"+iASEAFile::DefaultModalityFileName);
		}

		iASEAFile metaFile(
			m_dlgModalities->GetModalities()->GetFileName(),
			m_dlgLabels->GetFileName(),
			m_samplingResults->GetFileName(),
			m_cltFile,
			"");		// TODO: store current layout
		metaFile.Store(m_outputFolder+"/sampling.sea");
	}

	m_dlgGEMSe->SetTree(
		m_clusterer->GetResult(),
		originalImage,
		m_samplingResults->GetAttributes(),
		m_dlgModalities->GetModalities(),
		*m_dlgLabels
	);

}

void dlg_GEMSeControl::StoreClustering()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save clustering"),
		QString(), // TODO get directory of current file
		tr("Clustering file (*.clt );;" ) );
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->StoreClustering(fileName);
	}
}

void dlg_GEMSeControl::StoreSampling()
{
	QString smpFile = QFileDialog::getSaveFileName(this, tr("Save sampling data"),
		QString(), // TODO get directory of current file
		tr("Sampling data file (*.smp );;" ) );
	if (smpFile.isEmpty())
		return;
	QString spsFile = QFileDialog::getSaveFileName(this, tr("Save sampling parameter set"),
		QString(), // TODO get directory of current file
		tr("Sampling parameter set (*.sps );;" ) );
	if (spsFile.isEmpty())
		return;
	QString chrFile = QFileDialog::getSaveFileName(this, tr("Save sampling characteristics"),
		QString(), // TODO get directory of current file
		tr("Sampling characteristics file (*.chr );;" ) );
	if (chrFile.isEmpty())
		return;
	m_samplingResults->Store(smpFile, spsFile, chrFile);
}

void dlg_GEMSeControl::CalcCharacteristics()
{
	assert ( m_samplingResults );
	
	if (!m_samplingResults)
	{
		return;
	}
	QString chrFile = QFileDialog::getSaveFileName(this, tr("Save sampling characteristics"),
		QString(), // TODO get directory of current file
		tr("Sampling characteristics file (*.chr );;" ) );
	if (chrFile.isEmpty())
		return;

	int derivedOutputStart = NonModalityParamCount + m_dlgModalities->GetModalities()->size() * ModalityParamCount;


	for (int i=0; i<m_samplingResults->size(); ++i)
	{
		CharacteristicsCalculator calc(m_samplingResults->Get(i), m_samplingResults->GetAttributes(), derivedOutputStart);
		/* TODO: perform async!
		connect(calc, SIGNAL(finished()), this, SLOT(CharacteristicsCalculated()));
		*/
		calc.start();
		calc.wait();
	}
	QString smp(m_samplingResults->GetFileName());
	if (smp.isEmpty())
	{
		smp = m_outputFolder + "/" + iASEAFile::DefaultSMPFileName;
	}
	m_samplingResults->Store(
		smp,
		m_outputFolder + "/" + iASEAFile::DefaultSPSFileName,
		chrFile);
	m_dlgGEMSe->RecreateCharts();
}


void dlg_GEMSeControl::DataAvailable()
{
	pbSample->setEnabled(m_dlgModalities->GetModalities()->size() > 0 && m_dlgLabels->AreSeedsAvailable());
	pbSamplingLoad->setEnabled(m_dlgModalities->GetModalities()->size() > 0 && m_dlgLabels->AreSeedsAvailable());
}

void dlg_GEMSeControl::StoreAll()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save all"),
		QString(), // TODO get directory of current file
		tr("Segmentation Explorer Analysis (*.sea );;" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	iASEAFile metaFile(
		m_dlgModalities->GetModalities()->GetFileName(),
		m_dlgLabels->GetFileName(),
		m_samplingResults->GetFileName(),
		m_cltFile,
		"");		// TODO: store current layout
	metaFile.Store(fileName);
}

void dlg_GEMSeControl::ShowImage(vtkSmartPointer<vtkImageData> imgData)
{
	m_dlgGEMSe->ShowImage(imgData);
}

void ExportClusterIDs(QSharedPointer<iAImageClusterNode> node, std::ostream & out)
{
	if (node->GetChildCount() > 0)
	{
		for (int i = 0; i < node->GetChildCount(); ++i)
		{
			ExportClusterIDs(node->GetChild(i), out);
		}
	}
	else
	{
		static int curr = 0;
		out << node->GetID() << "\n";
	}
}


void dlg_GEMSeControl::ExportIDs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export cluster IDs"),
		QString(), // TODO get directory of current file
		tr("Comma-separated values (*.csv);;"));
	if (fileName.isEmpty())
	{
		return;
	}
	QSharedPointer<iAImageClusterNode> cluster = m_dlgGEMSe->GetCurrentCluster();
	std::ofstream out(fileName.toStdString());
	ExportClusterIDs(cluster, out);
}

#include "dlg_modalitySPLOM.h"


void dlg_GEMSeControl::ModalitySPLOM()
{
	m_dlgModalitySPLOM = new dlg_modalitySPLOM();
	m_dlgModalitySPLOM->SetData(m_dlgModalities->GetModalities());
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	mdiChild->tabifyDockWidget(this, m_dlgModalitySPLOM);
}


void dlg_GEMSeControl::ResetFilters()
{
	m_dlgGEMSe->ResetFilters();
}

void dlg_GEMSeControl::CalcRefImgComp()
{
	QSharedPointer<iAModalityList const> mods = m_dlgModalities->GetModalities();
	iAConnector con;
	con.SetImage(mods->Get(mods->size() - 1)->GetImage());

	m_dlgGEMSe->CalcRefImgComp(dynamic_cast<LabelImageType*>(con.GetITKImage()));
}

void dlg_GEMSeControl::SetMagicLensOpacity(int newValue)
{
	double opacity = (double)newValue / slMagicLensOpacity->maximum();
	m_dlgGEMSe->SetMagicLensOpacity(opacity);
}


void dlg_GEMSeControl::SetIconSize(int newSize)
{
	m_dlgGEMSe->SetIconSize(newSize);
}

void dlg_GEMSeControl::SetColorTheme(const QString &themeName)
{
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme(themeName);
	m_dlgLabels->SetColorTheme(theme);
	m_dlgGEMSe->SetColorTheme(theme, *m_dlgLabels);
}


void dlg_GEMSeControl::SetRepresentative(const QString & reprType)
{
	// Difference
	// Average Entropy
	// Label Distribution
	int representativeType =
		(reprType == "Difference") ?			iARepresentativeType::Difference :
		(reprType == "Label Distribution") ?	iARepresentativeType::LabelDistribution :
		(reprType == "Average Label") ?         iARepresentativeType::AverageLabel:
		/* reprType == "Average Entropy" */		iARepresentativeType::AverageEntropy;
	m_dlgGEMSe->SetRepresentativeType(representativeType);
}


void dlg_GEMSeControl::ExportAttributeRangeRanking()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Attribute Range Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportAttributeRangeRanking(fileName);
	}
}


void dlg_GEMSeControl::ExportRankings()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportRankings(fileName);
	}
}


void dlg_GEMSeControl::ImportRankings()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ImportRankings(fileName);
	}
}


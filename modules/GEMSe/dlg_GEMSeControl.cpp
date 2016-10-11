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
#include "dlg_GEMSeControl.h"

#include "dlg_commoninput.h"
#include "dlg_labels.h"
#include "dlg_modalities.h"
#include "dlg_samplings.h"
#include "dlg_samplingSettings.h"
#include "dlg_progress.h"
#include "dlg_GEMSe.h"
#include "iAAttributes.h"
#include "iACharacteristics.h"
#include "iAColorTheme.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAImageSampler.h"
#include "iALabelInfo.h"
#include "iAModality.h"
#include "iAImageClusterer.h"
#include "iASamplingResults.h"
#include "iASEAFile.h"
#include "mdichild.h"

#include <itkVTKImageToImageFilter.h>

#include "ParametrizableLabelVotingImageFilter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

class iASimpleLabelInfo : public iALabelInfo
{
private:
	int m_labelCount;
	iAColorTheme const * m_theme;
public:
	iASimpleLabelInfo() :
		m_labelCount(-1),
		m_theme(0)
	{}
	virtual int count() const
	{
		return m_labelCount;
	}
	virtual QString GetName(int idx) const
	{
		assert(idx >= 0 && idx < m_labelCount);
		return QString("Label %1").arg(idx);
	}

	virtual QColor GetColor(int idx) const
	{
		assert(m_theme);
		if (!m_theme)
			return QColor(0, 0, 0);
		return m_theme->GetColor(idx);
	}

	void SetLabelCount(int labelCount)
	{
		m_labelCount = labelCount;
	}

	void SetColorTheme(iAColorTheme const * theme)
	{
		m_theme = theme;
	}
};


dlg_GEMSeControl::dlg_GEMSeControl(
	QWidget *parentWidget,
	dlg_GEMSe* dlgGEMSe,
	dlg_modalities* dlgModalities,
	dlg_labels* dlgLabels,
	dlg_samplings* dlgSamplings,
	iAColorTheme const * colorTheme
):
	dlg_GEMSeControlUI(parentWidget),
	m_dlgSamplingSettings(0),
	m_dlgProgress(0),
	m_dlgGEMSe(dlgGEMSe),
	m_dlgModalities(dlgModalities),
	m_dlgLabels(dlgLabels),
	m_dlgSamplings(dlgSamplings),
	m_simpleLabelInfo(new iASimpleLabelInfo())
{
	dlgLabels->hide();
	m_simpleLabelInfo->SetColorTheme(colorTheme);
	for (QString themeName : iAColorThemeManager::GetInstance().GetAvailableThemes())
	{
		cbColorThemes->addItem(themeName);
		if (themeName == colorTheme->GetName())
		{
			cbColorThemes->setCurrentText(themeName);
		}
	}

	connect(pbSample,           SIGNAL(clicked()), this, SLOT(StartSampling()));
	connect(pbSamplingLoad,     SIGNAL(clicked()), this, SLOT(LoadSampling()));
	connect(pbClusteringCalc,   SIGNAL(clicked()), this, SLOT(CalculateClustering()));
	connect(pbClusteringLoad,   SIGNAL(clicked()), this, SLOT(LoadClustering()));
	connect(pbClusteringStore,  SIGNAL(clicked()), this, SLOT(StoreClustering()));
	connect(pbRefImgComp,       SIGNAL(clicked()), this, SLOT(CalcRefImgComp()));
	connect(pbAllStore,         SIGNAL(clicked()), this, SLOT(StoreAll()));
	connect(pbResetFilters,     SIGNAL(clicked()), m_dlgGEMSe, SLOT(ResetFilters()));
	connect(pbSelectHistograms, SIGNAL(clicked()), m_dlgGEMSe, SLOT(SelectHistograms()));
	connect(pbMajorityVoting, SIGNAL(clicked()), this, SLOT(MajorityVoting()));

	connect(m_dlgModalities,  SIGNAL(ModalityAvailable()), this, SLOT(DataAvailable()));
	connect(m_dlgLabels,      SIGNAL(SeedsAvailable()), this, SLOT(DataAvailable()));
	connect(m_dlgModalities,  SIGNAL(ShowImage(vtkSmartPointer<vtkImageData>)), this, SLOT(ShowImage(vtkSmartPointer<vtkImageData>)));


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
		DEBUG_LOG("No data available.");
		return;
	}
	if (m_dlgSamplingSettings || m_sampler)
	{
		DEBUG_LOG("Cannot start sampling while another sampling is still running...");
		QMessageBox::warning(this, "GEMSe", "Another sampler still running / dialog is still open...");
		return;
	}
	m_dlgSamplingSettings = new dlg_samplingSettings(this, m_dlgModalities->GetModalities());
	if (m_dlgSamplingSettings->exec() == QDialog::Accepted)
	{
		// get parameter ranges
		QSharedPointer<iAAttributes> parameters = m_dlgSamplingSettings->GetAttributes();
		m_outputFolder = m_dlgSamplingSettings->GetOutputFolder();
		QDir outputFolder(m_outputFolder);
		outputFolder.mkpath(".");
		if (m_dlgSamplingSettings->GetLabelCount() < 2)
		{
			DEBUG_LOG("Label Count must not be smaller than 2!");
			QMessageBox::warning(this, "GEMSe", "Label Count must not be smaller than 2!");
			return;
		}
		m_simpleLabelInfo->SetLabelCount(m_dlgSamplingSettings->GetLabelCount());
		m_sampler = QSharedPointer<iAImageSampler>(new iAImageSampler(
			m_dlgModalities->GetModalities(),
			parameters,
			m_dlgSamplingSettings->GetGenerator(),
			m_dlgSamplingSettings->GetSampleCount(),
			m_dlgSamplingSettings->GetLabelCount(),
			m_outputFolder,
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			m_dlgSamplingSettings->GetExecutable(),
			m_dlgSamplingSettings->GetAdditionalArguments()
		));
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


void dlg_GEMSeControl::LoadSampling()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		QString(), // TODO get directory of current file
		tr("Sampling data file (*.smp );;" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	int labelCount = m_simpleLabelInfo->count();
	if (labelCount < 2)
	{
		QStringList inList;
		inList << tr("*Label Count");
		QList<QVariant> inPara;
		inPara << tr("%1").arg(2);
		dlg_commoninput lblCountInput(this, "Label Count", 1, inList, inPara, nullptr);
		if (lblCountInput.exec() != QDialog::Accepted)
		{
			DEBUG_LOG("Cannot load sampling without label count input!");
			return;
		}
		labelCount = lblCountInput.getSpinBoxValues()[0];
	}
	LoadSampling(fileName, labelCount, iASamplingResults::GetNewID());
}


bool dlg_GEMSeControl::LoadSampling(QString const & fileName, int labelCount, int datasetID)
{
	m_simpleLabelInfo->SetLabelCount(labelCount);
	if (fileName.isEmpty())
	{
		DEBUG_LOG("No filename given, not loading.");
		return false;
	}
	QSharedPointer<iASamplingResults> samplingResults = iASamplingResults::Load(fileName, datasetID);
	if (!samplingResults)
	{
		DEBUG_LOG("Loading Sampling failed.");
		return false;
	}
	m_dlgSamplings->Add(samplingResults);
	pbClusteringCalc->setEnabled(true);
	pbClusteringLoad->setEnabled(true);
	pbAllStore->setEnabled(true);
	pbResetFilters->setEnabled(true);
	QFileInfo fi(fileName);
	m_outputFolder = fi.absolutePath();
	return true;
}


void dlg_GEMSeControl::SamplingFinished()
{
	// retrieve results from sampler
	QSharedPointer<iASamplingResults> samplingResults = m_sampler->GetResults();
	delete m_dlgProgress;
	m_dlgProgress = 0;

	if (!samplingResults || m_sampler->IsAborted())
	{
		DEBUG_LOG("Sampling was aborted, skipping clustering.");
		m_sampler.clear();
		return;
	}
	m_sampler.clear();
	m_dlgSamplings->Add(samplingResults);

	samplingResults->Store(
		m_outputFolder + "/" + iASEAFile::DefaultSMPFileName,
		m_outputFolder + "/" + iASEAFile::DefaultSPSFileName,
		m_outputFolder + "/" + iASEAFile::DefaultCHRFileName);

	pbClusteringCalc->setEnabled(true);
	pbClusteringLoad->setEnabled(true);
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
	if (m_simpleLabelInfo->count() < 2)
	{
		DEBUG_LOG("Label Count must not be smaller than 2!");
		return false;
	}
	assert(m_dlgSamplings->SamplingCount() > 0);
	if (m_dlgSamplings->SamplingCount() == 0 || fileName.isEmpty())
	{
		DEBUG_LOG("No sampling data is available!");
		return false;
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	vtkSmartPointer<vtkImageData> originalImage = mdiChild->getImageData();
	QSharedPointer<iAImageTree> tree = iAImageTree::Create(
		fileName,
		m_dlgSamplings->GetSamplings(),
		m_simpleLabelInfo->count()
	);
	if (!tree)
	{
		DEBUG_LOG("Loading Clustering failed!");
		return false;
	}
	double * origSpacing = originalImage->GetSpacing();
	const itk::Vector<double, 3> resultSpacing =
		tree->m_root->GetRepresentativeImage(iARepresentativeType::Difference)->GetSpacing();
	if (origSpacing[0] != resultSpacing[0] ||
		origSpacing[1] != resultSpacing[1] ||
		origSpacing[2] != resultSpacing[2])
	{
		DEBUG_LOG("Spacing of original images and of result images does not match!");
	}
	m_dlgGEMSe->SetTree(
		tree,
		originalImage,
		m_dlgModalities->GetModalities(),
		*m_simpleLabelInfo.data(),
		m_dlgSamplings->GetSamplings()
	);
	EnableClusteringDependantButtons();
	m_cltFile = fileName;
	return true;
}


void dlg_GEMSeControl::CalculateClustering()
{
	if (m_dlgSamplings->SamplingCount() == 0)
	{
		DEBUG_LOG("No Sampling Results available!");
		return;
	}
	assert( !m_dlgProgress );
	if (m_dlgProgress)
	{
		DEBUG_LOG("Other operation still running?");
		return;
	}
	QString m_outputFolder = QFileDialog::getExistingDirectory(this, tr("Output Directory"), QString());
	if (m_outputFolder.isEmpty())
	{
		return;
	}
	QString cacheDir = m_outputFolder + "/representatives";
	QDir qdir;
	if (!qdir.mkpath(cacheDir))
	{
		DEBUG_LOG("Can't create representative directory!");
		return;
	}
	m_clusterer = QSharedPointer<iAImageClusterer>(new iAImageClusterer(m_simpleLabelInfo->count(), cacheDir));
	m_dlgProgress = new dlg_progress(this, m_clusterer, m_clusterer, "Clustering Progress");
	for (int samplingIdx=0; samplingIdx<m_dlgSamplings->SamplingCount(); ++samplingIdx)
	{
		QSharedPointer<iASamplingResults> sampling = m_dlgSamplings->GetSampling(samplingIdx);
		for (int sampleIdx = 0; sampleIdx < sampling->size(); ++sampleIdx)
		{
			m_clusterer->AddImage(sampling->Get(sampleIdx));
		}
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
	EnableClusteringDependantButtons();
	assert(m_dlgGEMSe);
	if (!m_dlgGEMSe)
	{
		DEBUG_LOG("GEMSe not initialized!");
		return;
	}
	if (m_clusterer->IsAborted() || !m_clusterer->GetResult())
	{
		DEBUG_LOG("Clusterer aborted / missing Clustering Result!");
		return;
	}
	if (!m_outputFolder.isEmpty())
	{
		m_cltFile = m_outputFolder+"/"+iASEAFile::DefaultCLTFileName;
		m_clusterer->GetResult()->Store(m_cltFile);

		if (m_dlgModalities->GetModalities()->GetFileName().isEmpty())
		{
			m_dlgModalities->Store(m_outputFolder+"/"+iASEAFile::DefaultModalityFileName);
		}
		StoreGEMSeProject(m_outputFolder + "/sampling.sea");
	}
	m_dlgGEMSe->SetTree(
		m_clusterer->GetResult(),
		originalImage,
		m_dlgModalities->GetModalities(),
		*m_simpleLabelInfo.data(),
		m_dlgSamplings->GetSamplings()
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


void dlg_GEMSeControl::DataAvailable()
{
	pbSample->setEnabled(m_dlgModalities->GetModalities()->size() > 0);
	pbSamplingLoad->setEnabled(m_dlgModalities->GetModalities()->size() > 0);
}


void dlg_GEMSeControl::StoreAll()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save all"),
		QString(), // TODO get directory of current file
		tr("GEMSe project (*.sea );;") );
	if (fileName.isEmpty())
	{
		return;
	}
	StoreGEMSeProject(fileName);
}


void dlg_GEMSeControl::StoreGEMSeProject(QString const & fileName)
{
	QMap<int, QString> samplingFilenames;
	for (QSharedPointer<iASamplingResults> sampling : m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->GetID(), sampling->GetFileName());
	}
	iASEAFile metaFile(
		m_dlgModalities->GetModalities()->GetFileName(),
		m_simpleLabelInfo->count(),
		samplingFilenames,
		m_cltFile,
		"");		// TODO: store current layout
	metaFile.Store(fileName);
}


void dlg_GEMSeControl::EnableClusteringDependantButtons()
{
	pbClusteringStore->setEnabled(true);
	pbRefImgComp->setEnabled(true);
	pbSelectHistograms->setEnabled(true);
	pbMajorityVoting->setEnabled(true);
}


void dlg_GEMSeControl::ShowImage(vtkSmartPointer<vtkImageData> imgData)
{
	m_dlgGEMSe->ShowImage(imgData);
}


void ExportClusterIDs(QSharedPointer<iAImageTreeNode> node, std::ostream & out)
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
	QSharedPointer<iAImageTreeNode> cluster = m_dlgGEMSe->GetCurrentCluster();
	std::ofstream out(fileName.toStdString());
	ExportClusterIDs(cluster, out);
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
	m_simpleLabelInfo->SetColorTheme(theme);
	m_dlgGEMSe->SetColorTheme(theme, *m_simpleLabelInfo.data());
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

#include "iASingleResult.h"

void dlg_GEMSeControl::MajorityVoting()
{
	typedef itk::Image<unsigned int, 3> UIntImageType;
	typedef itk::CastImageFilter<LabelImageType, UIntImageType> CastImageFilter;
	typedef ParametrizableLabelVotingImageFilter<UIntImageType> LabelVotingType;
	LabelVotingType::Pointer labelVotingFilter;
	labelVotingFilter = LabelVotingType::New();
	labelVotingFilter->SetDecisionMinimumPercentage(
		static_cast<double>(slMajorityVotingMinimumPercentage->value()) /
			slMajorityVotingMinimumPercentage->maximum());
	// iterate over currently selected cluster and add all non-filtered images

	QVector<QSharedPointer<iASingleResult> > m_selection;
	m_dlgGEMSe->GetSelection(m_selection);
	for (unsigned int i = 0; i < static_cast<unsigned int>(m_selection.size()); ++i)
	{
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(m_selection[i]->GetLabelledImage().GetPointer());
		CastImageFilter::Pointer castImageFilter = CastImageFilter::New();
		castImageFilter->ReleaseDataFlagOff();
		castImageFilter->SetInput(lblImg);
		labelVotingFilter->SetInput(i, castImageFilter->GetOutput());
	}
	labelVotingFilter->Update();
	// Put output somewhere!
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

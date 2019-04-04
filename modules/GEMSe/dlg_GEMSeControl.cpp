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
#include "dlg_GEMSeControl.h"

#include "dlg_GEMSe.h"
#include "dlg_labels.h"
#include "dlg_Consensus.h"
#include "dlg_progress.h"
#include "dlg_samplings.h"
#include "dlg_samplingSettings.h"
#include "iAAttributes.h"
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAImageTreeLeaf.h" // for VisitLeafs
#include "iAImageSampler.h"
#include "iALabelInfo.h"
#include "iAImageClusterer.h"
#include "iASamplingResults.h"
#include "iASEAFile.h"

#include <dlg_commoninput.h>
#include <dlg_modalities.h>
#include <iAAttributeDescriptor.h>
#include <iAColorTheme.h>
#include <iAConnector.h>
#include <iAConsole.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAToolsITK.h>
#include <io/iAIOProvider.h>
#include <mdichild.h>

#include <vtkImageData.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

class iASimpleLabelInfo : public iALabelInfo
{
private:
	int m_labelCount;
	iAColorTheme const * m_theme;
	QStringList m_labelNames;
public:
	iASimpleLabelInfo() :
		m_labelCount(-1),
		m_theme(0)
	{}
	virtual ~iASimpleLabelInfo()
	{}
	virtual int count() const
	{
		return m_labelCount;
	}
	virtual QString name(int idx) const
	{
		assert(idx >= 0 && idx < m_labelCount);
		if (idx < m_labelNames.size())
		{
			return m_labelNames[idx];
		}
		else
		{
			return QString("Label %1").arg(idx);
		}
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

	iAColorTheme const * GetColorTheme() const
	{
		return m_theme;
	}
	void SetLabelNames(QStringList const & labelNames)
	{
		if (labelNames.size() > 1 || labelNames[0].length() > 0)
		{
			m_labelNames = labelNames;
		}
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
	m_dlgConsensus(0),
	m_dlgGEMSe(dlgGEMSe),
	m_dlgModalities(dlgModalities),
	m_dlgLabels(dlgLabels),
	m_dlgSamplings(dlgSamplings),
	m_simpleLabelInfo(new iASimpleLabelInfo())

{
	connect(m_dlgSamplings, SIGNAL(AddSampling()), this, SLOT(LoadSampling()));
	dlgLabels->hide();
	m_simpleLabelInfo->SetColorTheme(colorTheme);
	cbColorThemes->addItems(iAColorThemeManager::GetInstance().GetAvailableThemes());
	cbColorThemes->setCurrentText(colorTheme->name());

	connect(pbSample,           SIGNAL(clicked()), this, SLOT(StartSampling()));
	connect(pbSamplingLoad,     SIGNAL(clicked()), this, SLOT(LoadSampling()));
	connect(pbClusteringCalc,   SIGNAL(clicked()), this, SLOT(CalculateClustering()));
	connect(pbClusteringLoad,   SIGNAL(clicked()), this, SLOT(LoadClustering()));
	connect(pbClusteringStore,  SIGNAL(clicked()), this, SLOT(StoreClustering()));
	connect(pbAllStore,         SIGNAL(clicked()), this, SLOT(StoreAll()));
	connect(pbSelectHistograms, SIGNAL(clicked()), m_dlgGEMSe, SLOT(SelectHistograms()));
	connect(pbLoadRefImage,     SIGNAL(clicked()), this, SLOT(LoadRefImg()));
	connect(pbStoreDerivedOutput, SIGNAL(clicked()), this, SLOT(StoreDerivedOutput()));
	connect(pbFreeMemory, SIGNAL(clicked()), this, SLOT(FreeMemory()));

	connect(m_dlgModalities,  SIGNAL(modalityAvailable(int)), this, SLOT(DataAvailable()));
	connect(m_dlgModalities,  SIGNAL(modalitySelected(int)), this, SLOT(ModalitySelected(int)));

	connect(sbClusterViewPreviewSize, SIGNAL(valueChanged(int)), this, SLOT(SetIconSize(int)));
	connect(sbMagicLensCount, SIGNAL(valueChanged(int)), this, SLOT(SetMagicLensCount(int)));
	connect(cbColorThemes, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(SetColorTheme(const QString &)));
	connect(cbRepresentative, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(SetRepresentative(const QString &)));
	connect(cbProbabilityProbing, SIGNAL(stateChanged(int)), this, SLOT(SetProbabilityProbing(int)));
	connect(cbCorrectnessUncertainty, SIGNAL(stateChanged(int)), this, SLOT(SetCorrectnessUncertainty(int)));

	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	connect(mdiChild, SIGNAL(transferFunctionChanged()), this, SLOT(DataTFChanged()));
	
	DataAvailable();
}


void dlg_GEMSeControl::StartSampling()
{
	if (!m_dlgModalities->modalities()->size())
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
	m_dlgSamplingSettings = new dlg_samplingSettings(this, m_dlgModalities->modalities(), m_samplingSettings);
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
			m_dlgModalities->modalities(),
			parameters,
			m_dlgSamplingSettings->GetGenerator(),
			m_dlgSamplingSettings->GetSampleCount(),
			m_dlgSamplingSettings->GetLabelCount(),
			m_outputFolder,
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			m_dlgSamplingSettings->GetExecutable(),
			m_dlgSamplingSettings->GetAdditionalArguments(),
			m_dlgSamplingSettings->GetPipelineName(),
			m_dlgSamplingSettings->GetImageBaseName(),
			m_dlgSamplingSettings->GetSeparateFolder(),
			m_dlgSamplingSettings->GetCalcChar(),
			m_dlgSamplings->GetSamplings()->size()
		));
		m_dlgProgress = new dlg_progress(this, m_sampler, m_sampler, "Sampling Progress");
		MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
		mdiChild->tabifyDockWidget(this, m_dlgProgress);
		connect(m_sampler.data(), SIGNAL(finished()), this, SLOT(SamplingFinished()) );
		connect(m_sampler.data(), SIGNAL(Progress(int)), m_dlgProgress, SLOT(SetProgress(int)) );
		connect(m_sampler.data(), SIGNAL(Status(QString const &)), m_dlgProgress, SLOT(SetStatus(QString const &)) );
		
		// trigger parameter set creation & sampling (in foreground with progress bar for now)
		m_sampler->start();
		m_dlgSamplingSettings->GetValues(m_samplingSettings);
	}
	delete m_dlgSamplingSettings;
	m_dlgSamplingSettings = 0;
}


void dlg_GEMSeControl::LoadSampling()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Sampling"),
		QString(), // TODO get directory of current file
		tr("Attribute Descriptor file (*.smp );;" ) );
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
		dlg_commoninput lblCountInput(this, "Label Count", inList, inPara, nullptr);
		if (lblCountInput.exec() != QDialog::Accepted)
		{
			DEBUG_LOG("Cannot load sampling without label count input!");
			return;
		}
		labelCount = lblCountInput.getIntValue(0);
	}
	LoadSampling(fileName, labelCount, m_dlgSamplings->GetSamplings()->size());
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
	EnableSamplingDependantUI();
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
		m_sampler.clear();
		return;
	}
	m_sampler.clear();
	m_dlgSamplings->Add(samplingResults);
	samplingResults->Store(
		m_outputFolder + "/" + iASEAFile::DefaultSMPFileName,
		m_outputFolder + "/" + iASEAFile::DefaultSPSFileName,
		m_outputFolder + "/" + iASEAFile::DefaultCHRFileName);
	EnableSamplingDependantUI();
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
	vtkSmartPointer<vtkImageData> originalImage = mdiChild->imagePointer();
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
		tree->m_root->GetRepresentativeImage(iARepresentativeType::Difference,
			LabelImagePointer())->GetSpacing();
	if (origSpacing[0] != resultSpacing[0] ||
		origSpacing[1] != resultSpacing[1] ||
		origSpacing[2] != resultSpacing[2])
	{
		DEBUG_LOG("Spacing of original images and of result images does not match!");
	}
	m_dlgGEMSe->SetTree(
		tree,
		originalImage,
		m_dlgModalities->modalities(),
		m_simpleLabelInfo.data(),
		m_dlgSamplings->GetSamplings()
	);
	EnableClusteringDependantUI();
	m_dlgConsensus->EnableUI();
	m_cltFile = fileName;
	QFileInfo fi(m_cltFile);
	m_outputFolder = fi.absolutePath();
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
	m_outputFolder = QFileDialog::getExistingDirectory(this, tr("Output Directory"), m_outputFolder);
	if (m_outputFolder.isEmpty())
	{
		return;
	}
	DEBUG_LOG(QString("Clustering and writing results to %1").arg(m_outputFolder));
	QString cacheDir = m_outputFolder + "/representatives";
	QDir qdir;
	if (!qdir.mkpath(cacheDir))
	{
		DEBUG_LOG(QString("Can't create representative directory %1!").arg(cacheDir));
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
	mdiChild->tabifyDockWidget(this, m_dlgProgress);
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
	vtkSmartPointer<vtkImageData> originalImage = mdiChild->imagePointer();

	QSharedPointer<iAImageTree> tree = m_clusterer->GetResult();
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
		m_cltFile = m_outputFolder + "/" + iASEAFile::DefaultCLTFileName;
		m_clusterer->GetResult()->Store(m_cltFile);

		if (m_dlgModalities->modalities()->fileName().isEmpty())
		{
			mdiChild->saveProject(m_outputFolder + "/" + iASEAFile::DefaultModalityFileName);
		}
		StoreGEMSeProject(m_outputFolder + "/sampling.sea", "");
	}
	m_dlgGEMSe->SetTree(
		m_clusterer->GetResult(),
		originalImage,
		m_dlgModalities->modalities(),
		m_simpleLabelInfo.data(),
		m_dlgSamplings->GetSamplings()
	);
	EnableClusteringDependantUI();
	m_dlgConsensus->EnableUI();
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
	pbSample->setEnabled(m_dlgModalities->modalities()->size() > 0);
	pbSamplingLoad->setEnabled(m_dlgModalities->modalities()->size() > 0);
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
	StoreGEMSeProject(fileName, m_dlgGEMSe->GetSerializedHiddenCharts());
}


void dlg_GEMSeControl::StoreGEMSeProject(QString const & fileName, QString const & hiddenCharts)
{
	QMap<int, QString> samplingFilenames;
	for (QSharedPointer<iASamplingResults> sampling : *m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->GetID(), sampling->GetFileName());
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	iASEAFile metaFile(
		m_dlgModalities->modalities()->fileName(),
		m_simpleLabelInfo->count(),
		samplingFilenames,
		m_cltFile,
		mdiChild->layoutName(),
		leRefImage->text(),
		hiddenCharts,
		m_simpleLabelInfo->GetColorTheme()->name(),
		m_dlgGEMSe->GetLabelNames()
	);
	metaFile.Store(fileName);
}


void dlg_GEMSeControl::EnableClusteringDependantUI()
{
	pbClusteringStore->setEnabled(true);
	pbSelectHistograms->setEnabled(true);
	if (!m_dlgConsensus)
	{
		MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
		m_dlgConsensus = new dlg_Consensus(mdiChild, m_dlgGEMSe, m_simpleLabelInfo->count(), m_outputFolder,
			m_dlgSamplings);
		if (m_refImg)
			m_dlgConsensus->SetGroundTruthImage(m_refImg);
		mdiChild->splitDockWidget(this, m_dlgConsensus, Qt::Vertical);
	}
}

void dlg_GEMSeControl::EnableSamplingDependantUI()
{
	pbClusteringCalc->setEnabled(true);
	pbClusteringLoad->setEnabled(true);
	pbAllStore->setEnabled(true);
	pbStoreDerivedOutput->setEnabled(true);
}


void dlg_GEMSeControl::ModalitySelected(int modalityIdx)
{
	vtkSmartPointer<vtkImageData> imgData = m_dlgModalities->modalities()->get(modalityIdx)->image();
	m_dlgGEMSe->ShowImage(imgData);
}


void ExportClusterIDs(QSharedPointer<iAImageTreeNode> node, std::ostream & out)
{
	VisitLeafs(node.data(), [&](iAImageTreeLeaf const * leaf)
	{
		static int curr = 0;
		out << leaf->GetDatasetID() << "\t" << leaf->GetID() << "\n";
	});
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
	std::ofstream out( getLocalEncodingFileName(fileName) );
	ExportClusterIDs(cluster, out);
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
	m_dlgGEMSe->SetColorTheme(theme, m_simpleLabelInfo.data());
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
		(reprType == "Correctness") ?           iARepresentativeType::Correctness:
		/* reprType == "Average Entropy" */		iARepresentativeType::AverageEntropy;
	if (!m_dlgGEMSe->SetRepresentativeType(representativeType, m_refImg))
	{   // could not set representative, reset
		int reprType = m_dlgGEMSe->GetRepresentativeType();
		cbRepresentative->setCurrentIndex(reprType);
	}
}

void dlg_GEMSeControl::LoadRefImg()
{
	QString refFileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Files"),
		"",
		iAIOProvider::MetaImages
	);
	if (refFileName.isEmpty())
		return;
	LoadRefImg(refFileName);
}

bool dlg_GEMSeControl::LoadRefImg(QString const & refImgName)
{
	try
	{
		iAITKIO::ScalarPixelType pixelType;
		auto img = iAITKIO::readFile(refImgName, pixelType, false);
		if (pixelType != itk::ImageIOBase::INT)
		{
			img = CastImageTo<int>(img);
		}
		m_refImg = dynamic_cast<LabelImageType*>(img.GetPointer());
		m_dlgGEMSe->CalcRefImgComp(m_refImg);
		if (m_dlgConsensus)
			m_dlgConsensus->SetGroundTruthImage(m_refImg);
	}
	catch (std::exception & e)
	{
		DEBUG_LOG(QString("Could not load reference image, problem: %1").arg(e.what()));
		return false;
	}
	leRefImage->setText(refImgName);
	return true;
}

void dlg_GEMSeControl::StoreDerivedOutput()
{
	SamplingVectorPtr samplings = m_dlgSamplings->GetSamplings();
	for (int i = 0; i < samplings->size(); ++i)
	{
		QString derivedOutputFileName = QFileDialog::getSaveFileName(this, tr("Save Derived Output"),
			QString(), // TODO get directory of current file
			tr("Derived Output (*.chr );;"));
		QString attributeDescriptorOutputFileName = QFileDialog::getSaveFileName(this, tr("Save Attribute Descriptor"),
			QString(), // TODO get directory of current file
			tr("Attribute Descriptor file (*.smp );;")
		);
		if (derivedOutputFileName.isEmpty() || attributeDescriptorOutputFileName.isEmpty())
		{
			return;
		}
		StoreDerivedOutput(derivedOutputFileName, attributeDescriptorOutputFileName, samplings->at(i));
	}
}


void dlg_GEMSeControl::StoreDerivedOutput(
	QString const & derivedOutputFileName,
	QString const & attributeDescriptorOutputFileName,
	QSharedPointer<iASamplingResults> results)
{

	// TODO: update smp file with all attribute descriptors
	// for now: write to separate descriptor file:
	QFile paramRangeFile(attributeDescriptorOutputFileName);
	if (!paramRangeFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Could not open parameter descriptor file '%1' for writing!").arg(attributeDescriptorOutputFileName));
		return;
	}
	QTextStream out(&paramRangeFile);
	results->GetAttributes()->Store(out);

	// store derived output:
	results->StoreAttributes(iAAttributeDescriptor::DerivedOutput, derivedOutputFileName, false);
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

void dlg_GEMSeControl::SetSerializedHiddenCharts(QString const & hiddenCharts)
{
	m_dlgGEMSe->SetSerializedHiddenCharts(hiddenCharts);
}


void dlg_GEMSeControl::SetMagicLensCount(int count)
{
	m_dlgGEMSe->SetMagicLensCount(count);
}


void dlg_GEMSeControl::FreeMemory()
{
	m_dlgGEMSe->FreeMemory();
}


void dlg_GEMSeControl::SetProbabilityProbing(int state)
{
	if (!m_dlgGEMSe)
		return;
	m_dlgGEMSe->SetProbabilityProbing(state == Qt::Checked);
}

void dlg_GEMSeControl::SetCorrectnessUncertainty(int state)
{
	if (!m_dlgGEMSe)
		return;
	m_dlgGEMSe->SetCorrectnessUncertaintyOverlay(state == Qt::Checked);
}

void dlg_GEMSeControl::DataTFChanged()
{
	if (!m_dlgGEMSe)
		return;
	m_dlgGEMSe->DataTFChanged();
}

void dlg_GEMSeControl::SetLabelInfo(QString const & colorTheme, QString const & labelNames)
{
	m_simpleLabelInfo->SetLabelNames(labelNames.split(","));
	int colorThemeIdx = cbColorThemes->findText(colorTheme);
	if (colorTheme != "" && colorThemeIdx != -1)
	{
		cbColorThemes->setCurrentIndex(colorThemeIdx);
			//SetColorTheme(); // maybe already done via signal, need to check
	}
}

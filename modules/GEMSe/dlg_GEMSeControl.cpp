/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAImageTreeLeaf.h" // for VisitLeafs
#include "iALabelInfo.h"
#include "iAImageClusterer.h"
#include "iASEAFile.h"

// MetaFilters
#include <iAImageSampler.h>
#include <iAParameterGeneratorImpl.h>
#include <iASamplingResults.h>
#include <iASampleParameterNames.h>
#include <dlg_samplingSettings.h>

// core
#include <dlg_commoninput.h>
#include <dlg_modalities.h>
#include <iAAttributes.h>
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
	int count() const override
	{
		return m_labelCount;
	}
	QString name(int idx) const override
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

	QColor color(int idx) const override
	{
		assert(m_theme);
		if (!m_theme)
			return QColor(0, 0, 0);
		return m_theme->color(idx);
	}

	void setLabelCount(int labelCount)
	{
		m_labelCount = labelCount;
	}

	void setColorTheme(iAColorTheme const * theme)
	{
		m_theme = theme;
	}

	iAColorTheme const * colorTheme() const
	{
		return m_theme;
	}
	void setLabelNames(QStringList const & labelNames)
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
	m_dlgModalities(dlgModalities),
	m_dlgSamplingSettings(nullptr),
	m_dlgProgress(nullptr),
	m_dlgGEMSe(dlgGEMSe),
	m_dlgLabels(dlgLabels),
	m_dlgSamplings(dlgSamplings),
	m_dlgConsensus(nullptr),
	m_simpleLabelInfo(new iASimpleLabelInfo())

{
	connect(m_dlgSamplings, &dlg_samplings::AddSampling, this, &dlg_GEMSeControl::loadSamplingSlot);
	dlgLabels->hide();
	m_simpleLabelInfo->setColorTheme(colorTheme);
	cbColorThemes->addItems(iAColorThemeManager::instance().availableThemes());
	cbColorThemes->setCurrentText(colorTheme->name());

	connect(pbSample, &QPushButton::clicked, this, &dlg_GEMSeControl::startSampling);
	connect(pbSamplingLoad, &QPushButton::clicked, this, &dlg_GEMSeControl::loadSamplingSlot);
	connect(pbClusteringCalc, &QPushButton::clicked, this, &dlg_GEMSeControl::calculateClustering);
	connect(pbClusteringLoad, &QPushButton::clicked, this, &dlg_GEMSeControl::loadClusteringSlot);
	connect(pbClusteringStore, &QPushButton::clicked, this, &dlg_GEMSeControl::saveClustering);
	connect(pbAllStore, &QPushButton::clicked, this, &dlg_GEMSeControl::saveAll);
	connect(pbSelectHistograms, &QPushButton::clicked, m_dlgGEMSe, &dlg_GEMSe::selectHistograms);
	connect(pbLoadRefImage, &QPushButton::clicked, this, &dlg_GEMSeControl::loadRefImgSlot);
	connect(pbStoreDerivedOutput, &QPushButton::clicked, this, &dlg_GEMSeControl::saveDerivedOutputSlot);
	connect(pbFreeMemory, &QPushButton::clicked, this, &dlg_GEMSeControl::freeMemory);

	connect(m_dlgModalities, &dlg_modalities::modalityAvailable, this, &dlg_GEMSeControl::dataAvailable);
	connect(m_dlgModalities, &dlg_modalities::modalitySelected, this, &dlg_GEMSeControl::modalitySelected);

	connect(sbClusterViewPreviewSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_GEMSeControl::SetIconSize);
	connect(sbMagicLensCount, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_GEMSeControl::setMagicLensCount);
	connect(cbColorThemes, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_GEMSeControl::setColorTheme);
	connect(cbRepresentative, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_GEMSeControl::setRepresentative);
	connect(cbProbabilityProbing, &QCheckBox::stateChanged, this, &dlg_GEMSeControl::setProbabilityProbing);
	connect(cbCorrectnessUncertainty, &QCheckBox::stateChanged, this, &dlg_GEMSeControl::setCorrectnessUncertainty);

	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	connect(mdiChild, &MdiChild::transferFunctionChanged, this, &dlg_GEMSeControl::dataTFChanged);

	dataAvailable();
}

void dlg_GEMSeControl::startSampling()
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
	m_dlgSamplingSettings = new dlg_samplingSettings(this, m_dlgModalities->modalities()->size(), m_samplingSettings);
	if (m_dlgSamplingSettings->exec() == QDialog::Accepted)
	{
		QSharedPointer<iAAttributes> parameterRanges = m_dlgSamplingSettings->parameterRanges();
		m_dlgSamplingSettings->getValues(m_samplingSettings);
		m_outputFolder = m_samplingSettings["Output folder"].toString();
		QDir outputFolder(m_outputFolder);
		outputFolder.mkpath(".");
		if (m_samplingSettings["Compute derived output"].toBool() &&
			m_samplingSettings["Number of labels"].toInt() < 2)
		{
			DEBUG_LOG("Label Count must not be smaller than 2!");
			QMessageBox::warning(this, "GEMSe", "Label Count must not be smaller than 2!");
			return;
		}
		m_simpleLabelInfo->setLabelCount(m_samplingSettings["Number of labels"].toInt());
		QStringList fileNames;
		auto datasets = m_dlgModalities->modalities();
		for (int i = 0; i < datasets->size(); ++i)
		{
			fileNames << datasets->get(i)->fileName();
		}
		auto parameterSetGenerator = GetParameterGenerator(m_samplingSettings[spnSamplingMethod].toString());
		if (!parameterSetGenerator)
		{
			return;
		}
		m_sampler = QSharedPointer<iAImageSampler>(new iAImageSampler(
			fileNames,
			parameterRanges,
			parameterSetGenerator,
			m_samplingSettings[spnNumberOfSamples].toInt(),
			m_samplingSettings[spnNumberOfLabels].toInt(),
			m_samplingSettings[spnOutputFolder].toString(),
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			m_samplingSettings[spnExecutable].toString(),
			m_samplingSettings[spnAdditionalArguments].toString(),
			m_samplingSettings[spnAlgorithmName].toString(),
			m_samplingSettings[spnBaseName].toString(),
			m_samplingSettings[spnSubfolderPerSample].toBool(),
			m_samplingSettings[spnComputeDerivedOutput].toBool(),
			m_samplingSettings[spnAbortOnError].toBool(),
			m_dlgSamplings->GetSamplings()->size()
		));
		m_dlgProgress = new dlg_progress(this, m_sampler, m_sampler, "Sampling Progress");
		MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
		mdiChild->tabifyDockWidget(this, m_dlgProgress);
		connect(m_sampler.data(), &iAImageSampler::finished, this, &dlg_GEMSeControl::samplingFinished);
		connect(m_sampler.data(), &iAImageSampler::progress, m_dlgProgress, &dlg_progress::setProgress);
		connect(m_sampler.data(), &iAImageSampler::status, m_dlgProgress, &dlg_progress::setStatus);

		// trigger parameter set creation & sampling (in foreground with progress bar for now)
		m_sampler->start();
	}
	delete m_dlgSamplingSettings;
	m_dlgSamplingSettings = 0;
}

void dlg_GEMSeControl::loadSamplingSlot()
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
	loadSampling(fileName, labelCount, m_dlgSamplings->GetSamplings()->size());
}

bool dlg_GEMSeControl::loadSampling(QString const & fileName, int labelCount, int datasetID)
{
	m_simpleLabelInfo->setLabelCount(labelCount);
	if (fileName.isEmpty())
	{
		DEBUG_LOG("No filename given, not loading.");
		return false;
	}
	QSharedPointer<iASamplingResults> samplingResults = iASamplingResults::load(fileName, datasetID);
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

void dlg_GEMSeControl::samplingFinished()
{
	// retrieve results from sampler
	QSharedPointer<iASamplingResults> samplingResults = m_sampler->results();
	delete m_dlgProgress;
	m_dlgProgress = 0;

	if (!samplingResults || m_sampler->isAborted())
	{
		m_sampler.clear();
		return;
	}
	m_sampler.clear();
	m_dlgSamplings->Add(samplingResults);
	samplingResults->store(
		m_outputFolder + "/" + iASEAFile::DefaultSMPFileName,
		m_outputFolder + "/" + iASEAFile::DefaultSPSFileName,
		m_outputFolder + "/" + iASEAFile::DefaultCHRFileName);
	EnableSamplingDependantUI();
}

void dlg_GEMSeControl::loadClusteringSlot()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		QString(), // TODO get directory of current file
		tr("Clustering filt(*.clt );;" ) );
	if (!fileName.isEmpty())
	{
		m_cltFile = fileName;
		loadClustering(fileName);
	}
}

bool dlg_GEMSeControl::loadClustering(QString const & fileName)
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

void dlg_GEMSeControl::calculateClustering()
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
			m_clusterer->AddImage(sampling->get(sampleIdx));
		}
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	mdiChild->tabifyDockWidget(this, m_dlgProgress);
	connect(m_clusterer.data(), &iAImageClusterer::finished, this, &dlg_GEMSeControl::clusteringFinished);
	connect(m_clusterer.data(), &iAImageClusterer::Progress, m_dlgProgress, &dlg_progress::setProgress);
	connect(m_clusterer.data(), &iAImageClusterer::Status, m_dlgProgress, &dlg_progress::setStatus);
	m_clusterer->start();
}

void dlg_GEMSeControl::clusteringFinished()
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
		saveGEMSeProject(m_outputFolder + "/sampling.sea", "");
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

void dlg_GEMSeControl::saveClustering()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save clustering"),
		QString(), // TODO get directory of current file
		tr("Clustering file (*.clt );;" ) );
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->StoreClustering(fileName);
	}
}

void dlg_GEMSeControl::dataAvailable()
{
	pbSample->setEnabled(m_dlgModalities->modalities()->size() > 0);
	pbSamplingLoad->setEnabled(m_dlgModalities->modalities()->size() > 0);
}

void dlg_GEMSeControl::saveAll()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save all"),
		QString(), // TODO get directory of current file
		tr("GEMSe project (*.sea );;") );
	if (fileName.isEmpty())
	{
		return;
	}
	saveGEMSeProject(fileName, m_dlgGEMSe->GetSerializedHiddenCharts());
}

void dlg_GEMSeControl::saveGEMSeProject(QString const & fileName, QString const & hiddenCharts)
{
	QMap<int, QString> samplingFilenames;
	for (QSharedPointer<iASamplingResults> sampling : *m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->id(), sampling->fileName());
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	iASEAFile seaFile(
		m_dlgModalities->modalities()->fileName(),
		m_simpleLabelInfo->count(),
		samplingFilenames,
		m_cltFile,
		mdiChild->layoutName(),
		leRefImage->text(),
		hiddenCharts,
		m_simpleLabelInfo->colorTheme()->name(),
		m_dlgGEMSe->GetLabelNames()
	);
	seaFile.save(fileName);
}

void dlg_GEMSeControl::saveProject(QSettings & metaFile, QString const & fileName)
{
	// TODO: remove duplication between saveProject and saveGEMSeProject!
	QMap<int, QString> samplingFilenames;
	for (QSharedPointer<iASamplingResults> sampling : *m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->id(), sampling->fileName());
	}
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());
	iASEAFile seaFile(
		"", // don't store modalities here!
		m_simpleLabelInfo->count(),
		samplingFilenames,
		m_cltFile,
		mdiChild->layoutName(),
		leRefImage->text(),
		m_dlgGEMSe->GetSerializedHiddenCharts(),
		m_simpleLabelInfo->colorTheme()->name(),
		m_dlgGEMSe->GetLabelNames()
	);
	seaFile.save(metaFile, fileName);
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
		{
			m_dlgConsensus->SetGroundTruthImage(m_refImg);
		}
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

void dlg_GEMSeControl::modalitySelected(int modalityIdx)
{
	vtkSmartPointer<vtkImageData> imgData = m_dlgModalities->modalities()->get(modalityIdx)->image();
	m_dlgGEMSe->ShowImage(imgData);
}

void ExportClusterIDs(QSharedPointer<iAImageTreeNode> node, std::ostream & out)
{
	VisitLeafs(node.data(), [&](iAImageTreeLeaf const * leaf)
	{
		//static int curr = 0;
		out << leaf->GetDatasetID() << "\t" << leaf->GetID() << "\n";
	});
}

void dlg_GEMSeControl::exportIDs()
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

void dlg_GEMSeControl::setColorTheme(int index)
{
	QString const themeName = cbColorThemes->itemText(index);
	iAColorTheme const * theme = iAColorThemeManager::instance().theme(themeName);
	m_dlgLabels->setColorTheme(theme);
	m_simpleLabelInfo->setColorTheme(theme);
	m_dlgGEMSe->setColorTheme(theme, m_simpleLabelInfo.data());
}

void dlg_GEMSeControl::setRepresentative(int index)
{
	QString const reprType = cbRepresentative->itemText(index);
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
		int reprTypeIdx = m_dlgGEMSe->GetRepresentativeType();
		cbRepresentative->setCurrentIndex(reprTypeIdx);
	}
}

void dlg_GEMSeControl::loadRefImgSlot()
{
	QString refFileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Files"),
		"",
		iAIOProvider::MetaImages
	);
	if (refFileName.isEmpty())
	{
		return;
	}
	loadRefImg(refFileName);
}

bool dlg_GEMSeControl::loadRefImg(QString const & refImgName)
{
	try
	{
		iAITKIO::ScalarPixelType pixelType;
		auto img = iAITKIO::readFile(refImgName, pixelType, false);
		if (pixelType != itk::ImageIOBase::INT)
		{
			img = castImageTo<int>(img);
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

void dlg_GEMSeControl::saveDerivedOutputSlot()
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
		saveDerivedOutput(derivedOutputFileName, attributeDescriptorOutputFileName, samplings->at(i));
	}
}

void dlg_GEMSeControl::saveDerivedOutput(
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
	storeAttributes(out, *results->attributes().data());

	// store derived output:
	results->storeAttributes(iAAttributeDescriptor::DerivedOutput, derivedOutputFileName, false);
}

void dlg_GEMSeControl::exportAttributeRangeRanking()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Attribute Range Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportAttributeRangeRanking(fileName);
	}
}

void dlg_GEMSeControl::exportRankings()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportRankings(fileName);
	}
}

void dlg_GEMSeControl::importRankings()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Rankings"),
		QString(), // TODO get directory of current file
		tr("Comma separated file (*.csv);;"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ImportRankings(fileName);
	}
}

void dlg_GEMSeControl::setSerializedHiddenCharts(QString const & hiddenCharts)
{
	m_dlgGEMSe->SetSerializedHiddenCharts(hiddenCharts);
}

void dlg_GEMSeControl::setMagicLensCount(int count)
{
	m_dlgGEMSe->setMagicLensCount(count);
}

void dlg_GEMSeControl::freeMemory()
{
	m_dlgGEMSe->freeMemory();
}

void dlg_GEMSeControl::setProbabilityProbing(int state)
{
	if (!m_dlgGEMSe)
	{
		return;
	}
	m_dlgGEMSe->SetProbabilityProbing(state == Qt::Checked);
}

void dlg_GEMSeControl::setCorrectnessUncertainty(int state)
{
	if (!m_dlgGEMSe)
	{
		return;
	}
	m_dlgGEMSe->SetCorrectnessUncertaintyOverlay(state == Qt::Checked);
}

void dlg_GEMSeControl::dataTFChanged()
{
	if (!m_dlgGEMSe)
	{
		return;
	}
	m_dlgGEMSe->DataTFChanged();
}

void dlg_GEMSeControl::setLabelInfo(QString const & colorTheme, QString const & labelNames)
{
	m_simpleLabelInfo->setLabelNames(labelNames.split(","));
	int colorThemeIdx = cbColorThemes->findText(colorTheme);
	if (colorTheme != "" && colorThemeIdx != -1)
	{
		cbColorThemes->setCurrentIndex(colorThemeIdx);
			//setColorTheme(); // maybe already done via signal, need to check
	}
}
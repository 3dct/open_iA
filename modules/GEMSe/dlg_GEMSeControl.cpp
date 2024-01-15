// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_GEMSeControl.h"

#include "dlg_GEMSe.h"
#include "dlg_Consensus.h"
#include "dlg_samplings.h"
#include "iAGEMSeConstants.h"
#include "iAImageTree.h"
#include "iAImageTreeLeaf.h" // for VisitLeafs
#include "iALabelInfo.h"
#include "iAImageClusterer.h"
#include "iASEAFile.h"

// MetaFilters
#include <iAImageSampler.h>
#include <iASamplingMethodImpl.h>
#include <iASamplingResults.h>
#include <iAParameterNames.h>
#include <iASamplingSettingsDlg.h>

// libs
#include <iAAttributes.h>
#include <iAAttributeDescriptor.h>
#include <iAColorTheme.h>
#include <iAConnector.h>
#include <iAFileUtils.h>    // for getLocalEncodingFileName
#include <iAImageData.h>
#include <iAJobListView.h>
#include <iALog.h>
#include <iAMdiChild.h>
#include <iAMetaFileIO.h>
#include <iAParameterDlg.h>
#include <iAToolsITK.h>
#include <iAVolumeViewer.h>

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
	dlg_samplings* dlgSamplings,
	iAColorTheme const * colorTheme
):
	dlg_GEMSeControlUI(parentWidget),
	m_dlgSamplingSettings(nullptr),
	m_dlgGEMSe(dlgGEMSe),
	m_dlgSamplings(dlgSamplings),
	m_dlgConsensus(nullptr),
	m_simpleLabelInfo(new iASimpleLabelInfo())
{
	connect(m_dlgSamplings, &dlg_samplings::AddSampling, this, &dlg_GEMSeControl::loadSamplingSlot);
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

	connect(sbClusterViewPreviewSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_GEMSeControl::SetIconSize);
	connect(sbMagicLensCount, QOverload<int>::of(&QSpinBox::valueChanged), this, &dlg_GEMSeControl::setMagicLensCount);
	connect(cbColorThemes, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_GEMSeControl::setColorTheme);
	connect(cbRepresentative, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_GEMSeControl::setRepresentative);
	connect(cbProbabilityProbing, &QCheckBox::stateChanged, this, &dlg_GEMSeControl::setProbabilityProbing);
	connect(cbCorrectnessUncertainty, &QCheckBox::stateChanged, this, &dlg_GEMSeControl::setCorrectnessUncertainty);

	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	// TODO NEWIO: listen to viewer 
	//connect(mdiChild, &iAMdiChild::transferFunctionChanged, this, &dlg_GEMSeControl::dataTFChanged);
	connect(mdiChild, &iAMdiChild::dataSetRendered, this, &dlg_GEMSeControl::dataAvailable);
	connect(mdiChild, &iAMdiChild::dataSetSelected, this, &dlg_GEMSeControl::dataSetSelected);

	dataAvailable();
}

void dlg_GEMSeControl::startSampling()
{
	iAMdiChild* child = dynamic_cast<iAMdiChild*>(parent());
	auto numDataSets = child->dataSetMap().size();
	if (numDataSets == 0)
	{
		LOG(lvlError, "No data available.");
		return;
	}
	if (m_dlgSamplingSettings || m_sampler)
	{
		LOG(lvlError, "Cannot start sampling while another sampling is still running...");
		QMessageBox::warning(this, "GEMSe", "Another sampler still running / dialog is still open...");
		return;
	}
	m_dlgSamplingSettings = new iASamplingSettingsDlg(this, numDataSets, m_samplingSettings);
	if (m_dlgSamplingSettings->exec() == QDialog::Accepted)
	{
		m_dlgSamplingSettings->getValues(m_samplingSettings);
		m_outputFolder = m_samplingSettings[spnOutputFolder].toString();
		QDir outputFolder(m_outputFolder);
		outputFolder.mkpath(".");
		if (m_samplingSettings[spnComputeDerivedOutput].toBool() &&
			m_samplingSettings[spnNumberOfLabels].toInt() < 2)
		{
			LOG(lvlError, "Label Count must not be smaller than 2!");
			QMessageBox::warning(this, "GEMSe", "Label Count must not be smaller than 2!");
			return;
		}
		m_simpleLabelInfo->setLabelCount(m_samplingSettings[spnNumberOfLabels].toInt());
		auto samplingMethod = createSamplingMethod(m_samplingSettings);
		if (!samplingMethod)
		{
			return;
		}
		samplingMethod->setSampleCount(m_samplingSettings[spnNumberOfSamples].toInt(), m_dlgSamplingSettings->parameterRanges());
		iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
		m_sampler = std::make_shared<iAImageSampler>(
			mdiChild->dataSetMap(),
			m_samplingSettings,
			m_dlgSamplingSettings->parameterRanges(),
			m_dlgSamplingSettings->parameterSpecs(),
			samplingMethod,
			iASEAFile::DefaultSMPFileName,
			iASEAFile::DefaultSPSFileName,
			iASEAFile::DefaultCHRFileName,
			m_dlgSamplings->GetSamplings()->size(),
			&m_progress
		);
		iAJobListView::get()->addJob("Sampling Progress", &m_progress, m_sampler.get(), m_sampler.get());
		connect(m_sampler.get(), &iAImageSampler::finished, this, &dlg_GEMSeControl::samplingFinished);

		// trigger parameter set creation & sampling (in foreground with progress bar for now)
		m_sampler->start();
	}
	delete m_dlgSamplingSettings;
	m_dlgSamplingSettings = 0;
}

void dlg_GEMSeControl::loadSamplingSlot()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Sampling"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Attribute Descriptor file (*.smp );;All files (*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	int labelCount = m_simpleLabelInfo->count();
	if (labelCount < 2)
	{
		iAAttributes param;
		addAttr(param, "Label Count", iAValueType::Discrete, 2, 2);
		iAParameterDlg dlg(this, "Label Count", param);
		if (dlg.exec() != QDialog::Accepted)
		{
			LOG(lvlError, "Cannot load sampling without label count input!");
			return;
		}
		labelCount = dlg.parameterValues()["Label Count"].toInt();
	}
	loadSampling(fileName, labelCount, m_dlgSamplings->GetSamplings()->size());
}

bool dlg_GEMSeControl::loadSampling(QString const & fileName, int labelCount, int datasetID)
{
	m_simpleLabelInfo->setLabelCount(labelCount);
	if (fileName.isEmpty())
	{
		LOG(lvlError, "No filename given, not loading.");
		return false;
	}
	std::shared_ptr<iASamplingResults> samplingResults = iASamplingResults::load(fileName, datasetID);
	if (!samplingResults)
	{
		LOG(lvlError, "Loading Sampling failed.");
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
	std::shared_ptr<iASamplingResults> samplingResults = m_sampler->results();

	if (!samplingResults || m_sampler->isAborted())
	{
		m_sampler.reset();
		return;
	}
	m_sampler.reset();
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
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Clustering filt(*.clt );;All files (*)" ) );
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
		LOG(lvlError, "Label Count must not be smaller than 2!");
		return false;
	}
	assert(m_dlgSamplings->SamplingCount() > 0);
	if (m_dlgSamplings->SamplingCount() == 0 || fileName.isEmpty())
	{
		LOG(lvlError, "No sampling data is available!");
		return false;
	}
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	auto originalImage = mdiChild->firstImageData();
	std::shared_ptr<iAImageTree> tree = iAImageTree::Create(
		fileName,
		m_dlgSamplings->GetSamplings(),
		m_simpleLabelInfo->count()
	);
	if (!tree)
	{
		LOG(lvlError, "Loading Clustering failed!");
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
		LOG(lvlError, "Spacing of original images and of result images does not match!");
	}
	std::vector<std::shared_ptr<iADataSet>> imgDataSets;
	std::vector<iATransferFunction*> transfer;
	for (auto d: mdiChild->dataSetMap())
	{
		if (d.second->type() == iADataSetType::Volume)
		{
			imgDataSets.push_back(d.second);
			transfer.push_back(dynamic_cast<iAVolumeViewer*>(mdiChild->dataSetViewer(d.first))->transfer());
		}
	}
	m_dlgGEMSe->SetTree(
		tree,
		originalImage,
		imgDataSets, transfer,
		m_simpleLabelInfo.get(),
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
		LOG(lvlError, "No Sampling Results available!");
		return;
	}
	m_outputFolder = QFileDialog::getExistingDirectory(this, tr("Output Directory"), m_outputFolder);
	if (m_outputFolder.isEmpty())
	{
		return;
	}
	LOG(lvlInfo, QString("Clustering and writing results to %1").arg(m_outputFolder));
	QString cacheDir = m_outputFolder + "/representatives";
	QDir qdir;
	if (!qdir.mkpath(cacheDir))
	{
		LOG(lvlError, QString("Can't create representative directory %1!").arg(cacheDir));
		return;
	}
	m_clusterer = std::make_shared<iAImageClusterer>(m_simpleLabelInfo->count(), cacheDir, &m_progress);
	iAJobListView::get()->addJob("Clustering Progress", &m_progress, m_clusterer.get(), m_clusterer.get());
	for (int samplingIdx=0; samplingIdx<m_dlgSamplings->SamplingCount(); ++samplingIdx)
	{
		std::shared_ptr<iASamplingResults> sampling = m_dlgSamplings->GetSampling(samplingIdx);
		for (int sampleIdx = 0; sampleIdx < sampling->size(); ++sampleIdx)
		{
			m_clusterer->AddImage(sampling->get(sampleIdx));
		}
	}
	connect(m_clusterer.get(), &iAImageClusterer::finished, this, &dlg_GEMSeControl::clusteringFinished);
	m_clusterer->start();
}

void dlg_GEMSeControl::clusteringFinished()
{
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	auto originalImage = mdiChild->firstImageData();

	std::shared_ptr<iAImageTree> tree = m_clusterer->GetResult();
	assert(m_dlgGEMSe);
	if (!m_dlgGEMSe)
	{
		LOG(lvlError, "GEMSe not initialized!");
		return;
	}
	if (m_clusterer->IsAborted() || !m_clusterer->GetResult())
	{
		LOG(lvlError, "Clusterer aborted / missing Clustering Result!");
		return;
	}
	if (!m_outputFolder.isEmpty())
	{
		m_cltFile = m_outputFolder + "/" + iASEAFile::DefaultCLTFileName;
		m_clusterer->GetResult()->Store(m_cltFile);
		/*
		// TODO NEWIO
		if (m_dlgModalities->modalities()->fileName().isEmpty())
		{
			mdiChild->saveProject(m_outputFolder + "/" + iASEAFile::DefaultModalityFileName);
		}
		*/
		saveGEMSeProject(m_outputFolder + "/sampling.sea", "");
	}
	std::vector<std::shared_ptr<iADataSet>> imgDataSets;
	std::vector<iATransferFunction*> transfer;
	for (auto d : mdiChild->dataSetMap())
	{
		if (d.second->type() == iADataSetType::Volume)
		{
			imgDataSets.push_back(d.second);
			transfer.push_back(dynamic_cast<iAVolumeViewer*>(mdiChild->dataSetViewer(d.first))->transfer());
		}
	}
	m_dlgGEMSe->SetTree(
		m_clusterer->GetResult(),
		originalImage,
		imgDataSets, transfer,
		m_simpleLabelInfo.get(),
		m_dlgSamplings->GetSamplings()
	);
	EnableClusteringDependantUI();
	m_dlgConsensus->EnableUI();
}

void dlg_GEMSeControl::saveClustering()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save clustering"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Clustering file (*.clt );;All files (*)" ) );
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->StoreClustering(fileName);
	}
}

void dlg_GEMSeControl::dataAvailable()
{
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	pbSample->setEnabled(mdiChild->firstImageData());
	pbSamplingLoad->setEnabled(mdiChild->firstImageData());
}

void dlg_GEMSeControl::saveAll()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save all"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("GEMSe project (*.sea );;All files (*)") );
	if (fileName.isEmpty())
	{
		return;
	}
	saveGEMSeProject(fileName, m_dlgGEMSe->GetSerializedHiddenCharts());
}

void dlg_GEMSeControl::saveGEMSeProject(QString const & fileName, QString const & hiddenCharts)
{
	QMap<int, QString> samplingFilenames;
	for (std::shared_ptr<iASamplingResults> sampling : *m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->id(), sampling->fileName());
	}
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	iASEAFile seaFile(
		mdiChild->fileInfo().fileName(),    // TODO NEWIO: provide project file name? need to store before ...?
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
	for (std::shared_ptr<iASamplingResults> sampling : *m_dlgSamplings->GetSamplings())
	{
		samplingFilenames.insert(sampling->id(), sampling->fileName());
	}
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	iASEAFile seaFile(
		"", // don't store datasets here!
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
	pbFreeMemory->setEnabled(true);
	if (!m_dlgConsensus)
	{
		iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
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

void dlg_GEMSeControl::dataSetSelected(size_t dataSetIdx)
{
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	auto imgData = dynamic_cast<iAImageData*>(mdiChild->dataSetMap().at(dataSetIdx).get())->vtkImage();
	m_dlgGEMSe->ShowImage(imgData);
}

void ExportClusterIDs(std::shared_ptr<iAImageTreeNode> node, std::ostream & out)
{
	VisitLeafs(node.get(), [&](iAImageTreeLeaf const * leaf)
	{
		//static int curr = 0;
		out << leaf->GetDatasetID() << "\t" << leaf->GetID() << "\n";
	});
}

void dlg_GEMSeControl::exportIDs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export cluster IDs"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Comma-separated values (*.csv);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	std::shared_ptr<iAImageTreeNode> cluster = m_dlgGEMSe->GetCurrentCluster();
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
	m_simpleLabelInfo->setColorTheme(theme);
	m_dlgGEMSe->setColorTheme(theme, m_simpleLabelInfo.get());
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
	iAMetaFileIO io;
	QString refFileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Files"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		io.filterString()
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
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		auto img = iAITKIO::readFile(refImgName, pixelType, scalarType, false);
		assert(pixelType == iAITKIO::PixelType::SCALAR);
		if (scalarType != iAITKIO::ScalarType::INT)
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
		LOG(lvlError, QString("Could not load reference image, problem: %1").arg(e.what()));
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
			dynamic_cast<iAMdiChild*>(parent())->filePath(),
			tr("Derived Output (*.chr );;All files (*)"));
		QString attributeDescriptorOutputFileName = QFileDialog::getSaveFileName(this, tr("Save Attribute Descriptor"),
			dynamic_cast<iAMdiChild*>(parent())->filePath(),
			tr("Attribute Descriptor file (*.smp );;All files (*)")
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
	std::shared_ptr<iASamplingResults> results)
{

	// TODO: update smp file with all attribute descriptors
	// for now: write to separate descriptor file:
	QFile paramRangeFile(attributeDescriptorOutputFileName);
	if (!paramRangeFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open parameter descriptor file '%1' for writing!").arg(attributeDescriptorOutputFileName));
		return;
	}
	QTextStream out(&paramRangeFile);
	storeAttributes(out, *results->attributes().get());

	// store derived output:
	results->storeAttributes(iAAttributeDescriptor::DerivedOutput, derivedOutputFileName, false);
}

void dlg_GEMSeControl::exportAttributeRangeRanking()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Attribute Range Rankings"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Comma separated file (*.csv);;All files (*)"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportAttributeRangeRanking(fileName);
	}
}

void dlg_GEMSeControl::exportRankings()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Store Rankings"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Comma separated file (*.csv);;All files (*)"));
	if (!fileName.isEmpty())
	{
		m_dlgGEMSe->ExportRankings(fileName);
	}
}

void dlg_GEMSeControl::importRankings()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Rankings"),
		dynamic_cast<iAMdiChild*>(parent())->filePath(),
		tr("Comma separated file (*.csv);;All files (*)"));
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

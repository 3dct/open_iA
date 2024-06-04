// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMeanObject.h"

#include "ui_FeatureScoutMeanObjectView.h"

// base
#include <defines.h>    // for DIM
#include <iALog.h>
#include <iAToolsVTK.h>

// charts
#include <iAChartWithFunctionsWidget.h>

// objectvis
#include <iAObjectType.h>

// guibase
#include <iAJobListView.h>
#include <iATransferFunctionOwner.h>
#include <iAMultiStepProgressObserver.h>
#include <iAQVTKWidget.h>
#include <iARunAsync.h>
#include <iAMdiChild.h>

// ITK
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include <itkAddImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelMapMaskImageFilter.h>
#include <itkPasteImageFilter.h>
#include <itkVTKImageToImageFilter.h>

// VTK
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCornerAnnotation.h>
#include <vtkCubeAxesActor.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkImageCast.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMarchingCubes.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkSTLWriter.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

// Qt
#include <QMessageBox>
#include <QStandardItem>
#include <QFileDialog>

class iAMeanObjectDockWidget : public QDockWidget, public Ui_FeatureScoutMO
{
public:
	iAMeanObjectDockWidget(QWidget* parent = nullptr) : QDockWidget(parent)
	{
		setupUi(this);
	}
};

class iAMeanObjectData
{
public:
	QList<iATransferFunctionOwner*> moHistogramList;
	QList<vtkSmartPointer<vtkVolume>> moVolumesList;
	QList<vtkSmartPointer<vtkRenderer>> moRendererList;
	QList<vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>> moVolumeMapperList;
	QList<vtkSmartPointer<vtkVolumeProperty>> moVolumePropertyList;
	QList<vtkSmartPointer<vtkImageData>> moImageDataList;
};

iAMeanObject::iAMeanObject(iAMdiChild* activeChild, QString const& sourcePath) :
	m_dwMO(nullptr),
	m_motfView(nullptr),
	m_MOData(std::make_shared<iAMeanObjectData>()),
	m_meanObjectWidget(nullptr),
	m_activeChild(activeChild),
	m_filterID(-1),
	m_sourcePath(sourcePath)
{
}

void iAMeanObject::render(QStringList const& classNames, QList<vtkSmartPointer<vtkTable>> const& tableList,
	int filterID, QDockWidget* nextToDW, vtkCamera* commonCamera, QList<QColor> const& classColor)
{
	auto classCount = classNames.size();
	if (classCount <= 1)
	{
		QMessageBox::warning(m_activeChild, "MObjects", "You need to define at least one class for Mean Objects to be computed!");
		return;
	}
	try
	{
		m_filterID = filterID;
		iAProgress p;
		auto jobHandle = iAJobListView::get()->addJob("Compute Mean Object", &p);
		// Delete old mean object data lists
		for (int i = 0; i < m_MOData->moHistogramList.size(); ++i)
		{
			delete m_MOData->moHistogramList[i];
		}
		m_MOData->moVolumesList.clear();
		m_MOData->moHistogramList.clear();
		m_MOData->moRendererList.clear();
		m_MOData->moVolumeMapperList.clear();
		m_MOData->moVolumePropertyList.clear();
		m_MOData->moImageDataList.clear();

		// Casts image to long (if necessary) and convert it to an ITK image
		using IType = itk::Image<long, DIM>;
		auto vtkToItkConverter = itk::VTKImageToImageFilter<IType>::New();
		auto img = m_activeChild->firstImageData();
		if (!img)
		{
			return;
		}
		if (img->GetScalarType() != 8)  // long type
		{
			auto cast = vtkSmartPointer<vtkImageCast>::New();
			cast->SetInputData(img);
			cast->SetOutputScalarTypeToLong();
			cast->Update();
			vtkToItkConverter->SetInput(cast->GetOutput());
		}
		else
		{
			vtkToItkConverter->SetInput(img);
		}
		vtkToItkConverter->Update();

		IType::Pointer itkImageData = vtkToItkConverter->GetOutput();
		itk::Size<DIM> itkImageDataSize = itkImageData->GetLargestPossibleRegion().GetSize();

		auto thresholdFilter = itk::BinaryThresholdImageFilter<IType, IType>::New();
		thresholdFilter->SetInput(itkImageData);
		thresholdFilter->SetLowerThreshold(0);
		thresholdFilter->SetUpperThreshold(0);
		thresholdFilter->SetInsideValue(0);
		thresholdFilter->SetOutsideValue(1);
		thresholdFilter->Update();
		using LabelMapType = itk::LabelMap<itk::LabelObject<long, DIM>>;
		auto i2l = itk::LabelImageToLabelMapFilter<IType, LabelMapType>::New();
		i2l->SetInput(itkImageData);

		auto mask = itk::LabelMapMaskImageFilter<LabelMapType, IType>::New();
		mask->SetInput(i2l->GetOutput());
		mask->SetFeatureImage(thresholdFilter->GetOutput());
		mask->SetBackgroundValue(0);
		mask->SetCrop(true);

		// Defines mean object output image
		using MObjectImagePixelType = long;
		using MObjectImageType = itk::Image<MObjectImagePixelType, DIM>;
		MObjectImageType::RegionType outputRegion;
		MObjectImageType::RegionType::IndexType outputStart;
		outputStart[0] = 0;
		outputStart[1] = 0;
		outputStart[2] = 0;
		itk::Size<DIM> moImgSize;
		itk::Size<DIM> moImgCenter;
		for (int i = 0; i < DIM; ++i)
		{
			itkImageDataSize[i] % 2 == 0 ? moImgSize[i] = itkImageDataSize[i] + 1 : moImgSize[i] = itkImageDataSize[i];
			moImgCenter[i] = std::round(moImgSize[i] / 2.0);
		}
		outputRegion.SetSize(moImgSize);
		outputRegion.SetIndex(outputStart);
		auto mObjectITKImage = MObjectImageType::New();
		mObjectITKImage->SetRegions(outputRegion);
		const MObjectImageType::SpacingType& out_spacing = itkImageData->GetSpacing();
		const MObjectImageType::PointType& inputOrigin = itkImageData->GetOrigin();
		double outputOrigin[DIM];
		for (unsigned int i = 0; i < DIM; ++i)
		{
			outputOrigin[i] = inputOrigin[i];
		}
		mObjectITKImage->SetSpacing(out_spacing);
		mObjectITKImage->SetOrigin(outputOrigin);
		mObjectITKImage->Allocate();

		// Defines add image
		using AddImageType = itk::Image<long, DIM>;
		AddImageType::RegionType addoutputRegion;
		AddImageType::RegionType::IndexType addoutputStart;
		addoutputStart[0] = 0;
		addoutputStart[1] = 0;
		addoutputStart[2] = 0;
		addoutputRegion.SetSize(moImgSize);
		addoutputRegion.SetIndex(outputStart);
		auto addImage = AddImageType::New();
		addImage->SetRegions(addoutputRegion);
		const auto& addout_spacing = itkImageData->GetSpacing();
		const auto& addinputOrigin = itkImageData->GetOrigin();
		double addoutputOrigin[DIM];
		for (unsigned int i = 0; i < DIM; ++i)
		{
			addoutputOrigin[i] = addinputOrigin[i];
		}
		addImage->SetSpacing(addout_spacing);
		addImage->SetOrigin(addoutputOrigin);
		addImage->Allocate();

		for (qsizetype currClass = 1; currClass < classCount; ++currClass)
		{
			std::map<int, int> meanObjectIds;
			for (int j = 0; j < tableList[currClass]->GetNumberOfRows(); ++j)
			{
				meanObjectIds[tableList[currClass]->GetValue(j, 0).ToInt()] =
					tableList[currClass]->GetValue(j, 0).ToFloat();
			}

			typedef itk::ImageRegionIterator<MObjectImageType> IteratorType;
			IteratorType mOITKImgIt(mObjectITKImage, outputRegion);
			for (mOITKImgIt.GoToBegin(); !mOITKImgIt.IsAtEnd(); ++mOITKImgIt)
			{
				mOITKImgIt.Set(0);
			}

			typedef itk::ImageRegionIterator<AddImageType> IteratorType;
			IteratorType addImgIt(addImage, addoutputRegion);
			for (addImgIt.GoToBegin(); !addImgIt.IsAtEnd(); ++addImgIt)
			{
				addImgIt.Set(0);
			}

			int progress = 0;
			std::map<int, int>::const_iterator it;
			for (it = meanObjectIds.begin(); it != meanObjectIds.end(); ++it)
			{
				mask->SetLabel(it->first);
				mask->Update();
				itk::Size<DIM> maskSize;
				maskSize = mask->GetOutput()->GetLargestPossibleRegion().GetSize();

				IType::IndexType destinationIndex;
				destinationIndex[0] = moImgCenter[0] - std::round(maskSize[0] / 2);
				destinationIndex[1] = moImgCenter[1] - std::round(maskSize[1] / 2);
				destinationIndex[2] = moImgCenter[2] - std::round(maskSize[2] / 2);
				auto pasteFilter = itk::PasteImageFilter<IType, MObjectImageType>::New();
				pasteFilter->SetSourceImage(mask->GetOutput());
				pasteFilter->SetDestinationImage(mObjectITKImage);
				pasteFilter->SetSourceRegion(mask->GetOutput()->GetLargestPossibleRegion());
				pasteFilter->SetDestinationIndex(destinationIndex);

				auto addFilter = itk::AddImageFilter<MObjectImageType, MObjectImageType>::New();
				addFilter->SetInput1(addImage);
				addFilter->SetInput2(pasteFilter->GetOutput());
				addFilter->Update();
				addImage = addFilter->GetOutput();

				double percentage = std::round((currClass - 1) * 100.0 / (classCount - 1) +
					(progress + 1.0) * (100.0 / (classCount - 1)) / meanObjectIds.size());
				p.emitProgress(percentage);
				QCoreApplication::processEvents();
				++progress;
			}

			// Normalize voxels values to 1
			using OutputImageType = itk::Image<float, DIM>;
			auto caster = itk::CastImageFilter<AddImageType, OutputImageType>::New();
			caster->SetInput(addImage);
			caster->Update();
			using CasterIteratorType = itk::ImageRegionIterator<OutputImageType>;
			CasterIteratorType casterImgIt(caster->GetOutput(), caster->GetOutput()->GetLargestPossibleRegion());
			for (casterImgIt.GoToBegin(); !casterImgIt.IsAtEnd(); ++casterImgIt)
			{
				casterImgIt.Set(casterImgIt.Get() / meanObjectIds.size());
			}

			// Convert resulting MObject ITK image to an VTK image
			auto itkToVTKConverter = itk::ImageToVTKImageFilter<OutputImageType>::New();
			itkToVTKConverter->SetInput(caster->GetOutput());
			itkToVTKConverter->Update();
			auto meanObjectImage = vtkSmartPointer<vtkImageData>::New();
			meanObjectImage->DeepCopy(itkToVTKConverter->GetOutput());
			m_MOData->moImageDataList.append(meanObjectImage);

			// Create histogram and TFs for each MObject
			QString moHistName = classNames[currClass];
			moHistName.append(QString(" %1 Mean Object").arg(MapObjectTypeToString(filterID)));
			iATransferFunctionOwner* moHistogram =
				new iATransferFunctionOwner(m_MOData->moImageDataList[currClass - 1]->GetScalarRange());
			m_MOData->moHistogramList.append(moHistogram);

			// Create MObject default Transfer Tunctions
			if (filterID == iAObjectType::Fibers)  // Fibers
			{
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.01, 1.0, 1.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.095, 1.0, 1.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.1, 0.0, 0.0, 1.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(1.00, 0.0, 0.0, 1.0);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.01, 0.01);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.095, 0.01);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.1, 0.05);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(1.00, 0.18);
			}
			else  // Voids
			{
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.0001, 0.0, 0.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.001, 1.0, 1.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.18, 1.0, 1.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(0.2, 0.0, 0.0, 1.0);
				m_MOData->moHistogramList[currClass - 1]->colorTF()->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.0, 0.0);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.0001, 0.0);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.001, 0.005);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.18, 0.005);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(0.2, 0.08);
				m_MOData->moHistogramList[currClass - 1]->opacityTF()->AddPoint(1.0, 0.5);
			}

			// Create the property and attach the transfer functions
			auto vProperty = vtkSmartPointer<vtkVolumeProperty>::New();
			m_MOData->moVolumePropertyList.append(vProperty);
			vProperty->SetColor(m_MOData->moHistogramList[currClass - 1]->colorTF());
			vProperty->SetScalarOpacity(m_MOData->moHistogramList[currClass - 1]->opacityTF());
			vProperty->SetInterpolationTypeToLinear();
			vProperty->ShadeOff();

			// Create volume and mapper and set input for mapper
			auto volume = vtkSmartPointer<vtkVolume>::New();
			m_MOData->moVolumesList.append(volume);
			auto mapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
			m_MOData->moVolumeMapperList.append(mapper);
			mapper->SetAutoAdjustSampleDistances(1);
			mapper->SetSampleDistance(1.0);
			mapper->SetInputData(m_MOData->moImageDataList[currClass - 1]);
			mapper->Update();
			mapper->UpdateDataObject();
			volume->SetProperty(m_MOData->moVolumePropertyList[currClass - 1]);
			volume->SetMapper(m_MOData->moVolumeMapperList[currClass - 1]);
			volume->Update();
		}

		// Create the outline for volume
		auto outline = vtkSmartPointer<vtkOutlineFilter>::New();
		outline->SetInputData(m_MOData->moVolumesList[0]->GetMapper()->GetDataObjectInput());
		auto outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		outlineMapper->SetInputConnection(outline->GetOutputPort());
		auto outlineActor = vtkSmartPointer<vtkActor>::New();
		outlineActor->SetMapper(outlineMapper);
		outlineActor->GetProperty()->SetColor(0, 0, 0);
		outlineActor->GetProperty()->SetLineWidth(1.0);
		outlineActor->GetProperty()->SetOpacity(0.1);

		// Calculates the max dimension of the image
		double maxDim = 0.0;
		for (int i = 0; i < 6; ++i)
		{
			if (outlineActor->GetBounds()[i] > maxDim)
			{
				maxDim = outlineActor->GetBounds()[i];
			}
		}

		// Setup Mean Object view
		if (!m_dwMO)
		{
			m_dwMO = new iAMeanObjectDockWidget(m_activeChild);
			connect(m_dwMO->pb_ModTF, &QToolButton::clicked, this, &iAMeanObject::modifyMeanObjectTF);
			connect(m_dwMO->tb_saveStl, &QToolButton::clicked, this, &iAMeanObject::saveStl);
			connect(m_dwMO->tb_saveVolume, &QToolButton::clicked, this, &iAMeanObject::saveVolume);

			// Create a render window and an interactor for all the MObjects
			m_meanObjectWidget = new iAQVTKWidget();

			m_dwMO->verticalLayout->addWidget(m_meanObjectWidget);
			auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
			renderWindowInteractor->SetRenderWindow(m_meanObjectWidget->renderWindow());
			auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
			renderWindowInteractor->SetInteractorStyle(style);

			m_dwMO->setWindowTitle(QString("%1 Mean Object View").arg(MapObjectTypeToString(filterID)));
		}

		// Update MOClass comboBox
		m_dwMO->cb_Classes->clear();       // skip the "Unclassified" class, for which no MObject was created
		QStringList mobjectNames(classNames.begin()+1, classNames.end());
		m_dwMO->cb_Classes->addItems(mobjectNames);
		m_activeChild->tabifyDockWidget(nextToDW, m_dwMO);
		m_dwMO->show();
		m_dwMO->raise();

		// Remove old renderers
		m_meanObjectWidget->renderWindow()->GetRenderers()->RemoveAllItems();

		// Define viewport variables
		auto numberOfMeanObjectVolumes = m_MOData->moVolumesList.size();
		float viewportColumns = numberOfMeanObjectVolumes < 3 ? fmod(numberOfMeanObjectVolumes, 3.0) : 3.0;
		float viewportRows = std::ceil(numberOfMeanObjectVolumes / viewportColumns);
		float fieldLengthX = 1.0 / viewportColumns, fieldLengthY = 1.0 / viewportRows;
		int numOfViewPorts = static_cast<int>(viewportColumns * viewportRows);
		// Set up viewports
		for (int i = 0; i < numOfViewPorts; ++i)
		{
			auto renderer = vtkSmartPointer<vtkRenderer>::New();
			m_MOData->moRendererList.append(renderer);
			renderer->GetActiveCamera()->ParallelProjectionOn();
			renderer->SetBackground(1.0, 1.0, 1.0);
			m_meanObjectWidget->renderWindow()->AddRenderer(m_MOData->moRendererList[i]);
			renderer->SetViewport(fmod(i, viewportColumns) * fieldLengthX,
				1 - (std::ceil((i + 1.0) / viewportColumns) / viewportRows),
				fmod(i, viewportColumns) * fieldLengthX + fieldLengthX,
				1 - (std::ceil((i + 1.0) / viewportColumns) / viewportRows) + fieldLengthY);

			if (i < m_MOData->moVolumesList.size())
			{
				renderer->AddVolume(m_MOData->moVolumesList[i]);
				renderer->SetActiveCamera(commonCamera);
				renderer->GetActiveCamera()->SetParallelScale(
					maxDim);  //use maxDim for right scaling to fit the data in the viewports

				auto cornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
				cornerAnnotation->SetLinearFontScaleFactor(2);
				cornerAnnotation->SetNonlinearFontScaleFactor(1);
				cornerAnnotation->SetMaximumFontSize(25);
				cornerAnnotation->SetText(2, classNames[i + 1].toStdString().c_str());
				cornerAnnotation->GetTextProperty()->SetColor(
					classColor.at(i + 1).redF(), classColor.at(i + 1).greenF(), classColor.at(i + 1).blueF());
				cornerAnnotation->GetTextProperty()->BoldOn();

				auto cubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
				cubeAxesActor->SetBounds(outlineActor->GetBounds());
				cubeAxesActor->SetCamera(renderer->GetActiveCamera());
				cubeAxesActor->SetFlyModeToOuterEdges();
				cubeAxesActor->SetTickLocationToOutside();
				cubeAxesActor->SetScreenSize(10.0);  //changes axes font size
				cubeAxesActor->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);
				cubeAxesActor->DrawXGridlinesOn();
				cubeAxesActor->DrawYGridlinesOn();
				cubeAxesActor->DrawZGridlinesOn();
				cubeAxesActor->GetTitleTextProperty(0)->SetColor(1.0, 0.0, 0.0);
				cubeAxesActor->GetLabelTextProperty(0)->SetColor(1.0, 0.0, 0.0);
				cubeAxesActor->GetXAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);
				cubeAxesActor->SetXUnits("microns");
				cubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 1.0, 0.0);
				cubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 1.0, 0.0);
				cubeAxesActor->GetYAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);
				cubeAxesActor->SetYUnits("microns");
				cubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.0, 1.0);
				cubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.0, 1.0);
				cubeAxesActor->GetZAxesGridlinesProperty()->SetColor(0.3, 0.3, 0.3);
				cubeAxesActor->SetZUnits("microns");
				cubeAxesActor->XAxisLabelVisibilityOn();
				cubeAxesActor->XAxisTickVisibilityOn();
				cubeAxesActor->XAxisMinorTickVisibilityOff();
				cubeAxesActor->GetXAxesLinesProperty()->SetColor(1.0, 0.0, 0.0);
				cubeAxesActor->YAxisLabelVisibilityOn();
				cubeAxesActor->YAxisTickVisibilityOn();
				cubeAxesActor->YAxisMinorTickVisibilityOff();
				cubeAxesActor->GetYAxesLinesProperty()->SetColor(0.0, 1.0, 0.0);
				cubeAxesActor->ZAxisLabelVisibilityOn();
				cubeAxesActor->ZAxisTickVisibilityOn();
				cubeAxesActor->ZAxisMinorTickVisibilityOff();
				cubeAxesActor->GetZAxesLinesProperty()->SetColor(0.0, 0.0, 1.0);

				renderer->AddViewProp(cornerAnnotation);
				renderer->AddActor(cubeAxesActor);
				renderer->AddActor(outlineActor);
			}
			m_meanObjectWidget->renderWindow()->Render();
		}
	}
	catch (itk::ExceptionObject& excep)
	{
		QString msg = QString("Error in computation: %1 in File %2, Line %3.")
						  .arg(excep.GetDescription())
						  .arg(excep.GetFile())
						  .arg(excep.GetLine());
		LOG(lvlError, QString("MObjects: %1").arg(msg));
		QMessageBox::warning(m_activeChild, "MObjects", msg + "\nCheck whether you provided a proper labeled image!");
	}
	catch (std::bad_alloc& e)
	{
		QString msg = QString("Allocation failed: %1").arg(e.what());
		LOG(lvlError, QString("MObjects: %1").arg(msg));
		QMessageBox::warning(m_activeChild, "MObjects", msg + "\nCheck whether you can free some memory, operate on a smaller dataset, or use a machine with more RAM!");
	}
}

void iAMeanObject::modifyMeanObjectTF()
{
	//delete m_motfView;	// in case it was previously open
	int moIndex = m_dwMO->cb_Classes->currentIndex();
	if (moIndex < 0 || moIndex >= m_MOData->moHistogramList.size())
	{	// if outside valid range - just to be on the safe side
		LOG(lvlError, QString("Invalid Mean Object index %1!").arg(moIndex));
		return;
	}
	m_motfView = new QDialog(m_activeChild);
	m_motfView->setWindowTitle(QString("%1 Mean Object Transfer Function")
								   .arg(m_dwMO->cb_Classes->itemText(m_dwMO->cb_Classes->currentIndex())));
	//iAChartWithFunctionsWidget* histogram = m_activeChild->histogram();
	auto histogram = new iAChartWithFunctionsWidget(m_motfView, "Probability", "Frequency");
	histogram->setTransferFunction(m_MOData->moHistogramList[moIndex]);
	connect(histogram, &iAChartWithFunctionsWidget::transferFunctionChanged, this,
		[this] { m_meanObjectWidget->renderWindow()->Render(); });
	m_motfView->setLayout(new QHBoxLayout);
	m_motfView->layout()->addWidget(histogram);
	histogram->show();
	m_motfView->show();
}

void iAMeanObject::saveStl()
{
	int moIndex = m_dwMO->cb_Classes->currentIndex();
	if (moIndex < 0 || moIndex >= m_MOData->moHistogramList.size())
	{  // if outside valid range - just to be on the safe side
		LOG(lvlError, QString("Invalid Mean Object index %1!").arg(moIndex));
		return;
	}
	auto fileName = QFileDialog::getSaveFileName(m_dwMO, tr("Save STL File"), m_sourcePath, tr("STL Files (*.stl)"));
	if (fileName.isEmpty())
	{
		return;
	}
	int isoValue = m_dwMO->dsb_IsoValue->value();

	auto progress = new iAMultiStepProgressObserver(2);
	auto job = runAsync(
		[this, progress, moIndex, isoValue, fileName]
		{
			auto moSurface = vtkSmartPointer<vtkMarchingCubes>::New();
			progress->observe(moSurface);
			moSurface->SetInputData(m_MOData->moImageDataList[moIndex]);
			moSurface->ComputeNormalsOn();
			moSurface->ComputeGradientsOn();
			moSurface->SetValue(0, isoValue);

			progress->setCompletedSteps(1);
			auto stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
			progress->observe(stlWriter);
			stlWriter->SetFileName(fileName.toStdString().c_str());
			stlWriter->SetInputConnection(moSurface->GetOutputPort());
			stlWriter->Write();
		},
		[progress] { delete progress; }, m_dwMO);
	iAJobListView::get()->addJob("Saving STL", progress->progressObject(), job);
}

void iAMeanObject::saveVolume()
{
	int moIndex = m_dwMO->cb_Classes->currentIndex();
	if (moIndex < 0 || moIndex >= m_MOData->moHistogramList.size())
	{  // if outside valid range - just to be on the safe side
		LOG(lvlError, QString("Invalid Mean Object index %1!").arg(moIndex));
		return;
	}
	auto fileName = QFileDialog::getSaveFileName(m_dwMO, tr("Save MObject as Volume"),
		m_sourcePath, tr("MHD files (*.mhd);;")); // TODO: enable choice of all other supported volume formats?
	if (fileName.isEmpty())
	{
		return;
	}
	auto progress = new iAProgress;
	auto job = runAsync([this, progress, moIndex, fileName]
		{
			storeImage(m_MOData->moImageDataList[moIndex], fileName);
			progress->emitProgress(100);
		},
		[progress] { delete progress; }, m_dwMO);
	iAJobListView::get()->addJob("Saving Mean Object volume", progress, job);
}

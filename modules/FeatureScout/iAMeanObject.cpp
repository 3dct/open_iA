// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMeanObject.h"

#include "ui_FeatureScoutMeanObjectView.h"

// base
#include <defines.h>    // for DIM
#include <iAAbortListener.h>
#include <iALog.h>
#include <iAToolsVTK.h>

// charts
#include <iAPlotTypes.h>
#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>

// objectvis
#include <iAObjectType.h>

// guibase
#include <iADockWidgetWrapper.h>
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

class iAMeanObjectWidget : public QWidget, public Ui_FeatureScoutMO
{
public:
	iAMeanObjectWidget(QWidget* parent = nullptr) : QWidget(parent)
	{
		setupUi(this);
	}
};

//! data for a single mean object
class iAMeanObjectData
{
public:
	std::unique_ptr<iATransferFunctionOwner> tf;
	vtkNew<vtkVolume> volume;
	vtkNew<vtkFixedPointVolumeRayCastMapper> volumeMapper;
	vtkNew<vtkVolumeProperty> volumeProperty;
	vtkNew<vtkImageData> imageData;
	vtkSmartPointer<vtkRenderer> renderer;
	std::shared_ptr<iAPlot> histoPlot;
	QDialog* tfDlg;
};

iAMeanObject::iAMeanObject(iAMdiChild* activeChild, QString const& sourcePath) :
	m_dwMO(nullptr),
	m_meanObjectWidget(nullptr),
	m_activeChild(activeChild),
	m_sourcePath(sourcePath)
{}

iAMeanObject::~iAMeanObject() = default;

void iAMeanObject::render(QStringList const& classNames, QList<vtkSmartPointer<vtkTable>> const& tableList,
	int filterID, QDockWidget* nextToDW, vtkCamera* commonCamera, QList<QColor> const& classColor)
{
	auto classCount = classNames.size();
	if (classCount <= 1)
	{
		QMessageBox::warning(
			m_activeChild, "MObjects", "You need to define at least one class for Mean Objects to be computed!");
		return;
	}
	auto aborter = std::make_shared<iASimpleAbortListener>();
	auto p = std::make_shared<iAProgress>();
	auto fw = runAsync(
		[this, classNames, tableList, filterID, classCount, p, aborter]
	{
		m_MOData.clear();
		try
		{
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
				itkImageDataSize[i] % 2 == 0 ? moImgSize[i] = itkImageDataSize[i] + 1
											 : moImgSize[i] = itkImageDataSize[i];
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

			for (qsizetype currClass = 1; currClass < classCount && !aborter->isAborted(); ++currClass)
			{
				m_MOData.push_back(std::move(std::make_unique<iAMeanObjectData>()));
				auto moData = m_MOData[m_MOData.size() - 1].get();
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
				for (it = meanObjectIds.begin(); it != meanObjectIds.end() && !aborter->isAborted(); ++it)
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
					p->emitProgress(percentage);
					++progress;
				}
				if (aborter->isAborted())
				{
					break;
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
				moData->imageData->DeepCopy(itkToVTKConverter->GetOutput());

				// Create MObject default Transfer functions
				moData->tf = std::make_unique<iATransferFunctionOwner>(moData->imageData->GetScalarRange());
				if (filterID == iAObjectType::Fibers)  // Fibers
				{
					moData->tf->colorTF()->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.01, 1.0, 1.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.095, 1.0, 1.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.1, 0.0, 0.0, 1.0);
					moData->tf->colorTF()->AddRGBPoint(1.00, 0.0, 0.0, 1.0);
					moData->tf->opacityTF()->AddPoint(0.0, 0.0);
					moData->tf->opacityTF()->AddPoint(0.01, 0.01);
					moData->tf->opacityTF()->AddPoint(0.095, 0.01);
					moData->tf->opacityTF()->AddPoint(0.1, 0.05);
					moData->tf->opacityTF()->AddPoint(1.00, 0.18);
				}
				else  // Voids
				{
					moData->tf->colorTF()->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.0001, 0.0, 0.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.001, 1.0, 1.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.18, 1.0, 1.0, 0.0);
					moData->tf->colorTF()->AddRGBPoint(0.2, 0.0, 0.0, 1.0);
					moData->tf->colorTF()->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
					moData->tf->opacityTF()->AddPoint(0.0, 0.0);
					moData->tf->opacityTF()->AddPoint(0.0001, 0.0);
					moData->tf->opacityTF()->AddPoint(0.001, 0.005);
					moData->tf->opacityTF()->AddPoint(0.18, 0.005);
					moData->tf->opacityTF()->AddPoint(0.2, 0.08);
					moData->tf->opacityTF()->AddPoint(1.0, 0.5);
				}

				moData->volumeProperty->SetColor(moData->tf->colorTF());
				moData->volumeProperty->SetScalarOpacity(moData->tf->opacityTF());
				moData->volumeProperty->SetInterpolationTypeToLinear();
				moData->volumeProperty->ShadeOff();
				moData->volumeMapper->SetAutoAdjustSampleDistances(1);
				moData->volumeMapper->SetSampleDistance(1.0);
				moData->volumeMapper->SetInputData(moData->imageData);
				moData->volumeMapper->Update();
				moData->volumeMapper->UpdateDataObject();
				moData->volume->SetProperty(moData->volumeProperty);
				moData->volume->SetMapper(moData->volumeMapper);
				moData->volume->Update();

				// create histogram
				QString moHistName =
					classNames[currClass] + QString(" %1 Mean Object").arg(MapObjectTypeToString(filterID));
				auto histData = iAHistogramData::create(moHistName, moData->imageData, 512);
				moData->histoPlot =
					std::make_shared<iABarGraphPlot>(histData, QApplication::palette().color(QPalette::Shadow));
			}
		}
		catch (itk::ExceptionObject& excep)
		{
			QString msg = QString("Error in computation: %1 in File %2, Line %3.")
							  .arg(excep.GetDescription())
							  .arg(excep.GetFile())
							  .arg(excep.GetLine());
			LOG(lvlError, QString("MObjects: %1").arg(msg));
			QMessageBox::warning(
				m_activeChild, "MObjects", msg + "\nCheck whether you provided a proper labeled image!");
		}
		catch (std::bad_alloc& e)
		{
			QString msg = QString("Allocation failed: %1").arg(e.what());
			LOG(lvlError, QString("MObjects: %1").arg(msg));
			QMessageBox::warning(m_activeChild, "MObjects",
				msg +
					"\nCheck whether you can free some memory, operate on a smaller dataset, or use a machine with "
					"more RAM!");
		}
	},
		[this, classNames, filterID, nextToDW, commonCamera, classColor, aborter]
	{
		if (aborter->isAborted())
		{
			return;
		}
		// Create the outline for volume
		auto outline = vtkSmartPointer<vtkOutlineFilter>::New();
		outline->SetInputData(m_MOData[0]->volume->GetMapper()->GetDataObjectInput());
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
			m_dwMO = new iAMeanObjectWidget(m_activeChild);
			connect(m_dwMO->tb_ModTF, &QToolButton::clicked, this, &iAMeanObject::modifyMeanObjectTF);
			connect(m_dwMO->tb_saveStl, &QToolButton::clicked, this, &iAMeanObject::saveStl);
			connect(m_dwMO->tb_saveVolume, &QToolButton::clicked, this, &iAMeanObject::saveVolume);

			// Create a render window and an interactor for all the MObjects
			m_meanObjectWidget = new iAQVTKWidget();
			m_dwMO->verticalLayout->addWidget(m_meanObjectWidget);
			auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
			m_meanObjectWidget->renderWindow()->GetInteractor()->SetInteractorStyle(style);
			auto moDW = new iADockWidgetWrapper(m_dwMO, "Mean Object", "FeatureScoutMO", "https://github.com/3dct/open_iA/wiki/MObjects");
			moDW->setWindowTitle(QString("%1 Mean Object View").arg(MapObjectTypeToString(filterID)));

			m_activeChild->tabifyDockWidget(nextToDW, moDW);
		}

		// Update MOClass comboBox
		m_dwMO->cb_Classes->clear();       // skip the "Unclassified" class, for which no MObject was created
		QStringList mobjectNames(classNames.begin()+1, classNames.end());
		m_dwMO->cb_Classes->addItems(mobjectNames);
		m_dwMO->show();
		m_dwMO->raise();

		// Remove old renderers
		m_meanObjectWidget->renderWindow()->GetRenderers()->RemoveAllItems();

		// Define viewport variables
		float viewportColumns = m_MOData.size() < 3 ? fmod(m_MOData.size(), 3.0) : 3.0;
		float viewportRows = std::ceil(m_MOData.size() / viewportColumns);
		float fieldLengthX = 1.0 / viewportColumns, fieldLengthY = 1.0 / viewportRows;
		int numOfViewPorts = static_cast<int>(viewportColumns * viewportRows);
		// Set up viewports
		for (int i = 0; i < numOfViewPorts; ++i)
		{
			auto renderer = vtkSmartPointer<vtkRenderer>::New();
			renderer->GetActiveCamera()->ParallelProjectionOn();
			renderer->SetBackground(1.0, 1.0, 1.0);
			m_meanObjectWidget->renderWindow()->AddRenderer(renderer);
			renderer->SetViewport(fmod(i, viewportColumns) * fieldLengthX,
				1 - (std::ceil((i + 1.0) / viewportColumns) / viewportRows),
				fmod(i, viewportColumns) * fieldLengthX + fieldLengthX,
				1 - (std::ceil((i + 1.0) / viewportColumns) / viewportRows) + fieldLengthY);

			if (i < m_MOData.size())
			{
				m_MOData[i]->renderer = renderer;
				m_MOData[i]->renderer->AddVolume(m_MOData[i]->volume);
				m_MOData[i]->renderer->SetActiveCamera(commonCamera);
				m_MOData[i]->renderer->GetActiveCamera()->SetParallelScale(
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
		}
		m_meanObjectWidget->renderWindow()->Render();
	}, this);
	iAJobListView::get()->addJob("Compute Mean Object", p.get(), fw, aborter.get());
}

void iAMeanObject::modifyMeanObjectTF()
{
	int moIndex = m_dwMO->cb_Classes->currentIndex();
	if (moIndex < 0 || moIndex >= m_MOData.size())
	{	// if outside valid range - just to be on the safe side
		LOG(lvlError, QString("Invalid Mean Object index %1!").arg(moIndex));
		return;
	}
	if (m_MOData[moIndex]->tfDlg)
	{
		m_MOData[moIndex]->tfDlg->show();  // make sure it is visible (brings it back when closed by the user)
		m_MOData[moIndex]->tfDlg->raise(); // make sure it is the topmost dialog (if there are other open dialogs)
		return;
	}
	m_MOData[moIndex]->tfDlg = new QDialog(m_activeChild);
	m_MOData[moIndex]->tfDlg->setWindowTitle(QString("%1 Mean Object Transfer Function")
								   .arg(m_dwMO->cb_Classes->itemText(m_dwMO->cb_Classes->currentIndex())));
	//iAChartWithFunctionsWidget* histogram = m_activeChild->histogram();
	auto chart = new iAChartWithFunctionsWidget(m_MOData[moIndex]->tfDlg, "Probability", "Frequency");
	chart->setTransferFunction(m_MOData[moIndex]->tf.get());
	chart->addPlot(m_MOData[moIndex]->histoPlot);
	connect(chart, &iAChartWithFunctionsWidget::transferFunctionChanged, this,
		[this] { m_meanObjectWidget->renderWindow()->Render(); });
	m_MOData[moIndex]->tfDlg->setLayout(new QHBoxLayout);
	m_MOData[moIndex]->tfDlg->layout()->addWidget(chart);
	m_MOData[moIndex]->tfDlg->resize(800, 300);
	m_MOData[moIndex]->tfDlg->show();
}

void iAMeanObject::saveStl()
{
	int moIndex = m_dwMO->cb_Classes->currentIndex();
	if (moIndex < 0 || moIndex >= m_MOData.size())
	{  // if outside valid range - just to be on the safe side
		LOG(lvlError, QString("Invalid Mean Object index %1!").arg(moIndex));
		return;
	}
	auto fileName = QFileDialog::getSaveFileName(m_dwMO, tr("Save STL File"), m_sourcePath, tr("STL Files (*.stl)"));
	if (fileName.isEmpty())
	{
		return;
	}
	double isoValue = m_dwMO->dsb_IsoValue->value();

	auto progress = new iAMultiStepProgressObserver(2);
	auto job = runAsync(
		[this, progress, moIndex, isoValue, fileName]
		{
			auto moSurface = vtkSmartPointer<vtkMarchingCubes>::New();
			progress->observe(moSurface);
			moSurface->SetInputData(m_MOData[moIndex]->imageData);
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
	if (moIndex < 0 || moIndex >= m_MOData.size())
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
			storeImage(m_MOData[moIndex]->imageData, fileName);
			progress->emitProgress(100);
		},
		[progress] { delete progress; }, m_dwMO);
	iAJobListView::get()->addJob("Saving Mean Object volume", progress, job);
}

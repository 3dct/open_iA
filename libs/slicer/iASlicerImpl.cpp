// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASlicerImpl.h"

#include <iAAbortListener.h>
#include <iAChannelID.h>    // for NotExistingChannel
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iAConnector.h>
#include <iADefaultSettings.h>
#include <iAImageStackFileIO.h>
#include <iAJobListView.h>
#include <iALog.h>
#include <iAMagicLens.h>
#include <iAMagicLensConstants.h> // for DefaultMagicLensSize
#include <iAMathUtility.h>
#include <iAMovieHelper.h>
#include <iAParameterDlg.h>
#include <iAProgress.h>
#include <iARulerWidget.h>
#include <iARulerRepresentation.h>
#include <iASlicerSettings.h>
#include <iAStringHelper.h>
#include <iAToolsITK.h>
#include <iAToolsVTK.h>

// slicer
#include "iASlicerInteractorStyle.h"
#include "iASlicerProfile.h"
#include "iASlicerProfileHandles.h"
#include "iAVtkText.h"

// need to get rid of these dependencies:
#include "iAMainWindow.h"    // used for synchronizing positions - this should probably not happen in here!
#include "iAMdiChild.h"
#include "iAThemeHelper.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCaptionActor2D.h>
#include <vtkCommand.h>
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkDiskSource.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageBlend.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkLineSource.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkTransform.h>
#include <vtkWindowToImageFilter.h>
#include <vtkWorldPointPicker.h>

#include <QActionGroup>
#include <QBitmap>
#include <QCoreApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QEvent>
#include <QHoverEvent>

#include <cassert>
#include <numbers>

//! observer needs to be a separate class; otherwise there is an error when destructing,
//! as vtk deletes all its observers...
class iAObserverRedirect : public vtkCommand
{
public:
	iAObserverRedirect(iASlicer* redirect) : m_redirect(redirect)
	{}
private:
	void Execute(vtkObject * caller, unsigned long eventId, void * callData) override
	{
		m_redirect->execute(caller, eventId, callData);
	}
	iASlicer* m_redirect;
};


inline constexpr char SlicerSettingsName[] = "Default Settings/View: Slicer";
//! Settings applicable to a single slicer window.
class iASingleSlicerSettings : iASettingsObject<SlicerSettingsName, iASingleSlicerSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			QStringList mouseCursorOptions = QStringList()
				<< "Crosshair default"
				<< "Crosshair thick red" << "Crosshair thin red"
				<< "Crosshair thick orange" << "Crosshair thin orange"
				<< "Crosshair thick yellow" << "Crosshair thin yellow"
				<< "Crosshair thick blue" << "Crosshair thin blue"
				<< "Crosshair thick cyan" << "Crosshair thin cyan";
			selectOption(mouseCursorOptions, "Crosshair default");
			addAttr(attr, iASlicerImpl::MouseCursor, iAValueType::Categorical, mouseCursorOptions);
			addAttr(attr, iASlicerImpl::LinearInterpolation, iAValueType::Boolean, true);
			addAttr(attr, iASlicerImpl::AdjustWindowLevelEnabled, iAValueType::Boolean, false);
			addAttr(attr, iASlicerImpl::ShowPosition, iAValueType::Boolean, false);
			addAttr(attr, iASlicerImpl::ShowOtherSlicePlanes, iAValueType::Boolean, false);
			addAttr(attr, iASlicerImpl::ShowAxesCaption, iAValueType::Boolean, false);
			addAttr(attr, iASlicerImpl::ToolTipFontSize, iAValueType::Discrete, 12);
			addAttr(attr, iASlicerImpl::ShowTooltip, iAValueType::Boolean, true);
			addAttr(attr, iASlicerImpl::MagicLensSize, iAValueType::Discrete, DefaultMagicLensSize, MinimumMagicLensSize, MaximumMagicLensSize); //!< size (width & height) of the 2D magic lens (in pixels / pixel-equivalent units considering scaling)
			addAttr(attr, iASlicerImpl::MagicLensFrameWidth, iAValueType::Discrete, DefaultMagicLensFrameWidth, 0);  //!< width of the frame of the 2D magic lens
			addAttr(attr, iASlicerImpl::BackgroundColor, iAValueType::Color, ""); // by default, invalid color -> results in gray scale color depending on slicer mode
			// move to separate iso visualization maybe ?
			// {
			addAttr(attr, iASlicerImpl::ShowIsoLines, iAValueType::Boolean, false);
			addAttr(attr, iASlicerImpl::NumberOfIsoLines, iAValueType::Discrete, 5);
			addAttr(attr, iASlicerImpl::MinIsoValue, iAValueType::Continuous, 20000);    // should probably be dependent on the value range of the image ...?
			addAttr(attr, iASlicerImpl::MaxIsoValue, iAValueType::Continuous, 40000);    // should probably be dependent on the value range of the image ...?
			// }
			selfRegister();
		}
		return attr;
	}
};



iASlicerImpl::iASlicerImpl(QWidget* parent, const iASlicerMode mode,
	bool decorations /*= true*/, bool magicLensAvailable /*= true*/, vtkSmartPointer<vtkTransform> transform) :
	iASlicer(parent),
	m_contextMenu(new QMenu(this)),
	m_isSliceProfEnabled(false),
	m_sliceProfile(nullptr),
	m_profileHandlesEnabled(false),
	m_profileHandles(nullptr),
	m_mode(mode),
	m_decorations(decorations),
	m_showPositionMarker(false),
	m_magicLensInput(NotExistingChannel),
	m_fisheyeLensActivated(false),
	m_fisheyeRadius(80.0),
	m_innerFisheyeRadius(70.0),
	m_interactorStyle(iASlicerInteractorStyle::New()),
	m_renWin(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
	m_ren(vtkSmartPointer<vtkRenderer>::New()),
	m_camera(vtkCamera::New()),
	m_cameraOwner(true),
	m_transform(transform? transform: vtkSmartPointer<vtkTransform>::New()),
	m_pointPicker(vtkSmartPointer<vtkWorldPointPicker>::New()),
	m_textInfo(vtkSmartPointer<iAVtkText>::New()),
	m_slabThickness(0),
	m_roiActive(false),
	m_sliceNumber(0),
	m_cursorSet(false),
	m_sliceNumberChannel(NotExistingChannel),
	m_linkedMdiChild(nullptr)
{
	std::fill(m_angle, m_angle + 3, 0);
	setAutoFillBackground(false);
	setFocusPolicy(Qt::StrongFocus);    // to receive the KeyPress Event!
	setMouseTracking(true);             // to receive the Mouse Move Event
	m_renWin->AlphaBitPlanesOn();
	m_renWin->LineSmoothingOn();
	m_renWin->PointSmoothingOn();
	// Turned off, because of gray strokes e.g., on scalarBarActors. Only on NVIDIA graphic cards:
	m_renWin->PolygonSmoothingOff();
	setRenderWindow(m_renWin);
	setDefaultInteractor();

	m_renWin->AddRenderer(m_ren);
	m_ren->SetActiveCamera(m_camera);

	m_renWin->GetInteractor()->SetPicker(m_pointPicker);
	m_renWin->GetInteractor()->Initialize();
	m_interactorStyle->SetDefaultRenderer(m_ren);
	m_interactorStyle->setRightButtonDragZoomEnabled(false);   // we do have a context menu, this doesn't combine well with right-click zooming

	connect(&m_interactorStyle->qtEventObject(), &iASlicerInteractionEvents::selection, this,
		[this](int dragStart[2], int dragEnd[2])
		{
			uint channelID = firstVisibleChannel();
			if (channelID == NotExistingChannel)
			{
				return;
			}
			const int Component = 0;  // only check first component
			// acquire coordinates of clicks and convert to slicer output coordinates:
			double slicerPosStart[3], slicerPosEnd[3], globalPosStart[4], globalPosEnd[4];
			screenPixelPosToImgPos(dragStart, slicerPosStart, globalPosStart);
			screenPixelPosToImgPos(dragEnd, slicerPosEnd, globalPosEnd);
			int const* slicerExtent = m_channels[channelID]->output()->GetExtent();
			QPoint slicePixelPos[2] = {
				slicerPosToImgPixelCoords(channelID, slicerPosStart),
				slicerPosToImgPixelCoords(channelID, slicerPosEnd)
			};
			// make sure the coordinates stay inside valid range for slicer pixel image coordinates
			for (int i = 0; i < 2; ++i)
			{
				slicePixelPos[i].setX(clamp(slicerExtent[0], slicerExtent[1], slicePixelPos[i].x()));
				slicePixelPos[i].setY(clamp(slicerExtent[2], slicerExtent[3], slicePixelPos[i].y()));
			}
			// find the correctly ordered start/end for x/y:
			// TODO: maybe use computeMinMax?
			int startX = std::min(slicePixelPos[0].x(), slicePixelPos[1].x());
			int endX   = std::max(slicePixelPos[0].x(), slicePixelPos[1].x());
			int startY = std::min(slicePixelPos[0].y(), slicePixelPos[1].y());
			int endY   = std::max(slicePixelPos[0].y(), slicePixelPos[1].y());

			// extract image part, get minimum/maximum intensity value:
			double minVal = std::numeric_limits<double>::max();
			double maxVal = std::numeric_limits<double>::lowest();
			for (int x = startX; x <= endX; ++x)
			{
				for (int y = startY; y <= endY; ++y)
				{
					double value = m_channels[channelID]->output()->GetScalarComponentAsDouble(x, y, 0, Component);
					if (value > maxVal)
					{
						maxVal = value;
					}
					if (value < minVal)
					{
						minVal = value;
					}
				}
			}
			emit regionSelected(minVal, maxVal, channelID);
		});
	connect(&m_interactorStyle->qtEventObject(), &iASlicerInteractionEvents::sliceChange, this,
		[this](int direction)
		{
			setSliceNumber(sliceNumber() + direction);
		});

	iAObserverRedirect* redirect(new iAObserverRedirect(this));
	m_renWin->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::LeftButtonReleaseEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::RightButtonPressEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::MouseMoveEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::KeyReleaseEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::MouseWheelBackwardEvent, redirect);
	m_renWin->GetInteractor()->AddObserver(vtkCommand::MouseWheelForwardEvent, redirect);

	updateBackground();

	auto settingsAction = m_contextMenu->addAction(tr("Settings"), this, [this]
	{
		if (editSettingsDialog<iASlicerImpl>(iASingleSlicerSettings::defaultAttributes(), m_settings, "Slicer view settings", *this, &iASlicerImpl::applySettings))
		{
			update();
		}
	});
	settingsAction->setIcon(iAThemeHelper::icon("settings_slicer"));

	m_contextMenu->addSeparator();

	m_actionLinearInterpolation = m_contextMenu->addAction(tr("Linear Interpolation"), this, &iASlicerImpl::toggleLinearInterpolation);
	m_actionLinearInterpolation->setCheckable(true);
	m_actionShowTooltip = m_contextMenu->addAction(tr("Show Tooltip"), this, &iASlicerImpl::toggleShowTooltip);
	m_actionShowTooltip->setCheckable(true);

	m_contextMenu->addSeparator();
	m_actionToggleNormalInteraction = new QAction(tr("Click+Drag: disabled"), m_contextMenu);
	m_actionToggleNormalInteraction->setCheckable(true);
	m_contextMenu->addAction(m_actionToggleNormalInteraction);
	m_actionToggleRegionTransferFunction = new QAction(tr("Click+Drag: Set Transfer Function for Region"), m_contextMenu);
	m_actionToggleRegionTransferFunction->setCheckable(true);
	m_contextMenu->addAction(m_actionToggleRegionTransferFunction);
	m_actionToggleWindowLevelAdjust = new QAction(tr("Click+Drag: Adjust Window+Level"), m_contextMenu);
	m_actionToggleWindowLevelAdjust->setCheckable(true);
	m_contextMenu->addAction(m_actionToggleWindowLevelAdjust);
	m_actionInteractionMode = new QActionGroup(m_contextMenu);
	m_actionInteractionMode->addAction(m_actionToggleNormalInteraction);
	m_actionInteractionMode->addAction(m_actionToggleRegionTransferFunction);
	m_actionInteractionMode->addAction(m_actionToggleWindowLevelAdjust);
	connect(m_actionInteractionMode, &QActionGroup::triggered, this, &iASlicerImpl::toggleInteractionMode);
	m_contextMenu->addSeparator();

	m_actionFisheyeLens = m_contextMenu->addAction(iAThemeHelper::icon("fisheyelens"), tr("Fisheye Lens"), this, &iASlicerImpl::fisheyeLensToggled);
	m_actionFisheyeLens->setShortcut(Qt::Key_O);
	m_actionFisheyeLens->setCheckable(true);
	m_actionFisheyeLens->setChecked(false);

	if (magicLensAvailable)
	{
		m_magicLens = std::make_shared<iAMagicLens>(activeBGColor());
		m_magicLens->setRenderWindow(m_renWin);
		// setup context menu for the magic lens view options
		m_contextMenu->addSeparator();
		m_actionMagicLens = m_contextMenu->addAction(iAThemeHelper::icon("magic_lens_2d"), tr("Magic Lens"), this, &iASlicerImpl::magicLensToggled);
		m_actionMagicLens->setCheckable(true);
		m_actionMagicLens->setChecked(false);

		QActionGroup * actionGr(new QActionGroup(this));
		m_actionMagicLensCentered = m_contextMenu->addAction(tr("Magic Lens: Centered"), this, &iASlicerImpl::menuCenteredMagicLens);
		m_actionMagicLensCentered->setCheckable(true);
		m_actionMagicLensCentered->setChecked(true);
		actionGr->addAction(m_actionMagicLensCentered);
		m_actionMagicLensOffset = m_contextMenu->addAction(tr("Magic Lens: Offset"), this, &iASlicerImpl::menuOffsetMagicLens);
		m_actionMagicLensOffset->setCheckable(true);
		actionGr->addAction(m_actionMagicLensOffset);
	}

	m_textInfo->addToScene(m_ren);
	m_textInfo->setText(" ");
	m_textInfo->show(m_decorations);

	if (decorations)
	{
		m_sliceProfile = new iASlicerProfile();
		m_sliceProfile->setVisibility(false);

		m_profileHandles = new iASlicerProfileHandles();
		m_profileHandles->setVisibility(false);

		m_scalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
		m_rulerWidget = vtkSmartPointer<iARulerWidget>::New();

		vtkNew<vtkTextProperty> textProperty;
		textProperty->SetBold(1);
		textProperty->SetItalic(1);
		textProperty->SetColor(1, 1, 1);
		textProperty->SetJustification(VTK_TEXT_CENTERED);
		textProperty->SetVerticalJustification(VTK_TEXT_CENTERED);
		textProperty->SetOrientation(1);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelFormat("%.2f");
		m_scalarBarWidget->GetScalarBarActor()->SetTitleTextProperty(textProperty);
		m_scalarBarWidget->GetScalarBarActor()->SetLabelTextProperty(textProperty);
		m_scalarBarWidget->GetScalarBarRepresentation()->SetOrientation(1);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.92, 0.2);
		m_scalarBarWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.06, 0.75);
		m_scalarBarWidget->GetScalarBarActor()->SetTitle("Range");
		m_scalarBarWidget->SetRepositionable(true);
		m_scalarBarWidget->SetResizable(true);
		m_scalarBarWidget->SetInteractor(m_renWin->GetInteractor());

		m_posMarker.actor->GetProperty()->SetColor(0, 1, 0);
		m_posMarker.actor->GetProperty()->SetOpacity(1);
		m_posMarker.actor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		m_posMarker.actor->SetVisibility(false);

		m_measureLine.source->SetPoint1(0.0, 0.0, 0.0);
		m_measureLine.source->SetPoint2(10.0, 10.0, 0.0);
		m_measureLine.source->Update();
		m_measureLine.actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		m_measureLine.actor->GetProperty()->SetOpacity(1);
		m_measureLine.actor->SetVisibility(false);
		m_measureDisk.source->SetCircumferentialResolution(50);
		m_measureDisk.source->Update();
		m_measureDisk.actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		m_measureDisk.actor->GetProperty()->SetOpacity(1);
		m_measureDisk.actor->SetVisibility(false);
		m_measureDisk.actor->GetProperty()->SetRepresentation(VTK_WIREFRAME);

		m_roi.actor->SetVisibility(false);
		m_roi.actor->GetProperty()->SetColor(1, 0, 0);
		m_roi.actor->GetProperty()->SetOpacity(1);
		m_roi.actor->GetProperty()->SetRepresentation(VTK_WIREFRAME);
		m_roi.mapper->Update();

		for (int i = 0; i < 2; ++i)
		{
			m_axisTextActor[i] = vtkSmartPointer<vtkCaptionActor2D>::New();
			m_axisTextActor[i]->SetPickable(false);
			m_axisTextActor[i]->SetDragable(false);
			m_axisTextActor[i]->GetCaptionTextProperty()->SetFontFamilyToArial();
			m_axisTextActor[i]->GetCaptionTextProperty()->SetColor(1.0, 1.0, 1.0);
			m_axisTextActor[i]->SetBorder(false);
			m_axisTextActor[i]->SetLeader(false);
			m_ren->AddActor(m_axisTextActor[i]);
			m_axisTextActor[i]->SetVisibility(false);
			m_axisTextActor[i]->GetTextActor()->SetTextScaleModeToNone();
		}
		m_axisTextActor[0]->SetCaption(axisName(mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)).toStdString().c_str());
		m_axisTextActor[0]->GetCaptionTextProperty()->SetVerticalJustificationToTop();
		m_axisTextActor[0]->GetCaptionTextProperty()->SetJustificationToCentered();
		m_axisTextActor[1]->SetCaption(axisName(mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)).toStdString().c_str());
		m_axisTextActor[1]->GetCaptionTextProperty()->SetVerticalJustificationToCentered();
		m_axisTextActor[1]->GetCaptionTextProperty()->SetJustificationToRight();

		m_rulerWidget->SetInteractor(m_renWin->GetInteractor());
		m_rulerWidget->SetEnabled(true);
		m_rulerWidget->SetRepositionable(true);
		m_rulerWidget->SetResizable(true);
		m_rulerWidget->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue(0.333, 0.05);
		m_rulerWidget->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue(0.333, 0.051);

		m_ren->AddActor(m_posMarker.actor);
		m_ren->AddActor(m_measureLine.actor);
		m_ren->AddActor(m_measureDisk.actor);
		m_ren->AddActor(m_roi.actor);
		m_ren->AddActor(m_otherSliceAxes[0].actor);
		m_ren->AddActor(m_otherSliceAxes[1].actor);
	}
	m_renWin->SetNumberOfLayers(4);
	m_camera->SetParallelProjection(true);

	if (m_decorations)
	{
		m_sliceProfile->addToRenderer(m_ren);
		m_profileHandles->addToRenderer(m_ren);
	}
	applySettings(extractValues(iASingleSlicerSettings::defaultAttributes()));
	m_ren->ResetCamera();
}

iASlicerImpl::~iASlicerImpl()
{
	disconnect();

	m_interactorStyle->Delete();

	if (m_cameraOwner)
	{
		m_camera->Delete();
	}
	if (m_decorations)
	{
		delete m_contextMenu;
		delete m_sliceProfile;
		delete m_profileHandles;
	}
}

void iASlicerImpl::setMode( const iASlicerMode mode )
{
	m_mode = mode;
	for (auto ch : m_channels)
	{
		ch->updateResliceAxesDirectionCosines(m_mode);
	}
	updateBackground();
}

iASlicerMode iASlicerImpl::mode() const
{
	return m_mode;
}

void iASlicerImpl::enableInteractor(bool enable)
{
	if (enable)
	{
	// TODO NEWIO: "also updates widget" -> this shouldn't do multiple things at once, check if required
		m_renWin->GetInteractor()->ReInitialize();
		update();
	}
	else
	{
		m_renWin->GetInteractor()->Disable();
	}
	LOG(lvlInfo, tr("Slicer %1: interaction %2.").arg(slicerModeString(m_mode)).arg(iAConverter<bool>::toString(enable)));
}

bool iASlicerImpl::isInteractorEnabled() const
{
	return m_renWin->GetInteractor()->GetEnabled();
}

void iASlicerImpl::update()
{
	if (!isVisible())
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->updateMapper();
		ch->reslicer()->Update();
	}
	m_renWin->GetInteractor()->ReInitialize();
	m_renWin->GetInteractor()->Render();
	m_ren->Render();
	if (m_magicLens)
	{
		m_magicLens->render();
	}
	iAVtkWidget::update();

	emit updateSignal();
}

void iASlicerImpl::saveMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();
	if( movie_file_types.isEmpty() )
	{
		QMessageBox::information( this, "Movie Export", "This version of open_iA was built without movie export support!");
		return;
	}
	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel)
	{
		return;
	}
	QString fileName = QFileDialog::getSaveFileName( this,
		tr( "Export as a movie" ),
		"", // TODO: get directory of file?
		movie_file_types );
	if (fileName.isEmpty())
	{
		return;
	}
	int const* imgExtent = m_channels[channelID]->input()->GetExtent();
	int const sliceZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int const sliceFrom = imgExtent[sliceZAxisIdx * 2];
	int const sliceTo = imgExtent[sliceZAxisIdx * 2 + 1];
	iAAttributes params;
	addAttr(params, "Start slice", iAValueType::Discrete, sliceFrom, sliceFrom, sliceTo);
	addAttr(params, "End slice", iAValueType::Discrete, sliceTo, sliceFrom, sliceTo);
	addAttr(params, "Video quality", iAValueType::Discrete, 2, 0, 2);
	addAttr(params, "Frame rate", iAValueType::Discrete, 25, 1, 1000);
	iAParameterDlg dlg(this, "Save movie options", params,
		"Creates a movie by slicing through the object.<br>"
		"The <em>start slice</em> defines the number of the first slice shown in the video. "
		"The <em>end slice</em> defines the number of the last slice shown in the video. "
		"The <em>video quality</em> specifies the quality of the output video (range: 0..2, 0 - worst, 2 - best; default: 2). "
		"The <em>frame rate</em> specifies the frames per second (default: 25). ");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto pVals = dlg.parameterValues();
	auto start = pVals["Start slice"].toInt();
	auto end = pVals["End slice"].toInt();
	if (start == end)
	{
		QMessageBox::information(this, "Movie Export", "Start slice is the same as end slice! To export a single slice, please use the image export!");
		return;
	}
	auto quality = pVals["Video quality"].toInt();
	auto fps = pVals["Frame rate"].toInt();

	saveSliceMovie(fileName, start, end, quality, fps);
}

void iASlicerImpl::setSliceNumber( int sliceNumber )
{
	if (m_sliceNumberChannel == NotExistingChannel)
	{
		return;
	}
	// TODO: set slice position (in scene coordinates) instead of number
	//       then we wouldn't need image spacing and origin below
	//       (which don't make too much sense anyway, if it's not the same between loaded datasets)
	// also, maybe clamp to boundaries of all currently loaded datasets?

	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int maxSliceNr = m_channels[m_sliceNumberChannel]->input()->GetDimensions()[sliceAxis];
	sliceNumber = clamp(0, maxSliceNr, sliceNumber);
	if (sliceNumber == m_sliceNumber)
	{
		return;
	}
	m_sliceNumber = sliceNumber;
	double xyz[3] = { 0.0, 0.0, 0.0 };
	xyz[sliceAxis] = sliceNumber;
	if (m_roiActive)
	{
		m_roi.actor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < (m_roiSlice[1]));
	}
	double const * spacing = m_channels[m_sliceNumberChannel]->input()->GetSpacing();
	double const * origin = m_channels[m_sliceNumberChannel]->input()->GetOrigin();
	for (auto ch : m_channels)
	{
		ch->setResliceAxesOrigin(origin[0] + xyz[0] * spacing[0], origin[1] + xyz[1] * spacing[1], origin[2] + xyz[2] * spacing[2]);
	}
	updateMagicLensColors();
	emit sliceNumberChanged( m_mode, sliceNumber );
	update();
}

void iASlicerImpl::setSlicePosition(double slicePos)
{
	if (m_sliceNumberChannel == NotExistingChannel)
	{
		return;
	}
	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	double const* spacing = m_channels[m_sliceNumberChannel]->input()->GetSpacing();
	// TODO: once all occurrences of setSliceNumber have been replaced with setSlicePosition,
	//    move implementation from there to here, should simplify stuff a little bit
	//    (e.g., no more spacing computations required)
	setSliceNumber(slicePos / spacing[sliceAxis]);
}

void iASlicerImpl::setOtherSlicePlanePos(int sliceAxis, double slicePos)
{
	sliceAxis = mapGlobalToSliceAxis(m_mode, sliceAxis);
	if (sliceAxis == 2)
	{
		LOG(lvlDebug, QString("No need to call setOtherSlicePlanePos with slice axis itself (mode: %1; sliceAxis: %2, slicePos: %3!")
			.arg(m_mode).arg(sliceAxis).arg(slicePos));
		return;
	}
	auto src = m_otherSliceAxes[sliceAxis].source;
	double pt[3];
	src->GetPoint1(pt); pt[sliceAxis] = slicePos; src->SetPoint1(pt);
	src->GetPoint2(pt); pt[sliceAxis] = slicePos; src->SetPoint2(pt);
	if (!m_settings[ShowOtherSlicePlanes].toBool())
	{
		return;
	}
	update();
}

void iASlicerImpl::setLinearInterpolation(bool enabled)
{
	for (auto channel: m_channels)
	{
		channel->setInterpolate(enabled);
	}
	if (m_magicLens)
	{
		m_magicLens->setInterpolate(enabled);
	}
}

void iASlicerImpl::applySettings( QVariantMap const & settings )
{
	m_settings = settings;
	m_actionLinearInterpolation->setChecked(settings[iASlicerImpl::LinearInterpolation].toBool());
	setLinearInterpolation(settings[iASlicerImpl::LinearInterpolation].toBool());
	m_interactorStyle->setInteractionMode(settings[iASlicerImpl::AdjustWindowLevelEnabled].toBool()
		? iASlicerInteractorStyle::imWindowLevelAdjust
		: iASlicerInteractorStyle::imNormal
	);
	setMouseCursor(settings[iASlicerImpl::MouseCursor].toString());
	setContours(settings[iASlicerImpl::NumberOfIsoLines].toInt(),
		settings[iASlicerImpl::MinIsoValue].toDouble(), settings[iASlicerImpl::MaxIsoValue].toDouble());
	showIsolines(settings[iASlicerImpl::ShowIsoLines].toBool());
	showPosition(settings[iASlicerImpl::ShowPosition].toBool());
	if (m_decorations)
	{
		m_axisTextActor[0]->SetVisibility(settings[iASlicerImpl::ShowAxesCaption].toBool());
		m_axisTextActor[1]->SetVisibility(settings[iASlicerImpl::ShowAxesCaption].toBool());
		m_otherSliceAxes[0].actor->SetVisibility(m_settings[ShowOtherSlicePlanes].toBool());
		m_otherSliceAxes[1].actor->SetVisibility(m_settings[ShowOtherSlicePlanes].toBool());
	}
	// compromise between keeping old behavior (tooltips disabled if m_decorations == false),
	// but still making it possible to enable tooltips via context menu: only enable tooltips
	// from settings if decorations turned on:
	m_settings[iASlicerImpl::ShowTooltip] = m_settings[iASlicerImpl::ShowTooltip].toBool() && m_decorations;
	m_textInfo->setFontSize(settings[iASlicerImpl::ToolTipFontSize].toInt());
	m_axisTextActor[0]->GetCaptionTextProperty()->SetFontSize(1.2 * settings[iASlicerImpl::ToolTipFontSize].toInt());
	m_axisTextActor[1]->GetCaptionTextProperty()->SetFontSize(1.2 * settings[iASlicerImpl::ToolTipFontSize].toInt());
	setBackground(variantToColor(settings[iASlicerImpl::BackgroundColor]));
	m_textInfo->show(m_settings[iASlicerImpl::ShowTooltip].toBool());
	setMagicLensFrameWidth(m_settings[iASlicerImpl::MagicLensFrameWidth].toInt());
	setMagicLensSize(m_settings[iASlicerImpl::MagicLensSize].toInt());
	if (m_magicLens)
	{
		updateMagicLens();
	}
	else
	{   // updateMagicLens updates the render window
		m_renWin->Render();
	}
}

void iASlicerImpl::setMagicLensEnabled( bool isEnabled )
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "setMagicLensEnabled called on slicer which doesn't have a magic lens!");
		return;
	}
	setCursor  (isEnabled ? Qt::BlankCursor : mouseCursor());
	if (isEnabled)
	{
		m_textInfo->show(false);
	}
	m_magicLens->setEnabled(isEnabled);
	updateMagicLens();
}

void iASlicerImpl::setMagicLensSize(int newSize)
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "setMagicLensSize called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setSize(newSize);
	updateMagicLens();
}

int iASlicerImpl::magicLensSize() const
{
	return m_magicLens ? m_magicLens->size() : 0;
}

void iASlicerImpl::setMagicLensFrameWidth(int newWidth)
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "SetMagicLensFrameWidth called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setFrameWidth(newWidth);
	updateMagicLens();
}

void iASlicerImpl::setMagicLensCount(int count)
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "setMagicLensCount called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setLensCount(count);
	updateMagicLens();
}

void iASlicerImpl::setMagicLensInput(uint id)
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "setMagicLensInput called on slicer which doesn't have a magic lens!");
		return;
	}
	iAChannelSlicerData * d = channel(id);
	assert(d);
	if (!d)
	{
		return;
	}
	m_magicLensInput = id;
	m_magicLens->addInput(d->reslicer(), d->colorTF(), d->name());
	update();
}

uint iASlicerImpl::magicLensInput() const
{
	return m_magicLensInput;
}

void iASlicerImpl::setMagicLensOpacity(double opacity)
{
	if (!m_magicLens)
	{
		LOG(lvlWarn, "setMagicLensOpacity called on slicer which doesn't have a magic lens!");
		return;
	}
	m_magicLens->setOpacity(opacity);
	update();
}

double iASlicerImpl::magicLensOpacity() const
{
	return (m_magicLens) ? m_magicLens->opacity() : 0;
}

vtkRenderer * iASlicerImpl::renderer()
{
	return m_ren;
}

vtkCamera * iASlicerImpl::camera()
{
	return m_camera;
}

vtkRenderWindowInteractor * iASlicerImpl::interactor()
{
	return m_renWin->GetInteractor();
}

void iASlicerImpl::addChannel(uint id, iAChannelData const & chData, bool enable)
{
	assert(!m_channels.contains(id));
	auto chSlicerData = createChannel(id, chData);
	auto image = chData.image();
	double const * imgSpc = image->GetSpacing();
	if (m_channels.size() == 1)    // TODO: update required for new channels?
	{
		setSlicerRange(id);
		if (m_decorations)
		{
			setScalarBarTF(chData.colorTF());
			updatePositionMarkerExtent();
			// update position of axis caption:
			double const* spc = m_channels[id]->output()->GetSpacing();
			int    const* dim = m_channels[id]->output()->GetDimensions();
			std::array<double, 2> size = { dim[0]  * spc[0], dim[1] * spc[1] };
			// 0.5 offsets probably necessary due to border on ?
			std::array<double, 2> halfSize = { size[0] / 2.0 - 0.5 * spc[0], size[1] / 2.0 - 0.5 * spc[1] };
			double ofs = std::max(size[0], size[1]) / 20;
			m_axisTextActor[0]->SetAttachmentPoint(halfSize[0], -(ofs + 0.5 * spc[1]), 0);
			m_axisTextActor[1]->SetAttachmentPoint(-(ofs + 0.5 * spc[0]), halfSize[1], 0);
		}
	}
	double origin[3];
	image->GetOrigin(origin);
	int axis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	origin[axis] += static_cast<double>(sliceNumber()) * imgSpc[axis];
	setResliceChannelAxesOrigin(id, origin[0], origin[1], origin[2]);
	if (enable)
	{
		enableChannel(id, true);
	}
}

void iASlicerImpl::setSlicerRange(uint channelID)
{
	m_sliceNumberChannel = channelID;
	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	auto img = channel(m_sliceNumberChannel)->input();
	auto ext = img->GetExtent();
	auto spc = img->GetSpacing();
	int minIdx = ext[sliceAxis * 2];
	int maxIdx = ext[sliceAxis * 2 + 1];
	setSliceNumber((maxIdx - minIdx) / 2 + minIdx);
	// Initialize line indicators for other slice planes:
	for (int axis = 0; axis < 2; ++axis)
	{
		double pt[3] = {};
		int axis1 = mapSliceToGlobalAxis(m_mode, axis);   //< global index of "current" axis in slicer; should be set to middle slice
		int axis2 = mapSliceToGlobalAxis(m_mode, 1-axis); //< global index of "other" axis in slicer; varies from min to max
		pt[axis] = (ext[axis1 * 2] + (ext[axis1 * 2 + 1] - ext[axis1 * 2]) / 2.0) * spc[axis1];
		pt[1 - axis] = ext[axis2 * 2] * spc[axis2];
		m_otherSliceAxes[axis].source->SetPoint1(pt);
		pt[1 - axis] = ext[axis2 * 2 + 1] * spc[axis2];
		m_otherSliceAxes[axis].source->SetPoint2(pt);
		auto c = slicerColor(static_cast<iASlicerMode>(axis));
		m_otherSliceAxes[axis].actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
		m_otherSliceAxes[axis].mapper->Update();
	}
	emit sliceRangeChanged(minIdx, maxIdx, m_sliceNumber);
}

void iASlicerImpl::updateMagicLensColors()
{
	if (m_magicLens)
	{
		m_magicLens->updateColors();
	}
}

void iASlicerImpl::setDefaultInteractor()
{
	m_renWin->GetInteractor()->SetInteractorStyle(m_interactorStyle);
}

void iASlicerImpl::addImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->AddActor(imgActor);
}

void iASlicerImpl::removeImageActor(vtkSmartPointer<vtkImageActor> imgActor)
{
	m_ren->RemoveActor(imgActor);
}

void iASlicerImpl::setROIVisible(bool visible)
{
	if (!m_decorations)
	{
		return;
	}
	m_roiActive = visible;
	m_roi.actor->SetVisibility(visible);
}

void iASlicerImpl::updateROI(int const roi[6])
{
	auto channelID = firstVisibleChannel();
	if (!m_decorations || !m_roiActive || channelID == NotExistingChannel)
	{
		return;
	}
	double const * spacing = m_channels[channelID]->output()->GetSpacing();
	int sliceXAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X);
	int sliceYAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y);
	int sliceZAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	m_roiSlice[0] = roi[sliceZAxis];
	m_roiSlice[1] = roi[sliceZAxis] + roi[sliceZAxis + 3];
	// apparently, image actor starts output at -0,5spacing, -0.5spacing (probably a side effect of BorderOn)
	// That's why we have to subtract 0.5 from the coordinates!
	double xMin = (roi[sliceXAxis] - 0.5)  * spacing[sliceXAxis],
	       yMin = (roi[sliceYAxis] - 0.5)  * spacing[sliceYAxis];
	double xMax = xMin + roi[sliceXAxis+3] * spacing[sliceXAxis],
	       yMax = yMin + roi[sliceYAxis+3] * spacing[sliceYAxis];
	m_roi.source->SetBounds(xMin, xMax, yMin, yMax, 0, 0);
	m_roi.actor->SetVisibility(m_roiSlice[0] <= m_sliceNumber && m_sliceNumber < m_roiSlice[1]);
	m_roi.mapper->Update();
	m_renWin->GetInteractor()->Render();
}

void iASlicerImpl::setResliceAxesOrigin(double x, double y, double z)
{
	if (m_renWin->GetInteractor()->GetEnabled())
	{
		for (auto ch : m_channels)
		{
			ch->setResliceAxesOrigin(x, y, z);
		}
		m_renWin->GetInteractor()->Render();
	}
}

void iASlicerImpl::setPositionMarkerCenter(double x, double y, double z)
{
	uint channelID = firstVisibleChannel();
	if (!m_decorations || channelID == NotExistingChannel)
	{
		return;
	}
	if (m_renWin->GetInteractor()->GetEnabled() && m_showPositionMarker)
	{
		double const* spacing = m_channels[channelID]->output()->GetSpacing();
		int zIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
		// we only want to show the position in a small slab around the current slice;
		// but we also want to make the position marker easy to spot;
		// so we scale the size of the position marker inversely to the distance to the current slice
		double scale = 1.0 / std::max(1.0, (std::abs(z / spacing[zIdx] - m_sliceNumber) - (m_slabThickness/2)) / 3 ) * m_positionMarkerSize;
		m_posMarker.source->SetXLength(scale * spacing[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)]);
		m_posMarker.source->SetYLength(scale * spacing[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
		m_posMarker.actor->SetVisibility(true);
		m_posMarker.source->SetCenter(x, y, 0);
		m_posMarker.mapper->Update();
		update();
	}
}

void iASlicerImpl::showIsolines(bool s)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setShowContours(m_ren, s);
	}
}

void iASlicerImpl::showPosition(bool s)
{
	if (!m_decorations)
	{
		return;
	}
	m_showPositionMarker = s;
}

void iASlicerImpl::saveSliceMovie(QString const& fileName, int sliceFrom, int sliceTo, int qual, int fps)
{
	// TODO: select channel / for all channels?
	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel)
	{
		return;
	}
	QString movie_file_types = GetAvailableMovieFormats();
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "This version of open_iA was built without movie export support!");
		return;
	}
	auto movieWriter = GetMovieWriter(fileName, qual, fps);
	if (movieWriter.GetPointer() == nullptr)
	{
		return;
	}
	iAProgress p;
	iASimpleAbortListener aborter;
	auto jobHandle = iAJobListView::get()->addJob("Exporting Movie", &p, &aborter);
	LOG(lvlInfo, tr("Movie export started, output file name: %1.").arg(fileName));
	m_renWin->GetInteractor()->Disable();

	auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = m_renWin->GetSize();
	if (rws[0] % 2 != 0)
	{
		rws[0]++;
	}
	if (rws[1] % 2 != 0)
	{
		rws[1]++;
	}
	m_renWin->SetSize(rws);
	m_renWin->Render();
	windowToImage->SetInput(m_renWin);
	windowToImage->ReadFrontBufferOff();

	movieWriter->SetInputConnection(windowToImage->GetOutputPort());
	movieWriter->Start();

	double const * imgOrigin = m_channels[channelID]->input()->GetOrigin();
	double const * imgSpacing = m_channels[channelID]->input()->GetSpacing();

	double oldResliceAxesOrigin[3];
	m_channels[channelID]->resliceAxesOrigin(oldResliceAxesOrigin);

	double movingOrigin[3];
	for (int i = 0; i < 3; ++i)
	{
		movingOrigin[i] = imgOrigin[i];
	}
	int const sliceZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	auto sliceIncr = (sliceTo - sliceFrom) > 0 ? 1 : -1;
	for (int slice = sliceFrom; slice != (sliceTo+sliceIncr) && !aborter.isAborted(); slice += sliceIncr)
	{
		movingOrigin[sliceZAxisIdx] = imgOrigin[sliceZAxisIdx] + slice * imgSpacing[sliceZAxisIdx];
		m_channels[channelID]->setResliceAxesOrigin(movingOrigin[0], movingOrigin[1], movingOrigin[2]);
		m_channels[channelID]->updateReslicer();
		m_renWin->Render();
		windowToImage->Modified();
		windowToImage->Update();
		movieWriter->Write();
		if (movieWriter->GetError())
		{
			LOG(lvlError, movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		p.emitProgress((slice - sliceFrom) * 100.0 / (sliceTo - sliceFrom));
		QCoreApplication::processEvents();
	}
	m_channels[channelID]->setResliceAxesOrigin(oldResliceAxesOrigin[0], oldResliceAxesOrigin[1], oldResliceAxesOrigin[2]);
	update();
	movieWriter->End();
	m_renWin->GetInteractor()->Enable();

	printFinalLogMessage(movieWriter, aborter);
}

namespace
{
	QString const SaveNative("Save native image");
	QString const Output16Bit("16 bit native output");
}

void iASlicerImpl::saveAsImage()
{
	if (!hasChannel(0))
	{
		return;
	}
	iAImageStackFileIO io;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"", // TODO: get directory of file?
		io.filterString());
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	QString const Channel("Channel");
	addAttr(params, SaveNative, iAValueType::Boolean, true);
	bool moreThanOneChannel = m_channels.size() > 1;
	QFileInfo fi(fileName);
	if (moreThanOneChannel)
	{
		QStringList currentChannels;
		for (auto ch : m_channels)
		{
			currentChannels << ch->name();
		}
		addAttr(params, Channel, iAValueType::Categorical, currentChannels);
	}
	if ((QString::compare(fi.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
		(QString::compare(fi.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		addAttr(params, Output16Bit, iAValueType::Boolean, false);
	}

	iAParameterDlg dlg(this, "Save options", params, "<em>" + SaveNative + "</em> means that the image to be written will be taken directly from the slicer, "
		"and the intensity will be rescaled to the range of the output format (0..255 unless .tif[f] is chosen and <em>16 bit native output</em> is selected). "
		"If disabled, a screen shot of this slicer will be stored (including scale bar, color scale, ...) "
		"<em>" + Channel + "</em> determines which image channel will be considered for native output (only affects native output) "
		"<em>" + Output16Bit + "</em> if enabled (and if native output chosen above), native output will be 16 bit. "
		"Note that this option is only available for .tif[f] output files (other image file formats do not support 16 bit depth).");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	bool saveNative = values[SaveNative].toBool();
	bool output16Bit = values.contains(Output16Bit)	? values[Output16Bit].toBool() : false;
	iAConnector con;
	vtkSmartPointer<vtkImageData> img;
	auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	if (saveNative)
	{
		int selectedChannelID = 0;
		if (moreThanOneChannel)
		{
			QString selectedChannelName = values[Channel].toString();
			for (auto key : m_channels.keys())
			{
				if (m_channels[key]->name() == selectedChannelName)
				{
					selectedChannelID = key;
					break;
				}
			}
		}
		con.setImage(m_channels[selectedChannelID]->output());
		iAITKIO::ImagePointer imgITK;
		if (!output16Bit)
		{
			imgITK = rescaleImageTo<unsigned char>(con.itkImage(), 0, 255);
		}
		else
		{
			imgITK = rescaleImageTo<unsigned short>(con.itkImage(), 0, 65535);
		}
		con.setImage(imgITK);
		img = con.vtkImage();
	}
	else
	{
		windowToImage->SetInput(m_renWin);
		windowToImage->Update();
		img = windowToImage->GetOutput();
	}
	writeSingleSliceImage(fileName, img);
}

void iASlicerImpl::saveImageStack()
{
	// TODO: allow selecting channel to export? export all channels?
	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel)
	{
		return;
	}
	auto imageData = m_channels[channelID]->input();
	iAImageStackFileIO io;
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image Stack"),
		"",  // TODO: get directory of file?
		io.filterString());
	if (file.isEmpty())
	{
		return;
	}

	QFileInfo fileInfo(file);
	QString baseName = fileInfo.absolutePath() + "/" + fileInfo.baseName();

	int const* imgExtent = imageData->GetExtent();
	double const* imgSpacing = imageData->GetSpacing();
	int const sliceZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	int const sliceMin = imgExtent[sliceZAxisIdx * 2];
	int const sliceMax = imgExtent[sliceZAxisIdx * 2 + 1];
	iAAttributes params;
	addAttr(params, "From Slice Number:", iAValueType::Discrete, sliceMin, sliceMin, sliceMax);
	addAttr(params, "To Slice Number:", iAValueType::Discrete, sliceMax, sliceMin, sliceMax);
	iAParameterDlg dlg(this, "Save options", params, "<strong>Note:</strong> "
		"This is intended for a screenshot-like export of slices - "
		"that is, including the full slicer window with scale- and color bar. "
		"If you want to export a stack of axis-aligned images in native resolution, "
		"use File -> Save Dataset -> Image Stack instead!");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	int sliceFrom = values["From Slice Number:"].toInt();
	int sliceTo = values["To Slice Number:"].toInt();
	if (sliceFrom < sliceMin || sliceFrom > sliceTo || sliceTo > sliceMax)
	{
		QMessageBox::information(this, "Save Image Stack", QString("Invalid input: 'From Slice Number' is greater than 'To Slice Number',"
			" or 'From Slice Number' or 'To Slice Number' are outside of valid region [%1..%2]!").arg(sliceMin).arg(sliceMax));
		return;
	}
	iAProgress p;
	iASimpleAbortListener aborter;
	auto jobHandle = iAJobListView::get()->addJob("Exporting Image Stack", &p, &aborter);
	m_renWin->GetInteractor()->Disable();
	double movingOrigin[3];
	imageData->GetOrigin(movingOrigin);
	double const * imgOrigin = imageData->GetOrigin();

	// workaround for slice 0 bug - as "warmup", switch to "from" slice already before main loop:
	movingOrigin[sliceZAxisIdx] = imgOrigin[sliceZAxisIdx];
	setResliceAxesOrigin(movingOrigin[0], movingOrigin[1], movingOrigin[2] + sliceFrom * imgSpacing[sliceZAxisIdx]);
	m_channels[channelID]->updateReslicer();
	update();
	QCoreApplication::processEvents();

	for (int slice = sliceFrom; slice <= sliceTo && !aborter.isAborted(); slice++)
	{
		movingOrigin[sliceZAxisIdx] = imgOrigin[sliceZAxisIdx] + slice * imgSpacing[sliceZAxisIdx];
		setResliceAxesOrigin(movingOrigin[0], movingOrigin[1], movingOrigin[2]);
		m_channels[channelID]->updateReslicer();
		update();
		QCoreApplication::processEvents();
		// windowToImage needs to be created every loop iteration, otherwise it will cache its output (despite explicitly called SetInput and Update)
		auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
		windowToImage->SetInput(m_renWin);
		windowToImage->ReadFrontBufferOff();
		windowToImage->Update();
		auto img = windowToImage->GetOutput();
		p.emitProgress((slice - sliceFrom) * 100.0 / (sliceTo - sliceFrom));

		QString newFileName(QString("%1%2.%3").arg(baseName).arg(slice).arg(fileInfo.suffix()));
		writeSingleSliceImage(newFileName, img);
	}
	m_renWin->GetInteractor()->Enable();
	LOG(lvlInfo, tr("Image stack saved in folder: %1")
		.arg(fileInfo.absoluteDir().absolutePath()));
	if (aborter.isAborted())
	{
		LOG(lvlInfo, "Note that since you aborted saving, the stack is probably not complete!");
	}
}

void iASlicerImpl::updatePositionMarkerExtent()
{
	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel || !m_decorations)
	{
		return;
	}
	// TODO: how to choose spacing? currently fixed from first image? should be relative to voxels somehow...
	auto imageData = m_channels[channelID]->input();
	m_posMarker.source->SetXLength(m_positionMarkerSize * imageData->GetSpacing()[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)]);
	m_posMarker.source->SetYLength(m_positionMarkerSize * imageData->GetSpacing()[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	m_posMarker.source->SetZLength(0);
}

void iASlicerImpl::setPositionMarkerSize(int size)
{
	m_positionMarkerSize = size;
	updatePositionMarkerExtent();
}

QColor iASlicerImpl::activeBGColor() const
{
	if (m_backgroundColor.isValid())
	{
		return m_backgroundColor;
	}
	switch (m_mode)
	{
	default:
	case iASlicerMode::YZ: return QColor(51, 51, 51);
	case iASlicerMode::XY: return QColor(77, 77, 77);
	case iASlicerMode::XZ: return QColor(153, 153, 153);
	}
}

void iASlicerImpl::updateBackground()
{
	QColor c = activeBGColor();
	m_ren->SetBackground(c.redF(), c.greenF(), c.blueF());
}

void iASlicerImpl::setBackground(QColor color)
{
	m_backgroundColor = color;
	if (m_magicLens)
	{
		m_magicLens->setBackgroundColor(activeBGColor());
	}
	updateBackground();
}

void iASlicerImpl::execute(vtkObject * /*caller*/, unsigned long eventId, void * /*callData*/)
{
	if (eventId == vtkCommand::LeftButtonPressEvent)
	{
		m_leftMouseDrag = true;
		emit clicked();
	}
	if (eventId == vtkCommand::MouseWheelForwardEvent ||
		eventId == vtkCommand::MouseWheelBackwardEvent)
	{
		emit userInteraction();
	}
	updatePosition();
	switch (eventId)
	{
	case vtkCommand::LeftButtonPressEvent:
	{
		emit leftClicked(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
		emit userInteraction();
		break;
	}
	case vtkCommand::LeftButtonReleaseEvent:
	{
		m_leftMouseDrag = false;
		emit leftReleased(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
		emit userInteraction();
		break;
	}
	case vtkCommand::RightButtonPressEvent:
	{
		emit rightClicked(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
		break;
	}
	case vtkCommand::MouseMoveEvent:
	{
		//LOG(lvlInfo, "iASlicerImpl::execute vtkCommand::MouseMoveEvent");
		if (m_cursorSet && cursor() != mouseCursor() && !m_magicLens->isEnabled())
		{
			setCursor(mouseCursor());
		}
		if (m_decorations)
		{
			m_posMarker.actor->SetVisibility(false);
			printVoxelInformation();
		}
		if (m_leftMouseDrag)
		{
			emit leftDragged(m_globalPt[0], m_globalPt[1], m_globalPt[2]);
		}
		emit mouseMoved(m_globalPt[0], m_globalPt[1], m_globalPt[2], m_mode);
		emit userInteraction();
		break;
	}
	case vtkCommand::KeyPressEvent:
		if (m_decorations)
		{
			executeKeyPressEvent();
		}
		break;
	default:
		break;
	}
}

void iASlicerImpl::updatePosition()
{
	// get slicer event position:
	int const* epos = m_renWin->GetInteractor()->GetEventPosition();
	screenPixelPosToImgPos(epos, m_slicerPt, m_globalPt);
}

void iASlicerImpl::screenPixelPosToImgPos(int const pos[2], double * slicerPos, double* globalPos)
{
	if (pos[0] < 0 || pos[1] < 0)
	{
		LOG(lvlDebug, QString("Invalid pos (%1, %2): both values must not be < 0!").arg(pos[0]).arg(pos[1]));
		return;
	}
	m_pointPicker->Pick(pos[0], pos[1], 0, m_ren); // z is always zero
	m_pointPicker->GetPickPosition(slicerPos);     // get position in local slicer scene/world coordinates
	slicerPos[2] = 0;                              // for some reason, sometimes slicerPos[2] is not zero here (but it should be); so make sure it stays 0
	// compute global point:
	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel)
	{
		std::fill(globalPos, globalPos + 3, 0);
		return;
	}
	double point[4] = {slicerPos[0], slicerPos[1], slicerPos[2], 1};
	auto reslicer = m_channels[channelID]->reslicer();
	vtkMatrix4x4 *resliceAxes = vtkMatrix4x4::New();
	resliceAxes->DeepCopy(reslicer->GetResliceAxes());
	resliceAxes->MultiplyPoint(point, globalPos);
	resliceAxes->Delete();
}

QPoint iASlicerImpl::slicerPosToImgPixelCoords(int channelID, double const slicerPt[3])
{
	double const* slicerSpacing = m_channels[channelID]->output()->GetSpacing();
	double const* slicerBounds = m_channels[channelID]->output()->GetBounds();
	double dcX = (slicerPt[0] - slicerBounds[0]) / slicerSpacing[0] + 0.5;
	double dcY = (slicerPt[1] - slicerBounds[2]) / slicerSpacing[1] + 0.5;
	return QPoint(static_cast<int>(std::floor(dcX)), static_cast<int>(std::floor(dcY)));
}

namespace
{
	const int MaxNameLength = 15;

	QString filePixel(iASlicer* slicer, int const * coord, int slicerXAxisIdx, int slicerYAxisIdx)
	{
		vtkImageData* sliceImg = slicer->channel(0)->output();
		int const * dim = sliceImg->GetDimensions();
		bool inRange =
			coord[slicerXAxisIdx] >= 0     && coord[slicerYAxisIdx] >= 0 &&
			coord[slicerXAxisIdx] < dim[0] && coord[slicerYAxisIdx] < dim[1];
		double tmpPix = (inRange) ?
			sliceImg->GetScalarComponentAsDouble(coord[slicerXAxisIdx], coord[slicerYAxisIdx], 0, 0): 0;
		QString file = slicer->channel(0)->name();
		return QString("%1: %2 [%3 %4 %5]\n")
			.arg(padOrTruncate(file, MaxNameLength))
			.arg(inRange ? QString::number(tmpPix) : "exceeds img. dim.")
			.arg(coord[0]).arg(coord[1]).arg(coord[2]);
	}

	const double FisheyeMinRadius = 4.0;
	const double FisheyeMaxRadius = 220.0;

	double fisheyeMinInnerRadius(double radius) { return std::max(1, static_cast<int>((radius - 1) * 0.7)); }
}

void iASlicerImpl::printVoxelInformation()
{
	if (!m_decorations || !m_settings[iASlicerImpl::ShowTooltip].toBool() || m_channels.isEmpty() || m_magicLens->isEnabled())
	{
		return;
	}
	bool infoAvailable = false;
	QString strDetails;
	if (m_interactorStyle->interactionMode() != iASlicerInteractorStyle::imWindowLevelAdjust || !m_interactorStyle->leftButtonDown())
	{
		strDetails = QString("%1: %2, %3, %4\n").arg(padOrTruncate("Position", MaxNameLength))
			.arg(m_globalPt[0]).arg(m_globalPt[1]).arg(m_globalPt[2]);
	}
	for (auto channelID: m_channels.keys())
	{
		if (!m_channels[channelID]->isEnabled())
		{
			continue;
		}

		if (m_interactorStyle->interactionMode() == iASlicerInteractorStyle::imWindowLevelAdjust && m_interactorStyle->leftButtonDown())
		{
			infoAvailable = true;
			strDetails += QString("%1: window: %2; level: %3")
				.arg(padOrTruncate(m_channels[channelID]->name(), MaxNameLength))
				.arg(m_channels[channelID]->imageActor()->GetProperty()->GetColorWindow())
				.arg(m_channels[channelID]->imageActor()->GetProperty()->GetColorLevel());
		}
		else
		{
			int const* slicerExtent = m_channels[channelID]->output()->GetExtent();
			auto pixelPos = slicerPosToImgPixelCoords(channelID, m_slicerPt);

			// check image extent; if outside ==> default output
			if (pixelPos.x() < slicerExtent[0] || pixelPos.x() > slicerExtent[1] || pixelPos.y() < slicerExtent[2] ||
				pixelPos.y() > slicerExtent[3])
			{
				continue;
			}
			QString valueStr;
			for (int i = 0; i < m_channels[channelID]->input()->GetNumberOfScalarComponents(); i++)
			{
				// TODO:
				//   - consider slab thickness / print slab projection result
				double value = m_channels[channelID]->output()->GetScalarComponentAsDouble(pixelPos.x(), pixelPos.y(), 0, i);
				if (i > 0)
				{
					valueStr += " ";
				}
				valueStr += QString::number(value);
			}
			auto coord = mapWorldCoordsToIndex(m_channels[channelID]->input(), m_globalPt);
			infoAvailable = true;
			strDetails += QString("%1: %2 [%3 %4 %5]")
				.arg(padOrTruncate(m_channels[channelID]->name(), MaxNameLength))
				.arg(valueStr)
				.arg(coord.x()).arg(coord.y()).arg(coord.z());

		}
		strDetails += "\n";
	}
	if (m_linkedMdiChild)
	{
		QList<iAMdiChild*> mdiwindows = iAMainWindow::get()->mdiChildList();
		for (int i = 0; i < mdiwindows.size(); i++)
		{
			iAMdiChild *tmpChild = mdiwindows.at(i);
			if (m_linkedMdiChild == tmpChild || !tmpChild->firstImageData())
			{
				continue;
			}
			double* const tmpSpacing = tmpChild->firstImageData()->GetSpacing();
			int tmpCoord[3];
			for (int c = 0; c < 3; ++c)
			{
				tmpCoord[c] = static_cast<int>(m_globalPt[c] / tmpSpacing[c]);
			}
			int slicerXAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X),
				slicerYAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y),
				slicerZAxisIdx = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
			// TODO: check if coords inside other window's image(s)?
			dynamic_cast<iASlicerImpl*>(tmpChild->slicer(m_mode))->setPositionMarkerCenter(m_globalPt[slicerXAxisIdx], m_globalPt[slicerYAxisIdx], m_globalPt[slicerZAxisIdx]);
			tmpChild->slicer(m_mode)->setSliceNumber(tmpCoord[slicerZAxisIdx]);
			tmpChild->slicer(m_mode)->update();
			infoAvailable = true;
			strDetails += filePixel(tmpChild->slicer(m_mode), tmpCoord, slicerXAxisIdx, slicerYAxisIdx);
			tmpChild->update();
		}
	}

	// if requested calculate distance and show actor
	if (m_measureLine.actor->GetVisibility())
	{
		double distance = std::sqrt(std::pow((m_startMeasurePoint[0] - m_slicerPt[0]), 2) +
			std::pow((m_startMeasurePoint[1] - m_slicerPt[1]), 2));
		m_measureLine.source->SetPoint2(m_slicerPt[0], m_slicerPt[1], 0.0);
		m_measureDisk.source->SetOuterRadius(distance);
		m_measureDisk.source->SetInnerRadius(distance);
		m_measureDisk.source->Update();
		infoAvailable = true;
		strDetails += QString("%1: %2\n").arg(padOrTruncate("Distance", MaxNameLength)).arg(distance);
	}
	if (infoAvailable)
	{
		int x, y;
		const int TextOfs = 10;
		m_renWin->GetInteractor()->GetEventPosition(x, y);
		m_textInfo->setPosition(x + TextOfs, y + TextOfs);
		m_textInfo->setText(strDetails.toStdString().c_str());
		m_posMarker.mapper->Update();
	}
	bool visibilityChange = infoAvailable != m_textInfo->isShown();
	if (visibilityChange)
	{
		m_textInfo->show(infoAvailable);
	}
	if (infoAvailable || visibilityChange)
	{
		m_renWin->GetInteractor()->Render();
	}
}

void iASlicerImpl::setMeasurementVisibility(bool visible)
{
	m_measureLine.actor->SetVisibility(visible);
	m_measureDisk.actor->SetVisibility(visible);
	printVoxelInformation();
}

void iASlicerImpl::executeKeyPressEvent()
{
	switch (m_renWin->GetInteractor()->GetKeyCode())
	{
	case 'm':
		if (m_decorations)
		{
			m_startMeasurePoint[0] = m_slicerPt[0];
			m_startMeasurePoint[1] = m_slicerPt[1];
			// does not work reliably (often snaps to positions not the highest gradient close to the current position)
			// and causes access to pixels outside of the image:
			//snapToHighGradient(m_startMeasurePoint[0], m_startMeasurePoint[1]);
			bool newVisibility = !m_measureLine.actor->GetVisibility();
			if (newVisibility)
			{
				m_measureLine.source->SetPoint1(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
				m_measureDisk.actor->SetPosition(m_startMeasurePoint[0], m_startMeasurePoint[1], 0.0);
			}
			setMeasurementVisibility(newVisibility);
		}
		break;
	case 27: //ESCAPE
		if (m_decorations)
		{
			setMeasurementVisibility(false);
		}
		break;
	}
}

/*
void iASlicerImpl::snapToHighGradient(double &x, double &y)
{
	double range[2];
	imageData->GetScalarRange(range);
	double gradThresh = range[1] * 0.05;

	double * imageSpacing = imageData->GetSpacing();
	double * imageBounds = imageData->GetBounds();

	int xInd = mapSliceToGlobalAxis(m_mode, iAAxisIndex::X);
	int yInd = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y);
	double coord1 = (int)(x/imageSpacing[xInd]);
	double coord2 = (int)(y/imageSpacing[yInd]);
	double dataCoord1 = (int)((x-imageBounds[xInd*2])/imageSpacing[xInd]);
	double dataCoord2 = (int)((y-imageBounds[yInd*2])/imageSpacing[yInd]);

	std::list<double>H_x;
	std::list<double>H_y;
	std::list<double>H_grad;

	double H_maxGradMag = 0;
	double H_maxcoord[2];H_maxcoord[0] = 0; H_maxcoord[1] = 0;
	double cursorposition[2]; cursorposition[0] = coord1; cursorposition[1] = coord2;
	//move horizontally
	for (int i = -2; i <= 2; i++)
	{
		//if ( i != 0)
		{
			double center[2], right[2], left[2], top[2], bottom[2];
			center[0] = dataCoord1 + i; center[1] = dataCoord2;
			left[0] = dataCoord1 + i - 1; left[1] = dataCoord2;
			right[0] = dataCoord1 + i + 1; right[1] = dataCoord2;
			top[0] = dataCoord1 + i; top[1] = dataCoord2+1;
			bottom[0] = dataCoord1 + i; bottom[1] = dataCoord2 - 1;

			double left_pix = imageData->GetScalarComponentAsDouble(left[0],left[1],0,0);
			double right_pix = imageData->GetScalarComponentAsDouble(right[0],right[1],0,0);

			double top_pix = imageData->GetScalarComponentAsDouble(top[0],top[1],0,0);
			double bottom_pix = imageData->GetScalarComponentAsDouble(bottom[0],bottom[1],0,0);

			double derivativeX = fabs(right_pix - left_pix);
			double derivativeY = fabs(top_pix - bottom_pix);

			double gradmag = std::sqrt ( std::pow(derivativeX,2) + std::pow(derivativeY,2) );

			H_x.push_back(center[0]);
			H_y.push_back(center[1]);
			H_grad.push_back(gradmag);

			//check whether the calculated gradient magnitude => maxGradientMagnitude
			if ( gradmag >= H_maxGradMag )
			{
				//check whether the gradient magnitude = to maxgradient magnitude
				//if yes calculate the distance between the cursorposition and center called newdist
				// and calculate the distance between the cursorposition and HMaxCoord called the maxdist
				//if NO take the current center position as the HMaxCoord
				if ( gradmag == H_maxGradMag )
				{
					//calculate the distance
					double newdist = std::sqrt (std::pow( (cursorposition[0]-center[0]),2) + std::pow( (cursorposition[1]-center[1]),2));
					double maxdist = std::sqrt (std::pow( (cursorposition[0]-H_maxcoord[0]),2) + std::pow( (cursorposition[1]-H_maxcoord[1]),2));
					//if newdist is < than the maxdist (meaning the current center position is closer to the cursor position
					//replace the hMaxCoord with the current center position
					if ( newdist < maxdist )
					{
						H_maxcoord[0] = center[0];
						H_maxcoord[1] = center[1];
					}//if
				}//if else
				else
				{
					H_maxGradMag = gradmag;
					H_maxcoord[0] = center[0];
					H_maxcoord[1] = center[1];
				}//if else

			}//if
		}//if
	}//for

	std::list<double>V_x;
	std::list<double>V_y;
	std::list<double>V_grad;

	double V_maxGradMag = 0;
	double V_maxcoord[2]; V_maxcoord[0] = 0; V_maxcoord[1] = 0;
	//move vertically
	for (int i = -2; i <= 2; i++)
	{
		if ( i != 0 )
		{
			double center[2], right[2], left[2], top[2], bottom[2];
			center[0] = dataCoord1; center[1] = dataCoord2+i;
			left[0] = dataCoord1-1; left[1] = dataCoord2+i;
			right[0] = dataCoord1+1; right[1] = dataCoord2+i;
			top[0] = dataCoord1; top[1] = dataCoord2+i+1;
			bottom[0] = dataCoord1; bottom[1] = dataCoord2+i-1;

			double left_pix = imageData->GetScalarComponentAsDouble(left[0],left[1],0,0);
			double right_pix = imageData->GetScalarComponentAsDouble(right[0],right[1],0,0);

			double top_pix = imageData->GetScalarComponentAsDouble(top[0],top[1],0,0);
			double bottom_pix = imageData->GetScalarComponentAsDouble(bottom[0],bottom[1],0,0);

			double derivativeX = fabs(right_pix - left_pix);
			double derivativeY = fabs(top_pix - bottom_pix);

			double gradmag = std::sqrt ( std::pow(derivativeX,2) + std::pow(derivativeY,2) );

			V_x.push_back(center[0]);
			V_y.push_back(center[1]);
			V_grad.push_back(gradmag);

			//check whether the calculated gradient magnitude => maxGradientMagnitude
			if ( gradmag >= V_maxGradMag )
			{
				//check whether the gradient magnitude = to maxgradient magnitude
				//if yes calculate the distance between the cursorposition and center called newdist
				// and calculate the distance between the cursorposition and HMaxCoord called the maxdist
				//if NO take the current center position as the HMaxCoord
				if ( gradmag == V_maxGradMag )
				{
					//calculate the distance
					double newdist = std::sqrt (std::pow( (cursorposition[0]-center[0]),2) + std::pow( (cursorposition[1]-center[1]),2));
					double maxdist = std::sqrt (std::pow( (cursorposition[0]-V_maxcoord[0]),2) + std::pow( (cursorposition[1]-V_maxcoord[1]),2));
					//if newdist is < than the maxdist (meaning the current center position is closer to the cursor position
					//replace the hMaxCoord with the current center position
					if ( newdist < maxdist )
					{
						V_maxcoord[0] = center[0];
						V_maxcoord[1] = center[1];
					}//if
				}//if else
				else
				{
					V_maxGradMag = gradmag;
					V_maxcoord[0] = center[0];
					V_maxcoord[1] = center[1];
				}//if else
			}//if
		}//if
	}//for

	//checking whether the V_maxGradMag and H_maxGradMag is higher than the gradient threshold
	bool v_bool = false, h_bool = false;
	if ( V_maxGradMag >= gradThresh )
		v_bool = true;
	if (H_maxGradMag >= gradThresh)
		h_bool = true;

	//selection of V_maxGradMag or H_maxGradMag as new point
	int pointselectionkey;
	if ( v_bool == false && h_bool == true )
		pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
	else if ( v_bool == true && h_bool == false )
		pointselectionkey = 2; //new point is in horizontal direction V_maxcoord
	else if ( v_bool == true && h_bool == true )
	{
		//pointselectionkey = 3; //new point is shortest distance between V_maxcoord,currentposition and H_maxcoord ,currentposition
		double Hdist = std::sqrt (std::pow( (cursorposition[0]-H_maxcoord[0]),2) + std::pow( (cursorposition[1]-H_maxcoord[1]),2));
		double Vdist = std::sqrt (std::pow( (cursorposition[0]-V_maxcoord[0]),2) + std::pow( (cursorposition[1]-V_maxcoord[1]),2));
		if ( Hdist < Vdist )
			pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
		else if ( Hdist > Vdist )
			pointselectionkey = 2; //new point is in horizontal direction V_maxcoord
		else
			pointselectionkey = 1; //new point is in horizontal direction H_maxcoord
	}
	else
	{
		// do nothing as v_bool and h_bool are false which means the gradient difference in both direction is not >= to grad threshold
		// and the cursor position is the final position meaning no change in the position
	}
	switch(pointselectionkey)
	{
	case 1:
		x = H_maxcoord[0]*imageSpacing[xInd];
		y = H_maxcoord[1]*imageSpacing[yInd];
		break;
	case 2:
		x = V_maxcoord[0]*imageSpacing[xInd];
		y = V_maxcoord[1]*imageSpacing[yInd];
		break;
	}
}
*/

void iASlicerImpl::setShowTooltip(bool isVisible)
{
	m_settings[iASlicerImpl::ShowTooltip] = isVisible;
	m_textInfo->show(isVisible);
	if (isVisible)
	{
		printVoxelInformation();
	}
	m_renWin->GetInteractor()->Render();
}

void iASlicerImpl::enableChannel(uint id, bool enabled)
{
	channel(id)->setEnabled(m_ren, enabled);
}

void iASlicerImpl::updateChannel(uint id, iAChannelData const & chData)
{
	channel(id)->update(chData);
}

void iASlicerImpl::setResliceChannelAxesOrigin(uint id, double x, double y, double z)
{
	channel(id)->setResliceAxesOrigin(x, y, z);
}

void iASlicerImpl::setChannelOpacity(uint id, double opacity)
{
	channel(id)->setActorOpacity(opacity);
}

void iASlicerImpl::setSlabThickness(int thickness)
{
	m_slabThickness = thickness;
	for (auto ch : m_channels)
	{
		ch->setSlabNumberOfSlices(thickness);
	}
	update();
}

void iASlicerImpl::setSlabCompositeMode(int slabCompositeMode)
{
	m_slabCompositeMode = slabCompositeMode;
	for (auto ch : m_channels)
	{
		ch->setSlabMode(slabCompositeMode);
	}
	update();
}

uint iASlicerImpl::firstVisibleChannel() const
{
	for (auto key : m_channels.keys())
	{
		if (m_channels[key]->isEnabled())
		{
			return key;
		}
	}
	return NotExistingChannel;
}

std::shared_ptr<iAChannelSlicerData> iASlicerImpl::createChannel(uint id, iAChannelData const & chData)
{
	if (m_channels.contains(id))
	{
		throw std::runtime_error(QString("iASlicer: Channel with ID %1 already exists!").arg(id).toStdString());
	}

	std::shared_ptr<iAChannelSlicerData> newData(new iAChannelSlicerData(chData, m_mode));
	newData->setInterpolate(m_settings[iASlicerImpl::LinearInterpolation].toBool());
	newData->setSlabNumberOfSlices(m_slabThickness);
	newData->setSlabMode(m_slabCompositeMode);
	newData->setTransform(m_transform);
	m_channels.insert(id, newData);
	return newData;
}

iAChannelSlicerData * iASlicerImpl::channel(uint id)
{
	assert(m_channels.contains(id));
	if (!m_channels.contains(id))
	{
		return nullptr;
	}
	return m_channels.find(id)->get();
}

void iASlicerImpl::removeChannel(uint id)
{
	if (channel(id)->isEnabled())
	{
		enableChannel(id, false);
	}
	m_channels.remove(id);
	if (m_sliceNumberChannel == id)
	{
		m_sliceNumberChannel = NotExistingChannel;
		if (!m_channels.isEmpty())
		{
			setSlicerRange(m_channels.keys()[0]);
		}
	}
}

bool iASlicerImpl::hasChannel(uint id) const
{
	return m_channels.contains(id);
}

void iASlicerImpl::setCamera(vtkCamera* cam, bool camOwner /*=true*/)
{
	m_ren->SetActiveCamera(cam);
	if (m_camera && m_cameraOwner)
	{
		m_camera->Delete();
	}
	m_cameraOwner = camOwner;
	m_camera = cam;
}

void iASlicerImpl::resetCamera()
{
	m_ren->ResetCamera();
}

void iASlicerImpl::updateChannelMappers()
{
	for (auto chData: m_channels)
	{
		chData->updateLUT();
		chData->updateMapper();
	}
}

void iASlicerImpl::setAngle(int mode, double angle)
{
	if (mode == m_mode)
	{
		return;
	}
	m_angle[mode] = angle;
	update();
}

void iASlicerImpl::rotateSlice(double angle)
{
	m_angle[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z)] = angle;

	auto channelID = firstVisibleChannel();
	if (channelID == NotExistingChannel)
	{
		return;
	}

	double center[3];

	// TODO: allow selecting center for rotation? current: always use first image!
	auto imageData = m_channels[channelID]->input();
	double* spacing = imageData->GetSpacing();
	int* ext = imageData->GetExtent();

	center[0] = (ext[1] - ext[0]) / 2 * spacing[0];
	center[1] = (ext[3] - ext[2]) / 2 * spacing[1];
	center[2] = (ext[5] - ext[4]) / 2 * spacing[2];

	vtkTransform *t1 = vtkTransform::New();
	t1->Translate(-center[0], -center[1], -center[2]);

	vtkTransform *t2 = vtkTransform::New();
	t2->RotateWXYZ(m_angle[0], 1, 0, 0);
	t2->RotateWXYZ(m_angle[1], 0, 1, 0);
	t2->RotateWXYZ(m_angle[2], 0, 0, 1);

	vtkTransform *t3 = vtkTransform::New();
	t3->Translate(center[0], center[1], center[2]);

	m_transform->Identity();
	m_transform->PostMultiply();
	m_transform->Concatenate(t1);
	m_transform->Concatenate(t2);
	m_transform->Concatenate(t3);
	for (auto ch : m_channels)
	{
		ch->setTransform(m_transform);
	}

	update();
	emit sliceRotated(m_mode, angle);
}

/*
void iASlicerImpl::switchContourSourceToChannel(uint id )
{
	if (!m_decorations)
		return;

	cFilter->SetInputConnection( getChannel(id)->reslicer->GetOutputPort() );
}
*/

void iASlicerImpl::setContours(int numberOfContours, double contourMin, double contourMax)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setContours(numberOfContours, contourMin, contourMax);
	}
}

void iASlicerImpl::setContours(int numberOfContours, double const * contourValues)
{
	if (!m_decorations)
	{
		return;
	}
	for (auto ch : m_channels)
	{
		ch->setContours(numberOfContours, contourValues);
	}
}

void iASlicerImpl::setMouseCursor(QString const & s)
{
	QString color = s.section(' ', -1);
	if (color != "default")
	{
		QString shape = s.section(' ', 0, 0);
		QPixmap pm;
		if (shape == "Crosshair")
		{
			pm = QPixmap(":/images/" + s.section(' ', 0, 1) + ".png");
		}
		QPixmap cpm(pm.size());
		cpm.fill(color);
		cpm.setMask(pm.createMaskFromColor(Qt::transparent));
		m_mouseCursor = QCursor(cpm);
	}
	else
	{
		m_mouseCursor = QCursor(Qt::CrossCursor);
	}
	setCursor(mouseCursor());
	m_cursorSet = true;
}

void iASlicerImpl::setScalarBarTF(vtkScalarsToColors* ctf)
{
	if (!m_scalarBarWidget)
	{
		return;
	}
	m_scalarBarWidget->GetScalarBarActor()->SetLookupTable(ctf);
	m_scalarBarWidget->SetEnabled(ctf != nullptr);
}

QCursor iASlicerImpl::mouseCursor()
{
	return m_mouseCursor;
}

int iASlicerImpl::sliceNumber() const
{
	return m_sliceNumber;
}

double iASlicerImpl::slicePosition() const
{
	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	double const* origin = m_channels[m_sliceNumberChannel]->input()->GetOrigin();
	double const* spacing = m_channels[m_sliceNumberChannel]->input()->GetSpacing();
	return origin[sliceAxis] + (m_sliceNumber * spacing[sliceAxis]);
}

double iASlicerImpl::sliceThickness() const
{
	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	double const* spacing = m_channels[m_sliceNumberChannel]->input()->GetSpacing();
	return spacing[sliceAxis];
}

std::pair<double, double> iASlicerImpl::sliceRange() const
{
	auto img = m_channels[m_sliceNumberChannel]->input();
	int sliceAxis = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
	double sliceOrigin = img->GetOrigin()[sliceAxis];
	double sliceSpacing = img->GetSpacing()[sliceAxis];
	auto const ext = img->GetExtent();
	int minIdx = ext[sliceAxis * 2];
	int maxIdx = ext[sliceAxis * 2 + 1];
	return std::make_pair(sliceOrigin + (minIdx * sliceSpacing), sliceOrigin + (maxIdx * sliceSpacing));
}

void iASlicerImpl::keyPressEvent(QKeyEvent *event)
{
	// TODO: merge with iASlicerImpl::execute, switch branch vtkCommand::KeyPressEvent ?
	if (!hasChannel(0))
	{
		return;
	}
	if (event->key() == Qt::Key_R)
	{
		m_ren->ResetCamera();
	}
	if (event->key() == Qt::Key_O)
	{
		fisheyeLensToggled(!m_fisheyeLensActivated);
	}

	// magnify and unmagnify fisheye lens and distortion radius
	if (m_fisheyeLensActivated && (
		event->key() == Qt::Key_Minus ||
		event->key() == Qt::Key_Plus
#if defined(VK_OEM_PLUS)
		|| event->nativeVirtualKey() == VK_OEM_PLUS // "native..." - workaround required for Ctrl+ "+" with non-numpad "+" key
#endif
		))
	{
		m_ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);

		// TODO: fisheye lens on all channels???
		auto reslicer = channel(0)->reslicer();
		double oldInnerRadius = m_innerFisheyeRadius,
			oldRadius = m_fisheyeRadius;
		if (event->modifiers().testFlag(Qt::ControlModifier))
		{
			// change inner radius (~ zoom level) alone:
			double ofs = ((m_fisheyeRadius <= 20.0) ? 1 : 2)    // how much to change it
				* ((event->key() != Qt::Key_Minus) ? -1 : 1);   // which direction (!= instead of ==, as below, is intended!)
			m_innerFisheyeRadius = clamp(fisheyeMinInnerRadius(m_fisheyeRadius), m_fisheyeRadius, m_innerFisheyeRadius + ofs);
		}
		else
		{
			// change radius of displayed fisheye lens, which requires changing inner radius as well:
			double ofs = ((m_fisheyeRadius <= 20) ? 1 : 10)     // how much to change it
				* ((event->key() == Qt::Key_Minus) ? -1 : 1);   // which direction
			m_fisheyeRadius = clamp(FisheyeMinRadius, FisheyeMaxRadius, m_fisheyeRadius + ofs);
			if (m_fisheyeRadius != oldRadius)
			{
				m_innerFisheyeRadius = clamp(fisheyeMinInnerRadius(m_fisheyeRadius), m_fisheyeRadius, m_innerFisheyeRadius + ofs);
			}
		}
		if (oldRadius != m_fisheyeRadius || oldInnerRadius != m_innerFisheyeRadius) // only update if something changed
		{
			updateFisheyeTransform(m_ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
		}
	}
	iAVtkWidget::keyPressEvent(event);
}

void iASlicerImpl::mousePressEvent(QMouseEvent *event)
{
	// TODO: merge with iASlicerImpl::execute, switch branch vtkCommand::LeftButtonPressEvent ?
	if (m_channels.empty())
	{
		iAVtkWidget::mousePressEvent(event);
		return;
	}
	if (m_isSliceProfEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)  // if slice profile is enabled, do all the necessary operations
	{
		updateRawProfile(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}

	if (m_profileHandlesEnabled
		&& (event->modifiers() == Qt::NoModifier)
		&& event->button() == Qt::LeftButton)  // if profile handles are shown, do all the necessary operations
	{
		m_profileHandles->findSelectedPointIdx(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)], m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}
	iAVtkWidget::mousePressEvent(event);
}

void iASlicerImpl::mouseMoveEvent(QMouseEvent *event)
{
	// TODO: merge with iASlicerImpl::execute, switch branch vtkCommand::MouseMoveEvent ?
	iAVtkWidget::mouseMoveEvent(event);

	if (!hasChannel(0)) // nothing to do if no data
	{
		return;
	}

	if (m_fisheyeLensActivated)
	{
		// TODO: fisheye lens on all channels???
		auto reslicer = channel(0)->reslicer();
		vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
		ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);
		updateFisheyeTransform(ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
	}

	if (!event->modifiers().testFlag(Qt::ShiftModifier))
	{
		updateMagicLens();
	}
	if (m_isSliceProfEnabled && (event->modifiers() == Qt::NoModifier) && (event->buttons() & Qt::LeftButton))
	{
		updateRawProfile(m_globalPt[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]);
	}
	if (m_profileHandlesEnabled && m_decorations && hasChannel(0))
	{
		int profilePointIdx = m_profileHandles->pointIdx();
		if (event->modifiers() == Qt::NoModifier && profilePointIdx >= 0 && event->buttons().testFlag(Qt::LeftButton))
		{
			double const * ptPos = m_profileHandles->position(profilePointIdx);
			const int zind = mapSliceToGlobalAxis(m_mode, iAAxisIndex::Z);
			double globalPos[3];
			std::copy(m_globalPt, m_globalPt + 3, globalPos);
			globalPos[zind] = ptPos[zind];
			// TODO NEWIO: clamp to range of selected channel / all channels?
			auto imageData = channel(0)->input();
			double* spacing = imageData->GetSpacing();
			double* origin = imageData->GetOrigin();
			int* dimensions = imageData->GetDimensions();
			for (int i = 0; i < 3; ++i)
			{
				globalPos[i] = clamp(origin[i], origin[i] + (dimensions[i] - 1) * spacing[i], globalPos[i]);
			}
			if (ptPos[0] != globalPos[0] || ptPos[1] != globalPos[1] || ptPos[2] != globalPos[2])
			{
				setProfilePointInternal(profilePointIdx, globalPos);
				emit profilePointChanged(profilePointIdx, globalPos);
			}
			else
			{
				//LOG(lvlDebug, "No profile position change!");
			}
		}
	}
}

void iASlicerImpl::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit dblClicked();
	}
}

void iASlicerImpl::contextMenuEvent(QContextMenuEvent *event)
{
	m_actionToggleWindowLevelAdjust->setChecked(m_interactorStyle->interactionMode() == iASlicerInteractorStyle::imWindowLevelAdjust);
	m_actionToggleRegionTransferFunction->setChecked(m_interactorStyle->interactionMode() == iASlicerInteractorStyle::imRegionSelect);
	m_actionToggleNormalInteraction->setChecked(m_interactorStyle->interactionMode() == iASlicerInteractorStyle::imNormal);
	m_actionShowTooltip->setChecked(m_settings[iASlicerImpl::ShowTooltip].toBool());
	m_actionFisheyeLens->setChecked(m_fisheyeLensActivated);
	if (m_magicLens)
	{
		m_actionMagicLens->setChecked(m_magicLens->isEnabled());
		m_actionMagicLensCentered->setVisible(m_magicLens->isEnabled());
		m_actionMagicLensOffset->setVisible(m_magicLens->isEnabled());
	}
	m_contextMenu->exec(event->globalPos());
}

void iASlicerImpl::updateRawProfile(double posY)
{
	if (!hasChannel(0))
	{
		return;
	}
	// TODO: slice "raw" profile on selected/current channel
	vtkImageData * reslicedImgData = channel(0)->output();
	if (!m_sliceProfile->updatePosition(posY, reslicedImgData))
	{
		return;
	}
	renderWindow()->GetInteractor()->Render();
}

void iASlicerImpl::setProfilePoint(int pointIdx, double const * globalPos)
{
	if (!m_decorations || !hasChannel(0))
	{
		return;
	}
	setProfilePointInternal(pointIdx, globalPos);
}

void iASlicerImpl::setProfilePointInternal(int pointIdx, double const * globalPos)
{
	double profileCoord2d[2] = {
		globalPos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::X)],
		globalPos[mapSliceToGlobalAxis(m_mode, iAAxisIndex::Y)]
	};
	m_profileHandles->setup(pointIdx, globalPos, profileCoord2d, channel(0)->output());
	renderWindow()->GetInteractor()->Render();
}

void iASlicerImpl::setSliceProfileOn(bool isOn)
{
	m_isSliceProfEnabled = isOn;
	m_sliceProfile->setVisibility(m_isSliceProfEnabled);
	updateRawProfile(channel(0)->output()->GetOrigin()[1]);
}

void iASlicerImpl::setProfileHandlesOn(bool isOn)
{
	if (!m_decorations)
	{
		return;
	}
	m_profileHandlesEnabled = isOn;
	m_profileHandles->setVisibility(m_profileHandlesEnabled);
	renderWindow()->GetInteractor()->Render();
}

void iASlicerImpl::setLinkedMdiChild(iAMdiChild* mdiChild)
{
	m_linkedMdiChild = mdiChild;
}

int iASlicerImpl::globalAxis(int slicerAxis)
{
	return mapSliceToGlobalAxis(m_mode, slicerAxis);
}

QVariantMap const& iASlicerImpl::settings()
{
	return m_settings;
}

void iASlicerImpl::resizeEvent(QResizeEvent * event)
{
	updateMagicLens();
	iAVtkWidget::resizeEvent(event);
}

void iASlicerImpl::wheelEvent(QWheelEvent* event)
{
	event->accept();
	if (event->modifiers().testFlag(Qt::ControlModifier))
	{
		int chg = event->angleDelta().y() / 120.0;
		if (m_magicLens)
		{
			double sizeFactor = 1.1 * (std::abs(chg));
			if (chg < 0)
			{
				sizeFactor = 1 / sizeFactor;
			}
			int newSize = std::max(MinimumMagicLensSize, static_cast<int>(magicLensSize() * sizeFactor));
			setMagicLensSize(newSize);
		}
		emit ctrlMouseWheel(chg);
	}
	else if (event->modifiers().testFlag(Qt::ShiftModifier) && receivers(SIGNAL(shiftMouseWheel(int))) > 0)
	{
		emit shiftMouseWheel(event->angleDelta().y() / 120);
	}
	else if (event->modifiers().testFlag(Qt::AltModifier))
	{
		int chg = event->angleDelta().x() / 120;
		if (m_magicLens)
		{
			setMagicLensOpacity(magicLensOpacity() + (chg * 0.05));
		}
		emit altMouseWheel(chg);
	}
	else if (event->angleDelta().x() != 0 && receivers(SIGNAL(altMouseWheel(int))) > 0)
	{
		this->setSliceNumber(event->angleDelta().x() / 120.0 + this->sliceNumber());
	}
	else
	{
		iAVtkWidget::wheelEvent(event);
	}
	updateMagicLens();
}

void iASlicerImpl::menuCenteredMagicLens()
{
	if (!m_magicLens)
	{
		return;
	}
	m_magicLens->setViewMode(iAMagicLens::CENTERED);
	updateMagicLens();
}

void iASlicerImpl::menuOffsetMagicLens()
{
	if (!m_magicLens)
	{
		return;
	}
	m_magicLens->setViewMode(iAMagicLens::OFFSET);
	updateMagicLens();
}

void iASlicerImpl::toggleLinearInterpolation()
{
	setLinearInterpolation(m_actionLinearInterpolation->isChecked());
}

void iASlicerImpl::toggleInteractionMode(QAction * )
{
	m_interactorStyle->setInteractionMode(
		m_actionToggleWindowLevelAdjust->isChecked() ?
			iASlicerInteractorStyle::imWindowLevelAdjust :
			m_actionToggleRegionTransferFunction->isChecked() ?
				iASlicerInteractorStyle::imRegionSelect :
				iASlicerInteractorStyle::imNormal);
}

void iASlicerImpl::toggleShowTooltip()
{
	setShowTooltip(!m_settings[iASlicerImpl::ShowTooltip].toBool());
}

void iASlicerImpl::fisheyeLensToggled(bool enabled)
{
	m_fisheyeLensActivated = enabled;
	vtkRenderer* ren = m_renWin->GetRenderers()->GetFirstRenderer();
	// TODO: fisheye lens on all channels???
	auto reslicer = channel(0)->reslicer();
	if (m_fisheyeLensActivated)
	{
		reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());
		ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);

		initializeFisheyeLens(reslicer);

		updateFisheyeTransform(ren->GetWorldPoint(), reslicer, m_fisheyeRadius, m_innerFisheyeRadius);
	}
	else
	{
		reslicer->SetAutoCropOutput(!reslicer->GetAutoCropOutput());

		// Clear outdated circles and actors (not needed for final version)
		for (int i = 0; i < m_circle1ActList.length(); ++i)
		{
			ren->RemoveActor(m_circle1ActList.at(i));
		}
		//circle1List.clear();
		m_circle1ActList.clear();

		for (int i = 0; i < m_circle2ActList.length(); ++i)
		{
			ren->RemoveActor(m_circle2ActList.at(i));
		}
		m_circle2List.clear();
		m_circle2ActList.clear(); //*/

		ren->RemoveActor(m_fisheyeActor);

		// No fisheye transform
		double bounds[6];
		reslicer->GetInformationInput()->GetBounds(bounds);
		m_pointsTarget->SetNumberOfPoints(4);
		m_pointsSource->SetNumberOfPoints(4);
		m_pointsTarget->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
		m_pointsTarget->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
		m_pointsTarget->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
		m_pointsTarget->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right
		m_pointsSource->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
		m_pointsSource->SetPoint(1, bounds[0], bounds[3], 0); //x_min, y_max, top left
		m_pointsSource->SetPoint(2, bounds[1], bounds[3], 0); //x_max, y_max, top right
		m_pointsSource->SetPoint(3, bounds[1], bounds[2], 0); //x_max, y_min, bottom right

		m_fisheyeTransform->SetSourceLandmarks(m_pointsSource);
		m_fisheyeTransform->SetTargetLandmarks(m_pointsTarget);
		reslicer->SetResliceTransform(m_fisheyeTransform);
		update();
	}
}

void iASlicerImpl::initializeFisheyeLens(vtkImageReslice* reslicer)
{
	vtkRenderer * ren = renderWindow()->GetRenderers()->GetFirstRenderer();

	m_fisheyeTransform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
	m_fisheyeTransform->SetBasisToR2LogR();
	m_pointsSource = vtkSmartPointer<vtkPoints>::New();
	m_pointsTarget = vtkSmartPointer<vtkPoints>::New();
	m_pointsSource->SetNumberOfPoints(32);
	m_pointsTarget->SetNumberOfPoints(32);
	reslicer->SetInterpolationModeToLinear(); // added while testing

	m_fisheye = vtkSmartPointer<vtkRegularPolygonSource>::New();
	m_fisheye->SetNumberOfSides(60);
	m_fisheye->GeneratePolygonOff(); // just outlines;
	m_fisheye->SetRadius(50.0);
	m_fisheyeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_fisheyeMapper->SetInputConnection(m_fisheye->GetOutputPort());
	m_fisheyeActor = vtkSmartPointer<vtkActor>::New();
	m_fisheyeActor->GetProperty()->SetColor(1.00000, 0.50196, 0.00000);
	m_fisheyeActor->GetProperty()->SetOpacity(1.0);
	m_fisheyeActor->SetMapper(m_fisheyeMapper);
	ren->AddActor(m_fisheyeActor);

	// Create circle actors (green and red) to show the transform landmarks
	for (int i = 0; i < m_pointsTarget->GetNumberOfPoints(); ++i)
	{
		// Create a sphere and its associated mapper and actor.
		auto circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides(50);
		circle->SetRadius(1.0 * reslicer->GetOutput()->GetSpacing()[0]);

		auto circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection(circle->GetOutputPort());

		auto circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
		circleActor->GetProperty()->SetOpacity(1.0);
		circleActor->SetMapper(circleMapper);
		circleActor->VisibilityOff(); // comment to show landmarks

		m_circle1List.append(circle);
		m_circle1ActList.append(circleActor);
		ren->AddActor(circleActor);
	}

	for (int i = 0; i < m_pointsSource->GetNumberOfPoints(); ++i)
	{
		auto circle = vtkSmartPointer<vtkRegularPolygonSource>::New();
		circle->GeneratePolygonOff(); // Uncomment to generate only the outline of the circle
		circle->SetNumberOfSides(50);
		circle->SetRadius(3.0 * reslicer->GetOutput()->GetSpacing()[0]);

		auto circleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		circleMapper->SetInputConnection(circle->GetOutputPort());

		auto circleActor = vtkSmartPointer<vtkActor>::New();
		circleActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
		circleActor->GetProperty()->SetOpacity(1.0);
		circleActor->SetMapper(circleMapper);
		circleActor->VisibilityOff(); // comment to show landmarks

		m_circle2List.append(circle);
		m_circle2ActList.append(circleActor);
		ren->AddActor(circleActor);
	}
}

void iASlicerImpl::updateFisheyeTransform(double focalPt[3], vtkImageReslice* reslicer, double lensRadius, double innerLensRadius)
{
	vtkImageData * reslicedImgData = reslicer->GetOutput();

	double * spacing = reslicedImgData->GetSpacing();

	double bounds[6];
	reslicer->GetInformationInput()->GetBounds(bounds);

	const vtkIdType FixPoints = 8;
	const vtkIdType NumPoints = 4 * FixPoints;
	m_pointsTarget->SetNumberOfPoints(NumPoints); // already set above!
	m_pointsSource->SetNumberOfPoints(NumPoints);
	int sn = sliceNumber();

	switch (m_mode)
	{
	case iASlicerMode::YZ:
		m_pointsTarget->SetPoint(0, sn * spacing[0], bounds[2], bounds[2]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, sn * spacing[0], bounds[2], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(2, sn * spacing[0], bounds[2], bounds[5]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, sn * spacing[0], 0.5  * bounds[3], bounds[5]);
		m_pointsTarget->SetPoint(4, sn * spacing[0], bounds[3], bounds[5]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, sn * spacing[0], bounds[3], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(6, sn * spacing[0], bounds[3], bounds[2]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, sn * spacing[0], 0.5  * bounds[3], bounds[2]);
		break;
	case iASlicerMode::XZ:
		m_pointsTarget->SetPoint(0, bounds[0], sn * spacing[1], bounds[4]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, bounds[0], sn * spacing[1], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(2, bounds[0], sn * spacing[1], bounds[5]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, 0.5  * bounds[1], sn * spacing[1], bounds[5]);
		m_pointsTarget->SetPoint(4, bounds[1], sn * spacing[1], bounds[5]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, bounds[1], sn * spacing[1], 0.5  * bounds[5]);
		m_pointsTarget->SetPoint(6, bounds[1], sn * spacing[1], bounds[4]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, 0.5  * bounds[1], sn * spacing[1], bounds[4]);
		break;
	case iASlicerMode::XY:
		m_pointsTarget->SetPoint(0, bounds[0], bounds[2], sn * spacing[2]); //x_min, y_min, bottom left // left border points
		m_pointsTarget->SetPoint(1, bounds[0], 0.5  * bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(2, bounds[0], bounds[3], sn * spacing[2]); //x_min, y_max, top left // top border points
		m_pointsTarget->SetPoint(3, 0.5  * bounds[1], bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(4, bounds[1], bounds[3], sn * spacing[2]); //x_max, y_max, top right // right border points
		m_pointsTarget->SetPoint(5, bounds[1], 0.5  * bounds[3], sn * spacing[2]);
		m_pointsTarget->SetPoint(6, bounds[1], bounds[2], sn * spacing[2]); //x_max, y_min, bottom right // bottom border points
		m_pointsTarget->SetPoint(7, 0.5  * bounds[1], bounds[2], sn * spacing[2]);
		break;
	default:
		break;
	}
	for (vtkIdType i = 0; i < FixPoints; ++i)
	{
		m_pointsSource->SetPoint(i, m_pointsTarget->GetPoint(i));
	}
	// outer circle 1
	double fixRadiusX;
	double fixRadiusY;
	for (auto fix = FixPoints; fix < 2*FixPoints; ++fix)
	{
		fixRadiusX = (lensRadius + 15.0)* std::cos(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];
		fixRadiusY = (lensRadius + 15.0)* std::sin(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		default:
			break;
		}
	}
	// outer circle 2
	for (auto fix = 2*FixPoints; fix < 3*FixPoints; ++fix)
	{
		fixRadiusX = (lensRadius + 80.0)* std::cos(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];
		fixRadiusY = (lensRadius + 80.0)* std::sin(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, sn * spacing[0], focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, sn * spacing[1], focalPt[1] + fixRadiusY);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			m_pointsSource->SetPoint(fix, focalPt[0] + fixRadiusX, focalPt[1] + fixRadiusY, sn * spacing[2]);
			break;
		default:
			break;
		}
	}

	for (auto fix = 3*FixPoints; fix < NumPoints; ++fix)
	{
		double xCoordCircle1 = (innerLensRadius)* std::cos(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];
		double yCoordCircle1 = (innerLensRadius)* std::sin(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];

		double xCoordCircle2 = (lensRadius)* std::cos(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];
		double yCoordCircle2 = (lensRadius)* std::sin(fix * (360 / FixPoints) * std::numbers::pi / 180) * spacing[0];

		switch (m_mode)
		{
		case iASlicerMode::YZ:
			m_pointsTarget->SetPoint(fix, sn * spacing[0], focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1);
			m_pointsSource->SetPoint(fix, sn * spacing[0], focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2);
			break;
		case iASlicerMode::XZ:
			m_pointsTarget->SetPoint(fix, focalPt[0] + xCoordCircle1, sn * spacing[1], focalPt[1] + yCoordCircle1);
			m_pointsSource->SetPoint(fix, focalPt[0] + xCoordCircle2, sn * spacing[1], focalPt[1] + yCoordCircle2);
			break;
		case iASlicerMode::XY:
			m_pointsTarget->SetPoint(fix, focalPt[0] + xCoordCircle1, focalPt[1] + yCoordCircle1, sn * spacing[2]);
			m_pointsSource->SetPoint(fix, focalPt[0] + xCoordCircle2, focalPt[1] + yCoordCircle2, sn * spacing[2]);
			break;
		default:
			break;
		}
	}

	// Set position and text for green circle1 actors
	for (vtkIdType i = 0; i < NumPoints; ++i)
	{
		int idx1 = (m_mode == iASlicerMode::YZ) ? 1 : 0;
		int idx2 = (m_mode == iASlicerMode::XY) ? 1 : 2;
		m_circle1List.at(i)->SetCenter(m_pointsTarget->GetPoint(i)[idx1], m_pointsTarget->GetPoint(i)[idx2], 0.0);
		m_circle2List.at(i)->SetCenter(m_pointsSource->GetPoint(i)[idx1], m_pointsSource->GetPoint(i)[idx2], 0.0);
	}

	m_fisheye->SetCenter(focalPt[0], focalPt[1], 0.0);
	m_fisheye->SetRadius(lensRadius * reslicer->GetOutput()->GetSpacing()[0]);

	m_fisheyeTransform->SetSourceLandmarks(m_pointsSource);  // red
	m_fisheyeTransform->SetTargetLandmarks(m_pointsTarget);  // green

	reslicer->SetResliceTransform(m_fisheyeTransform);
	reslicer->Update();
	update();
}

void iASlicerImpl::updateMagicLens()
{
	if (!m_magicLens || !m_magicLens->isEnabled())
	{
		return;
	}
	vtkRenderer * ren = m_renWin->GetRenderers()->GetFirstRenderer();
	ren->SetWorldPoint(m_slicerPt[0], m_slicerPt[1], 0, 1);
	ren->WorldToDisplay();
	double dpos[3];
	ren->GetDisplayPoint(dpos);
	float lensSz = m_magicLens->size();
	// consider device pixel  ratio since measures from Qt are in "virtual" coordinates, but VTK ones are in "real" pixels
	qreal pixelWidth = devicePixelRatio() * geometry().width();
	qreal pixelHeight = devicePixelRatio() * geometry().height();
	lensSz = (std::min)(static_cast<qreal>(lensSz),
		(std::min)(pixelWidth, pixelHeight));  // restrict size to size of smallest side
	qreal lensSzHalf = 0.5*lensSz;
	// clamp to image, round to int (=pixels)
	dpos[0] = clamp(lensSzHalf, pixelWidth - lensSzHalf - 1, dpos[0]);
	dpos[1] = clamp(lensSzHalf, pixelHeight - lensSzHalf - 1, dpos[1]);
	dpos[2] = qRound(dpos[2]);
	ren->SetDisplayPoint(dpos);
	ren->DisplayToWorld();
	int const mousePos[2] = { static_cast<int>(dpos[0]), static_cast<int>(dpos[1]) };
	double const * worldP = ren->GetWorldPoint();
	m_magicLens->updatePosition(ren->GetActiveCamera(), worldP, mousePos);
	m_renWin->GetInteractor()->Render();
}

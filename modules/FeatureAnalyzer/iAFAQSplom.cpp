// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFAQSplom.h"

#include "FeatureAnalyzerHelpers.h"

#include "iAFeatureScoutTool.h"

#include <iASPLOMData.h>
#include <iAScatterPlot.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAModuleDispatcher.h>

#include <QAbstractTextDocumentLayout>
#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>

#include <omp.h>

const int maskOpacity = 127;

iAFAQSplom::iAFAQSplom( iAMainWindow *mWnd, QWidget * parent):
	iAQSplom(parent),
	m_fixedPointInd(iASPLOMData::NoDataIdx),
	m_mainWnd(mWnd),
	m_mdiChild(nullptr),
	m_csvName("")
{
	m_fixAction = m_contextMenu->addAction( "Fix Point", this, &iAFAQSplom::fixPoint);
	m_removeFixedAction = m_contextMenu->addAction( "Remove Fixed Point", this, &iAFAQSplom::removeFixedPoint);
	m_detailsToFeatureScoutAction = m_contextMenu->addAction("Detailed View...", this, &iAFAQSplom::sendToFeatureScout);

	m_detailsToFeatureScoutAction->setVisible(false);
	m_fixAction->setVisible( false );
	m_removeFixedAction->setVisible( false );
	setHistogramVisible(false);
}

void iAFAQSplom::setData( const QTableWidget * data )
{
	std::shared_ptr<iASPLOMData> newData(new iASPLOMData);
	int maskCol = data->columnCount() - 1;
	m_maskNames.clear();
	m_datasetIndices.clear();
	if( data->rowCount() )
	{
		int datasetIndexCol = -1;
		std::vector<QString> paramNames;
		for( int c = 0; c < maskCol; ++c ) //header
		{
			QString s = data->item( 0, c )->text();
			if (s == "Dataset Index")
			{
				datasetIndexCol = c;
			}
			paramNames.push_back(s);
		}
		newData->setParameterNames(paramNames, maskCol);
		for (int r = 1; r < data->rowCount(); ++r) //points
		{
			for (int c = 0; c < maskCol; ++c)
			{
				newData->data()[c].push_back(data->item(r, c)->text().toDouble());
			}
			m_maskNames.push_back( data->item( r, maskCol )->text() );
			if (datasetIndexCol >= 0)
			{
				m_datasetIndices.push_back(data->item(r, datasetIndexCol)->text().toInt());
			}
		}
	}
	std::vector<char> visibility(newData->numParams(), true);
	newData->updateRanges();
	iAQSplom::setData( newData, visibility );
}

void iAFAQSplom::setPreviewSliceNumbers( QList<int> sliceNumberLst )
{
	m_sliceNumPopupLst = sliceNumberLst;
	updatePreviewPixmap();
	update();
}

void iAFAQSplom::setROIList( QList<QRectF> roiLst )
{
	m_roiLst = roiLst;
	updatePreviewPixmap();
	update();
}

void iAFAQSplom::setSliceCounts( QList<int> sliceCnts )
{
	m_sliceCntLst = sliceCnts;
	m_sliceNumPopupLst.clear();
	for (const int& sc : m_sliceCntLst)
	{
		m_sliceNumPopupLst.push_back(sc * 0.5);
	}
}

void iAFAQSplom::setDatasetsDir( QString datsetsDir )
{
	m_datasetsDir = datsetsDir;
}

void iAFAQSplom::setDatasetsByIndices( QStringList selDatasets, QList<int> indices )
{
	m_dsIndices = indices;
	m_datasets.clear();
	for (const QString& d: selDatasets)
	{
		m_datasets.push_back(m_datasetsDir + "/" + QFileInfo(d).baseName());
	}
}

void iAFAQSplom::reemitFixedPixmap()
{
	if( m_fixedPointInd != iASPLOMData::NoDataIdx )
	{
		int dsInd = getDatasetIndexFromPointIndex( m_fixedPointInd );
		QString sliceFilename = getSliceFilename( m_maskNames[m_fixedPointInd], m_sliceNumPopupLst[dsInd] );
		QImage fixedMaskImg;
		if (!fixedMaskImg.load(sliceFilename, "PNG"))
		{
			return;
		}
		fixedMaskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		fixedMaskImg.setColor( 1, qRgba( 0, 255, 0, maskOpacity ) );

		m_maskPxmp = QPixmap::fromImage( fixedMaskImg );
		emit maskHovered( &m_maskPxmp, dsInd );
	}
	else
	{
		emit maskHovered(nullptr, -1);
	}
}

int iAFAQSplom::getDatasetIndexFromPointIndex(size_t pointIndex)
{
	if (pointIndex == iASPLOMData::NoDataIdx)
	{
		return -1;
	}
	int absInd = m_datasetIndices[pointIndex];
	int relInd = m_dsIndices.indexOf( absInd );
	return relInd;
}

bool iAFAQSplom::drawPopup( QPainter& painter )
{
	if (!iAQSplom::drawPopup(painter))
	{
		return false;
	}
	if (m_maskPxmp.isNull())
	{
		return false;
	}
	size_t curInd = m_activePlot->getCurrentPoint();
	double anim = 1.0;
	if( curInd == iASPLOMData::NoDataIdx )
	{
		if( m_viewData->animOut() > 0.0 && m_viewData->animIn() >= 1.0 )
		{
			anim = m_viewData->animOut();
			curInd = m_activePlot->getPreviousPoint();
		}
		else
		{
			return false;
		}
	}
	else if (m_activePlot->getPreviousIndex() == iASPLOMData::NoDataIdx)
	{
		anim = m_viewData->animIn();
	}

	painter.save();

	QPointF popupPos = m_activePlot->getPointPosition( curInd );
	double pPM = m_activePlot->settings.pickedPointMagnification;
	double ptRad = m_activePlot->getPointRadius();
	popupPos.setY( popupPos.y() - pPM * ptRad );
	painter.translate( popupPos );


	int dsInd = getDatasetIndexFromPointIndex( curInd );
	const QRectF & roi = m_roiLst[dsInd];
	double scaledH = settings.popupWidth / roi.width() * roi.height();
	painter.setOpacity( anim*1.0 );

	//draw current dataset name, pipeline name, slice number
	QString text =
		"<table width =\"100%\" cellspacing = \"0\" cellpadding= \"0px\" border =\"0\">"
		"<tr><td width =\"40%\" align =\"left\"><div align =\"left\"><u>Dataset:</u></div></td>"
		"<td width =\"60%\" align =\"right\"><div align =\"right\">Z-Slice: " + QString::number( m_sliceNumPopupLst[dsInd] ) + "</div></td></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\">" + m_currPrevDatasetName + "</div></td></tr>"
		"<tr></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\"><u>Pipeline:</u></div></td></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\">" + m_currPrevPipelineName + "</div></td></tr>"
		"</table>";
	QTextDocument doc;
	doc.setHtml( text );
	doc.setTextWidth( settings.popupWidth);
	QColor col = settings.popupFillColor; col.setAlpha( col.alpha()* anim ); painter.setBrush( col );
	col = settings.popupBorderColor; col.setAlpha( col.alpha()* anim ); painter.setPen( col );

	//draw dataset preview
	QPointF pixOrigin(-(settings.popupWidth/2), - m_popupHeight - settings.popupTipDim[1] - scaledH);
	painter.drawPixmap(pixOrigin, m_curSlicePxmp);
	painter.drawPixmap(pixOrigin, m_maskPxmpRoi);

	painter.drawRect( pixOrigin.x(), pixOrigin.y() - doc.size().rheight() - 3, settings.popupWidth, doc.size().rheight() + 3 );
	painter.translate( pixOrigin.x(), pixOrigin.y() - doc.size().rheight() - 3 );
	QAbstractTextDocumentLayout::PaintContext ctx;
	col = settings.popupTextColor; col.setAlpha( col.alpha()* anim );
	ctx.palette.setColor( QPalette::Text, col );
	ctx.palette.setColor( QPalette::Text, settings.popupTextColor );
	doc.documentLayout()->draw( &painter, ctx ); //doc.drawContents( &painter );

	painter.restore();
	return true;
}

void iAFAQSplom::keyPressEvent( QKeyEvent * event )
{
	if (!m_activePlot)
	{
		return;
	}
	int dsInd = getDatasetIndexFromPointIndex( m_activePlot->getCurrentPoint() );
	if (dsInd != -1)
	{
		switch (event->key())
		{
		case Qt::Key_Plus: //if plus is pressed increment slice number for the popup
			m_sliceNumPopupLst[dsInd] = clamp<int>( 0, m_sliceCntLst[dsInd] - 1, ++m_sliceNumPopupLst[dsInd] );
			update();
			updatePreviewPixmap();
			emit previewSliceChanged( m_sliceNumPopupLst[dsInd] );
			break;
		case Qt::Key_Minus: //if minus is pressed decrement slice number for the popup
			m_sliceNumPopupLst[dsInd] = clamp<int>( 0, m_sliceCntLst[dsInd] - 1, --m_sliceNumPopupLst[dsInd] );
			update();
			updatePreviewPixmap();
			emit previewSliceChanged( m_sliceNumPopupLst[dsInd] );
			break;
		}
	}
	iAQSplom::keyPressEvent( event );
}

void iAFAQSplom::updatePreviewPixmap()
{
	if (!m_activePlot)
	{

		return;
	}

	size_t curInd = m_activePlot->getCurrentPoint();
	if( curInd == iASPLOMData::NoDataIdx && m_fixedPointInd == iASPLOMData::NoDataIdx )
	{
		emit maskHovered( 0, -1 );
		return;
	}

	QImage fixedMaskImg;
	int dsInd = -1;
	if( m_fixedPointInd != iASPLOMData::NoDataIdx)
	{
		dsInd = getDatasetIndexFromPointIndex( m_fixedPointInd );
		QString sliceFilename = getSliceFilename( m_maskNames[m_fixedPointInd], m_sliceNumPopupLst[dsInd] );
		if (!fixedMaskImg.load(sliceFilename, "PNG"))
		{
			return;
		}
		fixedMaskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		fixedMaskImg.setColor( 1, qRgba( 0, 255, 0, maskOpacity ) );
	}

	QImage maskImg;
	if (curInd == iASPLOMData::NoDataIdx)
	{
		maskImg = fixedMaskImg;
	}
	else
	{
		dsInd = getDatasetIndexFromPointIndex(curInd);
		QString sliceFilename = getSliceFilename( m_maskNames[curInd], m_sliceNumPopupLst[dsInd] );
		if (!maskImg.load(sliceFilename, "PNG"))
		{
			return;
		}

		// Extract dataset and pipeline string
		m_currPrevDatasetName = sliceFilename.section( '/', -5, -5 ).section( '_', -1, -1 ).append( ".mhd" );
		QString mix = sliceFilename.section( '/', -5, -5 );
		mix.truncate( mix.lastIndexOf( '_' ) );
		m_currPrevPipelineName = mix;

		maskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		maskImg.setColor( 1, qRgba( 255, 0, 0, maskOpacity ) );
		maskImg.setColor( 2, qRgba( 0, 0, 255, maskOpacity ) );
		maskImg.setColor( 3, qRgba( 255, 255, 0, maskOpacity ) );

		if( m_fixedPointInd != iASPLOMData::NoDataIdx && getDatasetIndexFromPointIndex(m_fixedPointInd) == dsInd )
		{
			uchar * maskPtr = maskImg.bits();
			uchar * fixedPtr = fixedMaskImg.bits();
			const long size = maskImg.height()*maskImg.width();

			#pragma omp parallel for
			for (long i = 0; i < size; ++i)
			{
				if (maskPtr[i] && !fixedPtr[i])
				{
					maskPtr[i] = 1;
				}
				else if (!maskPtr[i] && fixedPtr[i])
				{
					maskPtr[i] = 2;
				}
				else if (maskPtr[i] && fixedPtr[i])
				{
					maskPtr[i] = 3;
				}
			}
		}
	}

	m_maskPxmp = QPixmap::fromImage( maskImg );
	QRect roi = m_roiLst[dsInd].toRect();
	m_maskPxmpRoi = m_maskPxmp.copy( roi ).scaledToWidth(settings.popupWidth);

	QString dataSliceFilename = getSliceFilename( m_datasets[dsInd], m_sliceNumPopupLst[dsInd] );
	QPixmap dataPixmap;
	if (!dataPixmap.load(dataSliceFilename, "PNG"))
	{
		return;
	}
	m_curSlicePxmp = dataPixmap.copy( roi ).scaledToWidth(settings.popupWidth);
	emit maskHovered( &m_maskPxmp, dsInd );
}

void iAFAQSplom::currentPointUpdated( size_t index )
{
	if (index != iASPLOMData::NoDataIdx)
	{
		m_fixAction->setVisible(true);
		m_detailsToFeatureScoutAction->setVisible(true);
	}
	else
	{
		m_fixAction->setVisible(false);
		m_detailsToFeatureScoutAction->setVisible(false);
	}
	updatePreviewPixmap();
	iAQSplom::currentPointUpdated( index );
}

void iAFAQSplom::fixPoint()
{
	if (!m_activePlot)
	{
		return;
	}

	if (m_fixedPointInd != iASPLOMData::NoDataIdx)
	{
		removeFixedPoint();
	}

	m_removeFixedAction->setVisible( true );
	m_fixedPointInd = m_activePlot->getCurrentPoint();
	m_viewData->addHighlightedPoint( m_fixedPointInd );
	updatePreviewPixmap();
}


void iAFAQSplom::sendToFeatureScout()
{
	if (!m_activePlot)
	{
		return;
	}
	QString fileName = "";
	QString mhdName = "";
	getFilesLabeledFromPoint(fileName, mhdName);

	m_mdiChild = m_mainWnd->createMdiChild(false);
	if (!m_mdiChild)
	{
		return;
	}
	m_mdiChild->show();
	connect(m_mdiChild, &iAMdiChild::dataSetRendered, this, &iAFAQSplom::startFeatureScout);
	m_mainWnd->loadFile(mhdName, m_mdiChild);
}

void iAFAQSplom::getFilesLabeledFromPoint(QString &fileName, QString &mhdName)
{
	size_t curPoint = m_activePlot->getCurrentPoint();
	int dsInd = getDatasetIndexFromPointIndex(curPoint);
	QString sliceFileName = getSliceFilename(m_maskNames[curPoint], m_sliceNumPopupLst[dsInd]);
	QString dataPath = sliceFileName.section('/', 0, -3);
	fileName = sliceFileName.section('/', -2, -2).section('_', 0, 0);
	mhdName = dataPath + "/" + fileName + "_labeled" + ".mhd";
	m_csvName = dataPath + "/" + fileName + ".mhd.csv";
}

void iAFAQSplom::startFeatureScout()
{
	iAFeatureScoutTool::addToChild(m_mdiChild, m_csvName);
	disconnect(m_mdiChild, &iAMdiChild::dataSetRendered, this, &iAFAQSplom::startFeatureScout);
}

void iAFAQSplom::removeFixedPoint()
{
	if (m_fixedPointInd != iASPLOMData::NoDataIdx)
	{
		m_viewData->removeHighlightedPoint(m_fixedPointInd);
	}
	m_fixedPointInd = iASPLOMData::NoDataIdx;
	m_removeFixedAction->setVisible( false );
	updatePreviewPixmap();
}

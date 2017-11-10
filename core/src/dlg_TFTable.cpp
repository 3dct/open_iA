/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "dlg_TFTable.h"
#include "dlg_transfer.h"
#include "charts/iADiagramFctWidget.h"

#include <vtkSmartPointer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QColorDialog>
#include <QMessageBox>


const QStringList columnNames = QStringList()\
<< "X" << "Y" << "Color";

class MyTableWidgetItem : public QTableWidgetItem
{
public:
	bool operator <( const QTableWidgetItem &other ) const
	{
		return text().toDouble() < other.text().toDouble();
	}
};

dlg_TFTable::dlg_TFTable( iADiagramFctWidget * parent, dlg_function* func ) : dlg_TFTableWidgetConnector( parent ),
	m_parent(parent),
	m_oTF( dynamic_cast<dlg_transfer*>( func )->GetOpacityFunction() ),
	m_cTF( dynamic_cast<dlg_transfer*>( func )->GetColorFunction() ),
	m_newPointColor( Qt::gray )
{
	Init();
	updateTable();
	resize( table->columnWidth( 0 ) * columnNames.size(), table->rowHeight( 0 ) * 13 );
}

void dlg_TFTable::Init()
{
	m_oTF->GetRange( m_xRange );
	dsbNewPointX->setRange( m_xRange[0], m_xRange[1] );
	
	QPixmap pxMap( 23, 23 );
	pxMap.fill( m_newPointColor );
	tbChangeColor->setIcon( pxMap );

	QAction* removePnt = new QAction( tr( "Remove Point" ), this );
	QAction* addPnt = new QAction( tr( "add Point" ), this );
	QAction* updateHisto = new QAction( tr( "Update Histogram" ), this );
	removePnt->setShortcut( Qt::Key_Delete );
	addPnt->setShortcut( Qt::Key_Space );
	updateHisto->setShortcut( Qt::Key_Enter );
	addAction( addPnt );
	addAction( updateHisto );
	table->addAction( removePnt );
	table->setColumnCount( columnNames.size() );
	table->setHorizontalHeaderLabels( columnNames );
	table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
	table->verticalHeader()->setDefaultSectionSize( 25 );
	table->setSelectionBehavior( QAbstractItemView::SelectRows );
	
	connect( tbChangeColor, SIGNAL( clicked() ), this, SLOT( changeColor() ) );
	connect( tbAddPoint, SIGNAL( clicked() ), this, SLOT( addPoint() ) );
	connect( addPnt, SIGNAL( triggered() ), this, SLOT( addPoint() ) );
	connect( tbRemovePoint, SIGNAL( clicked() ), this, SLOT( removeSelectedPoint() ) );
	connect( removePnt, SIGNAL( triggered() ), this, SLOT( removeSelectedPoint() ) );
	connect( tbUpdateHisto, SIGNAL( clicked() ), this, SLOT( updateHistogram() ) );
	connect( updateHisto, SIGNAL( triggered() ), this, SLOT( updateHistogram() ) );
	connect( table, SIGNAL( itemClicked( QTableWidgetItem * ) ), this, SLOT( itemClicked( QTableWidgetItem * ) ) );
	connect( table, SIGNAL( cellChanged( int, int ) ), this, SLOT( cellValueChanged( int, int ) ) );
}

void dlg_TFTable::updateTable()
{
	table->setRowCount( m_oTF->GetSize() );
	table->blockSignals( true );
	for ( int i = 0; i < m_oTF->GetSize(); ++i )
	{
		int t = m_oTF->GetSize();
		double pointValue[4], color[4];
		m_oTF->GetNodeValue( i, pointValue );
		m_cTF->GetIndexedColor( i, color );
		QColor c; c.setRgbF( color[0], color[1], color[2], color[3] );
		MyTableWidgetItem* xItem = new MyTableWidgetItem;
		MyTableWidgetItem* yItem = new MyTableWidgetItem;
		MyTableWidgetItem* colorItem = new MyTableWidgetItem;
		xItem->setData( Qt::DisplayRole, QString::number( (double) pointValue[0] ) );
		yItem->setData( Qt::DisplayRole, QString::number( (double) pointValue[1] ) );
		colorItem->setBackgroundColor( c );
		if ( i == 0 || i == m_oTF->GetSize() - 1 )
		{
			xItem->setFlags( xItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled );
			yItem->setFlags( yItem->flags() & ~Qt::ItemIsSelectable );
			colorItem->setFlags( colorItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable );
		}
		table->setItem( i, 0, xItem );
		table->setItem( i, 1, yItem );
		table->setItem( i, 2, colorItem );
	}
	table->blockSignals( false );
}

void dlg_TFTable::changeColor()
{
	m_newPointColor = QColorDialog::getColor( Qt::gray, this, "Set Color", QColorDialog::ShowAlphaChannel );
	QPixmap pxMap( 23, 23 );
	pxMap.fill( m_newPointColor );
	tbChangeColor->setIcon( pxMap );
}

void dlg_TFTable::addPoint()
{
	if ( !isValueXValid( dsbNewPointX->value() ) )
		return;
	table->insertRow( table->rowCount() );
	table->blockSignals( true );
	MyTableWidgetItem* newXItem = new MyTableWidgetItem;
	MyTableWidgetItem* newYItem = new MyTableWidgetItem;
	MyTableWidgetItem* newColorItem = new MyTableWidgetItem;
	newXItem->setData( Qt::DisplayRole, QString::number( (double) dsbNewPointX->value() ) );
	newYItem->setData( Qt::DisplayRole, QString::number( (double) dsbNewPointY->value() ) );
	table->setSortingEnabled( false );
	table->setItem( table->rowCount()-1, 0, newXItem );
	table->setItem( table->rowCount()-1, 1, newYItem );
	newColorItem->setBackgroundColor( m_newPointColor );
	table->setItem( table->rowCount()-1, 2, newColorItem );
	table->setSortingEnabled( true );
	table->sortByColumn( 0, Qt::AscendingOrder );
	table->blockSignals( false );
}

void dlg_TFTable::removeSelectedPoint()
{
	QList<QTableWidgetSelectionRange> selRangeList =  table->selectedRanges();
	QList<int> rowsToRemove;
	// Bug fix: first/last row selection (despite: ~Qt::ItemIsSelectable)
	for ( int i = 0; i < selRangeList.size(); ++i )	
	{
		for ( int j = selRangeList[i].topRow(); j <= selRangeList[i].bottomRow(); ++j )
		{
			if ( j == 0 || j == table->rowCount() - 1 )
				continue;
			rowsToRemove.append( j );
		}
	}
	qSort( rowsToRemove.begin(), rowsToRemove.end(), qGreater<int>() );
	foreach( int row, rowsToRemove )
		table->removeRow( row );
}

void dlg_TFTable::updateHistogram()
{
	m_oTF->RemoveAllPoints();
	m_cTF->RemoveAllPoints();
	for ( int i = 0; i < table->rowCount(); ++i )
	{
		double x = table->item( i, 0 )->data( Qt::DisplayRole ).toDouble();
		double y = table->item( i, 1 )->data( Qt::DisplayRole ).toDouble();
		QColor c = table->item( i, 2 )->backgroundColor();
		m_oTF->AddPoint( x, y );
		m_cTF->AddRGBPoint( x, c.redF(), c.greenF(), c.blueF() );
	}
	m_parent->SetTransferFunctions( m_cTF, m_oTF );
}

void dlg_TFTable::itemClicked( QTableWidgetItem * item )
{
	if ( item->column() == 2 )
	{
		table->blockSignals( true );
		QColor newItemColor = QColorDialog::getColor( Qt::gray, this, "Set Color", QColorDialog::ShowAlphaChannel );
		item->setBackgroundColor( newItemColor );
		table->blockSignals( false );
	}
	else
	{
		m_oldItemValue = item->data( Qt::DisplayRole ).toDouble();
	}
}

void dlg_TFTable::cellValueChanged( int changedRow, int changedColumn )
{
	double val = table->item( changedRow, changedColumn )->data( Qt::DisplayRole ).toDouble();
	table->blockSignals( true );
	switch ( changedColumn )
	{
		case 0:
			if ( !isValueXValid( val, changedRow ) )
				table->item( changedRow, changedColumn )->setData( Qt::DisplayRole, QString::number( m_oldItemValue ) );
			break;
		case 1:
			if ( val < 0.0 || val > 1.0 )
				table->item( changedRow, changedColumn )->setData( Qt::DisplayRole, QString::number( m_oldItemValue ) );
			break;
		default:
			break;
	}
	table->sortByColumn( 0, Qt::AscendingOrder );
	table->blockSignals( false );
}

bool dlg_TFTable::isValueXValid( double xVal, int row )
{
	if ( xVal < m_xRange[0] || xVal > m_xRange[1] )
	{
		QMessageBox msgBox;
		msgBox.setText( "X-value out of range. "
						"Set value between transfer function start point and end point" );
		msgBox.setWindowTitle( "Transfer Function Table View: Add Point" );
		msgBox.exec();
		return false;
	}
	if ( xVal == m_xRange[0] || xVal == m_xRange[1] )
	{
		QMessageBox msgBox;
		msgBox.setText( "X-value equals transfer function start point or end point. "
						"Set the X-value beween the start point and the end point." );
		msgBox.setWindowTitle( "Transfer Function Table View: Add Point" );
		msgBox.exec();
		return false;
	}
	for ( int i = 0; i < table->rowCount(); ++i )
	{
		if ( i == row )
			continue;
		if ( xVal == table->item( i, 0 )->data( Qt::DisplayRole ).toDouble() )
		{
			QMessageBox msgBox;
			msgBox.setText( tr( "Cannot add point. X-value already exists in row %1." ).arg( i + 1 ) );
			msgBox.setWindowTitle( "Transfer Function Table View: Add Point" );
			msgBox.exec();
			return false;
		}
	}
	return true;
}
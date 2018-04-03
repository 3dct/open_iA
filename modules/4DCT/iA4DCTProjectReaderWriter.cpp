/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "pch.h"
#include "iA4DCTProjectReaderWriter.h"
// iA
#include "iA4DCTMainWin.h"
//#include "iA4DCTStageData.h"
#include "iA4DCTData.h"
// Qt
#include <QFileInfo>
#include <QDir>
#include <QVector>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>

void iA4DCTProjectReaderWriter::save( iA4DCTMainWin * mainWin, QString path )
{
	// open file
	QFile file( path );
	file.open( QFile::WriteOnly );
	if( !file.isOpen( ) ) return;

	iA4DCTData * stageData = mainWin->getStageData( );

	QFileInfo fileInfo( path );
	QDir dir = fileInfo.absoluteDir( );

	QXmlStreamWriter stream( &file );
	stream.setAutoFormatting( true );
	stream.writeStartDocument( );
	stream.writeStartElement( "data" );
	double * size = mainWin->getSize( );
	stream.writeTextElement( "size", QString( "%1 %2 %3" ).arg( QString::number( size[0] ) ).arg( QString::number( size[1] ) ).arg( QString::number( size[2] ) ) );
	for( int i = 0; i < stageData->count( ); i++ )
	{
		iA4DCTStageData * sd = stageData->at( i );
		stream.writeStartElement( "stage" );
		stream.writeTextElement( "force", QString::number( sd->Force ) );

		stream.writeStartElement( "files" );
		for( auto f : sd->Files )
		{
			stream.writeStartElement( "file" );
			stream.writeTextElement( "absolutePath", f.Path );
			stream.writeTextElement( "relativePath", dir.relativeFilePath( f.Path ) );
			stream.writeTextElement( "name", f.Name );
			stream.writeEndElement( );
		}
		stream.writeEndElement( );

		stream.writeEndElement( );
	}
	stream.writeEndElement( );
	stream.writeEndDocument( );

	file.close( );
}

bool iA4DCTProjectReaderWriter::load( iA4DCTMainWin * mainWin, QString path )
{
	// open file
	QDomDocument doc;
	QFile file( path );
	if( !file.open( QFile::ReadOnly ) ) {
		return false;
	}
	if( !doc.setContent( &file ) ) {
		file.close( );
		return false;
	}

	QFileInfo fileInfo( path );
	QDir dir = fileInfo.absoluteDir( );

	// parse
	QDomElement sizeElem = doc.elementsByTagName( "data" ).item( 0 ).firstChildElement( "size" );
	if( !sizeElem.isNull( ) ) {
		QStringList stringList = sizeElem.text( ).split( ' ' );
		double size[3] = { stringList[0].toDouble( ), stringList[1].toDouble( ), stringList[2].toDouble( ) };
		mainWin->setSize( size );
	} else {
		// ToDo: give a warning
	}

	QDomNodeList stages = doc.elementsByTagName( "stage" );
	for( int i = 0; i < stages.count( ); i++ )
	{
		QDomNode stage = stages.item( i );
		QDomElement force = stage.firstChildElement( "force" );

		QDomElement files = stage.firstChildElement( "files" );
		QList<iA4DCTFileData> list;
		if( files.hasChildNodes( ) ) {
			QDomNodeList fileList = files.childNodes( );
			for( int i = 0; i < fileList.size( ); i++ ) {
				QDomNode f = fileList.item( i );
				QDomElement abssolutePath = f.firstChildElement( "absolutePath" );
				QDomElement relativePath = f.firstChildElement( "relativePath" );
				QDomElement name = f.firstChildElement( "name" );

				QString path = dir.filePath( abssolutePath.text( ) );
				if( !QFile::exists( path ) ) {
					path = dir.filePath( relativePath.text( ) );
					if( !QFile::exists( path ) ) {
						continue;
					}
				}
				path = QFileInfo( path ).absoluteFilePath( );	// cleaning path
				if( name.isNull( ) )
					continue;;
				iA4DCTFileData fd; fd.Name = name.text( ); fd.Path = path;
				list.push_back( fd );
			}
		}

		if( force.isNull( ) )
			continue;

		iA4DCTStageData sd;
		sd.Files = list;
		sd.Force = force.text( ).toInt( );

		mainWin->addStage( sd );
	}

	return true;
}

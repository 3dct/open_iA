// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTProjectReaderWriter.h"

#include "iA4DCTMainWin.h"
#include "iA4DCTData.h"

#include <QDir>
#include <QXmlStreamWriter>
#include <QDomElement>

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
		if( files.hasChildNodes( ) )
		{
			QDomNodeList fileList = files.childNodes( );
			for( int j = 0; j < fileList.size( ); j++ )
			{
				QDomNode f = fileList.item( j );
				QDomElement abssolutePath = f.firstChildElement( "absolutePath" );
				QDomElement relativePath = f.firstChildElement( "relativePath" );
				QDomElement name = f.firstChildElement( "name" );

				QString filePath = dir.filePath( abssolutePath.text( ) );
				if( !QFile::exists( filePath ) )
				{
					filePath = dir.filePath( relativePath.text( ) );
					if( !QFile::exists( filePath ) )
					{
						continue;
					}
				}
				filePath = QFileInfo( filePath ).absoluteFilePath( );	// cleaning path
				if (name.isNull())
				{
					continue;
				}
				iA4DCTFileData fd; fd.Name = name.text( ); fd.Path = filePath;
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

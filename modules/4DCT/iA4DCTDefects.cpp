// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTDefects.h"

#include <QFile>
#include <QTextStream>

bool iA4DCTDefects::save( VectorDataType defects, QString path )
{
	QFile file( path );
	QTextStream out( &file );
	if( file.open( QIODevice::WriteOnly ) ) {
		for( auto d : defects ) {
			out << std::to_string( d ).c_str( ) << '\n';
		}
		return true;
	}
	return false;
}

iA4DCTDefects::VectorDataType iA4DCTDefects::load( QString path )
{
	VectorDataType list;
	// open file
	QFile file( path );
	QTextStream stream( &file );
	// open and read file
	if( file.open( QIODevice::ReadOnly ) ) {
		QString line = stream.readLine( );
		while( !line.isNull( ) ) {
			list.push_back( line.toULong( ) );
			line = stream.readLine( );
		}
	}
	return list;
}

iA4DCTDefects::HashDataType iA4DCTDefects::DefectDataToHash( VectorDataType defects )
{
	HashDataType hash;
	for( auto d : defects ) {
		hash.insert( d, true );
	}
	return hash;
}

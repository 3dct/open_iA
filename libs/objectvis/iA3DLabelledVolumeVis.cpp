// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DLabelledVolumeVis.h"

#include "iACsvConfig.h"
#include "iAObjectType.h"

#include <vtkColorTransferFunction.h>
#include <vtkFloatArray.h>
#include <vtkPiecewiseFunction.h>
#include <vtkTable.h>

#include <QStandardItem>

iA3DLabelledVolumeVis::iA3DLabelledVolumeVis(vtkColorTransferFunction* color, vtkPiecewiseFunction* opac,
		vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, double const * bounds ):
	iA3DObjectVis(objectTable, columnMapping),
	oTF(opac),
	cTF(color)
{
	std::copy(bounds, bounds + 6, m_bounds);
}

void iA3DLabelledVolumeVis::renderSelection( std::vector<size_t> const & sortedSelInds, int /*classID*/, QColor const & classColor, QStandardItem* activeClassItem )
{
	QColor BackColor(128, 128, 128, 0);
	double backRGB[3];
	backRGB[0] = BackColor.redF(); backRGB[1] = BackColor.greenF(); backRGB[2] = BackColor.blueF(); // background color
	double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.5, backAlpha = 0.00, classRGB[3], selRGB[3];
	selRGB[0] = SelectedColor.redF();
	selRGB[1] = SelectedColor.greenF();
	selRGB[2] = SelectedColor.blueF();
	classRGB[0] = classColor.redF();
	classRGB[1] = classColor.greenF();
	classRGB[2] = classColor.blueF();

	// clear existing points
	oTF->RemoveAllPoints();
	cTF->RemoveAllPoints();
	oTF->ClampingOff();
	cTF->ClampingOff();
	oTF->AddPoint( 0, backAlpha, 0.5, 1.0 );
	cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );

	size_t const HidInvalid = std::numeric_limits<size_t>::max();
	size_t hid = 0, next_hid = 1, prev_hid = HidInvalid;
	size_t selectionIndex = 0, previous_selectionIndex = 0;
	bool starting = false, hid_isASelection = false, previous_hid_isASelection = false;

	int countClass = activeClassItem->rowCount();
	for (int j = 0; j < countClass; ++j )
	{
		hid = activeClassItem->child( j )->text().toULongLong();

		if (sortedSelInds.size() > 0 )
		{
			if (hid-1 == sortedSelInds[selectionIndex] )
			{
				hid_isASelection = true;
				red = SelectedColor.redF(), green = SelectedColor.greenF(), blue = SelectedColor.blueF();

				if ( selectionIndex + 1 < sortedSelInds.size() )
					selectionIndex++;
			}
			else
			{
				hid_isASelection = false;
				red = classRGB[0]; green = classRGB[1]; blue = classRGB[2];
			}

			if (prev_hid != HidInvalid)
			{
				if (prev_hid-1 == sortedSelInds[previous_selectionIndex])
				{
					previous_hid_isASelection = true;

					if ( previous_selectionIndex + 1 < sortedSelInds.size())
						previous_selectionIndex++;
				}
				else
					previous_hid_isASelection = false;
			}
		}
		else
		{
			red = classRGB[0]; green = classRGB[1]; blue = classRGB[2];
		}

		// If we are not yet at the last object (of the class) get the next hid
		if ( ( j + 1 ) < countClass )
		{
			next_hid = activeClassItem->child( j + 1 )->text().toULongLong();
		}
		else	// If hid = the last object (of the class) we have to set the last object points
		{
			if ( starting )	// If we are in a sequence we have to set the ending (\)
			{
				oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

				if ( hid_isASelection )
				{
					cTF->AddRGBPoint( hid - 0.5, 1.0, 0.0, 0.0, 0.5, 1.0 );
					cTF->AddRGBPoint( hid, 1.0, 0.0, 0.0, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}
				else
				{
					cTF->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}

				if ( previous_hid_isASelection )
					cTF->AddRGBPoint( hid - 1 + 0.3, 1.0, 0.0, 0.0, 0.5, 1.0 );
				else
					cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
				break;
			}
			else	// if we are not in a sequence we have to create the last tooth (/\)
			{
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				break;
			}
		}

		if ( next_hid > hid + 1 && !starting )		//Create one single tooth
		{
			oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
			oTF->AddPoint( hid, alpha, 0.5, 1.0 );
			oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
			cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
		else if ( next_hid == hid + 1 && !starting )	//Creates the beginning of a sequence (/)
		{
			starting = true;
			oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
			oTF->AddPoint( hid, alpha, 0.5, 1.0 );
			cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
		}
		else if ( next_hid == hid + 1 && starting )	//Continues the started sequence (-)
		{
			if ( !hid_isASelection && previous_hid_isASelection )
			{
				cTF->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

				oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
			}
			else if ( hid_isASelection && !previous_hid_isASelection )
			{
				cTF->AddRGBPoint( hid - 0.5, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

				oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
			}
		}
		else if ( next_hid > hid + 1 && starting )	//  (\)
		{
			starting = false;

			oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
			oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
			oTF->AddPoint( hid, alpha, 0.5, 1.0 );
			oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

			if ( previous_hid_isASelection )
				cTF->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
			else
				cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

			cTF->AddRGBPoint( hid - 0.5, red, green, blue, 0.5, 1.0 );
			cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
		prev_hid = hid;
	}

	if ( hid < static_cast<size_t>(m_objectTable->GetNumberOfRows()) )	// Creates the very last points (for all objects)  if it's not created yet
	{
		oTF->AddPoint( m_objectTable->GetNumberOfRows() + 0.3, backAlpha, 0.5, 1.0 );
		cTF->AddRGBPoint( m_objectTable->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
	}
	emit renderRequired();
}

void iA3DLabelledVolumeVis::renderSingle(IndexType selectedObjID, int /*classID*/, QColor const & classColor, QStandardItem* activeClassItem )
{
	int itemL = activeClassItem->rowCount();
	double red   = classColor.redF(),
		   green = classColor.greenF(),
		   blue  = classColor.blueF(),
		   alpha = 0.5,
		   backAlpha = 0.0,
		   backRGB[3] = { 0.0, 0.0, 0.0 };

	// clear existing points
	oTF->RemoveAllPoints();
	cTF->RemoveAllPoints();

	// set background opacity and color with clamping off
	oTF->ClampingOff();
	cTF->ClampingOff();
	oTF->AddPoint(0, backAlpha);
	cTF->AddRGBPoint(0, backRGB[0], backRGB[1], backRGB[2]);
	if (selectedObjID > 0 ) // for single object selection
	{
		if ( (selectedObjID - 1) >= 0)
		{
			oTF->AddPoint(selectedObjID - 0.5, backAlpha);
			oTF->AddPoint(selectedObjID - 0.49, alpha);
			cTF->AddRGBPoint(selectedObjID - 0.5, backRGB[0], backRGB[1], backRGB[2]);
			cTF->AddRGBPoint(selectedObjID - 0.49, red, green, blue);
		}
		oTF->AddPoint(selectedObjID, alpha);
		cTF->AddRGBPoint(selectedObjID, red, green, blue);
		if ((selectedObjID + 1) <= m_objectTable->GetNumberOfRows())
		{
			oTF->AddPoint(selectedObjID + 0.3, backAlpha);
			oTF->AddPoint(selectedObjID + 0.29, alpha);
			cTF->AddRGBPoint(selectedObjID + 0.3, backRGB[0], backRGB[1], backRGB[2]);
			cTF->AddRGBPoint(selectedObjID + 0.29, red, green, blue);
		}
	}
	else // for single class selection
	{
		int hid = 0, next_hid = 1;
		bool starting = false;
		for ( int j = 0; j < itemL; ++j )
		{
			hid = activeClassItem->child( j, 0 )->text().toInt();

			if (j + 1 < itemL)
			{
				next_hid = activeClassItem->child(j + 1, 0)->text().toInt();
			}
			else
			{
				if ( starting )
				{
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
				else
				{
					oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
			}

			//Create one single tooth
			if ( next_hid > hid + 1 && !starting )
			{
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && !starting )
			{
				starting = true;
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && starting )
				continue;
			else if ( next_hid > hid + 1 && starting )
			{
				starting = false;
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
		}

		if ( hid < m_objectTable->GetNumberOfRows() )
		{
			oTF->AddPoint( m_objectTable->GetNumberOfRows() + 0.3, backAlpha, 0.5, 1.0 );
			cTF->AddRGBPoint( m_objectTable->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
	}
	emit renderRequired();
}

void iA3DLabelledVolumeVis::multiClassRendering( QList<QColor> const & classColors, QStandardItem* rootItem, double alpha )
{
	double backAlpha = 0.00005;
	double backRGB[3];
	backRGB[0] = classColors.at(0).redF();
	backRGB[1] = classColors.at(0).greenF();
	backRGB[2] = classColors.at(0).blueF();

	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;

	// clear existing points
	oTF->RemoveAllPoints();
	cTF->RemoveAllPoints();

	// set background opacity and color
	oTF->ClampingOff();
	cTF->ClampingOff();

	// Iterate through all classes to render, starting with 0 unclassified, 1 Class1,...
	for (int i = 0; i < classColors.size(); i++)
	{
		red   = classColors.at(i).redF();
		green = classColors.at(i).greenF();
		blue  = classColors.at(i).blueF();

		QStandardItem *item = rootItem->child(i, 0);
		int itemL = item->rowCount();

		// Class has no objects, proceed with next class
		if (!itemL)
			continue;

		int hid = 0, next_hid = 1;
		bool starting = false;

		for (int j = 0; j < itemL; ++j)
		{
			hid = item->child(j, 0)->text().toInt();

			if ((j + 1) < itemL)
			{
				next_hid = item->child(j + 1, 0)->text().toInt();
			}
			else
			{
				if (starting)
				{
					oTF->AddPoint(hid, alpha, 0.5, 1.0);
					oTF->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
					cTF->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
					cTF->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					break;
				}
				else
				{
					oTF->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
					oTF->AddPoint(hid, alpha, 0.5, 1.0);
					oTF->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
					cTF->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					cTF->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
					cTF->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					break;
				}
			}

			//Create one single tooth
			if (next_hid > hid + 1 && !starting)
			{
				oTF->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
				oTF->AddPoint(hid, alpha, 0.5, 1.0);
				oTF->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
				cTF->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
				cTF->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
				cTF->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
			}
			else if (next_hid == hid + 1 && !starting)
			{
				starting = true;
				oTF->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
				oTF->AddPoint(hid, alpha, 0.5, 1.0);
				cTF->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
				cTF->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
			}
			else if (next_hid == hid + 1 && starting)
				continue;

			else if (next_hid > hid + 1 && starting)
			{
				starting = false;
				oTF->AddPoint(hid, alpha, 0.5, 1.0);
				oTF->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
				cTF->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
				cTF->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
			}
		}

		if ( hid < m_objectTable->GetNumberOfRows() )
		{
			oTF->AddPoint(m_objectTable->GetNumberOfRows() + 0.3, backAlpha, 0.5, 1.0);
			cTF->AddRGBPoint(m_objectTable->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
		}
	}
	emit renderRequired();
}

void iA3DLabelledVolumeVis::renderOrientationDistribution( vtkImageData* oi )
{
	double backRGB[3];
	backRGB[0] = 0.0; backRGB[1] = 0.0; backRGB[2] = 0.0;
	double backAlpha = 0.0, alpha = 0.5;
	// clear existing points
	oTF->RemoveAllPoints();
	cTF->RemoveAllPoints();
	oTF->AddPoint( 0, backAlpha );
	cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2] );

	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		QColor color = getOrientationColor( oi, objID );
		oTF->AddPoint( objID + 1, alpha );
		cTF->AddRGBPoint( objID + 1, color.redF(), color.greenF(), color.blueF() );
	}
	emit renderRequired();
}

void iA3DLabelledVolumeVis::renderLengthDistribution( vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range )
{
	// clear existing points
	oTF->RemoveAllPoints();
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(0, 0.0, 0.0, 0.0);

	for (IndexType objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID )
	{
		double ll = m_objectTable->GetValue(objID, m_columnMapping->value(iACsvConfig::Length)).ToDouble();
		QColor color = getLengthColor( ctFun, objID );

		if ( filterID == iAObjectType::Fibers )
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 1.0 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 1 ) + halfInc && ll < extents->GetValue( 2 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 2 ) + halfInc && ll < extents->GetValue( 5 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.015 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 7 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 1.0 );
			}
		}
		else
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 2 ) + halfInc )
			{
				oTF->AddPoint( objID + 1 - 0.5, 0.0 );
				oTF->AddPoint( objID + 1 + 0.3, 0.0 );
				oTF->AddPoint( objID + 1, 0.5 );
			}
		}
		cTF->AddRGBPoint( objID + 1, color.redF(), color.greenF(), color.blueF() );
		cTF->AddRGBPoint( objID + 1 - 0.5, color.redF(), color.greenF(), color.blueF() );
		cTF->AddRGBPoint( objID + 1 + 0.3, color.redF(), color.greenF(), color.blueF() );
	}
	emit renderRequired();
}

double const * iA3DLabelledVolumeVis::bounds()
{
	return m_bounds;
}

QSharedPointer<iA3DObjectActor> iA3DLabelledVolumeVis::createActor(vtkRenderer* ren)
{
	auto result = QSharedPointer<iA3DObjectActor>::create(ren);
	connect(this, &iA3DObjectVis::dataChanged, result.data(), &iA3DObjectActor::updateRenderer);
	connect(this, &iA3DObjectVis::renderRequired, result.data(), &iA3DObjectActor::updateRenderer);
	return result;
}

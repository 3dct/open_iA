// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabeledVolumeVis.h"

#include "iACsvConfig.h"
#include "iAObjectsData.h"
#include "iAObjectType.h"
#include "iATransferFunction.h"

#include <vtkColorTransferFunction.h>
#include <vtkFloatArray.h>
#include <vtkPiecewiseFunction.h>
#include <vtkTable.h>

#include <QStandardItem>

iALabeledVolumeVis::iALabeledVolumeVis(iATransferFunction* tf, iAObjectsData const* data, double const* bounds) :
	iAObjectVis(data),
	m_tf(tf)
{
	std::copy(bounds, bounds + 6, m_bounds);
}

void iALabeledVolumeVis::renderSelection( std::vector<size_t> const & sortedSelInds, int /*classID*/, QColor const & classColor, QStandardItem* activeClassItem )
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

	double prevRange[2];
	m_tf->colorTF()->GetRange(prevRange);

	// clear existing points
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();
	m_tf->opacityTF()->ClampingOff();
	m_tf->colorTF()->ClampingOff();
	m_tf->opacityTF()->AddPoint( 0, backAlpha, 0.5, 1.0 );
	m_tf->colorTF()->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );

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
				red = SelectedColor.redF();
				green = SelectedColor.greenF();
				blue = SelectedColor.blueF();

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
				m_tf->opacityTF()->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

				if ( hid_isASelection )
				{
					m_tf->colorTF()->AddRGBPoint( hid - 0.5, 1.0, 0.0, 0.0, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid, 1.0, 0.0, 0.0, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}
				else
				{
					m_tf->colorTF()->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}

				if ( previous_hid_isASelection )
					m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, 1.0, 0.0, 0.0, 0.5, 1.0 );
				else
					m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
				break;
			}
			else	// if we are not in a sequence we have to create the last tooth (/\)
			{
				m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

				m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				break;
			}
		}

		if ( next_hid > hid + 1 && !starting )		//Create one single tooth
		{
			m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
		else if ( next_hid == hid + 1 && !starting )	//Creates the beginning of a sequence (/)
		{
			starting = true;
			m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
		}
		else if ( next_hid == hid + 1 && starting )	//Continues the started sequence (-)
		{
			if ( !hid_isASelection && previous_hid_isASelection )
			{
				m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

				m_tf->opacityTF()->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
			}
			else if ( hid_isASelection && !previous_hid_isASelection )
			{
				m_tf->colorTF()->AddRGBPoint( hid - 0.5, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

				m_tf->opacityTF()->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
			}
		}
		else if ( next_hid > hid + 1 && starting )	//  (\)
		{
			starting = false;

			m_tf->opacityTF()->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
			m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

			if ( previous_hid_isASelection )
				m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
			else
				m_tf->colorTF()->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

			m_tf->colorTF()->AddRGBPoint( hid - 0.5, red, green, blue, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
		prev_hid = hid;
	}

	if ( hid < static_cast<size_t>(m_data->m_table->GetNumberOfRows()) )	// Creates the very last points (for all objects)  if it's not created yet
	{
		m_tf->opacityTF()->AddPoint(m_data->m_table->GetNumberOfRows() + 0.3, backAlpha, 0.5, 1.0 );
		m_tf->colorTF()->AddRGBPoint(m_data->m_table->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
	}
	m_tf->ensureValidity(prevRange);
	emit renderRequired();
}

void iALabeledVolumeVis::renderSingle(IndexType selectedObjID, int /*classID*/, QColor const & classColor, QStandardItem* activeClassItem )
{
	double prevRange[2];
	m_tf->colorTF()->GetRange(prevRange);
	int itemL = activeClassItem->rowCount();
	double red   = classColor.redF(),
		   green = classColor.greenF(),
		   blue  = classColor.blueF(),
		   alpha = 0.5,
		   backAlpha = 0.0,
		   backRGB[3] = { 0.0, 0.0, 0.0 };

	// clear existing points
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();

	// set background opacity and color with clamping off
	m_tf->opacityTF()->ClampingOff();
	m_tf->colorTF()->ClampingOff();
	m_tf->opacityTF()->AddPoint(0, backAlpha);
	m_tf->colorTF()->AddRGBPoint(0, backRGB[0], backRGB[1], backRGB[2]);
	if (selectedObjID > 0 ) // for single object selection
	{
		if ( (selectedObjID - 1) >= 0)
		{
			m_tf->opacityTF()->AddPoint(selectedObjID - 0.5, backAlpha);
			m_tf->opacityTF()->AddPoint(selectedObjID - 0.49, alpha);
			m_tf->colorTF()->AddRGBPoint(selectedObjID - 0.5, backRGB[0], backRGB[1], backRGB[2]);
			m_tf->colorTF()->AddRGBPoint(selectedObjID - 0.49, red, green, blue);
		}
		m_tf->opacityTF()->AddPoint(selectedObjID, alpha);
		m_tf->colorTF()->AddRGBPoint(selectedObjID, red, green, blue);
		if ((selectedObjID + 1) <= m_data->m_table->GetNumberOfRows())
		{
			m_tf->opacityTF()->AddPoint(selectedObjID + 0.3, backAlpha);
			m_tf->opacityTF()->AddPoint(selectedObjID + 0.29, alpha);
			m_tf->colorTF()->AddRGBPoint(selectedObjID + 0.3, backRGB[0], backRGB[1], backRGB[2]);
			m_tf->colorTF()->AddRGBPoint(selectedObjID + 0.29, red, green, blue);
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
					m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
					m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
				else
				{
					m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
					m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
			}

			//Create one single tooth
			if ( next_hid > hid + 1 && !starting )
			{
				m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && !starting )
			{
				starting = true;
				m_tf->opacityTF()->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && starting )
				continue;
			else if ( next_hid > hid + 1 && starting )
			{
				starting = false;
				m_tf->opacityTF()->AddPoint( hid, alpha, 0.5, 1.0 );
				m_tf->opacityTF()->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				m_tf->colorTF()->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
		}

		if ( hid < m_data->m_table->GetNumberOfRows() )
		{
			m_tf->opacityTF()->AddPoint(m_data->m_table->GetNumberOfRows() + 0.3, backAlpha, 0.5, 1.0 );
			m_tf->colorTF()->AddRGBPoint(m_data->m_table->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
	}
	m_tf->ensureValidity(prevRange);
	emit renderRequired();
}

void iALabeledVolumeVis::multiClassRendering( QList<QColor> const & classColors, QStandardItem* rootItem, double alpha )
{
	double prevRange[2];
	m_tf->colorTF()->GetRange(prevRange);
	double backAlpha = 0.00005;
	double backRGB[3];
	backRGB[0] = classColors.at(0).redF();
	backRGB[1] = classColors.at(0).greenF();
	backRGB[2] = classColors.at(0).blueF();

	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;

	// clear existing points
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();

	// set background opacity and color
	m_tf->opacityTF()->ClampingOff();
	m_tf->colorTF()->ClampingOff();
	m_tf->opacityTF()->AddPoint(0, backAlpha);
	m_tf->colorTF()->AddRGBPoint(0, backRGB[0], backRGB[1], backRGB[2]);

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
					m_tf->opacityTF()->AddPoint(hid, alpha, 0.5, 1.0);
					m_tf->opacityTF()->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
					m_tf->colorTF()->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
					m_tf->colorTF()->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					break;
				}
				else
				{
					m_tf->opacityTF()->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
					m_tf->opacityTF()->AddPoint(hid, alpha, 0.5, 1.0);
					m_tf->opacityTF()->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
					m_tf->colorTF()->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					m_tf->colorTF()->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
					m_tf->colorTF()->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
					break;
				}
			}

			//Create one single tooth
			if (next_hid > hid + 1 && !starting)
			{
				m_tf->opacityTF()->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
				m_tf->opacityTF()->AddPoint(hid, alpha, 0.5, 1.0);
				m_tf->opacityTF()->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
			}
			else if (next_hid == hid + 1 && !starting)
			{
				starting = true;
				m_tf->opacityTF()->AddPoint(hid - 0.5, backAlpha, 0.5, 1.0);
				m_tf->opacityTF()->AddPoint(hid, alpha, 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
			}
			else if (next_hid == hid + 1 && starting)
				continue;

			else if (next_hid > hid + 1 && starting)
			{
				starting = false;
				m_tf->opacityTF()->AddPoint(hid, alpha, 0.5, 1.0);
				m_tf->opacityTF()->AddPoint(hid + 0.3, backAlpha, 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid, red, green, blue, 0.5, 1.0);
				m_tf->colorTF()->AddRGBPoint(hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
			}
		}
	}
	m_tf->opacityTF()->AddPoint(m_data->m_table->GetNumberOfRows()+0.3, backAlpha, 0.5, 1.0);
	m_tf->colorTF()->AddRGBPoint(m_data->m_table->GetNumberOfRows() + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0);
	m_tf->ensureValidity(prevRange);
	emit renderRequired();
}

void iALabeledVolumeVis::renderOrientationDistribution( vtkImageData* oi )
{
	double prevRange[2];
	m_tf->colorTF()->GetRange(prevRange);
	double backRGB[3];
	backRGB[0] = 0.0; backRGB[1] = 0.0; backRGB[2] = 0.0;
	double backAlpha = 0.0, alpha = 0.5;
	// clear existing points
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();
	m_tf->opacityTF()->AddPoint( 0, backAlpha );
	m_tf->colorTF()->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2] );

	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID )
	{
		QColor color = getOrientationColor( oi, objID );
		m_tf->opacityTF()->AddPoint( objID + 1, alpha );
		m_tf->colorTF()->AddRGBPoint( objID + 1, color.redF(), color.greenF(), color.blueF() );
	}
	m_tf->ensureValidity(prevRange);
	emit renderRequired();
}

void iALabeledVolumeVis::renderLengthDistribution( vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range )
{
	double prevRange[2];
	m_tf->colorTF()->GetRange(prevRange);
	// clear existing points
	m_tf->opacityTF()->RemoveAllPoints();
	m_tf->colorTF()->RemoveAllPoints();
	m_tf->opacityTF()->AddPoint(0, 0.0);
	m_tf->colorTF()->AddRGBPoint(0, 0.0, 0.0, 0.0);

	for (IndexType objID = 0; objID < m_data->m_table->GetNumberOfRows(); ++objID )
	{
		double ll = m_data->m_table->GetValue(objID, m_data->m_colMapping->value(iACsvConfig::Length)).ToDouble();
		QColor color = getLengthColor( ctFun, objID );

		if ( filterID == iAObjectType::Fibers )
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 1.0 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 1 ) + halfInc && ll < extents->GetValue( 2 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 2 ) + halfInc && ll < extents->GetValue( 5 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.015 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 7 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 1.0 );
			}
		}
		else
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 2 ) + halfInc )
			{
				m_tf->opacityTF()->AddPoint( objID + 1 - 0.5, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1 + 0.3, 0.0 );
				m_tf->opacityTF()->AddPoint( objID + 1, 0.5 );
			}
		}
		m_tf->colorTF()->AddRGBPoint( objID + 1, color.redF(), color.greenF(), color.blueF() );
		m_tf->colorTF()->AddRGBPoint( objID + 1 - 0.5, color.redF(), color.greenF(), color.blueF() );
		m_tf->colorTF()->AddRGBPoint( objID + 1 + 0.3, color.redF(), color.greenF(), color.blueF() );
	}
	m_tf->ensureValidity(prevRange);
	emit renderRequired();
}

double const * iALabeledVolumeVis::bounds()
{
	return m_bounds;
}

std::shared_ptr<iAObjectVisActor> iALabeledVolumeVis::createActor(vtkRenderer* ren)
{
	auto result = std::make_shared<iAObjectVisActor>(ren);
	connect(this, &iAObjectVis::dataChanged, result.get(), &iAObjectVisActor::updateRenderer);
	connect(this, &iAObjectVis::renderRequired, result.get(), &iAObjectVisActor::updateRenderer);
	return result;
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QObject>

#include <cstddef>    // for size_t
#include <vector>

class iALookupTable;
class iAQSplom;

class vtkTable;

class QColor;
class QDockWidget;

class iAFeatureScoutSPLOM: public QObject
{
	Q_OBJECT
public:
	iAFeatureScoutSPLOM();
	void initScatterPlot(vtkTable* csvTable, std::vector<char> const & columnVisibility);  //!< initialize SPLOM
	void updateColumnVisibility(std::vector<char> const & columnVisibility); //!< update column visibility
	void setParameterVisibility(size_t paramIndex, bool visible);      //!< matrix proxy method
	void setDotColor(QColor const & color);                            //!< set color for all SPLOM dots (TODO: move range calculations to iASplomData!)
	void setFilter(int classID);                                       //!< specify a filter on class column
	void multiClassRendering(QList<QColor> const & colors);            //!< colors each dot according to its class
	void setFilteredSelection(std::vector<size_t> const & selection);  //!< set filtered selection in SPLOM
	void classAdded(int classID);                                      //!< notifies SPLOM that class was added of current selection
	void classDeleted(int classID);                                    //!< notifies SPLOM that class was deleted
	void changeClass(size_t objID, int classID);                       //!< set class of single object to given ID
	void classesChanged();
	std::vector<size_t> getFilteredSelection() const;                  //!< proxy for getFilteredSelection in SPLOM
	bool isShown() const;
	void clearSelection();
	void enableSelection(bool enable);
	QWidget* matrixWidget();                                           //!< return the GUI widget displaying the actual matrix
signals:
	void selectionModified(std::vector<size_t>);
	void parameterVisibilityChanged(size_t paramIndex, bool visible);
	void addClass();
	void renderLUTChanges(QSharedPointer<iALookupTable> lut, size_t colInd);
private:
	iAQSplom * matrix;
	bool selectionEnabled;
private slots:
	void lookupTableChanged();
};

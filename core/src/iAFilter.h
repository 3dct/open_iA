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
#pragma once

#include "open_iA_Core_export.h"

#include "iAValueType.h"

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <QVector>

class iAAttributeDescriptor;
class iAConnector;
class iALogger;
class iAProgress;

typedef QSharedPointer<iAAttributeDescriptor> pParameter;

//! Base class for image filters
//! Derived classes should:
//!     - fill the m_parameters vector in their constructor
//!     - override the SetParameters method to transfer the parameters to the actual algorithm
//!     - override the Run method to perform the actual calculations
//!       on m_inImg and (allocate and) store the result in m_outImg
class open_iA_Core_API iAFilter
{
public:
	//! Constructor initializing name, category and description of the filter
	//! @param name The name of the filter. It will be used in its menu entry,
	//!     as part of the title of the result window, and as title for the
	//!     parameter selection dialog.
	//! @param category The filter category. It will be used for putting the
	//!     filter into a fitting submenu and for categorizing the filter
	//!     parameters when storing them in the registry you can use the slash
	//!     character ("/") to specify subcategories.
	//!     A category of "Segmentation/Global Thresholding" for example would put
	//      a filter in the "Global Thresholding" submenu of the "Segmentation"
	//!     menu, which in turn is added as submenu to the Filter menu.
	//!     When left empty, the filter will be added directly in the Filter menu
	//! @param description An (optional) description of the filter algorithm, and
	//!     ideally its settings. Can contain HTML (e.g. links)
	//! @param requiredInputs The number of inputs required for this filter;
	//!     by default, filters are assumed to require exactly one input image; you
	//!     can override the number of inputs required for your filter with this parameter
	//! @param outputCount the number of outputs this filter creates. Set it to 0 to
	//!     disable image output. If you don't know this yet at the time of creating the
	//!     filter (because it for example depends on the number of input images or the
	//!     parameters), you can always adapt it at a later point (e.g. during
	//!     iAFilter::Run) by calling SetOutputCount; but if you have some image output,
	//!     make sure that you leave it at the default value of 1 or set it to some value
	//!     other than zero, because setting it to zero has immediate side effects (e.g.
	//!     not opening a result window if configured, in  the GUI).
	iAFilter(QString const & name, QString const & category, QString const & description = "",
		unsigned int requiredInputs = 1, unsigned int outputCount = 1);
	//! Destructor
	virtual ~iAFilter();
	//! Retrieve the filter name
	QString Name() const;
	//! Retrieve the filter category (if sub-categories were specified, this only
	//! returns the first one)
	QString Category() const;
	//! Retrieve the full category string (just as specified in the constructor)
	QString FullCategory() const;
	//! Retrieve the filter description
	QString Description() const;
	//! Retrieve a list of the filter parameters
	QVector<pParameter> const & Parameters() const;
	//! Used internally by the filter runner to set up the resources required in the
	//! filter
	bool SetUp(QVector<iAConnector*> const & cons, iALogger* logger, iAProgress* progress);
	//! Check whether the filter can be run with the given parameters. If
	//! you need to perform special checks on your parameters, override this
	//! method. The standard implementation here just checks parameters with
	//! Discrete and Continuous value type for minimum and maximum values.
	//! @param parameters the generic list of parameters that the filter will
	//!     be called with
	//! @return true if the given parameters are acceptable for the filter, false
	//!     otherwise
	//! TODO: also allow to check input files here (e.g. for AddImage to check
	//!     if input images are all of the same type!
	virtual bool CheckParameters(QMap<QString, QVariant> & parameters);
	//! Initialize and run the filter
	//! @param parameters the map of parameters to use in this specific filter run
	void Run(QMap<QString, QVariant> const & parameters);
	//! Adds the description of a parameter to the filter
	//! @param name the parameter's name
	//! @param valueType the type of value this parameter can have
	//! @param defaultValue the default value of the parameter; for Categorical
	//!     valueTypes, this should be the list of possible values
	//! @param min the minimum value this parameter can have (inclusive).
	//! @param max the maximum value this parameter can have (inclusive)
	void AddParameter(
		QString const & name, iAValueType valueType,
		QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(),
		double max = std::numeric_limits<double>::max());
	//! Returns the number of image inputs required by this filter.
	//! for typical image filters, this returns 1.
	//! @return the number of images required as input
	unsigned int RequiredInputs() const;
	//! Returns the number of output images returned by this filter.
	//! for typical image filters, this returns 1. The filter can modify
	//! this through SetOutputCount.
	unsigned int OutputCount() const;
	//! Sets the output count. Note that at this point, it is not supported
	//! (or at least, it might cause unintended side effects) to switch from
	//! a non-zero value to zero via SetOutputCount or the other way round; this
	//! will also cause a warning in the debug console. See also the note for
	//! the outputCount parameter in the Constructor.
	void SetOutputCount(unsigned int outputCount);
	//! input/output connectors
	QVector<iAConnector*> & Connectors();
	//! returns the number of input channels from the first input image
	unsigned int FirstInputChannels() const;
	//! sets the first input channels
	void SetFirstInputChannels(unsigned int c);
	//! adds an output value
	void AddOutputValue(QString const & name, QVariant value);
	//! retrieve a list of output values
	QVector<QPair<QString, QVariant> > const & OutputValues() const;
	//! retrieve a list of names of the output values that this filter can produce
	QVector<QString> const & OutputValueNames() const;
	//! adds an output value name
	void AddOutputValue(QString const & name);
	//! Adds some message to the targeted output place for this filter
	//! Typically this will go into the log window of the result MdiChild
	//! @param msg the message to print
	void AddMsg(QString msg);
protected:
	//! An accessor to the image to be processed by the filter
	iAConnector* m_con;
	//! An accessor to all input images (if more than one input is required
	QVector<iAConnector*> m_cons;
	//! The class that is watched for progress. Typically you will call
	//! m_progress->Observe(someItkFilter) to set up the progress observation
	iAProgress* m_progress;
	//! The logger
	iALogger* m_log;
private:
	//! The actual implementation of the filter
	//! @param parameters the map of parameters to use in this specific filter run
	virtual void PerformWork(QMap<QString, QVariant> const & parameters) = 0;
	QVector<pParameter> m_parameters;
	QVector<QPair<QString, QVariant> > m_outputValues;
	QVector<QString> m_outputValueNames;
	QString m_name, m_category, m_description;
	unsigned int m_requiredInputs, m_outputCount, m_firstInputChannels;
};

//! Convenience Macro for creating the static Create method for your filter
#define IAFILTER_CREATE(FilterName) \
QSharedPointer<FilterName> FilterName::Create() \
{ \
	return QSharedPointer<FilterName>(new FilterName()); \
}

#define IAFILTER_DEFAULT_CLASS(FilterName) \
class FilterName : public iAFilter \
{ \
public: \
	static QSharedPointer<FilterName> Create(); \
private: \
	void PerformWork(QMap<QString, QVariant> const & parameters) override; \
	FilterName(); \
};

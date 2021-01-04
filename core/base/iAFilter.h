/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "base_export.h"

#include "iAAbortListener.h"
#include "iAAttributes.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAConnector;
class iALogger;
class iAProgress;

class vtkImageData;

class QVariant;

//! Base class for image filters.
//! Derived classes should:
//!     - fill the m_parameters vector in their constructor
//!     - override the SetParameters method to transfer the parameters to the actual algorithm
//!     - override the Run method to perform the actual calculations
//!       on m_inImg and (allocate and) store the result in m_outImg
class base_API iAFilter: public iAAbortListener
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
	//! Virtual destructor because of inheritance (mostly to avoid warnings about missing virtual destructor).
	virtual ~iAFilter();
	//! Retrieve the filter name.
	QString name() const;
	//! Retrieve the "base" filter category.
	//! If sub-categories were specified, this only returns the first one.
	QString category() const;
	//! Retrieve the full category string (just as specified in the constructor).
	QString fullCategory() const;
	//! Retrieve the filter description.
	QString description() const;
	//! Retrieve a list of the filter parameters.
	iAAttributes const & parameters() const;
	//! Set the logger to be used for status output / error messages.
	void setLogger(iALogger* logger);
	//! Set the facility for progress reporting.
	void setProgress(iAProgress* progress);
	//! Check whether the filter can be run with the given parameters. If
	//! you need to perform special checks on your parameters, override this
	//! method. The standard implementation here just checks parameters with
	//! Discrete and Continuous value type for minimum and maximum values.
	//! @param parameters the generic list of parameters that the filter will
	//!     be called with
	//! @return true if the given parameters are acceptable for the filter, false
	//!     otherwise
	virtual bool checkParameters(QMap<QString, QVariant> & parameters);
	//! Clears the list of input images to this filter.
	//! Call this in case you are re-using a filter already called before,
	//! and you want to call it with new input images
	void clearInput();
	//! Adds an image as input.
	void addInput(iAConnector* con, QString const & fileName);
	//! Initialize and run the filter.
	//! @param parameters the map of parameters to use in this specific filter run
	bool run(QMap<QString, QVariant> const & parameters);
	//! Adds the description of a parameter to the filter.
	//! @param name the parameter's name
	//! @param valueType the type of value this parameter can have
	//! @param defaultValue the default value of the parameter; for Categorical
	//!     valueTypes, this should be the list of possible values
	//! @param min the minimum value this parameter can have (inclusive).
	//! @param max the maximum value this parameter can have (inclusive)
	void addParameter(
		QString const & name, iAValueType valueType,
		QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(),
		double max = std::numeric_limits<double>::max());
	//! Returns the number of image inputs required by this filter.
	//! for typical image filters, this returns 1.
	//! @return the number of images required as input
	int requiredInputs() const;
	//! input/output connectors.
	QVector<iAConnector*> const & input() const;
	QVector<iAConnector*> const & output() const;
	QVector<QString> const & fileNames() const;

	itk::ImageIOBase::IOComponentType inputPixelType() const;
	//! returns the number of input channels from the first input image.
	unsigned int firstInputChannels() const;
	//! sets the first input channels.
	void setFirstInputChannels(unsigned int c);
	//! retrieve a list of output values.
	QVector<QPair<QString, QVariant> > const & outputValues() const;
	//! Retrieve a list of names of the output values that this filter can produce.
	QVector<QString> const & outputValueNames() const;
	//! Adds an output value name.
	//! Call this method in the constructor of derived classes,
	//! to let the filter know which output values will later be added
	//! via the addOutputValue method with two parameters
	//! @param name the name of the output value
	void addOutputValue(QString const & name);
	//! Adds an output value.
	//! @param name the name of the output value
	//! @param value the actual output value
	void addOutputValue(QString const & name, QVariant value);
	//! @{ Adds an output image.
	//! @param img output image from the filter
	void addOutput(itk::ImageBase<3> * img);
	void addOutput(vtkSmartPointer<vtkImageData> img);
	//! @}
	//! Sets the mesh output of this filter.
	void setPolyOutput(vtkSmartPointer<vtkPolyData> poly);
	//! Retrieves output mesh if existing.
	vtkSmartPointer<vtkPolyData> polyOutput() const;
	//! The planned number of outputs the filter will produce.
	int outputCount() const;
	//! Adds some message to the targeted output place for this filter.
	//! Typically this will go into the log window of the result MdiChild
	//! @param msg the message to print
	void addMsg(QString const & msg);
	//! Retrieve the progress reporting object for this filter.
	iAProgress* progress();
	//! Retrieve the logger used for log messages emitted by the filter.
	iALogger* logger();
	//! Retrieve the name of the input image with given index.
	QString inputName(unsigned int i) const;
	//! Retrieve the name of the output image with given index.
	QString outputName(unsigned int i, QString defaultName=QString()) const;
	//! Abort the filter.
	void abort() override;
	//! Whether the filter supports aborting
	virtual bool canAbort() const;
protected:
	//! Set the name of the input with the given index.
	void setInputName(unsigned int i, QString const & name);

	//! Set the name of the output with the given index.
	void setOutputName(unsigned int i, QString const & name);

private:
	//! The actual implementation of the filter.
	//! @param parameters the map of parameters to use in this specific filter run
	virtual void performWork(QMap<QString, QVariant> const & parameters) = 0;
	//! Clears the output values.
	void clearOutput();

	//! input images.
	QVector<iAConnector*> m_input;
	//! file names of the input images.
	QVector<QString> m_fileNames;
	//! output images (if any).
	QVector<iAConnector*> m_output;
	//! output mesh (if any).
	vtkSmartPointer<vtkPolyData> m_outputMesh;
	//! output values (if any).
	QVector<QPair<QString, QVariant> > m_outputValues;
	//! The class that is watched for progress.
	//! Typically you will call m_progress->observe(someItkFilter) to set up the progress observation
	iAProgress* m_progress;
	//! The logger.
	iALogger* m_log;
	//! Describes the parameters of the algorithm.
	iAAttributes m_parameters;
	//! Names for the output values of the algorithm.
	QVector<QString> m_outputValueNames;
	//! Names for the input images of the algorithm.
	QMap<unsigned int, QString> m_inputNames;
	//! Names for the output images of the algorithm.
	QMap<unsigned int, QString> m_outputNames;
	//! Name, category and description of the filter.
	QString m_name, m_category, m_description;
	//! Number of input images required by the filter.
	int m_requiredInputs;
	//! Number of output images produced by the filter.
	int m_outputCount;
	//! In case this filter requires two "kinds" of inputs, this marks the number of inputs belonging to the first kind.
	int m_firstInputChannels;
};

//! Convenience Macro for creating the static Create method for your filter
#define IAFILTER_CREATE(FilterName) \
QSharedPointer<FilterName> FilterName::create() \
{ \
	return QSharedPointer<FilterName>(new FilterName()); \
}

#define IAFILTER_DEFAULT_CLASS(FilterName) \
class FilterName : public iAFilter \
{ \
public: \
	static QSharedPointer<FilterName> create(); \
private: \
	void performWork(QMap<QString, QVariant> const & parameters) override; \
	FilterName(); \
};

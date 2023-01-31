/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAbase_export.h"

#include "iAAbortListener.h"
#include "iAAttributes.h"
#include "iAITKIO.h"

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include <memory>

class iADataSet;
class iAImageData;
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
class iAbase_API iAFilter: public iAAbortListener
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
	//!     a filter in the "Global Thresholding" submenu of the "Segmentation"
	//!     menu, which in turn is added as submenu to the Filter menu.
	//!     When left empty, the filter will be added directly in the Filter menu
	//! @param description An (optional) description of the filter algorithm, and
	//!     ideally its settings. Can contain HTML (e.g. links)
	//! @param requiredImages The number of (image) inputs required for this filter;
	//!     by default, filters are assumed to require exactly one input image; you
	//!     can override the number of inputs required for your filter with this parameter
	//!     for filters that require mesh input, see the setRequiredMeshInputs method;
	//!     set this to 0 if the filter requires no images in addition to the mesh
	//! @param outputCount the number of outputs this filter creates. Set it to 0 to
	//!     disable image output. If you don't know this yet at the time of creating the
	//!     filter (because it for example depends on the number of input images or the
	//!     parameters), you can always adapt it at a later point (e.g. during
	//!     iAFilter::Run) by calling SetOutputCount; but if you have some image output,
	//!     make sure that you leave it at the default value of 1 or set it to some value
	//!     other than zero, because setting it to zero has immediate side effects (e.g.
	//!     not opening a result window if configured, in  the GUI).
	//! @param supportsAbort whether the filter supports aborting. The typical way of aborting
	//!     is by checking the boolean flag accessible via isAbort, but derived classes
	//!     can implement custom abort listeners through overriding the abort() function
	iAFilter(QString const & name, QString const & category, QString const & description = "",
		unsigned int requiredImages = 1, unsigned int outputCount = 1, bool supportsAbort = false);
	//! virtual destructor, to enable proper destruction in derived classes and to avoid warnings
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
	//! if required, adapt (loaded/default) parameters to image
	//! only used from GUI for the moment
	//! if feature is implemented where parameters can be omitted (on command line),
	//! this could later also be used to provide useful defaults for parameters that need to adapt to input image
	virtual void adaptParametersToInput(QVariantMap& parameters, std::map<size_t, std::shared_ptr<iADataSet>> const & dataSets);
	//! Check whether the filter can be run with the given parameters. If
	//! you need to perform special checks on your parameters, override this
	//! method. The standard implementation here just checks parameters with
	//! Discrete and Continuous value type for minimum and maximum values.
	//! @param parameters the generic list of parameters that the filter will
	//!     be called with
	//! @return true if the given parameters are acceptable for the filter, false
	//!     otherwise
	virtual bool checkParameters(QVariantMap const & parameters);
	//! the default check for a single parameter descriptor & value combination
	bool defaultParameterCheck(QSharedPointer<iAAttributeDescriptor> param, QVariant const& paramValue);
	//! Clears the list of input images to this filter.
	//! Call this in case you are re-using a filter already called before,
	//! and you want to call it with new input images
	void clearInput();
	//! @{
	//! Adds a dataSet as input.
	void addInput(std::shared_ptr<iADataSet> con);
	//! @}
	//! Initialize and run the filter.
	//! @param parameters the map of parameters to use in this specific filter run
	bool run(QVariantMap const & parameters);
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
	//! Returns the number of input images required by this filter.
	//! For typical image filters, this returns 1.
	//! @return the number of images required as input
	unsigned int requiredImages() const;
	//! Returns the number of input meshes required by this filter.
	unsigned int requiredMeshes() const;

	//! Get input with the given index.
	//! @see inputCount for the number of available inputs
	std::shared_ptr<iADataSet> input(size_t idx) const;
	//! convenience function returning input datasets converted to image data (with error checks)
	iAImageData const* imageInput(size_t idx) const;
	//! Get the number of available inputs (that were already added to the filter).
	size_t inputCount() const;

	//! get output with the given index.
	//! @see outputCount for the number of available inputs
	std::shared_ptr<iADataSet> output(size_t idx) const;
	//! convenience function returning output datasets converted to image data (with error checks)
	iAImageData* imageOutput(size_t idx) const;
	//! Get the number of available outputs.
	//! Only set after the filter has run and the outputs are actually produced!
	//! see plannedOutputCount() for the number of probable outcomes before the filter has run!
	size_t finalOutputCount() const;
	//! return the full list of outputs
	std::vector<std::shared_ptr<iADataSet>> outputs();
	//! return the ITK pixel type of the image in the first given input dataset
	//! (no check is performed whether that dataset actually contains an image!)
	iAITKIO::ScalarType inputScalarType() const;
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
	void addOutput(itk::ImageBase<3>* img);
	void addOutput(vtkSmartPointer<vtkImageData> img);
	//! @}
	//! adds an output dataset
	void addOutput(std::shared_ptr<iADataSet> dataSet);
	//! The planned number of outputs the filter will produce.
	unsigned int plannedOutputCount() const;
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
	QString outputName(unsigned int i) const;
	//! Abort the filter.
	void abort() override;
	//! Whether the filter supports aborting
	bool canAbort() const;
	//! Whether the filter was aborted by the user
	bool isAborted() const;

protected:
	//! Set the name of the input with the given index.
	void setInputName(unsigned int i, QString const & name);

	//! Set the name of the output with the given index.
	void setOutputName(unsigned int i, QString const & name);

	//! Set the number of required mesh inputs (0 by default)
	void setRequiredMeshInputs(unsigned int i);

	//! "Writable" list of the filter parameters.
	iAAttributes & paramsWritable();

private:
	//! The actual implementation of the filter.
	//! @param parameters the map of parameters to use in this specific filter run
	virtual void performWork(QVariantMap const & parameters) = 0;
	//! Clears the output values.
	void clearOutput();

	//! input images.
	std::vector<std::shared_ptr<iADataSet>> m_input;
	//! output images (if any).
	// TODO: make unique_ptr: -> compile error: `attempting to reference deleted function` in iAFilterRegistry...
	std::vector<std::shared_ptr<iADataSet>> m_output;
	//! output mesh (if any).
	vtkSmartPointer<vtkPolyData> m_outputMesh;
	//! output values (if any).
	QVector<QPair<QString, QVariant> > m_outputValues;
	//! The class that is watched for progress.
	//! Typically you will call m_progress->observe(someItkFilter) to set up the progress observation
	std::unique_ptr<iAProgress> m_progress;
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
	unsigned int m_requiredImages;
	//! Number of output images produced by the filter.
	unsigned int m_outputCount;
	//! Number of input meshes required by the filter (default: 0)
	unsigned int m_requiredMeshes;
	//! In case this filter requires two "kinds" of inputs, this marks the number of inputs belonging to the first kind.
	unsigned int m_firstInputChannels;
	//! flag storing whether the filter supports aborting
	bool m_canAbort;
	//! flag storing whether the filter was aborted by the user
	bool m_isAborted = false;
};

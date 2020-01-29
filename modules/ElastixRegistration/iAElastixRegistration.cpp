#include "iAElastixRegistration.h"

#include "defines.h" // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include <itkDerivativeImageFilter.h>


//#include "itkParameterFileParser.h"
#include "itkImage.h"
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkVectorIndexSelectionCastImageFilter.h>
#include <itkVectorImage.h>

#include<qtemporarydir.h>


template <class InPixelType>
void extractChannels(typename itk::VectorImage<InPixelType, DIM>::Pointer vectorImg, iAFilter* filter, QString dir, QString Name, QStringList Dimension, bool loadTransformixResult)
{
	typedef itk::VectorImage<InPixelType, DIM> VectorImageType;
	typedef itk::Image<InPixelType, DIM> OutImageType;
	typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, OutImageType> IndexSelectionType;

	typedef itk::Image<float, DIM> OutputImageType;
	typedef  itk::ImageFileWriter< OutputImageType  > WriterType;


	for (unsigned int p = 0; p < vectorImg->GetVectorLength(); ++p)
	{
		auto indexSelectionFilter = IndexSelectionType::New();
		indexSelectionFilter->SetIndex(p);
		indexSelectionFilter->SetInput(vectorImg);
		indexSelectionFilter->Update();

		QString path = dir + "/"+ Name + "_" + Dimension[p] + ".mhd";

		WriterType::Pointer imageWriter = WriterType::New();
		imageWriter->SetFileName(path.toStdString());
		imageWriter->SetInput(dynamic_cast<OutputImageType *>(indexSelectionFilter->GetOutput()));
		imageWriter->Update();

		if (loadTransformixResult) {
			filter->addOutput(indexSelectionFilter->GetOutput());
		}
	}
}

void writeDeformationImage(iAFilter* filter, QString dirname, bool loadTransformixResult) {

	QString deformationImagePath = dirname + "/deformationField.mhd";
	typedef itk::VectorImage<float, DIM> deformationInputImageType;
	typedef itk::ImageFileReader<deformationInputImageType> deformationReaderType;

	//split deformation

	auto deformationImage = deformationReaderType::New();
	deformationImage->SetFileName(deformationImagePath.toStdString());
	deformationImage->Update();



	extractChannels<float>(deformationImage->GetOutput(), filter, dirname, "Deformationfield", { "X", "y", "Z" }, loadTransformixResult);
}

void writeFullJacobian(iAFilter* filter, QString dirname, bool loadTransformixResult) {
	
	QString fullJacobianImagePath = dirname + "/fullSpatialJacobian.mhd";
	//Split jacobian

	typedef itk::VectorImage<float, DIM> deformationInputImageType;
	typedef itk::ImageFileReader<deformationInputImageType> deformationReaderType;

	auto fullJacobianImage = deformationReaderType::New();
	fullJacobianImage->SetFileName(fullJacobianImagePath.toStdString());
	fullJacobianImage->Update();

	extractChannels<float>(fullJacobianImage->GetOutput(), filter, dirname, "Jacobian", { "11", "12", "13", "21", "22", "23", "31", "32", "33" }, loadTransformixResult);
}

void createOutput(iAFilter* filter, QString dirname, bool tranformixActive, bool loadTransformixResult) {
	typedef itk::Image<float, DIM> InputImageType;
	typedef itk::ImageFileReader<InputImageType> ReaderType;


	QString resulImagePath = dirname + "/result.0.mhd";
	QString jacobianImagePath = dirname + "/spatialJacobian.mhd";
	
	



	ReaderType::Pointer resulImage = ReaderType::New();
	resulImage->SetFileName(resulImagePath.toStdString());
	resulImage->Update();
	filter->addOutput(resulImage->GetOutput());
	
	
	if (tranformixActive) {
		ReaderType::Pointer jacobianImage = ReaderType::New();
		jacobianImage->SetFileName(jacobianImagePath.toStdString());
		jacobianImage->Update();
		filter->addOutput(jacobianImage->GetOutput());

		writeDeformationImage(filter, dirname, loadTransformixResult);

		writeFullJacobian(filter, dirname, loadTransformixResult);
	}
}


void runElastix(QString dirname, QString fixedImagePath, QString movingImagePath, QString parameterPath, QString executablePath, int timeout=300000) {
	QStringList argumentsElastix;

	argumentsElastix.append("-f");
	argumentsElastix.append(fixedImagePath);

	argumentsElastix.append("-m");
	argumentsElastix.append(movingImagePath);

	argumentsElastix.append("-p");
	argumentsElastix.append(parameterPath);

	argumentsElastix.append("-out");
	argumentsElastix.append(dirname);



	QProcess elastix;
	elastix.setProgram(executablePath + "/elastix.exe");
	elastix.setArguments(argumentsElastix);
	elastix.setProcessChannelMode(QProcess::MergedChannels);
	elastix.start();

	if (!elastix.waitForStarted()) {
		throw  std::runtime_error("Error Start Elastix");
	}

	if (!elastix.waitForFinished(timeout)) {
		throw  std::runtime_error("Error Execute Finish");
	}
}


void runTransformix(QString dirname, QString executablePath, int timeout = 300000) {
	QString pathParameterfile = dirname + "/TransformParameters.0.txt";

	QStringList argumentsTransformix;

	argumentsTransformix.append("-jac");
	argumentsTransformix.append("all");

	argumentsTransformix.append("-jacmat");
	argumentsTransformix.append("all");

	argumentsTransformix.append("-def");
	argumentsTransformix.append("all");

	argumentsTransformix.append("-out");
	argumentsTransformix.append(dirname);

	argumentsTransformix.append("-tp");
	argumentsTransformix.append(pathParameterfile);

	QProcess tranformix;
	tranformix.setProgram(executablePath + "/transformix.exe");
	tranformix.setArguments(argumentsTransformix);
	tranformix.setProcessChannelMode(QProcess::MergedChannels);
	tranformix.start();

	if (!tranformix.waitForStarted())
		throw  std::runtime_error("Error Start Transformix");

	if (!tranformix.waitForFinished(timeout))
		throw  std::runtime_error("Error execute Transformix");
}

template<class T> 
void derivative(iAFilter* filter, QMap<QString, QVariant> const & params)
{

	QString pathElastix = params["PathElastix"].toString();
	QString dirname;

	QTemporaryDir dir;

	if (params["Outputdir"].toString() == "") {
		if (dir.isValid()) {
			dirname = dir.path();
			dir.setAutoRemove(true);
		}
	}
	else {
		dirname = params["Outputdir"].toString();

	}

	QString fixedImagePath = dirname + "/fixed.mhd";
	QString movingImagePath = dirname + "/moving.mhd";

	typedef itk::Image<T, DIM> OutputImageType;
	typedef  itk::ImageFileWriter< OutputImageType  > WriterType;
	typename WriterType::Pointer fixedWriter = WriterType::New();
	fixedWriter->SetFileName(fixedImagePath.toStdString());
	fixedWriter->SetInput(dynamic_cast<OutputImageType *>(filter->input()[0]->itkImage()));

	typename WriterType::Pointer movingWriter = WriterType::New();
	movingWriter->SetFileName(movingImagePath.toStdString());
	movingWriter->SetInput(dynamic_cast<OutputImageType *>(filter->input()[1]->itkImage()));

	try
	{
		fixedWriter->Update();
		movingWriter->Update();
	}
	catch (itk::ExceptionObject & /*err*/)
	{
		
		throw  std::runtime_error(("Exception save temp files in directory" + dirname).toStdString());

	}

	int timeoutSec = params["Timeout[sec]"].toInt() * 1000;

	runElastix(dirname, fixedImagePath, movingImagePath, params["ParameterFile"].toString(), pathElastix, timeoutSec);

	if (params["Run Transformix"].toBool()) {
		runTransformix(dirname, pathElastix, timeoutSec);
	}

	try
	{
		createOutput(filter, dirname, params["Run Transformix"].toBool(), params["Load Files"].toBool());
	}
	catch(itk::ImageFileReaderException & /*err*/)
	{

		throw std::runtime_error( "Error reading files please set a Outputdir and check Elastix/Transformix logs for more information");
	}

}




void iAElastixRegistration::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(derivative, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAElastixRegistration)

iAElastixRegistration::iAElastixRegistration() :
	iAFilter("Elastix Registration", "Registration",
		"Makes a registration of two images and computes the deformation matrix and the spatial jacobion using elastix<br/>"
		"To use this filter please download elastix and set Path to elastix executeable as Parameter<br/>"
		"More Information on <a href=\"http://elastix.isi.uu.nl/index.php\">elastix.isi.uu.nl</a><br/>"
		"Outputs"
		"<ul>"
		"<ol>Registration of both images</ol>"
		"<ol>Spatial Jacobian</ol>"
		"<ol>Deformation X</ol>"
		"<ol>Deformation Y</ol>"
		"<ol>Deformation Z</ol>"
		"< / ul>",2,2)
{
	addParameter("ParameterFile", FileNameOpen);
	addParameter("PathElastix", Folder);
	addParameter("Outputdir", Folder,"");
	addParameter("Timeout[sec]", Discrete, 300);

	addParameter("Load Files", Boolean, true);
	addParameter("Run Transformix", Boolean, true);
	


	setInputName(1, "Moving Image");

	setOutputName(0, "Registration of both images");
	setOutputName(1, "Spatial Jacobian");
	setOutputName(2, "Deformation X");
	setOutputName(3, "Deformation Y");
	setOutputName(4, "Deformation Z");

	setOutputName(5, "Jacobian 11");
	setOutputName(6, "Jacobian 12");
	setOutputName(7, "Jacobian 13");
	setOutputName(8, "Jacobian 21");
	setOutputName(9, "Jacobian 22");
	setOutputName(10, "Jacobian 23");
	setOutputName(11, "Jacobian 31");
	setOutputName(12, "Jacobian 32");
	setOutputName(13, "Jacobian 33");
}
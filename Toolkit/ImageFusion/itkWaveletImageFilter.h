#ifndef __itkWaveletImageFilter_h
#define __itkWaveletImageFilter_h

#include "itkImageToImageFilter.h"
//#include "cl/cl.h"
//#include "cl\SDKUtil\CLUtil.hpp"

#define SIGNAL_LENGTH (1 << 10)

namespace itk
{
	using namespace appsdk;

	//Haar Wavelet on GPU
	//https://www10.informatik.uni-erlangen.de/Publications/Talks/2010/Para2010Iceland.pdf
	//good! http://sourceforge.net/p/leasy-wavelet/code/HEAD/tree/Wavelet/test_1d.c
	//installed graphic card: AMD RADEON HD 6450

	template<class TInputImage1, class TInputImage2, class TOutputImage>
	class WaveletImageFilter : public ImageToImageFilter<TInputImage1, TOutputImage>
	{

	public: 
		typedef WaveletImageFilter								Self;
		typedef ImageToImageFilter<TInputImage1, TOutputImage>	Superclass; 
		//typedef GPUImageToImageFilter < TInputImage1, TOutputImage> Superclass;
		typedef SmartPointer<Self>								Pointer; 
		typedef SmartPointer<const Self>						ConstPointer; 
		
		itkNewMacro(Self);
		itkTypeMacro(WaveletImageFilter, ImageToImageFilter);

		typedef TInputImage1							Input1ImageType; 
		typedef typename Input1ImageType::ConstPointer	Input1ImagePointer; 
		typedef typename Input1ImageType::RegionType	Input1ImageRegionType; 
		typedef typename Input1ImageType::PixelType		Input1ImagePixelType; 

		typedef TInputImage2							Input2ImageType;
		typedef typename Input2ImageType::ConstPointer	Input2ImagePointer;
		typedef typename Input2ImageType::RegionType	Input2ImageRegionType;
		typedef typename Input2ImageType::PixelType		Input2ImagePixelType;

		typedef typename NumericTraits<Input1ImagePixelType>::RealType	Input1RealType; 

		typedef TOutputImage                           OutputImageType;
		typedef typename OutputImageType::Pointer      OutputImagePointer;
		typedef typename OutputImageType::RegionType   OutputImageRegionType;
		typedef typename OutputImageType::PixelType    OutputImagePixelType;

		void SetInputImage1(const TInputImage1 *img);
		void SetInputImage2(const TInputImage2 *img2);
		
		itkStaticConstMacro(InputImage1Dimension, unsigned int, TInputImage1::ImageDimension);
		itkStaticConstMacro(InputImage2Dimension, unsigned int, TInputImage1::ImageDimension);
		itkStaticConstMacro(OutputImageDimension, unsigned int, TInputImage1::ImageDimension);

#ifdef ITK_USE_CONCEPT_CHECKING
		itkConceptMacro(SameDimensionCheck1,
			(Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
			itkGetStaticConstMacro(OutputImageDimension)>));
		itkConceptMacro(SameDimensionCheck2,
			(Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
			itkGetStaticConstMacro(InputImage2Dimension)>));
#endif

	protected: 
		WaveletImageFilter();
		virtual ~WaveletImageFilter(){}



		void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
			itk::ThreadIdType threadId) override;
		
	private: 
		WaveletImageFilter(const Self&);
		void operator=(const Self&);

		struct data {
		public:
			int pos;
			float value;
			data(int p, float v) : pos(p), value(v) {}

			inline bool operator<(data const &lhs){
				return (lhs.value > value);
			}
		};

		class ParaClass
		{
		public:
			ParaClass()
			{
				tLevels = 0;
				signalLength = 0;
				levelsDone = 0;
				mLevels = 0;
			}
			~ParaClass(){};

			void SetValue(int v1, int v2, int v3, int v4){
				this->tLevels = v1;
				this->signalLength = v2;
				this->levelsDone = v3;
				this->mLevels = v4;
			}

			int getValuetL() {
				return this->tLevels;
			}
			int getValuesLen() {
				return this->signalLength;
			}
			int getValues1D() {
				return this->levelsDone;
			}
			int getValuesmL() {
				return this->mLevels;
			}
		private:
			int tLevels;
			int signalLength;
			int levelsDone;
			int mLevels;
		};

		cl_uint signalLength;           /**< Signal length (Must be power of 2)*/
		cl_float *inData;               /**< input data */
		cl_float *dOutData;             /**< output data */
		cl_float *dPartialOutData;      /**< partial decomposed signal */
		cl_float *hOutData;             /**< output data calculated on host */

		cl_double setupTime;            /**< time taken to setup OpenCL resources and building kernel */
		cl_double kernelTime;           /**< time taken to run kernel and read result back */

		cl_context context;             /**< CL context */
		cl_device_id *devices;          /**< CL device list */

		cl_mem inDataBuf;               /**< CL memory buffer for input data */
		cl_mem dOutDataBuf;             /**< CL memory buffer for output data */
		cl_mem dPartialOutDataBuf;      /**< CL memory buffer for partial decomposed signal */
		cl_mem classObj;

		cl_command_queue commandQueue;  /**< CL command queue */
		cl_program program;             /**< CL program  */
		cl_kernel kernel;               /**< CL kernel for histogram */
		cl_uint maxLevelsOnDevice;      /**< Maximum levels to be computed on device */
		int iterations;                 /**< Number of iterations to be executed on kernel */
		size_t        globalThreads;    /**< global NDRange */
		size_t         localThreads;    /**< Local Work Group Size */
		SDKDeviceInfo deviceInfo;        /**< Structure to store device information*/
		KernelWorkGroupInfo kernelInfo;      /**< Structure to store kernel related info */

		SDKTimer    *sampleTimer;		 /**< SDKTimer object */

	public:
		CLCommandArgs * args;

		int setupDwtHaar1D(int idx);
		int setWorkGroupSize();

		int genBinary();
		int setupCL();
		int runCLKernels();
		void printStats();

		int initialize();
		int setup();
		int run();
		int cleanup();
		int verifyResults();

	private:
		cl_int groupSize;
		cl_int totalLevels;
		cl_int curSignalLength;
		cl_int levelsDone;

		int getLevels(unsigned int lenght, unsigned int *levels);
		int runDwtHaar1DKernel();
		int calApproxFinalOnHost();
	};
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkWaveletImageFilter.txx"
#endif

#endif //__itkWaveletImageFilter_h
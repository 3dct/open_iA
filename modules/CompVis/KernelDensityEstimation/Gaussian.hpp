#ifndef KDE_GAUSSIAN_HPP
#define KDE_GAUSSIAN_HPP

#include <KernelDensityEstimation/KernelDensityEstimation>

#include <vtkMath.h>

namespace kde
{
    template<typename realScalarType>
    realScalarType GaussPDF(realScalarType const& x,
        realScalarType const& mu, realScalarType const& sigma)
    {
        realScalarType z = (x - mu)/sigma;
        return std::exp(-realScalarType(0.5)*z*z)
            /(sigma*std::sqrt( realScalarType(2.)*vtkMath::Pi()));
    }

    /**
     * \class Gaussian
     *
     * \brief A class for computing Gaussian kernel
     *
     * \tparam _realScalarType floating poin type
     *
     * This class computes the Gaussian Kernel defined by
     * \f$ K(\mathbf{x}) = (2\pi)^{-d/2} \exp\left(-\frac{1}{2}
     * \mathbf{x}^{\mathrm{T}} \mathbf{x} \right) \f$.
     */
    template<typename _realScalarType>
    class Gaussian
    {
    public:
        typedef _realScalarType realScalarType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,Eigen::Dynamic> realMatrixType;
        typedef Eigen::Matrix<realScalarType,Eigen::Dynamic,1> realVectorType;

        /**
         * \brief A function that returns the log of the Gaussian kernel
         * \param x the point at wich the kernel is sought
         * \return the log of the kernel sought
         */
        static realScalarType compute(realVectorType const & x)
        {
            assert(x.rows()>0);
            return -0.5 * x.squaredNorm();
        }
    };

}//namespace kde

#endif //KDE_GAUSSIAN_HPP

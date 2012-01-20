/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680

  $Id$
*/

#ifndef SEPREPTENSOR_H_
#define SEPREPTENSOR_H_

#define HAVE_GENTENSOR 1


/*!
 *
 * \file SepRepTensor.h
 * \brief Provides a tensor with taking advantage of possibly low rank
 *
 ****************************************************************
 * MAIN DIFFERENCES (Tensor t; GenTensor g)
 *  t=t1(s) is shallow
 *  g=g1(s) is deep
 ****************************************************************
 *
 *
 * a SepRepTensor is a generalized form of a Tensor
 *
 * for now only little functionality shall be implemented; feel free to extend
 *
 * a consequence is that we (usually) can't directly access individual
 * matrix elements
 *
 * LowRankTensors' ctors will most likely not be shallow
 *
 * note that a LowRankTensor might have zero rank, but is still a valid
 * tensor, and should therefore return a FullTensor with zeros in it.
 *
 * \par Slicing in LowRankTensors:
 *
 * LowRankTensors differ from FullTensors in that we can't directly access
 * individual matrix elements, and thus can't directly assign or manipulate slices
 * as lvalues. For rvalues we simply provide a slices of the constituent vectors
 * in SRConf, which are valid LowRankTensors by themselves
 * \code
 *  lhs = rhs(s)
 * \endcode
 *
 * Manipulations of slices of LowRankTensors are heavily restricted, but should
 * cover the most important cases:
 * - assignment to a slice that was zero before, as in (t being some Tensor of
 *   dimensions 2,2,2,2, and s being the (2,2,2,2) slice); this is performed
 *   by inplace addition
 * 	  \code
 *    LowRankTensor lrt1(3,3,3,3), lrt2(t);
 *    lrt1(s) = lrt2;
 * 	  \endcode
 * - assignment to zero; done by inplace subtraction of the slice
 *    \code
 *    LowRankTensor lrt1(t);
 *    lrt1(s) = 0.0;
 *    \endcode
 * - in-place addition
 *    \code
 *    LowRankTensor lrt1(3,3,3,3), lrt2(t);
 *    lrt1(s) += lrt2;
 *    \endcode
 *
 * Note that *all* of these operation increase the rank of lhs
 */


#include "tensor/tensor.h"
#include "mra/seprep.h"

namespace madness {

	template <class T> class SliceLowRankTensor;
	template <class T> class SepRepTensor;
	template <class T> class FullTensor;
	template <class T> class LowRankTensor;
	template <class T> class SliceGenTensor;


	/// TensorArgs holds the arguments for creating a LowRankTensor
	struct TensorArgs {
		double thresh;
		TensorType tt;
		TensorArgs() {
			MADNESS_EXCEPTION("no default ctor for TensorArgs",0);
		}
		TensorArgs(const double& thresh1, const TensorType& tt1)
			: thresh(thresh1)
			, tt(tt1) {
		}
		template <typename Archive>
		void serialize(const Archive& ar) {}
	};


#if !HAVE_GENTENSOR

	template <typename T>
	class GenTensor : public Tensor<T> {

	public:

		GenTensor<T>() : Tensor<T>() {}

		GenTensor<T>(const Tensor<T>& t1) : Tensor<T>(t1) {}
		GenTensor<T>(const Tensor<T>& t1, const TensorArgs& targs) : Tensor<T>(t1) {}
		GenTensor<T>(const Tensor<T>& t1, double eps, const TensorType tt) : Tensor<T>(t1) {}
		GenTensor<T>(const TensorType tt): Tensor<T>() {}
		GenTensor<T>(std::vector<long> v, const TensorType& tt) : Tensor<T>(v) {}
		GenTensor<T>(std::vector<long> v, const TensorArgs& targs) : Tensor<T>(v) {}


		GenTensor<T> reconstruct_tensor() const {return *this;}
		GenTensor<T> full_tensor() const {return *this;}
		GenTensor<T>& full_tensor() {return *this;}
        GenTensor<T> full_tensor_copy() const {return *this;}
        GenTensor<T> full_tensor_copy() {return *this;}

        bool has_data() const {return this->size()!=0;};
		long rank() const {return -1;}
        void reduceRank(const double& eps) {return;};

		/// return the type of the derived class for me
        std::string what_am_i() const {return "GenTensor, aliased to Tensor";};
		TensorType type() const {return TT_FULL;}

		template <typename Archive>
        void serialize(Archive& ar) {}

	};

#else



	/// A GenTensor is an interface to a Tensor or a LowRankTensor

	/// This class provides a wrapper for an abstract base class SepRepTensor,
	/// which implements FullTensor (aka Tensor), and LowRankTensor (aka SepRep).
	/// Since not all operations are possible (or sensible) for LowRankTensors,
	/// only those that are are provided. There are no assignment for slices,
	/// neither for numbers nor for other GenTensors, use inplace-addition instead.
	///
	/// Behavior (in particular shallow and deep construction/assignment) should
	/// resemble the one of Tensor as far as possible:
	/// assignments/construction to/from other GenTensors are shallow
	/// assignments/construction from Tensor is deep
	/// assignments/construction to/from Slices are deep
	template<typename T>
	class GenTensor {

	private:

		friend class SliceGenTensor<T>;
		typedef std::shared_ptr<SepRepTensor<T> > sr_ptr;

		/// pointer to the abstract base class; implements FullTensor and LowRankTensor
		sr_ptr _ptr;

	public:

		/// empty ctor
		GenTensor() : _ptr() {}

		/// default ctor
		GenTensor(const TensorType tt) : _ptr() {
			if (tt==TT_FULL) _ptr=sr_ptr(new FullTensor<T>());
			else _ptr=sr_ptr(new LowRankTensor<T>(tt));
		}

		/// copy ctor, shallow
		GenTensor(const GenTensor<T>& rhs) : _ptr(rhs._ptr) {
		};

		/// ctor with dimensions
		GenTensor(const std::vector<long>& dim, const TensorType tt) : _ptr() {

			if (tt==TT_FULL) _ptr=sr_ptr(new FullTensor<T>(dim));
			else _ptr=sr_ptr(new LowRankTensor<T>(dim,tt));
		}

		/// ctor with dimensions
		GenTensor(const std::vector<long>& dim, const TensorArgs& targs) : _ptr() {

			if (targs.tt==TT_FULL) _ptr=sr_ptr(new FullTensor<T>(dim));
			else _ptr=sr_ptr(new LowRankTensor<T>(dim,targs.tt));
		}


		/// ctor with a regular Tensor and some arguments, deep
		GenTensor(const Tensor<T>& rhs, const TensorArgs& args) : _ptr() {

			if (args.tt==TT_FULL) _ptr=sr_ptr(new FullTensor<T> (copy(rhs)));
			else if (args.tt==TT_3D or args.tt==TT_2D) {
				MADNESS_ASSERT(args.thresh>0.0);
				if (rhs.iscontiguous()) _ptr=sr_ptr(new LowRankTensor<T> (rhs,args.thresh,args.tt));
				else _ptr=sr_ptr(new LowRankTensor<T> (copy(rhs),args.thresh,args.tt));
			}
		}


		/// ctor with a regular Tensor, deep
		GenTensor(const Tensor<T>& rhs, double eps, const TensorType tt) : _ptr() {

			if (tt==TT_FULL) _ptr=sr_ptr(new FullTensor<T> (copy(rhs)));
			else if (tt==TT_3D or tt==TT_2D) {
				MADNESS_ASSERT(eps>0.0);
				if (rhs.iscontiguous()) _ptr=sr_ptr(new LowRankTensor<T> (rhs,eps,tt));
				else _ptr=sr_ptr(new LowRankTensor<T> (copy(rhs),eps,tt));
			}
		}

		/// ctor with a SliceGenTensor, deep
		GenTensor(const SliceGenTensor<T>& rhs) : _ptr() {
			*this=rhs;
		}

		/// dtor
		~GenTensor() {
			this->clear();
		}

		/// shallow assignment
		GenTensor<T>& operator=(const GenTensor<T>& rhs) {
			_ptr=rhs._ptr;
			return *this;
		}

		/// deep assignment with slices: g0 = g1(s)
		GenTensor<T>& operator=(const SliceGenTensor<T>& rhs) {
			this->clear();
			_ptr=sr_ptr(rhs._refGT._ptr->clone(rhs._s));
			return *this;
		}

		/// deep copy
		friend GenTensor<T> copy(const GenTensor<T>& rhs) {
			if (rhs._ptr) return GenTensor<T> (rhs._ptr->copy_this());
			return GenTensor<T>();
		}

		/// general slicing, shallow; for temporary use only!
		SliceGenTensor<T> operator()(const std::vector<Slice>& s) {
			return SliceGenTensor<T>(*this,s);
		}

		/// general slicing, for g0 = g1(s), shallow, for temporary use only!
		const SliceGenTensor<T> operator()(const std::vector<Slice>& s) const {
			return SliceGenTensor<T>(*this,s);
		}

		/// assign a number
		GenTensor<T>& operator=(const double& fac) {
			MADNESS_ASSERT(0);
		}

		/// inplace addition
		GenTensor<T>& operator+=(const GenTensor<T>& rhs) {

			if (this->_ptr->type()!=rhs._ptr->type()) {
				print("damn");
				MADNESS_ASSERT(this->_ptr->type()==rhs._ptr->type());
			}
			rhs.accumulate_into(*this,1.0);
//			const std::vector<Slice> s(this->ndim(),Slice(_));
//			this->_ptr->inplace_add(rhs._ptr.get(),s,s);
			return *this;

		}

		/// inplace addition
		GenTensor<T>& operator+=(const SliceGenTensor<T>& rhs) {

			MADNESS_ASSERT(this->_ptr->type()==rhs._refGT._ptr->type());
			const std::vector<Slice> s(this->ndim(),Slice(_));
			this->_ptr->inplace_add(rhs._refGT._ptr.get(),s,rhs._s);
			return *this;

		}

		/// inplace addition
		GenTensor<T>& update_by(const GenTensor<T>& rhs) {
			_ptr->update_by(rhs._ptr.get());
			return *this;
		}

		/// finalize update_by
		void finalize_accumulate() {_ptr->finalize_accumulate();}

	    /// Multiplication of tensor by a scalar of a supported type to produce a new tensor

	    /// @param[in] x Scalar value
	    /// @return New tensor
		template <class Q>
		GenTensor<TENSOR_RESULT_TYPE(T,Q)> operator*(const Q& x) const {
	    	print("GenTensor<T>::operator* only with T==Q");
	    	MADNESS_ASSERT(0);
	    	GenTensor<TENSOR_RESULT_TYPE(T,Q)> result;
        	return result;

        }

        GenTensor<T> operator*(const T& x) const {
        	GenTensor<T> result(copy(*this));
        	result.scale(x);
        	return result;

        }


		/// Often used to transform all dimensions from one basis to another
        /// \code
        /// result(i,j,k...) <-- sum(i',j', k',...) t(i',j',k',...) c(i',i) c(j',j) c(k',k) ...
        /// \endcode
        /// The input dimensions of \c t must all be the same and agree with
        /// the first dimension of \c c .  The dimensions of \c may differ in
        /// size.  If the dimensions of \c c are the same, and the operation
        /// is being performed repeatedly, then you might consider calling \c
        /// fast_transform instead which enables additional optimizations and
        /// can eliminate all constructor overhead and improve cache locality.
		GenTensor<T> transform(const Tensor<T> c) const {
	    	return this->_ptr->transform(c);
		}

	    template <class Q>
	    GenTensor<TENSOR_RESULT_TYPE(T,Q)> general_transform(const Tensor<Q> c[]) const {
	    	print("GenTensor<T>::general_transform only with T==Q");
	    	MADNESS_ASSERT(0);
	    }

	    GenTensor<T> general_transform(const Tensor<T> c[]) const {
	        return this->_ptr->general_transform(c);
	    }

    	GenTensor<T> transform_dir(const Tensor<T>& c, int axis) const {
    		return this->_ptr->transform_dir(c,axis);
    	}

    	void fillrandom() {
    		_ptr->fillrandom();;
    	}

	    /// Inplace generalized saxpy ... this = this*alpha + other*beta
	    GenTensor<T>& gaxpy(std::complex<double> alpha, const GenTensor<T>& rhs, std::complex<double> beta) {
	    	MADNESS_EXCEPTION("GenTensor::gaxpy only with double factors",0);
	    }

	    /// Inplace generalized saxpy ... this = this*alpha + other*beta
	    GenTensor<T>& gaxpy(double alpha, const GenTensor<T>& rhs, double beta) {
	    	this->_ptr->gaxpy(alpha, rhs._ptr.get(), beta);
	    	return *this;
	    }

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(Tensor<T>& t, const double fac) const {
	    	_ptr->accumulate_into(t,fac);
	    }

	    /// accumulate this into t
	    void accumulate_into(GenTensor<T>& t, const double fac) const {
	    	_ptr->accumulate_into(t._ptr.get(),fac);
	    }

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(Tensor<T>& t, const std::complex<double> fac) const {
	    	MADNESS_EXCEPTION("GenTensor::accumulate_into only with double fac",0);
	    }

	    /// accumulate this into t
	    void accumulate_into(GenTensor<T>& rhs, const std::complex<double> fac) const {
	    	MADNESS_EXCEPTION("GenTensor::accumulate_into only with double fac",0);
	    }

		/// reduce the rank of this; don't do nothing if FullTensor
		void reduceRank(const double& eps) {_ptr->reduceRank(eps);};

		/// returns if this GenTensor exists
		bool has_data() const {
			if (!_ptr) return false;
			return _ptr->has_data();
		}

		/// returns the number of coefficients (might return zero, although tensor exists)
		size_t size() const {
			if (_ptr) return _ptr->size();
			return 0;
		};

		/// returns the TensorType of this
		TensorType type() const {
			if (_ptr) return _ptr->type();
			return TT_NONE;
		};

        /// return the type of the derived class for me
        std::string what_am_i() const {return _ptr->what_am_i();};

		/// returns the rank of this; if FullTensor returns -1
		long rank() const {return _ptr->rank();};

		/// returns the dimensions
		long dim(const int& i) const {return _ptr->dim(i);};

		/// returns the number of dimensions
		long ndim() const {
			if (_ptr) return _ptr->ndim();
			return -1;
		};

		/// returns the Frobenius norm
		double normf() const {return _ptr->normf();};

        /// Returns new view/tensor swaping dimensions \c i and \c j

        /// @return New tensor (viewing same underlying data as the original but with reordered dimensions)
        GenTensor<T> swapdim(long idim, long jdim) const {
			print("no swapdim for GenTensor for now");
			MADNESS_ASSERT(0);
        	return this->_ptr->swapdim(idim,jdim);
        }


		/// returns the trace of <this|rhs>
		T trace_conj(const GenTensor<T>& rhs) const {
			MADNESS_ASSERT(this->type()==rhs.type());
			return _ptr->trace_conj(rhs._ptr.get());
		}

        /// Inplace multiplication by scalar of supported type (legacy name)

        /// @param[in] x Scalar value
        /// @return %Reference to this tensor
        template <typename Q>
        typename IsSupported<TensorTypeData<Q>,GenTensor<T>&>::type
        scale(Q x) {
            if (_ptr) _ptr->scale(x);
            return *this;
        }

        /// Inplace multiply by corresponding elements of argument Tensor
        GenTensor<T>& emul(const GenTensor<T>& t) {
        	print("no GenTensor<T>::emul yet");
        	MADNESS_ASSERT(0);
        }

        /// returns a reference to FullTensor of this; no reconstruction
		Tensor<T>& full_tensor() const {
			return _ptr->fullTensor();
		}

		/// returns a FullTensor of this; reconstructs a LowRankTensor
		Tensor<T> reconstruct_tensor() const {
			return _ptr->reconstructTensor();
		}

		/// return a Tensor, no matter what
		Tensor<T> full_tensor_copy() const {
			if (_ptr->type()==TT_FULL) return _ptr->fullTensor();
			else if (_ptr->type()==TT_NONE) return Tensor<T>();
			else if (_ptr->type()==TT_3D or _ptr->type()==TT_2D) return _ptr->reconstructTensor();
			else {
				print(_ptr->type(),"unknown tensor type");
				MADNESS_ASSERT(0);
			}
		}

		template <typename Archive>
        void serialize(Archive& ar) {}

	private:

		/// release memory
		void clear() {_ptr.reset();};

		/// ctor with a SepRepTensor, shallow
		GenTensor(SepRepTensor<T>* sr) : _ptr(sr) {}

		/// inplace add rhs to this, provided slices: *this(s1)+=rhs(s2)
		GenTensor<T>& inplace_add(const GenTensor<T>& rhs, const std::vector<Slice>& lhs_s,
				const std::vector<Slice>& rhs_s) {

			this->_ptr->inplace_add(rhs._ptr.get(),lhs_s,rhs_s);
			return *this;
		}


	};


	/// implements a slice of a GenTensor
	template <typename T>
	class SliceGenTensor {

	private:
		friend class GenTensor<T>;

		GenTensor<T>& _refGT;
		std::vector<Slice> _s;

		// all ctors are private, only accessible by GenTensor

		/// default ctor
		SliceGenTensor<T> () {}

		/// ctor with a GenTensor;
		SliceGenTensor<T> (const GenTensor<T>& gt, const std::vector<Slice>& s)
				: _refGT(const_cast<GenTensor<T>& >(gt))
				, _s(s) {}

	public:

		/// assignment as in g(s) = g1;
		SliceGenTensor<T>& operator=(const GenTensor<T>& rhs) {
			print("You don't want to assign to a SliceGenTensor; use operator+= instead");
			MADNESS_ASSERT(0);
			return *this;
		};

		/// assignment as in g(s) = g1(s);
		SliceGenTensor<T>& operator=(const SliceGenTensor<T>& rhs) {
			print("You don't want to assign to a SliceGenTensor; use operator+= instead");
			MADNESS_ASSERT(0);
			return *this;
		};

		/// inplace addition
		SliceGenTensor<T>& operator+=(const GenTensor<T>& rhs) {
			std::vector<Slice> s(this->_refGT.ndim(),Slice(_));
			_refGT.inplace_add(rhs,this->_s,s);
			return *this;
		}

		/// inplace addition
		SliceGenTensor<T>& operator+=(const SliceGenTensor<T>& rhs) {
			_refGT.inplace_add(rhs._refGT,this->_s,rhs._s);
			return *this;
		}

		/// inplace zero-ing
		SliceGenTensor<T>& operator=(const double& number) {
			MADNESS_ASSERT(number==0.0);
			GenTensor<T> tmp=*this;
			if (this->_refGT.type()==TT_FULL) tmp=copy(tmp);
			tmp.scale(-1.0);
			_refGT.inplace_add(tmp,_s,_s);
			return *this;
		}

		/// for compatibility with tensor
		friend GenTensor<T> copy(const SliceGenTensor<T>& rhs) {
			return GenTensor<T>(rhs);
		}

	};



    /// abstract base class for the SepRepTensor
    template<typename T>
    class SepRepTensor {

    public:

    	typedef typename TensorTypeData<T>::float_scalar_type float_scalar_type;

    	/// default ctor
    	SepRepTensor() {};

    	/// implement the virtual constructor idiom; "sliced copy constructor"
    	virtual SepRepTensor* clone(const std::vector<Slice>& s) const =0;

    	/// implement the virtual constructor idiom; "copy constructor"
    	virtual SepRepTensor* clone() const =0;

    	/// implement the virtual constructor idiom, deep copy
    	virtual SepRepTensor* copy_this() const =0;

    	/// virtual destructor
    	virtual ~SepRepTensor() {};

       	/// inplace add
    	virtual SepRepTensor<T>* inplace_add(const SepRepTensor<T>* rhs,
    			const std::vector<Slice>& lhs_s, const std::vector<Slice>& rhs_s) =0;

    	/// inplace add
    	virtual SepRepTensor* update_by(const SepRepTensor<T>* rhs) =0;

		virtual void finalize_accumulate() =0;

    	/// return the type of the derived class
        virtual TensorType type() const =0;

        /// return the type of the derived class for me
        virtual std::string what_am_i() const =0;

        /// Returns if this Tensor exists
        virtual bool has_data() const =0;

    	virtual long size() const =0;

    	virtual long dim(const int) const =0;

    	virtual long ndim() const =0;

    	virtual long rank() const =0;

    	virtual void reduceRank(const double& eps) =0;

    	virtual void fillrandom() =0;

    	virtual SepRepTensor* swapdim(const long idim, const long jdim) const =0;

    	virtual T trace_conj(const SepRepTensor<T>* rhs) const =0;

    	virtual Tensor<T> reconstructTensor() const {
    		print("reconstructTensor not implemented for",this->what_am_i());
    		MADNESS_ASSERT(0);
    	}

    	virtual Tensor<T>& fullTensor() =0;
    	virtual const Tensor<T>& fullTensor() const =0;

    	virtual T operator()(long i, long j, long k) const=0;

    	/// compute the Frobenius norm
    	virtual float_scalar_type normf() const =0;

    	/// scale by a number
    	virtual void scale(T a) {throw;};

	    /// Inplace generalized saxpy ... this = this*alpha + other*beta
	    virtual SepRepTensor<T>& gaxpy(double alpha, const SepRepTensor<T>* rhs, double beta) =0;

    	/// transform as a member function
    	virtual SepRepTensor<T>* transform(const Tensor<T>& c) const =0;

	    virtual SepRepTensor<T>* general_transform(const Tensor<T> c[]) const =0;

    	virtual SepRepTensor<T>* transform_dir(const Tensor<T>& c, int axis) const =0;

    	/// accumulate this into t, reconstruct this if necessary
	    virtual void accumulate_into(Tensor<T>& t, const double fac) const = 0;

	    /// accumulate this into t
	    virtual void accumulate_into(SepRepTensor<T>* t, const double fac) const = 0;

    };

    template<typename T>
    class LowRankTensor : public SepRepTensor<T> {

    private:
    	typedef typename TensorTypeData<T>::float_scalar_type float_scalar_type;

    public:
    	SepRep<T> _data;		///< the tensor data


       	/// default constructor (tested)
    	LowRankTensor<T>() : _data(SepRep<T>()) {
    		print("there is no default ctor for LowRankTensor; provide a TensorType!");
    		MADNESS_ASSERT(0);
    	};

    	/// default constructor (tested)
    	LowRankTensor<T>(const TensorType& tt) : _data(SepRep<T>(tt)) {
    	};

    	/// construct am empty tensor
    	LowRankTensor<T> (const std::vector<long>& s, const TensorType& tt)
    			: _data(SepRep<T>(tt,s[0],s.size())) {

    		// check consistency
    		const long ndim=s.size();
    		const long maxk=s[0];
    		for (long idim=0; idim<ndim; idim++) {
    			MADNESS_ASSERT(maxk==s[0]);
    		}
    	}

    	/// copy constructor, shallow (tested)
    	LowRankTensor<T>(const LowRankTensor<T>& rhs) : _data(rhs._data) {
    	};

    	/// constructor w/ slice on rhs, deep
    	LowRankTensor<T>(const SliceLowRankTensor<T>& rhs) : _data(rhs.ref_data()(rhs._s)) {
    	};

    	/// ctor with a SepRep (shallow)
    	LowRankTensor<T>(const SepRep<T>& rhs) : _data(rhs) {
    	}

    	/// ctor with regular Tensor (tested)
    	LowRankTensor<T>(const Tensor<T>& t, const double& eps, const TensorType& tt)
    			: _data(SepRep<T>(t,eps,tt)) {}

    	/// dtor
    	~LowRankTensor<T>() {}

    	/// shallow assignment with rhs=LowRankTensor (tested)
    	LowRankTensor<T>& operator=(const LowRankTensor<T>& rhs) {
    		_data=rhs._data;
    		return *this;
    	};

    	/// assignment with rhs=SliceLowRankTensor, deep
    	LowRankTensor<T>& operator=(const SliceLowRankTensor<T>& rhs) {
    		_data=rhs.ref_data()(rhs._s);
    		return *this;
    	};

    	/// assignment to a number
    	LowRankTensor<T>& operator=(const T& a) {
    		MADNESS_ASSERT(0);
    	}

    	/// implement the virtual constructor idiom; "sliced copy constructor"
    	LowRankTensor<T>* clone(const std::vector<Slice>& s) const {
    		return new LowRankTensor<T>((this->_data)(s));
    	}


    	/// implement the virtual constructor idiom, shallow
    	LowRankTensor<T>* clone() const {
    		return new LowRankTensor<T>(*this);
    	}

    	/// implement the virtual constructor idiom, deep
    	LowRankTensor* copy_this() const {
    		return new LowRankTensor<T>(copy(*this));
    	}

    	/// deep copy
    	friend LowRankTensor<T> copy(const LowRankTensor<T>& rhs) {
    		return LowRankTensor<T>(copy(rhs._data));
    	}

       	/// return this tensor type
        TensorType type() const {return _data.tensor_type();};

        /// return the type of the derived class for me
        std::string what_am_i() const {
        	if (this->type()==TT_2D) return "LowRank-2D";
        	else if (this->type()==TT_3D) return "LowRank-3D";
        	else {
        		print("unknown tensor type",this->type());
        		MADNESS_ASSERT(0);
        	}
        };

        /// reduce the rank of this; return if FullTensor
        void reduceRank(const double& eps) {_data.reduceRank(eps);};

        void fillrandom() {_data.fillrandom();}

        /// return the rank of this
        long rank() const {return _data.rank();};

        /// Returns if this Tensor exists
        bool has_data() const {return _data.is_valid();};

        /// Return the number of coefficients (valid for all SPR)
        long size() const {
        	if (!this->has_data()) return 0;
        	return _data.nCoeff();
        };

    	virtual long dim(const int) const {return _data.get_k();};

		virtual long ndim() const {return _data.dim();};

		/// return a shallow copy of this with swapped dimensions
		LowRankTensor<T>* swapdim(const long idim, const long jdim) const {
			print("no LowRankTensor::swapdim");
			MADNESS_ASSERT(0);
		}

		/// reconstruct this to a full tensor
		Tensor<T> reconstructTensor() const {
			MADNESS_ASSERT(_data.is_valid());
			return _data.reconstructTensor();

		};

		/// (Don't) return a full tensor representation
		Tensor<T>& fullTensor() {
			MADNESS_EXCEPTION("No fullTensor in LowRankTensor; reconstruct first",0);
		};

		/// (Don't) return a full tensor representation
		Tensor<T>& fullTensor() const {
			MADNESS_EXCEPTION("No fullTensor in LowRankTensor; reconstruct first",0);
		};

       	/// compute the inner product
    	T trace_conj(const SepRepTensor<T>* rhs) const {
    		const LowRankTensor<T>* other=dynamic_cast<const LowRankTensor<T>* >(rhs);
    		T result=overlap(this->_data,other->_data);
    		return result;
    	}


    	T operator()(long i, long j, long k) const {throw;};

    	/// compute the Frobenius norm
    	float_scalar_type normf() const {return _data.FrobeniusNorm();};

    	LowRankTensor<T>& gaxpy(double alpha, const SepRepTensor<T>* rhs, double beta) {

    		MADNESS_ASSERT(this->type()==rhs->type());
    		MADNESS_ASSERT(this->ndim()==rhs->ndim());

    		const LowRankTensor<T>* t=dynamic_cast<const LowRankTensor<T>* > (rhs);

			const std::vector<Slice> s(this->ndim(),Slice(_));
    		this->_data.inplace_add(t->_data,s,s,alpha,beta);
    		return *this;
    	}

    	/// scale by a number
    	void scale(T a) {_data.scale(a);};

    	/// transform
    	LowRankTensor<T>* transform(const Tensor<T>& c) const {
    		SepRep<T> sr=this->_data.transform(c);
    		return new LowRankTensor<T> (sr);
    	}

	    LowRankTensor<T>* general_transform(const Tensor<T> c[]) const {
    		SepRep<T> sr=this->_data.general_transform(c);
    		return new LowRankTensor<T> (sr);

	    }

    	/// result(i,j,k,m) = sum_l this(i,j,k,l) c(l,m)
    	LowRankTensor<T>* transform_dir(const Tensor<T>& c, int axis) const {
    		return new LowRankTensor<T>(_data.transform_dir(c,axis));
    	}

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(Tensor<T>& t, const double fac) const {
	    	_data.accumulate_into(t,fac);
	    }

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(SepRepTensor<T>* t, const double fac) const {
	    	LowRankTensor<T>* other=dynamic_cast<LowRankTensor<T>* >(t);
	    	_data.accumulate_into(other->_data,fac);
	    }

		void finalize_accumulate() {_data.finalize_accumulate();}

    private:

    	/// inplace add: this(lhs_s) += rhs(rhs_s)
    	LowRankTensor<T>* inplace_add(const SepRepTensor<T>* rhs,
    			const std::vector<Slice>& lhs_s, const std::vector<Slice>& rhs_s) {

    		// some checks
    		MADNESS_ASSERT(this->type()==rhs->type());

    		const LowRankTensor<T>* rhs2=dynamic_cast<const LowRankTensor<T>* > (rhs);
    		this->_data.inplace_add(rhs2->_data,lhs_s,rhs_s,1.0,1.0);
    		return this;
    	}



    	/// inplace add: this(lhs_s) += rhs(rhs_s)
    	LowRankTensor<T>* update_by(const SepRepTensor<T>* rhs) {

    		// some checks
    		MADNESS_ASSERT(this->type()==rhs->type());

    		const LowRankTensor<T>* rhs2=dynamic_cast<const LowRankTensor<T>* > (rhs);
    		this->_data.update_by(rhs2->_data);
    		return this;
    	}

    };


    /// the case of a full rank tensor, as tensor.h

    /**
     * this class merely wraps the tensor class, but we need to do so for
     * we want it to be a derived class of SepRepTensor;
     */
    template<typename T>
    class FullTensor : public SepRepTensor<T> {

    private:

    	typedef typename TensorTypeData<T>::float_scalar_type float_scalar_type;

    	Tensor<T> data;	///< the tensor data

    public:

    	/// default constructor constructs an empty tensor
    	FullTensor<T>() {
    		data=Tensor<T>() ;
    	};

    	/// copy constructor (tested)
		FullTensor<T>(const FullTensor<T>& rhs) {
			*this=rhs;
		};

		/// ctor with a regular Tensor (tested)
		FullTensor<T>(const Tensor<T>& t) {
			data=t;
		}

		/// ctor with an empty Tensor
		FullTensor<T>(const std::vector<long>& s) {
			data=Tensor<T>(s);
		}

		/// dtor
		~FullTensor<T>() {};

    	/// assignment with rhs=FullTensor (tested)
    	FullTensor<T>& operator=(const FullTensor<T>& rhs) {
    		if (this!=&rhs) {
    			data=rhs.data;
    		}
    		return *this;
    	}

    	/// assignment with rhs=Tensor (tested)
    	FullTensor<T>& operator=(const Tensor<T>& rhs) {
    		if (&this->data!=&rhs) {
    			data=rhs;
    		}
    		return *this;
    	}

    	/// assignment to a number (tested)
    	FullTensor<T>& operator=(const T& a) {
    		data=a;
    		return *this;
    	}

    	/// general slicing (tested)
    	FullTensor<T> operator()(const std::vector<Slice>& s) const {
//    		return SliceTensor<T> (data,&(s[0]));
    		return copy(data(s));
    	}

        /// Inplace generalized saxpy ... this = this*alpha + other*beta
    	FullTensor<T>& gaxpy(double alpha, const SepRepTensor<T>* rhs, double beta) {

    		MADNESS_ASSERT(this->type()==rhs->type());
    		const FullTensor<T>* t=dynamic_cast<const FullTensor<T>* >(rhs);
    		this->data.gaxpy(alpha,t->data,beta);
    		return *this;
    	}

    	/// implement the virtual constructor idiom
    	FullTensor<T>* clone() const {
    		return new FullTensor<T>(*this);
    	}

    	/// implement the virtual constructor idiom, "sliced copy ctor"
    	FullTensor<T>* clone(const std::vector<Slice>& s) const {
    		return new FullTensor<T>((*this)(s));
    	}

    	/// implement the virtual constructor idiom, deep
    	FullTensor<T>* copy_this() const {
    		return new FullTensor<T>(copy(*this));
    	}

    	/// return this tensor type
    	TensorType type() const {return TT_FULL;};

        /// return the type of the derived class for me
        std::string what_am_i() const {return "FullRank";};

        /// reduce the rank of this; return if FullTensor
        void reduceRank(const double& eps) {return;};

        void fillrandom() {data.fillrandom();}

        /// return the rank of this
        long rank() const {return -1;};

        /// Returns if this Tensor exists
        bool has_data() const {return size()!=0;};

    	/// the number of elements of this tensor
    	long size() const {return data.size();};

    	/// the number of dimensions (number of indices)
    	long ndim() const {return data.ndim();};

    	/// the length of each dimension (range for each index)
    	long dim(const int i) const {return data.dim(i);};

    	/// return a tensor of this
    	Tensor<T>& fullTensor() {return data;};

    	/// return a const tensor of this
    	const Tensor<T>& fullTensor() const {return data;};

    	/// compute the Frobenius norm
       	float_scalar_type normf() const {return data.normf();};

       	/// returns a shallow copy of this with swapped dimensions
       	FullTensor<T>* swapdim(const long idim, const long jdim) const {
       		return new FullTensor(data.swapdim(idim,jdim));
       	}

       	/// compute the inner product
    	T trace_conj(const SepRepTensor<T>* rhs) const {
    		const FullTensor<T>* other=dynamic_cast<const FullTensor<T>* >(rhs);
    		return this->data.trace_conj(other->data);
    	}

    	/// scale by a number
    	void scale(T a) {data*=a;};

        template <typename Archive>
        void serialize(Archive& ar) {}

    	T operator()(long i, long j, long k) const {return data(i,j,k);};

    	friend FullTensor<T> copy(const FullTensor<T>& rhs) {
    		return FullTensor<T>(copy(rhs.data));
    	}

    	/// transform this by c
    	FullTensor<T>* transform(const Tensor<T>& c) const {
    		const Tensor<T>& t=this->data;
    		Tensor<T> result=madness::transform(t,c);
    		return new FullTensor<T>(result);
    	}

	    FullTensor<T>* general_transform(const Tensor<T> c[]) const {
    		const Tensor<T>& t=this->data;
    		Tensor<T> result=madness::general_transform(t,c);
    		return new FullTensor<T>(result);

	    }


    	/// inner product
    	FullTensor<T>* transform_dir(const Tensor<T>& c, int axis) const {
    		return new FullTensor<T>(madness::transform_dir(data,c,axis));
    	}

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(Tensor<T>& t, const double fac) const {
	    	t.gaxpy(1.0,data,fac);
	    }

	    /// accumulate this into t, reconstruct this if necessary
	    void accumulate_into(SepRepTensor<T>* rhs, const double fac) const {
	    	MADNESS_ASSERT(this->type()==rhs->type());
	    	FullTensor<T>* rhs2=dynamic_cast<FullTensor<T>* >(rhs);
	    	rhs2->data.gaxpy(1.0,this->data,fac);
	    }

    private:

    	/// inplace add: this(lhs_s) += rhs(rhs_s)
    	FullTensor<T>* inplace_add(const SepRepTensor<T>* rhs,
    			const std::vector<Slice>& lhs_s, const std::vector<Slice>& rhs_s) {

    		// some checks
    		MADNESS_ASSERT(this->type()==rhs->type());

    		const FullTensor<T>* rhs2=dynamic_cast<const FullTensor<T>* >(rhs);
    		this->data(lhs_s)+=rhs2->data(rhs_s);
    		return this;
    	}

    	/// inplace add
    	FullTensor<T>* update_by(const SepRepTensor<T>* rhs) {
    		// some checks
    		MADNESS_ASSERT(this->type()==rhs->type());

    		const FullTensor<T>* rhs2=dynamic_cast<const FullTensor<T>* >(rhs);
    		this->data+=rhs2->data;
    		return this;

    	}

		void finalize_accumulate() {return;}

    };

    /// transform the argument SepRepTensor to FullTensor form
    template <typename T>
    void to_full_rank(GenTensor<T>& arg) {

    	if (arg.has_data()) {
        	if (arg.type()==TT_FULL) {
        		;
        	} else if (arg.type()==TT_3D) {
        		Tensor<T> t=arg.reconstruct_tensor();
            	arg=GenTensor<T>(t,0.0,TT_FULL);
        	} else {
        		throw std::runtime_error("unknown TensorType in to_full_tensor");
        	}

    	} else {
    		arg=GenTensor<T>(TT_FULL);
    	}

//    	print("to_full_rank");
    }

    /// transform the argument SepRepTensor to LowRankTensor form
    template <typename T>
    void to_low_rank(GenTensor<T>& arg, const double& eps, const TensorType& target_type) {

//    	print("to_low_rank");

    	if (arg.has_data()) {
        	if (arg.type()==TT_FULL) {
				const Tensor<T> t1=arg.full_tensor();
				arg=(GenTensor<T>(t1,eps,target_type));
	     	} else if (arg.type()==TT_3D) {
         		;
         	} else {
         		throw std::runtime_error("unknown TensorType in to_full_tensor");
         	}
    	} else {
    		arg=GenTensor<T>(target_type);
    	}
     }

    /// Often used to transform all dimensions from one basis to another
    /// \code
    /// result(i,j,k...) <-- sum(i',j', k',...) t(i',j',k',...) c(i',i) c(j',j) c(k',k) ...
    /// \endcode
	/// cf tensor/tensor.h
    template <class T, class Q>
    GenTensor< TENSOR_RESULT_TYPE(T,Q) > transform(const GenTensor<Q>& t, const Tensor<T>& c) {
    	MADNESS_ASSERT(0);
//    	return t.transform(c);
    }

    template <class T>
    GenTensor<T> transform(const GenTensor<T>& t, const Tensor<T>& c) {
    	return t.transform(c);
    }


    /// Transform all dimensions of the tensor t by distinct matrices c

    /// Similar to transform but each dimension is transformed with a
    /// distinct matrix.
    /// \code
    /// result(i,j,k...) <-- sum(i',j', k',...) t(i',j',k',...) c[0](i',i) c[1](j',j) c[2](k',k) ...
    /// \endcode
    /// The first dimension of the matrices c must match the corresponding
    /// dimension of t.
    template <class T, class Q>
    GenTensor<TENSOR_RESULT_TYPE(T,Q)> general_transform(const GenTensor<T>& t, const Tensor<Q> c[]) {
        return t.general_transform(c);
    }

    template <class T>
    GenTensor<T> general_transform(const GenTensor<T>& t, const Tensor<T> c[]) {
    	return t.general_transform(c);
    }


    /// Transforms one dimension of the tensor t by the matrix c, returns new contiguous tensor

    /// \ingroup tensor
    /// \code
    /// transform_dir(t,c,1) = r(i,j,k,...) = sum(j') t(i,j',k,...) * c(j',j)
    /// \endcode
    /// @param[in] t Tensor to transform (size of dimension to be transformed must match size of first dimension of \c c )
    /// @param[in] c Matrix used for the transformation
    /// @param[in] axis Dimension (or axis) to be transformed
    /// @result Returns a new, contiguous tensor
    template <class T, class Q>
    GenTensor<TENSOR_RESULT_TYPE(T,Q)> transform_dir(const GenTensor<Q>& t, const Tensor<T>& c,
                                          int axis) {

    	return t.transform_dir(c,axis);
    }

    #endif /* HAVE_GENTENSOR */

}

#endif /* SEPREPTENSOR_H_ */
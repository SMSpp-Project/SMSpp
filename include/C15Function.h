/*--------------------------------------------------------------------------*/
/*-------------------------- File C15Function.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C15Function class, which implements
 * C05Function and is able to provide approximations to its Hessian.
 *
 * \version 0.10
 *
 * \date 20 - 10 - 2017
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __C15Function
 #define __C15Function
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05Function.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*--------------------- C15Function-RELATED TYPES --------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup C15Function_TYPES C15Function-related types.
 *  @{ */

/** @} end( group( C15Function_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup C15Function_CLASSES Classes in C15Function.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS C15Function ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of functions that can provide subgradients
/** The class C15Function implements Function and it is the base class for
 * functions that, besides linearizations (first-order-type information),
 * can also provide quadratic models (second-order-type information).
 *
 * The class uses Eigen data structures to represent sparse and dense
 * matrices. */

class C15Function : public C05Function {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 typedef Eigen::Matrix< FunctionValue , Eigen::Dynamic , Eigen::Dynamic >
  DenseHessian;
 ///< type used to store a dense Hessian matrix

 typedef Eigen::SparseMatrix<FunctionValue> SparseHessian;
 ///< type used to store a sparse Hessian matrix

/**@} ----------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of C15Function: only calls that of C05Function
 /** Constructor of C15Function. Takes as input an optional pointer to an
  * Observer and passes it to the constructor of C05Function. */

 C15Function( Observer * const observer = nullptr )
  : C05Function( observer ) { }

/*--------------------------------------------------------------------------*/

 virtual ~C15Function() { }  ///< destructor: it is virtual, and empty

/**@} ----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A C15Function -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a C15Function
 *  @{ */

 /// compute an approximation to the Hessian for this Function
 /** Pure virtual method: it has to compute an approximation to the Hessian
  * matrix of this function at the current point. */

 virtual void compute_hessian_approximation( void ) = 0;

/*--------------------------------------------------------------------------*/
 /// obtain an approximation to the Hessian for this Function
 /** This method will store in the object provided as argument an
  * approximation to the Hessian. This method can only be called after the
  * method compute_hessian_approximation() has been called. */

 virtual void get_hessian_approximation( DenseHessian &hessian ) const = 0;

/*--------------------------------------------------------------------------*/
 /// obtain an approximation to the Hessian for this Function
 /** This method will store in the object provided as argument an
  * approximation to the Hessian. This method can only be called after the
  * method compute_hessian_approximation() has been called. */

 virtual void get_hessian_approximation( SparseHessian &hessian ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this Function has continuous Hessian
 /** Method that returns true if and only if this Function has continuous
  * second order derivative. By default, false is returned. */

 virtual bool is_twice_continuously_differentiable( void ) const {
  return( false );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the C15Function on an ostream
 /** Protected method intended to print information about the C15Function; it
  * is virtual so that derived classes can print their specific information 
  *in the format they choose. */

 virtual void print( std::ostream &output ) const override {
  output << "C15Function [" << this << "]" << " with "
	 << get_num_active_var() << " active variables";
  }

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

  };  // end( class( C15Function ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS C15FunctionMod ---------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a C15Function
/** Derived class from FunctionMod to describe modifications to a
 * C15Function. Placeholder only: so far no specific uses for C15FunctionMod
 * have been identified (but there will lilely be some). */

class C15FunctionMod : public C05FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// Definition of the possible type of C15FunctionMod
 enum c15function_mod_type {
  C15FunctionModLastParam = C05FunctionModLastParam ,
  ///< first allowed parameter value for derived classes
  /**< convenience value for easily allow derived classes to extend
   * the set of types of modifications */
 };

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 C15FunctionMod( C15Function * const f , const int mod ,
		 Function::FunctionValue shift = 0 ,
		 const bool cB = true  )
  : C05FunctionMod( f , mod , shift , cB ) { }

 ///< constructor: takes the type of Modification and a C15Function pointer
 /**< constructor: takes the type of the Modification and a pointer to
  * the affected C15Function. Note that while the enum
  * c15function_mod_type is provided to encode the possible values of
  * modification, the field f_type is of type "int", and therefore so
  * is the parameter of the constructor, in order to allow derived
  * classes to "extend" the set of possible types of modifications. */

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~C15FunctionMod() { }  ///< destructor: does nothing

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C15FunctionMod
 virtual inline void print( std::ostream &output ) const {
   output << "C15FunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function [" << f_function << " ]" << std::endl;
  }

/*--------------------------------------------------------------------------*/

};  // end( class( C15FunctionMod ) )

/** @} end( group( C15Function_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C15Function.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File C15Function.h --------------------------*/
/*--------------------------------------------------------------------------*/

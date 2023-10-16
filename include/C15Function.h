/*--------------------------------------------------------------------------*/
/*-------------------------- File C15Function.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C15Function class, which implements
 * C05Function and is able to provide approximations to its Hessian.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __C15Function
#define __C15Function /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05Function.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it {

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

 typedef Eigen::Matrix< FunctionValue, Eigen::Dynamic, Eigen::Dynamic >
  DenseHessian;
 ///< type used to store a dense Hessian matrix

 typedef Eigen::SparseMatrix< FunctionValue > SparseHessian;
 ///< type used to store a sparse Hessian matrix

/*--------------------------------------------------------------------------*/
 /// public enum "extending" int_par_type_C05F to the case of C15Function

 enum int_par_type_C15F {
  intLastParC15F = intLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of int parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" dbl_par_type_C05F to the case of C15Function

 enum dbl_par_type_C15F {
  dblLastParC15F = dblLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of double parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" str_par_type_C05F to the case of C15Function

 enum str_par_type_C15F {
  strLastParC15F = strLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of string parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vint_par_type_C05F to the of C15Function

 enum vint_par_type_C15F {
  vintLastParC15F = vintLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vdbl_par_type_C05F to the case of C15Function

 enum vdbl_par_type_C15F {
  vdblLastParC15F = vdblLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vstr_par_type_C05F to the case of C15Function

 enum vstr_par_type_C15F {
  vstrLastParC15F = vstrLastParC05F
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string parameters. */
  };

/** @} ---------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of C15Function: only calls that of C05Function
 /** Constructor of C15Function. Takes as input an optional pointer to an
  * Observer and passes it to the constructor of C05Function. */

 explicit C15Function( Observer * observer = nullptr )
  : C05Function( observer ) {}

/*--------------------------------------------------------------------------*/

 ~C15Function() override = default;  ///< destructor: it is virtual, and empty

/** @} ---------------------------------------------------------------------*/
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

 virtual void get_hessian_approximation( DenseHessian & hessian ) const = 0;

/*--------------------------------------------------------------------------*/
 /// obtain an approximation to the Hessian for this Function
 /** This method will store in the object provided as argument an
  * approximation to the Hessian. This method can only be called after the
  * method compute_hessian_approximation() has been called. */

 virtual void get_hessian_approximation( SparseHessian & hessian ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true only if this Function has continuous Hessian
 /** Method that returns only if this Function has continuous second order
  * derivative. By default, false is returned. */

 [[nodiscard]] virtual bool is_twice_continuously_differentiable( void )
  const { return( false ); }

/** @} ---------------------------------------------------------------------*/
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

 void print( std::ostream & output ) const override {
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
  C15FunctionModLastParam = C05FunctionModLastParam,
  ///< first allowed parameter value for derived classes
  /**< convenience value for easily allow derived classes to extend
   * the set of types of modifications */
 };

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 C15FunctionMod( C15Function * const f , const int mod ,
		 Subset && which = {} ,
                 Function::FunctionValue shift = 0 ,
                 const bool cB = true )
  : C05FunctionMod( f , mod , std::move( which ) , shift , cB ) {}

 ///< constructor: takes the type of Modification and a C15Function pointer
 /**< constructor: takes the type of the Modification and a pointer to
  * the affected C15Function. Note that while the enum
  * c15function_mod_type is provided to encode the possible values of
  * modification, the field f_type is of type "int", and therefore so
  * is the parameter of the constructor, in order to allow derived
  * classes to "extend" the set of possible types of modifications. */

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~C15FunctionMod() override = default;  ///< destructor: does nothing

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the C15FunctionMod
 void print( std::ostream & output ) const override {
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

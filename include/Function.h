/*--------------------------------------------------------------------------*/
/*---------------------------- File Function.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Function class, a quite general base class of
 * all the possible types of functions. Very few assumptions are made
 * about what form the function actually has, this being demanded to
 * derived classes. Since a Function can be costly to compute (think multiple
 * integrals or min-max functions requiring the solution of a hard
 * optimization problem), the class implements the ThinComputeInterface
 * paradigm. Also, since a Function depends on a set of "active" Variable, it
 * implements the ThinVarDepInterface paradigm.
 *
 * \version 0.40
 *
 * \date 13 - 03 - 2019
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
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Function
 #define __Function  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Modification.h"
#include "ThinComputeInterface.h"
#include "ThinVarDepInterface.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Observer;  // forward definition of Observer
 class Variable;  // forward definition of Variable

/*--------------------------------------------------------------------------*/
/*----------------------- Function-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Function_TYPES Function-related types.
 *  @{ */

/*@}  end( group( Function_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Function_CLASSES Classes in Function.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Function -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of all possible real-valued functions
/** The class Function is the base class intended to represent "all possible"
 * real-valued functions.
 *
 * This base class is meant to provide only the "most basic" interface of a
 * function, i.e., only access to function *values*; things like derivatives
 * and/or specific algebraic forms of the function are demanded to derived
 * classes. As such, Function only supports a few fundamental facts:
 *
 * - a Function "reports to" one Observer [see Observer.h], i.e., sends it all
 *   its Modification;
 *
 * - a Function is influenced by a set of "active" Variables, i.e., those
 *   which contribute to it setting its (real) value: hence, the class
 *   implements the ThinVarDepInterface paradigm (i.e., derives from
 *   ThinVarDepInterface); yet, as that base class, Function makes no
 *   provisions about how this set is stored in order to leave more freedom
 *   to derived classes to implement it in specialized ways;
 *
 * - a Function must be computed, and this can be costly (think multiple
 *   integrals or min-max functions requiring the solution of a hard
 *   optimization problem), which is why the class implements the
 *   ThinComputeInterface paradigm (i.e., derives from
 *   ThinComputeInterface).
 *
 * - a Function can be approximately evaluated, which means that one can be
 *   content with only lower and/or upper estimates of the function values,
 *   provided they are "exact enough" (see details in the interface).
 *
 * IMPORTANT NOTE: any ThinVarDepInterface object ostensibly has to register
 * itself with the "active" Variable it depends upon. However,
 *
 *     Function OBJECTS ARE NOT SUPPOSED TO DO REGISTER THEIR Variable
 *
 * The rationale is that Function objects may either be a part of some other
 * object (Constraint, Objective, ... ) or, possibly, be "free-floating"
 * ones; when it is constructed, a Function has no way of knowing which of
 * the two cases it is in. Hence, the Function must not register itself
 * with its Variable. Rather,
 *
 *     IF SOME ThinVarDepInterface USES A Function TO "IMPLEMENT ITSELF",
 *     THEN THAT ThinVarDepInterface WILL HAVE TO REGISTER ITSELF IN THE
 *     Variable OF THE Function
 *
 * Thus, if the Function is used inside a Constraint, Objective, ... then it
 * will be the Constraint, Objective, ... that is registered. The burden of
 * this operation is entirely on the Constraint, Objective, ... This also
 * holds for the fact that
 *
 *     EACH TIME A Variable IS ADDED/REMOVED FROM THE Function, THE
 *     ThinVarDepInterface WILL HAVE TO REGISTER/UNREGISTER ITSELF FROM
 *     THAT Variable
 *
 * For this to be possible, the ThinVarDepInterface must be aware of the
 * addition/deletion of the Variable. This is easy if the ThinVarDepInterface
 * is the Observer of the Function, or the Observer of the Observer, ...
 * Indeed, in this case the addition/deletion of the Variable issues an
 * appropriate :FunctionModVars, which therefore can be "seen" by the
 * Observer that can react accordingly.
 *
 * The base Function class also supports providing the user with some very
 * basic information about properties of the function that only depend on
 * function values, such as:
 *
 * - [upper and/or lower semi-]continuity;
 *
 * - convexity and/or concavity;
 *
 * - existence of a (finite) Lipschitz constant.
 *
 * A fundamental design decision in SMS++ is that THE "NAME" OF A Function IS
 * ITS MEMORY ADDRESS. This means that MOVING A Function IS NOT POSSIBLE:
 * copying a Function to a different memory location makes a distinct
 * Function. */

class Function : public ThinComputeInterface , public ThinVarDepInterface {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 typedef double FunctionValue;                  ///< type of the returned value

 typedef const FunctionValue c_FunctionValue;   ///< a const FunctionValue

 typedef std::vector<FunctionValue> Vec_FunctionValue;
 ///< a std::vector of FunctionValue

 typedef const Vec_FunctionValue c_Vec_FunctionValue;
 ///< a const Vec_FunctionValue

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters of Function
 /** Public enum describing the different parameters of "int" type that a
  * Function must have (although specific Function may choose to ignore some
  * of them). The value intLastParFun is provided so that the list can be
  * easily further extended by derived classes. */

 enum int_par_type_F {
 intMaxIter = 0 ,  ///< maximum iterations for the next call to compute()
                   /**< The algorithmic parameter for setting the maximum
		    * number of iterations in the next call to compute().
 * The concept of "what exactly an iteration is" is clearly dependent on the
 * Function, and it may not even make sense for all Function. However, some
 * Function will actually be iterative processes, and therefore it makes
 * sense to offer support for this notion in the base class. The default is
 * Inf<int>() = no limits. */

 intLastParFun   ///< first allowed new int parameter for derived classes
                 /**< Convenience value for easily allow derived classes
		  * to extend the set of int algorithmic parameters. */
 };  // end( int_par_type_F )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters of Function
 /** Public enum describing the different parameters of "double" type that a
  * Function must have (although specific Function may choose to ignore some
  * of them). The value dblLastParFun is provided so that the list can be
  * easily further extended by derived classes. */

 enum dbl_par_type_F {
 dblMaxTime = 0 ,  ///< maximum time for the next call to compute()
                   /**< The parameter for setting the maximum time limit for
		    * the next call to compute(). The value is assumed to be
 * in seconds, and it's a double (so both very fast and very slow
 * computations are supported). The Function class (so far) does not
 * explicitly distinguish between "wall-clock time" and "CPU time", which may
 * be rather different especially in a parallel environment. The default is
 * Inf<double>(). */

 dblRelAcc ,    ///< relative accuracy for the value of the function
		/**< The parameter for setting the *relative* accuracy
		 * required to the function value. That is, if both an upper
 * bound "ub" [see get_upper_estimate()] and a lower bound "lb" [see
 * get_lower_estimate()] on the value have been found, then compute() can
 * stop as soon as 
 *
 *    ub - lb <= dblRelAcc * max( abs( lb ) , 1 )
 *
 * The default is 1e-6. */

 dblAbsAcc   ,  ///< absolute accuracy for the value of the function
		/**< The parameter for setting the *absolute* accuracy
		 * required to the function value. That is, if both an upper
 * bound "ub" [see get_upper_estimate()] and a lower bound "lb" [see
 * get_lower_estimate()] on the value have been found, then compute() can
 * stop as soon as 
 *
 *    ub - lb <= dblRelAcc
 *
 * The default is Inf<OFValue>(), which is intended to mean that the only
 * working accuracy is the relative one. */

 dblUpCutOff ,  ///< upper cutoff on the value of the function
		/**< The parameter for setting the "upper cut off" of the
		 * computation; that is, if a lower bound "lb" [see
 * get_lower_estimate()] on the value has been found, then compute() can
 * stop as soon as 
 *
 *   lb >= dblUpCutOff
 *
 * This is a *certificate* that the value is at *least* dblUpCutOff. The
 * default is Inf<OFValue>(), i.e., no upper cut off. */

 dblLwCutOff ,  ///< lower cutoff on the value of the function
		/**< The parameter for setting the "lower cut off" of the
		 * computation; that is, if an upper bound "ub" [see
 * get_upper_estimate()] on the value has been found, then compute() can
 * stop as soon as 
 *
 *   ub <= dblLwCutOff
 *
 * This is a *certificate* that the value is at *most* dblLwCutOff. The
 * default is -Inf<OFValue>(), i.e., no lower cut off. */

 dblLastParFun   ///< first allowed new double parameter for derived classes
                 /**< Convenience value for easily allow derived classes
		  * to extend the set of double algorithmic parameters. */
 };  // end( dbl_par_type_F )

/*@}------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of Function, taking the Observer it reports to
 /** Constructor of Function. It accepts a pointer to the Observer which the
  * Function reports to, defaulting to nullptr so that this can be used as the
  * void constructor. If nullptr is passed, then register_Observer() [see
  * below] can be used later to initialize it, unless one wants to keep this
  * Function a "free floating" one. If a non-nullptr Observer is passed,
  * register_Observer() is called (because this may do other things apart from
  * just setting the pointer, such as registering the Observer, if it also is
  * a ThinVarDepInterface, to the Variable of the Function). */

 Function( Observer * const observer = nullptr ) : ThinComputeInterface() ,
  ThinVarDepInterface() , f_Observer( nullptr ) {
  if( observer )
   register_Observer( observer );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: it cannot be used, but it is not deleted
 /** The copy constructor of Function is not currently implemented, but it
  * cannot be deleted because it is required to resize() empty vectors of
  * :Function. */

 Function( const Function & ) : ThinComputeInterface() ,
                                ThinVarDepInterface() {
  throw( std::logic_error( "copy constructor of Function invoked" ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 /** The destructor of the base Function class has nothing to do. However,
  * the destructor of any derived class, besides deallocating all its data
  * structures, will have to un-register the Observer of the :Function--rather
  * than the :Function itsefl--from all its "active" Variable; this provided
  * the Observer also is a ThinVarDepInterface. A simple and general way to
  * do this is to call register_Observer() (with default = nullptr argument),
  * see below. This provided that the clear() method has not been invoked
  * before, in which case un-registering is not required. */

 virtual ~Function() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the pointer to the Observer of this Function
 /** Method to set the pointer to the Observer of this Function. If no
  * pointer is provided (either in the constructor or here), or the Function
  * is "unregistered" from the current observe by calling the method with
  * nullptr (default), then the Function is left "free floating", which means
  * that no Modification is ever produced and no Block/Solver ever gets
  * informed of any change occurring in the Function.
  *
  * Note that an Observer (or an Observer of the Observer, ...) which also is
  * a ThinVarDepInterface may register *itself* into the Variable of the
  * Function, which is made possible by the fact that the Function does not do
  * this itself. However, this also means that if the Observer is changed, the
  * previous Observer must be unregistered prior to the new one can take its
  * place. Again
  *
  *    IT IS COMPLETELY A RESPONSIBILITY OF THE Observer (WHICH IS ALSO A
  *    ThinVarDepInterface) TO HANDLE THE REGISTRATION/UNREGISTRATION OF
  *    ITSELF FROM THE Variable OF THE Function *IF* IT CHOOSES TO DO SO
  *
  * The Function has no clue whether this is happening and therefore no role
  * in all this. This implies that the change/removal of an Observer from the
  * Function must also be communicated in some way to the Observer itself,
  * but this necessarily requires an Observer-specific method, in that there
  * cannot be support for this in the base Observer class (not all Observer
  * are ThinVarDepInterface, and not all ThinVarDepInterface use a Function).
  */

 virtual void register_Observer( Observer * const observer = nullptr ) {
  f_Observer = observer;
  }

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF A Function --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Function
 * The methods in this section allow to retrieve 0th-order information (the
 * value) about the point where the Function have been last evaluated by
 * calling compute(). Calls to to any of these methods are therefore
 * associated with that point: it is expected that the point (i.e., the
 * values of the active Variable) at the moment in which these methods are
 * called is the same as the one in which compute() was called. In other
 * words, these methods are  "extensions" of compute(), used to extract
 * further information (likely) computed in there, and therefore in principle
 * an extension to the fundamental rule regarding compute() is to be enforced:
 *
 *   between a call to compute() and all the calls to these methods intended
 *   to retrieve information about values computed in that point, no
 *   changes must occur to the Function which change the answer that
 *   compute() was supposed to compute
 *
 * The rule leaves scope for some changes occurring, although these must
 * ensure that the answer is not affected; see comments to compute(). As
 * always,
 *
 *               IT IS UNIQUELY THE CALLER'S RESPONSIBILITY
 *                 TO ENSURE THAT THE RULE IS RESPECTED
 * @{ */

 /// compute the Function
 /** Pure virtual method: it has to compute the Function and possibly store
  * the result into some protected field, so that get_value() and related
  * methods can be used to read it. Evaluating a Function can be a lengthy
  * task, involving e.g. numerical integrals or solving "hard" optimization
  * problems, which is why Function are not computed by default, and need to
  * be computed explicitly with this method. This is also why this implements
  * the compute() method of ThinComputeInterface; see the comments on the base
  * class, in particular about the rules concerning what can change during a
  * call to this method, and between two calls depending on the value of the
  * changedvars parameter.
  *
  * This method can compute the value only approximately, according to the
  * parameters set with set_par(). This means in particular that if compute()
  * returns a value comprised between kOK and kError, extremes excluded, the
  * computation may have been forced to stop early on, e.g. by a limit imposed
  * on the available computational resources. By calling compute() again, the
  * Function may further proceed in the computation process, possibly
  * providing a kOK-type answer, which means that the Function is computed
  * with the required accuracy.
  *
  * However, the base class provides as much as possible implementations for
  * the methods corresponding to the case where the value is (efficiently)
  * computed exactly, and therefore all the parameters can be ignored. */

 virtual int compute( bool changedvars = true ) override = 0;

/*--------------------------------------------------------------------------*/
 /// returns the value of the Function
 /** Pure virtual method that returns the value of the Function that was
  * computed in the most recent call to compute(); if the latter has never
  * been invoked, then the value returned by this method is meaningless. */

 virtual FunctionValue get_value( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a lower estimate of the value of the Function
 /** Virtual method that returns a lower estimate of the Function value
  * obtained by the most recent call to the compute() method. If compute()
  * has never been invoked, then the value returned by this method is
  * meaningless. The method is given a default implementation suited to
  * "easy" functions for which get_value() always returns "exact" (save
  * possibly unavoidable numerical errors) values. */

 virtual FunctionValue get_lower_estimate( void ) const {
  return( get_value() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns an upper estimate of the value of the Function
 /** Virtual method that returns an upper estimate of the Function value
  * obtained by the most recent call to the compute() method. If compute()
  * has never been invoked, then the value returned by this method is
  * meaningless. The method is given a default implementation suited to
  * "easy" functions for which get_value() always returns "exact" (save
  * possibly unavoidable numerical errors) values. */

 virtual FunctionValue get_upper_estimate( void ) const {
  return( get_value() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (global) Lipschitz constant for the Function
 /** Method that returns a (global) Lipschitz constant L for this Function,
  * i.e., a (real) scalar L such that
  *
  *   | f( x ) - f( y ) | <= L * | x - y |
  *
  * for all x and y in the domain of the Function. By default, the method
  * returns Inf<FunctionValue>(), which means that the Function does
  * *not* have a Lipschitz constant. Note that a finite Lipschitz constant
  * implies that is_continuous() must return true. */

 virtual FunctionValue get_Lipschitz_constant( void ) {
  return( std::numeric_limits<FunctionValue>::infinity() );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this Function is convex
 /** Method that returns true if and only if this function is convex. The
  * default is false (convexity being good for optimization, in particular
  * minimization, often too good to be true). */

 virtual bool is_convex( void ) const { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if and only if this Function is concave
 /** Method that returns true if and only if this function is concave. The
  * default is false (concavity being good for optimization, in particular
  * maximization, often too good to be true). */

 virtual bool is_concave( void ) const { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this Function is linear
 /** Method that returns true if and only if this Function is linear. The
  * base implementation of the class exploits the fact that the only class
  * of functions that are both convex and concave is precisely that of
  * linear functions (and therefore this method is not very useful ... ) */

 virtual bool is_linear( void ) const {
  return( this->is_convex() && this->is_concave() );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this Function is lower semi-continuous
 /** Method that returns true if and only if this function is lower
  * semi-continuous. The default is true (continuity being an important
  * property for optimization). */

 virtual bool is_lower_semicontinuous( void ) const { return( true ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if and only if this Function is upper semi-continuous
 /** Method that returns true if and only if this function is upper
  * semi-continuous. The default is true (continuity being an important
  * property for optimization). */

 virtual bool is_upper_semicontinuous( void ) const { return( true ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if and only if this Function is continuous
 /** Method that returns true if and only if this function is continuous.
  * The base implementation of the class exploits the fact that by
  * this means that it is both upper semi-continuous and lower
  * semi-continuous (and therefore this method is not very useful ... ) */

 bool is_continuous( void ) const {
  return( this->is_lower_semicontinuous() &&
	  this->is_upper_semicontinuous() );
  }

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Function
 *  @{ */

 virtual idx_type get_num_int_par( void ) const override
 {
  return( idx_type( intLastParFun ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type get_num_dbl_par( void ) const override
 {
  return( idx_type( dblLastParFun ) );
  }

/*--------------------------------------------------------------------------*/
 
 virtual int get_dflt_int_par( const idx_type par ) const override
 {
  if( par == intMaxIter )
   return( std::numeric_limits<int>::infinity() );
  else
   return( ThinComputeInterface::get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 
 virtual double get_dflt_dbl_par( const idx_type par ) const override
 {
  switch( par ) {
   case( dblMaxTime ):  return( std::numeric_limits<double>::infinity() );
   case( dblRelAcc ):   return( 1e-6 );
   case( dblAbsAcc ):  
   case( dblUpCutOff ): return( std::numeric_limits<double>::infinity() );
   case( dblLwCutOff ): return( - std::numeric_limits<double>::infinity() );
   }
  return( ThinComputeInterface::get_dflt_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 virtual idx_type int_par_str2idx( const std::string & name ) const override
 {
  if( name == "intMaxIter" )
   return( intMaxIter );
  else
   return( ThinComputeInterface::int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual idx_type dbl_par_str2idx( const std::string & name ) const override
 {
  if( name == "dblMaxTime" )
   return( dblMaxTime );
  if( name == "dblRelAcc" )
   return( dblRelAcc );
  if( name == "dblAbsAcc" )
   return( dblAbsAcc );
  if( name == "dblUpCutOff" )
   return( dblUpCutOff );
  if( name == "dblLwCutOff" )
   return( dblLwCutOff );

  return( ThinComputeInterface::dbl_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 virtual const std::string & int_par_idx2str( const idx_type idx )
  const override
 {
  static const std::string par = "intMaxIter";
  if( idx == intMaxIter )
   return( par );
  else
   return( ThinComputeInterface::int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual const std::string & dbl_par_idx2str( const idx_type idx )
  const override
 {
  static const std::vector< std::string > pars =
   { "dblMaxTime" , "dblRelAcc" , "dblAbsAcc" , "dblUpCutOff" , "dblLwCutOff"
   };

  if( ( idx >= dblMaxTime ) && ( idx <= dblLwCutOff ) )
   return( pars[ idx ] );
  else
   return( ThinComputeInterface::dbl_par_idx2str( idx ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------- METHODS FOR READING THE DATA OF THE Function ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the Function
 *  @{ */

 /// returns the pointer to the Observer of this Function
 Observer * get_Observer( void ) const { return( f_Observer ); }

/*@} -----------------------------------------------------------------------*/
/*----------- METHODS FOR LOADING, PRINTING & SAVING THE Function ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Function
 */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the operator<<()
  * is defined for each Function, but its behavior can be customized by
  * derived classes. */

 friend std::ostream& operator<< ( std::ostream& out , const Function &o ) {
  o.print( out );
  return( out );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the Function on an ostream
 /** Protected method intended to print information about the Function; it is
  * virtual so that derived classes can print their specific information in
  * the format they choose. */

 virtual void print( std::ostream &output ) const {
  output << "Function [" << this << "]" << " with " << get_num_active_var()
	 << " active variables";
  }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Observer *f_Observer = nullptr;   ///< the Observer of this Function
 
/*--------------------------------------------------------------------------*/

  };  // end( class( Function ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FunctionMod -----------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications that are specific to a Function
/** Derived class from AModification to describe modifications to a Function.
 * This is the base class for all Modifications related to Functions.
 *
 * It is important to notice that a FunctionMod contain sufficient information
 * regarding the modification that was made on the Function. This means that
 * the information provided by a FunctionMod can be safely used to update
 * previous knowledge about the Function.
 *
 * This base class defines the simplest type of changes, which are those to
 * the value of the function. There are four possible cases of such changes 
 * that are covered by this Modification, which are encoded in the field
 * f_shift in the following way:
 *
 * - NaN, e.g. as what is reported by
 *   std::numeric_limits<FunctionValue>::quiet_NaN() or by
 *   std::numeric_limits::signaling_NaN<FunctionValue>()): the value of the
 *   Function has changed "unpredictably" all over the space, any previously
 *   computed value is no longer reliable. Although the convenient constexpr
 *   "NaNshift" is defined in the class, note that testing if f_shift is NaN
 *   must *not* be done with "f_shift == NaNshift", but rather with
 *   std::isnan( f_shift ).
 *
 * - Any finite non-NaN number: conversely, the value of the Function has
 *   changed in a very predictable way: computing the value of the Function at
 *   any point now returns f_v + v_shift, where f_v is the value that would
 *   have been returned prior to the Modification. It should be noted that
 *   f_shift = 0 does not really have a sense in this case as it would not
 *   be a change in the Function; however, the value is still allowed for
 *   uniformity with FunctionModVars [see].
 *
 * - +Infty (= std::numeric_limits<FunctionValue>::infinity(), for which
 *   the convenience constexpr "INFshift" is defined): this means
 *   that the value of the Function has changed "unpredictably but
 *   monotonically upwards": computing the value of the Function at any
 *   point now returns a value that is surely greater than or equal to the
 *   value that would have been returned prior to the Modification.
 *
 * - -Infty (= - std::numeric_limits<FunctionValue>::infinity(), i.e.,
 *   "-INFshift" exploiting the defined convenience constexpr): this means
 *   that the value of the Function has changed "unpredictably but
 *   monotonically downwards": computing the value of the Function at any
 *   point now returns a value that is surely smaller than or equal to the
 *   value that would have been returned prior to the Modification. */

class FunctionMod : public AModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 typedef Function::FunctionValue FunctionValue;
 ///< "import" FunctionValue from Function

/*----------------------------- CONSTANTS ----------------------------------*/

 static constexpr FunctionValue NaNshift
                            = std::numeric_limits<FunctionValue>::quiet_NaN();
 ///< convenience constexpr for "NaN", *not* to be used with ==
 static constexpr FunctionValue INFshift
                             = std::numeric_limits<FunctionValue>::infinity();
 ///< convenience constexpr for "Infty"
 
/*---------------------------- CONSTRUCTOR ---------------------------------*/

 FunctionMod( Function * const f , const FunctionValue shift = NaNshift ,
	      const bool cB = true )
  : AModification( cB ) , f_function( f ) , f_shift( shift ) { }

 ///< constructor: takes a Function pointer and a shift
 /**< Constructor: takes a pointer to the affected Function and the value of
  * the shift. If shift is finite, then it indicates that the Function value
  * has been shifted by a constant. This means that if the Function value at
  * a given point was f_v before this Modification, then the correct value is
  * now f_v + shift. If shift is Inf<FunctionValue>(), then it means that the
  * Function has been modified in an unpredictable way, so that any prior
  * information regarding the Function can be disregarded. */

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~FunctionMod() { }  ///< destructor

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 Function *f_function;
 ///< pointer to the Function where the Modification occurs

 FunctionValue f_shift;
 ///< Amount the value of the function has been shifted
 /**< This field encloses four types of changes, depending on its value:
 *
 * - NaN: the value of the Function has changed "unpredictably" all over the
 *   space, any previously computed value is no longer reliable. Although the
 *   convenient constexpr "NaNshift" is defined in the class, note that
 *   testing if f_shift is NaN must *not* be done with "f_shift == NaNshift", *    but rather with std::isnan( f_shift ).
 *
 * - Any finite non-NaN number: conversely, the value of the Function has
 *   changed in a very predictable way: computing the value of the Function at
 *   any point now returns f_v + v_shift, where f_v is the value that would
 *   have been returned prior to the Modification. It should be noted that
 *   f_shift = 0 does not really have a sense in this case as it would not
 *   be a change in the Function; however, the value is still allowed for
 *   uniformity with FunctionModVars.
 *
 * - +INFshift: this means that the value of the Function has changed
 *   "unpredictably but monotonically upwards": computing the value of the
 *   Function at any point now returns a value that is surely greater than or
 *   equal to the value that would have been returned prior to the
 *   Modification.
 *
 * - -INFshif: this means that the value of the Function has changed
 *   "unpredictably but monotonically downwards": computing the value of the
 *   Function at any point now returns a value that is surely smaller than or
 *   equal to the value that would have been returned prior to the
 *   Modification. */

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FunctionMod

 virtual inline void print( std::ostream &output ) const override
 {
  output << "FunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
   output << "] on Function " << f_function << " ]: f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= -INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( FunctionMod ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FunctionModVars -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe adding/removing Variable of a Function
/** Derived class from AModification to describe changes of a Function that
 * involve adding/removing the Variable "active" in it. The class holds the
 * subset of affected Variable under the form of the field v_vars, a
 * std::vector<Variable *>.
 *
 * The FunctionModVars also tells whether the Function is "quasi-additive"
 * in the added/removed Variable, which is an important property to allow
 * re-using previously computed function values. 
 *
 * The quasi-additivity property is defined as follows.
 *
 * Suppose that new Variables are added to the Function. Let f_old( x ) be
 * the Function, and x its vector of "active" Variable, before the
 * Modification. Let y be the vector of Variable that were added, and
 * f( x , y ) be the Function after the modification. We say that the
 * Variable y are quasi-additively added to the function if and only if
 *
 *    f( x , 0 ) = f_old( x ) + f_shift   for all x
 *
 * In plain words, the new value of the Function just a constant shift to the
 * old value of the Function (identical if f_shift == 0) whenever all the new
 * Variable are fixed to their default value (0).
 *
 * Similarly, suppose that Variables y are removed from the Function. Let
 * f_old( x , y ) be the Function before the modification, and f( x ) be the
 * Function after the modification. We say that the variables y are
 * quasi-additively removed from the Function if and only if
 *
 *    f( x ) = f_old( x , 0 ) + f_shift    for all x
 *
 * Again, the new value of the Function just a constant shift to the old value
 * of the Function (identical if f_shift == 0) whenever the latter were
 * computed at points where all the removed Variable are fixed to their
 * default value (0).
 *
 * Note that the property depends on the specific Variable being
 * added/removed and what how exactly "adding" and "removing" means. For
 *
 *   f_old( x , y , z ) = x y + z
 *
 * removing z leads to f( x , y ) = x y, and this clearly is quasi-additive,
 * while removing y may lead to f( x , z ) = x + z which is not. Yet,
 * removing (all algebraic terms containing) y could also lead to
 * f( x , z ) = z, which is a quasi-additive removal. Of course, an additivw
 * function f( x , y ) = f_1( x ) + f_2( y ) is quasi additive for whatever
 * reasonable concept of addition (of other additive terms) and removal one
 * can think of, but the concept also covers non-additive functions:
 *
 * f( x , y ) = f_1( x ) + f_2( y ), can be dealt with:
 *
 *     f_old( x ) = e^x     and      f( x , y ) = e^( x + y )
 *
 * is quasi-additive, provided of course that the removal of y from f()
 * brings back to f_old() rather than deliting all the terms containing it.
 *
 * Also, note that for the shift to be finite, *all* the involved Variable
 * must be quasi-additive. Yet, if some of them were, and some of them were
 * not, there would be little solace in having two different Modifiction,
 * one with finite shift and one with infinite one, since at the end of the
 * day the result would still be that the new value of the Function is
 * utterly unknown.
 *
 * The fact that 0 is chosen as the "special" value for the Variable is
 * somehow arbitrary, but if a Function would be quasi-additive in some
 * Variable y when that Variable is fixed to some \bar{y} \neq 0, it would
 * be simple to make it quasi-additive by translating the Variable as
 * y - \bar{y}. Anyway, 0 is a reasonable "special" value. An important case
 * of quasi-additive function is that of a structured optimization problem
 *
 *   (P)  max \{ c u : A u = b , u \in U \}
 *
 * and of its (convex) Lagrangian function
 *
 *   (P( x ))  f( x ) = max \{ c u + x ( b - A u ) : u \in U \}
 *
 * Again, f() can *not* be written in an additive way, i.e., f( x ) =
 * f_1( x_1 ) + ... + f_n( x_n ). Yet, setting x_i = 0 or removing the
 * i-th constraint A_i u = b_i from (P), thereby eliminating the x_i
 * variable for good, has exactly the same effect.
 *
 * The value of the field f_shift signals whether or not the just occurred
 * Modification was a quasi-additive one, with the following encoding:
 *
 * - NaN, e.g. as what is reported by
 *   std::numeric_limits<FunctionValue>::quiet_NaN() or by
 *   std::numeric_limits::signaling_NaN<FunctionValue>()): the Modification
 *   was not quasi-additive one, and the value of the Function has changed
 *   "unpredictably" all over the space. Although the convenient constexpr
 *   "NaNshift" is defined in the class, note that testing if f_shift is NaN
 *   must *not* be done with "f_shift == NaNshift", but rather with
 *   std::isnan( f_shift ).
 *
 * - Any finite non-NaN number (comprised 0, which makes full sense): the
 *   Modification was a quasi-additive one, with v_shift being the value of
 *   the shift.
 *
 * - +Infty (= std::numeric_limits<FunctionValue>::infinity(), for which
 *   the convenience constexpr "INFshift" is defined): the Modification was
 *   not a quasi-additive one, but while the value of the Function has
 *   changed "unpredictably" all over the space, the change is "upward
 *   monotone" in the sense that:
 *   = for addition, the value of the Function at any point ( x , 0 ) is
 *     surely greater than or equal to the value that the Function had at x;
 *   = for deletion, the value of the Function at any point x is surely
 *     greater than or equal to the value that the Function had at ( x , 0 ).
 *
 * - -Infty (= -std::numeric_limits<FunctionValue>::infinity(), i.e.,
 *   "-INFshift" exploiting the defined convenience constexpr): the
 *   Modification was not a quasi-additive one, but while the value of the
 *   Function has changed "unpredictably" all over the space, the change is
 *   "downward monotone" in the sense that:
 *   = for addition, the value of the Function at any point ( x , 0 ) is
 *     surely less than or equal to the value that the Function had at x;
 *   = for deletion, the value of the Function at any point x is surely
 *     less than or equal to the value that the Function had at ( x , 0 ).
 */

class FunctionModVars : public AModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 typedef Function::FunctionValue FunctionValue;
 ///< "import" FunctionValue from Function

 /// Definition of the possible type of Modification
 enum function_mod_variables_type {
  AddVar    , ///< addition of variables
  RemoveVar , ///< deletion of variables
  FunctionModVarsLastParam
  ///< First allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend
   * the set of types of modifications. */
  };

/*----------------------------- CONSTANTS ----------------------------------*/

 static constexpr FunctionValue NaNshift =
                              std::numeric_limits<FunctionValue>::quiet_NaN();
 ///< convenience constexpr for "NaN", *not* to be used with ==
 static constexpr FunctionValue INFshift =
                               std::numeric_limits<FunctionValue>::infinity();
 ///< convenience constexpr for "Infty"
 
/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes the type, a Function *, and the changed Variable
 /** constructor: takes the type of the Modification, a pointer to the
  * affected Function, and the subset of affected Variable under the form
  * of a std::vector<Variable *>. As the the && tells, the vector "becomes
  * property" of the FunctionModVars object. Note that while the enum
  * function_mod_variables_type is provided to encode the possible values of
  * Modification, the field f_type is of type "int", and therefore so is
  * the parameter of the constructor, in order to allow derived classes to
  * "extend" the set of possible types of FunctionModVars. The boolean
  * parameter ordered allows to tell whether or not the vars set passed
  * to the constructor is ordered: if not this is done right away, so that
  * whomever receives the Modification can assume v_vars always is. */

 FunctionModVars( Function * const f , const int mod ,
		  Vec_p_Var && vars , const bool ordered = true ,
		  const FunctionValue shift = NaNshift ,
		  const bool cB = true )
  : AModification( cB ) , f_function( f ) , f_shift( shift ) ,
   f_type( mod ) , v_vars( std::move( vars ) )
 {
  if( ! ordered )
   std::sort( v_vars.begin() , v_vars.end() );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, it has nothing to do
 virtual ~FunctionModVars() { }

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 Function *f_function;  ///< the Function where the Modification occurs

 FunctionValue f_shift;   ///< tells if the Modification was quasi-additive
 /**< The value of the field f_shift signals whether or not the just occurred
  * Modification was a quasi-additive one, with the following encoding:
  *
  * - NaN: the Modification was not quasi-additive one, and the value of the
  *   Function has changed "unpredictably" all over the space. Although the
  *   convenient constexpr "NaNshift" is defined in the class, note that
  *   testing if f_shift is NaN must *not* be done with "f_shift == NaNshift",
  *   but rather with std::isnan( f_shift ).
  *
  * - Any finite non-NaN number (comprised 0, which makes full sense): the
  *   Modification was a quasi-additive one, with v_shift being the value of
  *   the shift.
  *
  * - INFshift: the Modification was not a quasi-additive one, but while the
  *   value of the Function has changed "unpredictably" all over the space,
  *   the change is "upward monotone".
  *
  * - -INFshift: the Modification was not a quasi-additive one, but while the
  *   value of the Function has changed "unpredictably" all over the space,
  *   the change is "downward monotone". */

 int f_type;        ///< "type" of the Modification
 Vec_p_Var v_vars;  ///< vector of pointers to affected Variable

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the FunctionModVars

 virtual inline void print( std::ostream &output ) const override
 {
  output << "FunctionModVars[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function << " ]: ";
  if( std::isnan( f_shift ) )
   output << "non quasi-additively (+-)";
  else
   if( f_shift >= INFshift )
    output << "non quasi-additively (+)";
   else
    if( f_shift <= -INFshift )
     output << "non quasi-additively (-)";
    else
     output << "quasi-additively (" << f_shift << ") ";

  if( f_type == AddVar )
   output << "adding ";
  else
   output << "deleting ";

  output  << v_vars.size() << " variables" << std::endl;
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( FunctionModVars ) )

/*@}  end( group( Function_CLASSES ) ) -------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Function.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Function.h ---------------------------*/
/*--------------------------------------------------------------------------*/

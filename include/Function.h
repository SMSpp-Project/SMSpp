/*--------------------------------------------------------------------------*/
/*---------------------------- File Function.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Function class, a quite general base class of all the
 * possible types of real-valued functions. Very few assumptions are made
 * about what form the function actually has, this being demanded to derived
 * classes. Since a Function can be costly to compute (think multiple 
 * integrals or min-max functions requiring the solution of a hard
 * optimization problem), the class implements the ThinComputeInterface
 * paradigm. Also, since a Function depends on a set of "active" Variable, it
 * implements the ThinVarDepInterface paradigm.
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

#ifndef __Function
 #define __Function   /* self-identification: #endif at the end of the file */

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

namespace SMSpp_di_unipi_it {

class Observer;  // forward definition of Observer

class Variable;  // forward definition of Variable

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
 *   provisions about how this set is stored in order to leave complete
 *   freedom to derived classes to implement as they best see fit.
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

class Function : public ThinComputeInterface , public ThinVarDepInterface
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 using FunctionValue = double;  ///< type of the returned value

 using c_FunctionValue = const FunctionValue;  ///< a const FunctionValue

 /// a std::vector of FunctionValue
 using Vec_FunctionValue = std::vector< FunctionValue >;

 /// a const Vec_FunctionValue
 using c_Vec_FunctionValue = const Vec_FunctionValue;

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters of Function
 /** Public enum "extending" int_par_type_TCI to describe the different
  * algorithmic parameters of "int" type that any Function should reasonably
  * have on top of those defined by ThinComputeInterface (although specific
  * :Function may choose to ignore some of them); currently none. The value
  * intLastParFun is provided so that the list can be easily extended by
  * derived classes. */

 enum int_par_type_F {
  intLastParFun = intLastAlgParTCI
                  ///< first allowed new int parameter for derived classes
                  /**< Convenience value for easily allow derived classes
		   * to extend the set of int algorithmic parameters. */

 };  // end( int_par_type_F )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters of Function
 /** Public enum "extending" dbl_par_type_TCI to describe the different
  * algorithmic parameters of "double" type that any Function should
  * reasonably have on top of those defined by ThinComputeInterface (although 
  * specific Function may choose to ignore some of them). The value
  * dblLastParFun is provided so that the list can be easily extended by
  * derived classes. */

 enum dbl_par_type_F {
  dblRelAcc = dblLastAlgParTCI  ,
                 ///< relative accuracy for the value of the Function
                 /**< The parameter for setting the *relative* accuracy
		  * required to the Function value. That is, if both an upper
   * bound "ub" [see get_upper_estimate()] and a lower bound "lb" [see
   * get_lower_estimate()] on the value have been found, then compute() can
   * stop as soon as
   *
   *    ub - lb <= dblRelAcc * max( abs( lb ) , 1 )
   *
   * The default is 1e-6. */

  dblAbsAcc ,  ///< absolute accuracy for the value of the Function
               /**< The parameter for setting the *absolute* accuracy
		* required to the function value. That is, if both an upper
   * bound "ub" [see get_upper_estimate()] and a lower bound "lb" [see
   * get_lower_estimate()] on the value have been found, then compute() can
   * stop as soon as
   *
   *    ub - lb <= dblRelAcc
   *
   * The default is Inf< OFValue >(), which is intended to mean that the only
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
   * default is Inf< OFValue >(), i.e., no upper cut off. */

  dblLwCutOff ,  ///< lower cutoff on the value of the function
                 /**< The parameter for setting the "lower cut off" of the
                  * computation; that is, if an upper bound "ub" [see
   * get_upper_estimate()] on the value has been found, then compute() can
   * stop as soon as
   *
   *   ub <= dblLwCutOff
   *
   * This is a *certificate* that the value is at *most* dblLwCutOff. The
   * default is - Inf< OFValue >(), i.e., no lower cut off. */

  dblLastParFun   ///< first allowed new double parameter for derived classes
                  /**< Convenience value for easily allow derived classes
		   * to extend the set of double algorithmic parameters. */
 };  // end( dbl_par_type_F )

/*--------------------------------------------------------------------------*/
 /// public enum for the string algorithmic parameters
 /** Public enum describing the different algorithmic parameters of "string"
  * type that any Function should reasonably have (none so far). The value
  * strLastParFun is provided so that the list can be easily extended by
  * derived classes. */

 enum str_par_type_F {
  strLastParFun = strLastAlgParTCI
  ///< first allowed new string parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of string algorithmic parameters. */
  };  // end( str_par_type_F )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-int algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of int that any Function should reasonably have (none so far).
  * The value vintLastAlgPar is provided so that the list can be easily
  * extended by derived classes. */

 enum vint_par_type_F {
  vintLastParFun = vintLastAlgParTCI
  ///< first allowed new  vector-of-int parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int parameters. */
  };  // end( vint_par_type_F )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-double algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of double that any Function should reasonably have (none so far).
  * The value vdblLastParFun is provided so that the list can be easily
  * extended by derived classes. */

 enum vdbl_par_type_F {
  vdblLastParFun = vdblLastAlgParTCI
  ///< first allowed new  vector-of-double parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double parameters. */
  };  // end( vdbl_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-string algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of string that any Function should reasonably have (none so far).
  * The value vstrLastParFun is provided so that the list can be easily
  * extended by derived classes. */

 enum vstr_par_type_F {
  vstrLastParFun = vstrLastAlgParTCI
  ///< first allowed new vector-of-string parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string parameters. */
  };  // end( vstr_par_type_S )

/** @} ---------------------------------------------------------------------*/
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

 explicit Function( Observer * observer = nullptr )
  : ThinComputeInterface() , ThinVarDepInterface() , f_Observer( observer ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: it cannot be used, but it is not deleted
 /** The copy constructor of Function is not currently implemented, but it
  * cannot be deleted because it is required to resize() empty vectors of
  * :Function. */

 Function( const Function & )
  : ThinComputeInterface() , ThinVarDepInterface() {
  throw( std::logic_error( "copy constructor of Function invoked" ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 /** The destructor of the base Function class has nothing to do. Indeed,
  * unlike other ThinVarDepInterface, a Function should *not* un-register
  * itself from all its "active" Variable. If the Function is used by an
  * Observer that to "implement itself", then the Observer will be
  * registered in the Function's "active" Variable, and un-registration will
  * have to be performed in its destructor. This unless the clear() method
  * has been invoked, in which case un-registering is not required (and,
  * indeed, clear() deletes the list of "active" Variable in the Function,
  * so the Observer cannot un-register from them even if it tried). */

 ~Function() override = default;

/** @} ---------------------------------------------------------------------*/
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

 virtual void register_Observer( Observer * observer = nullptr ) {
  f_Observer = observer;
  }

/** @} ---------------------------------------------------------------------*/
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
 *
 * Note that several methods in this section (get_Lipschitz_constant(),
 * get_global_[lower/upper]_bound(), is_convex(), is_concave(), ...) refer to
 * properties of the Function that may be true at a certain moment, but may
 * become false when the Function is modified. The guidelines is therefore
 * that each time a "major" Modification is issued (say a FunctionMod telling
 * that the function has changed "a lot"), the properties that could
 * potentially change should be re-checked. For instance, adding and removing
 * Variable, as signalled by a FunctionModVar, almost certainly change the
 * Lipschitz constant, which must therefore be re-checked (if needed). On
 * the other hand, if the Function is only shifted by a fixed constant then
 * the Lipschitz constant does not change, and if it changes monotonically
 * upwards [downwards] then any previous global valid lower [upper] bound at
 * the very least remains valid, although of course it may have changed
 * upwards [downwards] as well, so one may want to re-check it anyway.
 * Other properties [like convexity and linearity] should be more "stable",
 * but the idea is that the methods returning them should be "quick", and
 * therefore checking them often should not be a big issue.
 *
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

 int compute( bool changedvars = true ) override = 0;

/*--------------------------------------------------------------------------*/
 /// returns the value of the Function
 /** Pure virtual method that returns the value of the Function that was
  * computed in the most recent call to compute(); if the latter has never
  * been invoked, then the value returned by this method is meaningless. */

 [[nodiscard]] virtual FunctionValue get_value( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a lower estimate of the value of the Function
 /** Virtual method that returns a lower estimate of the Function value
  * obtained by the most recent call to the compute() method. If compute()
  * has never been invoked, then the value returned by this method is
  * meaningless. The method is given a default implementation suited to
  * "easy" functions for which get_value() always returns "exact" (save
  * possibly unavoidable numerical errors) values. */

 [[nodiscard]] virtual FunctionValue get_lower_estimate( void ) const {
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

 [[nodiscard]] virtual FunctionValue get_upper_estimate( void ) const {
  return( get_value() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the "constant term" of the Function
 /** Virtual method that returns "the constant term" of the Function. Each
  * Function can be seen to have a "constant term", i.e., to have the form
  *
  *    f( x ) = f_0 + h( x )
  *
  * with f_0 constant and h( x ) depending on x. Of course for a general
  * Function one can choose f_0 arbitrarily by changing h(), but for many
  * Function the "constant term" is clearly defined. In fact, the only
  * change that the basic FunctionMod [see] supports can be seen, for the
  * special case where f_shift is finite, as saying that f_0 has changed,
  * while the other cases represent all the other possible changes in h().
  * This method provides access to "the constant term" f_0. Since this may
  * not really make sense for all Function, a default implementation is
  * provided that just returns 0, a non-invasive constant term. */

 [[nodiscard]] virtual FunctionValue get_constant_term( void ) const {
  return( 0 );
  }

/*--------------------------------------------------------------------------*/
 /// returns a valid global lower bound on the Function value
 /** The Function may know that it is bounded below on its domain, and be
  * able to (cheaply) compute a finite number guaranteed to be <= than any
  * value that get_value() can possibly return. If so, such a value should
  * be returned by this method. The base class implementation returns the
  * always safe - Inf< FunctionValue >().
  *
  * This method is not const because the computation of the global lower bound
  * may not be cheap. The standard approach is then to only compute it if the
  * method is called, but on the other hand to "cache" the value to answer
  * quickly if no change has happened that changed it. Thus, the method has
  * to be able to write into the fields of the class. */

 virtual FunctionValue get_global_lower_bound( void ) {
  return( - Inf< FunctionValue >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a valid global upper bound on the Function value
 /** The Function may know that it is bounded above on its domain, and be
  * able to (cheaply) compute a finite number guaranteed to be >= than any
  * value that get_value() can possibly return. If so, such a value should
  * be returned by this method. The base class implementation returns the
  * always safe Inf< FunctionValue >().
  *
  * This method is not const because the computation of the global upper bound
  * may not be cheap. The standard approach is then to only compute it if the
  * method is called, but on the other hand to "cache" the value to answer
  * quickly if no change has happened that changed it. Thus, the method has
  * to be able to write into the fields of the class. */

 virtual FunctionValue get_global_upper_bound( void ) {
  return( Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (global) Lipschitz constant for the Function
 /** Method that returns a (global) Lipschitz constant L for this Function,
  * i.e., a (real) scalar L such that
  *
  *   | f( x ) - f( y ) | <= L * | x - y |
  *
  * for all x and y in the domain of the Function. By default, the method
  * returns Inf< FunctionValue >(), which means that the Function is *not*
  * Lipschitz continuous. Note that a finite Lipschitz constant implies that
  * is_continuous() must return true.
  *
  * This method is not const because the computation of the Lipschitz constant
  * may not be cheap. The standard approach is then to only compute it if the
  * method is called, but on the other hand to "cache" the value to answer
  * quickly if no change has happened that changed it. Thus, the method has
  * to be able to write into the fields of the class. */

 virtual FunctionValue get_Lipschitz_constant( void ) {
  return( Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// returns true only if this Function is convex
 /** Method that returns true only if this function is convex. The default is
  * false (convexity being good for optimization, in particular minimization,
  * often too good to be true). */

 [[nodiscard]] virtual bool is_convex( void ) const { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this Function is concave
 /** Method that returns true only if this function is concave. The default is
  * false (concavity being good for optimization, in particular maximization,
  * often too good to be true). */

 [[nodiscard]] virtual bool is_concave( void ) const { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this Function is linear
 /** Method that returns true only if this Function is linear. The base
  * implementation of the class exploits the fact that the only class of
  * functions that are both convex and concave is precisely that of linear
  * functions (and therefore this method is not very useful ... ) */

 [[nodiscard]] virtual bool is_linear( void ) const {
  return( this->is_convex() && this->is_concave() );
  }

/*--------------------------------------------------------------------------*/
 /// returns true only if this Function is lower semi-continuous
 /** Method that returns true only if this function is lower
  * semi-continuous. The default is true (continuity being an important
  * property for optimization). */

 [[nodiscard]] virtual bool is_lower_semicontinuous( void ) const {
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this Function is upper semi-continuous
 /** Method that returns true only if this function is upper
  * semi-continuous. The default is true (continuity being an important
  * property for optimization). */

 [[nodiscard]] virtual bool is_upper_semicontinuous( void ) const {
  return( true );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this Function is continuous
 /** Method that returns true only if this function is continuous. The base
  * implementation of the class exploits the fact that by this means that it
  * is both upper semi-continuous and lower semi-continuous (and therefore
  * this method is not very useful ... ) */

 [[nodiscard]] bool is_continuous( void ) const {
  return( this->is_lower_semicontinuous() &&
	  this->is_upper_semicontinuous() );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Function
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( idx_type( intLastParFun ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( idx_type( dblLastParFun ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  switch( par ) {
   case( intMaxIter ):   return( Inf< int >() );
   case( intMaxThread ): return( 0 );
   case( intEverykIt ):  return( 0 );
   }

  throw( std::invalid_argument( "invalid int parameter name" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  switch( par ) {
   case( dblMaxTime ):  return( Inf< double >() );
   case( dblEveryTTm ): return( 0 );
   case( dblRelAcc ):   return( 1e-6 );
   case( dblAbsAcc ):
   case( dblUpCutOff ): return( Inf< double >() );
   case( dblLwCutOff ): return( - Inf< double >() );
   }

  throw( std::invalid_argument( "invalid double parameter name" ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  if( name == "intMaxIter" )
   return( intMaxIter );
  if( name == "intMaxThread" )
   return( intMaxThread );
  if( name == "intEverykIt" )
   return( intEverykIt );

  return( Inf< idx_type >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  if( name == "dblMaxTime" )
   return( dblMaxTime );
  if( name == "dblEveryTTm" )
   return( dblEveryTTm );
  if( name == "dblRelAcc" )
   return( dblRelAcc );
  if( name == "dblAbsAcc" )
   return( dblAbsAcc );
  if( name == "dblUpCutOff" )
   return( dblUpCutOff );
  if( name == "dblLwCutOff" )
   return( dblLwCutOff );

  return( Inf< idx_type >() );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars = { "intMaxIter" ,
						   "intMaxThread" ,
						   "intEverykIt" };
  if( ( idx >= intMaxIter ) && ( idx <= intEverykIt ) )
   return( pars[ idx ] );

  throw( std::invalid_argument( "invalid int parameter name" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars = { "dblMaxTime" ,
  "dblEveryTTm" , "dblRelAcc" , "dblAbsAcc" , "dblUpCutOff" , "dblLwCutOff" };

  if( ( idx >= dblMaxTime ) && ( idx <= dblLwCutOff ) )
   return( pars[ idx ] );

  throw( std::invalid_argument( "invalid double parameter name" ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR READING THE DATA OF THE Function ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the Function
 *  @{ */

 /// returns the pointer to the Block to which the Function belongs
 [[nodiscard]] Block * get_Block( void ) const override;

/*--------------------------------------------------------------------------*/
 /// returns the pointer to the Observer of this Function

 [[nodiscard]] Observer * get_Observer( void ) const { return( f_Observer ); }

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS FOR LOADING, PRINTING & SAVING THE Function ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the Function
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the operator<<()
  * is defined for each Function, but its behavior can be customized by
  * derived classes. */

 friend std::ostream & operator<<( std::ostream & out , const Function & o ) {
  o.print( out );
  return( out );
  }

/** @} ---------------------------------------------------------------------*/
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

 virtual void print( std::ostream & output ) const {
  output << "Function [" << this << "]" << " with " << get_num_active_var()
         << " active variables";
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Observer * f_Observer = nullptr;   ///< the Observer of this Function

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
 * that are covered by this Modification, which are encoded by the return
 * value of shift() in the following way:
 *
 * - NaN, e.g. as what is reported by
 *   std::numeric_limits::quiet_NaN< FunctionValue >() or by
 *   std::numeric_limits::signaling_NaN< FunctionValue >()): the value of the
 *   Function has changed "unpredictably" all over the space, any previously
 *   computed value is no longer reliable. Although the convenient constexpr
 *   "NaNshift" is defined in the class, note that testing if shift() is NaN
 *   must *not* be done with "shift() == NaNshift", but rather with
 *   std::isnan( shift() ).
 *
 * - Any finite non-NaN number: conversely, the value of the Function has
 *   changed in a very predictable way: computing the value of the Function at
 *   any point now returns f_v + shift(), where f_v is the value that would
 *   have been returned prior to the Modification. One clean way to see this
 *   change is that "the constant term of the Function has changed", see
 *   get_constant_term(). It should be noted that shift() = 0 does not really
 *   have a sense in this case as it would not be a change in the Function;
 *   however, the value is still allowed for uniformity with FunctionModVars
 *   [see].
 *
 * - +Infty (= Inf< FunctionValue >(), for which the convenience constexpr
 *   "INFshift" is defined): this means that the value of the Function has
 *   changed "unpredictably but monotonically upwards": computing the value of
 *   the Function at any point now returns a value that is surely greater than
 *   or equal to the value that would have been returned prior to the
 *   Modification.
 *
 * - - Infty (= - Inf< FunctionValue >(), i.e., "- INFshift" exploiting the
 *   defined convenience constexpr): this means that the value of the Function
 *   has changed "unpredictably but monotonically downwards": computing the
 *   value of the Function at any point now returns a value that is surely
 *   smaller than or equal to the value that would have been returned prior to
 *   the Modification.
 *
 * Note that a FunctionMod with std::isnan( shift() ) == true is the "nuclear
 * Modification for Function": it basically says that everything that was
 * previously known before about the function is no longer reliable, *except
 * for the set of "active" Variable* that has remained the same. Indeed, 
 * changes in the set of "active" Variable have their own separate 
 * Modification. */

class FunctionMod : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
/* Note: several of these types are not directly used by FunctionModVars, but
 * they may be useful to derived classes, so we do the "importing" once and
 * for all here. */

 /// "import" Index (equivalent to Block::Index) from Function
 using Index = Function::Index;

 using c_Index = const Index;   ///< a const Index

 /// "import" Range (equivalent to Block::Range) from Function
 using Range = Function::Range;

 using c_Range = const Range;   ///< a const Range

 /// "import" Subset (equivalent to Block::Subset) from Function
 using Subset = Function::Subset;

 using c_Subset = const Subset;  ///< a const Subset

 /// "import" FunctionValue from Function
 using FunctionValue = Function::FunctionValue;

 using c_FunctionValue = const FunctionValue;  ///< a const FunctionValue

 /// "import" Vec_FunctionValue from Function
 using Vec_FunctionValue = Function::Vec_FunctionValue;

 using c_Vec_FunctionValue = const Vec_FunctionValue;
 ///< a const Vec_FunctionValue

/*----------------------------- CONSTANTS ----------------------------------*/

 static constexpr FunctionValue NaNshift
                          = std::numeric_limits< FunctionValue >::quiet_NaN();
 ///< convenience constexpr for "NaN", *not* to be used with ==

 static constexpr FunctionValue INFshift = Inf< FunctionValue >();
 ///< convenience constexpr for "Infty"

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 explicit FunctionMod( Function * f , FunctionValue shift = NaNshift ,
                       bool cB = true )
  : AModification( cB ) , f_function( f ) , f_shift( shift ) {}

 ///< constructor: takes a Function pointer and a shift
 /**< Constructor: takes a pointer to the affected Function and the value of
  * the shift. If shift is finite, then it indicates that the Function value
  * has been shifted by a constant. This means that if the Function value at
  * a given point was f_v before this Modification, then the correct value is
  * now f_v + shift. If shift is Inf< FunctionValue >(), then it means that the
  * Function has been modified in an unpredictable way, so that any prior
  * information regarding the Function can be disregarded. */

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FunctionMod() override = default;  ///< destructor

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// returns the Block to which the Observer of the Function belongs

 [[nodiscard]] Block * get_Block( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the pointer to) the affected Constraint

 [[nodiscard]] Function * function( void ) const { return( f_function ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the type of Modification
 /** This value encodes for four types of changes, depending on its value:
 *
 * - NaN: the value of the Function has changed "unpredictably" all over the
 *   space, any previously computed value is no longer reliable. Although the
 *   convenient constexpr "NaNshift" is defined in the class, note that
 *   testing if f_shift is NaN must *not* be done with "shift() == NaNshift",
 *   but rather with std::isnan( shift() ).
 *
 * - Any finite non-NaN number: conversely, the value of the Function has
 *   changed in a very predictable way: computing the value of the Function at
 *   any point now returns f_v + shift(), where f_v is the value that would
 *   have been returned prior to the Modification. It should be noted that
 *   shift() = 0 does not really have a sense in this case as it would not
 *   be a change in the Function; however, the value is still allowed for
 *   uniformity with FunctionModVars.
 *
 * - +INFshift: this means that the value of the Function has changed
 *   "unpredictably but monotonically upwards": computing the value of the
 *   Function at any point now returns a value that is surely greater than or
 *   equal to the value that would have been returned prior to the
 *   Modification.
 *
 * - - INFshif: this means that the value of the Function has changed
 *   "unpredictably but monotonically downwards": computing the value of the
 *   Function at any point now returns a value that is surely smaller than or
 *   equal to the value that would have been returned prior to the
 *   Modification. */

 [[nodiscard]] FunctionValue shift( void ) const { return( f_shift ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the FunctionMod

 void print( std::ostream & output ) const override {
  output << "FunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function " << f_function << " ]: f-values changed";
  if( std::isnan( f_shift ) )
   output << "(+-)";
  else if( f_shift >= INFshift )
   output << "(+)";
  else if( f_shift <= - INFshift )
   output << "(-)";
  else
   output << " by " << f_shift;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Function * f_function;   ///< pointer to the modified Function

 FunctionValue f_shift;  ///< how the value of the function has changed

/*--------------------------------------------------------------------------*/

};  // end( class( FunctionMod ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FunctionModVars -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe adding/removing Variable of a Function
/** Derived class from AModification to describe changes of a Function that
 * involve adding/removing the Variable "active" in it. The class holds the
 * subset of affected Variable under the form of a std::vector< Variable * >,
 * returned by vars(), containing their "name == pointer". This is already
 * enough information to completely identify the affected Variable. However,
 * derived classes may add some operation-specific information that make the
 * task easier and/or more efficient under specific scenarios.
 *
 * Note that even in case all the Variable are reset, vars() cannot be
 * empty(). The reason is that Function do not register/unregister itself in
 * its "active" Variable. This has to be done by something else; but on the
 * other hand, this something else typically relies on Function to keep the
 * list. If all Variable are reset the list is gone, and therefore the
 * something else cannot do the un-registering.
 *
 * In this setting, the actual set of pointers is clearly not required, and it
 * is likely that the operation can be performed more efficiently by knowing
 * that the Variable to be eliminated are just "everyone".
 *
 * The FunctionModVars also tells whether the Function is "quasi-additive"
 * in the added/removed Variable, which is an important property to allow
 * re-using previously computed function values. 
 *
 * The quasi-additivity property is encoded into a single extended-real value
 * (possibly NaN), returned by the method shift(). Note that this is similar,
 * but not the same, to the shift() of FunctionMod (indeed, FunctionModVars
 * does *not* derive from FunctionMod). The definition is as follows.
 *
 * Suppose that new Variables are added to the Function. Let f_old( x ) be
 * the Function, and x its vector of "active" Variable, before the
 * modification. Let y be the vector of Variable that were added, and
 * f( x , y ) be the Function after the modification. We say that the
 * Variable y are quasi-additively added to the function if and only if
 *
 *    f( x , 0 ) = f_old( x ) + shift()   for all x
 *
 * In plain words, the new value of the Function just a constant shift to the
 * old value of the Function (identical if shift() == 0) whenever all the new
 * Variable are fixed to their default value (0).
 *
 * Similarly, suppose that Variables y are removed from the Function. Let
 * f_old( x , y ) be the Function before the modification, and f( x ) be the
 * Function after the modification. We say that the variables y are
 * quasi-additively removed from the Function if and only if
 *
 *     f( x ) = f_old( x , 0 ) + shift()    for all x
 *
 * Again, the new value of the Function just a constant shift to the old value
 * of the Function (identical if shift() == 0) whenever the latter were
 * computed at points where all the removed Variable are fixed to their
 * default value (0).
 *
 * Note that the property depends on the specific Variable being
 * added/removed and what how exactly "adding" and "removing" means. For
 *
 *     f_old( x , y , z ) = x y + z
 *
 * removing z leads to f( x , y ) = x y, and this clearly is quasi-additive,
 * while removing y may lead to f( x , z ) = x + z which is not. Yet,
 * removing (all algebraic terms containing) y could also lead to
 * f( x , z ) = z, which is a quasi-additive removal. Of course, an additive
 * function f( x , y ) = f_1( x ) + f_2( y ) is quasi additive for whatever
 * reasonable concept of addition (of other additive terms) and removal one
 * can think of, but the concept also covers non-additive functions:
 *
 * f( x , y ) = f_1( x ) + f_2( y ), can be dealt with:
 *
 *     f_old( x ) = e^x     and      f( x , y ) = e^( x + y )
 *
 * is quasi-additive, provided of course that the removal of y from f()
 * brings back to f_old() rather than deleting all the terms containing it.
 *
 * Also, note that for the shift() to be finite, *all* the involved Variable
 * must be quasi-additive. Yet, if some of them were, and some of them were
 * not, there would be little solace in having two different Modification,
 * one with finite shift() and one with infinite one, since at the end of
 * the day the result would still be that the new value of the Function is
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
 * The value returned by shift() signals whether or not the just occurred
 * Modification was a quasi-additive one, with the following encoding:
 *
 * - NaN, e.g. what is reported by
 *   std::numeric_limits< FunctionValue >::quiet_NaN() or by
 *   std::numeric_limits::signaling_NaN< FunctionValue >(): the Modification
 *   was not quasi-additive one, and the value of the Function has changed
 *   "unpredictably" all over the space. Although the convenient constexpr
 *   "NaNshift" is defined in the class, note that testing if shift() is
 *   NaN must *not* be done with "shift() == NaNshift", but rather with
 *   std::isnan( f_shift() ).
 *
 * - Any finite non-NaN number (comprised 0, which makes full sense): the
 *   Modification was a quasi-additive one, with shift() being the value of
 *   the shift.
 *
 * - +Infty (= Inf< FunctionValue >(), for which the convenience constexpr
 *   "INFshift" is defined): the Modification was not a quasi-additive one,
 *   but while the value of the Function has changed "unpredictably" all over
 *   the space, the change is "upward monotone" in the sense that:
 *   = for addition, the value of the Function at any point ( x , 0 ) is
 *     surely greater than or equal to the value that the Function had at x;
 *   = for deletion, the value of the Function at any point x is surely
 *     greater than or equal to the value that the Function had at ( x , 0 ).
 *
 * - - Infty (= - Inf< FunctionValue >(), i.e., "- INFshift" exploiting the
 * defined
 *   convenience constexpr): the Modification was not a quasi-additive one,
 *   but while the value of the Function has changed "unpredictably" all over
 *   the space, the change is "downward monotone" in the sense that:
 *   = for addition, the value of the Function at any point ( x , 0 ) is
 *     surely less than or equal to the value that the Function had at x;
 *   = for deletion, the value of the Function at any point x is surely
 *     less than or equal to the value that the Function had at ( x , 0 ).
 *
 * A FunctionModVars may represent either an addition or a deletion of
 * Variable, and the method added() allows to discriminate between the two.
 * However, the base FunctionModVars class does *not* allow to set the value
 * returned by the method, which is pure virtual. The method is implemented
 * by derived classes which correspond to either additions or deletions,
 * each of which has specific information attached. However, some Solver may
 * not have use for the extra information that the derived classes carry, and
 * may prefer to only "catch" the base FunctionModVars and use the added()
 * method to understand what has happened, rather than going through the
 * problem of "catching" each of the derived classes individually. */

class FunctionModVars : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/
/* Note: several of these types are not directly used by FunctionModVars, but
 * they may be useful to derived classes, so we do the "importing" once and
 * for all here. */

 /// "import" Index (equivalent to Block::Index) from Function
 using Index = Function::Index;

 using c_Index = const Index;    ///< a const Index

 /// "import" Range (equivalent to Block::Range) from Function
 using Range = Function::Range;

 using c_Range = const Range;    ///< a const Range

 /// "import" Subset (equivalent to Block::Subset) from Function
 using Subset = Function::Subset;

 using c_Subset = const Subset;  ///< a const Subset

 /// "import" FunctionValue from Function
 using FunctionValue = Function::FunctionValue;

 using c_FunctionValue = const FunctionValue;  ///< a const FunctionValue

 /// "import" Vec_FunctionValue from Function
 using Vec_FunctionValue = Function::Vec_FunctionValue;

 using c_Vec_FunctionValue = const Vec_FunctionValue;
 ///< a const Vec_FunctionValue

/*----------------------------- CONSTANTS ----------------------------------*/

 static constexpr FunctionValue NaNshift =
                            std::numeric_limits< FunctionValue >::quiet_NaN();
 ///< convenience constexpr for "NaN", *not* to be used with ==

 static constexpr FunctionValue INFshift = Inf< FunctionValue >();
 ///< convenience constexpr for "Infty"

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** constructor: takes a pointer to the affected Function, the subset of
  * affected Variable under the form of a std::vector< Variable * >, the
  * value of the shift encoding the quasi-additivity status of the
  * modification, and the "concerns" value. As the && tells, the vector
  * "becomes property" of the FunctionModVars object. Note that the order
  * of the vars[] vector is irrelevant for deletions (as the final set of
  * indices is the same in whatever order they are performed), while it is
  * *not* irrelevant for additions. In this case, the order of vars[] is
  * supposed to be that of addition: vars[ 0 ] is the first new Variable
  * added, vars[ 1 ] the second, ... (this is logically speaking, and
  * regardless to the fact that a :Function may well implement a "add a
  * bunch of Variable in one blow" operation; still, any such operation must
  * define what the addition order conceptually is). */

 FunctionModVars( Function * f , Vec_p_Var && vars ,
                  FunctionValue shift = NaNshift , bool cB = true )
  : AModification( cB ) , f_function( f ) , f_shift( shift ) ,
    v_vars( std::move( vars ) ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FunctionModVars() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// returns the Block to which the Observer of the Function belongs

 [[nodiscard]] Block * get_Block( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the pointer to) the affected Function

 [[nodiscard]] Function * function( void ) const { return( f_function ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tells if the Modification was quasi-additive
 /**< This value of signals whether or not the just occurred Modification
  * was a quasi-additive one, with the following encoding:
  *
  * - NaN: the Modification was not quasi-additive one, and the value of the
  *   Function has changed "unpredictably" all over the space. Although the
  *   convenient constexpr "NaNshift" is defined in the class, note that
  *   testing if f_shift is NaN must *not* be done with "shift() == NaNshift",
  *   but rather with std::isnan( shift() ).
  *
  * - Any finite non-NaN number (comprised 0, which makes full sense): the
  *   Modification was a quasi-additive one, with shift() being the value of
  *   the shift.
  *
  * - INFshift: the Modification was not a quasi-additive one, but while the
  *   value of the Function has changed "unpredictably" all over the space,
  *   the change is "upward monotone".
  *
  * - - INFshift: the Modification was not a quasi-additive one, but while the
  *   value of the Function has changed "unpredictably" all over the space,
  *   the change is "downward monotone". */

 [[nodiscard]] FunctionValue shift( void ) const { return( f_shift ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the vector of pointers to affected Variable

 [[nodiscard]] c_Vec_p_Var & vars( void ) const { return( v_vars ); }

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method telling if the Variables have been added or removed
 /** This method has to return true if the Variables have been added, and
  * false if they have been removed. The method is pure virtual, and it is
  * properly implemented in derived classes. However, some Solver may not
  * have use for the extra information that the derived classes carry, and
  * may prefer to only "catch" the base FunctionModVars and use this method
  * to understand what has happened, rather than going through the problem of
  * "catching" each of the derived classes individually. */

 [[nodiscard]] virtual bool added( void ) const = 0;

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the FunctionModVars

 void print( std::ostream & output ) const override {
  output << "FunctionModVars[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function << " ]: ";
  if( std::isnan( f_shift ) )
   output << "non quasi-additively (+-)";
  else if( f_shift >= INFshift )
   output << "non quasi-additively (+)";
  else if( f_shift <= - INFshift )
   output << "non quasi-additively (-)";
  else
   output << "quasi-additively (" << f_shift << ") ";

  if( added() )
   output << "adding ";
  else
   output << "deleting ";

  output << v_vars.size() << " variables" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Function * f_function;   ///< pointer to the modified Function

 FunctionValue f_shift;  ///< how the value of the function has changed

 Vec_p_Var v_vars;       ///< vector of pointers to affected Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( FunctionModVars ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS FunctionModVarsAddd -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe adding a set of Variable from a Function
/** Derived class from FunctionModVars to describe a specific change of a
 * Function: adding a set of Variable. It only extends FunctionModVars by
 * specifying the indices that the Variable took, which is simply done by
 * returning which was that the index that the *first* of them took
 * (because when adding Variable, all the new ones get consecutive indices),
 * which is also equal to the number of "active" Variable *before* the
 * modification happened. 
 *
 * The important remark that this refers to the index that the Variable
 * (whose pointer is anyway returned by vars() by the base class)
 *
 *     HAD AT THE MOMENT IN WHICH THE FunctionModVarsAddd WAS ISSUED,
 *     SINCE WHEN THE FunctionModVarsAddd IS PROCESSED, THESE Variable MAY
 *     HAVE CHANGED INDEX (IF Variable WITH SMALLER INDEX HAVE BEEN
 *     REMOVED), OR COULD HAVE EVEN BEEN REMOVED (IN WHCH CASE AN
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Variable in the
 * Function, as it then would immediately know where to put the information
 * about the newly added Variable. Note that
 *
 *     THE INDEX THAT THE ADDED Variable HAVE AT THE MOMENT IN WHICH THE
 *     FunctionModVarsAddd IS PROCESSED CAN ONLY BE SMALLER THAN OR EQUAL
 *     TO THAT THAT THE INFORMATION REPORTED HERE IMPLIES, EXCEPT IF A
 *     Variable HAS BEEN DELETED AND RE-ADDED (IN WHCH CASE TWO
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * This may allow to simplify somewhat the work for some Solver. */

class FunctionModVarsAddd : public FunctionModVars {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * FunctionModVars, it takes the index that the first of the added Variable
  * took. Note that this hinges on vars[] to be "ordered as the Variable
  * have been added". That is, vars[ 0 ] is the first Variable having been
  * added, and therefore it took index first. vars[ 1 ] is the second
  * Variable having been added, and therefore it took index first + 1 ...
  * Of course, a :Function is likely to implement an "add a bunch of Variable
  * in a single blow" operation, which is why this Modification allow to
  * deal with many new Variable at a time. Still, any such operation must
  * define what the addition order conceptually is, and this must be
  * reflected in the order of vars[]. */

 FunctionModVarsAddd( Function * f , Vec_p_Var && vars , Index first ,
                      FunctionValue shift = NaNshift , bool cB = true )
  : FunctionModVars( f , std::move( vars ) , shift , cB ) , f_first( first )
 {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FunctionModVarsAddd() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the index obtained by the first added Variable

 [[nodiscard]] Index first( void ) const { return( f_first ); }

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method telling that the Variables have been added

 [[nodiscard]] bool added( void ) const override { return( true ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the FunctionModVarsAddd

 void print( std::ostream & output ) const override {
  output << "FunctionModVarsAddd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function << " ]: ";
  if( std::isnan( f_shift ) )
   output << "non quasi-additively (+-)";
  else if( f_shift >= INFshift )
   output << "non quasi-additively (+)";
  else if( f_shift <= - INFshift )
   output << "non quasi-additively (-)";
  else
   output << "quasi-additively (" << f_shift << ") ";

  output << "adding variable";
  if( v_vars.size() > 1 )
   output << "s [ " << f_first << " , "
          << f_first + v_vars.size() << " )";
  else
   output << " " << f_first;
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Index f_first;   ///< the index obtained by the first added Variable

/*--------------------------------------------------------------------------*/

};  // end( class( FunctionModVarsAddd ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS FunctionModVarsRngd -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe removing a range of Variable from a Function
/** Derived class from FunctionModVars to describe a specific change of a
 * Function: removing a range of Variable. It only extends FunctionModVars
 * by specifying the range.
 *
 * The important remark that the range refers to the index that the Variable
 * (whose pointer is anyway returned by vars() by the base class)
 *
 *     HAD AT THE MOMENT IN WHICH THE FunctionModVarsRngd WAS ISSUED,
 *     SINCE WHEN THE FunctionModVarsRngd IS PROCESSED, THESE Variable ARE
 *     NO LONGER "ACTIVE" IN THE Function UNLESS THEY HAVE LATER BEEN
 *     RE-ADDED, IN WHCH CASE AN APPROPRIATE Modification MUST BE SITTING
 *     IN THE QUEUE AFTER THIS ONE.
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Variable in the
 * Function, as knowing that the removed Variable are precisely those that
 * were in the range just prior than the FunctionModVarsRngd was processed
 * (Modification are thought to be processed in FIFO order) allows to
 * immediately access the range in the internal data structure without
 * having to individually look for each of the Variable from vars(), and
 * maybe having to figure out a-posteriori that these were actually a range.
 */

class FunctionModVarsRngd : public FunctionModVars
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * FunctionModVars, it takes the range identifying the indices of the removed
  * Variable at the moment in which the FunctionModVarsRngd was issued. The
  * range is a pair of indices ( start , stop ) representing the typical
  * left-closed, right-open range { i : start <= i < stop }, and the
  * correspondence between that and vars is positional: vars[ 0 ] had index
  * range, vars[ 1 ] had index range + 1 ..., which implies that
  * vars.size() == stop - start. */

 FunctionModVarsRngd( Function * f , Vec_p_Var && vars , c_Range & range ,
                      FunctionValue shift = NaNshift , bool cB = true )
  : FunctionModVars( f , std::move( vars ) , shift , cB ) , f_range( range )
 {
  if( v_vars.size() != f_range.second - f_range.first )
   throw( std::invalid_argument( "vars and range sizes do not match" ) );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FunctionModVarsRngd() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the range of the deleted Variable

 [[nodiscard]] c_Range & range( void ) const { return( f_range ); }

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method telling that the Variables have been removed

 [[nodiscard]] bool added( void ) const override { return( false ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the FunctionModVarsRngd

 void print( std::ostream & output ) const override {
  output << "FunctionModVarsRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function << " ]: ";
  if( std::isnan( f_shift ) )
   output << "non quasi-additively (+-)";
  else if( f_shift >= INFshift )
   output << "non quasi-additively (+)";
  else if( f_shift <= - INFshift )
   output << "non quasi-additively (-)";
  else
   output << "quasi-additively (" << f_shift << ") ";

  output << "deleting variables [ " << f_range.first << " , "
         << f_range.second << " ]" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Range f_range;   ///< the range of the removed Variable

/*--------------------------------------------------------------------------*/

};  // end( class( FunctionModVarsRngd ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS FunctionModVarsSbst -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe removing a subset of Variable from a Function
/** Derived class from FunctionModVars to describe a specific change of a
 * Function: removing a arbitrary subset of the Variable. It only extends
 * FunctionModVars by specifying the subset.
 *
 * The important remark that the subset of indices specified are those that
 *
 *     THE DELETED Variable HAD AT THE MOMENT IN WHICH THE
 *     FunctionModVarsSbst WAS ISSUED, SINCE WHEN THE FunctionModVarsRngd IS
 *     PROCESSED, THESE Variable ARE NO LONGER "ACTIVE" IN THE Function
 *     UNLESS THEY HAVE LATER BEEN RE-ADDED, IN WHCH CASE AN APPROPRIATE
 *     Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE.
 *
  * The "name == pointer" of the Variable, which is anyway returned by vars()
 * by the base class, should in principle be sufficient alone to reconstruct
 * what the index was. However, this may require the Solver to keep some
 * internal data structure to map the Variable pointer to its index in the
 * function. By knowing that the removed Variable are precisely those that
 * were in the subset just prior than the FunctionModVarsSbst was processed
 * (Modification are thought to be processed in FIFO order) may allow to
 * immediately access the subset in some internal data structure without
 * having to individually look for each of the Variable from vars(). */

class FunctionModVarsSbst : public FunctionModVars {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: takes all the necessary information
 /** Constructor: besides all the information required by the base
  * FunctionModVars, it takes the subset identifying the indices of the
  * removed Variable. The indices of have the obvious positional
  * correspondence: subset[ i ] is the index that the Variable vars[ i ] had
  * *at the moment in which the FunctionModVarsRngd was issued*. As the the
  * && tells, the vector "becomes property" of the FunctionModVarsSbst
  * object. The ordered parameter tells if subset is ordered by increasing
  * Index, which may be helpful for some Block/Solver having to deal with
  * this FunctionModVarsSbst; indeed, if subset is not "naturally" ordered,
  * it is ordered in the constructor. Of course, this means that vars gets
  * re-ordered at the same time. */

 FunctionModVarsSbst( Function * f, Vec_p_Var && vars , Subset && subset ,
                      bool ordered = false , FunctionValue shift = NaNshift ,
                      bool cB = true )
  : FunctionModVars( f , std::move( vars ) , shift , cB ),
    v_subset( std::move( subset ) ) {
  if( ( ! v_subset.empty() ) && ( v_vars.size() != v_subset.size() ) )
   throw( ( std::invalid_argument( "vars and subset sizes do not match" ) ) );
  if( ( ! ordered ) && ( ! v_subset.empty() ) && ( v_vars.size() > 1 ) ) {
   using IdxVar = std::pair< Index,  Variable * >;
   std::vector< IdxVar > tmp( v_vars.size() );
   for( Index i = 0 ; i < v_vars.size() ; ++i )
    tmp[ i ] = IdxVar( v_subset[ i ] , v_vars[ i ] );
   std::sort( tmp.begin() , tmp.end() ,
              []( auto & a , auto & b ) {
               return( ( a.first < b.first ) );
              } );
   for( Index i = 0 ; i < v_vars.size() ; ++i ) {
    v_subset[ i ] = tmp[ i ].first;
    v_vars[ i ] = tmp[ i ].second;
   }
  }
 #ifndef NDEBUG
  for( Index i = 1 ; i < v_subset.size() ; ++i )
   if( v_subset[ i - 1 ] >= v_subset[ i ] )
    throw( std::invalid_argument( "unordered or repeated index in subset" ) );
 #endif
 }

/*------------------------------ DESTRUCTOR --------------------------------*/

 ~FunctionModVarsSbst() override = default;  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the subset of the deleted Variable

 [[nodiscard]] c_Subset & subset( void ) const { return( v_subset ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method telling that the Variables have been removed

 [[nodiscard]] bool added( void ) const override { return( false ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the FunctionModVarsSbst

 void print( std::ostream & output ) const override {
  output << "FunctionModVarsSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Function[" << &f_function << " ]: ";
  if( std::isnan( f_shift ) )
   output << "non quasi-additively (+-)";
  else if( f_shift >= INFshift )
   output << "non quasi-additively (+)";
  else if( f_shift <= - INFshift )
   output << "non quasi-additively (-)";
  else
   output << "quasi-additively (" << f_shift << ") ";
  output << "deleting " << v_subset.size() << " variables" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Subset v_subset;   ///< the subset of the removed Variable

/*--------------------------------------------------------------------------*/

};  // end( class( FunctionModVarsSbst ) )

/** @} end( group( Function_CLASSES ) ) ------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Function.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Function.h ---------------------------*/
/*--------------------------------------------------------------------------*/

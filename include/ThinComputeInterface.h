/*--------------------------------------------------------------------------*/
/*-------------------- File ThinComputeInterface.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* ThinComputeInterface class, which factors
 * out the basic interface between all objects in SMS++ that can (but do not
 * necessarily) have a significant computational component. It also defines
 * a ComputeConfig object that allows to set and get all the parameters of a
 * :ThinComputeInterface object in one blow.
 *
 * \version 0.10
 *
 * \date 03 - 09 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ThinComputeInterface
 #define __ThinComputeInterface
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Configuration.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class ComputeConfig;  // forward definition of ComputeConfig
 
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ThinComputeInterface_CLASSES Classes in ThinComputeInterface.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS ThinComputeInterface --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// abstract base class for any SMS++ object with a computational part
/** Several objects in SMS++ can (but do not necessarily) have a significant
 * computational component, i.e., some operations are computationally
 * demanding and therefore require being dealt with care:
 *
 * - the operations have parameters (possibly, many and complex) determining
 *   how exactly they are performed;
 *
 * - the operations may not necessarily suceed, especially if only allowed a
 *   limited the amount of computational resources (which may be done by
 *   setting some of the above parameters);
 *
 * - the computation may depend on the value of some set of Variable, and a
 *   (simple) mechanism is provided for knowing whether the Variable have or
 *   not changed their value since the last call without having to check this
 *   explicitly;
 *
 * - the operations may be performed asyncronously on a different thread
 *   (although this aspect is not treated yet, and left for a future revision
 *   of the class).
 *
 * Objects with this behaviour are (obviously) Solver [see Solver.h], but also
 * possibly Function [see Function.h], Constraint [see Constraint.h] and
 * Objective [see Objective.h]. The link between these is obvious enough:
 * Constraint and Objective may well be implemented in terms of Function, and
 * evaluating a Function may well be a computationally demanding task, up to
 * requiring a Solver. Yet, not all Function (and hence, Constraint and
 * Objective) are computationally heavy enough to warrant such a complex
 * interface, and although likely most Solver are, there might be Solver
 * that have low enough complexity to allow foregoing it.
 *
 * The *abstract* ThinComputeInterface class is meant to factor out many of
 * the methods required to deal with these aspects. Factoring them is
 * primarily meant to avoid un-necessary code duplication and to ensure
 * consistency between similar parts of the interface of different objects,
 * although there is (currently) no planned direct use of the fact that the
 * Solver, Function, Constraint and Objective classes then have a common
 * ancestor.
 *
 * The interface is "thin" in the sense that *almost* all the methods are
 * either pure virtual, or are given a thin default implementation suiting
 * the case where the computational cost of the object is low. The idea is
 * that the actual implementation of the methods is deferred until towards
 * the end of the derivatiobn chain, so that classes that do not have a
 * computational cost warranting the complex interface (say, LinearFunction)
 * do not have to pay any implementation cost for features that are not
 * justified. The exception to this rule is the definition of the
 * ComputeConfig class and the methods for reading and writing a
 * ComputeConfig; these can be implemented using the abstract part of
 * the interface, and therefore potentially need to be implemented only once.
 *
 * A reason for introducing the class at this point of the design cycle is
 * that SMS++ will eventually have to cater for asyncronous calls, which is
 * going to be expecially important for the computationally-heavy objects
 * that ThinComputeInterface targets. However, how this is to be accomplished
 * is not decided yet. By factoring all aspects of the computational interface
 * into a single class, the effort to later refactor the interfaces to allow
 * for asyncronous calls should be decreased. */

class ThinComputeInterface {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 typedef unsigned short int idx_type;  ///< the type of parameters indices
 
/*--------------------------------------------------------------------------*/

 /// public enum for the possible return values of compute()

 enum compute_type {
 kUnEval = 0 ,   ///< compute() has not been called yet

 kOK = 7 ,       ///< successful compute()
                 /**< Any return value between kUnEval (excluded) and kOK
		  * (included) means that the object ran smoothly, obtaining
 * the desired answer within the allowed limits on the available computational
 * resources (if any). The fact that multiple values are allowed corresponds
 * to the fact that "the desired answer" may in fact be of different types,
 * such as that some optimization problem has been conclusively solved to
 * optimality, or conclusively shown to be empty, or conclusively shown to be
 * unbounded. */

 kError = 15 ,   ///< compute() stopped because of unrecoverable error
                 /**< Any return value >= kError means that the object was
		  * forced to stop due to some error, e.g. of numerical nature
 * or because of lack of some crucial resource (say, memory). The error is of
 * the irrecoverable type, i.e., the computation is assumed to have not been
 * able to obtain all of the desired answer (although it may have obtained a
 * part of it, say proving that an optimization problem is not unfeasible by
 * producing at least a feasible solution, but not being able to certify an
 * optimal solution). Specific values > kError can be used to specify more
 * information about the specific kind of error that the object is
 * experiencing.
 *
 * Note that this leaves out all return values comprised between kOK and
 * kError, extremes excluded. These are left for "recoverable error states"
 * where the object were not able to conclusively obtain all of the desired
 * answer (although it may have already obtained a part of it), but this was
 * due to some reason that forced it to stop early on, such as a limit imposed
 * on the available computational resources. By relaxing the limit, which may
 * be as simple as calling compute() again, the object may further proceed in
 * the computation process, possibly providing a kOK-type answer. */

 };  // end( compute_type )

/*@} -----------------------------------------------------------------------*/
/*----------- CONSTRUCTING AND DESTRUCTING ThinComputeInterface ------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Block
 *  @{ */

 /// constructor: does nothing, the class is thin
 ThinComputeInterface( void ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual and does nothing, the class is thin

 virtual ~ThinComputeInterface() { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. The method is given a 
  * void implementation doing nothing (i.e., ignoring the set value), rather
  * than being pure virtual, so that derived classes not having any working
  * int parameter (i.e., either not having any or not really reacting to the
  * ones that they supposedly have) do not have to bother with implementing
  * it. */

 virtual void set_par( const idx_type par , const int value ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. The method is given a 
  * void implementation doing nothing (i.e., ignoring the set value), rather
  * than being pure virtual, so that derived classes not having any working
  * double parameter (i.e., either not having any or not really reacting to
  * the ones that they supposedly have) do not have to bother with
  * implementing it. */

 virtual void set_par( const idx_type par , const double value ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given string parameter
 /** Set a given string parameter. The method is given a void implementation
  * doing nothing (i.e., ignoring the set value), rather than being pure
  * virtual, so that derived classes not having any working string parameter
  * (i.e., either not having any or not really reacting to the ones that they
  * supposedly have) do not have to bother with implementing it. */

 virtual void set_par( const idx_type par , const std::string & value ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the whole set of parameters in one blow
 /** This method sets the whole set of parameters in one blow using a
  * ComputeConfig object.
  *
  * Although the class is thin, this method is given a working configuration
  * using the class interface; hence, derived classes correctly implementing
  * set_par() (all the required versions), get_num_*_par(), get_dflt_*_par()
  * and *_par_str2idx() can in principle avoid to re-implement it.
  *
  * Note that the ComputeConfig in principle only contains a subset of the
  * possible parameters. The way in which the ComputeConfig must be
  * "interpreted" depends on the f_diff field in there: if f_diff == true then
  * all the other parameters are left unchanged, while if f_diff == false then
  * all the parameters that are *not* specified in the ComputeConfig are
  * rather reset to their default value. Note that for a "fresh" (just
  * constructed) object, the two values of f_diff are equivalent. Since
  * calling set_ComputeConfig( nullptr ) would not make sense if the "empty"
  * ComputeConfig were intended in the differential sense (it would do
  * nothing), when scfg == nullptr the field f_diff should be interpreted as
  * false, which "resets the object to its factory defaults" (hence calling
  * set_ComputeConfig( nullptr ) on a "fresh" object makes little sense).
  * Note that in the base ThinComputeInterface class implementation this is
  * obtained by just calling set_par() for all parameters, resetting them to
  * default values, and then calling it again on the parameters specified in
  * the ComputeConfig (if any); if this has negative performance impacts on a
  * given :ThinComputeInterface, it will have to re-implement its version of
  * the method. 
  *
  * The above is already reason enough for the method to be virtual, but not
  * the only one. For instance, derived classes may need to do something to
  * react to changes of the parameters which is different from what is done
  * when they are individually changed. A particularly simple case is when
  * the :ThinComputeInterface formally has some parameters but in fact it
  * "listens to no-one", in which case the implementation does not need to
  * do anything. Conversely, the implementation in the base class does not
  * use the f_extra_Configuration field of the ComputeConfig, so any
  * :ThinComputeInterface which needs it will have to derive its own version
  * of the method (but, not necessarily of ComputeConfig). */

 virtual void set_ComputeConfig( ComputeConfig *scfg = nullptr );

/*@} -----------------------------------------------------------------------*/
/*-------------------- METHODS FOR DOING THE COMPUTATION -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Compute whatever the object is supposed to
 *  @{ */

 /// (try to) compute whatever the object is supposed to
 /** Starts or the computation process, or restarts a previously interrupted
  * computation process, and returns the status of the solver at termination.
  *
  * The return code indicates the status of the computation; it is expected to
  * follow the rules set by compute_type [see above], but it is a generic int
  * so that derived classes can add their own specific return codes.
  *
  * The computation can be expected to be (although it is not necessarily)
  * costly, i.e., it can take "a lot of time". It is therefore necessary to
  * clarify what "can happen while the computation in underway", in particular
  * considering the multi-threaded scenario. In principle, the object doing
  * the computation may undergo changes, which however can be of two different
  * types:
  *
  * - "Normal" changes which happen in the object (Constraint, Objective,
  *   Function, ...) and/or in the Block it is attached to (Solver), which
  *   the object either "knows directly" (as they happen at it) or is 
  *   properly notified (by an appropriate Modification).
  *
  * - In addition, for all objects that share the ThinComputeInterface
  *   interface the computation depends, among other things, from a set of
  *   Variable. For Constraint, Objective and Function these are the "active"
  *   Variable, while for Solver, these are the Variable in the Constraint of
  *   the Block it is attached to which do not belong to the Block itself,
  *   and therefore have to be treated as constrants when the Block is solved.
  *   These may change their value, but there is no mechanism that signals any
  *   abject if such a change has occurred.
  *
  * During the execution of compute(), any of these may in principle occur:
  * either because of another thread working on the same objects, or because
  * the computation itself triggers some complex chain of events resulting in
  * a change. However, the changes may affect the result of the computation,
  * which could therefore be no longer correct for the state of the object
  * at the beginning of the call, but only for that at the end (and note that
  * there may be no bound on the number of changes which may occurr in the
  * meantime). The point is whether this is allowed to happen, and the general
  * answer is "no". That is, the rule is that
  *
  *   during a call to compute(), no changes must occur to the object
  *   (say, the whole Block to which the Solver is attached to) which
  *   change the answer that compute() is supposed to compute
  *
  * The rule leaves scope for some changes occurring, although these must
  * ensure that the answer is not affected. Changes of this type include
  * generation of valid inequalities/lazy constraints/dynamic variables in
  * a Block (being solved by a Solver), as these are supposed not to change
  * the optimal solution of the underlying problem. Note that, however,
  *
  *               IT IS UNIQUELY THE CALLER'S RESPONSIBILITY
  *                 TO ENSURE THAT THE RULE IS RESPECTED
  *
  * This should always be possible, if necessary by making copies of the
  * objects (e.g., via an R3 Block). The object doing compute() is *not*
  * required to check for violation of the rules, as this might be complex
  * and/or computational costly. This is for instance true for the abrupt
  * change of the value of Variable, which would require keeping a copy of
  * the initial value and periodic checks. Other changes may be easier to
  * detect, e.g. because they are conveyed by a Modification. It is therefore
  * possible (and, potentially, advised) for an object to check for such an
  * occurrence and react by immediately terminating computation returning a
  * kError status (or any more specific status > kError), but this is not
  * required and left to each specific object to decide.
  *
  * The parameter changedvars is meant to cater for a similar occurrence
  * happening not inside a single call to compute(), but between two
  * consecutive ones. The point is that when compute() is called, it may be
  * for restarting a previously started computation process that has been
  * temporarily interrupted. If no changes occurred in the object, this 
  * should be "very easy": just "jump in the main loop where you exited it,
  * and continue as if nothing had happened" (this is provided the state of
  * the solution process is properly preserved, which is usually the case).
  * Otherwise, the computation may have to be significantly reshaped: in the
  * best case reoptimization techniques can be used to warm-start the new one
  * using results from the old one, but in some cases there could be no other
  * resort than restarting everything from scratch.
  *
  * Hence, each time compute() is called and a previous state is available,
  * the object will have to decide how to make best use of it. This depends
  * on whether or not there were changes in the object. As previously
  * mentioned, there are two types of changes: those that are surely "known"
  * to the object (either because they occurred in a Constraint, Objective
  * or Function, or because are properly signalled by an appropriate
  * Modification), and the changes of the value of the relevant Variable. The
  * only way to know whether the latter has happened would be to read the
  * value (whatever that is) and to compare it with the previous one, which
  * in turn would imply having kept a copy of the latter, which is typically
  * not done (see the general rule above).
  *
  * The changedvars parameters is here to solve this issue. If it is false,
  * then the compute() method is allowed to assume that the value of the
  * relevant Variable (if any) has not changed since the last call. This
  * therefore allows the object to "resume the computation where it started",
  * provided of course that no other changes of different type occurred. Note
  * that this is actually providing the object with the *guarantee* that the
  * Variable have not changed: thus, compute() can read the current value of
  * the Variable and be guaranteed that it is the same as it was previously,
  * directly extending the general rule valid within one call to compute()
  * to possibly arbitrarily long sequences of calls. As for the general rule,
  * it is uniquely the caller's responsibility to ensure that the property
  * holds, as the object will typically not have the means for checking it
  * (and anyway are not required to).
  *
  * If changedvars == true instead, the object has to assume that the
  * relevant variables have changed. If it is important for the object to
  * detect which ones have changed and which ones have not, it will have to
  * implement this check internally. This implies keeping a copy of the
  * previous values, which is what the global rule avoids being required to
  * do, but that clearly the global rule does not forbid doing. */

 virtual int compute( bool changedvars = true ) = 0;

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the ThinComputeInterface
 *
 *  The base class does not have parameters of its own, but it sets the
 *  interface so that derived classes can uniformly set and access to their
 *  parameters, possibly using a ComputeConfig object to do so.
 *  @{ */

 /// get the number of int parameters
 /** Get the number of int parameters. The method is given an implementation
  * (returning 0), rather than being pure virtual, so that derived classes
  * not having any int parameter do not have to bother with implementing it.
  */

 virtual idx_type get_num_int_par( void ) const {
  return( idx_type( 0 ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get the number of double parameters
 /** Get the number of double parameters. The method is given an
  * implementation (returning 0), rather than being pure virtual, so that
  * derived classes not having any double parameter do not have to bother
  * with implementing it. */

 virtual idx_type get_num_dbl_par( void ) const {
  return( idx_type( 0 ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get the number of string parameters
 /** Get the number of string parameters. The method is given an
  * implementation (returning 0), rather than being pure virtual, so that
  * derived classes not having any string parameter do not have to bother
  * with implementing it. */

 virtual idx_type get_num_str_par( void ) const {
  return( idx_type( 0 ) );
  }

/*--------------------------------------------------------------------------*/
 /// get the default value of an int parameter
 /** Get the default value of the int parameter with given index. The method
  * is given a void implementation (returning 0), rather than being pure
  * virtual, so that derived classes not having any int parameter do not
  * have to bother with implementing it. */
 
 virtual int get_dflt_int_par( const idx_type par ) const {
  return( int( 0 ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get the default value of a double parameter
 /** Get the default value of the double parameter with given index. The
  * method is given a void implementation (returning 0), rather than being
  * pure virtual, so that derived classes not having any double parameter do
  * not have to bother with implementing it. */
 
 virtual double get_dflt_dbl_par( const idx_type par ) const {
  return( double( 0 ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get the default value of a string parameter
 /** Get the default value of the string parameter with given index. The
  * method is given a void implementation (returning the empty string),
  * rather than being pure virtual, so that derived classes not having any
  * string parameter do not have to bother with implementing it. */
 
 virtual const std::string & get_dflt_str_par( const idx_type par ) const {
  static const std::string empty = "";
  return( empty );
  }

/*--------------------------------------------------------------------------*/
 /// get a specific integer (int) numerical parameter
 /** Get a specific integer (int) numerical parameter. The method is given a
  * void implementation always returning the default value for the parameter,
  * rather than being pure virtual, so that derived classes not having any
  * working int parameter (i.e., either not having any or not really reacting
  * to the ones that they supposedly have) do not have to bother with
  * implementing it. */

 virtual int get_int_par( const idx_type par ) const {
  return( get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a specific float (double) numerical parameter
 /** Get a specific float (double) numerical parameter. The method is given a
  * void implementation always returning the default value for the parameter,
  * rather than being pure virtual, so that derived classes not having any
  * working double parameter (i.e., either not having any or not really
  * reacting to the ones that they supposedly have) do not have to bother with
  * implementing it. */

 virtual double get_dbl_par( const idx_type par ) const {
  return( get_dflt_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a specific string numerical parameter
 /** Get a specific string numerical parameter. The method is given a void
  * implementation always returning the default value for the parameter,
  * rather than being pure virtual, so that derived classes not having any
  * working string parameter (i.e., either not having any or not really
  * reacting to the ones that they supposedly have) do not have to bother
  * with implementing it. */
 
 virtual const std::string & get_str_par( const idx_type par ) const {
  return( get_dflt_str_par( par ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the index of the int parameter with given string name
 /** This method takes a string, which is assumed to be the name of an int
  * parameter, and returns its index, i.e., the integer value that can be
  * used in [set/get]_par() to set/get it. The method is given a void
  * implementation (throwing exception), rather than being pure virtual, so
  * that derived classes not having any int parameter do not have to bother
  * with implementing it. */

 virtual idx_type int_par_str2idx( const std::string & name ) const {
  throw( std::invalid_argument( std::string( "int parameter " ) + name +
				std::string( " unknown" ) ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the double parameter with given string name
 /** This method takes a string, which is assumed to be the name of a double
  * parameter, and returns its index, i.e., the integer value that can be
  * used in [set/get]_par() to set/get it. The method is given a void
  * implementation (throwing exception), rather than being pure virtual, so
  * that derived classes not having any double parameter do not have to bother
  * with implementing it. */

 virtual idx_type dbl_par_str2idx( const std::string & name ) const {
  throw( std::invalid_argument( std::string( "double parameter " ) + name +
				std::string( " unknown" ) ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the string parameter with given string name
 /** This method takes a string, which is assumed to be the name of a string
  * parameter, and returns its index, i.e., the integer value that can be
  * used in [set/get]_par() to set/get it. The method is given a void
  * implementation (throwing exception), rather than being pure virtual, so
  * that derived classes not having any string parameter do not have to bother
  * with implementing it. */

 virtual idx_type str_par_str2idx( const std::string & name ) const {
  throw( std::invalid_argument( std::string( "string parameter " ) + name +
				std::string( " unknown" ) ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the string name of the int parameter with given index
 /** This method takes an int parameter index, i.e., the integer value that
  * can be used in [set/get]_par() [see above] to set/get it, and returns its
  * "string name". The method is given a void implementation (throwing
  * exception), rather than being pure virtual, so that derived classes not
  * having any int parameter do not have to bother with implementing it. */

 virtual const std::string & int_par_idx2str( const idx_type idx ) const {
  throw( std::invalid_argument( "invalid int parameter name" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the string name of the double parameter with given index
 /** This method takes a double parameter index, i.e., the integer value that
  * can be used in [set/get]_par() [see above] to set/get it, and returns its
  * "string name". The method is given a void implementation (throwing
  * exception), rather than being pure virtual, so that derived classes not
  * having any double parameter do not have to bother with implementing it. */

 virtual const std::string & dbl_par_idx2str( const idx_type idx ) const {
  throw( std::invalid_argument( "invalid double parameter name" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the string name of the string parameter with given index
 /** This method takes a string parameter index, i.e., the integer value that
  * can be used in [set/get]_par() [see above] to set/get it, and returns its
  * "string name". The method is given a void implementation (throwing
  * exception), rather than being pure virtual, so that derived classes not
  * having any string parameter do not have to bother with implementing it. */

 virtual const std::string & str_par_idx2str( const idx_type idx ) const {
  throw( std::invalid_argument( "invalid string parameter name" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 ///< get the whole set of parameters in one blow
 /**< This method gets the whole set of parameters in one blow using a
  * ComputeConfig object.
  *
  * If a non-null ocfg is provided, then the pointed object (that is assumed
  * to be "empty", at least insomuch as the fields int_pars, dbl_pars and
  * str_pars are concerned) is "filled" with the data of the current
  * configuration, and returned. Otherwise, a new ComputeConfig object is
  * created, filled and returned. This is done to allow :ThinComputeInterface
  * to return objects of a class derived from ComputeConfig containing
  * information about specific parts of their configuration that are not
  * present in the base ComputeConfig class, filling that part only and then
  * using method of the base ThinComputeInterface class to fill-in the
  * standard part. However, the "final user" should not bother and assume
  * that the :ThinComputeInterface will ultimately produce the right kind of
  * object, thereby leaving the parameter to its nullptr default value.
  *
  * If all == true, then the whole set of parameters is copied. If all ==
  * false instead (default), only the parameters that are *not* at their
  * default value are.
  *
  * Although the class is thin, this method is given a working configuration
  * using the class interface; hence, derived classes correctly implementing
  * get_*_par(), get_num_*_par(), get_dflt_*_par() and *_par_idx2str() can in
  * principle avoid to re-implement it. Note that when all == false, the
  * resulting ComputeConfig object may turn out to be "empty": no values
  * stored, which means that all the parameters are at their default value.
  * In this case, if the ComputeConfig object has *not* been provided by the
  * caller, then no ComputeConfig is returned: nullptr is. This tells in a
  * "compact form" (analogous to set_ComputeConfig()) the same information,
  * i.e., "all parameters to their defult value". This is done by the base
  * class implementation, but note that a ComputeConfig is only meant to
  * actually store information about (non-default) values of parameters
  * *that its :ThinComputeInterface actually cares about*. This means that
  * if a specific :ThinComputeInterface formally is has some parameter, but
  * in fact does not "listen" to it, it should never even process them,
  * thereby increasing the chance of a nullptr result. Actually, a
  * :ThinComputeInterface that either has no parameter, or formally has some
  * but in fact "listens to none", can re-define this method to uniformly
  * return nullptr.
  *
  * A :ThinComputeInterface may have several other reasons for wanting to
  * re-define it. In particular, the base class version does not use the
  * f_extra_Configuration field of the ComputeConfig, so any
  * :ThinComputeInterface which needs it will have to derive its own version
  * of the method (but, not necessarily of ComputeConfig). */

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
				      ComputeConfig * ocfg = nullptr ) const;

/*@} -----------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
// empty, this is a thin interface
 
/*--------------------------------------------------------------------------*/

 };   // end( class ThinComputeInterface )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ComputeConfig --------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Configuration for the ThinComputeInterface parameters
/** Derived class from Configuration to describe all the parameters that a
 * :ThinComputeInterface may have (derived classes only, of course, the base
 * class is thin and does not have any).
 *
 * The class holds three lists:
 *
 *  - a list of pairs < string , int >
 *  - a list of pairs < string , double >
 *  - a list of pairs < string , string >
 *
 * The idea is that each list contains the pairs < parameter name , value >
 * to be changed/set. The lists need *not* contain all the parameters (of the
 * given type), all those not directly specified are treated as speficied in
 * the bool field f_diff. If f_diff == true, then the ComputeConfig has to be
 * "interpreted in a differential sense": all parameters not specified must
 * not be changed from their current value. If f_diff == false instead, then
 * all the parameters that are *not* specified in the ComputeConfig are
 * rather reset to their default value. Note that the same holds for the
 * provided field f_extra_Configuration, which allows to specify an arbitrary
 * extra Configuration object: if f_diff == true and f_extra_Configuration
 * == nullptr this has to be interpreted as "leave the previous extra
 * Configuration as it is", whereas if f_diff == false then the current extra
 * Configuration is replaced by that in the ComputeConfig (which may be
 * nullptr; depending on the :ThinComputeInterface this may be implemented by
 * just storing the nullptr, or by resetting that part of the configuration
 * to its default value as the rest). Note, however, that for a "fresh" (just
 * constructed) solver, the two values of f_diff are equivalent.
 * 
 * It is always possible to define a specific :ComputeConfig corresponding to
 * a specific :ThinComputeInterface, but the fact that the set of parameters
 * is freely extendable together with the flexibility provided by the extra
 * Configuration field may be enough to cover many use cases without a 
 * specific :ComputeConfig class definition. */

class ComputeConfig : public Configuration
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: initializes everything to "default configuration"
 ComputeConfig( void ) : Configuration() , f_diff( true ) ,
  f_extra_Configuration( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin
 ComputeConfig( const ComputeConfig &old ) : Configuration() {
  f_diff = old.f_diff;
  int_pars = old.int_pars;
  dbl_pars = old.dbl_pars;
  str_pars = old.str_pars;
  f_extra_Configuration = old.f_extra_Configuration->clone();
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor; it deletes the f_extra_Configuration (if any)
 virtual ~ComputeConfig() {
  delete f_extra_Configuration;
  }

/*------------------------------- CLONE -----------------------------------*/
 /// clone method
 virtual ComputeConfig * clone( void ) const override {
  return( new ComputeConfig( *this ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a ComputeConfig. Besides the mandatory "type" attribute of
  * any :Configuration, the group should contain the following:
  *
  * - the attribute "diff" of int type containing the value for the f_diff
  *   field of the ComputeConfig (basically, a bool telling if the
  *   information in it has to be taken as "the configuration to be set" or
  *   as "the changes to be made from the current configuration");
  *
  * - the dimension "num_int_par" containing the number of int parameters;
  *
  * - the variable "int_par_names", of type string and indexed over the
  *   dimension "num_int_par"; the i-th entry of the variable is assumed to
  *   contain the string name of an int parameter (see int_par_idx2str());
  *
  * - the variable "int_par_vals", of type int and indexed over the
  *   dimension "num_int_par"; the i-th entry of the variable is assumed to
  *   contain the value of the int parameter whose string name is to be found
  *   in the i-th entry of "int_par_names";
  *
  * - the dimension "num_dbl_par" containing the number of double parameters;
  *
  * - the variable "dbl_par_names", of type string and indexed over the
  *   dimension "num_dbl_par"; the i-th entry of the variable is assumed to
  *   contain the string name of a double parameter (see dbl_par_idx2str());
  *
  * - the variable "dbl_par_vals", of type double and indexed over the
  *   dimension "num_dbl_par"; the i-th entry of the variable is assumed to
  *   contain the value of the double parameter whose string name is to be
  *   found in the i-th entry of "dbl_par_names";
  *
  * - the dimension "num_str_par" containing the number of string parameters;
  *
  * - the variable "str_par_names", of type string and indexed over the
  *   dimension "num_str_par"; the i-th entry of the variable is assumed to
  *   contain the string name of a string parameter (see int_par_idx2str());
  *
  * - the variable "str_par_vals", of type string and indexed over the
  *   dimension "num_str_par"; the i-th entry of the variable is assumed to
  *   contain the value of the string parameter whose string name is to be
  *   found in the i-th entry of "str_par_names";
  *
  * - the group "extra" containing a Configuration object, which has no
  *   direct use in the base ComputeConfig class, but is added so that
  *   derived classes can put there any configuration information without
  *   having to define further derived classes form ComputeConfig (which,
  *   however, they can still do if they want).
  *
  * The three dimensions "num_*_par" are mandatory. If any of these is zero,
  * the corresponding variables "*_par_names" and "*_par_vals" may not be
  * defined. The "extra" group may not exist, in which case the corresponding
  * Configuration object is set to a nullptr. */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a ComputeConfig. See
  * ComputeConfig::deserialize( netCDF::NcGroup ) for details of the format
  * of the created netCDF group. */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/*--------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 bool f_diff;  ///< tells is the configuration is a "differential" one

 /// list of pairs < string , int > for integer-valued parameters
 std::vector< std::pair< std::string , int > > int_pars;

 /// list of pairs < string , double > for float-valued parameters
 std::vector< std::pair< std::string , double > > dbl_pars;

 /// list of pairs < string , string > for string-valued parameters
 std::vector< std::pair< std::string , std::string > > str_pars;

 /// any extra ThinComputeInterface-specific Configuration
 Configuration *f_extra_Configuration;

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the ComputeConfig
 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this ComputeConfig out of an istream
 /** Load this ComputeConfig out of an istream. The format of the istream is:
  *
  * a bool
  *
  * number k of the names of int parameters
  *
  * for i = 1 ... k
  * - a string containing the name of the int perameter
  * - an int (the parameter)
  *
  * number k of the names of double parameters
  *
  * for i = 1 ... k
  * - a string containing the name of the double perameter
  * - a double (the parameter)
  *
  * number k of the names of string parameters
  *
  * for i = 1 ... k
  * - a string containing the name of the string perameter
  * - a string (the parameter)
  *
  * a string containing the class type of the extrs Configuration object,
  * '*' means none (nullptr)
  *
  * if the above is not '*', the description of the :Configuration object
  */

 virtual void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ComputeConfig ) )

/*@}  end( group( ThinComputeInterface_CLASSES ) ) -------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ThinComputeInterface.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File ThinComputeInterface.h -----------------------*/
/*--------------------------------------------------------------------------*/






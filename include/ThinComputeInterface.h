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
 * \version 0.11
 *
 * \date 15 - 06 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
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

 using idx_type = unsigned short int;  ///< the type of parameters indices
 
/*--------------------------------------------------------------------------*/
 /// public enum for the possible return values of compute()

 enum compute_type {
 kUnEval = 3 ,   ///< compute() has not been called yet
                 /**< Any return value between 0 and kUnEval (extremes
		  * included) means that the process of compute() has not
 * finished yet. This may mean a few different things (which is why kUnEval
 * is not 0), like "compute() has not been called", "compute() is actually
 * running right now", or "compute() had been called and it did finish, but
 * the thing that had to be compute()-d changed in the meantime" (like,
 * this was solving a Block that has undergone come change). The specific
 * values are left to derived classes, but the general gist is that any
 * value <= kUnEval means that the solution process has not yet reached a
 * state where a solution can be declared (or such a state has been lost for
 * some reason. */

 kOK = 10 ,      ///< successful compute()
                 /**< Any return value between kUnEval (excluded) and kOK
		  * (included) means that the object ran smoothly, obtaining
 * the desired answer within the allowed limits on the available computational
 * resources (if any). The fact that multiple values are allowed corresponds
 * to the fact that "the desired answer" may in fact be of different types,
 * such as that some optimization problem has been conclusively solved to
 * optimality, or conclusively shown to be empty, or conclusively shown to be
 * unbounded. */

 kError = 18 ,   ///< compute() stopped because of unrecoverable error
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
 * where compute() was not able to conclusively obtain all of the desired
 * answer (although it may have already obtained a part of it), but this was
 * due to some reason that forced it to stop early on, such as a limit imposed
 * on the available computational resources. By relaxing the limit, which may
 * be as simple as calling compute() again, compute() may further proceed in
 * the computation process, possibly providing a kOK-type answer. */

 };  // end( compute_type )

/*--------------------------------------------------------------------------*/
 /// the type of an event handler
 /** An event is some occurrence happening inside compute() to which the
  * caller may want to promptly react, before termination of the call. In
  * order to achieve that, the caller may register event handlers with the
  * ThinComputeInterface. An event handler, as defined by the EventHandler
  * type, is a [std::]function taking no input and returning an int that
  * tells the ThinComputeInterface which action it has to take after the
  * event (see action_type). Each ThinComputeInterface will support a set of
  * different types of events (see event_type), and will call all the event
  * handlers registered under a certain event type when the corresponding
  * condition occurs. Note that the return type is int rather than action_type
  * to allow derived classes to extend the typer of actions they sypport. */

 using EventHandler = std::function< int ( void ) >;

/*--------------------------------------------------------------------------*/
 /// the type of the internal ID of an event handler
 /** When an event handler is registered into a ThinComputeInterface, it gets
  * a unique ID that can be, and should, used later on to un-register it. One
  * does not expect more than 65536 event handlers being registered to any
  * sentible ThinComputeInterface, but should it be so, the definition of
  * EventID could be changed accordingly. */

 using EventID = unsigned short int;

/*--------------------------------------------------------------------------*/
 /// public enum for the possible types of events
 /** This enum defines a set of "basic" event types that every
  * :ThinComputeInterface should reasonably be able to manage, although each
  * :ThinComputeInterface is completely free to choose which ones it actually
  * supports. Also, a :ThinComputeInterface is completely free to "extend"
  * event_type and define new class-specific events that their compute() can
  * support. */

 enum event_type {
 eBeforeTermination = 0 ,   ///< event to be called just prior to terminating
                            /**< Type of events that will be called right
			     * before compute() terminates. This is provided
 * in particular to handle cases such as a computation entailing the solution
 * of an optimization problem whose model is dynamically generated (say, row
 * and/or column generation). In such a case the optimality conditions may
 * have been satisfied for the current partial model (master problem) which
 * would lead compute() to terminate; before doing this, these events are
 * invoked, which allows to trigger the generation of new rows and/or columns
 * (separation, pricing). The event can then return eForceContinue [see] to
 * instruct compute() to incorporate the new information into the model 
 * (provided, of course, that this makes any sense for the compute() at hand)
 * and check the stopping conditions again. */

 eEverykIteration   = 1 ,   ///< events to be called every k iterations
                            /**< Type of events that will be called every
			     * k iterations, whatever "iteration" means for
 * the compute() at hand. The value of k is to be set with a separate
 * algorithmic parameter, properly defined by derived classes actually
 * implementing the mechanism. */

 eEveryTTime        = 2 ,   ///< events to be called periodically in time
                            /**< Type of events that will be called 
			     * periodically every fixed amount T of time.
 * The value of T is to be set with a separate algorithmic parameter, properly
 * defined by derived classes actually implementing the mechanism. Note that
 * in general one does not expect derived classes to be very "tight" in heeding
 * to the time interval T, in the sense that they will typically check
 * periodically (say, every iteration) whether the elapsed time has passed,
 * and call the event of it has. If iterations are much longer than T this
 * may cause some events not to be called at all, although of course a
 * derived class may place appropriate checks in multiple places to try to
 * avoid this. */

 e_last_event_type  = 4     ///< conveniemce value to define new events
                            /**< conveniemce value to allow derived classes
			     * to "extend" event_type and define new
 * class-specific events that their compute() can support. */
 
 };  // end( compute_type )

/*--------------------------------------------------------------------------*/
 /// public enum for the possible types of actions in response to events
 /** This public enum provides values that describe general actions that an
  * implementation of compute() is supposed to being instructed to perform
  * by the return value of the event handler. */

 enum action_type {
 eForceContinue = 0 ,   ///< force compute() to continue even if it would stop
                        /**< If compute() was going to stop because it
			 * considered the computation to be over, force it
 * to reconsider this. A typical case in which this can happen is if the
 * computation entails the solution of an optimization problem whose model
 * is dynamically generated (say, row and/or column generation). In such a
 * case the optimality conditions may have been satisfied for the current
 * partial model (master problem), but the event may have triggered the
 * generation of new rows and/or columns (separation, pricing). This return
 * value instructs compute() to check if this has happened (provided, of
 * course, that this makes any sense for the compute() at hand). */

 eContinue = kUnEval ,   ///< continue compute()
                         /**< If the event handler returns any value
			  * comprised between 0 and eContinue (extremes
 * included, then compute() will continue. In particular, eContinue means
 * "business as usual", while values < eContinue may give specific
 * instructions (cf. e.g. eForceContinue). eContinue is taken equal to
 * kUnEval to simplify handling of return errors between the event handler
 * and compute(), see eStopOK and eStopError for details. */

 eStopOK = kOK ,         ///< force compute() to stop returning success
                         /**< If the event handler returns any value
			  * comprised between eContinue (excluded) and
 * eStopOK (included), compute() should immediately stop (some delay is
 * possible if required by the implementation) because the event has
 * detected that whatever needed to be compute()-d, has already been
 * satsfactorily compute()-d. eStopOK is taken equal to kOK, so that
 * 
 *     compute() WILL RETURN AS ITS STATUS PRECISELY THE VALUE
 *     RETURNED BY THE EVENT HANDLER
 *
 * This allows the event handler to more finely specify "what kind of good
 * stop has occurred", in case compute() supports more than one (say,
 * compute() requires solving a Block which may have an optimal solution, or
 * be empty, or be unbounded). The event handler needs to know which compute()
 * it is handling, and therefore ensure that the return value is valid for
 * that compute(). Furthermore, obviously the event handler has to force
 * this termination "for good reasons", possibly providing to compute() the
 * extra information it needs to properly function after termination (say,
 * compute() requires solving a Block, and the event handler has can detect
 * termination early by ab ad-hoc computation of a dual solution; then, the
 * dual solution will have to be provided to compute() if it can be required
 * by the user after compute() terminates.) */

 eStopError = kError     ///< force compute() to stop returning error
                         /**< If the event handler returns any value
			  * > eStopOK, then compute() should immediately
 * stop (some delay is possible if required by the implementation) because
 * the event has detected that whatever needed to be compute()-d can or need
 * no longer be computed, say because some required computational resource
 * (that compute() does not directly knows of or controls) is terminated.
 * In this case
 * 
 *     compute() WILL RETURN AS ITS STATUS PRECISELY THE VALUE
 *     RETURNED BY THE EVENT HANDLER
 *
 * This allows the event handler to more finely specify "what kind of bad
 * stop has occurred", in case compute() supports more than one. In
 * particular, values >= eStopError (which is taken equal to kError) are meant
 * to represent "irrecoverable" errors, from which compute() is not likely to
 * be able to ever recover. Instead, values between eStopOK and eStopError,
 * extremes excluded, are left for "recoverable error states" where compute()
 * was not able to conclusively obtain all of the desired answer (although it
 * may have already obtained a part of it), but this was due to some reason
 * that forced it to stop early on, such as a limit imposed on the available
 * computational resources. By relaxing the limit, which may be as simple as
 * calling compute() again, compute() may further proceed in the computation
 * process, possibly finally providing a "good" answer. The event handler
 * needs to know which compute() it is handling, and therefore ensure that
 * the return value is valid for that compute(). */

 };  // end( compute_type )

/**@} ----------------------------------------------------------------------*/
/*----------- CONSTRUCTING AND DESTRUCTING ThinComputeInterface ------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing ThinComputeInterface
 *  @{ */

 /// constructor: does nothing, the class is thin

 ThinComputeInterface( void ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual and does nothing, the class is thin

 virtual ~ThinComputeInterface() { }

/**@} ----------------------------------------------------------------------*/
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

/**@} ----------------------------------------------------------------------*/
/*---------------------- METHODS FOR EVENTS HANDLING -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Set event handlers
 *
 *  The computation can be a long and complex process. Although some control
 *  on it (say, its maximum time) is already allowed by the parameters, a
 *  more fine-grained and ultimately interactive control may be needed. This
 *  is what event handlers provide.
 *
 * An event is some occurrence happening inside compute() to which the caller
 * may want to promptly react "immediately", i.e., before termination of the
 * call. In order to achieve that, the caller may register event handlers
 * with the ThinComputeInterface. An event handler, as defined by the
 * EventHandler type, is a std::function taking no input and returning an int
 * that tells the ThinComputeInterface which action it has to take after the
 * event (see action_type). Each ThinComputeInterface will support a set of
 * different types of events (see event_type), and will call all the event
 * handlers registered under a certain event type when the corresponding
 * condition occurs.
 *
 * Note that each ThinComputeInterface defines its own set of events, and
 * can throw exception if required to handle events it does not support.
 * Indeed, a ThinComputeInterface can be "so light" (a linear function ...)
 * that events are not really a sensible option, which is why the default
 * implementation of set_event_handler() unconditionally throws exception.
 *
 * The interface of event handlers may seem excessively threadbare; in
 * particular, the event handler does not apparently even know "which
 * ThinComputeInterface originated it". However, the typical usage pattern
 * of event handlers is via lambdas, which cleanly solve the issue. That is,
 * assume for instance that a callback foo( ThinComputeInterface * tci ) is
 * available that one just want to call: it is easy to embed the information
 * of the ThinComputeInterface * the callback is registered to (say, mytci)
 * into the std::function by
 *
 *     mytci->set_event_handler( mytype ,
 *                               [ mytci ] () { return( foo( mytci ) ); } );
 *
 * Note that this works also if foo() is, say, a member of the class where
 * the call occurs, without requiring the use of std::bind, provided that
 * this is explicitly added to the capture list. Also, note that the thusly
 * defined lambda is a temporary whose lifetime does not extend beyond the
 * end of the call to set_event_handler(). However, this is not an issue
 * because the std::function that is created is then (allegedly) moved into
 * the internal data structures of the ThinComputeInterface. This would not
 * be possible by using a function pointer instead, as it would point to a
 * soon-to-be temporary object. Besides, while there is a standard conversion
 * between a lambda which *does not capture any context* to a function
 * pointer, this does not work when the capture list is nonempty.
 *
 * The drawback is that calling a std::function incurs into a (hopefully,
 * minimal) overhead w.r.t. calling a function pointer, but this seems to be
 * largely justified by the increased functionality of the solution. However,
 * this also implies that
 *
 *     EVENT HANDLERS SHOULD NOT BE CALLED IN THE TIGHTEST LOOPS Of compute()
 *
 * which does not seem to be too harsh a requirement.
 *
 * By being provided with, among any other information, a pointer to the
 * ThinComputeInterface, the event handler can access all its public
 * interface and therefore fetch from it all the information it needs to
 * process the event. This will typically be :ThinComputeInterface-specific,
 * although a skeleton interface is defined in ThinComputeInterface already
 * for some very general concept that many ThinComputeInterface will probably
 * support. An important note should also be made in this respect:
 *
 *     THE EVENT HANDLER WILL BE EXECUTED IN THE (MAIN) THREAD EXECUTING
 *     compute(), WHICH THEREFORE WILL "NOT BE RUNNING" WHILE THE EVENT
 *     HANDLER IS BEING EXECUTED
 *
 * This is of course a general statement, which will be true for any
 * completely serial compute(), but may not be so for a compute() that is
 * itself multi-threaded. However, the design principle is that
 *
 *     EVERY METHOD IN THE PUBLIC INTERFACE OF THE ThinComputeInterface
 *     IS CALLABLE BY AN EVENT HANDLER UNLESS EXPLICTLY DECLARED OTHERWISE
 *
 * That is, each :ThinComputeInterface will have to specify, possibly
 * separately for each type of event it supports, if some methods of its
 * public interface are not available to be called by the event handler
 * (maybe because the :ThinComputeInterface is multi-threaded and it cannot
 * or does not want to handle the necessary synchronization), with the
 * default being that if nothing is said then all of them can. Of course, a
 * :ThinComputeInterface is allowed to rather specify a very small set of
 * methods that can be called, which is OK provided it is explicitly done.
 *
 * It should be obvious, but let us explicitly remark that
 *
 *     IF THE EVENT HANDLER CAN BECOME INVALID, SAY BECAUSE THE OBJECT
 *     WHOSE foo() METHOD IS INVOKED GOES OUT OF EXISTENCE, IT MUST BE
 *     UNREGISTERED FROM THE ThinComputeInterface BEFORE THIS HAPPENS
 *
 * This is of course different from the fact that the EventHandler object
 * itself becomes invalid; that the latter does not happen will be guaranteed
 * by the ThinComputeInterface, but it is the caller's responsibility to
 * ensure that any information that the event handler relies onto (apart of
 * course from the ThinComputeInterface *, if any) will still be correct each
 * time that the event handler is invoked. This is why reset_event_handler()
 * and the handler id concept are provided.
 *
 *  @{ */

 /// register a new event handler, returning its id
 /** Adds a new event handler to these regostered for the given type. As the
  * && tells, the event handler becomes property of the ThinComputeInterface,
  * which is completely OK if, as one expects, it is defined via a lambda
  * function. The method returns a unique id for the handler, which can (and
  * must) be later used to remove the handler before it becomes invalid. Note
  * that the handler is type-specific, i.e., two event handlers of different
  * types can have the same id; in other words, the "real" id is the pair
  * ( type , id ). An exception is thrown if the ThinComputeInterface is not
  * capable of handling this type or event for whatever reason, among which
  * that it has exhausted the available maximum number of event handlers
  * slots for the given type. The method of the base class always throws
  * exception. */

 virtual EventID set_event_handler( int type , EventHandler && event )
 {
  throw( std::logic_error( "ThinComputeInterface::set_event_handler called" )
	 );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// unregister an existing event handler
 /** Removes the event handler with the given id from the list of those
  * registered for the given type. If there is no event handler with the
  * given id for the given type, exception will be thrown. The method of the
  * base class always throws exception. */

 virtual void reset_event_handler( int type , EventID id )
 {
  throw( std::logic_error( "ThinComputeInterface::reset_event_handler called"
			   ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- METHODS FOR DOING THE COMPUTATION -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Compute whatever the object is supposed to
 *  @{ */

 /// (try to) compute whatever the object is supposed to, synchronously
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

/*--------------------------------------------------------------------------*/
 /// (try to) compute whatever the object is supposed to, asynchronously
 /** This is just an one-line wrapper over compute() that runs it into a
  * separate task and returns a std::future<int> upon which the caller can
  * wait() for the result. Not really a significant contribution (it is not
  * even virtual) and by no means the only way to make an asynchronous call
  * to compute(), just a little convenience method that conveys what is
  * perhaps the most convenient current C++ technique for asynchronous calls
  * to compute(). */

 std::future<int> compute_async( bool changedvars = true ) {
  return( std::async( std::launch::async ,
		      &ThinComputeInterface::compute , this , changedvars ) );
  }

/**@} ----------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading results
 *
 * Since ThinComputeInterface is a very "abstract" interface, which is
 * independent from what is actually computed, there can hardly be methods for
 * reading the results. However, a common aspect from all compute() is that
 * they take time; also, most of them will be complex, iterative processes.
 * Thus, ThinComputeInterface offers two skeleton methods to read the elapsed
 * running time and iteration number from the last call to compute(). Among
 * other possible uses, these can be useful to event handlers (see
 * set_event_handler()).
 *  
 *  @{ */


/**@} ----------------------------------------------------------------------*/
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
 /// get the whole set of parameters in one blow
 /** This method gets the whole set of parameters in one blow using a
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
  * default value are. The f_diff field of the produced ComputeConfig is
  * set accordingly, i.e., f_diff == true <==> all == false. If for some
  * reason this is not the intended value it can easily be changed later.
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

/**@} ----------------------------------------------------------------------*/
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

 virtual ~ComputeConfig() { delete f_extra_Configuration; }

/*------------------------------- CLONE -----------------------------------*/
 /// clone method

 ComputeConfig * clone( void ) const override {
  return( new ComputeConfig( *this ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a ComputeConfig. Besides the mandatory "type" attribute of
  * any :Configuration, the group should contain the following:
  *
  * - the optional attribute "diff" of int type containing the value for the
  *   f_diff field of the ComputeConfig (basically, a bool telling if the
  *   information in it has to be taken as "the configuration to be set" or
  *   as "the changes to be made from the current configuration"); if the
  *   attribute is not there, f_diff == false is assumed.
  *
  * - the optional dimension "num_int_par" containing the number of int
  *   parameters; if the dimension is not provided, 0 is assumed;
  *
  * - the variable "int_par_names", of type string and indexed over the
  *   dimension "num_int_par"; the i-th entry of the variable is assumed to
  *   contain the string name of an int parameter (see int_par_idx2str());
  *   the variable is optional if num_int_par == 0 (e.g., it not provided),
  *   since in this case it is ignored;
  *
  * - the variable "int_par_vals", of type int and indexed over the
  *   dimension "num_int_par"; the i-th entry of the variable is assumed to
  *   contain the value of the int parameter whose string name is to be found
  *   in the i-th entry of "int_par_names"; the variable is optional if
  *   num_int_par == 0 (e.g., it not provided), since in this case it is
  *   ignored;
  *
  * - the opional dimension "num_dbl_par" containing the number of double
  *   parameters; if the dimension is not provided, 0 is assumed;
  *
  * - the variable "dbl_par_names", of type string and indexed over the
  *   dimension "num_dbl_par"; the i-th entry of the variable is assumed to
  *   contain the string name of a double parameter (see dbl_par_idx2str());
  *   the variable is optional if num_dbl_par == 0 (e.g., it not provided),
  *   since in this case it is ignored;
  *
  * - the variable "dbl_par_vals", of type double and indexed over the
  *   dimension "num_dbl_par"; the i-th entry of the variable is assumed to
  *   contain the value of the double parameter whose string name is to be
  *   found in the i-th entry of "dbl_par_names"; the variable is optional if
  *   num_dbl_par == 0 (e.g., it not provided), since in this case it is
  *   ignored;
  *
  * - the optional dimension "num_str_par" containing the number of string
  *   parameters; if the dimension is not provided, 0 is assumed;
  *
  * - the variable "str_par_names", of type string and indexed over the
  *   dimension "num_str_par"; the i-th entry of the variable is assumed to
  *   contain the string name of a string parameter (see int_par_idx2str());
  *   the variable is optional if num_str_par == 0 (e.g., it not provided),
  *   since in this case it is ignored;
  *
  * - the variable "str_par_vals", of type string and indexed over the
  *   dimension "num_str_par"; the i-th entry of the variable is assumed to
  *   contain the value of the string parameter whose string name is to be
  *   found in the i-th entry of "str_par_names"; the variable is optional if
  *   num_int_par == 0 (e.g., it not provided), since in this case it is
  *   ignored;
  *
  * - the group "extra" containing a Configuration object, which has no
  *   direct use in the base ComputeConfig class, but is added so that
  *   derived classes can put there any configuration information without
  *   having to define further derived classes form ComputeConfig (which,
  *   however, they can still do if they want); the group is optional, if it
  *   does not exist the corresponding Configuration * is set to nullptr. */

 void deserialize( netCDF::NcGroup & group ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a ComputeConfig. See
  * ComputeConfig::deserialize( netCDF::NcGroup ) for details of the format
  * of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// clears the vectors of parameters and the extra Configuration
 /** This method clears the vectors holding the integer, double, and string
  * parameters (#int_pars, #dbl_pars, and #str_pars). If
  * #f_extra_Configuration is not nullptr then Configuration::clear() is
  * invoked for #f_extra_Configuration. Moreover, #f_diff is set to false. */

 void clear( void ) override {
  int_pars.clear();
  dbl_pars.clear();
  str_pars.clear();

  f_diff = false;

  if( f_extra_Configuration )
   f_extra_Configuration->clear();
  }

/*--------------------- METHODS FOR CHANGING PARAMETERS --------------------*/
 /// set the given integer (int) numerical parameter
 /** Set the integer (int) numerical parameter specified by \p name. If the
  * parameter is not in the corresponding list it is added, otherwise its
  * current value is changed to \p value. */

 void set_par( std::string && name , int value ) {
  auto it = std::find_if( int_pars.begin() , int_pars.end() ,
		       [ & name ]( const std::pair< std::string , int > & el )
		                 { return( name == el.first ); } );

  if( it == int_pars.end() )
   int_pars.push_back( std::pair< std::string , int >( std::move( name ) ,
						       value ) );
  else
   it->second = value;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the given float (double) numerical parameter
 /** Set the float (double) numerical parameter specified by \p name. If the
  * parameter is not in the corresponding list it is added, otherwise its
  * current value is changed to \p value. */

 void set_par( std::string && name , double value ) {
  auto it = std::find_if( dbl_pars.begin() , dbl_pars.end() ,
		   [ & name ]( const std::pair< std::string , double > & el )
		             { return( name == el.first ); } );

  if( it == dbl_pars.end() )
   dbl_pars.push_back( std::pair< std::string , double >(
					       std::move( name ) , value ) );
  else
   it->second = value;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set the given string numerical parameter
 /** Set the numerical parameter specified by \p name. If the parameter is
  * not in the corresponding list it is added, otherwise its current value is
  * changed to \p value. */

 void set_par( std::string && name , std::string && value ) {
  auto it = std::find_if( str_pars.begin() , str_pars.end() ,
	      [ & name ]( const std::pair< std::string , std::string > & el )
		        { return( name == el.first ); } );

  if( it == str_pars.end() )
   str_pars.push_back( std::pair< std::string , std::string >(
				  std::move( name ) , std::move( value ) ) );
  else
   it->second = std::move( value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes a given parameter
 /** Seeks the parameter with given \p name in the list of integer, double or
  * string parameters as specified by \p type (with the values 'i', 'd' and
  * 's', respectively); if it is found it is removed from the list, otherwise
  * nothing is done. */

 void reset_par( const std::string & name , char type = 'i' ) {
  switch( type ) {
   case( 'i' ): {
    auto it = std::find_if( int_pars.begin() , int_pars.end() ,
		       [ & name ]( const std::pair< std::string , int > & el )
		                 { return( name == el.first ); } );
    if( it != int_pars.end() ) {
     *it = std::move( int_pars.back() );
     int_pars.pop_back();
     }
    }
   case( 'd' ): {
    auto it = std::find_if( dbl_pars.begin() , dbl_pars.end() ,
		    [ & name ]( const std::pair< std::string , double > & el )
		              { return( name == el.first ); } );
    if( it != dbl_pars.end() ) {
     *it = std::move( dbl_pars.back() );
     dbl_pars.pop_back();
     }
    }
   case( 's' ): {
    auto it = std::find_if( str_pars.begin() , str_pars.end() ,
	      [ & name ]( const std::pair< std::string , std::string > & el )
		        { return( name == el.first ); } );
    if( it != str_pars.end() ) {
     *it = std::move( str_pars.back() );
     str_pars.pop_back();
     }
    }
   }
  }  // end( reset_par )

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
 void print( std::ostream &output ) const override;

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
  * a string containing the class type of the extra Configuration object,
  * '*' means none (nullptr)
  *
  * if the above is not '*', the description of the :Configuration object
  */

 void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ComputeConfig ) )

/** @} end( group( ThinComputeInterface_CLASSES ) ) ------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ThinComputeInterface.h included */

/*--------------------------------------------------------------------------*/
/*------------------ End File ThinComputeInterface.h -----------------------*/
/*--------------------------------------------------------------------------*/






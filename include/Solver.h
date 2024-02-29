/*--------------------------------------------------------------------------*/
/*--------------------------- File Solver.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* Solver class, which represent the basic
 * interface between a Block [see Block.h], representing (possibly together
 * with its recursively nested sub-Blocks) a (block-structured mathematical)
 * model, and any algorithm capable of "solving" it, i.e., (hopefully)
 * producing feasible solutions, either "good" or provably (approximately)
 * optimal ones, or proving that there is none. Since doing this has to be
 * expected to costly, the class implements the ThinComputeInterface paradigm.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Solver
 #define __Solver  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Modification.h"

#include "ThinComputeInterface.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

class Block;                // forward definition of Block

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Solver ---------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// abstract base class for any solution algorithm for any Block
/** The *abstract* Solver class is meant to represent the basic interface
 * between a Block [see Block.h], representing (possibly together with its
 * recursively nested sub-Blocks) a (block-structured mathematical) model,
 * and any algorithm capable of "solving" it. This means (trying to) produce
 * feasible solutions, i.e., values for all the Variables [see Variable.h] of
 * the Block that satisfy all its Constraints [see Constraint.h], or proving
 * that there is none.
 *
 * It is important to clearly state the *ownership* of Constraint and Variable
 * [see Block.h], which is that they are directly defined either in the Block,
 * or in any of its inner Blocks (recursively). This has important
 * consequences, related to the fact that a Constraint defined in a Block may
 * contain Variables that the Block does not own. Conversely, some Variable of
 * the Block can be active in a Constraint that is not owned by the block. The
 * intended semantic of these cases are:
 *
 * - If a Constraint defined in a Block contains Variable that the Block
 *   does not own, when the Solver is asked to solve the Block these Variables
 *   have to be treated as *constants*, with the value that they currently
 *   have. Note that this is a specific case where the data of the
 *   corresponding mathematical problem changes, but there is no Modification
 *   that signals this to any Solver attached to the Block. This means that
 *   the Solver needing to know about this will have to "manually" check
 *   whether or not a change occurred.
 *
 * - A Constraint not owned by the Block is irrelevant to the Block even if
 *   it contains Variable owned by the Block; any Solver has to thoroughly
 *   ignore it. Paradoxically, such a Constraint may contain *only* Variable
 *   owned by the Block, and therefore logically belonging to it, but still
 *   it has to be ignored.
 *
 * - The Solver may change all, and only, the Variables owned by the Block
 *   when solving it, and has to take into account all, and only, the
 *   Constraint owned by the Block when defining what a feasible solution is.
 *
 * The Solver class is thought primarily (although not exclusively) for 
 * mathematical models in the form of an optimization problem; hence, the
 * solutions should be either generically "good" ones, or, better, provably
 * (approximately) optimal ones. This is provided an optimal solution exists
 * at all: if the mathematical model is unbounded (below for a minimization
 * problem, above for a maximization one), then the Solver should be able to
 * detect it and, hopefully, "prove" it.
 *
 * The base Solver class is very generic, and therefore it only supports a
 * number of very general notions:
 *
 * - a Solver is attached to a Block, from which it picks all the data
 *   characterizing the model and that it uses to output any solution it
 *   finds (in the Variable of the Block);
 *
 * - a Solver can (in fact, must) be specialized to work on a specific class
 *   of models, represented by one (or more) specific class(es) derived from
 *   Block;
 *
 * - a Solver can (in fact, must) react to changes in the data of the Block,
 *   of which it is made aware by appropriate Modification;
 *
 * - a Solver can (attempt at) solve the model, i.e., obtain at least one 
 *   feasible solution (to within the required tolerance for each Block,
 *   see Block::is_feasible()) that is also provably optimal to within the
 *   required accuracy, see SetPar() below; upon termination it will provide
 *   a full account about how the solution attempt went [see sol_type below];
 *
 * - a Solver may be able to provide multiple solutions to the problem for
 *   each call of solve() [see new_var_solution() and get_var_solution()
 *   below], be them feasible or unfeasible ones, in an appropriate order
 *   (from the "more interesting" to the "less interesting");
 *
 * - a Solver computes solutions, and this can only be expected to be costly
 *   (up to extremely costly), which is why the class implements the
 *   ThinComputeInterface paradigm (i.e., derives from
 *   ThinComputeInterface).
 *
 * Despite tying to be quite general, the class is clearly somewhat bent
 * towards *single-objective, real-valued* optimization problems, as shown by
 * extensive support towards the concept of "objective function value" and
 * (upper and lower) bounds upon that. Hence, although not strictly necessary,
 * it is somewhat intended that the Objective of the Block actually is a
 * RealObjective. Thus, one would expect that the OFValue type [see below]
 * is the same as that defined in RealObjective. Steps are taken so that
 * problems without an objective function should not find this choice "too
 * intrusive"; basically, every method having to do with the (real) objective
 * function value is given here a safe default implementation so that it can
 * be basically ignored by classes not having that concept (say, because they
 * have no objective function) at all. For Block with more complex,
 * non-real-valued objective functions, appropriate derived classes will have
 * to be defined that extend the "simple" treatment of real-valued objectives
 * to the necessarily more complex concepts. */

class Solver : public ThinComputeInterface
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

 /// public enum for the possible return values of compute()
 /** Public enum "extending" ThinComputeInterface::compute_type with more
  * detailed values specific to Solver. Also, a few comments about the
  * specific interpretation in Solver of the values there is in order:
  *
  * - kOK has to be taken to specifically mean that the Solver obtained at
  *   least one feasible solution (to within the required tolerance for each
  *   Block, see Block::set_BlockConfiguration()) that is also provably
  *   optimal to within the required accuracy; in other words, it stands for
  *   "optimal solution found". Note that this usually means that Solver has
  *   some "certificate of optimality", which may be of extreme interest for
  *   the user. However, the actual form of the certificate (a dual optimal
  *   solution ...) is highly dependent on the nature of the problem solved,
  *   and therefore it is not possible to provide a specific method for that
  *   in the general interface (not to mention that certificates of optimality
  *   may be exponentially large). Yet, this is not the only "success"
  *   value, cf. kUnbounded and kInfeasible.
  *
  * - kError is a general value for "something has gone very wrong", which
  *   may mean that no solution at all may have been produced; however, what
  *   it surely means is that there is little or no hope that calling
  *   compute() again may rectify the problem. Yet, it should probably be
  *   never used "directly", as each specific :Solver should be able to
  *   provide more information about the kind of error it is experiencing,
  *   e.g. by "extending the set of return codes" after kError. Indeed, a
  *   specific error, kLowPrecision, is already provided for the case where
  *   the issue is not numerical, but inherent in what the solver is
  *   (heuristic, hence explicitly incapable of solving every instance to
  *   optimality. */

 enum sol_type {
  kUnbounded = kUnEval + 1 ,  ///< the model is provably unbounded
                              /**< Means that the problem does not have any
                               * optimal solution, i.e., that for each real
  * value M (large enough or small enough, respectively if the problem is a
  * maximization or minimization one) it is possible to produce a feasible
  * solution with that value. The Solver is able to *prove* that this is true,
  * which means it should be able to explicitly produce solutions with any
  * chosen (large or small enough) value, see set_unbounded_threshold(). This
  * is by no means a "failure" to find an optimal solution, but rather one of
  * the "good ways" in which compute() can end. Also, note that this usually
  * means that Solver has some "certificate of infeasibility", which may be of
  * extreme interest for the user. However, the actual form of the certificate
  * (a ray of the polyhedron, a directed cycle of negative weight, ...) is
  * highly dependent on the nature of the problem solved, and therefore it is
  * not possible to provide a specific method for that in the general
  * interface. */

  kInfeasible ,   ///< the model is provably infeasible
                  /**< Means that the problem does not have any optimal
                   * solution because it does not have any solution at all,
  * and the Solver is able to *prove* that this is true. Note that this means
  * that the Solver has some "certificate of infeasibility", which may be of
  * extreme interest for the user. However, the actual form of the certificate
  * (a ray of the dual polyhedron, a cut showing that no feasible flow can
  * exist, ...) is highly dependent on the nature of the problem solved, and
  * therefore it is not possible to provide a specific method for that in the
  * general interface. */

  kStopTime = kOK + 1 ,  ///< stopped because of time limit
                         /**< The Solver didn't manage to either obtain any
                          * feasible solution (to within the required
  * tolerance for each Block) and/or to reach the required accuracy, not
  * because it is not in principle capable of doing so (see kLowPrecision) or
  * for some kind of error (see kError), but because the maximum running time
  * it is allowed to operate [see SetPar() and the corresponding dblMaxTime
  * value] is elapsed. Note that calling compute() again resets the time
  * counter, allowing the Solver to proceed further in the solution process.
  * An *exact* Solver should always eventually return one among kOK,
  * kUnbounded and kInfeasible given enough time (although "enough" can be
  * longer than the thermal death of the universe), unless an error occurs,
  * but not all problems are even decidable and therefore allow an exact
  * Solver. Besides, non-exact Solver also make full sense in many cases. */

  kStopIter ,  ///< stopped because of iteration limit
               /**< The Solver didn't manage to either obtain any feasible
                * solution (to within the required tolerance for each
  * Block) and/or to reach the required accuracy, not because it is not in
  * principle capable of doing so (see kLowPrecision) or for some kind of
  * error (see kError), but because the maximum "number of iterations" it is
  * allowed to execute [see SetPar() and the corresponding intMaxIter value]
  * has been expended already. The concept of "what exactly an iteration is"
  * is clearly solver-dependent, and the user of the Solver need supposedly be
  * aware of which concrete :Solver it is actually using to be able to
  * sensibly set this parameter. Note that calling compute() again resets the
  * iterations counter, allowing the Solver to proceed further in the solution
  * process. An *exact* solver will always eventually return kOK / kUnbounded
  * / kInfeasible (although "eventually" may be after the thermal death of the
  * universe) given enough resources, unless an error occurs, but not all
  * problems are decidable and therefore allow an exact Solver. */

  kBlockLocked = kError + 1 ,  ///< could not acquire the lock on the Block
                  /**< compute() needed to lock the Block to work, but
                   * acquiring the lock was unsuccessful and the Solver does
  * not have in place any mechanism to overcome this issue. */

  kLowPrecision ,  ///< a solution found but not provably optimal
                   /**< The Solver was indeed able to obtain 
                    * feasible solution (to within the required tolerance for
  * each Block) but *not* to reach the required accuracy, and this not for an
  * issue relative to limits on the available resources (see kStopTime and
  * kStopIter) or for some kind of error (see kError), but because the Solver
  * is an intrinsically heuristic approach which cannot guarantee to always be
  * able to solve the model. Heuristic Solver are still very useful because
  * they can be "fast", and some kind of problems may not allow for any exact
  * solver anyway. The specific information that kLowPrecision provides over
  * kError, besides the fact that the error is not due to "strange"
  * occurrences but to the nature of the Solver, is that at least one solution
  * has been found. */

  kLastSolverError  ///< first allowed new error code for derived classes
                    /**< Convenience value for easily allow derived classes
  * to extend the set of error codes. */

  };  // end( sol_type )

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters of Solver
 /** Public enum "extending" int_par_type_TCI to describe the different
  * algorithmic parameters of "int" type that any Solver should reasonably
  * have on top of those defined by ThinComputeInterface. The value
  * intLastAlgPar is provided so that the list can be easily extended by
  * derived classes.
  *
  * Note: WHILE THE BASE Solver CLASS DEFINES THESE VALUES, IT DOES NOT
  *       HANDLE THEM SAVE FOR PROVIDING THE DEFAULT VALUES.
  *
  * That is, Solver does *not* re-define set_par( int ) and properly read
  * these parameters into fields of the class. This is entirely demanded to
  * derived classes. What Solver does, however (unlike ThinComputeInterface),
  * is to properly define get_dflt_int_par() so that the parameters have their
  * stated default value. */

 enum int_par_type_S {
  intMaxSol = intLastAlgParTCI ,
                ///< maximum number of different solutions to report
                /**< The algorithmic parameter for setting the maximum 
                 * number of different solutions to the Block that the
  * Solver should attempt to obtain and store. Mathematical models can have
  * (very) many solutions: an objective function precisely helps in selecting
  * among them, but even that may not be enough to narrow the choice down to
  * a single solution (multiple optima may exist, or quasi-optimal solutions
  * may also be sought for for any number of reasons). On the other hand, a
  * solution can be a "big" object, and storing it may be costly. It may be
  * therefore helpful for a Solver to know beforehand how many different
  * solutions the user would like to get. Among the reasonable values for
  * this parameter, 1 says "I don't care of multiple solutions, give me only
  * the best one", and 0 says "I don't care of solutions at all, just tell
  * me if there is any, and what its value is". The default is 1. */

  intLogVerb ,  ///< "verbosity" of the log
                /**< An integer parameter dictating how "verbose" the log of
                 * the Solver [see set_log()] has to be. The specific meaning
  * of each value is Solver-dependent, but it is intended that 0 means "no log
  * at all", and increasing values correspond to increasing verbosity. The
  * default value is 0 (no log). */

  intLastAlgPar   ///< first allowed new int parameter for derived classes
                  /**< Convenience value for easily allow derived classes to
		   * extend the set of int algorithmic parameters. */
  };  // end( int_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters of Solver
 /** Public enum "extending" dbl_par_type_TCI to describe the different
  * algorithmic parameters of "double" type that any Solver should reasonably
  * have on top of those defined by ThinComputeInterface. The value
  * dblLastAlgPar is provided so that the list can be easily extended by
  * derived classes.
  *
  * Note: WHILE THE BASE Solver CLASS DEFINES THESE VALUES, IT DOES NOT
  *       HANDLE THEM SAVE FOR PROVIDING THE DEFAULT VALUES.
  *
  * That is, Solver does *not* re-define set_par( double ) and properly read
  * these parameters into fields of the class. This is entirely demanded to
  * derived classes. What Solver does, however, is to properly define
  * get_dflt_dbl_par() so that the parameters have their stated default
  * value. */

 enum dbl_par_type_S {
  dblRelAcc = dblLastAlgParTCI ,
                 ///< relative accuracy for declaring a solution optimal
                 /**< The algorithmic parameter for setting the *relative*
  * accuracy required to the solution of the Block. This is
  * geared towards single-objective optimization problems, and it is defined
  * as follows: a solution for a *minimization* problem has a relative
  * accuracy of \eps if a feasible (to within the required tolerance for each
  * Block) solution has been obtained, an upper bound "ub" on its objective
  * function value has been found [see get_ub()], a lower bound "lb" on the
  * *optimal* value of the problem has been found [see get_lb()], and
  *
  *    ub - lb <= \eps * max( abs( lb ) , 1 )
  *
  * The roles of ub and lb are suitably reversed for a maximization problem
  * [see the comments to get_ub(), get_lb() and Objective::set_sense()]. The
  * default is 1e-6. */

  dblAbsAcc ,    ///< absolute accuracy for declaring a solution optimal
                 /**< The algorithmic parameter for setting the *absolute*
                  * accuracy required to the solution of the model. This is
  * geared towards single-objective optimization problems, and it is defined
  * as follows: a solution for a *minimization* problem has a absolute
  * accuracy of \eps if a feasible  (to within the required tolerance for
  * each Block) solution has been obtained, an upper bound "ub" on its
  * objective function value has been found [see get_ub()], a lower bound
  * "lb" on the *optimal* value of the problem has been found [see get_lb()],
  * and
  *
  *    ub - lb <= \eps
  *
  * The roles of ub and lb are suitably reversed for a maximization problem
  * [see the comments to get_ub(), get_lb() and Objective::set_sense()]. The
  * default is Inf< OFValue >(), which is intended to mean that the only working
  * accuracy is the relative one. */

  dblUpCutOff ,  ///< upper cutoff for stopping the algorithm
                 /**< The algorithmic parameter for setting the "upper cut 
                  * off" of the solution process. This is a value basically
  * telling the Solver "when enough is enough". In particular, if the Solver
  * obtains a certified lower bound "lb" on the optimal value such that
  *
  *   lb >= \eps
  *
  * with the provided parameter \eps, then it can stop. This is a
  * *certificate* that the optimal value is at *least* \eps. For a
  * maximization problem the condition says: I actually needed a solution
  * with objective function value at least as good as (i.e., larger than)
  * \eps, now that I have found one the problem is as good as solved to me.
  * Hence this parameter is analogous to the maximum absolute accuracy (see
  * dblAbsAcc), but "weaker" in that it does not not require any upper bound
  * to work. For a minimization problem, instead, the condition says: I
  * actually needed a solution with objective function value at least as
  * good as (i.e., smaller than) \eps, now that I have know for sure that
  * this is never going to happen the problem is as good as unfeasible to me.
  * The default is Inf< OFValue >(). */

  dblLwCutOff ,  ///< lower cutoff for stopping the algorithm
                 /**< The algorithmic parameter for setting the "lower cut 
                  * off" of the solution process. This is a value basically
  * telling the Solver "when enough is enough". In particular, if the Solver
  * obtains a certified upper bound "ub" on the optimal value such that
  *
  *   ub <= \eps
  *
  * with the provided parameter \eps, then it can stop. This is a
  * *certificate* that the optimal value is at *most* \eps. For a
  * minimization problem the condition says: I actually needed a solution
  * with objective function value at least as good as (i.e., smaller than)
  * \eps, now that I have found one the problem is as good as solved to me.
  * Hence this parameter is analogous to the maximum absolute accuracy (see
  * dblAbsAcc), but "weaker" in that it does not not require any lower bound
  * to work. For a maximization problem, instead, the condition says: I
  * actually needed a solution with objective function value at least as
  * good as (i.e., larger than) \eps, now that I have know for sure that
  * this is never going to happen the problem is as good as unfeasible to me.
  * The default is - Inf< OFValue >(). */

  dblRAccSol ,   ///< maximum relative error in any reported solution
                 /**< The algorithmic parameter for setting the relative
                  * accuracy of the accepted solutions. It instructs the
  * Solver not to even consider a solution among the ones to be reported (see
  * intMaxSol) if its objective function value is "too" bad. For a
  * minimization problem, the objective function value of a feasible solution
  * provides an upper bound "ub" on the optimal value. Assuming a lower bound
  * "lb" on the *optimal* value of the problem has been found [see get_lb()],
  * a solution is deemed acceptable with the provided parameter \eps if
  *
  *    ub - lb <= \eps * max( abs( ub ) , abs( lb ) , 1 )
  *
  * Note that if no lb is available, the above formula can be replaced with
  *
  *    ub - fbest <= \eps * max( abs( ub ) , abs( fbest ) , 1 )
  *
  * where fbest is the value of the best (with smallest objective value)
  * solution found so far. The roles of ub and lb are suitably reversed for
  * a maximization problem. The default is Inf< OFValue >(). */

  dblAAccSol ,   ///< maximum absolute error in any reported solution
                 /**< Similar to dblRAccSol but for an *absolute* accuracy;
                  * that is, a solution is deemed acceptable with the
  * provided parameter \eps if
  *
  *    ub - lb <= \eps
  *
  * or
  *
  *    ub - fbest <= \eps
  *
  * with the same notation as in dblRAccSol and the same provisions about the
  * case of a maximization problem. The default is Inf< OFValue >(). */

  dblFAccSol ,   ///< maximum constraint violation in any reported solution
                 /**< The algorithmic parameter for setting the maximum
                  * relative allowed violation of constraints. Whenever the
  * Solver is incapable of finding feasible solutions (maybe because there is
  * none), it may still be useful that it returns the "least unfeasible" ones.
  * This parameter instructs the Solver not to even consider a solution among
  * the ones to be reported (see intMaxSol) if its violation is "too" bad.
  * The actual meaning of this parameter may be Solver-dependent, but it can
  * be thought to work as the "relative constraint violation". A setting of 0
  * may be taken as a way to tell the Solver not to bother to produce
  * unfeasible solutions at all, which is why this is the default value of
  * the parameter. */

  dblLastAlgPar   ///< first allowed new double parameter for derived classes
                  /**< Convenience value for easily allow derived classes to
		   * extend the set of double algorithmic parameters. */
  };  // end( dbl_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for the string algorithmic parameters
 /** Public enum describing the different algorithmic parameters of "string"
  * type that any Solver should reasonably have (none so far). The value
  * strLastAlgPar is provided so that the list can be easily extended by
  * derived classes. */

 enum str_par_type_S {
  strLastAlgPar = strLastAlgParTCI  ///< first allowed new string parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of string algorithmic parameters. */
  };  // end( str_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-int algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of int that any Solver should reasonably have (none so far). The
  * value vintLastAlgPar is provided so that the list can be easily extended
  * by derived classes. */

 enum vint_par_type_S {
  vintLastAlgPar = vintLastAlgParTCI
  ///< first allowed new vector-of-int parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int parameters. */
  };  // end( vint_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-double algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of double that any Solver should reasonably have (none so far).
  * The value vdblLastAlgPar is provided so that the list can be easily
  * extended by derived classes. */

 enum vdbl_par_type_S {
  vdblLastAlgPar = vdblLastAlgParTCI
  ///< first allowed new vector-of-double parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double parameters. */
  };  // end( vdbl_par_type_S )

/*--------------------------------------------------------------------------*/
 /// public enum for vector-of-string algorithmic parameters
 /** Public enum describing the different algorithmic parameters that are
  * vectors of string that any Solver should reasonably have (none so far).
  * The value vdblLastAlgPar is provided so that the list can be easily
  * extended by derived classes. */

 enum vstr_par_type_S {
  vstrLastAlgPar = vstrLastAlgParTCI
  ///< first allowed new vector-of-string parameter
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string parameters. */
  };  // end( vstr_par_type_S )

/*--------------------------------------------------------------------------*/

 using OFValue = double;  ///< type of the objective function value
                          /**< This is the type of the value of the objective
			   * function, generically "a real". One may expect
 * this to be defined exactly like the same-named type in RealObjective. */

/** @} ---------------------------------------------------------------------*/
/*------------------ CONSTRUCTING AND DESTRUCTING Solver -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Solver
 *  @{ */

 /// constructor: initialise the few data structures of the base class
 Solver( void ) : f_Block( nullptr ) , f_log( nullptr ) , f_no_Mod( false ) {
  f_id = this;
  f_mod_lock.clear();
  }

/*--------------------------------------------------------------------------*/
 /// construct a :Solver of specific type using the Solver factory
 /** Use the Solver factory to construct a :Solver object of type specified
  * by classname (a std::string with the name of the class inside). Note that
  * the method is static because the factory is static, hence it is to be
  * called as
  *
  *   Solver *mySolver = Solver::new_Solver( some_class );
  *
  * i.e., without any reference to any specific Solver (and, therefore, it can
  * be used to construct the very first Solver if needed).
  * 
  * For this to work, each :Solver has to:
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_h;
  *
  *   to its definition (typically, in the private part in its .h file);
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( name_of_the_class );
  *
  *   to exactly *one* .cpp file, typically that :Solver .cpp file. If the
  *   name of the class contains any parentheses, then one must enclose the
  *   name of the class in parentheses and instead add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( ( name_of_the_class ) );
  *
  * Any whitespaces that the given \p classname may contain is ignored. So,
  * for example, to create an instance of the class MySolver< int > one could
  * pass "MySolver< int >" or "MySolver< int >" (even " M y S o l v e r < int >
  * " would work).
  *
  * @param classname The name of the :Solver class that must be
  *        constructed. */

 static Solver * new_Solver( const std::string & classname ) {
  const std::string classname_( SMSpp_classname_normalise(
					        std::string( classname ) ) );
  const auto it = Solver::f_factory().find( classname_ );
  if( it == Solver::f_factory().end() )
   throw( std::invalid_argument( classname +
				 " not present in Solver factory" ) );
  return( ( it->second )() );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it has to release all the Modifications
 /** The destructor of the Solver class has to delete all the Modifications
  * that the Solver has received so far and has not processed yet. Because
  * these are shared pointers, the actual Modification objects will be
  * deleted as soon as the last Solver having received them processes
  * (and then immediately deletes) them, or is destroyed. However, this is
  * automatic: v_mod is a list which gets destroyed when the Solver is,
  * and in so doing it also destroys all its elements (you don't explicitly
  * delete a std::shared_ptr<>, you just destroy the object). */

 ~Solver() override = default;

/** @} ---------------------------------------------------------------------*/
/*---------------------- LOCKING AND UNLOCKING Solver ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Locking and Unlocking Solver
 *
 * Solver are typically going to be "complex" objects, with a significant
 * internal state that has to be protected in a multi-threaded environment.
 * For this reason, the base Solver includes a std::recursive_mutex as a
 * protected field, and exposes methods to lock and unlock it:
 *
 *     EACH TIME THAT A NON-CONST METHOD OF Solver IS CALLED, THE Solver
 *     SHOULD FIRST BE LOCKED, AS THIS IS NOT SUPPOSED TO BE DONE
 *     AUTOMATICALLY INSIDE THE METHODS THEMSELVES, UNLESS ONE CAN BE
 *     SURE THAT NO OTHER THREAD CAN POSSIBLY OPERATE ON THE Solver
 *
 * The rationale for this choice is:
 *
 * - it dramatically simplifies the implementation of :Solver, especially in
 *   the case where a method of a derived class must call that of the base
 *   class;
 *
 * - since taking and releasing the lock are "costly" operations, it allows
 *   to bunch a block of them under the same lock/unlock stretch to
 *   improve performances;
 *
 * - there can be cases when one can be 100% sure that no other thread/entity
 *   can possibly operate on the Solver, either because the overall
 *   application is rigidly single-threaded, or because the Solver can only
 *   be accessed via a rigidly controlled access point (for instance, the
 *   Solver has just been created out of a factory and there currently is
 *   only one pointer to it that no other thread can possibly be sharing
 *   because it is, say, a local variable in a function).
 *
 * Of course
 *
 *     EACH TIME THE Solver IS LOCKED IT HAS TO BE UNLOCKED, WHICH MEANS
 *     THAT ALL LOCK/UNLOCK STRETCHES THAT MAY THROW EXCEPTION SHOULD
 *     BE ENCLOSED INSIDE A try - catch BLOCK, UNLESS THE EXCEPTIONS ARE
 *     ANYWAY GOING TO BE TERMINAL ONES
 *
 * The mutex is recursive so as to allow the same thread to repeatedly
 * obtaining the lock. This has a performance cost, but it might be
 * necessary because the usage patterns of Solver may be complex, and there
 * may be the chance that operations on a Solver may be performed by two
 * pieces of code that are unaware of each other.
 *
 * Note that this means that acquiring the lock does not imply that the
 * Solver may not be "in use" by a different piece of code in the same thread.
 * This is typically not a big issue for "simple" methods, such as those
 * setting parameters (unless of course the different pieces do things that
 * cancel out each other), that surely complete before giving back control.
 * However, it can be an issue in particular for compute(), because the
 * control can be "arbitrarily taken away" via event handlers (see the
 * comments around set_event_handler()), inside which everything can happen.
 * Thus, compute() in particular will have to protect itself against multiple
 * invocations within the same thread.
 *  @{ */

 /// lock the Solver, waiting if the lock is taken by another thread

 void lock( void ) { f_mutex.lock(); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// try to lock the Solver, returns false if the lock is taken

 bool try_lock( void ) { return( f_mutex.try_lock() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// unlock the Solver

 void unlock( void ) { f_mutex.unlock(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the (pointer to the) Block that the Solver has to solve
 /** Method to set the (pointer to the) Block that the Solver has to solve.
  * If there were any other block attached to Block this Solver, any
  * information about solutions to the previous Block the Solver had
  * computed is lost for good. This is one reason why the method is virtual:
  * derived classes may need to do more to reach to such an abrupt change.
  * In general, a :Solver may have to (lock and) read (data from) the Block
  * to be stored in its internal data structures, hence this method is
  * likely to be a rather costly one.
  *
  * Passing \p block == nullptr signals to the Solver to discard every
  * information related to the solution process of the previous Block (if
  * any), and sit down quietly in a corner waiting for new orders.
  *
  * IMPORTANT NOTE: the moment when the Block is passed to the Solver, the
  * Solver should in principle do all the necessary initializations, since
  * immediately afterwords compute() may be called already. However, some of
  * the initializations could be heavily impacted by the algorithmic
  * parameters of the Solver. This means that
  *
  *     IT IS EXPECTED THAT set_ComputeConfig() SHOULD BE CALLED *BEFORE*
  *     set_Block() IS
  *
  * so that when set_Block() is finally called, the Solver fully knows how to
  * initialize itself.
  *
  * IMPORTANT NOTE: set_Block( block ) is *not* expected call
  *
  *      block->register_Solver( this )
  *
  * to register the Solver to the Block, and likewise set_Block( nullptr )
  * is *not* expected to call
  *
  *      f_Block->unregister_Solver( this )
  *
  * to unregister the Solver from the previous Block (if any). This is
  * because this method is called when the Solver is un/registered to the
  * Block; see Block::register_Solver(), Block::unregister_Solver() and
  * Block::replace_Solver(). Hence, the preferred way to set/rescind the ties
  * between a Block and a Solver is to do that via the Block; if set_Block()
  * is called before that, the methods of the Block will still have to be
  * called to make the Block aware of the changes. However, this means that
  *
  *     Solver::set_Block( block ) COULD BE CALLED TWICE FOR THE SAME block
  *     (IF IT IS CALLED FIRST, AND THEN Block::register_Solver() IS CALLED
  *     TO MAKE block AWARE OF THIS), WHICH MEANS THAT ANY :Solver SHOULD
  *     CHECK IF THE block BEING SET IS THE SAME ONE ALREADY SET AND DO
  *     NOTHING IN CASE (which is easy, cheap and very reasonable). */

 virtual void set_Block( Block * block );

/*--------------------------------------------------------------------------*/
 /// set the ostream for the Solver log
 /** Set the (pointer to) the ostream upon which the Solver is supposed to
  * log all its algorithmic developments. Passing nullptr instructs the 
  * Solver to stay mum. The "verbosity" of such log is specified by
  * intLogVerb [see set_par( int )], and setting intLogVerb = 0 should be
  * equivalent to setting f_log = nullptr.
  *
  * The method is virtual because derived classes may need to do something to
  * react to changes of the output log other than storing the new log
  * stream. */

 virtual void set_log( std::ostream * log_stream = nullptr ) {
  f_log = log_stream;
  }

/** @} ---------------------------------------------------------------------*/
/*----------------- METHODS FOR MANAGING THE "IDENTITY" --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Managing the "identity" of a Solver
 *
 * Actually implement the methods of ThinComputeInterface relative to
 * temporarily changing the "identity" of the Solver.
 *
 *  @{ */

 /// set the "identity" of the Solver
 /** Actually implement the ThinComputeInterface::set_id() by setting the
  * f_id member of the Solver class , which is initialized with "this" and
  * returned by id(). Each :Solver should always use id() to try to "lock
  * and own" the Block. This method allows to change the id() ("lend another
  * identity") of the Solver to any value; when called with nullptr argument,
  * the id() is reset to the default "this". Note that a Solver should always
  * lend its id() to its sub-Solver, if any, so that "lending can be
  * propagated downwards" along a chain of sub-Solver. */

 void set_id( void * id = nullptr ) override { f_id = id ? id : this; }

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR EVENTS HANDLING -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Set event handlers
 *
 * Unlike the base ThinComputeInterface, Solver has a working implementation
 * of the set_event_handler() and reset_event_handler() methods, based on a
 * minimal set of fields of the class. Although this may be redundant for a
 * Solver that want to implement the mechanism in a different way, the
 * overhead should be small enough so that the convenience of possibly
 * requiring (almost) no code from derived classes to handle the mechanism
 * should justify it.
 *
 * All that a derived class need to do is:
 *
 * - to implement the method max_event_number();
 *
 * - to properly resize the v_events vector in the constructor, which is the
 *   one-liner
 *
 *         v_events.resize( max_event_number() );
 *
 *   (note that it would be nice to have this done automatically, say in the
 *   constructor of the base Solver class, but this cannot be done because
 *   "virtual methods do not work in the constructor": during construction of
 *   the base class, called by the constructor of a derived one, the base
 *   class version of any method is called).
 *
 * - of course
 *
 *       ACTUALLY PROPERLY HANDLING THE EVENTS INSIDE compute()
 *
 *   which just requires something like
 *
 *       for( auto ev : v_events[ type ] )
 *        switch( ev() ) {
 *         case( eContinue ): break; 
 *         case(    ...    ): < properly perform the requested action >
 *         ...
 *         }
 *
 *   in the appropriate places of compute(), depending on type.
 *
 * Note that max_event_number() by default returns 0, which implies that the
 * :Solver does not support *any* event, and therefore that there is no need
 * to resize v_events. */

 /// returns the maximum number of event types supported by the :Solver
 /** Returns the maximum number of event types supported by the :Solver,
  * which means that type in set_event_handler() and reset_event_handler()
  * can only go from 0 to max_event_number() - 1. The method of the base
  * class returns 0, which implies that the Solver does not support *any*
  * event. By returning anything != 0, a :Solver can have the setting and
  * resetting of event handlers automatically handled by the base class
  * implementation. */

 [[nodiscard]] virtual EventID max_event_number( void ) const { return( 0 ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// register a new event handler, returning its id
 /** The new event handler is added at the back of v_events[ type ]. As the
  * && tells, the event handler becomes property of the ThinComputeInterface,
  * which is completely OK if, as one expects, it is defined via a lambda
  * function. The method returns a unique id for the handler, which can (and
  * must) be later used to remove the handler before it becomes invalid. Note
  * that the handler is type-specific, i.e., two event handlers of different
  * types can have the same id; in other words, the "real" id is the pair
  * ( type , id ). An exception is thrown if the ThinComputeInterface is not
  * capable of handling this type or event for whatever reason, among which
  * that it has exhausted the available maximum number of event handlers
  * slots for the given type. */

 EventID set_event_handler( int type , EventHandler && event ) override {
  if( type >= max_event_number() )
   throw( std::invalid_argument( "unsupported event type " +
				 std::to_string( type ) ) );

  if( v_events[ type ].size() > std::numeric_limits< EventID >::max() )
   throw( std::invalid_argument( "too many event handlers for type" +
				 std::to_string( type ) ) );

  EventID id = v_events[ type ].size();
  v_events[ type ].push_back( std::move( event ) );

  return( id );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// unregister an existing event handler
 /** Removes the event handler with the given id from the list of those
  * registered for the given type. If there is no event handler with the
  * given id for the given type, exception will be thrown. */

 void reset_event_handler( int type , EventID id ) override;

/** @} ---------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE Block ----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solving the model encoded by the current Block
 *  @{ */

 /// (try to) solve the model encoded in the Block
 /** Starts the solution process of the problem, or restarts a previously
  * interrupted solution process, and returns the status of the solver at
  * termination.
  *
  * If everything runs smoothly then the function is expected to return kOK
  * which means that the optimization process was finished successfully, i.e.,
  * a solution was found that can be *proven* feasible and optimal w.r.t. the
  * prescribed accuracy levels. However, the return codes kInfeasible and
  * kUnbounded, corresponding respectively to the case that the Solver
  * *proves* the problem to be infeasible and unbounded, are also "kOK-type"
  * return codes. Different Solver may be able to provide some further
  * information about certificates of optimality, unboundedness or
  * infeasibility, and more functionalities can be expected to exist in the
  * derived classes about this.
  *
  * If compute() returned kStopTime or kStopIter, then calling it again resets
  * the time counter, allowing the Solver to proceed further in the
  * optimization process. An *exact* Solver will always eventually return
  * kOK given enough time (although "enough" can be longer than the thermal
  * death of the universe), unless an error occurs (see kError). However, not
  * all Solvers are exact, which is what the return status kLowPrecision is
  * for: to indicate that, limits to computational resources notwithstanding,
  * the Solver has exhausted all its means to try to look for a (better)
  * solution, so calling again compute() will not help.
  *
  * In case compute() returns anything >= kError, then this means that the
  * Solver was forced to stop due to some error, e.g. of numerical nature or
  * because of lack of some crucial resource (say, memory). This may mean
  * that no solution at all may have been produced, but some may instead be
  * available, cf. kLowPrecision; in fact, even the *optimal* solution could
  * be available, but *with no certificate of optimality*. However, what a
  * return code surely means is that there is little or no hope that calling
  * compute() again may rectify the problem.
  *
  * In general, calling compute() again after that it has reported kOK,
  * kInfeasible, kUnbounded, kLowPrecision, or kError is possible, but it
  * should not be expected to obtain a different result unless:
  *
  * - some Modification has occurred in the meantime to the data of the
  *   Block, i.e., add_Modification() has been called at least once;
  *
  * - some of the Variable that the problem depends onto (if any), i.e.,
  *   Variable not belonging to the Block but still present in some of its
  *   Constraint/Objective, have changed.
  *
  * While the first occurrence is easy for the Solver to verify, the second is
  * not unless the previous value of the Variable is stored and checked; this
  * is not required to happen, see the extensive comments to compute() in the
  * base ThinComputeInterface class.
  *
  * As a partial exception to this rule, if a Solver has been asked to
  * provide "all" its stored solutions [see kMaxSol, kRAccSol, kAAccSol and
  * kFAccSol above and new_var_solution() below], be them feasible or not, a
  * new call to compute() after a return value of kOK is interpreted as
  * "please do provide more solutions". The return value of compute() should
  * be the same (unless, for instance, a kError occurs), but the list of
  * available solutions might be replenished.
  *
  * Note that, being Solver usually complex objects with a significant state,
  * two simultaneous call to compute() of the same Solver must not happen.
  * The lock() / unlock() methods are provided precisely to ensure this in a
  * multi-threaded environment: the Solver should be lock()-ed before
  * compute() is called, and unlock()-ed only after termination. However,
  * note that
  *
  *     THE MUTEX DOES NOT PROTECT A Solver AGAINST MULTIPLE INVOCATIONS
  *     OF compute() FROM WITHIN THE SAME THREAD.
  *
  * These may conceivably happen, since the control can be "arbitrarily
  * taken away" from compute() via event handlers (see the comments around
  * set_event_handler()), inside which everything can happen. Thus,
  * implementations of compute() will have to protect themselves against
  * multiple invocations within the same thread. This is easy enough if,
  * say, a field "f_state" is used to keep the state to be returned as in
  * the following fragment of code
  *
  *     int compute( bool changedvars = true ) override {
  *      if( f_state == kStillRunning )
  *       return( kError );
  *      f_state = kStillRunning;
  *      < do the rest of compute >
  *      < set f_state to the right return code >
  *      return( f_state );
  *      }
  *
  * which is precisely why the kStillRunning value has been defined. */

 int compute( bool changedvars = true ) override = 0;

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Accessing the found solutions (if any)
 * The methods in this section allow to retrieve solution information
 * generated (at least, conceptually) during the last call to compute().
 * Calls to to any of these methods are therefore associated with the state
 * of Block (comprised the value of the Variable not belonging to it but
 * appearing in its Constraint, see the discussion about compute()) at the
 * moment in which compute() was last called; hence, it is expected that the
 * state is the same. In other words, these methods are "extensions" of
 * compute(), used to extract further information (likely) computed there,
 * and therefore in principle an extension to the fundamental rule regarding
 * compute() is to be enforced:
 *
 *   between a call to compute() and all the calls to any of these methods
 *   intended to retrieve information about solutions computed in that
 *   moment, no changes must occur to the Block which change the answer
 *   that compute() was supposed to compute
 *
 * The rule leaves scope for some changes occurring, although these must
 * ensure that the answer is not affected; see comments to compute(). As
 * always,
 *
 *               IT IS UNIQUELY THE CALLER'S RESPONSIBILITY
 *                 TO ENSURE THAT THE RULE IS RESPECTED
 *  @{ */

 /// return a valid lower bound on the optimal objective function value
 /** Returns a valid lower bound on the optimal objective function value.
  * The meaning of this information clearly depends on whether the problem is
  * a minimization or a maximization one, and on the status of the solution
  * process.
  *
  * For a minimization problem, get_lb() returns:
  *
  * - - Inf< OFValue > if compute() returned kUnbounded.
  *
  * - Inf< OFValue > if compute() returned kInfeasible.
  *
  * - A finite value if compute() returned kOK. Since the lower bound for a
  *   minimization problem is not attached to a solution in terms of the
  *   Variable of the Block, the lower bound is not assumed to change if
  *   get_var_solution() [see below] is called again. In several cases the
  *   lower bound is attached to a *dual* solution, but this concept is not
  *   general enough to be supported in the base class [see CDASolver.h].
  *
  * - any value, comprised - Inf< OFValue > (meaning that no lower bound is
  *   currently available), if compute() returned any other value.
  *
  * For a maximization problem, get_lb() returns:
  *
  * - Inf< OFValue > if compute() returned kUnbounded.
  *
  * - - Inf< OFValue > if compute() returned kInfeasible.
  *
  * - A finite value if compute() returned kOK. Since the lower bound for a
  *   maximization problem is typically attached to a feasible solution,
  *   get_lb() should return the objective function value of the feasible
  *   solution that has been returned by the last call to get_var_solution()
  *   [see below], if any. If get_var_solution() has not been called yet
  *   after the last call to compute(), then get_lb() should return the value
  *   of the best feasible solution ever found, the one which will be
  *   returned by the first call to get_var_solution() (if any). If
  *   new_var_solution() is called, it returns true, and get_lb() is called
  *   prior to the next call to get_var_solution() after that, then the value
  *   returned by get_lb() will be the value of the *new* solution, the one
  *   that has *not* been written into the Variable of the Block yet, and
  *   that will be written when get_var_solution() is called (if ever).
  *   Hence, the value returned by get_lb() may change immediately after a
  *   call to new_var_solution(). This is so that the caller may know
  *   beforehand the value of the solution that is returned before it is
  *   even written to the Block (it is assumed that this is known basically
  *   for free by the Solver), which can help her to decide whether or not
  *   the cost of writing (and then reading) is worth. The sequence of 
  *   values of solutions returned by successive calls to get_lb() (after
  *   calls to new_var_solution() that return true) is expected to be
  *   generically nonincreasing (i.e., solutions with "larger" values are
  *   reported first) but this does not need to be strictly enforced by the
  *   Solver, except that the first value ever reported should always be the
  *   best lower bound available. Note, however, that the link between lower
  *   bounds and solutions only holds if the solution itself is feasible [see
  *   is_var_feasible()]; if unfeasible solutions are generated, the value
  *   returned by get_lb() may (necessarily have to) be different from their
  *   objective function value.
  *
  * - Any value, comprised - Inf< OFValue > (meaning that no lower bound is
  *   currently available), if compute() returned any other value. However,
  *   whatever the return status of compute(), the returned value should
  *   always be a valid lower bound.
  *
  * The method is given an implementation returning -Inf< OFValue >(), which
  * should be safe for any derived classes which simply don't have the
  * concept of real objective function. */

 [[nodiscard]] virtual OFValue get_lb( void ) {
  return( - Inf< OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// return a valid upper bound on the optimal objective function value
 /** Returns a valid upper bound on the optimal objective function value.
  * The meaning of this information clearly depends on whether the problem is
  * a minimization or a maximization one, and on the status of the solution
  * process.
  *
  * For a maximization problem, get_ub() returns:
  *
  * - Inf< OFValue > if compute() returned kUnbounded.
  *
  * - - Inf< OFValue > if compute() returned kInfeasible.
  *
  * - A finite value if compute() returned kOK. Since the upper bound for a
  *   maximization problem is not attached to a solution in terms of the
  *   Variable of the Block, the upper bound is not assumed to change if
  *   get_var_solution() [see below] is called again. In several cases the
  *   upper bound is attached to a *dual* solution, but this concept is not
  *   general enough to be supported in the base class [see CDASolver.h].
  *
  * - any value, comprised Inf< OFValue > (meaning that no upper bound is
  *   currently available), if compute() returned any other value.
  *
  * For a minimization problem, get_ub() returns:
  *
  * - - Inf< OFValue > if compute() returned kUnbounded.
  *
  * - Inf< OFValue > if compute() returned kInfeasible.
  *
  * - A finite value if compute() returned kOK. Since the upper bound for a
  *   minimization problem is typically attached to a feasible solution,
  *   get_ub() should return the objective function value of the feasible
  *   solution that will be returned by the next call to get_var_solution()
  *   [see below], if any. If get_var_solution() has not been called yet
  *   after the last call to compute(), then get_ub() should return the value
  *   of the best feasible solution ever found, the one which will be
  *   returned by the first call to get_var_solution() (if any). If
  *   new_var_solution() is called, it returns true, and get_ub() is called
  *   prior to the next call to get_var_solution() after that, then the value
  *   returned by get_ub() will be the value of the *new* solution, the one
  *   that has *not* been written into the Variable of the Block yet, and
  *   that will be written when get_var_solution() is called (if ever).
  *   Hence, the value returned by get_ub() may change immediately after a
  *   call to new_var_solution(). This is so that the caller may know
  *   beforehand the value of the solution that is returned before it is
  *   even written to the Block (it is assumed that this is known basically
  *   for free by the Solver), which can help her to decide whether or not
  *   the cost of writing (and then reading) is worth. Note, however, that
  *   since new_var_solution() means "do you have a solution that you
  *   haven't given me yet?", it may change the value returned by get_ub()
  *   in all calls *but the first*. That's because in the first call to
  *   get_ub() after compute() the returned value is that of the "best"
  *   solution, that still have to be explicitly "requested" by calling
  *   new_var_solution(); hence, the first call to that method is just
  *   asking for that first solution, and not a new one. The sequence of 
  *   values of solutions returned by successive calls to get_ub() (after
  *   calls to new_var_solution() that return true) is expected to be
  *   generically nondecreasing (i.e., solutions with "smaller" values are
  *   reported first) but this does not need to be strictly enforced by the
  *   Solver, except that the first value ever reported should always be the
  *   best upper bound available. Note, however, that the link between upper
  *   bounds and solutions only holds if the solution itself is feasible [see
  *   is_var_feasible() below]; if unfeasible solutions are generated, the
  *   value returned by get_ub() may (necessarily have to) be different from
  *   their objective function value.
  *
  * - Any value, comprised Inf< OFValue > (meaning that no upper bound is
  *   currently available), if compute() returned any other value. However,
  *   whatever the return status of compute(), the returned value should
  *   always be a valid upper bound.
  *
  * The method is given an implementation returning Inf< OFValue >(), which
  * should be safe for any derived classes which simply don't have the
  * concept of real objective function. */

 [[nodiscard]] virtual OFValue get_ub( void ) {
  return( Inf< OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// tells whether a solution is available
 /** Called after compute() this method has to return true if a solution is
  * available to be read with get_var_solution(). It is often the case that
  * a(n approximately optimal) solution is "automatically" available as a
  * by-product of compute(); hence, this method is typically (but not
  * necessarily) true after the end of compute().
  *
  * Once "the first" solution (if ever) has been read, new ones may be
  * produced, if the Solver allows it, by means of new_var_solution().
  *
  * The default implementation in the base class always returns true, which
  * is OK, for instance, for "easy" problems that are never empty and can
  * surely be solved "efficiently enough". */

 [[nodiscard]] virtual bool has_var_solution( void ) { return( true ); }

/*--------------------------------------------------------------------------*/
 /// returns true if the "current" solution is feasible
 /** After a call to has_var_solution() and/or new_var_solution() that
  * returned true, this method can be used to check whether or not the
  * solution is feasible. Note that this can be called before that the
  * solution is actually written into the Variable of the Block via
  * get_var_solution(), as a Solver is assumed to know this. However, the
  * method can also be called after that get_var_solution() is called,
  * returning the same value (until new_var_solution() is called again and
  * it returns true).
  *
  * Note that a user can know the value of the objective function of the same
  * solution by calling the appropriate one between get_lb() and get_ub().
  * This, however, is *only* true for *feasible* solutions, as only these
  * provide valid lower/upper bounds on the optimal value.
  *
  * This virtual method is provided a simple implementation always returning
  * true, which is OK for a solver that always only returns feasible solutions
  * (if any). */

 [[nodiscard]] virtual bool is_var_feasible( void ) { return( true ); }

/*--------------------------------------------------------------------------*/
 /// returns the value of the (current) solution, if any
 /** Assuming that a solution has been constructed (cf. has_var_solution()),
  * this method returns its objective value. In principle one would expect
  * that this is called only if is_var_feasible() returned true, but there
  * may be cases where the Solver detects unfeasibility and finds something
  * like "the least unfeasible solution", so this rule is not necessarily
  * enforced.
  *
  * This method is provided with a standard implementation that just looks
  * if the problem is a minimization or a maximization one, and calls
  * get_ub() and get_lb() accordingly (for a minimization problem each
  * feasible solution provides a valid upper bound, for a maximization one a
  * feasible lower bound). However the method is virtual and can be redefined
  * by derived classes. */

 [[nodiscard]] virtual OFValue get_var_value( void );

/*--------------------------------------------------------------------------*/
 /// write the "current" solution in the Variable of the Block
 /** After a call to has_var_solution() and/or new_var_solution() that
  * returned true, this method can be used to have the solution actually 
  * written in the Variable of the Block. Any previous solution (or
  * direction, see get_var_direction()) is lost for good, as the Block class
  * does not provide any mechanism to retrieve the previous value in the
  * Variable at a later time: only one solution at a time can be stored in
  * the Variable. However, the state of the Variable in the Block can be
  * saved in a Solution [see Solution.h] object before this method is called
  * if it is to be kept for future use. Note that the base Solver() class
  * makes no assumption about how the solution is stored, this being
  * dependent on what the Variable actually are.
  *
  * It is an error to call this method if has_var_solution() or
  * new_var_solution() have not been called and returned true (which means,
  * in particular, if no Block is attached to this Solver).
  *
  * Important note: writing solution information inside a Block is not
  * counted as a change of the Block and therefore no Modification is issued.
  * However, this is indeed a change of the state of the Block, and therefore
  * has the issue that concurrent calls to get_var_solution() to different
  * Solver registered to the same Block must be avoided since they would
  * result in garbled data. As a consequence
  *
  *     THE Block MUST ALWAYS BE lock()-ED WHEN get_var_solution() IS CALLED
  *
  * unless of course the user is 100% sure that no concurrent access to the
  * Block is ever possible. However, it is also important to remark that
  *
  *      lock()-ING OF THE Block MUST NOT BE DONE BY get_var_solution()
  *
  * The rationale is that if it was get_var_solution() to lock() the Block,
  * then it should also unlock() it at the end. But the reason for wanting a
  * solution written into the Block is to make some computations out of it
  * (say, write it to a file or separate a Variable / Constraint out of it).
  * For this to happen one must be sure that the solution information has not
  * been accidentally rewritten by some other Solver, and therefore the lock
  * on the Block must be kept for all the time in which the solution
  * information is useful to the user. Hence the lock must be acquired prior
  * to calling get_var_solution(). It might in principle make sense to have
  * a separate lock for solution information, but this is not currently
  * considered crucial enough to warrant the extra effort.
  *
  * Note that, while the largest burden of producing a solution is typically
  * bore by compute() and/or new_var_solution(), it is still possible that
  * "decoding" the internal information of the Solver in order to produce one
  * in the format that the Variable of the Block require may be somewhat
  * costly. This is in particular true because a solution may involve a rather
  * large amount of data. In some cases, not all that data is actually
  * necessary, as only "a part" of the solution might be enough (say, that
  * which is required to separate one given family of valid inequalities).
  * This is why support is offered in the method to only retrieve "a part" of
  * the current solution by means of a (pointer to a) Configuration object.
  * The full generality of a Configuration is required because the solution of
  * a Block comprises that of all its sub-Block (recursively), so an
  * arbitrarily large tree-shaped data structure may be required to pin down
  * the relevant parts in all this. The default is nullptr, which has to be
  * intended as "the whole solution".
  *
  * Not coincidentally, all methods in Block that concern solutions, i.e.,
  * get_Solution() and map_[back/forward]_solution(), also take a(n optional)
  * Configuration parameter. Indeed, it should be expected that, for Solver
  * accessing to the "physical representation" of the Block (and, therefore,
  * knowing exactly which :Block it exactly is), the "format" of the
  * Configuration objects should be the same, although this is not a strict
  * requirement (as there may be some reasons not to do that). However, for
  * general-purpose Solver using the "abstract representation", this is
  * clearly not possible. Yet, these Solver can still be instructed to only
  * read "a part" of the solution, for instance specified in terms of which
  * of the "groups" of Variable of the Block, as returned by
  * get_static_variables() and get_dynamic_variables(), need be changed.
  *
  * As a consequence to the fact that get_var_solution() can retrieve only "a
  * part" of the solution information, it may make sense to call it more than
  * once for each call to has_var_solution() or new_var_solution() that
  * returns true, if each call specifies for "a different part" via a
  * different Configuration. Note that, if a different "part" of a solution
  * is read after that another one has been previously read, it is intended
  * that the previous part remains in place. This means that the "full"
  * solution can eventually be retrieved piecemeal with a finite number of
  * calls to get_var_solution() with appropriate Configuration(s), although
  * in this case a unique call with nullptr will probably be more efficient.
  * Also, note that the "parts" that two different Configuration specify for
  * may in principle have nonempty intersection. This, however, means that the
  * common part may be written identically twice, with an unjustified
  * performance hit, since the Solver is not required to (although it might,
  * if it so chooses) keep track of which "parts" of the solution have been
  * retrieved already; hence, this is better avoided. */

 virtual void get_var_solution( Configuration * solc = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if it is possible to generate a new solution
 /** This method must be called each time the user of the Solver wants that
  * it produces a *new* solution.
  *
  * This method can only be called after the compute() method of this Solver
  * has been invoked at least once, and if has_var_solution() has already
  * returned true. The method has to return true if a solution that is
  * *different* from the previously available one is available. This method
  * returning false means that the Solver has no means of producing any other
  * solution for good, so it does not make sense to call it again unless
  * something has changed in the problem encoded in the Block [see the
  * discussion in compute()] or, in case the last call to compute() has
  * returned kStopTime or kStopIter, compute() is called again. For a typical
  * algorithm that only produces one solution, this method should always 
  * return false; indeed, this is what the default implementation of the
  * method does.
  * 
  * The user can restrict the set of generated solutions, both in number
  * (see intMaxSol) and in terms of their quality (see dblRAccSol and
  * dblAAccSol) or feasibility (see dblFAccSol). In particular, note that the
  * Solver may be instructed (via dblFAccSol) to also return *unfeasible*
  * solutions if it has produced any (which may be the case if, for instance,
  * the models is infeasible, and therefore no feasible solution can ever be
  * found). This means in particular that it is in principle sensible to call
  * this method even if compute() has returned a value not necessarily
  * implying that a solution has been found (like kError), and even if it is
  * guaranteed that no solution exists (kInfeasible).
  *
  * Note that a call to new_var_solution() may be costly, in that the Solver
  * is only supposed to return true if it is *certain* to be able to produce
  * a new solution, which typically means having in fact produced it. However,
  * the solution is not really written to the Variable until
  * get_var_solution() is called, so part of the cost of producing it may in
  * fact be bore by that method.
  *
  * While producing its solutions, it is expected that a Solver should:
  *
  * - return first feasible solutions, if any, and only after having exhausted
  *   them start returning infeasible ones;
  *
  * - return feasible solutions, at least roughly, in order of their objective
  *   function values, i.e., the better ones first (and in particular, the
  *   absolute best solution ever found as absolute first);
  *
  * - return infeasible solutions, at least roughly, in order of their 
  *   feasibility (the "least unfeasible ones first", with some appropriate
  *   global measure of infeasibility) and/or in order of their objective
  *   function values (the better ones first), although the objective value
  *   of an unfeasible solution may not make too much of a sense, so that
  *   feasibility should be considered first (with objective value used e.g.
  *   to break ties).
  *
  * A special use of this method is after that compute() has returned
  * kUnbounded and set_unbounded_threshold() has been called; see the
  * comments to that method. */

 [[nodiscard]] virtual bool new_var_solution( void ) { return( false ); }

/*--------------------------------------------------------------------------*/
 /// set the min/max objective value of the solution for the unbounded case
 /** A call to compute() that returned kUnbounded supposedly mean that the 
  * Solver can guarantee that it is always possible to find feasible
  * solutions whose value is "better" than any given threshold. Usually, this
  * guarantee can be turned into a way of actually produce these solutions.
  * Calling this method instructs the Solver to only produce solutions for
  * the given value of the threshold, i.e.:
  *
  * - for a minimization problem, with objective function value <= thr;
  *
  * - for a maximization problem, with objective function value >= thr.
  *
  * Hence, after a call to this method, new_var_solution() will return true
  * only if it is capable of producing a new solution that has a "better"
  * objective function value than thr. All the solutions produced by calls
  * to new_var_solution() / get_var_solution() after this call to
  * set_unbounded_threshold() (and before any call that modify the threshold)
  * must then satisfy the requirement. Note that if the previous produced
  * solution already did, the solver may return false; however, it is then
  * simple to have new solutions produced (just be more demanding with thr).
  *
  * The method is given a default implementation in the base Solver class
  * doing nothing, which should be safe for any derived classes which simply
  * don't have the concept of real objective function.
  *
  * Note that a Solver returning kUnbounded usually means that it has produced
  * some "certificate of infeasibility", which may be of extreme interest for
  * the user. However, the actual form of the certificate (a ray of the
  * polyhedron, a directed cycle of negative weight, ...) is highly dependent
  * on the nature of the problem solved, and therefore it is not possible to
  * provide a specific method for that in the general interface. This method
  * is a completely general way to exploit this information and therefore it
  * can be added to the general interface, but it can be expected that
  * specialized Solver will offer their own specific ways to access to the
  * unboundedness certificates. A (largish) step in this direction is
  * provided by the [has/get/new]_var_direction() methods. */

 virtual void set_unbounded_threshold( OFValue thr ) {}

/*--------------------------------------------------------------------------*/
 /// tells whether an unbounded direction is available
 /** A call to compute() that returned kUnbounded supposedly mean that the 
  * Solver can guarantee that it is always possible to find feasible
  * solutions whose value is "better" than any given threshold. For many
  * models, this means that there exist an unbounded [a/de]scent direction,
  * i.e., a direction along which one may go forever without leaving the
  * feasible region and so that the objective function is [in/de]creasing
  * indefinitely along it. This gives a convenient *certificate of
  * unboundedness* for the model (and, notably, a convenient *certificate of
  * infeasibility* for their dual, if one exists, see CDASolver.h). Hence, the
  * user may be interested in accessing one (or more) of these directions.
  * This can be done implicitly and indirectly via set_unbounded_threshold(),
  * but a more direct representation may be useful.
  *
  * Called after compute() that returned kUnbounded, this method has to
  * return true if an unbounded [a/de]scent direction is available to be read
  * with get_var_direction(). It is often the case that such a direction is
  * "automatically" available as a by-product of compute() whenever the latter
  * returns kUnbounded; hence, this method is typically (but not necessarily)
  * true after the end of compute() whenever kUnbounded is returned.
  *
  * Once "the first" direction (if ever) has been read, new ones may be
  * produced, if the Solver allows it, by means of new_var_direction().
  *
  * The default implementation in the base class always returns false, which
  * is OK for Solver that cannot produce any unbounded direction. */

 [[nodiscard]] virtual bool has_var_direction( void ) { return( false ); }

/*--------------------------------------------------------------------------*/
 /// write the "current" unbounded direction in the Block
 /** After a call to has_var_direction() and/or new_var_direction() that
  * returned true, this method can be used to have the solution actually
  * written in the Variable of the Block. Because directions are in the same
  * space of solutions, the method assumes that the direction can be suitably
  * represented with the same Variable that are used for solutions. Apart
  * from that, the base Solver class makes no assumption about how the
  * direction is stored, this being dependent on what the Variable actually
  * are. Note that the previous value in the Variable is lost for good,
  * irrespectively of the fact that it contained a solution or a direction,
  * and the Solver class does not provide any mechanism to retrieve it at a
  * later time (except saving them beforehand in a Solution object, see
  * Solution.h).
  *
  * One possible use for such a direction is to construct a solution with
  * "arbitrarily good" (small for a minimization problem, large for a
  * maximization one) value of the objective function. However, this
  * typically requires first constructing a feasible solution, and then
  * modifying it by means of the direction. Because of the fact that the
  * Variable of a Block can only hold one value at a time, this is not
  * exactly trivial to attain with the tools provided here (some copy of
  * the Variable content has to be done). However, the method
  * set_unbounded_threshold() exactly caters for this need.
  *
  * It is an error to call this method if has_var_direction() and
  * new_var_direction() have not been called and returned true (which
  * means, in particular, if no Block is attached to this Solver).
  *
  * See the comments to get_var_solution() about the need of lock()-ing the
  * Block prior to calling this method, which of course apply verbatim here.
  *
  * Note that, while the largest burden of producing a direction is typically
  * bore by compute() and/or new_var_direction(), it is still possible that
  * "decoding" the internal information of the CDASolver in order to produce
  * one in the proper format that the Variables of the Block require may be
  * somewhat costly. This is why, as in get_var_solution(), a (pointer to a)
  * Configuration object can be passed to specify that only "a part" of the
  * direction need to be retrieved. See get_var_solution() for more comments
  * about this parameter; here we just mention that the format of the
  * Configuration for that method need not necessarily be the same as the
  * format of the Configuration for this one, although of course it is
  * somehow nice if this is true. Note, however, that the original :Block
  * may already have specific Configuration for directions, to be used in
  * its solution-related methods get_Solution() and
  * map[forward/back]_solution(). Hence, for Solver using the "physical
  * representation" of the Block, the set of Configuration of the Block (at
  * least those concerning only the "primal" solution, since a "dual" one
  * may also be there) can be "partitioned" between this method and
  * get_var_solution().
  *
  * The method is given a default implementation in the base Solver class
  * doing nothing, for solvers that cannot produce any unbounded direction. */

 virtual void get_var_direction( Configuration * dirc = nullptr ) {}

/*--------------------------------------------------------------------------*/
 ///< returns true if it is possible to generate a new unbounded direction
 /**< This method must be called each time the user of the Solver wants that
  * it produces a *new* unbounded direction.
  *
  * This method can only be called after the compute() method of this Solver
  * has been invoked at least once, has reported kUnbounded, and if
  * has_var_direction() has already returned true. The method has to return
  * true if an unbounded [a/de]scent direction that is *different* from the
  * previously returned one is available. This method returning false means
  * that the Solver has no means of producing any other unbounded direction
  * for good, so it does not make sense to call it again unless something has
  * changed in the problem encoded in the Block [see the discussion in
  * compute()] or, in case the last call to compute() has returned kStopTime
  * or kStopIter, compute() is called again. For a typical algorithm that
  * only produces at most one unbounded [a/de]scent direction, this method
  * should always false; indeed, this is what the default implementation of
  * the method does.
  *
  * Note that a call to new_var_direction() may be costly, in that the Solver
  * is only supposed to return true if it is *certain* to be able to produce a
  * new direction, which typically means having in fact produced it. However,
  * the direction is not really written to the Variable until
  * get_var_direction() is called, so part of the cost of producing it may in
  * fact be bore by that method.
  *
  * There is no default assumption about the order in which unbounded
  * directions are generated by the Solver, although each one may have some
  * reasonable definition of an appropriate notion of "better direction"
  * that might be employed. */

 [[nodiscard]] virtual bool new_var_direction( void ) { return( false ); }

/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS FOR READING THE DATA OF THE Solver ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the state of the Solver
 *  @{ */

 /// retrieve the Block this Solver is attached to (if any)

 [[nodiscard]] virtual Block * get_Block( void ) const { return( f_Block ); }

/*--------------------------------------------------------------------------*/
 /// getting the "identity" of this Solver

 [[nodiscard]] virtual void * id( void ) { return( f_id ); }

/*--------------------------------------------------------------------------*/
 /// getting the classname of this Solver
 /** Given a Solver, this method returns a string with its class name; unlike
  * std::type_info.name(), there *are* guarantees, i.e., the name will
  * always be the same.
  *
  * The method works by dispatching the private virtual method private_name().
  * The latter is automatically implemented by the 
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h], hence this
  * comes at no cost since these have to be called somewhere to ensure that
  * any :Solver will be added to the factory. Actually, since
  * Solver::private_name() is pure virtual, this ensures that it is not
  * possible to forget to call the appropriate SMSpp_insert_in_factory_cpp_*
  * for any :Solver because otherwise it is a pure virtual class (unless the
  * programmer purposely defines private_name() without calling the macro,
  * which seems rather pointless). */

 [[nodiscard]] const std::string & classname( void ) const {
  return( private_name() );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the Solver
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( idx_type( intLastAlgPar ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( idx_type( dblLastAlgPar ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_str_par( void ) const override {
  return( idx_type( strLastAlgPar ) );
  } 

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  if( par >= intLastAlgPar )
   throw( std::invalid_argument( "invalid int parameter name" ) );
  return( dflt_int_par[ par ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  if( par >= dblLastAlgPar )
   throw( std::invalid_argument( "invalid double parameter name" ) );
  return( dflt_dbl_par[ par ] );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  const auto it = int_pars_map.find( name );
  if( it == int_pars_map.end() )
   return( Inf< idx_type >());
  return( it->second );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  const auto it = dbl_pars_map.find( name );
  if( it == dbl_pars_map.end() )
   return( Inf< idx_type >() );
  return( it->second );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  if( idx >= intLastAlgPar )
   throw( std::invalid_argument( "invalid int parameter name" ) );
  return( int_pars_str[ idx ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  if( idx >= dblLastAlgPar )
   throw( std::invalid_argument( "invalid double parameter name" ) );
  return( dbl_pars_str[ idx ] );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/** @name Changing the data of the model
 *  @{ */

 /// add a new Modification to the list
 /** This method must be used by the Block (or any of its components:
  * Variables, Constraints, Objective Function) to pass the (shared) pointer
  * to an (object of class derived from) Modifications corresponding to
  * any change that occurred in the data. Note that the method gets a
  * reference to a shared pointer, which is OK because push_back() then
  * copies the object in the protected field v_mod, so a copy is done (but
  * only one).
  *
  * The Solver is supposed to delete each Modification object it receives as
  * soon as it has finished to process the information they contain. Because
  * the pointer is a shared one, this will result in the actual Modification
  * object to be deleted when all the Solvers concerned with the same Block
  * (these either attached to the Block or to some of its ancestors) have
  * finished processing it. Modification objects can have some nontrivial
  * amount of information attached to them [see, e.g.,
  * remove_dynamic_variables() in Block.h], that only gets deleted then.
  * Hence, it is crucial that Solvers do actually clear the (smart pointers to
  * the) Modification.
  *
  * In general, Modification should be treated in a rigidly sequential order
  * to ensure that the state of the Solver is consistent with what was the 
  * state of the Block at the moment in which they were issued (which is, in
  * general, different from the current state of the Block). However, the
  * NBModification, which must be issued by Block::load(), is an exception:
  * when it is issued,
  *
  *    ALL EXISTING Modification RELATED TO THIS Block SHOULD BE
  *    IMMEDIATELY DELETED WITHOUT BEING ACTED UPON FIRST
  *
  * This is due to the fact that the, despite being the same physical object
  * with the same memory address, the Block is now in fact a "completely
  * different Block". In particular, handling of Modification can usually rely
  * on the fact that "some aspects" of the Block cannot change, such as the
  * number and type of static Variable and Constraint. However, after a
  * NBModification, this is no longer true: a(n abstract) Modification
  * (pointing to some static Variable / Constraint) may contain invalid
  * pointers to objects that have meanwhile ceased to exist. Therefore, the
  * only safe recourse is to immediately delete them all. This is, anyway,
  * also the cheapest thing to do: there is no point in wasting time on these
  * Modification, since they refer to the status of a Block that is fact no
  * longer exist.
  *
  * Doing all this is also quite simple for Solver of "leaf" Block that have
  * no sub-block: if that Modification is received the current queue of
  * outstanding Modification (received, but not acted upon, by the Solver) is
  * just cleared right away before inserting the new one. This may not be
  * appropriate for Solver of "complex" Block where the re-loading of a
  * sub-Block can be faced without a complete reset of all the state of the
  * Solver, but this is still relatively easy to manage: each time a
  * Modification is received it should be checked for being one of these, and
  * an appropriate flag should be set so that the Solver knows that it has to
  * react in a "nonstandard" way. This is, however, left to specific :Solver,
  * while the simple reaction is directly implemented in the method of the
  * base Solver class.
  *
  * There is another case in which Modification are treated in a non-standard
  * way: they can be plainly ignored if inhibit_Modification( true ) has been
  * called (and inhibit_Modification( false ) has not been called since). The
  * rationale for this is that
  *
  *     A Solver CAN ITSELF CHANGE THE Block FOR ALGORITHMIC PURPOSES
  *
  * (think adding dynamic Variable / Constraint, in case this is not done by
  * the Block itself via generate_dynamic_*()). In this case, having the
  * corresponding Modification received by the Solver would be wasteful and
  * confusing. This should therefore be avoided, and there are two different
  * ways in which this can be achieved:
  *
  * - The Solver avoids any Modification to be issued. This, however, is
  *   only possible if:
  *   
  *   - the Solver is the only one attached to the Block, but this is not
  *     easy to check;
  *
  *   - the Solver pledges to undo all the changes it did before releasing
  *     the lock on the Block, so that any other Solver attached to it
  *     does not have to react to changes that "have never happened".
  *
  *    Clearly, these are rather restrictive conditions.
  *
  * - The Solver allows modifications to be issued, but ignores the
  *   Modification it itself caused.
  *
  * This is what inhibit_Modification() provides. Note that this inhibits
  * *any* Modification to be received, and hence one may be worried that
  *
  *     THE Solver MAY MISS OUT ON SOME MODIFICATION "NOT OF ITS OWN" THAT 
  *     GET ISSUED IN THE MEANTIME, BUT THIS CANNOT HAPPEN IF THE Solver
  *     BEHAVES "SINGLE THREADED" IN THIS RESPECT
  *
  * The point is that any operation that changes the Block (and therefore
  * issues Modification) need be called with the Block under lock(). This
  * implies that the Block must be locked by the Solver itself when the
  * changes are done, which in turns implies that no other Solver / thread
  * can be issuing other Modification on the same Block in the meantime
  * (note that locking a Block also locks all its sub-Block, and that
  * Modification travel "upwards" on the Block tree, so that other Solver
  * / threads working on the sub-Block cannot cause Modification to be
  * issued, either). Also Solver need be locked when performing
  * status-changing operations, so the operation is safe
  *
  *     UNLESS THE Solver ITSELF IS MULTI-THREADED
  *
  * A specific case in which this may happen is when a Solver for a Block
  * uses sub-Solver for the sub-Block, in which case it can "borrow them its
  * identity" (cf. set_id()) and allow them to (possibly, concurrently)
  * operate on the sub-Block on its behalf while keeping the ownership of
  * the Block. In this case, Modification to sub-Block may in principle be
  * issued when the "main" solver is "not listening", and therefore be lost.
  *
  *     IT IS THE Solver RESPONSIBILITY TO HANDLE THESE CASES
  *
  * (basically, some form of synchronization with the Solver threads will
  * be required).
  *
  * The base Solver class provides a std::list of smart pointers to
  * Modification where the Modification are stored. In the implementation of
  * the method in the base class, the std::list is protected from concurrent
  * access via a std::atomic_flag. This implies active wait is involved, but
  * operations on the std::list are very quick.
  *
  * Actually, it can be argued that the std::atomic_flag is not even needed,
  * because Modification should only happen when a Block is locked, and
  * therefore the Block lock should also work as a lock for the std::list.
  * However, there is nothing guaranteeing that the entity having locked the
  * Block is not itself multi-threaded. In particular, Block ownership can
  * be "lent" to other entities, and therefore it is possible that, say,
  * different threads could be concurrently working on different sub-Block of
  * a given block. Therefore, it is in general possible that the std::list of
  * Modification to a given Solver is concurrently accessed by different
  * threads, say making changes to sub-Block of the Block the Solver is
  * attached to. Since the active wait on the std::atomic_flag should be
  * quite cheap, the mechanism is added to the base Solver class so that all
  * derived classes can rely on this being handled already. */

 virtual void add_Modification( sp_Mod & mod ) {
  if( f_no_Mod )
   return;

  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) );

  if( std::dynamic_pointer_cast< const NBModification >( mod ) )
   v_mod.clear();

  v_mod.push_back( mod );

  f_mod_lock.clear( std::memory_order_release );  // release lock
  }

/*--------------------------------------------------------------------------*/
 /// temporarily inhibits the Solver to receive Modification
 /** inhibit_Modification( true ) makes the Solver temporarily ignore any
  * Modification from the Block, the idea being that these are Modification
  * corresponding to changes made by itself; see the comments to
  * add_Modification() for details. inhibit_Modification( false ) restores
  * the usual storing of any received Modification.
  *
  * There should be no reason why derived classes should mess up with this
  * mechanism, but the method is virtual for extra flexibility. */

 virtual void inhibit_Modification( bool do_it = true ) { f_no_Mod = do_it; }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 using SolverFactory = boost::function< Solver *() >;
 // type of the factory of Solver

 using SolverFactoryMap = std::map< std::string , SolverFactory >;
 // type of the map between strings and the factory of Solver

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected method for the Modification queue
 *
 * Since the Modification queue is "protected" by the f_mod_lock atomic flag,
 * these methods are provided for convenience to derived classes to retrieve
 * and delete the front() sp_Mod in the queue. This is enough for "simple"
 * handling of the Modification queue that just looks at them in the natural
 * order; derived classes requiring more complicated logic will hava to
 * implement it themselves.
 * @{ */

 /// add the Modification to the queue, without checking for NBModification

 void push_back( sp_Mod & mod ) {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) );

  v_mod.push_back( mod );

  f_mod_lock.clear( std::memory_order_release );  // release lock
  }

/*--------------------------------------------------------------------------*/
 /// returns the front sp_Mod on the Modification queue, nullptr if empty

 sp_Mod front( void ) {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) )
   ;

  auto mod = v_mod.empty() ? sp_Mod() : v_mod.front();

  f_mod_lock.clear( std::memory_order_release );  // release lock

  return( mod );
  }

/*--------------------------------------------------------------------------*/
 /// removes the front sp_Mod from the Modification queue

 void pop_front( void ) {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) )
   ;

  v_mod.pop_front();  // remove the first Modification

  f_mod_lock.clear( std::memory_order_release );  // release lock
  }

/*--------------------------------------------------------------------------*/
 /// removes and returns the front sp_Mod from the Modification queue

 sp_Mod pop( void ) {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) )
   ;

  sp_Mod mod;
  if( ! v_mod.empty() ) {  // there is anything in the queue
   mod = v_mod.front();    // get the first Modification
   v_mod.pop_front();      // remove it from the queue
   }

  f_mod_lock.clear( std::memory_order_release );  // release lock

  return( mod );
  }

/*--------------------------------------------------------------------------*/
 /// clear the Modification queue

 void mod_clear( void ) {
  // try to acquire lock, spin on failure
  while( f_mod_lock.test_and_set( std::memory_order_acquire ) )
   ;

  v_mod.clear();  // clear all Modification

  f_mod_lock.clear( std::memory_order_release );  // release lock
  }

/** @} ---------------------------------------------------------------------*/
/** @name Protected methods for handling static fields
 *
 * These methods allow derived classes to partake into static initialization
 * procedures performed once and for all at the start of the program. These
 * are typically related with factories.
 * @{ */

 /// method encapsulating the Solver factory
 /** This method returns the Solver factory, which is a static object.
  * The rationale for using a method is that this is the "Construct On First
  * Use Idiom" that solves the "static initialization order problem". */

 static SolverFactoryMap & f_factory( void );

/*--------------------------------------------------------------------------*/
 /// empty placeholder for class-specific static initialization
 /** The method static_initialization() is an empty placeholder which is made
  * available to derived classes that need to perform some class-specific
  * static initialization besides these of any :Solver class, i.e., the
  * management of the factory. This method is invoked by the
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h] during the
  * standard initialization procedures. If a derived class needs to perform
  * any static initialization it just have to do this into its version of
  * this method; if not it just has nothing to do, as the (empty) method of
  * the base class will be called.
  *
  * This mechanism has a potential drawback in that a redefined
  * static_initialization() may be called multiple times. Assume that a
  * derived class X redefines the method to perform something, and that a
  * further class Y is derived from X that has to do nothing, and that
  * therefore will not define Y::static_initialization(): them, within the
  * SMSpp_insert_in_factory_cpp_* of Y, X::static_initialization() will be
  * called again.
  *
  * If this is undesirable, X will have to explicitly instruct derived classes
  * to redefine their (empty) static_initialization(). Alternatively,
  * X::static_initialization() may contain mechanisms to ensure that it will
  * actually do things only the very first time it is called. One standard
  * trick is to do everything within the initialisation of a static local
  * variable of X::static_initialization(): this is guaranteed by the
  * compiler to happen only once, regardless of how many times the function
  * is called. Alternatively, an explicit static boolean could be used (this
  * may just be the same as what the compiler does during the initialization
  * of static variables without telling you). */

 static void static_initialization( void ) {}

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS  ---------------------------*/
/*--------------------------------------------------------------------------*/

 std::recursive_mutex f_mutex;  ///< the mutex for locking the Solver

 Block * f_Block;       ///< pointer to the Block

 std::ostream * f_log;  ///< pointer to the stream where the log is done

 void * f_id;           ///< the "identity" of the Solver 

 bool f_no_Mod;        ///< if Solver should ignore (its own) Modification

 std::atomic_flag f_mod_lock{};  ///< active lock for v_mod

 Lst_sp_Mod v_mod;     ///< list of (shared pointers to) Modification

 std::vector< std::vector< EventHandler > > v_events;
 ///< container of event handlers
 /**< v_events[ h ][ i ] contains the event handler of
  * ID i for the event type h. */

 const static std::vector< int > dflt_int_par;
 ///< the (static const) vector of int parameters default values

 const static std::vector< double > dflt_dbl_par;
 ///< the (static const) vector of double parameters default values

 const static std::vector< std::string > int_pars_str;
 ///< the (static const) vector of int parameters names

 const static std::vector< std::string > dbl_pars_str;
 ///< the (static const) vector of double parameters names

 const static std::map< std::string , idx_type > int_pars_map;
 ///< the (static const) map for int parameters names

 const static std::map< std::string , idx_type > dbl_pars_map;
 ///< the (static const) map for double parameters names

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Solver::private_name() (pure virtual)

 [[nodiscard]] virtual const std::string & private_name( void ) const = 0;

/*--------------------------------------------------------------------------*/

 };   // end( class Solver )

/*--------------------------------------------------------------------------*/
/*------------------------- Solver-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Solver_TYPES Solver-related types
 *  @{ */

typedef Solver * p_Solver;
///< a pointer to Solver

typedef std::vector< p_Solver > Vec_Solver;
///< a vector of pointers to Solver

typedef Vec_Solver::iterator Vec_Solver_it;
///< iterator for a Vec_Solver

typedef const std::vector< p_Solver > c_Vec_Solver;
///< a const vector of pointers to Solver

typedef Vec_Solver::const_iterator c_Vec_Solver_it;
///< iterator for a c_Vec_Solver

typedef std::list< p_Solver > Lst_Solver;
///< a vector of pointers to Solver

typedef Lst_Solver::iterator Lst_Solver_it;
///< iterator for a Vec_Solver

typedef const std::list< p_Solver > c_Lst_Solver;
///< a const vector of pointers to Solver

typedef Lst_Solver::const_iterator c_Lst_Solver_it;
///< iterator for a c_Vec_Solver

/** @} end( group( Solver_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------- inline methods implementation ------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Solver.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Solver.h -----------------------------*/
/*--------------------------------------------------------------------------*/

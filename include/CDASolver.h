/*--------------------------------------------------------------------------*/
/*-------------------------- File CDASolver.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* CDASolver class, which derives from Solver
 * [see Solver.h] and extends the interface of the base class to support the
 * concept that it is Aware that (upper or lower, as appropriate) bounds on
 * the optimal value of the problem are obtained by means of Convex Duality
 * arguments.
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

#ifndef __CDASolver
 #define __CDASolver  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Solver.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CDASolver -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class for a Convex Duality Aware Solver
/** The *abstract* CDASolver class derives from Solver [see Solver.h] and
 * extends the interface of the base class to support the concept that it is
 * Aware that (upper or lower, as appropriate) bounds on the optimal value of
 * the problem are obtained by means of Convex Duality arguments. In other
 * words, this is a Convex-Duality-Aware Solver.
 *
 * The main theoretical consequences of the fact that a Solver is based on
 * the properties of convex sets/functions are:
 *
 * - There typically is a *dual* problem, usually having *dual variables*
 *   associated to the constraints of the original (primal) problem, which
 *   can be used to obtain (upper or lower, as appropriate) bounds on the
 *   optimal value of the (primal) problem.
 *
 * - Since convex sets/functions "only go to infinity as (their recession)
 *   cones"; the fact that the objective function value of a model is
 *   unbounded below/above is basically equivalent to the fact that there
 *   exist an unbounded [a/de]scent direction, i.e., a direction along which
 *   one may go forever without leaving the feasible region and so that the
 *   objective function is [in/de]creasing indefinitely along it. This gives
 *   both a convenient *certificate of unboundedness* for unbounded models,
 *   and a convenient *certificate of infeasibility* for infeasible models
 *   via a convenient *certificate of unboundedness of their dual*.
 *
 * Hence, this class does little more than extend the interface of Solver to
 * the concepts that:
 *
 * - (multiple) dual solutions can be generated;
 *
 * - (upper or lower, as appropriate) bounds on the optimal value of the
 *   (primal) problem are associated to daid dual solutions, and therefore
 *   may change as different solutions are generated;
 *
 * - unbounded [a/de]scent directions can be produced for unbounded primal
 *   and dual problems, providing both convenient certificates of
 *   unboundedness and certificates of infeasibility;
 *
 * - the dual problem may be either an "exact" or an "inexact" one, i.e.,
 *   strong duality holds, as opposed to only weak duality; in other words,
 *   being v(P) the optimal value of the primal (minimization) problem and
 *   v(D) the optimal value of the dual (maximization) problem,
 *
 *       v(D) <= v(P)      (weak duality)
 *
 *   is always assumed to hold, but
 *
 *       v(D) == v(P)      (strong duality)
 *
 *   may or may not hold, see is_dual_exact() (of course, the sign of the
 *   inequality is reversed if the primal is a maximization problem).
 *
 * Regarding the second point, the interface of CDASolver does not really
 * change that of Solver, in that the methods get_ub() and get_lb() remain
 * the same. However, the following revision of the comments need to be
 * taken into account:
 *
 * - For get_[lb/ub]() and a [min/max]imization problem, any returned finite
 *   [low/upp]er bound has to be considered as being attached to a feasible
 *   *dual* solution, in the sense of being its objective function value. If
 *   get_dual_solution() has not been called yet after the last call to
 *   compute(), then get_[lb/ub]() should return the value of the best dual
 *   feasible solution ever found, the one which will be returned by the
 *   first call to get_dual_solution() (if any). If new_dual_solution() is
 *   called, it returns true, and get_[lb/ub]() is called prior to the next
 *   call to get_dual_solution() after that, then the value returned by
 *   get_[lb/ub]() will be the value of the *new* dual solution, the one that
 *   has *not* been written into the Block (in the specific way of the
 *   current model, e.g., in terms of dual variables attached to the
 *   Constraints) yet, and that will be written when get_dual_solution() is
 *   called (if ever). Hence, the value returned by get_[lb/ub]() may change
 *   immediately after a call to new_dual_solution(). This is so that the
 *   caller may know beforehand the value of the dual solution that is
 *   returned before it is even written to the Block (it is assumed that this
 *   is know basically for free by the Solver), which can help her to decide
 *   whether or not the cost of writing (and then reading) is worth. The
 *   sequence of values of solutions returned by successive calls to
 *   get_[lb/ub]() (after calls to new_var_solution() that return true) is
 *   expected to be generically "improving", i.e., solutions with
 *   [smaller/larger] values are reported first; however, this does not need
 *   to be strictly enforced by the Solver, except that the first value ever
 *   reported should always be the best [low/upp]er bound available. Note,
 *   however, that the link between [lo/upp]wer bounds and solutions only
 *   holds if the solution itself is dual feasible [see is_dual_feasible()];
 *   if dual unfeasible solutions are generated, the value returned by
 *   get_[lb/ub]() may (necessarily have to) be different from their
 *   objective function value. 
 *
 * Similarly, the CDASolver class re-uses and extends the 
 * set_unbounded_threshold() method to produce "arbitrarily good" dual
 * solutions when the dual problem is unbounded, which implies that the
 * primal one is infeasible. That is, when compute() returns kInfeasible, the
 * method can be used to set a threshold thr such that each subsequent call
 * to new_dual_solution() / get_dual_solution() only produces (new) dual
 * solutions that:
 *
 * - for a primal minimization problem, which means that the dual is a
 *   maximization one, have objective function value >= thr;
 *
 * - for a primal maximization problem, which means that the dual is a
 *   minimization one, have objective function value <= thr.
 *
 * There is no need for a separate set_unbounded_threshold() method for the
 * dual problem because there is no risk that both problems are
 * simultaneously unbounded: to which of the two the threshold necessarily
 * refers is immediately obvious from the return code of compute().
 *
 * It should also be noted that, since the dual of a minimization model is a
 * maximization one and vice-versa, the interpretation of upper cutoffs and
 * lower cutoffs [see dblUpCutOff and dblLwCutOff in Solver.h] can be done
 * both in terms of the primal and of the dual problem. For instance, assume
 * an upper cutoff \eps is set for a minimization problem: the condition
 *
 *   lb >= \eps
 *
 * means *both*:
 *
 * - for the primal problem, that is a minimization one, the condition says
 *   that a solution with objective function value at least as good as (i.e.,
 *   smaller than) \eps was needed, but it is now clear that this is never
 *   going to happen, so the problem is treated as being as being detected
 *   to be unfeasible and the algorithm can stop already;
 *
 * - for the dual problem, that is a maximization one, the condition says
 *   that a solution with objective function value at least as good as (i.e.,
 *   greater than) \eps was needed; this has already been found, so the
 *   problem is treated as being solved already and the algorithm can stop.
 *
 * Of course, this is symmetric if the primal is a maximization problem. */

class CDASolver : public Solver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// public enum "extending" sol_type to a specific case of CDASolvers

 enum cda_sol_type {
  kBothInfeasible = kInfeasible + 1  ///< both primal and dual infeasible
                                     /**< *Both* the primal and the dual
                                      * models are infeasible. This is
   * typically a rare but not impossible occurrence, and it has the
   * unfortunate consequence that it is impossible to provide a convenient
   * certificate of infeasibility of either problem in terms of a certificate
   * of unboundedness of the other. However, if for some reason the Solver is
   * certain that this is what is happening, this is still a "kOK-type"
   * return value in that the problem is "solved for good". */

  };              // end( cda_sol_type )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" int_par_type_S to a specific case of CDASolvers

 enum int_par_type_CDAS {
  intMaxDSol = intLastAlgPar,  ///< maximum number of different dual solutions
                               /**< The algorithmic parameter for setting the
                                * maximum number of different *dual* solutions
   * that the Solver should attempt to obtain and store. Since dual solutions
   * can be a "big" objects (in some cases, much bigger than primal sones),
   * storing them may be costly. It may be therefore helpful for a CDASolver
   * to know beforehand how many different dual solutions the user would like
   * to get. Among the reasonable values for this parameter, 1 says "I don't
   * care of multiple dual solutions, give me only the best one", and 0 says
   * "I don't care of dual solutions at all, just tell me if there is any, and
   * what its value is". The default is 1. */

  intLastParCDAS    ///< first allowed parameter value for derived classes
                    /**< convenience value for easily allow derived classes
                     * to further extend the set of types of return codes */
  };             // end( int_par_type_CDAS )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" dbl_par_type_S to a specific case of CDASolvers

 enum dbl_par_type_CDAS {
  dblRAccDSol = dblLastAlgPar ,  ///< maximum relative error in dual solution
                                 /**< The algorithmic parameter for setting
                                  * the relative accuracy of the accepted dual
   * solutions. It instructs the CDASolver not to even consider a solution
   * among the ones to be reported (see kMaxDSol) if its objective function
   * value is "too" bad. If the original problem is a minimization one, its
   * dual is a maximization one. Hence, the objective function value of a dual
   * solution provides a lower bound "lb" on the optimal dual value (which
   * may, or may not, be equal to that of the primal problem, see
   * is_dual_exact() below). Assuming an upper bound "ub" on the *optimal*
   * value of the *dual* problem has been found (note that this can be
   * obtained by means of the objective value of a feasible *primal*
   * solution, see get_ub()), a solution is deemed acceptable with the
   * provided parameter \eps if
   *
   *    ub - lb <= \eps * max( abs( ub ) , abs( lb ) , 1 )
   *
   * Note that if no ub is available, the above formula can be replaced with
   *
   *    fbest - lb <= \eps * max( abs( fbest ) , abs( lb ) , 1 )
   *
   * where fbest is the value of the best (with largest objective value)
   * solution found so far. The roles of ub and lb are suitably reversed if
   * the primal is a maximization problem, so that the dual is a minimization
   * one. The default is Inf< OFValue >. */

  dblAAccDSol ,     ///< maximum absolute error in any dual solution
                    /**< Similar to dblRAccDSol but for an *absolute*
		     * accuracy; that is, a dual solution is deemed
   * acceptable with the provided parameter \eps if
   *
   *    ub - lb <= \eps
   *
   * or
   *
   *    fbest - lb <= \eps
   *
   * with the same notation as in dblRAccDSol and the same provisions about
   * the case of a maximization problem. The default is Inf< OFValue >. */

  dblFAccDSol ,    ///< maximum absolute error in any dual solution
                   /**< The algorithmic parameter for setting the maximum
                    * relative allowed violation of *dual* constraints,
   * assuming of course something like that exists in the specific dual that
   * is being dealt with. Whenever the CDASolver is incapable of finding
   * feasible dual solutions (maybe because there is none), it may still be
   * useful that it returns the "least unfeasible" ones. This parameter
   * instructs the CDASolver not to even consider a solution among the ones to
   * be reported (see intMaxDSol) if its violation is "too" bad. The actual
   * meaning of this parameter is necessarily CDASolver-dependent;
   * intuitively, it may be thought to work as the "relative constraint
   * violation" feas_epsilon of the Block [see Block.h] if the concept of
   * "dual constraint" is applicable. A setting of 0 may be taken as a way to
   * tell the CDASolver not to bother to produce unfeasible solutions at all,
   * which is why this is the default value of the parameter. */

  dblLastParCDAS    ///< first allowed parameter value for derived classes
                    /**< convenience value for easily allow derived classes
                     * to further extend the set of types of return codes */
  };             // end( dbl_par_type_CDAS )

/*--------------------------------------------------------------------------*/
 /// public enum "extending" str_par_type_S to a specific case of CDASolvers

 enum str_par_type_CDAS {
  strLastParCDAS = strLastAlgPar
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of string parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vint_par_type_S to a specific case of CDASolvers

 enum vint_par_type_CDAS {
  vintLastParCDAS = vintLastAlgPar
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vdbl_par_type_S to a specific case of CDASolvers

 enum vdbl_par_type_CDAS {
  vdblLastParCDAS = vdblLastAlgPar
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double parameters. */
  };

/*--------------------------------------------------------------------------*/
 /// public enum "extending" vstr_par_type_S to a specific case of CDASolvers

 enum vstr_par_type_CDAS {
  vstrLastParCDAS = vstrLastAlgPar
  ///< first allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string parameters. */
  };

/** @} ---------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
 /** @name Constructor and destructor
     @{ */

 /// constructor: does nothing special

 CDASolver( void ) : Solver() {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: it is virtual, and it is empty

 ~CDASolver() override = default;

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Accessing dual solutions and dual unbounded directions
 * The methods in this section allow to retrieve dual solution information
 * generated (at least, conceptually) during the last call to compute().
 * Calls to to any of these methods are therefore associated with the state
 * of Block (comprised the value of the Variable not belonging to it but
 * appearing in its Constraint, see the discussion about compute()) at the
 * moment in which compute() was last called; hence, it is expected that the
 * state is the same. In other words, these methods are "extensions" of
 * compute(), used to extract further (dual) information (likely) computed
 * there, and therefore in principle an extension to the fundamental rule
 * regarding compute() is to be enforced:
 *
 *   between a call to compute() and all the calls to any of these methods
 *   intended to retrieve dual information about solutions computed in that
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

/*--------------------------------------------------------------------------*/
 /// tells whether a dual solution is available
 /** Called after compute() this method has to return true if a dual solution
  * is available to be read with get_dual_solution(). For a Solver using
  * duality (i.e., a CDASolver), it is often the case that a(n approximately
  * optimal) dual solution is "automatically" available as a by-product of
  * compute(); hence, this method is typically (but not necessarily) true
  * after the end of compute().
  *
  * Once "the first" solution (if ever) has been read, new ones may be
  * produced, if the Solver allows it, by means of new_dual_solution().
  *
  * The default implementation in the base class always returns true, which
  * is OK, for instance, for "easy" problems that are never unbounded and can
  * surely be solved "efficiently enough". */

 [[nodiscard]] virtual bool has_dual_solution( void ) { return( true ); }

/*--------------------------------------------------------------------------*/
 /// returns true if the "current" dual solution is feasible
 /** After a call to has_dual_solution() and/or new_dual_solution() that
  * returned true, this method can be used to check whether or not the dual
  * solution is feasible. Note that this can be called before that the
  * solution is actually written into the Block (in whatever format the
  * actual dual in question requires) via get_dual_solution(), as a CDASolver
  * is assumed to know this. However, the method can also be called after
  * that get_dual_solution() is called, returning the same value (until
  * new_dual_solution() is called again and it returns true).
  *
  * Note that a user can know the value of the objective function of the same
  * dual solution by calling the appropriate one between get_lb() and
  * get_ub(). This, however, is *only* true for *feasible* solutions, as only
  * these provide valid lower/upper bounds on the optimal value.
  *
  * This virtual method is provided a simple implementation always returning
  * true, which is OK for a solver that always only returns dual feasible
  * solutions (if any). */

 [[nodiscard]] virtual bool is_dual_feasible( void ) { return( true ); }

/*--------------------------------------------------------------------------*/
 /// write the "current" dual solution in (the Constraint of ?) the Block
 /** After a call to has_dual_solution() and/or new_dual_solution() that
  * returned true, this method can be used to have the dual solution actually
  * written in the Block. Note that the base CDASolver class makes no
  * assumption about how exactly a dual solution is shaped, only that the
  * specific derived classes from Constraint and/or Variable involved have
  * some support to hold this information (e.g., in terms of dual variables
  * attached to the Constraint). Any previous dual solution (or direction,
  * see get_dual_direction()) is typically lost for good, as presumably the
  * interested :Block class does not provide any mechanism to retrieve it at
  * a later time. However, it could presumably allow to save it beforehand in
  * an appropriate Solution object, see Solution.h.
  *
  * It is an error to call this method if has_dual_solution() and
  * new_dual_solution() have not been called and returned true (which means,
  * in particular, if no Block is attached to this CDASolver).
  *
  * See the comments to Solver::get_var_solution() about the need of
  * lock()-ing the Block prior to calling this method, which of course apply
  * verbatim here.
  *
  * Note that, while the largest burden of producing a dual solution is
  * typically bore by compute() and/or new_dual_solution(), it is still
  * possible that "decoding" the internal information of the CDASolver in
  * order to produce one in the required format may be somewhat costly. This
  * is in particular true because a dual solution may involve a rather large
  * amount of data. In some cases, not all that data is actually necessary,
  * as only "a part" of the dual solution migt be enough (say, that which is
  * required to price one given family of variables). This is why support is
  * offered in the method to only retrieve "a part" of the current dual
  * solution by means of a (pointer to a) Configuration object. The full
  * generality of a Configuration is required because the dual solution of a
  * Block comprises that of all its sub-Block (recursively), so an
  * arbitrarily large tree-shaped data structure may be required to pin down
  * the relevant parts in all this. The default is nullptr, which has to be
  * intended as "the whole dual solution".
  *
  * Not coincidentally, all methods in Block that concern solutions, i.e.,
  * get_Solution() and map_[back/forward]_solution(), also take a(n optional)
  * Configuration parameter. Indeed, it should be expected that, for Solver
  * accessing to the "physical representation" of the Block (and, therefore,
  * knowing exactly which :Block it exactly is), the "format" of the
  * Configuration objects should be the same, although this is not a strict
  * requirement (as there may be some reasons not to do that). Note that it
  * can as well be expected that some of these Configuration refer the
  * "primal" part of the solution rather then to the dual one, and some to
  * both parts at once. Hence, it would be nice if the Configuration of the
  * :Block were arranged so that the "primal and dual parts" be neatly
  * separated, so that the same Configuration object can be passed to
  * get_var_solution() and get_dual_solution() to do the right job; yet, this
  * is necessarily left to the specific :Block and :Solver. General-purpose
  * Solver using the "abstract representation" cannot possibly follow this
  * rule; yet, these Solver can still be instructed to only read "a part" of
  * the dual solution, for instance specified in terms of which of the
  * "groups" of Constraint of the Block, as returned by
  * get_static_constraints() and get_dynamic_constraints(), need have their
  * corresponding dual information changed.
  *
  * As a consequence to the fact that get_dual_solution() can retrieve only
  * "a part" of the solution information, it may make sense to call it more
  * than once for each call to has_dual_solution() or new_dual_solution()
  * that returns true, if each call specifies for "a different part" via a
  * different Configuration. Note that, if a different "part" of a dual
  * solution is read after that another one has been previously read, it is
  * intended that the previous part remains in place. This means that the
  * "full" dual solution can eventually be retrieved piecemeal with a finite
  * number of calls to get_dual_solution() with appropriate Configuration(s),
  * although in this case a unique call with nullptr will probably be more
  * efficient. Also, note that the "parts" that two different Configuration
  * specify for may in principle have nonempty intersection. This, however,
  * means that the common part may be written identically twice, with an
  * unjustified performance hit, since the Solver is not required to (although
  * it might, if it so chooses) keep track of which "parts" of the dual
  * solution have been retrieved already; hence, this is better avoided. */

 virtual void get_dual_solution( Configuration * solc = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if it is possible to generate a new dual solution
 /** This method must be called each time the user of the Solver wants that
  * it produces a *new dual* solution.
  *
  * This method can only be called after the compute() method of this Solver
  * has been invoked at least once, and if has_dual_solution() has already
  * returned true. The method has to return true if a dual solution that is
  * *different* from the previously available one is available. This method
  * returning false means that the CDSolver has no means of producing any
  * other dual solution for good, so it does not make sense to call it again
  * unless something has changed in the problem encoded in the Block [see the
  * discussion in compute()] or, in case the last call to compute() has
  * returned kStopTime or kStopIter, compute() is called again. For a typical
  * algorithm that only produces one dual solution, this method should always
  * return false; indeed, this is what the default implementation of the
  * method does.
  * 
  * The user can restrict the set of generated dual solutions, both in number
  * (see intMaxDSol) and in terms of their quality (see dblRAccDSol and
  * dblAAccDSol) or feasibility (see dblFAccDSol). In particular, note that
  * the CDASolver may be instructed (via dblFAccSol) to also return
  * *unfeasible* dual solutions if it has produced any (which may be the case
  * if, for instance, the models is dual infeasible, and therefore no feasible
  * dual solution can ever be found). This means in particular that it is in
  * principle sensible to call this method even if compute() has returned a
  * value not necessarily implying that a dual solution has been found (like
  * kLowPrecision or kError), and even if it is guaranteed that no dual
  * solution exists, i.e., either kUnbounded or kBothInfeasible.
  *
  * Note that a call to new_dual_solution() may be costly, in that the
  * CDASolver  is only supposed to return true if it is *certain* to be able
  * to produce a new dual solution, which typically means having in fact
  * produced it. However, the dual solution is not really written in the
  * Block until get_dual_solution() is called, so part of the cost of
  * producing it may in fact be bore by that method.
  *
  * While producing its dual solutions, it is expected that a CDASolver
  * should:
  *
  * - return first feasible dual solutions, if any, and only after having
  *   exhausted them start returning infeasible ones;
  *
  * - return feasible dual solutions, at least roughly, in order of their
  *   objective function values, i.e., the better ones first (and in
  *   particular, the absolute best solution ever found as absolute first);
  *
  * - return infeasible dual solutions, at least roughly, in order of their 
  *   feasibility (the "least unfeasible ones first", with some appropriate
  *   global measure of infeasibility) and/or in order of their objective
  *   function values (the better ones first), although the objective value
  *   of an unfeasible dual solution may not make too much of a sense, so
  *   that feasibility should be considered first (with objective value used
  *   e.g. to break ties).
  *
  * A special use of this method is after that compute() has returned
  * kInfeasible and set_unbounded_threshold() has been called; see the
  * general comments. */

 [[nodiscard]] virtual bool new_dual_solution( void ) { return( false ); }

/*--------------------------------------------------------------------------*/
 /// tells whether a dual unbounded direction is available
 /** A call to compute() that returned kInfeasible supposedly mean that the 
  * CDASolver can guarantee that it is impossible to find any feasible
  * solution to the primal problem. This typically (but not always, see
  * kBothInfeasible) means that it is possible to find feasible *dual*
  * solutions whose value is "better" than any given threshold. For models
  * with underlying convex structure, this typically means that there
  * exist an unbounded [a/de]scent direction in the dual space, i.e., a
  * direction along which one may go forever without leaving the feasible
  * region of the dual and so that the dual objective function is
  * [in/de]creasing indefinitely along it. This gives both a convenient 
  * *certificate of unboundedness* of the dual, and a convenient *certificate
  * of infeasibility* for the primal. Hence, the user may be interested in
  * accessing one (or more) of these directions. This can be done implicitly
  * and indirectly via set_unbounded_threshold() [see the general comments],
  * but a more direct representation may be useful.
  *
  * Called after compute() that returned kInfeasible, this method has to
  * return true if an unbounded dual [a/de]scent direction is available to be
  * read with get_var_direction(). It is often the case that such a direction
  * is "automatically" available as a by-product of compute() whenever the
  * latter returns kInfeasible; hence, this method is typically (but not
  * necessarily) true after the end of compute() whenever kInfeasible is
  * returned.
  *
  * Once "the first" direction (if ever) has been read, new ones may be
  * produced, if the Solver allows it, by means of new_dual_direction().
  *
  * The default implementation in the base class always returns false, which
  * is OK for Solver that cannot produce any dual unbounded direction. */

 [[nodiscard]] virtual bool has_dual_direction( void ) { return( false ); }

/*--------------------------------------------------------------------------*/
 /// write the "current" dual unbounded direction in the Block
 /** After a call to has_dual_direction() and/or new_dual_direction() that
  * returned true, this method can be used to have the direction actually
  * written in the Block. Because dual directions are in the same space of
  * dual solutions, the method assumes that the dual direction can be
  * suitably represented with the same scheme that is used for dual solutions
  * [see get_dual_solution()] and is done with it. Note that the value of any
  * dual solution or dual direction previously stored in the Block is 
  * typically lost for good, as presumably the interested :Block class does
  * not provide any mechanism to retrieve it at a later time. However, it
  * could presumably allow to save it beforehand in an appropriate Solution
  * object, see Solution.h.
  *
  * One possible use for such a direction is to construct a dual solution
  * with "arbitrarily good" (large for a primal minimization problem whose
  * dual is a maximization one, large vice-versa) value of the objective
  * function. However, this typically requires first constructing a feasible
  * dual solution, and then modifying it by means of the direction. Because
  * of the fact that the Block is only supposed to hold one dual solution at
  * a time, this is not exactly trivial to attain with the tools provided
  * here. However, the method set_unbounded_threshold() of the base Solver
  * class exactly caters for this need.
  *
  * It is an error to call this method if has_dual_direction() and
  * new_dual_direction() have not been called and returned true (which
  * means, in particular, if no Block is attached to this CDASolver).
  *
  * See the comments to Solver::get_var_solution() about the need of
  * lock()-ing the Block prior to calling this method, which of course apply
  * verbatim here.
  *
  * Note that, while the largest burden of producing a dual direction is
  * typically bore by compute() and/or new_dual_direction(), it is still
  * possible that "decoding" the internal information of the CDASolver in
  * order to produce one in the proper format that the Block requires may be
  * somewhat costly. This is why, as in get_dual_solution(), a (pointer to a)
  * Configuration object can be passed to specify that only "a part" of the
  * direction need to be retrieved. See get_dual_solution() for more comments
  * about this parameter; here we just mention that the format of the
  * Configuration for that method need not necessarily be the same as the
  * format of the Configuration for this one, although of course it is
  * somehow nice if this is true. Note, however, that the original :Block
  * may already have specific Configuration for dual directions, to be used in
  * its solution-related methods get_Solution() and
  * map[forward/back]_solution(). Hence, for Solver using the "physical
  * representation" of the Block, the set of Configuration of the Block may
  * be "neatly partitioned" between this method, get_dual_solution(), and
  * their primal counterparts get_var_direction() and get_var_solution(). Yet,
  * likely the Block will have configurations that specify for both primal and
  * dual information together; hence, it would be nice if the Configuration of
  * the :Block were arranged so that the "primal and dual parts" be neatly
  * separated, so that the same Configuration object can be passed to the
  * separate [CDA]Solver methods dealing with the two parts. Yet, note that
  * this likely applies only to get_var_solution() and get_dual_solution() 
  * and not to get_*_direction(), as the existence of an unbounded direction
  * in a problem means that the dual one is empty, and therefore it does not
  * make sense to retrieve dual solution information for that. However, both
  * problems may be empty, so it might be in principle possible that
  * unbounded rays exist for both which do not imply that they are both
  * unbounded because "the ray has nowhere to start from". To see that,
  * consider the pair of dual Linear Programs
  *
  *  (P)  max  -1 * x_1 + 0 * x_2
  *                 x_1 +     x_2 <= -1
  *               - x_1 -     x_2 <=  0
  *
  *  (D)  min  -1 * y_1 + 0 * y_2
  *                 y_1 -     y_2 = -1
  *                 y_1 -     y_2 =  0
  *                 y_1   ,   y_2 >= 0
  *
  * Clearly, both problems are empty. Yet, the coefficient matrix of the
  * primal
  *
  *    A =  |  1   1 |
  *         | -1  -1 |
  *
  * has a the linearity direction d = [ -1 , 1 ]. This means that A * d = 0;
  * hence, if there were any feasible solution x for (P) (which there is
  * not), then x + \alpha * d would be feasible for all \alpha >= 0. Also,
  * c * d > 0 (for c = [ -1 , 0 ] the objective function vector of (P) =
  * right-hand side of (D)), hence (P) would be unbounded above, were it not
  * empty. Symmetrically, the direction v = [ 1 , 1 ] has the property that
  * v * A = 0 and v >= 0, which means that if there were any feasible
  * solution y for (D) (which there is not), then y + \alpha * v would be
  * feasible for all \alpha >= 0. Also, b * v < 0 (for b = [ -1 , 0 ] the
  * objective function vector of (D) = right-hand side of (P)), which means
  * that (D) would be unbounded below were it not empty. Thus, getting both
  * primal and dual directions would be possible, although this is a rather
  * "exotic" case.
  *
  * The method is given a default implementation in the base CADSolver class
  * doing nothing, for solvers that cannot produce any unbounded dual
  * direction. */

 virtual void get_dual_direction( Configuration * dirc = nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// returns true if is possible to generate a new dual unbounded direction
 /** This method must be called each time the user of the CDASolver wants
  * that it produces a *new* dual unbounded direction.
  *
  * This method can only be called after the compute() method of this Solver
  * has been invoked at least once, has reported kInfeasible, and if
  * has_dual_direction() has already returned true. The method has to return
  * true if an unbounded dual [a/de]scent direction that is *different* from
  * the previously returned one is available. This method returning false
  * means that the CDASolver has no means of producing any other dual
  * unbounded direction for good, so it does not make sense to call it again
  * unless something has changed in the problem encoded in the Block [see
  * the discussion in compute()] or, in case the last call to compute() has
  * returned kStopTime or kStopIter, compute() is called again. For a typical
  * algorithm that only produces at most one dual unbounded [a/de]scent
  * direction, this method should always false; indeed, this is what the
  * default implementation of the method does.
  *
  * Note that a call to new_dual_direction() may be costly, in that the
  * CDASolver is only supposed to return true if it is *certain* to be able
  * to produce a new dual direction, which typically means having in fact
  * produced it. However, the dual direction is not really written to the 
  * Block until get_dual_direction() is called, so part of the cost of
  * producing it may in fact be bore by that method.
  *
  * There is no default assumption about the order in which unbounded dual
  * directions are generated by the CDASolver, although each one may have
  * some reasonable definition of an appropriate notion of "better direction"
  * that might be employed.
  *
  * The method is given a default implementation in the base CADSolver class
  * always returning false, for solvers that cannot produce any dual
  * unbounded direction */

 [[nodiscard]] virtual bool new_dual_direction( void ) { return( false ); }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE CDASolver ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the state of the CDASolver
 *  @{ */

 /// returns true if the dual problem is an "exact" one
 /** This method returns true if the dual problem is an "exact" one. That
  * is, being v(P) the optimal value of the primal (minimization) problem and
  * v(D) the optimal value of the dual (maximization) problem, this method
  * returns true only if it is theoretically guaranteed (which, usually,
  * means that it is also practically true) that
  *
  *       v(D ) == v(P)      (strong duality)
  *
  * holds. Note that
  *
  *       v(D ) <= v(P)      (weak duality)
  *
  * is always assumed to hold, regardless to the return value of this method.
  * Of course, the sign of the inequality is reversed if the primal is a
  * maximization problem.
  *
  * Most often, the return value of this method will be constant for a whole
  * class of problems, i.e., for a CDASolver irrespectively of the data of
  * the actual problem to be solved: a class of problems either has a strong
  * dual, or it has not. However, in some cases strong duality may depend on
  * the instance of the model (e.g., if some appropriate constraint
  * qualification holds).
  *
  * The method is given a default implementation in the base CDASolver class
  * catering the very lucky case in which the dual is exact. */

 [[nodiscard]] virtual bool is_dual_exact( void ) const { return( true ); }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the CDASolver
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( idx_type( intLastParCDAS ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  return( idx_type( dblLastParCDAS ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  return( par == intMaxDSol ? int( 1 ) : Solver::get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  if(( par == dblRAccDSol ) || ( par == dblAAccDSol ) )
   return( Inf< double >() );

  if( par == dblFAccDSol )
   return( 0 );

  return( Solver::get_dflt_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  return( name == "intMaxDSol" ? intMaxDSol
	                       : Solver::int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  if( name == "dblRAccDSol" )
   return( dblRAccDSol );
  if( name == "dblAAccDSol" )
   return( dblAAccDSol );
  if( name == "dblFAccDSol" )
   return( dblFAccDSol );

  return( Solver::dbl_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
 const override {
  static const std::string par = "intMaxDSol";

  return( idx == intMaxDSol ? par : Solver::int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
 const override {
  static const std::vector< std::string > pars =
   { "dblRAccDSol", "dblAAccDSol", "dblFAccDSol" };

  if( ( idx >= dblRAccDSol ) && ( idx <= dblFAccDSol ) )
   return( pars[ idx - dblRAccDSol ] );

  return( Solver::dbl_par_idx2str( idx ) );
  }

/** @} ---------------------------------------------------------------------*/

 };   // end( class CDASolver )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* CDASolver.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File CDASolver.h ----------------------------*/
/*--------------------------------------------------------------------------*/

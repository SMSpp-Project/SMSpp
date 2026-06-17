/*--------------------------------------------------------------------------*/
/*-------------------------- File ChangeSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* classes ChangeSolver and RelaxationSolver,
 * which extend the Solver concept [see Solver.h] with the notions needed by
 * an enumerative (Branch-and-Bound) algorithm:
 *
 * - a ChangeSolver is a Solver that can apply() a Change [see Change.h] to
 *   the Block it is attached to - and, symmetrically, the undo Change that
 *   apply() returns - so that the same Solver object can be efficiently
 *   moved between the nodes of an enumeration tree;
 *
 * - a RelaxationSolver is a ChangeSolver that solves a *relaxation* of the
 *   problem encoded by the Block: besides the relaxation value (a valid
 *   dual bound), it can produce *true* solutions of the original problem
 *   (valid primal bounds, see get_true_lb() / get_true_ub()) and, foremost,
 *   it can branch(): produce the set of Changes that generate the children
 *   of the current node.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Federica Di Pasquale \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Federica Di Pasquale, Filippo Magi,
 *                    Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ChangeSolver
 #define __ChangeSolver
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Change.h"

#include "Configuration.h"

#include "Solver.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- NAMESPACE & USING -----------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ChangeSolver_CLASSES Classes in ChangeSolver.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ChangeSolver -----------------------------*/
/*--------------------------------------------------------------------------*/
/// a Solver that can apply a Change to the Block it is attached to
/** The class ChangeSolver extends Solver with the ability of applying a
 * Change to the Block it is attached to (and of un-doing it, see apply()).
 * This is the basic capability that an enumerative algorithm requires of
 * the Solver it drives: moving between two nodes of the enumeration tree
 * amounts to applying the Changes found along the path joining them. The
 * inheritance from Solver is virtual so that a concrete :ChangeSolver can
 * derive both from a problem-specific :Solver and from (Change)Solver-
 * extending interfaces with a single Solver sub-object. */

class ChangeSolver : public virtual Solver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 ChangeSolver( void ) : Solver() {}    ///< constructor: does nothing

 ~ChangeSolver() override = default;   ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

 /// apply the given Change to the Block managed by this Solver
 /** Apply the given Change to the Block managed by this Solver. If
  * \p doUndo is true, the returned (pointer to a newly minted) Change is
  * the one that un-does \p chg, i.e., applying it brings the Block (and
  * the internal state of the Solver) back to the state prior to the call;
  * ownership of the returned Change is transferred to the caller. With
  * \p doUndo false, nullptr is returned.
  *
  * The default implementation just forwards to Change::apply() on the
  * Block; derived classes can intercept the Changes they understand and
  * apply them *internally* (i.e., to their own state, without touching the
  * Block), which is the key for an efficient enumeration. */

 virtual Change * apply( Change * chg , bool doUndo = false ) {
  return( chg->apply( f_Block , doUndo ) );
  }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 /// return the Solution of the Block corresponding to the current solution
 /** Writes the current solution into the Block (under lock) and returns the
  * corresponding newly minted Solution object, whose ownership is
  * transferred to the caller. Derived classes able to construct the
  * Solution directly out of their internal state can (and should) bypass
  * the Block entirely. */

 virtual Solution * get_Solution( Configuration * solc = nullptr ) {
  // TODO: check that lock()-ing / unlock()-ing here is appropriate
  f_Block->lock( this );
  get_var_solution( solc );
  auto sol = f_Block->get_Solution( solc );
  f_Block->unlock( this );
  return( sol );
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( ChangeSolver ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS GlobalInformation --------------------------*/
/*--------------------------------------------------------------------------*/
/// the search-global information an enumerative Solver shares with relaxations
/** The information that is global to a whole Branch-and-X search and that the
 * enumerative Solver makes available to each :RelaxationSolver it drives [see
 * RelaxationSolver::set_global_information()], so that the latter can do
 * preprocessing, reduced-cost fixing or cut / column management on its own
 * terms inside compute() / branch() rather than through an externally driven
 * protocol. It carries the incumbent - the value of the best feasible
 * solution found so far, the natural cutoff for reduced-cost arguments - and,
 * optionally, the globally-valid cuts (and, in perspective, columns).
 * Information local to a node, i.e. fixings valid only in a subtree, travels
 * in the branching Change instead, not here. */

class GlobalInformation {

 public:

 GlobalInformation( void ) : f_incumbent( nullptr ) ,
                             f_local_fixing_allowed( true ) {}

 /// destructor: deletes the owned global cuts
 virtual ~GlobalInformation() {
  for( auto cut : f_global_cuts )
   delete cut;
  }

/*--------------------------------------------------------------------------*/
 /// bind the incumbent to the enumerative Solver's live best-bound cell
 /** The enumerative Solver calls this once, passing (the address of) the cell
  * where it keeps the value of its best feasible solution: the incumbent is
  * then read live through it, with no need to push every update here. */

 void bind_incumbent( const Solver::OFValue * cell ) { f_incumbent = cell; }

 /// whether a feasible incumbent is available
 [[nodiscard]] bool has_incumbent( void ) const {
  return( f_incumbent && ( *f_incumbent != Inf< Solver::OFValue >() ) &&
                         ( *f_incumbent != - Inf< Solver::OFValue >() ) );
  }

 /// the value of the incumbent, in the natural sense of the original problem
 /** Only meaningful if has_incumbent() is true. It may be read out of any
  * lock by a :RelaxationSolver - a slightly stale value only makes a
  * reduced-cost fixing marginally less aggressive, never wrong - as the
  * enumerative Solver updates the bound cell under its own incumbent lock. */
 [[nodiscard]] Solver::OFValue incumbent( void ) const {
  return( *f_incumbent );
  }

/*--------------------------------------------------------------------------*/
 /// whether incumbent-dependent local fixing is currently allowed
 /** Reduced-cost fixing folded into the branching Change is valid only for
  * the incumbent in force when the Change is generated: it is sound within a
  * single solve, where the incumbent only improves, but not when the
  * enumerative Solver retains the tree across re-solves with different
  * incumbents [see intReoptimize]. The Solver clears this flag in that case,
  * and a :RelaxationSolver must then skip such fixings; it is set by default. */

 [[nodiscard]] bool local_fixing_allowed( void ) const {
  return( f_local_fixing_allowed );
  }

 /// allow or forbid incumbent-dependent local fixing [see local_fixing_allowed]
 void set_local_fixing_allowed( bool a ) { f_local_fixing_allowed = a; }

/*--------------------------------------------------------------------------*/
 /// the globally-valid cuts known so far (valid in every node of the tree)
 [[nodiscard]] const std::vector< Change * > & global_cuts( void ) const {
  return( f_global_cuts );
  }

 /// contribute a globally-valid cut (ownership is taken by this object)
 void add_global_cut( Change * cut ) { f_global_cuts.push_back( cut ); }

/*--------------------------------------------------------------------------*/

 protected:

 const Solver::OFValue * f_incumbent;    ///< the live incumbent cell, or null
 bool f_local_fixing_allowed;            ///< incumbent-dependent fixing OK?
 std::vector< Change * > f_global_cuts;  ///< the globally-valid cuts, owned

 };  // end( class( GlobalInformation ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS RelaxationSolver ---------------------------*/
/*--------------------------------------------------------------------------*/
/// a ChangeSolver solving a relaxation, able to branch()
/** The class RelaxationSolver extends ChangeSolver for the Solver of a
 * *relaxation* of the problem encoded by the Block. The inheritance is
 * virtual: a concrete Solver typically derives both from a problem-specific
 * :ChangeSolver and from RelaxationSolver (a diamond on ChangeSolver), and
 * there must be a single ChangeSolver (hence Solver) sub-object.
 *
 * Besides the value of the relaxation (a valid dual bound for the original
 * problem, available through the standard get_lb() / get_ub()), it may
 * produce *true* solutions of the original problem - see
 * has_true_var_solution(),
 * get_true_lb() / get_true_ub() and get_true_[var_]solution() - and it can
 * branch(): produce the Changes generating the children of the current
 * node of the enumeration tree. */

class RelaxationSolver : public virtual ChangeSolver {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 RelaxationSolver( void ) : ChangeSolver() {}   ///< constructor: nothing

 ~RelaxationSolver() override = default;        ///< destructor: nothing

/*--------------------------------------------------------------------------*/
/*--------------------- METHODS FOR SOLVING THE MODEL ----------------------*/
/*--------------------------------------------------------------------------*/

 /// produce the Changes generating the children of the current node
 /** Given the current state of the Block, return the vector of Changes
  * each of which, applied to it, generates one of the children of the
  * current node of the enumeration tree; the returned vector is not
  * supposed to be empty, and ownership of the Changes is transferred to
  * the caller. The Changes are supposed to be "atomic", i.e., directly
  * applicable via apply().
  *
  * The order of the returned vector is significant: the caller may decide
  * to explore the children in that order, so a RelaxationSolver should
  * return first the Change it deems most promising (say, for a binary
  * variable, "x = 1" before "x = 0" if fixing to 1 changes the relaxation
  * the most). */

 virtual std::vector< Change * > branch() = 0;

/*--------------------------------------------------------------------------*/
 /// possible effects of a Modification on an enumeration tree
 /** Classification of how a Modification impacts the validity of the
  * fencing certificates of an enumeration tree built on this
  * RelaxationSolver [see classify()]: the values are bitwise-composable
  * (eModObjective | eModFeasibility == eModBoth). */

 enum mod_effect {
  eModNothing = 0 ,     ///< the Modification does not touch the model
  eModObjective = 1 ,   ///< only the objective function changed
  eModFeasibility = 2 , ///< only the feasible region changed
  eModBoth = 3 ,        ///< both the objective and the feasible region
  eModEverything = 4    ///< anything else: the tree is invalid
  };

/*--------------------------------------------------------------------------*/
 /// classify the effect of a Modification on an enumeration tree
 /** Tells how the given Modification impacts the validity of an
  * enumeration tree built on this RelaxationSolver [see mod_effect]: e.g.,
  * an eModObjective change leaves every infeasibility certificate valid,
  * so a reoptimizing enumeration only needs to re-evaluate the part of the
  * fenced frontier that was pruned by bound. This is problem-specific
  * knowledge, which is why it lives here and NOT in the enumerative Solver
  * driving this one: the default implementation conservatively claims that
  * the Modification invalidates everything. */

 [[nodiscard]] virtual int classify( const sp_Mod & mod ) {
  return( eModEverything );
  }

/*--------------------------------------------------------------------------*/
 /// give the Solver access to the search-global information
 /** Called by the enumerative Solver before driving this one: it hands a
  * (non-owned) GlobalInformation [see], through which the Solver can read the
  * incumbent and consult or contribute the global cuts and columns. This is
  * how a relaxation gets the global data it needs for the tightenings it may
  * choose to do on its own terms inside compute() / branch() - preprocessing,
  * reduced-cost fixing, cut and column management - without any externally
  * driven protocol: those are internal details of the Solver, and the ones
  * local to a node are folded into the branching Change [see branch()], while
  * the global ones live in the GlobalInformation. nullptr means none is
  * available. */

 virtual void set_global_information( GlobalInformation * gi ) {
  f_global_information = gi;
  }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 /// return a valid lower bound on the optimal value of the TRUE problem
 /** Return a valid lower bound on the optimal objective function value of
  * the *original* (not relaxed) problem, typically the value of a feasible
  * solution produced alongside the relaxation. The default implementation
  * returns -infinity, i.e., no bound. */

 [[nodiscard]] virtual OFValue get_true_lb( void ) {
  return( - Inf< OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// return a valid upper bound on the optimal value of the TRUE problem
 /** Return a valid upper bound on the optimal objective function value of
  * the *original* (not relaxed) problem. The default implementation
  * returns +infinity, i.e., no bound. */

 [[nodiscard]] virtual OFValue get_true_ub( void ) {
  return( Inf< OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// tells whether a true solution of the not-relaxed problem is available
 /** Called after compute(), this method has to return true if a true
  * solution of the original problem (not the relaxed one solved by this
  * RelaxationSolver) is available to be read with
  * get_true_var_solution(). Once "the first" solution (if ever) has been
  * read, new ones may be produced, if the RelaxationSolver allows it, by
  * means of new_true_var_solution().
  *
  * The default implementation always returns true, which is OK, for
  * instance, for relaxations that always produce a feasible rounding. */

 [[nodiscard]] virtual bool has_true_var_solution( void ) {
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if it is possible to generate a new true solution
 /** This method must be called each time the user wants the
  * RelaxationSolver to produce a *new* true solution of the not-relaxed
  * problem, different from the previously available one; it can only be
  * called after compute() and a has_true_var_solution() that returned
  * true. Returning false means that no other solution can be produced for
  * good (unless the Block changes or compute() is called again after a
  * kStopTime / kStopIter). For a typical algorithm producing one solution
  * per solve this method should always return false, which is what the
  * default implementation does. */

 [[nodiscard]] virtual bool new_true_var_solution( void ) {
  return( false );
  }

/*--------------------------------------------------------------------------*/
 /// write the "current" true solution in the Variable of the Block
 /** After a call to has_true_var_solution() and/or
  * new_true_var_solution() that returned true, this method can be used to
  * have the true solution actually written in the Variable of the Block.
  *
  * The same locking rules as Solver::get_var_solution() apply: writing
  * solution information is not a change of the Block (no Modification is
  * issued), but it is a change of its state, so THE Block MUST ALWAYS BE
  * lock()-ED WHEN THIS METHOD IS CALLED, and the lock()-ing MUST NOT BE
  * DONE BY THE METHOD ITSELF (the caller needs the lock to persist while
  * it uses the written solution). The optional Configuration can be used,
  * as customary, to only retrieve "a part" of the solution. */

 virtual void get_true_var_solution( Configuration * solc = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// return the Solution corresponding to the current true solution
 /** After a call to has_true_var_solution() and/or
  * new_true_var_solution() that returned true, this method returns the
  * (newly minted, caller-owned) Solution object corresponding to the
  * current true solution. The default implementation writes the solution
  * into the Block (under lock) and reads it back; derived classes able to
  * construct the Solution directly out of their internal state can (and
  * should) bypass the Block entirely. */

 virtual Solution * get_true_solution( Configuration * solc = nullptr ) {
  // TODO: check that lock()-ing / unlock()-ing here is appropriate
  f_Block->lock( this );
  get_true_var_solution( solc );
  Solution * sol = f_Block->get_Solution( solc );
  f_Block->unlock( this );
  return( sol );
  }

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

 /// the search-global information, nullptr if none [see set_global_information]
 GlobalInformation * f_global_information = nullptr;

/*--------------------------------------------------------------------------*/

 };  // end( class( RelaxationSolver ) )

/** @} end( group( ChangeSolver_CLASSES ) ) --------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* ChangeSolver.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ChangeSolver.h --------------------------*/
/*--------------------------------------------------------------------------*/

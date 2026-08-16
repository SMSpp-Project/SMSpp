/*--------------------------------------------------------------------------*/
/*-------------------------- File ChangeSolver.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* classes ChangeSolver and RelaxationSolver,
 * the "traits" that complement the Solver concept [see Solver.h] with the
 * notions needed by an algorithm that solves a whole *sequence* of related
 * problems, each obtained from the previous one by a Change [see Change.h]:
 *
 * - a ChangeSolver can apply() a Change to the Block it works on - and,
 *   symmetrically, the undo Change that apply() returns - so that the same
 *   object can be efficiently moved along the sequence, possibly applying
 *   the Change *internally* to its own state without touching the Block;
 *
 * - a RelaxationSolver is a ChangeSolver that solves a *relaxation* of the
 *   problem encoded by the Block: besides the relaxation value (a valid
 *   dual bound), it can produce *true* solutions of the original problem
 *   (valid primal bounds, see get_true_lb() / get_true_ub()) and, foremost,
 *   it can branch(): produce the set of Changes that generate the children
 *   of the current node.
 *
 * Neither class derives from Solver: they are traits that a concrete class
 * derives from *alongside* its problem-specific :Solver base, with plain
 * (non-virtual) inheritance and therefore no diamond on Solver. Whoever
 * drives them discovers the traits on a registered Solver by a dynamic_cast
 * cross-cast [see the ChangeSolver class comment].
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

#include "GlobalInformation.h"

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
/// the trait of being able to apply a Change to a Block
/** The class ChangeSolver captures the ability of applying a Change to the
 * Block it works on (and of un-doing it, see apply()). This is the basic
 * capability that any algorithm exploring a discrete set of related problems
 * requires of the objects it drives: an enumerative one moves between two
 * nodes of the enumeration tree by applying the Changes found along the path
 * joining them, but a greedy or a local search algorithm equally moves
 * between two solutions by applying the Change encoding the move.
 *
 * ChangeSolver deliberately does *not* derive from Solver: it is a trait
 * that a concrete class derives from *alongside* its problem-specific
 * :Solver base,
 *
 *     class MySolver : public SomeSolver , public ChangeSolver { ... };
 *
 * with plain inheritance on both sides. This avoids any virtual-inheritance
 * diamond on Solver when a concrete class combines several such traits (or
 * traits with heuristic bases), and makes the trait equally applicable to
 * Solver and CDASolver hierarchies. Whoever drives such an object holds
 * plain Solver pointers (say, those registered to a Block) and discovers
 * the trait with a dynamic_cast cross-cast, which succeeds precisely on
 * objects whose concrete class derives from both:
 *
 *     auto cs = dynamic_cast< ChangeSolver * >( some_solver );
 *
 * (and symmetrically dynamic_cast< Solver * >( some_change_solver ) to go
 * back).
 *
 * The trait is a pure interface: it declares what the driver needs and
 * implements nothing, so that no piece of information that the :Solver base
 * of the concrete class already has (the Block, in the first place) is
 * duplicated here. In particular apply() is pure virtual, and the concrete
 * class implements it out of its own Solver state, which for a solver that
 * has nothing to do internally is just
 *
 *     Change * apply( Change * chg , bool doUndo ) override {
 *      return( chg->apply( f_Block , doUndo ) );
 *      }
 *
 * The only state of the trait is the (non-owned) GlobalInformation, which
 * is a notion of its own [see set_global_information()]. */

class ChangeSolver
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 ChangeSolver( void ) = default;   ///< constructor: does nothing

 virtual ~ChangeSolver() = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

 /// apply the given Change to the Block managed by this ChangeSolver
 /** Apply the given Change to the Block managed by this ChangeSolver. If
  * \p doUndo is true, the returned (pointer to a newly minted) Change is
  * the one that un-does \p chg, i.e., applying it brings the Block (and
  * the internal state of the solver) back to the state prior to the call;
  * ownership of the returned Change is transferred to the caller. With
  * \p doUndo false, nullptr is returned.
  *
  * The simplest implementation forwards to Change::apply() on the Block of
  * the concrete class [see the class comment], but a solver can rather
  * intercept the Changes it understands and apply them *internally*, i.e.,
  * to its own state and without touching the Block, which is the key for an
  * efficient exploration of a sequence of related problems. */

 virtual Change * apply( Change * chg , bool doUndo = false ) = 0;

/*--------------------------------------------------------------------------*/
 /// give the ChangeSolver access to the global information
 /** Called by whoever drives this ChangeSolver: it hands a (non-owned)
  * GlobalInformation [see GlobalInformation.h], through which the solver
  * can read and contribute the information shared by all the cooperating
  * solvers - say, the incumbent, or the globally valid cuts and columns.
  * This is how it gets the global data it needs for the tightenings it may
  * choose to do on its own terms inside compute() / branch() -
  * preprocessing, reduced-cost fixing, cut and column management - without
  * any externally driven protocol: those are internal details of the
  * solver, and the ones local to a node are folded into the branching
  * Change [see RelaxationSolver::branch()], while the global ones live in
  * the GlobalInformation. nullptr means none is available, in which case
  * the solver must work exactly as if the information did not exist. */

 virtual void set_global_information( GlobalInformation * gi ) {
  f_global_information = gi;
  }

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

 /// the global information, nullptr if none [see set_global_information]
 GlobalInformation * f_global_information = nullptr;

/*--------------------------------------------------------------------------*/

 };  // end( class( ChangeSolver ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS RelaxationSolver ---------------------------*/
/*--------------------------------------------------------------------------*/
/// a ChangeSolver solving a relaxation, able to branch()
/** The class RelaxationSolver extends ChangeSolver for the solver of a
 * *relaxation* of the problem encoded by the Block. Like ChangeSolver it
 * is a trait [see the ChangeSolver class comment]: a concrete class
 * derives both from a problem-specific :Solver and from RelaxationSolver,
 *
 *     class MyRelaxation : public SomeSolver , public RelaxationSolver
 *
 * (the inheritance on ChangeSolver is virtual, so that a concrete class
 * reaching the trait through several paths still has a single ChangeSolver
 * sub-object).
 *
 * Besides the value of the relaxation (a valid dual bound for the original
 * problem, available through the standard Solver get_lb() / get_ub() of
 * the concrete class), it may produce *true* solutions of the original
 * problem - see has_true_var_solution(), get_true_lb() / get_true_ub() and
 * get_true_[var_]solution() - and it can branch(): produce the Changes
 * generating the children of the current node of the enumeration tree. */

class RelaxationSolver : public virtual ChangeSolver
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 RelaxationSolver( void ) = default;    ///< constructor: does nothing

 ~RelaxationSolver() override = default;  ///< destructor: does nothing

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
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

 /// return a valid lower bound on the optimal value of the TRUE problem
 /** Return a valid lower bound on the optimal objective function value of
  * the *original* (not relaxed) problem, typically the value of a feasible
  * solution produced alongside the relaxation. The default implementation
  * returns -infinity, i.e., no bound. */

 [[nodiscard]] virtual Solver::OFValue get_true_lb( void ) {
  return( - Inf< Solver::OFValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// return a valid upper bound on the optimal value of the TRUE problem
 /** Return a valid upper bound on the optimal objective function value of
  * the *original* (not relaxed) problem. The default implementation
  * returns +infinity, i.e., no bound. */

 [[nodiscard]] virtual Solver::OFValue get_true_ub( void ) {
  return( Inf< Solver::OFValue >() );
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
  * current true solution. The obvious implementation writes the solution
  * into the Block (under lock) and reads it back with Block::get_Solution(),
  * but a solver able to construct the Solution directly out of its internal
  * state can (and should) bypass the Block entirely. */

 virtual Solution * get_true_solution( Configuration * solc = nullptr ) = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( RelaxationSolver ) )

/** @} end( group( ChangeSolver_CLASSES ) ) --------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* ChangeSolver.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ChangeSolver.h --------------------------*/
/*--------------------------------------------------------------------------*/

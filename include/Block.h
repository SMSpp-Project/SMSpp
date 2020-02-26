/*--------------------------------------------------------------------------*/
/*---------------------------- File Block.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* class Block, which represent the basic
 * concept of a "block" in a block-structured mathematical model.
 *
 * A Block contains some Variable [see Variable.h], some Constraint [see
 * Constraint.h], one Objective [see Objective.h], and some sub-Block;
 * furthermore, it can be contained into a father Block. Variable and
 * Constraint can either be static (i.e., they are guaranteed to be always
 * there throughout the life of the model, although of course the value that
 * the Variable attain is, well, variable) or dynamic (i.e., that may appear
 * and disappear during the life of the model). Conversely, the sub-Block
 * are static, i.e., they cannot individually appear or disappear. Dynamic
 * Variable and Constraint allow to cope with "very large models" by means of
 * column and row generation techniques.
 *
 * A Block can be attached to any number of Solver [see Solver.h], that can
 * then be used to solve the corresponding mathematical model.
 *
 * Variable and Constraint in a Block can be arranged in any number of
 * "sets", or "groups", each of which can be a multi-dimensional array with
 * (in principle) an arbitrary number of dimensions. The idea is that a model
 * with a specific  structure (say, a Knapsack, a Traveling Salesman Problem,
 * a SemiDefinite program, ...) be represented as a specific derived class of
 * Block. Hence, its Variable and Constraint will be organized into
 * appropriate, "natural" (multi-dimensional) vectors, and will be accessed as
 * such by specialized Solver that can exploit the specific structure of the
 * Block. Actually, the Variable and Constraint can be represented implicitly
 * by just providing the data that characterizes them (the weights and costs
 * of the item in a knapsack, an annotated graph, the size of a square
 * semidefinite matrix, ...), and a specialized Solver will only need access
 * to that data (characterizing the instance of the problem) to be able to
 * solve the Block. We call this the "physical representation" of the Block.
 * This means that the Constraint may not even need to be explicitly
 * constructed, as specialized Solver already "know of{ them. Conversely,
 * the Variable will always have to be constructed, as they are the place
 * where Solver will write the solution of the Block once they have found it.
 *
 * However, a Block can also be attached to general-purpose solvers that only
 * need the Variable and Constraint to be of some specific type (say, single
 * real numbers and linear functions ...). Hence, the base Block class
 * provides a mechanism whereby, upon request, the Block "exhibits" its
 * Variable and Constraint as "unstructured" lists of (multi-dimensional
 * arrays of) Constraint and Variable; we call this the "abstract
 * representation" of the Block.
 *
 * A Block supports "lazy" modifications of all its components: each time a
 * modification occurs, an appropriate Modification object [see
 * Modification.h] is dispatched to all Solver "interested" in the Block,
 * i.e., either directly attached to the Block or attached to one of its
 * ancestors. Hence, the next time they are required to solve the Block they
 * will know which modifications have occurred since the last time they have
 * solved it (if any) and be able to react accordingly, hopefully
 * re-optimizing to improve the efficiency of the solution process. Each
 * Solver is only interested in the Modification that occurred after it was
 * (indirectly) attached to the Block and since the last time it solved the
 * Block (if any), but it has the responsibility of cleaning up its list of
 * Modification. The specific classes BlockMod, BlockModAdd and BlockModRmv
 * are also defined in this file to contain all Block-specific Modification.
 *
 * Block can "save" the current status of its Variable into a Solution object
 * [see Solution.h], and read it back from a Solution object. If Constraint
 * have dual information attached to them, this can similarly be saved.
 *
 * Block explicitly supports the notion that a model may need to be modified
 * for algorithmic purposes, i.e., by producing either a Reformulation (a
 * different Block that encodes a problem whose optimal solutions are optimal
 * also for the original Block), a Relaxation (a different Block whose optimal
 * value provides valid global lower/upper bounds for the optimal value of the
 * original Block, assuming that was a minimization/maximization problem,
 * while hopefully being easier to solve), or a Restriction (a different Block
 * that encodes a problem whose feasible region is a strict subset of that of
 * the original Block, which hopefully makes it easier to solve). These are
 * called "R3 Block" of the original Block. The set of R3 Block of a given
 * Block is defined by the Block itself; the base class provides no general
 * R3 Block. However, since one of the basic design decisions of SMS++ is that
 * "names" of Variable (and Constraint) are their memory address, it is not
 * in general possible to "copy Variable" (a new Variable will always be a
 * different Variable for any existing one). Therefore, the Block class
 * provides support from the fact that an original Block can map back solution
 * information produced by one of its R3 Blocks. This operation is, again,
 * specific for each Block and R3 Block of its, and the base class provides
 * no general mechanism for it (besides the interface).
 *
 * \version 0.33
 *
 * \date 09 - 01 - 2020
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
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato, Kostas
 * Tavlaridis-Gyparakis
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Block
 #define __Block /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"
#include "Configuration.h"
#include "Observer.h"
#include "Solver.h"

#include <boost/bimap.hpp>
#include "netcdf"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Variable;            // forward definition of Variable

 class Constraint;          // forward definition of Constraint

 class Objective;           // forward definition of Objective

 class Solution;            // forward definition of Solution

 class BlockConfig;         // forward definition of BlockConfig

 class BlockSolverConfig;   // forward definition of BlockSolverConfig

/*--------------------------------------------------------------------------*/
/*------------------------- Block-RELATED TYPES ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_TYPES Block-related types
 *  @{ */

 typedef Block * p_Block;
 ///< a pointer to Block

 typedef std::vector<p_Block> Vec_Block;
 ///< a vector of pointers to Block

 typedef const std::vector<p_Block> c_Vec_Block;
 ///< a vector of pointers to Block

 typedef Vec_Block::iterator Vec_Block_it;
 ///< iterator for a Vec_Block

/** @}  end( group( Block_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_CLASSES Classes in Block.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Block ---------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// the cornerstone of the SMS++ system: a class for "a block in a model"
/** The Block class is the cornerstone of the SMS++ system. It is meant to
 * represent the basic concept of a "block" in a block-structured
 * mathematical model. In general a Block contains:
 *
 * - any number of Variable [see Variable.h];
 *
 * - any number of Constraint [see Constraint.h];
 *
 * - one Objective [see Objective.h];
 *
 * - any number of sub-Block.
 *
 * Furthermore, it is possibly contained into a father Block. Variable and
 * Constraint can either be static (i.e., they are guaranteed to be always
 * there throughout the life of the model, although of course the value that
 * the Variable attain is, well, variable) or dynamic (i.e., that may appear
 * and disappear during the life of the model). Conversely, the sub-Block of
 * a given Block are assumed to be always static, i.e., their number and
 * type cannot change. One of the main design decisions in SMS++ is that the
 * "name" of a Variable or Constraint is just its memory address; this means
 * that, once constructed, they cannot be moved to a different memory
 * location. Hence, the difference between static and dynamic stuff is that
 * only the former can live into arrays, provided that the arrays are
 * *never shortened or lengthened* (for doing so may cause the memory to be
 * re-allocated, which would change the address violating the basic
 * assumption). Conversely, dynamic stuff can only live into lists, so that
 * elements can be added or removed without causing other elements to be
 * reallocated.
 *
 * Yet, both static and dynamic Variable and Constraint in a Block can be
 * arranged in any number of "groups", each of which can be a
 * multi-dimensional array with (in principle) an arbitrary number of
 * dimensions. For dynamic Variable/Constraint, only the last dimension can
 * by varying. Block relies onto boost::any to be able to handle vector of
 * variables with an arbitrary number of indices, and to boost::multi_array
 * to implement them. Tools are provided so that both (multi-dimensional
 * vectors of lists of) *any possible derived class* from
 * Variable/Constraint, as well as (multi-dimensional vectors of lists of)
 * *pointers* to *any possible derived class* from Variable/Constraint, can
 * be handled.
 *
 * Block is a *base* class for representing the general concept of "a part of
 * a mathematical model with a well-understood identity"; in other words, it
 * is expected that it will be used to define *derived* classes, each of which
 * represents a model with a specific structure (say, a Knapsack, a Traveling
 * Salesman Problem, a SemiDefinite program, ...). Hence, the Variable and
 * Constraint of such a specific model will be organized into appropriate,
 * "natural" (multi-dimensional) vectors. Actually, the Constraint/Objective
 * can then be represented *implicitly* by just providing the data that
 * characterizes them (the weights and costs of the item in a knapsack, an
 * annotated graph, the size of a square semidefinite matrix, ...). Such data
 * (characterizing the instance of the problem) will be available as
 * fields/methods of the derived class. We call this the "physical
 * representation" of the Block.
 *
 * The reason for wanting to define a Block is to have the corresponding
 * mathematical model solved. For this reason, the general interface class
 * Solver [see Solver.h] is defined. The idea is that a Block any number of
 * *appropriate* Solver -- i.e., ones that are able to solve the
 * corresponding mathematical model -- can be attached to the Block. All
 * Solver are specialized, i.e., they are only able to solve a Block with
 * specific characteristics; this is necessarily so because, basically, the
 * base Block is completely generic and can represent anything, and there is
 * no Solver that can solve "anything". So, a Solver may only accept to be
 * attached to specific sub-classes of Block. Hence, the specific data for
 * that specific sub-class will be visible to the Solver, which may in
 * principle only need it, and *not* the Constraint/Objective of the Block,
 * to solve the corresponding problem. Thus, the base Block class supports
 * the notion that the constraints and objective may not necessarily be
 * explicitly defined (in terms of the corresponding Constraint/Objective
 * objects. Conversely, Variable are in principle always needed, as they are
 * a crucial part of the interface between the user of the Block and the
 * Solver: the latter write its solutions into the Variable of the Block, so
 * that the user of the Block can read them. However, any specialized :Block
 * can also have a specialized way to represent its solution, and a user of
 * that particular :Block can conceptually only use that. Indeed, the status
 * at any point in time of the solution of a Block can be saved into a
 * Solution object [see Solution.h], and read it back from a Solution object;
 * this need not necessarily be expressed in terms of Variable (although of
 * course it has to be written in the Variable if they are defined). Since
 * Constraint can have dual information attached to them, this can similarly
 * be saved.
 *
 * However, some Solver can be "general-purpose", i.e., that only need the
 * Variable and Constraint/Objective to be of some specific type (say, single
 * real numbers and affine functions, ...). Hence, the base Block class
 * provides a mechanism whereby, upon request, the Block "exhibits" its
 * Constraint/Objective and Variable as an "unstructured" list of
 * [multi-dimensional arrays of] [lists of] [pointers to] objects of class
 * (derived from) Constraint/Objective and Variable; we call this the
 * "abstract representation" of the Block. This representation somewhat
 * "abstracts away" from the very specific structure of the problem, allowing
 * to see it as just an "unstructured" instance of some very general class.
 *
 * Variable and Constraint/Objective in a Blocks are in principle dynamic
 * (even static ones), in the sense that, at the very least, Variable can take
 * different values and can be fixed/unfixed, while Constraints can be
 * relaxed/enforced. Because several Solver can be attached to a Block at the
 * same time, changes in its components are handled in a "lazy" way. That is,
 * each time a change occurs, an appropriate Modification object [see
 * Modification.h] is dispatched to the Block, which is why Block derives from
 * Observer. Other classes, like FRowConstraint and FRealObjective are
 * :Observer in order to allow Function to pass them Modification that are
 * ultimately dispatched to the Block. Block in turn dispatches them to all
 * "interested" Solver, so that the next time they are required to solve the
 * Block they know which changes have occurred since the last time they have
 * solved it, and therefore are able to react accordingly (hopefully
 * re-optimizing to improve the efficiency of the solution process). A Solver
 * is "interested" in a Modification only if the latter occurs either in the
 * Block to which it is attached or in one of its sub-Block (recursively), and
 * only if is has occurred after it was attached to the Block and since the
 * last time it solved the Block. A Solver has the responsibility of always
 * cleaning up its list of Modification, because it is passed *smart pointers*
 * and therefore a Modification is only deleted when the last "interested"
 * Solver it has been passed to deletes its smart pointer. A Modification can
 * refer to either the "physical representation" or the "abstract
 * representation" of a Block, which means that actually a single change may
 * cause more than one Modification to be issued to a Solver. It is assumed
 * that a Solver will know which of the equivalent Modification are of its
 * interest and disregard the others.
 *
 * The fact that a Block has *two* representations, the "abstract" one and the
 * "physical" one, means that there are actually two different ways to perform
 * what is conceptually the same change:
 *
 * - via a call to some specialized method of the :Block interface (say,
 *   change_weights() for a KnapsackBlock);
 *
 * - by accessing the corresponding abstract representation (say, the
 *   LinearFunction inside the FRowConstraint representing the knapsack
 *   constraint) and changing it.
 *
 * Doing the first is supposed to also change the abstract representation (if
 * it has been constructed, which it may not have). However, this must happen
 * also in the second case. For this reason, Modification are explicitly
 * characterized between "physical" ones and "abstract" ones. The second
 * correspond to changes that happen to the abstract representation: when the
 * (abstract) Modification object is passed to the :Block, the latter has to
 * "intercept" it and, before shipping it to its attached Solver and ancestors,
 * has to "reflect" the changes to the "abstract" representation into ones to
 * the "physical" one (if any). Note that this may not be possible, or be too
 * difficult so that the :Block may not support it: which "abstract"
 * Modification (changes in the :Block issued by directly accessing the
 * "abstract" representation) are supported is entirely a :Block decision.
 *
 * This underlines how there are two different mechanisms to change the data
 * of a :Block:
 *
 * - one that requires knowing exactly which :Block this is, in order to
 *   access its specialized interface;
 *
 * - one that is "completely general", in that it only accesses the
 *   "abstract" representation of the :Block.
 *
 * As such, the second is more general; however, it has drawbacks. First of
 * all it requires the "abstract" representation to be constructed, which may
 * not be necessary if this is not used by the attached Solver(s). Also, it
 * may be difficult to make some problem-specific changes via the "abstract"
 * representation, which therefore may limit which changes the :Block support.
 * Finally, the mechanism is somehow less efficient in that it relies on the
 * :Block to "intercept" and process "abstract" Modification. To ease all
 * these drawback, a third mechanism is provided which allows to access the
 * specialized interface of the :Block without having to know at compile time
 * its exact type. This is the "methods factory" mechanism, whereby the :Block
 * can register into some factories its data-changing functions; these can then
 * be retrieved by name (a string that can be pulled out some Configuration at
 * runtime) and called, without the caller knowing which specific :Block they
 * belong to at compile time. This is helped by some degree of standardization
 * between the data-changing functions of a :Block; a small number of "default"
 * parameter type lists are defined that are directly supported by the base
 * Block class, although the methods factory mechanism is fully generic and can
 * be easily extended to any other function type.
 *
 * It is important to clearly state the *ownership* of Constraint and
 * Variable, which is that they are directly defined either in the Block,
 * or in any of its sub-Block (recursively). This has important consequences,
 * related to the fact that a Constraint defined in a Block may contain
 * Variable that the Block does not own. Conversely, some Variable of the
 * Block can be active in a Constraint that is not owned by the Block. The
 * intended semantic of these cases are:
 *
 * - If a Constraint defined in a Block contains a Variable that the Block
 *   does not own, at the time when the Block is solved they have to be
 *   treated as *constants*, with the value that they currently have. Note
 *   that this is a specific case where the data of the corresponding
 *   mathematical problem changes, but there is no Modification that signals
 *   this to any Solver attached to the Block. This means that a Solver
 *   needing to know about this will have to "manually" check whether or not
 *   a change occurred.
 *
 * - A Constraint not owned by the Block is irrelevant to the Block even if
 *   it contains Variable owned by the Block; it has to be thoroughly ignored.
 *   Paradoxically, such a Constraint may contain *only* Variables owned by
 *   the Block, and therefore logically belonging to it, but still it has to
 *   be ignored.
 *
 * - All, and only, the Constraint owned by the Block have to be taken into
 *   account when defining what a feasible solution is. All, and only, the
 *   Variable owned by the Block can be changed by the Solver.
 *
 * It is now necessary to comment on the Objective. First of all, having only
 * one Objective would seem to prevent a Block from representing a
 * multi-objective optimization problem. However this is not true, as the base
 * Block class (and the base Objective class) make no stipulations about the
 * return value of the Objective. By having the Objective to produce, say, a
 * vector of reals (or any other complex data structure with a partial
 * ordering) rather than a single one, it is easy to represent multi-objective
 * optimization problems. Of course this means that the concept of "optimal
 * solution" becomes that of "Pareto-optimal solutions". This should not be
 * much of an issue as Solver are well equipped already for producing multiple
 * (approximate) solutions, since this is anyway useful even for
 * single-objective optimization, although admittedly how the Pareto-optimal
 * solutions (which may be infinitely many) are produced is still not clearly
 * stipulated by the Solver interface. Perhaps a MOSolver class will be
 * needed. Anyway, both Block and Solver are somewhat slanted towards the
 * single-objective case where the value of the Objective is a single real
 * value. Indeed, both Solver and Block have mechanism,s (see e.g. the methods
 * Block::get_valid_lower_bound(), Block::get_valid_upper_bound(),
 * Solver::get_lb() and Solver::get_ub(), and the Solver parameters dealing
 * with cutoffs) that make reference to upper and lower bounds on the optimal
 * value of the Block, intended as a single real number. These mechanisms
 * would not be appropriate for the case where the Block actually encodes for
 * (and, therefore, the Solver  needs to solve) multi-objective optimization
 * problems, but so far no better solution has been found.
 *
 * Since a Block has a set of sub-Block, each of which has its own
 * Objective, it actually has "many" objectives. It is therefore crucial to
 * define how the Objective of the sub-Block "contribute" to defining the
 * "total" objective of the problem represented by the whole Block (the father
 * one plus the sub-Block, recursively). The principle is simple: the total
 * objective of a Block is given by the *sum* of its Objective and of all the
 * Objective of the sub-Block (recursively).
 *
 * This principle need some commenting. First of all, it says that whatever
 * is the return value of the Objective, it must admit a sum operation
 * (this seems a quite minimal requirement, although in the multi-objective
 * case it means that the number of different objectives must be the same in
 * the father Block and all its sub-Block). Also, the sum is just one of the
 * very many composition operations that may conceivably be used to define
 * the objective function of the father Block in terms of the objective
 * functions of its sub-Block. However, it is in principle always possible to
 * keep as the objective function of each sub-Block only the terms of the
 * "total" objective function that only depend on variables of the given
 * sub-Block; if there is none, the objective function of the sub-Block can
 * be set as null (constantly 0). This indeed makes good sense, as if the
 * objective function contains a term (say) f( x , y ) where x belongs to one
 * sub-Block and y to a different one, then it is more natural (although
 * not strictly necessary) to define it in the father Block rather than in
 * either one of the sub-Block. In fact, it is natural (although not strictly
 * necessary) that each sub-Block only defines its Constraint and Objective
 * in terms of the Variable owned by the sub-Block itself, leaving any
 * "linking term" (be them linking Constraint or terms in the Objective with
 * Variable not owned by the sub-Block) to be defined in the father Block.
 * This argument may be countered by saying that in some cases it may be
 * useful to move these terms inside the sub-Block, but this is not really
 * an issue due to the fact that a Block can always "reformulate itself" so
 * as to move around any linking terms to the place that any specific
 * algorithm requires; see the discussion about the R3 Blocks below. Hence,
 * assuming the sum as the default (only) composition is very natural and
 * does not prevent using any arbitrarily complex overall objective function.
 *
 * A further detail need explicit discussion, though. An Objective, besides
 * the actual function, also has a sense. This means that there are actually
 * two rather different cases:
 *
 * - the Objective of the father Block and that of the sub-Block have *the
 *   same* sense (say, min-min);
 *
 * - the Objective of the father Block and that of the sub-Block have
 *   *different* senses (say, min-max).
 *
 * The first case is completely obvious: the sub-Block is "just a part of
 * the father Block". This can be stated as follows: if one removes everything
 * from the sub-Block and moves it to the father Block, the described
 * problem remains exactly the same.
 *
 * The second case is rather different, though. On the outset, all previous
 * definitions remain valid: each Variable in the sub-Block is owned by the
 * father Block (which means that a Solver attached to the father Block can
 * change it as it sees fit), and the Objective of the sub-Block is one term
 * of that of the father Block. However, being this a (say) min-max setting,
 * this means that for any solution (considering all the Variable owned by the
 * father Block, which include those owned by the sub-Block) to be
 * considered optimal, *the variables of the sub-Block must be maximizing its
 * Objective*. Note that the value of the inner Objective is still comprised
 * in that of the father Block, which is instead minimized. This allows to
 * naturally define classical min-max settings, such as
 * \f[
 *   min { yb + max { ( c - yA ) x : x \in X } : y \in Y }
 * \f]
 * that should be familiar to many users. However, it is important to remark
 * that in this case removing everything from the sub-Block and moving it to
 * the father Block does *not* define an equivalent problem, as it is
 * immediately seen by considering the difference between the above and
 * \f[
 *   min { yb + ( c - yA ) x : x \in X , y \in Y }
 * \f]
 * An apparently unfortunate consequence of this choice is that, while neatly
 * handling min-max settings, it would seem to entirely prevent a Block
 * representing multi-level optimization problems, where sub-Blocks may be
 * (in the multi-level parlance) "followers", and therefore have to optimize
 * a completely unrelated objective function from that of the father Block.
 * This is not true, however. In particular, multi-level optimization can be
 * made possible by defining a constraint like OptimalWRT( g ), where g is a
 * (possibly, vector-valued) Objective (and, therefore, includes an
 * optimization sense). For an sub-Block having such a Constraint, the only
 * feasible solutions are those that optimize w.r.t. the Objective g Hence,
 * the Objective of the sub-Block can still be used to define a part of the
 * "total" objective function, with OptimalWRT( g ) defining the
 * "follower objective function".
 *
 * Another very important, feature of Block is that it explicitly supports
 * the notion that a model may need to be modified for algorithmic purposes.
 * There are, on the outset, three different kinds of modified Blocks that
 * are typically useful:
 *
 * - A Reformulation, i.e., a different Block that encodes a problem whose
 *   optimal solutions are optimal also for the original Block, while being
 *   for some reason "more convenient to solve" by some specific algorithm.
 *   Note that Reformulations may be defined over a completely different
 *   space of Variable, provided that some appropriate mapping can be
 *   defined between the original and reformulated space. The mapping need
 *   not be algebraic, but must obviously be algorithmic.
 *
 * - A Relaxation, i.e., a different Block whose optimal value provides a
 *   valid global lower bound (for a minimization problem, upper bound for a
 *   maximization one) on the optimal value of the original Block. This is
 *   typically obtained by having a larger feasible region (e.g., relaxing
 *   some Constraint) and/or an Objective whose value is smaller than the
 *   original one (for a minimization problem, larger for a maximization one)
 *   on the original feasible region. One also expects that the Relaxation is
 *   easier to solve than the original Block. Usually, a(n algorithmic) map
 *   between solutions of the Relaxation and those of the original problem is
 *   also available, although solutions of the Relaxation may clearly not be
 *   feasible for the original problem (but they may as well be; in this
 *   case, if the Objective value also coincides, such a solution is actually
 *   optimal for the original problem).
 *
 * - A Restriction, i.e., a different Block whose optimal value provides a
 *   valid global upper bound (for a minimization problem, lower bound for a
 *   maximization one) on the optimal value of the original Block. This is
 *   typically obtained by having a smaller feasible region (e.g., adding
 *   some Constraint, as in fixing some Variable) and/or an Objective whose
 *   value is larger than the original one (for a minimization problem,
 *   smaller for a maximization one) on the original feasible region. One
 *   also expects that the Restriction is easier to solve than the original
 *   Block. Again, a(n algorithmic) map between solutions of the Restriction
 *   and those of the original problem is also usually available, and these
 *   solutions are most often feasible for the original problem.
 *
 * These are called "R3 Blocks" of the original Block. The set of R3 Blocks
 * of a given derived class from Block is defined by the derived class itself;
 * the base class provides no general R3 Block. However, due to the basic
 * design decision about "names" of Variable (and Constraint), it is not
 * in general possible to "copy Variable". Therefore, the Block class
 * provides support from the fact that an original Block can map back solution
 * information produced by one of its R3 Block, and, vice-versa, map the
 * solution information currently stored in the Block to one of its R3 Block.
 * These operations are, again, specific for each derived class from Block
 * and its R3 Blocks of its; the base class provides no general mechanism for
 * this (besides the interface). Also, since the mapping between the original
 * Block and one of its R3 Block can be arbitrarily complex, any Modification
 * occurring to the original Block may require a rather complex set of
 * different Modification to achieve the same "logical" effect on the R3
 * Block, and vice-versa. Therefore, the Block class defines a general
 * interface for a Block to "apply the equivalent to a given Modification of
 * its to a R3 Block", or "map a Modification occurring in a R3 Block to a
 * set of Modification for the original Block". The overall R3 mechanism
 * should be able to support a very large class of reformulation techniques.
 *
 * Finally, the base Block class also supports a number of minor but still
 * relevant needs:
 *
 * - saving current solution information, comprised dual information if
 *   available, to a Solution object [see Solution.h] and reading it back;
 *
 * - changing all the relevant parameters governing the Block behaviour in
 *   one blow by means of a single BlockConfig object;
 *
 * - changing all the attached Solver and their algorithmic configurations in
 *   one blow by means of a single BlockSolverConfig object;
 *
 * - printing the model in a human-readable form;
 *
 * - to help in the above, allow Variable and Constraint to be given
 *   human-meaningful "names";
 *
 * - providing basic information (assumed constant throughout the Block)
 *   about the acceptable thresholds in Constraint violation.
 */

class Block : public Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// public enum for defining the level of verbosity of the print methods
 enum verbosity_type {
  silent = 0 ,  ///< no output at all
  low ,         ///< very low verbosity
  medium ,      ///< a little bit more verbose
  high ,        ///< rather verbose, but still human-readable
  complete      ///< a full Block-description file (for derived classes)
                /**< no longer (necessarily) human-readable, it is supposed to
		 * allow a (class derived from) Block to save its entire
                 * contents to a file, in a format amenable to be read back
		 * with the operator>>() / load() method. This is alternative
		 * to the serialization(), which should be preferred unless
		 * there are strong reasons for wanting a standard file
		 * instead of a netCDF one (say, compatibility with legacy
		 * instance formats). */
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** @name Types for the "methods factory"
 *
 * The "methods factory" (more properly, methods factor*ies*) is a map between
 * strings and pointer to functions that could be used, for example, to modify
 * the data of a given :Block. There are in principle as many factories as
 * there are function types, although a factory only exists if someone
 * registers at least a function in it (cf. register_methods()). However, for
 * the methods factories to be useful, only relatively few different function
 * types should reasonably be used, so that some high degree of modularity is
 * achieved between different :Block. This is why the base Block class defines
 * (and hardly ever uses) a bunch of types that are intended to provide the
 * basis for most of the functions in the interface of derived classes:
 *
 * - Index, an index into any internal data structure;
 *
 * - Range, a pair of indices ( start , stop ) representing the typical
 *   left-closed, right-open range { i : start <= i < stop };
 *
 * - Subset, an arbitrary subset of indices (currently a simple
 *   std::vector< Index >, although this may change) together with a
 *   [const] iterator [c_]Subset_it in it;
 *
 * - MF_dbl_it, a const_iterator into a std::vector< double >;
 *
 * - MF_int_it, a const_iterator into a std::vector< int >;
 *
 * Also defined here are types useful for the registration process itself:
 *
 * - FunctionType (variadic template), a std::function with the function type
 *   of all function that should go in the methods factory;
 *
 * - MemberFunctionType (variadic template), the type of the class member
 *   functions corresponding to the type dictated by FunctionType;
 *
 * - arg_packer_helper and arg_packer (variadic template), helper types for
 *   template shenanigans for methods factory;
 *
 * - The six types MS[_D]_S with D in { dbl , int } (or not there) and S in
 *   { rngd , sbst } representing six standard parameter type lists for
 *   functions to be inserted in the methods factory.
 *  @{ */

 /// an index in any internal data structure of the Block
 using Index = unsigned int;
 using c_Index = const Index;                 ///< a const Index

 /// a pair of indices for the "range" functions in the methods factory
 using Range = std::pair< Index , Index >;
 using c_Range = const Range;                 ///< a const Range

 /// a vector of indices for the "subset" functions in the methods factory
 using Subset = std::vector< Index >;
 using c_Subset = const Subset;               ///< a const Subset

 using Subset_it = Subset::iterator;          ///< iterator in Subset
 using c_Subset_it = Subset::const_iterator;  ///< const iterator in Subset

 /// iterator for double data received by the functions in the methods factory
 using MF_dbl_it = std::vector< double >::const_iterator;

 /// iterator for int data received by the functions in the methods factory
 using MF_int_it = std::vector< int >::const_iterator;

 /// typedef for functions to be added to the methods factory
 /** Items added to the methods factory should typically be (pointers to)
  * std::functions (usually adapter functions for some :Block member function)
  * that take a Block * first, two c_ModParam (for "physical" and "abstract"
  * Modification, respectively) at the end, and in the middle as many parameters
  * as they want. */

 template< typename ... Args >
 using FunctionType =
  std::function< void ( Block * , Args ... , c_ModParam , c_ModParam ) >;

 /// typedef for class member functions to be added to the methods factory
 /** The class member functions (whose adapters are to be) added to the methods
  * factory should take two c_ModParam (for "physical" and "abstract"
  * Modification, respectively) at the end, before them as many parameters as
  * they want, and be functions of some \p dBlock derived from Block; it is
  * clearly "the same parameter type list" as FunctionType< Args > (with the
  * same Args) for a function of the given \p dBlock. */

 template< class dBlock , typename ... Args >
 using MemberFunctionType =
       void ( dBlock::* ) ( Args ... , c_ModParam , c_ModParam );

 /// helper type for template shenanigans for methods factory
 template< typename ... >
 struct arg_packer_helper { };

 /// type for packing variadic lists (template shenanigans for methods factory)
 template< typename ... Args >
 struct arg_packer { using args = arg_packer_helper<Args...>; };

 /// type for ( void , range ) functions
 using MS_rngd = arg_packer< Range >;

 /// type for ( double , range ) functions
 using MS_dbl_rngd = arg_packer< MF_dbl_it , Range >;

 /// type for ( int , range ) functions
 using MS_int_rngd = arg_packer< MF_int_it , Range >;

 /// type for ( void , subset ) functions
 using MS_sbst = arg_packer< Subset && , const bool >;

 /// type for ( double , subset ) functions
 using MS_dbl_sbst = arg_packer< MF_dbl_it , Subset && , const bool >;

 /// type for ( int , subset ) functions
 using MS_int_sbst = arg_packer< MF_int_it , Subset && , const bool >;

/**@} ----------------------------------------------------------------------*/
/*------------------------------- FRIENDS ----------------------------------*/
/*--------------------------------------------------------------------------*/

 // currently, none

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------ CONSTRUCTING AND DESTRUCTING Block --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Block
 *  @{ */

 /// constructor of Block, taking a pointer to the father Block
 /** Constructor of Block. It accepts a pointer to the father Block
  * (defaulting to nullptr, both because the root Block has no father and so
  * that this can also be used as the void constructor), which may be useful
  * early on to a Block to initialize itself. */

 Block( Block *father = nullptr ) : Observer() ,
  f_at( false ) , verbosity_lvl( low ) , f_BlockConfig( nullptr ) ,
  f_channel( 0 ) , f_Block( father ) , f_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: it is deleted
 /** The copy constructor of Block is deleted (disallowed); copies should be
  * possible via get_R3_Block() if the :Block supports them. */

 Block( const Block & ) = delete;

/*--------------------------------------------------------------------------*/
 /// construct a :Block of specific type using the Block factory
 /** Use the Block factory to construct a :Block object of type specified by
  * classname (a std::string with the name of the class inside). If there is
  * no class with the given name, exception is thrown. The optional parameter
  * father is the pointer to the father Block, that (if provided) is passed
  * to the :Block constructor inside the factory.
  *
  * Note that the method is static because the factory is static, hence it is
  * to be called as
  *
  *       Block *myBlock = Block::new_Block( someclass );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed).
  *
  * Note that the :Block returned my this method is "empty": it contains no
  * (instance) data, and therefore it has to be explicitly initialized with
  * any of the corresponding methods (operator>>, serialize(), anything that
  * the specific :Block class provides) before it can be used.
  *
  * For this to work, each :Block has to:
  *
  * - add the line
  *
  *       SMSpp_insert_in_factory_h;
  *
  *   to its definition (typically, in the private part in its .h file);
  *
  * - add the line
  *
  *       SMSpp_insert_in_factory_cpp_1( name_of_the_class );
  *
  *   to exactly *one* .cpp file, typically that :Block .cpp file. */

 static Block *new_Block( const std::string &classname ,
			  Block *father = nullptr )
 {
  const auto it = Block::f_factory().find( classname );
  if( it == Block::f_factory().end() )
   throw( std::invalid_argument( classname +
			   std::string( " not present in Block factory" ) ) );

  return( (it->second)( father ) );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of a netCDF file
 /** Top-level de-serialization method: takes the filename of a SMS++ netCDF
  * file, opens it in a netCDF::NcFile object, and returns the complete
  * :Block object whose description is *the first one* found in the file. It
  * does so by just forwarding to deserialize( netCDF::NcFile ). If something
  * goes wrong with the entire operation, nullptr is returned. See
  * deserialize( netCDF::NcFile ) for details of the SMS++ netCDF file format.
  *
  * Note that the method is static, hence it is to be called as
  *
  *       Block *myBlock = Block::deserialize( somefile );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed).
  *
  * Besides the newly created Block, the method also returns a && reference to
  * the netCDF::NcFile created, which allows chaining of [de]serializing
  * operations on it. Note the "&&", which comes from the insistence of
  * netCDF C++ interface on never copying netCDF::NcFile / netCDF::NcGroup
  * and always rather transfer ownership to the caller.
  *
  * Note that the :Block returned my this method is clearly not "empty", as
  * opposed as :Block fresh out of the factory (see new_Block()), but is it
  * "un-configured": the "abstract representation" is not constructed (unless
  * the :Block does this by its own volition), both the BlockConfig and the
  * BlockSolverConfig are not set, and (therefore) there are no Solver
  * attached. */

 static Block * deserialize( const char * filename )
 {
  try {
   netCDF::NcFile f( filename , netCDF::NcFile::read );
   return( Block::deserialize( f ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of an open netCDF SMS++ file
 /** Second-level de-serialization method: takes an open netCDF file and the
  * index of a Block into the file, and returns the corresponding complete
  * :Block object.
  *
  * There are three types of SMS++ netCDF files, corresponding to three values
  * of the enum smspp_netCDF_file_type [see SMSTypedefs.h]. Each file, when
  * opened in a netCDF::NcFile (which is also a netCDF::NcGroup), must have an
  * int netCDF attribute "SMS++_file_type" with one of the three values of the
  * enum. The structure of the corresponding files is:
  *
  * - eProbFile: the file (which is also a group) has any number of child
  *   groups with names "Prob_0", "Prob_1", ... In turn, each child group
  *   has exactly three child groups with names "Block", "BlockConfig" and
  *   "BlockSolver", respectively. The first is intended to contain the
  *   serialization of a :Block, the second the serialization of a
  *   :BlockConfig of the same :Block, and the third the serialization of a
  *   :BlockSolverConfig of the same :Block, although any of the three can
  *   in principle be empty. If any of the child is not empty, it must
  *   necessarily contain a string attribute "type" containing the classname()
  *   of the corresponding :Block / :Configuration class, plus of course all
  *   the information necessary to reconstruct the specific instance. Note
  *   that sub-Block of the Block and sub-Configuration of the Configuration
  *   (if any) are assumed each to be contained into a child of the group
  *   containing the original :Block / :Configuration, recursively.
  *
  * - eBlockFile: the file (which is also a group) has any number of child
  *   groups with names "Block_0", "Block_1", ... Each child group contains
  *   the serialization of a :Block (the string attribute "type" and all the
  *   rest).
  *
  * - eConfigFile: the file (which is also a group) has any number of child
  *   groups with names "Config_0", "Config_1", ... Each child group contains
  *   the serialization of a :Configuration (the string attribute "type" and
  *   all the rest).
  *
  * The :Block extracted from the file is specified by the parameter idx:
  * for an eProbFile it is extracted out of the netCDF::NcGroup "Prob_<idx>",
  * while for an eBlockFile it is extracted out of the netCDF::NcGroup
  * "Block_<idx>". Note that a :Block cannot be de-serialized out of a
  * eConfigFile. Trying to do that, as well as if something goes wrong with
  * the entire operation (the file is not there, the "SMS++_file_type"
  * attribute is not there, there is no required "Prob_<idx>" or
  * "Block_<idx>" child group, there is any fatal error during the process,
  * ...) results in nullptr being returned.
  *
  * Note that the method is static, hence it is to be called as
  *
  *       Block *myBlock = Block::deserialize( netCDFfile );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed).
  *
  * What this method does is finding the right child group and forward to
  * new_Block( netCDF::NcGroup ). */

 static Block * deserialize( netCDF::NcFile & f , const unsigned int idx = 0 )
 {
  try {
   netCDF::NcGroupAtt gtype = f.getAtt( "SMS++_file_type" );
   if( gtype.isNull() )
    return( nullptr );

   int type;
   gtype.getValues( & type );

   if( ( type != eProbFile ) && ( type != eBlockFile ) )
    return( nullptr );

   netCDF::NcGroup bg;

   if( type == eProbFile ) {
    netCDF::NcGroup dg = f.getGroup( "Prob_" + std::to_string( idx ) );
    if( dg.isNull() )
     return( nullptr );

    bg = dg.getGroup( "Block" );
    }
   else
    bg = f.getGroup( "Block_" + std::to_string( idx ) );

   return( new_Block( bg ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of netCDF::NcGroup, returns it
 /** Third-level de-serialization method: takes a netCDF::NcGroup supposedly
  * containing a Block, extracts the string attribute "type" out of the
  * netCDF::NcGroup, uses it in the factory to construct the "empty" :Block
  * [see new_Block( string )], and then finally dispatches to
  * deserialize( netCDF::NcGroup , Block * ), which is where the
  * :Block-dependent de-serialization happens. Since the latter method
  * accepts a pointer to the father Block, this one does, too. This method is
  * static (see the previous versions for comments about it) and returns a
  * pointer to the newly minted Block, hence it has to have a different name
  * from deserialize( netCDF::NcGroup , Block * ) (since the signature is the
  * same but for the return type). */

 static Block * new_Block( netCDF::NcGroup & group ,
			   Block * const father = nullptr )
 {
  try {
   if( group.isNull() )
    return( nullptr );

   netCDF::NcGroupAtt gtype = group.getAtt( "type" );
   if( gtype.isNull() )
    return( nullptr );

   std::string blocktype;
   gtype.getValues( blocktype );
   Block * result = new_Block( blocktype , father );
   result->deserialize( group );
   return( result );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize the current :Block out of netCDF::NcGroup
 /** Fourth and final level de-serialization method: takes a netCDF::NcGroup
  * supposedly containing all the information required to de-serialize the
  * Block, and initialize the current Block out of it.
  *
  * The format of a group containing a :Block must be the following:
  *
  * - the mandatory string attribute "type" that contains the classname() of
  *   the :Block, which is actually useful at the higher levels of the
  *   deserialize() hierarchy where the :Block has already to be constructed,
  *   rather than at this point where it clearly already has;
  *
  * - the optional string attribute "name" that contains the name() of this
  *   particular instance of :Block (typically of no algorithmic value, but
  *   potentially very useful to more easily keeping track of what the
  *   different parts of a Block mean); if "name" is not present, an empty
  *   name() results.
  *
  * - whatever other information is required by the specific :Block.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * and in fact it is virtual. The format of the information is clearly that
  * set by the serialize( netCDF::NcGroup ) method of the specific :Block
  * class, and exception should be thrown if anything goes wrong in the
  * process.
  *
  * If there is any Solver "interested" to this Block, then a NBModification
  * *must* be issued to "inform" them that anything it knew about the Block is
  * now completely outdated. This is *not* optional (and therefore no issueMod
  * param is provided), because the reaction of a Solver to an NBModification
  * should be akin to clearing the list of all previous Modification. Indeed,
  * since these are no longer relevant and, worse, they may refer to elements
  * of the Block that simply no longer exist; thus, they cannot possibly be
  * processed in any meaningful way, which is why the NBModification cannot be
  * avoided. This is unless the Block is only a sub-Block of the Block that
  * the Solver is solving, in which case Modification pertaining to other
  * parts of the Block still are relevant; see the comments to
  * Solver::add_Modification. Note that the NBModification is sent to the
  * "default channel", since it "must be seen immediately" rather then being
  * "hidden" into any GroupModification.
  *
  * It is also important to remark that
  *
  *      AFTER deserialize() THE :Block IS UN-CONFIGURED
  *
  * Although clearly not "empty", as opposed as :Block fresh out of the
  * factory (see new_Block( string )), a freshly loaded Block is otherwise
  * "in pristine state": the "abstract representation" is not constructed
  * (unless the :Block does this by its own volition), both the BlockConfig
  * and the BlockSolverConfig are not set, and (therefore) there are no Solver
  * attached, unless there were before. The eProbFile SMS++ netCDF file type
  * is precisely provided for allowing to save *all* the information required
  * to solve a Block (the Block itself, its BlockConfig and its
  * BlockSolverConfig), but de-serializing the Configuration and applying
  * them is still the user's responsibility.
  *
  * The method of the base class actually ignores the "type" attribute, on
  * the grounds that that is thought to be used as input to the factory to
  * create the object (as in new_Block( netCDF::NcGroup )); once the :Block
  * has been constructed, which has necessarily already happened when this
  * method is called, the value is irrelevant (one could only check if
  * "type" agrees with classname() and throw exception otherwise). So, what
  * the method currently does is just to handle the optional "name"
  * attribute. It does *not* handle the sub-Block, because there can
  * hardly be any reasonably general way in which they can be structured
  * (there can be different groups of sub-Block with different properties).
  * By not even trying, we can leave in this method only things that are
  * sensible for each and every :Block. Because of this
  *
  *     THE deserialize() METHOD OF ANY :Block SHOULD CALL
  *     Block::deserialize()
  *
  * While this currently does so little that one might well be tempted to
  * skip the call and just copy the three lines of code, enforcing this
  * standard is forward-looking since in this way any future revision of the
  * base Block class may add other mandatory/optional fields: as soon as they
  * are managed by the (revised) method of the base class, they would then be
  * automatically dealt with by the derived classes without them even knowing
  * it happened.
  *
  * An added bonus is that
  *
  *     Block::deserialize() ISSUES THE NBModification
  *
  * and therefore by calling it one is also relieved from the need of doing
  * it explicitly. Note, however, that
  *
  *      THE NBModification SHOULD BE ISSUED AT THE END OF THE CALL, AND
  *      THEREFORE THE CALL TO Block::deserialize() SHOULD BE AT THE END
  *
  * This is due to the rule n. 2 of Modification: when one is issued, the
  * change must have happened already.
  *
  * This is usually not a big deal, except in a case: that where a one has
  * Block2 deriving from Block1 deriving from Block. Here,
  * Block2::deserialize() may need something done in Block1::deserialize()
  * to work, but when Block1::deserialize() calls Block::deserialize() the
  * NBModification is issued.
  *
  * The solution to this is that any :Block that expects to be further derived
  * should provide a guts_of_deserialization() method that does all the work
  * without calling Block::deserialize(), so that it can be called by the
  * methods of the derived classes.
  *
  * A different case is that of an "abstract" :Block, that *must*
  * necessarily be derived from. In this case deserialize() in the base class
  * should not call Block::deserialize(), leaving this to derived ones. */

 virtual void deserialize( netCDF::NcGroup & group )
 {
  netCDF::NcGroupAtt gname = group.getAtt( "name" );
  if( gname.isNull() )
   f_name.clear();
  else
   gname.getValues( f_name );

  // issue a NBModification, the "nuclear option"
  if( anyone_there() )
   add_Modification( std::make_shared<NBModification>( this ) );
  }

/*--------------------------------------------------------------------------*/
 /// destructor of Block: it is virtual
 /** Destructor of Block: it invokes set_BlockConfig() and set_SolverConfig()
  * to clean up the configuration of the Block and all Solver attached to it
  * (and recursively for its sub-Block), hence if there is any Solver that
  * has to survive the Block it has to be manually unregistered from the
  * Block before the latter is deleted. It also cleans up any currently
  * open GroupModification. */

 virtual ~Block() {
  set_BlockConfig();
  set_SolverConfig();
  for( auto ptr : v_current_GroupMod )
   delete ptr;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// setting the "father" Block of this Block
 virtual void set_f_Block( p_Block new_f_Block ) { f_Block = new_f_Block; }

/*--------------------------------------------------------------------------*/
 /// setting the string name of the Block
 /** Sets the string name of the Block. As the && tells, the string becomes
  * property of the Block. */

 virtual void set_name( std::string && name ) { f_name = std::move( name ); }

/*--------------------------------------------------------------------------*/
 /// setting the verbosity level

 void set_verbosity( verbosity_type new_verb_lvl ) {
  verbosity_lvl = new_verb_lvl;
  }

/*--------------------------------------------------------------------------*/
 /// setting the BlockConfig
 /** This method sets the BlockConfig of this Block. The BlockConfig object
  * pointed by newBC is not copied, but it becomes property of the Block.
  * This means that if there already is a BlockConfig of the Block, it gets
  * deleted during the call.
  *
  * Since a BlockConfig has a vector of sub-BlockConfig for the sub-Block of
  * this Block, it calls itself recursively to set them. Note that this means
  * that there are at typically *two* "live" pointers to the same
  * BlockConfig, one in the Block itself and one in the BlockConfig object of
  * its father Block, if any.
  *
  * Hence, a call to set_BlockConfig() (with default = nullptr newBC) deletes
  * the BlockConfig to this Block and all of its sub-Block, recursively. Note
  * that the vector of sub-BlockConfig is allowed to be of different size
  * than the number of sub-Block; if it is larger any extra BlockConfig is
  * simply ignored, if it shorted then all missing sub-BlockConfig are
  * treated as nullptr. This means that, for instance, it is possible to set
  * the BlockConfig of the father Block while resetting that of its sub-Block
  * by just leaving the vector of sub-BlockConfig empty.
  *
  * Note, however, that changing (or deleting) a BlockConfig is only
  * automatically safe if done top-down from the "root" Block (the one having
  * no father Block), as changing the BlockConfig of a sub-Block leaves a
  * dangling pointer in the BlockConfig of the father Block. This is the
  * reason for the second parameter of the method. If safe == true, the
  * method can assume that the BlockConfig (if any) of the father Block (if
  * any) has already been updated. If not, the method will have to climb up
  * (provided there is anywhere to climb) and update it (provided there is
  * anything to update). */

 virtual void set_BlockConfig( BlockConfig *newBC = nullptr ,
			       const bool safe = true );

/*--------------------------------------------------------------------------*/
 /// incrementally changing information in the BlockConfig
 /** A BlockConfig is a complicated object; in general, one may want to just
  * "change a few bits of it" rather than setting an entirely new
  * BlockConfig. This is what this method is for.
  *
  * The trick is that the provided BlockConfig aBC is interpreted "in a
  * differential sense". This means that every field of aBC that is "void"
  * (either nullptr or empty) is interpreted as "leave the current value as it
  * is", whereas a nonempty value causes the current value in the BlockConfig
  * to be replaced with the one in aBC. Note that when pointers to object are
  * involved, the previously pointed object is deleted and replaced with the
  * one in aBC, i.e., those parts of aBC become "property" of the current
  * BlockConfig of the Block. Indeed, the aBC object is "consumed" by this
  * method; at the end, some parts of aBC will have become part of
  * BlockConfig, and all the rest is safely disposed of. Note that if aBC (or
  * one of its sub-BlockConfig) is going to replace a "void" (nullptr)
  * (sub-)BlockConfig, then this is simple because "adding to void is
  * equivalent to substituting": a non-existing BlockConfig is equivalent to
  * one having all nullptr fields, and therefore adding aBC (...) to a "void"
  * BlockConfig is the same as set_BlockConfig( aBC ). In this case, the
  * whole aBC becomes property of the Block, and nothing part it is disposed
  * of. Hence, this has the same problem as set_BlockConfig(), i.e., that if
  * the father of this Block (if any) has a BlockConfig, then it should be
  * updated to reflect the addition of aBC. This is why this method also has
  * the "safe" parameter, although note that a call is surely safe if the
  * BlockConfig of this Block is non-nullptr. */

 virtual void add_to_BlockConfig( BlockConfig *aBC , const bool safe = true );

/*--------------------------------------------------------------------------*/
 /// generate the "abstract representation" of the Variable of the Block
 /** This method serves is to ensure that the "abstract representation" of
  * the Variable, be they static or dynamic, of the Block is initialized,
  * so that it can be read with get_static_variables() and
  * get_dynamic_variables(). For the dynamic ones this may (or may not)
  * imply that the lists are empty, with the dynamic generation being done
  * into generate_dynamic_variables().
  *
  * This method has to be used (rather, *not* used) with caution, because the
  * Variable of a Block are in general "thought to be always there". Indeed,
  * the Variable of a Block are "the interface between the Solver and the user
  * of the Block", since they are where the solution information is written.
  * However, specialized Block may have more compact/efficient ways to
  * represent their solution information (say, in a TSP one could use n
  * integers describing the permutation rather than n^2 binaries), which a
  * Solver may exploit and that a user of that specialized Block may read
  * through the specialized interface. Thus, if only using a :Block via its
  * specialized interface, the "abstract representation" of the Variable of
  * the Block can be avoided.
  *
  * Furthermore, not all Variable of a Block may be "equally important". For
  * instance, only a (maybe, quite small) subset of the Variable may be
  * required for certain uses of the Block, which however logically also has a
  * (much larger) set of "auxiliary" Variable. If the latter are not needed by
  * any of the Solver/users interacting with the Block, it is possible to
  * avoid to construct them at all. This is somehow related to, albeit
  * different from, the fact that certain "types" of Variable may be "many",
  * and therefore necessarily have to be generated dynamically, see
  * generate_dynamic_variables(). Note, however, that "the shape" of the set
  * of abstract Variable of the Block, i.e., the number of elements in the
  * vectors returned by get_static_variables() and get_dynamic_variables()
  * must always be the same: it can only change with the *first* call to this
  * method (i.e., when the vectors are initialized) and never afterwards.
  * Besides, the subset of "groups" of static Variable that are constructed
  *
  *    IS NOT SUPPOSED TO CHANGE OVER THE LIFETIME OF THE Block
  *
  * That is, a Block may be initialized to have less ("abstract") Variable
  * than all the ones it might; if this is the case, all the other ones will
  * *never* be available. This means that calling this method multiple times
  * with different configuration parameters [see below] implying different
  * "groups" of static Variable is not allowed: any such call should either
  * be ignored (once the set of static Variable of the Block is initialized,
  * it is so for good) or throw exception. As a result,
  *
  *    NONE OF THE OPERATIONS IN THIS METHOD SHOULD ISSUE A Modification
  *
  * The idea, again, is that this operation is made once and for all in the
  * lifetime of the object, and never repeated.
  *
  * Note, however, that all Variable appearing in any ("abstract
  * representation" of a) Constraint that is explicitly constructed [see
  * generate_abstract_constraints()] must obviously be constructed, and this
  * *before* the Constraint is. Of course, a derived class can ignore all
  * this and just construct them all right away, but this may be work and
  * memory wasted if no Solver actually uses them. Any Solver should ensure
  * that this method has been called at least once before making any attempt
  * at using the Variable, unless it knows for sure that the Block it is
  * working with has done that already. Note that it is easy to check whether
  * or not generate_abstract_variables() still has to be called by just
  * testing if get_static_variables().size() == 0, although this is a
  * necessary but not sufficient condition, as a Block may not have any
  * static Variable at all.
  *
  * Most expected uses of this method rely on the fact that a Block can have
  * several different types (groups) of static Variable. This is why the
  * method has the parameter stvv, which is a pointer to an arbitrarily
  * complex Configuration object. The actual parameter may be of any specific
  * derived class from Configuration, and contain all the information that
  * specifies which of the types (groups) of static Variable must actually be
  * constructed. If the Block has sub-Block, then the :Configuration object
  * should contain an appropriate :Configuration object for each of these.
  *
  * Note that the stcc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with stcc = nullptr then the
  * corresponding configuration from the BlockConfig is used. Indeed, the
  * method is not pure virtual, but it is given a default implementation
  * doing nothing but calling itself for each sub-Block with no (= default
  * = nullptr) argument. This is correct for those :Block that either have no
  * static Variable at all or that construct them all anyway, and for which
  * all the sub-Block either have the same property or are only to be
  * called with the default configuration set by the BlockConfig. Note that
  * if the BlockConfig is not set (nullptr) or the corresponding field is not
  * set (nullptr), this is assumed to mean "construct all the static Variable
  * that you have, if any". Thus, the default implementation automatically
  * works in the "simple" cases, while for any other case the :Block will
  * have to implement its own version, say "unpacking" its :Configuration
  * object to specific sub-Configuration for each of its sub-Block. This
  * cannot be done in the default implementation because the Configuration
  * stvv for the father Block will likely have to be "unpacked", with each
  * sub-Block getting its own specific sub-Configuration, but this can only
  * be done by a specific :Block for a specific :Configuration, as the base
  * Configuration class does not have direct support for the fact that a
  * Configuration contains a sub-Configuration for each specific sub-Block of
  * a Block. */

 virtual void generate_abstract_variables( Configuration *stvv = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_abstract_variables();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// generate the dynamic Variables of the Block
 /** This method is intended as a hook for dynamic generation of Variable.
  * The idea is that the information available to the Block (e.g. stored in
  * dual variables of its Constraint, if they are defined) may allow the
  * Block to generate more Variable that can be used to get better solutions.
  * This is necessary for Block that have in principle a "very large" number
  * of Variable (say, exponentially many) which therefore can only be
  * generated dynamically.
  *
  * Note that, from a semantic viewpoint, dynamic variables are considered to
  * "be there even they are not there". That is, it is assumed that all the
  * dynamic variables not explicitly generated are implicitly present in the
  * Block set at their default value (most often, zero), and that this does
  * not change the fact that the (explicitly constructed part of the) solution
  * is feasible. Hence, since the dynamic variables can be "many", only those
  * that are actually necessary to encode the optimal solution need be
  * explicitly constructed (although algorithms will typically generate more
  * than these while iteratively searching for the best set: if the latter
  * were known a-priori, it could be encoded as a set of static Variables).
  * This in particular means that if the Block has dynamic variables, any call
  * to is_optimal() cannot rely only on the abstract representation of the
  * Block to produce a correct return value.
  *
  * When this method is called, the Block should attempt at generating new
  * Variable. If the "abstract representation" of these Variable has been
  * constructed [see generate_abstract_variables()], then any newly found
  * Variable will be added to the corresponding list via a call to
  * add_dynamic_variable(), thereby triggering the appropriate (abstract)
  * Modification. This may not be necessary, since a specialized Solver may
  * be able to work with a specialized version of the dynamic variables (say,
  * paths in an appropriate graph). In this case, a specific physical
  * Modification has to be issued that the specialized Solver has to be
  * capable to recognize, thereby ignoring the corresponding abstract
  * Modification. Note that if the "abstract representation" has been also
  * constructed, both types of Modification will have to be issued, with each
  * type of Solver having to figure out which one to react to.
  *
  * Note that any new Variable in the Block typically allows more feasible
  * solutions to exist; hence, in general if dynamic Variable exist, they
  * have necessarily to be generated (priced in) before a solution is
  * certified to be optimal. There can be exceptions for Variable that only
  * reformulate the set of feasible solutions without actually adding it any
  * new element, such as those corresponding to dual-optimal inequalities;
  * this is analogous to the distinction between lazy constraints and valid
  * inequalities.
  *
  * In principle, one Block can have several different types of dynamic
  * Variable; not all of them must necessarily be generated all the time.
  * For instance, some families of dynamic Variable may be cheaper to
  * generate (priced in) than others, and it may make algorithmic sense to
  * give different priorities at different stages of the solution process.
  * Furthermore, even for the same family of dynamic Variable there could be
  * different generation (pricing) procedures (say, heuristic and exact),
  * each with different algorithmic parameters, among which possibly the
  * time that can be spent in the process. Finally, a Block can have several
  * (different) sub-Block, recursively, and dynamic variables for all
  * sub-Block should in principle be generated whenever this method is called
  * for the father Block.
  *
  * For all these reasons, the method has the parameter dyvv, which is a
  * pointer to an arbitrarily complex Configuration object. The actual
  * parameter may be of any specific derived class from Configuration, and
  * contain all the information that is relevant, such as which families of
  * dynamic variables to be priced-in, and which which algorithm. Possibly
  * the :Configuration object may even contain one or more Block with
  * attached Solver to represent and solve the pricing problem, so that
  * the algorithmic parameters of the Solver (say, maximum time and required
  * accuracy) can be used to control the pricing process. Also, if the Block
  * has sub-Block, some of which have dynamic variables, then the
  * :Configuration object should contain an appropriate :Configuration
  * object for each of these.
  *
  * Note that not all "groups" (families) of "abstract" dynamic Variable may
  * have been constructed [see generate_abstract_variables()]. This does *not*
  * mean that those that have not been constructed may not be generated: if
  * this is done, only the "physical representation" of these is generated,
  * and the "abstract" one is not.
  *
  * Note that the dyvv parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with dycc = nullptr then the
  * corresponding configuration from the BlockConfig() is used. Indeed, the
  * method is not pure virtual, but it is given a default implementation
  * doing nothing but calling itself for each sub-Block with no (= default =
  * nullptr) argument. This is correct for those :Block have no dynamic
  * Variable at all, and for which all the sub-Block either have the same
  * property or are only to be called with the default configuration set by
  * the BlockConfig. Note that if the BlockConfig is not set (nullptr) or the
  * corresponding field is not set (nullptr), this is assumed to mean
  * "price-in all the dynamic Variable that you have, if any", which is fine
  * if the pricing process actually has no parameters (say, a single class of
  * dynamic variables with an easy exact pricing algorithm). Thus, the
  * default implementation automatically works in the "simple" cases, while
  * for any other case the :Block will have to implement its own version, say
  * "unpacking" its :Configuration object to specific sub-Configuration for
  * each of its sub-Block. This cannot be done in the default implementation
  * because the Configuration dyvv for the father Block will likely have to
  * be "unpacked", with each sub-Block getting its own specific
  * sub-Configuration, but this can only be done by a specific :Block for a
  * specific :Configuration, as the base Configuration class does not have
  * direct support for the fact that a Configuration contains a
  * sub-Configuration for each sub-Block of a Block. */

 virtual void generate_dynamic_variables( Configuration *dyvv = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_dynamic_variables();
  }

/*--------------------------------------------------------------------------*/
 /// generate the "abstract representation" of the Constraint of the Block
 /** This method serves is to ensure that the "abstract representation" of
  * the Constraint, be they static or dynamic, of the Block is initialized,
  * so that it can be read with get_static_constraints() and
  * get_dynamic_constraints(). For the dynamic ones this may (or may not)
  * imply that the lists are empty, with the dynamic generation being done
  * into generate_dynamic_constraints(). Of course, in order to be able to
  * construct the "abstract representation" of the Constraint one must have
  * already constructed the "abstract representation" of the Variable first,
  * see generate_abstract_variables().
  *
  * Because a (specialized) Solver may not need the description of the Block
  * in terms of its "abstract" Constraint, these may not actually be
  * constructed until this method is called for the first time. Furthermore,
  * not all Constraint of a Block may be "equally important". For instance,
  * only a (maybe, quite small) subset of the Constraint may be required for
  * certain uses of the Block, which however logically also has a (much
  * larger) set of "auxiliary" Constraint. If the latter are not needed by
  * any of the Solver/users interacting with the Block, it is possible to
  * avoid to construct them at all. This is somehow related to, albeit
  * different from, the fact that certain "types" of Constraint may be
  * "many", and therefore necessarily have to be generated dynamically, see
  * generate_dynamic_constraint(). Note, however, that "the shape" of the set
  * of abstract Constraint of the Block, i.e., the number of elements in the
  * vectors returned by get_static_constraints() and get_dynamic_constraints()
  * must always be the same: it can only change with the *first* call to this
  * method (i.e., when the vectors are initialized) and never afterwards.
  * Besides, the subset of "groups" of static Constraint that are constructed
  *
  *    IS NOT SUPPOSED TO CHANGE OVER THE LIFETIME OF THE Block
  *
  * That is, a Block may be initialized to have less ("abstract") Constraint
  * than all the ones it might; if this is the case, all the other ones will
  * *never* be available. This means that calling this method multiple times
  * with different configuration parameters [see below] implying different
  * "groups" of static Constraint is not allowed: any such call should either
  * be ignored (once the set of static Constraint of the Block is initialized,
  * it is so for good) or throw exception. As a result,
  *
  *    NONE OF THE OPERATIONS IN THIS METHOD SHOULD ISSUE A Modification
  *
  * The idea, again, is that this operation is made once and for all in the
  * lifetime of the object, and never repeated.
  *
  * Note that a derived class can ignore all this and just construct all the
  * Constraint right away, but this may be work and memory wasted if no
  * Solver actually uses them. Any Solver should ensure that this method has
  * been called at least once before making any attempt at using the
  * Constraint, unless it knows for sure that the Block it is working with
  * has done that already. Note that it is easy to check whether or not
  * generate_abstract_constraints() still has to be called by just testing if
  * get_static_constraints().size() == 0, although this is a necessary but
  * not sufficient condition, as a Block may not have any static Constraint
  * at all.
  *
  * In general, a Block can have several different types (groups) of static
  * Constraint; not all the Solver may require all of them, be them directly
  * of the father Block or of any of its sub-Block, recursively. This is why
  * the method has the parameter stcc, which is a pointer to an arbitrarily
  * complex Configuration object. The actual parameter may be of any specific
  * derived class from Configuration, and contain all the information that
  * specifies which of the types (groups) of static Constraint must actually
  * be constructed. If the Block has sub-Block, then the :Configuration object
  * should contain an appropriate :Configuration object for each of these.
  *
  * Note that the stcc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with stcc = nullptr then the
  * corresponding configuration from the BlockConfig is used. Indeed,
  * the method is not pure virtual, but it is given a default implementation
  * doing nothing but calling itself for each sub-Block with no (= default
  * = nullptr) argument. This is correct for those :Block that either have no
  * static Constraints at all or that construct them all anyway, and for which
  * all the sub-Block either have the same property or are only to be called
  * with the default configuration set by the BlockConfig. Note that if the
  * BlockConfig is not set (nullptr) or the corresponding field is not set
  * (nullptr), this is assumed to mean "construct all the static
  * Constraint that you have, if any". Thus, the default implementation
  * automatically works in the "simple" cases, while for any other case the
  * :Block will have to implement its own version, say "unpacking" its
  * :Configuration object to specific sub-Configuration for each of its
  * sub-Block. This cannot be done in the default implementation because the
  * Configuration stcc for the father Block will likely have to be "unpacked",
  * with each sub-Block getting its own specific sub-Configuration, but this
  * can only be done by a specific :Block for a specific :Configuration, as
  * the base Configuration class does not have direct support for the fact
  * that a Configuration contains a sub-Configuration for each specific
  * sub-Block of a Block. */

 virtual void generate_abstract_constraints( Configuration *stcc = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_abstract_constraints();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// generate the dynamic Constraints of the Block
 /** This method is intended as a hook for dynamic generation of Constraint.
  * The idea is that the information available to the Block, e.g. stored in
  * its Variable, may allow the Block to generate more Constraint. This is
  * necessary to use valid inequalities (better describing the feasible region
  * of the Block than the "straightforward" formulation can) or lazy
  * constraints (describing the feasible region of the Block) when the
  * corresponding "families" of Constraint are "very large" (say,
  * exponentially many) and therefore can only be generated dynamically.
  *
  * Note that, from a semantic viewpoint, dynamic constraints in Block are
  * thought as "being there even they are not there"; that is, they contribute
  * to defining the feasible region of the Block, which means that each
  * feasible solution must satisfy them all. Only, since they can be "many"
  * it is better not to explicitly construct all of them, but only "the
  * subset that is useful for algorithmic reasons". This in particular means
  * that if the Block has dynamic Constraint, any call to is_feasible()
  * cannot rely only on the abstract representation of the Block to produce a
  * correct return value. Also, it is important to remark that there are two
  * different classes of dynamic Constraint, "valid inequalities" and "lazy
  * constraints", with a different semantic meaning. Valid inequalities
  * potentially strengthen the formulation of the problem but do not change
  * the set of feasible solutions; therefore, how many of them are generated
  * (comprised none) does not change the optimal solutions of the Block.
  * Conversely, lazy constraints change the set of feasible solutions;
  * therefore, lazy constraints *must* be generated (separated) before a
  * solution is confirmed as feasible. This is, of course, true for
  * general-purpose Solver relying on the abstract representation of the
  * Block; specialized Solver may know the feasible set without any need
  * for dynamic lazy constraints to be ever generated.
  *
  * When this method is called, the Block should attempt at generating new
  * Constraint. If the "abstract representation" of these Constraint has been
  * constructed [see generate_abstract_constraints()], then any newly found
  * Constraint will be added to the corresponding list via a call to
  * add_dynamic_constraint(), thereby triggering the appropriate (abstract)
  * Modification. This may not be necessary, since a specialized Solver may
  * be able to work with a specialized version of the dynamic constraints
  * (say, cutsets in an appropriate graph). In this case, a specific
  * (physical) Modification has to be issued that the specialized Solver has
  * to be capable to recognize, thereby ignoring the corresponding abstract
  * modification. Note that if the "abstract representation" has been also
  * constructed, both types of Modification will have to be issued, with each
  * type of Solver having to figure out which one to react to. Of course, in
  * order to be able to construct the "abstract representation" of a dynamic
  * Constraint one must have already constructed the "abstract representation"
  * of the corresponding Variable first, but this is almost free because
  * generate_abstract_variables() has to be called (with appropriate
  * parameters) before generate_abstract_constraints() is.
  *
  * In general, a Block can have several different types (groups) of dynamic
  * Constraint (be them valid inequalities or lazy constraints); not all of
  * them must necessarily be generated all the time. For instance, valid
  * inequalities may be generated on unfeasible continuous solutions, whereas
  * lazy constraints may be generated on feasible integer ones. Also, some
  * families of dynamic constraints may be cheaper to generate than others,
  * and it may make algorithmic sense to give different priorities at
  * different stages of the solution process. Furthermore, even for the same
  * family of dynamic constraints there could be different generation
  * (separation) procedures (say, heuristic and exact), each with different
  * algorithmic parameters, among which possibly the time that can be spent
  * in the process. Finally, a Block can have several (different) sub-Block,
  * recursively, and dynamic constraints for all sub-Block should in
  * principle be generated whenever this method is called for the father
  * Block.
  *
  * For all these reasons, the method has the parameter dycc, which is a
  * pointer to an arbitrarily complex Configuration object. The actual
  * parameter may be of any specific derived class from Configuration, and
  * contain all the information that is relevant, such as which families of
  * dynamic Constraint to be separated, and which which algorithm. Possibly
  * the :Configuration object may even contain one or more Block with
  * attached Solver to represent and solve the separation problem, so that
  * the algorithmic parameters of the Solver (say, maximum time and required
  * accuracy) can be used to control the separation process. Also, if the
  * Block has sub-Block, some of which have dynamic Constraint, then the
  * :Configuration object should contain an appropriate :Configuration
  * object for each of these.
  *
  * Note that not all "groups" (families) of "abstract" dynamic Constraint may
  * have been constructed [see generate_abstract_constraints()]. This does *not*
  * mean that those that have not been constructed may not be generated: if
  * this is done, only the "physical representation" of these is generated,
  * and the "abstract" one is not.
  *
  * Note that the dycc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with dycc = nullptr then the
  * corresponding configuration from the BlockConfig() is used. Indeed, the
  * method is not pure virtual, but it is given a default implementation
  * doing nothing but calling itself for each sub-Block with no (= default
  * = nullptr) argument. This is correct for those :Block have no dynamic
  * Constraint at all, and for which all the sub-Block either have the same
  * property or are only to be called with the default configuration set by
  * the BlockConfig. Note that if the BlockConfig is not set (nullptr) or the
  * corresponding field is not set (nullptr), this is assumed to mean
  * "separate all the dynamic Constraint that you have, if any", which is
  * fine if the separation process actually has no parameters (say, a single
  * class of valid inequalities with an easy exact separation algorithm).
  * Thus, the default implementation automatically works in the "simple"
  * cases, while for any other case the :Block will have to implement its
  * own version, say "unpacking" its :Configuration object to specific
  * sub-Configuration for each of its sub-Block. This cannot be done in the
  * default implementation because the Configuration dycc for the father
  * Block will likely have to be "unpacked", with each sub-Block getting
  * its own specific sub-Configuration, but this can only be done by a
  * specific :Block for a specific :Configuration, as the base Configuration
  * class does not have direct support for the fact that a Configuration
  * contains a sub-Configuration for each sub-Block of a Block. */

 virtual void generate_dynamic_constraints( Configuration *dycc = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_dynamic_constraints();
  }

/*--------------------------------------------------------------------------*/
 /// generate the "abstract representation" of the Objective of the Block
 /** This method serves is to ensure that the "abstract representation" of
  * the Objective of the Block is initialized, so that it can be read with
  * get_objective(). Of course, in order to be able to construct the "abstract
  * representation" of the Objective one must have already constructed the
  * "abstract representation" of the Variable first,
  * see generate_abstract_variables().
  *
  * Because a (specialized) Solver may not need the description of the Block
  * in terms of its "abstract" Objective, that may not actually be constructed
  * until this method is called for the first time. A derived class can ignore
  * all this and just construct it right away, but this may be work and memory
  * wasted if no Solver actually uses it. Any Solver should ensure that this
  * method has been called at least once before making any attempt at using
  * the Objective, unless it knows for sure that the Block it is working with
  * has done that already.
  *
  * In general, a Block can have any arbitrarily complex objective. Thus,
  * like all other generate_*(), the method has the parameter objc, which is
  * a pointer to an arbitrarily complex Configuration object. The actual
  * parameter may be of any specific derived class from Configuration, and
  * contain all the information that is relevant. Possibly the :Configuration
  * object may even contain one or more Block with attached Solver to
  * represent and solve some complex (say, Lagrangian) function.
  *
  * Note that the objc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with objc = nullptr then the
  * corresponding configuration from the BlockConfig() is used. Indeed, the
  * method is not pure virtual, but it is given a default implementation
  * doing nothing but calling itself for each sub-Block with no (= default
  * = nullptr) argument. This is correct for those :Block have no complex
  * objective, and for which all the sub-Block either have the same
  * property or are only to be called with the default configuration set by
  * the BlockConfig. */

 virtual void generate_objective( Configuration *objc = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_objective();
  }

/**@} ----------------------------------------------------------------------*/
/*----------------- Methods for reading the data of the Block --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the Block
 *  @{ */

 /// getting the classname of this Block
 /** Given a Block, this method returns a string with its class name; unlike
  * std::type_info.name(), there *are* guarantees, i.e., the name will
  * always be the same.
  *
  * The method works by dispatching the private virtual method private_name().
  * The latter is automatically implemented by the
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h], hence this
  * comes at no cost since these have to be called somewhere to ensure that
  * any :Block will be added to the factory. Actually, since
  * Block::private_name() is pure virtual, this ensures that it is not
  * possible to forget to call the appropriate SMSpp_insert_in_factory_cpp_*
  * for any :Block because otherwise it is a pure virtual class (unless
  * the programmer purposely defines private_name() without calling the macro,
  * which seems rather pointless). */

 inline const std::string & classname( void ) const {
  return( private_name() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the string name of this Block

 inline const std::string & name( void ) const { return( f_name ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the "father" Block of this Block

 p_Block get_f_Block( void ) const { return( f_Block ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the BlockConfig object of this Block
 /** The method returns a (const) pointer to the current BlockConfig object
  * of this Block, if any. */

 const BlockConfig * get_BlockConfig( void ) {
  return( f_BlockConfig );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the BlockSolverConfig of this Block
 /**< This method gets information about the current set of Solver attached to
  * the Block (and its sub-Block, recursively) under the form of a single
  * BlockSolverConfig object.
  *
  * If a non-null svcc is provided, then the pointed object (that is assumed
  * to be "empty") is "filled" with the data of the current configuration,
  * and returned. Otherwise, a new BlockSolverConfig object is created,
  * filled and returned. This is done to allow :Block to return objects of a
  * class derived from BlockSolverConfig containing information about specific
  * parts of their Solver configuration that are not present in the base
  * BlockSolverConfig class, filling only that part and then using method of
  * the base Block class to fill-in the standard part. However, the "final
  * user" should not bother and assume that the :Block will ultimately
  * produce the right kind of object, thereby leaving the parameter to its
  * nullptr default value. */

 virtual BlockSolverConfig *  get_SolverConfig(
	                                BlockSolverConfig * svcc = nullptr );

/*--------------------------------------------------------------------------*/
 /// getting the pointer to the current Objective
 /** Getting a pointer to current :Objective. Of course, for this method to
  * return something meaningful (i.e., for the returned pointer to be
  * non-nullptr) the abstract representation of the Objective must have been
  * constructed, cf. generate_objective(). */

 Objective * get_objective( void ) const {
  return( f_Objective );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the the current Objective
 /** This method, template over the class Obj (which must derive from
  * Objective), get the the current Objective, which is supposed to be an
  * Obj *, and returns it. If the Objective is not of the required type,
  * exception is thrown. */

 template< class Obj >
 Obj * get_objective( void ) const {
  static_assert( std::is_base_of< Objective , Obj >::value ,
                 "get_objective: Obj must inherit from Objective" );
  auto obj = dynamic_cast< Obj * >( f_Objective );
  if( ! obj )
   throw( std::invalid_argument( "get_objective: objective is not of "
                                 "required type" ) );
  return( obj );
  }


/*--------------------------------------------------------------------------*/
 /// getting the current sense of the Objective
 /** Getting the current sense (minimization or maximization) of the
  * Objective of the Block. This method has a default implementation that
  * relies on the existence of an Objective in the "abstract representation"
  * of the Block but it is virtual, so that :Block for which this may not be
  * true can override it and answer using the "physical representation" (or
  * maybe just answer a constant since the sense of the problem is fixed).
  * If there is no "abstract representation" of the Objective, the default
  * implementation arbitrarily returns 0 (== Objective::eMin), i.e.,
  * minimization. The rationale is that a Block that only encodes for a
  * feasibility problem actually can have no Objective even even the
  * "abstract representation" is fully constructed, but then this means that
  * the Objective is constantly 0 across all feasible solutions and therefore
  * in principle the sense does not matter (although this changes if an
  * unfeasible solution should be associated with a value +Infinity or
  * -Infinity). Besides, minimization problems are somewhat more common that
  * maximization ones in practice. */

 virtual int get_objective_sense( void ) const;

/*--------------------------------------------------------------------------*/
 /// getting upper bounds on the value of the Objective
 /** This method should return an upper bound on the optimal value of the
  * Objective. This immediately implies that the value of the Objective is a
  * real number, something that is purposely *not* stipulated by the Objective
  * interface. In other words, this method implicitly assumes that the
  * Objective is a RealObjective, which may *not* be true. Should this happen,
  * this method makes no sense and it should not be called. But bounds on the
  * optimal value of the Objective are very important in single-objective
  * optimization, and they are typically associated *to the Block as a whole*
  * rather than to only a part of it, such as the Objective (think of the case
  * where the Objective is linear, and therefore has no finite upper/lower
  * bound unless the feasible region is suitably restricted). Thus, the base
  * Block class has to have this method, which is virtual and whose default
  * implementation just returns "+ Infinity", i.e., "no upper bound".
  *
  * This method can be used to query about two different kinds of upper bounds.
  * The default type (when the "conditional" parameter is at its default value
  * of false) is a globally valid upper bound, which is simply a value
  * guaranteed to be above the value of the [Real]Objective of any feasible
  * solution. If the sense of the [Real]Objective is "maximization" and the
  * Block returns a *finite* globally valid upper bound:
  *
  * - this is a certificate that the problem is *not unbounded above*;
  *
  * - any feasible solution whose value is (approximately) equal to the value
  *   returned by this method is guaranteed to be an (approximately) optimal
  *   solution;
  *
  * although note that for both things one has to "trust the Block" that the
  * returned value is correct. If the sense of the [Real]Objective rather is
  * "minimization" and the Block returns a *finite* globally valid upper bound:
  *
  * - this is a certificate that the problem is *not empty*;
  *
  * - the value returned by this method provides a bound on "how much bad a
  *   solution can be";
  *
  * although again one has to "trust the Block" about the correctness of the
  * returned value is correct. This (in particular the last part) may look
  * rather weird, but it also has its uses. Indeed, it is tied to the other
  * type of upper bound, which is the one required when the "conditional"
  * parameter is true: a "conditionally valid upper bound". The formal
  * definition is the following:
  *
  *    a value v is a conditionally valid upper bound for the problem if
  *    it is a valid upper bound on the optimal value of the problem
  *    PROVIDED THAT THE OPTIMAL VALUE IS NOT +INFINITY
  *
  * The interpretation is, however, quite different if the problem encoded by
  * the Block is a maximization problem or a minimization one:
  *
  * - if the problem encoded by the Block is a maximization problem, then its
  *   optimal value being + infinity means that the problem is unbounded
  *   above; hence, v is a conditionally valid upper bound if whenever one
  *   finds a feasible solution whose objective value is greater than v, then
  *   the problem is unbounded above;
  *
  * - if the problem encoded by the Block is a minimization problem, then its
  *   optimal value being + infinity means that the problem is empty; hence, v
  *   is a conditionally valid upper bound if whenever one finds a valid lower
  *   bound on the optimal value that is larger than v, then the problem is
  *   empty.
  *
  * Explaining how conditionally valid upper bounds can be derived requires a
  * bit of discussion. For the sake of illustration let us assume that the
  * problem encoded in the Block is
  *
  *     (P)   max { c(x) : x \in X }
  *
  * and that (P) is "nice": it has a dual problem
  *
  *     (D)   min { f(y) : y \in Y }
  *
  * that, besides weak duality (f(y) >= c(x) for each y \in Y and x \in X)
  * also satisfies strong duality in the strongest possible sense: the
  * optimal values of (P) and (D) are identical *even when they are plus or
  * minus infinity", which means that (P) is empty <==> (D) is unbounded below
  * and (D) is empty <==> (P) is unbounded above. In other words, (P) and (D)
  * are completely equivalent in terms of optimal values; most often this
  * means that any algorithm solving one actually solves the other as well.
  * One can therefore equivalently consider Block a representation of (P) (a
  * maximization problem) or of (D) (a minimization one). In this setting we
  * may be able to derive a conditionally valid upper bound on both.
  *
  * To do that we assume that we know an "easy" relaxation of (D) that is
  * surely nonempty
  *
  *     (D')   min { f(y) : y \in Y' }
  *
  * For instance, one may know a compact box Y' = [ l , u ] such that
  * l <= y <= u for all y \in Y. Now, let us assume that we can find a
  * *globally valid* finite *upper* bound v on (D') (note that this is a
  * minimization problem, so this is a bound on how *bad* a solution of (D)
  * can ever be); for instance, we may be able to compute a linear upper
  * approximation of f(y) which is valid on Y' (or f() may have been linear
  * in the first place), which makes the computation of v trivial. Then, we
  * know that v >= f(y) for all y \in Y (note, again, that (D) is a
  * minimization problem). Now, let us assume that we find some x \in X such
  * that c(x) > v: that is, we have found a valid lower bound on the optimal
  * value of (D) which is greater than v. Then, (D) is empty as required by
  * the definition of conditionally valid upper bound for a minimization
  * problem (in this case, (D)). Indeed, assume there is any y \in Y: by weak
  * duality f(y) >= c(x) > v, but on the other hand by construction v >= f(y),
  * which yields the contradiction. Hence, (D) must be empty. But for the
  * strong duality assumption, this means that (P) must be unbounded above.
  * Thus, v is a conditionally valid upper bound also according to the
  * definition given for a maximization problem (in this case, (P)): in fact,
  * as soon as we find x \in X such that c(x) > v, we can conclude (by duality
  * arguments) that (P) is unbounded above.
  *
  * Algorithmically, one can use such a construction to declare (D) empty when
  * it is solving it by dual methods, and the algorithm that is solving (P) is
  * "converging to +Infinity". Alternatively, one can see this v providing a
  * convenient stopping criterion when one is solving a (P) which is unbounded
  * above, but for which there is no easy way to characterise things like
  * unbounded ascent directions (say, X is convex and c(x) is concave but it
  * is provided by some completely obscure black box): a conditional valid
  * upper bound on (P) -- obtained by duality arguments -- can allow the
  * optimization to finitely stop declaring that (P) is unbounded above
  * "without having finitely reached +Infinity" (which is not possible).
  *
  * The boolean parameter "conditional", if true, indicates that the required
  * upper bound only has to be conditionally valid, as opposed to globally
  * valid. There are basically two different cases:
  *
  * - the global valid upper bound (conditional == false) is + infinity;
  *   then, necessarily the conditionally valid upper bound
  *   (conditional == true) is <= than the global valid upper bound, and (as
  *   we have discussed) it can be finite (or not);
  *
  * - the global valid upper bound (conditional == false) is finite
  *   (< + infinity); then, necessarily the conditionally valid upper bound
  *   makes no sense (since the problem cannot be unbounded above), and in
  *   particular it can be expected to be > than the global valid upper
  *   bound (cf. the discussion), but this is pointless since there is no
  *   reason for checking it.
  *
  * Global upper bounds are "fragile" values: in principle, *any* change in
  * any part of the Block (Variable, Constraint, Objective, ...) can lead to
  * a change in this value. Thus, the current design decision is that there
  * is no specific Modification for changes in this particular value, which
  * has to be intended as "basically, any Modification changes this". The
  * rationale is that specific Modifications would likely be produced very
  * many times, thus posing an unnecessary strain on the mechanism. So, a
  * user (or, most likely, Solver) interested in this value should just check
  * it "frequently" to see if it has changed. Since the computation of this
  * value can be costly, the Block will have to have a way to assess if it
  * really will have to be done again (say, by putting the value of some
  * field to + infinity). If not, the method should cost very little, hence
  * there is little harm in calling it frequently. When a change in the Block
  * happens, the Block can simply properly set the value; if the method is
  * called (which it may not) the computation is done, otherwise effort is
  * saved.
  *
  * As far as what "frequently" should mean, this is surely "at least each
  * time a Modification is issued, unless it is one of the few Modification
  * that cannot change it (say, addition of dynamic Variable which is
  * guaranteed not to change the optimal value). However, note that the
  * return value may change even if no Modification is issued. A possible
  * example is when the Block encodes the Lagrangian Dual of an minimization
  * problem (which means it is a maximization one): every feasible solution
  * of the original problem provides a valid upper bound to the optimal value
  * of the Lagrangian Dual. Such solution may be "revealed" to the Block by
  * means of some method of its specialized interface, and the Block may
  * react by changing this value. This would actually be a case where a
  * Modification signalling it may be appropriate, but for the reasons above
  * it has been decided against it. */

 virtual double get_valid_upper_bound( bool conditional = false ) {
  return( + Inf<double>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting a global valid lower bound on the value of the Objective
 /** This method should return valid lower bounds on the optimal value of the
  * Objective. See the companion method  get_valid_upper_bound() for comments,
  * obviously exchanging "minimization" with "maximization", "+ infinity"
  * with "- infinity" and unbounded with empty where appropriate. To
  * summarize:
  *
  * - this method is virtual and its default implementation just returns
  *   "- Infinity", i.e., "no lower bound".
  *
  * - when the "conditional" parameter is at its default value of false, the
  *   method should return is a globally valid lower bound, which is a value
  *   guaranteed to be below the value of the [Real]Objective of any feasible
  *   solution. If the sense of the [Real]Objective is "minimization" and the
  *   Block returns a *finite* globally valid lower bound:
  *
  *   - this is a certificate that the problem is *not unbounded below*;
  *
  *   - any feasible solution whose value is (approximately) equal to the value
  *     returned by this method is guaranteed to be an (approximately) optimal
  *     solution;
  *
  *   while if the sense of the [Real]Objective rather is "maximization" and
  *   the Block returns a *finite* globally valid lower bound:
  *
  *   - this is a certificate that the problem is *not empty*;
  *
  *   - the value returned by this method provides a bound on "how much bad a
  *     solution can be".
  *
  * - when the "conditional" parameter is at true, the method should return a
  *   "conditionally valid upper bound", i.e., a value v such that
  *
  *    v is a valid lower bound on the optimal value of the problem
  *    PROVIDED THAT THE OPTIMAL VALUE IS NOT -INFINITY
  *
  *   The interpretation is:
  *
  *   - if the problem encoded by the Block is a minimization problem, then
  *     its optimal value being - infinity means that the problem is unbounded
  *     below; hence, v is a conditionally valid lower bound if whenever one
  *     finds a feasible solution whose objective value is smaller than v,
  *     then the problem is unbounded below;
  *
  *   - if the problem encoded by the Block is a maximization problem, then its
  *     optimal value being - infinity means that the problem is empty; hence,
  *     v is a conditionally valid lower bound if whenever one finds a valid
  *     upper bound on the optimal value that is smaller than v, then the
  *     problem is empty.
  *
  * - There are basically two different cases:
  *
  *   - the global valid lower bound (conditional == false) is - infinity;
  *   then, necessarily the conditionally valid lower bound
  *   (conditional == true) is >= than the global valid lower bound, and (as
  *   we have discussed) it can be finite (or not);
  *
  * - the global valid lower bound (conditional == false) is finite
  *   (> - infinity); then, necessarily the conditionally valid lower bound
  *   makes no sense (since the problem cannot be unbounded below), and in
  *   particular it can be expected to be < than the global valid lower
  *   bound (cf. the discussion), but this is pointless since there is no
  *   reason for checking it.
  *
  * Conditionally valid lower bounds can sometimes be found by duality arguments
  * and can be used as a convenient stopping condition in empty/unbounded cases
  * for algorithms solving the problem, possibly via duality. */

 virtual double get_valid_lower_bound( bool conditional = false ) {
  return( - Inf<double>() );
  }

/*--------------------------------------------------------------------------*/
 /// reading the vector of sub-Blocks of the Block
 /** Method for reading the vector of sub-Blocks of the Block. Note that the
  * vector v_Block is the "abstract representation" of the sub-Block. In this
  * case it may well coincide with the "physical" one, but the point is that
  * the base Block class does not claim ownership of the Blocks in the
  * v_Block field. In other words, the sub-Blocks in that vector are *not*
  * deleted by the destructor of the base Block class: whomever produced them
  * in the first place (likely, the current derived Block class itself) must
  * take responsibility for this. */

 c_Vec_Block & get_nested_Blocks( void ) const { return( v_Block ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the verbosity level

 verbosity_type get_verbosity( void ) const { return( verbosity_lvl ); }

/**@} ----------------------------------------------------------------------*/
/** @name Methods for reading the Block's Variables and Constraints
 *  @{ */

 /// reading the *static* Constraint of the Block
 /** Method for reading the *static* Constraint of the Block. It returns a
  * vector of boost::any, each element of which is supposed to contain only
  * one among:
  *
  * - nothing (empty() returns true), which means that the corresponding
  *   "group" of static Constraint has not been constructed [see
  *   generate_abstract_constraints()];
  *
  * - a pointer to a single Constraint (p_Const in Constraint.h) or to any
  *   class derived from Constraint;
  *
  * - a pointer to a std::vector of any class derived from Constraint
  *   (obviously you can't make a std::vector of the base Constraint class,
  *   which is why pointers to Constraint are also allowed, see below);
  *
  * - a pointer to a boost::multi_array<C , K>, where C is any class derived
  *   from Constraint (obviously you can't make a multi_array of the base
  *   Constraint class), in principle with any K (but the limit for K may be
  *   dictated by un_any_static() and un_any_thing() in SMSTypedefs.h);
  *
  * Note that this is the "abstract representation" of the Block, which is
  * why these are all pointers. It is assumed that the actual [vectors or
  * multi_array] of Constraints have been defined in the derived class, and
  * can therefore be accessed in some model-specific version from its
  * specialized interface. Anyway, they depend on the specific data that
  * characterizes the derived class, which is responsible of the allocation
  * and deallocation of the corresponding memory (the "physical
  * representation" of the Block). The two representations of the Block may,
  * or may not, coincide: however, Variable and Constraint MUST *NEVER* BE
  * COPIED BY VALUE, BECAUSE THEIR MEMORY ADDRESS IS THEIR NAME. Hence,
  * a derived class has to be *very* careful to *NEVER* MODIFY THESE VECTORS
  * (in particular, increase their size) during the lifetime of the Block in
  * order to avoid the risk that some Constraints may change their memory
  * location, hence their "name".
  *
  * A FORTIORI, SIZE OF THE [...] ARRAYS WHOSE POINTERS ARE PROVIDED BY THIS
  * METHOD MUST *NEVER* BE CHANGED BY WHOMEVER READS THEM. Since it must,
  * conversely, be possible to change the individual Constraints, the arrays
  * cannot be const (a size-cons, contents-mutable array should be used,
  * which is possible but just too complicated at this point).
  *
  * Similarly, the size of the vector of static Constraint is *not* supposed
  * to change along the life of the Block: which *groups* of Constraint are
  * there is "the structure of the Block", and this is assumed to be given.
  * Individual Constraint can indeed appear and disappear, which is what
  * dynamic Constraint are for, but "the set of indices of Constraint" is
  * assumed to be given once and for all. That is, add_static_constraint()
  * should only be called (by derived classes) during the initialization of
  * the Block, and never thereafter. More precisely, because some Solver may
  * not need to access the Constraint at all, the idea is that Constraint
  * (be them static or dynamic) are only generated if and when they are
  * actually required by calling generate_abstract_constraints(): this method
  * can only be called if the latter has. However, once the set of static
  * and dynamic Constraints have been constructed, they are not supposed to
  * change (except for addition/deletion of dynamic Constraint and changes in
  * the Constraint that are handled by the appropriate Modification). */

 c_Vec_any & get_static_constraints( void ) const {
  return( v_s_Constraint );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" static Constraint
 /** This method, template over the class Cnst (which must derive from
  * Constraint), extracts the i-th static constraint group, which is supposed
  * to be a simple Cnst *, and returns it. If anything goes wrong, exception
  * is thrown. */

 template< class Cnst >
 Cnst * get_static_constraint( c_Index i ) const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_static_constraint: Cnst must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "get_static_constraint: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto cnst = boost::any_cast< Cnst * >( v_s_Constraint[ i ] );
  if( ! cnst )
   throw( std::invalid_argument( "get_static_constraint: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a (static) std::vector< Constraint >
 /** This method, template over the class Cnst (which must derive from
  * Constraint), extracts the i-th static constraint group, which is supposed
  * to be a std::vector< Cnst > *, and returns it. If anything goes wrong,
  * exception is thrown. */

 template< class Cnst >
 std::vector< Cnst > * get_static_constraint_v( c_Index i ) const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_static_constraint_v: Cnst must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "get_static_constraint_v: group " +
                                 std::to_string( i ) + " of constraints " +
                                 "does not exist" ) );
  auto cnst = boost::any_cast< std::vector< Cnst > * >( v_s_Constraint[ i ] );
  if( ! cnst )
   throw( std::invalid_argument( "get_static_constraint_v: group " +
                                 std::to_string( i ) + " is not of "
                                 "required type" ) );
  return( cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a (static) boost::multi_array< Cnst , K >
 /** This method, template over the class Cnst (which must derive from
  * Constraint) and the integer K, extracts the i-th static constraint group,
  * which is supposed to be a boost::multi_array< Cnst , K > *, and returns
  * it. If anything goes wrong, exception is thrown. */

 template< class Cnst , unsigned short K >
 boost::multi_array< Cnst , K > * get_static_constraint( c_Index i ) const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_static_constraint: Cnst must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "get_static_constraint: group " +
                                 std::to_string( i ) + " of constraints " +
                                 "does not exist" ) );
  auto cnst = boost::any_cast< boost::multi_array< Cnst , K > * >(
						      v_s_Constraint[ i ] );
  if( ! cnst )
   throw( std::invalid_argument( "get_static_constraint: group " +
                                 std::to_string( i ) + " is not of " +
                                 "required type" ) );
  return( cnst );
  }

/*--------------------------------------------------------------------------*/
 /// reading the *static* Variable of the Block
 /** Method for reading the *static* Variable of the Block. It returns a
  * vector of boost::any, each element of which is supposed to contain only
  * one among:
  *
  * - nothing (empty() returns true), which means that the corresponding
  *   "group" of static Variable has not been constructed [see
  *   generate_abstract_variables()];
  *
  * - a pointer to a single Variable (p_Var in Variable.h) or to any
  *   class derived from Variable;
  *
  * - a pointer to a std::vector of any class derived from Variable
  *   (obviously you can't make a std::vector of the base Variable class,
  *   which is why pointers to Variable are also allowed, see below);
  *
  * - a pointer to a boost::multi_array<V , K>, where V is any class derived
  *   from Variable (obviously you can't make a multi_array of the base
  *   Variable class), in principle with any K (but the limit for K may be
  *   dictated by un_any_static() and un_any_thing() in SMSTypedefs.h);
  *
  * Note that this is the "abstract representation" of the Block, which is
  * why these are all pointers. It is assumed that the actual [vectors or
  * multi_array] of Variable have been defined in the derived class, and can
  * therefore be accessed in some model-specific version from its specialized
  * interface. Anyway, they depend on the specific data that characterizes
  * the derived class, which is responsible of the allocation and
  * deallocation of the corresponding memory (the "physical representation"
  * of the Block). The two representations of the Block may, or may not,
  * coincide: however, Variable and Constraint MUST *NEVER* BE COPIED BY
  * VALUE, BECAUSE THEIR MEMORY ADDRESS IS THEIR NAME. Hence, a derived class
  * has to be *very* careful to *NEVER* MODIFY THESE VECTORS (in particular,
  * increase their size) during  the lifetime of the Block in order to avoid
  * the risk that some Variable may change their memory location, hence their
  * "name".
  *
  * A FORTIORI, SIZE OF THE [...] ARRAYS WHOSE POINTERS ARE PROVIDED BY THIS
  * METHOD MUST *NEVER* BE CHANGED BY WHOMEVER READS THEM. Since it must,
  * conversely, be possible to change the individual Variable, the arrays
  * cannot be const (a size-cons, contents-mutable array should be used,
  * which is possible but just too complicated at this point).
  *
  * Similarly, the size of the vector of static Variable is *not* supposed
  * to change along the life of the Block: which *groups* of Variable are
  * there is "the structure of the Block", and this is assumed to be given.
  * Individual Variable can indeed appear and disappear, which is what
  * dynamic Variable are for, but "the set of indices of Variable" is
  * assumed to be given once and for all. That is, add_static_variable()
  * should only be called (by derived classes) during the initialization of
  * the Block, and never thereafter. More precisely, as opposed to
  * Constraint, Variable necessarily need to be initialized immediately when
  * the Block is constructed. However, this does not mean that their
  * "abstract representation" is necessarily available: this method can only
  * be called if generate_abstract_variables() has been called. However,
  * once the "abstract representation" of the sets of static and dynamic
  * Variable have been constructed, they are not supposed to change (except
  * for addition/deletion of dynamic Variable and changes in the Variable
  * that are handled by the appropriate Modification). */

 c_Vec_any & get_static_variables( void ) const {
  return ( v_s_Variable );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" static Variable
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th static variable group, which is supposed to
  * be a simple Var *, and returns it. If anything goes wrong, exception is
  * thrown. */

 template< class Var >
 Var * get_static_variable( c_Index i ) const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_static_variable: Var must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "get_static_variable: group " +
                                 std::to_string( i ) + " of constraints "
                                 "does not exist" ) );
  auto var = boost::any_cast< Var * >( v_s_Variable[ i ] );
  if( ! var )
   throw( std::invalid_argument( "get_static_variable: group " +
                                 std::to_string( i ) + " is not of " +
                                 "required type" ) );
  return( var );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a (static) std::vector< Variable >
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th static variable group, which is supposed to
  * be a std::vector< Var > *, and returns it. If anything goes wrong,
  * exception is thrown. */

 template< class Var >
 std::vector< Var > * get_static_variable_v( c_Index i ) const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_static_variable_v: Var must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "get_static_variable_v: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto var = boost::any_cast< std::vector< Var > * >( v_s_Variable[ i ] );
  if( ! var )
   throw( std::invalid_argument( "get_static_variable_v: group "
                                 + std::to_string( i ) +
                                 " is not of required type" ) );
  return( var );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a (static) boost::multi_array< Var , K >
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the i-th static variable group,
  * which is supposed to be a boost::multi_array< Var , K > *, and returns
  * it. If anything goes wrong, exception is thrown. */

 template< class Var , unsigned short K >
 boost::multi_array< Var , K > * get_static_variable( c_Index i ) const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_static_variable: Var must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "get_static_variable: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto var = boost::any_cast< boost::multi_array< Var , K > * >(
						          v_s_Variable[ i ] );
  if( ! var )
   throw( std::invalid_argument( "get_static_variable: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( var );
  }

/*--------------------------------------------------------------------------*/
 /// reading the *dynamic* Constraint of the Block
 /** Method for reading the *dynamic* Constraint of the Block. It returns a
  * vector of boost::any, each element of which is supposed to contain only
  * one among:
  *
  * - nothing (empty() returns true), which means that the corresponding
  *   "group" of dynamic Constraint has not been constructed [see
  *   generate_dynamic_constraints()];
  *
  * - a pointer to a single std::list<C>, where class C is derived from
  *   Constraint (obviously you can't make a std::list of the base Constraint
  *   class, which is why pointers to Constraint are also allowed, see below);
  *
  * - a pointer to a std::vector<std::list<C> >, where class C is derived from
  *   Constraint;
  *
  * - a pointer to a boost::multi_array<std::list<C> , K>,  where class C is
  *   derived from Constraint, in principle with any K (but the limit for K
  *   may be dictated by un_any_static() and un_any_thing() in SMSTypedefs.h);
  *
  * Note that this is the "abstract representation" of the Block, which is
  * why these are all pointers. It is assumed that the actual [vector or
  * multi_array of] list of Constraint have been defined in the derived
  * class, and can therefore be accessed in some model-specific version from
  * its specialized interface. Anyway, they depend on the specific data that
  * characterizes the derived class, which is responsible of the allocation
  * and deallocation of the corresponding memory (the "physical
  * representation" of the Block). These being *dynamic* Constraints, the
  * lists can well be (but need not necessarily be) empty when the object is
  * initialized, and be populated (and de-populated) dynamically during the
  * lifetime of the Block. The two representations of the Block may, or may
  * not, coincide: however, Variable and Constraint MUST *NEVER* BE COPIED BY
  * VALUE, BECAUSE THEIR MEMORY ADDRESS IS THEIR NAME. Hence, a derived class
  * has to be *very* careful to *NEVER* MODIFY THESE VECTORS (in particular,
  * increase their size) during the lifetime of the Block in order to avoid
  * the risk that some Constraint may change their memory location, hence
  * their "name".
  *
  * A FORTIORI, SIZE OF THE [...] ARRAYS WHOSE POINTERS ARE PROVIDED BY THIS
  * METHOD MUST *NEVER* BE CHANGED BY WHOMEVER READS THEM. This also implies
  * that THE ADDRESS OF ALL LISTS OF DYNAMIC Constraints WILL NEVER CHANGE,
  * AND THEREFORE IT CAN BE USED AT THE COLLECTIVE NAME FOR THAT SET OF
  * DYNAMIC Constraint (each one of which will then have its individual name
  * given by its memory address, which will also never change). Since it must,
  * conversely, be possible to change the individual Constraint, the arrays
  * cannot be const (a size-cons, contents-mutable array should be used,
  * which is possible but just too complicated at this point).
  *
  * The rationale of the structure is that the lists can be indiced over (in
  * principle) as many indices ad one wants, but each element of the list is
  * a *single Constraint*. If the user needs to have multi-dimensional
  * dynamic Constraints (say, c[ i ] with a dynamic index "i" where each
  * c[ i ] is a matrix of Constraints depending on two static indices "j" and
  * "k"), then she has to put the dynamic index at the end and ensure that
  * all the lists are updated in the same way; say, define the 2-dimensional
  * array of lists of Constraints c[ j ][ k ], and be sure that all the lists
  * for all the indices "j" and "k" are updated simultaneously each time a
  * new index "i" is added, or an old one is removed. Lists of lists of
  * Constraint are *not* supported (just make that a unique list). In other
  * words, the size of the vectors of lists is *fixed* and must *never* be
  * changed: the only thing that can change (freely) is the size of each list.
  *
  * Similarly, the size of the vector of dynamic Constraints is *not* supposed
  * to change along the life of the Block: which *groups* of Constraint are
  * there is "the structure of the Block", and this is assumed to be given.
  * Individual Constraint can indeed appear and disappear, which is precisely
  * what dynamic Constraint are for, but "the set of indices of
  * Constraint" is assumed to be given once and for all. That is,
  * add_dynamic_constraint() should only be called (by derived classes)
  * during the initialization of the Block, and never thereafter. More
  * precisely, because some Solver may not need to access the Constraint at
  * all, the  idea is that Constraint (be them static or dynamic) are only
  * generated if and when they are actually required. This is what the method
  * generate_dynamic_constraints() is about: this method can only be called if
  * the latter has. Note that, once the sets of static and dynamic Constraint
  * have been constructed, new dynamic Constraint can be repeatedly generated
  * by calling generate_dynamic_constraints(): while the "shape" of the set
  * of dynamic Constraint is constant, it still makes sense to call that
  * method more than once, as the "separators" doing the actual generation
  * may use different information (typically, the current value of the
  * Variable) and/or have resource constraints that can be reset by calling
  * the method again. Yet, new Constraint generated by
  * generate_dynamic_constraints() will trigger an appropriate Modification,
  * which means that there should be no need to call this method again in
  * order to "incorporate" this new information. */

 c_Vec_any & get_dynamic_constraints( void ) const {
  return( v_d_Constraint );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" dynamic Constraint
 /** This method, template over the class Cnst (which must derive from
  * Constraint), extracts the i-th dynamic constraint group, which is
  * supposed to be a std::list< Cnst > *, and returns it. If anything goes
  * wrong, exception is thrown. */

 template< class Cnst >
 std::list< Cnst > * get_dynamic_constraint( c_Index i ) const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_dynamic_constraint: Cnst must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "get_dynamic_constraint: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto cnst = boost::any_cast< std::list< Cnst > * >( v_d_Constraint[ i ] );
  if( ! cnst )
   throw( std::invalid_argument( "get_dynamic_constraint: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a dynamic std::vector< std::list< Cnst > >
 /** This method, template over the class Cnst (which must derive from
  * Constraint), extracts the i-th dynamic constraint group, which is supposed
  * to be a std::vector< std::list< Cnst > > *, and returns it. If anything
  * goes wrong, exception is thrown. */

 template< class Cnst >
 std::vector< std::list< Cnst > > * get_dynamic_constraint_v( c_Index i )
  const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_dynamic_constraint_v: "
                 "Cnst must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "get_dynamic_constraint_v: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto cnst = boost::any_cast< std::vector< std::list< Cnst > > * >(
						       v_d_Constraint[ i ] );
  if( ! cnst )
   throw( std::invalid_argument( "get_dynamic_constraint_v: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the a dynamic boost::multi_array< std::list< Cnst > , K >
 /** This method, template over the class Cnst (which must derive from
  * Constraint) and the integer K, extracts the i-th dynamic constraint group,
  * which is supposed to be a boost::multi_array< std::list< Cnst > , K > *,
  * and returns it. If anything goes wrong, exception is thrown. */

 template< class Cnst , unsigned short K >
 boost::multi_array< std::list< Cnst > , K > * get_dynamic_constraint(
							 c_Index i ) const {
  static_assert( std::is_base_of< Constraint , Cnst >::value ,
                 "get_dynamic_constraint: Cnst must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "get_dynamic_constraint: group " +
                                 std::to_string( i ) +
                                 " of constraints does not exist" ) );
  auto cnst =
   boost::any_cast< boost::multi_array< std::list< Cnst > , K > * >(
						      v_d_Constraint[ i ] );
  if( ! cnst )
    throw( std::invalid_argument( "get_dynamic_constraint: group " +
                                  std::to_string( i ) +
                                  " is not of required type" ) );
  return( cnst );
  }

/*--------------------------------------------------------------------------*/
 /// reading the *dynamic* Variable of the Block
 /** Method for reading the *dynamic* Variable of the Block. It returns a
  * vector of boost::any, each element of which is supposed to contain only
  * one among:
  *
  * - nothing (empty() returns true), which means that the corresponding
  *   "group" of dynamic Variable has not been constructed [see
  *   generate_dynamic_variables()];
  *
  * - a pointer to a single std::list<V>, where class V is derived from
  *   Variable (obviously you can't make a std::list of the base Variable
  *   class, which is why pointers to Variable are also allowed, see below);
  *
  * - a pointer to a std::vector<std::list<V> >, where class V is derived from
  *   Variable;
  *
  * - a pointer to a boost::multi_array<std::list<V> , K>,  where class V is
  *   derived from Variable, in principle with any K (but the limit for K
  *   may be dictated by un_any_static() and un_any_thing() in SMSTypedefs.h);
  *
  * Note that this is the "abstract representation" of the Block, which is
  * why these are all pointers. It is assumed that the actual [vector or
  * multi_array of] list of Variable have been defined in the derived class,
  * and can therefore be accessed in some model-specific version from its
  * specialized interface. Anyway, they depend on the specific data that
  * characterizes the derived class, which is responsible of the allocation
  * and deallocation of the corresponding memory (the "physical
  * representation" of the Block). These being *dynamic* Variables, the lists
  * can well be (but need not necessarily be) empty when the object is
  * initialized, and be populated (and de-populated) dynamically during the
  * lifetime of the Block. The two representations of the Block may, or may
  * not, coincide: however, Variable and Constraint MUST *NEVER* BE COPIED BY
  * VALUE, BECAUSE THEIR MEMORY ADDRESS IS THEIR NAME. Hence, a derived class
  * has to be *very* careful to *NEVER* MODIFY THESE VECTORS (in particular,
  * increase their size) during the lifetime of the Block in order to avoid
  * the risk that some Variable may change their memory location, hence their
  * "name".
  *
  * A FORTIORI, SIZE OF THE [...] ARRAYS WHOSE POINTERS ARE PROVIDED BY THIS
  * METHOD MUST *NEVER* BE CHANGED BY WHOMEVER READS THEM. This also implies
  * that THE ADDRESS OF ALL LISTS OF DYNAMIC Variables WILL NEVER CHANGE,
  * AND THEREFORE IT CAN BE USED AT THE COLLECTIVE NAME FOR THAT SET OF
  * DYNAMIC Variable (each one of which will then have its individual name
  * given by its memory address, which will also never change). Since it must,
  * conversely, be possible to change the individual Variable, the arrays
  * cannot be const (a size-cons, contents-mutable array should be used,
  * which is possible but just too complicated at this point).
  *
  * The rationale of the structure is that the lists can be indiced over (in
  * principle) as many indices ad one wants, but each element of the list is
  * a *single Variable*. If the user needs to have multi-dimensional dynamic
  * Variable (say, x[ i ] with a dynamic index "i" where each x[ i ] is a
  * matrix of Constraints depending on two static indices "j" and "k"), then
  * she has to put the dynamic index at the end and ensure that all the lists
  * are updated in the same way (say, define the 2-dimensional array of lists
  * of Variable x[ j ][ k ], and be sure that all the lists for all the
  * indices "j" and "k" are updated simultaneously each time a new index "i"
  * is added, or an old one is removed). Lists of lists of Variable are *not*
  * supported (just make that a unique list). In other words, the size of the
  * vectors of lists is *fixed* and must *never* be changed: the only thing
  * that can change (freely) is the size of each list.
  *
  * Similarly, the size of the vector of dynamic Variables is *not* supposed
  * to change along the life of the Block: which *groups* of Variables are
  * there is "the structure of the Block", and this is assumed to be given.
  * Individual Variable can indeed appear and disappear, which is precisely
  * what dynamic Variables are for, but "the set of indices of Variable" is
  * assumed to be given once and for all. That is,
  * add_dynamic_variable() should only be called (by derived classes) during
  * the initialization of the Block, and never thereafter.
  *
  * As opposed to Constraint, Variable necessarily need to be initialized
  * immediately when the Block is constructed. However, this does not mean
  * that their "abstract representation" is necessarily available: this
  * method can only be called if generate_dynamic_variables() has. Note that,
  * once the sets of static and dynamic Variable have been constructed, new
  * dynamic Variable can be repeatedly generated by calling
  * generate_dynamic_variables(): while the "shape" of the set of dynamic
  * Variable is constant, it still makes sense to call that method more than
  * once, as the "pricers" doing the actual generation may use different
  * information (typically, the current value of dual information of the
  * Constraint) and/or have resource constraints that can be reset by calling
  * the method again. Yet, new Variable generated by
  * generate_dynamic_variables() will trigger an appropriate Modification,
  * which means that there should be no need to call this method again in
  * order to "incorporate" this new information. */

 c_Vec_any & get_dynamic_variables( void ) const {
  return( v_d_Variable );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" dynamic Variable
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th dynamic variable group, which is supposed
  * to be a std::list< Var > *, and returns it. If anything goes wrong,
  * exception is thrown. */

 template< class Var >
 std::list< Var > * get_dynamic_variable( c_Index i ) const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_dynamic_variable: Var must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "get_dynamic_variable: group " +
                                 std::to_string( i ) +
                                 " of variables does not exist" ) );
  auto var = boost::any_cast< std::list< Var > * >( v_d_Variable[ i ] );
  if( ! var )
   throw( std::invalid_argument( "get_dynamic_variable: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( var );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a dynamic std::vector< std::list< Var > >
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th dynamic variable group, which is supposed to
  * be a std::vector< std::list< Var > > *, and returns it. If anything goes
  * wrong, exception is thrown. */

 template< class Var >
 std::vector< std::list< Var > > * get_dynamic_variable_v( c_Index i )
  const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_dynamic_variable_v: Var must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "get_dynamic_variable_v: group " +
                                 std::to_string( i ) +
                                 " of variables does not exist" ) );
  auto var = boost::any_cast< std::vector< std::list< Var > > * >(
						       v_d_Variable[ i ] );
  if( ! var )
   throw( std::invalid_argument( "get_dynamic_variable_v: group " +
                                 std::to_string( i ) +
                                 " is not of required type" ) );
  return( var );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get the a dynamic boost::multi_array< std::list< Var > , K >
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the i-th dynamic variable group,
  * which is supposed to be a boost::multi_array< std::list< Var > , K > *,
  * and returns it. If anything goes wrong, exception is thrown. */

 template< class Var , unsigned short K >
 boost::multi_array< std::list< Var > , K > * get_dynamic_variable(
							 c_Index i ) const {
  static_assert( std::is_base_of< Variable , Var >::value ,
                 "get_dynamic_variable: Var must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "get_dynamic_variable: group " +
                                 std::to_string( i ) +
                                 " of variables does not exist" ) );
  auto var =
   boost::any_cast< boost::multi_array< std::list< Var > , K > * >(
						      v_d_Variable[ i ] );
  if( ! var )
    throw( std::invalid_argument( "get_dynamic_variable: group " +
                                  std::to_string( i ) +
                                  " is not of required type" ) );
  return( var );
  }

/*--------------------------------------------------------------------------*/
 /// getting the static Constraints' names
 /** Returns a const reference to the vector storing the names of the
  * different types (sets) of static Constraints of the Block */

 c_Vec_string & get_s_const_name( void ) const {
  return( v_s_Constraint_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the static Variables' names
 /** Returns a const reference to the vector storing the names of the
  * different types (sets) of static Variables of the Block */

 c_Vec_string & get_s_var_name( void ) const {
  return( v_s_Variable_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the dynamic Constraints' names
 /** Returns a const reference to the vector storing the names of the
  * different types (sets) of dynamic Constraints of the Block */

 c_Vec_string & get_d_const_name( void ) const {
  return( v_d_Constraint_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the dynamic Variables' names
 /** Returns a const reference to the vector storing the names of the
  * different types (sets) of dynamic Variables of the Block */

 c_Vec_string & get_d_var_name( void ) const {
  return( v_d_Variable_names );
  }

/**@} ----------------------------------------------------------------------*/
/*----- Methods for adding/removing (dynamic) Variables and Constraints ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for changing Variable, Constraint and Objective
 *
 * Dynamic Variable and Constraint are always organized in lists (which may
 * themselves be in a std::vector or boost::multi_array, but the size of
 * these must be fixed, and only that of the list can change). These
 * methods allow to add and remove elements from the lists, triggering the
 * appropriate Modification.
 *  @{ */

 /// adds a bunch of new Constraint at the end of the given list
 /** Adds a bunch of new Constraint at the end of the given list.
  *
  * The parameter list is obviously not "const", as the list will be updated.
  * Note that the base class implementation of this method just does this and
  * possibly issues the appropriate BlockModAdd; as list is supposed to be a
  * part of the "abstract representation", this means that the "physical
  * representation" of the corresponding dynamic Constraint (if any exists)
  * is not updated, as this should clearly be responsibility of the derived
  * class (maybe within the corresponding implementation of this method).
  *
  * Note that the new Constraint must already be organized in a list, which
  * is "spliced" into the given one. This is crucial because splicing is the
  * only way in which elements can be added to a list without being copied,
  * i.e., their memory address being changed.
  *
  * The parameter issueMod decides if and how the BlockModAdd is issued, as
  * described in Observer::make_par(). */

 template<class Const>
 void add_dynamic_constraints( std::list<Const> &list ,
			       std::list<Const> &newlist ,
			       c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// adds a bunch of new Variable at the end of the given list
 /** Adds a bunch of new Variable at the end of the given list.
  *
  * The parameter list is obviously not "const", as the list will be updated.
  * Note that the base class implementation of this method just does this and
  * possibly issues the appropriate BlockModAdd; as list is supposed to be a
  * part of the "abstract representation", this means that the "physical
  * representation" of the corresponding dynamic Variable (if any exists) is
  * not updated, as this should clearly be responsibility of the derived
  * class (maybe within the corresponding implementation of this method).
  *
  * Note that the new Variables must already be organized in a list, which
  * is "spliced" into the given one. This is crucial because splicing is the
  * only way in which elements can be added to a list without being copied,
  * i.e., their memory address being changed.
  *
  * The parameter issueMod decides if and how the BlockModAdd is issued, as
  * described in Observer::make_par(). */

 template<class Var>
 void add_dynamic_variables( std::list<Var> &list , std::list<Var> &newlist ,
			     c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// removes a bunch of Constraint from the given list
 /** Removes a bunch of Constraint from the given list.
  *
  * The parameter list is obviously not "const", as the list will be updated.
  * Note that the base class implementation of this method just does this and
  * possibly issues the appropriate BlockModRmv; as list is supposed to be a
  * part of the "abstract representation", this means that if the "physical
  * representation" of the corresponding dynamic Constraint (if any exists)
  * is different from its "abstract representation" is not updated, as this
  * should clearly be responsibility of the derived class (maybe within the
  * corresponding implementation of this method).
  *
  * The Constraint to be removed are these whose iterator is found in the
  * std::vector rmvd. Note that, if the BlockModRmv is issued, these
  * Constraint are not immediately deleted; rather, they are added to a list
  * stored into a field of the BlockModRmv object, so that they remain alive
  * until the last interested Solver had had the chance to use them to
  * perform the required changes. This also implies that the same memory
  * address cannot be used for new Constraint (or anything), which also
  * avoids potential problems. As soon as the BlockModRmv is eventually
  * destroyed, the Constraints are destroyed as well. If, instead, the
  * BlockModRmv is not issued, the Constraint are destroyed immediately.
  *
  * The parameter issueMod decides if and how the BlockModRmv is issued, as
  * described in Observer::make_par().
  *
  * Important note: each Constraint knows which are the Variable that are
  * active in it. Likewise, each Variable knows which Constraint (among other
  * things) it is active in. Therefore, the information about whether a
  * Variable is active in a Constraint can be obtained in two ways:
  *
  * 1) asking a Variable about whether it is active in a particular
  *    Constraint;
  *
  * 2) asking a Constraint about whether a particular Variable is
  *    active in it.
  *
  * When a dynamic Constraint is removed, any Variable that is still active
  * in this Constraint at the time this method is called will no longer be
  * active in it. This will be accomplished by removing the Constraint from
  * each Variable that was active in it. This means that the first source of
  * information about activeness listed above will be updated. However, the
  * Constraint would still keep the information about which Variable were
  * active in it before its removal. This would create a problem when the
  * dynamic Constraint is ultimately destructed (possibly having spent time
  * waiting inside the issued BlockModRmv), because the destructor of a
  * Constraint, unlike that of a Variable, is supposed to un-register it
  * from all the Variable that it is active in. To avoid this
  *
  *     ALL DELETED Constraint ARE clear()-ED WITHIN THE METHOD
  *
  * This means that the list of Variable that the Constraint was active in
  * is immediately cleared (without re-warning the Variable, who have just
  * been). As a consequence,
  *
  *     WHOMEVER HANDLES THE ISSUED BlockModRmv (IF ANY) CANNOT RELY ON
  *     THAT INFORMATION, SINCE IT WILL NO LONGER BE THERE
  *
  * Note that when the Constraint is removed from the Variable, no
  * Modification is issued (see Variable::remove_active()); thus, calling
  * this method does not trigger any other Modification apart from the
  * BlockModRmv. */

 template<class Const>
 void remove_dynamic_constraints( std::list<Const> &list ,
	              std::vector<typename std::list<Const>::iterator> &rmvd ,
		      c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// Like remove_dynamic_constraint*s*( Const ), just only one of them

 template<class Const>
 void remove_dynamic_constraint( std::list<Const> &list ,
				 typename std::list<Const>::iterator rmvd  ,
				 c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// removes a bunch of Variable from the given list
 /** Removes a bunch of Variable from the given list.
  *
  * The parameter list is obviously not "const", as the list will be updated.
  * Note that the base class implementation of this method just does this and
  * possibly issues the appropriate BlockModRmv; as list is supposed to be a
  * part of the "abstract representation", this means that if the "physical
  * representation" of the corresponding dynamic Variable (if any exists) is
  * different from its "abstract representation" is not updated, as this
  * should clearly be responsibility of the derived class (maybe within the
  * corresponding implementation of this method).
  *
  * The Variable to be removed are these whose iterator is found in the
  * std::vector rmvd. Note that, if the BlockModRmv is issued, these Variable
  * are not immediately deleted; rather, they are added to a list stored into
  * a field of the BlockModAD object, so that they remain alive until the
  * last interested Solver had had the chance to use them to perform the
  * required changes. This also implies that the same memory address cannot
  * be used for new Variable (or anything), which also avoids potential
  * problems. As soon as the BlockModAD is eventually destroyed, the Variable
  * are destroyed as well. If, instead, the BlockModAD is not issued, the
  * Variable are destroyed immediately.
  *
  * The parameter issueMod decides if and how the BlockModRmv is issued, as
  * described in Observer::make_par().
  *
  * Important note: each Constraint knows which are the Variable that are
  * active in it. Likewise, each Variable knows which "stuff" (Constraint,
  * Objective, ...) it is active in. Therefore, the information about whether
  * a Variable is active in a Constraint can be obtained in two ways:
  *
  * 1) asking a Variable about whether it is active in a particular "stuff";
  *
  * 2) Asking a "stuff" about whether a particular Variable is active in it.
  *
  * When a dynamic Variable is removed from a Block, it will no longer be
  * active in any "stuff". This will be accomplished by removing the Variable
  * from each "stuff" in which it is active. This means that the second
  * source of information about activeness listed above will be updated.
  * However, the Variable will still keep the information about which "stuff"
  * it was active in before its removal. This means that the first source of
  * information about activeness will not be updated. This is OK because the
  * destructor of Variable, unlike that of Constraint, is *not* supposed to
  * un-register the Variable from all the stuff (hence, Constraint) it was
  * active in. Hence, although the information is outdated (and possibly
  * dangerous: the Constraint may themselves be deleted, which would lead to
  * dangling pointers), it is not automatically used by the Variable, and
  * therefore it is safe to keep it there (Variable do not have clear() like
  * Constraint do). However,
  *
  *     WHOMEVER IS HANDLING THE ISSUED BlockModRmv (IF ANY) HAS TO BE
  *     CAREFUL NOT TO RELY ON THE INFORMATION ABOUT THE ACTIVE STUFF
  *     OF THE Variable, SINCE IT IS NOT RELIABLE.
  *
  * Also, unlike removing a dynamic Constraint (see the comments to
  * remove_dynamic_constraints()), removing a dynamic Variable from a Block
  * is a complex task. Besides this Block issuing a BlockModRmv stating that
  * this Variable has been removed, other Modification are in principle
  * issued. For each "stuff" in which the Variable is active,
  * ThinVarDepInterface::remove_variable() will be called, and this in
  * principle issues a Modification on its own. The further parameter
  * issueindMod is provided to control this, with the usual format described
  * in Observer::make_par().
  *
  * Modification are crucial to maintaining the coherence between the Block
  * and the Solver: hence, choosing to disable them should be a very well
  * thought out decision. However, avoiding issuing the individual
  * Modifications may be useful (more efficient) in some cases. For instance,
  * a Block may have a huge number of *linear* Constraint, and a dynamic
  * Variable may be active in many of those. If this Variable is removed from
  * the Block, it will be removed from each Constraint in which is active,
  * thereby triggering a large number of Modification being issued. However,
  * the Solver attached to the Block may be able to deal with the removal of
  * this Variable without relying on the individual Modification. Indeed,
  * the change in all the linear constraints due to such a removal are
  * trivial, and possibly handled with a single simple operation (say,
  * removing one column of the coefficients matrix). We stress, however, that
  * setting issueindMod to eNoMod must be a completely conscious decision. If
  * one is not sure whether the individual Modification are needed, this
  * parameter should be left to the default value (or to eNoBlck when
  * appropriate). */

 template<class Var>
 void remove_dynamic_variables( std::list<Var> &list ,
                       std::vector<typename std::list<Var>::iterator> &rmvd  ,
	               c_ModParam issueMod = eModBlck ,
		       c_ModParam issueindMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// Like remove_dynamic_variable*s*( Var ), just only one of them

 template<class Var>
 void remove_dynamic_variable( std::list<Var> &list ,
			       typename std::list<Var>::iterator rmvd ,
			       c_ModParam issueMod = eModBlck ,
			       c_ModParam issueindMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// change the Objective of the Block
 /** Method to change, the Objective of the Block. newOF is a pointer to an
  * object of (a derived) class (from) Objective, which is stored into the
  * f_Objective field. Note that this, being a pointer to a single object,
  * may well be the "physical representation" of the Objective, but it is
  * dealt with as being the "abstract representation" in that the base Block
  * class does not claim ownership of newOF, i.e., it does not delete it in
  * the destructor: whomever produced it in the first place (most likely, the
  * current derived Block class) must take responsibility for this.
  *
  * Note that, instead, set_objective() calls newOF->set_Block().
  *
  * The parameter issueMod decides if and how the BlockMod is issued, as
  * described in Observer::make_par(). */

 void set_objective( Objective * newOF , c_ModParam issueMod = eModBlck );

/**@} ----------------------------------------------------------------------*/
/*--------------------- Methods for checking the Block ---------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for checking solution information in the Block
 *
 * The following four methods allow to check that the solution information
 * (typically produced by a Solver) currently present in the Block has the
 * fundamental properties that may be required. This is primarily stored in
 * the Variable of the Block, but potentially also elsewhere (such as in the
 * dual variables associated to the Constraint of the Block, if any).
 *
 * In particular, the four methods check if the solution information provides:
 *
 * - a certificate of non-emptyness of the Block, which means it represents
 *   a(n almost) feasible solution;
 *
 * - a certificate of non-unboundedness of the Block, which typically means it
 *   represents something providing a bound (lower if the Block is a
 *   minimization problem, upper otherwise) that can be used to prove that a
 *   given feasible solution is (almost) optimal, and anyway proves that the
 *   the problem is not unbounded; for convex problems this is typically a
 *   feasible solution to the dual problem;
 *
 * - a certificate of non-optimality of the Block, which typically means it
 *   represents something like a ray of the feasible region along which the
 *   Objective is unbounded (either below or above as appropriate): note that
 *   the existence of such an object is not, strictly speaking, enough to
 *   prove that the problem is unbounded: this also requires the problem to be
 *   non-empty; yet, existence of such an object does prove that the problem
 *   at least does not have an optimal solution (either it does not have a
 *   solution at all, or it is unbounded);
 *
 * - a certificate of non-feasibility of the Block, i.e., something proving
 *   that the problem cannot have any solution; for convex problems this is
 *   typically a ray of the feasible region of the dual problem along which
 *   the dual objective is unbounded (either above or below as appropriate):
 *   note that the existence of such an object is not, strictly speaking,
 *   enough to prove that the dual problem is unbounded (and therefore the
 *   primal is empty): this also requires the dual problem to be non-empty;
 *   yet, existence of such an object does prove that the dual problem
 *   at least does not have an optimal solution (either it does not have a
 *   solution at all, or it is unbounded), which under some qualification
 *   conditions implies that the primal problem is empty.
 *
 * The rationale for providing these methods is two-fold:
 *
 * - they can be used to "debug" a Solver, in that when the Solver declares
 *   to, say, have found a feasible solution and have written it in the
 *   Variable of the Block, the methods can be used to ensure that the
 *   Solver indeed did the right job;
 *
 * - they can be used to verify if some solution information (say, obtained
 *   "a long time ago" and stored into a Solution object) still has the
 *   required properties (say, feasibility) even after all the Modification
 *   that may have occurred in the Meantime (note that for this to happen the
 *   Solution has to be read back into the Block).
 *
 * Note that these checks may be either "easy" or "hard". For instance,
 * feasibility and ray-ness should be "easy" for an NP-hard problem, while
 * optimality and emptyness are "hard"; the converse happens for co-NP-hard
 * problems. For everything to be "easy", the problem should be an "easy"
 * (polynomial) one to start with.
 *
 * All these methods have a useabstract parameter dictates how the check
 * should be performed:
 *
 * - if true, it should use the abstract representation of the Block;
 *
 * - if false, it should rather use the physical representation of the Block.
 *
 * The second version is useful for several reasons:
 *
 * - it can be more computationally efficient;
 *
 * - it does not require the abstract representation of the Block to be
 *   even constructed;
 *
 * - it can serve as "debug" of the abstract representation of the Block
 *   itself.
 *
 * Also, all these methods have a (pointer to) Configuration parameter. This
 * can serve two different purposes:
 *
 * - Checking the property (whatever that is) is likely to entail at the
 *   very least some numerical computation, say to verify that some
 *   matrix-vector scalar product is "zero". This may require numerical
 *   accuracy parameters, which the Configuration can hold. For instance,
 *   the Configuration (pointer) may simply be (a pointer to) a
 *   SimpleConfiguration<double> specifying, say, the maximum relative
 *   accuracy in a "x == 0" computation).
 *
 * - One may be interested in checking "only partly" the property. This
 *   goes hand-in-hand with the fact that a Solution can hold only "a
 *   part" of all the solution information, say, only a subset of the
 *   Variable. It may thus not be possible, or just not be useful, to
 *   check (say) feasibility of all Constraint, but only of those that
 *   concern a particular subset of the Variable. Just as the methods
 *   for producing Solution have a Configuration to allow specifying
 *   what part of the Solution is "interesting", so the Configuration
 *   parameter of these methods can serve a similar purpose.
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// returns true if the current solution is (approximately) feasible
 /** Returns true if the solution encoded in the current value of the
  * Variable of the Block is approximately feasible within the given
  * tolerances.
  *
  * The useabstract parameter being true dictates that the solution is
  * checked for feasibility w.r.t. "abstract representation" of the Block,
  * otherwise the "physical representation" of the Block is used. This may
  * yield different outcomes, as discussed below. Of course, for
  * is_feasible( true ) to work the "abstract representation" have to
  * have been constructed in the first place.
  *
  * Note that significant differences exist between the two versions that
  * may "reasonably" lead is_feasible( true ) and is_feasible( false ) to
  * return different values:
  *
  * - When the Block has dynamic constraints, the "abstract representation"
  *   of the Block only contains those that have been explicitly generated so
  *   far, while the "physical representation" (logically) "contains them
  *   all". This means that is_feasible( true ) may return true while
  *   is_feasible( false ) may return false, as there might be dynamic
  *   constraints that have not been explicitly generated yet and that are
  *   violated by the current solution. This is not an issue, as it is always
  *   possible to call generate_dynamic_constraints() before is_feasible() to
  *   ensure that violated dynamic constraints, if any, are generated. Note
  *   that a call to is_feasible( false ) may well rely on the same separation
  *   procedures as generate_dynamic_constraints() to verify "logical"
  *   feasibility of the dynamic constraints; if the corresponding dynamic
  *   constraints are actually generated (a Block-specific decision), then
  *   is_feasible( false ) and is_feasible( true ), called in this order,
  *   should indeed give the same result.
  *
  * - A Block may have two types of Variable: "structural" ones and
  *   "auxiliary" ones. Consider for instance the case of a Knapsack problem:
  *   the standard representation of the problem only has x[ i ] variables
  *   corresponding to taking (or not) the objects in the knapsack, and the
  *   feasibility can be checked (trivially) using the "physical
  *   representation" and these variables alone, so this is what one assumes
  *   would happen when is_feasible( false ) is called. However, the
  *   "abstract representation" of the problem can be written in different
  *   ways, for instance in terms of the graph of the Dynamic Programming
  *   formulation: besides the x[ i ] variables, that would have some other
  *   (flow) variables f[ i , t ]. Hence is_feasible( true ) would have to
  *   look at both the x[] and f[] variables to work. It is therefore
  *   conceivable that the x[] part of the solution may be feasible, but the
  *   f[] part may be not; thus, is_feasible( true ) would return false,
  *   while is_feasible( false ) would return true.
  *
  * Since these methods are primarily provided for "debugging" Block and/or
  * Solver, it is intended that the user will be well-aware of what it is
  * testing; the two is_feasible() returning different values may well be
  * the valuable information that the process was meant to find.
  *
  * The parameter fsbc, which is a pointer to an arbitrarily complex
  * Configuration object, is meant to specify "how much approximately feasible
  * the solution can be". It can be a very simple quantity, such as a
  * SimpleConfiguration<double> specifying, say, the maximum relative
  * violation that a simple single numerical Constraint can have, or any
  * arbitrarily complex Configuration specifying different thresholds for
  * different groups of Constraint of the Block (say, via
  * SimpleConfiguration<std::vector<double> >), and arbitrarily complex
  * sub-Configurations (recursively) for the sub-Block of the Block. Also,
  * the parameter can be used to specify that only "a part" of the
  * feasibility check need be performed.
  *
  * Note that the fsbc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with fsbc = nullptr then the
  * corresponding configuration from the BlockConfig() is used. If the
  * BlockConfig is not set (nullptr) or the corresponding field is not set
  * (nullptr), this is assumed to mean "all Constraint must be satisfied
  * exactly", which may be possible in some cases (say, a numerical
  * Constraint only producing "small" integer numbers).
  *
  * The method is given a default implementation working for those Blocks for
  * which feasibility is never an issue, in the sense that they are feasible
  * if an only if all of its sub-Block are. This should basically mean that
  * the father Block has no Constraint on its own, and all Constraint are in
  * the sub-Block (which either mean that the Block is a separable problem,
  * or that there are either linking Variable or linking nonlinear terms in
  * the Objective). However, note that the default implementation only works
  * for the sub-Block being called with the default Configuration for this
  * task set by means of set_BlockConfig(). For any other case the :Block
  * will have to implement its own version; this cannot be done in the
  * default implementation because the Configuration fsbc for the father Block
  * will likely have to be "unpacked", with each sub-Block getting its own
  * specific sub-Configuration, but this can only be done by a specific
  * :Block for a specific :Configuration, as the base Configuration class
  * does not have direct support for the fact that a Configuration contains a
  * sub-Configuration for each sub-Block of a Block. */

 virtual bool is_feasible( bool useabstract = false ,
			   Configuration *fsbc = nullptr )
 {
  for( auto blck : v_Block )
   if( ! blck->is_feasible( useabstract ) )
    return( false );

  return( true );
  }

/*--------------------------------------------------------------------------*/
 ///< returns true if the current solution is (approximately) optimal
 /**< Returns true if the solution encoded in the current value of the
  * Variable of the Block can be proven to be approximately optimal within
  * the given tolerances. This might very well be a hard task, say if the
  * Block encodes for an NP-hard problem.
  *
  * A case in which this is possible is if the problem is convex (with a
  * compact dual), since then a dual feasible solution satisfying the
  * Complementary Slackness conditions with the "primal one" encoded in the
  * current value of the Variable of the Block is a convenient "compact"
  * optimality certificate. For such a case, this method can be taken as
  * being equivalent to "is_dual_feasible()" (which means that the primal
  * solution in the Variable may not actually be optimal, if it is either not
  * feasible or does not satisfy the Complementary Slackness conditions).
  *
  * The useabstract parameter being true dictates that the check should be
  * performed using the "abstract representation" of the Block, otherwise
  * the "physical representation" of the Block should be used.
  *
  * However, a significant difference exists between the two versions in case
  * the Block has dynamic variables. Indeed, in that case the abstract
  * representation of the Block only contains those that have been explicitly
  * generated so far, while the physical representation (logically) "contains
  * them all". This means that is_optimal( true ) may return true while
  * is_optimal( false ) may return false, as there might be dynamic variables
  * that have not been explicitly generated yet and that can be used to
  * improve the value of the current solution. This is not an issue, as it is
  * always possible to call generate_dynamic_variables() before is_optimal()
  * to ensure that useful dynamic variables, if any, are priced in. Note that
  * a call to is_optimal( false ) may well rely on the same pricing algorithm
  * as generate_dynamic_variables() to verify "logical" optimality; if the
  * corresponding dynamic variables are actually generated (a Block-specific
  * decision), then is_optimal( false ) and is_optimal( true ), called in this
  * order, should indeed give the same result. See also the comments to
  * is_feasible() for the cases where the check using the "abstract
  * representation" may give different results that that using the "physical"
  * one.
  *
  * The parameter optc, which is a pointer to an arbitrarily complex
  * Configuration object, is meant to specify "how much approximately optimal
  * the solution can be". It can be a very simple quantity, such as a
  * SimpleConfiguration<double> specifying, say, the maximum relative
  * violation that any simple single numerical constraint in the *dual* of a
  * convex problem may have, or, say, two such constants, one for dual
  * constraint violation and another for Complementary Slackness violations.
  * However, it can also be any arbitrarily complex Configuration, say
  * containing arbitrarily complex sub-Configurations (recursively) for the
  * sub-Block of the Block. Also, the parameter can be used to specify that
  * only "a part" of the optimality (dual feasibility) check need be performed.
  *
  * Note that the optc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which
  * means that if the method is called with optc = nullptr then the
  * corresponding configuration from the BlockConfig() is used. If the
  * BlockConfig is not set (nullptr) or the corresponding field is not set
  * (nullptr), this is assumed to mean "the solution must be exactly
  * optimal", which may be possible in some cases (say, a problem in which
  * both the primal and the dual solutions only contain "small" integer
  * numbers so that dual feasibility and Complementary Slackness can be
  * verified without any numerical error).
  *
  * The method is given a default implementation working for those Blocks for
  * which optimality is never an issue, in the sense that they are optimal
  * if an only if all of its sub-Block are. This is a "fake" implementation
  * that basically only works if the problem is actually decomposable, hence
  * one expects that true :Block will have to implement their own version,
  * not least because the Configuration optc for the father Block will likely
  * have to be "unpacked", with each sub-Block getting its own specific
  * sub-Configuration, which cannot be done with the base Configuration
  * class. */

 virtual bool is_optimal( bool useabstract = false  ,
			  Configuration *optc = nullptr )
 {
  for( auto blck : v_Block )
   if( ! blck->is_optimal( useabstract ) )
    return( false );

  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if the current solution is an unbounded ray
 /** Returns true if the values stored in the Variable of the Block are a
  * certificate that the problem is unbounded (either below, if it is a
  * minimization problem, or above if it is a maximization one). Often this
  * means that the values represent a ray of the feasible region along which
  * the Objective is unbounded (either below or above). Note that the
  * existence of an unbounded ray is not, strictly speaking, enough to prove
  * that the problem is unbounded: this also requires the problem to be
  * non-empty. This method is only required to check that the Variable
  * encode for a proper ray, with non-emptyness having to be established
  * in different ways (basically, this is a remit of the Solver).
  *
  * The useabstract parameter being true dictates that the check should be
  * performed using the "abstract representation" of the Block, otherwise
  * the "physical representation" of the Block should be used. See the
  * comments to is_feasible() for the cases where the check using the
  * "abstract representation" may give different results that that using
  * the "physical" one.
  *
  * Checking the property is likely to entail some numerical computation, say
  * to verify that some matrix-vector scalar product is "zero". This may
  * require numerical accuracy parameters, which is what the parameter fsbc
  * is designed to provide. If non-null, it is meant to point to an
  * arbitrarily complex Configuration object (although it can in fact be
  * as simple as a SimpleConfiguration<double> specifying, say, the maximum
  * relative accuracy in a "x == 0" computation). Also, the parameter can be
  * used to specify that only "a part" of the check, say considering only a
  * subset of the Variable, need be performed.
  *
  * Note that the fsbc parameter is meant as an *override* of the default
  * Configuration for is_feasible() set by means of set_BlockConfig(). That
  * is, if the method is called with fsbc = nullptr then the corresponding
  * configuration from the BlockConfig() is used. If the BlockConfig is not
  * set (nullptr) or the corresponding field is not set (nullptr), some
  * default value will have to be used. Note that the rationale for re-using
  * the is_feasible() configuration is mostly to avoid excessive proliferation
  * of Configuration objects in a Block; however, this also makes sense in at
  * least some important cases. For instance, in Linear Programming the
  * numerical tolerances for defining "a solution is feasible" and "a vector
  * is an unbounded ray" are basically the same. Yet, a Configuration object
  * can contain arbitrarily many values, so if the is_feasible() Configuration
  * requires more values to be specified to also cover the use within this
  * method, this can always be done.
  *
  * The method is given a default implementation always returning false, which
  * is appropriate for Block which cannot ever be unbounded (say, the feasible
  * region is compact). */

 virtual bool is_unbounded( bool useabstract = false ,
			    Configuration *fsbc = nullptr )
 {
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if the Block provably has no feasible solutions
 /** Returns true if the Block provably has no feasible solutions, and a
  * certificate for this is readily available. This might very well be a hard
  * task, say if the Block encodes for an NP-hard problem.
  *
  * A case in which this is possible is if the problem is convex, since then a
  * convenient way to prove emptyness of the primal is to prove that the dual
  * is unbounded (above if the primal is a minimization problem, below
  * otherwise). In turn, this can be proven by exhibiting a ray of the dual
  * feasible region along which the dual objective is unbounded (either above
  * or below). For such a case, this method can be taken as being equivalent
  * to "is_dual_unbounded()". More precisely, the method can be assumed to
  * check that the dual solution stored in the Block (however this is done)
  * represents the appropriate dual ray. Note that this may not, strictly
  * speaking, be enough to prove that the dual is unbounded: this also
  * requires the dual problem to be non-empty. This method is only required
  * to check that the dual solution encodes for a proper dual ray, with
  * non-emptyness of the dual having to be established in different ways
  * (basically, this is a remit of the Solver).
  *
  * The useabstract parameter being true dictates that the check should be
  * performed using the "abstract representation" of the Block, otherwise
  * the "physical representation" of the Block should be used. See the
  * comments to is_feasible() for the cases where the check using the
  * "abstract representation" may give different results that that using
  * the "physical" one.
 *
  * Checking the property is likely to entail some numerical computation, say
  * to verify that some matrix-vector scalar product is "zero". This may
  * require numerical accuracy parameters, which is what the parameter optc
  * is designed to provide. If non-null, it is meant to point to an
  * arbitrarily complex Configuration object (although it can in fact be
  * as simple as a SimpleConfiguration<double> specifying, say, the maximum
  * relative accuracy in a "x == 0" computation). Also, the parameter can be
  * used to specify that only "a part" of the check, say considering only a
  * subset of the Constraint, need be performed.
  *
  * Note that the fsbc parameter is meant as an *override* of the default
  * Configuration for is_optimal() set by means of set_BlockConfig(). That
  * is, if the method is called with optc == nullptr then the corresponding
  * configuration from the BlockConfig() is used. If the BlockConfig is not
  * set (nullptr) or the corresponding field is not set (nullptr), some
  * default value will have to be used. Note that the rationale for re-using
  * the is_optimal() configuration is mostly to avoid excessive proliferation
  * of Configuration objects in a Block; however, this also makes sense in at
  * least some important cases. For instance, in Linear Programming the
  * numerical tolerances for defining "a dual solution is feasible" and "a
  * vector is an unbounded ray of the dual" are basically the same. Yet, a
  * Configuration object can contain arbitrarily many values, so if the
  * is_optimal() Configuration requires more values to be specified to also
  * cover the use within this method, this can always be done.
  *
  * The method is given a default implementation always returning false, which
  * is appropriate for Block which cannot ever be empty. */

 virtual bool is_empty( bool useabstract = false ,
			Configuration *optc = nullptr )
 {
  return( true );
  }

/**@} ----------------------------------------------------------------------*/
/*------------------------- Methods for R3 Blocks --------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for R3 Blocks
 *
 * This section contain the methods that allow to implement one of the most
 * innovative features of SMS++: that of "R3 Blocks". The idea is that each
 * (derived class from) Block may (but need not necessarily) be capable of
 * producing "Block of different types that encode problems meaningfully
 * related to the original one". These are typically Block that encode:
 *
 * - A Reformulation, i.e., a different Block that encodes a problem whose
 *   optimal solutions are optimal also for the original Block, while being
 *   for some reason "more convenient to solve" by some specific algorithm.
 *   Note that Reformulations may be defined over a completely different
 *   space of Variables, provided that some appropriate mapping can be
 *   defined between the original and reformulated space. The mapping need
 *   not be algebraic, but must obviously be algorithmic [see
 *   map_back_solution() and map_forward_solution()].
 *
 * - A Relaxation, i.e., a different Block whose optimal value provides a
 *   valid global lower bound (for a minimization problem, upper bound for a
 *   maximization one) on the optimal value of the original Block. This is
 *   typically obtained by having a larger feasible region (e.g., relaxing
 *   some constraints) and/or an objective function whose value is smaller
 *   than the original one (for a minimization problem, larger for a
 *   maximization one) on the original feasible region. One also expects that
 *   the Relaxation is easier to solve than the original Block. Usually,
 *   a(n algorithmic) map between solutions of the Relaxation and those of the
 *   original problem is also available, although solutions of the Relaxation
 *   may clearly not be feasible for the original problem (but they may as
 *   well be; in this case, if the objective value also coincides, such a
 *   solution is actually optimal for the original problem).
 *
 * - A Restriction, i.e., a different Block whose optimal value provides a
 *   valid global upper bound (for a minimization problem, lower bound for a
 *   maximization one) on the optimal value of the original Block. This is
 *   typically obtained by having a smaller feasible region (e.g., adding
 *   some constraints, as in fixing some variables) and/or an objective
 *   function whose value is larger than the original one (for a minimization
 *   problem, smaller for a maximization one) on the original feasible region.
 *   One also expects that the Restriction is easier to solve than the
 *   original Block. Again, a(n algorithmic) map between solutions of the
 *   Restriction and those of the original problem is also usually available,
 *   and these solutions are most often feasible for the original problem.
 *
 * These three kinds of "derived" Blocks are often crucial to derive efficient
 * (as much as possible) solution methods for the original Block. The issue is
 * that there are very many ways to produce R3 Blocks, some of which requiring
 * rather complex maps between the variables space of the R3 Block and the
 * original one. In order to allow arbitrarily maps to be used, the base Block
 * class does not make any assumption on how the map is done, except that:
 *
 * - The original Block can map the current solution from an R3 Block to a
 *   solution of itself [see map_back_solution()] and vice-versa [see
 *   and map_forward_solution()]. There is no guarantee that the mapped back
 *   solution be feasible, say if the R3 Block is a relaxation, nor that
 *   the mapped forward solution is, say if the R3 Block is a restriction.
 *
 * - The original Block can in principle "translate" a Modification occurring
 *   to itself to a Modification of the R3 Block, see
 *   map_forward_Modification(). A Block can freely decide which
 *   Modification it will map.
 *
 * - The original Block can in principle "translate" a Modification occurring
 *   to the R3 Block to a Modification of itself, see map_back_Modification().
 *   A Block can freely decide which Modification it will map.
 *
 * Note that, in order to be able to perform these tasks, the original Block
 * will have to be able to access possibly everything in the R3 Block. Hence,
 * the R3 Block will either have to make the (specific derived class of) Block
 * friend, or will have to provide methods for accessing all the necessary
 * data structures, or to make them public outright.
 *
 *  @{ */

 /// produces a new R3 Block
 /** This method is the general interface by which a Block can produce a
  * related "R3 Block", i.e., typically:
  *
  * - a Reformulation, i.e., a different Block that encodes a problem whose
  *   optimal solutions are optimal also for the original Block;
  *
  * - a Relaxation, i.e., a different Block whose optimal value provides valid
  *   global lower/upper bounds for the optimal value of the original Block,
  *   assuming that was a minimization/maximization problem, while hopefully
  *   being easier to solve;
  *
  * - a Restriction, i.e., a different Block whose optimal value provides
  *   valid global upper/lower bounds for the optimal value of the original
  *   Block, assuming that was a minimization/maximization problem, while
  *   hopefully being easier to solve.
  *
  * The set of R3 Blocks of a given derived class from Block is defined by the
  * derived class itself; the base Block class provides no general R3 Block.
  * The only provision that the base Block class makes is that any Block
  * should allow for a very specific type of Reformulation: copy (producing a
  * Block that is identical to the original one). This is, however, not really
  * enforced: the expectation is that get_R3_Block() (with default = nullptr
  * parameter) is equivalent to a typical clone() method, but there is
  * nothing really forcing derived classes to abide to this provision.
  *
  * Note that, due to the basic design decision about the "names" of Variable
  * and Constraint, it is not in general possible to "copy Variable". Hence,
  * the newly produced Block -- even if just a straight copy -- will have
  * different Variable than the original one. Using solution information
  * from the new Block to (help) solv(e/ing) the old one, or vice-versa,
  * requires being able to map the new Variable into the old ones, or
  * vice-versa. This is something that specific derived classes, having
  * implemented the R3 transformation, can efficiently do, and therefore it
  * is demanded to them [see map_back_solution() and map_forward_solution()];
  * the base class provides no general mechanism for this, besides the
  * interface.
  *
  * Each derived class can produce, in principle, any number of different R3
  * Blocks; the parameter r3bc is a pointer to an arbitrarily complex
  * Configuration object which allows to tell which one has to be produced.
  * Of course, the R3 Block of a :Block with sub-Block may require each
  * sub-Block to produce R3 Block of its (recursively), hence the use of
  * Configuration that allows arbitrarily complex nested parameters to be
  * used.
  *
  * A specific point to be clarified concerns dynamic Constraint and
  * Variable. In principle, dynamic Constraint in Block are thought as "being
  * there even they are not there"; that is, they contribute to defining the
  * feasible region of the Block, which means that each feasible solution
  * must satisfy them all. Only, since they can be "many" not all of them are
  * explicitly constructed, but only "the subset that is useful for
  * algorithmic reasons". Similarly, dynamic Variable are "there even they are
  * not there": all dynamic Variable not explicitly generated are assumed to
  * be there in the Block set at their default value (most often, zero), and
  * it is assumed that this does not change the fact that the (explicitly
  * constructed part of the) solution is feasible. Hence, since the dynamic
  * Variable can be "many", only those that are actually necessary to encode
  * the optimal solution need be explicitly constructed.
  *
  * When an R3 Block is constructed, say a copy, two different cases may
  * occur:
  *
  * - the R3 Block inherits "all" the dynamic Constraint and/or Variable;
  *
  * - the R3 Block only inherits the dynamic Constraints and/or Variable that
  *   have been explicitly constructed in the original Block at the time when
  *   the R3 Block is constructed.
  *
  * The difference lies in the fact that the R3 Block may be "generic", i.e.,
  * constructed to only work with the abstract implementation of the original
  * Block. In this case, it cannot possibly really "know" all the dynamic
  * Variable/Constraint. In other words, generating Variable/Constraint may
  * require complex separation/pricing procedures: these are surely available
  * to the original Block, but may not be so to the R3 Block. If they are,
  * then the R3 Block may generate Variable/Constraint independently from the
  * original one: these can then "imported back" by the original Block with
  * appropriate methods [see map_back_modification()]. If they are not,
  * dynamic Variable/Constraint can only be generated by the original Block;
  * they may possibly be added to the R3 Block later on [see
  * map_forward_Modification()], if the original Block supports it.
  *
  * The method is given an extremely lazy default implementation refusing to
  * produce any kind of R3 Block, comprised the "copy" one. Thus, the caller
  * should always check the returned argument for non-nullptr-dness to
  * ensure that the :Block was actually able to produce the required R3 one. */

 virtual Block * get_R3_Block( Configuration *r3bc = nullptr )
 {
  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// maps back solution information from an R3 Block to the original Block
 /** Once a R3 Block has been produced [see get_R3_Block()], it will be
  * typically necessary to map solution information back from the R3 Block
  * to the original one. This method is assumed to be exactly this: R3B is
  * assumed to be a R3 Block produced by the current one of "type" r3bc (the
  * same, or identical, Configuration object used in get_R3_Block() to produce
  * R3B in the first place), and the Block should map back the solution
  * information contained into the Variables of R3B into its own.
  *
  * Note that in general the mapping can be highly nontrivial: the Variable
  * in the R3B can be less than the ones in the Block (in case of a
  * Restriction), but can even be a completely different set (in case of a
  * Reformulation that expresses the feasible region in an entirely different
  * way, think e.g. of the Dantzig-Wolfe reformulation). Hence the mapping may
  * not even be easy to write algebraically; yet, this is not a problem since
  * it is implemented algorithmically in this method.
  *
  * This method supports the general notion that "not all the solution might
  * be required", i.e., that a partial map (say, only some of the Variable)
  * may only be required. This is why the method has the third parameter solc,
  * which is intended to allow the caller to restrict the map to only
  * some subset of the Variable. Because, as usual, a Block can itself have
  * multiple "sets" of Variable, and also have sub-Block (recursively) each of
  * which can in turn have many ones, a Configuration object is required to be
  * able to specify any arbitrarily complex subset of the Variable.
  *
  * Actually, it may make sense in several scenarios that the thusly mapped
  * back solution information is then saved with a call to get_Solution().
  * This is why the solc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which is
  * shared between this method, get_Solution() and map_forward_solution().
  * That is, if the method is called with solc = nullptr then the
  * corresponding Configuration from the BlockConfig() is used. If the
  * BlockConfig is not set (nullptr) or the corresponding field is not set
  * (nullptr), this is assumed to mean "map them back all". If the original
  * Block (and hence, hopefully, the R3B) also has dual information attached
  * to the Constraint together than solution information stored in the
  * Variable, this should be taken to mean "copy both solution and dual
  * information".
  *
  * A specific twist of mapping back solutions is that the R3B may have,
  * after having been constructed, independently generated dynamic Variable
  * and/or Constraint [see get_R3_Block()]. If it has generated dynamic
  * Variable that are not present in the original Block, then it may not be
  * possible for the original Block to fully map back the solution in the R3B
  * (although it may also be possible, depending on exactly how the map is
  * done). In this case, map_back_solution() is assumed to do a best effort
  * attempt to copy as much as possible of the solution, but there is no
  * guarantee that the copied solution will be equivalent (in particular, it
  * may not be feasible even if the one in the R3B would have been so). A
  * similar case happens with dual variables of dynamic Constraint, assuming
  * they exist. However, it should always be possible to ensure that a full
  * map exists by ensuring that the dynamic Variable/Constraint of the R3B and
  * of the original Block are "in sync" *before* calling map_back_solution().
  * This should always be possible by using map_[back/forward]_modification(),
  * of course assuming that the original Block supports the corresponding
  * Modification. Alternatively, the Block may in principle be capable of
  * doing the adding of Variable/Constraint on-the-fly during the call to
  * this method, i.e., generate in itself all the Variable/Constraint that
  * are present in the R3B and that are necessary for the mapping to work;
  * of course, in this case the appropriate Modification should be issued.
  *
  * Note that the original Block will have to be able to access the Variable
  * of the R3 Block (and to its Constraint if dual information also has to
  * be mapped). This is obvious in case of a copy (as the R3 Block can be
  * expected to be of the exact same class as the original Block), but not so
  * otherwise. Hence, the R3 Block will either have to make the (specific
  * derived class of) the original Block friend, or will have to provide
  * methods for accessing all the necessary Variable/Constraint, or to make
  * them public outright.
  *
  * The method is given an extremely lazy default implementation refusing to
  * map back solution from any kind of R3 Block, comprised the "copy" one. */

 virtual void map_back_solution( Block *R3B , Configuration *r3bc = nullptr ,
				 Configuration *solc = nullptr )
 {
  throw( std::invalid_argument( "R3 Block type not supported" ) );
  }

/*--------------------------------------------------------------------------*/
 /// maps forward solution information from the original Block to n R3 Block
 /** Once a R3 Block has been produced [see get_R3_Block()], it might be
  * useful to map solution information from the original Block to the R3
  * Block, the reverse of what map_back_solution() does. This method is
  * assumed to be exactly this: R3B is assumed to be a R3 Block produced by
  * the current one of "type" r3bc (the same, or identical, Configuration
  * object used in get_R3_Block() to produce R3B in the first place), and the
  * Block should map the solution information contained into its own Variable
  * into these of the R3B. Similar observations apply as in
  * map_back_solution():
  *
  * - In general the mapping can be highly nontrivial to write algebraically,
  *   bit this is not a problem since it is implemented algorithmically.
  *
  * - This method supports the general notion that "not all the solution might
  *   be required", i.e., that a partial map (say, only some of the Variable)
  *   may only be required. This is why the method has the third parameter
  *   solc, and arbitrarily complex Configuration object which is intended to
  *   allow the caller to restrict the map to only some subset of the Variable
  *   (among the possibly many different ones of the father Block and those of
  *   each of its sub-Block, recursively). Again, the solc parameter is meant
  *   as an *override* of the default Configuration for this task set by means
  *   of set_BlockConfig(), which is shared between this method,
  *   get_Solution() and map_back_solution(). That is, if the method is
  *   called with solc = nullptr then the corresponding Configuration from
  *   the BlockConfig() is used. If the BlockConfig is not set (nullptr) or
  *   the corresponding field is not set (nullptr), this is assumed to mean
  *   "map them forward all". If the R3B (and hence, hopefully, the original
  *   Block) also has dual information attached to the Constraint together
  *   than solution information stored in the Variable, this should be taken
  *   to mean "copy both solution and dual information".
  *
  * - After having created the R3B, the original Block may have generated
  *   dynamic Variable and/or Constraint that are not present in the R3B.
  *   This may (or may not, depending on exactly how the map is done) make it
  *   impossible to map forward the full solution. In this case,
  *   map_forward_solution() is assumed to do a best effort attempt to copy as
  *   much as possible of the solution, but there is no guarantee that the
  *   copied solution will be equivalent; a similar case happens with dual
  *   information. However, it should always be possible to ensure that a full
  *   map exists by ensuring that the dynamic Variable/Constraint of the R3B
  *   and of the original Block are "in sync" *before* calling
  *   map_forward_solution(), which should always be possible by using
  *   map_[back/forward]_modification(). Alternatively, the Block may in
  *   principle be capable of adding to the R3B the Variable/Constraint that
  *   are necessary for the mapping to work on-the-fly during the call to
  *   this method, in which case the appropriate Modification should be
  *   issued.
  *
  * Note that the original Block will have to be able to access the Variable
  * of the R3 Block (and to its Constraint if dual information also has to
  * be mapped). This is obvious in case of a copy (as the R3 Block can be
  * expected to be of the exact same class as the original Block), but not so
  * otherwise. Hence, the R3 Block will either have to make the (specific
  * derived class of) the original Block friend, or will have to provide
  * methods for accessing all the necessary Variable/Constraint, or to make
  * them public outright.
  *
  * The method is given an extremely lazy default implementation refusing to
  * map forward solution from any kind of R3 Block, comprised the "copy" one.
  */

 virtual void map_forward_solution( Block *R3B ,
				    Configuration *r3bc = nullptr ,
				    Configuration *solc = nullptr )
 {
  throw( std::invalid_argument( "R3 Block type not supported" ) );
  }

/*--------------------------------------------------------------------------*/
 /// maps forward a Modification from the original Block to an R3 Block
 /** Once a R3 Block has been produced [see get_R3_Block()], it is a
  * completely independent object from the original Block that created it.
  * Hence, any modification to the original Block does not affect the R3
  * Block. Also, because the R3 Block may be "very different" from the
  * original one, a Modification in the latter may be nontrivial (or even
  * impossible) to map into one or more Modification to the former that keep
  * the R3 Block "in sync" with its original one.
  *
  * Because this may nonetheless be desirable in many scenarios, the base
  * Block class provides this method to support the case in which specific
  * modifications to the original Block should be applied "verbatim" to some
  * R3 Block of its. This actually means that the R3 Block will be subject to
  * some changes that "have the same effect of the original Modification",
  * whatever this exactly means for this R3 Block (and assuming this is
  * possible at all), so that specific Modification will be issued to any
  * Solver attached to the R3 Block.
  *
  * R3B is assumed to be a R3 Block produced by the current one of "type"
  * r3bc (the same, or identical, Configuration object used in get_R3_Block()
  * to produce R3B in the first place). mod is assumed to be a (smart)
  * pointer to a Modification object that applies to the current Block.
  *
  * One example of use of such a mechanism is when the original Block has
  * dynamic Variable/Constraint, but the R3B lacks the corresponding
  * pricing/separation methods and cannot therefore generate them itself.
  * It is, however, possible to "simulate" the generation of
  * Variable/Constraint by the R3B as follows:
  *
  * - copy the solution (primal and/or dual as it need be) from the R3B
  *   into the original Block [see map_back_solution()];
  *
  * - have the original Block generate whatever new dynamic
  *   Variable/Constraint as dictated by the imported solution;
  *
  * - intercept the corresponding Modification and use this method to have
  *   the same new Variable/Constraint be added to the R3B.
  *
  * Note that "intercepting" the Modifications is only possible for a Solver
  * attached to the original Block. However, any number of Solver can be
  * attached to a given Block, even if they are not actually used to solve
  * it (but only exploit being attached to snoop on the Modification
  * occurring in the Block).
  *
  *     IMPORTANT NOTE: A :Block SHOULD ONLY MAP EITHER ITS "PHYSICAL
  *     Modification" OR ITS "ABSTRACT Modification", BUT NOT BOTH.
  *
  * The reasonable behaviour should be to map "physical Modification", and
  * only map "abstract Modification" if there is no corresponding
  * "physical" one (say, a certain part of the :Block has no "physical
  * representation" distinct from the "abstract" one). Alternatively, a :Block
  * may decide to only map "abstract" ones. However, when one of the two
  * Modification is mapped, doing the same with the other is wasteful and a
  * likely source of errors. In fact, when a "physical Modification" is
  * mapped, it is intended that also the abstract representation of the Block
  * is modified. Symmetrically, when the abstract representation of the Block
  * is modified, this is supposed to be captured by add_Modification() so that
  * the physical representation is also updated. All in all, when one of the
  * two "equivalent" Modification corresponding to the same change has been
  * mapped, there is no reason (and good reasons not) to map the other.
  *
  * Mapping a Modification to another Block likely involves some new
  * Modification to be issued by that Block (and/or its Variable, Objective,
  * Constraint, Function, ...). The two parameters issuePMod and issueAMod
  * control how this is done for "physical Modification" and "abstract
  * Modification" respectively, with the format of Observer::make_par().
  * Note that for "physical Modification" the setting "eModBlock" makes no
  * sense because a "physical Modification" is never a concern of the :Block
  * that has issued it; in fact, its default value is eNoBlck, as opposed to
  * eModBlck for that of the "abstract Modification". Note that the "channel
  * names" in the parameters obviously have to refer to channels *of the
  * R3 Block R3B*, because it is there that the new Modification (if any)
  * will be issued.
  *
  * As mapping a Modification may be a rather complex (if at all possible)
  * task, a Block may not support all possible mappings. If the required
  * operation is not supported, the method will do nothing; however, it
  * will signal this by returning false, while it will return true if the
  * Modification has been correctly mapped. The rationale is that one can
  * therefore throw to this method all Modification without having to check
  * first which ones are supported, which would be complex. This is
  * especially important in view of the fact that for any change in the Block
  * there will typically be (at least) *two* Modification in flight, a
  * "physical" and an "abstract" one: rather than having to check which is
  * which and avoid to call this method on the "wrong" one, it is simpler to
  * just throw them all and have only the right one processed. Yet, if it is
  * crucial that the Modification is actually processed, the return value
  * allows to check that this has happened. The default implementation of
  * the method works for "extremely lazy" Block not being willing to
  * implement any of the possible mapping, or extremely unlucky ones not
  * having any workable one to implement. */

 virtual bool map_forward_Modification( Block *R3B , sp_Mod mod ,
					Configuration *r3bc = nullptr ,
					c_ModParam issuePMod = eNoBlck ,
					c_ModParam issueAMod = eModBlck )
 {
  return( false );
  }

/*--------------------------------------------------------------------------*/
 /// maps back a Modification from an R3 Block to the original Block
 /** Once a R3 Block has been produced [see get_R3_Block()], it is a
  * completely independent object from the original Block that created it.
  * Hence, any modification to the R3 Block does not affect the original Block.
  * Also, because the R3 Block may be "very different" from the original one,
  * a Modification in the former may be nontrivial (or even impossible) to
  * map into one or more Modification to the latter that keep the original
  * Block "in sync" with its R3 one.
  *
  * Because this may nonetheless be desirable in many scenarios, the base
  * Block class provides this method to support the case in which specific
  * Modifications to the R3 Block should be applied "verbatim" to its original
  * Block. This actually means that the original Block will be subject to some
  * changes that "have the same effect of the original Modification",
  * whatever this exactly means for this original Block (and assuming this is
  * possible at all), so that specific Modification will be issued to any
  * Solver attached to the original Block.
  *
  * R3B is assumed to be a R3 Block produced by the current one of "type"
  * r3bc (the same, or identical, Configuration object used in get_R3_Block()
  * to produce R3B in the first place). mod is assumed to be a (smart)
  * pointer to a Modification object that applies to the R3 Block.
  *
  * IMPORTANT NOTE: A :Block SHOULD ONLY MAP EITHER ITS "PHYSICAL
  * Modification" OR ITS "ABSTRACT Modification", BUT NOT BOTH. See
  * map_forward_Modification() for a discussion on this issue.
  *
  * Mapping a Modification from another Block likely involves some new
  * Modification to be issued by the current Block (and/or its Variable,
  * Objective, Constraint, Function, ...). The two parameters issuePMod and
  * issueAMod control how this is done for "physical Modification" and
  * "abstract Modification" respectively, with the format of
  * Observer::make_par(). Note that for "physical Modification" the setting
  * "eModBlock" makes no sense because a "physical Modification" is never a
  * concern of the :Block that has issued it; in fact, its default value is
  * eNoBlck, as opposed to eModBlck for that of the "abstract Modification".
  * Note that the "channel names" in the parameters obviously have to refer
  * to channels *of this Block* (as opposed to as of R3B), because it is
  * here that the new Modification (if any) will be issued.
  *
  * As mapping a Modification may be a rather complex (if at all possible)
  * task, a Block may not support all possible mappings. If the required
  * operation is not supported, the method will do nothing; however, it
  * will signal this by returning false, while it will return true if the
  * Modification has been correctly mapped. The rationale is that one can
  * therefore throw to this method all Modification without having to check
  * first which ones are supported, which would be complex. This is
  * especially important in view of the fact that for any change in the Block
  * there will typically be (at least) *two* Modification in flight, a
  * "physical" and an "abstract" one: rather than having to check which is
  * which and avoid to call this method on the "wrong" one, it is simpler to
  * just throw them all and have only the right one processed. Yet, if it is
  * crucial that the Modification is actually processed, the return value
  * allows to check that this has happened. The default implementation of
  * the method works for "extremely lazy" Block not being willing to
  * implement any of the possible mapping, or extremely unlucky ones not
  * having any workable one to implement. */

 virtual bool map_back_Modification( Block *R3B , sp_Mod mod ,
				     Configuration *r3bc = nullptr ,
				     c_ModParam issuePMod = eNoBlck ,
				     c_ModParam issueAMod = eModBlck )
 {
  return( false );
  }


/**@} ----------------------------------------------------------------------*/
/*----------------------- Methods for handling Solution --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Solution
 *  @{ */

 /// returns a Solution representing the current solution of this Block
 /** This method must construct and return a (pointer to a) Solution object
  * representing the current "solution state" of this Block. A Solution
  * would typically store the values of the static and dynamic Variable of
  * this Block, as well as the values of the dual variables of the Constraints
  * if any, although it can in principle do it in whatever format and without
  * explicit reference to the abstract representation of the Variable and
  * Constraints, which may not have been constructed. Indeed, the returned
  * object will clearly not be of the base Solution class, but of an
  * appropriate derived class, either specialized for the :Block at hand, or
  * at least "compatible" with it (e.g., only using the abstract
  * representation of the Variable and Constraint, which in this case must
  * then clearly have been constructed).
  *
  * This method supports the general notion that "not all the solution might
  * be required", i.e., that a Solution object may only store partial (dual)
  * solution information corresponding to only some subset of the Variable
  * (Constraint). Because, as usual, a Block can itself have multiple "sets"
  * of Variable/Constraint, and also have sub-Block (recursively) each of
  * which can in turn have many ones, a Configuration object is required to be
  * able to specify any arbitrarily complex subset of the solution information.
  * Note that the specified subset of the solution information is "baked in"
  * the returned ::Solution object, which will therefore for all its life only
  * read/write that part of the overall solution information.
  *
  * As usual, the solc parameter is meant as an *override* of the default
  * Configuration for this task set by means of set_BlockConfig(), which is
  * shared between this method, map_back_solution() and
  * map_forward_solution(). That is, if the method is called with solc =
  * nullptr then the corresponding Configuration from the BlockConfig()
  * is used. If the BlockConfig is not set (nullptr) or the corresponding
  * field is not set (nullptr), this is assumed to mean "save all solution
  * information". If the Block also has dual information attached to the
  * Constraint, this should be taken to mean "copy all of both solution and
  * dual information".
  *
  * Once constructed, a Solution object can read the (corresponding subset
  * of) solution information from the Block that has created any number of
  * times. Note, however, that (unless explicitly declared otherwise by the
  * specific ::Solution class), a Solution object can only read/write
  * solution information from *the very same* Block that created it, i.e.,
  * not even from Block of the same derived class. Since doing the copying
  * may have a nontrivial cost, the second parameter emptys specifies (if
  * false) that the returned Solution object must be already "loaded" with
  * the current (subset of) solution information of the Block, or (if true)
  * that the Solution object will be "empty" (un-initialized), so that it
  * will have to read() itself from the object before being significant.
  *
  * The default implementation of the method works for "extremely lazy" Block
  * that does not ever want to save any solution information, and/or relies to
  * "general-purpose" Solution objects that only use the abstract
  * representation of the Variable and Constraint to work. */

 virtual Solution * get_Solution( Configuration *solc = nullptr ,
				  bool emptys = true )
 {
  return( nullptr );
  }

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// returns the Block that this Observer is
 /** A Block is an Observer, so the Block that this Observer belongs to is
  * itself. However, note that const-ness has to be casted away from "this",
  * which is const in a const method. */

 Block * get_Block( void ) const override {
  return( const_cast< Block * >( this ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if there is any Solver "listening to this Block"
 /** Returns true if there is any Solver "listening to this Block", which
  * means either registered to this Bock or registered to any ancestor
  * (father, father of father, ...) of this Block. */

 bool anyone_there( void ) const override {
  return( f_at || ( ! v_Solver.empty() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tell a Block if someone "listening to" its father
 /** This method has to be called from the father Block to inform each of
  * his sons whether or not there is someone "listening to him", and
  * therefore to them. */

 void anyone_there( bool isthere );

/*--------------------------------------------------------------------------*/
 /// notify the Block about a Modification
 /** Block::add_Modification() implements the main mechanics of Modification
  * handling; in particular:
  *
  * - if chnl != 0, it "packs" the Modification into the appropriate
  *   GroupModification and does nothing else, which in particular means
  *   that it does *not* dispatch it to its father and the attached Solver
  *   (this being done by close_channel);
  *
  * - if chnl == 0, it rather immediately dispatches the Modification to its
  *   father and the attached Solver.
  *
  * While this mechanism is not thought to be modified by derived classes,
  * these *will* have to redefine add_Modification() to "catch" the
  * "abstract" Modification and use them to update the "physical
  * representation" of the :Block to keep it in sync with the "abstract"
  * representation. Any such re-implementation should follow the scheme
  *
  *     void SomeBlock::add_Modification( sp_Mod mod , ChnlName chnl )
  *     {
  *      if( mod->concerns_Block() ) {
  *       mod->concerns_Block( false );
  *       < handle "abstract" Modification >
  *       }
  *
  *      Block::add_Modification( mod , chnl );
  *      }
  *
  * The important aspect in this scheme is that
  *
  *     SomeBlock WILL "SEE" THE "ABSTRACT" Modification IMMEDIATELY,
  *     I.E., BEFORE IT IS "PACKED" INTO A GroupModification, EVEN IF
  *     IT IS BEING SENT TO SOME NON-0 CHANNEL
  *
  * (since the latter operation is handled by Block::add_Modification()).
  * This means that if SomeBlock handles some "atomic" :Modification that
  * changes its data structure, there is no risk that the :Modification is
  * "packed" into a GroupModification and held there for a long time before
  * the :Block has the chance of processing it. As a consequence:
  *
  * - A :Block DOES NOT HAVE TO HANDLE "ABSTRACT" GroupModification UNLESS
  *   THEY REFER TO CHANGES HAPPENING INTO SOME OF ITS sub-Block (HENCE,
  *   A "LEAF" Block WITHOUT ANY sub-Block NEVER HAS TO)
  *
  * - THE STATE OF THE DATA STRUCTURE IN THE :Block WHEN IT HANDLES THE
  *   "ABSTRACT" Modification IS PRECISELY THE ONE IN WHICH THE
  *   Modification WAS ISSUED: NO COMPLICATED OPERATIONS (Variable AND/OR
  *   Constraint BEING ADDED/REMOVED ...) CAN HAVE BEEN PERFORMED IN THE
  *   MEANTIME
  *
  * This assumption can drastically simplify the logic that a :Block has
  * to deploy to handle "abstract" Modification. */

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

/*--------------------------------------------------------------------------*/

 ChnlName open_channel( GroupModification * gmpmod = nullptr ,
			c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void nest_channel( c_ChnlName chnl , GroupModification * gmpmod = nullptr ,
		    c_ModParam issueMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void un_nest_channel( c_ChnlName chnl ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void close_channel( ChnlName chnl ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void set_default_channel( c_ChnlName chnl = 0 ) override;

/**@} ----------------------------------------------------------------------*/
/*---------------------- Methods for handling Solver -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Solvers
    @{ */

 c_Lst_Solver & get_registered_solvers( void ) const {
  return( v_Solver );
  }
 ///< reading the list of (pointers to) currently registered Solvers
 /**< Method for reading the list of (pointers to) the Solvers currently
  * registered with the Block. */

/*--------------------------------------------------------------------------*/
 /// adding a Solver to the set of those currently registered
 /** Method for adding a Solver to the set of those currently registered
  * with the Block. Note that the Block does sets itself to the Solver by
  * calling Solver::set_Block(), which is why the converse is not done (see
  * Solver.h). Note that the method is virtual because derived classes may
  * have to do more. */

 virtual void register_Solver( Solver *newSolver ) {
  if( v_Solver.empty() ) {    // this is the first Solver listening to me
   if( ! f_at ) {             // and no one was listening from above already
    for( auto el : v_Block )  // now someone is listening to all my sons
     el->anyone_there( true );
    }
   }
  else {                      // there are other Solver listening to me
   auto it = find( v_Solver.begin() , v_Solver.end() , newSolver );
   if( it != v_Solver.end() )  // the Solver is already there
    return;                    // silently return
   }

  newSolver->set_Block( this );
  v_Solver.push_back( newSolver );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removing oldSolver from the set of those currently registered
 /** Method for removing a Solver from the set of those currently registered
  * with the Block. If oldSolver is not among the registered solvers, then
  * nothing is done (and no warning is issued); otherwise the vector of
  * registered Solver is shortened by one, and the remaining solvers (if any)
  * are shifted in the obvious way. Note that the Block calls
  * Solver::set_Block( nullptr ) to the Solver that is un-registered, which is
  * why the converse is not done (see Solver.h). Note that the method is
  * virtual because derived classes may have to do more. */

 virtual void unregister_Solver( Solver *oldSolver ) {
  auto it = find( v_Solver.begin() , v_Solver.end() , oldSolver );
  if( it == v_Solver.end() )  // the Solver is not there
   return;                    // silently return

  oldSolver->set_Block( nullptr );
  v_Solver.erase( it );

  if( v_Solver.empty() && ( ! f_at ) ) {
   // this was the last solver listening to me, and nobody is listening
   // from above
   for( auto el : v_Block )    // now no one is listening to all my sons
    el->anyone_there( false );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removing the solver in position it of the set of the registered ones
 /** Method for removing a Solver from the set of those currently registered
  * with the Block. The parameter it is supposed to be the position into
  * the list returned by get_registered_solvers() where the Solver currently
  * is; the vector of registered Solver is therefore shortened by one, and
  * the remaining solvers (if any) are shifted in the obvious way. Note that
  * the iterator parameter is const because the only place where it might
  * have been taken from (except if the method is called from another Block
  * method) is get_registered_solvers(); however, we "cast away const-ness"
  * inside. Note that the Block calls Solver::set_Block( nullptr ) to the
  * Solver that is un-registered, which is why the converse is not done (see
  * Solver.h). Also, note that the method is virtual because derived classes
  * may have to do more.
  *
  * Warning: checking if an iterator really belongs to a list is complicated,
  * hence the method does not try to do that; clearly, calling the method
  * with something that is not an iterator of that list results in indefinite
  * behaviour. */

 virtual void unregister_Solver( c_Lst_Solver_it it )
 {
  unregister_Solver( v_Solver.erase( it , it ) );
  // cast away const-ness and call the protected version
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// replace an old Solver with a new Solver
 /** Method for substituting the Solver at the position "it" into the vector
  * returned by get_registered_solvers() with the given new Solver. Note that
  * the iterator parameter is const because the only place where it might
  * have been taken from (except if the method is called from another Block
  * method) is get_registered_solvers(); however, we "cast away const-ness"
  * inside. Note that the Block calls Solver::set_Block( nullptr ) to the
  * Solver that is replaced, which is why the converse is not done (see
  * Solver.h). Note that the method is virtual because derived classes may
  * have to do more.
  *
  * Warning: checking if an iterator really belongs to a list is complicated,
  * hence the method does not try to do that; clearly, calling the method
  * with something that is not an iterator of that list results in indefinite
  * behaviour. */

 virtual void replace_Solver( Solver *newSolver , c_Lst_Solver_it it )
 {
  replace_Solver( newSolver , v_Solver.erase( it , it ) );
  // cast away const-ness and call the protected version
  }

/*--------------------------------------------------------------------------*/
 /// create and set all the Solver attached to this Block (and its sub-Block)
 /** Method for creating, configuring and registering all the Solver that
  * the Block may need.
  *
  * The method uses a BlockSolverConfig, that holds:
  *
  * - a std::vector<std::string> containing the class names of the required
  *   :Solver;
  * - a std::vector<SolverConfig *> containing the corresponding algorithmic
  *   parameters;
  * - a std::vector<BlockSolverConfig *> containing the same information for
  *  each of the sub-Block of this Block;
  *
  * Note that the vector of SolverConfig * is allowed to be shorter than that
  * of names, in which case all the missing entries are treated as nullptr.
  * It is also allowed to be longer, in which case all the extra entries are
  * ignored.
  *
  * The BlockSolverConfig also has a field f_diff that indicates whether it
  * has to be interpreted in "differential mode". The behaviour of this
  * method is the following:
  *
  * - First, the list of Solver registered to this Block is scanned, and for
  *   each of them the corresponding elements in the BlockSolverConfig are
  *   examined. Then:
  *   = If f_diff == true
  *     * if the name of the Solver is empty then the Solver is left there,
  *       otherwise the existing solver is un-registered and deleted and a
  *       new solver is created and registered in that position
  *     * if the corresponding SolverConfig * is null then nothing is done,
  *       otherwise the SolverConfig * is passed to the Solver
  *   = If f_diff == false
  *     the existing solver is un-registered and deleted, then
  *     * if the name of the Solver is empty then the vector of registered
  *       Solver is shortened by one (any SolverConfig is ignored)
  *     * otherwise a new solver is created and registered in that position,
  *       and the corresponding SolverConfig is passed to it.
  *   Note that this would seem to not allow completely resetting the
  *   configuration of some existing Solver without changing it, but this is
  *   not true: it is sufficient to pass it a SolverConfig object (hence, not
  *   nullptr) which is "empty" (no parameter set) but with its f_diff field
  *   == false [see SolverConfig].
  *
  * - After the end of the list of currently registered Solver is reached (if
  *   ever), the behaviour is instead independent on the value of f_diff
  *   (adding to nothing is setting). If the name of the Solver is empty then
  *   the entry of the vector is ignored, otherwise a new solver is created
  *   and registered in that position (at the end) and the corresponding
  *   SolverConfig is passed to it (unless it is nullptr, because setting a
  *   nullptr configuration to a newly minted solver is useless).
  *
  * After this is done for this Block, the method is called recursively on
  * each sub-Block. Here, again, f_diff dictates how "void" information is
  * treated:
  *
  * - if f_diff == true and some BlockSolverConfig * is nullptr, then the
  *   corresponding sub-Block is ignored; if the vector of BlockSolverConfig
  *   is shorter than the number of sub-Block, all the non-specified ones are
  *   left unchanged;
  *
  * - if f_diff == false and some BlockSolverConfig * is nullptr, then
  *   set_SolverConfig( nullptr ) is called for the corresponding sub-Block;
  *   if the vector of BlockSolverConfig is shorter than the number of
  *   sub-Block, set_SolverConfig( nullptr ) is called for all the
  *   non-specified ones.
  *
  * Since a call to set_SolverConfig( nullptr ) would not make sense if the
  * "empty" SolverConfig were intended in the differential sense (it would do
  * nothing), when svcc == nullptr the field f_diff should be interpreted as
  * false. Hence, calling set_SolverConfig() will unregister and delete all
  * Solver attached to this Block and to each of its sub-Block.
  *
  * Important note: the moment when the Block is passed to the Solver, the
  * Solver should in principle do all the necessary initializations, since
  * immediately afterwords compute() may be called already. However, some
  * of the initializations could be heavily impacted by the algorithmic
  * parameters of the Solver. This means that
  *
  *     IT IS EXPECTED THAT, IN A Solver, set_ComputeConfig() SHOULD BE
  *     CALLED *BEFORE* set_Block() IS
  *
  * This is in fact how this is done here inside.
  *
  * Note an important difference between this method and register_Solver(),
  * unregister_Solver() and replace_Solver(): in the latter the new solver
  * have to be already constructed outside of Block, and the ones that get
  * un-registered are *not* deleted, which therefore has to be done by
  * whomever created them in the first place outside the Block. In this
  * method, instead, the Solver are directly constructed (using the Solver
  * factory) inside the Block, and correspondingly each Solver that gets
  * un-registered is also immediately deleted. Mixing the two styles of
  * managing Solver is therefore tricky and caution should be exercised.
  *
  * Finally, note that the BlockSolverConfig pointed by svcc is not changed
  * by the call, and it is *not* retained by the Block. This means that it
  * can (and should) be deleted after the call (provided it is not useful
  * later). */

 virtual void set_SolverConfig( BlockSolverConfig * svcc = nullptr );

/**@} ----------------------------------------------------------------------*/
/*--------------- Methods for handling the methods factory -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the methods factory
 *
 * The "methods factory" (more properly, methods factor*ies*) is a powerful
 * general concept that allows to interact with a :Block (mainly in the sense
 * of changing its data) whose type is not known at compile time, and yet
 * calling member functions of the specialized interface of the :Block.
 *
 * This is done by constructing a (bi-directional) map between names and
 * pointer to functions that could interact with a :Block (for example,
 * changing its data). In this way a pointer can be retrieved via its name
 * (a std::string that can be read at runtime, e.g. by some :Configuration
 * object) and the corresponding specialized function of the :Block can be
 * invoked (for instance, to change its data), even if the type of the
 * :Block is not known at compile time.
 *
 * This of course requires fixing the type of the functions. Actually, the
 * mechanism is flexible in that
 *
 *     IN PRINCIPLE ANY NUMBER OF FUNCTION TYPES IS POSSIBLE
 *
 * Indeed,
 *
 *      EACH TIME THE (PRIVATE) FUNCTION methods< F >() IS CALLED WITH A
 *      DIFFERENT FUNCTION TYPE F, WHICH IN TURN HAPPENS IF EITHER
 *      register_method( , F * ), OR get_method< F >(), OR
 *      get_method_name< F >() ARE CALLED, A NEW METHODS FACTORY FOR F IS
 *      AUTOMAGICALLY CREATED (AT COMPILE TIME)
 *
 * However, the issue is that
 *
 *      POINTERS IN THE MAPPING SHOULD NOT BE TO ACTUAL POINTERS TO CLASS
 *      MEMBEr FUNCTIONS IF THE METHODS FACTORY HAS TO BE EMPLOYED WITHOUT
 *      KNOWLEDGE OF THE SPECIFIC :Block INVOLVED (WHICH IS ITS DEFINING USE
 *      CASE). THIS IS BECAUSE THE FUNCTION CANNOT THEN HAVE A POINTER TO A
 *      SPECIFIC DERIVED CLASS FROM Block, AND THEREFORE IT MUST HAVE A
 *      POINTER TO THE BASE Block CLASS.
 *
 * This is not so say that constructing methods factories holding actual class
 * member functions is not possible, but any such factory would be tied to a
 * very specific :Block class, which goes squarely against the rationale for
 * having a methods factory in the first place.
 *
 * Thus, a bit of type-handling has to go on behind the scenes, which
 * requires some support. This is provided in two levels.
 *
 * For the first, more generic level, the (variadic template) types
 *
 *     template< class dBlock , typename ... Args >
 *     using MemberFunctionType =
 *           void ( dBlock::* ) ( Args ... , c_ModParam , c_ModParam );
 *
 *     template< typename ... Args >
 *     using FunctionType =
 *      std::function< void ( Block * , Args ... ,
 *                            c_ModParam , c_ModParam ) >;
 *
 * are defined. MemberFunctionType defines the interface of a generic class
 * member function that, besides any set of arguments, has two final ones
 * concerning if and how "physical" and "abstract" Modification are issued by
 * the (possible) change in the :Block brought about by the
 * function. FunctionType< Args > is the interface of an adapter function that
 * may correspond to MemberFunctionType< dBlock , Args >. This is what is
 * automatically constructed and put in the corresponding factory if
 *
 *     register_method< dBlock , Args >()
 *
 * is called. The adapter function simply static_cast< dBlock >()-s the
 * Block * and invokes the given function. Similarly,
 * get_method_fs< dBlock , Args >() and
 * get_method_name_fs< dBlock , Args >() are provided to search into the
 * corresponding FunctionType< Args > methods factories.
 *
 * A further level of support comes by defining some "general parameter type
 * lists" that functions in the methods factory should have. These should be
 * many enough to offer a reasonable flexibility, but on the other hand few
 * enough so that each methods factory is hopefully populated enough so that
 * the mechanism can be used often. For this purpose the following types are
 * defined:
 *
 * - Index, an index into any internal data structure;
 *
 * - Range, a pair of indices ( start , stop ) indicating the typical
 *   left-closed, right-open range { i : start <= i < stop };
 *
 * - Subset, an arbitrary subset of indices (currently a simple
 *   std::vector< Index >, although this may change) together with a
 *   [const] iterator [c_]Subset_it in it;
 *
 * - MF_dbl_it, a const_iterator into a std::vector< double >;
 *
 * - MF_int_it, a const_iterator into a std::vector< int >;
 *
 * These are thought to form the basis of "most" data-changing member
 * functions in any :Block class. In particular, six parameter type lists
 * are defined based on these, which have the form (clearly compatible with
 * the above types)
 *
 *     my_method_name( [ < data > , ] < slice > , c_ModParam , c_ModParam )
 *
 * where:
 *
 * - data is either not there, or a MF_dbl_it, or a MF_int_it;
 *
 * - slice indicates a subset of the data, in two possible forms:
 *
 *   = (range) a Range object (passed by value), yielding the function type
 *
 *         my_method_name( [ < data > , ] Range , c_ModParam , c_ModParam )
 *
 *   = (subset) an arbitrary subset of the data, yielding the function type
 *
 *         my_method_name( [ < data > , ] Subset && , const bool ,
 *                         c_ModParam , c_ModParam )
 *
 *     where the bool being true indicates that Subset is already ordered
 *     by increasing index on call (if not it can be ordered inside: anyway
 *     the Subset is &&, meaning that it is expected to be "consumed" by the
 *     function, e.g. to be shipped to some appropriate form of Modification).
 *
 *   Note that, if present, the provided  MF_X_it (call it "iter") must
 *   point to a std::vector< X > at least as long (after the position
 *   pointed by iter) as how many elements are there in the slice, and
 *   the X value *( iter + h ) has to be taken as the new value for the
 *   data structure in the :Block corresponding to the h-th Index in slice.
 *
 * These parameter type lists are "encoded" in the predefined six types
 * MS[_D]_S with D in { dbl , int } (or not there) and S in { rngd , sbst },
 * representing (in obvious ways) the six possible interfaces. Specific
 * versions of register_method(), get_method() and get_method_name() are
 * provided which take a final
 *
 *     MS[_D]_S::args()
 *
 * parameter (don't ask why the "args", that's pretty weird template wizardry)
 * specifying the type of function to be inserted in the methods factory.
 *
 * The rationale for defining these types is twofold:
 *
 * - make life a bit easier to the user, as the corresponding versions of
 *   register_method(), get_method() and get_method_name() are just a tiny
 *   bit easier to use;
 *
 * - gently nudge the user into adopting, as far as possible, these six
 *   parameter type lists for (as many as possible of) the data-changing
 *   functions of her :Block; this makes it straightforward to then register
 *   them in the methods factories, which greatly increases the value of the
 *   mechanism.
 *
 * Indeed, the "easy" use case of the methods factory is when the :Block has
 * member functions with precisely this structure. For instance, consider a
 * class NetworkBlock : Block representing (an optimization problem defined
 * over some) weighted graph. NetworkBlock may have two functions
 *
 *     void set_arc_weight( MF_dbl_it , Range , c_ModParam , c_ModParam );
 *
 *     void set_arc_weight( MF_dbl_it , Subset && , const bool ,
 *                          c_ModParam , c_ModParam );
 *
 * for changing the weights in the arcs of the graph. These functions can be
 * registered straight away in the methods factory by just calling anywhere
 *
 *     register_method< NetworkBlock , MF_dbl_it , Range>(
 *                                         "NetworkBlock::set_arc_weight" ,
 *                                         & NetworkBlock::set_arc_weight );
 *
 *     register_method< NetworkBlock , MF_dbl_it , Subset && , const bool >(
 *                                         "NetworkBlock::set_arc_weight" ,
 *                                         & NetworkBlock::set_arc_weight );
 *
 * (note how the first form is ever so slightly more convenient). Note that
 * the functions are overloaded, and get the same "name" in the factory:
 * however this is not an issue, because they end up in different factories.
 * Indeed, the "name" string can in general be arbitrary; yet, we recommend
 * following the pattern "ClassName::function_name" so as to avoid any name
 * collisions and to make it easier for the user to identify the available
 * functions. This is, of course, if there is one and only one underlying
 * function that is called, which may not be the case.
 *
 * Indeed, the above discussion clearly reveals that there is actually no need
 * for the functions in the :Block to have any of the pre-set parameter type
 * lists in order for them to be used in the methods factory. Indeed, even if
 * they do, an adapter function need to be written anyway (although this can
 * be, and is, done automatically). More in general, adapter functions can be
 * written to use existing :Block functions (or, conceivably, friend
 * functions) to perform changes that can be added to the methods factory.
 * For instance, assume that our fictional NetworkBlock class can only change
 * the weight of a single arc at a time via the function
 *
 *     void set_arc_weight( Index arc , double weight ,
 *                          c_ModParam issuePMod = eNoBlck ,
 *                          c_ModParam issueAMod = eModBlck );
 *
 * A user who would like to have a function in the methods factory for setting
 * the weight of subsets of arcs could implement the following functions:
 *
 *     void set_weight_range( Block * block , MF_dbl_it begin ,
 *                            Range range ,
 *                            c_ModParam issuePMod = eNoBlc ,
 *                            c_ModParam issueAMod = eModBlck ) {
 *      for( Index i = range.first ; i < range.second ; ++i , ++begin )
 *       static_cast<NetworkBlock *>( block )->set_arc_weight( i , *begin ,
 *                                                             issuePMod ,
 *                                                             issueAMod );
 *      }
 *
 *     void set_weight_subset( Block * block , MF_dbl_it begin ,
 *                             Subset && sbst , const bool ordered = false ,
 *                             c_ModParam issuePMod = eNoBlc ,
 *                             c_ModParam issueAMod = eModBlck ) {
 *      for( auto i : sbst )
 *       static_cast<NetworkBlock *>( block )->set_arc_weight( i ,
 *                                                             *(begin++) ,
 *                                                             issuePMod ,
 *                                                             issueAMod );
 *      }
 *
 * These could then be freely registered in the methods factory as in
 *
 *     register_method( "NetworkBlock::set_weight_range" ,
 *                      & set_weight_range );
 *
 *     register_method( "NetworkBlock::set_weight_subset" ,
 *                      & set_weight_subset );
 *
 * Note that the version taking a generic type F, in this case being a
 * FunctionType< Args ... > for appropriate Args, is being used here,
 * as opposed to the one taking a MemberFunctionType< dBlock , Args ... >. Of
 * course, any data format adapter can be implemented here; think e.g. of
 * the case where the :Block functions expect a boost::multi_array< double >,
 * that therefore has to be "flattened" into a std::vector< double >, or of
 * the case where the :Block functions expect something else than integer or
 * double values. Adapters still allow for the methods factory to be used in
 * these cases; save, of course, for the ever-present possibility of adding
 * new kinds of functions allowing the corresponding different input
 * capabilities, but this would increase the number of different methods
 * factories, possibly decreasing their utility.
 *
 * The typical place in which these calls to register_method() should be put
 * is in the protected static_initialization() function of the :Block.
 * However, somehow counter-intuitively, these member functions are public,
 * which means that anyone with access to a :Block can register "its
 * functions" (actually, adapter functions calling them) in the methods
 * factory. This allows to handle cases where:
 *
 * - the :Block owner couldn't be bothered to do the registration herself, but
 *   some user needs it;
 *
 * - some non-obvious adapter function has to be written, e.g. to support
 *   some new form of "Set" that the :Block owner did not know at the time
 *   where she wrote it.
 *
 * So, while one would expect that most of the registration work is done by
 * the :Block owner in static_initialization() once and for all, the
 * possibility is always left open that some registration may happen outside
 * it.
 *  @{ */

 /// register a new function in the methods factory
 /** This function registers the given \p function in the appropriate methods
  * factory specified by the template function type F, and associates it with
  * the given \p name. If the methods factory already has a function
  * associated with the given \p name, the currently present function is
  * replaced by the new one. Note that it is in principle possible to make a
  * factory for actual member functions of a :Block (say, F being like
  * "void ( NetworkBlock::* ) ( ... )"). This entails knowing a-priori that
  * the Block that will be used is a :Block (say, a NetworkBlock) or some of
  * its further derived classes. Such an occurrence is less likely to be
  * useful than that of having F being, say, a FunctionType< Set > so as to
  * allow any :Block, but it is still feasible.
  *
  * Although the name of the function can be arbitrarily chosen, it is
  * recommended to follow the pattern "ClassName::function_name" (insomuch as
  * this is possible, i.e., there is one and only one function with this name
  * corresponding to the inserted function pointer), so as to avoid any name
  * collisions and make it easier for the user to identify the available
  * functions.
  *
  * @param name The name that will identify the given \p function in the
  *             methods factory; as the "&&" tells, the std::string becomes
  *             "property" of the methods factory.
  *
  * @param function The (pointer to the) function to be added to the
  *                 corresponding methods factory. */

 template< class F >
 static void register_method( std::string && name , F * function )
 {
  if( name.empty() )
   throw( std::invalid_argument( "register_method: name is empty" ) );
  auto iter = methods< F >().left.find( name );
  if( iter != methods< F >().left.end() ) {
   delete iter->second;
   auto replaced = methods< F >().left.replace_data( iter , function );
   assert( replaced );
   }
  else
   methods< F >().insert( typename bimap< F >::value_type( std::move( name ) ,
							   function ) );
  }

/*--------------------------------------------------------------------------*/
 /// register a new ("class member") function in the methods factory
 /** This template function registers a class member function in the
  * appropriate methods factory, and associates it with the given \p name. It
  * has a template parameter and a variadic parameter pack. The template
  * parameter \p dBlock is the class (derived from Block) of which the
  * function is a member. The parameter pack \p Args specifies the parameter
  * type list (actually, part of it, without considering the two c_ModParam
  * parameters) of the function by means of
  * MemberFunctionType< dBlock , Args... >.
  *
  * This function serves as a wrapper for the "general" register_method< F >()
  * which has a single template parameter corresponding to the type F of
  * the function to be inserted in the methods factory. Indeed, what this
  * version does is to create a FunctionType< Args > lambda function
  * which just static_cast<> the Block * argument to a dBlock *, and
  * then invokes \p function.
  *
  * @param name The name that will identify the given \p function in the
  *             methods factory; as the "&&" tells, the std::string becomes
  *             "property" of the Block.
  *
  * @param fnct The pointer to the class member function whose adapter
  *             function is to be added to the corresponding methods factory.
  */

 template< class dBlock , typename ... Args >
 static void register_method( std::string && name ,
                              MemberFunctionType< dBlock , Args... > fnct )
 {
  // ensure dBlock derives from Block
  static_assert( std::is_base_of< Block , dBlock >::value ,
                 "register_method: dBlock must inherit from Block" );

  register_method( std::move( name ) ,
		   new FunctionType< Args... >(
		       [ fnct ]( Block * blck , Args&&... args ,
				 c_ModParam issuePMod ,
				 c_ModParam issueAMod )
		       {
			std::invoke( fnct ,
				     static_cast< dBlock * >( blck ) ,
				     std::forward< Args >( args )... ,
				     issuePMod , issueAMod );
		        } ) );
  }

/*--------------------------------------------------------------------------*/
 /// register a new function in the methods factory
 /** This template function registers a class member function in the
  * appropriate methods factory, and associates it with the given \p name. It
  * has a single template parameter, the \p dBlock class (derived from Block)
  * of which the function is a member.
  *
  * This function serves as a wrapper for the "general" register_method< F >()
  * which has a single template parameter corresponding to the type F of the
  * function to be inserted in the methods factory. Indeed, what this version
  * does is to create an adapter function which just static_cast<> the Block *
  * argument to a dBlock *, and then invokes \p function. However, the type of
  * the adapter function is now specified by means of the third parameter,
  * which is dummy arg_packer_helper<Args...>. This is intended to be used
  * with existing parameter type list-specifying types, such as in
  * MS_rngd::args() or MS_int_sbst::args(), although there is nothing
  * preventing from defining new ones.
  *
  * @param name The name that will identify the given \p function in the
  *             methods factory; as the "&&" tells, the std::string becomes
  *             "property" of the Block
  *
  * @param fnct The pointer to the class member function whose adapter
  *             function is to be added to the corresponding methods factory.
  *
  * @param void Dummy arg_packer_helper<Args...> parameter to specify the
  *             parameter type list of the function to be registered. */

 template< class dBlock , typename ... Args >
 static void register_method( std::string && name ,
			      MemberFunctionType< dBlock , Args... > fnct ,
			      arg_packer_helper<Args...> )
 {
  register_method< dBlock , Args... >( std::move( name ) , fnct );
  }

/*--------------------------------------------------------------------------*/
 /// returns the function with the given name in the methods factory
 /** This function returns a pointer to the function associated with the given
  * \p name in the methods factory specified by the template function type F.
  * If there is no function associated with the given \p name in that
  * factory, this method returns nullptr.
  *
  * Suppose, for example, that the methods factory has a function associated
  * with the name "NetworkBlock::set_arc_weight" that has the typical "double,
  * Range" interface, i.e., a #MF_dbl_it parameter, a #Range parameter, and
  * two c_ModParam parameters. This has been inserted in the interface under
  * the guise of a FunctionType< MF_dbl_it , Range > pointer. Thus,
  * to invoke such a function one should do
  *
  *     auto mthd = get_method< FunctionType< MF_dbl_it , Range > >(
  *                                        "NetworkBlock::set_arc_weight" );
  *     std::invoke( *mthd , NB , iter , range , issuePMod , issueAMod );
  *
  * where NB is a pointer to a NetworkBlock object (assuming the function
  * obtained from the methods factory is associated with this class, as it is
  * a good practice, considering the name of the function), iter is an
  * iterator of type #MF_dbl_it, range is a #Range, and issuePMod and
  * issueAMod are the last two parameters of the function.
  *
  * @param name The name associated with the function. */

 template< class F >
 static const F * get_method( const std::string & name ) {
  auto it = methods< F >().left.find( name );
  return( it != methods< F >().left.end() ? it->second : nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// returns the function with the given name in the methods factory
 /** This template function returns a pointer to the adapter function with
  * the given \p name in the methods factory corresponding to the function
  * type F implied by the variadic template parameter Args. Basically, this
  * function is equivalent to get_method< F > with
  * F == FunctionType< Args... >.
  *
  * Suppose, for example, that the methods factory has a function associated
  * with the name "NetworkBlock::set_arc_weight" that has the typical "double,
  * Range" interface, i.e., a #MF_dbl_it parameter, a #Range parameter, and
  * two c_ModParam parameters. This has been inserted in the interface under
  * the guise of a FunctionType< MF_dbl_it , Range > pointer. Thus,
  * to invoke such a function one should do
  *
  *     auto mthd = get_method_fs< MF_dbl_it , Range >(
  *                                        "NetworkBlock::set_arc_weight" );
  *     std::invoke( *mthd , NB , iter , range , issuePMod , issueAMod );
  *
  * where NB is a pointer to a NetworkBlock object (assuming the function
  * obtained from the methods factory is associated with this class, as it is
  * good practice considering the name given to the function), iter is an
  * iterator of type #MF_dbl_it, range is a #Range, and issuePMod and
  * issueAMod are the last two parameters of the function.
  *
  * @param name The name associated with the function. */

 template< typename... Args >
 static const FunctionType< Args... > * get_method_fs(
						  const std::string & name )
 {
  return get_method< FunctionType< Args... > >( name );
  }

/*--------------------------------------------------------------------------*/
 /// returns the function associated with the given name in the methods factory
 /** This function returns a pointer to the adapter function associated with
  * the given \p name in the methods factory implied by the second dummy
  * parameter.
  *
  * Suppose, for example, that the methods factory has a function associated
  * with the name "NetworkBlock::set_arc_weight" that has the typical "double,
  * Range" interface, i.e., a #MF_dbl_it parameter, a #Range parameter, and
  * two c_ModParam parameters. This has been inserted in the interface under
  * the guise of a FunctionType< MF_dbl_it , Range > pointer. Thus,
  * to invoke such a function one should do
  *
  *     auto mthd = get_method_fs( "NetworkBlock::set_arc_weight" ,
  *                                MS_dbl_rngd::args() );
  *     std::invoke( *mthd , NB , iter , range , issuePMod , issueAMod );
  *
  * where NB is a pointer to a NetworkBlock object (assuming the function
  * obtained from the methods factory is associated with this class, as it is
  * good practice considering the name given to the function), iter is an
  * iterator of type #MF_dbl_it, range is a #Range, and issuePMod and
  * issueAMod are the last two parameters of the function.
  *
  * @param name The name associated with the function.
  *
  * @param void Dummy arg_packer_helper<Args...> parameter to specify
  *             parameter type list of the function to be retrieved. */

 template< typename... Args >
 static const FunctionType< Args... > * get_method_fs(
		     const std::string & name , arg_packer_helper<Args...> )
 {
  return get_method< FunctionType< Args... > >( name );
  }

/*--------------------------------------------------------------------------*/
 /// returns the name that is associated with the given function
 /** This template function returns (a reference to) the name that is
  * associated with the given (pointer to a) function in the methods factory
  * specified by the template function type F. If the given function is not
  * present in that methods factory, a (reference to a)n empty string is
  * returned.
  *
  * @param fnct A pointer to the function whose associated name is desired. */

 template< class F >
 static const std::string & get_method_name( const F * fnct )
 {
  static const std::string empty;
  auto it = methods< F >().right.find( fnct );
  return( it != methods< F >().right.end() ? it->second : empty );
  }

/*--------------------------------------------------------------------------*/
 /// returns the name that is associated with the given function
 /** This template function returns (a reference to) the name that is
  * associated with the given (pointer to a) function in the methods factory
  * corresponding to the function type F implied by the variadic template
  * parameter Args. Basically, this function is equivalent to
  * get_method_name< F > with F == FunctionType< Args... >.
  *
  * @param fnct A pointer to the function whose associated name is desired. */

 template< typename... Args >
 static const std::string & get_method_name_fs(
			               const FunctionType< Args... > * fnct )
 {
  return get_method_name< FunctionType< Args... > >( fnct );
  }

/*--------------------------------------------------------------------------*/
 /// returns the name that is associated with the given function
 /** This template function returns (a reference to) the name that is
  * associated with the given (pointer to a) function in the methods factory
  * implied by the second dummy parameter.
  *
  * @param fnct A pointer to the function whose associated name is desired.
  *
  * @param void Dummy arg_packer_helper<Args...> parameter to specify the
  *             parameter type list of the function whose associated name is
  *             desired. */

 template< typename... Args >
 static const std::string & get_method_name_fs(
			               const FunctionType< Args... > * fnct ,
			               arg_packer_helper<Args...> )
 {
  return get_method_name< FunctionType< Args... > >( fnct );
  }

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Block ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the Block
 *
 * The base Block class provides two friend operator<<() and operator>>()
 * dispatching to protected virtual methods print( std::ostream& ) and
 * load( std::istream & ); the idea is that derived classes will implement the
 * latter two in order to provide input and output on std::stream.
 *
 * The base Block class also defines the interface for serializing and
 * de-serializing a :Block onto netCDF files. This is done via the three
 * versions of serialize() taking, respectively, a file name (char *), a
 * netCDF file and a netCDF group. The first dispatches on the second, and
 * the latter ultimately to the third, which is where the true
 * :Block-dependent serialization is supposed to happen.
 * @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way the
  * operator<<() is defined for each Block, but its behavior can be
  * customized by derived classes. */

 friend std::ostream & operator<< ( std::ostream & out , const Block& b ) {
   b.print( out );
   return( out );
   }

/*--------------------------------------------------------------------------*/
 /// friend operator>>(), dispatching to *pure* virtual protected load()
 /** Not really a method, but a friend operator>>() that just calls the
   * protected *pure* virtual method load(). This way the operator>>() is
   * defined for each Block, but it won't work for the case class, which is
   * abstract: it can only work for concrete derived classes which have
   * actually implemented load() (because they have some actual data to
   * load). */

 friend std::istream & operator>> ( std::istream & in , Block& b ) {
  b.load( in );
  return( in );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Block (recursively) to a netCDF file given the filename
 /** Top-level method to serialize a Block (recursively) to a file in
  * netCDF-based SMS++-format, given the filename and its type. See
  * deserialize( netCDF::NcFile & ) for details of the different file types.
  * Note that any existing content  of the file is overwritten, and that the
  * Block is saved as *the first one* in the newly created file.
  *
  * The base class implementation opens the netCDF file, creates the required
  * attribute "SMS++_file_type", assigns it the type, and dispatches to the
  * netCDF::NcFile & version of the method. If anything goes wrong with any
  * step of the process, exception is thrown. Although the method is virtual,
  * it is not expected that derived classes will have a need to re-define it.
  */

 virtual void serialize( const char *filename , const int type = eProbFile )
 {
  if( ( type != eProbFile ) && ( type != eBlockFile ) )
   throw( std::invalid_argument( "invalid SMS++ netCDF file type" ) );

  netCDF::NcFile f( filename , netCDF::NcFile::replace );

  f.putAtt( "SMS++_file_type" , netCDF::NcInt() , type );

  serialize( f , type );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Block (recursively) to an open netCDF file
 /** Second-level method to serialize a Block (recursively) to an open
  * netCDF file in netCDF-based SMS++-format. The type of the file, provided
  * as a parameter (mainly to make the signature of the method not ambiguous
  * with the serialize( netCDF::NcGroup ) one), must be the same as that
  * found in the :SMS++_file_type" attribute, with the meaning set out by the
  * enum smspp_netCDF_file_type [see SMSTypedefs.h].
  *
  * The current Block is *appended* after any existing Block in the file.
  *
  * The base class implementation creates the new group (and, if necessary,
  * child group) in the file and dispatches to serialize( netCDF::NcGroup ),
  * which is where the :Block-dependent serialization happens. If anything
  * goes wrong with any step of the process, exception is thrown. Although
  * the method is virtual, it is not expected that derived classes will have
  * a need to re-define it. */

 virtual void serialize( netCDF::NcFile & f , const int type )
 {
  if( ( type != eProbFile ) && ( type != eBlockFile ) )
   throw( std::invalid_argument( "invalid SMS++ netCDF file type" ) );

  const int idx = f.getGroupCount();

  netCDF::NcGroup bg;

  if( type == eProbFile ) {
   netCDF::NcGroup dg = f.addGroup( "Prob_" + std::to_string( idx ) );
   bg = dg.addGroup( "Block" );
   }
  else
   bg = f.addGroup( "Block_" + std::to_string( idx ) );

  serialize( bg );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Block (recursively) to a netCDF NcGroup
 /** Third, and final, level to serialize (recursively) to a netCDF NcGroup.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * All the information required to de-serialize the Block need be saved in
  * the provided netCDF NcGroup, which is assumed to be "empty", starting
  * with the "type" attribute that has to contain the name() of the Block.
  * Although each :Block is completely free to organize the netCDF NcGroup as
  * it best sees fit, the idea is that sub-Blocks (if any) should be saved
  * into child groups of the current group. The idea is that this should be
  * done in pre-order: the father Block will create the child groups for each
  * of its sons, and then call the sub-Block's serialize method with the right
  * NcGroup argument. This means that the sub-Block can rely on dimensions,
  * variables and attributes to have been declared at surrounding scope (in
  * the parent NcGroup) before they are called.
  *
  * An important note applies to serialize():
  *
  *      ANY :Block IS SERIALIZED "NAKED"
  *
  *   This means that the minimum amount of information required to fully
  *   reconstruct it should be saved; typically this is the "physical
  *   representation" of the Block, if any exists, maybe with the smallest
  *   possible amount of information about the "abstract representation" that
  *   is strictly required. As a consequence, nothing of the configuration of
  *   the Block (the BlockConfig and the BlockSolverConfig) is saved when the
  *   Block is serialized, even less so the attached solvers. The rationale
  *   is that Configuration objects can themselves be serialized, so if
  *   saving their current state is required this can (and it is better)
  *   done separately.
  *
  * The method of the base class just creates and fills the "type" attribute
  * (with the right name, thanks to the classname() method) and the optional
  * "name" attribute. It does *not* handle the sub-Block, because there can
  * hardly be any reasonably general way in which they can be structured
  * (there can be different groups of sub-Block with different properties).
  * By not even trying, we can leave in this method only things that are
  * sensible for each and every :Block. Because of this
  *
  *     THE serialize() METHOD OF ANY :Block SHOULD CALL Block::serialize()
  *
  * While this currently does so little that one might well be tempted to
  * skip the call and just copy the three lines of code, enforcing this
  * standard is forward-looking since in this way any future revision of the
  * base Block class may add other mandatory/optional fields: as soon as they
  * are managed by the (revised) method of the base class, they would then be
  * automatically dealt with by the derived classes without them even knowing
  * it happened. */

 virtual void serialize( netCDF::NcGroup & group ) const
 {
  group.putAtt( "type" , classname() );
  if( ! f_name.empty() )
   group.putAtt( "name" , f_name );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function<Block*(Block *)> BlockFactory;
 // type of the factory of Block

 typedef std::map<std::string,BlockFactory> BlockFactoryMap;
 // Type of the map between strings and the factory of Block

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for handling the "abstract representation"
 *
 * The following methods are the only ones that derived classes can use to
 * manipulate the four vectors v_s_Constraint, v_s_Variable, v_d_Constraint,
 * v_s_Variable, that are purposely *private*. This ensures that only pointers
 * of the right type can be found there; these being vectors of boost::any, it
 * would be very easy for derived classes to accidentally put there pointers
 * of the wrong type. Also, the methods ensure that the v_X_Y_names vectors
 * are kept of the same size as the corresponding v_X_Y ones. The add methods
 * accept an optional string to be put in the proper v_X_Y_names vector as an
 * arbitrary name for the new stuff; the v_X_Y_names vectors are protected,
 * so derived classes can mess up with them later at their leisure).
 *
 * There are two forms of the methods, for each of four combinations of
 *  X = "s" or "d" and Y = "Constraint" and "Variable":
 *
 * - add_X_Y( stuff [ , name , front ] ) that adds a new position to the
 *   corresponding vector of boost::any and puts a pointer to "stuff" there,
 *   (checking if the type is right), adds a new position to the corresponding
 *   std::vector< std::string > of names and puts "name" (if any) there;
 *   the optional parameter front tells, if false (default value), that the
 *   new position is added at the back of the std::vector-s, while if true
 *   that the new position is added at the front. Note that a form of the
 *   methods exist with "no stuff" (i.e., add_X_Y( [ name , front ] )) that
 *   just creates an empty slot in the vector of boost::any.
 *
 * - set_X_Y( Index , stuff [ , name ] ) that puts "stuff" in the position
 *   "Index" of the corresponding vector of boost::any (assumed existing,
 *   otherwise exception is thrown), and of course "name" (if any) in the
 *   same position to the corresponding std::vector< std::string > of names.
 *   Note that the current content of both vectors in that position is
 *   overwritten without any check, so it's the caller responsibility to
 *   ensure that nothing bad happens (like, erasing the only existing
 *   pointer to some stuff that was previously there before having deleted
 *   the stuff).
 *   
 * The methods will allow derived classes some flexibility in the order in
 * which the "abstract" representation is constructed, in particular for the
 * case in which this happens in different steps (say, a :Block class does
 * a part of it, but a further derived class does another part).
 *
 * For sake of consistency, set_Block( this ) is called on every new added
 * element; users may set another Block later at their own risk.
 * @{ */

 /// removes any existing static Constraint; to be used with care

 void reset_static_constraints( void )
 {
  v_s_Constraint.clear();
  v_s_Constraint_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing static Variable; to be used with care

 void reset_static_variables( void )
 {
  v_s_Variable.clear();
  v_s_Variable_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing dynamic Constraint; to be used with care

 void reset_dynamic_constraints( void )
 {
  v_d_Constraint.clear();
  v_d_Constraint_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing dynamic Variable; to be used with care

 void reset_dynamic_variables( void )
 {
  v_d_Variable.clear();
  v_d_Variable_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing objective; to be used with care

 void reset_objective( void ) { f_Objective = nullptr; }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_static_constraint( std::string && name = "" ,
			     bool front = false )
 {
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin() , boost::any() );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_s_Constraint.push_back( boost::any() );
   v_s_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// single object of class (derived from) Constraint

 template< class Const >
 void add_static_constraint( Const & newc , std::string && name = "" ,
			     bool front = false  )
 {
  // ensure derived classes insert a derivate of Constraint
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );

  newc.set_Block( this );
  Const * cnewc = &newc;
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin() , cnewc );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_s_Constraint.push_back( cnewc );
   v_s_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// single object of class (derived from) Constraint

 template< class Const >
 void set_static_constraint( Index i , Const & newc ,
			     std::string && name = "" )
 {
  // ensure derived classes insert a derivate of Constraint
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_s_Constraint" ) );

  newc.set_Block( this );
  Const * cnewc = &newc;
  v_s_Constraint[ i ] = cnewc;
  v_s_Constraint_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of (derived class from) Constraint

 template< class Const >
 void add_static_constraint( std::vector< Const > & newc ,
                             std::string && name = "" ,
			     bool front = false  )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );

  for( auto & c : newc )
   c.set_Block( this );

  std::vector< Const > * cnewc = &newc;
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin() , cnewc );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_s_Constraint.push_back( cnewc );
   v_s_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of (derived class from) Constraint

 template< class Const >
 void set_static_constraint( Index i , std::vector< Const > & newc ,
                             std::string && name = "" )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_s_Constraint" ) );

  for( auto & c : newc )
   c.set_Block( this );

  std::vector< Const > * cnewc = &newc;
  v_s_Constraint[ i ] = cnewc;
  v_s_Constraint_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of (...) Constraint

 template< class Const, unsigned long K >
 void add_static_constraint( boost::multi_array< Const , K > & newc ,
                             std::string && name = "" ,
			     bool front = false )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );

  for( auto i = newc.data() ; i < ( newc.data() + newc.num_elements() ) ; ++i )
   i->set_Block( this );

  boost::multi_array< Const, K > * cnewc = &newc;
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin() , cnewc );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_s_Constraint.push_back( cnewc );
   v_s_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of (...) Constraint

 template< class Const, unsigned long K >
 void set_static_constraint( Index i ,
			     boost::multi_array< Const , K > & newc ,
                             std::string && name = "" )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
                 "add_static_constraint: newc must inherit from Constraint" );
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_s_Constraint" ) );

  for( auto & c : newc )
   c.set_Block( this );

  boost::multi_array< Const, K > * cnewc = &newc;
  v_s_Constraint[ i ] = cnewc;
  v_s_Constraint_names[ i ] = std::move( name );
  }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_static_variable( std::string && name = "" , bool front = false )
 {
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin() , boost::any() );
   v_s_Variable_names.insert( v_s_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_s_Variable.push_back( boost::any() );
   v_s_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// single object of class (derived from) Variable

 template< class Var >
 void add_static_variable( Var & newv , std::string && name = "" ,
			   bool front = false )
 {
  // ensure derived classes insert a derivate of Variable
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );

  newv.set_Block( this );
  Var * cnewv = &newv;
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin() , cnewv );
   v_s_Variable_names.insert( v_s_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_s_Variable.push_back( cnewv );
   v_s_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// single object of class (derived from) Variable

 template< class Var >
 void set_static_variable( Index i , Var & newv ,
			   std::string && name = "" )
 {
  // ensure derived classes insert a derivate of Variable
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_s_Variable" ) );

  newv.set_Block( this );
  Var * cnewv = &newv;
  v_s_Variable[ i ] = cnewv;
  v_s_Variable_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of (derived class from) Variable

 template< class Var >
 void add_static_variable( std::vector< Var > & newv ,
                           std::string && name = "" , bool front = false )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );

  for( auto & v : newv )
   v.set_Block( this );

  std::vector< Var > * cnewv = &newv;
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin() , cnewv );
   v_s_Variable_names.insert( v_s_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_s_Variable.push_back( cnewv );
   v_s_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of (derived class from) Variable

 template< class Var >
 void set_static_variable( Index i , std::vector< Var > & newv ,
                           std::string && name = "" )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_s_Variable" ) );

  for( auto & v : newv )
   v.set_Block( this );

  std::vector< Var > * cnewv = &newv;
  v_s_Variable[ i ] = cnewv;
  v_s_Variable_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of (...) Variable

 template< class Var, unsigned long K >
 void add_static_variable( boost::multi_array< Var , K > & newv ,
                           std::string && name = "" , bool front = false )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );

  for( auto i = newv.data() ; i < ( newv.data() + newv.num_elements() ) ; ++i )
   i->set_Block( this );

  boost::multi_array< Var, K > * cnewv = &newv;
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin() , cnewv );
   v_s_Variable_names.insert( v_s_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_s_Variable.push_back( cnewv );
   v_s_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of (...) Variable

 template< class Var, unsigned long K >
 void set_static_variable( Index i , boost::multi_array< Var , K > & newv ,
                           std::string && name = "" )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_static_variable: newv must inherit from Variable" );
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_s_Variable" ) );

  for( auto & v : newv )
   v.set_Block( this );

  boost::multi_array< Var, K > * cnewv = &newv;
  v_s_Variable[ i ] = cnewv;
  v_s_Variable_names[ i ] = std::move( name );
  }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_dynamic_constraint( std::string && name = "" , bool front = false )
 {
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin() , boost::any() );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_d_Constraint.push_back( boost::any() );
   v_d_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// std::list of (derived class from) Constraint

 template< class Const >
 void add_dynamic_constraint( std::list< Const > & newc ,
                              std::string && name = "" , bool front = false )
 {
  // ensure derived classes insert a derivate of Constraint
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );

  for( auto & c : newc )
   c.set_Block( this );

  std::list< Const > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin() , cnewc );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_d_Constraint.push_back( cnewc );
   v_d_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::list of (derived class from) Constraint

 template< class Const >
 void set_dynamic_constraint( Index i , std::list< Const > & newc ,
                              std::string && name = "" )
 {
  // ensure derived classes insert a derivate of Constraint
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_d_Constraint" ) );

  for( auto & c : newc )
   c.set_Block( this );

  std::list< Const > * cnewc = &newc;
  v_d_Constraint[ i ] = cnewc;
  v_d_Constraint_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of std::list of (...) Constraint

 template< class Const >
 void add_dynamic_constraint( std::vector< std::list< Const > > & newc ,
                              std::string && name = "" , bool front = false )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );

  for( auto & c : newc )
   for( auto & j : c )
    j.set_Block( this );

  std::vector< std::list< Const > > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin() , cnewc );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_d_Constraint.push_back( cnewc );
   v_d_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of std::list of (...) Constraint

 template< class Const >
 void set_dynamic_constraint( Index i ,
			      std::vector< std::list< Const > > & newc ,
                              std::string && name = "" )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_d_Constraint" ) );

  for( auto & c : newc )
   for( auto & j : c )
    j.set_Block( this );

  std::vector< std::list< Const > > * cnewc = &newc;
  v_d_Constraint[ i ] = cnewc;
  v_d_Constraint_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of std::list of (...) Constraint

 template< class Const, unsigned long K >
 void add_dynamic_constraint( boost::multi_array< std::list< Const > , K >
                              & newc , std::string && name = "" ,
			      bool front = false )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );

  for( auto i = newc.data() ; i < ( newc.data() + newc.num_elements() ) ; ++i )
   for( auto & j : *i )
    j.set_Block( this );

  boost::multi_array< std::list< Const >, K > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin() , cnewc );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin() ,
				std::move( name ) );
   }
  else {
   v_d_Constraint.push_back( cnewc );
   v_d_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of std::list of (...) Constraint

 template< class Const, unsigned long K >
 void set_dynamic_constraint( Index i ,
			      boost::multi_array< std::list< Const > , K >
                              & newc , std::string && name = "" )
 {
  static_assert( std::is_base_of< Constraint, Const >::value,
               "add_dynamic_constraint: newc must inherit from Constraint" );
  if( i >= v_d_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_d_Constraint" ) );

  for( auto & c : newc )
   for( auto & j : c )
    j.set_Block( this );

  boost::multi_array< std::list< Const >, K > * cnewc = &newc;
  v_d_Constraint[ i ] = cnewc;
  v_d_Constraint_names[ i ] = std::move( name );
  }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_dynamic_variable( std::string && name = "" , bool front = false )
 {
  if( front ) {
   v_d_Variable.insert( v_d_Variable.begin() , boost::any() );
   v_d_Variable_names.insert( v_d_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_d_Variable.push_back( boost::any() );
   v_d_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*--------------------------------------------------------------------------*/
 /// std::list of (derived class from) Variable

 template< class Var >
 void add_dynamic_variable( std::list< Var > & newv ,
                            std::string && name = "" , bool front = false )
 {
  // ensure derived classes insert a derivate of Variable
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );

  for( auto & v : newv )
   v.set_Block( this );

  std::list< Var > * cnewv = &newv;
  if( front ) {
   v_d_Variable.insert( v_d_Variable.begin() , cnewv );
   v_d_Variable_names.insert( v_d_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_d_Variable.push_back( cnewv );
   v_d_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::list of (derived class from) Variable

 template< class Var >
 void set_dynamic_variable( Index i , std::list< Var > & newv ,
                            std::string && name = "" )
 {
  // ensure derived classes insert a derivate of Variable
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_d_Variable" ) );

  for( auto & v : newv )
   v.set_Block( this );

  std::list< Var > * cnewv = &newv;
  v_d_Variable[ i ] = cnewv;
  v_d_Variable_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of std::list of (...) Variable

 template< class Var >
 void add_dynamic_variable( std::vector< std::list< Var > > & newv ,
                            std::string && name = "" , bool front = false )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );

  for( auto & v : newv )
   for( auto & j : v )
    j.set_Block( this );

  std::vector< std::list< Var > > * cnewv = &newv;
  if( front ) {
   v_d_Variable.insert( v_d_Variable.begin() , cnewv );
   v_d_Variable_names.insert( v_d_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_d_Variable.push_back( cnewv );
   v_d_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// std::vector of std::list of (...) Variable

 template< class Var >
 void set_dynamic_variable( Index i , std::vector< std::list< Var > > & newv ,
                            std::string && name = "" )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_d_Variable" ) );

  for( auto & v : newv )
   for( auto & j : v )
    j.set_Block( this );

  std::vector< std::list< Var > > * cnewv = &newv;
  v_d_Variable[ i ] = cnewv;
  v_d_Variable_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of std::list of (...) Variable

 template< class Var, unsigned long K >
 void add_dynamic_variable( boost::multi_array< std::list< Var > , K >
			    & newv , std::string && name = "" ,
			    bool front = false )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );

  for( auto i = newv.data() ; i < ( newv.data() + newv.num_elements() ) ; ++i )
   for( auto & j : *i )
    j.set_Block( this );

  boost::multi_array< std::list< Var >, K > * cnewv = &newv;
  if( front ) {
   v_d_Variable.insert( v_d_Variable.begin() , cnewv );
   v_d_Variable_names.insert( v_d_Variable_names.begin() ,
			      std::move( name ) );
   }
  else {
   v_d_Variable.push_back( cnewv );
   v_d_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array<K> of std::list of (...) Variable

 template< class Var, unsigned long K >
 void set_dynamic_variable( Index i ,
			    boost::multi_array< std::list< Var > , K >
			    & newv , std::string && name = "" )
 {
  static_assert( std::is_base_of< Variable, Var >::value,
                 "add_dynamic_variable: newv must inherit from Variable" );
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_d_Variable" ) );

  for( auto & v : newv )
   for( auto & j : v )
    j.set_Block( this );

  boost::multi_array< std::list< Var >, K > * cnewv = &newv;
  v_d_Variable[ i ] = cnewv;
  v_d_Variable_names[ i ] = std::move( name );
  }

/**@} ----------------------------------------------------------------------*/
/** @name Protected methods for handling Solver list
 *
 * These are the protected versions of the same-named public methods, which
 * take non-const iterators: they do the brunt of the job avoiding to
 * cast away the const-ness. Of course they can only be called by someone
 * having access to the protected v_Solver fields, i.e., Block or a :Block.
 *
 * @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removing the solver in position it of the set of the registered ones
 virtual void unregister_Solver( Lst_Solver_it it )
 {
  (*it)->set_Block( nullptr );  // unregister the Block in the Solver
  v_Solver.erase( it );  // erase the solver from its position in the list

  if( v_Solver.empty() && ( ! f_at ) ) {
   // this was the last solver listening to me, and nobody is listening
   // from above
   for( auto el : v_Block )    // now no one is listening to all my sons
    el->anyone_there( false );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// replace an old Solver with a new Solver

 virtual void replace_Solver( Solver *newSolver , Lst_Solver_it it )
 {
  (*it)->set_Block( nullptr );
  (*it) = newSolver;
  }

/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 *
 * The Block class provides two pairs of symmetric print() / load() and
 * serialize() / deserialize() methods to save information about it on a
 * std::stream / netCDF::NcGroup and retrieve it. For print() the saved
 * information may or may not be enough to fully reconstruct the original
 * Block via load(), depending on the verbosity level, while for serialize()
 * the saved information is always enough to fully reconstruct the original
 * Block via deserialize().
 *
 * For conditions that need be respected by load() see the comments to
 * deserialize( netCDF::NcGroup & ), while for those that need be respected
 * by print() see comments to serialize( netCDF::NcGroup & ).
 * @{ */

 /// print information about the Block on an ostream with the given verbosity
 /** Protected method intended to print information about the Block; it is
  * virtual so that derived classes can print their specific information in
  * the format they choose.
  * The level of the verbosity of the printed information is defined by the
  * verbosity_lvl field of the class; in particular, the "complete" level is
  * is assumed to save enough information to allow a Block to completely
  * re-read its structure from the file. This makes little sense for the base
  * Block class, in that it has no real structure of its own to save. */

 virtual void print( std::ostream &output ) const;

/*--------------------------------------------------------------------------*/
 /// load the Block out of an istream
 /** *pure virtual* method intended to provide support for Blocks to load
  * themselves out of an istream. This is precisely what makes Block an
  * *abstract* base class: the actual content of the Block depends on the
  * specific derived class, which is why this method cannot be implemented.
  *
  * If there is any Solver "interested" to this Block, then a NBModification
  * *must* be issued to "inform" them that anything it knew about the Block is
  * now completely outdated. This is *not* optional (and therefore no issueMod
  * param is provided), because the reaction of a Solver to an NBModification
  * should be akin to clearing the list of all previous Modification. Indeed,
  * since these are no longer relevant and, worse, they may refer to elements
  * of the Block that simply no longer exist; thus, they cannot possibly be
  * processed in any meaningful way, which is why the NBModification cannot be
  * avoided. This is unless the Block is only a sub-Block of the Block that
  * the Solver is solving, in which case Modification pertaining to other
  * parts of the Block still are relevant; see the comments to
  * Solver::add_Modification. Note that the NBModification is sent to the
  * "default channel", since it "must be seen immediately" rather then being
  * "hidden" into any GroupModification.
  *
  * It is also important to remark that
  *
  *      AFTER load() THE :Block IS UN-CONFIGURED
  *
  * Although clearly not "empty", as opposed as :Block fresh out of the
  * factory (see new_Block( string )), a freshly loaded Block is otherwise
  * "in pristine state": the "abstract representation" is not constructed
  *  (unless the :Block does this by its own volition), both the BlockConfig
  * and the BlockSolverConfig are not set, and (therefore) there are no Solver
  * attached, unless there were before. */

 virtual void load( std::istream &input ) = 0;

/**@} ----------------------------------------------------------------------*/
/** @name Protected methods for handling static fields
 *
 * These methods allow derived classes to partake into static initialization
 * procedures performed once and for all at the start of the program. These
 * are typically related with factories.
 * @{ */

 /// method encapsulating the Block factory
 /** This method returns the Block factory, which is a static object. The
  * rationale for using a method is that this is the "Construct On First Use
  * Idiom" that solves the "static initialization order problem". */

 static BlockFactoryMap & f_factory( void );

/*--------------------------------------------------------------------------*/
 /// empty placeholder for class-specific static initialization
 /** The method static_initialization() is an empty placeholder which is made
  * available to derived classes that need to perform some class-specific
  * static initialization besides these of any :Block class, i.e., the
  * management of the factory. This method is invoked by the
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h] during the
  * standard initialization procedures. If a derived class needs to perform
  * any static initialization it just have to do this into its version of
  * this method; if not it just has nothing to do, as the (empty) method of
  * the base class will be called. One such activity that :Block classes
  * should always consider doing is adding their data-changing methods (or
  * adapters for those) to the corresponding "methods factories", see the
  * comments in the appropriate section.
  *
  * This mechanism has a potential drawback in that a redefined
  * static_initialization() may be called multiple times. Assume that a
  * derived class X redefines the method to perform something, and that a
  * further class Y is derived from X that has to do nothing, and that
  * therefore will not define Y::static_initialization(): then, within the
  * SMSpp_insert_in_factory_cpp_* of Y, X::static_initialization() will be
  * called at least twice (once for X and once for Y).
  *
  * If this is undesirable, X will have to explicitly instruct derived
  * classes to redefine their (empty) static_initialization(). Alternatively
  * (and preferably), X::static_initialization() may contain mechanisms to
  * ensure that it will actually do things only the very first time it is
  * called. One standard trick is to do everything within the initialisation
  * of a static local variable of X::static_initialization(): this is
  * guaranteed by the compiler to happen only once, regardless of how many
  * times the function is called. Alternatively, an explicit static boolean
  * could be used (this may just be the same as what the compiler does during
  * the initialization of static variables without telling you). */

 static void static_initialization( void ) { }

/**@} ----------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 Vec_Block v_Block;
 ///< vector of pointers of the nested blocks inside the Block

 Lst_Solver v_Solver;
 ///< list of pointers to the registered Solvers with this Block

 bool f_at;  ///< true if there is any Solver "listening" to this Block

 verbosity_type verbosity_lvl;  ///< the verbosity level of the Block

 std::string f_name;            ///< the string name of the Block
 
 Vec_string v_s_Constraint_names;   ///< the names of the static Constraints
 /**< vector to store the name of the different types of static constraints of
  * the Block. v_s_Constraint_names[ i ] (if nonempty) is the name of the set
  * of static Constraints v_s_Constraint[ i ]; hence, the two vectors must
  * have the same size. */

 Vec_string v_s_Variable_names;     ///< the names of the static Variables
 /**< vector to store the name of the different types of static variables of
  * the Block. v_s_Variable_names[ i ] (if nonempty) is the name of the set of
  * static Variables v_s_Variable[ i ]; hence, the two vectors must have the
  * same size. */

 Vec_string v_d_Constraint_names;   ///< the names of the dynamic Constraints
 /**< vector to store the name of the different types of dynamic constraints
  * of the Block. v_d_Constraint_names[ i ] (if nonempty) is the name of the
  * set of dynamic Constraints v_d_Constraint[ i ]; hence, the two vectors
  * must have the same size. */

 Vec_string v_d_Variable_names;     ///< the names of the dynamic Variables
 /**< vector to store the name of the different types of dynamic variables of
  * the Block. v_d_Variable_names[ i ] (if nonempty) is the name of the set
  * of dynamic Variable v_d_Variable[ i ]; hence, the two vectors must have
  * the same size. */

 BlockConfig *f_BlockConfig;        ///< the BlockConfig for this Block

 std::vector<GroupModification *> v_current_GroupMod;
 ///< the vector of current GroupModification of the Block

 unsigned int f_channel;   ///< the "default GroupModification channel"

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 template<class F>
 using bimap = boost::bimap< std::string , const F * >;
 ///< a bidirectional map for the methods factory

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Block::private_name() (pure virtual)

 virtual const std::string & private_name( void ) const = 0;

/*--------------------------------------------------------------------------*/
/** This method removes the given Constraint from each Variable that
 * is active in it. */

 void remove_constraint_from_variables( Constraint * constraint );

/*--------------------------------------------------------------------------*/
/** This method removes the given Variable from all Constraints and Objectives
 * in which it is active. Since the removal of a Variable from a Constraint or
 * Objective may result in a Modification being thrown, this Modification can
 * be avoided by setting the parameter throw_individual_modifications to
 * false [see remove_dynamic_variables()]. */

 void remove_variable_from_stuff( Variable * const variable ,
				  const int issueindMod  );

/*--------------------------------------------------------------------------*/
/// returns the bimap associated with the methods of type F
/** This method returns the bimap implementing the "methods factory" for the
 * methods of type F. This is where the pointer to the methods (and their
 * names) in the methods factory are stored. */

 template<class F>
 static inline bimap<F> & methods( void ) {
  static bimap<F> methods;
  return( methods );
  }

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 Block *f_Block;
 ///< pointer to the block where the current Block is nested (if any)

 Objective * f_Objective;     ///< the objective function of the Block
 /**< A pointer to the objective function of the Block */

 Vec_any v_s_Constraint;        ///< the static Constraints of the Block
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] Constraint */

 Vec_any v_s_Variable;          ///< the static Variables of the Block
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] [classes derived from] Variable */

 Vec_any v_d_Constraint;        ///< the dynamic Constraints of the Block
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] lists of [classes derived from] Constraint */

 Vec_any v_d_Variable;          ///< the dynamic Variables of the Block
 /**< vector of pointers to [multi/single dimensional arrays of]
  * [pointers to] lists of [classes derived from] Variable */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

};  // end( class( Block ) )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BlockMod ------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Modification for "simple" modifications to a Block
/** Derived class from Modification to describe "simple" modifications to a
 *  Block, i.e., the Objective has changed.
 */

class BlockMod : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 /// constructor: takes the Block and the "concerns" value

 BlockMod( Block *fblock , bool cB = false )
  : AModification( cB ) , f_Block( fblock ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BlockMod() = default;   ///< destructor, does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the pointer to the Block to which the Modification refers

 Block * get_Block( void ) const override  { return( f_Block ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*------------------- PROTECTED METHODS OF THE CLASS -----------------------*/
 /// print the BlockMod

 virtual inline void print( std::ostream &output ) const override {
  output << "BlockMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Block [" << &f_Block << "]: obj changed" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Block *f_Block;  ///< reference to the Block to which the Modification refers

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockMod ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BlockModAD ------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from AModification for adding/removing stuff to a Block
/** Derived class from AModification to describe modifications to a Block
 * involving either the addition or the removal of dynamic either Variable or
 * Constraint. This is a base class for all Modification of this type, which
 * are actually represented by BlockModAdd and BlockModRmv. However, these
 * are template classes, and therefore are a bit more cumbersome to "catch"
 * because you need to know the exact type of Variable / Constraint involved.
 * This base class only conveys the general information that some Variable or
 * Constraint have been either added or removed. It does not say *which*, but
 * it does say *how*. If some Solver is only interested in this, it can
 * "catch" the base class irrespective to the type of Variable / Constraint
 * involved. */

class BlockModAD : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor, taking the "concerns" value

 BlockModAD( bool cB = false ) : AModification( cB ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BlockModAD() = default;  ///< destructor, does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// returns true if a Variable is involved, false if a Constraint is involved
 /** Returns true if a Variable is involved, false if a Constraint is
  * involved. The method is pure virtual and it is actually implemented by
  * derived classes. */

 virtual bool is_variable( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if < something > is added, false if it is removed
 /** Returns true if < something > is added, false if it is removed. The
  * method is pure virtual and it is actually implemented by derived classes.
 */

 virtual bool is_added( void ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores the pointers to the affected Variable into the given vector
 /** If this BlockModAD is related to Variable, then this function stores the
  * pointers of the affected Variable in the given \p variables vector. The \p
  * variables vector is resized to the number of affected Variable. If this
  * BlockModAD is not related to Variable, the elements of the given vector
  * are erased from it and the size of the vector becomes zero.
  *
  * @param variables the vector in which the pointers to the affected Variable
  *        will be stored.
  */

 virtual void get_elements( std::vector< Variable * > & variables ) const = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// stores the pointers to the affected Constraint into the given vector
 /** If this BlockModAD is related to Constraint, then this function stores
  * the pointers of the affected Constraint in the given \p constraints
  * vector. The \p constraints vector is resized to the number of affected
  * Constraint. If this BlockModAD is not related to Constraint, the elements
  * of the given vector are erased from it and the size of the vector becomes
  * zero.
  *
  * @param constraints the vector in which the pointers to the affected
  *        Constraint will be stored.
  */

 virtual void get_elements( std::vector< Constraint * > & constraints )
  const = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModAD ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BlockModAdd -----------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Modification for adding stuff to a Block
/** Derived class from BlockModAD to describe modifications to a Block
 * involving the *addition* of dynamic Variable or Constraint. Note that no
 * pointer to the affected Block is required, since it can always be inferred
 * from the other information (Constraint/Variable) that the Modification
 * contains. The class is template over the type of the Constraint or Variable
 * that have been added, which must be either a :Constraint or a :Variable. */

template< class ConstOrVar >
class BlockModAdd : public BlockModAD
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor, taking all the data of the Modification
 /** Constructor, taking:
  *
  * @param the std::list< ConstOrVar > & whc, a reference to the list where
  *        the Constraint or Variable (of type ConstOrVar) have been added;
  *
  * @param the std::vector< ConstOrVar * > && add, containing the pointers
  *        to the Constraint or Variable (of type ConstOrVar) that have been
  *        added; as the "&&" suggests, the object becomes property of the
  *        BlockModAdd;
  *
  * @param the bool cB, containing the "concerns" value.
  *
  * Note that a pointer to the affected Block can always be inferred from the
  * other information that the Modification contains, and therefore is not
  * needed. */

 BlockModAdd( std::list< ConstOrVar > & whc ,
	      std::vector< ConstOrVar * > && add , bool cB = false )
  : BlockModAD( cB ) , whc_list( whc ) , add_vec( std::move( add ) )
 {
  static_assert( std::is_base_of< Variable , ConstOrVar >::value ||
		 std::is_base_of< Constraint , ConstOrVar >::value ,
		 "BlockModAD: must inherit from Variable or Constraint" );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, no specific code needed (all is done automatically)

 virtual ~BlockModAdd() = default;

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 Block * get_Block( void ) const override final {
  return( add_vec[ 0 ]->get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// accessor to (the reference to) the affected list of Constraint/Variable

 std::list<ConstOrVar> & whc( void ) const { return( whc_list ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// accessor to the array of the added Constraint/Variable

 const std::vector<ConstOrVar *> & added( void ) const { return( add_vec ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_variable( void ) const override final {
  return( std::is_base_of< Variable , ConstOrVar >::value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual bool is_added( void ) const override final { return( true ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_elements( std::vector< Variable * > & variables )
  const override {
  get_elements_( variables );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_elements( std::vector< Constraint * > & constraints )
  const override {
  get_elements_( constraints );
  }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModAdd

 virtual inline void print( std::ostream &output ) const override {
  output << "BlockModAdd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: added " << add_vec.size();
  if( std::is_base_of< Variable , ConstOrVar >::value )
   output << " Variable";
  else
   output << " Constraint";
  output << " from list" << whc_list << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 std::list< ConstOrVar > & whc_list;   ///< reference to the affected list

 std::vector< ConstOrVar * > add_vec;  ///< vector of pointers to added stuff

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*--------------------------- PRIVATE METHODS ------------------------------*/

 template<class T>
 void get_elements_( std::vector< T * > & elements ) const {
  if constexpr( std::is_base_of< T , ConstOrVar >::value )
   elements.assign( add_vec.cbegin() , add_vec.cend() );
  else
   elements.clear();
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModAdd ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BlockModRmv -----------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockModAD for removing stuff to a Block
/** Derived class from BlockModAD to describe modifications to a Block
 * involving the removal of dynamic Variable or Constraint. Note that no
 * pointer to the affected Block is required, since it can always be inferred
 * from the other information (Constraint/Variable) that the Modification
 * contains. The class is template over the type of the Constraint or Variable
 * that have been removed, which must be either a :Constraint or a :Variable.
 */

template< class ConstOrVar >
class BlockModRmv : public BlockModAD
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor, taking all the data of the Modification
 /** Constructor, taking:
  *
  * @param the std::list< ConstOrVar > & whc, a reference to the list from
  *        which the Constraint or Variable (of type ConstOrVar) have been
  *        removed;
  *
  * @param the std::list< ConstOrVar > && rmvd, containing the *actual*
  *        Constraint or Variable (of type ConstOrVar) that have been
  *        removed from the std::list< ConstOrVar > & whc; as the "&&"
  *        suggests, the object becomes property of the BlockModRmv, which
  *        is crucial for proper timely disposal of the objects themselves
  *        (see comments to the destructor);
  *
  * @param the bool cB, containing the "concerns" value.
  *
  * Note that a pointer to the affected Block can always be inferred from the
  * other information that the Modification contains, and therefore is not
  * needed. */

 BlockModRmv( std::list< ConstOrVar > & whc ,
	      std::list< ConstOrVar > && rmvd , bool cB = false )
  : BlockModAD( cB ) , whc_list( whc ) , rmvd_list( std::move( rmvd ) )
 {
  static_assert( std::is_base_of< Variable , ConstOrVar >::value ||
		 std::is_base_of< Constraint , ConstOrVar >::value ,
		 "BlockModRmv: must inherit from Variable or Constraint" );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, *apparently* doing nothing
 /** Although the destructor of BlockModRmv seems to be empty, it actually
  * performs a very important and nontrivial task, i.e., destroying the list
  * of removed Constraint / Variable (the actual objects), held in the
  * rmvd_list field. This happens automagically when the last Solver having
  * received the BlockModRmv deletes the pointer, hence when it is actually
  * safe to delete the Constraint / Variable because no-one still need to
  * access them to see what they held and what was their "name = pointer". */

 virtual ~BlockModRmv() = default;

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 Block * get_Block( void ) const override final {
  return( rmvd_list.front().get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// accessor to (the reference to) the affected list of Constraint/Variable

 std::list< ConstOrVar > & whc( void ) const { return( whc_list ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 /// accessor to the list of the removed Constraint/Variable

 const std::list< ConstOrVar > & removed( void ) const { return( rmvd_list ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_variable( void ) const override final {
  return( std::is_base_of< Variable , ConstOrVar >::value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_added( void ) const override final { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_elements( std::vector< Variable * > & variables )
  const override {
  get_elements_( variables );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void get_elements( std::vector< Constraint * > & constraints )
  const override {
  get_elements_( constraints );
  }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModRmv

 virtual inline void print( std::ostream &output ) const override {
  output << "BlockModRmv[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: removed " << rmvd_list.size();
  if( std::is_base_of< Variable , ConstOrVar >::value )
   output << " Variable";
  else
   output << " Constraint";
  output << " from list" << whc_list << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 std::list< ConstOrVar > & whc_list;     ///< reference to the affected list

 std::list< ConstOrVar > rmvd_list;      ///< list of removed stuff

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*--------------------------- PRIVATE METHODS ------------------------------*/

 template<class T>
 void get_elements_( std::vector< T * > & elements ) const {
  if constexpr( std::is_base_of< T , ConstOrVar >::value ) {
   elements.resize( rmvd_list.size() );
   auto it = elements.begin();
   auto it2 = rmvd_list.begin();
   while( it != elements.end() )
    *it++ = const_cast< ConstOrVar * >( &*it2++ );
   }
  else
   elements.clear();
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModRmv ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BlockConfig -----------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Configuration for the Block configuration parameters
/** Derived class from Configuration to describe all the parameters that a
 *  Block may have, which are:
 *
 * - the Configuration for static Constraint;
 * - the Configuration for dynamic Constraint;
 * - the Configuration for static Variable;
 * - the Configuration for dynamic Variable;
 * - the Configuration for the Objective;
 * - the Configuration for is_feasible();
 * - the Configuration for is_optimal();
 * - the Configuration related to solutions (get_Solution() and
 *   map_[back/forward]_solution.
 *
 * It is always possible to define a specific :BlockConfig corresponding to a
 * specific :Block, but in order to avoid this as much as possible an "extra"
 * Configuration is also available. Due to the flexibility of Configuration,
 * this may be enough to cover many use cases without a specific :BlockConfig.
 */

class BlockConfig : public Configuration
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: initializes everything to "default configuration"

 BlockConfig( void ) : Configuration() ,
  f_static_constraints_Configuration( nullptr ),
  f_dynamic_constraints_Configuration( nullptr ),
  f_static_variables_Configuration( nullptr ),
  f_dynamic_variables_Configuration( nullptr ),
  f_objective_Configuration( nullptr ),
  f_is_feasible_Configuration( nullptr ),
  f_is_optimal_Configuration( nullptr ),
  f_solution_Configuration( nullptr ) ,
  f_extra_Configuration( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 BlockConfig( const BlockConfig &old );

/*--------------------------------------------------------------------------*/
 /// "extends" Configuration::deserialize( netCDF::NcFile ) to eProbFile
 /** Since a BlockConfig knows it is a BlockConfig, it "knows its place" in
  * an eProbFile netCDF SMS++ file. */

 static BlockConfig * deserialize( netCDF::NcFile & f ,
				   const unsigned int idx = 0 );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a BlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should contain the following:
  *
  * - the group "static_constraints" containing a Configuration object for
  *   the static Constraint of the Block;
  *
  * - the group "dynamic_constraints" containing a Configuration object for
  *   the dynamic Constraint of the Block;
  *
  * - the group "static_variables" containing a Configuration object for
  *   the static Variable of the Block;
  *
  * - the group "dynamic_variables" containing a Configuration object for
  *   the dynamic Variable of the Block;
  *
  * - the group "objective" containing a Configuration object for the
  *   objective of the Block;
  *
  * - the group "is_feasible" containing a Configuration object for the
  *   is_feasible() method of the Block;
  *
  * - the group "is_optimal" containing a Configuration object for the
  *   is_optimal() method of the Block;
  *
  * - the group "solution" containing a Configuration object for the
  *   methods of the Block dealing with solutions (get_Solution() and
  *   map_[back/forward]_solution());
  *
  * - the group "extra" containing a Configuration object, which has no
  *   direct use in the base Block class, but is added so that derived
  *   classes can put there any configuration information without having to
  *   define further derived classes form BlockConfig (which, however, they
  *   can still do if they want);
  *
  * - the dimension "n_sub_Block" containing the number of BlockConfig
  *   descriptions for the sub-Block of the current Block;
  *
  * - with n being the size of n_sub_Block, n groups, with name
  *   "sub-BlockConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a BlockConfig for one of the sub-Block of the current
  *   :Block.
  *
  * Of all these, only the "name" attribute and the "n_sub_Block" dimension
  * are mandatory (they must exist, although they may be empty/zero). All
  * other groups may not exist, in which case the corresponding field of the
  * class is filled with a nullptr, indicating that the "default"
  * configuration (whatever that may mean for the :Block in question) have
  * to be used. Note that that the matching between the sub-BlockConfig and
  * the sub-Block is positional: the BlockConfig found in the group
  * "sub-BlockConfig_<i>" is that for the i-th sub-Block. Note that the
  * vector of sub-BlockConfig is allowed to be of different size than the
  * number of sub-Block; if it is larger any extra BlockConfig is simply
  * ignored, if it shorted then all missing sub-BlockConfig are treated as
  * nullptr (default configuration). */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all sub-BlockConfig
 virtual ~BlockConfig()
 {
  for( auto sBC : v_sub_BlockConfig )
   delete sBC;

  delete f_extra_Configuration;
  delete f_solution_Configuration;
  delete f_is_optimal_Configuration;
  delete f_is_feasible_Configuration;
  delete f_objective_Configuration;
  delete f_dynamic_variables_Configuration;
  delete f_static_variables_Configuration;
  delete f_dynamic_constraints_Configuration;
  delete f_static_constraints_Configuration;
  }

/*------------------------------- CLONE -----------------------------------*/

 virtual BlockConfig * clone( void ) const override
 {
  return( new BlockConfig( *this ) );
  }

/*--------------------------------------------------------------------------*/
 /// extends Configuration::serialize( netCDF::NcFile , type ) to eProbFile
 /** Since a BlockConfig knows it is a BlockConfig, it "knows its place" in
  * an eProbFile netCDF SMS++ file. */

 virtual void serialize( netCDF::NcFile & f , const int type )
  const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a BlockConfig. See BlockConfig::deserialize( netCDF::NcGroup )
  * for details of the format of the created netCDF group. */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/*--------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 /// the Configuration for generate_abstract_constraints()
 Configuration *f_static_constraints_Configuration;
 /// the Configuration for generate_dynamic_constraints()
 Configuration *f_dynamic_constraints_Configuration;
 /// the Configuration for generate_abstract_variables()
 Configuration *f_static_variables_Configuration;
 /// the Configuration for generate_dynamic_variables()
 Configuration *f_dynamic_variables_Configuration;
 /// the Configuration for set_objective()
 Configuration *f_objective_Configuration;
 /// the Configuration for is_feasible()
 Configuration *f_is_feasible_Configuration;
 /// the Configuration for is_optimal()
 Configuration *f_is_optimal_Configuration;
 /// the Configuration for get_Solution() and map_[back/forward]_solution
 Configuration *f_solution_Configuration;
 /// any extra Block-specific Configuration
 Configuration *f_extra_Configuration;

 /// the vector of sub-BlockConfig for each of the sub-Block
 std::vector<BlockConfig *> v_sub_BlockConfig;

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BlockConfig
 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this BlockConfig out of an istream
 /** Load this BlockConfig out of an istream, with the following format:
  *
  * for all of:  static constraints Configuration ,
  *              dynamic constraints Configuration ,
  *              static variables Configuration ,
  *              dynamic variables Configuration ,
  *              objective Configuration ,
  *              is_feasible Configuration ,
  *              is_optimal Configuration ,
  *              solution Configuration ,
  *              extra Configuration
  *              (in this order)
  *
  *  - a string containing the class type of a Configuration object, '*'
  *    means none (nullptr)
  *
  * - if the above is not '*', the description of the :Configuration object
  *
  * number k of the sub-BlockConfig objects
  *
  * for i = 1 ... k
  *  - a string containing the class type of a BlockConfig object,
  *    '*' means none (nullptr)
  *
  *  - if the above is not '*', the description of the :BlockConfig object
  */

 virtual void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockConfig ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BlockSolverConfig --------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Configuration for configuring the Solver of the Block
/** Derived class from Configuration to configure in one all the Solver of
 * a given Block, comprised those of the sub-Block (recursively), each with
 * all its algorithmic parameters.
 *
 * It contains three fields:
 * - a vector of strings containing the names of Solver to be attached to
 *   the Block;
 * - a vector of ComputeConfig* for these same Solver
 * - a vector of BlockSolverConfig for each of the sub-Block of this Block.
 *
 * It also contains a bool field f_diff which, if true, tells that the
 * BlockSolverConfig has to be "interpreted in a differential sense": this
 * means that all Solver whose name is not specified (empty string) must be
 * left in their current state, all nullptr ComputeConfig correspond to not
 * changing the configuration of the Solver, and all nullptr
 * BlockSolverConfig correspond to not changing any of the configurations of
 * any of the Solver attached to the corresponding sub-Block. */

class BlockSolverConfig : public Configuration
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: initializes everything to "nothing happens"
 BlockSolverConfig( void ) : Configuration() , f_diff( true ) { }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin
 BlockSolverConfig( const BlockSolverConfig &old );

/*--------------------------------------------------------------------------*/
 /// extends Configuration::deserialize( netCDF::NcFile ) to eProbFile
 /** Since a BlockSolverConfig knows it is a BlockSolverConfig, it "knows its
  * place" in an eProbFile netCDF SMS++ file. */

 static BlockSolverConfig * deserialize( netCDF::NcFile & f ,
					 const unsigned int idx = 0 );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a BlockSolverConfig. Besides the mandatory "type" attribute of
  * any :Configuration, the group should contain the following:
  *
  * - the dimension "n_SolverConfig" containing the number of Solver that
  *   are to be attached to this Block, and therefore the number of their
  *   SolverConfig objects;
  *
  * - the variable "SolverNames", of type string and indexed over the
  *   dimension "n_SolverConfig"; the i-th entry of the variable is assumed
  *   to contain the classname of a :Solver object to be attached to the
  *   Block (this must be exact, i.e., exactly as returned by the protected
  *   virtual method Solver::classname(), since it is used in the factory when
  *   creating the object;
  *
  * - with n being the size of n_SolverConfig, n groups, with name
  *   "SolverConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a ComputeConfig object for the i-th :Solver;
  *
  * - the dimension "n_BlockSolverConfig" containing containing the number
  *   of BlockSolverConfig descriptions for the sub-Block of the current
  *   Block;
  *
  * - with m being the size of n_BlockSolverConfig, m groups, with name
  *   "BlockSolverConfig_<i>}" for all i = 0, \ldots, m - 1, containing each
  *   the description of a BlockSolverConfig for one of the sub-Block of the
  *   current Block.
  *
  * Of all these, only the "n_SolverConfig" and "n_BlockSolverConfig"
  * dimensions and the "SolverNames" variable are mandatory; the individual
  * groups may not exist if the corresponding dimension is 0. Note that that
  * the matching between the sub-BlockSolverConfig and the sub-Block is
  * positional: the BlockSolverConfig found in the group
  * "BlockSolverConfig_<i>" is that for the i-th sub-Block. Note that the
  * vector of sub-BlockSolverConfig is allowed to be of different size than
  * the number of sub-Block; if it is larger any extra BlockSolverConfig is
  * simply ignored, if it shorted then all missing sub-BlockConfig are
  * treated as nullptr (default configuration). */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor

 virtual ~BlockSolverConfig()
 {
  for( auto sBSC : v_BlockSolverConfigs )
   delete sBSC;

  for( auto sSC : v_SolverConfigs )
   delete sSC;
  }

/*------------------------------- CLONE -----------------------------------*/

 virtual BlockSolverConfig * clone( void ) const override
 {
  return( new BlockSolverConfig( *this ) );
  }

/*--------------------------------------------------------------------------*/
 /// "extends" Configuration::serialize( netCDF::NcFile , type ) to eProbFile
 /** Since a BlockSolverConfig knows it is a BlockSolverConfig, it "knows its
  * place" in an eProbFile netCDF SMS++ file. */

 virtual void serialize( netCDF::NcFile & f , const int type )
  const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a BlockSolverConfig. See
  * BlockSolverConfig::deserialize( netCDF::NcGroup ) for details of the
  * format of the created netCDF group. */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/*--------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 bool f_diff;  ///< tells is the configuration is a "differential" one

 /// the names of all Solver of the father Block
 std::vector<std::string> v_SolverNames;

 /// (pointer to) the ComputeConfig of all Solver of the father Block
 std::vector<ComputeConfig *> v_SolverConfigs;

 /// the vector of (pointer to) the sub-SolverConfig for each sub-Block
 std::vector<BlockSolverConfig *> v_BlockSolverConfigs;

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BlockSolverConfig
 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this BlockSolverConfig out of an istream
 /** Load this BlockSolverConfig out of an istream, with the format:
  *
  * a bool
  *
  * number k of the names of Solver for this Block
  *
  * for i = 1 ... k
  *  - a string containing the class type of a Solver object,
  *    '*' means none (nullptr)
  *
  * number k of the ComputeConfig for the Solver for this Block
  *
  * for i = 1 ... k
  *  - a string containing the class type of a ComputeConfig object,
  *    '*' means none (nullptr)
  *  - if the above is not '*', the description of the :ComputeConfig object
  *
  * number k of the BlockSolverConfig for the sub-Block of this Block
  *
  * for i = 1 ... k
  *  - a string containing the class type of a BlockSolverConfig object,
  *    '*' means none (nullptr)
  *  - if the above is not '*', the description of the :BlockSolverConfig
  *    object
  */

 virtual void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockSolverConfig ) )

/** @}  end( group( Block_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------ TYPEDEFS ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_TYPEDEFS Type definitions in Block.h
 *  @{ */

/** @}  end( group( Block_TYPEDEFS ) ) */
/*--------------------------------------------------------------------------*/
/*---------------------- INLINE METHODS IMPLEMENTATION ---------------------*/
/*--------------------------------------------------------------------------*/

template< class Const >
void Block::add_dynamic_constraints( std::list< Const > &list ,
                                     std::list< Const > &newlist ,
				     c_ModParam issueMod )
{
 // ensure Const is a derivate of Constraint
 static_assert( std::is_base_of< Constraint , Const >::value ,
              "add_dynamic_constraints: newc must inherit from Constraint" );

 if( newlist.empty() )  // actually no Constraint to add
  return;               // cowardly (and silently) return

 if( issue_mod( issueMod ) ) {
  // initialize the vector of pointer to added Constraint
  auto names = std::vector<Const *>( newlist.size() );
  auto it = names.begin();
  for( auto & el : newlist ) {  // all the new Constraint
   el.set_Block( this );        // now belong to this Block
   *(it++) = &el;               // keep their names
   }

  // now issue the BlockModAdd
  add_Modification( std::make_shared<BlockModAdd<Const>>( list ,
							  std::move( names ) ,
					 Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  for( auto & el : newlist )    // all the new Constraint
   el.set_Block( this );        // now belong to this Block

 list.splice( list.end() , newlist );  // add them at the end

 }  // end( Block::add_dynamic_constraints( Const ) )

/*--------------------------------------------------------------------------*/

template<class Var>
void Block::add_dynamic_variables( std::list< Var > &list ,
				   std::list< Var > &newlist ,
				   c_ModParam issueMod )
{
 // ensure Var is a derivate of Variables
 static_assert( std::is_base_of< Variable , Var >::value ,
                       "add_dynamic_variables: must inherit from Variable" );

 if( newlist.empty() )  // actually no Variables to add
  return;               // cowardly (and silently) return

 if( issue_mod( issueMod ) ) {
  // initialize the vector of pointer to added Constraint
  auto names = std::vector<Var *>( newlist.size() );
  auto it = names.begin();
  for( auto & el : newlist ) {  // all the new Constraint
   el.set_Block( this );        // now belong to this Block
   *(it++) = &el;               // keep their names
   }

  // now issue the BlockModAdd
  add_Modification( std::make_shared< BlockModAdd< Var > >( list ,
							std::move( names ) ,
				       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  for( auto & el : newlist )    // all the new Variable
   el.set_Block( this );        // now belong to this Block

 list.splice( list.end() , newlist );  // add them at the end

 }  // end( Block::add_dynamic_variables( Var ) )

/*--------------------------------------------------------------------------*/

template< class Const >
void Block::remove_dynamic_constraints( std::list< Const > &list ,
                  std::vector< typename std::list< Const >::iterator > &rmvd ,
		  c_ModParam issueMod )
{
 // ensure Const is a derivate of Constraint
 static_assert( std::is_base_of< Constraint , Const >::value ,
                "remove_dynamic_constraints: must inherit from Constraint" );

 if( rmvd.empty() )  // actually no Constraints to remove
  return;            // cowardly (and silently) return

 // TODO What if the iterator is not valid?
 for( const auto & const_it : rmvd ) {
  remove_constraint_from_variables( &(*const_it) );
  const_it->clear();
  }

 if( issue_mod( issueMod ) ) {
  std::list< Const > removed;

  // remove all the Constraint (whose iterators are found) in rmvd and add
  // them to the removed list; note that by using splice() the address of
  // the actual Constraints objects is not changed
  for( auto el : rmvd )
   removed.splice( removed.end() , list , el );

  // now issue the BlockModRmv
  add_Modification( std::make_shared< BlockModRmv< Const > >( list ,
						      std::move( removed ) ,
				       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  for( auto & el : rmvd )
   list.erase( el );

 }  // end( Block::remove_dynamic_constraints( Const ) )

/*--------------------------------------------------------------------------*/

template< class Const >
void Block::remove_dynamic_constraint( std::list< Const > &list ,
                                  typename std::list< Const >::iterator rmvd ,
	                          c_ModParam issueMod )
{
 // ensure Const is a derivate of Constraint
 static_assert( std::is_base_of< Constraint , Const >::value ,
                "remove_dynamic_constraint: must inherit from Constraint" );

 remove_constraint_from_variables( &(*rmvd) );
 rmvd->clear();

 if( issue_mod( issueMod ) ) {
  std::list< Const > removed;

  // remove the Constraint pointed by rmvd and add it the removed list; note
  // that by using splice() the address of the actual Constraint object is
  // not changed
  removed.splice( removed.end() , list , rmvd );

  // now issue the BlockModRmv
  add_Modification( std::make_shared< BlockModRmv< Const > >( list ,
						      std::move( removed ) ,
				       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  list.erase( rmvd );

 }  // end( Block::remove_dynamic_constraint( Const ) )

/*--------------------------------------------------------------------------*/

template< class Var >
void Block::remove_dynamic_variables( std::list< Var > &list ,
                    std::vector< typename std::list< Var >::iterator > &rmvd ,
		    c_ModParam issueMod , c_ModParam issueindMod )
{
 // ensure Var is a derivate of Variable
 static_assert( std::is_base_of< Variable , Var >::value ,
                    "remove_dynamic_variables: must inherit from Variable" );

 if( rmvd.empty() )  // actually no Variables to remove
  return;            // cowardly (and silently) return

 for( const auto & var_it : rmvd )
  remove_variable_from_stuff( &( *var_it ) , issueindMod );

 if( issue_mod( issueMod ) ) {
  std::list< Var > removed;

  // remove all the Variable (whose iterators are found) in rmvd and add them
  // to the removed list; note that by using splice() the address of the
  // actual Variable objects is not changed
  for( auto el : rmvd )
   removed.splice( removed.end() , list , el );

  // now issue the BlockModRmv
  add_Modification( std::make_shared< BlockModRmv< Var > >( list ,
						      std::move( removed ) ,
				       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  for( auto & el : rmvd )  // just remove them
   list.erase( el );

 }  // end( Block::remove_dynamic_variables( Var ) )

/*--------------------------------------------------------------------------*/

template< class Var >
void Block::remove_dynamic_variable( std::list< Var > &list ,
				    typename std::list< Var >::iterator rmvd ,
				    c_ModParam issueMod ,
				    c_ModParam issueindMod )
{
 // ensure Var is a derivate of Variable
 static_assert( std::is_base_of< Variable , Var >::value ,
                "remove_dynamic_variable: must inherit from Variable" );

 remove_variable_from_stuff( &(*rmvd) , issueindMod );

 if( issue_mod( issueMod ) ) {
  std::list< Var > removed;

  // remove the Variable pointed by rmvd and add it the removed list; note
  // that by using splice() the address of the actual Variable object is
  // not changed
  removed.splice( removed.end() , list , rmvd );

  // now issue the BlockModRmv
  add_Modification( std::make_shared< BlockModRmv< Var > >( list ,
						      std::move( removed ) ,
				       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else
  list.erase( rmvd );  // just remove it

 }  // end( Block::remove_dynamic_variable( Var ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Block.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Block.h -----------------------------*/
/*--------------------------------------------------------------------------*/

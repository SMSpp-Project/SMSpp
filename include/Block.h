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
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Block
 #define __Block      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"
#include "Configuration.h"
#include "Observer.h"
#include "Solver.h"

#include <boost/bimap.hpp>
#include <netcdf>

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

/*--------------------------------------------------------------------------*/
/*------------------------- Block-RELATED TYPES ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_TYPES Block-related types
 *  @{ */

typedef Block * p_Block;
///< a pointer to Block

typedef std::vector< p_Block > Vec_Block;
///< a vector of pointers to Block

typedef const std::vector< p_Block > c_Vec_Block;
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

 /// an "infinite Range", i.e., [ 0 , INF ), i.e., "everything"
 static constexpr auto INFRange = Range( 0 , Inf< Index >() );

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
  * that take a Block * first, two ModParam (for "physical" and "abstract"
  * Modification, respectively) at the end, and in the middle as many
  * parameters as they want. */

 template< typename ... Args >
 using FunctionType =
  std::function< void( Block * , Args ... , ModParam , ModParam ) >;

 /// typedef for class member functions to be added to the methods factory
 /** The class member functions (whose adapters are to be) added to the
  * methods factory should take two ModParam (for "physical" and "abstract"
  * Modification, respectively) at the end, before them as many parameters as
  * they want, and be functions of some \p dBlock derived from Block; it is
  * clearly "the same parameter type list" as FunctionType< Args > (with the
  * same Args) for a function of the given \p dBlock. */

 template< class dBlock , typename ... Args >
 using MemberFunctionType =
  void ( dBlock::* )( Args ... , ModParam , ModParam );

 /// helper type for template shenanigans for methods factory
 template< typename ... >
 struct arg_packer_helper {};

 /// template shenanigans for methods factory: type for packing variadic lists
 template< typename ... Args >
 struct arg_packer { using args = arg_packer_helper< Args ... >; };

 /// type for ( void , range ) functions
 using MS_rngd = arg_packer< Range >;

 /// type for ( double , range ) functions
 using MS_dbl_rngd = arg_packer< MF_dbl_it , Range >;

 /// type for ( int , range ) functions
 using MS_int_rngd = arg_packer< MF_int_it , Range >;

 /// type for ( void , subset ) functions
 using MS_sbst = arg_packer< Subset && , bool >;

 /// type for ( double , subset ) functions
 using MS_dbl_sbst = arg_packer< MF_dbl_it , Subset && , bool >;

 /// type for ( int , subset ) functions
 using MS_int_sbst = arg_packer< MF_int_it , Subset && , bool >;

/** @} ---------------------------------------------------------------------*/

 /// ConstraintID identifies a Constraint within a Block
 /** A single Constraint of a Block can be identified by three pieces of
  * information: whether it is a static or a dynamic Constraint, the index of
  * the group to which is belongs, and the index of the Constraint in its
  * group. The first information can be encoded in the second one if we
  * redefine the index of the i-th dynamic group to be
  *
  *   i + number_static_constraint_groups.
  *
  * Let the pair ( i, j ) be a ConstraintID. If
  *
  *   i < number_static_constraint_groups,
  *
  * then it refers to a static Constraint, located at position j of the i-th
  * static group. If
  *
  *   i >= number_static_constraint_groups,
  *
  * then it refers to a dynamic Constraint, located at position j of the
  * dynamic group whose index is
  *
  *   i - number_static_constraint_groups.
  *
  * We now explain how the index of a Constraint within a group can be
  * determined, as it may not be obvious for multidimensional groups. It is
  * important to notice that all indices mentioned here belong to zero-based
  * numbered sequences, i.e., sequences whose first element is 0.
  *
  * A static group of Constraint can be one of three types:
  *
  * 1. It is a single Constraint;
  *
  * 2. It is a vector of Constraint;
  *
  * 3. It is a multidimensional array of Constraint.
  *
  * In the first case, in which the group is a single Constraint, the index of
  * the Constraint is 0. In the second case, in which the group is a vector of
  * Constraint, the index of the Constraint is simply its position in that
  * vector. In the last case, in which the group is a multidimensional array
  * of Constraint, the index of the Constraint is its position in the
  * vectorized multidimensional array in row-major layout. For instance, if
  * the multidimensional array has two dimensions with sizes m and n,
  * respectively, then the Constraint at position ( p , q ) would have an
  * index equal to "n * p + q" (recall the indices start from 0). In general,
  * for a multidimensional array with k dimensions with sizes ( n_0 , ... ,
  * n_{k-1} ), the Constraint at position ( i_0 , ... , i_{k-1} ) would have
  * an index equal to
  * \f[
  *   \sum_{r = 0}^{k-1} ( \prod_{s = r + 1}^{k-1} n_s ) i_r
  * \f]
  *
  * A dynamic group of Constraint can be one of three types:
  *
  * 1. It is list of Constraint;
  *
  * 2. It is a vector of lists of Constraint;
  *
  * 3. It is a multidimensional array of lists of Constraint.
  *
  * In the first case, in which the group is a list of Constraint, the index
  * of the Constraint is its position in that list. In the second case, in
  * which the group is a vector of lists of Constraint, for a Constraint at
  * position j of the k-th list of the vector, its index is given by
  * \f[
  *    j + \sum_{t = 0}^{k-1} s_t
  * \f]
  * where s_t is the number of Constraint in the t-th list of the vector. The
  * last case is analogous. */

 using ConstraintID = std::pair< Index , Index >;

/*--------------------------------------------------------------------------*/
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

 Block( Block * father = nullptr ) : Observer() , f_owner( nullptr ) ,
  f_at( false ) , f_BlockConfig( nullptr ) , f_channel( 0 ) ,
  f_Block( father ) , f_Objective( nullptr ) {}

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
  *       Block * myBlock = Block::new_Block( some_class );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed).
  *
  * Note that the :Block returned by this method is "empty": it contains no
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
  *   to exactly *one* .cpp file, typically that :Block .cpp file. If the name
  *   of the class contains any parentheses, then one must enclose the name of
  *   the class in parentheses and instead add the line
  *
  *       SMSpp_insert_in_factory_cpp_1( ( name_of_the_class ) );
  *
  * Any whitespaces that the given \p classname may contain is ignored. So,
  * for example, to create an instance of the class MyBlock< int > one could
  * pass "MyBlock< int >" or "MyBlock< int >" (even " M y B l o c k < int > "
  * would work).
  *
  * @param classname The name of the :Block class that must be constructed.
  *
  * @param father A pointer to the father of the Block that will be
  *        constructed. */

 static Block * new_Block( const std::string & classname ,
                           Block * father = nullptr ) {
  const std::string classname_( SMSpp_classname_normalise(
						std::string( classname ) ) );
  const auto it = Block::f_factory().find( classname_ );
  if( it == Block::f_factory().end() )
   throw( std::invalid_argument( classname +
				 " not present in Block factory" ) );
  return( ( it->second )( father ) );
  }

/*--------------------------------------------------------------------------*/
 /// set the executable-wide prefix for all Block filenames
 /** Loading a Block from file (either text or netCDF) is likely to be one of
  * the most common way to create one, and the static method
  * deserialize( std::string [ , Block * ] ) is provided for this purpose.
  * That method is in turn used for "file redirection"; a Block file (either
  * text or netCDF) can contain one (or many) filenames in which a part of
  * the description of that Block (typically a sub-Block) can be found, see
  * new_Block( netCDF::NcGroup [ , Block * ] ) and
  * deserialize( std::istream [ , Block * ] ). It could arguably be
  * convenient to be able to specify filenames relative to some given prefix,
  * so as to be able to freely move the description of a Block (which can be
  * a rather large object, and therefore require many files) across the
  * filesystem. Block provides a *static* member for this purpose, that can
  * be set with this (static) method. Note that
  *
  *     BEING THE MEMBER STATIC, THE PREFIX IS APPLIED TO ALL LOADING 
  *     OPERATIONS OF ANY Block IN THE EXECUTABLE
  *
  * Use of this feature therefore requires care. */

 static void set_filename_prefix( std::string && prefix ) {
  f_prefix = prefix;
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of a file
 /** Top-level de-serialization method: takes the \p filename of a file
  * (possibly also encoding a position into it), and possibly a \p father,
  * and returns the complete :Block object whose description is the one
  * found (at the specified position) in the file.
  *
  * The method supports two different kind of files:
  *
  * - text files,
  *
  * - SMS++ netCDF files.
  *
  * It distinguishes between the two by the suffix. In particular, the format
  * of \p filename can be:
  *
  * - If \p filename terminates by ".txt" (case sensitive), the :Block is
  *   loaded out of one (or more) text file(s) via a call to
  *   load( std::string & ). Since load() supports the notion of different
  *   file formats via its \p frmt parameter, this method also supports
  *   "filename mangling" to specify that parameter as follows:
  *
  *     * if \p filename ends with "[X].txt", 'X' any single character, then
  *       only the part of \p filename preceding the '[' is passed as the
  *       filename argument of load(), while 'X' is passed as the \p frmt
  *       argument (note that this excises ".txt" as well);
  *
  *     * otherwise, the whole of \p filename is passed as the filename
  *       argument of load() and the \p frmt argument is left at its default
  *
  *   Note that load( std::string & ) can in principle be used to support
  *   multi-file formats where \p filename (with ot without mangling) is
  *   used as a prefix of multiple files with different suffixes. Also,
  *   note that load() uses the global prefix set by set_filename_prefix(),
  *   if any.
  *
  * - Otherwise a netCDF::NcFile is opened and deserialize( netCDF::NcFile )
  *   is called. Since SMS++ netCDF::NcFile support the notion of having
  *   multiple Block inside, \p filename can be used to encode the position
  *   (Block) in the file:
  *
  *     * if \p filename ends with ']', then is supposed to have the
  *       form "real filename[idx]": the "[idx] part is excised and used to
  *       compute the int parameter of deserialize() (the position), with the
  *       remaining part being used for the string parameter (the filename);
  *
  *     * otherwise, the whole string is used as the string parameter (the
  *       filename).
  *
  *   Again, if a filename prefix has been defined (for all Block) by means
  *   of set_filename_prefix(), then \p filename has to be intended as
  *   relative to that prefix (in the sense that the prefix is prefix to
  *   \p filename).
  *
  * If anything goes wrong with the entire operation, nullptr is returned.
  *
  * Note that the method is static, hence it is to be called as
  *
  *      auto myBlock = Block::deserialize( some_file [ , father ] );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed, in which case there
  * will be no \p father). */

 static Block * deserialize( const std::string & filename ,
			     Block * father = nullptr );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of an open netCDF SMS++ file at given position
 /** Second-level de-serialization method: takes an open netCDF file, the
  * index \pos of a Block into the file, and possibly a \p father, and 
  * returns the corresponding complete :Block object with the prescribed
  * father (if any).
  *
  * There are three types of SMS++ netCDF files, corresponding to three values
  * of the enum smspp_netCDF_file_type; see SMSTypedefs.h for details, the
  * two ones relevant here being
  *
  * - eProbFile: the file (which is also a group) has any number of child
  *   groups with names "Prob_0", "Prob_1", ... In turn, each child group
  *   has exactly three child groups with names "Block", "BlockConfig" and
  *   "BlockSolver", respectively.
  *
  * - eBlockFile: the file (which is also a group) has any number of child
  *   groups with names "Block_0", "Block_1", ...
  *
  * The :Block extracted from the file is specified by the parameter idx: for
  * an eProbFile it is extracted out of the netCDF::NcGroup "Block" inside
  * the netCDF::NcGroup "Prob_< idx >", while for an eBlockFile it is extracted
  * out of the netCDF::NcGroup "Block_< idx >".
  *
  * Once the appropriate group is selected, the :Block is loaded from it with
  * a call to new_Block( netCDF::NcGroup & ); see the corresponding comments
  * for the format options. Anything going wrong with the entire operation
  * (the file is not there, the "SMS++_file_type" attribute is not there,
  * there is no required "Prob_< idx >" or "Block_< idx >" child group, there is
  * any fatal error during the process, ...) results in nullptr being
  * returned.
  *
  * Note that the method is static, hence it is to be called as
  *
  *       Block * myBlock = Block::deserialize( netCDFfile );
  *
  * i.e., without any reference to any specific Block (and, therefore, it can
  * be used to construct the very first Block if needed). */

 static Block * deserialize( const netCDF::NcFile & f ,
			     unsigned int idx = 0 ,
			     Block * father = nullptr );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of netCDF::NcGroup, returns it
 /** Third-level de-serialization method: takes a netCDF::NcGroup supposedly
  * containing  (all the information describing) a :Block (either "directly"
  * or "indirectly"), and possibly the \p father, and returns a pointer to a
  * newly minted :Block object corresponding to what is found in the file
  * and having the prescribed father (if any).
  *
  * The method works with two different kinds of netCDF::NcGroup:
  *
  * - A "direct" group that contains at least the string attribute "type";
  *   this is used it in the factory to construct an "empty" :Block of that
  *   type (and with the given \p father, if any) [see
  *   new_Block( std::string & [ , Block * ] )], and then the method
  *   deserialize( netCDF::NcGroup ) of the newly minted :Block is invoked
  *   (with argument \p group) to finish the work.
  *
  * - An "indirect" group that just need to contain the single string
  *   attribute "filename"; in this case, the attribute is used as argument
  *   for a call to deserialize( const std::string & [ , int , Block * ] )
  *   that will extract the :Block by the corresponding file (be it a text or
  *   netCDF one), with the given \p father. Note that for netCDF files the
  *   filename string can also be used to encode the position in the file,
  *   see the comments in the method for details.
  *
  * In case \p group contains both "type" and "filename", the first takes the
  * precedence (direct groups have precedence over indirect ones).
  *
  * Note that this method is static (see the previous versions for comments
  * about it) and returns a pointer to Block, hence it has to have a
  * different name from deserialize( netCDF::NcGroup ) (since the signature
  * is the same but for the return type).
  *
  * If anything goes wrong with the process, nullptr is returned. */

 static Block * new_Block( const netCDF::NcGroup & group ,
                           Block * father = nullptr );

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
  * (unless the :Block does this by its own volition), the BlockConfig is not
  * set, and there are no Solver attached, unless there were before. The
  * eProbFile SMS++ netCDF file type is precisely provided for allowing to
  * save *all* the information required to solve a Block (the Block itself,
  * its BlockConfig and all its Solver information, see RBlockConfig.h and
  * BlockSolverConfig.h), but de-serializing the Configuration and applying
  * them is still the user's responsibility.
  *
  * The method of the base class actually ignores the "type" attribute. This
  * is done because "type" is thought to be used as input to the factory to
  * create the object (as in new_Block( netCDF::NcGroup )); once the :Block
  * has been constructed, which has necessarily already happened when this
  * method is called, the value is irrelevant. The only thing that could be
  * done with the value inside this method would be to check if "type" agrees
  * with classname() and throw exception otherwise. However, derived classes
  * are free to do that if they so choose. Indeed, note that a potential
  * issue would seem to be the case where Block2 could derive from Block1;
  * then, Block2::deserialize() would typically call Block1::deserialize()
  * (or a "guts_of_" version of it, see below) to do the part pertaining to
  * Block1. Block1::deserialize() can actually check if "type" agrees with
  * classname(), since classname() is virtual: even called inside
  * Block1::deserialize() it would return Block2::classname(), and therefore
  * the check would pass.
  *
  * So, what the method currently does is just to handle the optional "name"
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

 virtual void deserialize( const netCDF::NcGroup & group )
 {
  netCDF::NcGroupAtt gname = group.getAtt( "name" );
  if( gname.isNull() )
   f_name.clear();
  else
   gname.getValues( f_name );

  // issue a NBModification, the "nuclear option"
  if( anyone_there() )
   add_Modification( std::make_shared< NBModification >( this ) );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Block out of std::istream, returns it
 /** Convenience static method that, given the open std::istream \p input,
  * and possibly the \p father, creates a :Block reading all its data from
  * \p input, and having the prescribed father (if any).
  *
  * The  format of the istream (from the point the pointer is onwards) is
  * assumed to be:
  *
  * - Either the character '*' is the first one that is found after any
  *   whitespace and comment, after which two cases arise:
  *
  *   = The characters immediately following '*' form a nonempty string,
  *     which means that '*' is not immediately followed by a whitespace
  *     (note that comments are *not* skipped here): then, the string is
  *     used as the filename of deserialize( std::string & ), which opens
  *     it and reads the :Block from there without advancing the pointer
  *     in \p input (save for discarding '*' and the string). Check the
  *     comments of deserialize( std::string & ) for the details of the
  *     possible formats of the string.
  *
  *   = The characters immediately following '*' form an empty string (which
  *     means that '*' is immediately followed by whitespaces or comments):
  *     then, nullptr is returned;
  *
  * - Or the first character that is found after any whitespace and comment
  *   is not '*', in which case the method expects:
  *
  *   = first of all a nonempty string that specifies the classname of the
  *     :Block, as required by the Block factory, which is immediately
  *     used to construct the :Block object;
  *
  *   = optionally, if the first subsequent character (after whitespaces and
  *     comments) is '[', then it is taken to be the beginning of a string
  *     of the form "[X]", with 'X' any character: the string is read and
  *     the 'X' is used as the \p frmt parameter in the subsequent call to
  *     load(), of either of the two types, as detailed below.
  *
  *   = possibly after the previous string, it is checked again if the
  *     first character (after whitespaces and comments) is '*':
  *
  *     * if so, then the subsequent string is extracted from the istream
  *       and it is used as the filename argument in a call to
  *       load( std::string & ) to the newly constructed :Block (with the
  *       previously extracted \p frmt parameter, if any, or the default
  *       one otherwise);
  *
  *     * otherwise, load( istream ) is called in the newly constructed
  *       :Block passing the same \p input istream (with the previously
  *       extracted \p frmt parameter, if any, or the default one
  *       otherwise);
  *
  *   Finally, the thusly load()-ed newly constructed :Block is returned.
  *
  *   Note that in the first case (filename preceded by '*') only the
  *   filename is extracted from \p input, while in the second case the
  *   whole description of the :Block is. Furthermore, by calling
  *   load( std::string & ) the first case directly supports the case where
  *   :Block input format is a multi-file one, see the comment to that
  *   version of load() for details. */

 static Block * deserialize( std::istream & input ,
			     Block * father = nullptr );

/*--------------------------------------------------------------------------*/
 /// destructor of Block: it is virtual
 /** Destructor of Block: it invokes set_BlockConfig() to clean up the
  * configuration of the Block. It also cleans up any currently open
  * GroupModification. */

 virtual ~Block() {
  set_BlockConfig();
  for( auto &el : v_GroupMod )
   delete el.second;
  }

/** @} ---------------------------------------------------------------------*/
/*----------------- Methods for acquiring/releasing the Block --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for acquiring/releasing the Block
 *
 * One of the main reasons for the existence of SMS++ is to support
 * asynchronous operations on different parts of an optimization problem
 * (different Block and sub-Block of a Block), and in general approaches
 * capable of exploiting multiple available processors. The fundamental
 * design principles of SMS++ basically mandate a shared memory (although a
 * distributed framework could be constructed on top of SMS++); thus, access
 * control and synchronization primitives need be provided by Block.
 *
 * This access control is basically structured as the classical "Big Fat
 * Lock": before being modified, a Block need be "locked". Once the lock is
 * acquired by any entity, only that entity is entitled to modify it. Until
 * the entity releases the lock, no other entity can take it. This means that
 * not only no other entity is allowed to modify the Block, but that even
 * reading the Block data is unadvisable, in that it can be modified at any
 * time by the other entity, which easily leads to data inconsistencies.
 *
 * Note that sub-Block are "a part of the Block"; this means that locking a
 * Block implies locking all its sub-Block, recursively. In turn, this implies
 * that locking a Block is not a "cheap" operation. Because of this, the
 * locking and unlocking operations are *not* automatically performed by any
 * method changing anything in the Block, but must be explicitly called before
 * invoking any such method. The rationale for this choice is:
 *
 * - it dramatically simplifies the implementation of :Block, especially in
 *   the case where a method of a derived class must call that of the base
 *   class;
 *
 * - it allows to bunch a set of Block-changing operations under the same
 *   lock/unlock stretch to improve performances;
 *
 * - there can be cases when one can be 100% sure that no other thread/entity
 *   can possibly operate on the Block, either because the overall application
 *   is rigidly single-threaded, or because the Block can only be accessed
 *   via a rigidly controlled access point (for instance, the Block has
 *   just been created out of a factory and there currently is only one
 *   pointer to it that no other thread can possibly be sharing because it
 *   is, say, a local variable in a function).
 *
 * Of course
 *
 *     EACH TIME THE Block IS LOCKED IT HAS TO BE UNLOCKED, WHICH MEANS
 *     THAT ALL LOCK/UNLOCK STRETCHES THAT MAY THROW EXCEPTION SHOULD
 *     BE ENCLOSED INSIDE A try - catch BLOCK OR ANALOGOUS CONSTRUCT
 *
 * While all this is mostly useful for asynchronous approaches, in general
 * even in a synchronous setting it may not be trivial to ensure that
 * different parts of a complex solution process "agree" on what kind of
 * changes may be allowed to a given Block at any point in time. Therefore,
 * the lock/unlock mechanism of Block is thought to be used even in a
 * single-threaded environment to ensure that different parts of a solution
 * process do not overstep the boundaries of their allowed changes. To handle
 * these aspects, the lock/unlock mechanism of Block has the following
 * specific features:
 *
 * - It explicitly indicates the "owner" of the Block (under the form of a
 *   void *). This allows the owner to cheaply check if the Block is already
 *   locked by itself and avoid to re-lock it. This is particularly useful
 *   in that is allows "transfer of ownership" between different entities:
 *   the entity owning a Block can provide another entity "access right" to,
 *   say, a sub-Block of the Block, knowing that this may produce localized
 *   changes that it is able to manage.
 *
 * - It explicitly stores the thread::id of the owner. This is necessary in
 *   that attempts to locking an already locked block need necessarily be
 *   handled differently according to the fact that the new would-be owner
 *   runs in a different thread than the current owner or the same one.
 *   Indeed, if it runs in a different thread it can just "go to sleep on
 *   the mutex" (that the Block provides) and be automatically woken up when
 *   the current owner relinquishes the lock; in other words, the lock
 *   operation always eventually succeeds. However, if it runs in the same
 *   thread this cannot happen, which means that the lock operation may
 *   have to explicitly fail.
 *
 * The latter part, however, means that the
 *
 *     THE RESULT OF lock() AND unlock() OPERATIONS MAY DEPEND ON WHICH
 *     THREAD THEY ARE CALLED BY.
 *
 * See the comment to the individual methods for details.
 *
 * Since a BFL may impose high synchronization costs, a "relaxed" version of
 * the concept is provided by means of read_[only_]lock() and
 * read_[only_]unlock(). Unlike the "normal" lock, which is read-write, any
 * number of read-only locks can be concurrently acquired on the same Block;
 * however, each of the entities is only permitted to read the Block, but not
 * to modify it.
 *
 * Read-only locks are different from read-write-lock in that there is no
 * concept of a single owner. Clearly, a read-lock is incompatible with a
 * read-write-lock, and vice-versa. Any number of read-lock can belong to
 * the same thread, but any read-write-lock from the same thread will
 * immediately fail, as will any read-lock from the same thread as the
 * owner of a read-write-lock. In all other cases the locks will wait on
 * the mutex.
 *
 * Note that read-locks are somewhat simpler than read-write locks to manage
 * because they are "counted": a new read-lock on an already read-locked
 * Block just increments the counter, which means that the simple rule
 * "one read-unlock for each read-lock" holds (unlike read-write-locks
 * where a single unlock undoes any number of locks). However, note that
 *
 *      READ-ONLY LOCKS ARE COUNTED *PER THREAD*,  WHICH IMPLIES THAT EACH
 *      THREAD THAT MAKES A READ-ONLY LOCK MUST EVENTUALLY MAKE A
 *      READ-ONLY-UNLOCK
 *
 * Conversely, read-write-unlocks are thread-independent (but there still
 * are requirements about doing read-write-locks and read-write-unlocks
 * for the same owner from different threads, see the comments to the
 * individual methods for details).
 * @{ */

 /// returns true if the Block is already owned by owner
 /** The method returns true if the Block is already owned by owner. Note
  * that this method just compares the internal field with the argument,
  * and it is therefore
  *
  *     DANGEROUS TO BE USED IN A CONCURRENT CONTEXT IF owner RUNS IN A
  *     DIFFERENT THREAD THAN THE CURRENT OWNER
  *
  * This is because, obviously, the internal field can change at any point
  * in time immediately after the operation and before its result can be
  * used. However, the method
  *
  *     CAN TYPICALLY BE USED TO ESTABLISH THAT THE ENTITY MAKING THE CALL
  *     IS THE CURRENT OWNER
  *
  * This is so if the entity making the call is single-threaded, or if the
  * call is made by the "main thread" where the other threads do not mess up
  * with ownership of the Block. Indeed, the main reason for the existence of
  * this method is to cheaply solve the issue poised by calls to
  * lock( owner ) where owner is already the current owner. The issue is that
  * such a call is supposed to immediately return true without doing nothing;
  * however, in so doing it breaks the concept that each lock() should be
  * paired with an unlock(), because the first unlock() by the owner undoes
  * any number of previous consecutive lock() by the same entity. Therefore,
  * if there is any doubt that the entity could already own the Block, the
  * call scheme should be
  *
  *     bool owned = block->is_owned_by( me );
  *     if( ( ! owned ) && ( ! block->lock( me ) ) )
  *      < something happens, typically a disaster >
  *
  *     < block is mine, do whatever I want with it >
  *
  *     if( ! owned )
  *      block->unlock( me );
  *
  * This works if "me" is sure that no other thread can "share its identity"
  * and be concurrently trying to lock/unlock the Block.
  *
  * A similar use of is_owned_by() is advised even when read_lock()-ing a
  * Block, i.e.,
  *
  *     bool owned = block->is_owned_by( me );
  *     if( ( ! owned ) && ( ! block->read_lock() ) )
  *      < something happens, typically a disaster >
  *
  *     < block is mine, surely at least to read >
  *
  *     if( ! owned )
  *      block->read_unlock();
  *
  * The rationale for this is the possibility for a Solver to (be forced to)
  * "acquire someone else's identity" [cf. Solver::set_id()]. This is tied to
  * the fact that the entity (say, a Solver) that has locked a Block may rely
  * on [sub-]Solver to operate on (typically, sub-Block of) it. If the entity
  * read_lock()s the Block, then the [sub-]Solver can freely read_lock() it
  * again with no issues. However, if the entity has write lock()d the Block,
  * the [sub-]Solver cannot read_lock() it. It is therefore always better to
  * check if a Block need not be lock()d, be it a read_lock() or a write
  * lock(), because it is already owned.
  *
  * Of course, this puts the onus on the entity that originally write
  * lock()d the Block not to release the lock while any of the involved
  * [sub-]Solver need to keep it. */

 bool is_owned_by( const void * owner ) { return( f_owner == owner ); }

/*--------------------------------------------------------------------------*/
 /// tries to lock the Block, return true on success
 /** Attempts to acquire the lock on the Block for the entity "owner".
  *
  * The "owner" parameter is a void * and can in principle be anything;
  * the only real restriction is that
  *
  *     OWNER MUST NOT BE nullptr, NOR A POINTER TO SOME INTERNAL FIELD OF
  *     THE Block (BECAUSE TWO SUCH POINTERS ARE USED FOR INTERNAL PURPOSES,
  *     SEE ReadOnlyLock() and v_ownersLock())
  *
  * In general one would want that "no other entity else should use the same
  * value". Typically, the entity will be an object (say, a Solver), and
  * "owner" will be the "this" of the object, so that no-one else has the
  * same identity unless the object purposely "lends its identity". Note that
  * there is no way for an external entity to know the owner's identity
  * before the call and therefore "impersonate" it.
  *
  * If "owner" is already the owner, the operation surely succeeds at almost
  * zero cost. This in particular means that
  *
  *     TWO LOCK OF ONE Block BY THE SAME OWNER EFFECTIVELY ARE ONE SINGLE
  *     LOCK, AND THEREFORE THEY MUST BE UNLOCKED ONLY ONCE
  *
  * It is responsibility of the caller of lock() to keep track of this, and
  * call unlock() the right number of times; see the comments to
  * is_owned_by(). Analogously, if the owner is a multi-threaded entity,
  * it must coordinate locking of Block between its threads on its own; in
  * particular, it should never happen that a thread is unlocking the Block
  * for a given owner while another thread is locking it for the same
  * owner, because in this case the lock may succeed but the Block may end
  * up being unlocked.
  *
  * It is also important to remark that
  *
  *     THE THREAD RUNNING THE FIRST SUCCESSFUL LOCK IS RECORDED AS THE
  *     OWNER'S THREAD, WHICH IMPACTS THE OUTCOME OF SUBSEQUENT lock()
  *
  * In fact, if the Block owner's thread is the same as the one calling
  * lock(), the operation immediately fails
  *
  *     UNLESS THE OWNER IS THE SAME, IN WHICH CASE THE THREAD IS IRRELEVANT
  *
  * If, instead, owner's thread is different from the one calling lock(),
  * then lock() goes to sleep until the Block is unlocked by the current
  * owner; then, one of the interested threads gets to own the lock, the
  * others being kept sleeping.
  *
  * If the Block is un-owned, the operation succeeds if also all the sub-Block
  * can be successfully locked. This means that the operation immediately
  * fails if the owner's thread of the first encountered sub-Block that is
  * already owned is the same as the thread making the call, unless the owner
  * is "owner" already. In this case, all the sub-Block that had been
  * tentatively locked during the unsuccessful attempt are immediately
  * unlocked. If, instead, the owner's thread of the first already owned
  * sub-Block is different from the thread making the call (and the owner is
  * not "owner"), then lock() goes to sleep until that sub-Block is unlocked
  * by the current owner; then a race is possibly ran between interested
  * threads to see who gets to own it first, the others being put back to
  * sleep. Note that the previously locked sub-Block are not unlocked
  * while lock() sleeps. It is therefore crucial that locking is always
  * performed in a pre-visit (the father is locked before all its sons). This
  * ensures that a locking sweep of the tree can only be stalled by another
  * thread locking of one of its sub-trees, but that the locking of the
  * sub-tree cannot be itself stalled (if not by a locking of a
  * sub-sub-tree). In other words, the "inner" locking sweep between all
  * the active ones will surely succeed without being stalled by the others,
  * which ensures that there will be no deadlock between concurrent locking
  * operations. This always work provided that
  *
  *     AN ENTITY OWNING A sub-Block NEVER TRIES TO OWN A Block THAT
  *     INCLUDES IT, AND NO ENTITY EVER TRIES TO OWN TWO DIFFERENT
  *     Block.
  *
  * Indeed, consider the case of two Block Father --> Son, where Son is
  * already owned by an entity, while a different one (running on a different
  * thread) is trying to lock Father. This lock attempt will stall waiting
  * for Son to be unlocked. However, if the owner of Son also tries to lock
  * Father, this will also stall and a deadlock will ensue. Since lock
  * operations naturally travel "downward" on a tree, the lock attempts must
  * always proceed top-down. Obviously, if an entity is trying to own two
  * different Block, say B1 and B2 in this order, any other entity trying to
  * own B2 and B1 (in this order) may deadlock. If this can happen, an 
  * additional ad-hoc synchronization layer will be needed. Yet, note that
  *
  *     NO ADDITIONAL MECHANISM IS REQUIRED IF B1 AND B2 ARE BOTH DESCENDANTS
  *     OF THE SAME BLOCK B, AND BOTH ENTITIES TRY TO OWN B DIRECTLY
  *
  * In fact, one of the two will surely succeed. Hence, it is always safe to
  * lock a subset of Block by locking their (say) nearest common ancestor, if
  * any exists.
  *
  * In most cases, there should be no reason for derived classes to mess
  * up with this mechanism. However, some :Block may have nonstandard
  * behavior, e.g. about how they store their sub-Block, and therefore for
  * maximal flexibility this method is virtual. */

 virtual bool lock( const void * owner ) {
  if( ( ! owner ) || ( owner == ReadOnlyLock() ) ||
      ( owner == v_ownersLock() ) )
   throw( std::logic_error( "invalid owner in lock()" ) );

  for( ; ; ) {  // this may have to be repeated many times
   const void * current_owner = nullptr;
   if( f_owner.compare_exchange_strong( current_owner, owner ) ) {
    // the Block was un-owned, we now try to own it
    // this may still fail because in order to own a Block all of its
    // sub-Block must also be owned: try to do that

    f_mutex.lock();  // first of all, lock the mutex: this is a bit weird
    // because the Block should be "free", but it is done
    // immediately so that other Block willing to take ownership from
    // different threads can sleep on the mutex even during the potentially
    // long time its takes to (try to) lock all sub-Block, instead of
    // actively looping on the compare_exchange_strong()

    if( lock_sub_block( owner ) ) {  // if all sub-Block can be owned
     f_owner_thread_id = std::this_thread::get_id();   // record own id
     return( true );                                   // all done
     }

    // it was not possible to lock all the sub-Block;
    f_owner = nullptr;  // release ownership of the Block
    f_mutex.unlock();   // release the mutex
    return( false );    // failed to lock the Block
    }
   else  // an owner already existed
    if( current_owner == owner )  // but it's the would-be owner
     return( true );              // nothing to do, it already owns the Block
     // note: we are assuming that the current owner is not multi-threaded,
     //       in the sense that no other thread sharing the owner identity
     //       can be unlocking the Block in this very instant
    else {                        // the current owner is different
     if( ( current_owner == ReadOnlyLock() ) ||
	 ( current_owner == v_ownersLock() ) ) {
      // and it's a read-lock rather than a read-write lock
      for( ; ; ) {  // acquire the "active lock" on v_owners
       // note that the Block may be read-locked by a single entity which
       // was precisely in the process of releasing the last read-lock; in
       // the picosecond between the first compare_exchange_strong() and
       // the one below the read-lock may have been released and a new
       // read-write lock may have been established, so we still have to
       // consider the read-write-lock case
       current_owner = ReadOnlyLock();
       if( f_owner.compare_exchange_strong( current_owner,
					    v_ownersLock() ) ) {
	// a read-lock was indeed still in place and v_owner was
	// active-unlocked, active lock successfully acquired
	// check if there is any read-write lock from the current thread
	auto it = v_owners.find( std::this_thread::get_id() );
	// immediately release the active lock on v_ownersLock
	f_owner = ReadOnlyLock();
	if( it == v_owners.end() )  // if not
	 break;            // it's OK to go to sleep on the mutex
	else
	 return( false );  // the only recourse is to fail
	// note: clearly, even if there were a single read-lock from this
	//       thread, it is not possible that the read-lock is being
	//       released right now, because this should be done by this
	//       thread, which is now doing this operation rather than
	//       read-unlocking the Block
        }
       else
	if( current_owner == v_ownersLock() )  // v_owner was active-locked
	 continue;                             // repeat until access granted
       // else, a read-write lock succeeded in sneaking in: however, this
       // read-write lock cannot possibly be running in the same thread, so
       // it's still OK to go to sleep on the mutex without further checks
       break;
       }  // end( for( ever ) )
      }
     else  // the current owner is a read-write lock
      if( f_owner_thread_id == std::this_thread::get_id() )
       // but it runs on the same thread
       return( false );           // the only recourse is to fail
       // note: clearly, it is not possible that the current owner is
       //       releasing the Block precisely at this point, because it
       //       runs in the very same thread, and this thread is now doing
       //       this operation rather than unlocking the Block

     // if we get here, either the current owner is a read-write lock that
     // runs in a different thread, or it is a bunch of read-locks, all of
     // which run in a different thread

     f_mutex.lock();  // first of all, lock the mutex
     // one expects the mutex to be locked, so that the thread will go to
     // sleep; in the weird case where the entity having taken ownership of
     // the Block has not succeeded to lock the mutex already, the next step
     // of trying to get ownership of the Block will fail immediately, the
     // mutex will be released and a new attempt will be made

     // (possibly) after having slept on the mutex, try to own the Block
     current_owner = nullptr;
     if( f_owner.compare_exchange_strong( current_owner , owner ) ) {
      // no owner came in and beat us, the Block can be owned
      if( lock_sub_block( owner ) ) {  // ... if all sub-Block can be owned
       f_owner_thread_id = std::this_thread::get_id();  // record own id
       return( true );                                  // all done
       }

      // it was not possible to lock all the sub-Block;
      f_owner = nullptr;  // release ownership of the Block
      f_mutex.unlock();   // release the mutex
      return( false );    // failed to lock the Block
      }
     else {
      // someone managed to do the atomic swap before us and it is now
      // trying to lock the mutex to finalize locking of the sub-Block:
      // the only solution is to give up the mutex to allow it do that,
      // and then repeat everything (try the atomic swap, fail, go to
      // sleep on the mutex, rinse and repeat)
      f_mutex.unlock();
      }
     }
   }  // end infinite loop
  }  // end( lock )

/*--------------------------------------------------------------------------*/
 /// unlock the Block
 /** Removes the current read-write-lock on the Block, relinquishing
  * ownership. This of course also relinquishes ownership on all the
  * sub-Block of the Block.
  *
  * The unlocking is performed "in reverse" than the locking, i.e., with a
  * post-visit (the father is unlocked after all the sons are) of the
  * Block tree.
  *
  * It is clearly incorrect both to read-write-unlock a non-read-write-locked
  * Block (which means, either an unlocked Block or a read-locked one) and
  * for a non-owner to unlock a Block that some other entity had locked.
  * This is why unlock() also has the owner parameter; if any of the above
  * incorrect operations are attempted, exception will be thrown.
  *
  * Also, note that
  *
  *     AN OWNER RE-LOCKING AN ALREADY OWNED Block IS A NO-OP, WHICH
  *     IMPLIES THAT ANY SEQUENCE OF LOCKING A Block BY THE SAME OWNER
  *     MUST BE UNLOCKED ONLY ONCE
  *
  * It is responsibility of the caller of lock()/unlock() to keep track of
  * this, see the comments to is_owned_by().
  *
  * In most cases, there should be no reason for derived classes to mess up
  * with this mechanism. However, some :Block may have nonstandard behavior,
  * e.g. about how they store their sub-Block, and therefore for maximal
  * flexibility this method is virtual. */

 virtual void unlock( const void * owner ) {
  if( ( ! owner ) || ( owner == ReadOnlyLock() ) ||
      ( owner == v_ownersLock() ) || ( f_owner != owner ) )
   throw( std::logic_error( "invalid owner in unlock()" ) );

  for( auto sb = v_Block.end() ; sb != v_Block.begin() ; )
   ( *( --sb ) )->unlock( owner );

  f_owner = nullptr;  // release ownership of the Block
  f_mutex.unlock();   // release the mutex
  }

/*--------------------------------------------------------------------------*/
 /// tries to read-lock the Block, return true on success
 /** Attempts to acquire a read-only lock. Unlike the "normal" lock, which is
  * read-write, any number of read-only locks can be concurrently acquired on
  * the same Block; however, each of the entities is only permitted to read
  * the Block, but not to modify it.
  *
  * As a consequence, the Block is not "owned" by any individual entity, but
  * rather from an "abstract" entity indicating "a group of read-locks". A
  * read-lock is incompatible with a read-write-lock: trying to acquire a
  * read-lock while the Block is read-write-locked results in immediate
  * failure if the owner of the read-write-lock runs on the same thread as
  * the caller, and in sleeping on the mutex waiting for the current owner to
  * release the read-write-lock. (Almost) symmetrically, trying to acquire a
  * read-write-lock while the Block is read-locked results in immediate
  * failure if *any* of the callers having acquired a read-lock run on the
  * same thread as the caller, and in sleeping on the mutex waiting for all
  * the current read-locks to be released otherwise.
  *
  * Note that the current implementation of the method does active wait on
  * the f_owner field, a std::atomic< void * >. This is sensible under the
  * assumption that std::atomic< void * >{}.is_lock_free() == true, which
  * should happen in most systems. A more sophisticated implementation
  * would require a std::atomic_flag added to the class and used instead
  * whenever std::atomic< void * >{}.is_lock_free() == false.
  *
  * In most cases, there should be no reason for derived classes to mess
  * up with this mechanism. However, some :Block may have nonstandard
  * behavior, e.g. about how they store their sub-Block, and therefore for
  * maximal flexibility this method is virtual. */

 virtual bool read_lock( void ) {
  for( ; ; ) {  // this may have to be repeated many times
   const void * current_owner = nullptr;
   if( f_owner.compare_exchange_strong( current_owner , v_ownersLock() ) ) {
    // the Block was un-owned: the first read-lock has been successfully
    // acquired, and v_owners has been "active locked" at the same time

    // insert the first element in v_owners: the id of the current thread
    // and the counter of how many readers are there in this thread (1)
    v_owners.insert( std::make_pair< std::thread::id , unsigned short >(
     std::this_thread::get_id() , 1 ) );

    // immediately release the active lock on v_owners
    f_owner = ReadOnlyLock();

    f_mutex.lock();  // lock the mutex: this is a bit weird because the
    // Block is surely "free", but it is done
    // immediately so that other Block willing to take a read-write lock from
    // different threads can sleep on the mutex even during the potentially
    // long time its takes to (try to) read-lock all sub-Block

    // try to read-lock all sub-Block and return the success of the
    // operation; upon failure, also read-unlock the Block
    return( read_lock_sub_block() );
    }
   else {  // an owner already existed
    if( ( current_owner == ReadOnlyLock() ) ||
        ( current_owner == v_ownersLock() ) ) {
     // ... but it's another read-lock

     for( ; ; ) {  // acquire the "active lock" on v_owners
      // note that the Block may have been read-locked by a single entity
      // which was precisely in the process of releasing the last read-lock;
      // in the picosecond between the first compare_exchange_strong() and
      // the one right below, the read-lock may have been released and a new
      // read-write lock has been established, in which case the read-lock
      // has to either fail or wait
      current_owner = ReadOnlyLock();
      if( f_owner.compare_exchange_strong( current_owner , v_ownersLock() ) ) {
       // v_owner was active-unlocked, active lock successfully acquired
       // find the record corresponding to thread::id if there is any, or
       // create one with count 0 (the default value of short int) otherwise,
       // and then increase the counter
       v_owners[ std::this_thread::get_id() ]++;

       // release the active lock on v_owners
       f_owner = ReadOnlyLock();

       // note that there is no need to lock the mutex, since it's been
       // locked already by the first successful read-lock

       // try to read-lock all sub-Block and return the success of the
       // operation; upon failure, also read-unlock the Block
       return( read_lock_sub_block() );
       }
      else
       if( current_owner == v_ownersLock() )  // v_owner was active-locked
	continue;                             // repeat until access granted
       else  // a different owner succeeded in sneaking in
	break;
      }  // end( for( ever ) )
     }  // end( if( was read-locked already ) )

    // a read-write-lock is established on the Block (or was established
    // on the fly while trying to acquire the read-lock instead)
    // if the owner of the read-write-lock runs in this thread
    if( f_owner_thread_id == std::this_thread::get_id() )
     return( false );  // the only recourse is to fail

    f_mutex.lock();  // go to sleep on the mutex, which should be locked
    // after the mutex is released, go try again your luck with the lock
    }
   }  // end infinite loop
  }  // end( read-lock() )

/*--------------------------------------------------------------------------*/
 /// read-unlock the Block
 /** Removes one of the current read-lock on the Block, possibly completely
  * relinquishing ownership if this was the last active read-lock on the
  * Block. This of course also removes one of the current read-lock on all
  * the sub-Block of the Block.
  *
  * The unlocking is performed "in reverse" than the locking, i.e., with a
  * post-visit (the father is unlocked after all the sons are) of the Block
  * tree.
  *
  * It is clearly incorrect to read-unlock a non-read-locked Block (which
  * means, either an unlocked Block or a read-write-locked one): if this is
  * attempted, exception will be thrown.
  *
  * However, note that
  *
  *     UNLIKE READ-WRITE-LOCKS, READ-ONLY LOCKS ARE COUNTED. THIS MEANS
  *     THAT THE SIMPLE RULE "ONE READ-UNLOCK FOR EACH READ-LOCK" DOES
  *     APPLY IN THIS CASE. YET, READ-ONLY LOCKS ARE COUNTED *PER THREAD*,
  *     WHICH IMPLIES THAT EACH THREAD THAT MAKES A READ-ONLY LOCK MUST
  *     EVENTUALLY MAKE A READ-ONLY-UNLOCK
  *
  * In most cases, there should be no reason for derived classes to mess
  * up with this mechanism. However, some :Block may have nonstandard
  * behavior, e.g. about how they store their sub-Block, and therefore for
  * maximal flexibility this method is virtual. */

 virtual void read_unlock( void ) {
  if( ( f_owner != ReadOnlyLock() ) && ( f_owner != v_ownersLock() ) )
   throw( std::logic_error( "trying to read-unlock a non-read-locked Block"
			    ) );

  // read-unlock all the sub-Block
  for( auto sb = v_Block.end() ; sb != v_Block.begin() ; )
   ( *( --sb ) )->read_unlock();

  guts_of_read_unlock();  // read-unlock the Block
  }

/** @} ---------------------------------------------------------------------*/
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
 /// setting the BlockConfig
 /** This method sets the BlockConfig of this Block. The BlockConfig object
  * pointed by \p newBC is not copied, but it becomes property of this
  * Block. If the given pointer is equal to the one currently stored in this
  * Block, a call to this function has no effect (no operation is performed;
  * in particular, no pointer is deleted). Otherwise:
  *
  * - If newBC == nullptr then the current BlockConfig of this Block is
  *   deleted if \p deleteold == true. Moreover, the pointer to the
  *   BlockConfig of this Block becomes nullptr.
  *
  * - If newBC->is_diff() == true, the individual non-nullptr
  *   sub-Configuration in \p newBC replace one-by-one those in the
  *   BlockConfig of this Block, if any, while the nullptr sub-Configuration
  *   mean that the existing ones in the BlockConfig of this Block are kept
  *   untouched. If this Block has no BlockConfig, \p newBC is just moved.
  *
  * - If newBC->is_diff() == false then \p newBC is moved into the BlockConfig
  *   of this Block. If \p deleteold is true then, before moving \p newBC, the
  *   pointer to the BlockConfig currently stored in this Block is deleted,
  *   destroying the previous BlockConfig. Hence, a call to set_BlockConfig()
  *   (with default \p newBC = nullptr and \p deleteold = true) with
  *   newBC->is_diff() == false deletes the BlockConfig to this Block.
  *
  * It is important to notice that, since \p newBC becomes property of this
  * Block, if newBC->is_diff() == true (and this Block already has a
  * BlockConfig) then \p newBC is deleted once the sub-Configuration of \p
  * newBC have been moved.
  *
  * Note that BlockConfig only refers to the "main" Block, i.e., it does not
  * provide any way to configure the sub-Block. This can of course be done
  * programmatically by visiting the whole tree structure of the Block and
  * calling set_BlockConfig() whenever appropriate. Furthermore, specific
  * components are separately provided for easing this task (see
  * RBlockConfig.h). */

 virtual void set_BlockConfig( BlockConfig * newBC = nullptr ,
                               bool deleteold = true );

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

 virtual void generate_abstract_variables( Configuration * stvv = nullptr ) {
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

 virtual void generate_dynamic_variables( Configuration * dyvv = nullptr ) {
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

 virtual void generate_abstract_constraints( Configuration * stcc = nullptr ) {
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

 virtual void generate_dynamic_constraints( Configuration * dycc = nullptr ) {
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

 virtual void generate_objective( Configuration * objc = nullptr ) {
  for( auto blck : v_Block )
   blck->generate_objective();
  }

/** @} ---------------------------------------------------------------------*/
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

 const std::string & classname( void ) const { return( private_name() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the string name of this Block

 const std::string & name( void ) const { return( f_name ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the "father" Block of this Block

 p_Block get_f_Block( void ) const { return( f_Block ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the BlockConfig object of this Block
 /** The method returns a (const) pointer to the current BlockConfig object
  * of this Block, if any. */

 const BlockConfig * get_BlockConfig( void ) { return( f_BlockConfig ); }

/*--------------------------------------------------------------------------*/
 /// getting the pointer to the current Objective
 /** Getting a pointer to current :Objective. Of course, for this method to
  * return something meaningful (i.e., for the returned pointer to be
  * non-nullptr) the abstract representation of the Objective must have been
  * constructed, cf. generate_objective(). */

 Objective * get_objective( void ) const { return( f_Objective ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the current Objective
 /** This method, template over the class Obj (which must derive from
  * Objective), get the current Objective, which is supposed to be an
  * Obj *, and returns it. If the Objective is not of the required type,
  * exception is thrown. */

 template< class Obj >
 std::enable_if_t< std::is_base_of_v< Objective , Obj > , Obj * >
 get_objective( void ) const {
  auto obj = dynamic_cast< Obj * >( f_Objective );
  if( ! obj )
   throw( std::invalid_argument(
		     "get_objective: objective is not of  required type" ) );
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
  * implementation arbitrarily returns Objective::eUndef, i.e., "the Block
  * does not care". The rationale is that a Block that only encodes for a
  * feasibility problem actually can have no Objective even if the
  * "abstract representation" is fully constructed, but then this means that
  * the Objective is constantly 0 across all feasible solutions and therefore
  * in principle the sense does not matter (although this changes if an
  * unfeasible solution should be associated with a value +Infinity or
  * -Infinity). */

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
  return( Inf< double >() );
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
  return( - Inf< double >() );
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
 /// returns the number of sub-Block of the Block

 Index get_number_nested_Blocks( void ) const { return( v_Block.size() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the i-th sub-Block of the Block (nullptr if not there)

 Block * get_nested_Block( Index i ) const {
  if( i >= v_Block.size() )
   return( nullptr );
  return( v_Block[ i ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a pointer to the sub-Block with given \p name, nullptr if none

 Block * get_nested_Block( const std::string & name ) const {
  for( auto bi : v_Block )
   if( bi && ( bi->name() == name ) )
    return( bi );

  return( nullptr );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the sub-Block with given \p name
 /** Returns the index the sub-Block  with given \p name; if no sub-Block
  * has this name, a number >= get_number_nested_Blocks() is returned. */

 Index get_nested_Block_index( const std::string & name ) const {
  auto bit = v_Block.begin();
  for( ; bit != v_Block.end(); ++bit )
   if( *bit && ( ( *bit )->name() == name ) )
    break;

  return( std::distance( v_Block.begin(), bit ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the given sub-Block
 /** Returns the index of the given sub-Block; if it is not a sub-Block of
  * the Block, a number >= get_number_nested_Blocks() is returned. */

 Index get_nested_Block_index( Block * const block ) const {
  auto bit = std::find( v_Block.begin(), v_Block.end(), block );
  return( std::distance( v_Block.begin(), bit ) );
  }

/** @} ---------------------------------------------------------------------*/
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
  * - a pointer to a boost::multi_array< C , K >, where C is any class derived
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
 /// returns the number of groups of static Constraint

 Index get_number_static_constraints( void ) const {
  return( v_s_Constraint.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" static Constraint by index
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the i-th static constraint group, which is supposed
  * to be a simple Const *, and returns it. If anything goes wrong, nullptr is
  * returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , Const * >
 get_static_constraint( Index i ) const {
  if( i >= v_s_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< Const * >( v_s_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" static Constraint by name
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the constraint group with given \p name, which is
  * supposed to be a simple Const *, and returns it. If anything goes wrong,
  * nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , Const * >
 get_static_constraint( const std::string & name ) const {
  auto it = std::find( v_s_Constraint_names.begin(),
                       v_s_Constraint_names.end(), name );
  if( it == v_s_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< Const * >(
   v_s_Constraint[ std::distance( v_s_Constraint_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a (static) std::vector of Constraint by index
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the i-th static constraint group, which is supposed
  * to be a std::vector< Const > *, and returns it. If anything goes wrong,
  * nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::vector< Const > * > get_static_constraint_v( Index i ) const {
  if( i >= v_s_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< std::vector< Const > * >( v_s_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a (static) std::vector of Constraint by name
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the constraint group with given \p name, which is
  * supposed to be a std::vector< Const > *, and returns it. If anything goes
  * wrong, nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::vector< Const > * >
 get_static_constraint_v( const std::string & name ) const {
  auto it = std::find( v_s_Constraint_names.begin(),
                       v_s_Constraint_names.end(), name );
  if( it == v_s_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< std::vector< Const > * >(
   v_s_Constraint[ std::distance( v_s_Constraint_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a (static) boost::multi_array of Constraint by index
 /** This method, template over the class Const (which must derive from
  * Constraint) and the integer K, extracts the i-th static constraint group,
  * which is supposed to be a boost::multi_array< Const , K > *, and returns
  * it. If anything goes wrong, nullptr is returned. */

 template< class Const , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  boost::multi_array< Const , K > * > get_static_constraint( Index i ) const {
  if( i >= v_s_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< Const , K > * >(
                                                    v_s_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a (static) boost::multi_array of Constraint by name
 /** This method, template over the class Const (which must derive from
  * Constraint) and the integer K, extracts the static constraint group with
  * given \p name, which is supposed to be a boost::multi_array< Const , K > *,
  * and returns it. If anything goes wrong, nullptr is returned. */

 template< class Const , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  boost::multi_array< Const , K > * >
 get_static_constraint( const std::string & name ) const {
  auto it = std::find( v_s_Constraint_names.begin(),
                       v_s_Constraint_names.end(), name );
  if( it == v_s_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< Const , K > * >(
   v_s_Constraint[ std::distance( v_s_Constraint_names.begin(), it ) ] ) );
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
  * - a pointer to a boost::multi_array< V , K >, where V is any class derived
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

 c_Vec_any & get_static_variables( void ) const { return( v_s_Variable ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the number of groups of static Variable

 Index get_number_static_variables( void ) const {
  return( v_s_Variable.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" static Variable by index
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th static variable group, which is supposed to
  * be a simple Var *, and returns it. If anything goes wrong, nullptr is
  * returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable, Var > , Var * >
 get_static_variable( Index i ) const {
  if( i >= v_s_Variable.size() )
   return( nullptr );
  return( boost::any_cast< Var * >( v_s_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" static Variable by name
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the static variable group with given \p name, which
  * is supposed to be a simple Var *, and returns it. If anything goes wrong,
  * nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable, Var > , Var * >
 get_static_variable( const std::string & name ) const {
  auto it = std::find( v_s_Variable_names.begin() ,
                       v_s_Variable_names.end() , name );
  if( it == v_s_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< Var * >(
   v_s_Variable[ std::distance( v_s_Variable_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a (static) std::vector< Variable > by index
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th static variable group, which is supposed to
  * be a std::vector< Var > *, and returns it. If anything goes wrong,
  * nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable, Var > ,
  std::vector< Var > * > get_static_variable_v( Index i ) const {
  if( i >= v_s_Variable.size() )
   return( nullptr );
  return( boost::any_cast< std::vector< Var > * >( v_s_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a (static) std::vector< Variable > by name
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the static variable group with given \p name, which
  * is supposed to be a std::vector< Var > *, and returns it. If anything
  * goes wrong, nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , std::vector< Var > * >
 get_static_variable_v( const std::string & name ) const {
  auto it = std::find( v_s_Variable_names.begin() ,
                       v_s_Variable_names.end() , name );
  if( it == v_s_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< std::vector< Var > * >(
   v_s_Variable[ std::distance( v_s_Variable_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a (static) boost::multi_array of Variable by index
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the i-th static variable group,
  * which is supposed to be a boost::multi_array< Var , K > *, and returns
  * it. If anything goes wrong, nullptr is returned. */

 template< class Var , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Variable , Var > ,
  boost::multi_array< Var , K > * >
 get_static_variable( Index i ) const {
  if( i >= v_s_Variable.size() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< Var, K > * >(
                                                      v_s_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a (static) boost::multi_array of Variable by name
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the static variable group with
  * given \p name, which is supposed to be a boost::multi_array< Var , K > *,
  * and returns it. If anything goes wrong, nullptr is returned. */

 template< class Var , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Variable, Var > ,
  boost::multi_array< Var , K > * >
 get_static_variable( const std::string & name ) const {
  auto it = std::find( v_s_Variable_names.begin() ,
                       v_s_Variable_names.end() , name );
  if( it == v_s_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< Var, K > * >(
   v_s_Variable[ std::distance( v_s_Variable_names.begin(), it ) ] ) );
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
  * - a pointer to a single std::list< C >, where class C is derived from
  *   Constraint (obviously you can't make a std::list of the base Constraint
  *   class, which is why pointers to Constraint are also allowed, see below);
  *
  * - a pointer to a std::vector< std::list< C > >, where class C is derived
  *   from Constraint;
  *
  * - a pointer to a boost::multi_array< std::list< C > , K >,  where class C is
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
  * The rationale of the structure is that the lists can be indexed over (in
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
 /// returns the number of groups of dynamic Constraint

 Index get_number_dynamic_constraints( void ) const {
  return( v_s_Constraint.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" dynamic Constraint by index
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the i-th dynamic constraint group, which is
  * supposed to be a std::list< Const > *, and returns it. If anything goes
  * wrong, nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::list< Const > * > get_dynamic_constraint( Index i ) const {
  if( i >= v_d_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< std::list< Const > * >( v_d_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a "simple" dynamic Constraint by name
 /** This method, template over the class Const (which must derive from
  * Constraint), extracts the i-th dynamic constraint group, which is
  * supposed to be a std::list< Const > *, and returns it. If anything goes
  * wrong, nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::list< Const > * >
 get_dynamic_constraint( const std::string & name ) const {
  auto it = std::find( v_d_Constraint_names.begin() ,
                       v_d_Constraint_names.end() , name );
  if( it == v_d_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< std::list< Const > * >(
   v_d_Constraint[ std::distance( v_d_Constraint_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a std::vector of dynamic Constraint by index
 /** This method, template over the class Const (which must derive from
 * Constraint), extracts the i-th dynamic constraint group, which is supposed
 * to be a std::vector< std::list< Const > > *, and returns it. If anything
 * goes wrong, nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::vector< std::list< Const > > * >
 get_dynamic_constraint_v( Index i ) const {
  if( i >= v_d_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< std::vector< std::list< Const > > * >(
                                                     v_d_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a std::vector of dynamic Constraint by name
 /** This method, template over the class Const (which must derive from
 * Constraint), extracts the dynamic constraint group with given \p name,
 * which is supposed to be a std::vector< std::list< Const > > *, and returns
 * it. If anything goes wrong, nullptr is returned. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  std::vector< std::list< Const > > * >
 get_dynamic_constraint_v( const std::string & name ) const {
  auto it = std::find( v_d_Constraint_names.begin() ,
                       v_d_Constraint_names.end() , name );
  if( it == v_d_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< std::vector< std::list< Const > > * >(
   v_d_Constraint[ std::distance( v_d_Constraint_names.begin(), it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a boost::multi_array of dynamic Constraint by index
 /** This method, template over the class Const (which must derive from
  * Constraint) and the integer K, extracts the i-th dynamic constraint group,
  * which is supposed to be a boost::multi_array< std::list< Const > , K > *,
  * and returns it. If anything goes wrong, nullptr is returned. */

 template< class Const , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  boost::multi_array< std::list< Const > , K > * >
 get_dynamic_constraint( Index i ) const {
  if( i >= v_d_Constraint.size() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< std::list< Const > , K > * >(
						     v_d_Constraint[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a boost::multi_array of dynamic Constraint by name
 /** This method, template over the class Const (which must derive from
  * Constraint) and the integer K, extracts the dynamic constraint group
  * with given \p name, which is supposed to be a
  * boost::multi_array< std::list< Const > , K > *, and returns it. If
  * anything goes wrong, nullptr is returned. */

 template< class Const , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > ,
  boost::multi_array< std::list< Const > , K > * >
 get_dynamic_constraint( const std::string & name ) const {
  auto it = std::find( v_d_Constraint_names.begin() ,
                       v_d_Constraint_names.end() , name );
  if( it == v_d_Constraint_names.end() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< std::list< Const > , K > * >(
   v_d_Constraint[ std::distance( v_d_Constraint_names.begin() , it ) ] ) );
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
  * - a pointer to a single std::list< V >, where class V is derived from
  *   Variable (obviously you can't make a std::list of the base Variable
  *   class, which is why pointers to Variable are also allowed, see below);
  *
  * - a pointer to a std::vector< std::list< V > >, where class V is derived
  *   from Variable;
  *
  * - a pointer to a boost::multi_array< std::list< V > , K >,  where class V is
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
  * The rationale of the structure is that the lists can be indexed over (in
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

 c_Vec_any & get_dynamic_variables( void ) const { return( v_d_Variable ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the number of groups of dynamic Variable

 Index get_number_dynamic_variables( void ) const {
  return( v_s_Variable.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" dynamic Variable by index
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th dynamic variable group, which is supposed
  * to be a std::list< Var > *, and returns it. If anything goes wrong,
  * nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , std::list< Var > * >
 get_dynamic_variable( Index i ) const {
  if( i >= v_d_Variable.size() )
   return( nullptr );
  return( boost::any_cast< std::list< Var > * >( v_d_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get the a "simple" dynamic Variable by name
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the dynamic variable group with given \p name,
  * which is supposed to be a std::list< Var > *, and returns it. If
  * anything goes wrong, nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable, Var > , std::list< Var > * >
 get_dynamic_variable( const std::string & name ) const {
  auto it = std::find( v_d_Variable_names.begin() ,
                       v_d_Variable_names.end() , name );
  if( it == v_d_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< std::list< Var > * >(
       v_d_Variable[ std::distance( v_d_Variable_names.begin() , it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a std::vector of dynamic Variable by index
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the i-th dynamic variable group, which is supposed to
  * be a std::vector< std::list< Var > > *, and returns it. If anything goes
  * wrong, nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > ,
  std::vector< std::list< Var > > * >
 get_dynamic_variable_v( Index i ) const {
  if( i >= v_d_Variable.size() )
   return( nullptr );
  return( boost::any_cast< std::vector< std::list< Var > > * >(
                                                      v_d_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// template method to get a std::vector of dynamic Variable by name
 /** This method, template over the class Var (which must derive from
  * Variable), extracts the dynamic variable group with given \p name, which
  * is supposed to be a std::vector< std::list< Var > > *, and returns it. If
  * anything goes wrong, nullptr is returned. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > ,
  std::vector< std::list< Var > > * >
 get_dynamic_variable_v( const std::string & name ) const {
  auto it = std::find( v_d_Variable_names.begin() ,
                       v_d_Variable_names.end() , name );
  if( it == v_d_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< std::vector< std::list< Var > > * >(
       v_d_Variable[ std::distance( v_d_Variable_names.begin() , it ) ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a boost::multi_array< , K > of dynamic Variable by index
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the i-th dynamic variable group,
  * which is supposed to be a boost::multi_array< std::list< Var > , K > *,
  * and returns it. If anything goes wrong, nullptr is returned. */

 template< class Var , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Variable , Var > ,
  boost::multi_array< std::list< Var > , K > * >
 get_dynamic_variable( Index i ) const {
  if( i >= v_d_Variable.size() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< std::list< Var > , K > * >(
						      v_d_Variable[ i ] ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// method to get a boost::multi_array< , K > of dynamic Variable by name
 /** This method, template over the class Var (which must derive from
  * Variable) and the integer K, extracts the dynamic variable group with
  * given \p name, which is supposed to be a
  * boost::multi_array< std::list< Var > , K > *, and returns it. If
  * anything goes wrong, nullptr is returned. */

 template< class Var , unsigned short K >
 std::enable_if_t< std::is_base_of_v< Variable , Var > ,
  boost::multi_array< std::list< Var > , K > * >
 get_dynamic_variable( const std::string & name ) const {
  auto it = std::find( v_d_Variable_names.begin() ,
                       v_d_Variable_names.end() , name );
  if( it == v_d_Variable_names.end() )
   return( nullptr );
  return( boost::any_cast< boost::multi_array< std::list< Var > , K > * >(
         v_d_Variable[ std::distance( v_d_Variable_names.begin() , it ) ] ) );
  }

/*--------------------------------------------------------------------------*/
 /// getting the static Constraint names
 /** Returns a const reference to the vector storing the names of the
  * different groups of static Constraints of the Block */

 c_Vec_string & get_s_const_name( void ) const {
  return( v_s_Constraint_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the name of the i-th group of static Constraint, "" if none

 const std::string & get_s_const_name( Index i ) const {
  static const std::string _empty;
  if( i >= v_s_Constraint_names.size() )
   return( _empty );
  return( v_s_Constraint_names[ i ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index the group of static Constraint with given name
 /** Returns the index the group of static Constraint with given name; if
  * no group has this name, a number >= get_number_static_constraints()
  * is returned. */

 Index get_s_const_index( const std::string & name ) const {
  auto it = std::find( v_s_Constraint_names.begin() ,
                       v_s_Constraint_names.end() , name );
  return( std::distance( v_s_Constraint_names.begin() , it ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the static Variable names
 /** Returns a const reference to the vector storing the names of the
  * different groups of static Variables of the Block */

 c_Vec_string & get_s_var_name( void ) const {
  return( v_s_Variable_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the name of the i-th group of static Variable, "" if none

 const std::string & get_s_var_name( Index i ) const {
  static const std::string _empty;
  if( i >= v_s_Variable_names.size() )
   return( _empty );
  return( v_s_Variable_names[ i ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the index the group of static Variable with given name
 /** Returns the index the group of static Variable with given name; if
  * no group has this name, a number >= get_number_static_variables()
  * is returned. */

 Index get_s_var_index( const std::string & name ) const {
  auto it = std::find( v_s_Variable_names.begin() ,
                       v_s_Variable_names.end() , name );
  return( std::distance( v_s_Variable_names.begin() , it ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the dynamic Constraint names
 /** Returns a const reference to the vector storing the names of the
  * different groupa of dynamic Constraints of the Block */

 c_Vec_string & get_d_const_name( void ) const {
  return( v_d_Constraint_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the name of the i-th group of dynamic Constraint, "" if none

 const std::string & get_d_const_name( Index i ) const {
  static const std::string _empty;
  if( i >= v_d_Constraint_names.size() )
   return( _empty );
  return( v_d_Constraint_names[ i ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the index the group of dynamic Constraint with given name
 /** Returns the index the group of dynamic Constraint with given name; if
  * no group has this name, a number >= get_number_dynamic_constraints()
  * is returned. */

 Index get_d_const_index( const std::string & name ) const {
  auto it = std::find( v_d_Constraint_names.begin() ,
                       v_d_Constraint_names.end() , name );
  return( std::distance( v_d_Constraint_names.begin() , it ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the dynamic Variable names
 /** Returns a const reference to the vector storing the names of the
  * different groups of dynamic Variables of the Block */

 c_Vec_string & get_d_var_name( void ) const {
  return( v_d_Variable_names );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the name of the i-th group of dynamic Variable, "" if none

 const std::string & get_d_var_name( Index i ) const {
  static const std::string _empty;
  if( i >= v_d_Variable_names.size() )
   return( _empty );
  return( v_d_Variable_names[ i ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the index the group of dynamic Variable with given name
 /** Returns the index the group of dynamic Variable with given name; if
  * no group has this name, a number >= get_number_dynamic_variables()
  * is returned. */

 Index get_d_var_index( const std::string & name ) const {
  auto it = std::find( v_d_Variable_names.begin() ,
                       v_d_Variable_names.end() , name );
  return( std::distance( v_d_Variable_names.begin() , it ) );
  }

/** @} ---------------------------------------------------------------------*/
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
  * \p list is obviously not "const", as the list will be updated. Note that
  * the base class implementation of this method just does this and possibly
  * issues the appropriate BlockModAdd; as list is supposed to be a part of
  * the "abstract representation", this means that the "physical
  * representation" of the corresponding dynamic Constraint (if any exists)
  * is not updated, as this should clearly be responsibility of the derived
  * class (maybe within the corresponding implementation of this method).
  *
  * Note that the new Constraint must already be organized in a list, which
  * is "spliced" into the given one. This is crucial because splicing is the
  * only way in which elements can be added to a list without being copied,
  * i.e., their memory address being changed. As a consequence of using
  * std::list::splice(), existing iterators in \p newlist are still valid
  * after the call and can be used, now iterating into \p list.
  *
  * The parameter issueMod decides if and how the BlockModAdd is issued, as
  * described in Observer::make_par(). */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_dynamic_constraints( std::list< Const > & list ,
                          std::list< Const > & newlist ,
                          ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// adds a bunch of new Variable at the end of the given list
 /** Adds a bunch of new Variable at the end of the given list.
  *
  * \p list is obviously not "const", as the list will be updated. Note that
  * the base class implementation of this method just does this and possibly
  * issues the appropriate BlockModAdd; as list is supposed to be a part of
  * the "abstract representation", this means that the "physical
  * representation" of the corresponding dynamic Variable (if any exists) is
  * not updated, as this should clearly be responsibility of the derived
  * class (maybe within the corresponding implementation of this method).
  *
  * Note that the new Variables must already be organized in a list, which
  * is "spliced" into the given one. This is crucial because splicing is the
  * only way in which elements can be added to a list without being copied,
  * i.e., their memory address being changed. As a consequence of using
  * std::list::splice(), existing iterators in \p newlist are still valid
  * after the call and can be used, now iterating into \p list.
  *
  * The parameter issueMod decides if and how the BlockModAdd is issued, as
  * described in Observer::make_par(). */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 add_dynamic_variables( std::list< Var > & list ,
                        std::list< Var > & newlist ,
                        ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// removes a bunch of Constraint from the given list
 /** Removes a bunch of Constraint from the given list.
  *
  * The parameter list is obviously not "const", as the list will be updated.
  * Note that the base class implementation of this method just does this and
  * possibly issues the appropriate BlockModRmv*; as list is supposed to be a
  * part of the "abstract representation", this means that if the "physical
  * representation" of the corresponding dynamic Constraint (if any exists)
  * is different from its "abstract representation" is not updated, as this
  * should clearly be responsibility of the derived class (maybe within the
  * corresponding implementation of this method).
  *
  * The Constraint to be removed are these whose iterator is found in the
  * std::vector rmvd. Note that, if the BlockModRmv* is issued, these
  * Constraint are not immediately deleted; rather, they are added to a list
  * stored into a field of the BlockModRmv* object, so that they remain alive
  * until the last interested Solver had had the chance to use them to
  * perform the required changes. This also implies that the same memory
  * address cannot be used for new Constraint (or anything), which also
  * avoids potential problems. As soon as the BlockModRmv* is eventually
  * destroyed, the Constraints are destroyed as well. If, instead, the
  * BlockModRmv* is not issued, the Constraint are destroyed immediately.
  *
  * Note that
  *
  *     IF THE BlockModRmv* IS ISSUED, THEN THE ITERATORS IN rmvd MUST BE
  *     ORDERED; THAT IS, THE *rmvd[ i ] MUST BE FOUND IN THE LIST BEFORE
  *     *rmvd[ i + 1 ] FOR ALL i
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

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 remove_dynamic_constraints( std::list< Const > & list ,
                             std::vector< typename
                                          std::list< Const >::iterator
                                          > & rmvd ,
                             ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// like remove_dynamic_constraint*s*( iterators ), just only one of them

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 remove_dynamic_constraint( std::list< Const > & list ,
                            typename std::list< Const >::iterator rmvd ,
                            ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of dynamic Constraint
 /** Removes a bunch of Constraint from the given list. Like
  * remove_dynamic_constraint( iterators ), but the elements to be deleted
  * are those in the usual left-closed, right-open interval range. The
  * Modification issued according to issueMod will be a BlockModRmvRngd,
  * unless the range actually covers all the list, in which case a
  * BlockModRmvSbst with subset().empty() is rather issued because it
  * "makes it clearer what happened". */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 remove_dynamic_constraints( std::list< Const > & list , Range range ,
                             ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a subset of dynamic Constraint
 /** Removes a bunch of Constraint from the given list. Like
  * remove_dynamic_constraint( iterators ), but the elements to be deleted
  * are given under the form of a std::vector< Index >. A special setting is
  * if subset.empty(), which means that all the elements in the list are
  * removed. The ordered parameter tells if subset is ordered by increasing
  * Index: if not, then it is ordered inside (unless of course if
  * subset.empty()). Not that this matters for the caller since, as the the
  * && tells, subset[] "becomes property" of Block, possibly to be shipped to
  * the issued BlockModRmvSbst if issueMod so instructs. */

 template< class Const >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 remove_dynamic_constraints( std::list< Const > & list ,
                             Subset && subset , bool ordered = false ,
                             ModParam issueMod = eModBlck );

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
  * Note that
  *
  *     IF THE BlockModRmv* IS ISSUED, THEN THE ITERATORS IN rmvd MUST BE
  *     ORDERED; THAT IS, THE *rmvd[ i ] MUST BE FOUND IN THE LIST BEFORE
  *     *rmvd[ i + 1 ] FOR ALL i
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

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 remove_dynamic_variables( std::list< Var > & list ,
                           std::vector< typename
                                        std::list< Var >::iterator > & rmvd ,
                           ModParam issueMod = eModBlck ,
                           ModParam issueindMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// like remove_dynamic_variable*s*( iterators ), just only one of them

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 remove_dynamic_variable( std::list< Var > & list ,
                          typename std::list< Var >::iterator rmvd ,
                          ModParam issueMod = eModBlck ,
                          ModParam issueindMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of dynamic Variable
 /** Removes a bunch of Variable from the given list. Like
  * remove_dynamic_constraint( iterators ), but the elements to be deleted
  * are those in the usual left-closed, right-open interval range. The
  * Modification issued according to issueMod will be a BlockModRmvRngd,
  * unless the range actually covers all the list, in which case a
  * BlockModRmvSbst with subset().empty() is rather issued because it
  * "makes it clearer what happened". */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 remove_dynamic_variables( std::list< Var > & list , Range range ,
                           ModParam issueMod = eModBlck ,
                           ModParam issueindMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a subset of dynamic Variable
 /** Removes a bunch of Variable from the given list. Like
  * remove_dynamic_constraint( iterators ), but the elements to be deleted
  * are given under the form of a std::vector< Index >. A special setting is
  * if subset.empty(), which means that all the elements in the list are
  * removed. The ordered parameter tells if subset is ordered by increasing
  * Index: if not, then it is ordered inside (unless of course if
  * subset.empty()). Not that this matters for the caller since, as the the
  * && tells, subset[] "becomes property" of Block, possibly to be shipped to
  * the issued BlockModRmvSbst if issueMod so instructs. */

 template< class Var >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 remove_dynamic_variables( std::list< Var > & list ,
                           Subset && subset , bool ordered = false ,
                           ModParam issueMod = eModBlck ,
                           ModParam issueindMod = eModBlck );

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

 void set_objective( Objective * newOF , ModParam issueMod = eModBlck );

/** @} ---------------------------------------------------------------------*/
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
 * - a certificate of non-emptiness of the Block, which means it represents
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
 * optimality and emptiness are "hard"; the converse happens for co-NP-hard
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
 *   SimpleConfiguration< double > specifying, say, the maximum relative
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
  * The useabstract parameter being true dictates that the solution should be
  * checked for feasibility w.r.t. "abstract representation" of the Block,
  * otherwise the "physical representation" of the Block should used. This
  * may yield different outcomes, as discussed below. However, useabstract
  * should only be considered a "hint", that the Block is allowed to ignore
  * if it must, This is obviously the case of is_feasible( true ) being
  * called without the "abstract representation" having been constructed.
  * Also, a Block may in principle not have a "physical representation" at
  * all (although weird, this is not ruled out) and therefore it may not be
  * able to use it when is_feasible( false ) is called. In these cases, the
  * Block should still do its best to return a meaningful return value with
  * the information it does possess, even if not the intended one.
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
  * SimpleConfiguration< double > specifying, say, the maximum relative
  * violation that a simple single numerical Constraint can have, or any
  * arbitrarily complex Configuration specifying different thresholds for
  * different groups of Constraint of the Block (say, via
  * SimpleConfiguration< std::vector< double > >), and arbitrarily complex
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
                           Configuration * fsbc = nullptr ) {
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
  * the "physical representation" of the Block should be used; as in
  * is_feasible(), this value has to be taken as a clue rather than as an
  * order, in the sense that if the required representation is not available
  * then the Block should still do its best to return a meaningful return
  * value with the information it does possess, even if not the intended one.
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
  * SimpleConfiguration< double > specifying, say, the maximum relative
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

 virtual bool is_optimal( bool useabstract = false ,
                          Configuration * optc = nullptr ) {
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
  * encode for a proper ray, with non-emptiness having to be established
  * in different ways (basically, this is a remit of the Solver).
  *
  * The useabstract parameter being true dictates that the check should be
  * performed using the "abstract representation" of the Block, otherwise
  * the "physical representation" of the Block should be used. See the
  * comments to is_feasible() for the cases where the check using the
  * "abstract representation" may give different results that that using
  * the "physical" one. Also, as in is_feasible(), this value has to be taken
  * as a clue rather than as an order, in the sense that if the required
  * representation is not available then the Block should still do its best
  * to return a meaningful return value with the information it does possess,
  * even if not the intended one
  *
  * Checking the property is likely to entail some numerical computation, say
  * to verify that some matrix-vector scalar product is "zero". This may
  * require numerical accuracy parameters, which is what the parameter fsbc
  * is designed to provide. If non-null, it is meant to point to an
  * arbitrarily complex Configuration object (although it can in fact be
  * as simple as a SimpleConfiguration< double > specifying, say, the maximum
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

 virtual bool is_unbounded( bool useabstract = false,
                            Configuration * fsbc = nullptr ) {
  return( true );
 }

/*--------------------------------------------------------------------------*/
 /// returns true if the Block provably has no feasible solutions
 /** Returns true if the Block provably has no feasible solutions, and a
  * certificate for this is readily available. This might very well be a hard
  * task, say if the Block encodes for an NP-hard problem.
  *
  * A case in which this is possible is if the problem is convex, since then a
  * convenient way to prove emptiness of the primal is to prove that the dual
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
  * non-emptiness of the dual having to be established in different ways
  * (basically, this is a remit of the Solver).
  *
  * The useabstract parameter being true dictates that the check should be
  * performed using the "abstract representation" of the Block, otherwise
  * the "physical representation" of the Block should be used. See the
  * comments to is_feasible() for the cases where the check using the
  * "abstract representation" may give different results that that using
  * the "physical" one. Also, as in is_feasible(), this value has to be taken
  * as a clue rather than as an order, in the sense that if the required
  * representation is not available then the Block should still do its best to
  * return a meaningful return value with the information it does possess,
  * even if not the intended one
  *
  * Checking the property is likely to entail some numerical computation, say
  * to verify that some matrix-vector scalar product is "zero". This may
  * require numerical accuracy parameters, which is what the parameter optc
  * is designed to provide. If non-null, it is meant to point to an
  * arbitrarily complex Configuration object (although it can in fact be
  * as simple as a SimpleConfiguration< double > specifying, say, the maximum
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
                        Configuration * optc = nullptr ) { return( false ); }

/** @} ---------------------------------------------------------------------*/
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
  * Blocks; the parameter \p r3bc is a pointer to an arbitrarily complex
  * Configuration object which allows to tell which one has to be produced.
  * Of course, the R3 Block of a :Block with sub-Block may require each
  * sub-Block to produce R3 Block of its (recursively), hence the use of
  * Configuration that allows arbitrarily complex nested parameters to be
  * used.
  *
  * The parameter \p base is provided for allowing the creation of R3 Blocks to
  * happen "piecemeal". The obvious use case is that of a class B1 : Block and
  * B2 : B1 where B1 provides some R3 Block, say the copy. If, as it is
  * sensible, B2 wants to provide the same R3 Block, it is useful for B2 to
  * be able to "ask B1 to work on its part of the R3 Block", while taking
  * care of the B2-specific part. The idea is then that B2 can construct the
  * R3 Block (say, of class B2 in case of the copy) an use B1::get_R3_Block()
  * to have the B1-specific part managed (say, copied). Of course, B1 will
  * have requirements about the specific type of :Block that it accepts as
  * second input, depending on r3bc; for instance, in the case of a copy it
  * has to be any class derived from B1 (such as B2).
  *
  * The parameter \p father is provided in case \p base is not, and therefore
  * the R3 Block has to be created inside the method; father is (the pointer
  * to) the "father Block" to the newly created R3 one, to be passed to the
  * constructor.
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
  * The method is given a default implementation just doing the following:
  *
  * - if there are no inner Block, just return the \p base argument;
  *
  * - if there are inner Block, \p base must not be nullptr (Block is an
  *   abstract class, so it cannot be Block to construct it): then, the
  *   inner Block of base are deleted, the vector is resized to the same size
  *   as that of this, and then a R3 Block is created for each inner Block of
  *   this and placed in the same position in base.
  *
  * If \p *r3bc is a SimpleConfiguration< std::vector< Configuration * >,
  * then the i-th element of the vector is passed as the r3bc parameter
  * in the call to get_R3_Block() to the i-the inner Block (otherwise, or
  * if the vector has no i-th element, nullptr is used). Note that the call
  * to get_R3_Block() is done with no \p base argument.
  *
  * A :Block should always explicitly state which kind of R3 Block can produce,
  * and the caller should always check the returned value for non-nullptr-dness
  * in case there is any doubt that the :Block is actually able to produce the
  * required R3 one. */

 virtual Block * get_R3_Block( Configuration * r3bc = nullptr ,
                               Block * base = nullptr ,
                               Block * father = nullptr ) {
  if( ! v_Block.empty() ) {
   // automate the creation of R3 Block for the sub-Block, hence do nothing
   // if there aren't any

   // base must be given because the base Block class is virtual
   if( ! base )
    throw( std::invalid_argument( "Block::get_R3_Block with no base" ) );

   // ensure that eny existing sub-Block is deleted (there should not be any)
   base->reset_nested_Block();

   auto cv = dynamic_cast< SimpleConfiguration< std::vector< Configuration * >
                                                > * >( r3bc );

   // set the i-th sub-Block of base as the R3 Block of the i-th sub-Block
   // of this, using base as its father and the i-th entry of the r3bc
   // vector (if any) as Configuration; however, no base can be provided,
   // so the sub-Block must be able to provide one itself (this cannot
   // recourse)
   for( Index i = 0 ; i < v_Block.size() ; ++i )
    base->add_nested_Block( v_Block[ i ]->get_R3_Block(
     ( cv && ( cv->f_value.size() > i ) ) ? cv->f_value[ i ] : nullptr ,
     nullptr , base ) );
   }

  return( base );
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

 virtual void map_back_solution( Block * R3B ,
				 Configuration * r3bc = nullptr ,
                                 Configuration * solc = nullptr ) {
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

 virtual void map_forward_solution( Block * R3B ,
                                    Configuration * r3bc = nullptr ,
                                    Configuration * solc = nullptr ) {
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
  * to produce R3B in the first place). mod is assumed to be a (const)
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
  * having any workable one to implement.
  *
  *     IMPORTANT NOTE: MAPPING SOME Modification MAY ONLY BE POSSIBLE IF
  *     PERFORMED "IMMEDIATELY" AFTER THE ORIGINAL Block HAS BEEN MODIFIED
  *
  * A case in point is if the Block has some dynamic Constraint or Variable.
  * These can be changed, deleted and added. This kind of change may be very
  * hard to manage if one lets Modification accumulate. For instance, a
  * dynamic Constraint may first be modified, and then deleted. If the
  * map_forward_Modification() corresponding to changing the constraint is
  * called *after* that the Constraint has been deleted, the original Block
  * no longer has any information on the Constraint, and therefore may have
  * no way to properly implement the change. Each :Block should clearly 
  * state if it supports deferred map_forward_Modification() for all the
  * Modification it produces, or which Modification need be mapped
  * "immediately".
  *
  * The method is given a default implementation returning false if \p mod
  * refers to this; otherwise it seeks \p mod in the list of inner Block
  * of this, and if it finds it calls map_forward_Modification() of the
  * inner Block and the corresponding inner Block of R3B (if any, otherwise
  * false is returned).
  *
  * If \p *r3bc is a SimpleConfiguration< std::vector< Configuration * >,
  * then the i-th element of the vector is passed as the r3bc parameter
  * in the call to map_forward_Modification() to the i-the inner Block
  * (otherwise, or if the vector has no i-th element, nullptr is used). */

 virtual bool map_forward_Modification( Block * R3B , c_p_Mod mod ,
                                        Configuration * r3bc = nullptr ,
                                        ModParam issuePMod = eNoBlck ,
                                        ModParam issueAMod = eModBlck ) {
  if( mod->get_Block() == this )
   return( false );

  auto i = get_nested_Block_index( mod->get_Block() );
  if( ( i >= get_number_nested_Blocks() ) ||
      ( i >= R3B->get_number_nested_Blocks() ) )
   return( false );

  auto cv =
   dynamic_cast< SimpleConfiguration< std::vector< Configuration * > > *
    >( r3bc );

  return( mod->get_Block()->map_forward_Modification(
		                      R3B->get_nested_Block( i ) , mod ,
		                      ( cv && ( cv->f_value.size() > i ) ) ?
				                cv->f_value[ i ] : nullptr ,
				      issuePMod , issueAMod ) );
  }

/*--------------------------------------------------------------------------*/
 /// maps forward a list of Modification from the original Block to a R3 Block
 /** Maps forward an entire  list of Modification from the original Block to
  * a R3 Block in one blow.
  *
  * As discussed in map_forward_Modification(), mapping Modifications is in
  * general complex. This is so because a Block and its R3 Block can be "very
  * different", and therefore one Modification in one can result in many
  * Modification in the other and vice-versa. While GroupModification may
  * help in this respect, it may not solve everything. Furthermore, even if
  * there is a nice one-to-one correspondence, mapping a Modification that
  * can have been issued "a long time ago", with many changes having possibly
  * occurred in Block in the meantime, can already be a daunting task if the
  * Block has any form of dynamic components that may be freely created and
  * destroyed.
  *
  * A part of the complexity, in both cases, is therefore due to the myopic
  * definition of the task: mapping *one* Modification. In general, it may
  * be simpler to map *all the issued Modification*. Knowing the whole list
  * allows, for instance, to traverse if backwards, and therefore be able to
  * completely map the current status of the Block (after all the changes)
  * to the current status of the R3 Block (before any change). Althiugh
  * "simpler" may not be the most appropriate adjective there, knowing the
  * full list of Modification may make it possible to deal with tasks that
  * would otherwise not be feasible.
  *
  * This method allows for such an operation. It takes the same inputs as
  * map_forward_Modification(), save that the individual (const pointer to a)
  * Modification is now a list of (smart pointerd to) Modification. The
  * assumption is that the list contains "all the Modification issued that
  * cause the Block to be not in synch with its R3 Block", although individual
  * :Block may relax this to a subset of "difficult" Modification.
  *
  * The method is supposed to scan through the list of Modification,
  * properly reacting to those it can deal with and ignoring the others; the
  * Modification acted upon are deleted from the list, leaving the rest.
  *
  * The method is given a default implementation just dumbly scanning the
  * list top-to-bottom and calling map_forward_Modification() on each
  * element in turn. */

 virtual void map_forward_Modifications( Block * R3B , Lst_sp_Mod lmod ,
                                         Configuration * r3bc = nullptr ,
                                         ModParam issuePMod = eNoBlck ,
                                         ModParam issueAMod = eModBlck ) {
  for( auto mit = lmod.begin() ; mit != lmod.end() ; ) {
   auto nmit = ++mit;
   if( map_forward_Modification( R3B , mit->get() , r3bc ,
				 issuePMod , issueAMod ) )
    lmod.erase( mit );
   mit = nmit;
   }
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
  * to produce R3B in the first place). mod is assumed to be a (const)
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
  * allows to check that this has happened.
  *
  * The method is given a default implementation returning false if \p mod
  * refers to this; otherwise it seeks \p mod in the list of inner Block
  * of this, and if it finds it calls map_back_Modification() of the inner
  * Block and the corresponding inner Block of R3B (if any, otherwise false
  * is returned).
  *
  * If \p *r3bc is a SimpleConfiguration< std::vector< Configuration * >,
  * then the i-th element of the vector is passed as the r3bc parameter
  * in the call to map_back_Modification() to the i-the inner Block
  * (otherwise, or if the vector has no i-th element, nullptr is used). */

 virtual bool map_back_Modification( Block * R3B , c_p_Mod mod ,
                                     Configuration * r3bc = nullptr ,
                                     ModParam issuePMod = eNoBlck ,
                                     ModParam issueAMod = eModBlck ) {
  if( mod->get_Block() == this )
   return( false );

  auto i = get_nested_Block_index( mod->get_Block() );
  if( ( i >= get_number_nested_Blocks() ) ||
      ( i >= R3B->get_number_nested_Blocks() ) )
   return( false );

  auto cv =
   dynamic_cast< SimpleConfiguration< std::vector< Configuration * > > *
    >( r3bc );

  return( mod->get_Block()->map_back_Modification(
                                      R3B->get_nested_Block( i ) , mod ,
                                      ( cv && ( cv->f_value.size() > i ) ) ?
				                cv->f_value[ i ] : nullptr ,
				      issuePMod , issueAMod ) );
  }

/*--------------------------------------------------------------------------*/
 /// maps back a list of Modification from an R3 Block to the original Block
 /** Maps back an entire list of Modification from an R3 Block to the original
  * Block in one blow.
  *
  * See the comments to map_forward_Modifications(). */

 virtual void map_back_Modifications( Block * R3B , Lst_sp_Mod lmod ,
                                      Configuration * r3bc = nullptr ,
                                      ModParam issuePMod = eNoBlck ,
                                      ModParam issueAMod = eModBlck ) {
  for( auto mit = lmod.begin() ; mit != lmod.end() ; ) {
   auto nmit = ++mit;
   if( map_back_Modification( R3B , mit->get() , r3bc ,
                              issuePMod , issueAMod ) )
    lmod.erase( mit );
   mit = nmit;
   }
  }

/** @} ---------------------------------------------------------------------*/
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

 virtual Solution * get_Solution( Configuration * solc = nullptr ,
                                  bool emptys = true ) {
  return( nullptr );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// returns the Block that this Observer is
 /** A Block is an Observer, so the Block that this Observer belongs to is
  * itself. However, note that const-ness has to be casted away from "this",
  * which is const in a const method. */

 Block * get_Block() const override {
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
 /** This method has to be called by the father Block to inform each of his
  * sons whether or not there is someone "listening to him", and therefore
  * to them. */

 void anyone_there( bool isthere );

/*--------------------------------------------------------------------------*/
 /// notify the Block about a Modification
 /** Block::add_Modification() implements the main mechanics of Modification
  * handling; in particular:
  *
  * - if chnl != 0 is one of the channels defined by this very Block, it
  *   "packs" the Modification into the appropriate GroupModification and does
  *   nothing else, which in particular means that it does *not* dispatch it
  *   to its father and the attached Solver (this being done by close_channel);
  *
  * - if chnl == 0 or it is not one of the channels defined by this very Block,
  *   it rather immediately dispatches the Modification to its father (on the
  *   very same channel) and the attached Solver.
  *
  * Note that
  *
  *     IT IS AN ERROR IF chnl != 0 IS NOT ONE OF THE CHANNELS DEFINED IN THIS
  *     VERY Block AND THE Block HAS NO FATHER, BECAUSE IT MEANS THE
  *     Modification HAS NOWHERE TO GO TO; MORE IN GENERAL, IT IS AN ERROR TO
  *     SEND A Modification TO A CHANNEL THAT IS NOT DEFINED IN SOME ANCESTOR
  *     OF THE Block. 
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
  *     IT IS BEING SENT TO SOME NON-0 CHANNEL DEFINED IN THE Block
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

 ChnlName open_channel( ChnlName chnl = 0 ,
			GroupModification * gmpmod = nullptr ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void close_channel( ChnlName chnl , bool force = false ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 void set_default_channel( ChnlName chnl = 0 ) override { f_channel = chnl; }

/** @} ---------------------------------------------------------------------*/
/*---------------------- Methods for handling Solver -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Solvers
 *
 * The following methods are provided for changing the set of Solver
 * registered with the Block.
 *
 * Note that these methods only refer to the "main" Block, i.e., they do not
 * provide any way to manage the Solver registered with the sub-Block. This
 * can of course be done programmatically by visiting the whole tree
 * structure of the Block and calling the appropriate methods whenever
 * appropriate. Furthermore, specific components are separately provided for
 * easing this task (see BlockSolverConfig.h).
 * @{ */

 /// reading the list of (pointers to) currently registered Solvers
 /** Method for reading the list of (pointers to) the Solvers currently
  * registered with the Block. */

 c_Lst_Solver & get_registered_solvers( void ) const { return( v_Solver ); }

/*--------------------------------------------------------------------------*/
 /// adding a Solver to the set of those currently registered
 /** Method for adding a Solver to the set of those currently registered
  * with the Block. Note that the Block does sets itself to the Solver by
  * calling Solver::set_Block(), which is why the converse is not done (see
  * Solver.h). Note that the method is virtual because derived classes may
  * have to do more.
  *
  * By default the new Solver (pointer) is pushed to the back of the list of
  * registered Solver, unless \p tofront is set to true, in which case it is
  * pushed to the front. */

 virtual void register_Solver( Solver * newSolver , bool tofront = false ) {
  if( ! newSolver )
   throw( std::invalid_argument( "registering nullptr Solver" ) );

  if( v_Solver.empty() ) {    // this is the first Solver listening to me
   if( ! f_at ) {             // and no one was listening from above already
    for( auto el : v_Block )  // now someone is listening to all my sons
     el->anyone_there( true );
    }
   }
  else {                      // there are other Solver listening to me
   auto it = find( v_Solver.begin(), v_Solver.end(), newSolver );
   if( it != v_Solver.end() )  // the Solver is already there
    return;                    // silently return
   }

  newSolver->set_Block( this );
  if( tofront )
   v_Solver.push_front( newSolver );
  else
   v_Solver.push_back( newSolver );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removing all Solver from the set of those currently registered
 /** Clear all the list of registered Solver in one blow. If \p deleteold is
  * true, then the Solver are also deleted. */

 virtual void unregister_Solvers( bool deleteold = true ) {
  if( v_Solver.empty() )  // no registered Solver
   return;                // nothing to do

  if( deleteold )
   for( auto slvr : v_Solver ) {
    slvr->set_Block( nullptr );
    delete slvr;
   }
  else
   for( auto slvr : v_Solver )
    slvr->set_Block( nullptr );

  v_Solver.clear();

  if( ! f_at )                 // nobody is listening from above
   for( auto el : v_Block )    // now no one is listening to all my sons
    el->anyone_there( false );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removing oldSolver from the set of those currently registered
 /** Method for removing a Solver from the set of those currently registered
  * with the Block. If oldSolver is not among the registered solvers, then
  * nothing is done (and no warning is issued); otherwise the vector of
  * registered Solver is shortened by one, and the remaining solvers (if any)
  * are shifted in the obvious way. If \p deleteold is true, then the 
  * Solver is also deleted. Note that the Block calls
  * Solver::set_Block( nullptr ) to the Solver that is un-registered, which is
  * why the converse is not done (see Solver.h). Note that the method is
  * virtual because derived classes may have to do more. */

 virtual void unregister_Solver( Solver * oldSolver ,
                                 bool deleteold = false ) {
  auto it = find( v_Solver.begin() , v_Solver.end() , oldSolver );
  if( it == v_Solver.end() )  // the Solver is not there
   return;                    // silently return

  oldSolver->set_Block( nullptr );
  if( deleteold )
   delete oldSolver;
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
  * inside. If \p deleteold is true, then the Solver is also deleted. Note
  * that the Block calls Solver::set_Block( nullptr ) to the Solver that is
  * un-registered, which is why the converse is not done (see Solver.h).
  * Also, note that the method is virtual because derived classes may have
  * to do more.
  *
  * Warning: checking if an iterator really belongs to a list is complicated,
  * hence the method does not try to do that; clearly, calling the method
  * with something that is not an iterator of that list results in indefinite
  * behaviour. */

 virtual void unregister_Solver( c_Lst_Solver_it it ,
                                 bool deleteold = false ) {
  // cast away const-ness and call the protected version
  unregister_Solver( v_Solver.erase( it , it ) , deleteold );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// replace an old Solver with a new Solver
 /** Method for substituting the Solver at the position "it" into the vector
  * returned by get_registered_solvers() with the given new Solver. Note that
  * the iterator parameter is const because the only place where it might
  * have been taken from (except if the method is called from another Block
  * method) is get_registered_solvers(); however, we "cast away const-ness"
  * inside. If \p deleteold is true, then the replaced Solver is also deleted.
  * Note that the Block calls Solver::set_Block( nullptr ) to the replaced
  * Solver, which is why the converse is not done (see Solver.h). Note that
  * the method is virtual because derived classes may have to do more.
  *
  * Warning: checking if an iterator really belongs to a list is complicated,
  * hence the method does not try to do that; clearly, calling the method
  * with something that is not an iterator of that list results in indefinite
  * behaviour. */

 virtual void replace_Solver( Solver * newSolver ,
                              c_Lst_Solver_it it , bool deleteold = false ) {
  if( ! newSolver )
   throw( std::invalid_argument( "registering nullptr Solver not allowed" ) );

  // cast away const-ness and call the protected version
  replace_Solver( newSolver, v_Solver.erase( it, it ), deleteold );
 }

/** @} ---------------------------------------------------------------------*/
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
 *      MEMBER FUNCTIONS IF THE METHODS FACTORY HAS TO BE EMPLOYED WITHOUT
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
 *           void ( dBlock::* ) ( Args ... , ModParam , ModParam );
 *
 *     template< typename ... Args >
 *     using FunctionType =
 *      std::function< void ( Block * , Args ... , ModParam , ModParam ) >;
 *
 * are defined. MemberFunctionType defines the interface of a generic class
 * member function that, besides any set of arguments, has two final ones
 * concerning if and how "physical" and "abstract" Modification are issued by
 * the (possible) change in the :Block brought about by the function.
 * FunctionType< Args > is the interface of an adapter function that may
 * correspond to MemberFunctionType< dBlock , Args >. This is what is
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
 *     my_method_name( [ < data > , ] < slice > , ModParam , ModParam )
 *
 * where:
 *
 * - data is either not there, or a MF_dbl_it, or a MF_int_it;
 *
 * - slice indicates a subset of the data, in two possible forms:
 *
 *   = (range) a Range object (passed by value), yielding the function type
 *
 *         my_method_name( [ < data > , ] Range , ModParam , ModParam )
 *
 *   = (subset) an arbitrary subset of the data, yielding the function type
 *
 *         my_method_name( [ < data > , ] Subset && , bool ,
 *                         ModParam , ModParam )
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
 *     void set_arc_weight( MF_dbl_it , Range , ModParam , ModParam );
 *
 *     void set_arc_weight( MF_dbl_it , Subset && , bool ,
 *                          ModParam , ModParam );
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
 *                          ModParam issuePMod = eNoBlck ,
 *                          ModParam issueAMod = eModBlck );
 *
 * A user who would like to have a function in the methods factory for setting
 * the weight of subsets of arcs could implement the following functions:
 *
 *     void set_weight_range( Block * block , MF_dbl_it begin ,
 *                            Range range , ModParam issuePMod = eNoBlc ,
 *                                          ModParam issueAMod = eModBlck ) {
 *      for( Index i = range.first ; i < range.second ; ++i , ++begin )
 *       static_cast< NetworkBlock * >( block )->set_arc_weight( i , *begin ,
 *                                                               issuePMod ,
 *                                                               issueAMod );
 *      }
 *
 *     void set_weight_subset( Block * block , MF_dbl_it begin ,
 *                             Subset && sbst , bool ordered = false ,
 *                             ModParam issuePMod = eNoBlc ,
 *                             ModParam issueAMod = eModBlck ) {
 *      for( auto i : sbst )
 *       static_cast< NetworkBlock * >( block )->set_arc_weight( i ,
 *                                                               *(begin++) ,
 *                                                               issuePMod ,
 *                                                               issueAMod );
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
 static void register_method( std::string && name, F * function ) {
  if( name.empty() )
   throw( std::invalid_argument( "register_method: name is empty" ) );
  auto iter = methods< F >().left.find( name );
  if( iter != methods< F >().left.end() ) {
   delete iter->second;
   auto replaced = methods< F >().left.replace_data( iter, function );
   assert( replaced );
  } else
   methods< F >().insert( typename bimap< F >::value_type( std::move( name ),
                                                           function ) );
 }

/*--------------------------------------------------------------------------*/
 /// register a new ("class member") function in the methods factory
 /** This template function registers a class member function in the
  * appropriate methods factory, and associates it with the given \p name. It
  * has a template parameter and a variadic parameter pack. The template
  * parameter \p dBlock is the class (derived from Block) of which the
  * function is a member. The parameter pack \p Args specifies the parameter
  * type list (actually, part of it, without considering the two ModParam
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
 static std::enable_if_t< std::is_base_of_v< Block , dBlock > , void >
 register_method( std::string && name ,
                  MemberFunctionType< dBlock , Args... > fnct ) {
  register_method( std::move( name ),
                   new FunctionType< Args... >(
                    [ fnct ]( Block * blck , Args && ... args ,
                              ModParam issuePMod , ModParam issueAMod ) {
                     std::invoke( fnct,
                                  static_cast< dBlock * >( blck ),
                                  std::forward< Args >( args )...,
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
  * which is dummy arg_packer_helper< Args... >. This is intended to be used
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
  * @param void Dummy arg_packer_helper< Args... > parameter to specify the
  *             parameter type list of the function to be registered. */

 template< class dBlock , typename ... Args >
 static void register_method( std::string && name ,
                              MemberFunctionType< dBlock , Args... > fnct ,
                              arg_packer_helper< Args... > ) {
  register_method< dBlock, Args... >( std::move( name ) , fnct );
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
  * two ModParam parameters. This has been inserted in the interface under
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
  * two ModParam parameters. This has been inserted in the interface under
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
 static const FunctionType< Args... > *
 get_method_fs( const std::string & name ) {
  return( get_method< FunctionType< Args... > >( name ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the function with the given name in the methods factory
 /** This function returns a pointer to the adapter function associated with
  * the given \p name in the methods factory implied by the second dummy
  * parameter.
  *
  * Suppose, for example, that the methods factory has a function associated
  * with the name "NetworkBlock::set_arc_weight" that has the typical "double,
  * Range" interface, i.e., a #MF_dbl_it parameter, a #Range parameter, and
  * two ModParam parameters. This has been inserted in the interface under
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
  * @param void Dummy arg_packer_helper< Args... > parameter to specify
  *             parameter type list of the function to be retrieved. */

 template< typename... Args >
 static const FunctionType< Args... > *
 get_method_fs( const std::string & name , arg_packer_helper< Args... > ) {
  return( get_method< FunctionType< Args... > >( name ) );
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
 static const std::string & get_method_name( const F * fnct ) {
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
				     const FunctionType< Args... > * fnct ) {
  return( get_method_name< FunctionType< Args... > >( fnct ) );
  }

/*--------------------------------------------------------------------------*/
 /// returns the name that is associated with the given function
 /** This template function returns (a reference to) the name that is
  * associated with the given (pointer to a) function in the methods factory
  * implied by the second dummy parameter.
  *
  * @param fnct A pointer to the function whose associated name is desired.
  *
  * @param void Dummy arg_packer_helper< Args... > parameter to specify the
  *             parameter type list of the function whose associated name is
  *             desired. */

 template< typename... Args >
 static const std::string & get_method_name_fs(
				       const FunctionType< Args... > * fnct ,
				       arg_packer_helper< Args... > ) {
  return( get_method_name< FunctionType< Args... > >( fnct ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Block ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the Block
 *
 * The base Block class provides two pairs of symmetric print() / load() and
 * serialize() / deserialize() methods to save information about it on a
 * std::stream / netCDF::NcGroup and retrieve it. For print() the saved
 * information may or may not be enough to fully reconstruct the original
 * Block via load(), depending on the verbosity level, while for serialize()
 * the saved information is always enough to fully reconstruct the original
 * Block via deserialize().
 *
 * print() and load() have two forms, one taking a filename and the other
 * taking a [o/i]stream &. In principle the first form just opens the
 * appropriate [i/o]fstream and dispatches to the second, but different
 * implementations may be provided for the case where the input/output
 * format is a multi-file one and therefore requires the reading/writing of
 * multiple fies (typically with the same initial part of the filename and
 * different suffixes). Both print() and load() support the generic notion
 * that input/output may happen in different formats, governed by a simple
 * char parameter.
 *
 * Block also provides a friend operator<<() and two friend operator>>() (one
 * for references and one for pointers) dispatching to print() and load(),
 * respectively (with "default verbosity"=; the idea is that derived classes
 * will implement the latter two in order to provide input and output on
 * std::stream.
 *
 * The interface for serializing and de-serializing a :Block onto netCDF
 * files is a bit more complex in that the base Block class provides some
 * means to automate part of the process: besides like opening the netCDF
 * file, possibly with different formats, also finding the right
 * netCDF::NcGroup inside it. This is done via the three versions of
 * serialize() taking, respectively, a file name (std::string), a
 * netCDF::NcFile and a netCDF::NcGroup. The first dispatches to the second,
 * and the latter ultimately to the third, which is where the true
 * :Block-dependent serialization is supposed to happen.
 * @{ */

 /// print information about the Block on a file, given its name
 /** Method intended to print information about the Block on the one (or
  * more) text file(s) with the filename given in \p fname (prefixed as set
  * by set_filename_prefix(), if any), replacing any current content if the
  * file already exists.
  *
  * The parameter \p vlvl is assumed to control the "level of the verbosity"
  * of the printed information, i.e., the format of the output file; see
  * print( ostream & ) for comments. In fact, the implementation of this
  * method in the base Block class just opens an ofstream using the given
  * filename and then calls the print( ostream & ) version, which is where
  * the true :Block-specific printing is supposed to mostly happen.
  *
  * However, this method is virtual and there is a clear scenario in which
  * derived classes may want to override it: that of multi-file formats.
  * That is, a :Block may want to save itself on a number of different
  * files, typically with names of the form [global prefix] fname [suffix]
  * for different choices of [suffix], possibly depending on the value of
  * \p vlvl. Clearly, the print( ostream & ) version cannot be used in this
  * case, and a proper override of this method is required instead. */

 virtual void print( const std::string & fname , char vlvl = 0 ) const {
  std::ofstream f( f_prefix + fname , std::ofstream::trunc );
  if( f.is_open() )
   print( f , vlvl );
  else
   std::cerr << "Error: cannot open text file " << f_prefix + fname
	     << std::endl;
  }

/*--------------------------------------------------------------------------*/
 /// print information about the Block on an ostream with the given verbosity
 /** Method intended to print information about the Block on the given
  * std::ostream; it is virtual so that derived classes can print their
  * specific information in the format they choose, although the base class
  * also provides a rudimentary implementation printing a modicum of data
  * about the "abstract representation".
  *
  * The parameter \p vlvl is assumed to control the "level of the verbosity"
  * of the printed information. The format of the parameter should depend
  * on the derived :Block, but the base Block class sets the standard that
  * 0 means "minimal information" and 'C' means "complete information", i.e.,
  * enough information to allow a Block to completely re-read itself back
  * from the stream with load(). Of course the base Block class cannot
  * support the 'C' case, but it will interpret anything except 0 and 'C' as
  * "recursively print the nested Block with the same verbosity".
  *
  * In general, a :Block may have more than one "complete" output formats,
  * hence other values apart from 'C' may still provide enough information
  * for load() to be able to reconstruct the Block; yet, one format can be
  * designed as "standard" and get the 'C' value. It is expected that
  * values of \p vlvl here denoting "complete" formats will correspond to
  * values of the \p frmt parameter in load() denoting the same format.
  *
  * In case a "complete" format requires more than one file, the
  * corresponding implementation has to be provided by the
  * print( std::string & ) version of the method, ideally with specific 
  * values of \p vlvl (not handled by this version). It might be possible
  * to read back such multi-file outputs using load( std::string & ),
  * possibly with the corresponding value of the \p frmt parameter, but the
  * set of supported input/output formats is strictly dependent on the
  * specific :Block. */

 virtual void print( std::ostream & output , char vlvl = 0 ) const;

/*--------------------------------------------------------------------------*/
 /// load the Block out of a text file with given filename
 /** Method intended to provide support for Blocks to load themselves out of
  * one (or more) file(s) with the the filename given in \p fname (prefixed
  * as set by set_filename_prefix(), if any).
  *
  * The parameter \p frmt is provided to support the notion that the input
  * file(s) may have different formats; see load( istream & ) for comments.
  * In fact, the implementation of this method in the base Block class just
  * opens an ifstream using the given filename and then calls the
  * load( istream & ) version, which is where the true :Block-specific
  * loading is supposed to mostly happen.
  *
  * However, this method is virtual and there is a clear scenario in which
  * derived classes may want to override it: that of multi-file formats.
  * That is, a :Block may want to load itself out of a number of different
  * files, typically with names of the form [global prefix] fname [suffix]
  * for different choices of [suffix], possibly depending on the value of
  * \p frmt. Clearly, the load( istream & ) version cannot be used in this
  * case, and a proper override of this method is required instead.
  *
  * As the load( istream & ) version, this method will issue a
  * NBModification to alert all "interested" Solver (if any) that the
  * :Block has "completely changed", see the comments to load( istream & )
  * for details. */

 virtual void load( const std::string & input , char frmt = 0 ) {
  std::ifstream f( f_prefix + input , std::fstream::in );
  if( f.is_open() )
   load( f , frmt );
  else
   std::cerr << "Error: cannot open text file " << f_prefix + input
	     << std::endl;
  }

/*--------------------------------------------------------------------------*/
 /// load the Block out of an istream
 /** *pure virtual* method intended to provide support for Blocks to load
  * themselves out of an istream. This is precisely what makes Block an
  * *abstract* base class: the actual content of the Block depends on the
  * specific derived class, which is why this method cannot be implemented
  * (although the same in principle holds for deserialize(), Block provides a
  * basic implementation for that method that may be useful to derived
  * classes).
  *
  * The parameter \p frmt is provided to support the notion that the input
  * istream may have different formats. These should ideally be the same
  * formats that print() produces with the same values of its \p vlvl
  * parameter, but the set of supported input/output formats is strictly
  * dependent on the specific :Block. Also, note that print() should
  * reasonably have "more formats" than load() because it should reasonably
  * support "non complete" output formats, useful, e.g., for debugging but
  * not for save/load operations, besides "complete" ones.
  *
  * In case a "complete" format requires more than one file, the
  * corresponding implementation cannot be handled here and has rather to be
  * provided by the load( std::string & ) version of the method, ideally
  * with specific values of \p frmt (not handled by this version)
  * corresponding to values of \p vlvl in print( std::string & ).
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
  * (unless the :Block does this by its own volition), the BlockConfig is not
  * set, and there are no Solver attached, unless there were before. */

 virtual void load( std::istream & input , char frmt = 0 ) = 0;

/*--------------------------------------------------------------------------*/
 /// friend operator<<(), dispatching to virtual print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the virtual method print() (with default "verbosity level",
  * i.e., low). This way the operator<<() is defined for each Block, but its
  * behavior can be customized by derived classes. */

 friend std::ostream & operator<<( std::ostream & out , const Block & b ) {
  b.print( out );
  return( out );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator>>(), dispatching to *pure* virtual load()
 /** Not really a method, but a friend operator>>() that just calls the
   **pure* virtual method load(). This way the operator>>() is defined for
   * each Block, but it won't work for the base class, which is abstract: it
   * can only work for concrete derived classes which have actually
   * implemented load() (because they have some actual data to load). */

 friend std::istream & operator>>( std::istream & in , Block & b ) {
  b.load( in );
  return( in );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator>>() for pointers
 /** Not really a method, but a friend operator>>() that loads a new :Block
  * and stores its pointer; this is basically calling Block::deserialize(
  * stream ) to load the string classname, use the factory to build the
  * object, and then use the standard operator>>() to finish loading. For
  * obvious reasons it cannot take care of the father, which is therefore
  * always nullpr. It would not even really need to be a friend. */

 friend std::istream & operator>>( std::istream & in , Block * &c ) {
  c = Block::deserialize( in );
  return( in );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Block (recursively) to a netCDF file given the filename
 /** Top-level method to serialize a Block (recursively) to a file in
  * netCDF-based SMS++-format, given the filename and its type. See
  * deserialize( netCDF::NcFile & ) for details of the different file types.
  * Note that any existing content of the file is overwritten, and that the
  * Block is saved as *the first one* in the newly created file.
  *
  * The base class implementation opens the netCDF file, creates the required
  * attribute "SMS++_file_type", assigns it the type, and dispatches to the
  * netCDF::NcFile & version of the method. If anything goes wrong with any
  * step of the process, exception is thrown. Although the method is virtual,
  * it is not expected that derived classes will have a need to re-define it.
  */

 virtual void serialize( const std::string & filename ,
			 int type = eProbFile ) const
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

 virtual void serialize( netCDF::NcFile & f , int type ) const
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
  *   is strictly required. As a consequence, no configuration of the Block
  *   (the BlockConfig) and no information about the registered solver is
  *   saved when the Block is serialized. The rationale is that
  *   Configuration objects can themselves be serialized, so saving this
  *   information it better done separately (see BlockSolverConfig.h and
  *   RBlockConfig.h).
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

 virtual void serialize( netCDF::NcGroup & group ) const {
  group.putAtt( "type", classname() );
  if( ! f_name.empty() )
   group.putAtt( "name", f_name );
   }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function< Block *( Block * ) > BlockFactory;
 // type of the factory of Block

 typedef std::map< std::string, BlockFactory > BlockFactoryMap;
 // Type of the map between strings and the factory of Block

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for locking and unlocking
 *  @{ */

 bool lock_sub_block( const void * owner ) {
  if( v_Block.empty() )
   return( true );

  // visit the Block tree depth-first, left-to-right order
  // note that we need to keep track if a sub-Block was already owned by
  // owner in order to avoid unlocking it in case of failure

  bool success = true;
  std::vector< bool > locked( v_Block.size() , true );

  auto sb = v_Block.begin();
  auto oit = locked.begin();
  for( ; sb != v_Block.end() ; ++sb , ++oit )
   if( ( *sb )->is_owned_by( owner ) )
    *oit = false;
   else
    if( !( *sb )->lock( owner ) ) {
     success = false;
     break;
     }

  if( ! success )   // some of the sub-Block could not be locked
   while( sb-- != v_Block.begin() )  // re-unlock those that were locked
    if( *( --oit ) )                   // except those were owned already
     ( *sb )->unlock( owner );

  return( success );
  }

/*--------------------------------------------------------------------------*/
/* Try to read-lock all sub_Block of a Block that has already been
 * successfully read-locked; upon success return true, upon failure
 * read-unlock the Block and return false. */

 bool read_lock_sub_block( void ) {
  bool success = true;
  // visit the Block tree depth-first, left-to-right order
  auto sb = v_Block.begin();
  for( ; sb != v_Block.end(); ++sb )
   if( !( *sb )->read_lock() ) {
    success = false;
    break;
    }

  if( success )     // all the sub-Block could be read-locked
   return( true );  // return success

  while( sb-- != v_Block.begin() )  // re-read-unlock the read-locked ones
   ( *sb )->read_unlock();

  guts_of_read_unlock();  // read-unlock the Block

  return( false );        // return failure
  }

/*--------------------------------------------------------------------------*/
 /// define a special owner for "locked read-only"
 /** This address is not supposed to be used as that of any owner, and it
  * is used to mean "the Block is under any number of read-locks". */

 const void * ReadOnlyLock() const { return( & f_owner ); }

/*--------------------------------------------------------------------------*/
 /// define a special owner for "locked read-only and working on v_owners"
 /** This address is not supposed to be used as that of any owner, and it
  * is used to mean "the Block is under any number of read-locks, and one of
  * the read-locks is currently operating on the v_owners field, which means
  * that nobody else should be even looking at it". */

 const void * v_ownersLock() const { return( & f_mutex ); }

/*--------------------------------------------------------------------------*/
/* Read-lock the Block (which must be read-locked). */

 void guts_of_read_unlock( void ) {
  for( ; ; ) {  // acquire the "active lock" on v_owners
   // note that since the Block is surely read-locked, the only possible
   // contents of f_owner can be v_ownersLock and ReadOnlyLock
   const void * current_owner = ReadOnlyLock();
   if( f_owner.compare_exchange_strong( current_owner, v_ownersLock() ) ) {
    // v_owner was active-unlocked, active lock successfully acquired
    // find the record corresponding to the thread::id, it must be there
    auto it = v_owners.find( std::this_thread::get_id() );
    if( it == v_owners.end() )
     throw( std::logic_error( "unbalanced read lock/unlock" ) );
    if( ! --( it->second ) ) {
     // decrease the counter, and if it reached 0 erase the record
     v_owners.erase( it );
     // if there are no longer readers for the Block
     if( v_owners.empty() ) {
      f_owner = nullptr;  // entirely release the lock
      f_mutex.unlock();   // release the mutex
      break;
      }
     }
    // release the active lock on v_owners
    f_owner = ReadOnlyLock();
    break;
    }
   // else, v_owner was active-locked: repeat until access granted
   }  // end( for( ever ) )
  }  // end( guts_of_read_unlock )

/** @} ---------------------------------------------------------------------*/
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
 *
 * Similar methods are provided to handle the set of sub-Block. Although
 * currently v_Block is protected and derived classes can manipulate it
 * freely, this may change in the future, and therefore the use of these
 * methods is strongly advised for better future-proof code.
 * @{ */

 /// removes any existing sub-Block; to be used with care

 void reset_nested_Block() {
  for( auto bi : v_Block )
   delete bi;
  v_Block.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add a single sub-Block

 void add_nested_Block( Block * newb , bool front = false ) {
  if( newb->get_f_Block() != this )
   newb->set_f_Block( this );

  if( front )
   v_Block.insert( v_Block.begin(), newb );
  else
   v_Block.push_back( newb );
  }

/*--------------------------------------------------------------------------*/
 /// removes any existing static Constraint; to be used with care

 void reset_static_constraints( void ) {
  v_s_Constraint.clear();
  v_s_Constraint_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing static Variable; to be used with care

 void reset_static_variables( void ) {
  v_s_Variable.clear();
  v_s_Variable_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing dynamic Constraint; to be used with care

 void reset_dynamic_constraints( void ) {
  v_d_Constraint.clear();
  v_d_Constraint_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing dynamic Variable; to be used with care

 void reset_dynamic_variables( void ) {
  v_d_Variable.clear();
  v_d_Variable_names.clear();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// removes any existing objective; to be used with care

 void reset_objective( void ) { f_Objective = nullptr; }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_static_constraint( std::string && name = "" ,
                             bool front = false ) {
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin(), boost::any() );
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_static_constraint( Const & newc , std::string && name = "" ,
                        bool front = false ) {
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_static_constraint( Index i , Const & newc ,
                        std::string && name = "" ) {
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_static_constraint( std::vector< Const > & newc ,
                        std::string && name = "" , bool front = false ) {
  for( auto & c : newc )
   c.set_Block( this );

  std::vector< Const > * cnewc = &newc;
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin() , cnewc );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin(),
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_static_constraint( Index i , std::vector< Const > & newc ,
                        std::string && name = "" ) {
  if( i >= v_s_Constraint.size() )
   throw( std::invalid_argument( "wrong index into v_s_Constraint" ) );

  for( auto & c : newc )
   c.set_Block( this );

  std::vector< Const > * cnewc = &newc;
  v_s_Constraint[ i ] = cnewc;
  v_s_Constraint_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array< K > of (...) Constraint

 template< class Const , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_static_constraint( boost::multi_array< Const , K > & newc ,
                        std::string && name = "" , bool front = false ) {
  for( auto i = newc.data() ; i < ( newc.data() + newc.num_elements() ) ; )
   (i++)->set_Block( this );

  boost::multi_array< Const, K > * cnewc = &newc;
  if( front ) {
   v_s_Constraint.insert( v_s_Constraint.begin(), cnewc );
   v_s_Constraint_names.insert( v_s_Constraint_names.begin() ,
                                std::move( name ) );
   }
  else {
   v_s_Constraint.push_back( cnewc );
   v_s_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array< K > of (...) Constraint

 template< class Const , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_static_constraint( Index i , boost::multi_array< Const , K > & newc ,
                        std::string && name = "" ) {
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

 void add_static_variable( std::string && name = "" , bool front = false ) {
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin() , boost::any() );
   v_s_Variable_names.insert( v_s_Variable_names.begin(),
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
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 add_static_variable( Var & newv , std::string && name = "" ,
                      bool front = false ) {
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
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 set_static_variable( Index i , Var & newv , std::string && name = "" ) {
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
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 add_static_variable( std::vector< Var > & newv ,
                      std::string && name = "" , bool front = false ) {
  for( auto & v : newv )
   v.set_Block( this );

  std::vector< Var > * cnewv = &newv;
  if( front ) {
   v_s_Variable.insert( v_s_Variable.begin(), cnewv );
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
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 set_static_variable( Index i , std::vector< Var > & newv ,
                      std::string && name = "" ) {
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_s_Variable" ) );

  for( auto & v : newv )
   v.set_Block( this );

  std::vector< Var > * cnewv = &newv;
  v_s_Variable[ i ] = cnewv;
  v_s_Variable_names[ i ] = std::move( name );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array< K > of (...) Variable

 template< class Var , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 add_static_variable( boost::multi_array< Var , K > & newv ,
                      std::string && name = "" , bool front = false ) {
  for( auto i = newv.data() ; i < ( newv.data() + newv.num_elements() ) ; )
   ( i++ )->set_Block( this );

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
 /// boost::multi_array< K > of (...) Variable

 template< class Var , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
 set_static_variable( Index i , boost::multi_array< Var , K > & newv ,
                      std::string && name = "" ) {
  if( i >= v_s_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_s_Variable" ) );

  for( auto & v : newv )
   v.set_Block( this );

  boost::multi_array< Var , K > * cnewv = &newv;
  v_s_Variable[ i ] = cnewv;
  v_s_Variable_names[ i ] = std::move( name );
  }

/*--------------------------------------------------------------------------*/
 /// empty slot

 void add_dynamic_constraint( std::string && name = "", bool front = false ) {
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin(), boost::any() );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin(),
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_dynamic_constraint( std::list< Const > & newc ,
                         std::string && name = "" ,
                         bool front = false ) {
  for( auto & c : newc )
   c.set_Block( this );

  std::list< Const > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin(), cnewc );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin(),
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_dynamic_constraint( Index i , std::list< Const > & newc ,
                         std::string && name = "" ) {
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_dynamic_constraint( std::vector< std::list< Const > > & newc ,
                         std::string && name = "" ,
                         bool front = false ) {
  for( auto & c : newc )
   for( auto & j : c )
    j.set_Block( this );

  std::vector< std::list< Const > > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin(), cnewc );
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
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_dynamic_constraint( Index i ,
                         std::vector< std::list< Const > > & newc ,
                         std::string && name = "" ) {
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
 /// boost::multi_array< K > of std::list of (...) Constraint

 template< class Const , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 add_dynamic_constraint(
  boost::multi_array< std::list< Const > , K > & newc ,
  std::string && name = "" , bool front = false ) {

  for( auto i = newc.data(); i < ( newc.data() + newc.num_elements() ) ;
       ++i )
   for( auto & j : *i )
    j.set_Block( this );

  boost::multi_array< std::list< Const > , K > * cnewc = &newc;
  if( front ) {
   v_d_Constraint.insert( v_d_Constraint.begin() , cnewc );
   v_d_Constraint_names.insert( v_d_Constraint_names.begin(),
                                std::move( name ) );
   }
  else {
   v_d_Constraint.push_back( cnewc );
   v_d_Constraint_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array< K > of std::list of (...) Constraint

 template< class Const , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
 set_dynamic_constraint( Index i ,
                         boost::multi_array< std::list< Const > , K > & newc ,
                         std::string && name = "" ) {
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

 void add_dynamic_variable( std::string && name = "" , bool front = false ) {
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
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 add_dynamic_variable( std::list< Var > & newv ,
                       std::string && name = "" , bool front = false ) {
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
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 set_dynamic_variable( Index i , std::list< Var > & newv ,
                       std::string && name = "" ) {
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
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 add_dynamic_variable( std::vector< std::list< Var > > & newv ,
                       std::string && name = "" , bool front = false ) {
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
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 set_dynamic_variable( Index i ,
                       std::vector< std::list< Var > > & newv ,
                       std::string && name = "" ) {
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
 /// boost::multi_array< K > of std::list of (...) Variable

 template< class Var , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 add_dynamic_variable( boost::multi_array< std::list< Var > , K > & newv ,
                       std::string && name = "" , bool front = false ) {
  for( auto i = newv.data(); i < ( newv.data() + newv.num_elements() ) ; ++i )
   for( auto & j : *i )
    j.set_Block( this );

  boost::multi_array< std::list< Var > , K > * cnewv = &newv;
  if( front ) {
   v_d_Variable.insert( v_d_Variable.begin(), cnewv );
   v_d_Variable_names.insert( v_d_Variable_names.begin() ,
                              std::move( name ) );
   }
  else {
   v_d_Variable.push_back( cnewv );
   v_d_Variable_names.emplace_back( std::move( name ) );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// boost::multi_array< K > of std::list of (...) Variable

 template< class Var , std::size_t K >
 std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
 set_dynamic_variable( Index i ,
                       boost::multi_array< std::list< Var > , K > & newv ,
                       std::string && name = "" ) {
  if( i >= v_d_Variable.size() )
   throw( std::invalid_argument( "wrong index into v_d_Variable" ) );

  for( auto & v : newv )
   for( auto & j : v )
    j.set_Block( this );

  boost::multi_array< std::list< Var >, K > * cnewv = &newv;
  v_d_Variable[ i ] = cnewv;
  v_d_Variable_names[ i ] = std::move( name );
  }

/** @} ---------------------------------------------------------------------*/
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

 virtual void unregister_Solver( Lst_Solver_it it ,
                                 bool deleteold = false ) {
  ( *it )->set_Block( nullptr );  // unregister the Block in the Solver
  if( deleteold )
   delete *it;
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

 virtual void replace_Solver( Solver * newSolver , Lst_Solver_it it ,
                              bool deleteold = false ) {
  ( *it )->set_Block( nullptr );
  if( deleteold )
   delete *it;
  ( *it ) = newSolver;
  newSolver->set_Block( this );
  }

/*--------------------------------------------------------------------------*/
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

 static void static_initialization( void ) {}

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the "owner" of this Block
 /** A void * meant to encode the "unique identity" of the entity having
  * read-write-locked this Block; it contains nullptr if the Block is not
  * currently "owned". Two special values ReadOnlyLock() and v_ownersLock()
  * (that hopefully will never be used as "identity" by any entity trying to
  * lock the Block) are used to encode the fact that the Block is
  * read-locked and therefore it has multiple non-exclusive read-ony owners.
  * The (thread::id) of these owners is contained in v_owners, which is
  * managed under active wait (since only "fast" operations are needed);
  * the value v_ownersLock() says that some thread is currently operating
  * on v_owners and that therefore one should not even look at that until
  * f_owner turns back to ReadOnlyLock(). */
 std::atomic< const void * > f_owner;

 /// a map for counting how many times the Block has been read-locked
 /** v_owners associates with any thread::id of any thread that is
  * currently read_locking the Block the number of outstanding locks. The
  * number is increased with any new read_lock() (from that thread) and
  * decreased with any new read_unlock() (from that thread), so that one
  * knows when a thread has released its last read-lock on the Block. The
  * map is empty if the Block is not read-locked. Note that operations on
  * v_owners (comprised simple reads) have to be performed under active
  * wait controlled by the std::atomic<> f_owner. */
 std::map< std::thread::id , unsigned short > v_owners;

 std::atomic< std::thread::id > f_owner_thread_id;
 ///< the thread::id of the owner

 std::mutex f_mutex;            ///< the std::mutex of this Block

 Vec_Block v_Block;
 ///< vector of pointers of the nested blocks inside the Block

 Lst_Solver v_Solver;
 ///< list of pointers to the registered Solvers with this Block

 bool f_at;  ///< true if there is any Solver "listening" to this Block

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

 BlockConfig * f_BlockConfig;        ///< the BlockConfig for this Block

 std::vector< std::pair< ChnlName , GroupModification * > > v_GroupMod;
 ///< the vector of current GroupModification of the Block
 /** This vector contains all the GroupModification corresponding to all the
  * channels defined in this Block, with the corresponding unique channel
  * name. The vector has to be scanned to find if a channel has been defined
  * in the Block, which has a linear cost on the number of such channels;
  * however, this number is expected to be very low, and therefore using a
  * vector is preferred to, say, a std::map for the lower memory overhead. */

 unsigned int f_channel;   ///< the "default GroupModification channel"

 static std::string f_prefix;  ///< the executable-wide filename prefix

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 template< class F >
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
 * in which it is active. The removal of a Variable from a Constraint or
 * Objective typically results in a Modification being issued, which may be
 * wasteful in some cases; to avoid this one could "just" use issueindMod ==
 * eNoMod, although this has to be done with great care [see the comments to
 * remove_dynamic_variables()]. */

 void remove_variable_from_stuff( Variable * const variable ,
				  int issueindMod );

/*--------------------------------------------------------------------------*/
/// returns the bimap associated with the methods of type F
/** This method returns the bimap implementing the "methods factory" for the
 * methods of type F. This is where the pointer to the methods (and their
 * names) in the methods factory are stored. */

 template< class F >
 static inline bimap< F > & methods( void ) {
  static bimap< F > methods;
  return( methods );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 Block * f_Block;
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
 *  Block, i.e., the Objective has changed. */

class BlockMod : public AModification
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/

 /// constructor: takes the Block and the "concerns" value

 BlockMod( Block * fblock , bool cB = false )
  : AModification( cB ),  f_Block( fblock ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BlockMod() = default;   ///< destructor, does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the pointer to the Block to which the Modification refers

 Block * get_Block( void ) const override { return( f_Block ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*------------------- PROTECTED METHODS OF THE CLASS -----------------------*/
 /// print the BlockMod

 void print( std::ostream & output ) const override {
  output << "BlockMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on Block [" << &f_Block << "]: obj changed" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Block * f_Block;  ///<  Block (*) which the Modification refers to

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

 BlockModAD( bool cB = false ) : AModification( cB ) {}

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
  * IMPORTANT NOTE: all the Variable whose pointers are written into
  * \p variables (if any) are guaranteed to be EXACTLY OF THE SAME TYPE,
  * since they all belong to the same :BlockModAD, which are all template on
  * a specific type of :Variable (if it is Variable at all). Thus, if it is
  * necessary to dynamic_cast<> the Variable * to better understand which
  * type of :Variable the Modification is about, one can do it only on the
  * first element of the vector.
  *
  * @param variables the vector in which the pointers to the affected Variable
  *        will be stored. */

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
  *        Constraint will be stored. */

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
 * that have been added, which must be either a :Constraint or a :Variable.
 *
 * Whatever stuff is added to a list of dynamic stuff, the "names == indices
 * in the list" that the added Constraint/Variable take will be "length of the
 * list", "length of the list + 1", where the length is taken immediately
 * before of the insertion. The Modification stores the first of these names
 * (i.e., "length of the list") in case it may be useful to better handle it.
 * Note that this refers to the index that the Constraint/Variable
 * (whose pointers are anyway returned by added())
 *
 *     HAD AT THE MOMENT IN WHICH THE BlockModAdd WAS ISSUED, SINCE WHEN THE
 *     BlockModAdd IS PROCESSED, THESE Constraint/Variable MAY HAVE CHANGED
 *     "NAME" (IF Constraint/Variable BEFORE THEM IN THE LIST HAVE BEEN
 *     REMOVED), OR COULD HAVE EVEN BEEN REMOVED (IN WHCH CASE AN
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * Yet, this information may still be useful to a Solver which keeps some
 * internal data structures depending on the order of the Constraint/Variable
 * in the list, as it then would immediately know where to put the information
 * about the newly added Constraint/Variable. Note that
 *
 *     THE INDEX THAT THE ADDED Constraint/Variable HAVE AT THE MOMENT IN
 *     WHICH THE BlockModAdd IS PROCESSED CAN ONLY BE SMALLER THAN OR EQUAL
 *     TO THAT THAT THE INFORMATION REPORTED HERE IMPLIES, EXCEPT IF A
 *     Constraint/Variable HAS BEEN DELETED AND RE-ADDED (IN WHCH CASE TWO
 *     APPROPRIATE Modification MUST BE SITTING IN THE QUEUE AFTER THIS ONE).
 *
 * This may allow to simplify somewhat the work for some Solver. */

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
              std::vector< ConstOrVar * > && add , Block::Index first ,
              bool cB = false )
  : BlockModAD( cB ) , whc_list( whc ) , add_vec( std::move( add ) ) , 
    f_first( first ) {
  static_assert( std::is_base_of< Variable , ConstOrVar >::value ||
                 std::is_base_of< Constraint , ConstOrVar >::value ,
                 "BlockModAD: must inherit from Variable or Constraint" );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, no specific code needed (all is done automatically)

 virtual ~BlockModAdd() = default;

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 Block * get_Block( void ) const override {
  return( add_vec[ 0 ]->get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the reference to) the affected list of Constraint/Variable

 std::list< ConstOrVar > & whc( void ) const { return( whc_list ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the array of the added Constraint/Variable

 const std::vector< ConstOrVar * > & added( void ) const {
  return( add_vec );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the "name" that the first added stuff got

 Block::Index first( void ) const { return( f_first ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_variable( void ) const override {
  return( std::is_base_of< Variable, ConstOrVar >::value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_added( void ) const override { return( true ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_elements( std::vector< Variable * > & variables )
  const override { get_elements_( variables ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_elements( std::vector< Constraint * > & constraints )
  const override { get_elements_( constraints ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModAdd

 void print( std::ostream & output ) const override {
  output << "BlockModAdd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: added " << add_vec.size();
  if( std::is_base_of< Variable, ConstOrVar >::value )
   output << " Variable";
  else
   output << " Constraint";
  if( add_vec.size() > 1 )
   output << "s [ " << f_first << " , "
          << f_first + add_vec.size() << " )";
  else
   output << " " << f_first;

  output << " to list:" << whc_list << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 std::list< ConstOrVar > & whc_list;   ///< reference to the affected list

 std::vector< ConstOrVar * > add_vec;  ///< vector of pointers to added stuff

 Block::Index f_first;  ///< "name" that the first added stuff got

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*--------------------------- PRIVATE METHODS ------------------------------*/

 template< class T >
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
 *
 * Although this class is defined, it is purposely pure virtual because at it
 * is the information is incomplete. The issue is that while rmvd completely
 * identifies which Constraint/Variable have been removed,
 *
 *     WITHOUT FURTHER INFORMATION THE ORIGINAL "NAMES" OF THE REMOVED
 *     Constraint/Variable, I.E., THEIR POSITION IN THE LIST AT THE MOMENT
 *     THEY ARE REMOVED, IS IRREMEDIABLY LOST. INDEED, ONE CANNOT LOOK IT
 *     UP BY SEEKING THE Constraint/Variable IN THE LIST, SINCE THEY ARE BY
 *     DEFINITION NO LONGER THERE (UNLESS IF BY CHANCE A Constraint/Variable
 *     WITH THE SAME NAME == POINTER HAS BEEN ADDED LATER).
 *
 * To handle this, a Solver should therefore be able to associate the pointer
 * of the Constraint/Variable to the appropriate bits of its internal data
 * structures. While this is possible, it may be much more convenient if the
 * information is stored in the Modification. This is in fact done by derived
 * classes of BlockModRmv. The important remark is that these Modification can
 * necessarily only store
 *
 *     THE "NAMES == INDICES IN THE LIST" THAT THE DELETED Constraint/Variable
 *     HAD AT THE MOMENT IN WHICH THE Modification WAS ISSUED
 *
 * Anyway, "name == pointer" of the removed Constraint/Variable is always
 * returned by removed(). */

template< class ConstOrVar >
class BlockModRmv : public BlockModAD {

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
  : BlockModAD( cB ) , f_whc( whc ) , f_rmvd( std::move( rmvd ) ) {
  static_assert( std::is_base_of< Variable , ConstOrVar >::value ||
                 std::is_base_of< Constraint , ConstOrVar >::value ,
                 "BlockModRmv: must inherit from Variable or Constraint" );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, finally deleting the list of Constraint / Variable
 /** The destructor of BlockModRmv automatically performs a very important
  * and nontrivial task: by deleting the list of removed Constraint /
  * Variable held in the f_rmvd field, it finally deletes the actual objects.
  * This happens automagically when the last Solver / Block / whatever having
  * received the BlockModRmv deletes the pointer, hence when it is actually
  * safe to delete the Constraint / Variable because no-one still need to
  * access them to see what they held and what was their "name = pointer".
  * However, when the content is Constraint, before being deleted they must
  * be clear()-ed, since this is not done by (the various versions of)
  * remove_dynamic_constraints() in order not to delete the list of active
  * Variable of the Constraint, which may help the :Solver to manage the
  * BlockModRmv. */

 virtual ~BlockModRmv() {
  if constexpr( std::is_base_of< Constraint , ConstOrVar >::value ) {
   for( auto & el : f_rmvd )
    el.clear();
   }
  }

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 Block * get_Block() const override {
  return( f_rmvd.front().get_Block() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to (the reference to) the affected list of Constraint/Variable

 std::list< ConstOrVar > & whc( void ) const { return( f_whc ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the list of the removed Constraint/Variable

 const std::list< ConstOrVar > & removed( void ) const { return( f_rmvd ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_variable( void ) const override {
  return( std::is_base_of< Variable , ConstOrVar >::value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_added( void ) const override { return( false ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_elements( std::vector< Variable * > & variables )
  const override { get_elements_( variables ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_elements( std::vector< Constraint * > & constraints )
  const override { get_elements_( constraints ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModRmv (not)

 void print( std::ostream & output ) const override = 0;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 std::list< ConstOrVar > & f_whc;     ///< reference to the affected list

 std::list< ConstOrVar > f_rmvd;      ///< list of removed stuff

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*--------------------------- PRIVATE METHODS ------------------------------*/

 template< class T >
 void get_elements_( std::vector< T * > & elements ) const {
  if constexpr( std::is_base_of< T , ConstOrVar >::value ) {
   elements.resize( f_rmvd.size() );
   auto it = elements.begin();
   auto it2 = f_rmvd.begin();
   while( it != elements.end() )
    *it++ = const_cast< ConstOrVar * >( &*it2++ );
   }
  else
   elements.clear();
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModRmv ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BlockModRmvRngd ---------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockModRmv for removing ranges of stuff to a Block
/** Derived class from BlockModRmv to describe modifications to a Block
 * involving the removal of a range of dynamic Variable or Constraint. It
 * basically only extends BlockModRmv with a Range field. */

template< class ConstOrVar >
class BlockModRmvRngd : public BlockModRmv< ConstOrVar >
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
  * @param the Range range, containing the usual left-closed, right-open
  *        interval with the original names that the Constraint or Variable
  *        (of type ConstOrVar) in \p rmvd had before they were removed;
  *        note that
  *
  *     THE MAPPING BETWEEN rmvd AND range IS POSITIONAL, I.E., rmvd[ 0 ]
  *     IS THE Constraint/Variable THAT WAS IN whc IN POSITION range.first,
  *     rmvd[ 1 ] IS THE ONE IN POSITION range.first + 1, ...; IN OTHER
  *     WORDS, rmvd WAS THE SUB-LIST OF whc BETWEEN range.first (INCLUDED)
  *     AND range.second (EXCLUDED), AND OF COURSE ONE EXPECTS THE ORDER OF
  *     ELEMENTS IN rmvd TO BE THE ORIGINAL ORDER IN whc
  *
  * @param the bool cB, containing the "concerns" value.
  *
  * Note that a pointer to the affected Block can always be inferred from the
  * other information that the Modification contains, and therefore is not
  * needed. */

 BlockModRmvRngd( std::list< ConstOrVar > & whc ,
                  std::list< ConstOrVar > && rmvd , Block::Range range ,
                  bool cB = false )
  : BlockModRmv< ConstOrVar >( whc , std::move( rmvd ) , cB ) ,
    f_range( range ) {
  // why on earth one needs "this->" to reference the fields of the base
  // class is totally obscure to me
  if( f_range.second - f_range.first != this->f_rmvd.size() )
   throw( std::invalid_argument(
                          "BlockModRmvRngd: incompatible range and rmvd" ) );
   }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, *apparently* doing nothing

 virtual ~BlockModRmvRngd() = default;

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
 /// accessor to the range

 Block::c_Range & range( void ) const { return( f_range ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModRmvRngd

 void print( std::ostream & output ) const override {
  output << "BlockModRmvRngd[";
  if( this->concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: removed ";
  if( std::is_base_of< Variable, ConstOrVar >::value )
   output << "Variable";
  else
   output << "Constraint";
  if( this->f_rmvd.size() > 1 )
   output << "s [ " << f_range.first << " , " << f_range.second << " )";
  else
   output << f_range.first;
  output << " from list " << this->f_whc << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Block::Range f_range;  ///< the range of removed stuff

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModRmvRngd ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BlockModRmvSbst ---------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockModRmv for removing subsets of stuff to a Block
/** Derived class from BlockModRmv to describe modifications to a Block
 * involving the removal of a subset of dynamic Variable or Constraint. It
 * basically only extends BlockModRmv with a Subset field. */

template< class ConstOrVar >
class BlockModRmvSbst : public BlockModRmv< ConstOrVar >
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
  * @param the Subset subset, containing the original names that the
  *        Constraint or Variable (of type ConstOrVar) in \p rmvd had before
  *        they were removed; note that
  *
  *     subset IS ASSUMED TO BE ORDERED IN INCREASING SENSE AND WITHOUT
  *     REPEATED ELEMENTS, AND THE MAPPING BETWEEN rmvd AND subset IS
  *     POSITIONAL, I.E., rmvd[ 0 ] IS THE Constraint/Variable THAT WAS IN
  *     whc IN POSITION subset[ 0 ], rmvd[ 1 ] IS THE ONE IN POSITION 
  *     subset[ 1 ], ...; THIS IMPLIES THAT THE ELEMENTS IN rmvd ARE
  *     ORDERED BETWEEN THEMSELVES EXACTLY AS THEY WERE IN whc
  *
  *        As a special case, however,
  *
  *     subset IS ALLOWED TO BE EMPTY, MEANING THAT ALL THE Constraint OR
  *     Variable FROM THE LIST WHICH HAVE BEEN REMOVED.
  *
  * @param the bool cB, containing the "concerns" value.
  *
  * Note that a pointer to the affected Block can always be inferred from the
  * other information that the Modification contains, and therefore is not
  * needed. */

 BlockModRmvSbst( std::list< ConstOrVar > & whc ,
                  std::list< ConstOrVar > && rmvd ,
                  Block::Subset && subset , bool cB = false )
  : BlockModRmv< ConstOrVar >( whc , std::move( rmvd ) , cB ) ,
    f_subset( subset ) {
  // why on earth one needs "this->" to reference the fields of the base
  // class is totally obscure to me
  if( ( ! f_subset.empty() ) && ( f_subset.size() != this->f_rmvd.size() ) )
   throw( std::invalid_argument(
                         "BlockModRmvSbst: incompatible subset and rmvd" ) );
   }

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor, *apparently* doing nothing

 virtual ~BlockModRmvSbst() = default;

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
 /// accessor to the range

 Block::c_Subset & subset( void ) const { return( f_subset ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the BlockModRmvSbst

 void print( std::ostream & output ) const override {
  output << "BlockModRmvSbst[";
  if( this->concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "]: removed " << f_subset.size();
  if( std::is_base_of< Variable, ConstOrVar >::value )
   output << " Variable";
  else
   output << " Constraint";
  if( this->f_rmvd.size() > 1 )
   output << "s";
  output << " from list " << this->f_whc << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Block::Subset f_subset;  ///< the subset of removed stuff

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockModRmvSbst ) )

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
 *
 * Crucially, a BlockConfig can be used in two different ways: "setting mode"
 * and "differential mode". How the current object has to be interpreted is
 * specified by the field #f_diff. A full description of the difference
 * between the two modes is provided in the comments to the
 * Block::set_BlockConfig() method; however, the general gist is that in
 * "setting mode" (#f_diff == false) the previous Configuration of a Block are
 * replaced by the ones in this BlockConfig. Instead, in "differential mode"
 * (#f_diff == true), all nullptr Configuration in this BlockConfig indicate
 * that the corresponding Configuration of the Block must not be changed;
 * while all non-nullptr Configuration in this BlockConfig replace the
 * corresponding Configuration of the Block. */

class BlockConfig : public Configuration {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING BlockConfig -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: initializes everything to "default configuration"

 /** It constructs an empty BlockConfig, which can then be initialized by
  * calling the methods deserialize(), load() or get(). The \p diff parameter
  * indicates whether this BlockConfig must be considered as a "differential"
  * one. This parameter has true as default value, so that this can be used as
  * the void constructor.
  *
  * @param diff indicates if this configuration is a "differential" one. */

 BlockConfig( bool diff = true ) : Configuration() ,
  f_static_constraints_Configuration( nullptr ) ,
  f_dynamic_constraints_Configuration( nullptr ) ,
  f_static_variables_Configuration( nullptr ) ,
  f_dynamic_variables_Configuration( nullptr ) ,
  f_objective_Configuration( nullptr ) ,
  f_is_feasible_Configuration( nullptr ) ,
  f_is_optimal_Configuration( nullptr ) ,
  f_solution_Configuration( nullptr ) ,
  f_extra_Configuration( nullptr ) ,
  f_diff( diff ) {}

/*--------------------------------------------------------------------------*/
 /// constructs a BlockConfig out of the given netCDF \p group
 /** It constructs a BlockConfig out of the given netCDF \p group.  Please
  * refer to the deserialize() method for the format of the netCDF::NcGroup of
  * a BlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        BlockConfig. */

 BlockConfig( netCDF::NcGroup & group ) : BlockConfig() {
  BlockConfig::deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs a BlockConfig out of an istream
 /** It constructs a BlockConfig out of the given istream \p input.
  * Please refer to the load() method for the format of a BlockConfig.
  *
  * @param input The istream containing the description of the
  *        BlockConfig. */

 BlockConfig( std::istream & input ) : BlockConfig() {
  BlockConfig::load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs a BlockConfig for the given Block
 /** It constructs a BlockConfig for the given \p block. It creates an empty
  * BlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which a BlockConfig will be
  *        constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 BlockConfig( Block * block , bool diff = false ) : BlockConfig( diff ) {
  BlockConfig::get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 BlockConfig( const BlockConfig & old );

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 BlockConfig( BlockConfig && old );

/*--------------------------------------------------------------------------*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a BlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should contain the following:
  *
  * - the attribute "diff" of netCDF::NcInt type containing the value for the
  *   #f_diff field (basically, a bool telling if the information in it has to
  *   be taken as "the configuration to be set" or as "the changes to be made
  *   from the current configuration"); this attribute is optional: if it is
  *   not provided, then diff = true is assumed;
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
  * All these groups are optional. If a group is not provided, the
  * corresponding field of the class is filled with a nullptr, indicating that
  * the "default" configuration (whatever that may mean for the :Block in
  * question) has to be used. */

 virtual void deserialize( const netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: deletes all Configuration

 virtual ~BlockConfig() { delete_sub_Configuration(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the BlockConfig of the given Block
 /** This method gets information about the current set of parameters of the
  * given Block and stores in this BlockConfig. This information consists of
  * the sub-Configuration of the Block (see the general notes of
  * BlockConfig). Any sub-Configuration that this BlockConfig may have is
  * deleted and the sub-Configuration of the BlockConfig of the given Block is
  * cloned into the Configuration of this BlockConfig.
  *
  * @param block A pointer to the Block whose BlockConfig must be filled. */

 virtual void get( Block * block );

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE BlockConfig ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the BlockConfig
 *  @{ */

 /// delete all sub-Configuration
 /** This method deletes all sub-Configuration and sets the value of #f_diff
  * to false. */

 void clear( void ) override {
  f_diff = false;
  delete_sub_Configuration();
  }

/*--------------------------------------------------------------------------*/
 /// configure the given Block
 /** Method for configuring the given Block. The configuration depends on the
  * field #f_diff, which indicates whether it has to be interpreted in
  * "differential mode". Please refer to Block::set_BlockConfig() for
  * understanding how #f_diff and \p deleteold affect the configuration of a
  * Block. The behaviour of this method is the following:
  *
  * First, a new BlockConfig is constructed and the individual Configuration
  * of this BlockConfig are moved into that new one. This means that all the
  * individual Configuration of this BlockConfig become nullptr. The new
  * BlockConfig is then set as the BlockConfig for the given Block \p block by
  * invoking Block::set_BlockConfig().
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current Configuration of Block must
  *        be deleted (see Block::set_BlockConfig()). */

 virtual void apply( Block * block ,  bool deleteold = true ) {
  if( ! block )
   return;

  auto newBC = new BlockConfig( this->is_diff() );
  this->move_non_null_configuration_to( newBC );
  block->set_BlockConfig( newBC, deleteold );
  }

/*--------------------------------------------------------------------------*/
 /// Moves the non-nullptr sub-Configuration from this BlockConfig into \p bc
 /** This method moves one-by-one the individual non-nullptr sub-Configuration
  * stored in this BlockConfig into \p bc. If a sub-Configuration in this
  * BlockConfig is nullptr, the corresponding sub-Configuration in \p bc is
  * kept untouched. If \p deleteold is true then each sub-Configuration in \p
  * bc that gets replaced is deleted. */

 void move_non_null_configuration_to( BlockConfig * bc ,
				      bool deleteold = true ) {
  if( ! bc )
   return;

  auto move = [ deleteold ]( Configuration *& this_config,
                             Configuration *& other_config ) {
   if( this_config ) {  // replace the sub-Configuration
    if( deleteold ) {
     delete other_config;
     other_config = nullptr;
     }
    other_config = this_config;
    this_config = nullptr;
    }
   };

  move( f_static_constraints_Configuration ,
        bc->f_static_constraints_Configuration );
  move( f_dynamic_constraints_Configuration ,
        bc->f_dynamic_constraints_Configuration );
  move( f_static_variables_Configuration ,
        bc->f_static_variables_Configuration );
  move( f_dynamic_variables_Configuration ,
        bc->f_dynamic_variables_Configuration );
  move( f_objective_Configuration , bc->f_objective_Configuration );
  move( f_is_feasible_Configuration , bc->f_is_feasible_Configuration );
  move( f_is_optimal_Configuration , bc->f_is_optimal_Configuration );
  move( f_solution_Configuration , bc->f_solution_Configuration );
  move( f_extra_Configuration , bc->f_extra_Configuration );
 }

/*------------------------------- CLONE -----------------------------------*/

 BlockConfig * clone( void ) const override {
  return( new BlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE BlockConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the BlockConfig
 * @{ */

 /// extends Configuration::serialize( netCDF::NcFile , type ) to eProbFile
 /** Since a BlockConfig knows it is a BlockConfig, it "knows its place" in
  * an eProbFile netCDF SMS++ file. */

 void serialize( netCDF::NcFile & f , int type ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a BlockConfig. See BlockConfig::deserialize( netCDF::NcGroup )
  * for details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/** @} ---------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE BlockConfig -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the BlockConfig
 *  @{ */

 /// change the mode of this configuration
 /** This function changes the mode of this BlockConfig. If \p diff is true,
  * then this BlockConfig starts to be "interpreted in a differential
  * sense". */

 void set_diff( bool diff = true ) { f_diff = diff; }

/** @} ---------------------------------------------------------------------*/
/*------------- Methods for reading the data of the BlockConfig ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the BlockConfig
 *  @{ */

 /// tells if the configuration is a "differential" one (reads #f_diff)

 bool is_diff( void ) const { return( f_diff ); }

/*--------------------------------------------------------------------------*/
 /// returns true if the BlockConfig is "empty"
 /** Returns true if the BlockConfig is "empty", i.e., all of its
  * sub-Configuration are nullptr. */

 virtual bool empty( void ) const {
  return( ( ! f_static_constraints_Configuration ) &&
          ( ! f_dynamic_constraints_Configuration ) &&
	  ( ! f_static_variables_Configuration ) &&
	  ( ! f_dynamic_variables_Configuration ) &&
	  ( ! f_objective_Configuration ) &&
	  ( ! f_is_feasible_Configuration ) &&
	  ( ! f_is_optimal_Configuration ) &&
	  ( ! f_solution_Configuration ) &&
	  ( ! f_extra_Configuration ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PUBLIC FIELDS OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public fields of the class
 *  @{ */

 /// the Configuration for generate_abstract_constraints()
 Configuration * f_static_constraints_Configuration;

 /// the Configuration for generate_dynamic_constraints()
 Configuration * f_dynamic_constraints_Configuration;

 /// the Configuration for generate_abstract_variables()
 Configuration * f_static_variables_Configuration;

 /// the Configuration for generate_dynamic_variables()
 Configuration * f_dynamic_variables_Configuration;

 /// the Configuration for set_objective()
 Configuration * f_objective_Configuration;

 /// the Configuration for is_feasible()
 Configuration * f_is_feasible_Configuration;

 /// the Configuration for is_optimal()
 Configuration * f_is_optimal_Configuration;

 /// the Configuration for get_Solution() and map_[back/forward]_solution
 Configuration * f_solution_Configuration;

 /// any extra Block-specific Configuration
 Configuration * f_extra_Configuration;

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BlockConfig
 void print( std::ostream & output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this BlockConfig out of an istream
 /** Load this BlockConfig out of an istream, with the following format:
  *
  * - a binary number b to determine the value of #f_diff. If b == 0 then
  *   #f_diff = false, otherwise, #f_diff = true.
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
  *  information describing the corresponding :Configuration in the format
  *  accepted by Configuration::deserialize( std::istream ), with all the
  *  corresponding input options, like '*' for  nullptr and "*<filename>"
  *  for loading it out of a different file. */

 void load( std::istream & input ) override;

/*-------------------- PROTECTED FIELDS OF THE CLASS -----------------------*/

 bool f_diff;  ///< tells if the configuration is a "differential" one

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*---------------------------- PRIVATE METHODS -----------------------------*/

 /// delete all sub-Configuration of this BlockConfig

 void delete_sub_Configuration( void )
 {
  delete f_static_constraints_Configuration;
  f_static_constraints_Configuration = nullptr;

  delete f_dynamic_constraints_Configuration;
  f_dynamic_constraints_Configuration = nullptr;

  delete f_static_variables_Configuration;
  f_static_variables_Configuration = nullptr;

  delete f_dynamic_variables_Configuration;
  f_dynamic_variables_Configuration = nullptr;

  delete f_objective_Configuration;
  f_objective_Configuration = nullptr;

  delete f_is_feasible_Configuration;
  f_is_feasible_Configuration = nullptr;

  delete f_is_optimal_Configuration;
  f_is_optimal_Configuration = nullptr;

  delete f_solution_Configuration;
  f_solution_Configuration = nullptr;

  delete f_extra_Configuration;
  f_extra_Configuration = nullptr;
  }

/*--------------------------------------------------------------------------*/
 /// clone the sub-Configuration of bc into those of this BlockConfig

 void clone_sub_Configuration( const BlockConfig & bc )
 {
  if( bc.f_static_constraints_Configuration )
   f_static_constraints_Configuration =
    bc.f_static_constraints_Configuration->clone();

  if( bc.f_dynamic_constraints_Configuration )
   f_dynamic_constraints_Configuration =
    bc.f_dynamic_constraints_Configuration->clone();

  if( bc.f_static_variables_Configuration )
   f_static_variables_Configuration =
    bc.f_static_variables_Configuration->clone();

  if( bc.f_dynamic_variables_Configuration )
   f_dynamic_variables_Configuration =
    bc.f_dynamic_variables_Configuration->clone();

  if( bc.f_objective_Configuration )
   f_objective_Configuration = bc.f_objective_Configuration->clone();

  if( bc.f_is_feasible_Configuration )
   f_is_feasible_Configuration = bc.f_is_feasible_Configuration->clone();

  if( bc.f_is_optimal_Configuration )
   f_is_optimal_Configuration = bc.f_is_optimal_Configuration->clone();

  if( bc.f_solution_Configuration )
   f_solution_Configuration = bc.f_solution_Configuration->clone();

  if( bc.f_extra_Configuration )
   f_extra_Configuration = bc.f_extra_Configuration->clone();
  }

/*--------------------------------------------------------------------------*/

};  // end( class( BlockConfig ) )

/** @}  end( group( Block_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------- Block-RELATED TYPES ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_TYPES Type definitions in Block.h
 *  @{ */


/** @}  end( group( Block_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*---------------------- Block-RELATED FUNCTIONS ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Block_FUNCTIONS Block-related functions.
 *  @{ */

/// deserialize a Block (*) out of a given group
/** Deserialize a Block (*) , out of the given \p group and into \p data.
 * This is done by calling Block::new_Block() on the given \p group if
 * \p name is empty, and otherwise on the sub.group of \p group with the
 * given \p name. */

inline void deserialize( const netCDF::NcGroup & group , Block * & data ,
			 const std::string & name = "" )
{
 if( name.empty() )
  data = Block::new_Block( group );
 else
  data = Block::new_Block( group.getGroup( name ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a Block (*) into a given group
/** Serialize a Block (*) out of \p data. This is done by serializing the
 * Block in \p group if \p name is empty, and otherwise by creating the
 * sub-group of \p group with the given \p name and serializing the Block
 * there. */

inline void serialize( netCDF::NcGroup & group , const Block * data ,
		       const std::string & name = "" )
{
 if( name.empty() )
  data->serialize( group );
 else {
  auto gr = group.addGroup( name );
  data->serialize( gr );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize basically any STL container of Block (*)
/** Deserialize basically any STL container of Block (*) out of the given
 * \p group and into \p data. This is supposed to be represented by the
 * dimension with name \p size giving the size of the container, plus by as
 * many sub-groups of \p group with name <name>0, <name>1, ..., each one
 * containing one of the Block. */

template< template< class ... > class C >
void deserialize( const netCDF::NcGroup & group , C< Block * > & data ,
		  const std::string & size = "size" ,
		  const std::string & name = "Block_" )
{
 for( auto el : data )
  delete el;
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  data.clear();
  return;
  }
 data.resize( dim.getSize() );
 size_t i = 0;
 for( auto & el : data )
  el = Block::new_Block( group.getGroup( name + std::to_string( i++ ) ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize basically any STL container of Block (*)
/** Serialize basically any STL container of Block (*) into the given
 * \p group and into \p data. This is supposed to be represented by the
 * dimension with name \p size giving the size of the container, plus by as
 * many sub-groups of \p group with name <name>0, <name>1, ..., each one
 * containing one of the Block. */

template< template< class ... > class C >
void serialize( netCDF::NcGroup & group , const C< Block * > & data ,
		const std::string & size = "size" ,
		const std::string & name = "Block_" )
{
 group.addDim( size , data.size() );
 size_t i = 0;
 for( auto el : data )
  el->serialize( group.addGroup( name + std::to_string( i++ ) ) );
 }

/** @} end( group( Block_FUNCTIONS ) ) */

/*--------------------------------------------------------------------------*/
/*---------------------- INLINE METHODS IMPLEMENTATION ---------------------*/
/*--------------------------------------------------------------------------*/

template< class Const >
std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
Block::add_dynamic_constraints( std::list< Const > & list ,
                                std::list< Const > & newlist ,
                                ModParam issueMod ) {
 if( newlist.empty() )  // actually no Constraint to add
  return;               // cowardly (and silently) return

 if( issue_mod( issueMod ) ) {
  // initialize the vector of pointer to added Constraint
  Index first = list.size();
  auto names = std::vector< Const * >( newlist.size() );
  auto it = names.begin();
  for( auto & el : newlist ) {  // all the new Constraint
   el.set_Block( this );        // now belong to this Block
   *(it++) = &el;             // keep their names
   }

  // add them at the end, *before* issuing the BlockModAdd
  list.splice( list.end() , newlist );

  // now issue the BlockModAdd
  add_Modification( std::make_shared< BlockModAdd< Const > >( list ,
                                        std::move( names ) , first ,
                                        Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else {
  for( auto & el : newlist )    // all the new Constraint
   el.set_Block( this );        // now belong to this Block

  list.splice( list.end(), newlist );  // add them at the end
  }
 }  // end( Block::add_dynamic_constraints( Const ) )

/*--------------------------------------------------------------------------*/

template< class Var >
std::enable_if_t< std::is_base_of_v< Variable, Var > , void >
Block::add_dynamic_variables( std::list< Var > & list ,
                              std::list< Var > & newlist ,
                              ModParam issueMod ) {
 if( newlist.empty() )  // actually no Variable to add
  return;               // cowardly (and silently) return

 if( issue_mod( issueMod ) ) {
  // initialize the vector of pointer to added Constraint
  Index first = list.size();
  auto names = std::vector< Var * >( newlist.size() );
  auto it = names.begin();
  for( auto & el : newlist ) {  // all the new Variable
   el.set_Block( this );        // now belong to this Block
   *(it++) = &el;               // keep their names
   }

  // add them at the end, *before* issuing the BlockModAdd
  list.splice( list.end() , newlist );

  // now issue the BlockModAdd
  add_Modification( std::make_shared< BlockModAdd< Var > >( list ,
                                       std::move( names ) , first ,
                                       Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else {
  for( auto & el : newlist )    // all the new Variable
   el.set_Block( this );        // now belong to this Block

  list.splice( list.end(), newlist );  // add them at the end
  }
 }  // end( Block::add_dynamic_variables( Var ) )

/*--------------------------------------------------------------------------*/

template< class Const >
std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
Block::remove_dynamic_constraints( std::list< Const > & list ,
                                   std::vector< typename
                                                std::list< Const >::iterator
                                                > & rmvd ,
                                   ModParam issueMod ) {
 if( rmvd.empty() )  // actually no Constraints to remove
  return;            // cowardly (and silently) return

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 // note that each Constraint is removed from its active Variable, but it
 // is not clear()-ed now: if a Modification is issued, the list of active
 // Variable remaining available may help the :Solver to manage it, and
 // clear()-ing will be done in the Modification destructor
 for( const auto const_it : rmvd )
  remove_constraint_from_variables( &( *const_it ) );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Const > removed;
  Subset subset( rmvd.size() );

  // remove all the Constraint (whose iterators are found) in rmvd and add
  // them to the removed list; note that by using splice() the address of
  // the actual Constraints objects is not changed. meanwhile also construct
  // the list of names, thereby checking that rmvd is correct
  Index i = 0;
  auto rit = rmvd.begin();
  auto sit = subset.begin();
  for( auto lit = list.begin() ;
       ( lit != list.end() ) && ( rit != rmvd.end() ) ; ++i )
   if( &( *lit ) == &( *( *rit ) ) ) {
    ++lit;  // increment the iterator before removing
    removed.splice( removed.end(), list, *(rit++) );  // move element
    *(sit++) = i;                                       // record position
    }
   else
    ++lit;

  if( rit != rmvd.end() )
   throw( std::invalid_argument( "invalid or unordered removed list" ) );

  // now check if this actually was a range
  bool isr = true;
  if( list.empty() ) {  // unless the list has been emptyed
   isr = false;
   subset.clear();
   }
  else
   for( i = subset.front() , sit = subset.begin() ; ++sit != subset.end() ; )
    if( *sit != ++i ) {
     isr = false;
     break;
     }

  // now issue the BlockModRmv*
  if( isr )
   add_Modification( std::make_shared< BlockModRmvRngd< Const > >( list ,
		                  std::move( removed ) ,
		                  Range( subset.front() , subset.back() + 1 ) ,
				  Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvSbst< Const > >( list ,
                                  std::move( removed ) , std::move( subset ) ,
				  Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else                           // nobody is listening, just do it
  for( auto el : rmvd ) {
   el->clear();  // ... which means the Constraint need be clear()-ed now
   list.erase( el );
  }
 }  // end( Block::remove_dynamic_constraints( iterators ) )

/*--------------------------------------------------------------------------*/

template< class Const >
std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
Block::remove_dynamic_constraint( std::list< Const > & list ,
                                  typename std::list< Const >::iterator rmvd ,
                                  ModParam issueMod ) {
 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 // note that the Constraint is removed from its active Variable, but it
 // is not clear()-ed now: if a Modification is issued, the list of active
 // Variable remaining available may help the :Solver to manage it, and
 // clear()-ing will be done in the Modification destructor
 remove_constraint_from_variables( &( *rmvd ) );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Const > removed;

  // remove the Constraint pointed by rmvd and add it the removed list; note
  // that by using splice() the address of the actual Constraint object is
  // not changed. meanwhile also find its name, thereby checking that it is
  // correct

  Index i = 0;
  auto lit = list.begin();
  for( ; lit != list.end() ; ++lit , ++i )
   if( &( *lit ) == &( *rmvd ) ) {
    removed.splice( removed.end() , list , rmvd );
    break;
    }

  if( lit == list.end() )
   throw( std::invalid_argument( "invalid removed iterator" ) );

  // now issue the BlockModRmv*
  if( list.empty() )
   add_Modification( std::make_shared< BlockModRmvSbst< Const > >( list,
                                        std::move( removed ) , Subset() ,
                                        Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvRngd< Const > >( list ,
                                  std::move( removed ) , Range( i , i + 1 ) ,
				  Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else {                         // nobody is listening, just do it
  rmvd->clear();  // ... which means the Constraint need be clear()-ed now
  list.erase( rmvd );
  }
 }  // end( Block::remove_dynamic_constraint )

/*--------------------------------------------------------------------------*/

template< class Const >
std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
Block::remove_dynamic_constraints( std::list< Const > & list ,
                                   Range range , ModParam issueMod ) {
 if( range.second <= range.first )  // actually no Constraints to remove
  return;                           // cowardly (and silently) return

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 if( range.second > list.size() )
  throw( std::invalid_argument( "invalid removing range" ) );

 // construct iterators to range.first and range.second at minimum cost
 typename std::list< Const >::iterator sit;
 typename std::list< Const >::iterator eit;
 if( list.size() - range.second < range.first ) {
  eit = std::prev( list.end(), list.size() - range.second );
  if( range.second - range.first < range.first )
   sit = std::prev( eit, range.second - range.first );
  else
   sit = std::next( list.begin(), range.first );
  }
 else {
  sit = std::next( list.begin(), range.first );
  if( range.second - range.first < list.size() - range.second )
   eit = std::next( sit, range.second - range.first );
  else
   eit = std::prev( list.end(), list.size() - range.second );
  }

 // note that each Constraint is removed from its active Variable, but it
 // is not clear()-ed now: if a Modification is issued, the list of active
 // Variable remaining available may help the :Solver to manage it, and
 // clear()-ing will be done in the Modification destructor
 for( auto it = sit ; it != eit ; ++it )
  remove_constraint_from_variables( &( *it ) );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Const > removed;

  // remove all the Constraint in the range and add them to the removed list
  removed.splice( removed.end() , list , sit , eit );

  if( list.empty() )
   add_Modification( std::make_shared< BlockModRmvSbst< Const > >( list ,
                                        std::move( removed ) , Subset() ,
                                        Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvRngd< Const > >( list ,
                                        std::move( removed ) , range ,
                                        Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else {                           // nobody is listening, just do it
  // ... which means the Constraint need be clear()-ed now
  for( auto it = sit ; it != eit ; ++it )
   it->clear();
  list.erase( sit, eit );
  }

 }  // end( Block::remove_dynamic_constraints( range ) )

/*--------------------------------------------------------------------------*/

template< class Const >
std::enable_if_t< std::is_base_of_v< Constraint , Const > , void >
Block::remove_dynamic_constraints( std::list< Const > & list ,
                                   Subset && subset , bool ordered ,
                                   ModParam issueMod ) {
 if( subset.empty() ) {  // completely cleanup the list
  if( list.empty() )     // which is empty already
   return;               // cowardly (and silently) return

  // note that each Constraint is removed from its active Variable, but it
  // is not clear()-ed now: if a Modification is issued, the list of active
  // Variable remaining available may help the :Solver to manage it, and
  // clear()-ing will be done in the Modification destructor
  for( auto & c : list )
   remove_constraint_from_variables( &c );

  if( issue_mod( issueMod ) ) {  // somebody is listening
   std::list< Const > removed;

   // remove all the Constraint add them to the removed list
   removed.splice( removed.begin() , std::move( list ) );

   add_Modification( std::make_shared< BlockModRmvSbst< Const > >( list ,
                                        std::move( removed ) , Subset() ,
                                        Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
   }
  else {                           // nobody is listening, just do it
   for( auto & c : list )
    remove_constraint_from_variables( &c );
   list.clear();
   }

  return;
  }

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 if( ! ordered )
  std::sort( subset.begin(), subset.end() );

 if( subset.back() >= list.size() )
  throw( std::invalid_argument( "invalid index in removing subset" ) );

 // check if subset actually is a range
 bool isr = true;
 Index i = subset.front();
 for( auto sit = subset.begin() ; ++sit != subset.end() ; )
  if( *sit != ++i ) {
   isr = false;
   break;
   }

 // if so, defer to the simpler range version of the method
 if( isr ) {
  remove_dynamic_constraints( list ,
                              Range( subset.front() , subset.back() + 1 ) ,
                              issueMod );
  return;
  }

 // deletion is easier if performed backwards; note that, unusually,
 // eit points to list[ subset.back() ] and not one position to the right
 i = subset.size() - 1;
 Index pos = subset.back();
 typename std::list< Const >::iterator eit = list.size() - pos < pos
  ? std::prev( list.end(), list.size() - pos )
  : std::next( list.begin(), pos );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Const > removed;

  // remove each Constraint in the subset and add them to the removed
  // list; meanwhile, remove it from its active Variable, but do not
  // clear() it, so that the list of active Variable remains available,
  // which can help the :Solver to manage the Modification (clear()-ing
  // will be done in the Modification destructor)
  // note that to keep the positional relation between subset and removed,
  // insertion in removed has to be done at the beginning

  while( i > 0 ) {
   // the next value of the iterator has to be found before moving *eit
   // out of the list, since this may invalidate the iterator
   auto npos = subset[ --i ];
   auto nit = std::prev( eit, pos - npos );
   remove_constraint_from_variables( &( *eit ) );
   removed.splice( removed.begin(), list, eit );
   pos = npos;
   eit = nit;
   }

  // the last element has to be done offline
  remove_constraint_from_variables( &( *eit ) );
  removed.splice( removed.begin() , list , eit );

  // note that the list cannot be empty, since this would mean that
  // subset was the complete range 0, 1, ..., list.size() - 1, which
  // would have been discovered and acted upon before
  add_Modification( std::make_shared< BlockModRmvSbst< Const > >( list ,
                                std::move( removed ) , std::move( subset ) ,
                                Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else {                             // nobody is listening, just do it
  while( i > 0 ) {
   // the next value of the iterator has to be found before removing *eit
   // from the list, since this may invalidate the iterator
   auto npos = subset[ --i ];
   auto nit = std::prev( eit, pos - npos );
   remove_constraint_from_variables( &( *eit ) );
   eit->clear();  // ... which means the Constraint need be clear()-ed now
   list.erase( eit );
   pos = npos;
   eit = nit;
   }

  // the last element has to be done offline
  remove_constraint_from_variables( &( *eit ) );
  eit->clear();
  list.erase( eit );
  }
 }  // end( Block::remove_dynamic_constraints( subset ) )

/*--------------------------------------------------------------------------*/

template< class Var >
std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
Block::remove_dynamic_variables( std::list< Var > & list ,
                                 std::vector< typename
                                              std::list< Var >::iterator
                                              > & rmvd ,
                                 ModParam issueMod , ModParam issueindMod ) {
 if( rmvd.empty() )  // actually no Variables to remove
  return;            // cowardly (and silently) return

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 for( const auto & var_it : rmvd )
  remove_variable_from_stuff( &( *var_it ) , issueindMod );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Var > removed;
  Subset subset( rmvd.size() );

  // remove all the Variable (whose iterators are found) in rmvd and add
  // them to the removed list; note that by using splice() the address of
  // the actual Variable objects is not changed. meanwhile also construct
  // the list of names, thereby checking that rmvd is correct
  Index i = 0;
  auto rit = rmvd.begin();
  auto sit = subset.begin();
  for( auto lit = list.begin() ;
       ( lit != list.end() ) && ( rit != rmvd.end() ) ; ++i )
   if( &( *lit ) == &( *( *rit ) ) ) {
    ++lit;  // increment the iterator before removing
    removed.splice( removed.end(), list, *(rit++) );  // move element
    *(sit++) = i;                                       // record position
    }
   else
    ++lit;

  if( rit != rmvd.end() )
   throw( std::invalid_argument( "invalid or unordered removed list" ) );

  // now check if this actually was a range
  bool isr = true;
  if( list.empty() ) {  // unless the list has been emptyed
   isr = false;
   subset.clear();
   }
  else
   for( i = subset.front() , sit = subset.begin() ; ++sit != subset.end() ; )
    if( *sit != ++i ) {
     isr = false;
     break;
     }

  // now issue the BlockModRmv*
  if( isr )
   add_Modification( std::make_shared< BlockModRmvRngd< Var > >( list ,
                                std::move( removed ) ,
				Range( subset.front() , subset.back() + 1 ) ,
                                Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvSbst< Var > >( list ,
                                std::move( removed ) , std::move( subset ) ,
                                Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else                           // nobody is listening, just do it
  for( auto & el : rmvd )
   list.erase( el );

 }  // end( Block::remove_dynamic_variables( iterators ) )

/*--------------------------------------------------------------------------*/

template< class Var >
std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
Block::remove_dynamic_variable( std::list< Var > & list ,
                                typename std::list< Var >::iterator rmvd ,
                                ModParam issueMod , ModParam issueindMod ) {
 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 remove_variable_from_stuff( &( *rmvd ) , issueindMod );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Var > removed;

  // remove the Variable pointed by rmvd and add it the removed list; note
  // that by using splice() the address of the actual Variable object is
  // not changed. meanwhile also find its name, thereby checking that it is
  // correct

  Index i = 0;
  auto lit = list.begin();
  for( ; lit != list.end() ; ++lit , ++i )
   if( &( *lit ) == &( *rmvd ) ) {
    removed.splice( removed.end() , list , rmvd );
    break;
    }

  if( lit == list.end() )
   throw( std::invalid_argument( "invalid removed iterator" ) );

  // now issue the BlockModRmv*
  if( list.empty() )
   add_Modification( std::make_shared< BlockModRmvSbst< Var > >( list ,
                                       std::move( removed ) , Subset() ,
                                       Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvRngd< Var > >( list ,
                                  std::move( removed ) , Range( i , i + 1 ) ,
				  Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else                           // nobody is listening, just do it
  list.erase( rmvd );

 }  // end( Block::remove_dynamic_variable )

/*--------------------------------------------------------------------------*/

template< class Var >
std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
Block::remove_dynamic_variables( std::list< Var > & list , Range range ,
                                 ModParam issueMod , ModParam issueindMod ) {
 if( range.second <= range.first )  // actually no Constraints to remove
  return;                           // cowardly (and silently) return

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 if( range.second > list.size() )
  throw( std::invalid_argument( "invalid removing range" ) );

 // construct iterators to range.first and range.second at minimum cost
 typename std::list< Var >::iterator sit;
 typename std::list< Var >::iterator eit;
 if( list.size() - range.second < range.first ) {
  eit = std::prev( list.end(), list.size() - range.second );
  if( range.second - range.first < range.first )
   sit = std::prev( eit, range.second - range.first );
  else
   sit = std::next( list.begin(), range.first );
  }
 else {
  sit = std::next( list.begin(), range.first );
  if( range.second - range.first < list.size() - range.second )
   eit = std::next( sit, range.second - range.first );
  else
   eit = std::prev( list.end(), list.size() - range.second );
  }

 // cleanup
 for( auto it = sit ; it != eit ; ++it )
  remove_variable_from_stuff( &( *it ) , issueindMod );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Var > removed;

  // remove all the Variable in the range and add them to the removed list
  removed.splice( removed.end() , list , sit , eit );

  if( list.empty() )
   add_Modification( std::make_shared< BlockModRmvSbst< Var > >( list ,
                                       std::move( removed ) , Subset() ,
                                       Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  else
   add_Modification( std::make_shared< BlockModRmvRngd< Var > >( list ,
                                        std::move( removed ) , range ,
                                        Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
  }
 else                             // nobody is listening, just do it
  list.erase( sit , eit );

 }  // end( Block::remove_dynamic_variables( range ) )

/*--------------------------------------------------------------------------*/

template< class Var >
std::enable_if_t< std::is_base_of_v< Variable , Var > , void >
Block::remove_dynamic_variables( std::list< Var > & list ,
                                 Subset && subset , bool ordered ,
                                 ModParam issueMod , ModParam issueindMod ) {
 if( subset.empty() ) {  // completely cleanup the list
  if( list.empty() )     // which is empty already
   return;               // cowardly (and silently) return

  // cleanup
  for( auto & v : list )
   remove_variable_from_stuff( &v, issueindMod );

  if( issue_mod( issueMod ) ) {  // somebody is listening
   std::list< Var > removed;

   // remove all the Constraint add them to the removed list
   removed.splice( removed.begin() , std::move( list ) );

   add_Modification( std::make_shared< BlockModRmvSbst< Var > >( list ,
                                       std::move( removed ) , Subset() ,
                                       Observer::par2concern( issueMod ) ) ,
		     Observer::par2chnl( issueMod ) );
   }
  else                             // nobody is listening, just do it
   list.clear();

  return;
  }

 if( list.empty() )
  throw( std::invalid_argument( "removing from empty list" ) );

 if( ! ordered )
  std::sort( subset.begin(), subset.end() );

 if( subset.back() >= list.size() )
  throw( std::invalid_argument( "invalid index in removing subset" ) );

 // check if subset actually is a range
 bool isr = true;
 Index i = subset.front();
 for( auto sit = subset.begin() ; ++sit != subset.end() ; )
  if( *sit != ++i ) {
   isr = false;
   break;
   }

 // if so, defer to the simpler range version of the method
 if( isr ) {
  remove_dynamic_variables( list ,
                            Range( subset.front() , subset.back() + 1 ) ,
                            issueMod , issueindMod );
  return;
  }

 // deletion is easier if performed backwards; note that, unusually,
 // eit points to list[ subset.back() ] and not one position to the right
 i = subset.size() - 1;
 Index pos = subset.back();
 typename std::list< Var >::iterator eit = list.size() - pos < pos
  ? std::prev( list.end(), list.size() - pos )
  : std::next( list.begin(), pos );

 if( issue_mod( issueMod ) ) {  // somebody is listening
  std::list< Var > removed;

  // remove all the Variable in the subset and add them to the removed
  // list; meanwhile, do the cleanup. note that to keep the positional
  // relation between subset and removed, insertion in removed has to be
  // done at the beginning

  while( i > 0 ) {
   // the next value of the iterator has to be found before moving *eit
   // out of the list, since this may invalidate the iterator
   auto npos = subset[ --i ];
   auto nit = std::prev( eit, pos - npos );
   remove_variable_from_stuff( &( *eit ), issueindMod );
   removed.splice( removed.begin() , list , eit );
   pos = npos;
   eit = nit;
   }

  // the last element has to be done offline
  remove_variable_from_stuff( &( *eit ) , issueindMod );
  removed.splice( removed.begin() , list , eit );

  // note that the list cannot be empty, since this would mean that
  // subset was the complete range 0, 1, ..., list.size() - 1, which
  // would have been discovered and acted upon before
  add_Modification( std::make_shared< BlockModRmvSbst< Var > >( list ,
                                std::move( removed ) , std::move( subset ) ,
                                Observer::par2concern( issueMod ) ) ,
		    Observer::par2chnl( issueMod ) );
  }
 else {                             // nobody is listening, just do it
  while( i > 0 ) {
   // the next value of the iterator has to be found before removing *eit
   // from the list, since this may invalidate the iterator
   auto npos = subset[ --i ];
   auto nit = std::prev( eit , pos - npos );
   remove_variable_from_stuff( &( *eit ) , issueindMod );
   list.erase( eit );
   pos = npos;
   eit = nit;
   }

  // the last element has to be done offline
  remove_variable_from_stuff( &( *eit ) , issueindMod );
  list.erase( eit );
  }
 }  // end( Block::remove_dynamic_variables( subset ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Block.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Block.h -----------------------------*/
/*--------------------------------------------------------------------------*/

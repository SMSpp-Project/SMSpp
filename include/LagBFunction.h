/*--------------------------------------------------------------------------*/
/*------------------------- File LagBFunction.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the LagBFunction class, which is derived from both a
 * C05Function and a Block and implements the concept of the Lagrangian
 * function of some Block (the unique sub-Block of LagBFunction when "seen"
 * as a Block) w.r.t. a given set of linear terms.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Enrico Gorgone.
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __LagBFunction
 #define __LagBFunction
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include "C05Function.h"

#include "DQuadFunction.h"

#include "FRealObjective.h"

#include "LinearFunction.h"

#include "Solution.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class BlockSolverConfig;  // forward definition of BlockSolverConfig

 class LagBFunctionState;  // forward declaration of LagBFunctionState

 class BoxSolver;          // forward declaration of BoxSolver

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup LagBFunction_CLASSES Classes in LagBFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LagBFunction -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Lagrangian Function, which is also a Block
/** The class LagBFunction is a convenience class implementing the "abstract"
 * concept of Lagrangian Relaxation of "any" Block. LagBFunction derives from
 * *both* C05Function and Block.
 *
 * The main ingredients of a LagBFunction are three:
 *
 * 1) A "base" Block B, representing "any" optimization problem
 *
 *      (B)    max { c( x ) : x \in X }
 *
 *    This will be the one, and only, inner Block of LagBFunction (when
 *    "seen" as a Block).
 *
 * 2) A vector of pairs defining the Lagrangian term: < y , g(x) > =
 *    [ < y_i , g_i(x) > ]_{ i \in I }. The vector-valued function g(x) =
 *    [ g_i( x ) ]_{ i \in I } is defined in the same Variable x as B, and
 *    it should be thought as [a part of] the "complicating constraints" of
 *    some original problem
 *
 *      (O)  max { c(x) [+ ...] : g(x) [+ ...] [<]=[>] 0 , x \in X [, ...] }
 *
 *    that are relaxed to make it easier. The "[...]" are meant to convey the
 *    fact that (O) may have other groups of variables besides x, but when
 *    the constraints are relaxed in a Lagrangian way the links between the
 *    other groups of variables and x are rescinded, and (B) (with a changed
 *    objective) is one of the independent subproblems which form the
 *    Lagrangian relaxations. Each of them clearly makes a different and
 *    independent Lagrangian function (of y), and here we are focussing on
 *    just an arbitrary one of them. In the comments, the relaxed constraints
 *    will sometimes be also referred to as (RCs). The notation "[<]=[>]"
 *    means that the constraints can (almost) indifferently be equalities or
 *    inequalities, the difference simply yielding (or not) sign constraints
 *    (on either direction) on the Lagrangian multipliers. This also depends
 *    on whether g(x) is the original constraint or is opposite (- g(x)).
 *    LagBFunction assumes that the right choices regarding to the signs are
 *    done by whomever constructed the g(x) it is passed and takes no
 *    responsibility for this. The vector y = [ y_i ]_{ i \in I } of
 *    Lagrangian multipliers, real-valued Variable (ColVariable) each one
 *    associated to one of the complicating terms g_i(x), must *not* be of
 *    Variable of the LagBFunction seen as a Block, because they are
 *    conceptually fixed when the Block is solved. The y_i corresponding to
 *    inequality constraints will have an appropriate sign constraint (either
 *    by y_i being is_positive)= or is_negative() or by, say, some appropriate
 *    :OneVarConstraint regarding y_i); however, this will be a concern of
 *    the "outer" Block defining them, and therefore is not a concern of the
 *    LagBFunction.
 *
 * 3) A list of pairs of relaxed functions and their Lagrangian multipliers:
 *    { ( y_i , g_i( x ) ) }_{ i \in \bar{I} } which handle the dynamic
 *    generation/removal which handle the dynamic relaxation of constraints.
 *
 * Note that the LagBFunction is not supposed to have any Constraint or
 * Variable in itself, that is, besides "x" and "x in X"  (respectively) that
 * come with (B).
 *
 * With these elements, LagBFunction represents the Lagrangian function
 *
 *   (L_y)   l( y ) = max { c( x ) + \sum_{ i \in I } y_i g_i( x ) : x \in X }
 *
 * The function l( y ) is convex in y (concave if (B) is a minimization
 * problem), since it is the pointwise maximum of (possibly, infinitely
 * many) linear functions in y. As such it is continuous in the interior of
 * its domain; however, it typically is non differentiable. Indeed, if x( y ) 
 * is the (eps-)optimal solution of (L_y), then
 *
 *       h = g( x( y ) ) = [ g_i( x( y ) ) ]_{ i \in I }
 *
 * is a(n eps-)subgradient of l( y ) at y (supergradient if (B) is a
 * minimization problem and therefore l is concave). Hence, the gradient of l
 * at y is well-defined only if x( y ) is unique, which may easily not happen.
 * This is why in general LagBFunction is a C05Function depending on the
 * variables y; note that the LagBFunction can still easily declare to be
 * smooth [see C05Function::is_continuously_differentiable()] if it knows it
 * to be the case (say, c( x ) is strictly concave).
 *
 * The aim of LagBFunction is to automate the process of turning the block B
 * (given g and y) into the Lagrangian function l( y ), implementing all the
 * notrivial mechanics about local and global pool of linearizations (=
 * eps-subgradients = eps-optimal solutions to (L_y)), transforming
 * Modification of the block (B) into Modification of the C05Function, and so
 * on. To do this in the most general way, no assumption is made on (B) and g
 * save that:
 *
 * - the Objective c( x ) of (B) is a "simple" function, i.e., it belongs to
 *   following classes:
 *
 *   = FRealObjective whose inside Function is a LinearFunction
 *
 *   = FRealObjective whose inside Function is a DQuadFunction
 *
 * - each g[ i ] is a generic Function (*), but the Function has to be 
 *   "simple" function, i.e., belonging to following classes:
 *
 *   = LinearFunction
 *
 * - If (B) has sub-Block, each of them having a nonempty "physical Objective"
 *   must also have an abstract Objective, which must be a :RealObjective,
 *   so that the value() of all the Objective can be summed up to that of
 *   the modified FRealObjective of (B) to compute the cost of each Solution
 *   (this is not really needed for the Lagrangian cost, since this is
 *   supposedly done by the solver, but rather for the original cost of a
 *   solution which is necessary to compute the linearization_constant;
 *   note that only the "root" (B) Objective is modified, so that for all the
 *   Objective of the sub-Block of (B) the Lagrangian cost and the original
 *   cost are the same).
 *
 * Under these assumptions, LagBFunction can implement the required machinery
 * to use the inner Block (B), with any attached Solver, to implement the
 * C05Function interface for the Lagrangian function l( y ).
 *
 *     SINCE LagBFunction RELIES ON THE Solver ATTACHED TO THE INNER Block TO
 *     COMPUTE ITSELF, THERE MUST BE SUCH A Solver BY THE TIME compute() IS
 *     FIRST CALLED. IF MORE THAN ONE Solver IS ATTACHED TO THE INNER Block,
 *     LagBFunction USES THE ONE SPECIFIED BY THE InnrSlvr PARAMETER (by
 *     default the first of them). REGISTERING OR UN-REGISTERING Solver FROM
 *     THE INNER Block OF THE LagBFunction MUST NEVER CAUSE InnrSlvr TO
 *     BECOME INVALID, OR TO CHANGE IF INFORMATION PRODUCED IN THE LAST
 *     compute() (FUNCTION VALUES, LINEARIZATIONS, ...) IS STILL TO BE
 *     RETRIEVED.
 *
 * It should, however, in principle be possible to change the Solver at every
 * call of compute(), provided this is done "right before the call".
 *
 * The Observer of a LagBFunction will be, say, a FRealObjective or a
 * FRowConstraint belonging to some Block. In turn, a LagBFunction is the
 * Observer of both the (linear) Function g[ i ] defining the Lagrangian
 * term, as well as the inner Block (B) (since the LagBFunction is also a
 * Block, and the father of (B)). This means that a LagBFunction have to
 * handle the Modification which come to it from both ways: the inner Block
 * (B) and the constraints (RCs). Note that these Modification, per se, will
 * *not* be sent to the Oserver of the LagBFunction; rather, they will be
 * "digested" and become (typically, C05)FunctionMod* which describe the
 * impact on the LagBFunction of the corresponding chenges.
 *
 * In principle, therefore, the father Block of a LagBFunction should not be
 * defined, in that the LagBFunction is not supposed to be directly a
 * sub-Block of some other Block (although it will be inside a FRealObjective
 * or a FRowConstraint. However, since f_Block is available, a special
 * mechanism is implemented: if f_Block is defined (not nullptr), then all the
 * Modification sent to the LagBFunction are also forwarded to it. This is
 * done to support the case (that somehow breaks the Block interface but can
 * be useful) where the sub-Block (B) of the LagBFunction was originally part
 * of some father Block (F) but it has been "stolen" from it: by setting the
 * father of the LagBFunction to (F) one "reconstructs the link between (B)
 * and (F)" (i.e., (B) is now a "grandson" of (F) rather than it son, but at
 * least still a sibling) and ensures that the original flow of Modification
 * is not interrupted. However, note that
 *
 *     ALL Modification HAPPENING WHEN f_play_dumb == true, I.E., THOSE THAT
 *     ARE DIRECTLY GENERATED BY THE LagBFunction ITSELF, ARE NOT FORWARDED
 *     TO THE f_Block OF THE LagBFunction (if it is defined)
 *
 * The idea is that the changes in the (Objective of the) sub-Block are
 * "temporary" and will be undone by the time anything else has the chance
 * to "look again at (B)"; cf. cleanup_inner_objective().
 *
 * The current implementation of LagBFunction has the following limitations
 * (implementation decisions) that somewhat further limit the kind of
 * Block that can be used as (B):
 *
 * - THE LAGRANGIAN TERM c( x ) + \sum_{ i \in I } y_i g_i(x) FOR FIXED VALUE
 *   OF y_i AND LINEAR c( x ), g_i( x ) BECOMES EITHER A LinearFunction OR A
 *   DQuadFunction (DEPENDING ON WHAT IT WAS ORIGINALLY) THAT IS WRITTEN IN
 *   THE OBJECTIVE OF (B).
 *
 *   The first, obvious consequence is that (B) must necessarily have an
 *   "abstract") Objective that is a FRealObjective whose Function is either
 *   a LinearFunction or a DQuadFunction, as as already stated. This also
 *   means that if (B) has a "physical" representation, it must reflect all
 *   changes of its Objective resulting from the Lagrangian term into it.
 *   Some :Block may make assumptions on the form of its Objective (say, a
 *   certain set of Variable is *not* in it) that are satisfied by c(x) but
 *   are no longer so when the Lagrangian term is added; these cannot be used
 *   as (B). However, note that ALL "EXTRA" TERMS IN THE Objective OF (B) WILL
 *   BE ADDED "AFTER" THE ORIGINAL ONES, which may make it possible for a
 *   Block/Solver to ignore them since THE ORIGINAL ONES WILL STILL BE IN
 *   THEIR ORIGINAL POSITION.
 *
 * - ALL THE INDIVIDUAL TERMS c'_j x_j OF THE LAGRANGIAN TERM (WITH c_'j THE
 *   LAGRANGIAN COST FOR FIXED y) ARE ADDED TO/MODIFIED INTO THE OBJECTIVE
 *   OF (B), EVEN THOUGH THE Variable MAY IN FACT BE DEFINED IN A SUB-Block
 *   OF (B)
 *
 *   This is consistent with the assumption that a Variable in a sub-Block is
 *   still owned by the father Block, but it also means that the same Variable
 *   may end up in more than one Objective; say, that of the sub-Block
 *   defining it as well as that of (B). This should in general be supported,
 *   but there may be :Solver that do not allow such an arrangement. On the
 *   plus side, however, note that in such a case THE Objective OF THE
 *   sub-Block IN NEVER MODIFIED, and therefore it CAN IN PRINCIPLE BE ANY
 *   KIND OF Function, NOT NECESSARILY A LinearFunction OR A DQuadFunction
 *   (provided that the Solver attached to (B) can deal with it).
 *
 * - THE LAGRANGIAN TERM MAY HAVE A DIFFERENT "SHAPE" THAN THE ORIGINAL
 *   LinearFunction c( x ) (i.e., more ColVariable may have a nonzero
 *   coefficient than in happened in c(x)), which means that ColVariable
 *   CAN BE ADDED TO c( x ) THAT WERE NOT ORIGINERILY THERE.
 *
 *   This, as already mentioned, my be a problem for some Block/Solver that
 *   require a specific arrangement. Furthermore, it means that ANY PROCESS
 *   CHANGING THE COEFFICIENTS OF c(x) "FROM OUTSIDE OF THE LagBFunction"
 *   MAY FIND "UNEXPECTED" ColVariable IN IT; ALSO, A PROCESS MAY DELETE A
 *   ColVariable FROM c(x) AND "MYSTERIOUSLY SEE IT IMMEDIATELY REINSTATED"
 *   (since that ColVariable has a nonzero coefficient in the Lagrangian
 *   function even if it has not in c(x)). This must not be a problem for
 *   the "outside" process.
 *
 * - THE LagBFunction WILL ONLY *ADD* TERMS TO c(x), BUT NEVER REMOVE THEM.
 *   In particular, it may happen that a coefficient is added to c( x ) for
 *   some variable x_j due to some term g_i(x) having a nonzero coefficient
 *   in x_j, while, say, the original coefficient(s) of x_j in c( x ) was 0,
 *   i.e., x_j was *not* in c(x). Later, the Lagrangian term(s) g_i(x )  may
 *   be changed and x_j may no longer have a nonzero coefficient in the
 *   Lagrangian term for any value of y. YET, x_j IS KEPT IN THE
 *   LinearFunction / DQuadFunction IN THE Objective OF (B) WITH 0
 *   COEFFICIENT(S): no attempt is made to "optimize away" such Variable. The
 *   rationale is that it is hard to distinguish whether or not the 0
 *   coefficient(s) was there in the original c( x ) or x_j was not in c( x ).
 *   While the two things are mathematically equivalent, they may not be so
 *   for a Block/Solver assuming that some Variable *are* in the Objective
 *   even if their coefficient(s) is 0. Optimizing away these Variable would
 *   not allow such a Block/Solver to be used, so it is just not done. In the
 *   odd case where this is not appropriate, the external process changing the
 *   g_i( x ) (which is what may cause this to happen) also has to do the
 *   cleanup of c( x ). Indeed, removing ColVariable from c( x ) is allowed,
 *   with the only provision that if the ColVariable is present in any
 *   Lagrangian term g_i( x ) it will be automatically re-added.
 *
 * - Similarly, A TERM < x_j , a_{ij} > IN g_i( x ) WILL GIVE RISE TO AN
 *   EXPLICIT TERM < y_i , a_{ij} > IN THE (LINEAR) EXPRESSION FOR THE
 *   LAGRANGIAN COST OF x_j EVEN IF a_{ij} == 0; no attemp is done to optimize
 *   away zero coefficients from g_i( x ), in case the entity producing them
 *   relies on their position in the LinearFunction to handle them.
 *
 * We finish with a LARGELY THEORETICAL, BUT STILL POSSIBLY INTERESTING NOTE.
 * The LagBFunction is both a C05Function and a Block. This is done in order
 * for it to be able to "intercept" any Modification from its sub-Block (B)
 * and properly react to it; however, the current implementation
 *
 *    KNOWINGLY AND INTENTIONALLY VIOLATE SOME OF THE STANDARD ASSUMPTIONS
 *    OF Block, WHICH IMPLIES THAT LagBFunction SHOULD NOT BE DIRECTLY
 *    PASSED TO A GENERAL-PURPOSE Solver AS A STAND-ALONE Block
 *
 * The point is about the Objective of the LagBFunction and that of its
 * sub-Block (B). To properly represent, in an "abstract" form, the
 * mathematical reality of (L_y), these should be arranged as follows:
 *
 * - the Objective of the LagBFunction should be the function
 *
 *       \sum_{ i \in I } y_i g_i( x )
 *
 * - the Objective of (B) should be its original function c( x ).
 *
 * However, this is not how the object is implemented. The point is that
 * LagBFunction has to compute l( y ) using (B) and its Solver; this means
 * that it has to "translate" the Objective of (B) in a form that (B) (and
 * its solver) accepts. For instance, if c() is a linear function, then
 * (B) will typically allow its coefficients to be changed, but *not* it
 * to be transformed into another kind of function. If, say, g() are also
 * linear functions ( g( x ) = Ax ), the LagBFunction will, each time when
 * compute() is called [roughly], compute the Lagrangian costs
 *
 *      c^y = c + yA
 *
 * and change the Objective of (B) accordingly. Note, however, the g(x) may
 * have constant terms: for instanxe, it could be an affine function
 * ( g( x ) = Ax + b ) rather than a linear function. In this case, which is
 * actually supported by LagBFunction, the value of the LagBFunction has the
 * form
 *
 *       c x^* + y ( A x^* + b ) = ( c + y A x^* ) + yb = c^y x^* + yb
 *
 * with x^* the optimal solution of the Lagrangian problem. Then, the extra
 * linear term yb will have to be exogenously computed and added to the
 * optimal value c^y x^* directly produced by the Solver of (B).
 *
 * This is not mathematically required; in principle, one could define a
 * BilinearFunction l( x , y ) = cx + yAx, and insist that the objective
 * of (B) be that. This would work, since y are not variables of (B), and
 * therefore are fixed when it is solved. However, it would require (B) and
 * its Solver to specifically cater for this case. The choice has been to
 * rather have LagBFunction to perform the necessary machinery, in order to
 * leave (B) and its solver completely unaware of what is happening.
 *
 * The downside of this choice is that, when examining the "abstract"
 * implementation of the LagBFunction, one finds:
 *
 * - the Objective of the LagBFunction is empty
 *
 * - the Objective of (B) is that corresponding to the last value of
 *   \bar{y} on which compute() has been called (in the linear case,
 *   c^\bar{y} x).
 *
 * Thus, this violates the intended mathematical definition of Block. Yet,
 * this is acceptable in the following two scenarios. When the LagBFunction
 * is used as a C05Function (say, inside a FRealObjective), its "innards"
 * (B) are not looked at, and do not really appear in the "abstract
 * representation": C05Function is a "black box", and any Solver seeing it
 * makes no assumption about what's there inside. However, some Solver may
 * exploit the specific form of a LagBFunction. Yet, if they do this, they
 * will check if the C05Function actually is a LagBFunction, and in this
 * case they will know exactly what to do with it. In other words, even if
 * the Objective of the LagBFunction (seen as a Block) is empty, such a
 * Solver, which is *not* general-purpose but specialized for the case, will
 * be able to "pretend" to see the right term c(x) + \sum_{i \in I} y_i g_i(x)
 * in there. Thus, while it will not be possible to use a LagBFunction as
 * "any" Block to be passed to "any" general-purpose Solver, the most
 * important use cases for it are covered.
 *
 * It would actually be possible to make the "abstract representation" of the
 * LagBFunction to exactly match its intended mathematical semantic. To do
 * that, one should (in the linear case, for notational simplicity):
 *
 * - set the Objective of the LagBFunction as c x + y A x - c^\bar{y} x
 *   (a BiLinearFunction);
 *
 * - keep the Objective of (B) to c^\bar{y} x.
 *
 * Since the Objective of the son is (implicitly) summed to that of the father,
 * this would work being the f_sense of Objective of both LagBFunction
 * and its sub-block B is eMax. By contrast, the "outer" block defining
 * LagBFunction has to minimize (L_y) in the Variable y, so f_sense of the
 * Objective of the "outer" block must be eMin. In fact, the Lagrangian dual
 * is a min-max problem (max-min if (B) is a minimization).
 *
 * However, the Objective would change each time y changes and compute() is
 * called, which is unwieldy. Also, it would require the definition of
 * BiLinearFunction and extensions. More importantly, so far there is no
 * evidence that a specialized Solver exists that it may be appropriate to
 * use to solve this kind of problem, and therefore there does not appear to
 * be any compelling reason to implement this kludge. */

class LagBFunction : public C05Function , public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 /* Since LagBFunction is both a ThinVarDepInterface and a Block, it "sees"
  * two definitions of "Index", "Range" and "Subset". These are actually the
  * same, but compilers still don't like it. Disambiguate by declaring we
  * use the ThinVarDepInterface versions (but it could have been the Block
  * versions, as they are the same. */

 using Index = ThinVarDepInterface::Index;
 using c_Index = ThinVarDepInterface::c_Index;

 using Range = ThinVarDepInterface::Range;
 using c_Range = ThinVarDepInterface::c_Range;

 using Subset = ThinVarDepInterface::Subset;
 using c_Subset = ThinVarDepInterface::c_Subset;

 using OFValue = RealObjective::OFValue;

 /* LagBFunction uses stuff from LinearFunction a lot, hence "import" here
  * corresponding names. */
 using Coefficient = LinearFunction::Coefficient;
 using coeff_pair = LinearFunction::coeff_pair;
 using v_coeff_pair = LinearFunction::v_coeff_pair;

 /// a dual_pair: (the Function defining) a Constraint and its dual variable
 using dual_pair = std::pair< ColVariable * , Function * >;

 using v_dual_pair = std::vector< dual_pair >;  ///< a vector of dual_pair
 using v_c_dual_pair = const v_dual_pair;       ///< a const v_dual_pair

 /* LagBFunction also uses stuff from DQuadFunction, hence "import" here
  * corresponding names. */
 using coeff_triple = DQuadFunction::coeff_triple;
 using v_coeff_triple = DQuadFunction::v_coeff_triple;
  
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// an element of the global pool
 /** An element of the global pool ia a Solution equipped with a boolean
  * which defines the type of linearization (diagonal, vertical) */
 using gpool_el = std::pair< p_Solution , bool >;

 /// a global pool (a vector of gpool_el)
 using v_gpool_el = std::vector< gpool_el >;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// a pair to represent a monomial y_i * a_{ij} 
 using mon_pair = std::pair< Index , Coefficient >;

 /// a vector of mon_pair (a complete term y_i A_i) 
 using v_mon_pair = std::vector< mon_pair >;

 /// a pair to represent the original c_i and the vector of < y_i , A_i >
 using col_pair = std::pair< Coefficient , v_mon_pair >;

 using m_column = std::vector< col_pair >;   ///< a vector of col_pair

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the kind of SimpleConfiguration used as extra_Configuration

 using SimpleConfig_p_p = SimpleConfiguration< std::pair< Configuration * ,
							  Configuration * > >;

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for "sifting through" the "active"
  * Variable of a LagBFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( v_dual_pair::iterator & itr ) : itr_( itr ) {}
  explicit v_iterator( v_dual_pair::iterator && itr )
   : itr_( std::move( itr ) ) {}

  v_iterator * clone( void ) override final {
   return( new v_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++(itr_); }

  reference operator*( void ) const override final {
   return( *((*itr_).first) );
   }
  pointer operator->( void ) const override final {
   return( (*itr_).first );
   }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LagBFunction::v_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LagBFunction::v_iterator * >( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LagBFunction::v_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LagBFunction::v_iterator * >( & rhs );
    return( tmp ? itr_ != tmp->itr_ : false );
   #endif
   }

  private:

  v_dual_pair::iterator itr_;
  };

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator
 /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a LinearFunction. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator( v_dual_pair::const_iterator & itr )
   : itr_( itr ) {}
  explicit v_const_iterator( v_dual_pair::const_iterator && itr )
   : itr_( std::move( itr ) ) {}

  v_const_iterator * clone( void ) override final {
   return( new v_const_iterator( itr_ ) );
   }

  void operator++( void ) override final { (itr_)++; }
  reference operator*( void ) const override final {
   return( *((*itr_).first) );
   }
  pointer operator->( void ) const override final {
   return( (*itr_).first );
   }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LagBFunction::v_const_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LagBFunction::v_const_iterator * >( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const LagBFunction::v_const_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const LagBFunction::v_const_iterator * >( & rhs );
    return( tmp ? itr_ != tmp->itr_ : false );
   #endif
   }

  private:

  v_dual_pair::const_iterator itr_;
  };

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters
 /** Public enum describing the different algorithmic parameters of int type
  * that LagBFunction has in addition to these of C05Function. The value
  * intLastLagBFPar is provided so that the list can be easily further
  * extended by derived classes. */

 enum int_par_type_LagBF {

 intInnrSlvr = intLastParC05F ,  ///< index of the inner Solver

 intNoSol ,        ///< whether Solution are required from the inner Block

 intChkState ,     ///< whether to check the Solution in the put_State

 intLastLagBFPar   ///< first allowed new int parameter for derived classes
                   /**< Convenience value for easily allow derived classes
		    * to extend the set of int algorithmic parameters. */

 };  // end( int_par_type_LagBF )

/*--------------------------------------------------------------------------*/
 /// public enum for the double algorithmic parameters
 /** Public enum describing the different algorithmic parameters of double
  * type that LagBFunction has in addition to these of C05Function (currently,
  * none). The value dblLastLagBFPar is provided so that the list can be
  * easily further extended by derived classes. */

 enum dbl_par_type_LagBF {
  dblLastLagBFPar = dblLastParC05F
  ///< first allowed new double parameter for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of double algorithmic parameters. */

  };  // end( dbl_par_type_LagBF )

/*--------------------------------------------------------------------------*/
 /// public enum for the string algorithmic parameters
 /** Public enum describing the different algorithmic parameters of string
  * type that LagBFunction has in addition to these of C05Function (currently,
  * none). The value strLastLagBFPar is provided so that the list can be
  * easily further extended by derived classes. */

 enum str_par_type_LagBF {
  strLastLagBFPar = strLastParC05F
  ///< first allowed new string parameter for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of string algorithmic parameters. */

  };  // end( str_par_type_LagBF )

/*--------------------------------------------------------------------------*/
 /// public enum for the vector-of-int algorithmic parameters
 /** Public enum describing the different algorithmic parameters of type 
  * vector-of-int that LagBFunction has in addition to these of C05Function
  * (currently, none). The value vintLastLagBFPar is provided so that the
  * list can be easily further extended by derived classes. */

 enum vint_par_type_LagBF {
  vintLastLagBFPar = vintLastParC05F
  ///< first allowed new vector-of-int parameter for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-int algorithmic parameters. */

  };  // end( vint_par_type_LagBF )

/*--------------------------------------------------------------------------*/
 /// public enum for the vector-of-double algorithmic parameters
 /** Public enum describing the different algorithmic parameters of type
  * vector-of-double that LagBFunction has in addition to these of
  * C05Function (currently, none). The value vdblLastLagBFPar is provided so
  * that the list can be easily further extended by derived classes. */

 enum vdbl_par_type_LagBF {
  vdblLastLagBFPar = vdblLastParC05F
  ///< first allowed new vector-of-double parameter for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-double algorithmic parameters. */

  };  // end( vdbl_par_type_LagBF )

/*--------------------------------------------------------------------------*/
 /// public enum for the vector-of-string algorithmic parameters
 /** Public enum describing the different algorithmic parameters of type
  * vector-of-string that LagBFunction has in addition to these of
  * C05Function (currently, none). The value vstrLastLagBFPar is provided so
  * that the list can be easily further extended by derived classes. */

 enum vstr_par_type_LagBF {
  vstrLastLagBFPar = vstrLastParC05F
  ///< first allowed new vector-of-string parameter for derived classes
  /**< Convenience value for easily allow derived classes to extend the set
   * of vector-of-string algorithmic parameters. */

  };  // end( vstr_par_type_LagBF )

/** @} ---------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

 /// constructor of LagBFunction, taking the static Lagrangian pairs
 /** Constructor of LagBFunction. It accepts a pointer both to a Block and
  * an Observer, in particular the Block is the one and only inner Block (B),
  * the one defining the Lagrangian function.
  *
  * If the inner Block is not passed in the constructor, this has to be done
  * with set_inner_block().
  *
  * The constructor does not accept an array of dual pair, which consists of
  * a pair of a relaxed constraint g_i(x) and its Lagrangian multiplier y_i,
  * for i \in I. The assumption makes sure that, before saving the
  * Lagrangian multipliers, the Observer has been registered. */

 LagBFunction( Block * innerblock = nullptr , Observer * observer = nullptr );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// destructor of LagBFunction: delete the allocated memory.
 /** destructor of LagBFunction. It deletes delete the global pool and the
  * LagMatrix which is used to change the Lagrangian costs. */

 virtual ~LagBFunction( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void clear( void ) override;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// sets the inner Block (B)
 /** Method to set the inner Block (B). This is supposed to be called before
  * the LagBFunction gets an Observer, as it does not issue any Modification
  * to signal that the LagBFunction has (very radically) changed.
  *
  * If \p deleteold == true and there already is an inner Block in the
  * LagBFunction, it is deleted. */

 void set_inner_block( Block * innerblock , bool deleteold = true );

/*--------------------------------------------------------------------------*/
 /// set the Lagrangian term < y , g(x) >, hence the active Variable
 /** Method to initialise all the Lagrangian term < y , g(x) > of the
  * LagBFunction, hence its active Variable. This is supposed to be called
  * before the LagBFunction gets an Observer, as it does not issue any
  * Modification to signal that the LagBFunction has (very radically)
  * changed. In case the LagBFunction already has an Observer which is
  * actually "listening", add_dual_pairs() should be called instead
  * (preceded by a call to remove_variables( Subset() ) to remove all the
  * existing ones, if any. */

 void set_dual_pairs( v_dual_pair && dp );

/*--------------------------------------------------------------------------*/
 /// set a given int numerical parameter (see set_ComputeConfig())
 /** Set the int numerical parameters of the LagBFunction, which mostly (but
  * not exclusively) means setting those of the inner Solver used to
  * compute() it. In fact, the int parameters here can be divided in three
  * cases:
  *
  * - those of the LagBFunction itself
  *
  * - those of the LagBFunction that are "directly translated" into parameters
  *   of the inner Solver;
  *
  * - those that are directly meant for the inner Solver and therefore are not
  *   "understood" by the LagBFunction, and therefore are passed over to the
  *   inner Solver (after an appropriate name shift, see set_ComputeConfig()
  *   for details).
  *
  * The first group comprises:
  *
  * - intInnrSlvr: the index of the inner Solver, i.e., its position in the
  *                list of registered Solver in the inner Block; the default
  *                is 0 (first position);
  *
  * - intNoSol: if nonzero, it is taken to mean that the inner Block will not
  *             produce workable Solution objects and therefore that
  *   LagBFunction should not store them and use them; this severely cripples
  *   the ability of LagBFunction to efficiently respond to Modification from
  *   the inner Block, as almost every change there will have to result in a
  *   "very bad" C05FunctionMod being issued invalidting all previous
  *   information since it is impossible to update it, which is why the
  *   default value is 0 (false), which requires a working get_Solution()
  *   from the inner Block;
  *
  * - intChkState: if nonzero, it is taken to mean that the LagBFunction
  *                should check the Solution contained in its State, when
  *   the latter is put back (cf. put_State()), to verify that they are
  *   still feasible/correct directions. The default is 0, meaning that the
  *   LagBFunction will trust this being true; ensuring this is then the
  *   user's responsibility, requiring to keep a close watch on the state of
  *   the inner Block when the State is read (get_State) and put back but
  *   saving the potentially costly feasibility check. By setting this
  *   parameter to nonzero (true) the user gains considerable feasibility in
  *   re-using a State even if the inner Block is not completely identical
  *   (constraint-wise) to what it was when it was created, at a cost.
  *
  * - intGPMaxSz: controls the maximum number of stored Solution from
  *               the Solver, each one of which (again) corresponds to a
  *               linearization
  *
  * The second group comprises:
  *
  * - intLPMaxSz: the value of intLPMaxSz is passed to the inner Solver as
  *               the Solver::intMaxSol parameter, since each different
  *   Variable Solution produced by the Solver immediately translates into
  *   a linearization of the LagBFunction
  *
  * Finally, each parameter with index >= intLastLagBFPar belongs to the last
  * group. */

 void set_par( idx_type par , int value ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given double numerical parameter (see set_ComputeConfig())
 /** Set the doubke numerical parameters of the LagBFunction, which mostly
  * (but not exclusively) means setting those of the inner Solver used to
  * compute() it. In fact, the int parameters here can be divided in three
  * cases:
  *
  * - those of the LagBFunction itself
  *
  * - those of the LagBFunction that are "directly translated" into parameters
  *   of the inner Solver;
  *
  * - those that are directly meant for the inner Solver and therefore are not
  *   "understood" by the LagBFunction, and therefore are passed over to the
  *   inner Solver (after an appropriate name shift, see set_ComputeConfig()
  *   for details).
  *
  * The first group comprises: currently nothing.
  *
  * The second group comprises:
  *
  * - dblRelAcc: the value of dblRelAcc is passed to the inner Solver as
  *              the Solver::dblRelAcc parameter
  *
  * - dblAbsAcc: the value of dblAbsAcc is passed to the inner Solver as
  *              the Solver::dblAbsAcc parameter
  *
  * - dblUpCutOff: the value of dblUpCutOff is passed to the inner Solver as
  *                the Solver::dblUpCutOff parameter
  *
  * - dblLwCutOff: the value of dblLwCutOff is passed to the inner Solver as
  *                the Solver::dblLwCutOff parameter
  *
  * - dblRAccLin: the value of dblRAccLin is passed to the inner Solver as
  *               the Solver::dblRAccSol parameter, since the relative error
  *   made by the Solver in computing the Variable Solution immediately
  *   translates into the accuracy of the corresponding linearization.
  *
  * - dblAAccLin: the value of dblAAccLin is passed to the inner Solver as
  *               the Solver::dblAAccSol parameter, since the absolute error
  *   made by the Solver in computing the Variable Solution immediately
  *   translates into the accuracy of the corresponding linearization.
  *
  * Finally, each parameter with index >= dblLastLagBFPar belongs to the last
  * group. */

 void set_par( idx_type par , double value ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given string parameter (see set_ComputeConfig())
 /** Since LagBFunction does not have any string parameter itself, nor there
  * currently is any string parameter of C05Function that needs to be
  * translated to a parameter of the inner Solver, this method can only be
  * used to set the string parameters of the inner Solver. */

 void set_par( idx_type par , std::string && value ) override {
  // note: assumes no string params in C05Function & LagBFunction
  if( auto is = inner_Solver() )
   add_par( is->str_par_idx2str( str_par_lbf( par ) ) , std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given vector-of-int parameter (see set_ComputeConfig())
 /** Since LagBFunction does not have any vector-of-int parameter itself, nor
  * there currently is any vector-of-int parameter of C05Function that needs
  * to be translated to a parameter of the inner Solver, this method can only
  * be used to set the vector-of-int parameters of the inner Solver. */

 void set_par( idx_type par , std::vector< int > && value ) override {
  // note: assumes no vector-of-int params in C05Function & LagBFunction
  if( auto is = inner_Solver() )
   add_par( is->vint_par_idx2str( vint_par_lbf( par ) ) ,
	    std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given vector-of-double parameter (see set_ComputeConfig())
 /** Since LagBFunction does not have any vector-of-double parameter itself,
  * nor there currently is any vector-of-double parameter of C05Function that
  * needs to be translated to a parameter of the inner Solver, this method
  * can only be used to set the vector-of-double parameters of the inner
  * Solver. */

 void set_par( idx_type par , std::vector< double > && value ) override {
  // note: assumes no vector-of-double params in C05Function & LagBFunction
  if( auto is = inner_Solver() )
   add_par( is->vdbl_par_idx2str( vdbl_par_lbf( par ) ) ,
	    std::move( value ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given vector-of-string parameter (see set_ComputeConfig())
 /** Since LagBFunction does not have any vector-of-string parameter itself,
  * nor there currently is any vector-of-string parameter of C05Function that
  * needs to be translated to a parameter of the inner Solver, this method
  * can only be used to set the vector-of-string parameters of the inner
  * Solver. */

 void set_par( idx_type par , std::vector< std::string > && value ) override {
  // note: assumes no vector-of-string params in C05Function & LagBFunction
  if( auto is = inner_Solver() )
   add_par( is->vstr_par_idx2str( vstr_par_lbf( par ) ) ,
	    std::move( value ) );
  }

/*--------------------------------------------------------------------------*/
 /// "translate" an int parameter index of the inner Solver
 /** Takes the index \p par of an int parameter of the inner Solver and
  * returns the index that has to be passed to the LagBFunction to have
  * that very same parameter set in the inner Solver; see the comments to
  * set_ComputeConfig() for details. */

 idx_type int_par_is( idx_type par ) const {
  if constexpr( Solver::intLastAlgPar < intLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= intLastLagBFPar )
    par += intLastLagBFPar - Solver::intLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a double parameter index of the inner Solver
 /** Takes the index \p par of a double parameter of the inner Solver and
  * returns the index that has to be passed to the LagBFunction to have
  * that very same parameter set in the inner Solver; see the comments to
  * set_ComputeConfig() for details. */

 idx_type dbl_par_is( idx_type par ) const {
  if constexpr( Solver::dblLastAlgPar < dblLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= dblLastLagBFPar )
    par += dblLastLagBFPar - Solver::dblLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a string parameter index of the inner Solver
 /** Takes the index \p par of a string parameter of the inner Solver and
  * returns the index that has to be passed to the LagBFunction to have
  * that very same parameter set in the inner Solver; see the comments to
  * set_ComputeConfig() for details. */

 idx_type str_par_is( idx_type par ) const {
  if constexpr( Solver::strLastAlgPar < strLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= strLastLagBFPar )
    par += strLastLagBFPar - Solver::strLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a vector-of-int parameter index of the inner Solver
 /** Takes the index \p par of a vector-of-int parameter of the inner Solver
  * and returns the index that has to be passed to the LagBFunction to
  * have that very same parameter set in the inner Solver; see the comments
  * to set_ComputeConfig() for details. */

 idx_type vint_par_is( idx_type par ) const {
  if constexpr( Solver::vintLastAlgPar < vintLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vintLastLagBFPar )
    par += vintLastLagBFPar - Solver::vintLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a vector-of-double parameter index of the inner Solver
 /** Takes the index \p par of a vector-of-double parameter of the inner
  * Solver and returns the index that has to be passed to the LagBFunction
  * to have that very same parameter set in the inner Solver; see the
  * comments to set_ComputeConfig() for details. */

 idx_type vdbl_par_is( idx_type par ) const {
  if constexpr( Solver::vdblLastAlgPar < vdblLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vdblLastLagBFPar )
    par += vdblLastLagBFPar - Solver::vdblLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a vector-of-string parameter index of the inner Solver
 /** Takes the index \p par of a vector-of-string parameter of the inner
  * Solver and returns the index that has to be passed to the LagBFunction
  * to have that very same parameter set in the inner Solver; see the
  * comments to set_ComputeConfig() for details. */

 idx_type vstr_par_is( idx_type par ) const {
  if constexpr( Solver::vstrLastAlgPar < vstrLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vstrLastLagBFPar )
    par += vstrLastLagBFPar - Solver::vstrLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a int parameter index of the LagBFunction
 /** Takes the index \p par of an int parameter of the LagBFunction that is
  * actually meant for the inner Solver and returns the value that it would
  * have to be used to set directly in there; see the comments to
  * set_ComputeConfig() for details. */

 idx_type int_par_lbf( idx_type par ) const {
  if constexpr( Solver::intLastAlgPar < intLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= intLastLagBFPar )
    par -= intLastLagBFPar - Solver::intLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// "translate" a double parameter index of the LagBFunction
 /** Takes the index \p par of a double parameter of the LagBFunction that is
  * actually meant for the inner Solver and returns the value that it would
  * have to be used to set directly in there; see the comments to
  * set_ComputeConfig() for details. */

 idx_type dbl_par_lbf( idx_type par ) const {
  if constexpr( Solver::dblLastAlgPar < dblLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= dblLastLagBFPar )
    par -= dblLastLagBFPar - Solver::dblLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// "translate" a string parameter index of the LagBFunction
 /** Takes the index \p par of a string parameter of the LagBFunction that is
  * actually meant for the inner Solver and returns the value that it would
  * have to be used to set directly in there; see the comments to
  * set_ComputeConfig() for details. */

 idx_type str_par_lbf( idx_type par ) const {
  if constexpr( Solver::strLastAlgPar < strLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= strLastLagBFPar )
    par -= strLastLagBFPar - Solver::strLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// "translate" a vector-of-int parameter index of the LagBFunction
 /** Takes the index \p par of a vector-of-int parameter of the LagBFunction
  * that is actually meant for the inner Solver and returns the value that it
  * would have to be used to set directly in there; see the comments to
  * set_ComputeConfig() for details. */

 idx_type vint_par_lbf( idx_type par ) const {
  if constexpr( Solver::vintLastAlgPar < vintLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vintLastLagBFPar )
    par -= vintLastLagBFPar - Solver::vintLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// translate a vector-of-double parameter index of the LagBFunction
 /** Takes the index \p par of a vector-of-double parameter of the
  * LagBFunction that is actually meant for the inner Solver and returns the
  * value that it would have to be used to set directly in there; see the
  * comments to set_ComputeConfig() for details. */

 idx_type vdbl_par_lbf( idx_type par ) const {
  if constexpr( Solver::vdblLastAlgPar < vdblLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vdblLastLagBFPar )
    par -= vdblLastLagBFPar - Solver::vdblLastAlgPar;
   }
  return( par );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// translate a vector-of-string parameter index of the LagBFunction
 /** Takes the index \p par of a vector-of-string parameter of the
  * LagBFunction that is actually meant for the inner Solver and returns the
  * value that it would have to be used to set directly in there; see the
  * comments to set_ComputeConfig() for details. */

 idx_type vstr_par_lbf( idx_type par ) const {
  if constexpr( Solver::vstrLastAlgPar < vstrLastLagBFPar ) {
   if( par == Inf< idx_type >() )
    return( par );
   if( par >= vstrLastLagBFPar )
    par -= vstrLastLagBFPar - Solver::vstrLastAlgPar;
   }
  return( par );
  }

/*--------------------------------------------------------------------------*/
 /// set the whole set of parameters in one blow
 /** This method sets the whole set of parameters in one blow using a
  * ComputeConfig object.
  *
  * LagBFunction is, in some sense, no litle more than a wrapper of the
  * inner Solver used to compute() the Lagrangian function (except that this
  * wrapping is nontrivial). As such, it has comparatively few parameters,
  * while the inner Solver may have many. It is therefore advantageous to
  * allow to set the algorithmic parameters of the inner Solver to b
  * directly set by the set_par() of LagBFunction.
  * 
  * This is done by "translating" all the indices of the parameters of the
  * inner Solver, apart from the standard ones that any ThinComputeInterface
  * (and, therefore, Solver) has, so that they are > than the indices of
  * LagBFunction parameters (of the same type). If necessary *_par_is() and
  * *_par_lbf() are provided to translate an index of the inner Solver into
  * one of LagBFunction and vice-versa, respectively, but it is not necessary
  * to use them directly if the Configuration is made via set_ComputeConfig(),
  * as the translation is automatically done by the parameters setting /
  * getting methods. Basically
  *
  *     ALL PARAMETERS OF THE INNER Solver BEHAVE AS IF THEY WERE NATIVE
  *     PARAMETERS OF LagBFunction WHEN ACCESSED VIA THEIR STRING NAME
  *
  * Hence, to set the int parameter "intMyParam" of the iner Solver, it is
  * possible to just use
  *
  *     LBF->set_par( LBF->int_par_str2idx( "intMyParam" ) , value );
  *
  * as if one would be accessing the inner Solver directly; this allows all
  * the standard Configuration stuff to work unchanged. When using indices
  * to access parameters, instead, one has to do either
  *
  *     LBF->set_par( LBF->int_par_is( intMyParam ) , value );
  *
  * (assuming the enum value intMyParam to correspond to the string name
  * "intMyParam" as customary) or
  *
  *     LBF->inner_Solver()->set_par( intMyParam , value );
  *
  * (except inner_Solver() is protected, so this is only possible from a
  * derived class). In both cases one has to know that it is using a
  * LagBFunction, but this is not a big deal since using of an explicit
  * index (intMyParam) implies compile-time knowledge of the specific solver
  * one is using (in this case, a specific inner Solver inside a
  * LagBFunction).
  *
  * However, this mechanism has a consequence:
  *
  *     THE PARAMETER INDICES CHANGE MEANING IF THE TYPE OF THE INNER Solver
  *     CHANGES
  *
  * which happens changing intInnrSlvr. Hence, if one wants to change the
  * inner Solver and configure it, it should first do the change and then set
  * the parameters (which is logically required anyway). This is why in
  * set_ComputeConfig() first it is checked if "intInnrSlvr" changes, and
  * only after this is acted upon the standard
  * ThinComputeInterface::set_ComputeConfig() is called to do the bulk of the
  * work.
  *
  * LagBFunction also uses the f_extra_Configuration field of the provided
  * ComputeConfig. That field, if non-nullptr, is assumed to be:
  *
  * - either (a pointer to) a
  *
  *     SimpleConfiguration< std::pair< Configuration * , Configuration * > >
  *
  *   in which case, f_extra_Configuration->f_value.first is assumed to be a
  *   BlockSolverConfig *, and f_extra_Configuration->f_value.second is
  *   assumed to be a BlockConfig *
  *
  * - or a BlockConfig *
  *
  * - or a BlockSolverConfig *
  *
  * Note that passing nullptr resets the inner Block, as does passing a
  * non-nullptr scfg with a nullptr f_extra_Configuration if scfg->f_diff == 
  * false.  Also, if a pair of Configuration * is passed, any one of those is
  * nullptr and scfg->f_diff == false, then the corresponding part of the
  * inner Block (BlockConfig or BlockSolverConfig) is reset.
  *
  * These are used to configure the inner Block of the LagBFunction. Note that
  * exception will be thrown if any of the non-null pointers turns out not to
  * be of the right type. */

 void set_ComputeConfig( ComputeConfig * scfg = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// load a LagBFunction out of an istream - not implemented yet

 void load( std::istream & input , char frmt = 0 ) override {
  // input >> LPMaxSz;
  // input >> GPMaxSz;
  // input >> AAccLin;
  // input >> RAccLin;
  throw( std::logic_error( "LagBFunction::load not implemented yet" ) );
  }

/*--------------------------------------------------------------------------*/

 void deserialize( const netCDF::NcGroup & group ) override;

/** @} ---------------------------------------------------------------------*/
/*---------------------- METHODS FOR EVENTS HANDLING -----------------------*/
/*--------------------------------------------------------------------------*/
/** @name Set event handlers
 *
 * Since LagBFunction basically only acts as a "front end" for the "inner
 * Solver" that actually compute()s the Lagrangian function, it does not
 * handle the events itself; rather, it passes them through to the "true"
 * Solver.
 *
 *  @{ */

 EventID set_event_handler( int type , EventHandler && event ) override {
  if( auto is = inner_Solver() )
   return( is->set_event_handler( type , std::move( event ) ) );
  throw( std::logic_error(
      "LagBFunction::set_event_handler: inner Solver not available yet" ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void reset_event_handler( int type , EventID id ) override {
  if( auto is = inner_Solver() )
   is->reset_event_handler( type , id );
  else
   throw( std::logic_error(
    "LagBFunction::reset_event_handler: inner Solver not available yet" ) );
  }

/** @} ---------------------------------------------------------------------*/
/*----------------- METHODS FOR MANAGING THE "IDENTITY" --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Managing the "identity" of the LagBFunction
 *
 * Actually implement the methods of ThinComputeInterface relative to
 * temporarily changing the "identity" of the LagBFunction.
 *
 *  @{ */

 /// set the "identity" of the LagBFunction
 /** Actually implement the ThinComputeInterface::set_id() by setting the
  * f_id member of the LagBFunction class, which is initialized with "this"
  * in the constructor. LagBFunction always uses f_id to try to "lock and
  * own" the Block. This method allows to change f_id ("lend another
  * identity"); when called with nullptr argument, the id() is reset to the
  * default "this". Also, if the inner Block is set and has registered
  * Solver, their identity is also set (or reset) in anticipation that they
  * also may have to lock() the inner Block during their line of work. */

 void set_id( void * id = nullptr ) override {
  if( f_id == id )  // nothing to do
   return;          // silently (and cowardly) return

  f_id = id ? id : this;

  // propagate downwards the id change
  if( ! v_Block.empty() )
   for( auto s : v_Block.front()->get_registered_solvers() )
    s->set_id( id );
  }

/** @} ---------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE LagBFunction ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the LagBFunction
 *
 * Methods to add ar delete Lagrangian pairs < y , g( x ) >, i.e., active
 * Variable of the LagBFunction. Note that there are no explicit methods
 * to *modify* the g_i( x ) in the current Lagrangian terms because this can
 * be done by acquiring the pointer to the corresponding Function (currently
 * necessarily a LinearFunction) and directly modifying it.
 *
 *  @{ */

 /// add new Lagrangian pairs < y , g( x ) > (hence, active Variable)
 /** This method adds a bunch of new Lagrangian pairs < y , g( x ) > to the
  * existing ones, thereby increasing the number of the active [Col]Variable
  * of the LagBFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsAddd is
  * issued, as described in Observer::make_par(); note that a Lagrangian
  * function is strongly quasi-additive. */

 void add_dual_pairs( v_dual_pair && lp , ModParam issueMod = eNoBlck );

/*--------------------------------------------------------------------------*/
 /// remove the i-th Lagrangian term < y_i , g_i( x ) > from the LagBFunction
 /** Remove the i-th Lagrangian term < y_i , g_i( x ) > from the LagBFunction,
  * i.e., its i-th "active" Variable. If there is no Variable with the given
  * index, an exception is thrown.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(); Note that a LagBFunction is
  * strongly quasi-additive. */

 void remove_variable( Index i , ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// remove a range of Lagrangian terms from the LagBFunction
 /** Remove Lagrangian terms < y_i , g_i( x ) > with range.first <= i <
  * range.second from the LagBFunction, i.e., the corresponding range of
  * the "active" Variable.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsRngd is
  * issued, as described in Observer::make_par(); Note that a LagBFunction is
  * strongly quasi-additive. */

 void remove_variables( Range range , ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// remove a subset of Lagrangian terms from the LagBFunction
 /** Remove the Lagrangian terms < y_i , g_i( x ) > with i \in subset from
  * the LagBFunction, i.e., the corresponding subset of the "active" Variable.
  * If \p nms.empty(), then *all* the Lagrangian terms ("active" Variable) are
  * removed. \p ordered tells if \p nms is already ordered in increasing
  * sense.
  *
  * The parameter issueMod decides if and how the C05FunctionModVarsSbst is
  * issued, as described in Observer::make_par(); Note that a LagBFunction is
  * strongly quasi-additive. */

 void remove_variables( Subset && nms , bool ordered = false ,
			ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// stores the Solution in position i of the global pool to the Block
 /** Given a position \p i into the global pool, writes back the corresponding
  * Solution into the Block. */

 void global_pool_to_block( Index i ) {
  if( NoSol )
   throw( std::invalid_argument( "LagBFunction: Solution not stored" ) );
  if( ( i >= f_max_glob ) || ( !  g_pool[ i ].first ) )
   throw( std::invalid_argument( "global_pool_to_block: invalid index" ) );
  if( i == LastSolution )  // already there
   return;                 // nothing to do
  g_pool[ i ].first->write( v_Block.front() );
  LastSolution = i;  // and recall what's there
  }

/*--------------------------------------------------------------------------*/
 /// returns the objective of the inner Block to its "pristine" state
 /** This method removes the effect of all the Lagrangian terms from the
  * Objective of the inner Block, which is basically equivalent to forcing
  * all the y to be 0 (but without explicitly changing their value, which is
  * not something LagBFunction is supposed to do since they are not its
  * Variable). This supposedly returns the inner Block to a state completely
  * equivalent to its "initial" one.
  *
  * Note: this is not necessarily true. In fact, the Objective of the inner
  * Block may have "extra" active Variable that were not initially present,
  * save with a 0 coefficient. We are implicitly assuming that this difference
  * is irrelevant "to the outside", which is a somewhat reasonable but not
  * completely watertight assumption. */
 
 void cleanup_inner_objective( void );
 
/** @} ---------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

 /// handles Modification issued to the LagBFunction as a Block
 /** Modification reaching the LagBFunction can only be originated by its
  * only inner Block, and will be treated in an highly non-standard way:
  *
  * - There cannot be Solver attached to LagBFunction as a Block,
  *   LagBFunction as a Block has no father Block, and it makes no sense to
  *   open a channel on LagBFunction as a Block (since if a channel is
  *   needed, it can be opened directly on the inner Block); thus, it makes
  *   no sense call Block::add_Modification() to pass the Modification to the
  *   registered Solver and/or to the father Block and/or to open a channel.
  *
  * - The Modification as and of itself will not "exit" LagBFunction;
  *   rather, it can be converted in appropriate [C05]FunctionMod that will
  *   be issued to the Observer of LagBFunction.
  *
  *     FOR LACK OF A WAY TO DO DIFFERENTLY, ANY SUCH Modification WILL BE
  *     ISSUED ASSUMING THE STANDARD eModBlck TYPE, WITH concerns_Block() ==
  *     true AND THE DEFAULT CHANNEL (this could be a case where "hijacking"
  *     the default channel may be useful)
  *
  *     FOR THE EXCESSIVE RIGIDITY OF THE CURRENT Modification SYSTEM
  *     (OR, PERHAPS BETTER, ITS LACK OF GENERAL INFORMATION), ONLY THE
  *     "ABSTRACT" Modification COMING OUT OF THE INNER Block CAN BE
  *     PROCESSED, THE OTHER ONES BEING IGNORED. THIS IS CLEARLY AN
  *     ISSUE IF THE CHANGES IN THE INNER Block ONLY RESULT IN "PHISICAL"
  *     Modification BEING ISSUED (this will be improved upon by the
  *     planned re-haul of the Modification system).
  *
  * - Most of the changes made to the inner Block result in a C05FunctionMod
  *   (actually, LagBFunctionMod) with type() == GlobalPoolRemoved because
  *   they mess up with feasibility of the previous solutions / directions. 
  *   Thus, there is a risk of a long stream of basically identical
  *   Modification to be be produced, which is clearly useless. It would be
  *   nice to be able to "wait until the last Modification is received and
  *   answer only once", but there is no way to know when the "last
  *   Modification" will happen.
  *
  *   However, some degree of aggregation indeed happens when the original
  *   Modification are bunched together in a GroupModification, because in
  *   this case only one LagBFunctionMod is produced for them all. Hence
  *
  *     IF MANY CHANGES ARE MADE TO THE INNER Block, IT IS MOST DEFINITELY
  *     A GOOD IDEA TO BUNCH AS MANY AS POSSIBLE OF THE CORRESPONDING
  *     Modification INTO AS FEW AS POSSIBLE GroupModification
  *
  *   in order to reduce the number of produced LagBFunctionMod.
  *
  * - If the intNoSol parameter is set to nonzero (true), then basically all
  *   changes made to the inner Block will result in a C05FunctionMod
  *   (actually, LagBFunctionMod) with type() == GlobalPoolRemoved and
  *   which().empty() meaning that all the global pool need be reset since
  *   there is no way of checking if the previous Solution are still
  *   feasible. */

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

/**-------------------------------------------------------------------------*/
/*---------------------- Methods for handling Solver -----------------------*/
/*--------------------------------------------------------------------------*/

 /// LagBFunction is not supposed to have any Solver registered to it
 /** LagBFunction::register_Solver() throws exception if a non-nullptr Solver
  *  is registered, as it is not supposed to be used as a Block bur rather as
  *  a [C05]Function: the Solver must be attached to its sub-Block (B), not
  *  to the LagBFunction itself. */
 
 void register_Solver( Solver * newSolver , bool tofront = false ) override {
  if( newSolver )
   throw( std::logic_error( "LagBFunction::register_Solver() called" ) );
  }
  
/** @} ---------------------------------------------------------------------*/
/*---------- METHODS FOR PRINING/SAVING THE DATA OF THE LagBFunction -------*/
/*--------------------------------------------------------------------------*/
/** @name Saving the data of the LagBFunction
 *  @{ */

 /// printing very basic information about the LagBFunction

 void print( std::ostream & output , char vlvl = 0 ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// serialize a LagBFunction into a netCDF::NcGroup
 /** Serialize a LagBFunction into a netCDF::NcGroup. Note that, LagBFunction
  * being both a Function and a Block, the netCDF::NcGroup will have to have
  * the "standard format of a :Block", meaning whatever is managed by the
  * serialize() method of the base Block class, plus the
  * LagBFunction-specific data with the following format:
  *
  *     TO BE DONE
  */

 void serialize( netCDF::NcGroup & group ) const override;

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LagBFunction
 *  @{ */

 bool has_linearization( bool diagonal = true ) override;

/*--------------------------------------------------------------------------*/

 bool compute_new_linearization( bool diagonal = true ) override;

/*--------------------------------------------------------------------------*/

 void store_linearization( Index name , ModParam issueMod = eModBlck  )
  override;

/*--------------------------------------------------------------------------*/

 bool is_linearization_there( Index name ) const override {
  if( name >= g_pool.size() )
   throw( std::invalid_argument(
       "LagBFunction::is_linearization_there: invalid linearization name" ) );
  return( g_pool[ name ].first );
  }

/*--------------------------------------------------------------------------*/

 bool is_linearization_vertical( Index name ) const override {
  if( name >= g_pool.size() )
   throw( std::invalid_argument(
     "LagBFunction::is_linearization_vertical:invalid linearization name" ) );
  return( ! g_pool[ name ].second );
  }

/*--------------------------------------------------------------------------*/

 void store_combination_of_linearizations(
		           c_LinearCombination & coefficients , Index name  ,
		           ModParam issueMod = eModBlck ) override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 void set_important_linearization( LinearCombination && coefficients )
  override { zLC = std::move( coefficients ); };

/*--------------------------------------------------------------------------*/

 c_LinearCombination & get_important_linearization_coefficients( void )
  const override { return( zLC ); }

/*--------------------------------------------------------------------------*/

 void delete_linearization( Index name , ModParam issueMod = eModBlck )
  override;

/*--------------------------------------------------------------------------*/

 void delete_linearizations( Subset && which , bool ordered = true ,
			     ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
 /// compute the LagBFunction
 /** Compute the LagBFunction: this amounts at computing the Lagrangian costs
  * and changing the costs in the inner Block accordingly (if the Lagrangian
  * variables have changed, i.t., \p changedvars == true), and then using the
  * specified Solver attached to the inner Block to solve it.
  *
  * Note that LagBFunction "stealthily" adds [Col]Variable to the Objective
  * of the inner Block in case there are Lagrangian terms involving some
  * [Col]Variable that does not naturally belong there. If some Modification
  * causing new [Col]Variable to be thusly added is received from the inner
  * Block, the information corresponding to the [Col]Variable is stored, and
  * the actual addition to the Objective of the inner Block is done in the
  * next call to compute(), just before computing the new Lagrangian costs. */

 int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// ensure that the Objective of the inner Block is up-to-date
 /** LagBFunction "stealthily" adds [Col]Variable to the Objective of the
  * inner Block in case there are Lagrangian terms involving some
  * [Col]Variable that does not naturally belong there. If some Modification
  * causing new [Col]Variable to be thusly added is received from the inner
  * Block, the information corresponding to the [Col]Variable is stored, and
  * the actual addition to the Objective of the inner Block is only
  * *automatically* done in the next call to compute(). However, the user of
  * the LagBFunction may actually "look" at the inner Block, which is the
  * "physical representation" of the LagBFunction, rather than relying on
  * the LagBFunction to produce the function values and linearizations (which
  * are the "abstract representation"); see also the comments to
  * silence_inner_Modification(). Such a user may never call compute(), and
  * therefore never have the chance for the necessary [Col]Variable to be
  * "stealthily" added to the Objective of the inner Block. This method
  * ensures that the latter is, in fact, done. */

 void apply_obj_Modification( void ) {
  if( ( ! v_tmpCP.empty() ) && flush_v_tmpCP() )
   v_Block.front()->unlock( this );
  }

/*--------------------------------------------------------------------------*/
 /// returns the value of the Function
 /** It returns the value of the Function that was computed in the most recent
  * call to compute(); if the latter has never been invoked, then the value
  * returned by this method is meaningless.
  *
  * If (B) is computed with a low accuracy and the function value lays in an
  * interval, the upper estimate shall be returned if the function is convex,
  * i.e., (B) is a maximization problem, while the lower estimate is returned
  * otherwse (the function is concave, i.e., (B) is a minimization problem).
  *
  * The rationale for this choice is that the Lagrangian function is typically
  * used to compute bounds on the original problem, which is a maximization if
  * and only if (B) is. Thus, the bound is an upper bound if and only if (B)
  * is a maximization, i.e., the function is convex. In order to guarantee
  * that the value returned by the function is a valid upper bound, one
  * needs to return an upper estimate of it (the converse for a minimization
  * problem). */

 FunctionValue get_value( void ) const override {
  return( is_convex() ? get_upper_estimate() : get_lower_estimate() );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 FunctionValue get_lower_estimate( void ) const override {
  if( auto is = inner_Solver() ) {
   auto lb = is->get_lb();
   if( lb == - Inf< FunctionValue >() )
    return( lb );
   else {
    if( std::isnan( f_yb ) )
     throw( std::logic_error( "get_lower_estimate called before compute" ) );
    return( f_yb > - Inf< FunctionValue >() ? lb + f_yb : lb );
    }
   }
  else
   return( - Inf< FunctionValue >() );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 FunctionValue get_upper_estimate( void ) const override {
  if( auto is = inner_Solver() ) {
   auto ub = is->get_ub();
   if( ub == Inf< FunctionValue >() )
    return( ub );
   else {
    if( std::isnan( f_yb ) )
     throw( std::logic_error( "get_upper_estimate called before compute" ) );
    return( f_yb > - Inf< FunctionValue >() ? ub + f_yb : ub );
    }
   }
  else
   return( - Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the "constant term" of the LagBFunction

 FunctionValue get_constant_term( void ) const override;

/*--------------------------------------------------------------------------*/

 FunctionValue get_Lipschitz_constant( void ) override {
  //f_Lc = Inf< FunctionValue >();
  if( f_Lc < 0 )
   compute_Lipschitz_constant();

  return( f_Lc );
  }

/*--------------------------------------------------------------------------*/

 bool is_convex( void ) const override { return( IsConvex ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 bool is_concave( void ) const override { return( ! IsConvex ); }

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
				      Range range = Block::INFRange ,
				      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g , c_Subset & subset ,
				      bool ordered = false ,
				      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override;

/*--------------------------------------------------------------------------*/
 /// returns a pointer to the inner Block (B) defining the LagBFunction

 Block * get_inner_block( void ) const {
  return( v_Block.empty() ? nullptr : v_Block.front() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a pointer to the i-th Lagrangian term
 /** Returns a pointer to the Function defining the Lagrangian term g_i(x)
  * associated with the i-th ColVariable y_i of the LagBFunction (that
  * returned by get_active_var( i )). Currently this can only be a
  * LinearFunction, hence the return value can be safely static_cast-ed to
  * a LinearFunction *. */

 Function * get_Lagrangian_term( Index i ) const {
  return( LagPairs[ i ].second );
  }

/*--------------------------------------------------------------------------*/
 /// get the number of nonzeros in the matrix representation of g(x)
 /** Since LagBFunction currently only deals with linear [affine] functions
  * g(x) = A x [+ b], g(x) can be represented in matrix form. Usually the
  * matrix would be sparse, hence this method returns the total number of
  * nonzeros in such a representation. */

 int get_A_nz( void );

/*--------------------------------------------------------------------------*/
 /// get the by-columns representation of g(x)
 /** Since LagBFunction currently only deals with linear [affine] functions
  * g(x) = A x [+ b], g(x) can be represented in matrix form. The by-row
  * representation of the term (for each variable y_i, the linear function
  * g_i(x) = A_i x [+ b_i]) can be easily accessed via get_Lagrangian_term().
  * However, the by-column representation (for each variable x_j in the
  * inner Block, the information necessary to compute its Lagrangian cost
  * c_j - y A^j) can also be useful, and indeed LagBFunction does store it
  * internally. This method provides access to the data structure: given a
  * (pointer to) a [Col]Variable in the inner Block, it returns a (const
  * pointer to) a data structure describing its Lagrangian cost. In
  * particular, if the [Col]Variable does *not* belong to any Lagrangian
  * term, i.e., it appears nowhere in A, then it returns nullptr. Otherwise
  * it returns a (const) pointer to a std::pair< Coefficient , v_mon_pair >.
  * The coefficient is the original cost c_j of the [Col]Variable.
  * v_mon_pair is a vector of std::pair< Index , Coefficient > describing the
  * linear function y A^j = \sum_i y_i a_{ij}, where the Index is j (the
  * index of y_i as an active Variable of the LagBFunction), while the
  * Coefficient is a_{ij}.
  *
  * Note that if xj is actually not a [Col]Variable of the inner Block, the
  * method will not bother and still return nullptr. */

 const col_pair * get_A_by_col( const ColVariable * xj ) {
  if( qobj )
   throw( std::logic_error(
	   "matrix representation not available for quadratic objective" ) );
  auto j = obj->is_active( xj );
  if( j >= obj->get_num_active_var() )
   return( nullptr );
  else
   return( & CostMatrix[ j ] );
  }

/*--------------------------------------------------------------------------*/
 /// get the matrix representation of g(x) TO BE DELETED
 /** Since LagBFunction currently only deals with linear [affine] functions
  * g(x) = A x [+ b], g(x) can be represented in matrix form. This method
  * provides the usual three-vectors sparse matrix representation of A,
  * with the number of nonzeros being provided by get_NzMat().
  *
  * TODO: EXPLAIN THE FORMAT IN DETAIL. */

 void get_MatDesc( int *Abeg , int *Aind , double *Aval , int strt ,
		   int stp );

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LagBFunction
 *
 * While LagBFunction itself has comparatively few parameters, it allows to
 * change via its standard parameter interface all the parameters of the
 * "inner Solver" used to actually compute() the Lagrangian function. The
 * indices of these parameters are automatically translated (see *_par_is()
 * and the comments to set_ComputeConfig() for details) so that they can be
 * automatically set and queried as if they were "natural" parameters of
 * LagBFunction itself.
 * @{ */

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  idx_type rv = intLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , int_par_is( is->get_num_int_par() ) ) );
  return( rv );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_dbl_par( void ) const override {
  idx_type rv = dblLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , dbl_par_is( is->get_num_dbl_par() ) ) );
  return( rv );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_str_par( void ) const override {
  idx_type rv = strLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , str_par_is( is->get_num_str_par() ) ) );
  return( rv );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vint_par( void ) const override {
  idx_type rv = vintLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , vint_par_is( is->get_num_vint_par() ) ) );
  return( rv );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vdbl_par( void ) const override {
  idx_type rv = vdblLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , vdbl_par_is( is->get_num_vdbl_par() ) ) );
  return( rv );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type get_num_vstr_par( void ) const override {
  idx_type rv = vstrLastLagBFPar;
  if( auto is = inner_Solver() )
   return( std::max( rv , vstr_par_is( is->get_num_vstr_par() ) ) );
  return( rv );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  if( ( par == intInnrSlvr ) || ( par == intNoSol ) ||
      ( par == intChkState ) )
   return( 0 );

  if( par < intLastLagBFPar )
   return( C05Function::get_dflt_int_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_int_par( int_par_is( par ) ) );
  else
   return( Inf< int >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dflt_dbl_par( idx_type par ) const override {
  if( par < dblLastLagBFPar )
   return( C05Function::get_dflt_dbl_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_dbl_par( dbl_par_is( par ) ) );
  else
   return( Inf< double >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & get_dflt_str_par( idx_type par )
  const override {
  static const std::string _empty;

  if( par < strLastLagBFPar )
   return( C05Function::get_dflt_str_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_str_par( str_par_is( par ) ) );
  else
   return( _empty );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]]  const std::vector< int > & get_dflt_vint_par( idx_type par )
  const override {
  static const std::vector< int > _empty;

  if( par < vintLastLagBFPar )
   return( C05Function::get_dflt_vint_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_vint_par( vint_par_is( par ) ) );
  else
   return( _empty );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< double > & get_dflt_vdbl_par( idx_type par )
  const override {
  static const std::vector< double > _empty;

  if( par < vdblLastLagBFPar )
   return( C05Function::get_dflt_vdbl_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_vdbl_par( vdbl_par_is( par ) ) );
  else
   return( _empty );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< std::string > & get_dflt_vstr_par(
					                       idx_type par )
  const override {
  static const std::vector< std::string > _empty;

  if( par < vstrLastLagBFPar )
   return( C05Function::get_dflt_vstr_par( par ) );

  if( auto is = inner_Solver() )
   return( is->get_dflt_vstr_par( vstr_par_is( par ) ) );
  else
   return( _empty );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  if( ( par < intLastAlgParTCI ) || ( par >= intLastLagBFPar ) ) {
   if( auto is = inner_Solver() )
    return( is->get_int_par( int_par_is( par ) ) );
   else
    return( C05Function::get_dflt_int_par( par ) );
   }

  switch( par ) {
   case( intLPMaxSz ):  return( LPMaxSz );
   case( intGPMaxSz ):  return( g_pool.size() );
   case( intInnrSlvr ): return( InnrSlvr );
   case( intNoSol ):    return( NoSol ? 1 : 0 );
   case( intChkState ): return( ChkState ? 1 : 0 );
   }

  return( C05Function::get_dflt_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] double get_dbl_par( idx_type par ) const override {
  if( ( par < dblLastAlgParTCI ) || ( par >= dblLastLagBFPar ) ) {
   if( auto is = inner_Solver() )
    return( is->get_dbl_par( dbl_par_is( par ) ) );
   else
    return( C05Function::get_dflt_dbl_par( par ) );
   }

  if( auto is = inner_Solver() )
   switch( par ) {
    case( dblRelAcc ):   return( is->get_dbl_par( Solver::dblRelAcc ) );
    case( dblAbsAcc ):   return( is->get_dbl_par( Solver::dblAbsAcc ) );
    case( dblUpCutOff ): return( is->get_dbl_par( Solver::dblUpCutOff ) );
    case( dblLwCutOff ): return( is->get_dbl_par( Solver::dblLwCutOff ) );
    case( dblRAccLin ):  return( is->get_dbl_par( Solver::dblRAccSol ) );
    case( dblAAccLin ):  return( is->get_dbl_par( Solver::dblAAccSol ) );
    }

  return( C05Function::get_dflt_dbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & get_str_par( idx_type par )
  const override {
  if( auto is = inner_Solver() )
   return( is->get_str_par( str_par_is( par ) ) );
  else
   return( C05Function::get_dflt_str_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< int > & get_vint_par( idx_type par )
  const override {
  if( auto is = inner_Solver() )
   return( is->get_vint_par( vint_par_is( par ) ) );
  else
   return( C05Function::get_dflt_vint_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< double > & get_vdbl_par( idx_type par )
  const override {
  if( auto is = inner_Solver() )
   return( is->get_vdbl_par( vdbl_par_is( par ) ) );
  else
   return( C05Function::get_dflt_vdbl_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::vector< std::string > & get_vstr_par( idx_type par )
  const override {
  if( auto is = inner_Solver() )
   return( is->get_vstr_par( vstr_par_is( par ) ) );
  else
   return( C05Function::get_dflt_vstr_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  if( name == "intInnrSlvr" )
   return( intInnrSlvr );
  if( name == "intNoSol" )
   return( intNoSol );
  if( name == "intChkState" )
   return( intChkState );

  if( auto is = inner_Solver() )
   return( int_par_lbf( is->int_par_str2idx( name ) ) );
  else
   return( C05Function::int_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type dbl_par_str2idx( const std::string & name )
  const override {
  if( auto is = inner_Solver() )
   return( dbl_par_lbf( is->dbl_par_str2idx( name ) ) );
  else
   return( C05Function::dbl_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type str_par_str2idx( const std::string & name )
  const override {
  if( auto is = inner_Solver() )
   return( str_par_lbf( is->str_par_str2idx( name ) ) );
  else
   return( C05Function::str_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vint_par_str2idx( const std::string & name )
  const override {
  if( auto is = inner_Solver() )
   return( vint_par_lbf( is->vint_par_str2idx( name ) ) );
  else
   return( C05Function::vint_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vdbl_par_str2idx( const std::string & name )
  const override {
  if( auto is = inner_Solver() )
   return( vdbl_par_lbf( is->vdbl_par_str2idx( name ) ) );
  else
   return( C05Function::vdbl_par_str2idx( name ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] idx_type vstr_par_str2idx( const std::string & name )
  const override {
  if( auto is = inner_Solver() )
   return( vstr_par_lbf( is->vstr_par_str2idx( name ) ) );
  else
   return( C05Function::vstr_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars =
   { "intInnrSlvr", "intNoSol" , "intChkState" };

  if( idx == intInnrSlvr )
   return( pars[ 0 ] );
  if( idx == intNoSol )
   return( pars[ 1 ] );
  if( idx == intChkState )
   return( pars[ 2 ] );

  if( auto is = inner_Solver() )
   return( is->int_par_idx2str( int_par_is( idx ) ) );
  else
   return( C05Function::int_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & dbl_par_idx2str( idx_type idx )
  const override {
  if( auto is = inner_Solver() )
   return( is->dbl_par_idx2str( dbl_par_is( idx ) ) );
  else
   return( C05Function::dbl_par_idx2str( idx ) );
  }

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & str_par_idx2str( idx_type idx )
  const override {
  if( auto is = inner_Solver() )
   return( is->str_par_idx2str( str_par_is( idx ) ) );
  else
   return( C05Function::str_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vint_par_idx2str( idx_type idx )
  const override {
  if( auto is = inner_Solver() )
   return( is->vint_par_idx2str( dbl_par_is( idx ) ) );
  else
   return( C05Function::vint_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vdbl_par_idx2str( idx_type idx )
  const override {
  if( auto is = inner_Solver() )
   return( is->vdbl_par_idx2str( vdbl_par_is( idx ) ) );
  else
   return( C05Function::vdbl_par_idx2str( idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 [[nodiscard]] const std::string & vstr_par_idx2str( idx_type idx )
  const override {
  if( auto is = inner_Solver() )
   return( is->vstr_par_idx2str( vstr_par_is( idx ) ) );
  else
   return( C05Function::vstr_par_idx2str( idx ) );
  }

/*--------------------------------------------------------------------------*/
 /// get the whole set of parameters in one blow
 /** Mostly, this method has to fetch the :BlockConfig and :BlockSolverConfig
  * from the inner Block to fill the f_extra_Configuration field of the
  * ComputeConfig. If an appropriate
  *
  *     SimpleConfiguration< std::pair< Configuration * , Configuration * > >
  *
  * is passed as the f_extra_Configuration of the provided non-nullptr
  * \p ocfg, then
  *
  *     f_extra_Configuration->f_value.first
  *
  * is used as the :BlockSolverConfig, and
  *
  *     f_extra_Configuration->f_value.second
  *
  * is used as the :BlockConfig (note that exception will be thrown if
  * f_extra_Configuration or its two inner Configuration are not of the right
  * type). Otherwise the get_right_Block*Config() methods of OCRBlockConfig
  * and RBlockSolverConfig are used to get() the "smallest possible types" of
  * :BlockConfig and :BlockSolverConfig, which may still result in an overall
  * nullptr to be returned (although a functioning LagBFunction does need at
  * least a Solver registered to the inner Block and the cleanest way to have
  * this is to use set_ComputeConfig(), so one would expect
  * f_extra_Configuration *not* to be "empty" in the end). */

 ComputeConfig * get_ComputeConfig( bool all = false ,
				    ComputeConfig * ocfg = nullptr )
  const override;

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS FOR HANDLING THE State OF THE LagBFunction -----------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the State of the LagBFunction
 *  @{ */

 State * get_State( void ) const override;

/*--------------------------------------------------------------------------*/

 void put_State( const State & state ) override;

/*--------------------------------------------------------------------------*/

 void put_State( State && state ) override;

/*--------------------------------------------------------------------------*/

 void serialize_State( netCDF::NcGroup & group ,
		       const std::string & sub_group_name = "" )
  const override;

/** @} ---------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LagBFunction; this is the actual concrete implementation exploiting the
 * vector lag_p of Lagrangian pairs.
 * @{ */

 Index get_num_active_var( void ) const override {
  return( LagPairs.size() );
  }

/*--------------------------------------------------------------------------*/

 Index is_active( const Variable * const var ) const override;

/*--------------------------------------------------------------------------*/

 void map_active( c_Vec_p_Var & vars , Subset & map ,
		  const bool ordered = false ) const override;

/*--------------------------------------------------------------------------*/

 Subset map_index( const std::vector< Variable * > & vars , c_Subset & nms )
  const override {
  if( vars.size() != nms.size() )
   throw( std::invalid_argument( "vars and nms sizes do not match" ) );

  Subset map( vars.size() );
  if( map.empty() )
   return( map );

  auto nmsit = nms.begin();
  auto varsit = vars.begin();
  auto mapit = map.begin();

  // for all Variable in the set
  while( varsit != vars.end() ) {
   auto var = *(varsit++);  // next variable
   auto oi = *(nmsit++);    // its original index
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = LagPairs[ oi ].first;
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = LagPairs[ --i ].first;
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = LagPairs.size() , avoi = LagPairs[ --i ].first ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = LagPairs[ --i ].first;
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Subset )

/*--------------------------------------------------------------------------*/

 Subset map_index( const std::vector< Variable * > & vars , Range rng )
  const override {
  if( vars.size() != rng.second - rng.first )
   throw( std::invalid_argument( "vars and rng sizes do not match" ) );

  Subset map( vars.size() );
  if( map.empty() )
   return( map );

  auto varsit = vars.begin();
  auto mapit = map.begin();

  // for all Variable in the set
  for( auto oi = rng.first ; oi < rng.second ; ++oi ) {
   auto var = *(varsit++);  // next variable
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = LagPairs[ oi ].first;
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = LagPairs[ --i ].first;
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = LagPairs.size() , avoi = LagPairs[ --i ].first ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = LagPairs[ --i ].first;
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Range )


/*--------------------------------------------------------------------------*/

 Variable * get_active_var( Index i ) const override {
  return( LagPairs[ i ].first );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override {
  return( new LagBFunction::v_iterator( LagPairs.begin() ) );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 v_const_iterator * v_begin( void ) const override {
  return( new LagBFunction::v_const_iterator( LagPairs.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override {
  return( new LagBFunction::v_iterator( LagPairs.end() ) );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 v_const_iterator * v_end( void ) const override {
  return( new LagBFunction::v_const_iterator( LagPairs.end() ) );
  }

/*--------------------------------------------------------------------------*/
/*-------------------------------- FRIENDS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class LagBFunctionState;  // make LagBFunctionState friend

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 *  @{ */

// delete all the Lagrangian terms (and the ColVariable with them)

 void clear_lp( void ) {
  for( const auto & dp : LagPairs )
   delete dp.second;
  LagPairs.clear();
  f_Lc = -1;
  }

/*--------------------------------------------------------------------------*/

 bool flush_v_tmpCP( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/ 

 void add_to_CostMatrix( v_c_dual_pair & newdp );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/ 

 void mod_CostMatrix( Index i , Index first );

/*--------------------------------------------------------------------------*/

 void init_CC( void );

/*--------------------------------------------------------------------------*/

 void guts_of_destructor( bool deleteinner = true );

/*--------------------------------------------------------------------------*/

 OFValue get_objective_value( Block * blck ) {
  OFValue ret = 0;
  for( auto sb : blck->get_nested_Blocks() ) {
   if( auto ro = dynamic_cast< RealObjective * >( sb->get_objective() ) ) {
    ro->compute();
    ret += ro->value();
    }
   ret += get_objective_value( sb );
   }
  return( ret );
  }

/*--------------------------------------------------------------------------*/
 /** handle every Modification, comprised GroupModification. returns true if
  * the global pool has to be checked for feasibility. */

 char guts_of_add_Modification( p_Mod mod , ChnlName chnl );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /** handle only "basic" Modification, i.e., no GroupModification. returns
  * true if the global pool has to be checked for feasibility. */

 char guts_of_guts_of_add_Modification( p_Mod mod , ChnlName chnl );

/*--------------------------------------------------------------------------*/

 void triple_to_pair( const v_coeff_triple & orig , v_coeff_pair & vc ) {
  vc.resize( orig.size() );
  for( Index i = 0 ; i < orig.size() ; ++i ) {
   vc[ i ].first = std::get< 0 >( orig[ i ] );
   vc[ i ].second = std::get< 1 >( orig[ i ] );
   }
  }

/*--------------------------------------------------------------------------*/

 void update_CostMatrix_ModLinRngd( const v_coeff_pair & rc ,
				    c_Vec_p_Var & vars , c_Range & rng );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void update_CostMatrix_ModLinSbst( const v_coeff_pair & rc ,
				    c_Vec_p_Var & vars , c_Subset & sbst );

/*--------------------------------------------------------------------------*/

 void update_CostMatrix_ModVarsAddd( c_Vec_p_Var & vars , Index first );

/*--------------------------------------------------------------------------*/

 void update_CostMatrix_ModVarsRngd( c_Vec_p_Var & vars , c_Range & rng );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void update_CostMatrix_ModVarsSbst( c_Vec_p_Var & vars , c_Subset & sbst );

/*--------------------------------------------------------------------------*/
 /// reset the BlockConfig of the inner Block to the default one

 void set_default_inner_BlockConfig( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// reset the BlockSolverConfig of the inner Block to the default one

 void set_default_inner_BlockSolverConfig( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// reset the configuration of the inner Block to the default one
 /** Reset both the BlockConfig and the BlockSolverConfig of the inner Block
  * to the default ones. */

 void set_default_inner_Block_configuration( void ) {
  set_default_inner_BlockSolverConfig();
  set_default_inner_BlockConfig();
  }

/*--------------------------------------------------------------------------*/

 void update_f_max_glob( void ) {
  while( f_max_glob && ( ! g_pool[ f_max_glob - 1 ].first ) )
   --f_max_glob;
  }

/*--------------------------------------------------------------------------*/

 template< typename par_type >
 void add_par( std::string && name , par_type && value );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 template< typename par_type >
 void add_par( const std::string & name , par_type && value ) {
  add_par( std::string( name ) , std::move( value ) );
  }

/*--------------------------------------------------------------------------*/

 Solver * inner_Solver( void ) const {
  if( ( ! p_InnrSlvr ) && ( ! v_Block.empty() ) ) {
   Solver * iS;
   auto & rs = v_Block.front()->get_registered_solvers();
   if( rs.empty() )
    throw( std::logic_error( "LagBFunction: no Solver attached to inner Block"
			     ) );
   if( rs.size() > InnrSlvr ) {
    auto rsit = rs.begin();
    std::advance( rsit , InnrSlvr );
    iS = *rsit;
    }
   else
    iS = rs.back();

   // note the horribly dirty trick of casting away const-ness from this
   // to allow inner_Solver() to be const and therefore used in const methods
   const_cast< LagBFunction * >( this )->p_InnrSlvr = iS;
   }
  return( p_InnrSlvr );
  }

/*--------------------------------------------------------------------------*/

 void compute_Lipschitz_constant( void );
 
/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 LinearFunction * obj;  ///< the (LinearFunction inside the) Objective of (B)

 DQuadFunction * qobj;  ///< the (DQuadFunction inside the) Objective of (B)

 bool IsConvex;         ///< true if the LagBFunction is convex

 Index InnrSlvr;        ///< [index of the] Solver of the inner Block

 bool NoSol;            ///< true if no Solution are stored

 bool ChkState;         ///< true if the State is checked for correctness

 Solver * p_InnrSlvr;   ///< [pointer to the] Solver of the inner Block

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_dual_pair LagPairs;  ///< vector of Lagrangian dual pairs
                        /**< LagPairs has an element j for each active
			 * ColVariable y[ j ] of the LagBFunction:
  * LagPairs[ j ].first contains (a pointer to) y[ j ], while
  * LagPairs[ j ].second contains (a pointer to) a LinearFunction that
  * contains the Lagrangian term g_i(x) = A_i x + b_i. Note that the
  * LagBFunction is the Observer of all these LinearFunction. */

 v_gpool_el g_pool;     ///< the global pool
                        /**< g_pool has the size of the global pool;
			 * g_pool[ i ].first is (a pointer to) the Solution
 * corresponding to the i-th linearization of the global pool, or nullptr
 * if there is no such linearization, while g_pool[ i ].second is a bool
 * telling if the linearization is diagonal. If NoSol == true,
 * g_pool[ i ].first contains "any" non-nullptr when the linearization is
 * there. */

 Index f_max_glob;      ///< 1 + maximum active name in the global pool
                        /**< f_max_glob is strictly larger than the maximum
			 * index h such that g_pool[ h ].first != nullptr,
  * i.e., g_pool[ f_max_glob ].first == nullptr while
  * g_pool[ f_max_glob - 1 ] != nullptr. Note that one should never check
  * g_pool[ f_max_glob ], as f_max_glob == g_pool.size() may happen (in
  * particular when g_pool.empty() and f_max_glob == 0). */

 m_column CostMatrix;
 ///< the matrix < c_j , y A^j > used to update the Lagrangian cost vector
 /**< CostMatrix[ j ] contains the information < c_j , y A^j > that is
  * needed to construct the Lagrangian cost of x[ j ], the j-th ColVariable
  * in the LinearFunction of the inner Block. Therefore, CostMatrix HAS
  * THE SAME ORDERING AS THE ACTIVE Variable IN OBJ. At the beginning
  * CostMatrix has as many rows as there "naturally" are active Variable in
  * obj; if a Lagrangian term causes a nonzero Lagrangian cost to potentially
  * appear for an x[ j ] that originally had 0 coefficient in obj, x[ j ] is
  * added to the list of active Variable (at the bottom, as usual) and a new
  * row is added to CostMatrix (at the bottom). The linear term y A^j is
  * represented by a v_mod_pair, where each mod_pair < i , a_{ij} > is the
  * *index* of y_i (as active Variable in the LagBFunction) with its
  * coefficient. Note that THE VECTOR IS KEPT ORDERED BY INDEX.
  *
  * However, it is important to remark that CostMatrix CAN BE LONGER THAN THE
  * NUMBER OF ACTIVE [Col]Variable IN OBJ. This is because THE "STEALTH"
  * INSERTION OF [Col]Variable IN OBJ IS "BATCHED" IN compute(), which means
  * that until compute() is called, there can be elements of CostMatrix[ i ]
  * even for i >= obj->get_num_active_var(). However, these corresponds to
  * variables (and their coefficients) stored in v_tmpCP in the same order,
  * i.e., CostMatrix[ i ] corresponds to
  * v_tmpCP[ i - obv->get_num_active_var() ]. */

 v_coeff_pair v_tmpCP;  ///< temporary for coefficients to be re-added
 /**< If [Col]Variable are removed from the objective of the inner Block that
  * appear in any Lagrangian term, they must be "stealthily" be re-added.
  * This is done in compute() just before the Lagrangian costs are recomputed,
  * but doing this requires keeping information about what stuff should be
  * added. This vector contains the coeff_pair( ColVariable * , double ) to
  * be inserted in the objective, but note that the corresponding Lagrangian
  * terms are already stored in CostMatrix in the same order, i.e.,
  * v_tmpCP[ i ] corresponds to CostMatrix[ i + obj->get_num_active_var() ].
  */
 
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index LastSolution;  ///< the last Solution read by get_linearization
                      /**< the "name" of the Solution currently in the
		       * Variable of the inner Block:
 * - Inf : the Solution is the last one from the local pool
 *
 * - \in [ 0 , g_pool.size() ): is the index of a Solution of the global pool
 *
 * - \in [ g_pool.size() , INF ): the Solution is not significant
 *
 * It of course only makes sense if NoSol == false. */

 bool VarSol;         ///< true if Variable in the inner Block are a solution
                      /**< true if the values of the Variable in the inner
		       * Block describe a feasible solution, i.e., they
 * correspond to a diagonal linearization; false if the values of the 
 * Variable in the inner Block describe a direction of the feasible set,
 * i.e., they correspond to a vertical linearization. It of course only makes
 * sense if NoSol == false. */

 LinearCombination zLC;
 ///< the LinearCombination of the important linearization

 FunctionValue f_yb;  ///< the yb term of the Lagrangian function
                      /**< The yb term  of the Lagrangian function, or what
		       * need to be done to compute it:
  * - -INF: b == 0, hence the term is 0
  *
  * - +INF: have to check whether b == 0 or not
  *
  * -  NaN: b != 0 and yb have to be computed
  *
  *  - any other value: the correct value of yb */

 bool f_play_dumb;    ///< true if self-inflicted Modification are ignored
                      /**< Since LagBFunction modifies the objective of the
		       * inner Block (by both changing costs and adding
  * [Col]Variable to it) it causes Modification to be issued. LagBFunction
  * itself must ignore these, but it is in general not possible to avoid them
  * being issued in the first place because the Solver attached to the inner
  * Block need them. This field is used in add_Modification() to know that a
  * Modification coming from the inner Block must be ignored. Note that this
  * is in general dangerous in a parallel context since another thread may be
  * independently modifying the inner Block, but LagBFunction ONLY SETS
  * f_play_dumb == true WHEN THE INNER Block IS LOCKED, which avoids any
  * such problem. */

 bool f_dirty_Lc;     ///< true if Lagrangian costs have to be modified

 bool f_c_changed;    ///< true if the costs in the Block are not original

 FunctionValue f_Lc;  ///< the Lipschitz constant

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 int LPMaxSz;         ///< maximum size of the "local pool"

 BlockSolverConfig * f_BSC;  ///< a BlockSolverConfig for the inner Block

 ComputeConfig * f_CC;       ///< a ComputeConfig for the inner Solver

 bool f_CC_changed;          ///< true if the ComputeConfig has changed

 BoxSolver * f_BS;    ///< the BoxSolver to compute the Lipschitz constant
 
 void * f_id;         ///< the "identity" of the LagBFunction 

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;  // insert LagBFunction in the Block factory

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunction ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LagBFunctionMod --------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a LagBFunction
/** Derived class from C0FunctionMod to describe modifications to a
 * LagBFunction. This obviously "keeps the same interface" as C0FunctionMod,
 * so that it can be used by Solver and/or Block just relying on the
 * C0Function interface, but it also add LagBFunction-specific information,
 * so that Solver and/or Block can actually react in LagBFunction-specific
 * if they want to.
 *
 * The reason for having this LagBFunction-specific information is that
 * LagBFunction "filters away a lot of information" about what happens in the
 * inner Block. In particular, a very large number of different changes in the
 * inner Block results in the same C05FunctionMod (now LagBFunctionMod) with
 * type() == GlobalPoolRemoved because they mess up with feasibility of the
 * previous solutions / directions. A Solver and/or Block knowing they are
 * dealing with a LagBFunction may want to know more about what happened in
 * order to react more appropriately.
 *
 * A very "aggressive" way to do that would be to keep inside the
 * LagBFunctionMod a (smart) pointer to the original Modification, but that
 * would require the Solver and/or Block to delve too deep in the details of
 * what has happened. The current chosen middle ground is to add a simple
 * bit-coded integer field what(), whose bits have the following meaning (if
 * they are set to 1):
 *
 *   0 = the objective of the inner Block or of some sub-Block of the inner
 *       Block has changed; this results in type() == AlphaChanged if the
 *       (say) coefficients have changed and/or Variable have been added or
 *       removed, while type() == NothingChanged if the constant term in the
 *       objective (if any) has changed so that the function values have only
 *       been shifted by a constant.
 *
 *   1 = the LHS/RHS of some Constraint in the inner Block (possibly
 *       comprised the bounds on the ColVariable implemented via the
 *       OneVarConstraint, but not the "inherent" ones) have changed.
 *
 *   2 = the coefficients in some of the Constraint in the inner Block have
 *       changed.
 *
 *   3 = some Variable in the inner Block has changed state (fixed/unfixed,
 *       changed type ...) in a way that can take away feasibility
 *
 *   4 = some previously relaxed Constraint in the inner Block has been
 *       enforced (relaxing cannot take away feasibility and therefore is
 *       not an issue)
 *
 *   5 = some dynamic Variable has been removed or some dynamic Constraint
 *       has been added (it is assumed that adding Variable or removing
 *       Constraint cannot take away feasibility and therefore is not an
 *       issue)
 *
 *   6 = some other arbitrary change in a Block (a BlockMod), that can
 *       therefore take away feasibility
 *
 * Note that a single LagBFunctionMod may have more than one bit set to 1
 * (and surely it has at least one of them), since multiple original
 * Modification coming from the inner Block and "bunched together" in the
 * same GroupModification produce a single LagBFunctionMod. Indeed,
 *
 *     IF MANY CHANGES ARE MADE TO THE INNER Block., IT IS MOST DEFINITELY
 *     A GOOD IDEA TO BUNCH AS MANY AS POSSIBLE OF THE CORRESPONDING
 *     Modification INTO AN AS SMALL AS POSSIBLE NUMBER OF GroupModification
 *
 *   in order to reduce the number of produced LagBFunctionMod. */

class LagBFunctionMod : public C05FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: that of C05FunctionMod plus the what() value
 /** Constructor: like that of C05FunctionMod takes a pointer to the affected
  * LagBFunction, the type of the Modification, the Subset of affected
  * linearizations, the value of the shift, and the "concerns Block" value.
  * However, it also takes the \p what value that specifies how the inner
  * Block of the LagBFunction has changed, whose default value is
  * "everything". */

 LagBFunctionMod( LagBFunction * f , int type , Subset && which = {} ,
		  char what = 127 , FunctionValue shift = NaNshift ,
		  bool cB = true )
  : C05FunctionMod( f , type , std::move( which ) , shift , cB ) ,
    f_what( what ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~LagBFunctionMod() { }  ///< destructor: does nothing

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the what value

 char what( void ) const { return( f_what ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the LagBFunctionMod

 void print( std::ostream &output ) const override {
  output << "LagBFunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] with what() = " << what() << " on LagBFunction ["
	 << &f_function << " ]: ";
  guts_of_print( output );
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 char f_what;  ///< the "what" value

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunctionMod ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS LagBFunctionState -------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe the "internal state" of a LagBFunction
/** Derived class from State to describe the "internal state" of a
 * LagBFunction, i.e., its global pool. This means savng the stored Solution
 * (and their type). */

class LagBFunctionState : public State {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*------------ CONSTRUCTING AND DESTRUCTING LagBFunctionState --------------*/

 /// constructor, doing everything or nothing.
 /** Constructor of LagBFunctionState. If provided with a pointer to a
  * LagBFunction it immediately copies its "internal state", which is the
  * only way in which the LagBFunctionState can be initialised out of an
  * existing LagBFunction. If nullptr is passed (as by default), then an
  * "empty" LagBFunctionState is constructed that can only be filled by
  * calling deserialize(). */

 LagBFunctionState( const LagBFunction * lbf = nullptr ) : State() {
  if( ! lbf ) {
   f_max_glob = 0;
   return;
   }  
  if( lbf->NoSol )
   f_max_glob = 0;
  else {
   f_max_glob = lbf->f_max_glob;
   g_pool.resize( f_max_glob );
   auto gpit = lbf->g_pool.begin();
   for( auto & el : g_pool ) {
    if( gpit->first )
     el.first = gpit->first->clone();
    else
     el.first = nullptr;
    el.second = gpit->second;
    ++gpit;
    }
   }
  zLC = lbf->zLC;
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a LagBFunctionState out of netCDF::NcGroup
 /** De-serialize a LagBFunctionState out of netCDF::NcGroup; see
  * LagBFunctionState::serialize() for a description of the format. */

 void deserialize( const netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// destructor

 virtual ~LagBFunctionState() {
  for( auto el : g_pool )
   delete el.first;
  }

/*--------- METHODS DESCRIBING THE BEHAVIOR OF A LagBFunctionState ---------*/

 /// serialize a LagBFunctionState into a netCDF::NcGroup
 /** The method serializes the LagBFunctionState into the provided
  * netCDF::NcGroup, so that it can later be read back by deserialize().
  *
  * After the call, \p will contain:
  *
  * - The dimension "LagBFunction_MaxGlob" containing 1 + the maximum active
  *   name in the global pool; this means that there can be only
  *   LagBFunction_MaxGlob nonempty entries in the global pool, and the
  *   largest possible name of an active entry is LagBFunction_MaxGlob - 1.
  *   The variable is optional, if not present then 0 (empty global pool) is
  *   assumed.
  *
  * - The variable "LagBFunction_Type", of type netCDF::NcByte and indexed
  *   over the dimension LagBFunction_MaxGlob, which contains the vector of
  *   booleans specifying the type (solution/direction) of each element in
  *   the global pool. The variable is optional if LagBFunction_MaxGlob == 0.
  *
  * - At most LagBFunction_MaxGlob netCDF::NcGroup with name
  *   "LagBFunction_Sol_X", with X an integer between 0 and
  *   LagBFunction_MaxGlob - 1, each one containing the deserialization of
  *   the Solution in the corresponding position of the global pool. If the
  *   group ".._X" is not there, then position X in the global pool is empty
  *
  * - The dimension "LagBFunction_ImpCoeffNum" containing the number of
  *   elements of the important coefficients. The dimension is optional, if
  *   it is not provided than 0 (no important coefficients) is assumed.
  *
  * - The variable "LagBFunction_ImpCoeffInd", of type netCDF::NcInt and
  *   indexed over the dimension LagBFunction_ImpCoeffNum, which contains
  *   the vector of indices of the important coefficients. The variable is
  *   optional if LagBFunction_ImpCoeffNum == 0.
  *
  * - The variable "LagBFunction_ImpCoeffVal", of type netCDF::NcDouble and
  *   indexed over the dimension LagBFunction_ImpCoeffNum, which contains
  *   the vector of real values of the important coefficients. The variable
  *   is optional if LagBFunction_ImpCoeffNum == 0.*/

 void serialize( netCDF::NcGroup & group ) const override;

/*-------------------------------- FRIENDS ---------------------------------*/

 friend class LagBFunction;  // make LagBFunction friend

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 void print( std::ostream &output ) const override {
  output << "LagBFunctionState [" << this << "] with max global pool element "
	 << f_max_glob;
  }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 LagBFunction::Index f_max_glob;      ///< 1 + maximum active name

 LagBFunction::v_gpool_el g_pool;     ///< the global pool

 LagBFunction::LinearCombination zLC;
 ///< the LinearCombination of the important linearization

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunctionState ) )

/** @} end( group( LagBFunction_CLASSES ) ) --------------------------------*/
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LagBFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LagBFunction.h ----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class PolyhedralFunctionBlock, which derives from
 * AbstractBlock to define the class of Block who have the specific structure
 * of having a PolyhedralFunction as objective, but otherwise can contain any
 * kind of Variable and Constraint (provided these are handled by the base
 * AbstractBlock class).
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __PolyhedralFunctionBlock
 #define __PolyhedralFunctionBlock
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"

#include "ColVariable.h"

#include "LinearFunction.h"

#include "FRealObjective.h"

#include "FRowConstraint.h"

#include "OneVarConstraint.h"

#include "PolyhedralFunction.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

class BlockSolverConfig;  // forward declaration

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS PolyhedralFunctionBlock -----------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an AbstractBlock whose distinguishing feature is a PolyhedralFunction
/** The class PolyhedralFunctionBlock derives from AbstractBlock to define
 * the following concept: a Block whose sole distinguishing feature is a
 * PolyhedralFunction as objective, but which can otherwise contain any kind
 * of "abstract" Variable and Constraint (provided these are handled by the
 * base AbstractBlock class).
 *
 * The rationale for the PolyhedralFunctionBlock class is that a
 * PolyhedralFunction is a perfectly fine object in itself, but one that
 * several solver cannot easily deal with in its "natural" form. However, a
 * PolyhedralFunction can also be represented in a very natural way by means
 * of some extra continuous ColVariable plus a finite set of (dynamic)
 * FRowConstraint with LinearFunction inside (a.k.a., linear constraints).
 * Thus, the main feature that PolyhedralFunctionBlock implements is the
 * ability to "present" itself (construct an "abstract representation") as
 * either having a FRealObjective with a PolyhedralFunction inside, or having
 * a set of (continuous) ColVariable and (linear) RowConstraint. We call the
 * first the "natural representation" of a PolyhedralFunctionBlock, and the
 * latter its "linearized representation".
 *
 * Indeed, minimizing a convex PolyhedralFunction is equivalent to the
 * linear program
 * \f[
 *   \min_{x,v} \{ v :
 *                 v \ge a_i x + b_i        \quad i \in B_D,
 *                 a_j x + b_j \le 0        \quad j \in B_V
 *              \}
 * \f]
 * since this is the epigraph formulation of
 * \f[
 *     pf(x) = \max \{ a_i x + b_i : i \in B_D \}
 * \f]
 * over the domain
 * \f[
 *     a_j x + b_j \le 0 \quad j \in B_V.
 * \f]
 * Equivalently, maximizing a concave PolyhedralFunction is equivalent to the
 * linear program
 * \f[
 *     \max_{x,v} \{ v :
 *          v \le a_i x + b_i        \quad i \in B_D,
 *          a_j x + b_j \le 0        \quad j \in B_V
 *     \}
 * \f]
 * since this is the hypograph formulation of
 * \f[
 *     pf(x) = \min \{ a_i x + b_i : i \in B_D \}
 * \f]
 * over the domain
 * \f[
 *     a_j x + b_j \le 0 \quad j \in B_V.
 * \f]
 * In the convex case the PolyhedralFunction is the pointwise maximum of a
 * finite set of affine functions, while in the concave case it is the
 * pointwise minimum of a finite set of affine functions. Here, B_D and B_V
 * represent respectively the sets of the so-called "diagonal" and "vertical"
 * linearizations. Even though here they are considerate as separate set, in
 * the current implementation of PolyhedralFunction, they are contained in the
 * unique pool of linearizations, and an appropriate vector is used to mark
 * the vertical ones. In fact, whenever there is no need to distinguish
 * between them we will just use B = B_D \cup B_V.
 * 
 * Note that above
 *
 *     x IS FIXED AND v IS THE ONLY Variable
 *
 * This underlines the fact that
 *
 *     PolyhedralFunctionBlock IS NOT NATURALLY USED AS A STAND-ALONE
 *     Block, BECAUSE THE INPUT ("ACTIVE") ColVariable OF ITS
 *     PolyhedralFunction ARE NOT Variable OF THE PolyhedralFunctionBlock
 *
 * Thus, the standard use of a PolyhedralFunctionBlock is as a sub-Block of
 * some other Block. This is not strictly necessary, because actually the
 * x ColVariable can be in the "arbitrary part" of the AbstractBlock (from
 * which PolyhedralFunctionBlock derives). However, the point is that
 *
 *     THE x ColVariable ARE NOT MANAGED BY THE PolyhedralFunctionBlock,
 *     THIS BEING DEMANDED TO SOMETHING ELSE (see set_PolyhedralFunction())
 *
 * Thus, the "natural representation" and its "linearized representation"
 * differ regarding to which sets of Constraint, Variable and Objective are
 * "reserved" (see comments to AbstractBlock):
 *
 * - with the "natural representation", the Objective is reserved (since it
 *   must be a FRealObjective with a PolyhedralFunction inside), but nothing
 *   else is;
 *
 * - with the "linearized representation", the first group of static Variable
 *   contains a single ColVariable (v), the first group of dynamic Constraint
 *   contains the FRowConstraint (with LinearFunction inside, i.e.,
 *   v <op> a_i x + b_i OR a_j x + b_j \le 0, where <op> depends on if the
 *   PolyhedralFunction is convex or concave), and the  Objective is also
 *   reserved (since it must be a FRealObjective with another LinearFunction
 *   inside, having nonzero coefficient only for v).
 *   Note that there is a choice here about b_i: it could be set
 *
 *   = either as the constant of the LinearFunction inside the FRowConstraint;
 *
 *   = or as the LHS/RHS of the FRowConstraint (depending if the
 *     PolyhedralFunction is convex or concave).
 *
 *   The second choice is taken, i.e., the constant of the LinearFunction
 *   inside the FRowConstraint is always 0.
 *
 * One nontrivial issue in this setup is that, when the "linearized
 * representation" is used, it is necessary to:
 *
 * - "capture" the *FunctionMod* issued by the PolyhedralFunction and use
 *   them for properly changing the "linearized representation";
 *
 * - "capture" the Modification issued by elements in the "linearized
 *   representation" and use them for properly changing the
 *   PolyhedralFunction.
 *
 * Note that most individual changes in the PolyhedralFunction result in
 * many changes to the "linearized representation", that are properly bunched
 * into appropriate GroupModification. Conversely, many individual changes to
 * the "linearized representation" cannot (or would be too complex to) be
 * implemented in the PolyhedralFunction, because each one of them
 * individually would lead it to end in a partly inconsistent state, and only
 * a co-ordinated set of them (say, properly bunched into an appropriate
 * GroupModification) would work. Hence, a number of changes to the
 * "linearized representation" are not allowed; see the comments to the
 * protected method guts_of_add_Modification_LR() for details.
 * 
 * Other than that, PolyhedralFunctionBlock entirely relies on the machinery
 * proivided by AbstractBlock to handle all the rest of the Block, and
 * therefore is subject to the limitations of that class regarding what
 * kind of Constraint, Variable and Objective are supported. 
 *
 * ---------------------------------------------------------------------------
 * Dual (Fenchel Conjugate) Representation
 * ---------------------------------------------------------------------------
 *
 * In addition to the primal representation, the class can also be initialized
 * to represent the dual form of the polyhedral function, corresponding to its
 * Fenchel conjugate
 * \f[
 *  pf^*(z) = \sup_x \{ z * x - pf(x) \}
 * \f]
 * Given the (linearized) primal formulation above, the conjugate can be 
 * written as:
 * \f[
 *   pf^*(z) = 
 *   \max \left\{ -\sum_{i \in B} \theta_i b_i \;:\;
 *                \sum_{i \in B_D} \theta_i = 1 \;,\;
 *                \sum_{i \in B} \theta_i a_i = z
 *                \theta_i \geq 0 \quad i \in B
 *         \right\}
 * \f]
 * where the \f$\theta_i\f$ "dual variables" associated with each 
 * linearizations, be them "diagonal" or "vertical" (but only the "diagonal"
 * ones appear in the simplex constraint). This conjugate is a "pin
 * function", i.e., \f$ pf^*(z) \f$ is the minimum value of the convex
 * combination of the b_i corresponding to all possible conbinations of the
 * a_i that give z, if there is any, and +INF if z does not belong to the
 * convex hull of the a_i.
 *
 * Additionally, the PolyhedralFunction may include a global bound, namely a
 * lower bound in the convex case (or an upper bound in the concave case). In
 * the linearized primal representation, this simply appears as
 * \f[
 *   v \geq LB
 * \f]
 * This basically corresponds to a "flat" all-0 subgradient with a_i = 0 and
 * b_i = LB. Hence, the bound is reflected in the dual formulation through
 * the introduction of an additional variable, denoted by \f$\gamma\f$. The
 * dual problem is then modified as follows:
 *
 * - the objective function includes an additional term \f$ \gamma LB \f$;
 *
 * - the normalization constraint becomes
 *   \f[
 *     \sum_{i \in B_D} \theta_i + \gamma = 1
 *   \f]
 *
 * This follows the same structural principles as the primal case: the
 * "main" variables z lives "outside" the PolyhedralFunctionBlock, hence
 *
 *    THE PolyhedralFunctionBlock IS NOT INTENDED TO OPERATE IN ISOLATION
 *
 * In particular, besides the "main" variables z, the constraint
 * \f[
 *   \sum_{i \in B} \theta_i a_i = z
 * \f]
 * is *not* implemented internally as a standalone constraint of the
 * PolyhedralFunctionBlock. This is because, if the "outer" Block contains
 * (say) multiple PolyhedralFunctionBlocks within a larger model, this
 * becomes a coupling constraint due to the fact that "the conjugate of the
 * sum is the inf-convolution":
 * \f[
 *  [ f_1(x) + f_2(x) ]^* =
 *    \inf_{z_1, z_2} \{ f_1^*(z_1) + f_2^*(z_2) : z_1 + z_2 = z \}
 * \f]
 * Therefore, the class provides a dedicated method that receives a list of
 * external constraints and augments them with the corresponding terms 
 * \f[
 *  \sum_{i \in B} \theta_i a_i
 * \f]
 * so that the constraint linking this PolyhedralFunctionBlock (and,
 * possibly others) with the "main" z variables can be built in the father
 * Block that necessarily contains this (and possibly other)
 * PolyhedralFunctionBlock.
 *
 * ---------------------------------------------------------------------------
 * Internal Structures for the Dual Representation
 * ---------------------------------------------------------------------------
 *
 * When the block is initialized in its dual form, the following structures
 * are defined:
 *
 * - A first group of dynamic variables representing all the \f$\theta_i\f$,
 *   together with the additional static variable \f$\gamma\f$ (we expect it
 *   to be fixed to 0 when a bound is absent). 
 *   NOTE: the dual multipliers associated with the vertical and diagonal
 *   linearizations follows the same order of the corresponding rows in the
 *   primal representation. Hence, they will be mixed and not come one after
 *   the other.
 *
 * - An objective function given by a LinearFunction containing the terms
 *   \f$-\theta_i b_i\f$, and, if applicable, the term \f$\gamma LB\f$.
 *
 * - A single static constraint enforcing the normalization condition:
 *   \f[
 *     \sum_{i \in B_D} \theta_i + \gamma = 1
 *   \f]
 *   (which reduces to \f$\sum \theta_i = 1\f$ when no bound is present).
 *
 * The coupling constraint \f$\sum \theta_i a_i = z\f$ is handled
 * externally, as described above, to allow multiple
 * PolyhedralFunctionBlocks to share the same global constraint. */

class PolyhedralFunctionBlock : public AbstractBlock
{
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 using RealVector = PolyhedralFunction::RealVector;
 ///< "import" RealVector from PolyhedralFunction

 using c_RealVector = const RealVector;   ///< a const RealVector

 using MultiVector = PolyhedralFunction::MultiVector;
 ///< "import" MultiVector from PolyhedralFunction

 using c_MultiVector = const MultiVector;   ///< a const MultiVector

 using VarVector = PolyhedralFunction::VarVector;
 ///< "import" MultiVector from PolyhedralFunction

 using c_VarVector = const VarVector;
 ///< a const version of the x variables upon which the function depends

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------- CONSTRUCTING AND DESTRUCTING PolyhedralFunctionBlock -----------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing PolyhedralFunctionBlock
 *  @{ */

 /// constructor of PolyhedralFunctionBlock, taking a pointer to the father
 /** Constructor of PolyhedralFunctionBlock. It accepts a pointer to the
  * father Block (defaulting to nullptr, both because the root Block has no
  * father and so that this can also be used as the void constructor),
  * passes it to the Block constructor, and does little else. It constructs
  * an "empty" PolyhedralFunction to start with. */

 PolyhedralFunctionBlock( Block * father = nullptr )
  : AbstractBlock( father ) , f_rep( 0 ) , f_scaling( 0 ) ,
    f_no_objective( false ) , f_global_scale( 1.0 ) ,
    f_global_reference( 1.0 ) ,
    f_own_polyf( {} , {} , {} , -Inf< Function::FunctionValue >() , true , this
		 ) ,
    f_v() , f_const() { }

/*--------------------------------------------------------------------------*/
 /// constructor wrapping an *external* PolyhedralFunction
 /** Constructs a PolyhedralFunctionBlock that does not own its
  * PolyhedralFunction but operates on the given external \p polyf ( which
  * keeps its own Observer and is neither serialized nor destroyed by this
  * Block ). This is used to build a transient epigraph LP over an existing
  * PolyhedralFunction, e.g. in remove_redundant_rows(). */

 PolyhedralFunctionBlock( Block * father , PolyhedralFunction * polyf )
  : AbstractBlock( father ) , f_rep( 0 ) , f_scaling( 0 ) ,
    f_no_objective( false ) , f_global_scale( 1.0 ) ,
    f_global_reference( 1.0 ) ,
    f_own_polyf( {} , {} , {} , -Inf< Function::FunctionValue >() , true ,
		 nullptr ) ,
    f_v() , f_const() { if( polyf ) f_polyf_p = polyf; }

/*--------------------------------------------------------------------------*/
 /// load the PolyhedralFunctionBlock out of an istream
 /** Method to deserialize the PolyhedralFunctionBlock out of an istream.
  *
  *     IT IS CURRENTLY NOT IMPLEMENTED
  *
  * but it still have to be defined (throwing exception) to make the class
  * concrete. */

 void load( std::istream &input , char frmt = 0 ) override {
  throw( std::logic_error(
		     "PolyhedralFunctionBlock::load not implemented yet" ) );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize the current PolyhedralFunctionBlock out of netCDF::NcGroup
 /** The PolyhedralFunctionBlock de-serializes itself out of a
  * netCDF::NcGroup. Besides what is managed by the serialize() method of
  * the base Block class, the group should contain the following:
  *
  * - all the data necessary to describe a PolyhedralFunction; see
  *   PolyhedralFunction::serialize() for details;
  *
  * - any other data necessary to represent the "arbitrary" part of the
  *   AbstractBlock, see AbstractBlock::deserialize() for details. */

 void deserialize( const netCDF::NcGroup & group ) override
 {
  // have the PolyhedralFunction do all the dirty work for us
  // don't bother issuing individual Modification, since a NBModification will
  // anyway be issued soon (if anybody is listening)
  PF().deserialize( group , eNoMod );

  // the PolyhedralFunctionBlock is "naked": no abstract representaton
  f_rep = 0;
  f_scaling = 0;
  f_no_objective = false;
  f_global_scale = 1.0;
  f_global_reference = 1.0;
  f_row_scale.clear();
  f_row_measure.clear();
  f_lambda = nullptr;
  f_const.clear();

  // call the (guts_of version of the) base class method
  AbstractBlock::guts_of_deserialize( group );

  // call the method of Block
  // inside this the NBModification, the "nuclear option",  is issued

  Block::deserialize( group );
  }

/*--------------------------------------------------------------------------*/

#ifndef NDEBUG
 /// extends Block::expected_dims()

 std::vector< std::string > expected_dims( void ) const override {
  auto ret = AbstractBlock::expected_dims();
  auto pfev = PF().expected_dims();
  ret.insert( ret.end() , pfev.begin() , pfev.end() );
  return( ret );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Block::expected_vars()

 std::vector< std::string > expected_vars( void ) const override {
  auto ret = AbstractBlock::expected_vars();
  auto pfev = PF().expected_vars();
  ret.insert( ret.end() , pfev.begin() , pfev.end() );
  return( ret );
  }

#endif

/*--------------------------------------------------------------------------*/
 /// the destructor actually destroys the abstract representation
 /** The destructor of PolyhedralFunctionBlock (unlike that of Block, but
  * like that of AbstractBlock) takes care of (clear()-ing first, and)
  * destroying (then) all the "abstract" Constraint/Variable, the Objective
  * and the inner Block. This is actually done by the destructor of
  * AbstractBlock for the "arbitrary" part , while that of
  * PolyhedralFunctionBlock takes care of the PolyhedralFunction and of
  * all Variable and Constraint of the "linearized representation".
  *
  * Note that PolyhedralFunctionBlock does not assume to be a "leaf" class:
  * further derived classes can be implemented for structures like "a
  * PolyhedralFunction, some other specific stuff and then an "arbitrary
  * part". In this case, the deletion of the "other specific stuff" is due to
  * the destructur of the further derived class, while that of the "arbitrary
  * part" is due to that of AbstractBlock. */

 virtual ~PolyhedralFunctionBlock() { guts_of_destructor(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *
 * Note that PolyhedralFunctionBlock does not provide any specific method for
 * initializing or changing the PolyhedralFunction, since full access to it
 * is provided by get_PolyhedralFunction(), and one can use its methods to do
 * all required operations. PolyhedralFunctionBlock will "catch" all the
 * corresponding Modification and react accordingly, if needed.
 *
 *  @{ */

 /// generate the Variable in the abstract representation, primal or dual
 /** This method ensures that the "abstract representation" of the Variable
  * in the PolyhedralFunctionBlock is initialized, so that it can be read
  * with get_static_variables() and get_dynamic_variables(). The effect
  * changes depending on which representation is used. In fact,
  *
  *    THE CHOICE OF THE REPRESENTATION IS DONE PRECISELY IN THIS METHOD
  *
  * by means of the stvv parameter, which is interpreted as a
  * SimpleConfiguration< int > whose value is treated bit-wise:
  *
  * - bit 0 = 0 : the "natural representation" is used (the only abstract
  *               representation is the PolyhedralFunction itself, wrapped
  *               into a FRealObjective in generate_objective()); the value
  *               of bit 1 is irrelevant in this case;
  *
  * - bit 0 = 1, bit 1 = 0 : the "linearized primal representation" is
  *               used: a single ColVariable v is added as the first
  *               static Variable; see the original comments below;
  *
  * - bit 0 = 1, bit 1 = 1 : the "linearized dual representation"
  *               (the abstract form of the Fenchel conjugate / pin
  *               function) is used: see the notes below.
  *
  * The following two bits control optional numerical scaling of either
  * linearized representation:
  *
  * - bit 2 = 1 : local row scaling is enabled. Each row i receives the
  *               factor
  *               1 / sqrt( max( 1 , || A_i ||_inf , | b_i | ) );
  *
  * - bit 3 = 1 : global epigraph scaling is enabled. One shared factor is
  *               computed with the same formula, using the maximum over
  *               all rows. In the primal representation an extra internal
  *               ColVariable stores the scaled epigraph value. It is linked
  *               to the physical epigraph variable by an equality, so the
  *               scaling remains invisible outside this Block.
  *
  * The two scaling modes can be enabled independently or together. Local
  * factors are fixed when each row enters the representation. The global
  * factor is monitored in batches: PFB caches each row measure
  * max( 1 , || A_i ||_inf , | b_i | ), keeps their maximum, and changes the
  * shared factor to the inverse square root of this measure only when it
  * drifts by more than two orders of magnitude from the value used for the
  * previous scaling. When the factor changes, PFB preserves the existing
  * abstract Variable and Constraint objects and modifies only the affected
  * coefficients and row sides in place. Small fluctuations require no
  * abstract update at all.
  *
  * - bit 4 = 1 : the PolyhedralFunctionBlock is initialized without any
  *               objective function. This could be required by some
  *               particular classes that would like to handle directly
  *               the objective part.
  *
  * The chosen representation is then stored in the f_rep field (lowest
  * two bits) and never changed afterwards; this means that the parameters
  * of generate_abstract_constraints() and generate_objective() are plainly
  * ignored, and that this method has to be called before these.
  *
  * The value to be read into f_rep is determined as follows:
  *
  * - if stvv is not nullptr and it is a SimpleConfiguration< int >, then
  *   it is stvv->f_value;
  *
  * - otherwise, if f_BlockConfig is not nullptr,
  *   f_BlockConfig->f_static_variables_Configuration is not nullptr and it
  *   is a SimpleConfiguration< int >, then it is
  *   f_BlockConfig->f_static_variables_Configuration->f_value;
  *
  * - otherwise, 0 ("natural representation") is assumed.
  *
  * --------------------------------------------------------------------------
  * Linearized primal representation (the "01" choice)
  * --------------------------------------------------------------------------
  *
  * The first group of static Variable contains a single physical
  * ColVariable (v), and there are no extra dynamic Variable. With global
  * scaling enabled, a second static ColVariable contains the scaled
  * epigraph value used internally by the cuts. Note that "v" is added "in
  * front", so that even if the AbstractBlock has constructed some
  * "abstract" representation already (say, in deserialize()), "v" is still
  * the first group of static ColVariable. Classes derived from
  * PolyhedralFunctionBlock will have to be careful about this.
  *
  * Note that if further derived classes add some other structure, their
  * version of this method will have to call the method of this class
  * first, because it uses (if any) the *first* group of static Variable.
  *
  * --------------------------------------------------------------------------
  * Linearized dual representation (the "11" choice)
  * --------------------------------------------------------------------------
  *
  * The variable v is NOT generated. In its place we generate
  *
  * - one *non-negative* ColVariable \f$\theta_i\f$ per row of PF()
  *   (both diagonal and vertical, in the same order as in PF()), as
  *   the first group of *dynamic* Variable (f_theta);
  *
  * - one *non-negative* static ColVariable \f$\gamma\f$ as the first
  *   group of static Variable (f_gamma), playing the role of the dual
  *   multiplier of the global bound of PF(). If
  *   PF().is_bound_set() is false, then \f$\gamma\f$ is is_fixed-ed to
  *   0 to make sure it has no effect (the contribution to the
  *   normalization constraint is null and the contribution to the
  *   objective is null too, regardless of how the "ineffective" bound
  *   value gets handled). */

 void generate_abstract_variables( Configuration *stvv = nullptr ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// generate the Constraint in the "linearized representation"
 /** This method serves is to ensure that the "abstract representation" of
  * the Constraint, be they static or dynamic, of the PolyhedralFunctionBlock
  * is initialized, so that it can be read with get_static_constraints() and
  * get_dynamic_constraints(). Of course, the effect changes depending on
  * whether the "natural representation" or the "linearized representation"
  * are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE ELSEWHERE, PRECISELY IN
  *    generate_abstract_variables()
  *
  * and this method just assumes it has been done there and reads from the
  * f_rep field. This means that the stcc *Configuration is ignored, and so
  * is f_static_constraints_Configuration in the BlockConfig, if any.
  *
  * If f_rep == false, the PolyhedralFunctionBlock has no extra Constraint, be
  * them static or dynamic. If f_rep == true instead, then:
  *
  *  - The first group of dynamic Constraint contains a single
  *    std::list< FRowConstraint > with LinearFunction inside (a.k.a. "linear
  *    constraint") representing the m inequalities v >= [<=] a_i x + b_i
  *    OR a_j x + b_j <= 0.
  *    Note that the "verse" of the Constraint in the diagonal linearizations
  *    depend on PolyhedralFunction->is_convex(); if it is true than the 
  *    inequalities are ">=" (the LHS is -INF and the RHS is b_i), otherwise 
  *    they are "<=" (the LHS b_i and the RHS is INF).
  *
  *  - The first group of static Constraint contains a single BoxConstraint
  *    whose variable is "v", which serves to store the global lower/upper
  *    bound on the function vale. This clearly depends on
  *    PolyhedralFunction->is_convex(); if it is true, than the LHS is the
  *    global lower bound and the RHS is INF, otherwise the LHS is - INF and
  *    the RHS is the global upper bound.
  *
  *  - With global scaling enabled, the second group of static Constraint
  *    contains the internal equality scaled_v = global_scale * v. This
  *    keeps the scaling invisible to users of the physical v variable.
  *
  * Note that both groups of Constraint are added "in front" of their 
  * corresponding vector, so that even if the AbstractBlock has constructed
  * some "abstract" representation already (say, in deserialize()), they 
  * still are the first group of dynamic/static Constraint. Classes derived
  * from PolyhedralFunctionBlock will have to be careful about this. 
  * 
  * --------------------------------------------------------------------------
  * Dual Representation
  * --------------------------------------------------------------------------
  *
  * If f_rep & 3 == 3, then we initialize in this method only the static
  * constraint
  * \f[
  *  \sum_{i \in B_D} \theta_i + \gamma = 1.
  * \f]
  */

 void generate_abstract_constraints( Configuration *stcc = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// generate the Objective in the abstract representation, linearized or not
 /** This method serves is to ensure that the "abstract representation" of
  * the Objective of the PolyhedralFunctionBlock is initialized, so that it
  * can be read witt get_objective().Of course, the effect changes depending
  * on whether the "natural representation" or the "linearized representation"
  * are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE ELSEWHERE, PRECISELY IN
  *    generate_abstract_variables()
  *
  * and this method just assumes it has been done there and reads from the
  * f_rep field. This means that the *objc Configuration is ignored, and so is
  * is f_objective_Configuration in the BlockConfig, if any.
  *
  * If f_rep == false, the Objective of the PolyhedralFunctionBlock is a
  * FRealObjective having the PolyhedralFunction as Function. If f_rep ==
  * true the Objective of the PolyhedralFunctionBlock is still a
  * FRealObjective, but its Function is a LinearFunction having a single
  * nonzero coefficient (that of v, which is 1). Note that the "verse" of
  * the Objective depends on PolyhedralFunction->is_convex(); if it is true
  * then it is minimization, otherwise it is maximization. This is the
  * "natural verse", which is mandatory if the "linearised representation" is
  * used (because an LP can only represent convex or concave functions);
  * nonetheless, the verse can in principle be changed manually after that the
  * method is called (at your own risk). 
  * 
  * --------------------------------------------------------------------------
  * Dual Representation
  * --------------------------------------------------------------------------
  *
  * If f_rep & 3 == 3, then we crete the objective function with the
  * LinearFunction having as coeffcients -\theta_i b_i and \gamma LB. */

 void generate_objective( Configuration *objc = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// inject the external coupling multiplier into the normalization
 /// constraint of the dual representation
 /** In the standard dual representation of a PolyhedralFunctionBlock, the
  * normalization constraint has the form
  * \f[
  *   \sum_{i \in B_D} \theta_i + \gamma = 1 ,
  * \f]
  * where the internal \f$\gamma\f$ (this PolyhedralFunctionBlock's own
  * static ColVariable f_gamma, fixed to 0 if no bound is set) plays the
  * role of the dual multiplier of the global lower/upper bound of PF().
  *
  * When the PolyhedralFunctionBlock is used as one component of a larger
  * "inf-convolution" structure (typically: the father AbstractBlock packs
  * several PolyhedralFunctionBlock and has the coupling constraint
  * \f$\sum z_k = z\f$ over the z-variables, see set_conjugate_constraint),
  * and the father itself imposes a *global* lower (upper for concave)
  * bound on the sum of the v_k contributed by all the nested PFBs, the
  * dual of that global-LB constraint is a single shared variable
  * \f$\lambda\f$, owned by the father, that appears with coefficient
  * \f$+1\f$ in the simplex (= normalization) constraint of *every*
  * nested PolyhedralFunctionBlock. This is the LP-correct statement of
  * dual feasibility at the v_k column: both \f$\gamma\f$ (per-PFB LB)
  * and \f$\lambda\f$ (global LB) contribute a coefficient \f$+1\f$ to
  * v_k's column in the primal, so both appear on the LHS with
  * coefficient \f$+1\f$ in the dual:
  * \f[
  *   \sum_{i \in B_D} \theta_i + \gamma + \lambda = 1 .
  * \f]
  * Note that the variable passed to this method is not owned by the
  * PolyhedralFunctionBlock: it is expected to belong to the father block
  * (or another external block coupling several PolyhedralFunctionBlock).
  * Its objective coefficient (= the global LB) lives in the father's
  * own objective (not inside this PolyhedralFunctionBlock); when the
  * global LB is unset, the father simply fixes \f$\lambda\f$ to 0, and
  * the constraint reduces to \f$\sum \theta + \gamma = 1\f$.
  *
  * IMPORTANT NOTES:
  *
  * - this method must be called *after* generate_abstract_constraints()
  *   has built the normalization constraint (i.e. with the "dual
  *   representation" active);
  *
  * - calling this method more than once with the *same* lambda is a no-op
  *   (lambda is added only if not already present); each PolyhedralFunction-
  *   Block can however have multiple distinct lambda's registered, one
  *   per global-LB-style coupling it participates in. */

 void set_lambda( ColVariable * lambda );

/*--------------------------------------------------------------------------*/
 /// add this block's coupling terms to a list of external constraints
 /** In the dual representation of a PolyhedralFunctionBlock, the
  * "coupling" condition
  * \f[
  *   \sum_{i \in B} \theta_i a_i = z
  * \f]
  * (where the \f$a_i\f$'s are the rows of PF() and z is the "main"
  * coordinate w.r.t. which the conjugate is computed) is *not*
  * implemented internally. This design choice reflects the fact that
  * multiple PolyhedralFunctionBlock may coexist within the same parent
  * Block, sharing the same global z; the parent then maintains a list of
  * coupling constraints \f$\sum_{B \in blocks} \sum_{i \in B} \theta_i^B
  * a_i^B = z\f$, and each PolyhedralFunctionBlock adds its own
  * contribution to the parent's list.
  *
  * The list passed as input is assumed to already exist (i.e. it contains
  * one FRowConstraint per coordinate of z) and to be in 1:1
  * correspondence with the active Variable of PF(), that is:
  * \p constraints.size() must equal PF().get_num_active_var(), and the
  * j-th constraint (0-based) is the one corresponding to the j-th active
  * Variable of PF().
  *
  * The method augments each constraint by adding to its LinearFunction
  * the term \f$\theta_i \, a_{i,j}\f$ for every row i of PF() for
  * which \f$a_{i,j} \ne 0\f$.
  *
  * The variables \f$\theta_i\f$ involved in these terms are the dynamic
  * variables of the dual representation of the block, and they are
  * owned by this PolyhedralFunctionBlock.
  *
  * IMPORTANT NOTES:
  *
  * - this method must be called *after* generate_abstract_variables() has
  *   built the f_theta list (i.e. with the "dual representation" active);
  *
  * - this method is meant to be called *at most once*: the assumption is
  *   that the list of external constraints is set once and for all.
  *
  * - the LinearFunction of each provided FRowConstraint must already
  *   exist (so that this method can simply add the new coefficients to
  *   it) or be empty (so that this method creates it on the fly). */

 void set_conjugate_constraint( std::list< FRowConstraint > & constraints );

/** @} ---------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the PolyhedralFunctionBlock
 *  @{ */

 PolyhedralFunction & get_PolyhedralFunction( void ) { return( PF() ); }

/*--------------------------------------------------------------------------*/
 /// inhibit ( or restore ) the mirroring of the abstract representation
 /** Inhibits ( \p dumb == true ) or restores ( \p dumb == false, the default )
  * the mirroring of the abstract representation back onto the
  * PolyhedralFunction in add_Modification(). While "playing dumb", changes to
  * the Constraint / Objective of the abstract representation still reach any
  * attached Solver, but they are not reflected onto PF(). See f_play_dumb. */

 void set_play_dumb( bool dumb = true ) { f_play_dumb = dumb; }

/*--------------------------------------------------------------------------*/
 /// remove the redundant rows of the PolyhedralFunction
 /** Removes from the PolyhedralFunction all the rows that are redundant, i.e.,
  * that do not contribute to its value at any point of its domain. Letting
  * s = -1 if the PolyhedralFunction is convex and s = +1 if it is concave,
  * there are two kinds of redundant rows:
  *
  * - *parallel* ( dominated ) rows: if a_i == a_k and s b_k >= s b_i for some
  *   i != k, then row k is dominated by row i. These are removed geometrically
  *   by PolyhedralFunction::remove_parallel_rows(), which needs no Solver.
  *
  * - *inactive* rows: a row k that, although parallel to no other, never
  *   attains the pointwise maximum ( convex ) / minimum ( concave ). Deciding
  *   this needs an LP. Writing the convex PolyhedralFunction as the epigraph
  *   \f[
  *     \min \{ v : v \ge a_i x + b_i , \; i \in I \}
  *   \f]
  *   row k is useless exactly when
  *   \f[
  *     \min \{ v - ( a_k x + b_k ) : v \ge a_i x + b_i , \;
  *             i \in I \setminus \{ k \} \} \ge 0 ,
  *   \f]
  *   a negative ( possibly -INF ) optimum meaning that at some point row k
  *   lies strictly above all the others, so it cannot be removed; the concave
  *   case is symmetric.
  *
  * The LP is solved by the Solver configured by \p solver_config. Since the
  * "active" x of the PolyhedralFunction are not Variable of this Block ( they
  * belong to the parent model ), the LP is assembled in a transient
  * AbstractBlock holding fresh free x, inside which an inner
  * PolyhedralFunctionBlock shares this' PolyhedralFunction ( via the pointer
  * constructor ), so that its own linearized representation ( v and the
  * v >= a_i x + b_i rows ) is reused as the epigraph LP. Cycling over the
  * rows, the objective is aimed at row h and its own constraint is temporarily
  * relaxed; a redundant row is recorded and, at the end, delete_rows()-ed from
  * the PolyhedralFunction, so that the removal Modification reaches the parent
  * model. The inner Block "plays dumb" ( see set_play_dumb() ) so that the
  * relaxations are not mirrored back onto the shared PolyhedralFunction; the
  * inner Block and the AbstractBlock are built and torn down on each call.
  *
  * The four tolerances ( absolute and relative, for the parallel and for the
  * inactive test ) are read, like the other parameters of a
  * PolyhedralFunctionBlock, from this Block's BlockConfig: its "extra"
  * Configuration, a SimpleConfiguration< std::vector< double > > with up to
  * four entries [ parallel_abs, parallel_rel, optimization_abs,
  * optimization_rel ]; missing entries default to 0. */

 void remove_redundant_rows( BlockSolverConfig * solver_config );

 const PolyhedralFunction & get_PolyhedralFunction( void ) const
 { return( PF() ); }

/*--------------------------------------------------------------------------*/
 /// returns a pointer to the physical v variable (linearized primal rep)
 /** Returns &f_v, the auxiliary ColVariable that in the linearized
  * primal representation (is_linearized() == true) plays the role of
  * the epigraph variable, with f_v >= a_i x + b_i for every row of the
  * underlying PolyhedralFunction (convex case; concave case dually).
  * With global scaling enabled, cuts use an additional internal scaled
  * variable linked to f_v; the variable returned here always remains in the
  * physical units of the PolyhedralFunction. Meaningful only in the
  * linearized primal representation; in the
  * natural / linearized dual representation f_v is unused but the
  * pointer is still returned (the caller is responsible for not wiring
  * it into LinearFunction objects of an outer Block in those cases).
  * Designed for use by outer Block objects that want to "expose" f_v
  * as a master-side epigraph variable (cf. MasterProblemBlock primal
  * form), avoiding the need for a coupling row v_master = f_v. */

 [[nodiscard]] ColVariable * get_v( void ) { return( & f_v ); }

/*--------------------------------------------------------------------------*/
 /// returns the internal scale applied to the epigraph variable in the cuts
 /** This accessor is primarily useful for inspecting or directly modifying
  * the abstract representation. Code using get_v() does not need it:
  * get_v() always exposes the physical epigraph value. */

 [[nodiscard]] Function::FunctionValue get_v_scale( void ) const
 { return( f_global_scale ); }

/*--------------------------------------------------------------------------*/
 /// returns the local scale applied to the i-th row
 [[nodiscard]] Function::FunctionValue get_row_scale( Index i ) const
 { return( RowScale( i ) ); }

/*--------------------------------------------------------------------------*/
 /// returns the physical multiplier of the i-th row
 /** The multiplier stored by the abstract LP changes units when local or
  * global row scaling is active. This method maps it back to the multiplier
  * of the original PolyhedralFunction row, keeping callers independent from
  * the internal representation. It is meaningful in either linearized
  * representation. */

 [[nodiscard]] Function::FunctionValue get_row_multiplier( Index i ) const;

/*--------------------------------------------------------------------------*/

 double get_valid_upper_bound( bool conditional = false ) override
 {
  if( conditional )
   return( AbstractBlock::get_valid_upper_bound( true ) );
  else
   return( std::min( AbstractBlock::get_valid_upper_bound( false ) ,
		     PF().get_global_upper_bound() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 double get_valid_lower_bound( bool conditional = false ) override
 {
  if( conditional )
   return( AbstractBlock::get_valid_lower_bound( true ) );
  else
   return( std::max( AbstractBlock::get_valid_lower_bound( false ) ,
		     PF().get_global_lower_bound() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------------- Methods for R3 Blocks --------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for R3 Blocks
 *  @{ */

 /// gets an R3 Block of PolyhedralFunctionBlock, currently only the copy one
 /** Gets an R3 Block of the PolyhedralFunctionBlock. The list of currently
  * supported R3 Block is:
  *
  * - r3bc == nullptr: the copy (a PolyhedralFunctionBlock identical to this)
  *
  *   IMPORTANT NOTE: the copy R3 Block created in this way is not fully
  *                   functional because the active Variable of the
  *                   PolyhderalFunction have not been set. This is
  *                   something that is supposed to be done "externally",
  *                   outside of the PolyhedralFunctionBlock, and therefore
  *                   there is no way it can be done here.
  */

 Block * get_R3_Block( Configuration *r3bc = nullptr ,
		       Block * base = nullptr , Block * father = nullptr)
  override;

/*--------------------------------------------------------------------------*/
 /** No specific Configuration is expected for PolyhedralFunctionBlock.
  *
  * IMPORTANT NOTE: map_forward_Modification() only maps "physical"
  * Modification. The point is that if any part of the "abstract
  * representation" of PolyhedralFunctionBlock is changed, the corresponding
  * "abstract" Modification is intercepted in add_Modification() and a
  * "physical" Modification is also issued. Hence, for any change in
  * PolyhedralFunctionBlock there will always be both Modification "in
  * flight", and therefore there is no need (and good reasons not) to map
  * both.
  *
  * In particular, the method handles the following Modification:
  *
  * - GroupModification
  *
  * - PolyhedralFunctionModRngd
  *
  * - PolyhedralFunctionModSbst
  *
  * - PolyhedralFunctionModAddd
  *
  * - C05FunctionModVarsRng (with shift() == 0)
  *
  * - C05FunctionModVarsSbst (with shift() == 0)
  *
  * - PolyhedralFunctionMod with type() == C05FunctionMod::NothingChanged,
  *   i.e., the "sign" of the PolyhedralFunction has changed
  *
  * -  FunctionMod with f_shift == FunctionMod::NaNshift, i.e., "everything
  *    changed"
  *
  * Any other Modification is ignored (and false is returned). In particular,
  * note that the PolyhedralFunction does issue
  *
  * - C05FunctionModVarsAddd BUT THESE ARE NOT HANDLED BY THIS METHOD.
  *
  *   The rationale is that this is simply not possible, since
  *   PolyhedralFunctionBlock has no clue "where the active Variable of the
  *   PolyhedralFunction come from", and in particular the newly added
  *   Variable may not even be in the copy PolyhedralFunctionBlock. This
  *   kind of operation therefore have to be managed by
  *   map_forward_Modification() of whichever :Block contains the
  *   PolyhedralFunctionBlock. For this reason, if a C05FunctionModVarsAddd
  *   is received "true" is returned even if nothing is done (on the
  *   expectation that the right thing is anyway be done elsewhere).
  *
  *     IMPORTANT NOTE: PolyhedralFunctionMod[Rngd/Sbst] AND
  *     PolyhedralFunctionModAddd ALLOW TO ADD/DELETE ROWS IN THE
  *     PolyhedralFunction, WHICH ALSO CHANGES THE "NAMES" OF EXISTING ROWS,
  *     AND C05FunctionModVars[Rng/Sbst] ALLOW TO DELETE Variables, WHICH
  *     CHANGES THE "NAMES" OF THE REMAINING ONES. PolyhedralFunctionBlock
  *     IMPLEMENTS map_forward_Modification() IN A WAY THAT IS ONLY
  *     GUARANTEED TO BE CORRECT IF:
  *
  *     = EITHER THE SET OF ROWS AND Variable ARE NEVER CHANGED;
  *
  *     = OR THE Modification ARE MAPPED IMMEDIATELY AFTER THEY ARE ISSUED.
  *
  * This is because otherwise PolyhedralFunctionBlock should have to
  * understand whether the set of row/Variable "names" in the Modification
  * is still correct and do something in case it is not, which is too complex
  * to do at the moment.
  *
  * Note that for GroupModification, true is returned only if all the
  * inner Modification of the GroupModification return true.
  *
  * Note that if the issueAMod param is eModBlck, then it is "downgraded" to
  * eNoBlck: the method directly does "physical" changes, hence there is no
  * reason for it to issue "abstract" Modification with concerns_Block() ==
  * true. */

 bool map_forward_Modification( Block *R3B , c_p_Mod mod ,
				Configuration *r3bc = nullptr ,
				ModParam issuePMod = eNoBlck ,
				ModParam issueAMod = eModBlck ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /** No specific Configuration is expected for PolyhedralFunctionBlock.
  *
  * The current implementation of map_back_Modification() actually uses
  * map_forward_Modification() in reverse, so see the comments to the latter
  * method. */

 bool map_back_Modification( Block *R3B , c_p_Mod mod ,
			     Configuration *r3bc = nullptr ,
			     ModParam issuePMod = eNoBlck ,
			     ModParam issueAMod = eModBlck ) override;

/** @} ---------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

 /// returns true if anyone is "listening to this PolyhedralFunctionBlock"
 /** Returns true if there is anyone "listening to this
  * PolyhedralFunctionBlock", or if the PolyhedralFunctionBlock has to
  * "listen" anyway because the "linearized" representation is constructed,
  * and therefore "abstract" Modification have to be generated anyway to
  * keep the two representations in sync. Note that the "natural"
  * representation has no such issues, the Modification can just be passed
  * up to the father [Abstract]Block.
  *
  * No, this should not be needed. In fact, if the "abstract" representation
  * is modified with the default eModBlck value of issueMod, it is issued
  * irrespectively to the value of anyone_there(); see Observer::issue_mod().
  * If the value of issueMod is anything else the  "abstract" representation
  * has been modified already and there is no point in issuing the
  * Modification.
  * Note that that Observer::issue_mod() does not check if the "abstract"
  * representation has been constructed, but this is clearly not
  * necessary, as the Modification we are speaking of are issued while
  * changing the "abstract" representation, if that has not been
  * constructed then it cannot issue Modification

 bool anyone_there( void ) const override {
  return( f_rep & 1 ? true : AbstractBlock::anyone_there() );
  }
 */
/*--------------------------------------------------------------------------*/
 /// adding a new Modification to the PolyhedralFunctionBlock
 /** Method for handling Modification.
  *
  * The version of PolyhedralFunctionBlock has to do two "opposite" things:
  *
  * 1) Intercept any Modification coming out of its "physical"
  *    PolyhedralFunction component, and, if the "linearized representation"
  *    is used, properly change it to reflect those (otherwise the
  *    Modification is passed up to AbstractBlock::add_Modification() without
  *    further action).
  *
  * 2) Intercept any "abstract Modification" that modifies anything in the
  *    PolyhedralFunctionBlock *except* the PolyhedralFunction, which means
  *    components of the "linearized representation", and properly change the
  *    latter to reflect them.
  *
  * The former operation is handled by the protected method
  * guts_of_add_Modification_PF(), while the latter by the protected method
  * guts_of_add_Modification_LR(); see their comments for details.
  *
  * TODO: define and handle an appropriate GroupModification to manage
  *       addition and removal of Variables from the LinearFunction inside
  *       the FRowConstraint
  *
  * Note that while PolyhedralFunctionBlock regards itself as "leaf" Block,
  * i.e., it does not handle any sub-Block, these may actually can be there;
  * but if thet are, they must be handled either by AbstractBlock (which does
  * nothing about their Modification), or by whatever derived class from
  * PolyhedralFunctionBlock actually have defined them. Hence,
  *
  *     PolyhedralFunctionBlock WILL IGNORE ANY Modification WHICH IS NOT
  *     COMING FROM ANY OF THE COMPONENTS IT EXPLICITLY HANDLES, I.E., THE
  *     PolyhedralFunction AND ITS "LINEARIZED REPRESENTATION"
  *
  * Derived classes may then have to define their own add_Modification() to
  * work, which is fine because it will be called instead of
  * PolyhedralFunctionBlock::add_Modification(), which they then can call
  * (or the two separate guts_of_add_Modification_PF() and
  * guts_of_add_Modification_LR() if more appropriate) for all the
  * Modification they themselves don't handle.
  *
  * Note: any Modification resulting from processing mod will be sent to the
  *       same channel (chnl); if that's a GroupModification and chnl is not
  *       the default one, it will be nested. */

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override
 {
  //!! std::cout << *mod << std::endl;

  // if the "natural" representation is used (bit 0 of f_rep is 0), or this
  // Block is "playing dumb" (see set_play_dumb()), or the Modification comes
  // from a sub-Block, or it does not concern the Block any longer, just pass
  // it up. With the "linearized primal" ("01") and "linearized dual" ("11")
  // encodings this method intercepts Modifications and mirrors them between
  // PF() and the (primal or dual) abstract representation.
  if( is_natural() || f_play_dumb || ( mod->get_Block() != this ) ||
      ( ! mod->concerns_Block() ) ) {
   AbstractBlock::add_Modification( mod , chnl );  // just pass it up
   return;
   }

  mod->concerns_Block( false );  // recall it's been checked already

  auto tmod = std::dynamic_pointer_cast< const FunctionMod >( mod );
  if( tmod && ( tmod->function() == & PF() ) ) {
   // if the Modification comes from the PolyhedralFunction; it will
   // generate a (bunch of) Modification(s) in the abstract
   // representation, and this Modification itself will also remain to
   // serves a the "physical" Modification) unless the Modification
   // causes a NBModification to be issued, in which case it is useless
   const bool reissued_nb = is_dual()
      ? guts_of_add_Modification_PF_dual( tmod.get() , chnl )
      : guts_of_add_Modification_PF( tmod.get() , chnl );
   if( reissued_nb )
    return;
   }
  else {
   // this Modification comes from some other part of the abstract
   // representation of the PolyhedralFunctionBlock, possibly (but not
   // surely) the "linearized" (primal or dual) one: deal with it
   if( is_dual() )
    guts_of_add_Modification_LR_dual( mod.get() , chnl );
   else
    if( guts_of_add_Modification_LR( mod.get() , chnl ) )
     return;
   }

  // finally, pass iT up, but only if there really is someone "listening",
  // which may not be, because anyone_there() returns true anyway
  // (since the abstract representation is in use when we get here)
  // someone is listening if the PolyhedralFunctionBlock has any Solver
  // directly attached, or it has a father Block and the father says so

  if( ( ! v_Solver.empty() ) ||
      ( get_f_Block() && get_f_Block()->anyone_there() ) )
   AbstractBlock::add_Modification( mod , chnl );
  }

/** @} ---------------------------------------------------------------------*/
/*------- METHODS FOR PRINTING & SAVING THE PolyhedralFunctionBlock --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing & saving the PolyhedralFunctionBlock
 * @{ */

 /// print information about the PolyhedralFunctionBlock on an ostream 
 /** Print information about the PolyhedralFunctionBlock. With default
  * verbosity (vlvl == 0) it just prints summary information, otherwise it
  * basically prints the whole PolyhedralFunction.
  *
  * Note that none of these is a "complete" format allowing to read the
  * PolyhedralFunctionBlock back (anyway load() is not implemented). */

 void print( std::ostream & output , char vlvl = 0 ) const override;

/*--------------------------------------------------------------------------*/
 /// serialize the PolyhedralFunctionBlock (recursively) to a netCDF NcGroup
 /** The PolyhedralFunctionBlock serializes itself out of a netCDF::NcGroup.
  * This is easy, since it is done by simply asking the PolyhedralFunction
  * to do basically all the work, and then calling the method of the base
  * class to do the rest. */

 void serialize( netCDF::NcGroup & group ) const override
 {
  // call the base class method
  AbstractBlock::serialize( group );

  // have the PolyhedralFunction do all the dirty work for us
  PF().serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 */

/*--------------------------------------------------------------------------*/
 /// process a FunctionMod produced by the PolyhedralFunction
 /** Process a FunctionMod produced by the PolyhedralFunction. This requires
  * to patiently sift through the possible Modification types (but only those
  * derived from FunctionMod) to find what this Modification exactly, is and
  * appropriately mirror the changes to the PolyhedralFunction (which in this
  * case counts as the "physical representation") into the "abstract" one,
  * i.e., performing the corresponding changes on the LP.
  *
  * Note that this method only deals with FunctionMod coming directly out of
  * the PolyhedralFunction. As a consequence, this method does not have to
  * deal with GroupModification since these are produced by
  * Block::add_Modification(), but this method is called *before* that one is.
  *
  * As an important consequence, we can assume that
  *
  *   THE STATE OF THE DATA STRUCTURE IN PolyhedralFunctionBlock WHEN THIS
  *   METHOD IS EXECUTED IS PRECISELY THE ONE IN WHICH THE Modification WAS
  *   ISSUED: NO COMPLCATED OPERATIONS (Variable AND/OR Constraint BEING
  *   ADDED/REMOVED ...) CAN HAVE BEEN PERFORMED IN THE MEANTIME
  *
  * This assumption drastically simplifies some of the logic here. Hence,
  * derived classes must ensure they do not mess up with this property.
  *
  * The method returns true if and only if the FunctionMod produced by the
  * PolyhedralFunction is the "nuclear Modification for Function" that
  * causes a NBModification to be issued by PolyhedralFunctionBlock; in this
  * case, and in this case only, forwarding the original Modification is
  * pointless because the whole of the Block has been changed, */

 bool guts_of_add_Modification_PF( const FunctionMod * mod , ChnlName chnl );

/*--------------------------------------------------------------------------*/
 /// process a Modification produced by the "linearized" representation
 /** This requires to patiently sift through the possible Modification types
  * find what this Modification exactly, is and appropriately mirror the
  * changes of the "abstract" representation into the PolyhedralFunction
  * (which in this case counts as the "physical" one). Note, however, that
  *
  *     SOME Modification OF THE LP ARE NOT SUPPORTED SINCE THEY WOULD
  *     LEAVE THE PolyhedralFunction IN AN INCONSISTENT STATE
  *
  * In particular:
  *
  * - Adding/removing Variable from an individual LP constraint is not
  *   allowed. It could be for adding, doing the same to all other LP
  *   constraints (with 0 coefficients), but then one should ensure that
  *   "the same" additions later on are rather treated as coefficent
  *   changes. Similarly with removals.
  *
  * - Changing the Objective in any way is not allowed (changing the "verse"
  *   of the PolyhedralFunction also requires many co-ordinated changes).
  *
  * - Changing the RHS/LHS of a Constraint, or of the BoxConstraint giving
  *   upper/lower bounds on v, is only allowed if it is the "right" one
  *   (lower/upper depending if the PolyhedralFunction is convex/concave).
  *
  * Note that this method only deals with Modification coming directly out of
  * some element of the PolyhedralFunctionBlock (except the
  * PolyhedralFunction, which is treated independently). That is, the
  * Modification cannot come from the sub-Block. As a consequence, this
  * method does not have to deal with GroupModification since these are
  * produced by Block::add_Modification(), but this method is called
  * *before* that one is.
  *
  * As an important consequence, we can assume that
  *
  *   THE STATE OF THE DATA STRUCTURE IN PolyhedralFunctionBlock WHEN THIS
  *   METHOD IS EXECUTED IS PRECISELY THE ONE IN WHICH THE Modification WAS
  *   ISSUED: NO COMPLCATED OPERATIONS (Variable AND/OR Constraint BEING
  *   ADDED/REMOVED ...) CAN HAVE BEEN PERFORMED IN THE MEANTIME
  *
  * This assumption drastically simplifies some of the logic here. Hence,
  * derived classes must ensure they do not mess up with this property. */

 bool guts_of_add_Modification_LR( c_p_Mod mod , ChnlName chnl );

/*--------------------------------------------------------------------------*/
 /// PF -> dual abstract: counterpart of guts_of_add_Modification_PF
 /** Mirrors a Modification coming from PF() into the *dual* abstract
  * representation (f_theta dynamic variables, f_normcns normalization
  * constraint, the FRealObjective LinearFunction and, when registered, the
  * f_coupling external coupling constraints).
  *
  * The return value has the same semantics as guts_of_add_Modification_PF:
  * true means a NBModification was issued (so the caller should not
  * forward the original Modification any further), false otherwise. */

 bool guts_of_add_Modification_PF_dual( const FunctionMod * mod ,
                                        ChnlName chnl );

/*--------------------------------------------------------------------------*/
 /// dual abstract -> PF: counterpart of guts_of_add_Modification_LR
 /** Mirrors a Modification coming from the *dual* abstract representation
  * back into PF(). Most direct modifications of the dual abstract
  * structures (theta variables, normalization, objective LinearFunction,
  * coupling constraints) would leave PF() in an inconsistent state and
  * are rejected. The few "internal" Modifications produced by this class
  * itself while processing a PolyhedralFunctionMod from PF() are
  * recognised and silently absorbed. */

 void guts_of_add_Modification_LR_dual( c_p_Mod mod , ChnlName chnl );

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

 // clears all the abstract representaton, but not PF()
 void guts_of_destructor( void );

 // constructs the i-th constraint of the linearized representation
 void ConstructLPConstraint( Index i , FRowConstraint & ci );

 // initialise scaling for all rows already present in PF()
 void InitialiseScaling( void );

 // append per-row metadata for rows [ first , PF().get_A().size() )
 void AppendRowScaling( Index first );
 void AppendRowMeasure( Index first );

 // remove per-row metadata corresponding to deleted rows
 void DeleteRowScaling( Range range );
 void DeleteRowScaling( const Subset & rows );

 // numerical scaling helpers
 Function::FunctionValue ComputeRowMeasure( Index i ) const;
 Function::FunctionValue ComputeRowScale( Index i ) const;
 void RefreshRowMeasure( Index i );
 void RefreshRowMeasure( Range range );
 void RefreshRowMeasure( const Subset & rows );
 Function::FunctionValue RowScale( Index i ) const;
 Function::FunctionValue ScaledRowFactor( Index i ) const;
 Function::FunctionValue ScaledBound( Function::FunctionValue value ) const;
 ColVariable * LinearizedV( void );
 Function::FunctionValue ComputeGlobalMeasure( void ) const;
 bool UpdateGlobalScaleIfNeeded( bool force = false );
 bool RescaleGlobalIfNeeded( ChnlName chnl );
 void UpdateLinearizedPrimalScale( ChnlName chnl );
 void UpdateLinearizedDualScale( ChnlName chnl );
 void RebuildLinearizedPrimal( void );
 void RebuildLinearizedDual( void );

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/
/* These fields are private because PolyhedralFunctionBlock ha unique
 * jurisdiction on the PolyhedralFunction and its representation. Further
 * derved classes may do whatever they want with "the rest" of the "abstract"
 * representation, but they are not supposed to mess up with that part. */

 char f_rep;  ///< how the representation is constructed
              /**< This field is coded bit-wise. The first two bits,
	       * bit 0 and bit 1, encode which formulation is used:
 * - 00 : natural formulation (one PolyhedralFunction, wrapped in a
 *        FRealObjective);
 * - 01 : linearised primal formulation
 * - 10 : same as 00 (i.e., natural): if bit 0 is 0 the natural
 *        representation is used regardless of bit 1
 * - 11 : linearised dual formulation (Fenchel conjugate / pin function)
 * The following bits rather encode which part of the formulation has been
 * constructed already:
 * bit 2: 1 if the variables are constructed
 * bit 3: 1 if the constraints are constructed
 * bit 4: 1 if the objective is constructed
 * The named bit masks are defined as static constexpr in the .cpp; the
 * helper inline methods is_natural() / is_linearized() / is_dual() below
 * provide a readable API to query the chosen formulation. */

 // readable helpers for the formulation encoded in the lowest two bits
 // of f_rep, as documented above
 bool is_natural( void ) const { return( ( f_rep & 1 ) == 0 ); }
 bool is_linearized( void ) const  // primal linearized representation
 { return( ( f_rep & 3 ) == 1 ); }
 bool is_dual( void ) const  // dual linearized representation
 { return( ( f_rep & 3 ) == 3 ); }

 char f_scaling;         ///< optional scaling modes selected through stvv:
                         ///< bit 0: local row scaling, bit 1: global scaling

 bool has_local_scaling( void ) const { return( f_scaling & 1 ); }
 bool has_global_scaling( void ) const { return( f_scaling & 2 ); }

 Function::FunctionValue f_global_scale;
                         ///< shared epigraph scale, updated only in batches

 Function::FunctionValue f_global_reference;
                         ///< row measure at the last global rescaling

 RealVector f_row_scale;
                         ///< positive local factor for each row of PF()

 RealVector f_row_measure;
                         ///< max( 1, || A_i ||_inf, | b_i | ) for each row

 bool f_no_objective;    ///< whether the objective is excluded from the
                         ///< abstract representation

 PolyhedralFunction f_own_polyf;  ///< the owned PolyhedralFunction ( default )

 /// pointer to the *active* PolyhedralFunction: the owned one by default, or
 /// an external one when this Block wraps it ( see the second constructor )
 PolyhedralFunction * f_polyf_p{ & f_own_polyf };

 /// the active PolyhedralFunction ( owned or external ), used everywhere
 PolyhedralFunction & PF( void ) { return( * f_polyf_p ); }
 const PolyhedralFunction & PF( void ) const { return( * f_polyf_p ); }

 /// when true, add_Modification() does not mirror the abstract representation
 /// ( Constraint / Objective ) back onto PF(): the Modification are still
 /// passed on to any attached Solver, but PF() is left untouched. Set via
 /// set_play_dumb(); used by remove_redundant_rows() on a transient inner
 /// Block sharing an external PolyhedralFunction
 bool f_play_dumb = false;

 ColVariable f_v;        ///< the physical v of the linearized representation

 ColVariable f_scaled_v; ///< internal globally-scaled v used by primal cuts

 std::list< FRowConstraint > f_const;
                         ///< the constraints in the linearized representation

 BoxConstraint f_bcv;    ///< the box constraint on v

 FRowConstraint f_scale_cns;
                         ///< internal equality f_scaled_v = scale * f_v

 std::list< ColVariable > f_theta;
                         ///< the dynamic variables theta in the dual
                         /// representation, one per row of PF() (both
                         /// diagonal and vertical, in the same order)

 ColVariable f_gamma;    ///< the static ColVariable gamma in the dual
                         /// representation, playing the role of the dual
                         /// multiplier of the global lower/upper bound of
                         /// PF() (is_fixed-ed to 0 when no bound is set)

 FRowConstraint f_normcns;
                         ///< the static "normalization" constraint of the
                         /// dual representation, enforcing that the sum of
                         /// all the diagonal multipliers theta_i together
                         /// with gamma equals 1 (or, equivalently, the
                         /// external multiplier lambda passed to
                         /// set_lambda()).

 std::list< FRowConstraint > * f_coupling = nullptr;
                         ///< pointer (non-owning) to the list of external
                         /// "coupling" constraints registered via
                         /// set_conjugate_constraint(). Used by the
                         /// add_Modification machinery to keep the
                         /// coupling LinearFunctions in sync with the rows
                         /// of PF() in the dual representation.
                         /// nullptr until set_conjugate_constraint() is
                         /// called (and remains nullptr if it never is).

 ColVariable * f_lambda = nullptr;
                         ///< shared normalization variable set by set_lambda

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionBlock ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* PolyhedralFunctionBlock.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File PolyhedralFunctionBlock.h -------------------*/
/*--------------------------------------------------------------------------*/

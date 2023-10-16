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
 * of ome extra continuous ColVariable plus a finite set of (dynamic)
 * FRowConstraint with LinearFunction inside (a.k.a., linear constraints).
 * Thus, the main feature that PolyhedralFunctionBlock implements is the
 * ability to "present" itself (construct an "abstract representation") as
 * either having a FRealObjective with a PolyhedralFunction inside, or having
 * a set of (continuous) ColVariable and (linear) RowConstraint. We call the
 * first the "natural representation" of a PolyhedralFunctionBlock, and the
 * latter its "linearized representation".
 *
 * Indeed, having a PolyhedralFunction as objective is equivalent to the
 * linear program
 * \f[
 *     \min \{ v : v >= a_i x + b_i \}    \qquad     i = 0, ... , m - 1
 * \f]
 * in the convex case, as this corresponds to
 * \f[
 *     pf( x ) = max \{ a_i x + b_i : i = 0, ... , m - 1 \}
 * \f]
 * (pointwise maximum of a finite set of linear functions), and
 * \f[
 *     max \{ v : v <= a_i x + b_i \}     \qquad     i = 0, ... , m - 1
 * \f]
 * in the concave one, as this corresponds to
 * \f[
 *     pf( x ) = min \{ a_i x + b_i : i = 0, ... , m - 1 \}
 * \f]
 * (pointwise minimum of a finite set of linear functions). Note that above
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
 *   v <op> a_i x + b_i for i = 1, ..., m where <op> depends on if the
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
 * kind of Constraint, Variable and Objective are supported. */

class PolyhedralFunctionBlock : public AbstractBlock {

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
  : AbstractBlock( father ) , f_rep( 0 ) ,
    f_polyf( {} , {} , {} , -Inf< Function::FunctionValue >() , true , this ) ,
    f_v() , f_const() { }

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
  f_polyf.deserialize( group , eNoMod );

  // the PolyhedralFunctionBlock is "naked": no abstract representaton
  f_rep = 0;
  f_const.clear();

  // call the (guts_of version of the) base class method
  AbstractBlock::guts_of_deserialize( group );

  // call the method of Block
  // inside this the NBModification, the "nuclear option",  is issued

  Block::deserialize( group );
  }

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

 /// generate the Variable in the "linearized representation"
 /** This method serves is to ensure that the "abstract representation" of
  * the Variable in the PolyhedralFunctionBlock is initialized, so that it
  * can be read with get_static_variables() and get_dynamic_variables(). Of
  * course, the effect changes depending on whether the "natural
  * representation" or the "linearized representation" are used. In fact,
  *
  *    THE CHOICE BETWEEN THE TWO IS DONE PRECISELY IN THIS METHOD
  *
  * by means of the stvv parameter. The boolean field f_rep is set here to
  * true if the "linearized representation" is used, false otherwise, in
  * the following way:
  *
  * - if stvv is not nullptr and it is a SimpleConfiguration< int >, then it
  *   if bool( stvv->f_value );
  *
  * - otherwise, if f_BlockConfig is not nullptr,
  *   f_BlockConfig->f_static_variables_Configuration is not nullptr and it
  *   is a SimpleConfiguration< int >, then it is
  *   bool( f_BlockConfig->f_static_variables_Configuration->f_value );
  *
  * - otherwise, false ("natural representation") is assumed.
  *
  * The value is set upon call to this method, and never changed afterwards;
  * this means that the parameters of generate_abstract_constraints() and
  * generate_objective() are plainly ignored, and that this mathod has to
  * be called before these (which is only reasonable).
  *
  * If f_rep == false, the PolyhedralFunctionBlock has no extra Variable, be
  * them static or dynamic. If f_rep == true, the first group of static
  * Variable contains a single ColVariable (v), and there are no extra
  * dyanmic Variable. Note that "v" is added "in front", so that even if
  * the AbstractBlock has constructed some "abstract" representation
  * already (say, in deserialize()), "v" is still the first group of static
  * ColVariable. Classes derived from PolyhedralFunctionBlock will have to
  * be careful about this.
  *
  * Note that if further derived classes add some other structure, their
  * version of this method will have to call the method of this class
  * first, because it uses (if any) the *first* group of static Variable. */

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
  *    constraint") representing the m inequalities v >= [<=] a_i x + b_i.
  *    Note that the "verse" of the Constraint depend on
  *    PolyhedralFunction->is_convex(); if it is true than the inequalities
  *    are ">=" (the LHS is -INF and the RHS is b_i), otherwise they are "<="
  *    (the LHS b_i and the RHS is INF).
  *
  *  - The first group of static Constraint contains a single BoxConstraint
  *    whose variable is "v", which serves to store the global lower/upper
  *    bound on the function vale. This clearly depends on
  *    PolyhedralFunction->is_convex(); if it is true, than the LHS is the
  *    global lower bound and the RHS is INF, otherwise the LHS is - INF and
  *    the RHS is the global upper bound.
  *
  * Note that both groups of Constraint are added "in front" of their 
  * corresponding vector, so that even if the AbstractBlock has constructed
  * some "abstract" representation already (say, in deserialize()), they 
  * still are the first group of dynamic/static Constraint. Classes derived
  * from PolyhedralFunctionBlock will have to be careful about this. */

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
  * method is called (at your own risk). */

 void generate_objective( Configuration *objc = nullptr ) override;

/** @} ---------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the PolyhedralFunctionBlock
 *  @{ */

 PolyhedralFunction & get_PolyhedralFunction( void ) { return( f_polyf ); }

/*--------------------------------------------------------------------------*/

 double get_valid_upper_bound( bool conditional = false ) override
 {
  if( conditional )
   return( AbstractBlock::get_valid_upper_bound( true ) );
  else
   return( std::min( AbstractBlock::get_valid_upper_bound( false ) ,
		     f_polyf.get_global_upper_bound() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 double get_valid_lower_bound( bool conditional = false ) override
 {
  if( conditional )
   return( AbstractBlock::get_valid_lower_bound( true ) );
  else
   return( std::max( AbstractBlock::get_valid_lower_bound( false ) ,
		     f_polyf.get_global_lower_bound() ) );
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

  // if the "natural" representation is used, or the Modification comes
  // from a sub-Block, or it does not concern the Block any longer
  if( ( ! ( f_rep & 1 ) ) || ( mod->get_Block() != this ) ||
      ( ! mod->concerns_Block() ) ) {
   AbstractBlock::add_Modification( mod , chnl );  // just pass it up
   return;
   }

  mod->concerns_Block( false );  // recall it's been checked already

  auto tmod = std::dynamic_pointer_cast< const FunctionMod >( mod );
  if( tmod && ( tmod->function() == & f_polyf ) ) {
   // if the Modification comes from the PolyhedralFunction; it will
   // generate a (bunch of) Modification(s) in the "linearized"
   // representation, and this Modification itself will also remain to
   // serves a the "physical" Modification) unless the Modification
   // causes a NBModification to be issued, in which case it is useless
   if( guts_of_add_Modification_PF( tmod.get() , chnl ) )
    return;
   }
  else
   // this Modification comes from some other part of the abstract
   // representation of the PolyhedralFunctionBlock, possibly (but not
   // surely) the "linearized" one: deal with it
   guts_of_add_Modification_LR( mod.get() , chnl );

  // finally, pass iT up, but only if there really is someone "listening",
  // which may not be, because anyone_there() returns true anyway
  // (since f_rep & 1 == true when we get here)
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
  f_polyf.serialize( group );
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

 void guts_of_add_Modification_LR( c_p_Mod mod , ChnlName chnl );

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

 // clears all the abstract representaton, but not f_polyf
 void guts_of_destructor( void );

 // constructs the i-th constraint of the linearized representation
 void ConstructLPConstraint( Index i , FRowConstraint & ci );

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/
/* These fields are private because PolyhedralFunctionBlock ha unique
 * jurisdiction on the PolyhedralFunction and its representation. Further
 * derved classes may do whatever they want with "the rest" of the "abstract"
 * representation, but they are not supposed to mess up with that part. */

 char f_rep;                  ///< how the representation is constructed
                              /**< This field is coded bit-wise:
			       * bit 0: 1 if the linearized is used
			       * bit 1: 1 if the variable are constructed
			       * bit 2: 1 if the constraint are constructed
			       * bit 3: 1 if the objective is constructed
			       */

 PolyhedralFunction f_polyf;  ///< the PolyhedralFunction

 ColVariable f_v;        ///< the v variable in the linearized representation

 std::list< FRowConstraint > f_const;
                         ///< the constraints in the linearized representation

 BoxConstraint f_bcv;    ///< the box constraint on v

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

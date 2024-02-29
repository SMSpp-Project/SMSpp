/*--------------------------------------------------------------------------*/
/*----------------------- File BendersBFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class BendersBFunction, which implements C05Function
 * and Block with a Benders function.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BendersBFunction
#define __BendersBFunction
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "C05Function.h"
#include "CDASolver.h"
#include "ColVariable.h"
#include "Objective.h"
#include <limits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class AbstractPath;       // forward declaration of AbstractPath

 class ConstraintMod;      // forward declaration of ConstraintMod

 class RowConstraint;      // forward declaration of RowConstraint

 class Solution;           // forward declaration of Solution

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BendersBFun_CLASSES Classes in BendersBFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BendersBFunction ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Benders Function
/** The class BendersBFunction is a convenience class implementing the
 * "abstract" concept of a Benders Function of "any" Block. BendersBFunction
 * derives from *both* C05Function and Block.
 *
 * The main ingredients of a BendersBFunction are the following:
 *
 * 1) A "base" Block B, representing an optimization problem of the form
 *
 *        (B)    min { c(y) : w <= E(y) <= z, y \in Y },
 *
 *    where c, w, and z are vectors of appropriate sizes, E is a function of
 *    y, and Y is a convex set. We assume that the set Y is represented by a
 *    set of RowConstraint and bounds on the variables y. This will be the
 *    one, and only, sub-Block of BendersBFunction (when "seen" as a Block).
 *
 * 2) A matrix A with m rows and n columns, a vector b with m rows, and a
 *    vector of pairs [ ( C_i , S_i ) ]_{i \in I}, each pair being formed by a
 *    pointer C_i to a RowConstraint of Block B and a #ConstraintSide S_i,
 *    where I = {0, ..., m-1}.
 *
 *    Problem (B) would typically be associated with an original problem
 *
 *        (O)    min { d(x) + c(y) : g <= Fx + E(y) <= h, x \in X, y \in Y }
 *
 *    defined in terms of variables x and y. By reformulating problem (O) as
 *
 *        (O')   min { d(x) + phi(x) : x \in X },
 *
 *    where
 *
 *        (P)    phi(x) = min { c(y) : (g - Fx) <= E(y) <= (h - Fx), y \in Y },
 *
 *    we see that (P) assumes the form of (B) with w = g - Fx and z = h -
 *    Fx. The BendersBFunction represents the function phi whose underlying
 *    optimization problem is given by (B). The variables x are the active
 *    Variables of this BendersBFunction. As it can be seen in (P), the left-
 *    and right-hand sides of (some or all) the constraints may depend on
 *    x. The Block B, however, does not depend on x. In order to have the
 *    left- and right-hand sides of the constraints in (B) dependent on x, we
 *    consider the mappings "x -> g - Fx" and "x -> h - Fx", which are used to
 *    update the left- and right-hand sides w and z of the constraints in (B)
 *    according to the values of the variables x.
 *
 *    These mappings are bunched together into a single mapping M defined by
 *    the matrix A and the vector b such that M(x) = Ax + b. The i-th
 *    component of this mapping, namely M_i(x) = [ Ax + b ]_i, is associated
 *    with the left- or right-hand side (or both) of the RowConstraint of
 *    Block B pointed by C_i. The #ConstraintSide S_i indicates which sides of
 *    the RowConstraint (pointed by) C_i are affected, as follows:
 *
 *    - If S_i = eLHS, then M_i(x) gives the value of the left-hand side of
 *      the RowConstraint (pointed by) C_i.
 *
 *    - If S_i = eRHS, then M_i(x) gives the value of the right-hand side of
 *      the RowConstraint (pointed by) C_i.
 *
 *    - If S_i = eBoth, then M_i(x) gives the value of both the left- and
 *      right-hand sides of the RowConstraint (pointed by) C_i. This is the
 *      case, for example, of an equality constraint, in which the left- and
 *      right-hand sides are equal.
 *
 *    Notice that the affected constraints must necessarily be
 *    RowConstraint. Moreover, if a RowConstraint in Block B has finite bounds
 *    that can be different from each other and are affected by the values of
 *    x, then it would be listed twice (once for each of its bounds). That is,
 *    there would be indices i, j \in I such that i != j and C_i = C_j, S_i =
 *    eLHS, and S_j = eRHS.
 *
 *        The vector of pairs [ ( C_i , S_i ) ]_{i \in I} must not contain
 *        duplicate entries (i.e., there must not exist indices i, j \in I
 *        such that i != j, C_i == C_j and S_i == S_j). Moreover, if there is
 *        i \in I such that S_i == eBoth, there must not exist j \in I such
 *        that i != j and C_i == C_j.
 *
 *    We stress that the i-th active Variable of this BendersBFunction is
 *    associated with the i-th column of the mapping matrix A. In other words,
 *    the i-th active Variable serves as the coefficient of the i-th column of
 *    the A matrix in the mapping M.
 *
 * Note that the BendersBFunction is not supposed to have any Variable or
 * Constraint (besides those defined in its only inner Block B).
 *
 * BendersBFunction handles all possible changes in its input data:
 *
 * - A complete reset of all the data (A, b, constraints, sides), which
 *   results in a FunctionMod (with shift() == FunctionMod::NaNshift, i.e.,
 *   "everything changed") being issued.
 *
 * - Addition of variables (adding columns to A), either one or a range, which
 *   results in a C05FunctionModVarsAddd being issued (since a
 *   BendersBFunction is strongly quasi-additive, and with shift() == 0 as
 *   expected).
 *
 * - Removal of variables (removing columns from A), either one, or a range,
 *   or a subset of them, which results in either a C05FunctionModVarsRngd or
 *   a C05FunctionModVarsSbst being issued (since a BendersBFunction is
 *   strongly quasi-additive, and with shift() == 0 as expected).
 *
 * - Changes of rows in A (either one, or a range or a subset of them) and in
 *   the corresponding elements of b, which results in either a
 *   BendersBFunctionModRngd or a BendersBFunctionModSbst being issued, with
 *   shift() == NANshift (the function has changed "unpredictably"), type() ==
 *   AllLinearizationChanged (all the linearizations may have changed,
 *   although actually only a subset of them has) and BFtype() == ModifyRows.
 *
 * - Changes of elements of b (either one, or a range or a subset of them)
 *   only, which results in either a BendersBFunctionModRngd or a
 *   BendersBFunctionModSbst being issued, with type() == AlphaChanged (all
 *   the alphas may have changed, although actually only a subset of them
 *   has), BFtype() == ModifyCnst, and shift() == NANshift.
 *
 * - Addition of rows to A and b, either one or a range, which results in
 *   BendersBFunctionModAddd being issued with shift() == NANshift and either
 *   type() == C05FunctionMod::AllLinearizationChanged or type() ==
 *   C05FunctionMod::AllEntriesChanged (when the new constants b are all
 *   zero).
 *
 * - Removal of rows from A and b, either one, or a range, or a subset of
 *   them, which results in any of the following Modification being issued:
 *
 *   - a BendersBFunctionModRngd with BFtype() == DeleteRows and shift() ==
 *     C05FunctionMod::NaNshift;
 *
 *   - a BendersBFunctionModSbst with BFtype() == DeleteRows and shift() ==
 *     C05FunctionMod::NaNshift;
 *
 *   - a FunctionMod with shift() == FunctionMod::NaNshift. */

class BendersBFunction : public C05Function , public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*-------------------------------- FRIENDS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class BendersBFunctionState;

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 /* Since BendersBFunction is both a ThinVarDepInterface and a Block, it
  * "sees" two definitions of "Index", "Range", and "Subset". These are
  * actually the same, but compilers still don't like it. Disambiguate by
  * declaring we use the ThinVarDepInterface versions (but it could have been
  * the Block versions, as they are the same). */

 using Index    = ThinVarDepInterface::Index;
 using c_Index  = ThinVarDepInterface::c_Index;

 using Range    = ThinVarDepInterface::Range;
 using c_Range  = ThinVarDepInterface::c_Range;

 using Subset   = ThinVarDepInterface::Subset;
 using c_Subset = ThinVarDepInterface::c_Subset;

 /// public enum representing the sides of a RowConstraint
 /** Public enum representing the sides of a RowConstraint. */

 enum ConstraintSide : char {
  eLHS  = 'L' ,  ///< the left-hand side of a RowConstraint
  eRHS  = 'R' ,  ///< the right-hand side of a RowConstraint
  eBoth = 'B' ,  ///< both sides of a RowConstraint
  eNone = 'N'
  };

 using ConstraintVector = std::vector< RowConstraint * >;
 ///< a vector of pointers to RowConstraint

 using ConstraintSideVector = std::vector< ConstraintSide >;
 ///< a vector of ConstraintSide

 using RealVector = std::vector < FunctionValue >;
 ///< a real n-vector, useful for both the rows of A and b

 using c_RealVector = const RealVector;   ///< a const RealVector

 using MultiVector = std::vector< RealVector >;
 ///< representing the A matrix: a vector of m elements, each a real n-vector

 using c_MultiVector = const MultiVector;   ///< a const MultiVector

 // TODO It should be a vector of const ColVariable *
 using VarVector = std::vector< ColVariable * >;
 ///< representing the x variables upon which the function depends

 using c_VarVector = const VarVector;
 ///< a const version of the x variables upon which the function depends

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a BendersBFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( VarVector::iterator & itr ) : itr_( itr ) {}
  explicit v_iterator( VarVector::iterator && itr )
   : itr_( std::move( itr ) ) {}

  v_iterator * clone( void ) override final {
   return( new v_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++(itr_); }

  reference operator*( void ) const override final {
   return( *((*itr_)) );
   }
  pointer operator->( void ) const override final {
   return( (*itr_) );
   }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const BendersBFunction::v_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const BendersBFunction::v_iterator * >( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const BendersBFunction::v_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const BendersBFunction::v_iterator * >( & rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  VarVector::iterator itr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized concrete const_iterator
 /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a BendersBFunction. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator( VarVector::const_iterator & itr ) : itr_( itr ) {}
  explicit v_const_iterator( VarVector::const_iterator && itr )
   : itr_( std::move( itr ) ) {}

  v_const_iterator * clone( void ) override final {
   return( new v_const_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++(itr_); }

  reference operator*( void ) const override final { return( *((*itr_)) ); }
  pointer operator->( void ) const override final { return( (*itr_) ); }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const BendersBFunction::v_const_iterator * >(
								      & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const BendersBFunction::v_const_iterator * >(
								      & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const BendersBFunction::v_const_iterator * >(
								      & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const BendersBFunction::v_const_iterator * >(
								      & rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  VarVector::const_iterator itr_;
  };

/*--------------------------------------------------------------------------*/
 /// public enum for the int algorithmic parameters
 /** Public enum describing the different algorithmic parameters of int type
  * that BendersBFunction has in addition to those of C05Function. The value
  * intLastBendersBFPar is provided so that the list can be easily further
  * extended by derived classes. */

 enum int_par_type_BendersBF {

  intLinComp = intLastParC05F , ///< determines how linearizations are computed

  intLastBendersBFPar ///< first allowed new int parameter for derived classes
                      /**< Convenience value for easily allow derived classes
                       * to extend the set of int algorithmic parameters. */

 };  // end( int_par_type_BendersBF )

/** @} ---------------------------------------------------------------------*/
/*------------- CONSTRUCTING AND DESTRUCTING BendersBFunction --------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BendersBFunction
 *  @{ */

 /// constructor of BendersBFunction, possibly inputting the data
 /** Constructor of BendersBFunction, taking possibly all the data
  * characterising the function:
  *
  * @param inner_block the only sub-Block of this BendersBFunction,
  *        representing the problem (B) as stated in the general notes of this
  *        class.
  *
  * @param x an n-vector of pointers to ColVariable representing the x
  *        variable vector in the definition of the function. Note that the
  *        order of the variables in x is crucial, since
  *
  *            THE ORDER OF THE x VECTOR WILL DICTATE THE ORDER OF THE
  *            "ACTIVE" [Col]Variable OF THE BendersBFunction
  *
  *        That is, get_active_var( 0 ) == x[ 0 ], get_active_var( 1 ) == x[ 1
  *        ], ...
  *
  * @param A an m-vector of n-vectors of Function::FunctionValue representing
  *        the A matrix in the definition of the linear mapping; the
  *        correspondence between \p A[][] and \p x[] is positional, i.e.,
  *        entry \p A[ i ][ j ] is (obviously) meant to be the coefficient of
  *        variable <tt>*x[ j ]</tt> (i.e., get_active_var( j )) for the i-th
  *        row;
  *
  * @param b an m-vector of Function::FunctionValue representing the b vector
  *        in the definition of the linear mapping;
  *
  * @param constraints the vector of (pointers to) affected RowConstraint.
  *
  * @param sides the vector containing the affected sides of the affected
  *        RowConstraint. There is a correspondence between the vectors \p
  *        sides and \p constraints: \p sides[i] is associated with \p
  *        constraints[i]. Of course, \p sides and \p constraints must have
  *        the same size.
  *
  * @param observer a pointer to the Observer of this BendersBFunction.
  *
  * As the && implies, \p x, \p A, \p b, and \p constraints become property of
  * the BendersBFunction object.
  *
  * All inputs have a default (nullptr, {}, {}, {}, {}, {}, and nullptr,
  * respectively) so that this can be used as the void constructor. */

 BendersBFunction( Block * inner_block = nullptr ,
                   VarVector && x = {} , MultiVector && A = {} ,
                   RealVector && b = {} ,
                   ConstraintVector && constraints = {} ,
                   ConstraintSideVector && sides = {} ,
                   Observer * const observer = nullptr );

/*--------------------------------------------------------------------------*/
 /// load the BendersBFunction out of an istream
 /** This method loads the BendersBFunction out of an istream, or better it
  * should do that if it were properly implemented, which it is not. */

 void load( std::istream & input , char frmt = 0 ) override final {
  throw( std::logic_error( "BendersBFunction::load: not implemented yet" ) );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a BendersBFunction out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * *numerical* information required to de-serialize the BendersBFunction,
  * i.e., an m x n real matrix A, a real m-vector b, and a vector of pairs of
  * pointers to RowConstraint and their sides, and initializes the
  * BendersBFunction by calling set_mapping() with the recovered data. See the
  * comments to BendersBFunction::serialize() for the detailed description of
  * the expected format of the netCDF::NcGroup.
  *
  * Note that this method does *not* change the set of active variables, that
  * must be initialized independently either before (like, in the
  * constructor) or after a call to this method (cf. set_variable()).
  *
  * Note, however that there is a significant difference between calling
  * deserialize() before or after set_variables(). More specifically, the
  * difference is between calling the method when the current set of "active"
  * Variable is empty, or not. Indeed, in the former case the number of
  * "active" Variable is dictated by the data found in the netCDF::NcGroup;
  * calling set_variables() afterwards with a vector of different size will
  * fail. Symmetrically, if the set of "active" Variable is not empty when
  * this method is called, finding non-conforming data (a matrix A with a
  * different number of columns from get_num_active_var()) in the
  * netCDF::NcGroup within this method will cause it to fail. Also, note that
  * in the former case the function is "not completely initialized yet" after
  * deserialize(), and therefore it should not be passed to the Observer quite
  * as yet.
  *
  * Usually [de]serialization is done by Block, but BendersBFunction is a
  * complex enough object so that having its own ready-made [de]serialization
  * procedure may make sense. Besides, it *is* a Block, and  therefore the
  * netCDF::NcGroup will have to have contain whatever is managed by the
  * serialize() method of the base Block class in addition to the
  * BendersBFunction-specific data. However, because this method can then
  * conceivably be called when the BendersBFunction is attached to an
  * Observer (although it is expected to be used before that), it is also
  * necessary to specify if and how a Modification is issued.
  *
  * @param group a netCDF::NcGroup holding the data in the format described
  *        in the comments to serialize();
  *
  * @param issueMod which decides if and how the FunctionMod (with shift()
  *        == FunctionMod::NaNshift, i.e., "everything changed") is issued,
  *        as described in Observer::make_par(). */

 void deserialize( const netCDF::NcGroup & group , ModParam issueMod );

/*--------------------------------------------------------------------------*/
 /// de-serialize a BendersBFunction out of netCDF::NcGroup
 /** This method simply calls deserialize( group , eNoMod ). Please refer to
  * the comments to that method for details. The value eNoMod is passed as
  * argument, since this method is mostly thought to be used during
  * initialization when "no one is listening". */

 void deserialize( const netCDF::NcGroup & group ) override {
  deserialize( group , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor of BendersBFunction
 /** Destructor of BendersBFunction. It destroys the inner Block (if any),
  * releasing its memory. If the inner Block should not be destroyed then,
  * before this BendersBFunction is destroyed, the pointer to the inner Block
  * must be set to \c nullptr. This can be done by invoking set_inner_block(),
  * passing \c nullptr as a pointer to the new inner Block and \c false to the
  * \c destroy_previous_block parameter. */

 virtual ~BendersBFunction();

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_x field
 /** Method to "clear" the BendersBFunction: it clear() the vector v_x. This
  * destroys the list of "active" Variable without unregistering from them.
  * Not that the BendersBFunction would have to, but an Observer using it to
  * "implement itself" should. By not having any Variable, the Observer can no
  * longer do that. */

 void clear( void ) override { v_x.clear(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// sets the set of active Variable of the BendersBFunction
 /** Sets the set of active Variable of the BendersBFunction. This method is
  * basically provided to work in tandem with the methods which only load the
  * "numerical data" of the BendersBFunction, i.e., deserialize() and
  * set_mapping( A , b , constraints , sides ). These (if the variables have
  * not been defined prior to calling them, see below) leave the
  * BendersBFunction in a somewhat inconsistent state whereby one knows the
  * data but not the input Variable, cue this method.
  *
  * Note that there are two distinct patterns of usage:
  *
  * - set_variables() is called *before* deserialize() or
  *   set_mapping( A , b , constraints , sides );
  *
  * - set_variables() is called *after* deserialize() or
  *   set_mapping( A , b , constraints , sides ).
  *
  * In the former case, the BendersBFunction is in a "well defined state" at
  * all times: after the call to set_variables() everything is there, only A
  * is empty and therefore the function does not depend on its active
  * variables x. In the latter case, however, the data is there, except that
  * the BendersBFunction has no input Variable; the object is in a
  * not-fully-consistent defined state. Having an Observer (then, Solver)
  * dealing with a call to set_variables() would be possible by issuing a
  * FunctionModVars( ... , AddVar ), but this is avoided because this method
  * is only thought to be called during initialization where the Observer is
  * not there already, whence no "issueMod" parameter.
  *
  * @param x a n-vector of pointers to ColVariable representing the x variable
  *        vector in the definition of the function. Note that the order of
  *        the variables in x is crucial, since the correspondence with A
  *        (whether already provided, or to be provided later, cf. discussion
  *        above) is positional: entry \p A[ i ][ j ] is (obviously) meant to
  *        be the coefficient of variable <tt>*x[ j ]</tt> for the i-th
  *        row. In other words, after the call to this method, <tt>x[ 0 ] ==
  *        get_active_var( 0 ), x[ 1 ] = get_active_var( 1 )</tt>, ...
  *
  * As the && implies, x become property of the BendersBFunction object. */

 void set_variables( VarVector && x );

/*--------------------------------------------------------------------------*/
 /// set the (only) sub-Block of the BendersBFunction
 /** This method sets the only sub-Block of the BendersBFunction (a.k.a. Block
  * B representing problem (B) in the definition of this BendersBFunction).
  *
  * @param block the pointer to a Block satisfying the conditions stated in
  *        the definition of this BendersBFunction.
  *
  * @param destroy_previous_block indicates whether the previous inner Block
  *        must be destroyed. The default value of this parameter is \c true,
  *        which means that the previous inner Block (if any) is destroyed and
  *        its allocated memory is released. */

 void set_inner_block( Block * block , bool destroy_previous_block = true ) {
  if( ( ! v_Block.empty() ) && block == v_Block.front() &&
      ( ! destroy_previous_block ) )
   return; // the given Block is already here; silently return

  if( destroy_previous_block && ( ! v_Block.empty() ) )
   delete v_Block.front();

  v_Block.clear();
  v_Block.push_back( block );

  if( block )
   block->set_f_Block( this );

  send_nuclear_modification();
  }

/*--------------------------------------------------------------------------*/
 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
  * - intMaxIter: This parameter is associated with the Solver::intMaxIter
  *               parameter of the CDASolver attached to the inner Block.
  *   Setting this parameter causes the corresponding parameter of the
  *   CDASolver of the inner Block to be overwritten. The setting of this
  *   parameter only takes effect if this BendersBFunction has an inner Block
  *   and this inner Block has a CDASolver attached to it. If the inner Block
  *   of this BendersBFunction has not yet been constructed or it has no
  *   CDASolver attached to it, attempting setting this parameter will raise
  *   an exception. The default value for this parameter is defined by the
  *   C05Function.
  *
  * - intLPMaxSz: This parameter is associated with the CDASolver::intMaxDSol
  *               parameter of the CDASolver attached to the inner Block.
  *   Setting this parameter causes the corresponding parameter of the
  *   CDASolver of the inner Block to be overwritten. The setting of this
  *   parameter only takes effect if this BendersBFunction has an inner Block
  *   and this inner Block has a CDASolver attached to it. If the inner Block
  *   of this BendersBFunction has not yet been constructed or it has no
  *   CDASolver attached to it, attempting setting this parameter will raise
  *   an exception. The default value for this parameter is defined by the
  *   C05Function.
  *
  * - intGPMaxSz: This parameter specifies the maximum number of Solution from
  *               the Solver of the inner Block that can be stored in the
  *   global pool, each one of which corresponds to a linearization. The
  *   default value for this parameter is defined by the C05Function.
  *
  * - intLinComp [7]: This parameter, coded bit-wise, determines how
  *                   linearizations are computed. This parameter specifies
  *   how linearization constants are computed and whether the Solution of the
  *   inner Block stored in the global pool are enough to re-compute the
  *   linearization coefficients and/or the linearization constant.
  *
  *   The first bit indicates how a linearization constant associated with the
  *   most recent call to has_linearization() must be computed. If it is set
  *   to 1, then the linearization constant is computed by using the bound
  *   provided by the Solver of the inner Block and the dual values of the
  *   Constraint handled by this BendersBFunction. More specifically, in this
  *   case, the linearization constant is computed as
  *
  *     bound - g'x
  *
  *   where x is the vector with the values of the active Variable of this
  *   BendersBFunction, g are the linearization coefficients associated with
  *   the last call to has_linearization(), and "bound" is a bound on the
  *   function value obtained in the last call to compute(). The computation
  *   of g involves only the RowConstraint handled by this BendersBFunction
  *   and the mapping matrix A.
  *
  *   If the first bit is 0, the linearization constant is computed based on
  *   the dual values of all RowConstraint of the inner Block (and its
  *   sub-Block, recursively) instead, and does not use any bound provided by
  *   the Solver of the inner Block.
  *
  *   If the second bit is set to 1, it indicates that the Solution of the
  *   inner Block stored in the global pool contains enough information to
  *   recompute the linearization coefficients. If it is set to 0, it
  *   indicates that changes made to the inner Block that affect the
  *   linearization coefficients will result in the deletion of the
  *   linearization from the global pool.
  *
  *   If the third bit is set to 1, it indicates that the Solution of the
  *   inner Block stored in the global pool contains enough information to
  *   recompute the linearization constant. If it is set to 0, it indicates
  *   that changes made to the inner Block that affect the linearization
  *   constant will result in the deletion of the linearization from the
  *   global pool.
  *
  *   The default value for this parameter is 7, which means that all bits
  *   mentioned above are set to 1.
  *
  * Any other parameter is handled by the C05Function.
  *
  * @param par The parameter to be set.
  *
  * @return The value of the parameter. */

 void set_par( idx_type par , int value ) override;

/*--------------------------------------------------------------------------*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. BendersBFunction takes
  * care of the following parameters. Except for the first parameter, each of
  * these parameters is associated with a parameter of the CDASolver of the
  * sub-Block of this BendersBFunction, given between parenthesis.
  *
  * - dblAAccMlt
  * - dblMaxTime  ( Solver::dblMaxTime )
  * - dblRelAcc   ( Solver::dblRelAcc )
  * - dblAbsAcc   ( Solver::dblAbsAcc )
  * - dblUpCutOff ( Solver::dblUpCutOff )
  * - dblLwCutOff ( Solver::dblLwCutOff )
  * - dblRAccLin  ( CDASolver::dblRAccDSol )
  * - dblAAccLin  ( CDASolver::dblAAccDSol )
  *
  * Setting any of these parameters (except the first one) causes the
  * corresponding parameter of the CDASolver of the sub-Block to be
  * overwritten by the given \c value. The setting of these parameters only
  * take effect if this BendersBFunction has a sub-Block and this sub-Block
  * has a CDASolver attached to it. Any other parameter is handled by the
  * C05Function.
  *
  * @param par The parameter to be set.
  *
  * @return The value of the parameter. */

 void set_par( idx_type par , double value ) override {

  auto solver = get_solver< CDASolver >();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::set_par: the inner Block "
                                 "must have a CDASolver attached to it." ) );

  switch( par ) {
   case( dblAAccMlt ):
    AAccMlt = value;
    break;

   case( dblMaxTime ):
    solver->set_par( Solver::dblMaxTime , value );
    break;

   case( dblRelAcc ):
    solver->set_par( Solver::dblRelAcc , value );
    break;

   case( dblAbsAcc ):
    solver->set_par( Solver::dblAbsAcc , value );
    break;

   case( dblUpCutOff ):
    solver->set_par( Solver::dblUpCutOff , value );
    break;

   case( dblLwCutOff ):
    solver->set_par( Solver::dblLwCutOff , value );
    break;

   case( dblRAccLin ):
    solver->set_par( CDASolver::dblRAccDSol , value );
    break;

   case( dblAAccLin ):
    solver->set_par( CDASolver::dblAAccDSol , value );
    break;

   default: C05Function::set_par( par , value );
   }
  }

/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the BendersBFunction
 *  @{ */

 [[nodiscard]] idx_type get_num_int_par( void ) const override {
  return( intLastBendersBFPar );
  }

/*--------------------------------------------------------------------------*/

 /// get a specific integer (int) numerical parameter
 /** Get a specific integer (int) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
  * - intMaxIter
  * - intLPMaxSz
  * - intGPMaxSz
  * - intLinComp
  *
  * Any other parameter is handled by the C05Function.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the required parameter. */

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  switch( par ) {
   case( intMaxIter ): return( get_solver_int_par( Solver::intMaxIter ) );
   case( intLPMaxSz ): return( get_solver_int_par( CDASolver::intMaxDSol ) );
   case( intGPMaxSz ): return( global_pool.size() );
   case( intLinComp ): return( LinComp );
   }
  return( C05Function::get_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/
 /// get a specific float (double) numerical parameter
 /** Get a specific float (double) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
  * - dblAAccMlt
  * - dblMaxTime
  * - dblRelAcc
  * - dblAbsAcc
  * - dblUpCutOff
  * - dblLwCutOff
  * - dblRAccLin
  * - dblAAccLin
  *
  * Any other parameter is handled by the C05Function.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the required parameter. */

 [[nodiscard]] double get_dbl_par( idx_type par ) const override {
  switch( par ) {
   case( dblAAccMlt ):  return( AAccMlt );
   case( dblMaxTime ):  return( get_solver_dbl_par( Solver::dblMaxTime ) );
   case( dblRelAcc ):   return( get_solver_dbl_par( Solver::dblRelAcc ) );
   case( dblAbsAcc ):   return( get_solver_dbl_par( Solver::dblAbsAcc ) );
   case( dblUpCutOff ): return( get_solver_dbl_par( Solver::dblUpCutOff ) );
   case( dblLwCutOff ): return( get_solver_dbl_par( Solver::dblLwCutOff ) );
   case( dblRAccLin ): return( get_solver_dbl_par( CDASolver::dblRAccDSol ) );
   case( dblAAccLin ): return( get_solver_dbl_par( CDASolver::dblAAccDSol ) );
   }

  return( C05Function::get_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] int get_dflt_int_par( idx_type par ) const override {
  if( par == intLinComp )
   return( 7 );
  return( C05Function::get_dflt_int_par( par ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] idx_type int_par_str2idx( const std::string & name )
  const override {
  if( name == "intLinComp" )
   return( intLinComp );
  return( C05Function::int_par_str2idx( name ) );
  }

/*--------------------------------------------------------------------------*/

 [[nodiscard]] const std::string & int_par_idx2str( idx_type idx )
  const override {
  static const std::vector< std::string > pars = { "intLinComp" };

  if( idx == intLinComp )
   return( pars[ 0 ] );

  return( C05Function::int_par_idx2str( idx ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS FOR HANDLING THE State OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the State of the BendersBFunction
 */

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
/*----------------- METHODS FOR MANAGING THE "IDENTITY" --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Managing the "identity" of the BendersBFunction
 *
 * Actually implement the methods of ThinComputeInterface relative to
 * temporarily changing the "identity" of the BendersBFunction.
 *
 *  @{ */

 /// set the "identity" of the BendersBFunction
 /** Actually implement the ThinComputeInterface::set_id() by setting the
  * f_id member of the BendersBFunction class, which is initialized with
  * "this" in the constructor. BendersBFunction always uses f_id to try to
  * "lock and own" the Block. This method allows to change f_id ("lend
  * another identity"); when called with nullptr argument, the id() is reset
  * to the default "this". Also, if the inner Block is set and has
  * registered Solver, their identity is also set (or reset) in anticipation
  * that they also may have to lock() the inner Block during their line of
  * work. */

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
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE BendersBFunction -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * BendersBFunction; this is the actual concrete implementation exploiting
 * the vector v_x of pointers.
 * @{ */

 Index get_num_active_var( void ) const override final {
  return( v_x.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Index is_active( const Variable * const var ) const override final
 {
  auto idx = std::find( v_x.begin() , v_x.end() , var );
  if( idx == v_x.end() )
   return( Inf< Index >() );
  else
   return( std::distance( v_x.begin() , idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void map_active( c_Vec_p_Var & vars , Subset & map , bool ordered = false )
  const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Variable * get_active_var( Index i ) const override final {
  return( v_x[ i ] );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override final {
  return( new BendersBFunction::v_iterator( v_x.begin() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_begin( void ) const override final {
  return( new BendersBFunction::v_const_iterator( v_x.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override final {
  return( new BendersBFunction::v_iterator( v_x.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_end( void ) const override final {
  return( new BendersBFunction::v_const_iterator( v_x.end() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE BendersBFunction -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the BendersBFunction
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// sets the mapping used to update the constraints of the sub-Block
 /** This method sets the (linear) mapping M describing how the set of
  * Constraint of the inner Block is affected by the values of the active
  * Variable x of this BendersBFunction. The affected Constraint are assumed
  * to be RowConstraint and only the left- or right-hand side (or both) of the
  * RowConstraint are affected by the values of x.
  *
  * The mapping is given by a matrix A, having m rows and n columns, and a
  * vector b with size n, where m is the number of affected Constraint and n
  * is the number of active Variable of this BendersBFunction. This mapping is
  * defined as M(x) = Ax + b, and the i-th entry of M(x), denoted by [ M(x)
  * ]_i gives the value of the left- or right-hand side (or both in the case
  * of an equality constraint) of the i-th affected RowConstraint. The pointer
  * to the i-th affected RowConstraint is given by the first element of the
  * i-th pair of the vector \p constraints. The second element of the i-th
  * pair of the vector \p constraints indicates which sides of the i-th
  * affected RowConstraint is associated with [ M(x) ]_i. Notice that if both
  * the left- and right-hand side of a Constraint are affected, but in
  * different ways, then the mapping would have two entries associated with
  * this Constraint, one for the left- and the other for the right-hand side
  * of this Constraint.
  *
  *      AT THE TIME THIS METHOD IS INVOKED, THE VARIABLES x MUST HAVE ALREADY
  *      BEEN SET AND THE NUMBER OF COLUMNS OF THE A MATRIX MUST BE EQUAL TO
  *      THE SIZE OF x. ALSO, THE NUMBER OF ROWS OF A MUST BE EQUAL TO BOTH
  *      THE SIZE OF b AND THE SIZE OF constraints. IF ANY OF THOSE CONDITIONS
  *      IS NOT MET, AN EXCEPTION IS THROWN.
  *
  * @param A The matrix A in the linear mapping.
  *
  * @param b The vector b in the linear mapping.
  *
  * @param constraints the vector of (pointers to) affected RowConstraint.
  *
  * @param sides the vector containing the affected sides of the affected
  *        RowConstraint. There is a correspondence between the vectors \p
  *        side and \p constraints: \p sides[i] is associated with \p
  *        constraints[i]. Of course, \p sides and \p constraints must have
  *        the same size.
  *
  * @param issueMod It indicates if and how the FunctionMod (with f_shift ==
  *        FunctionMod::NaNshift, i.e., "everything changed") is issued, as
  *        described in Observer::make_par().
  *
  * As the && implies, \p A, \p b, and \p constraints become property of this
  * object. */

 void set_mapping( MultiVector && A , RealVector && b ,
                   ConstraintVector && constraints ,
                   ConstraintSideVector && sides ,
                   ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add a set of new Variable to the BendersBFunction
 /** This method adds a new set of Variable to the BendersBFunction, possibly
  * together with the columns of matrix A associated with these new Variable.
  * The method receives:
  *
  * @param nx a <tt>std::vector< ColVariable * > &&</tt> containing the
  *        pointers to k new ColVariable, which will take indices n, n + 1,
  *        ..., n + k - 1 where <tt>n = get_num_active_var()</tt> (*before*
  *        the call). Note that the order of the variables in nx dictates the
  *        index of the "active" Variable: after the call, <tt>nx[ 0 ] ==
  *        get_active_var( n ), nx[ 1 ] = get_active_var( n + 1 )</tt>,
  *        ... Also, the correspondence with nA is (see below)
  *        positional. Note that
  *
  *            IT IS EXPECTED THAT NONE OF THE NEW ColVariable IS ALREADY
  *            "ACTIVE" IN THE BendersBFunction, BUT NO CHECK IS DONE TO
  *            ENSURE THIS
  *
  *        Indeed, the check is costly, and the BendersBFunction does not
  *        really have a functional issue with repeated ColVariable. The
  *        real issue rather comes whenever the BendersBFunction is used
  *        within a Constraint or Objective that need to register itself
  *        among the "active" Variable of the BendersBFunction; this
  *        process is not structured to work with multiple copies of the same
  *        "active" Variable. Thus, a BendersBFunction used within such an
  *        object should not have repeated Variable, but if this is an issue
  *        then the check will have to be performed elsewhere.
  *
  * @param nA a MultiVector && having as many rows as the current A matrix (if
  *        the current A matrix is not empty) and exactly k columns
  *        representing the new part of the linear mapping; entry \p nA[ i ][
  *        h ] is (obviously) meant to be the coefficient of *nx[ h ] for the
  *        i-th row. This parameter is optional. If it is not provided, then
  *        the columns of the A matrix of the linear mapping associated with
  *        the new given Variable must be set later by calling, for example,
  *        set_mapping() or add_column().
  *
  * @param issueMod which decides if and how the C05FunctionModVarsAddd (since
  *        a BendersBFunction is strongly quasi-additive, and with shift() ==
  *        0 as expected) is issued, as described in Observer::make_par().
  *
  * As the && tells, nx and nA become "property" of the BendersBFunction
  * object, although this likely only happens if A is currently "empty of
  * columns" (say, only the rows have been defined, or all previous columns
  * have been deleted). */

 void add_variables( VarVector && nx , MultiVector && nA = {} ,
                     ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the BendersBFunction
 /** Like add_variables(), but it adds just only one Variable:
  *
  * @param var is a ColVariable *, and the pointed ColVariable must *not* be
  *        already among the active Variable of the BendersBFunction.
  *
  * @param Aj is a RealVector that, if present, must have size equal to the
  *        number of rows of the current A matrix (if A is not currently
  *        empty) and contain the new column of the linear mapping associated
  *        with the new active Variable; entry \p Aj[ i ][ h ] is (obviously)
  *        meant to be the coefficient of *var for the i-th row. This
  *        parameter is optional. If it is not provided, then the column of
  *        the A matrix of the linear mapping associated with the new given
  *        Variable must be set later by calling, for example, set_mapping()
  *        or add_column().
  *
  * @param issueMod which decides if and how the C05FunctionModVarsAddd
  *        (since a BendersBFunction is strongly quasi-additive, and with
  *        shift() == 0 as expected) is issued, as described in
  *        Observer::make_par(). */

 void add_variable( ColVariable * var , c_RealVector & Aj = {} ,
                    ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove the i-th active Variable
 /** This method removes the active Variable whose index is \p i.
  *
  * @param i the index of the Variable to be removed. It must be an integer
  *        between 0 and get_num_active_var() - 1.
  *
  * @param issueMod decides if and how the C05FunctionModVarsRngd (since a
  *        BendersBFunction is strongly quasi-additive, and with shift() == 0
  *        as expected) is issued, as described in Observer::make_par(). */

 void remove_variable( Index i , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// remove a range of active Variable
 /** This method removes a range of "active" Variable.
  *
  * @param range contains the indices of the Variable to be deleted
  *        (hence, range.second <= get_num_active_var());
  *
  * @param issueMod decides if and how the C05FunctionModVarsRngd (since a
  *        BendersBFunction is strongly quasi-additive, and with shift() == 0
  *        as expected) is issued, as described in Observer::make_par(). */

 void remove_variables( Range range , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// remove a subset of Variable
 /** This method removes all the Variable in the given set of indices. If \p
  * nms is empty, all Variable are removed.
  *
  * @param nms a Subset & containing the indices of the Variable to be
  *        removed, i.e., integers between 0 and get_num_active_var() - 1. If
  *        \p nms is empty, all Variable are removed.
  *
  * @param ordered a bool indicating if \p nms[] is already ordered in
  *        increasing sense (otherwise this is done inside the method,
  *        which is why \p nms[] is not const).
  *
  * @param issueMod decides if and how the C05FunctionModVars (with f_shift ==
  *        0, since a BendersBFunction is strongly quasi-additive) is issued,
  *        as described in Observer::make_par(). */

 void remove_variables( Subset && nms , bool ordered = false ,
                        ModParam issueMod = eModBlck ) override final;

/*--------------------------------------------------------------------------*/
 /// modify a range of rows of the linear mapping
 /** Modifies a range of rows of the linear mapping:
  *
  * @param range contains the indices of the rows to be modified, hence
  *        <tt>range.second <= get_b().size()</tt>;
  *
  * @param nA a MultiVector && with <tt>nA.size() == range.second -
  *        range.first</tt>, and exactly as many columns as the current A
  *        matrix: entry \p nA[ i ][ h ] is (obviously) meant to be the new
  *        coefficient for the h-th variable in row <tt>range.first + i</tt>;
  *        as the && implies, \p nA becomes property of the BendersBFunction
  *        object;
  *
  * @param nb a vector of Function::FunctionValue with <tt>nb.size() ==
  *        range.second - range.first</tt>: entry \p nb[ i ] is (obviously)
  *        meant to be the new value of the constant term for row
  *        <tt>range.first + i</tt>;
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par(). Note that shift() == NANshift
  *        (the function has changed "unpredictably"), type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and BFtype() ==
  *        ModifyRows. */

 void modify_rows( MultiVector && nA , c_RealVector & nb , Range range ,
                   ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify a subset of rows of the linear mapping
 /** Modifies a subset of rows of the linear mapping:
  *
  * @param rows contains the indices of the rows to be modified; all entries
  *        must therefore be numbers in 0, ..., get_A().size() - 1; as the &&
  *        tells, the vector becomes property of the BendersBFunction, to be
  *        dispatched to the issued BendersBFunctionModSbst (if any);
  *
  * @param ordered tells if \p rows is already ordered by increasing index (if
  *        not it may be ordered inside, after all it becomes property of the
  *        BendersBFunction);
  *
  * @param nA a MultiVector && with \p nA.size() == \p rows.size() (with one
  *        possible exception, see later) and exactly as many columns as the
  *        current A matrix; entry \p nA[ i ][ h ] is (obviously) meant to be
  *        the new coefficient for the h-th variable in row \p rows[ i ]; as
  *        the && implies, nA becomes property of the BendersBFunction object;
  *
  * @param nb a vector of Function::FunctionValue with \p nb.size() ==
  *        \p rows.size(): entry \p nb[ i ] is (obviously) meant to be the
  *        new value of the constant term for row \p rows[ i ];
  *
  * @param issueMod which decides if and how the BendersBFunctionModSbst is
  *        issued, as described in Observer::make_par(). Note that shift() ==
  *        NANshift (the function has changed "unpredictably"),  type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and BFtype() ==
  *        ModifyRows. */

 void modify_rows( MultiVector && nA , c_RealVector & nb , Subset && rows ,
                   bool ordered = false , ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify one single row of the linear mapping
 /** Like modify_rows(), but only for one row:
  *
  * @param i is the index of the row to be modified;
  *
  * @param Ai is the new RealVector, with exactly n = get_num_active_var()
  *        elements, to replace the existing vector of coefficients in the
  *        i-th linear mapping; as the && tells, \p Ai becomes "property" of
  *        the BendersBFunction object and physically replaces the previous
  *        vector;
  *
  * @param bi is the new constant term of the i-th mapping;
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par(). Note that shift() == NANshift
  *        (the function has changed "unpredictably"), type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and BFtype() ==
  *        ModifyRows. */

 void modify_row( Index i , RealVector && Ai , FunctionValue bi ,
                  ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a range of rows of the linear mapping
 /** Like modify_rows( range ), but modify the constant terms only.
  *
  * @param nb a vector of Function::FunctionValue with <tt>nb.size() ==
  *        range.second - range.first</tt>: entry \p nb[ i ] is (obviously)
  *        meant to be the new value of the constant term for row
  *        <tt>range.first + i</tt>;
  *
  * @param range contains the indices of the rows to be modified, hence
  *        <tt>range.second <= get_b().size()</tt>;
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and BFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does
  *        nothing). */

 void modify_constants( c_RealVector & nb , Range range ,
                        ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a subset of rows of the linear mapping
 /** Like modify_rows( subset ), but modify the constant terms only.
  *
  * @param rows contains the indices of the rows to be modified; all entries
  *        must therefore be numbers in 0, ..., get_A().size() - 1; as the &&
  *        tells, the vector becomes property of the BendersBFunction, to be
  *        dispatched to the issued BendersBFunctionModSbst (if any);
  *
  * @param ordered tells if \p rows is already ordered by increasing index
  *        (if not it may be ordered inside, after all it becomes property
  *        of the BendersBFunction);
  *
  * @param nb a vector of Function::FunctionValue with <tt>nb.size() ==
  *        rows.size()</tt>: entry \p nb[ i ] is (obviously) meant to be the
  *        new value of the constant term for row \p rows[ i ] of the linear
  *        mapping;
  *
  * @param issueMod decides if and how the BendersBFunctionModSbst is issued,
  *        as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and BFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does
  *        nothing). */

 void modify_constants( c_RealVector & nb , Subset && rows ,
                        bool ordered , ModParam issueMod );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a range of rows of the linear mapping
 /** Like modify_rows( range ), but modify the constant terms only.
  *
  * @param nb an iterator to a vector of double containing the new values of
  *        the constants. This iterator must have at least <tt>range.second -
  *        range.first</tt> valid successors (including \p nb itself).  For
  *        each i in [range.first, range.second), the i-th successor of \p nb
  *        provides the new value of the constant term for row <tt>range.first
  *        + i</tt>;
  *
  * @param range contains the indices of the rows whose constants will be
  *        modified, hence <tt>range.second <= get_b().size()</tt>;
  *
  * @param issuePMod this parameter is ignored as this function does not issue
  *        any physical modifications.
  *
  * @param issueAMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and BFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does nothing). */

 void modify_constants( MF_dbl_it nb , Range range = Block::INFRange ,
                        ModParam issuePMod = eNoBlck ,
                        ModParam issueAMod = eNoBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a subset of rows of the linear mapping
 /** Like modify_rows( subset ), but modify the constant terms only.
  *
  * @param nb an iterator to a vector of double containing the new values of
  *        the constants. This iterator must have at least
  *        <tt>rows.size()</tt> valid successors (including \p nb itself).
  *        For each i in {0, ..., rows.size()-1}, the i-th successor of \p nb
  *        provides the new value of the constant term for \p rows[ i ] of the
  *        linear mapping;
  *
  * @param rows contains the indices of the rows to be modified; all entries
  *        must therefore be numbers in 0, ..., get_A().size() - 1; as the &&
  *        tells, the vector becomes property of the BendersBFunction, to be
  *        dispatched to the issued BendersBFunctionModSbst (if any);
  *
  * @param ordered tells if \p rows is already ordered by increasing index
  *        (if not it may be ordered inside, after all it becomes property
  *        of the BendersBFunction);
  *
  * @param issuePMod this parameter is ignored as this function does not issue
  *        any physical modifications.
  *
  * @param issueAMod decides if and how the BendersBFunctionModSbst is issued,
  *        as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and BFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does
  *        nothing). */

 void modify_constants( MF_dbl_it nb , Subset && rows ,
			bool ordered = false ,
                        ModParam issuePMod = eNoBlck ,
                        ModParam issueAMod = eNoBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify only the constant term of one row of the linear mapping
 /** Like modify_constants(), but only for one row:
  *
  * @param i is the index of the row to be modified;
  *
  * @param bi is the new constant term of the i-th row of the linear mapping;
  *
  * @param issueMod which decides if and how the BendersBFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and BFtype() == ModifyCnst. As for
  *        shift(), the value of the function changes in a very predictable
  *        way: if bi is > than the current value the function has
  *        necessarily increased, otherwise necessarily decreased (if it is
  *        == it has not changed and the method does nothing), hence the
  *        shift is either +INFshift or -INFshift accordingly. */

 void modify_constant( Index i , FunctionValue bi ,
                       ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add some rows to the linear mapping in the BendersBFunction
 /** Adds some rows to the linear mapping in the BendersBFunction, leaving
  * the current set of n = get_num_active_var() input Variable and all the
  * current rows:
  *
  * @param nA a k-vector of n-vectors of Function::FunctionValue representing
  *        the new rows of the A matrix in the definition of the linear
  *        mapping; entry \p nA[ i ][ j ] is (obviously) meant to be the
  *        coefficient of variable <tt>*x[ j ]</tt> for the i-th new row; as
  *        the && tells, the object (most likely, its individual rows) becomes
  *        "property" of the BendersBFunction.
  *
  * @param nb a k-vector of Function::FunctionValue representing the new
  *        entries of b vector in the definition of the linear mapping (that
  *        is, \p nb[ i ] is the constant factor of the i-th given row of the
  *        linear mapping);
  *
  * @param nc the i-th element of this vector includes the pointer to the
  *        RowConstraint associated with the given i-th row.
  *
  * @param ns the vector containing the affected sides of the affected
  *        RowConstraint. There is a correspondence between the vectors \p ns
  *        and \p nc: \p ns[i] is associated with \p nc[i], i.e., ns[i] is the
  *        affected side of the nc[i] RowConstraint. Of course, ns and nc must
  *        have the same size.
  *
  * @param issueMod decides if and how the BendersBFunctionModAdd is issued,
  *        as described in Observer::make_par().
  */

 void add_rows( MultiVector && nA , c_RealVector & nb ,
                const ConstraintVector & nc ,
                const ConstraintSideVector & ns ,
                ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new row to the linear mapping
 /** Like add_row(), but just only one row of the linear mapping:
  *
  * @param Ai is the RealVector, with exactly n = get_num_active_var()
  *        elements, with the coefficients of the new row in the mapping; as
  *        the && tells, \p Ai becomes "property" of the BendersBFunction
  *        object;
  *
  * @param bi is the constant term of the new row in the mapping;
  *
  * @param ci is the pointer to the RowConstraint for the new row.
  *
  * @param si is the #ConstraintSide for the new row.
  *
  * @param issueMod decides if and how the BendersBFunctionModAdd is issued,
  *        as described in Observer::make_par().
  */

 void add_row( RealVector && Ai , FunctionValue bi , RowConstraint * ci ,
               ConstraintSide si , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// deletes a range of rows from the linear mapping in the BendersBFunction
 /** Deletes a range rows from the linear mapping in the BendersBFunction,
  * leaving the current set of n = get_num_active_var() input Variable and
  * all rows that are not explicitly deleted:
  *
  * @param range contains the indices of the rows to be deleted, hence
  *        <tt>range.second <= get_b().size()</tt>.
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par().
  *
  * TODO: if the deleted rows are "too many", rather issue a FunctionMod
  * with shift() == FunctionMod::NaNshift, i.e., "everything changed"
  * (cf. delete_rows( all )). */

 void delete_rows( Range range , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// deletes a subset of rows from the linear mapping
 /** Deletes a subset of rows from the linear mapping in the
  * BendersBFunction, leaving the current set of n = get_num_active_var()
  * input Variable and all rows that are not explicitly deleted:
  *
  * @param rows contains the indices of the rows to be deleted; all entries
  *        must therefore be numbers in 0, ..., get_A().size() - 1; as the &&
  *        tells, the vector becomes property of the BendersBFunction, to be
  *        dispatched to the issued BendersBFunctionModSbst (if any);
  *
  * @param ordered tells if \p rows is already ordered by increasing index (if
  *        not it may be ordered inside, after all it becomes property of the
  *        BendersBFunction);
  *
  * @param issueMod decides if and how the BendersBFunctionModSbst is issued,
  *        as described in Observer::make_par().
  *
  * TODO: if the deleted rows are "too many", rather issue a FunctionMod
  * with shift() == FunctionMod::NaNshift, i.e., "everything changed"
  * (cf. delete_rows( all )). */

 void delete_rows( Subset && rows , bool ordered = false ,
                   ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes one single existing row from the linear mapping
 /** Like delete_rows(), but just only the i-th row of the linear mapping.
  *
  * @param i the index of the row to be deleted; it must be an integer between
  *        0 and get_A().size() - 1;
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par().
  */

 void delete_row( Index i , ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes all rows from the linear mapping in the BendersBFunction
 /** Like delete_rows( range ), but immediately removes *all* the matrix A
  * and vector b, leaving the mapping "empty". Since no previous linearization
  * is valid after deleting all rows, a FunctionMod with shift() ==
  * FunctionMod::NaNshift, i.e., "everything changed", is issued. */

 void delete_rows( ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/

 /// set the whole set of parameters of this BendersBFunction
 /** The extra Configuration of the given ComputeConfig (see
  * ComputeConfig::f_extra_Configuration), if not nullptr, is assumed to be of
  * type SimpleConfiguration< std::map< std::string , Configuration * > >. If
  * it is not of this type, an exception is thrown. The map in that
  * SimpleConfiguration is meant to provide pointers to a number of
  * Configuration, that should be used in different situations. The following
  * keys, and their corresponding Configuration, are considered:
  *
  * - "BlockConfig": a pointer to a BlockConfig to be applied to the inner
  *    Block of this BendersBFunction.
  *
  * - "BlockSolverConfig": a pointer to a BlockSolverConfig to be applied to
  *    the inner Block of this BendersBFunction.
  *
  * - "get_dual_solution_partial": a pointer to a Configuration to be passed
  *    to the method Solver::get_dual_solution() of the Solver attached to the
  *    inner Block, whenever this method is invoked for obtaining the dual
  *    values of the RowConstraint handled by this BendersBFunction (i.e., the
  *    RowConstraint that are affected by the active Variable of this
  *    BendersBFunction).
  *
  * - "get_dual_direction_partial": a pointer to a Configuration to be passed
  *    to the method Solver::get_dual_direction() of the Solver attached to
  *    the inner Block, whenever this method is invoked for obtaining the dual
  *    values of the RowConstraint handled by this BendersBFunction (i.e., the
  *    RowConstraint that are affected by the active Variable of this
  *    BendersBFunction).
  *
  * - "get_dual_partial": a pointer to a Configuration to be passed to the
  *    methods Solver::get_dual_solution() and Solver::get_dual_direction() of
  *    the Solver attached to the inner Block, whenever these methods are
  *    invoked for obtaining the dual values of the RowConstraint handled by
  *    this BendersBFunction (i.e., the RowConstraint that are affected by the
  *    active Variable of this BendersBFunction). Clearly, this key is defined
  *    for convenience, so as to avoid the need of passing two identical
  *    Configuration by means of the "get_dual_solution_partial" and
  *    "get_dual_direction_partial" keys.
  *
  * - "get_dual_solution": a pointer to a Configuration to be passed to the
  *    method Solver::get_dual_solution() of the Solver attached to the inner
  *    Block, whenever this method is invoked for obtaining the dual values of
  *    all RowConstraint of the inner Block (and their sub-Block,
  *    recursively).
  *
  * - "get_dual_direction": a pointer to a Configuration to be passed to the
  *    method Solver::get_dual_direction() of the Solver attached to the inner
  *    Block, whenever this method is invoked for obtaining the dual values of
  *    all RowConstraint of the inner Block (and their sub-Block,
  *    recursively).
  *
  * - "get_dual": a pointer to a Configuration to be passed to the methods
  *    Solver::get_dual_solution() and Solver::get_dual_direction() of the
  *    Solver attached to the inner Block, whenever these methods are invoked
  *    for obtaining the dual values of all RowConstraint of the inner Block
  *    (and their sub-Block, recursively). Clearly, this key is defined for
  *    convenience, so as to avoid the need of passing two identical
  *    Configuration by means of the "get_dual_solution" and
  *    "get_dual_direction" keys.
  *
  * If a key, that is not any of the above, is provided, an exception is
  * thrown. No pointer is kept by the BendersBFunction, so the caller can (and
  * is responsible to) delete any pointer provided.
  *
  * If the given pointer to the ComputeConfig is nullptr, then the
  * Configuration of the BendersBFunction is reset to its default. This means
  * that
  *
  *  (1) all parameters of the BendersBFunction are reset to their default
  *      values;
  *
  *  (2) the inner Block (if any) is configured to its default configuration;
  *
  *  (3) the Solver of the inner Block (and their sub-Block, recursively) are
  *      unregistered and deleted.
  *
  * If the given pointer to the ComputeConfig is not nullptr but its extra
  * Configuration is nullptr, then (2) and (3) above are performed. If the
  * pointer to the BlockConfig (if provided) in the extra Configuration is
  * nullptr, then (2) above is performed. If the pointer to the
  * BlockSolverConfig (if provided) in the extra Configuration is nullptr,
  * then (3) above is performed.
  *
  * @param scfg a pointer to a ComputeConfig.
  */

 void set_ComputeConfig( ComputeConfig *scfg = nullptr ) override;

/** @} ---------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

 void add_Modification( sp_Mod mod , Observer::ChnlName chnl = 0 ) override;

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR Saving THE DATA OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Saving the data of the BendersBFunction
 *  @{ */

 /// print information about the BendersBFunction on an ostream
 /** Prints some very basuc information about the BendersBFunction; vlvl is
  * ignored, and no "complete" format is implemented. */

 void print( std::ostream & output , char vlvl = 0 ) const override {
  output << "BendersBFunction [" << this << "]" << " with "
	 << get_num_active_var() << " active variables and a"
         << " mapping with " << get_b().size() << " rows.";
  }

/*--------------------------------------------------------------------------*/
 /// serialize a BendersBFunction into a netCDF::NcGroup
 /** Serialize a BendersBFunction into a netCDF::NcGroup. Note that,
  * BendersBFunction being both a Function and a Block, the netCDF::NcGroup
  * will have to have the "standard format of a :Block", meaning whatever is
  * managed by the serialize() method of the base Block class, plus the
  * BendersBFunction-specific data with the following format:
  *
  * - The dimension "NumVar" containing the number of columns of the A matrix,
  *   i.e., the number of active variables.
  *
  * - The dimension "NumRow" containing the number of rows of the A
  *   matrix. This dimension is optional; if it is not provided then 0 (no
  *   rows) is assumed.
  *
  * - The dimension "NumNonzero", of type netCDF::NcUint, containing the
  *   number of nonzero entries in the A matrix. This dimension is optional
  *   and determines in which format the matrix A is given. If this dimension
  *   is present, then "NumRow" must also be. If "NumNonzero" is not present,
  *   then the A matrix is given as a dense matrix. If it is present, then the
  *   A matrix is given in a sparse format as defined by the variables "Row",
  *   "Column", and "A". During serialization, the following criterion is used
  *   to decide the format in which the A matrix is stored. If at most 25% of
  *   its elements are nonzero, it is stored in sparse format; otherwise, it
  *   is stored in dense format.
  *
  * - The variable "NumNonzeroAtRow", of type netCDF::NcUint and indexed over
  *   the dimension "NumRow", containing the number of nonzero elements of the
  *   A matrix in each row.  If the dimension "NumNonzero" is not present,
  *   then this variable is ignored.  If "NumNonzero" is present, then
  *   "NumNonzeroAtRow" is optional only if "NumVar", "NumRow", and
  *   "NumNonzero" are equal. In this case, if "NumNonzeroAtRow" is not
  *   provided, then the A matrix is assumed to be the identity matrix. If
  *   "NumVar", "NumRow", and "NumNonzero" are not all equal, then
  *   "NumNonzeroAtRow" must be provided. For each i in {0, ..., NumRow-1},
  *   NumNonzeroAtRow[i] is the number of nonzero elements in the i-th row of
  *   the A matrix.
  *
  * - The variable "Column", of type netCDF::NcUint and indexed over the
  *   dimension "NumNonzero", containing the column indices of the entries of
  *   the matrix. If "NumNonzero" or "NumNonzeroAtRow" is not present, then this
  *   variable is ignored. For each k in {0, ..., NumNonzero-1}, Column[k] is
  *   the column index of the k-th nonzero entry of the A matrix, whose value
  *   is given by A[k]. "Column" stores the column indices in row-major order,
  *   that is, if (i,j) and (p,q) are the entries of the k-th and l-th nonzero
  *   elements of A, respectively, with k < l, then i <= p.
  *
  * - The variable "A", of type netCDF::NcDouble. This variable stores the
  *   values of the elements of the A matrix of the mapping. If the dimension
  *   "NumNonzero" is provided but the variable "NumNonzeroAtRow" is not, then
  *   the A matrix is assumed to be the identity matrix and the variable "A"
  *   is ignored. If both the dimension "NumNonzero" and the variable
  *   "NumNonzeroAtRow" are provided, then the "A" variable is indexed over
  *   the "NumNonzero" dimension and contains the values of the (potentially)
  *   nonzero entries of the matrix. In this case, A[k] is the value of the
  *   k-th nonzero entry, whose column index is given by Column[k]. The
  *   nonzero elements of the matrix are given in left-to-right top-to-bottom
  *   (``row-major'') order. If the dimension "NumNonzero" is not present,
  *   then "A" is indexed over both the "NumRow" and "NumVar" dimensions (in
  *   this order); it contains the (row-major) representation of the matrix
  *   A. This variable is optional only if NumRow == 0.
  *
  * - The variable "b", of type netCDF::NcDouble and indexed over the
  *   dimension "NumRow", which contains the vector b. This variable is
  *   optional. If it is not provided, then we assume b[i] = 0 for each i in
  *   {0, ..., NumRow - 1}.
  *
  * - The variable "ConstraintSide", of type netCDF::NcChar and indexed over
  *   the dimension "NumRow", indicating, at position i, which side of the
  *   i-th Constraint is affected. The possible values are `L' for the
  *   left-hand (or lower bound) side, `R' for the right-hand (or upper bound)
  *   side, and `B' for both sides. This variable is optional. If it is not
  *   provided, then all entries of this array are assumed to be `B'.
  *
  * - The group "AbstractPath" containing the description of a vector of
  *   AbstractPath to the affected RowConstraints. The number of AbstractPath
  *   in that vector must be equal to the number of rows of the A matrix and,
  *   therefore, the dimension associated with the number of AbstractPath is
  *   "NumRow". The i-th AbstractPath in this vector must be the path to the
  *   i-th affected RowConstraint (which is associated with the i-th row of
  *   the A matrix). This group is optional only if NumRow == 0. Each
  *   AbstractPath is taken with respect to the inner Block of this
  *   BendersBFunction (i.e., the inner Block of this BendersBFunction must be
  *   the reference Block of the path).
  *
  * - The group "Block", containing the description of the inner Block. */

 void serialize( netCDF::NcGroup & group ) const override;

/** @} ---------------------------------------------------------------------*/
/*------- METHODS DESCRIBING THE BEHAVIOR OF THE BendersBFunction ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the BendersBFunction
 *  @{ */

 /// compute the BendersBFunction

 int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// returns the value of the BendersBFunction
 /** This method returns an approximation to the value of this
  * BendersBFunction associated with the most recent call to compute(). The
  * returned value depends on the sense of the Objective of the sub-Block. If
  * the sense of the Objective of the sub-Block is "minimization", then this
  * method returns a valid upper bound on the optimal objective function value
  * of the sub-Block (see Solver::get_ub()). If the sense of the Objective of
  * the sub-Block is "maximization", then this method returns a valid upper
  * bound on the optimal objective function value of the sub-Block (see
  * Solver::get_lb()).
  *
  * Notice that if compute() has never been invoked, then the value returned
  * by this method is meaningless. Moreover, if this BendersBFunction does not
  * have a sub-Block or its sub-Block does not have a Solver attached to it,
  * then an exception is thrown. */

 FunctionValue get_value( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a lower estimate of the BendersBFunction
 /** This method simply returns get_value().  */

 FunctionValue get_lower_estimate( void ) const override {
  return( get_value() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns an upper estimate of the BendersBFunction
 /** This method simply returns get_value().  */

 FunctionValue get_upper_estimate( void ) const override {
  return( get_value() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the "constant term" of the BendersBFunction

 FunctionValue get_constant_term( void ) const override;

/*--------------------------------------------------------------------------*/
 /// returns true only if this BendersBFunction is convex
 /** This method returns true only if this BendersBFunction is convex. If this
  * BendersBFunction has no sub-Block or the sense of the Objective of its
  * sub-Block is maximization, then this method returns false. Otherwise, it
  * returns true. */

 bool is_convex( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this BendersBFunction is concave
 /** This method returns true only if this BendersBFunction is concave. If
  * this BendersBFunction has no sub-Block or the sense of the Objective of
  * its sub-Block is minimization, then this method returns false. Otherwise,
  * it returns true.
  */

 bool is_concave( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this BendersBFunction is linear
 /** Method that returns true only if this BendersBFunction is linear. In
  * particular (and probably very rare) cases, this Function could be
  * linear. We do not attempt to find this out and this method simply returns
  * \c false. */

 bool is_linear( void ) const override { return( false ); }

/*--------------------------------------------------------------------------*/
 /// tells whether a linearization is available

 bool has_linearization( bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/
 /// compute a new linearization for this BendersBFunction

 bool compute_new_linearization( bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/
 /// store a linearization in the global pool

 void store_linearization( Index name , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/

 bool is_linearization_there( Index name ) const override final {
  return( global_pool.is_linearization_there( name ) );
 }

/*--------------------------------------------------------------------------*/

 bool is_linearization_vertical( Index name ) const override final {
  return( global_pool.is_linearization_vertical( name ) );
 }

/*--------------------------------------------------------------------------*/
 /// stores a combination of the given linearizations
  /** This method creates a combination of the given set of linearizations,
  * with the given coefficients, and stores it into the global pool of
  * linearizations with the given name.
  *
  * BendersBFunction can produce two types of linearizations: diagonal and
  * vertical ones. For a combination of linearizations to be valid, it must
  * satisfy one of the following two conditions:
  *
  * -# It is a combination involving only vertical linearizations, and each
  *    coefficient (multiplier) must be nonnegative (actually, greater than or
  *    equal to - #dblAAccMlt).
  *
  * -# It is a combination involving at least one diagonal linearization, each
  *    coefficient (multiplier) must be nonnegative (actually, greater than or
  *    equal to - #dblAAccMlt), and the sum of the coefficients of the
  *    diagonal linearizations must be approximately equal to 1:
  *
  *    abs( 1 - sum coefficients of diagonal linearizations ) <= K * #dblAAccMlt
  *
  *    where K is the number of linearizations being combined.
  *
  * In the first case, the resulting linearization is a vertical one, while in
  * the second case it is a diagonal linearization. If none of the above two
  * conditions are met, an exception is thrown. */

 void store_combination_of_linearizations(
			    c_LinearCombination & coefficients , Index name ,
			    ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// specify which linearization is "the important one"

 void set_important_linearization( LinearCombination && coefficients )
  override final {
  global_pool.set_important_linearization( std::move( coefficients ) );
  }

/*--------------------------------------------------------------------------*/
 /// return the combination used to form "the important linearization"

 c_LinearCombination & get_important_linearization_coefficients( void )
  const override final {
  return( global_pool.get_important_linearization_coefficients() );
  }

/*--------------------------------------------------------------------------*/
 /// delete the given linearization from the global pool of linearizations

 void delete_linearization( Index name ,
                            ModParam issueMod = eModBlck ) override final;

/*--------------------------------------------------------------------------*/

 void delete_linearizations( Subset && which , bool ordered = true ,
                             ModParam issueMod = eModBlck ) override final;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
				      Range range = Block::INFRange ,
                                      Index name = Inf< Index >() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g ,
				      Range range = Block::INFRange ,
                                      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g , c_Subset & subset  ,
				      bool ordered = false ,
                                      Index name = Inf< Index >() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g , c_Subset & subset ,
				      bool ordered = false ,
                                      Index name = Inf< Index >() ) override;

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override final;

/*--------------------------------------------------------------------------*/
 /// return a pointer to the (only) sub-Block of the BendersBFunction
 /** This method returns a pointer to the only sub-Block of the
  * BendersBFunction (a.k.a. Block B representing problem (B) in the
  * definition of this BendersBFunction). If this BendersBFunction has no
  * sub-Block, a \c nullptr is returned.
  */

 Block * get_inner_block( void ) const {
  if( v_Block.empty() )
   return( nullptr );
  return( v_Block.front() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current A matrix in the mapping

 const MultiVector & get_A( void ) const { return( v_A ); }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current b vector in the mapping

 const RealVector & get_b( void ) const { return( v_b ); }

/*--------------------------------------------------------------------------*/
 /// returns a pointer to the Solver attached to the sub-Block (if any)
 /** This method returns a pointer to the Solver attached to the sub-Block of
  * this BendersBFunction. The template parameter \p T indicates the type of
  * Solver whose pointer will be returned; its default value is Solver. If
  *
  * - this BendersBFunction does not have a sub-Block; or
  *
  * - the sub-Block of this BendersBFunction does not have a Solver attached
  *   to it; or
  *
  * - the Solver attached to the sub-Block is not or does not derive from \p T
  *
  * then a nullptr is returned. Otherwise, a pointer of type \p T is returned.
  */

 template< class T = Solver >
 inline T * get_solver() const {
  if( v_Block.empty() )
   return( nullptr );

  if( v_Block.front()->get_registered_solvers().empty() )
   return( nullptr );

  return( dynamic_cast< T * >
  ( v_Block.front()->get_registered_solvers().front() ) );
 }

/*--------------------------------------------------------------------------*/

 /// get the whole set of parameters of this BendersBFunction
 /** The extra Configuration (see ComputeConfig::f_extra_Configuration) of the
  * ComputeConfig of the BendersBFunction is a
  * SimpleConfiguration< std::pair< Configuration * , Configuration * > >. The first
  * Configuration of this pair is a :BlockConfig and the second one is a
  * :BlockSolverConfig, both of them being associated with the inner Block of
  * this BendersBFunction.
  *
  * If an appropriate extra Configuration is not provided in \p ocfg (either
  * \p ocfg is nullptr or ocfg->f_extra_Configuration does not have the type
  * above), the extra Configuration in \p ocfg is deleted (if any) and a new
  * SimpleConfiguration< std::pair< Configuration * , Configuration * > > is
  * constructed.
  *
  * If an appropriate extra Configuration is provided, then it is used to
  * obtain the BlockConfig (see BlockConfig::get()) and the BlockSolverConfig
  * (see BlockSolverConfig::get()) of the inner Block.
  *
  * The parameters of this BendersBFunction (that are not related with its
  * inner Block), are obtained via a call to
  * ThinComputeInterface::get_ComputeConfig(). So, please take a look at
  * ThinComputeInterface::get_ComputeConfig() to understand the behavior of
  * this method and how it may affect the (possibly) given \p ocfg.
  *
  * @param all see ThinComputeInterface::get_ComputeConfig().
  *
  * @param ocfg a pointer to a ComputeConfig.
  *
  * @return a pointer to the ComputeConfig of this BendersBFunction. */

 ComputeConfig * get_ComputeConfig
 ( bool all = false , ComputeConfig * ocfg = nullptr ) const override;

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 VarVector v_x;       ///< the pointer to the active variables x

 MultiVector v_A;     ///< the A matrix of the mapping A x + b

 RealVector v_b;      ///< the b vector of the mapping A x + b

 ConstraintVector v_constraints;
 ///< the pointers to RowConstraint

 std::vector< std::unique_ptr< AbstractPath > > v_paths_to_constraints;
 ///< The AbstractPath to the affected RowConstraint

 ConstraintSideVector v_sides;
 ///< the affected sides of the affected RowConstraint
 /**< The affected sides of the affected RowConstraint. There is a
  * correspondence between the vectors v_side and v_constraints: v_sides[i] is
  * associated with v_constraints[i]. */

 bool f_constraints_are_updated = false;
 ///< indicates whether the constraints of the sub-Block are updated

 int f_solver_status = 0;
 ///< the most recent status returned by the Solver of the sub-Block

 bool f_diagonal_linearization_required = false;
 ///< indicates whether a diagonal linearization is required

 FunctionValue AAccMlt; ///< maximum absolute error in the multipliers

 bool f_ignore_modifications = false; ///< ignore any Modification

 void * f_id; ///< the "identity" of the BendersBFunction

 int LinComp; ///< determines how linearizations are computed

 Configuration * f_get_dual_solution_config = nullptr;
 ///< Configuration to be passed to Solver::get_dual_solution()
 /**< Configuration to be passed to the method Solver::get_dual_solution() of
  * the Solver attached to the inner Block, whenever this method is invoked
  * for obtaining the dual values of all RowConstraint of the inner Block (and
  * their sub-Block, recursively). */

 Configuration * f_get_dual_direction_config = nullptr;
 ///< Configuration to be passed to Solver::get_dual_direction()
 /**< Configuration to be passed to the method Solver::get_dual_direction() of
  * the Solver attached to the inner Block, whenever this method is invoked
  * for obtaining the dual values of all RowConstraint of the inner Block (and
  * their sub-Block, recursively). */

 Configuration * f_get_dual_solution_partial_config = nullptr;
 ///< Configuration to be passed to Solver::get_dual_solution()
 /**< Configuration to be passed to the method Solver::get_dual_solution() of
  * the Solver attached to the inner Block, whenever this method is invoked
  * for obtaining the dual values of the RowConstraint handled by this
  * BendersBFunction (i.e., the RowConstraint that are affected by the active
  * Variable of this BendersBFunction). */

 Configuration * f_get_dual_direction_partial_config = nullptr;
 ///< Configuration to be passed to Solver::get_dual_direction()
 /**< Configuration to be passed to the method Solver::get_dual_direction() of
  * the Solver attached to the inner Block, whenever this method is invoked
  * for obtaining the dual values of the RowConstraint handled by this
  * BendersBFunction (i.e., the RowConstraint that are affected by the active
  * Variable of this BendersBFunction). */

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*---------------------- PRIVATE TYPES OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

 /// private enum for describing the behaviour of a function when it changes
 enum function_value_behaviour {
  unchanged ,
  increase ,
  decrease ,
  unknown
  };

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE CLASSES -------------------------------*/
/*--------------------------------------------------------------------------*/

 /// A convenience class for representing the global pool of linearizations
 class GlobalPool {

 public:

  static constexpr auto NaN = std::numeric_limits< FunctionValue >::quiet_NaN();

/*--------------------------------------------------------------------------*/

  GlobalPool() = default;

/*--------------------------------------------------------------------------*/

  ~GlobalPool();

/*--------------------------------------------------------------------------*/
  /// de-serialize a GlobalPool out of the given netCDF::NcGroup
  /** De-serialize a GlobalPool out of the given netCDF::NcGroup; see
   * GlobalPool::serialize() for a description of the format.
   *
   * @param group a netCDF::NcGroup out of which the GlobalPool will be
   *        de-serialized. */

  void deserialize( const netCDF::NcGroup & group );

/*--------------------------------------------------------------------------*/
  /// serialize this GlobalPool into the given netCDF::NcGroup
  /** This function serializes this GlobalPool into the given
   * netCDF::NcGroup. The netCDF::NcGroup \p group will contain the following
   * data:
   *
   * - The dimension "BendersBFunction_MaxGlob" containing 1 + the maximum
   *   active name in the global pool; this means that there can be only
   *   BendersBFunction_MaxGlob nonempty entries in the global pool, and the
   *   largest possible name of an active entry is BendersBFunction_MaxGlob -
   *   1. This variable is optional. If it is not present then 0 (empty
   *   global pool) is assumed.
   *
   * - The variable "BendersBFunction_Type", of type netCDF::NcByte and
   *   indexed over the dimension BendersBFunction_MaxGlob, which contains the
   *   vector of booleans specifying the type (diagonal/vertical) of each
   *   linearization in the global pool. The i-th element of this vector
   *   indicates the type of the i-th linearization: if it is zero, then the
   *   linearization is vertical; otherwise, the linearization is
   *   diagonal. This variable is optional only if
   *   BendersBFunction_MaxGlob == 0.
   *
   * - The variable "BendersBFunction_Constants", of type netCDF::NcDouble and
   *   indexed over the dimension BendersBFunction_MaxGlob, which contains the
   *   constants of the linearizations. This variable is optional only if
   *   BendersBFunction_MaxGlob == 0.
   *
   * - At most BendersBFunction_MaxGlob netCDF::NcGroup with name
   *   "BendersBFunction_Sol_X", with X being an integer between 0 and
   *   BendersBFunction_MaxGlob - 1, each one containing the serialization of
   *   the Solution in the corresponding position of the global pool. If the
   *   group "BendersBFunction_Sol_X" is not present, then position X in the
   *   global pool is empty.
   *
   * - The dimension "BendersBFunction_ImpCoeffNum" containing the number of
   *   coefficients of the important combination of linearizations. This
   *   dimension is optional. If it is not provided then 0 (no important
   *   linearization) is assumed.
   *
   * - The variable "BendersBFunction_ImpCoeffInd", of type netCDF::NcInt and
   *   indexed over the dimension BendersBFunction_ImpCoeffNum, which contains
   *   indices of the linearizations that are part of the important
   *   combination. This variable is optional only if
   *   BendersBFunction_ImpCoeffNum == 0.
   *
   * - The variable "BendersBFunction_ImpCoeffVal", of type netCDF::NcDouble
   *   and indexed over the dimension BendersBFunction_ImpCoeffNum, which
   *   contains the coefficients of the important combination of
   *   linearizations. The variable is optional only if
   *   BendersBFunction_ImpCoeffNum == 0.
   *
   * @param group a netCDF::NcGroup into which this GlobalPool will be
   *        serialized. */

  void serialize( netCDF::NcGroup & group ) const;

/*--------------------------------------------------------------------------*/
  /// clone the given GlobalPool into this one

  void clone( const GlobalPool & global_pool );

/*--------------------------------------------------------------------------*/
  /// clone the given GlobalPool into this one

  void clone( GlobalPool && global_pool );

/*--------------------------------------------------------------------------*/
  // resizes the global pool
  /** Resize the global pool to have the given \p size. It is important to
   * notice that
   *
   *         IF THE SIZE OF THE POOL IS BEING DECREASED, ANY LINEARIZATION
   *         WHOSE name IS GREATER THAN OR EQUAL TO THE GIVEN NEW size IS
   *         DESTROYED.
   *
   * @param size The size of the global pool.
   */

  void resize( Index size );

/*--------------------------------------------------------------------------*/
  /// returns the size of the global pool

  Index size() const { return( solutions.size() ); }

/*--------------------------------------------------------------------------*/
  /// returns true if and only if this GlobalPool contains no Solution

  bool empty() const {
   return( std::all_of( solutions.cbegin() , solutions.cend() ,
                        []( const auto & solution ) {
                         return( ! solution );
                        } ) );
  }

/*--------------------------------------------------------------------------*/
  /// stores the given linearization constant and solution in the global pool
  /** This function stores the given linearization constant and solution into
   * the global pool under the given \p name. If the given \p name is invalid,
   * an exception is thrown. If a Solution is currently stored under the given
   * \p name, this Solution is destroyed.
   *
   * @param linearization_constant the value of the linearization constant.
   *
   * @param solution a pointer to the Solution that must be stored.
   *
   * @param name the name under which the linearization constant and the
   *        pointer to the Solution will be stored.
   *
   * @param diagonal_linearization indicates whether the linearization is a
   *        diagonal one.
   */

  void store( FunctionValue linearization_constant , Solution * solution ,
              Index name , bool diagonal_linearization );

/*--------------------------------------------------------------------------*/
  /// tells if there is a linearization in this GlobalPool with the given name
  /** This method returns true if \p name is the index (name) of a
   * linearization currently in this GlobalPool. */
  bool is_linearization_there( Index name ) const;

/*--------------------------------------------------------------------------*/
 /// tells if the linearization in this GlobalPool with that name is vertical
 /** This method returns true if \p name is the index (name) of a vertical
  * linearization currently in this GlobalPool. */
  bool is_linearization_vertical( Index name ) const;

/*--------------------------------------------------------------------------*/
  /// returns a pointer to the Solution stored under the given name
  /** This function returns a pointer to the Solution that is stored under the
   * given \p name. If the given \p name is invalid, an exception is thrown.
   *
   * @param name the name of the desired Solution.
   *
   * @return a pointer to the Solution that is stored under the given \p name.
   */

  Solution * get_solution( Index name ) const {
   if( name < solutions.size() )
    return( solutions[ name ] );
   throw( std::invalid_argument( "GlobalPool::get_solution: linearization "
                                 "with name " + std::to_string( name ) +
                                 " does not exist." ) );
  }

/*--------------------------------------------------------------------------*/
  /// returns the linearization constant stored under the given name
  /** This function returns the value of the linearization constant that is
   * stored under the given \p name. If the given \p name is invalid, an
   * exception is thrown.
   *
   * @param name the name of the desired constant.
   *
   * @return the value of the linearization constant that is stored under the
   *         given \p name.
   */

  FunctionValue get_linearization_constant( Index name ) const {
   if( name < size() )
    return( linearization_constants[ name ] );
   throw( std::invalid_argument( "GlobalPool::get_linearization_constant: linea"
                                 "rization with name " + std::to_string( name )
                                 + " does not exist." ) );
  }

/*--------------------------------------------------------------------------*/
  /// sets the linearization constant under the given name
  /** This function sets the value of the linearization constant under the
   * given \p name. If the given \p name is invalid, an exception is thrown.
   *
   * @param constant the value of the linearization constant to be stored.
   *
   * @param name the name under which the constant will be stored.
   */

  void set_linearization_constant( FunctionValue constant , Index name ) {
   if( name >= size() )
    throw( std::invalid_argument( "GlobalPool::set_linearization_constant: "
                                  "linearization with name " +
                                  std::to_string( name ) +
                                  " does not exist." ) );

   linearization_constants[ name ] = constant;
  }

/*--------------------------------------------------------------------------*/
  /// invalidates all linearizations
  /** This function invalidates all linearizations, by setting NaN to each
   * linearization constant currently stored. This means that any
   * linearization previously computed may no longer be valid. Moreover, it
   * tells that the dual solutions that are currently stored may not be
   * feasible (and, therefore, the recalculation of the linearizations based
   * on these dual solutions may not provide valid linearizations. The dual
   * solutions, however, remain stored in this global pool. If they should be
   * destroyed, explicit calls to delete_linearization() must be made.
   */

  void invalidate( void ) {
   linearization_constants.assign( linearization_constants.size() , NaN );
  }

/*--------------------------------------------------------------------------*/
  /// resets all the linearization constants
  /** This function resets all linearizations, by setting Inf to each
   * linearization constant currently stored. This means that any
   * linearization previously computed may no longer be valid, but dual
   * solutions are still feasible and the linearizations can be recomputed
   * from them.
   */

  void reset_linearization_constants( void ) {
   linearization_constants.assign( linearization_constants.size() ,
                                   Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/

  void set_important_linearization( LinearCombination && coefficients ) {
   important_linearization_lin_comb = std::move( coefficients );
   }

/*--------------------------------------------------------------------------*/
  /// return the combination used to form "the important linearization"

  c_LinearCombination & get_important_linearization_coefficients( void ) const
  { return( important_linearization_lin_comb ); }

/*--------------------------------------------------------------------------*/
  /// stores a combination of the linearizations that are already stored
  /** This method creates a linear combination of a given set of
   * linearizations, with given \p coefficients, and stores it into the global
   * pool of linearizations with the given \p name (which must be an integer
   * between 0 and size() - 1). If \p coefficients is empty, an exception is
   * thrown. If any of the names in the given \p coefficients is invalid, an
   * exception is thrown. If the given name is invalid, an exception is
   * thrown.
   *
   * @param coefficients the LinearCombination containing the names of the
   *        linearizations and their respective coefficients in the
   *        combination.
   *
   * @param name the name under which the combination of linearizations will
   *        be stored.
   *
   * @param AAccMlt the maximum absolute error in the multipliers. */

  void store_combination_of_linearizations(
				        c_LinearCombination & coefficients ,
                                        Index name , FunctionValue AAccMlt );

/*--------------------------------------------------------------------------*/
  /// deletes the linearization with the given name
  /** This function deletes the linearization with the given \p name,
   * destroying the Solution associated with it. If the given \p name is
   * invalid, an exception is thrown.
   *
   * @param name the name of the linearization to be deleted. */

  void delete_linearization( Index name );

/*--------------------------------------------------------------------------*/
  /// deletes the linearizations with the given names
  /** This function deletes the linearizations with the given names given in
   * \p which, destroying all the Solution associated with them. If any given
   * name is invalid, an exception is thrown.
   *
   * @param which the names of the linearizations that must be deleted.
   */
  void delete_linearizations( Subset & which , bool ordered );

/*--------------------------------------------------------------------------*/

 private:

  std::vector< FunctionValue > linearization_constants;
  ///< linearization constants

  std::vector< Solution * > solutions;
  ///< pointers to the Solutions

  LinearCombination important_linearization_lin_comb;
  ///< the linear combination of the important linearization

  std::vector< bool > is_diagonal;
  ///< indicates whether a linearization is diagonal

 }; // end( class( GlobalPool ) )

/*--------------------------------------------------------------------------*/
 /// convenience class used to serialize the A matrix in sparse format

 template< class T >
 class SparseMatrix {
 public:

  using Index = BendersBFunction::Index;

  SparseMatrix( Index num_rows ) {
   nnz_at_row.resize( num_rows , 0 );
  }

  void reserve( Index size ) {
   nnz_at_row.reserve( size );
   column.reserve( size );
   values.reserve( size );
  }

  void clear() {
   nnz_at_row.clear();
   column.clear();
   values.clear();
  }

  void insert( Index i , Index j , T value ) {
   if( nnz_at_row.size() <= i )
    nnz_at_row.resize( i + 1 , 0 );
   ++nnz_at_row[ i ];
   column.push_back( j );
   values.push_back( value );
  }

  void serialize( netCDF::NcGroup & group , netCDF::NcDim & row_dim ) const {
   auto nnz_dim = group.addDim( "NumNonzero" , column.size() );
   SMSpp_di_unipi_it::serialize( group , "NumNonzeroAtRow" , netCDF::NcUint() ,
                                 row_dim , nnz_at_row );
   SMSpp_di_unipi_it::serialize( group , "Column" , netCDF::NcUint() , nnz_dim , column );
   SMSpp_di_unipi_it::serialize( group , "A" , netCDF::NcDouble() , nnz_dim , values );
  }

  inline Index get_nnz() const {
   return( column.size() );
  }

 private:
  std::vector< Index > nnz_at_row;
  std::vector< Index > column;
  std::vector< T > values;

 };  // end( class( SparseMatrix ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 /// set a given integer (int) numerical parameter of the inner Block's Solver
 /** Set a given integer (int) numerical parameter of the Solver of the inner
  * Block.
  *
  * @param par The parameter whose value must be set.
  *
  * @param value The value of the parameter. */

 void set_solver_par( const idx_type par , const int value ) {
  auto solver = get_solver< CDASolver >();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::set_solver_par: the inner "
                                 "Block must have a CDASolver attached "
                                 "to it." ) );
  solver->set_par( par , value );
  }

/*--------------------------------------------------------------------------*/
 /// set a given float (double) numerical parameter of the inner Block's Solver
 /** Set a given float (double) numerical parameter of the Solver of the inner
  * Block.
  *
  * @param par The parameter whose value must be set.
  *
  * @param value The value of the parameter. */

 void set_solver_par( const idx_type par , const double value ) {
  auto solver = get_solver< CDASolver >();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::set_solver_par: the inner "
                                 "Block must have a CDASolver attached "
                                 "to it." ) );
  solver->set_par( par , value );
  }

/*--------------------------------------------------------------------------*/
 /// get a specific integer numerical parameter of the inner Block's Solver
 /** Get a specific integer (int) numerical parameter of the Solver of the
  * inner Block.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the parameter. */

 int get_solver_int_par( const idx_type par ) const {
  auto solver = get_solver< CDASolver >();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::get_solver_int_par: the "
                                 "inner Block must have a CDASolver attached "
                                 "to it." ) );
  return( solver->get_int_par( par ) );
 }

/*--------------------------------------------------------------------------*/
 /// get a specific double  numerical parameter of the inner Block's Solver
 /** Get a specific float (double) numerical parameter of the Solver of the
  * inner Block.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the parameter. */

 double get_solver_dbl_par( const idx_type par ) const {
  auto solver = get_solver< CDASolver >();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::get_solver_dbl_par: the "
                                 "inner Block must have a CDASolver attached "
                                 "to it." ) );
  return( solver->get_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/
 /// update the RowConstraint of the sub-Block
 /** This function updates the RowConstraint of the sub-Block to reflect the
  * current mapping and values of the x variables.
  */

 void update_constraints();

/*--------------------------------------------------------------------------*/

 /// write the Solution with the given name in the sub-Block
 /** If <tt>name == Inf< Index >()</tt>, this function writes the dual solution
  * associated with the last computed linearization in the sub-Block. In this
  * case, only the dual solution associated with the Constraint handled by
  * this BendersBFunction may be considered (see
  * #f_get_dual_solution_partial_config and
  * #f_get_dual_direction_partial_config). If <tt>name != Inf< Index >()</tt>,
  * then it writes the Solution that is stored in the global pool under the
  * given \p name in the sub-Block. In the last case, if the given \p name is
  * invalid or the Solution is not present in the global pool, an exception is
  * thrown.
  *
  * @param name the name of the solution to be written
  */

 void write_dual_solution( Index name );

/*--------------------------------------------------------------------------*/

 /// write the Solution with the given name in the sub-Block
 /** This function writes the Solution stored in the global pool under the
  * given \p name in the sub-Block. If the given \p name is invalid or the
  * Solution is not present, an exception is thrown.
  *
  * @param name the name under which the Solution is stored in the global
  *        pool.
  */

 void write_dual_solution_from_global_pool( Index name );

/*--------------------------------------------------------------------------*/

 /// compute the linearization constant
 /** Compute the linearization constant considering the dual solution
  * currently stored in the sub-Block.
  *
  * @return the computed linearization constant.
  */

 FunctionValue compute_linearization_constant();

/*--------------------------------------------------------------------------*/

 /// compute the linearization constant
 /** Compute the linearization constant considering a bound to the function
  * value and the dual solution of the RowConstraint handled by this
  * BendersBFunction. The linearization constant is computed as
  *
  *     bound - g'x
  *
  * where x is the vector with the values of the active Variable of this
  * BendersBFunction, g are the linearization coefficients associated with the
  * last call to compute(), and "bound" is a bound on the function value
  * obtained in the last call to compute(). If the inner Block of this
  * BendersBFunction represents a minimization problem, then "bound" must be a
  * lower bound on the value of this BendersBFunction at x. Otherwise, "bound"
  * must be an upper bound on the value of this BendersBFunction at x. If the
  * computation of the value of this BendersBFunction is exact, then this
  * bound is typically the value of this BendersBFunction.
  *
  * @return the computed linearization constant.
  */

 FunctionValue compute_linearization_constant_from_bound();

/*--------------------------------------------------------------------------*/

 /// indicate whether the linearization constant must be computed from bound
 /** This method indicates whether linearization constants must be computed by
  * using the bound provided by the Solver attached to the inner Block
  * and the duals of the RowConstraint handled by this BendersBFunction.
  *
  * @return true if and only if linearization constants should be computed
  *         from the bound provided by the inner Solver and the duals of the
  *         RowConstraint handled by this BendersBFunction.
  */

 bool is_linearization_constant_computed_from_bound() const {
  return( LinComp & 1 );
 }

/*--------------------------------------------------------------------------*/

 /// indicate whether linearization constants can be recomputed
 /** This method indicates whether linearization constants can be recomputed
  * from a Solution stored in the global pool.
  *
  * @return true only if linearization constants can be recomputed from the
  *         Solution stored in the global pool.
  */

 bool can_recompute_linearization_constant() const {
  return( LinComp & 2 );
 }

/*--------------------------------------------------------------------------*/

 /// indicate whether linearization coefficients can be recomputed
 /** This method indicates whether linearization coefficients can be
  * recomputed from a Solution stored in the global pool.
  *
  * @return true only if linearization coefficients can be recomputed from the
  *         Solution stored in the global pool.
  */

 bool can_recompute_linearization_coefficients() const {
  return( LinComp & 4 );
 }

/*--------------------------------------------------------------------------*/

 /// sends a nuclear modification, invalidates the global pool
 /** Besides sending a "nuclear modification" for Function, it also invalidates
  * the global pool and declares that the Constraint of the sub-Block are not
  * updated.
  *
  * @param chnl the name of the channel to which the Modification should be
  *        sent.
  */

 void send_nuclear_modification( const Observer::ChnlName chnl = 0 );

/*--------------------------------------------------------------------------*/

 /// returns the behaviour of this Function considering the given Modification

 function_value_behaviour get_behaviour( std::shared_ptr< BlockModAD > mod );

/*--------------------------------------------------------------------------*/

 /// returns the behaviour of this Function considering the given Modification

 function_value_behaviour get_behaviour( std::shared_ptr< ConstraintMod > mod );

/*--------------------------------------------------------------------------*/

 /// returns the behaviour of this Function considering the given Modification
 /** Returns the behaviour of this BendersBFunction considering that some
  * Constraint was added or enforced (if \p added_or_enforced_constraint is
  * true) or removed or relaxed (if \p added_or_enforced_constraint is false)
  * in some sub-Block whose Objective has the given \p sense.
  *
  * @param sense the sense of the Objective of the Block to which the
  *        Constraint belongs.
  *
  * @param added_or_enforced_constraint if true, indicates that the Constraint
  *        was added or enforced; if false, indicates that the Constraint
  *        was removed or relaxed.
  */

 function_value_behaviour get_behaviour( Objective::of_type sense ,
                                         bool added_or_enforced_constraint )
  const;

/*--------------------------------------------------------------------------*/

 /// returns true if the given Constraint is handled by this BendersBFunction
 /** Returns true if and only if the Constraint pointed by the given pointer
  * is handled by this BendersBFunction.
  *
  * @param constraint the pointer to the Constraint.
  *
  * @return true if the given Constraint is handled by this BendersBFunction;
  *         false otherwise.
  */

 bool has_constraint( Constraint * constraint );

/*--------------------------------------------------------------------------*/

 /// returns true if and only if the A matrix is sparse
 /** This function returns true if and only if the A matrix stored in v_A is
  * sparse. This matrix is considered sparse if at most a quarter of its
  * elements is nonzero. If it is sparse, then its sparse representation is
  * stored in the given SparseMatrix \p matrix. If it is not sparse, then the
  * given \p matrix will become empty.
  *
  * @param matrix The SparseMatrix that will contain the sparse representation
  *        of the A matrix in case it turns out to be sparse.
  *
  * @return true if and ony if A is sparse.
  */
 template< class T >
 bool is_A_sparse( SparseMatrix< T > & matrix ) const;

/*--------------------------------------------------------------------------*/

 /// retrieve the list of affected RowConstraint from the inner Block
 /** If the vector of AbstractPath to the affected RowConstraint
  * (#v_paths_to_constraints) is not empty, this method retrieves the pointers
  * to the Constraint specified by those paths and put them in
  * #v_constraints. After the end of the call to this method,
  * #v_paths_to_constraints is emptied. */

 void retrieve_constraints();

/*--------------------------------------------------------------------------*/

 /// return the pointer to the RowConstraint at the given \p index
 RowConstraint * get_constraint( Block::Index index ) {
  retrieve_constraints();
  assert( index < v_constraints.size() );
  return( v_constraints[ index ] );
 }

/*--------------------------------------------------------------------------*/

 /// return the index of the given  RowConstraint
 Index get_constraint_index( RowConstraint * constraint ) {
  retrieve_constraints();
  auto it = std::find( std::begin( v_constraints ) , std::end( v_constraints ) ,
                       constraint );
  if( it != std::end( v_constraints ) )
   return( std::distance( std::begin( v_constraints ) , it ) );
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

 /// return the index of the given RowConstraint
 Index get_constraint_index( RowConstraint * constraint ,
                             ConstraintSide side ) {
  retrieve_constraints();
  for( Index i = 0 ; i < v_constraints.size() ; ++i )
   if( v_constraints[ i ] == constraint && v_sides[ i ] == side )
    return( i );
  return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

 /// add the pointers to the Constraint in the given vector
 void add_constraints( const ConstraintVector & nc );

/*--------------------------------------------------------------------------*/

 /// add the pointer to the given Constraint
 void add_constraint( RowConstraint * constraint );

/*--------------------------------------------------------------------------*/

 /// remove all the Constraint in the given \p range
 void remove_constraints( Range range );

/*--------------------------------------------------------------------------*/

 /// remove all the Constraint indicated in \p rows
 /** This function removes all the Constrant indicated in \p rows. It is
  * assumed that \p rows is ordered.
  */
 void remove_constraints( const Subset & rows );

/*--------------------------------------------------------------------------*/

 /// removes all Constraint
 void remove_constraints( void );

/*--------------------------------------------------------------------------*/

 /// remove the Constraint with the given \p index
 void remove_constraint( Block::Index index );

/*--------------------------------------------------------------------------*/

 /// reset the BlockConfig of the inner Block to the default one
 void set_default_inner_Block_BlockConfig();

/*--------------------------------------------------------------------------*/

 /// reset the BlockSolverConfig of the inner Block to the default one
 void set_default_inner_Block_BlockSolverConfig();

/*--------------------------------------------------------------------------*/

 /// reset the configuration of the inner Block to the default one
 /** Reset both the BlockConfig and the BlockSolverConfig of the inner Block
  * to the default ones. */
 void set_default_inner_Block_configuration() {
  set_default_inner_Block_BlockSolverConfig();
  set_default_inner_Block_BlockConfig();
  }

/*--------------------------------------------------------------------------*/

 static void static_initialization() {
  /*!!
   * Not all C++ compilers enjoy the template wizardry behind the three-args
   * version of register_method<> with the compact MS_*_*::args(), so we just
   * use the slightly less compact one with the explicit argument and be done
   * with it.

  register_method< BendersBFunction >( "BendersBFunction::modify_constants" ,
                                       & BendersBFunction::modify_constants ,
                                       MS_dbl_sbst::args() );

  register_method< BendersBFunction >( "BendersBFunction::modify_constants" ,
                                       & BendersBFunction::modify_constants ,
                                       MS_dbl_rngd::args() );
				       !!*/
  register_method< BendersBFunction , MF_dbl_it , Subset && , const bool >(
				       "BendersBFunction::modify_constants" ,
                                       & BendersBFunction::modify_constants );

  register_method< BendersBFunction , MF_dbl_it , Range >(
				       "BendersBFunction::modify_constants" ,
                                       & BendersBFunction::modify_constants );
  }

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h; // insert BendersBFunction in the Block factory

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS  -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// global pool of linearizations
 GlobalPool global_pool;

 /// Names of the netCDF sub-groups
 inline static const std::string BLOCK_NAME = "Block";

};  // end( class( BendersBFunction ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BendersBFunctionMod ------------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a BendersBFunction
/** Derived class from C05FunctionMod to describe modifications to a
 * BendersBFunction. This obviously "keeps the same interface" as
 * C05FunctionMod, so that it can be used by Solver and/or Block just relying
 * on the C05Function interface, but it also adds BendersBFunction-specific
 * information, so that Solver and/or Block can actually react in
 * BendersBFunction-specific if they want to.
 *
 * This base class actually has *no* BendersBFunction-specific information,
 * besides being of a specific type. */

class BendersBFunctionMod : public C05FunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

  /// Definition of the possibles types of BendersBFunctionMod
  /** This enum specifies what kind of assumption can be made about any
   * previously produced linearization. Note that the enum is *not* useful for
   * all derived classes, in particular for BendersBFunctionModAddd that only
   * encodes for a single type of operation, but it is still defined here to
   * avoid being defined identically multiple times. */
  enum benders_function_mod_type {
   ModifyRows = C05FunctionModLastParam,///< modify a set of rows (both A and b)
   ModifyCnst ,                         ///< modify a set of constants (b only)
   DeleteRows ,                         ///< delete a set of rows
   BendersBFunctionModLastParam
   ///< First allowed parameter value for derived classes
   /**< Convenience value for easily allow derived classes to extend
    * the set of types of modifications. */
   };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: identical to that of C05FunctionMod
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * Modification, which linearizations have changed, the value of the shift,
  * and the "concerns Block" value. No other BendersBFunction-specific
  * information is needed. */

 BendersBFunctionMod( C05Function * f , int type , Subset && which = {} ,
                      FunctionValue shift = NaNshift , bool cB = true )
  : C05FunctionMod( f , type , std::move( which ) , shift , cB ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
  /// print the BendersBFunctionMod

 inline void print( std::ostream &output ) const override
 {
  output << "BendersBFunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on BendersBFunction [" << &f_function << " ]: ";
  if( f_type == NothingChanged )
   output << "nothing changed";
  else {
   if( f_type <= AllLinearizationChanged ) {
    if( v_which.empty() )
     output << "in all";
    else
     output << "in " << v_which.size();
    output << " linearizations ";
   }
   else
    output << v_which.size();
   switch( f_type ) {
    case( AlphaChanged ): output << "all the \alpha"; break;
    case( AllEntriesChanged ): output << "all the g"; break;
    case( AllLinearizationChanged ):
     output << "both \alpha and g have changed"; break;
    case( GlobalPoolAdded ): output << " linearizations added"; break;
    case( GlobalPoolRemoved ): output << " linearizations removed"; break;
    default: output << " unknown operation (?)";
   }
  }
  if( f_shift ) {
   output << ", f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else if( f_shift >= INFshift )
    output << "(+)";
   else if( f_shift <= - INFshift )
    output << "(-)";
   else
    output << " by " << f_shift;
  }
  output << std::endl;
 }

/*--------------------------------------------------------------------------*/

 };  // end( class( BendersBFunctionMod ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS BendersBFunctionModAddd ---------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modification specific to a BendersBFunction: add rows
/** Derived class from BendersBFunctionMod to describe a very specific
 * modification to a BendersBFunction: add some new rows. The
 * BendersBFunction-specific information is therefore the number of added
 * rows. */

class BendersBFunctionModAddd : public BendersBFunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of BendersBFunctionMod + the added rows
 /** Constructor: takes a pointer \p f to the affected C05Function, the \p
  * type of the Modification, the number \p ar of added rows, the value of the
  * \p shift, and the "concerns Block" \p cb value. */

 explicit BendersBFunctionModAddd( C05Function * f , int type , Index ar ,
                                   FunctionValue shift = NaNshift ,
                                   bool cB = true )
  : BendersBFunctionMod( f , type , Subset( {} ) , shift , cB ) ,
    f_addedrows( ar ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionModAddd() = default;  ///< default destructor

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the number of added rows

 Index addedrows( void ) const { return( f_addedrows ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BendersBFunctionModAddd

  inline void print( std::ostream &output ) const override
  {
   output << "BendersBFunctionModAddd[";
   if( concerns_Block() )
    output << "t";
   else
    output << "f";
   output << "] on BendersBFunction [" << &f_function << " ]: added "
          << f_addedrows << " rows" << std::endl;
   }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

  Index f_addedrows;  ///< number of added rows

/*--------------------------------------------------------------------------*/

 };  // end( class( BendersBFunctionModAddd ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS BendersBFunctionModRngd ----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe range modification specific to a BendersBFunction
/** Derived class from BendersBFunctionMod to describe all modifications to a
 * BendersBFunction that involve a Range set of rows:
 *
 * - modify_row[s]
 * - modify_constant[s]
 * - delete_row[s]
 *
 * For all these, the Range of the affected rows is provided, as well as the
 * exact type of operation. */

class BendersBFunctionModRngd : public BendersBFunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of BendersBFunctionMod + the Range of affected rows
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * C05FunctionMod, the type of the BendersBFunctionMod, the Range of
  * concerned rows, which linearizations have changed, the value of the shift,
  * and the "concerns Block" value.
  */

 explicit BendersBFunctionModRngd( C05Function * f , int type ,
                                   int bftype , c_Range & range ,
                                   Subset && which = {} ,
                                   FunctionValue shift = NaNshift ,
                                   bool cB = true )
  : BendersBFunctionMod( f , type , std::move( which ) , shift , cB ) ,
    f_BFtype( bftype ) , f_range( range ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionModRngd() = default;  ///< default destructor

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the specific sub-type of BendersBFunctionMod

 int BFtype( void ) { return( f_BFtype ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the range of the affected rows

 c_Range & range( void ) { return( f_range ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BendersBFunctionModRngd

 inline void print( std::ostream &output ) const override
 {
  output << "BendersBFunctionModRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on BendersBFunction [" << &f_function << " ]: ";
  if( f_BFtype == ModifyCnst )
   output << " constants";
  else
   output << " rows";
  output << " [ " << f_range.first << " , " << f_range.second << " )";
  if( f_BFtype == DeleteRows )
   output << " deleted";
  else
   output << " modified";
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 int f_BFtype;    ///< the exact BendersBFunction-specific operation

 Range f_range;   ///< the set of affected rows

/*--------------------------------------------------------------------------*/

 };  // end( class( BendersBFunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS BendersBFunctionModSbst ---------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe subset modification specific to a BendersBFunction
/** Derived class from BendersBFunctionMod to describe all modifications to
 * a BendersBFunction that involve an arbitrary set of rows:
 *
 * - modify_row[s]
 * - modify_constant[s]
 * - delete_row[s]
 *
 * For all these, the Subset of the affected rows is provided, as well as
 * the exact type of operation. */

class BendersBFunctionModSbst : public BendersBFunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of BendersBFunctionMod + the affected rows
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * C05FunctionMod, the type of the BendersBFunctionMod, the Subset of
  * concerned rows, whether or not the subset is ordered, which linearizations
  * have changed, the value of the shift, and the "concerns Block" value. As
  * the && tells, the rows parameter becomes property of the
  * BendersBFunctionModRng. */

 explicit BendersBFunctionModSbst( C05Function * f , int type , int bftype ,
                                   Subset && rows , bool ordered = false ,
                                   Subset && which = {} ,
                                   FunctionValue shift = NaNshift ,
                                   bool cB = true )
  : BendersBFunctionMod( f , type , std::move( which ) , shift , cB ) ,
    f_BFtype( bftype ) , v_rows( std::move( rows ) ) , f_ordered( ordered ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionModSbst() = default;  ///< default destructor

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the specific sub-type of BendersBFunctionMod

 int BFtype( void ) { return( f_BFtype ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the subset of the deleted Variable

 c_Subset & rows( void ) { return( v_rows ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the ordered status

 bool ordered( void ) { return( f_ordered ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BendersBFunctionModSbst

 inline void print( std::ostream &output ) const override
 {
  output << "BendersBFunctionModSbst[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on BendersBFunction [" << &f_function << " ]: "
         << v_rows.size();
  if( f_ordered )
   output << " (ordered)";
  if( f_BFtype == ModifyCnst )
   output << " constants";
   else
    output << " rows";
  if( f_BFtype == DeleteRows )
   output << " deleted";
  else
   output << " modified";
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 int f_BFtype;    ///< the exact BendersBFunction-specific operation

 Subset v_rows;   ///< the set of affected rows

 bool f_ordered;  ///< true if v_subset is ordered

/*--------------------------------------------------------------------------*/

};  // end( class( BendersBFunctionModSbst ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS BendersBFunctionState -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe the "internal state" of a BendersBFunction
/** Derived class from State to describe the "internal state" of a
 * BendersBFunction, i.e., its global pool. This means saving the
 * linearization constants, the types of the linearizations, the associated
 * Solution, and the coefficients of the important combination of
 * linearizations. */

class BendersBFunctionState : public State {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

public:

/*-------------------------------- FRIENDS ---------------------------------*/

 friend BendersBFunction;

/*---------- CONSTRUCTING AND DESTRUCTING BendersBFunctionState ------------*/

 /// constructor, doing everything or nothing.
 /** Constructor of BendersBFunctionState. If provided with a pointer to a
  * BendersBFunction, it immediately copies its "internal state", which is the
  * only way in which the BendersBFunctionState can be initialised out of an
  * existing BendersBFunction. If nullptr is passed (as by default), then an
  * "empty" BendersBFunctionState is constructed that can only be filled by
  * calling deserialize(). */

 BendersBFunctionState( const BendersBFunction * function = nullptr );

/*--------------------------------------------------------------------------*/
 /// de-serialize a BendersBFunctionState out of netCDF::NcGroup
 /** De-serialize a BendersBFunctionState out of netCDF::NcGroup; see
  * BendersBFunctionState::serialize() for a description of the format. */

 void deserialize( const netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// destructor

 virtual ~BendersBFunctionState() {}

/*------- METHODS DESCRIBING THE BEHAVIOR OF A BendersBFunctionState -------*/

 /// serialize a BendersBFunctionState into a netCDF::NcGroup
 /** This method serializes this BendersBFunctionState into the provided
  * netCDF::NcGroup, so that it can later be read back by deserialize(). After
  * the call, \p group will contain the attribute "type", common to all State,
  * and everything necessary to describe a GlobalPool (see
  * BendersBFunction::GlobalPool::serialize()).
  *
  * @param group The netCDF::NcGroup into which into which this
  *        BendersBFunctionState will be serialized. */

 void serialize( netCDF::NcGroup & group ) const override;

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 void print( std::ostream &output ) const override {
  output << "BendersBFunctionState [" << this
         << "] with max global pool element " << global_pool.size();
  }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 /// global pool of linearizations
 BendersBFunction::GlobalPool global_pool;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( BendersBFunctionState ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/** @} end( group( BendersBFun_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BendersBFunction.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File BendersBFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/

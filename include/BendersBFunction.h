/*--------------------------------------------------------------------------*/
/*----------------------- File BendersBFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class BendersBFunction, which implements C05Function
 * and Block with a Benders function.
 *
 * \version 0.01
 *
 * \date 06 - 11 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato.
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
/**< The class BendersBFunction is a convenience class implementing the
 * "abstract" concept of a Benders Function of "any" Block. BendersBFunction
 * derives from *both* C05Function and Block.
 *
 * The main ingredients of a BendersBFunction are the following:
 *
 * 1) A "base" Block B, representing an optimization problem of the form
 *
 *     (B)    min { c(y) : w <= Ey <= z, y \in Y },
 *
 *    where E is a matrix and w and z are vectors of appropriate sizes, and Y
 *    is a convex set. This will be the one, and only, sub-Block of
 *    BendersBFunction (when "seen" as a Block).
 *
 * 2) A matrix A with m rows and n columns, a vector b with m rows, and a
 *    vector of pairs [ ( C_i , S_i ) ]_{i \in I}, each pair being formed by a
 *    pointer to a RowConstraint of Block B and a ConstraintSide, where I =
 *    {1, ..., m}.
 *
 *    Problem (B) would typically be associated with an original problem
 *
 *     (O)    min { d(x) + c(y) : g <= Fx + Ey <= h, x \in X, y \in Y }
 *
 *    defined in terms of variables x and y. By reformulating problem (O) as
 *
 *     (O')   min { d(x) + phi(x) : x \in X },
 *
 *    where
 *
 *     (P)    phi(x) = min { c(y) : (g - Fx) <= Ey <= (h - Fx), y \in Y },
 *
 *    we see that (P) assumes the form of (B) with w = g - Fx and z = h -
 *    Fx. The BendersBFunction represents the function phi whose underlying
 *    optimization problem is given by (B). The variables x are the active
 *    Variables of this BendersBFunction. As can be seen in (P), the left- and
 *    right-hand sides of (some or all) the constraints may depend on x. The
 *    Block B, however, does not depend on x. In order to have the left- and
 *    right-hand sides of the constraints in (B) dependent on x, we consider
 *    the mappings "x -> g - Fx" and "x -> h - Fx", which are used to update
 *    the left- and right-hand sides w and z of the constraints in (B)
 *    according to the values of the variables x.
 *
 *    These mappings are bunched together into a single mapping M defined by
 *    the matrix A and the vector b such that M(x) = Ax + b. The i-th
 *    component of this mapping, M_i(x) = [ Ax + b ]_i, is associated with the
 *    left- or right-hand side (or both) of the RowConstraint of Block B
 *    pointed by C_i. The ConstraintSide S_i indicates which sides of the
 *    RowConstraint (pointed by) C_i are affected, as follows:
 *
 *    - If S_i = eLHS, then M_i(x) gives the value of the left-hand side of
 *      the RowConstraint (pointed by) C_i.
 *
 *    - If S_i = eRHS, then M_i(x) gives the value of the right-hand side of
 *      the RowConstraint (pointed by) C_i.
 *
 *    - If S_i = eBoth, then M_i(x) gives the value of both the left- and
 *      right-hand side of the RowConstraint (pointed by) C_i. This is the
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
 *        such that i != j, C_i = C_j and S_i = S_j). Moreover, if there is
 *        i \in I such that S_i == eBoth, there must not exist j \in I such
 *        that C_i == C_j.
 *
 * Note that the BendersBFunction is not supposed to have any Variable other
 * than "x" or Constraint (besides those defined in the sub-Block B).
 */

class BendersBFunction : public C05Function , public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

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

 enum ConstraintSide {
  eLHS =  0 ,  ///< the left-hand side of a RowConstraint
  eRHS =  1 ,  ///< the right-hand side of a RowConstraint
  eBoth = 2    ///< both sides of a RowConstraint
  };

 using RealVector = std::vector < FunctionValue >;
 ///< a real n-vector, useful for both the rows of A and b

 using c_RealVector = const RealVector;   ///< a const RealVector

 using MultiVector = std::vector< RealVector >;
 ///< representing the A matrix: a vector of m elements, each a real n-vector

 using c_MultiVector = const MultiVector;   ///< a const MultiVector

 using VarVector = std::vector< ColVariable * >;
 ///< representing the x variables upon which the function depends

 using c_VarVector = const VarVector;
 ///< a const version of the x variables upon which the function depends

 using ConstraintSpecifier = std::pair< RowConstraint * , ConstraintSide >;
 ///< associates a ConstraintSide with a RowConstraint

 using v_ConstraintSpecifier = std::vector< ConstraintSpecifier >;
 ///< a vector of ConstraintSpecifier

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a BendersBFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  v_iterator( VarVector::iterator itr ) : itr_( itr ) { }
  virtual v_iterator * clone( void ) override {
   return( new v_iterator( itr_ ) );
   }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *((*itr_)) );
   }
  virtual pointer operator->( void ) const override final {
   return( (*itr_) );
   }
  virtual bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const BendersBFunction::v_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const BendersBFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const BendersBFunction::v_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const BendersBFunction::v_iterator *>( & rhs );
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

  v_const_iterator( VarVector::const_iterator itr ) : itr_( itr ) { }
  virtual v_const_iterator * clone( void ) override {
   return( new v_const_iterator( itr_ ) );
   }

  virtual void operator++( void ) override final { (itr_)++; }
  virtual reference operator*( void ) const override final {
   return( *((*itr_)) );
   }
  virtual pointer operator->( void ) const override final {
   return( (*itr_) );
   }
  virtual bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const BendersBFunction::v_const_iterator *>(
								      & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const BendersBFunction::v_const_iterator *>(
								      & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const BendersBFunction::v_const_iterator *>(
								      & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const BendersBFunction::v_const_iterator *>(
								      & rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  VarVector::const_iterator itr_;
  };

/**@} ----------------------------------------------------------------------*/
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
  * @param constraints the set of (pointers to) affected RowConstraint
  *        together with the information about which sides are affected.
  *
  * @param observer a pointer to the Observer of this BendersBFunction.
  *
  * As the && implies, \p x, \p A, \p b, and \p constraints become property of
  * the BendersBFunction object.
  *
  */

 BendersBFunction( Block * inner_block ,
                   VarVector && x = {} , MultiVector && A = {} ,
                   RealVector && b = {} ,
                   v_ConstraintSpecifier && constraints = {} ,
                   Observer * const observer = nullptr )
  : C05Function( observer ) , constraints_are_updated( false ) ,
    solver_status ( 0 ) , diagonal_linearization_required( false )
 {

  if( ! inner_block )
   throw( std::invalid_argument( "BendersBFunction: the given Block must "
                                 "be non-null." ) );

  set_inner_block( inner_block );
  set_variables( std::move( x ) );
  set_mapping( std::move( A ) , std::move( b ) , std::move( constraints ) ,
               eNoMod );
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
  * procedure may make sense. However, because this method can then
  * conceivably be called when the BendersBFunction is attached to an
  * Observer (although it is expected to be used before that), it is also
  * necessary to specify if and how a Modification is issued.
  *
  * @param group, a netCDF::NcGroup holding the data in the format described
  *        in the comments to deserialize();
  *
  * @param issueMod, which decides if and how the FunctionMod (with shift()
  *        == FunctionMod::NaNshift, i.e., "everything changed") is issued,
  *        as described in Observer::make_par(). The default is eNoMod,
  *        since the method is mostly thought to be used during initialization
  *        when "no one is listening". */

 void deserialize( netCDF::NcGroup & group , c_ModParam issueMod = eNoMod );

/*--------------------------------------------------------------------------*/
 /// destructor of BendersBFunction
 /** Destructor of BendersBFunction. It destroys the inner Block (if any),
  * releasing its memory. If the inner Block should not be destroyed then,
  * before this BendersBFunction is destroyed, the pointer to the inner Block
  * must be set to \c nullptr. This can be done by invoking set_inner_block(),
  * passing \c nullptr as a pointer to the new inner Block and \c false to the
  * \c destroy_previous_block parameter. */

 virtual ~BendersBFunction( void ) {
  if( ! v_Block.empty() )
   delete v_Block[ 0 ];
 }

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_x field
 /** Method to "clear" the BendersBFunction: it clear() the vector v_x. This
  * destroys the list of "active" Variable without unregistering from them.
  * Not that the BendersBFunction would have to, but an Observer using it to
  * "implement itself" should. By not having any Variable, the Observer can no
  * longer do that. */

 virtual void clear( void ) override { v_x.clear(); }

/**@} ----------------------------------------------------------------------*/
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
  * data but not the the input Variable, cue this method.
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
  *        its allocated memory is released.
  */
 void set_inner_block( Block * block , bool destroy_previous_block = true ) {

  if( destroy_previous_block && ! v_Block.empty() )
   delete v_Block[ 0 ];

  v_Block.resize( 1 );
  v_Block[ 0 ] = block;

  if( block && ! get_solver<CDASolver>() )
   throw( std::invalid_argument( "BendersBFunction::set_inner_block: "
                                 "the given Block must have a CDASolver "
                                 "attached to it." ) );
  }

/*--------------------------------------------------------------------------*/
 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
  * - intMaxIter
  * - intLPMaxSz
  * - intGPMaxSz
  *
  * Any other parameter is handled by the C05Function. The first two
  * parameters are associated with parameters of the CDASolver of the
  * sub-Block. The intMaxIter parameter is associated with the
  * Solver::intMaxIter parameter and the intLPMaxSz parameter is associated
  * with the CDASolver::intMaxDSol parameter. Setting any of these parameters
  * causes the corresponding parameter of the CDASolver of the sub-Block to be
  * overwritten. The setting of these two parameters only take effect if this
  * BendersBFunction has a sub-Block and this sub-Block has a CDASolver
  * attached to it.
  *
  * @param par The parameter to be set.
  *
  * @return The value of the parameter.
  */

 virtual void set_par( const idx_type par , const int value ) override {
  switch( par ) {
   case( intMaxIter ):
    set_solver_par( Solver::intMaxIter , value );
    break;

   case( intLPMaxSz ):
    if( value < 1 )
     throw( std::invalid_argument( "BendersBFunction::set_par: intLPMaxSz "
                                   "must be non-negative" ) );
    set_solver_par( CDASolver::intMaxDSol , value );
    break;

   case( intGPMaxSz ):
    if( value < 0 )
     throw( std::invalid_argument( "BendersBFunction::set_par: intGPMaxSz "
                                   "must be non-negative" ) );
    global_pool.resize( value );
    break;
   default: C05Function::set_par( par , value );
   }
  }

/*--------------------------------------------------------------------------*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. BendersBFunction takes
  * care of the following parameters. Each of these parameters is associated
  * with a parameter of the CDASolver of the sub-Block of this
  * BendersBFunction, given between parenthesis.
  *
  * - dblMaxTime  ( Solver::dblMaxTime )
  * - dblRelAcc   ( Solver::dblRelAcc )
  * - dblAbsAcc   ( Solver::dblAbsAcc )
  * - dblUpCutOff ( Solver::dblUpCutOff )
  * - dblLwCutOff ( Solver::dblLwCutOff )
  * - dblRAccLin  ( CDASolver::dblRAccDSol )
  * - dblAAccLin  ( CDASolver::dblAAccDSol )
  *
  * Setting any of these parameters causes the corresponding parameter of the
  * CDASolver of the sub-Block to be overwritten by the given \c value. The
  * setting of these parameters only take effect if this BendersBFunction has
  * a sub-Block and this sub-Block has a CDASolver attached to it. Any other
  * parameter is handled by the C05Function.
  *
  * @param par The parameter to be set.
  *
  * @return The value of the parameter.
  */

 virtual void set_par( const idx_type par , const double value ) override {

  auto solver = get_solver<CDASolver>();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::set_par: the inner Block "
                                 "must have a CDASolver attached to it." ) );

  switch( par ) {
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

/**@} ----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the BendersBFunction
 *  @{ */

 /// get a specific integer (int) numerical parameter
 /** Get a specific integer (int) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
  * - intMaxIter
  * - intLPMaxSz
  * - intGPMaxSz
  *
  * Any other parameter is handled by the C05Function.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the required parameter.
  */

 virtual int get_int_par( const idx_type par ) const override {
  switch( par ) {
   case( intMaxIter ):
    return get_solver_int_par( Solver::intMaxIter );
   case( intLPMaxSz ):
    return get_solver_int_par( CDASolver::intMaxDSol );
   case( intGPMaxSz ):
    return global_pool.size();
  }
  return C05Function::get_int_par( par );
 }

/*--------------------------------------------------------------------------*/
 /// get a specific float (double) numerical parameter
 /** Get a specific float (double) numerical parameter. BendersBFunction takes
  * care of the following parameters:
  *
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
  * @return The value of the required parameter.
  */

 virtual double get_dbl_par( const idx_type par ) const override {
  switch( par ) {
   case( dblMaxTime ):
    return get_solver_dbl_par( Solver::dblMaxTime );
   case( dblRelAcc ):
    return get_solver_dbl_par( Solver::dblRelAcc );
   case( dblAbsAcc ):
    return get_solver_dbl_par( Solver::dblAbsAcc );
   case( dblUpCutOff ):
    return get_solver_dbl_par( Solver::dblUpCutOff );
   case( dblLwCutOff ):
    return get_solver_dbl_par( Solver::dblLwCutOff );
   case( dblRAccLin ):
    return get_solver_dbl_par( CDASolver::dblRAccDSol );
   case( dblAAccLin ):
    return get_solver_dbl_par( CDASolver::dblAAccDSol );
  }

  return C05Function::get_dbl_par( par );
 }

/**@} ----------------------------------------------------------------------*/
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
   return( Inf<Index>() );
  else
   return( std::distance( v_x.begin() , idx ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void map_active( c_Vec_p_Var & vars , Subset & map , bool ordered = false )
  const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 Variable * get_active_var( const Index i ) const override final {
  return( v_x[ i ] );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override final
 {
  return( new BendersBFunction::v_iterator( v_x.begin() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_begin( void ) const override final
 {
  return( new BendersBFunction::v_const_iterator( v_x.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override final
 {
  return( new BendersBFunction::v_iterator( v_x.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_end( void ) const override final
 {
  return( new BendersBFunction::v_const_iterator( v_x.end() ) );
  }

/**@} ----------------------------------------------------------------------*/
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
  * @param constraints The vector of pairs containing the set of RowConstraint
  *        affected by the values of x and the respective affected sides.

  * @param issueMod It indicates if and how the FunctionMod (with f_shift ==
  *        FunctionMod::NaNshift, i.e., "everything changed") is issued, as
  *        described in Observer::make_par().
  *
  * As the && implies, \p A, \p b, and \p constraints become property of this
  * object. */

 void set_mapping( MultiVector && A , RealVector && b ,
                   v_ConstraintSpecifier && constraints ,
                   c_ModParam issueMod = eModBlck );

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
		     c_ModParam issueMod = eModBlck );

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

 void add_variable( ColVariable * const var , c_RealVector & Aj = {} ,
		    c_ModParam issueMod = eModBlck );

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

 void remove_variable( c_Index i , c_ModParam issueMod = eModBlck )
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

 void remove_variables( Range range , c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a subset of Variable
 /** This method removes all the Variable in the given set of indices.
  *
  * @param nms a Subset & containing the indices of the Variable to be
  *        removed, i.e., integers between 0 and get_num_active_var() - 1.
  *
  * @param ordered a bool indicating if \p nms[] is already ordered in
  *        increasing sense (otherwise this is done inside the method,
  *        which is why \p nms[] is not const).
  *
  * @param issueMod decides if and how the C05FunctionModVars (with f_shift ==
  *        0, since a BendersBFunction is strongly quasi-additive) is issued,
  *        as described in Observer::make_par(). */

 virtual void remove_variables( Subset & nms , const bool ordered = false ,
				c_ModParam issueMod = eModBlck );

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
  *        although actually only a subset of them has) and PFtype() ==
  *        ModifyRows. */

 void modify_rows( MultiVector && nA , c_RealVector & nb , Range range ,
		   c_ModParam issueMod = eModBlck );

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
  *        although actually only a subset of them has) and PFtype() ==
  *        ModifyRows. */

 void modify_rows( MultiVector && nA , c_RealVector & nb , Subset && rows ,
		   bool ordered = false , c_ModParam issueMod = eModBlck );

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
  *        although actually only a subset of them has) and PFtype() ==
  *        ModifyRows. */

 void modify_row( c_Index i , RealVector && Ai , c_FunctionValue bi ,
		  c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a range of rows of the linear mapping
 /** Like modify_rows( range ), but modify the constant terms only.
  *
  * @param range contains the indices of the rows to be modified, hence
  *        <tt>range.second < get_b().size()</tt>;
  *
  * @param nb a vector of Function::FunctionValue with <tt>nb.size() ==
  *        range.second - range.first</tt>: entry \p nb[ i ] is (obviously)
  *        meant to be the new value of the constant term for row
  *        <tt>range.first + i</tt>;
  *
  * @param issueMod decides if and how the BendersBFunctionModRngd is issued,
  *        as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and PFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant if > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does
  *        nothing). */

 void modify_constants( c_RealVector & nb , Range range ,
			c_ModParam issueMod = eModBlck );

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
  *        only a subset of them has) and PFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant if > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all* rows,
  *        then the function has necessarily decreased, hence the shift is
  *        -INFshift. Otherwise the value has changed "unpredictably" and the
  *        shift is NANshift (unless all the values are equal, in which case
  *        the function value has not changed and the method does
  *        nothing). */

 void modify_constants( c_RealVector & nb , Subset && rows ,
			bool ordered , c_ModParam issueMod );

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
  *        only a subset of them has) and PFtype() == ModifyCnst. As for
  *        shift(), the value of the function changes in a very predictable
  *        way: if bi is > than the current value the function has
  *        necessarily increased, otherwise necessarily decreased (if it is
  *        == it has not changed and the method does nothing), hence the
  *        shift is either +INFshift or -INFshift accordingly. */

 void modify_constant( Index i , FunctionValue bi ,
		       c_ModParam issueMod = eModBlck );

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
  * @param issueMod decides if and how the BendersBFunctionModAdd is issued,
  *        as described in Observer::make_par().
  */

 void add_rows( MultiVector && nA , c_RealVector & nb ,
		c_ModParam issueMod = eModBlck );

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
  * @param issueMod decides if and how the BendersBFunctionModAdd is issued,
  *        as described in Observer::make_par().
  */

 void add_row( RealVector && Ai , FunctionValue bi ,
	       c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// deletes a range of rows from the linear mapping in the BendersBFunction
 /**< Deletes a range rows from the linear mapping in the BendersBFunction,
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

 void delete_rows( Range range , c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// deletes a subset of rows from the linear mapping
 /**< Deletes a subset of rows from the linear mapping in the
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
		   c_ModParam issueMod = eModBlck );

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

 void delete_row( c_Index i , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes all rows from the linear mapping in the BendersBFunction
 /**< Like delete_rows( range ), but immediately removes *all* the matrix A
  * and vector b, leaving the mapping "empty". Since no previous linearization
  * is valid after deleting all rows, a FunctionMod with shift() ==
  * FunctionMod::NaNshift, i.e., "everything changed", is issued. */

 void delete_rows( c_ModParam issueMod = eModBlck );

/**@} ----------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

/*--------------------------------------------------------------------------*/

 virtual void add_Modification( sp_Mod mod , Observer::ChnlName chnl = 0 )
  override;

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR Saving THE DATA OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Saving the data of the BendersBFunction
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// serialize a BendersBFunction into a netCDF::NcGroup
 /** Serialize a BendersBFunction into a netCDF::NcGroup, with the following
  * format:
  *
  * - The dimension "BendersBFunction_NumVar" containing the number of columns
  *   of the A matrix, i.e., the number of active variables.
  *
  * - The dimension "BendersBFunction_NumRow" containing the number of rows of
  *   the A matrix. The dimension is optional; if it is not provided then 0
  *   (no rows) is assumed.
  *
  * - The variable "BendersBFunction_A", of type double and indexed over both
  *   the dimensions NumRow and NumVar (in this order); it contains the
  *   (row-major) representation of the matrix A. The variable is only
  *   optional if NumRow == 0.
  *
  * - The variable "BendersBFunction_b", of type double and indexed over the
  *   dimension NumRow, which contains the vector b. The variable is only
  *   optional if NumRow == 0.
  *
  * TODO: active Variables and vector of ConstraintSpecifier.
  */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*------- METHODS DESCRIBING THE BEHAVIOR OF THE BendersBFunction ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the BendersBFunction
 *  @{ */

 /// compute the BendersBFunction

 virtual int compute( bool changedvars = true ) override;

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

 virtual FunctionValue get_value( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns a lower estimate of the BendersBFunction
 /** This method simply returns get_value().  */

 virtual FunctionValue get_lower_estimate( void ) const override {
  return get_value();
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns an upper estimate of the BendersBFunction
 /** This method simply returns get_value().  */

 virtual FunctionValue get_upper_estimate( void ) const override {
  return get_value();
 }

/*--------------------------------------------------------------------------*/
 /// returns true only if this BendersBFunction is convex
 /** This method returns true only if this BendersBFunction is convex. If this
  * BendersBFunction has no sub-Block or the sense of the Objective of its
  * sub-Block is maximization, then this method returns false. Otherwise, it
  * returns true.
  */

 virtual bool is_convex( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this BendersBFunction is concave
 /** This method returns true only if this BendersBFunction is concave. If
  * this BendersBFunction has no sub-Block or the sense of the Objective of
  * its sub-Block is minimization, then this method returns false. Otherwise,
  * it returns true.
  */

 virtual bool is_concave( void ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this BendersBFunction is linear
 /** Method that returns true only if this BendersBFunction is linear. In
  * particular (and probably very rare) cases, this Function could be
  * linear. We do not attempt to find this out and this method simply returns
  * \c false. */

 virtual bool is_linear( void ) const override { return( false ); }

/*--------------------------------------------------------------------------*/
 /// tells whether a linearization is available

 virtual bool has_linearization( const bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/
 /// compute a new linearization for this BendersBFunction

 virtual bool compute_new_linearization( const bool diagonal = true )
  override final;

/*--------------------------------------------------------------------------*/
 /// store a linearization in the global pool

 void store_linearization( const Index name ) override final;

/*--------------------------------------------------------------------------*/
 /// stores a combination of the given linearizations

 void store_combination_of_linearizations( LinearCombination & coefficients ,
					   const Index name )
  override final;

/*--------------------------------------------------------------------------*/
 /// specify which linearization is "the important one"

 void set_important_linearization( LinearCombination && coefficients ,
				   Index name ) override final {
  global_pool.set_important_linearization( std::move( coefficients ), name );
 }

/*--------------------------------------------------------------------------*/
 /// return the name of "the important linearization"

 Index get_important_linearization_name( void ) override final {
  return global_pool.get_important_linearization_name();
 }

/*--------------------------------------------------------------------------*/
 /// return the combination used to form "the important linearization"

 c_LinearCombination & get_important_linearization_coefficients( void )
  override final {
  return global_pool.get_important_linearization_coefficients();
 }

/*-------------------------------------------------------------------------*/
 /// rename a linearization that is stored in the global pool

 void rename_linearization( const Index current_name ,
			    const Index new_name ) override final;

/*--------------------------------------------------------------------------*/
 /// delete the given linearization from the global pool of linearizations

 void delete_linearization( const Index name ) override final;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
			   Range range = std::make_pair( 0 , Inf<Index>() ) ,
				      Index name = Inf<Index>() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g ,
			   Range range = std::make_pair( 0 , Inf<Index>() ) ,
				      Index name = Inf<Index>() ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g , c_Subset & subset  ,
				      const bool ordered = false ,
				      Index name = Inf<Index>() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g ,
				      c_Subset & subset ,
				      const bool ordered = false ,
				      Index name = Inf<Index>() ) override;

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization

 FunctionValue get_linearization_constant( c_Index name = Inf<Index>() )
  override final;

/*--------------------------------------------------------------------------*/
 /// return a pointer to the (only) sub-Block of the BendersBFunction
 /** This method returns a pointer to the only sub-Block of the
  * BendersBFunction (a.k.a. Block B representing problem (B) in the
  * definition of this BendersBFunction). If this BendersBFunction has no
  * sub-Block, a \c nullptr is returned.
  */

 Block * get_inner_block() {
  if( v_Block.empty() )
   return nullptr;
  return v_Block[ 0 ];
 }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current A matrix in the mapping

 const MultiVector & get_A( void ) const { return( v_A ); }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current b vector in the mapping

 const RealVector & get_b( void ) const { return( v_b ); }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing
    @{ */

 /// print information about the BendersBFunction on an ostream
 /** Protected method intended to print information about the
  * BendersBFunction; it is virtual so that derived classes can print their
  * specific information in the format they choose. */

 virtual void print( std::ostream &output ) const override {
  output << "BendersBFunction [" << this << "]"
	 << " with " << get_num_active_var() << " active variables and a"
         << " mapping with " << get_b().size() << " rows.";
  }

 /// load the BendersBFunction out of an istream
 /** This method loads the BendersBFunction out of an istream. */

 virtual void load( std::istream &input ) override final;

/*--------------------------------------------------------------------------*/

/**@} ----------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 VarVector v_x;       ///< the pointer to the active variables x

 MultiVector v_A;     ///< the A matrix of the mapping A x + b

 RealVector v_b;      ///< the b vector of the mapping A x + b

 v_ConstraintSpecifier v_constraints;
 ///< the pointers to RowConstraint and their respective affected sides

 bool constraints_are_updated = false;
 ///< indicates whether the constraints of the sub-Block are updated

 int solver_status = 0;
 ///< the most recent status returned by the Solver of the sub-Block

 bool diagonal_linearization_required = false;
 ///< indicates whether a diagonal linearization is required

/**@} ----------------------------------------------------------------------*/
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
/*-------------------------- PRIVATE METHODS -------------------------------*/
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

 template<class T = Solver>
 inline T * get_solver() const {
  if( v_Block.empty() )
   return nullptr;

  if( v_Block[ 0 ]->get_registered_solvers().empty() )
   return nullptr;

  return dynamic_cast< T * >( v_Block[ 0 ]->get_registered_solvers().back() );
 }

/*--------------------------------------------------------------------------*/

 /// set a given integer (int) numerical parameter of the inner Block's Solver
 /** Set a given integer (int) numerical parameter of the Solver of the inner
  * Block.
  *
  * @param par The parameter whose value must be set.
  *
  * @param value The value of the parameter.
  */

 void set_solver_par( const idx_type par , const int value ) {
  auto solver = get_solver<CDASolver>();
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
  * @param value The value of the parameter.
  */

 void set_solver_par( const idx_type par , const double value ) {
  auto solver = get_solver<CDASolver>();
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
  * @return The value of the parameter.
  */

 inline int get_solver_int_par( const idx_type par ) const {
  auto solver = get_solver<CDASolver>();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::get_solver_int_par: the "
                                 "inner Block must have a CDASolver attached "
                                 "to it." ) );
  return solver->get_int_par( par );
 }

/*--------------------------------------------------------------------------*/

 /// get a specific double  numerical parameter of the inner Block's Solver
 /** Get a specific float (double) numerical parameter of the Solver of the
  * inner Block.
  *
  * @param par The parameter whose value is desired.
  *
  * @return The value of the parameter.
  */

 inline double get_solver_dbl_par( const idx_type par ) const {
  auto solver = get_solver<CDASolver>();
  if( ! solver )
   throw( std::invalid_argument( "BendersBFunction::get_solver_dbl_par: the "
                                 "inner Block must have a CDASolver attached "
                                 "to it." ) );
  return solver->get_dbl_par( par );
 }

/*--------------------------------------------------------------------------*/

 /// update the RowConstraint of the sub-Block
 /** This function updates the RowConstraint of the sub-Block to reflect the
  * current mapping and values of the x variables.
  */

 void update_constraints();

/*--------------------------------------------------------------------------*/

 /// write the Solution with the given name in the sub-Block
 /** If <tt>name == Inf<Index>()</tt>, this function writes the dual solution
  * associated with the last computed linearization in the sub-Block. If
  * <tt>name != Inf<Index>()</tt>, then it writes the Solution that is stored
  * in the global pool under the given \p name in the sub-Block. In the last
  * case, if the given \p name is invalid or the Solution is not present in
  * the global pool, an exception is thrown.
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

 /// returns the dual value associated with the given RowConstraint
 /** This function returns the dual value associated with the given \p
  * constraint and its specific \c side.
  *
  * @param constraint a pointer to the RowConstraint.
  *
  * @param side the side of the RowConstraint.
  *
  * @return the dual value associated with the given \p constraint and \p side.
  */

 FunctionValue get_dual_value( const RowConstraint * constraint ,
                               const ConstraintSide & side );

/*--------------------------------------------------------------------------*/

 /// compute the linearization constant
 /** Compute the linearization constant considering the dual solution
  * currently stored in the sub-Block.
  *
  * @return the computed linearization constant.
  */

 FunctionValue compute_linearization_constant();

/*--------------------------------------------------------------------------*/

 /// sends a nuclear modification, invalidates the global pool
 /** Besides sending a "nuclear modification" for Function, it also invalidates
  * the global pool and declares that the Constraint of the sub-Block are not
  * updated.
  *
  * @param chnl the name of the channel to which the Modification should be
  *        sent.
  */

 void send_nuclear_modification( const Observer::ChnlName chnl );

/*--------------------------------------------------------------------------*/

 /// returns the behaviour of this Function considering the given Modification

 function_value_behaviour get_behaviour( std::shared_ptr<BlockModAD> mod )
  const;

/*--------------------------------------------------------------------------*/

 /// returns the behaviour of this Function considering the given Modification

 function_value_behaviour get_behaviour( std::shared_ptr<ConstraintMod> mod )
  const;

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

 bool has_constraint( Constraint * constraint ) const;

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h; // insert BendersBFunction in the Block factory

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE CLASSES -------------------------------*/
/*--------------------------------------------------------------------------*/

 /// A convenience class for representing the global pool of linearizations
 class GlobalPool {

 public:

  static constexpr auto NaN = std::numeric_limits<FunctionValue>::quiet_NaN();

/*--------------------------------------------------------------------------*/

  GlobalPool() = default;

/*--------------------------------------------------------------------------*/

  ~GlobalPool();

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

  /// returns the the size of the global pool

  Index size() const {
   return solutions.size();
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
   */

  void store( FunctionValue linearization_constant , Solution * solution ,
              Index name );

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
    return solutions[ name ];
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
    return linearization_constants[ name ];
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
   * destroyed, explicity calls to delete_linearization() must be made.
   */

  void invalidate() {
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

  void reset_linearization_constants() {
   linearization_constants.assign( linearization_constants.size() ,
                                   Inf<FunctionValue>() );
  }

/*--------------------------------------------------------------------------*/

  /// specify which linearization is "the important one"
  /** This method sets the linearization with the given name as "the important
   * one".
   *
   * @param coefficients a LinearCombination that may define the important
   *        linearization.
   *
   * @param name the name of the important linearization.
   */

  void set_important_linearization( LinearCombination && coefficients ,
                                    Index name ) {
   important_linearization_lin_comb = std::move( coefficients );
   important_linearization_name = name;
  }

/*--------------------------------------------------------------------------*/

  /// return the name of "the important linearization"

  Index get_important_linearization_name( void ) {
   return( important_linearization_name );
  }

/*--------------------------------------------------------------------------*/

  /// return the combination used to form "the important linearization"

  c_LinearCombination & get_important_linearization_coefficients( void ) {
   return( important_linearization_lin_comb );
  }

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
   */

  void store_combination_of_linearizations( LinearCombination & coefficients ,
                                            const Index name );

/*--------------------------------------------------------------------------*/

  /// renames a linearization
  /** This methods renames the linearization currently stored under \p
   * current_name. If any of \p current_name or \p new_name is an invalid
   * name, an exception is thrown.
   *
   * @param current_name the current name of the linearization that will be
   *        renamed.
   *
   * @param new_name the new name of the linearization.
   */

  void rename_linearization( const Index current_name , const Index new_name );

  /// deletes the linearization currently stored under the given name
  /** This method deletes the linearization currently stored under the given
   * \p name. If the given \p name is invalid, an exception is thrown.
   *
   * @param name the name of the linearization to be deleted.
   */

/*--------------------------------------------------------------------------*/

  /// deletes the linearization with the given name
  /** This function deletes the linearization with the given \p name,
   * destroying the Solution associated with it. If the given \p name is
   * invalid, an exception is thrown.
   *
   * @param name the name of the linearization to be deleted.
   */
  void delete_linearization( const Index name );

/*--------------------------------------------------------------------------*/

 private:

  std::vector<FunctionValue> linearization_constants;
  ///< linearization constants

  std::vector<Solution *> solutions;
  ///< pointers to the Solutions

  LinearCombination important_linearization_lin_comb;
  ///< the linear combination of the important linearization

  Index important_linearization_name;
  ///< the name of the important linearization
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS  -----------------------------*/
/*--------------------------------------------------------------------------*/

 GlobalPool global_pool;
 ///< global pool of linearizations

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
  * Modification, the value of the shift, and the "concerns Block" value. No
  * other BendersBFunction-specific information is needed. */

 BendersBFunctionMod( C05Function * f , int type ,
                      FunctionValue shift = NaNshift , bool cB = true )
  : C05FunctionMod( f , type , shift , cB ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
  /// print the BendersBFunctionMod

  virtual inline void print( std::ostream &output ) const override
  {
   output << "BendersBFunctionMod[";
   if( concerns_Block() )
    output << "t";
   else
    output << "f";
   output << "] on BendersBFunction [" << &f_function << " ]: ";
   switch( f_type ) {
    case( AlphaChanged ): output << "all the \alpha"; break;
    case( AllEntriesChanged ): output << "all the g"; break;
    default: output << "both \alpha and g";
    }
   output << " have changed ==> f-values changed";
   if( std::isnan( f_shift ) )
    output << "(+-)";
   else
    if( f_shift >= INFshift )
     output << "(+)";
    else
     if( f_shift <= -INFshift )
      output << "(-)";
     else
      output << " by " << f_shift;
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
  : BendersBFunctionMod( f , type , shift , cB ) , f_addedrows( ar ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~BendersBFunctionModAddd() = default;  ///< default destructor

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the number of added rows

 Index addedrows( void ) const { return( f_addedrows ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BendersBFunctionModAddd

  virtual inline void print( std::ostream &output ) const override
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

class BendersBFunctionModRngd : public BendersBFunctionMod {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of BendersBFunctionMod + the Range of affected rows
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * C05FunctionMod, the type of the BendersBFunctionMod, the Range of
  * concerned rows, the value of the shift, and the "concerns Block" value.
  */

 explicit BendersBFunctionModRngd( C05Function * f , int type ,
                                   int bftype , c_Range & range ,
                                   FunctionValue shift = NaNshift ,
                                   bool cB = true )
  : BendersBFunctionMod( f , type , shift , cB ) , f_BFtype( bftype ) ,
    f_range( range ) { }

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

 virtual inline void print( std::ostream &output ) const override
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
  * concerned rows, whether or not the subset is ordered, the value of the
  * shift, and the "concerns Block" value. As the && tells, the rows
  * parameter becomes property of the BendersBFunctionModRng. */

 explicit BendersBFunctionModSbst( C05Function * f , int type , int bftype ,
                                   Subset && rows , bool ordered = false ,
                                   FunctionValue shift = NaNshift ,
                                   bool cB = true )
  : BendersBFunctionMod( f , type , shift , cB ) , f_BFtype( bftype ) ,
    v_rows( std::move( rows ) ) , f_ordered( ordered ) { }

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

 virtual inline void print( std::ostream &output ) const override
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

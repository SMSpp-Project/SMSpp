/*--------------------------------------------------------------------------*/
/*----------------------- File PolyhedralFunction.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the PolyhedralFunction class, which is a convex (or
 * concave) C05Function defined by the maximum (or minimum) of a "small"
 * number of explicitly provided affine forms.
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

#ifndef __PolyhedralFunction
 #define __PolyhedralFunction
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05Function.h"

#include "ColVariable.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class PolyhedralFunctionState;
 // forward declaration of PolyhedralFunctionState
 
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup PolyhedralFunction_CLASSES Classes in PolyhedralFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS PolyhedralFunction --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a C05Function with a fixed number of linearizations
/** The PolyhedralFunction class derives from C05Function, and defines a
 * simple implementation of a convex (or concave) Function defined by the
 * maximum (or minimum) of a "small" number of explicitly provided affine
 * forms. In other words, if the PolyhedralFunction depends on a set of n
 * ColVariable, its input data is a m \times n matrix A and a m \times 1
 * vector b (with m given and "small"), so that
 * \f[
 *     pf( x ) = max \{ a_i x + b_i : i = 0, ... , m - 1 \}
 * \f]
 * in the convex case (pointwise maximum of linear functions), and
 * \f[
 *     pf( x ) = min \{ a_i x + b_i : i = 0, ... , m - 1 \}
 * \f]
 * in the concave one (pointwise minimum of linear functions). A "special",
 * all-0 linearization (i.e., \f$ 0 x + b_m \f$) is separately handled with a
 * dedicated mechanism. This could be explicitly represented as "any one" of
 * the linearizations that just so happened to have \f$ A_i = 0 \f$, but the
 * dedicated mechanism both saves a tiny bit of memory, and especially
 * provides a finite lower (if the function is convex, upper otherwise) bound
 * on the value of the function everywhere that can be returned by
 *  get_global_[lower/upper]_bound().
 *
 * The function is anyhow finite-valued everywhere, and each of the pairs
 * \f$ ( A_i , b_i ) \f$ define one of the possible diagonal linearizations
 * (comprised the "flat" all-0 one associated with the lower/upper bound
 * \f$ b_m \f$, if defined); thus far vertical linearizations are not handled,
 * but adding them would not be too much of an issue. The only exception is
 * when \f$ m = 0 \f$ and \f$ b_m = - \infty \f$ (in the convex case), in
 * which case the function evaluates to \f$ - \infty \f$ (\f$ + \infty \f$ in
 * the concave one with the obvious change).
 *
 * When the function is evaluated, all the m ( + 1 if the lower/upper bound is
 * defined) linearizations enter the local pool in order of their value
 * \f$ v_i = A_i x + b_i \f$ (non-increasing in the convex case,
 * non-decreasing in the concave one), and are reported in that order. The
 * global pool is just a subset of the fixed index set 0, ..., m - 1 (, m if
 * the lower/upper bound is defined), *except if aggregate linearizations are
 * defined*. 
 *
 * PolyhedralFunction handles all possible changes in its input data:
 *
 * - A complete reset of all the data (A, b), with our without also changing
 *   the set of "active" Variable x, which results in a FunctionMod (with
 *   shift() == FunctionMod::NaNshift, i.e., "everything changed") being
 *   issued.
 *
 * - Changing the "verse" (maximization vs. minimization, i.e., if the
 *   function is convex or concave) while leaving all the rest of the data
 *   unchanged, which results in a C05FunctionMod being issued, with shift()
 *   being either INFshift (min --> max, hence the function has increased)
 *   or - INFshift (max --> in, hence the function has dencreased) if the new
 *   setting is respectively convex or concave, and type() having the
 *   weird-ish value C05FunctionMod::NothingChanged.
 *
 * - Addition of variables (adding columns to A), either one or a range,
 *   which results in a C05FunctionModVarsAddd being issued (since a
 *   PolyhedralFunction is strongly quasi-additive, and with shift() == 0
 *   as expected).
 *
 * - Removal of variables (removing columns from A), either one, or a range,
 *   or a subset of them, which results in either a C05FunctionModVarsRngd or
 *   a C05FunctionModVarsSbst being issued (since a PolyhedralFunction is
 *   strongly quasi-additive, and with shift() == 0 as expected).
 *
 * - Changes of rows in A (either one, or a range or a subset of them) and
 *   in the corresponding elements of b, which results in either a
 *   PolyhedralFunctionModRngd or a PolyhedralFunctionModSbst being issued,
 *   with shift() == NANshift (the function has changed "unpredictably"),
 *   type() == AllLinearizationChanged (all the linearizations may have
 *   changed, although actually only a subset of them has) and PFtype() ==
 *   ModifyRows.
 *
 * - Changes of elements of b (either one, or a range or a subset of them)
 *   only, which results in either a PolyhedralFunctionModRngd or a 
 *   PolyhedralFunctionModSbst being issued, with type() == AlphaChanged (all
 *   the alphas may have changed, although actually only a subset of them
 *   has) and PFtype() == ModifyCnst. As for with shift(), it may be
 *   == NANshift, but also to +-INFshift if the elements of b have changed
 *   "monotonically".

 * - Changes of the global lower/upper bound, which is basically the value
 *   of b for a "virtual" all-0 row that is not explicitly stored; this
 *   results a PolyhedralFunctionModRngd (with Range = <  0 , 0 >) with
 *   type() == AlphaChanged (all the alphas may have changed, although
 *   actually only one of them has) and PFtype() == ModifyCnst. As for with
 *   shift(), it is either +-INFshift according if the bound has increased
 *   or decreased.
 *
 * - Addition of cutting planes (adding rows to A), either one or a range,
 *   which results in PolyhedralFunctionModAddd being issued; note that adding
 *   new rows makes a "max" (convex) function to increase in value and a
 *   "min" (concave) one to decrease in value, which means that shift() will
 *   be +-INFshift accordingly, but all existing linearization are still
 *   valid ones, which is the poster case for the weird-ish setting
 *   C05FunctionMod::NothingChanged for the type() of the C05FunctionMod.
 *
 * - Removal of cutting planes (removing rows to A), either one, or a range,
 *   or a subset of them, which results in either a PolyhedralFunctionModRngd
 *   or a PolyhedralFunctionModSbst being issued; note that removing rows
 *   makes a "max" (convex) function to decrease in value and a "min"
 *   (concave) one to increase in value; also, existing lnearizations in the
 *   global pool may disappear. Even worse, and aggregated linearization may
 *   have been constructed out of the ones that are deleted, and there is no
 *   way of saying it in general. Hence, the PolyhedralFunctionMod* will
 *   have type() = C05FunctionMod::AlphaChanged; if
 *
 *   = any of the deleted rows are present in the global pool;
 *
 *   = any aggregated linearization is present in the global pool.
 *
 *   Otherwise no linearization is affected, and the PolyhedralFunctionMod*
 *   will have type() = C05FunctionMod::NothingChanged. In all cases, the
 *   PolyhedralFunctionMod* will have PFtype() == DeleteRows. */

class PolyhedralFunction : public C05Function {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 using RealVector = std::vector< FunctionValue >;
 ///< a real n-vector, useful for both the rows of A and b

 using c_RealVector = const RealVector;   ///< a const RealVector

 using MultiVector = std::vector< RealVector >;
 ///< representing the A matrix: a vector of m elements, each a real n-vector

 using c_MultiVector = const MultiVector;   ///< a const MultiVector

 using VarVector = std::vector< ColVariable * >;
 ///< representing the x variables upon which the function depends

 using c_VarVector = const VarVector;
 ///< a const version of the x variables upon which the function depends

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a PolyhedralFunction. */

 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( VarVector::iterator & itr ) : itr_( itr ) {}
  explicit v_iterator( VarVector::iterator && itr ) :
   itr_( std::move( itr ) ) {}
  v_iterator * clone( void ) override final {
   return( new v_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++(itr_); }
  reference operator*( void ) const override final { return( *((*itr_)) ); }
  pointer operator->( void ) const override final { return( (*itr_) ); }
  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const PolyhedralFunction::v_iterator * >( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const PolyhedralFunction::v_iterator * >( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const PolyhedralFunction::v_iterator * >( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const PolyhedralFunction::v_iterator * >( & rhs );
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
  * Variable of a PolyhedralFunction. */

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator( VarVector::const_iterator & itr ) :
   itr_( itr ) {}
  explicit v_const_iterator( VarVector::const_iterator && itr ) :
   itr_( std::move( itr ) ) {}
  v_const_iterator * clone( void ) override final {
   return( new v_const_iterator( itr_ ) );
   }
 
  void operator++( void ) override final { ++(itr_); }
  reference operator*( void ) const override final { return( *((*itr_)) ); }
  pointer operator->( void ) const override final { return( (*itr_) ); }
  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const PolyhedralFunction::v_const_iterator * >(
								      & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const PolyhedralFunction::v_const_iterator * >(
								      & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast< const PolyhedralFunction::v_const_iterator * >(
								      & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast< const PolyhedralFunction::v_const_iterator * >(
								      & rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  VarVector::const_iterator itr_;
  };

/** @} ---------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING PolyhedralFunction -------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing PolyhedralFunction
 *  @{ */

 /// constructor of PolyhedralFunction, possibly inputting the data
 /** Constructor of PolyhedralFunction, taking possibly all the data
  * characterising the function:
  *
  * @param x a n-vector of pointers to ColVariable representing the x variable
  *        vector in the definition of the function. Note that the order of
  *        the variables in x is crucial, since
  *
  *            THE ORDER OF THE x VECTOR WILL DICTATE THE ORDER OF THE
  *            "ACTIVE" [Col]Variable OF THE PolyhedralFunction
  *
  *        That is, get_active_var( 0 ) == x[ 0 ], get_active_var( 1 ) ==
  *        x[ 1 ], ... 
  *
  * @param A a m-vector of n-vectors of FunctionValue representing the A
  *        matrix in the definition of the function; the correspondence
  *        between A[][] and x[] is positional, i.e., entry A[ i ][ j ] is
  *        (obviously) meant to be the coefficient of variable *x[ j ] (i.e.,
  *        get_active_var( j )) for the i-th row;
  *
  * @param b a m-vector of FunctionValue representing the b vector in the
  *        definition of the function: b[ i ] is the constant factor of the
  *        i-th linear form;
  *
  * @param bound is the global valid lower (if the function is convex) or
  *        upper (if the function is convace) bound on the function value;
  *
  * @param is_convex a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization, and therefore it is a concave function.
  *
  * As the && implies, x, A and b become property of the PolyhedralFunction
  * object.
  *
  * All inputs have a default ({}, {}, {}, - Inf< FunctionValue >(), true, and
  * nullptr, respectively) so that this can be used as the void constructor. */

 PolyhedralFunction( VarVector && x = {} , MultiVector && A = {} ,
		     RealVector && b = {} ,
		     FunctionValue bound = - Inf< FunctionValue >() ,
		     bool is_convex = true ,
		     Observer * const observer = nullptr )
  : C05Function( observer ) , f_is_convex( is_convex ) , f_bound( bound ) ,
    f_loc_pool_sz( 1 ) , f_next( 0 ) ,  f_max_glob( 0 )
 {
  v_ord.resize( 1 );
  v_ord[ 0 ] = 0;
  set_variables( std::move( x ) );
  set_PolyhedralFunction( std::move( A ) , std::move( b ) , bound ,
			  is_convex , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a PolyhedralFunction out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * *numerical* information required to de-serialize the PolyhedralFunction,
  * i.e., a m x n real matrix A, a real m-vector b, the "verse" of the
  * function and its global [lower/upper] bound, and initializes the
  * PolyhedralFunction by calling set_PolyhedralFunction() with the recovered
  * data. See the comments to PolyhedralFunction::serialize() for the detailed
  * description of the expected format of the netCDF::NcGroup.
  *
  * Note that this method does *not* change the set of active variables, that
  * must be initialized independently either before (like, in the
  * constructor) or after a call to this method (cf. set_variable()).
  *
  * Note, however that there is a significant difference between calling
  * deserialize() before or after set_variables(). More specifically, the
  * difference is between calling the method when the current set of "active"
  * Variable is empty, or not. Indeed, in the former case the number of
  * "active" Variabble is dictated by the data foun in the netCDF::NcGroup;
  * calling set_variables() afterwards with a vector of different size will
  * fail. Symmetrically, if the set of "active" Variable is not empty when
  * this method is called, finding non-conforming data (a matrix A with a
  * different number of columns than get_num_active_var()) in the
  * netCDF::NcGroup within this method will cause it to fail. Also, note
  * that in the former case the function is "not completely initialized yet"
  * after deserialize(), and therefore it should not be passed to the
  * Observer quite as yet.
  *
  * Usually [de]serialization is done by Block, but PolyhedralFunction is a
  * complex enough object so that having its own ready-made [de]serialization
  * procedure may make sense. However, because this method can then
  * conceivably be called when the PolyhedralFunction is attached to an
  * Observer (although it is expected to be used before that), it is also
  * necessary to specify if and how a Modification is issued.
  *
  * @param group a netCDF::NcGroup holding the data in the format described
  *        in the comments to deserialize();
  *
  * @param issueMod which decides if and how the FunctionMod (with shift()
  *        == FunctionMod::NaNshift, i.e., "everything changed") is issued,
  *        as described in Observer::make_par(). The default is eNoMod,
  *        since the method is mostly thought to be used during initialization
  *        when "no one is listening". */

 void deserialize( const netCDF::NcGroup & group ,
		   ModParam issueMod = eNoMod );

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty

 virtual ~PolyhedralFunction() = default;

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_x field
 /** Method to "clear" the PolyhedralFunction: it clear() the vector v_x. This
  * destroys the list of "active" Variable without unregistering from them.
  * Not that the PolyhedralFunction would have to, but an Observer using it
  * to "implement iyself" should. By not having any Variable, the Observer
  * can no longer do that. */

 void clear( void ) override { v_x.clear(); }

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// sets the set of active Variable of the PolyhedralFunction
 /** Sets the set of active Variable of the PolyhedralFunction. This method
  * is basically provided to work in tandem with the methods which only load
  * the "numerical data" of the PolyhedralFunction, i.e., deserialize() and
  * set_PolyhedralFunction( A , b ). These (if the variables have not been
  * defined prior to calling them, see below) leave the PolyhedralFunction in
  * a somewhat inconsistent state whereby one knows the data but not the
  * the input Variable, cue this method.
  *
  * Note that there are two distinct patterns of usage:
  *
  * - set_variables() is called *before* deserialize() or
  *   set_PolyhedralFunction( A , b );
  *
  * - set_variables() is called *after* deserialize() or
  *   set_PolyhedralFunction( A , b ).
  *
  * In the former case, the PolyhedralFunction is in a "well defined state"
  * at all times: after the call to set_variables() everything is there, only
  * A is empty and therefore the function is identically - INF if convex,
  * + INF if concave (maximization / minimization over an empty set). In the
  * latter case, however, the data is there, except that the
  * PolyhedralFunction has no input Variable; the object is in a
  * not-fully-consistent defined state. Having an Observer (then, Solver)
  * dealing with a call to set_variables() would be possible by issuing a
  * FunctionModVars( ... , AddVar ), but this is avoided because this method
  * is only thought to be called during initialization where the Observer is
  * not there already, whence no "issueMod" parameter.
  * PolyhedralFunction until both the Variable and the data are set.
  *
  * @param x a n-vector of pointers to ColVariable representing the x variable
  *        vector in the definition of the function. Note that the order of
  *        the variables in x is crucial, since the correspondence with A
  *        (whether already provided, or to be provided later, cf. discussion
  *        above) is positional: entry A[ i ][ j ] is (obviously) meant to be
  *        the coefficient of variable *x[ j ] for the i-th row. In other
  *        words, after the call x[ 0 ] == get_active_var( 0 ), x[ 1 ] =
  *        get_active_var( 1 ), ...
  *
  * As the && implies, x become property of the PolyhedralFunction object. */

 void set_variables( VarVector && x );

/*--------------------------------------------------------------------------*/
 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. PolyhedralFunction takes
  * care of intLPMaxSz and intGPMaxSz, leaving all the rest to Function.
  *
  * note: if intGPMaxSz decreases w.r.t. the previous value, and there are
  *       linearizations in the global pool that get eliminated because of
  *       this, a C05FunctionMod will be issued with type AlphaChanged but
  *       f_shift == 0 because the PolyhedralFunction did not really change,
  *       only the stored linearizations were. */

 void set_par( idx_type par , int value ) override
 {
  switch( par ) {
   case( intLPMaxSz ):
    if( value < 1 )
     throw( std::invalid_argument( "intLPMaxSz must be positive" ) );
    if( Index( value ) != f_loc_pool_sz ) {
     f_loc_pool_sz = Index( value );
     reset_v_ord();
     }
    break;
  case( intGPMaxSz ): {
    if( value < 0 )
     throw( std::invalid_argument( "intGPMaxSz must be non-negative" ) );
    int oldsize = v_glob.size();
    if( oldsize == value )  // if the size is not changing
     break;                 // nothing to do

    // check if any of the linearizations that will be lost (if any) is an
    // aggregated one and manage v_aA and v_ab accordingly
    for( Index i = value ; i < f_max_glob ; ++i )
     if( v_glob[ i ] < 0 )          // it is an aggregated item
      // mark its position in v_ab[] with INF to signal it's not needed
      v_ab[ - v_glob[ i ] - 1 ] = Inf< FunctionValue >();

    // until the last position is not needed, shorten v_aA[] and v_ab[]
    while( ! v_ab.empty() ) {
     auto last = --v_ab.end();
     if( *last == Inf< FunctionValue >() ) {
      v_aA.pop_back();
      v_ab.pop_back();
      }
     else
      break;
     }

    v_glob.resize( value , Inf< int >() );  // resize v_glob

    if( f_max_glob >= Index( value ) ) {  // some linearizations are lost
     f_max_glob = value ? value - 1 : 0;  // value could be 0 ...

     update_f_max_glob();

     if( ! f_Observer )  // noone is there
      break;             // all done

     // issue the C05FunctionMod
     f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
      this , C05FunctionMod::AlphaChanged , Subset( {} ) , 0 ) );
     }
    break;
    }
   default: Function::set_par( par , value );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. PolyhedralFunction
  * ignores the C05Function-specific dblRAccLin and dblAAccLin, handles
  * dblAAccMlt, and leaves all the rest to Function. */

 void set_par( idx_type par , double value ) override
 {
  switch( par ) {
   case( dblAAccMlt ):
    AAccMlt = value;
   case( dblRAccLin ):
   case( dblAAccLin ):
    break;
   default: Function::set_par( par , value );
   }
  }

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF A PolyhedralFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a PolyhedralFunction
 * The methods in this section allow to compute the current value of the
 * PolyhedralFunction and retrieve first-order information about the point
 * where the C05Function have been last evaluated.
 * @{ */

 /// compute the PolyhedralFunction

 int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// returns the value of the PolyhedralFunction

 FunctionValue get_value( void ) const override { return( f_value ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the PolyhedralFunction is exact, hence lower_estimate == value
 
 FunctionValue get_lower_estimate( void ) const override { return( f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the PolyhedralFunction is exact, hence upper_estimate == value

 FunctionValue get_upper_estimate( void ) const override { return( f_value );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue get_global_lower_bound( void ) override {
  return( f_is_convex ? f_bound : - Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue get_global_upper_bound( void ) override {
  return( f_is_convex ? Inf< FunctionValue >() : f_bound );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue get_global_bound( void ) { return( f_bound ); }

/*--------------------------------------------------------------------------*/
 /// returns true if a finite lower/upper (if convex/concave) bound is set

 bool is_bound_set( void ) const {
  return( f_is_convex ? f_bound > - Inf< FunctionValue >()
                      : f_bound < Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (global) Lipschitz constant for the PolyhedralFunction

 FunctionValue get_Lipschitz_constant( void ) override
 {
  if( f_Lipschitz_constant < 0 )
   compute_Lipschitz_constant( v_A , 0 );
  return( f_Lipschitz_constant );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this PolyhedralFunction is convex

 bool is_convex( void ) const override { return( f_is_convex ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if and only if this PolyhedralFunction is concave

 bool is_concave( void ) const override { return( ! f_is_convex ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this PolyhedralFunction is linear
 /** Method that returns true if and only if this PolyhedralFunction is
  * linear. It therefore returns false. Actually, in the case where the
  * number of rows in A is less than or equal to one the PolyhedralFunction
  * is linear, but this is supposed to be an accident that may and probably
  * will change at any time, so we report the "safe" result. */

 bool is_linear( void ) const override { return( false ); }

/*--------------------------------------------------------------------------*/
 /// tells whether a linearization is available

 bool has_linearization( bool diagonal = true ) override;

/*--------------------------------------------------------------------------*/
 /// compute a new linearization for this PolyhedralFunction

 bool compute_new_linearization( bool diagonal = true ) override
 {
  if( ( ! diagonal ) || ( v_A.empty() && ( ! is_bound_set() ) ) ||
      ( f_next >= v_ord.size() - 1 ) || ( f_next >= f_loc_pool_sz - 1 ) )
   return( false );

  ++f_next;
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// store a linearization in the global pool 

 void store_linearization( Index name ,  ModParam issueMod = eModBlck )
  override;

/*--------------------------------------------------------------------------*/

 bool is_linearization_there( Index name ) const override {
  return( v_glob[ name ] < Inf< int >() );
  }

/*--------------------------------------------------------------------------*/
 /// stores a convex combination of the given linearizations
 /** This method creates a convex combination of the given set of
  * linearizations, with the given coefficients, and stores it into the
  * global pool of linearizations with the given name.
  *
  * As PolyhedralFunction only produces diagonal linearizations, all the
  * involved linearizations are necessarily diagonal. For the resulting
  * linearization to be valid, the coefficients must belong to the unitary
  * simplex, i.e., be non-negative and sum to 1. However, if a valid global
  * lower bound is present, then the coefficients can sum to a number strictly
  * less then 1. The remainder ( 1 - sum of coefficients ) is taken as the
  * convex multiplier of the "horizontal", all-0 subgradient corresponding to
  * the lower bound (which that PolyhedralFunction never explicitly produces),
  * and it is used to properly compute the constant of the newly produced
  * linearization, as it also has the term "lower bound * that multiplier".
  *
  * Note that in the current implementation of PolyhedralFunction,
  * linearizations obtained by convex combination are "very fragile". Each
  * time any linearization changes or is deleted, or Variable are added,
  * understanding if a linearization obtained by convex combination is still
  * valid, or computing its new entries, would require keeping track of the
  * original set of linearizations that produced it, and the corresponding
  * linear coefficients. This would be possible but it is currently not done.
  * As a consequence, many changes in the PolyhedralFunction produce the
  * immediate removal from the global pool of all the linearizations obtained
  * by linear combination. */

 void store_combination_of_linearizations(
			    c_LinearCombination & coefficients , Index name ,
			    ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/

 void set_important_linearization( LinearCombination && coefficients )
  override { f_imp_coeff = std::move( coefficients ); }

/*--------------------------------------------------------------------------*/

 c_LinearCombination & get_important_linearization_coefficients( void )
  const override { return( f_imp_coeff ); }

/*--------------------------------------------------------------------------*/

 void delete_linearization( Index name , ModParam issueMod = eModBlck )
  override;

/*--------------------------------------------------------------------------*/

 void delete_linearizations( Subset && which , bool ordered = true ,
			     ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
				      Range range = INFRange ,
				      Index name = Inf< Index >() ) override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void get_linearization_coefficients( SparseVector & g ,
				      Range range = INFRange ,
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

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override;

/*--------------------------------------------------------------------------*/
 /// serialize a PolyhedralFunction into a netCDF::NcGroup
 /** Serialize a PolyhedralFunction into a netCDF::NcGroup, with the following
  * format:
  *
  * - The dimension "PolyFunction_NumVar" containing the number of columns of
  *   the A matrix, i.e., the number of active variables.
  *
  * - The dimension "PolyFunction_NumRow" containing the number of rows of the
  *   A matrix. The dimension is optional, if it is not provided than 0 (no
  *   rows) is assumed.
  *
  * - The variable "PolyFunction_A", of type double and indexed over both
  *   the dimensions NumRow and NumVar (in this order); it contains the
  *   (row-major) representation of the matrix A. The variable is only
  *   optional if NumRow == 0.
  *
  * - The variable "PolyFunction_b", of type double and indexed over the
  *   dimension NumRow, which contains the vector b. The variable is only
  *   optional if NumRow == 0.
  *
  * - The dimension "PolyFunction_sign" (actually a bool), which contains the
  *   "verse" of the PolyhedralFunction, i.e., true for a convex max-function
  *   and false for a concave min-function (encoded in the obvious way, i.e.,
  *   zero for false, nonzero for true). The variable is optional, if it is
  *   not provided true is assumed.
  *
  * - The scalar variable "PolyFunction_lb", of type double and not indexed
  *   over any dimension, which contains the global lower (if
  *   PolyFunction_sign == true, upper otherwise) bound on the value of the
  *   function over all the space. The variable is optional, if it is not
  *   provided it means that no finite lower (upper) bound exist, i.e., the
  *   lower (upper) bound is -(+) Inf< FunctionValue >(). */
 
 void serialize( netCDF::NcGroup & group ) const;

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current A matrix in the mapping

 const MultiVector & get_A( void ) const { return( v_A ); }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current b vector in the mapping

 const RealVector & get_b( void ) const { return( v_b ); }
 
/*--------------------------------------------------------------------------*/
 /// returns the number of rows in the PolyhedralFunction

 Index get_nrows( void ) const { return( v_A.size() ); }
 
/** @} ---------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the PolyhedralFunction
 *  @{ */

 /// get a specific integer (int) numerical parameter
 /** Get a specific integer (int) numerical parameter. PolyhedralFunction
  * takes care of intLPMaxSz and intGPMaxSz, leaving all the rest to
  * Function. */

 int get_int_par( const idx_type par ) const override final {
  switch( par ) {
   case( intLPMaxSz ): return( f_loc_pool_sz );
   case( intGPMaxSz ): return( v_glob.size() );
   }
  return( Function::get_int_par( par ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a specific float (double) numerical parameter
 /** Get a specific float (double) numerical parameter. PolyhedralFunction
  * takes care of dblAAccMlt, leaving all the rest to C05Function. */

 double get_dbl_par( const idx_type par ) const override final {
  if( par == dblAAccMlt )
   return( AAccMlt );
  else
   return( C05Function::get_dbl_par( par ) );
  }

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS FOR HANDLING THE State OF THE PolyhedralFunction --------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the State of the PolyhedralFunction
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
/*---- METHODS FOR HANDLING "ACTIVE" Variable IN THE PolyhedralFunction ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * PolyhedralFunction; this is the actual concrete implementation exploiting
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

 Variable * get_active_var( const Index i ) const override final {
  return( v_x[ i ] );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override final
 {
  return( new PolyhedralFunction::v_iterator( std::move( v_x.begin() ) ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_begin( void ) const override final
 {
  return( new PolyhedralFunction::v_const_iterator( std::move( v_x.begin() )
						    ) );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override final
 {
  return( new PolyhedralFunction::v_iterator( v_x.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 v_const_iterator * v_end( void ) const override final
 {
  return( new PolyhedralFunction::v_const_iterator( v_x.end() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR MODIFYING THE PolyhedralFunction ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the PolyhedralFunction
 *  @{ */

 /// completely resets the PolyhedralFunction with entirely new data
 /** Completely resets the PolyhedralFunction with entirely new data,
  * but leaving the current set of n = get_num_active_var() input Variable:
  *
  * @param A a m-vector of n-vectors of FunctionValue representing the A
  *        matrix in the definition of the function: entry A[ i ][ j ] is
  *        (obviously) meant to be the coefficient of variable
  *        get_active_var( j ) for the i-th row;
  *
  * @param b a m-vector of FunctionValue representing the b vector in the
  *        definition of the function: b[ i ] is the constant factor of the
  *        i-th linear form;
  *
  * @param bound is the global valid lower (if the function is convex) or
  *        upper (if the function is convace) bound on the function value,
  *        with default - Inf< FunctionValue >();
  *
  * @param is_convex a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization and therefore it is a concave function, with
  *        default true.
  *
  * @param issueMod which decides if and how the FunctionMod (with f_shift
  *        == FunctionMod::NaNshift, i.e., "everything changed") is issued,
  *        as described in Observer::make_par().
  *
  * This completely resets the PolyhedralFunction, save that it remains
  * defined on the very same variable space, which means that
  *
  *     n (THE NUMBER OF COLUMNS IN A) MUST BE EQUAL TO get_num_active_var(),
  *     EXCEPT IN THE CASE WHERE get_num_active_var() == 0, WHICH MEANS THAT
  *     set_variables() WILL BE CALLES *AFTER* THIS METHOD (AND THEN,
  *     CLEARLY, THE NUMBER OF Variable WILL HAVE TO BE COMPATIBLE WITH
  *     THE SIZE OF A AS SET HERE), OR IN THE CASE WHERE A IS empty(), WHICH
  *     MEANS THAT THE MAPPING IS COMPLETELY RESET (LEAVING THE SAME Variable
  *
  * As the && implies, A and b become property of the PolyhedralFunction
  * object. */
 
 void set_PolyhedralFunction( MultiVector && A , RealVector && b ,
			      FunctionValue bound = - Inf< FunctionValue >() ,
			      bool is_convex = true ,
			      ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// change the "sign" of the PolyhedralFunction
 /**< The method allows to [re]set the parameter governing the "sign" of the
  * PolyhedralFunction:
  *
  * @param is_convex a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization and therefore it is a concave function.
  *
  * @param issueMod which decides if and how the PolyhedralFunctionMod is
  *        issued, as described in Observer::make_par().
  *
  * Note that when the sign changes from "max" to "min" (from convex to
  * concave) then the value of the function surely decreases, and vice-versa.
  * However, funnily enough *all the linearizations remain valid* without any
  * change. The difference is of course that when the function was (say)
  * convex they were (approximate) *sub*gradients, i.e., *lower*
  * linearizations of the *epi*graph; as the function is turned into (say)
  * concave they become (approximate) *super*gradients, i.e., *upper*
  * linearizations of the *ipo*graph. Still, each linearization is still a
  * valid one, which is the poster case for the weird-ish setting
  * C05FunctionMod::NothingChanged for the type() of the C05FunctionMod. */

 void set_is_convex( bool is_convex = true ,
		     ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add a set of new Variable to the PolyhedralFunction
 /**< The method receives:
  *
  * @param nx a std::vector< ColVariable * > && containing the pointers to k
  *        new ColVariable, which will take index n, n + 1, ..., n + k - 1
  *        where n = get_num_active_var() (*before* the call). Note that the
  *        order of the variables in nx dictates the index of the "active"
  *        Variable: after the call, nx[ 0 ] == get_active_var( n ),
  *        nx[ 1 ] = get_active_var( n + 1 ), ... Also, the correspondence
  *        with nA is (see below) positional. Note that
  *
  *            IT IS EXPECTED THAT NONE OF THE NEW ColVariable IS ALREADY
  *            "ACTIVE" IN THE PolyhedralFunction, BUT NO CHECK IS DONE TO
  *            ENSURE THIS
  *
  *        Indeed, the check is costly, and the PolyhedralFunction does not
  *        really have a functional issue with repeated ColVariable. The
  *        real issue rather comes whenever the PolyhedralFunction is used
  *        within a Constraint or Objective that need to register istelf
  *        among the "active" Variable of the PolyhedralFunction; this
  *        process is not structured to work with multiple copies of the same
  *        "active" Variable. Thus, a PolyhedralFunction used within such an
  *        object should not have repeated Variable, but if this is an issue
  *        then the check will have to be performed elsewhere.
  *
  * @param nA a MultiVector && having as many rows as the current A matrix (if
  *        the current A matrix is not empty) and exactly k columns
  *        representing the new part of the linear mapping; entry nA[ i ][ h ]
  *        is (obviously) meant to be the coefficient of *nx[ h ] for the i-th
  *        row;
  *
  * @param issueMod which decides if and how the C05FunctionModVarsAddd
  *        (since a PolyhedralFunction is strongly quasi-additive, and with
  *        shift() == 0 as expected) is issued, as described in
  *        Observer::make_par().
  *
  * As the && tells, nx and nA become "property" of the PolyhedralFunction
  * object, altough this likely only happens if A is currently "empty of
  * columns" (say, only the rows have been defined, or all previous columns
  * have been deleted). */

 void add_variables( VarVector && nx , MultiVector && nA ,
		     ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the PolyhedralFunction
 /** Like add_variables(), but just only one Variable:
  *
  * @param var is a ColVariable *, and the pointed ColVariable must *not* be
  *        already among the active Variable of the PolyhedralFunction.
  *
  * @param Aj is a RealVector having as many rows as the current A matrix and
  *        containing the new column of the linear mapping; entry Aj[ i ][ h ]
  *        is (obviously) meant to be the coefficient of *var for the i-th
  *        row.
  *
  * @param issueMod which decides if and how the C05FunctionModVarsAddd
  *        (since a PolyhedralFunction is strongly quasi-additive, and with
  *        shift() == 0 as expected) is issued, as described in
  *        Observer::make_par(). */

 void add_variable( ColVariable * const var , c_RealVector & Aj ,
		    ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove the i-th Variable
 /** Like remove_variable( Variable * ), but takes in input the index of
  * the Variable to be removed rather than its pointer. Useful if one knows
  * the index already, so that it need not be searched for.
  *
  * @param i is the index of the Variable to be removed, an integer between 0
  *        and get_num_active_var() - 1.
  *
  * @param issueMod which decides if and how the C05FunctionModVarsRngd
  *        (since a PolyhedralFunction is strongly quasi-additive, and with
  *        shift() == 0 as expected) is issued, as described in
  *        Observer::make_par(). */

 void remove_variable( Index i , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove a range of "active" Variable.
  *
  * @param range contains the indices of the Variable to be deleted
  *        (hence, range.second <= get_num_active_var());
  *
  * @param issueMod which decides if and how the C05FunctionModVarsRngd
  *        (since a PolyhedralFunction is strongly quasi-additive, and with
  *        shift() == 0 as expected) is issued, as described in
  *        Observer::make_par(). */

 void remove_variables( Range range , ModParam issueMod = eModBlck )
  override final;

/*--------------------------------------------------------------------------*/
 /// remove a subset of Variable
 /** Remove all the Variable in the given set of indices.
  *
  * @param nms is Subset & containing the indices of the Variable to be
  *        removed, i.e., integers between 0 and get_num_active_var() - 1;
  *        note that Subset is non-const because if it is not ordered (see
  *        next parameter) the method may decide to order it, and/or to
  *        move it into the Modification that is possibly issued (see third
  *        parameter), which means that it is in general not safe to assume
  *        that the parameter is still available after the call. a special
  *        setting is if
  *
  *     nms.empty() == true, IN WHICH CASE ALL Variable ARE ELIMINATED
  *
  *        note that removing a Variable causes the deletion of the
  *        corresponding column from th current A matrix, as a consequence,
  *
  *     nms.empty() == true CORRESPOND TO A COMPLETE RESET OF THE A MATRIX,
  *     BUT *NOT* OF THE b VECTOR (NOR OF THE GLOBAL BOUND, IF ANY)
  *
  * @param ordered is a bool indicating if nms[] is already ordered in
  *        increasing sense (otherwise this is done inside the method,
  *        which is why nms[] is not const);
  *
  * @param issueMod which decides if and how the C05FunctionModVarsSbst
  *        (with f_shift == 0, since a PolyhedralFunction is strongly
  *        quasi-additive) is issued, as described in Observer::make_par(). */

 void remove_variables( Subset && nms , bool ordered = false ,
			ModParam issueMod = eModBlck ) override final;

/*--------------------------------------------------------------------------*/
 /// modify a range of rows of the linear mapping
 /** Modifies a range of rows of the linear mapping:
  *
  * @param Range range contains the indices of the rows to be modified, hence
  *        range.second < get_A().size();
  *
  * @param nA, a MultiVector && with nA.size() == range.second - range.first,
  *        and exactly as many columns as the current A matrix; entry
  *        nA[ i ][ h ] is (obviously) meant to be the new coefficient for
  *        the h-th variable in row range.first + i; as the && implies, nA 
  *        becomes property of the PolyhedralFunction object;
  *
  * @param the RealVector & nb, a vector of FunctionValue with nb.size() ==
  *        range.second - range.first: entry nb[ i ] is (obviously) meant to
  *        be the new value of the constant term for row range.first + i;
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that shift() ==
  *        NANshift (the function has changed "unpredictably"),  type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and PFtype() ==
  *        ModifyRows. */  

 void modify_rows( MultiVector && nA , c_RealVector & nb , Range range ,
		   ModParam issueMod = eModBlck );
 
/*--------------------------------------------------------------------------*/
 /// modify a subset of rows of the linear mapping
 /** Modifies a subset of rows of the linear mapping:
  *
  * @param Subset && rows contains the indices of the rows to be modified; all
  *        entries must therefore be numbers in 0, ..., get_A().size() - 1;
  *        as the && tells, the vector becomes property of the
  *        PolyhedralFunction, to be dispatched to the issued
  *        PolyhedralFunctionModSbst (if any);
  *
  * @param bool ordered tells if rows is already ordered by increasing index
  *        (if not it may be ordered inside, after all it becomes property
  *        of the PolyhedralFunction);
  *
  * @param nA, a MultiVector && with nA.size() == rows.size() and exactly as
  *        many columns as the current A matrix; entry nA[ i ][ h ] is
  *        (obviously) meant to be the new coefficient for the h-th variable
  *        in row rows[ i ]; as the && implies, nA becomes property of the
  *        PolyhedralFunction object;
  *
  * @param the RealVector & nb, a vector of FunctionValue with nb.size() ==
  *        nms.size(): entry nb[ i ] is (obviously) meant to be the new value
  *        of the constant term for row rows[ i ];
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModSbst is
  *        issued, as described in Observer::make_par(). Note that shift() ==
  *        NANshift (the function has changed "unpredictably"),  type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and PFtype() ==
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
  *        i-th linear mapping; as the && tells, Ai becomes "property" of the
  *        PolyhedralFunction object and physically replaces the previous
  *        vector;
  *
  * @param bi is the new constant term of the i-th mapping;
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that shift() ==
  *        NANshift (the function has changed "unpredictably"),  type() ==
  *        AllLinearizationChanged (all the linearizations may have changed,
  *        although actually only a subset of them has) and PFtype() ==
  *        ModifyRows. */  

 void modify_row( Index i , RealVector && Ai , FunctionValue bi ,
		  ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a range of rows of the linear mapping
 /** Like modify_rows( range ), but modify the constant terms only.
  *
  * @param Range range contains the indices of the rows to be modified, hence
  *        range.second < get_A().size();
  * 
  * @param the RealVector & nb, a vector of FunctionValue with nb.size() ==
  *        range.second - range.first: entry nb[ i ] is (obviously) meant to
  *        be the new value of the constant term for row range.first + i;
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and PFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all*
  *        rows, then the function has necessarily decreased, hence the shift
  *        is -INFshift. Otherwise the value has changed "unpredictably" and
  *        the shift is NANshift (unless all the values are equal, in which
  *        case the function value has not chsanged and the method does
  *        nothing). */  

 void modify_constants( c_RealVector & nb , Range range ,
			ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify only the constant term of a subset of rows of the linear mapping
 /** Like modify_rows( subset ), but modify the constant terms only.
  *
  * @param Subset && rows contains the indices of the rows to be modified; all
  *        entries must therefore be numbers in 0, ..., get_A().size() - 1;
  *        as the && tells, the vector becomes property of the
  *        PolyhedralFunction, to be dispatched to the issued
  *        PolyhedralFunctionModSbst (if any);
  *
  * @param ordered tells if rows is already ordered by increasing index
  *        (if not it may be ordered inside, after all it becomes property
  *        of the PolyhedralFunction);
  *
  * @param nb a vector of FunctionValue with nb.size() == rows.size(): entry
  *        nb[ i ] is (obviously) meant to be the new value of the constant
  *        term for row rows[ i ];
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModSbst is
  *        issued, as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only a subset of them has) and PFtype() == ModifyCnst. As for
  *        shift(), however, the value of the function *may* change in a very
  *        predictable way: if the new value of the constant is > than the
  *        current value for *all* rows, then the function has necessarily
  *        increased, hence the shift is +INFshift. If it is < for *all*
  *        rows, then the function has necessarily decreased, hence the shift
  *        is -INFshift. Otherwise the value has changed "unpredictably" and
  *        the shift is NANshift (unless all the values are equal, in which
  *        case the function value has not chsanged and the method does
  *        nothing). */  

 void modify_constants( c_RealVector & nb , Subset && rows ,
			bool ordered = false ,
			ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify only the constant term of one row of the linear mapping
 /** Like modify_constants(), but only for one row:
  *
  * @param i is the index of the row to be modified;
  *
  * @param bi is the new constant term of the i-th mapping;
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (all the alphas may have changed, although actually
  *        only one of them has) and PFtype() == ModifyCnst. As for
  *        shift(), the value of the function changes in a very predictable
  *        way: if bi is > than the current value the function has
  *        necessarily increased, otherwise necessarily decreased (if it is
  *        == it has not changed and the method does nothing), hence the
  *        shift is either +INFshift or -INFshift accordingly. */  

 void modify_constant( Index i , FunctionValue bi ,
		       ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify the global lower/upper bound
 /** This is basically modify_constant() for the "virtual" all-0 row
  * corresponding to the global lower/upper bound:
  *
  * @param newbound is the new value for the bound; note that newbound ==
  *        - Inf< FunctionValue >() (no lower bound) is permitted for convex
  *        functions, while newbound == Inf< FunctionValue >() (no upper
  *        bound) is permitted for concave functions;
  *
  * @param issueMod, which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par(). Note that type() ==
  *        AlphaChanged (this is basically the change in one specific
  *        linearization) and PFtype() == ModifyCnst. To signal this very
  *        special kind of change, the Range in the PolyhedralFunctionModRngd
  *        is set to < 0 , 0 >. As for shift(), the value of the function
  *        changes in a very predictable way: if newbound is > than the
  *        current value the function has necessarily increased, otherwise
  *        necessarily decreased (if it is == it has not changed and the
  *        method does nothing), hence the shift() is either +INFshift or
  *        -INFshift accordingly. */  

 void modify_bound( FunctionValue newbound , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add some rows to the linear mapping in the PolyhedralFunction
 /** Adds some rows to the linear mapping in the PolyhedralFunction, leaving
  * the current set of n = get_num_active_var() input Variable and all the
  * current rows:
  *
  * @param nA a k-vector of n-vectors of FunctionValue representing the new
  *        rows of the A matrix in the definition of the function; entry nA[ i
  *        ][ j ] is (obviously) meant to be the coefficient of variable *x[ j
  *        ] for the i-th new row; as the && tells, the object (most likely,
  *        its individual rows) becomes "property" of the PolyhedralFunction.
  *
  * @param nb a k-vector of FunctionValue representing the new entries of b
  *        vector in the definition of the function (that is, nb[ i ] is the
  *        constant factor of the i-th given linear form);
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModAddd is
  *        issued, as described in Observer::make_par().
  *
  * Note that adding new rows makes a "max" (convex) function to increase in
  * value and a "min" (concave) one to decrease in value, but all existing
  * linearization are still valid ones, which is the poster case for the
  * weird-ish setting C05FunctionMod::NothingChanged for the type() of the
  * C05FunctionMod. */

 void add_rows( MultiVector && nA , c_RealVector & nb ,
		ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new row to the linear mapping
 /** Like add_row(), but just only one row of the linear mapping:
  *
  * @param Ai is the RealVector, with exactly n = get_num_active_var()
  *        elements, with the coefficients of the new row in the mapping; as
  *        the && tells, Ai becomes "property" of the PolyhedralFunction
  *        object;
  *
  * @param bi is the constant term of the new row in the mapping;
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModAddd is
  *        issued, as described in Observer::make_par().
  *
  * Note that adding a new row makes a "max" (convex) function to increase in
  * value and a "min" (concave) one to decrease in value, but all existing
  * linearization are still valid ones, which is the poster case for the
  * weird-ish setting C05FunctionMod::NothingChanged for the type() of the
  * C05FunctionMod. */

 void add_row( RealVector && Ai , FunctionValue bi ,
	       ModParam issueMod = eModBlck );
 
/*--------------------------------------------------------------------------*/
 /// deletes a range of rows from the linear mapping in the PolyhedralFunction
 /**< Deletes a range rows from the linear mapping in the PolyhedralFunction,
  * leaving the current set of n = get_num_active_var() input Variable and
  * all rows that are not explicitly deleted:
  *
  * @param range contains the indices of the rows to be deleted, hence
  *        range.second < get_b().size(); note that if range.second ==
  *        get_b().size() - 1 == get_A.size(), then also the "virtual"
  *        all-0 row corresponding to the global lower/upper bound is deleted,
  *        which means that the bound is reset to +/- INF as appropriate.
  *
  * @param issueMod which decides if and how the PolyhedralFunctionModRngd is
  *        issued, as described in Observer::make_par().
  *
  * Note that removing rows makes a "max" (convex) function to decrease in
  * value and a "min" (concave) one to increase in value; also, existing
  * lnearizations in the global pool may disappear. Even worse, and aggregated
  * linearization may have been constructed out of the ones that are deleted,
  * and there is no way of saying it in general. Hence, the
  * PolyhedralFunctionModRngd will have type() = C05FunctionMod::AlphaChanged
  * if:
  *
  * - any of the deleted rows are present in the global pool;
  *
  * - any aggregated linearization is present in the global pool.
  *
  * Otherwise no linearization is affected, and the PolyhedralFunctionModRngd
  * will have type() = C05FunctionMod::NothingChanged. The linearizations
  * that are not deleted (both those in the list and the aggregated ones)
  * remain identical (the constant term does not change, even less the vector
  * of coefficients), the others get constant == Inf< FunctionValue >(), and
  * therefore the vector of coefficients is no longer significant.
  *
  * TODO: if the deleted rows are "too many", rather issue a FunctionMod
  * with shift() == FunctionMod::NaNshift, i.e., "everything changed"
  * (cf. delete_rows( all )). */

 void delete_rows( Range range , ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// deletes a subset of rows from the linear mapping
 /**< Deletes a subset of rows from the linear mapping in the
  * PolyhedralFunction, leaving the current set of n = get_num_active_var()
  * input Variable and all rows that are not explicitly deleted:
  *
  * @param rows contains the indices of the rows to be modified;
  *        all entries must therefore be numbers in 0, ..., get_A().size();
  *        since the special value get_A().size() == get_b().size() - 1
  *        indicates the "virtual" all-0 row corresponding to the global
  *        lower/upper bound, is the index is found in rows then also that
  *        row is (virtually) deleted, which means that the bound is reset
  *        to +/- INF as appropriate; as the && tells, the vector becomes
  *        property of the PolyhedralFunction, to be dispatched to the issued
  *        PolyhedralFunctionModSbst (if any);
  *
  * @param ordered tells if rows is already ordered by increasing index
  *        (if not it may be ordered inside, after all it becomes property
  *        of the PolyhedralFunction);
  *
  * Note that removing rows makes a "max" (convex) function to decrease in
  * value and a "min" (concave) one to increase in value; also, existing
  * lnearizations in the global pool may disappear. Even worse, and aggregated
  * linearization may have been constructed out of the ones that are deleted,
  * and there is no way of saying it in general. Hence, the
  * PolyhedralFunctionModSbst will have type() = C05FunctionMod::AlphaChanged
  * if:
  *
  * - any of the deleted rows are present in the global pool;
  *
  * - any aggregated linearization is present in the global pool.
  *
  * Otherwise no linearization is affected, and the PolyhedralFunctionModSbst
  * will have type() = C05FunctionMod::NothingChanged. The linearizations
  * that are not deleted (both those in the list and the aggregated ones)
  * remain identical (the constant term does not change, even less the vector
  * of coefficients), the others get constant == Inf< FunctionValue >(), and
  * therefore the vector of coefficients is no longer significant.
  *
  * TODO: if the deleted rows are "too many", rather issue a FunctionMod
  * with shift() == FunctionMod::NaNshift, i.e., "everything changed"
  * (cf. delete_rows( all )). */

 void delete_rows( Subset && rows , bool ordered = false ,
		   ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes one single existing row from the linear mapping
 /** Like delete_rows(), but just only the i-th row of the linear mapping.
  */

 void delete_row( Index i , ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes all rows from the linear mapping in the PolyhedralFunction
 /**< Like delete_rows( range ), but immediately removes *all* the matrix A
  * and vector b, leaving the mapping "empty". This of course also resets 
  * all aggregated linearizations, which can no longer be valid, as wel as
  * the global lower/upper bouns. Hence, the best Modification to issue is a
  * FunctionMod with shift() == FunctionMod::NaNshift, i.e., "everything
  * changed". */

 void delete_rows( ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
/*-------------------------------- FRIENDS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class PolyhedralFunctionState;  // make PolihedralFunctionState friend

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the PolyhedralFunction on an ostream
 /** Protected method intended to print information about the C05Function; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. */

 void print( std::ostream &output ) const override
 {
  output << "PolyhedralFunction [" << this << "]"
	 << " with " << get_num_active_var() << " columns and"
	 << get_nrows() << " rows";
  }

/*--------------------------------------------------------------------------*/
 // returns the subset of elements in the global pool to be removed and added

 std::pair< Subset , Subset > guts_of_put_State(
				      const PolyhedralFunctionState & state );

/*--------------------------------------------------------------------------*/

 void set_f_uncomputed( void ) {
  f_value = f_is_convex ? Inf< FunctionValue >() : - Inf< FunctionValue >();
  }

/*--------------------------------------------------------------------------*/

 bool is_f_computed( void ) const {
  return( f_is_convex ? f_value < Inf< FunctionValue >()
                      : f_value > - Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue get_default_bound( void ) const {
  return( f_is_convex ? - Inf< FunctionValue >() : Inf< FunctionValue >() );
  }

/*--------------------------------------------------------------------------*/

 void reset_v_ord( void ) {
  v_ord.resize( f_loc_pool_sz == 1 ? 1 : get_nrows() + 1 );
  std::iota( v_ord.begin() , v_ord.end() , 0 );
  }

/*--------------------------------------------------------------------------*/

 void compute_Lipschitz_constant( const MultiVector & A , FunctionValue init )
 {
  f_Lipschitz_constant = init * init;
  for( const auto & Ai : A ) {
   FunctionValue L = 0;
   for( const auto aij : Ai )
    L += aij * aij;

   if( L > f_Lipschitz_constant )
    f_Lipschitz_constant = L;
   }

  f_Lipschitz_constant = sqrt( double( f_Lipschitz_constant ) );
  }

/*--------------------------------------------------------------------------*/

 FunctionValue * get_ai( Index name );

/*--------------------------------------------------------------------------*/

 void compact_combinations( void )
 {
  // until the last position is not needed, shorten v_aA[] and v_ab[]
  while( ! v_ab.empty() ) {
   auto last = --v_ab.end();
   if( *last == Inf< FunctionValue >() ) {
    v_aA.pop_back();
    v_ab.pop_back();
    }
   else
    break;
   }
  }

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 VarVector v_x;       ///< the pointer to the variables x in A x + b

 bool f_is_convex;    ///< true if the function is a "max" = convex one

 MultiVector v_A;     ///< the A matrix of A x + b
 
 RealVector v_b;      ///< the b vector of A x + b

 FunctionValue f_bound;  ///< the global (upper or lower) bound
 
 MultiVector v_aA;    ///< the A matrix for aggregated linearizations
 
 RealVector v_ab;     ///< the b vector for aggregated linearizations

 FunctionValue f_value;   ///< the value of the function

 FunctionValue f_Lipschitz_constant;  ///< the Lipschitz constant

 Index f_loc_pool_sz;  ///< size of the local pool
 Index f_next;         ///< next linearization in the local pool
 
 Subset v_ord;         ///< the ordering of linearizations

 std::vector< int > v_glob;  ///< the global pool, a vector of *signed* ints
 /**< h = v_glob[ i ] contains the place where the i-th item of the global
  * pool is stored:
  * - if h == Inf< int >() there is no item with this name
  * - if h == 0 then it's the all-0 linearization, which is not stored
  *   anywhere, and its constant term f_bound
  * - if 0 < h <= get_nrows() then it's an original linearization and it's
  *   found in v_A[ h - 1 ] and v_b[ h - 1 ];
  * - if h < 0 then it's an aggregated one and it's found in v_aA[ - h - 1 ]
  *   and v_ab[ - h - 1 ]. */

 Index f_max_glob;           ///< 1 + maximum active name in the global pool
 /**< f_max_glob is strictly larger than the maximum index h such that
  * v_glob[ h ] != Inf< int >(), i.e., v_glob[ f_max_glob ] == Inf< int >()
  * while v_glob[ f_max_glob - 1 ] != Inf< int >(). Note that one should
  * never check v_glob[ f_max_glob ], as f_max_glob == v_glob.size() may
  * happen (in particular when v_glob.empty() and f_max_glob == 0). */

 LinearCombination f_imp_coeff;  ///< coefficients of the important linear.

 FunctionValue AAccMlt;  ///< maximum absolute error in the multipliers

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 void reset_aggregate_linearizations( void );

 void reset_aggregate_linearizations( ModParam issueMod );

 void update_f_max_glob( void ) {
  while( f_max_glob && ( v_glob[ f_max_glob - 1 ] == Inf< int >() ) )
   --f_max_glob;
  }
 
/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunction ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS PolyhedralFunctionMod -----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modifications specific to a PolyhedralFunction
/** Derived class from C0FunctionMod to describe modifications to a
 * PolyhedralFunction. This obviously "keeps the same interface" as
 * C0FunctionMod, so that it can be used by Solver and/or Block just relying
 * on the C0Function interface, but it also add PolyhedralFunction-specific
 * information, so that Solver and/or Block can actually react in
 * PolyhedralFunction-specific if they want to.
 *
 * This base class actually has *no* PolyhedralFunction-specific information,
 * besides being of a specific type; a PolyhedralFunctionMod is issued by
 * the method set_is_convex(), which means that the "sign" of the
 * PolyhedralFunction is changed; therefore, no other information is needed.
 * Further derived classes contain data for other types of changes. */

class PolyhedralFunctionMod : public C05FunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

  /// Definition of the possibles type of PolyhedralFunctionMod
  /** This enum specifies what kind of assumption can be made about any
   * previously produced linearization. Note that the enum is *not* useful
   * for all derived classes, in particular for PolyhedralFunctionModAddd
   * that only encodes for a single type of operation, but it is still
   * defined here to avoid being defined identically multiple times. */
  enum poly_function_mod_type {
   ModifyRows ,         ///< modify a set of rows (both A and b)
   ModifyCnst ,         ///< modify a set of constants (b only)
   DeleteRows ,         ///< delete a set of rows
   PolyhedralFunctionModLastParam
   ///< First allowed parameter value for derived classes
   /**< Convenience value for easily allow derived classes to extend
    * the set of types of modifications. */
   };

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: identical to that of C05FunctionMod
 /** Constructor: takes a pointer to the affected PolyhedralFunction, the type
  * of the Modification, the Subset of affected linearizations, the value of
  * the shift, and the "concerns Block" value. No PolyhedralFunction-specific
  * information is added. */

 PolyhedralFunctionMod( PolyhedralFunction * f , int type ,
			Subset && which = {} ,
			FunctionValue shift = NaNshift , bool cB = true )
  : C05FunctionMod( f , type , std::move( which ) , shift , cB ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~PolyhedralFunctionMod() { }  ///< destructor: does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the PolyhedralFunctionMod

 void print( std::ostream &output ) const override {
  output << "PolyhedralFunctionMod[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on PolyhedralFunction [" << &f_function << " ]: ";
  guts_of_print( output );
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionMod ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PolyhedralFunctionModAddd --------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe modification specific to a PolyhedralFunction: add rows
/** Derived class from PolyhedralFunctionMod to describe a very specifuc
 * modifications to a PolyhedralFunction: add some new rows. The 
 * PolyhedralFunction-specific information is therefore the number of
 * added rows. */

class PolyhedralFunctionModAddd : public PolyhedralFunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of PolyhedralFunctionMod + the added rows
 /** Constructor: takes a pointer to the affected C05Function, the number of
  * added rows, and the "concerns Block" value. Note that the type of the
  * Modification is set to C05FunctionMod::NothingChanged, the set of changed
  * rows is set to {}, and the value of shift is automatically set to either
  * +INF or -INF depending on the convexity, hence they need not be provided.
  */

 explicit PolyhedralFunctionModAddd( PolyhedralFunction * f , Index ar ,
				     bool cB = true )
  : PolyhedralFunctionMod( f , C05FunctionMod::NothingChanged , Subset( {} ) ,
			   f->is_convex() ? INFshift : - INFshift , cB ) ,
    f_addedrows( ar ) {}

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~PolyhedralFunctionModAddd() = default;  ///< default destructor

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the number of added rows

 [[nodiscard]] Index addedrows( void ) const { return( f_addedrows ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the PolyhedralFunctionModAddd

 void print( std::ostream &output ) const override {
  output << "PolyhedralFunctionModAddd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on PolyhedralFunction [" << &f_function << " ]: added "
	 << f_addedrows << " rows" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

  Index f_addedrows;  ///< number of added rows

/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionModAddd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS PolyhedralFunctionModRngd ---------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe range modification specific to a PolyhedralFunction
/** Derived class from PolyhedralFunctionMod to describe all modifications to
 * a PolyhedralFunction that involve a Range of rows:
 *
 * - modify_row[s]
 * - modify_constant[s]
 * - delete_row[s]
 *
 * For all these, the Range of the affected rows is provided, as well as
 * the exact type of operation.
 *
 * This class also supports a special case: if the Range is empty (i.e.,
 * range.second <= range.first, e.g. as in Range( 0 , 0 )), a
 * PolyhedralFunctionModRngd of type ModifyCnst indicates the change of
 * the global lower/upper bound. */

class PolyhedralFunctionModRngd : public PolyhedralFunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of PolyhedralFunctionMod + the added rows
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * C05FunctionMod, the type of the PolyhedralFunctionMod, the Range of
  * concerned rows, the value of the shift, and the "concerns Block" value.
  */

 explicit PolyhedralFunctionModRngd( PolyhedralFunction * f , int type ,
				     int pftype , c_Range & range ,
				     Subset && which = {} ,
				     FunctionValue shift = NaNshift ,
				     bool cB = true )
  : PolyhedralFunctionMod( f , type , std::move( which ) , shift , cB ) ,
    f_PFtype( pftype ) , f_range( range ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~PolyhedralFunctionModRngd() = default;  ///< default destructor

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the specific sub-type of PolyhedralFunctionMod

 [[nodiscard]] int PFtype( void ) const { return( f_PFtype ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the range of the affected rows

 [[nodiscard]] c_Range & range( void ) const { return( f_range ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the PolyhedralFunctionModRngd

 void print( std::ostream &output ) const override {
  output << "PolyhedralFunctionModRngd[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on PolyhedralFunction [" << &f_function << " ]: ";
  if( f_PFtype == ModifyCnst )
   output << " constants";
  else
   output << " rows";
  output << " [ " << f_range.first << " , " << f_range.second << " )";
  if( f_PFtype == DeleteRows )
   output << " deleted";
  else
   output << " modified";
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 int f_PFtype;    ///< the exact PolyhedralFunction-specific operation

 Range f_range;   ///< the set of affected rows

/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PolyhedralFunctionModSbst --------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe subset modification specific to a PolyhedralFunction
/** Derived class from PolyhedralFunctionMod to describe all modifications to
 * a PolyhedralFunction that involve an arbitrary set of rows:
 *
 * - modify_row[s]
 * - modify_constant[s]
 * - delete_row[s]
 *
 * For all these, the Subset of the affected rows is provided, as well as
 * the exact type of operation. */

class PolyhedralFunctionModSbst : public PolyhedralFunctionMod
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- CONSTRUCTOR ---------------------------------*/
 /// constructor: like that of PolyhedralFunctionMod + the modified rows
 /** Constructor: takes a pointer to the affected C05Function, the type of the
  * C05FunctionMod, the type of the PolyhedralFunctionMod, the Subset of
  * concerned rows (which is *always* expected to be ordered in increasing
  * sense), the value of the shift, and the "concerns Block" value. As the &&
  * tells, the rows parameter becomes property of the
  * PolyhedralFunctionModRng. */

 explicit PolyhedralFunctionModSbst( PolyhedralFunction * f , int type ,
				     int pftype , Subset && rows ,
				     Subset && which = {} , 
				     FunctionValue shift = NaNshift ,
				     bool cB = true )
  : PolyhedralFunctionMod( f , type , std::move( which ) , shift , cB ) ,
    f_PFtype( pftype ) , v_rows( std::move( rows ) ) { }

/*------------------------------ DESTRUCTOR --------------------------------*/

 virtual ~PolyhedralFunctionModSbst() = default;  ///< default destructor

/*-------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// accessor to the specific sub-type of PolyhedralFunctionMod

 [[nodiscard]] int PFtype( void ) const { return( f_PFtype ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the subset of the modified rows

 [[nodiscard]] c_Subset & rows( void ) const { return( v_rows ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the PolyhedralFunctionModSbst

 void print( std::ostream &output ) const override {
  output << "PolyhedralFunctionModRng[";
  if( concerns_Block() )
   output << "t";
  else
   output << "f";
  output << "] on PolyhedralFunction [" << &f_function << " ]: "
	 << v_rows.size();
  if( f_PFtype == ModifyCnst )
   output << " constants";
   else
    output << " rows";
  if( f_PFtype == DeleteRows )
   output << " deleted";
  else
   output << " modified";
  output << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 int f_PFtype;    ///< the exact PolyhedralFunction-specific operation

 Subset v_rows;   ///< the set of affected rows

/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionModSbst ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PolyhedralFunctionState ----------------------*/
/*--------------------------------------------------------------------------*/
/// class to describe the "internal state" of a PoluhedralFunction
/** Derived class from State to describe the "internal state" of a
 * PoluhedralFunction, i.e., its global pool. This means which of the
 * original cuts are in the global pool, and the aggregated ones that are
 * also there. */

class PolyhedralFunctionState : public State
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------- CONSTRUCTING AND DESTRUCTING PolyhedralFunctionState -----------*/

 /// constructor, doing everything or nothing.
 /** Constructor of PolyhedralFunctionState. If provided with a pointer to a
  * PolyhedralFunction it immediately copies its "internal state", which is
  * the only way in which the PolyhedralFunctionState can be initialised out
  * of an existing PolyhedralFunction. If nullptr is passed (as by default),
  * then an "empty" PolyhedralFunctionState is constructed that can only be
  * filled by calling deserialize(). */

 PolyhedralFunctionState( const PolyhedralFunction * pf = nullptr )
  : State() {
  if( ! pf ) {
   f_nvar = 0;
   return;
   }
  f_nvar = pf->get_num_active_var();
  v_glob = pf->v_glob;
  v_aA = pf->v_aA;
  v_ab = pf->v_ab;
  f_imp_coeff = pf->f_imp_coeff;
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a PolyhedralFunctionState out of netCDF::NcGroup
 /** De-serialize a PolyhedralFunctionState out of netCDF::NcGroup; see
  * PolyhedralFunctionState::serialize() for a description of the format. */

 void deserialize( const netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 ///< destructor

 virtual ~PolyhedralFunctionState() { }

/*------ METHODS DESCRIBING THE BEHAVIOR OF A PolyhedralFunctionState ------*/

 /// serialize a PolyhedralFunctionState into a netCDF::NcGroup
 /** The method serializes the PolyhedralFunctionState into the provided
  * netCDF::NcGroup, so that it can later be read back by deserialize().
  *
  * After the call, \p will contain:
  *
  * - The dimension "PolyFunction_GlobSize" containing the size of the global
  *   pool.
  *
  * - The variable "PolyFunction_Glob", of type netCDF::NcInt and indexed over
  *   the dimension PolyFunction_GlobSize, which contains the vector v_glob
  *   representing the global pool of the PolyhedralFunction.
  *
  * - The dimension "PolyFunction_NumVar" containing the number of columns of
  *   the aA matrix, i.e., the number of active variables.
  *
  * - The dimension "PolyFunction_ANumRow" containing the number of rows of
  *   the aA (aggregated rows) matrix. The dimension is optional, if it is
  *   not provided than 0 (no rows) is assumed. This imples that the global
  *   pool does not contain any aggregated linearization, i.e., there are no
  *   negative elements of Glob.
  *
  * - The variable "PolyFunction_aA", of type netCDF::NcDouble and indexed
  *   over both the dimensions PolyFunction_ANumRow and PolyFunction_NumVar
  *   (in this order); it contains the (row-major) representation of the
  *   matrix aA. The variable is only optional if PolyFunction_NumRow == 0.
  *
  * - The variable "PolyFunction_ab", of type netCDF::NcDouble and indexed
  *   over the dimension PolyFunction_NumRow, which contains the vector ab
  *   (RHS of the aggregated rows. The variable is only optional if
  *   PolyFunction_NumRow == 0.
  *
  * - The dimension "PolyFunction_ImpCoeffNum" containing the number of
  *   elements of the important coefficients. The dimension is optional, if
  *   it is not provided than 0 (no important coefficients) is assumed.
  *
  * - The variable "PolyFunction_ImpCoeffInd", of type netCDF::NcInt and
  *   indexed over the dimension PolyFunction_ImpCoeffNum, which contains the
  *   vector of indices of the important coefficients. The variable is
  *   optional if PolyFunction_ImpCoeffNum == 0.
  *
  * - The variable "PolyFunction_ImpCoeffVal", of type netCDF::NcDouble and
  *   indexed over the dimension PolyFunction_ImpCoeffNum, which contains the
  *   vector of real values of the important coefficients. The variable is
  *   optional if PolyFunction_ImpCoeffNum == 0. */

 void serialize( netCDF::NcGroup & group ) const override;

/*-------------------------------- FRIENDS ---------------------------------*/

 friend class PolyhedralFunction;  // make PolihedralFunction friend

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 void print( std::ostream &output ) const override {
  output << "PolyhedralFunctionState [" << this << "] with " << f_nvar
	 << " variables, global pool size " << v_glob.size() << " and "
	 << v_aA.size() << " aggregated rows";
  }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 PolyhedralFunction::Index f_nvar;
 ///< number of variables in the PolyhedralFunction

 std::vector< int > v_glob;  ///< the global pool, 
 
 PolyhedralFunction::MultiVector v_aA;
 ///< the A matrix for aggregated linearizations
 
 PolyhedralFunction::RealVector v_ab;
 ///< the b vector for aggregated linearizations

 PolyhedralFunction::LinearCombination f_imp_coeff;
 ///< coefficients of the important lineariration

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunctionState ) )

/** @} end( group( PoluhedralFunction_CLASSES ) ) --------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* PoluhedralFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File PoluhedralFunction.h ----------------------*/
/*--------------------------------------------------------------------------*/

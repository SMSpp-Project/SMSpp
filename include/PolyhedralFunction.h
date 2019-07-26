/*--------------------------------------------------------------------------*/
/*----------------------- File PolyhedralFunction.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the PolyhedralFunction class, which is a convex (or
 * concave) C05Function defined by the maximum (or minimum) of a "small"
 * number of explicitly provided affine forms.
 *
 * \version 0.10
 *
 * \date 15 - 07 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
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

/*--------------------------------------------------------------------------*/
/*--------------------- PolyhedralFunction-RELATED TYPES -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup PolyhedralFunction_TYPES PolyhedralFunction-related types.
 *  @{ */

/** @}  end( group( PolyhedralFunction_TYPES ) ) */
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
 * vector b (with m given and small), so that
 *
 *   f(x) =  max \{ A_i x + b_i , i = 0, ..., m - 1 \}
 *
 * (in which case it is convex, exchange "max" with "min" to make it concave).
 * The function is therefore finite-valued everywhere, and each of the pairs
 * ( A_i , b_i ) define one of the possible diagonal linearizations; thus far
 * vertical linearizations are not handled, but adding them would not be too
 * much of an issue. The only exception is when m is 0, in which case the
 * function evaluates to - \infinity (in the convex case, + \infinity on the
 * concave one).
 *
 * When the function is evaluated, all m linearizations enter the local pool
 * in order of their value v_i = A_i x + b_i (non-increasing in the convex
 * case, non-decreasing in the concave one), and are reported in that order.
 * The global pool is just a subset of the fixed index set 0, ..., m - 1.
 *
 * PolyhedralFunction handles all possible changes in its input data:
 *
 * - addition of variables (adding columns to A);
 *
 * - removal of variables (removing columns from A);
 *
 * - addition of cutting planes (adding rows to A);
 *
 * - removal of cutting planes (removing rows to A);
 *
 * - changes in any subset of elements of b. */

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

 typedef std::vector< std::vector < FunctionValue > > MultiVector;
 ///< representing the A matrix: a vector of m elements, each a real n-vector

 typedef std::vector< ColVariable * > VarVector;
 ///< representing the x variables upon which the function depends

 typedef const VarVector c_VarVector;
 ///< a const version of the x variables upon which the function depends

/*--------------------------------------------------------------------------*/
 /// virtualized concrete iterator
 /** A concrete class deriving from ThinVarDepInterface::v_iterator and
  * implementing the concrete iterator for sifting through the "active"
  * Variable of a PolyhedralFunction. */

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
    auto tmp = static_cast<const PolyhedralFunction::v_iterator *>( & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const PolyhedralFunction::v_iterator *>( & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const PolyhedralFunction::v_iterator *>( & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const PolyhedralFunction::v_iterator *>( & rhs );
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
    auto tmp = static_cast<const PolyhedralFunction::v_const_iterator *>(
								      & rhs );
    return( itr_ == tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const PolyhedralFunction::v_const_iterator *>(
								      & rhs );
    return( tmp ? itr_ == tmp->itr_ : false );
   #endif
   }
  virtual bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   #ifdef NDEBUG
    auto tmp = static_cast<const PolyhedralFunction::v_const_iterator *>(
								      & rhs );
    return( itr_ != tmp->itr_ );
   #else
    auto tmp = dynamic_cast<const PolyhedralFunction::v_const_iterator *>(
								      & rhs );
    return( tmp ? itr_ != tmp->itr_ : true );
   #endif
   }

  private:

  VarVector::const_iterator itr_;
  };

/**@} ----------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING PolyhedralFunction -------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing PolyhedralFunction
 *  @{ */

 /// constructor of PolyhedralFunction, possibly inputting the data
 /** Constructor of PolyhedralFunction. Inputs:
  *
  * @param VarVector && x, a n-vector of pointers to ColVariable representing
  *        the x variable vector in the definition of the function; x *must*
  *        already be ordered by increasing pointer = ColVariable "name";
  *
  * @param the MultiVector && A, a m-vector of n-vectors of FunctionValue
  *        representing the A matrix in the definition of the function;
  *        entry A[ i ][ j ] is (obviously) meant to be the coefficient
  *        of variable *x[ j ] for the i-th row;
  *
  * @param the std::vector<FunctionValue> && b, a m-vector of FunctionValue
  *        representing the b vector in the definition of the function
  *        (that is, b[ i ] is the constant factor of the i-th linear form);
  *
  * @param is_convex, a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization and therefore it is a concave function;
  *
  * @param observer, a pointer to the Observer of this PolyhedralFunction.
  *
  * As the && implies, x, A, and b become property of the PolyhedralFunction
  * object.
  *
  * All inputs have a default ({}, {}, {}, true, and nullptr, respectively)
  * so that this can be used as the void constructor. */

 PolyhedralFunction( VarVector && x = {} , MultiVector && A = {} ,
		     std::vector<FunctionValue> && b = {} ,
		     const bool is_convex = true ,
		     Observer * const observer = nullptr)
  : C05Function( observer ) , f_is_convex( is_convex ) ,
    f_loc_pool_sz( 1 ) , f_next( 0 ) , f_imp( 0 )
 {
  guts_of_constructor_Ab( std::move( A ) , std::move( b ) );
  if( ! v_A.empty() )
   if( x.size() != v_A[ 0 ].size() )
    throw( std::invalid_argument( "A and x must have the same columns" ) );

  v_x = std::move( x );
  v_ord.resize( 1 );
  }

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty

 virtual ~PolyhedralFunction() { }

/*--------------------------------------------------------------------------*/
 /// clear method: clears the v_x field
 /** Method to "clear" the PolyhedralFunction: it clear() the vector v_x. This
  * destroys the list of "active" Variable without unregistering from them.
  * Not that the PolyhedralFunction would have to, but an Observer using it
  * to "implement iyself" should. By not having any Variable, the Observer
  * can no longer do that. */

 virtual void clear( void ) override { v_x.clear(); }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. PolyhedralFunction takes
  * care of intLPMaxSz and intGPMaxSz, leaving all the rest to Function. */

 virtual void set_par( const idx_type par , const int value ) override
 {
  switch( par ) {
   case( intLPMaxSz ):
    if( value < 1 )
     throw( std::invalid_argument( "intLPMaxSz must be positive" ) );
    if( value != f_loc_pool_sz ) {
     if( value == 1 )
      v_ord.resize( 1 );
     else {
      v_ord.resize( v_A.size() );
      std::iota( v_ord.begin() , v_ord.end() , 0 );
      }
     f_loc_pool_sz = value;
     }
    break;
   case( intGPMaxSz ):
    if( value < 0 )
     throw( std::invalid_argument( "intGPMaxSz must be non-negative" ) );
    v_glob.resize( value , Inf<Index>() );
    break;
   default: Function::set_par( par , value );
   }
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. PolyhedralFunction
  * ignores the C05Function-specific dblRAccLin and dblAAccLin, and leaves
  * all the rest to Function. */

 virtual void set_par( const idx_type par , const double value ) override
 {
  switch( par ) {
   case( dblRAccLin ):
   case( dblAAccLin ):
    break;
   default: Function::set_par( par , value );
   }
  }

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF A PolyhedralFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a PolyhedralFunction
 * The methods in this section allow to compute the current value of the
 * PolyhedralFunction and retrieve first-order information about the point
 * where the C05Function have been last evaluated.
 * @{ */

 /// compute the PolyhedralFunction

 virtual int compute( bool changedvars = true ) override final;

/*--------------------------------------------------------------------------*/
 /// returns the value of the PolyhedralFunction

 virtual FunctionValue get_value( void ) const override final {
  return( f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the PolyhedralFunction is exact, hence lower_estimate == value
 
 virtual FunctionValue get_lower_estimate( void ) const override final {
  return( f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// the PolyhedralFunction is exact, hence upper_estimate == value

 virtual FunctionValue get_upper_estimate( void ) const override final {
  return( f_value );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (global) Lipschitz constant for the PolyhedralFunction

 virtual FunctionValue get_Lipschitz_constant( void ) override final
 {
  if( f_Lipschitz_constant < 0 )
   compute_Lipschitz_constant();
  return( f_Lipschitz_constant );
  }

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this PolyhedralFunction is convex

 virtual bool is_convex( void ) const override final {
  return( f_is_convex );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true if and only if this PolyhedralFunction is concave

 virtual bool is_concave( void ) const override final {
  return( ! f_is_convex );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns true only if this PolyhedralFunction is linear
 /** Method that returns true if and only if this PolyhedralFunction is
  * linear. It therefore returns false. Actually, in the case where the
  * number of rows in A is exactly one the PolyhedralFunction is linear,
  * but this is supposed to be an accident that may and probably will change
  * at any time, while this kind of method should return a "stable" result. */

 virtual bool is_linear( void ) const override final { return( false ); }

/*--------------------------------------------------------------------------*/
 /// tells whether a linearization is available

 virtual bool has_linearization( const bool diagonal = true ) override final
 {
  return( diagonal ? ( ! v_A.empty() ) : false );
  }

/*--------------------------------------------------------------------------*/
 /// compute a new linearization for this PolyhedralFunction

 virtual bool compute_new_linearization( const bool diagonal = true )
  override final {
  if( ( ! diagonal ) || v_A.empty() || ( f_next >= v_ord.size() - 1 ) ||
      ( f_next >= f_loc_pool_sz - 1 ) )
   return( false );

  ++f_next;
  return( true );
  }

/*--------------------------------------------------------------------------*/
 /// store a linearization in the global pool 

 virtual void store_linearization( const LinearizationName name )
  override final {
  if( name >= v_glob.size() )
   throw( std::invalid_argument( "invalid global pool name" ) );

  v_glob[ name ] = v_ord[ f_next ];
  }

/*--------------------------------------------------------------------------*/
 /// stores a combination of the given linearizations

 virtual void store_combination_of_linearizations(
	  LinearCombination & coefficients , const LinearizationName name )
  override final;

/*--------------------------------------------------------------------------*/
 /// specify which linearization is "the important one"

 virtual void set_important_linearization( LinearCombination && coefficients ,
					   LinearizationName name )
  override final {
  if( name >= v_glob.size() )
   throw( std::invalid_argument( "invalid global pool name" ) );

  f_imp = name;
  f_imp_coeff = std::move( coefficients );
  }

/*--------------------------------------------------------------------------*/
 /// return the name of "the important linearization"

 virtual LinearizationName get_important_linearization_name( void )
  override final { return( f_imp ); }

/*--------------------------------------------------------------------------*/
 /// return the combination used to form "the important linearization"

 virtual c_LinearCombination &
  get_important_linearization_coefficients( void ) override final {
  return( f_imp_coeff );
  }

/*-------------------------------------------------------------------------*/
 /// rename a linearization that is stored in the global pool

 virtual void rename_linearization( const LinearizationName current_name ,
				    const LinearizationName new_name )
  override final;

/*--------------------------------------------------------------------------*/
 /// delete the given linearization from the global pool of linearizations

 virtual void delete_linearization( const LinearizationName name )
  override final;

/*--------------------------------------------------------------------------*/
 /// retrieve the coefficients (g vector) of a linearization in a vector

 virtual void get_linearization_coefficients( FunctionValue *g ,
                    const LinearizationName name = Inf<LinearizationName>() ,
		    c_Vec_Index & indices = {} , c_Index start = 0 ,
		    Index end = Inf<Index>() ) override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// retrieve the coefficients (g) of a linearization in a sparse vector

 virtual void get_linearization_coefficients( SparseVector &g ,
                     const LinearizationName name = Inf<LinearizationName>() ,
	             c_Vec_Index & indices = {} , c_Index start = 0 ,
	             Index end = Inf<Index>() ) override final;

/*--------------------------------------------------------------------------*/
 /// return the constant term of a linearization

 virtual FunctionValue get_linearization_constant(
     const LinearizationName name = Inf<LinearizationName>() ) override final
 {
  if( name >= v_glob.size() )
   return( v_b[ v_ord[ f_next ] ] );

  if( v_glob[ name ] == Inf<Index>() )
   // there is no item with such a name, which may mean that it was there
   // once but it has been deleted: the linearization is invalid
   return( Inf<FunctionValue>() );

  if( v_glob[ name ] < v_A.size() )
   return( v_b[ v_glob[ name ] ] );
  else
   return( v_ab[ v_glob[ name ] - v_A.size() ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current A matrix in the mapping

 const MultiVector & get_A( void ) const {
  return( v_A );
  }

/*--------------------------------------------------------------------------*/
 /// returns a (const reference) to the current b vector in the mapping

 const std::vector<FunctionValue> & get_b( void ) const {
  return( v_b );
  }

/**@} ----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the PolyhedralFunction
 *  @{ */

 /// get a specific integer (int) numerical parameter
 /** Get a specific integer (int) numerical parameter. PolyhedralFunction
  * takes care of intLPMaxSz and intGPMaxSz, leaving all the rest to
  * Function. */

 virtual int get_int_par( const idx_type par ) const override final {
  switch( par ) {
   case( intLPMaxSz ): return( f_loc_pool_sz );
   case( intGPMaxSz ): return( v_glob.size() );
   }
  return( Function::get_int_par( par ) );
  }

/**@} ----------------------------------------------------------------------*/
/*---- METHODS FOR HANDLING "ACTIVE" Variable IN THE PolyhedralFunction ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * PolyhedralFunction; this is the actual concrete implementation exploiting
 * the vector v_x of pointers.
 * @{ */

 virtual Index get_num_active_var( void ) const override final {
  return( v_x.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Index is_active( const Variable * const var )
  const override final {
  auto idx = std::lower_bound( v_x.begin() , v_x.end() , var );
  if( idx < v_x.end() )
   return( std::distance( v_x.begin() , idx ) );
  else
   return( Inf<Index>() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual void map_active( c_Vec_p_Var & vars , Vec_Index & map ,
			  const bool ordered = false ) const override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual Variable *get_active_var( const Index i ) const override final {
  return( v_x[ i ] );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_begin( void ) override final
 {
  return( new PolyhedralFunction::v_iterator( v_x.begin() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_begin( void ) const override final
 {
  return( new PolyhedralFunction::v_const_iterator( v_x.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 virtual v_iterator * v_end( void ) override final
 {
  return( new PolyhedralFunction::v_iterator( v_x.end() ) );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual v_const_iterator * v_end( void ) const override final
 {
  return( new PolyhedralFunction::v_const_iterator( v_x.end() ) );
  }

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR MODIFYING THE PolyhedralFunction ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the PolyhedralFunction
 *  @{ */

 /// completely resets the PolyhedralFunction with entirely new data
 /** Completely resets the PolyhedralFunction with entirely new data,
  * but leaving the current set of n = get_num_active_var() input Variable:
  *
  * @param the MultiVector && A, a m-vector of n-vectors of FunctionValue
  *        representing the A matrix in the definition of the function;
  *        entry A[ i ][ j ] is (obviously) meant to be the coefficient
  *        of variable *x[ j ] for the i-th row;
  *
  * @param the std::vector<FunctionValue> && b, a m-vector of FunctionValue
  *        representing the b vector in the definition of the function
  *        (that is, b[ i ] is the constant factor of the i-th linear form);
  *
  * @param is_convex, a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization and therefore it is a concave function.
  *
  * @param issueMod, which decides if and how the FunctionMod (with f_shift
  *        == FunctionMod::NaNshift, i.e., "everything changed") is issued,
  *        as described in Observer::make_par().
  *
  * This completely resets the PolyhedralFunction, save that it remains
  * defined on the very same variable space, which means that n (the number
  * of columns in A) must be equal to get_num_active_var().
  *
  * As the && implies, A and b become property of the PolyhedralFunction
  * object. */
 
 void set_PolyhedralFunction( MultiVector && A = {} ,
			      std::vector<FunctionValue> && b = {} ,
			      const bool is_convex = true ,
			      c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// change the "sign" of the PolyhedralFunction
 /**< The method allows to [re]set the parameter governing the "sign" of the
  * PolyhedralFunction:
  *
  * @param is_convex, a boolean indicating whether the function has to be
  *        defined as the maximization of the provided linear (affine)
  *        functions, and therefore is a convex function, or as the
  *        minimization and therefore it is a concave function.
  *
  * @param issueMod, which decides if and how the C05FunctionMod is issued,
  *        as described in Observer::make_par().
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
  * C05FunctionMod::NothingChanged for the f_type of the C05FunctionMod. */

 void set_is_convex( const bool is_convex = true ,
		     c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add a set of new Variable to the PolyhedralFunction
 /**< The method receives:
  *
  * @param nx, a std::vector< Variable * > && containing the pointers to k
  *        new ColVarable; nx *must* be ordered in increasing sense, and all
  *        the ColVariable in there must *not* be already among the active
  *        Variable of the PolyhedralFunction.
  *
  * @param nA, a MultiVector && having as many rows as the current A matrix
  *        and exactly k columns representing the new part of the linear
  *        mapping; entry nA[ i ][ h ] is (obviously) meant to be the
  *        coefficient of *nx[ h ] for the i-th row.
  *
  * @param issueMod, which decides if and how the C05FunctionModVars (with
  *        f_shift == 0, since a PolyhedralFunction is strongly quasi-additive)
  *        is issued, as described in Observer::make_par().
  *
  * Note that nx is a std::vector< Variable * > rather than a
  * std::vector< ColVariable * >, although of course all the pointers have
  * to be to a ColVariable. This is because the vector can then be passed
  * right away to the C05FunctionModVars, that expects one; indeed, as the
  * && tells, the nx (and nA) become "property" of the PolyhedralFunction
  * object. For nA this likely only happens if A is currently "empty of
  * columns" (say, only the rows have been defined, or all previous columns
  * have been deleted); nx, however, can be dispatched to the Modification.
  * The point is that since C05FunctionModVars is defined in C05Function, it is
  * not restricted to the case where Variable is a ColVariable, although it
  * should be "like" one (a Variable representing a single real value) for
  * the current form of linearizations to work. Although a
  * std::vector< ColVariable * > and a std::vector< Variable * > should be
  * physically indistinguishable, there is no sound way to cheaply pass the
  * former as the latter in C++; having the input as a
  * std::vector< Variable * > circumvents the problems (and each pointer
  * could be static_cast-ed to a ColVariable * as soon as it is confirmed
  * that the Variable is active in the LinearFunction, should this be
  * necessary). */

 void add_variables( Vec_p_Var && nx , MultiVector && nA ,
		     c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new Variable to the PolyhedralFunction
 /** Like add_variables(), but just only one Variable:
  *
  * @param var is a ColVariable *, and the pointed ColVariable must *not* be
  *        already among the active Variable of the PolyhedralFunction.
  *
  * @param Aj is a std::vector<FunctionValue> containing having as many rows
  *        as the current A matrix and containing the new column of the linear
  *        mapping; entry Aj[ i ][ h ] is (obviously) meant to be the
  *        coefficient of *vaf for the i-th row.
  *
  * @param issueMod, which decides if and how the C05FunctionModVars (with
  *        f_shift == 0, since a PolyhedralFunction is strongly quasi-additive)
  *        is issued, as described in Observer::make_par(). */

 void add_variable( ColVariable * const var ,
		    const std::vector<FunctionValue> & Aj ,
		    c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// modify one single row of the linear mapping
 /** Modifies one row of the linear mapping:
  *
  * @param i is the index of the row to be modified;
  *
  * @param Ai is the new std::vector<FunctionValue>, with exactly n =
  *        get_num_active_var() elements, to replace the existing vector of
  *        coefficients in the i-th linear mapping; as the && tells, Ai
  *        becomes "property" of the PolyhedralFunction object and physically
  *        replaces the previous vector;
  *
  * @param bi is the new constant term of the i-th mapping;
  *
  * @param issueMod, which decides if and how the C05FunctionMod, is issued,
  *        as described in Observer::make_par(). Note that the value of the
  *        function has changed "unpredictably" (hence, the shift is NANshift)
  *        and "all the linearizations may have changed" (the Modification type
  *        is "AllLinearizationChanged"), although actually only one of them
  *        has. */  

 void modify_row( c_Index i , std::vector<FunctionValue> && Ai ,
		  c_FunctionValue bi , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// modify only the constant term of one row of the linear mapping
 /** Like modify_row(), but modify the constant term only for one row of the
  * linear mapping:
  *
  * @param i is the index of the row to be modified;
  *
  * @param bi is the new constant term of the i-th mapping;
  *
  * @param issueMod, which decides if and how the C05FunctionMod, is issued,
  *        as described in Observer::make_par(). Note that the value of the
  *        function has changed "unpredictably" (hence, the shift is NANshift)
  *        and "all the alphas may have changed" (the Modification type
  *        is "AlphaChanged"), although actually only one of them has. */  

 void modify_constant( c_Index i , c_FunctionValue bi ,
		       c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// add some rows to the linear mapping in the PolyhedralFunction
 /**< Adds some rows to the linear mapping in the PolyhedralFunction, leaving
  * the current set of n = get_num_active_var() input Variable and all the
  * current rows:
  *
  * @param the MultiVector && nA, a k-vector of n-vectors of FunctionValue
  *        representing the new rows of the A matrix in the definition of the
  *        function; entry nA[ i ][ j ] is (obviously) meant to be the
  *        coefficient of variable *x[ j ] for the i-th new row; as the &&
  *        tells, the object (most likely, its individual rows) becomes
  *         "property" of the PolyhedralFunction.
  *
  * @param the std::vector<FunctionValue> & b, a k-vector of FunctionValue
  *        representing the new entries of b vector in the definition of the
  *        function (that is, b[ i ] is the constant factor of the new i-th
  *        linear form);
  *
  * @param issueMod, which decides if and how the C05FunctionMod is issued,
  *        as described in Observer::make_par().
  *
  * Note that adding new rows makes a "max" (convex) function to increase in
  * value and a "min" (concave) one to decrease in value, but all existing
  * linearization are still valid ones, which is the poster case for the
  * weird-ish setting C05FunctionMod::NothingChanged for the f_type of the
  * C05FunctionMod. */

 void add_rows( MultiVector && nA , std::vector<FunctionValue> & nb ,
		c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add one single new row to the linear mapping
 /** Like add_row(), but just only one row of the linear mapping:
  *
  * @param Ai is the std::vector<FunctionValue>, with exactly n =
  *        get_num_active_var() elements, with the coefficients of the new
  *        row in the mapping; as the && tells, Ai becomes "property" of the
  *        PolyhedralFunction object;
  *
  * @param bi is the constant term of the new row in the mapping;
  *
  * @param issueMod, which decides if and how the C05FunctionMod is issued,
  *        as described in Observer::make_par().
  *
  * Note that adding a new row makes a "max" (convex) function to increase in
  * value and a "min" (concave) one to decrease in value, but all existing
  * linearization are still valid ones, which is the poster case for the
  * weird-ish setting C05FunctionMod::NothingChanged for the f_type of the
  * C05FunctionMod. */

 void add_row( std::vector<FunctionValue> && Ai , c_FunctionValue bi ,
	       c_ModParam issueMod = eModBlck );
 
/*--------------------------------------------------------------------------*/
 /// deletes some rows from the linear mapping in the PolyhedralFunction
 /**< Deletes some rows from the linear mapping in the PolyhedralFunction,
  * leaving the current set of n = get_num_active_var() input Variable and
  * all rows that are not explicitly deleted:
  *
  * @param  std::vector<Index> & rows contans the indices of the rows to be
  *         deleted; all entries must therefore be numbers in 0, ...,
  *         get_A().size() - 1, *unique* and *ordered in increasing sense*;
  *
  * @param issueMod, which decides if and how the C05FunctionMod is issued,
  *        as described in Observer::make_par().
  *
  * Note that removing rows makes a "max" (convex) function to decrease in
  * value and a "min" (concave) one to increase in value; also, existing
  * lnearizations in the global pool may disappear. Even worse, and aggregated
  * linearization may have been constructed out of the ones that are deleted,
  * and there is no way of saying it in general. Hence, the C05FunctionMod
  * will have f_type = C05FunctionMod::AlphaChanged if:
  *
  * - any of the deleted rows are present in the global pool;
  *
  * - any aggregated linearization is present in the global pool.
  *
  * Otherwise no linearization is affected, and the C05FunctionMod will have
  * f_type = C05FunctionMod::NothingChanged. The linearizations that are not
  * deleted (both those in the list and the aggregated ones) remain identical
  * (the constant term does not change, even less the vector of coefficients),
  * the others get constant == Inf<FunctionValue>(), and therefore the vector
  * of coefficients is no longer significant. */

 void delete_rows( std::vector<Index> & rows ,
		   c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes one single existing row from the linear mapping
 /** Like delete_rows(), but just only the i-th row of the linear mapping.
  */

 void delete_row( c_Index i , c_ModParam issueMod = eModBlck );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// deletes all rows from the linear mapping in the PolyhedralFunction
 /**< Like delete_rows( some ), but immediately removes *all* the matrix A
  * and vector b, leaving the mapping "empty". This of course also resets 
  * alla aggregated linearizations, which can no longer be valid. Hence, the
  * best Modification to issue is a FunctionMod with f_shift ==
  * FunctionMod::NaNshift, i.e., "everything changed". */

 void delete_rows( c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove the given Variable from the PolyhedralFunction
 /** Removes the given Variable from the LinearFunction, thereby eliminating
  * the corresponding column of the matrix A:
  *
  * @param var is a ColVariable *, and the pointed ColVariable must be
  *        already among the active Variable of the PolyhedralFunction.
  *
  * @param issueMod, which decides if and how the C05FunctionModVars (with
  *        f_shift == 0, since a PolyhedralFunction is strongly quasi-additive)
  *        is issued, as described in Observer::make_par(). */

 virtual void remove_variable( Variable * var ,
			       c_ModParam issueMod = eModBlck )
  override final;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove the i-th Variable
 /** Like remove_variable( Variable * ), but takes in input the index of
  * the Variable to be removed rather than its pointer. Useful if one knows
  * the index already, so that it need not be searched for.
  *
  * @param i is the index of the Variable to be removed, an integer between 0
  *        and get_num_active_var().
  *
  * @param issueMod, which decides if and how the C05FunctionModVars (with
  *        f_shift == 0, since a PolyhedralFunction is strongly quasi-additive)
  *        is issued, as described in Observer::make_par(). */

 void remove_variable( c_Index i , c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable that are in position from start (included) to
  * min( stop , get_num_active_var() ) (excluded) in this PolyhedralFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a linear function
  * is additive, and therefore strongly quasi-additive. */

 void remove_variables( c_Index strt = 0 , Index stop = Inf<Index>() ,
			c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove all the Variable comprised between strt (included) and stop
  * (excluded). Setting strt == nullptr means "the first Variable", and
  * setting stop == nullptr means "(one after) the last Variable". If
  * no-nullptr arguments are provided, they *must* be "names" of Variable
  * currently active in this LinearFunction.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a polyhedral
  * function is strongly quasi-additive. */

 void remove_variables( const Variable * const strt = nullptr ,
			const Variable * const stop = nullptr ,
			c_ModParam issueMod = eModBlck )
 {
  c_Index istrt = strt ? is_active( strt ) : 0;
  if( istrt >= get_num_active_var() )
   throw( std::invalid_argument( "strt is not an active Variable" ) );

  Index istop;
  if( stop ) {
   istop = is_active( stop );
   if( istrt >= get_num_active_var() )
    throw( std::invalid_argument( "stop is not an active Variable" ) );
   }
  else
   istop = get_num_active_var();

  remove_variables( istrt , istop , issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// remove the given set of Variable
 /** Remove all the Variable in the given vector of pointers vars. If any
  * Variable in vars is not an active Variable in the LinearFunction,
  * exception is thrown. The parameter ordered tells if vars is already
  * ordered by Variable "name = pointer" or not, otherwise it gets ordered
  * inside the method (which is why it is not const).
  *
  * Note that vars is a std::vector< Variable * > rather than a
  * std::vector< ColVariable * >, although of course all the pointers have
  * to be to a ColVariable. This is because the vector can then be passed
  * right away to map_active() to produce a set of indices that can then be
  * used to call remove_variables( Vec_Index & nms ) which actually
  * implements the operation.
  *
  * The parameter issueMod decides if and how the C05FunctionModVars is
  * issued, as described in Observer::make_par(). Note that a polyhedral
  * function is strongly quasi-additive. */

 virtual void remove_variables( Vec_p_Var && vars ,
				const bool ordered = false ,
				c_ModParam issueMod = eModBlck )
  override final {
  if( vars.empty() )  // actually nothing to remove
   return;            // cowardly (and silently) return

  if( v_x.empty() )  // deleting from nothing
   throw( std::logic_error( "deleting from an empty set" ) );

  if( ! ordered )
   std::sort( vars.begin() , vars.end() );

  Vec_Index map;
  map_active( vars , map , true );

  remove_variables( map , true , issueMod );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove a set of Variable by index
 /** Like remove_variables( Vec_p_Var * ) (which is implemented via a call to
  * this), but takes in input a set of index of the Variable to be removed
  * rather than their pointers. Useful if one knows the indices already, so
  * that they need not be searched for.
  *
  * @param nms is Vec_Index & containing the indices of the Variable to be
  *        removed, i.e., integers between 0 and get_num_active_var()
  *
  * @param ordered is a bool indicating if nms[] is already ordered in
  *        increasing sense (otherwise this is done inside the method,
  *        which is why nms[] is not const) 
  *
  * @param issueMod, which decides if and how the C05FunctionModVars (with
  *        f_shift == 0, since a PolyhedralFunction is strongly quasi-additive)
  *        is issued, as described in Observer::make_par(). */

 virtual void remove_variables( Vec_Index & nms , const bool ordered = false ,
				c_ModParam issueMod = eModBlck );

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the C05Function on an ostream
 /** Protected method intended to print information about the C05Function; it
  * is virtual so that derived classes can print their specific information
  * in the format they choose. */

 virtual void print( std::ostream &output ) const override {
  output << "C05Function [" << this << "]"
	 << " with " << get_num_active_var() << " active variables";
  }

/*--------------------------------------------------------------------------*/

 void guts_of_constructor_Ab( MultiVector && A ,
			      std::vector<FunctionValue> && b )
 {
  if( A.size() != b.size() )
   throw( std::invalid_argument( "A and b must have the same rows" ) );
  if( ! A.empty() ) {
   const Index n = A[ 0 ].size();
   for( auto & a : A )
    if( a.size() != n )
     throw( std::invalid_argument( "all rows A must have the same size" ) );
   }
  v_A = std::move( A );
  v_b = std::move( b );
  f_value = - Inf<FunctionValue>();
  f_Lipschitz_constant = - Inf<FunctionValue>();
  }

/*--------------------------------------------------------------------------*/

 void compute_Lipschitz_constant( void )
 {
  f_Lipschitz_constant = 0;
  for( const auto & Ai : v_A ) {
   FunctionValue L = 0;
   for( const auto aij : Ai )
    L += aij * aij;

   if( L > f_Lipschitz_constant )
    f_Lipschitz_constant = L;
   }

  f_Lipschitz_constant = sqrt( double( f_Lipschitz_constant ) );
  }

/*--------------------------------------------------------------------------*/

 std::vector<FunctionValue> & get_ai( c_Index name )
 {
  if( name >= v_glob.size() )
   return( v_A[ v_ord[ f_next ] ] );
  else {
   auto pos = v_glob[ name ];
   if( pos < v_A.size() )
    return( v_A[ pos ] );
   else
    return( v_aA[ pos - v_A.size() ] );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 bool f_is_convex;    ///< true if the function is a "max" = convex one

 MultiVector v_A;     ///< the A matrix of A x + b
 
 std::vector<FunctionValue> v_b;  ///< the b vector of A x + b

 MultiVector v_aA;     ///< the A matrix for aggregated linearizations
 
 std::vector<FunctionValue> v_ab;  ///< the b vector for aggregated linear.

 VarVector v_x;       ///< the pointer to the variables x in A x + b

 FunctionValue f_value;   ///< the value of the function

 FunctionValue f_Lipschitz_constant;  ///< the Lipschitz constant

 Index f_loc_pool_sz;       ///< size of the local pool
 Index f_next;              ///< next linearization in the local pool
 
 std::vector<Index> v_ord;  ///< the ordering of linearizations

 std::vector<Index> v_glob;  ///< the global pool
                             /**< h = v_glob[ i ] contains the place where the
			      * i-th item of the global pool is stored; if
			      * h < v_A.size() then it's an original
			      * linearization and it's found in v_A[ h ] and
			      * v_b[ h ], otherwise is an aggregated one and
			      * it's found in v_aA[ k ] and v_ab[ k ] for
			      * k = h - v_A.size(). If h = Inf<Index>() there
			      * is no item with this name. */

 Index f_imp;                ///< the important linearization

 LinearCombination f_imp_coeff;  ///< coefficients of the important linear.
 
/*--------------------------------------------------------------------------*/

 };  // end( class( PolyhedralFunction ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C05Function.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File C05Function.h --------------------------*/
/*--------------------------------------------------------------------------*/

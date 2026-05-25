/*--------------------------------------------------------------------------*/
/*--------------------- File PolyhedralFunction.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the PolyhedralFunction class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------- MACROS -----------------------------------*/
/*--------------------------------------------------------------------------*/

#define EXPLICIT_BOUND 0

/* if EXPLICIT_BOUND != 0, the PolyhedralFunction will explicitly produce an
 * all-0 flat subgradient each time that compute() hits the lower/upper
 * bound; this should not be necessary since the bound is known and therefore
 * that information is implicitly available anyway, but some Solver may not be
 * smart enough to use it properly. */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"

#include "Observer.h"

#include "PolyhedralFunction.h"

#include "PolyhedralFunctionBlock.h"

#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- TYPES ----------------------------------*/
/*--------------------------------------------------------------------------*/

using vecsize = std::vector< ColVariable * >::size_type;
// to avoid pesky warnings about Eigen size being different from std::vector

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register PolyhedralFunctionState to the State factory

SMSpp_insert_in_factory_cpp_0( PolyhedralFunctionState );

/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING PolyhedralFunction -------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::deserialize( const netCDF::NcGroup & group ,
				      ModParam issueMod  )
{
 auto nv = group.getDim( "PolyFunction_NumVar" );
 if( nv.isNull() )
  throw( std::logic_error( "PolyFunction_NumVar dimension is required" ) );

 Index nvar = nv.getSize();
 if( auto av = get_num_active_var() )  // if there are active variable
  if( av != nvar )                     // they must agree with the data
   throw( std::invalid_argument(
    "A col size in netCDF::NcGroup does not match with active variable" ) );

 MultiVector tA;
 RealVector tb;

 auto nr = group.getDim( "PolyFunction_NumRow" );
 if( ( ! nr.isNull() ) && ( nr.getSize() ) ) {
   auto ncdA = group.getVar( "PolyFunction_A" );
   if( ncdA.isNull() )
    throw( std::logic_error( "PolyFunction_A not found" ) );

   auto ncdb = group.getVar( "PolyFunction_b" );
   if( ncdb.isNull() )
    throw( std::logic_error( "PolyFunction_b not found" ) );

  tA.resize( nr.getSize() );
  for( Index i = 0 ; i < tA.size() ; ++i ) {
   tA[ i ].resize( nvar );
   ncdA.getVar( { i , 0 } , { 1 , nvar } , tA[ i ].data() );
   }

  tb.resize( nr.getSize() );
  ncdb.getVar( tb.data() );
  }

 bool cnvx = true;
 auto sgn = group.getDim( "PolyFunction_sign" );
 if( ! sgn.isNull() )
  cnvx = sgn.getSize() > 0 ? true : false;

 FunctionValue bound;
 auto nclb = group.getVar( "PolyFunction_lb" );
 if( nclb.isNull() )
  bound = cnvx ? -Inf< FunctionValue >() : Inf< FunctionValue >();
 else
  nclb.getVar( & bound );

 // the netCDF format does not (yet) carry the per-row "is vertical" flag,
 // so we deserialize as "all diagonal" (the default)
 set_PolyhedralFunction( std::move( tA ) , std::move( tb ) , bound , cnvx ,
			 issueMod );

 }  // end( PolyhedralFunction::deserialize )

/*--------------------------------------------------------------------------*/

#ifndef NDEBUG

std::vector< std::string > PolyhedralFunction::expected_dims( void ) const {
 static const std::vector< std::string > ed =
 { "PolyFunction_NumVar" , "PolyFunction_NumRow" , "PolyFunction_sign" };

 return( ed );
 }

/*--------------------------------------------------------------------------*/

std::vector< std::string > PolyhedralFunction::expected_vars( void ) const {
 static const std::vector< std::string > ev =
 { "PolyFunction_A" , "PolyFunction_b" , "PolyFunction_lb" };

 return( ev );
 }

#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::set_variables( VarVector && x )
{
 if( ( ! v_A.empty() ) && ( v_A[ 0 ].size() != x.size() ) )
  throw( std::logic_error(
		     "PolyhedralFunction::set_variables: wrong x.size()" ) );
 #ifndef NDEBUG
  // check that all the variables are distinct 
  if( x.size() > 1 ) {
   std::vector< Index > sorted( x.size() );
   std::iota( sorted.begin() , sorted.end() , 0 );
   std::sort( sorted.begin() , sorted.end() ,
	      [ & x ]( Index i , Index j ) {
	       return( std::less< ColVariable * >{}( x[ i ] , x[ j ] ) );
	       } );
   for( Index i = 0 ; i < x.size() - 1 ; ++i )
    if( x[ sorted[ i ] ] == x[ sorted[ i + 1 ] ] )
     throw( std::invalid_argument( "PolyhedralFunction::set_variables: "
				   "repeated ColVariable in x[ "
				   + std::to_string( sorted[ i ] ) +
				   " ] and x[ "
				   + std::to_string( sorted[ i + 1 ] ) + " ]"
				   ) );
   }
 #endif

 v_x = std::move( x );

 f_next = 0;
 set_f_uncomputed();

 }  // end( PolyhedralFunction::set_variables )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE PolyhedralFunction -------*/
/*--------------------------------------------------------------------------*/

int PolyhedralFunction::compute( bool changedvars )
{
 if( ( ! changedvars ) && is_f_computed() )
  return( kOK );      //  nothing changed since last call, nothing to do

 f_value = f_bound;
 // at the very least the lower/upper bound, possibly -/+INF
 f_next = 0;
 f_inf_idx = 0;
 f_n_violated = 0;

 if( v_A.empty() ) {      // no "real" rows
  if( ! is_bound_set() )  // and the lower/upper bound is *not* set
   set_f_uncomputed();    // +INF for convex, -INF for concave
  else                    // but at least there is a bound
   v_ord[ 0 ] = 0;        // it is the linearization

  return( kOK );
  }

 // copy x into a std::vector<> (more cache friendly)
 RealVector x( v_x.size() );
 for( Index j = 0 ; j < v_x.size() ; ++j )
  x[ j ] = v_x[ j ]->get_value();

 // tolerance for declaring a vertical row violated: it must be looser
 // than the feasibility tolerance of any LP/QP solver evaluating the
 // same constraint, so that points at the boundary (where small
 // floating-point residuals are unavoidable) are not declared
 // infeasible here. 1e-6 matches the typical default feasibility
 // tolerance of CPLEX, Gurobi, OSI, ...
 //
 // TODO: a cleaner long-term design exposes this as a parameter (e.g.
 // via dblRelAcc or a new dblViol parameter on the function), so that
 // the caller can match the master problem solver's effective
 // feasibility tolerance.
 auto vert_violation_tol = [ this ]( Index i ) -> FunctionValue {
  FunctionValue rowmag = std::abs( v_b[ i ] );
  for( auto a : v_A[ i ] )
   rowmag += std::abs( a );
  return( 1e-6 * std::max( FunctionValue( 1 ) , rowmag ) );
  };

 // single-linearization local pool: do the cheap, in-place computation
 if( v_ord.size() <= 1 ) {
  // first scan vertical rows for the most violated one (if any)
  if( f_n_vert ) {
   Index inf_i = get_nrows();      // sentinel: no violation found
   FunctionValue inf_val = 0;
   for( Index i = 0 ; i < get_nrows() ; ++i ) {
    if( ! v_is_vert[ i ] )
     continue;
    FunctionValue vi = std::inner_product( x.begin() , x.end() ,
					   v_A[ i ].begin() , v_b[ i ] );
    const auto tol = vert_violation_tol( i );
    if( f_is_convex ) {
     if( vi > tol && ( ( inf_i == get_nrows() ) || ( vi > inf_val ) ) ) {
      inf_i = i; inf_val = vi;
      }
     }
    else {
     if( vi < - tol && ( ( inf_i == get_nrows() ) || ( vi < inf_val ) ) ) {
      inf_i = i; inf_val = vi;
      }
     }
    }
   if( inf_i != get_nrows() ) {  // a violation was found: f is +/- INF
    f_value = f_is_convex ? Inf< FunctionValue >() : - Inf< FunctionValue >();
    f_inf_idx = inf_i + 1;       // +1 to align with v_ord/v_glob convention
    v_ord[ 0 ] = f_inf_idx;
    f_n_violated = 1;
    return( kOK );
    }
   }

  // no vertical violation: usual max/min over the diagonal rows
  v_ord[ 0 ] = 0;  // == lower/upper bound
  if( f_is_convex ) {
   for( Index i = 0 ; i < get_nrows() ; ++i ) {
    if( is_row_vertical( i ) )
     continue;
    auto vi = std::inner_product( x.begin() , x.end() , v_A[ i ].begin() ,
				  v_b[ i ] );
    if( vi > f_value ) {
     f_value = vi;
     v_ord[ 0 ] = i + 1;
     }
    }
   }
  else {
   for( Index i = 0 ; i < get_nrows() ; ++i ) {
    if( is_row_vertical( i ) )
     continue;
    auto vi = std::inner_product( x.begin() , x.end() , v_A[ i ].begin() ,
				  v_b[ i ] );
    if( vi < f_value ) {
     f_value = vi;
     v_ord[ 0 ] = i + 1;
     }
    }
   }

  return( kOK );
  }

 // multi-linearization local pool: compute v[ ] for all rows + the bound
 // and sort v_ord lexicographically. After the sort:
 //   v_ord[ 0 .. get_nrows() - f_n_vert ] = bound + diagonal rows, sorted
 //                                          by value (most "active" first)
 //   v_ord[ get_nrows() - f_n_vert + 1 .. get_nrows() ] = vertical rows,
 //                                          sorted by violation (most
 //                                          violated first)
 // any diagonal entry comes lexicographically before any vertical one
 RealVector v( get_nrows() + 1 );
 v[ 0 ] = f_bound;
 for( Index i = 0 ; i < get_nrows() ; ++i )
  v[ i + 1 ] = std::inner_product( x.begin() , x.end() , v_A[ i ].begin() ,
				   v_b[ i ] );

 if( f_is_convex )
  std::sort( v_ord.begin() , v_ord.end() ,
	     [ & v , this ]( c_Index a , c_Index b ) {
	      const bool a_v = ( a > 0 ) && is_row_vertical( a - 1 );
	      const bool b_v = ( b > 0 ) && is_row_vertical( b - 1 );
	      if( a_v != b_v ) return( ! a_v );  // diagonal first
	      return( v[ a ] > v[ b ] );
	      } );
 else
  std::sort( v_ord.begin() , v_ord.end() ,
	     [ & v , this ]( c_Index a , c_Index b ) {
	      const bool a_v = ( a > 0 ) && is_row_vertical( a - 1 );
	      const bool b_v = ( b > 0 ) && is_row_vertical( b - 1 );
	      if( a_v != b_v ) return( ! a_v );  // diagonal first
	      return( v[ a ] < v[ b ] );
	      } );

 // count violated verticals at the head of the vertical section. Since
 // the section is sorted by decreasing violation, we can stop as soon as
 // we hit a non-violated entry
 if( f_n_vert ) {
  const Index first_v = get_nrows() - f_n_vert + 1;
  for( Index p = first_v ; p <= get_nrows() ; ++p ) {
   const Index name = v_ord[ p ];
   const Index row = name - 1;
   const FunctionValue vi = v[ name ];
   const auto tol = vert_violation_tol( row );
   if( f_is_convex ? vi > tol : vi < - tol )
    ++f_n_violated;
   else
    break;
   }
  }

 if( f_n_violated > 0 ) {
  // INF state: the "active" linearization is the most violated vertical,
  // which sits at v_ord[ first_v ] right after the diagonal section
  f_value = f_is_convex ? Inf< FunctionValue >() : - Inf< FunctionValue >();
  f_next = get_nrows() - f_n_vert + 1;
  f_inf_idx = v_ord[ f_next ];
  }
 else {
  // no violation: f_value is the most active diagonal (or the bound)
  f_value = v[ v_ord[ 0 ] ];
  }

 return( kOK );

 }  // end( PolyhedralFunction::compute )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunction::has_linearization( bool diagonal )
{
 // when f_inf_idx is non-zero, f_value is +/- INF because the row encoded
 // by f_inf_idx (a vertical linearization) is violated at the current point;
 // in that state the available linearization is vertical, not diagonal
 if( f_inf_idx )
  return( ! diagonal );

 #if( EXPLICIT_BOUND )
  // there is always a linearization provided there are rows, counting the
  // all-0 one implicit in the bound among them

  return( diagonal ? ( ! v_A.empty() ) || is_bound_set() : false );
 #else
 // outside of the INF state vertical linearizations are not available. also,
 // the flat all-zero subgradient corresponding to the lower bound is never
 // explicitly produced since there is no need for it (the lower bound
 // already implies it). as a consequence, if there are no linearizations
 // nothing is ever returned

  if( ( ! diagonal ) || v_A.empty() )
   return( false );
  else
   return( v_ord[ 0 ] != 0 );
 #endif
 }

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::store_linearization( Index name ,
					      ModParam issueMod )
{
 if( name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool name" ) );

 if( v_glob[ name ] < 0 ) {         // it is an aggregated item
  // mark its position in v_ab[] with INF to signal it's not needed
  v_ab[ - v_glob[ name ] - 1 ] = Inf< FunctionValue >();
  compact_combinations();  // then check if it has to be shortened
  }
 
 v_glob[ name ] = v_ord[ f_next ];
 if( name >= f_max_glob )  // update f_max_glob
  f_max_glob = name + 1;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::store_combination_of_linearizations(
        c_LinearCombination & coefficients , Index name , ModParam issueMod )
{
 if( name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool name" ) );

 if( coefficients.empty() )
  throw( std::invalid_argument( "empty coefficients" ) );
  
 // construct the aggregated linearization in a new vector
 RealVector a( v_x.size() , 0 );
 FunctionValue b = 0;

 FunctionValue sum = 0;

 for( const auto & coef : coefficients ) {
  auto pos = v_glob[ coef.first ];

  if( pos == Inf< int >() )
   throw( std::invalid_argument( "invalid name in coefficients" ) );

  auto mult = coef.second;

  if( mult < - AAccMlt )
   throw( std::invalid_argument( "negative convex multiplier" ) );

  if( mult < 0 )  // a small mult < 0 is mult == 0
   continue;      // which contributes nothing to the combination

  sum += mult;

  #if( EXPLICIT_BOUND )
   // this is only necessary when PolyhedralFunction explicitly produces
   // all-0 horizontal subgradients

   if( ! pos ) {                 // == bound
    b += f_bound * coef.second;  // A[ i ] is all-0
    continue;
    }
  #endif

  RealVector::iterator ait;
  if( pos > 0 ) {
   ait = v_A[ --pos ].begin();
   b += v_b[ pos ] * mult;
   }
  else {
   pos = - pos - 1;
   ait = v_aA[ pos ].begin();
   b += v_ab[ pos ] * mult;
   }

  for( auto & ai : a )
   ai += (*(ait++)) * mult;
  }

 // now check that the convex multipliers really arw convex
 /*!!
 if( sum > 1 + AAccMlt * coefficients.size() )
  throw( std::invalid_argument( "multipliers sum to > 1" ) );
  !!*/

 if( sum < 1 - AAccMlt * coefficients.size() ) {
  /*!!
  if( ! is_bound_set() )
   throw( std::invalid_argument( "multipliers sum to < 1, and no bound" ) );
  !!*/

  //!!
  if( is_bound_set() )
   b += f_bound * ( 1 - sum );
  }

 // now put the vector in the right place: 
 
 Index pos = 0;  // index into v_aA[], v_ab[]

 if( v_glob[ name ] >= 0 ) {
  // currently name is either an original linearization, or the bound, or
  // empty; in each case a new aggregated linearization must be created
  // search for a free position in aA[], ab[]
  for( ; pos < v_ab.size() ; ++pos )
   if( v_ab[ pos ] == Inf< FunctionValue >() )
    break;

  if( pos == v_ab.size() ) {  // no free position is found
   v_aA.resize( pos + 1 );    // thus create one
   v_ab.resize( pos + 1 );    // in both vectors
   }

  v_glob[ name ] = - pos - 1;  // recall where the thing went
  }
 else  // the aggregated linearization replaces an already aggregated one
  pos = - v_glob[ name ] - 1;

 // move the stuff in place
 v_aA[ pos ] = std::move( a );
 v_ab[ pos ] = b;

 if( name >= f_max_glob )  // update f_max_glob
  f_max_glob = name + 1;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 
 }  // end( PolyhedralFunction::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_linearization( Index name ,
					       ModParam issueMod )
{
 if( name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool name" ) );

 if( v_glob[ name ] == Inf< int >() )  // no item with that name
  return;                              // cowardly and silently return

 if( v_glob[ name ] < 0 ) {         // it is an aggregated item
  // mark its position in v_ab[] with INF to signal it's not needed
  v_ab[ - v_glob[ name ] - 1 ] = Inf< FunctionValue >();
  compact_combinations();  // then check if it has to be shortened
  }

 v_glob[ name ] = Inf< int >();        // mark the item as deleted
 update_f_max_glob();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;
  
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_linearizations( Subset && which ,
						bool ordered ,
						ModParam issueMod )
{
 if( which.empty() ) {  // delete them all
  v_glob.assign( f_max_glob , Inf< int >() );
  v_aA.clear();
  v_ab.clear();
  f_max_glob = 0;

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( which ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 // here, which is not empty, so we have to delete the given subset
 if( ! ordered )
  std::sort( which.begin() , which.end() );

 if( which.back() >= v_glob.size() )
  throw( std::invalid_argument(
  "PolyhedralFunction::delete_linearizations: invalid linearization name" ) );

 for( auto i : which ) {
  if( v_glob[ i ] == Inf< int >() )
   throw( std::invalid_argument(
   "PolyhedralFunction::delete_linearizations: invalid linearization name" ) );

  if( v_glob[ i ] < 0 )  // an aggregate linearization
   v_ab[ - v_glob[ i ] - 1 ] = Inf< FunctionValue >();  // mark it for deletion
  v_glob[ i ] = Inf< int >();
  }

 update_f_max_glob();

 compact_combinations();  // then check if v_a* have to be shortened

 if( f_Observer && f_Observer->issue_mod( issueMod ) )
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                 this , C05FunctionMod::GlobalPoolRemoved ,
                                 std::move( which ) , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( FunctionValue * g ,
							 Range range ,
							 Index name )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 FunctionValue * ai =  get_ai( name );

 #if( EXPLICIT_BOUND )
  if( ! ai )
   for( Index i = range.second - range.first ; i-- ; )
    *(g++) = 0;
  else
 #endif
   for( Index i = range.second - range.first ; i-- ; )
    *(g++) = (*ai++);

 }  // end( PolyhedralFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( SparseVector & g ,
							 Range range ,
							 Index name )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 FunctionValue * ai =  get_ai( name );

 #if( EXPLICIT_BOUND )
  if( ai )             // not the all-0 vector
 #endif
   ai += range.first;  // point to the right place

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  if( vecsize( g.size() ) < v_x.size() )
   g.resize( v_x.size() );

  g.reserve( range.second - range.first );

  #if( EXPLICIT_BOUND )
   if( ! ai )  // the all-0 vector
    return;    // all done
  #endif

  for( Index i = range.first ; i < range.second ; ++i , ++ai )
   if( *ai != 0 )
    g.insert( i ) = *ai;
  }
 else {                     // g contains some non-zero elements
  if( vecsize( g.size() ) != v_x.size() )
   throw( std::invalid_argument(
	     "get_linearization_coefficients: invalid SparseVector size" ) );

  #if( EXPLICIT_BOUND )
   if( ! ai )
    for( Index i = range.first ; i < range.second ; )
     g.coeffRef( i++ ) = 0;
   else
  #endif
    for( Index i = range.first ; i < range.second ; )
     g.coeffRef( i++ ) = *(ai++);

  g.prune( 0 , 0 );
  }
 }  // end( PolyhedralFunction::get_linearization_coefficients( sv , range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( FunctionValue * g ,
							 c_Subset & subset ,
							 const bool ordered ,
							 Index name )
{
 FunctionValue * ai =  get_ai( name );

 if( ai )
  for( auto i : subset ) {
   if( i >= v_x.size() )
   throw( std::invalid_argument(
			   "get_linearization_coefficients: wrong index" ) );
   g[ i ] = (*ai++);
   }
 else
  for( auto i : subset ) {
   if( i >= v_x.size() )
   throw( std::invalid_argument(
			   "get_linearization_coefficients: wrong index" ) );
   g[ i ] = 0;
   }

 }  // end( PolyhedralFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( SparseVector & g ,
							 c_Subset & subset ,
							 const bool ordered ,
							 Index name )
{
 FunctionValue * ai =  get_ai( name );

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  if( vecsize( g.size() ) < v_x.size() )
   g.resize( v_x.size() );

  g.reserve( subset.size() );

  #if( EXPLICIT_BOUND )
   if( ! ai )  // the all-0 vector
    return;    // all done
  #endif

  for( auto i : subset ) {
   if( i >= v_x.size() )
   throw( std::invalid_argument(
			   "get_linearization_coefficients: wrong index" ) );
   auto aiv = (*ai++);
   if( aiv )
    g.insert( i ) = aiv;
   }
  }
 else {                     // g contains some non-zero elements
  if( vecsize( g.size() ) != v_x.size() )
   throw( std::invalid_argument(
	     "get_linearization_coefficients: invalid SparseVector size" ) );

  #if( EXPLICIT_BOUND )
   if( ! ai )
    for( auto i : subset ) {
     if( i >= v_x.size() )
      throw( std::invalid_argument(
			   "get_linearization_coefficients: wrong index" ) );
     g.coeffRef( i ) = 0;
     }
   else
  #endif
    for( auto i : subset ) {
     if( i >= v_x.size() )
      throw( std::invalid_argument(
			   "get_linearization_coefficients: wrong index" ) );
     g.coeffRef( i ) = (*ai++);
     }

  g.prune( 0 , 0 );
  }
 }  // end( PolyhedralFunction::get_linearization_coefficients( sv, subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue PolyhedralFunction::get_linearization_constant(
								  Index name )
{
 int gn = name >= v_glob.size() ? v_ord[ f_next ] : v_glob[ name ];

 #if( EXPLICIT_BOUND )
  if( ! gn )              // name 0
   return( f_bound );     // == bound
 #endif

 if( gn < 0 )            // aggregated linearization
  return( v_ab[ - gn - 1 ] );

 if( Index( gn ) <= get_nrows() )  // normal linearization
  return( v_b[ --gn ] );

 // there is no item with such a name, which may mean that it was there
 // once but it has been deleted: the linearization is invalid
 return( std::numeric_limits< FunctionValue >::quiet_NaN() );
}

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::serialize( netCDF::NcGroup & group ) const
{
 c_Index nvar = get_num_active_var();

 netCDF::NcDim nv = group.addDim( "PolyFunction_NumVar" , nvar );

 if( get_nrows() ) {
  netCDF::NcDim nr = group.addDim( "PolyFunction_NumRow" , get_nrows() );

  auto ncdA = group.addVar( "PolyFunction_A" , netCDF::NcDouble() ,
			    { nr , nv } );

  for( Index i = 0 ; i < get_nrows() ; ++i )
   ncdA.putVar( { i , 0 } , { 1 , nvar } , v_A[ i ].data() );

  ( group.addVar( "PolyFunction_b" , netCDF::NcDouble() , nr ) ).putVar(
				      { 0 } , { get_nrows() } , v_b.data() );
  }

 if( ! f_is_convex )
  group.addDim( "PolyFunction_sign" , 0 );

 if( is_bound_set() )
  ( group.addVar( "PolyFunction_lb" , netCDF::NcDouble() ) ).putVar(
								  &f_bound );

 }  // end( PolyhedralFunction::serialize )

/*--------------------------------------------------------------------------*/
/*-------- METHODS FOR HANDLING THE State OF THE PolyhedralFunction --------*/
/*--------------------------------------------------------------------------*/

State * PolyhedralFunction::get_State( void ) const {
  return( new PolyhedralFunctionState( this ) );
  }

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::put_State( const State & state )
{
 // if state is not a PolyhedralFunctionState &, exception will be thrown
 const auto & s = dynamic_cast< const PolyhedralFunctionState & >( state );

 // find out which elements are removed from / added to the global pool
 auto res = guts_of_put_State( s );

 // now actually change the data
 if( v_glob.size() > s.v_glob.size() )
  std::copy( s.v_glob.begin() , s.v_glob.end() , v_glob.begin() );
 else
  v_glob = s.v_glob;
 v_aA = s.v_aA;
 v_ab = s.v_ab;
 f_imp_coeff = s.f_imp_coeff;

 // if there is no Observer, no-one is looking at what just happened
 if( ! f_Observer )
  return;

 // but if there is an Observer the Modification have to be issued *after*
 // the data change, which is why this is not done in guts_of_put_State()
 // first tell about removals (if there is anything to remove)
 if( ! res.first.empty() )
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , std::move( res.first ) , 0 ) );

 // then tell about additions (if there is anything to add), so that the
 // aggregated linearizations are substituted with the new ones
 if( ! res.second.empty() )
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( res.second ) , 0 ) );

 }  // end( PolyhedralFunction::put_State( const & ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::put_State( State && state )
{
 // if state is not a PolyhedralFunctionState &, exception will be thrown
 auto && s = dynamic_cast< PolyhedralFunctionState && >( state );

 // find out which elements are removed from / added to the global pool
 auto res = guts_of_put_State( s );

 // now actually change the data
 if( v_glob.size() > s.v_glob.size() )
  std::copy( s.v_glob.begin() , s.v_glob.end() , v_glob.begin() );
 else
  v_glob = std::move( s.v_glob );
 v_aA = std::move( s.v_aA );
 v_ab = std::move( s.v_ab );
 f_imp_coeff = std::move( s.f_imp_coeff );
 
 // if there is no Observer, no-one is looking at what just happened
 if( ! f_Observer )
  return;

 // but if there is an Observer the Modification have to be issued *after*
 // the data change, which is why this is not done in guts_of_put_State()
 // first tell about removals (if there is anything to remove)
 if( ! res.first.empty() )
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , std::move( res.first ) , 0 ) );

 // then tell about additions (if there is anything to add), so that the
 // aggregated linearizations are substituted with the new ones
 if( ! res.second.empty() )
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( res.second ) , 0 ) );

 }  // end( PolyhedralFunction::put_State( && ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::serialize_State( netCDF::NcGroup & group ,
					  const std::string & sub_group_name
					  ) const
{
 if( ! sub_group_name.empty() ) {
  auto gr = group.addGroup( sub_group_name );
  serialize_State( gr );
  return;
  }

 // do it "by hand" since there is no PolyhedralFunctionState available
 // to call State::serialize() from
 group.putAtt( "type", "PolyhedralFunctionState" );

 netCDF::NcDim gs = group.addDim( "PolyFunction_GlobSize" , v_glob.size() );

 ( group.addVar( "PolyFunction_Glob" , netCDF::NcInt() , gs ) ).putVar(
			       { 0 } , {  v_glob.size() } , v_glob.data() );


 c_Index nvar = get_num_active_var();
 netCDF::NcDim nv = group.addDim( "PolyFunction_NumVar" , nvar );

 if( ! v_aA.empty() ) {
  netCDF::NcDim nr = group.addDim( "PolyFunction_ANumRow" , v_aA.size() );

  auto ncdA = group.addVar( "PolyFunction_aA" , netCDF::NcDouble() ,
			    { nr , nv } );

  for( Index i = 0 ; i < v_aA.size() ; ++i )
   ncdA.putVar( { i , 0 } , { 1 , nvar } , v_aA[ i ].data() );

  ( group.addVar( "PolyFunction_ab" , netCDF::NcDouble() , nr ) ).putVar(
				    { 0 } , {  v_ab.size() } , v_ab.data() );
  }

 if( ! f_imp_coeff.empty() ) {
  netCDF::NcDim cn = group.addDim( "PolyFunction_ImpCoeffNum" ,
				   f_imp_coeff.size() );
  
  auto ncCI = group.addVar( "PolyFunction_ImpCoeffInd" , netCDF::NcInt() ,
			    cn );

  auto ncCV = group.addVar( "PolyFunction_ImpCoeffVal" , netCDF::NcDouble() ,
			    cn );
 
  for( Index i = 0 ; i < f_imp_coeff.size() ; ++i ) {
   ncCI.putVar( { i } , f_imp_coeff[ i ].first );
   ncCV.putVar( { i } , f_imp_coeff[ i ].second );
   }
  }
 }  // end( PolyhedralFunction::serialize_State )

/*--------------------------------------------------------------------------*/
/*--- METHODS FOR HANDLING "ACTIVE" Variable IN THE PolyhedralFunction -----*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
				     bool ordered ) const
{
 if( ! v_x.size() )
  return;

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 if( ordered ) {
  Index found = 0;
  for( Index i = 0 ; i < v_x.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() , v_x[ i ] );
   if( itvi != vars.end() ) {
    map[ std::distance( vars.begin() , itvi ) ] = i;
    ++found;
    }
   }
  if( found < vars.size() )
   throw( std::invalid_argument( "map_active: some Variable is not active" )
	  );
  }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = PolyhedralFunction::is_active( var );
   if( i >= v_x.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active" )
	   );
   *(it++) = i;
   }
  }
 }  // end( PolyhedralFunction::map_active )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE PolyhedralFunction ---------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::set_PolyhedralFunction( MultiVector && A ,
						 RealVector && b ,
						 FunctionValue bound ,
						 bool is_convex ,
						 ModParam issueMod ,
						 BoolVector && is_vert )
{
 if( ( ! A.empty() ) && ( ! v_x.empty() ) )
  if( v_x.size() != A[ 0 ].size() )
   throw( std::invalid_argument( "A and x must have the same columns" ) );

 if( ( ( is_convex ) && ( bound == Inf< FunctionValue >() ) ) ||
     ( ( ! is_convex ) && ( bound == -Inf< FunctionValue >() ) ) )
  throw( std::invalid_argument( "wrong INF value to global bound" ) );

 if( A.size() != b.size() )
  throw( std::invalid_argument( "A and b must have the same rows" ) );

 if( ( ! is_vert.empty() ) && ( is_vert.size() != A.size() ) )
  throw( std::invalid_argument( "is_vert and A must have the same rows "
				"(or is_vert empty)" ) );

 f_is_convex = is_convex;

 if( A.empty() ) {
  v_A.clear();
  v_b.clear();
  v_is_vert.clear();
  f_n_vert = 0;
  }
 else {
  const Index n = A[ 0 ].size();
  for( auto & a : A )
   if( a.size() != n )
    throw( std::invalid_argument( "all rows A must have the same size" ) );

  v_A = std::move( A );
  v_b = std::move( b );
  // count vertical rows; keep v_is_vert empty if none, to preserve the
  // default "no vertical" representation; otherwise store it
  f_n_vert = Index( std::count( is_vert.begin() , is_vert.end() , true ) );
  if( f_n_vert )
   v_is_vert = std::move( is_vert );
  else
   v_is_vert.clear();
  }

 f_bound = bound;
 f_max_glob = f_next = 0;
 f_inf_idx = 0;
 f_n_violated = 0;
 v_glob.assign( v_glob.size() , Inf< int >() );
 v_aA.clear();
 v_ab.clear();

 set_f_uncomputed();  // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();
 reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                this , FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::set_PolyhedralFunction )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::set_is_convex( bool is_convex , ModParam issueMod )
{
 if( is_convex == f_is_convex )  // actually doing nothing
  return;                        // cowardly (and silently) return

 // if the function was convex and the bound was -INF == not set, it is now
 // concave and the bound not set means it must be +INF (and vice-versa)
 if( f_is_convex ) {
  if( f_bound == -Inf< FunctionValue >() )
   f_bound = Inf< FunctionValue >();
   }
 else
  if( f_bound == Inf< FunctionValue >() )
   f_bound = -Inf< FunctionValue >();
  
 f_is_convex = is_convex;           // change the verse
 set_f_uncomputed();                // the function value has changed

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // issue the PolyhedralFunctionMod: if f_is_convex is true the function has
 // changed from min to max, hence has increased, and vice-versa
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionMod >(
                                this , C05FunctionMod::NothingChanged ,
                                Subset( {} ) ,
                                f_is_convex ? FunctionMod::INFshift
                                            : -FunctionMod::INFshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
}

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_variables( VarVector && nx , MultiVector && nA ,
				        ModParam issueMod )
{
 c_Index nn = nx.size();
 if( ! nn )  // actually nothing to add
  return;    // cowardly (and silently) return

 if( ! v_A.empty() && nA.size() != get_nrows() )
  throw( std::invalid_argument( "wrong number of rows in nA" ) );

 for( auto & a : nA )
  if( a.size() != nn )
   throw( std::invalid_argument( "all rows nA must have the size of nx" ) );

 c_Index n = v_x.size();

 if( ! n ) {    // very easy case: adding to nothing
  v_A = std::move( nA );
  v_x = std::move( nx );
  }
 else {         // not much more difficult: append at the end
  if( v_A.empty() ) {
   assert( ! nA.empty() );
   v_A.resize( nA.size() );
   }

  for( Index i = 0 ; i < get_nrows() ; ++i )
   v_A[ i ].insert( v_A[ i ].end() , nA[ i ].begin() , nA[ i ].end() );

  v_x.insert( v_x.end() , nx.begin() , nx.end() );
  }

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 reset_aggregate_linearizations( issueMod );

 Vec_p_Var vars( nn );
 std::copy( v_x.begin() + n , v_x.end() , vars.begin() );

 // now issue the C05FunctionModVarsAddd
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
					this , std::move( vars ) , n , 0 ,
					Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_variables )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_variable( ColVariable * const var ,
				       c_RealVector & Aj ,
				       ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 if( v_A.empty() )
  v_A.resize( Aj.size() );

 for( Index j = 0 ; j < get_nrows() ; ++j )
  v_A[ j ].push_back( Aj[ j ] );

 v_x.push_back( var );

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 reset_aggregate_linearizations( issueMod );

 // now issue the Modification
 // a polyhedral function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
				        this , Vec_p_Var( { var } ) ,
					v_x.size() - 1 , 0 ,
					Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_variable )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variable( Index i , ModParam issueMod )
{
 if( v_x.size() <= i )
  throw( std::logic_error( "invalid Variable index" ) );

 auto var = v_x[ i ];
 v_x.erase( v_x.begin() + i );    // erase it in v_x
 for( auto & ai : v_A )           // erase the column in A
  ai.erase( ai.begin() + i );

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a polyhedral function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                this , Vec_p_Var( { var } ) ,
                                Range( i , i + 1 ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variables( Range range , ModParam issueMod )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 if( ( range.first == 0 ) && ( range.second == v_x.size() ) ) {
  // removing *all* variable
  Vec_p_Var vars( v_x.size() );

  for( Index i = 0 ; i < v_x.size() ; ++i )
   vars[ i ] = v_x[ i ];

  v_x.clear();            // clear v_x
  for( auto & ai : v_A )  // erase all v_A
   ai.clear();

  // now issue the Modification
  // a polyhedral function is strongly quasi-additive, and nms is ordered
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                  this , std::move( vars ) , range , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  return;
  }

 // this is not a complete reset
 // erase the columns in v_A
 for( auto & ai : v_A )
  ai.erase( ai.begin() + range.first , ai.begin() + range.second );

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 // erase the elements in v_x
 const auto strtit = v_x.begin() + range.first;
 const auto stopit = v_x.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  std::copy( strtit , stopit , vars.begin() );
  v_x.erase( strtit , stopit );

  // now issue the Modification
  // a polyhedral function is strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                 this , std::move( vars ) , range , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  v_x.erase( strtit , stopit );

 }  // end( PolyhedralFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

template< class T >
static void compact( std::vector< T > x ,
		     const PolyhedralFunction::Subset & nms )
{
 PolyhedralFunction::Index i = nms.front();
 auto xit = x.begin() + (i++);
 for( auto nit = ++(nms.begin()) ; nit != nms.end() ; ++i )
  if( *nit == i )
   ++nit;
  else
   *(xit++) = std::move( x[ i ] );

 for( ; i < x.size() ; ++i )
  *(xit++) = std::move( x[ i ] );

 x.resize( x.size() - nms.size() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void PolyhedralFunction::remove_variables( Subset && nms , bool ordered ,
					   ModParam issueMod )
{
 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.empty() ||
     ( ( nms.back() < v_x.size() ) && ( nms.size() == v_x.size() ) ) ) {
  // removing *all* variable
  Vec_p_Var vars( v_x.size() );

  for( Index i = 0 ; i < v_x.size() ; ++i )
   vars[ i ] = v_x[ i ];

  v_x.clear();            // clear v_x
  for( auto & ai : v_A )  // erase all v_A
   ai.clear();

  // now issue the Modification: note that the subset is empty
  // a polyhedral function is strongly quasi-additive, and nms is ordered
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                  this , std::move( vars ) , Subset() ,
                                  true , 0 , Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  return;
  }

 // this is not a complete reset

 if( nms.back() >= v_x.size() )  // the last name is wrong
  throw( std::invalid_argument(
     "PolyhedralFunction::remove_variables: wrong Variable index in nms" ) );

 for( auto & ai : v_A )          // erase the columns in A
  compact( ai , nms );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();
  for( auto nm : nms )
   *(its++) = v_x[ nm ];

  compact( v_x , nms );

  // now issue the Modification
  // a polyhedral function is strongly quasi-additive, and nms is ordered
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                 this , std::move( vars ) ,
                                 std::move( nms ) , true , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  compact( v_x , nms );
  
 }  // end( PolyhedralFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
				      Range range , ModParam issueMod ,
				      BoolVector && is_vert )
{
 if( range.second <= range.first )  // actually nothing to modify
  return;                           // cowardly (and silently) return

 if( range.second > get_nrows() )
  throw( std::invalid_argument( "wrong indices in range" ) );

 if( nA.size() != range.second - range.first )
  throw( std::invalid_argument( "range and nA sizes do not match" ) );

 if( nA.size() != nb.size() )
  throw( std::invalid_argument( "na and nb sizes do not match" ) );

 if( ( ! is_vert.empty() ) && ( is_vert.size() != nA.size() ) )
  throw( std::invalid_argument( "is_vert size must match nA, or be empty" ) );

 // copy rows
 for( Index i = 0 , j = range.first ; i < nA.size() ; ++i ) {
  if( nA[ i ].size() != v_x.size() )
   throw( std::invalid_argument( "wrong row size" ) );

  v_A[ j ] = std::move( nA[ i ] );
  v_b[ j++ ] = nb[ i ];
  }

 // update v_is_vert for the modified range. If is_vert is empty the modified
 // rows become diagonal (the documented default), so we may need to clear the
 // corresponding entries in v_is_vert; otherwise we copy is_vert in place.
 // After the update we shrink v_is_vert back to empty if no row is vertical.
 if( ! is_vert.empty() ) {
  if( v_is_vert.empty() )
   v_is_vert.assign( get_nrows() , false );
  for( Index i = 0 , j = range.first ; j < range.second ; ++i , ++j ) {
   if( v_is_vert[ j ] && ! is_vert[ i ] )
    --f_n_vert;             // was vertical, becomes diagonal
   else if( ( ! v_is_vert[ j ] ) && is_vert[ i ] )
    ++f_n_vert;             // was diagonal, becomes vertical
   v_is_vert[ j ] = is_vert[ i ];
   }
  }
 else if( ! v_is_vert.empty() ) {
  for( Index j = range.first ; j < range.second ; ++j )
   if( v_is_vert[ j ] ) {
    --f_n_vert;
    v_is_vert[ j ] = false;
    }
  }
 if( ( ! v_is_vert.empty() ) && f_n_vert == 0 )
  v_is_vert.clear();

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( v_glob[ i ] < 0 ) {
    v_glob[ i ] = Inf< int >();
    whiche.push_back( i );
    }
   else
    if( ( v_glob[ i ] > int( range.first ) ) &&
	( v_glob[ i ] <= int( range.second ) ) )
     which.push_back( i );

  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  }
 else  // there are no aggregate linearizations
  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( ( v_glob[ i ] > int( range.first ) ) &&
       ( v_glob[ i ] <= int( range.second  ) ) )
    which.push_back( i );

 // issue the PolyhedralFunctionModRngd: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::AllLinearizationChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AllLinearizationChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::ModifyRows ,
                                range , std::move( which ) , C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_rows( range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
				      Subset && rows , bool ordered ,
				      ModParam issueMod ,
				      BoolVector && is_vert )
{
 if( rows.empty() )  // actually nothing to modify
  return;            // cowardly (and silently) return

 if( nA.size() != rows.size() )
  throw( std::invalid_argument( "rows and nA sizes do not match" ) );

 if( nA.size() != nb.size() )
  throw( std::invalid_argument( "nA and nb sizes do not match" ) );

 if( ( ! is_vert.empty() ) && ( is_vert.size() != nA.size() ) )
  throw( std::invalid_argument( "is_vert size must match nA, or be empty" ) );

 // if is_vert came in we have to keep it aligned with rows; sorting rows
 // requires sorting is_vert with the same permutation
 if( ! ordered ) {
  if( is_vert.empty() ) {
   std::sort( rows.begin() , rows.end() );
   }
  else {
   // permutation-aware sort: build pairs ( row , is_vert_flag ) and sort
   std::vector< std::pair< Index , bool > > tmp( rows.size() );
   for( Index i = 0 ; i < rows.size() ; ++i )
    tmp[ i ] = { rows[ i ] , is_vert[ i ] };
   std::sort( tmp.begin() , tmp.end() ,
	      []( const auto & a , const auto & b ) {
	       return( a.first < b.first );
	       } );
   for( Index i = 0 ; i < rows.size() ; ++i ) {
    rows[ i ]    = tmp[ i ].first;
    is_vert[ i ] = tmp[ i ].second;
    }
   }
  }

 if( rows.back() >= get_nrows() )
  throw( std::invalid_argument( "wrong row names" ) );

 for( Index i = 0 ; i < rows.size() ; ++i ) {
  if( nA[ i ].size() != v_x.size() )
   throw( std::invalid_argument( "wrong row size" ) );

  v_A[ rows[ i ] ] = std::move( nA[ i ] );
  v_b[ rows[ i ] ] = nb[ i ];
  }

 // update v_is_vert for the modified rows (same logic as in the Range version)
 if( ! is_vert.empty() ) {
  if( v_is_vert.empty() )
   v_is_vert.assign( get_nrows() , false );
  for( Index i = 0 ; i < rows.size() ; ++i ) {
   const auto r = rows[ i ];
   if( v_is_vert[ r ] && ! is_vert[ i ] )
    --f_n_vert;             // was vertical, becomes diagonal
   else if( ( ! v_is_vert[ r ] ) && is_vert[ i ] )
    ++f_n_vert;             // was diagonal, becomes vertical
   v_is_vert[ r ] = is_vert[ i ];
   }
  }
 else if( ! v_is_vert.empty() ) {
  for( auto r : rows )
   if( v_is_vert[ r ] ) {
    --f_n_vert;
    v_is_vert[ r ] = false;
    }
  }
 if( ( ! v_is_vert.empty() ) && f_n_vert == 0 )
  v_is_vert.clear();

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  // now search if some of the changed rows are in the global pool
  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( v_glob[ i ] < 0 ) {
    v_glob[ i ] = Inf< int >();
    whiche.push_back( i );
    }
   else
    #if( EXPLICIT_BOUND )
     if( v_glob[ i ] )  // unless it's the bound
    #endif
     {      
      auto it = std::lower_bound( rows.begin() , rows.end() ,
				  v_glob[ i ] - 1 );
      if( ( it != rows.end() ) && ( *it == Index( v_glob[ i ] - 1 ) ) )
       which.push_back( i );
      }

  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
 }
 else {  // there are no aggregate linearizations
  for( Index i = 0 ; i < f_max_glob ; ++i )
   #if( EXPLICIT_BOUND )
    if( v_glob[ i ] )  // unless it's the bound
   #endif
    {
     auto it = std::lower_bound( rows.begin() , rows.end() ,
				 v_glob[ i ] - 1 );
     if( ( it != rows.end() ) && ( int( *it ) == v_glob[ i ] - 1 ) )
      which.push_back( i );
     }
  }

 // issue the PolyhedralFunctionModSbst: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::AllLinearizationChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AllLinearizationChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModSbst >(
                                this , type , PolyhedralFunctionMod::ModifyRows ,
                                std::move( rows ) , std::move( which ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_rows( subset ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_row( Index i , RealVector && Ai ,
				     FunctionValue bi , ModParam issueMod ,
				     bool is_vert )
{
 if( i >= get_nrows() )
  throw( std::invalid_argument( "wrong row name" ) );

 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "wrong row size" ) );

 // actually change things
 v_A[ i ] = std::move( Ai );
 v_b[ i ] = bi;

 // keep v_is_vert / f_n_vert consistent
 if( is_vert ) {
  if( v_is_vert.empty() ) {
   v_is_vert.assign( get_nrows() , false );
   v_is_vert[ i ] = true;
   ++f_n_vert;
   }
  else if( ! v_is_vert[ i ] ) {
   v_is_vert[ i ] = true;
   ++f_n_vert;
   }
  }
 else if( ! v_is_vert.empty() ) {
  if( v_is_vert[ i ] ) {
   v_is_vert[ i ] = false;
   --f_n_vert;
   }
  if( f_n_vert == 0 )
   v_is_vert.clear();
  }

 set_f_uncomputed();                // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  // now search if the changed row is in the global pool
  for( Index j = 0 ; j < f_max_glob ; ++j )
   if( v_glob[ j ] < 0 ) {       // an aggregated one
    v_glob[ j ] = Inf< int >();    // kill it for sure
    whiche.push_back( j );
    }
   else
    if( v_glob[ j ] == int( i + 1 ) )
     which.push_back( j );
 
  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  }
 else {  // there are no aggregate linearizations
  for( Index j = 0 ; j < f_max_glob ; ++j )
   if( v_glob[ j ] == int( i + 1 ) )
    which.push_back( j );
  }

 // issue the PolyhedralFunctionModRngd: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::AllLinearizationChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AllLinearizationChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::ModifyRows ,
                                Range( i , i + 1 ) , std::move( which ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_row )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_constants( c_RealVector & nb , Range range ,
					   ModParam issueMod )
{
 if( range.second <= range.first )  // actually nothing to modify
  return;                           // cowardly (and silently) return

 if( range.second > v_b.size() )
  throw( std::invalid_argument( "wrong indices in range" ) );
  
 if( nb.size() != range.second - range.first )
  throw( std::invalid_argument( "range and nb sizes do not match" ) );

 // first check if actually something has changed
 FunctionValue shift = 0;
 for( Index i = 0 ; i < nb.size() ; ++i )
  if( nb[ i ] > v_b[ range.first + i ] ) {
   if( shift == - C05FunctionMod::INFshift ) {
    shift = C05FunctionMod::NaNshift;
    break;
    }
   else
    shift = C05FunctionMod::INFshift;
   }
  else
  if( nb[ i ] < v_b[ range.first + i ] ) {
   if( shift == C05FunctionMod::INFshift ) {
    shift = C05FunctionMod::NaNshift;
    break;
    }
   else
    shift = - C05FunctionMod::INFshift;
   }

 if( shift == 0 )  // actually nothing is changing
  return;          // cowardly (and silently) return

 // actually change the constants
 for( Index i = 0 , j = range.first ; i < nb.size() ; ++i )
  v_b[ j++ ] = nb[ i ];

 set_f_uncomputed();                // the function value has changed
 // but note that the Lipschitz constant obviously has not

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  // now search if some of the changed constants are in the global pool
  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( v_glob[ i ] < 0 ) {
    v_glob[ i ] = Inf< int >();
    whiche.push_back( i );
    }
   else
    if( ( v_glob[ i ] > int( range.first ) ) &&
	( v_glob[ i ] <= int( range.second ) ) )
     which.push_back( i );

  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  }
 else {  // there are no aggregate linearizations
  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( ( v_glob[ i ] > int( range.first ) ) &&
       ( v_glob[ i ] <= int( range.second ) ) )
    which.push_back( i );
  }

 // issue the PolyhedralFunctionModRngd: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is C05FunctionMod::AlphaChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AlphaChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type ,
                                PolyhedralFunctionMod::ModifyCnst , range ,
                                std::move( which ) , shift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_constants( c_RealVector & nb ,
					   Subset && rows , bool ordered ,
					   ModParam issueMod )
{
 if( rows.empty() )  // actually nothing to modify
  return;            // cowardly (and silently) return

 if( nb.size() != rows.size() )
  throw( std::invalid_argument( "rows and nb sizes do not match" ) );

 // ordering is not very useful, if not for making it easy to check for
 // wrong row names; yet, so PolyhedralFunctionModSbst always has ordered rows
 if( ! ordered )
  std::sort( rows.begin() , rows.end() );

 if( rows.back() >= get_nrows() )
  throw( std::invalid_argument( "wrong row name" ) );

 // first check if actually something has changed
 FunctionValue shift = 0;
 for( Index i = 0 ; i < rows.size() ; ++i )
  if( nb[ i ] > v_b[ rows[ i ] ] ) {
   if( shift == - C05FunctionMod::INFshift ) {
    shift = C05FunctionMod::NaNshift;
    break;
    }
   else
    shift = C05FunctionMod::INFshift;
   }
  else
  if( nb[ i ] < v_b[ rows[ i ] ] ) {
   if( shift == C05FunctionMod::INFshift ) {
    shift = C05FunctionMod::NaNshift;
    break;
    }
   else
    shift = - C05FunctionMod::INFshift;
   }

 if( shift == 0 )  // actually nothing is changing
  return;          // cowardly (and silently) return

 // actually change the constants
 for( Index i = 0 ; i < rows.size() ; ++i )
  v_b[ rows[ i ] ] = nb[ i ];

 set_f_uncomputed();                // the function value has changed
 // but note that the Lipschitz constant obviously has not

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  // now search if some of the changed constants are in the global pool
  for( Index i = 0 ; i < f_max_glob ; ++i )
   if( v_glob[ i ] < 0 ) {       // an aggregated one
    v_glob[ i ] = Inf< int >();    // kill it for sure
    whiche.push_back( i );
    }
   else
    #if( EXPLICIT_BOUND )
     if( v_glob[ i ] )  // unless it's the bound
    #endif
     {
      auto it = std::lower_bound( rows.begin() , rows.end() ,
				  v_glob[ i ] - 1 );
      if( ( it != rows.end() ) && ( int( *it ) == v_glob[ i ] - 1 ) )
       which.push_back( i );
      }

  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
  }
 else {  // there are no aggregate linearizations
  for( Index i = 0 ; i < f_max_glob ; ++i )
   #if( EXPLICIT_BOUND )
    if( v_glob[ i ] )  // unless it's the bound
   #endif
    {
     auto it = std::lower_bound( rows.begin() , rows.end() ,
				 v_glob[ i ] - 1 );
     if( ( it != rows.end() ) && ( int( *it ) == v_glob[ i ] - 1 ) )
      which.push_back( i );
     }
  }

 // issue the PolyhedralFunctionModSbst: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is C05FunctionMod::AlphaChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AlphaChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModSbst >(
                                this , type , PolyhedralFunctionMod::ModifyCnst ,
                                std::move( rows ) , std::move( which ) ,
                                shift , Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_constant( Index i , FunctionValue bi ,
					  ModParam issueMod )
{
 if( i >= get_nrows() )
  throw( std::invalid_argument( "wrong row name" ) );

 if( bi == v_b[ i ] )  // actually nothing is changing
  return;              // cowardly (and silently) return

 FunctionValue shift = bi > v_b[ i ] ?   C05FunctionMod::INFshift
                                     : - C05FunctionMod::INFshift;
 // actually change the constant
 v_b[ i ] = bi;

 set_f_uncomputed();                // the function value has changed
 // but note that the Lipschitz constant obviously has not

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 Subset which;

 // now search if some of the changed rows are in the global pool
 if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
  Subset whiche;

  v_aA.clear();
  v_ab.clear();

  // now search the changed constant is in the global pool
  for( Index j = 0 ; j < f_max_glob ; ++j )
   if( v_glob[ j ] < 0 ) {
    v_glob[ j ] = Inf< int >();
    whiche.push_back( j );
    }
   else
    if( v_glob[ j ] == int( i + 1 ) )
     which.push_back( j );

  update_f_max_glob();

  // a separate C0FunctionMod for removals (if any)
  if( ! whiche.empty() )
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
 }
 else {  // there are no aggregate linearizations
  for( Index j = 0 ; j < f_max_glob ; ++j )
   if( v_glob[ j ] == int( i + 1 ) )
    which.push_back( j );
  }

 // issue the PolyhedralFunctionModRngd: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is C05FunctionMod::AlphaChanged
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::AlphaChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::ModifyCnst ,
                                Range( i , i + 1 ) , std::move( which ) ,
                                shift , Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_constant )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_bound( FunctionValue newbound ,
				       ModParam issueMod )
{
 if( newbound == f_bound )  // actually nothing is changing
  return;                   // cowardly (and silently) return

 if( ( newbound == Inf< FunctionValue>() && f_is_convex ) ||
     ( newbound == -Inf< FunctionValue>() && ( ! f_is_convex ) ) )
  throw( std::invalid_argument( "wrong INF value to global bound" ) );

 FunctionValue shift = newbound > f_bound ?   C05FunctionMod::INFshift
                                          : - C05FunctionMod::INFshift;

 // note: even if PolyhedralFunction is not actually producing the all-0
 //       horizontal subgradient when computed at a global minima/maxima,
 //       that subgradient still implicitly contributes to aggregated
 //       linearizations if the sum is < 1. thus, if the bound changes
 //       then also these need be reset. yet, if PolyhedralFunction is
 //       actually producing the all-0 horizontal subgradient, one also
 //       has to check if it is in the global pool

 bool wasset = is_bound_set();

 // actually change the bound
 f_bound = newbound;

 set_f_uncomputed();                // the function value has changed
 // but note that the Lipschitz constant obviously has not

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  reset_aggregate_linearizations();
  return;
  }

 #if( EXPLICIT_BOUND )
  Subset which;

  if( wasset ) {
   // if the bound was not set, nothing else to do: surely it can not have
   // contributed to exising linearizations

   Subset whiche;

   if( ! v_aA.empty() ) {  // meanwhile, reset aggregate linearizations
    v_aA.clear();
    v_ab.clear();

    if( is_bound_set() ) {  // the bound has been changed
     // now search if the changed bound is in the global pool
     for( Index i = 0 ; i < f_max_glob ; ++i )
      if( v_glob[ i ] < 0 ) {       // an aggregated one
       v_glob[ i ] = Inf< int >();   // kill it for sure
       whiche.push_back( i );
       }
      else
       if( ! v_glob[ i ] )
	which.push_back( i );
     }
    else  // the bound has been eliminated: eliminate both the it and any
          // aggregated linearization from the global pool
     for( Index i = 0 ; i < f_max_glob ; ++i )
      if( v_glob[ i ] <= 0 ) {
       v_glob[ i ] = Inf< int >();
       whiche.push_back( i );
       }
    }
   else {  // there are no aggregate linearizations
    if( is_bound_set() ) {  // the bound has been changed
     for( Index i = 0 ; i < f_max_glob ; ++i )
      if( ! v_glob[ i ] )
       which.push_back( i );
     }
    else  // the bound has been eliminated
     for( Index i = 0 ; i < f_max_glob ; ++i )
      if( ! v_glob[ i ] )
       whiche.push_back( i );
    }

   update_f_max_glob();

   // a separate C05FunctionMod for removals, if any
   if( ! whiche.empty() )
    f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                   this , C05FunctionMod::GlobalPoolRemoved ,
                                   std::move( whiche ) , 0 ,
                                   Observer::par2concern( issueMod ) ) ,
                                  Observer::par2chnl( issueMod ) );
   }  // end( if( wasset ) )

  // issue the PolyhedralFunctionModRngd: if which.empty() then no
  // linearization in the global pool is impacted anf therefore the type is
  // C05FunctionMod::NothingChanged, otherwise is C05FunctionMod::AlphaChanged
  // note: the explicit definition of type here was originally avoided by
  //       having the ? expression directly in the constructor, but this
  //       meant that the same expressio had a check if which was nonempty
  //       and a std-move of which that would make it empty, i.e., the
  //       perfect example of an expression with side-effects whose result
  //       depended on the order of the sub-expressions and therefore was
  //       compiler-dependent, meaning extremely-hard-to-find errors 
  auto type = which.empty() ? C05FunctionMod::NothingChanged
                            : C05FunctionMod::AlphaChanged;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::ModifyCnst ,
                                Range( 0 , 0 ) , std::move( which ) ,
                                shift , Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 #else
  // if the bound was set, reset exising aggregate linearizations (if any)
  if( wasset && ( ! v_aA.empty() ) ) {
   Subset whiche;

   v_aA.clear();
   v_ab.clear();

   for( Index i = 0 ; i < f_max_glob ; ++i )
    if( v_glob[ i ] < 0 ) {     // an aggregated one
     v_glob[ i ] = Inf< int >();  // kill it for sure
     whiche.push_back( i );
     }

   update_f_max_glob();

   // a separate C05FunctionMod for removals
   f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                  this , C05FunctionMod::GlobalPoolRemoved ,
                                  std::move( whiche ) , 0 ,
                                  Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
   }

  // issue the PolyhedralFunctionModRngd
  f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                 this , C05FunctionMod::NothingChanged ,
                                 PolyhedralFunctionMod::ModifyCnst ,
                                 Range( 0 , 0 ) , Subset() , shift ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 #endif

 }  // end( PolyhedralFunction::modify_bound )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_rows( MultiVector && nA , c_RealVector & nb ,
				   ModParam issueMod ,
				   BoolVector && is_vert )
{
 c_Index k = nA.size();
 if( k != nb.size() )
  throw( std::invalid_argument( "nA and nb must have the same size" ) );

 if( ( ! is_vert.empty() ) && ( is_vert.size() != k ) )
  throw( std::invalid_argument( "is_vert size must match nA, or be empty" ) );

 c_Index n = v_x.size();
 for( auto & a : nA )
  if( a.size() != n )
   throw( std::invalid_argument( "some rows of nA have a wrong size" ) );

 // update the Lipschitz constant (if computed)
 if( f_Lipschitz_constant >= 0 )
  compute_Lipschitz_constant( nA , f_Lipschitz_constant );

 c_Index oldnr = v_A.size();

 v_A.insert( v_A.end() , std::make_move_iterator( nA.begin() ) ,
                         std::make_move_iterator( nA.end() ) );

 v_b.insert( v_b.end() , nb.begin() , nb.end() );

 // append per-row vertical flags. We materialise v_is_vert iff at least one
 // row (current or new) is vertical; otherwise we keep it empty
 const Index n_new_vert = Index( std::count( is_vert.begin() , is_vert.end() ,
					     true ) );
 if( n_new_vert || ( ! v_is_vert.empty() ) ) {
  if( v_is_vert.empty() )
   v_is_vert.assign( oldnr , false );
  if( is_vert.empty() )
   v_is_vert.insert( v_is_vert.end() , k , false );
  else
   v_is_vert.insert( v_is_vert.end() ,
		     std::make_move_iterator( is_vert.begin() ) ,
		     std::make_move_iterator( is_vert.end() ) );
  f_n_vert += n_new_vert;
  }

 set_f_uncomputed();                // the function value has changed
 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the PolyhedralFunctionModAddd
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModAddd >(
                                this , k , Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_rows )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_row( RealVector && Ai , FunctionValue bi ,
				  ModParam issueMod , bool is_vert )
{
 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "Ai has a wrong size" ) );

 c_Index oldnr = v_A.size();

 v_A.push_back( std::move( Ai ) );
 v_b.push_back( bi );

 // materialise v_is_vert iff at least one row (current or new) is vertical
 if( is_vert || ( ! v_is_vert.empty() ) ) {
  if( v_is_vert.empty() )
   v_is_vert.assign( oldnr , false );
  v_is_vert.push_back( is_vert );
  if( is_vert )
   ++f_n_vert;
  }

 set_f_uncomputed();                // the function value has changed
 // update the Lipschitz constant (if computed)
 if( f_Lipschitz_constant >= 0 ) {
  FunctionValue L = 0;
  for( const auto aij : v_A.back() )
   L += aij * aij;

  if( L > f_Lipschitz_constant * f_Lipschitz_constant )
   f_Lipschitz_constant = sqrt( double( L ) );
  }

 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the PolyhedralFunctionModAddd
 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModAddd >(
                                this , 1 , Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_row )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_rows( Range range , ModParam issueMod )
{
 if( range.second <= range.first )  // actually nothing to modify
  return;                           // cowardly (and silently) return

 if( range.second > get_nrows() )
  throw( std::invalid_argument( "wrong indices in range" ) );

 if( range.second - range.first == 1 ) {
  delete_row( range.first , issueMod );
  return;
  }

 // kill stuff in v_A[]
 v_A.erase( v_A.begin() + range.first , v_A.begin() + range.second );
 // kill stuff in v_b[]
 v_b.erase( v_b.begin() + range.first , v_b.begin() + range.second );
 // kill stuff in v_is_vert[] (if present); update f_n_vert and shrink to
 // empty if no vertical row remains
 if( ! v_is_vert.empty() ) {
  for( Index j = range.first ; j < range.second ; ++j )
   if( v_is_vert[ j ] )
    --f_n_vert;
  v_is_vert.erase( v_is_vert.begin() + range.first ,
		   v_is_vert.begin() + range.second );
  if( f_n_vert == 0 )
   v_is_vert.clear();
  }

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 v_aA.clear();
 v_ab.clear();

 Subset which;

 // now search and mark as deleted the rows in the global pool; also,
 // all the names in v_glob[] >= range.second + 1 must be decreased by
 // range.second - range.first
 auto delta = range.second - range.first;
 for( Index i = 0 ; i < f_max_glob ; ++i ) {
  if( v_glob[ i ] == Inf< int >() )  // non-existent
   continue;
  if( v_glob[ i ] < 0 ) {             // an aggregated one
   v_glob[ i ] = Inf< int >();        // kill it for sure
   which.push_back( i );
   }
  else
   if( v_glob[ i ] > int( range.second ) )  // > than any of the deleted
    v_glob[ i ] -= delta;                   // decreased by # of deleted
   else
    if( v_glob[ i ] > int( range.first ) ) {  // one of the deleted
     v_glob[ i ] = Inf< int >();              // kill it
     which.push_back( i );
     }
  }

 update_f_max_glob();

 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown
 set_f_uncomputed();      // the function value has changed
 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the PolyhedralFunctionModRng: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::GlobalPoolRemoved
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::GlobalPoolRemoved;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::DeleteRows ,
                                range , std::move( which ) ,
                                f_is_convex ? -FunctionMod::INFshift
                                            : FunctionMod::INFshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 
 }  // end( PolyhedralFunction::delete_rows( range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_rows( Subset && rows , bool ordered ,
				      ModParam issueMod )
{
 if( rows.empty() )  // actually nothing to remove
  return;            // cowardly (and silently) returning

 if( rows.size() == 1 ) {
  delete_row( rows.front() , issueMod );
  return;
  }

 if( ! ordered )
  std::sort( rows.begin() , rows.end() );

 if( rows.back() >= get_nrows() )
  throw( std::invalid_argument( "invalid names in rows" ) );

 // mark stuff to be killed in v_A[] and v_b[]
 for( auto idx : rows ) {
  v_A[ idx ].clear();
  v_b[ idx ] = std::numeric_limits< FunctionValue >::quiet_NaN();
  }

 // kill stuff in v_A[]
 v_A.erase( remove_if( v_A.begin() + rows.front() , v_A.end() ,
		       []( RealVector & ai ) { return( ai.empty() ); } ) ,
	    v_A.end() );

 // kill stuff in v_b[]
 v_b.erase( remove_if( v_b.begin() + rows.front() , v_b.end() ,
		       []( FunctionValue bi ) {	return( std::isnan( bi ) ); }
		       ) , v_b.end() );

 // kill stuff in v_is_vert[] for the same indices (if present); since we
 // sorted rows above, we can just erase from back to front
 if( ! v_is_vert.empty() ) {
  for( auto it = rows.rbegin() ; it != rows.rend() ; ++it ) {
   if( v_is_vert[ *it ] )
    --f_n_vert;
   v_is_vert.erase( v_is_vert.begin() + *it );
   }
  if( f_n_vert == 0 )
   v_is_vert.clear();
  }

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 v_aA.clear();
 v_ab.clear();

 Subset which;

 // now search and mark as deleted the rows in the global pool; also,
 // all the names in v_glob[] that are >= i + 1 for some i in rows[]
 // must be decreased by the number of elements before i
 // that is, suppose that rows = [ 3 5 9 ]: for any 3 < v_glob[ i ] <= 9
 // (recall that names are increased by 1) we seek for the smallest index
 // in rows greater than or equal to v_glob[ i ] - 1. If it is v_glob[ i ] - 1
 // we delete it. Otherwise, say that glob[ i ] - 1 == 6 and we find the
 // position 2: it means that glob[ i ] must be decreased by 2
 for( Index i = 0 ; i < f_max_glob ; ++i ) {
  if( v_glob[ i ] == Inf< int >() )     // non-existent
   continue;
  if( v_glob[ i ] < 0 ) {                // an aggregated one
   v_glob[ i ] = Inf< int >();           // kill it for sure
   which.push_back( i );
   }
  else
   if( v_glob[ i ] > int( rows.back() + 1 ) )  // > than any of the deleted
    v_glob[ i ] -= rows.size();                // decreased by # of deleted
   else
    if( v_glob[ i ] > int( rows.front() ) ) {
     auto it = std::lower_bound( rows.begin() , rows.end() ,
				 v_glob[ i ] - 1 );
     if( int( *it ) == v_glob[ i ] - 1 ) {     // one of the deleted
      v_glob[ i ] = Inf< int >();              // kill it
      which.push_back( i );
      }
     else
      v_glob[ i ] -= std::distance( rows.begin() , it );
     }
  }

 update_f_max_glob();
 
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown
 set_f_uncomputed();      // the function value has changed
 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the PolyhedralFunctionModSbst: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::GlobalPoolRemoved
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expressio had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::GlobalPoolRemoved;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModSbst >(
                                this , type , PolyhedralFunctionMod::DeleteRows ,
                                std::move( rows ) , std::move( which ) ,
                                f_is_convex ? -FunctionMod::INFshift
                                            : FunctionMod::INFshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::delete_rows( subset ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_row( Index i , ModParam issueMod )
{
 if( i >= get_nrows() )
  throw( std::invalid_argument( "invalid names in rows" ) );

 v_A.erase( v_A.begin() + i );      // kill i in v_A[]
 v_b.erase( v_b.begin() + i );      // kill i in v_b[]
 if( ! v_is_vert.empty() ) {        // and kill i in v_is_vert[] if present
  if( v_is_vert[ i ] )
   --f_n_vert;
  v_is_vert.erase( v_is_vert.begin() + i );
  if( f_n_vert == 0 )
   v_is_vert.clear();
  }

 // reset all aggregated linearizations, since there is no way to know if
 // they are still valid
 v_aA.clear();
 v_ab.clear();

 Subset which;

 // now search and mark as deleted the row i in the global pool; also,
 // all the names in v_glob[] > i + 1 must be decreased by 1

 for( Index j = 0 ; j < f_max_glob ; ++j ) {
  if( v_glob[ j ] == Inf< int >() )  // non-existent
   continue;
  if( v_glob[ j ] < 0 ) {            // an aggregated one
   v_glob[ j ] = Inf< int >();       // kill it for sure
   which.push_back( j );
   }
  else
   if( v_glob[ j ] > int( i + 1 ) )
    --v_glob[ j ];
   else
    if( v_glob[ j ] == int( i + 1 ) ) {
     v_glob[ j ] = Inf< int >();
     which.push_back( j );
     }
  }

 update_f_max_glob();

 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown
 set_f_uncomputed();      // the function value has changed
 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the PolyhedralFunctionModRngd: if which.empty() then no
 // linearization in the global pool is impacted anf therefore the type is
 // C05FunctionMod::NothingChanged, otherwise is
 // C05FunctionMod::GlobalPoolRemoved
 // note: the explicit definition of type here was originally avoided by
 //       having the ? expression directly in the constructor, but this
 //       meant that the same expression had a check if which was nonempty
 //       and a std-move of which that would make it empty, i.e., the
 //       perfect example of an expression with side-effects whose result
 //       depended on the order of the sub-expressions and therefore was
 //       compiler-dependent, meaning extremely-hard-to-find errors 
 auto type = which.empty() ? C05FunctionMod::NothingChanged
                           : C05FunctionMod::GlobalPoolRemoved;

 f_Observer->add_Modification( std::make_shared< PolyhedralFunctionModRngd >(
                                this , type , PolyhedralFunctionMod::DeleteRows ,
                                Range( i , i + 1 ) , std::move( which ) ,
                                f_is_convex ? - FunctionMod::INFshift
                                            : FunctionMod::INFshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::delete_row )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_rows( ModParam issueMod )
{
 v_A.clear();   // delete original rows
 v_b.clear();
 v_is_vert.clear();    // and the per-row vertical flags
 f_n_vert = 0;
 f_bound = get_default_bound();
 v_aA.clear();  // delete aggregated linearizations
 v_ab.clear();

 v_glob.assign( v_glob.size() , Inf< int >() );
 f_max_glob = 0;

 set_f_uncomputed();      // the function value has changed
 f_Lipschitz_constant = -Inf< FunctionValue >();  // == unknown
 if( f_loc_pool_sz > 1 )  // resize v_ord
  reset_v_ord();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                this , FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::delete_rows( all ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

std::pair< Function::Subset , Function::Subset >
PolyhedralFunction::guts_of_put_State( const PolyhedralFunctionState & state )
{
 if( state.f_nvar != get_num_active_var() )
  throw( std::invalid_argument(
	        "PolyhedralFunctionState has wrong number of variables" ) );

 for( auto i : state.v_glob )
  if( ( i < Inf< int >() ) && ( i > int( get_nrows() ) ) )
   throw( std::invalid_argument(
	             "PolyhedralFunctionState global pool inconsistent" ) );

 // if there is no Observer, no-one is looking at what happens
 if( ! f_Observer )
  return( std::make_pair( Subset() , Subset() ) );

 // handle the changes in the global pool. all aggregated linearizations
 // in the current global pool need be deleted since they are replaced by
 // entirely new ones and we don't want to check if they are equal (although
 // we could); this also applies if they happen to have the same name

 Subset Addd;
 Subset Rmvd;

 // process the common part between current and new v_glob
 for( Index i = 0 ; ( i < v_glob.size() ) && ( i < state.v_glob.size() ) ;
      ++i ) {
  if( v_glob[ i ] < 0 ) {
   Rmvd.push_back( i );
   if( state.v_glob[ i ] < Inf< int >() )
    Addd.push_back( i );
   continue;
   }

  if( v_glob[ i ] == state.v_glob[ i ] )
   continue;

  if( v_glob[ i ] == Inf< int >() ) {
   if( state.v_glob[ i ] < Inf< int >() )
    Addd.push_back( i );
   continue;
   }

  Rmvd.push_back( i );
  if( state.v_glob[ i ] < Inf< int >() )
   Addd.push_back( i );
  }

 // process what is in the new but not in the current (if any)
 for( Index i = v_glob.size() ; i < state.v_glob.size() ; ++i )
  if( state.v_glob[ i ] < Inf< int >() )
   Addd.push_back( i );

 // process what is in the current but not in the new (if any)
 for( Index i = state.v_glob.size() ; i < v_glob.size() ; ++i )
  if( v_glob[ i ] < Inf< int >() )
   Rmvd.push_back( i );

 return( std::make_pair( Rmvd , Addd ) );

 }  // end( PolyhedralFunction::guts_of_put_State )

/*--------------------------------------------------------------------------*/

Function::FunctionValue * PolyhedralFunction::get_ai( Index name )
{
 int gn = name >= v_glob.size() ? v_ord[ f_next ] : v_glob[ name ];

 #if( EXPLICIT_BOUND )
  if( ! gn )             // bound
   return( nullptr );    // == all-0 row
 #endif

 if( gn < 0 )            // aggregated linearization
  return( v_aA[ - gn - 1 ].data() );

 if( gn <= int( get_nrows() ) )  // original linearization
  return( v_A[ --gn ].data() );

 throw( std::invalid_argument(
	        "PolyhedralFunction::get_ai: invalid linearization name" ) );

 return( nullptr );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::reset_aggregate_linearizations( void )
{
 if( v_aA.empty() )
  return;

 v_aA.clear();
 v_ab.clear();

 for( Index i = 0 ; i < f_max_glob ; ++i )
  if( v_glob[ i ] < 0 )
   v_glob[ i ] = Inf< int >();

 update_f_max_glob();

 }  // end( PolyhedralFunction::reset_aggregate_linearizations( void ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::reset_aggregate_linearizations( ModParam issueMod )
{
 if( v_aA.empty() )
  return;

 v_aA.clear();
 v_ab.clear();

 Subset which;

 for( Index i = 0 ; i < f_max_glob ; ++i )
  if( v_glob[ i ] < 0 ) {
   v_glob[ i ] = Inf< int >();
   which.push_back( i );
   }

 update_f_max_glob();

 f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                std::move( which ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::reset_aggregate_linearizations( ModParam ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PolyhedralFunctionState ----------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionState::deserialize( const netCDF::NcGroup & group )
{
 auto gs = group.getDim( "PolyFunction_GlobSize" );
 if( gs.isNull() )
  throw( std::logic_error( "PolyFunction_GlobSize dimension is required" ) );

 auto ncg = group.getVar( "PolyFunction_Glob" );
 if( ncg.isNull() )
  throw( std::logic_error( "PolyFunction_Glob not found" ) );

 v_glob.resize( gs.getSize() );
 ncg.getVar( v_glob.data() );

 auto nv = group.getDim( "PolyFunction_NumVar" );
 if( nv.isNull() )
  throw( std::logic_error( "PolyFunction_NumVar dimension is required" ) );

 f_nvar = nv.getSize();

 auto nr = group.getDim( "PolyFunction_ANumRow" );
 if( ( ! nr.isNull() ) && ( nr.getSize() ) ) {
   auto ncdA = group.getVar( "PolyFunction_aA" );
   if( ncdA.isNull() )
    throw( std::logic_error( "PolyFunction_aA not found" ) );

   auto ncdb = group.getVar( "PolyFunction_ab" );
   if( ncdb.isNull() )
    throw( std::logic_error( "PolyFunction_ab not found" ) );

  v_aA.resize( nr.getSize() );
  for( PolyhedralFunction::Index i = 0 ; i < v_aA.size() ; ++i ) {
   v_aA[ i ].resize( f_nvar );
   ncdA.getVar( { i , 0 } , { 1 , f_nvar } , v_aA[ i ].data() );
   }

  v_ab.resize( nr.getSize() );
  ncdb.getVar( v_ab.data() );
  }
 else {
  v_aA.clear();
  v_ab.clear();
  }

 auto nic = group.getDim( "PolyFunction_ImpCoeffNum" );
 if( ( ! nic.isNull() ) && ( nic.getSize() ) ) {
  f_imp_coeff.resize( nic.getSize() );

  auto ncCI = group.getVar( "PolyFunction_ImpCoeffInd" );
  if( ncCI.isNull() )
   throw( std::logic_error( "PolyFunction_ImpCoeffInd not found" ) );

  auto ncCV = group.getVar( "PolyFunction_ImpCoeffVal" );
  if( ncCV.isNull() )
   throw( std::logic_error( "PolyFunction_ImpCoeffVal not found" ) );

  for( PolyhedralFunction::Index i = 0 ; i < f_imp_coeff.size() ; ++i ) {
   ncCI.getVar( { i } , &(f_imp_coeff[ i ].first) );
   ncCV.getVar( { i } , &(f_imp_coeff[ i ].second) );
   }
  }
 else
  f_imp_coeff.clear();

 }  // end( PolyhedralFunctionState::deserialize )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionState::serialize( netCDF::NcGroup & group ) const
{
 // always call the method of the base class first
 State::serialize( group );

 netCDF::NcDim gs = group.addDim( "PolyFunction_GlobSize" , v_glob.size() );

 ( group.addVar( "PolyFunction_Glob" , netCDF::NcInt() , gs ) ).putVar(
			       { 0 } , {  v_glob.size() } , v_glob.data() );

 netCDF::NcDim nv = group.addDim( "PolyFunction_NumVar" , f_nvar );

 if( ! v_aA.empty() ) {
  netCDF::NcDim nr = group.addDim( "PolyFunction_ANumRow" , v_aA.size() );

  auto ncdA = group.addVar( "PolyFunction_aA" , netCDF::NcDouble() ,
			    { nr , nv } );

  for( PolyhedralFunction::Index i = 0 ; i < v_aA.size() ; ++i )
   ncdA.putVar( { i , 0 } , { 1 , f_nvar } , v_aA[ i ].data() );

  ( group.addVar( "PolyFunction_ab" , netCDF::NcDouble() , nr ) ).putVar(
				    { 0 } , {  v_ab.size() } , v_ab.data() );
  }

 if( ! f_imp_coeff.empty() ) {
  netCDF::NcDim cn = group.addDim( "PolyFunction_ImpCoeffNum" ,
				   f_imp_coeff.size() );
  
  auto ncCI = group.addVar( "PolyFunction_ImpCoeffInd" , netCDF::NcInt() ,
			    cn );

  auto ncCV = group.addVar( "PolyFunction_ImpCoeffVal" , netCDF::NcDouble() ,
			    cn );
 
  for( PolyhedralFunction::Index i = 0 ; i < f_imp_coeff.size() ; ++i ) {
   ncCI.putVar( { i } , f_imp_coeff[ i ].first );
   ncCV.putVar( { i } , f_imp_coeff[ i ].second );
   }
  }
 }  // end( PolyhedralFunctionState::serialize )

/*--------------------------------------------------------------------------*/

Block * PolyhedralFunction::build_easy_Block( bool primal )
{
 // package this PolyhedralFunction inside a freshly-allocated
 // PolyhedralFunctionBlock. The new sub-Block wraps a copy of (A, b)
 // and shares the same active ColVariable pointers, so any change to
 // those Variable propagates to both representations. The convexity
 // flag and the global bound of this PolyhedralFunction are mirrored
 // into the wrapped function as well

 auto * pfb = new PolyhedralFunctionBlock;

 // forward the convex/concave flag, the (per-row) "is_vertical"
 // bitmask, and the global bound. set_PolyhedralFunction also takes
 // the (A, b) by move, but we want this PolyhedralFunction to remain
 // usable on its own after the call so we feed it a copy
 auto & inner = pfb->get_PolyhedralFunction();
 PolyhedralFunction::MultiVector A_copy = v_A;
 PolyhedralFunction::RealVector b_copy = v_b;
 PolyhedralFunction::BoolVector iv_copy = v_is_vert;
 inner.set_PolyhedralFunction( std::move( A_copy ) , std::move( b_copy ) ,
                               f_bound , f_is_convex , eNoMod ,
                               std::move( iv_copy ) );

 // share the same ColVariable pointers: the caller of build_easy_Block
 // typically grafts the returned Block under its own tree and expects
 // the inner f_polyf to react to changes on this PolyhedralFunction's
 // active Variable. set_variables takes the pointer vector by move
 PolyhedralFunction::VarVector vars_copy;
 vars_copy.reserve( v_x.size() );
 for( auto * v : v_x )
  vars_copy.push_back( v );
 inner.set_variables( std::move( vars_copy ) );

 // generate the requested abstract representation. The SimpleConfiguration
 // is treated bit-wise by PolyhedralFunctionBlock::generate_abstract_variables:
 //   1 = bit 0 only           -> linearized primal (epigraph rep)
 //   3 = bit 0 and bit 1 set  -> linearized dual (simplex-on-rows rep)
 SimpleConfiguration< int > rep( primal ? 1 : 3 );
 pfb->generate_abstract_variables( & rep );
 pfb->generate_abstract_constraints();
 pfb->generate_objective();

 return( pfb );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File PolyhedralFunction.cpp ----------------------*/
/*--------------------------------------------------------------------------*/

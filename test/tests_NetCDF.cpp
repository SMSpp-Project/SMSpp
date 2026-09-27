/** @file
 * Unit tests for the netCDF format of the components of the core.
 *
 * Each component is written to a netCDF group with serialize() and read back
 * with deserialize() (or with the factory, as Block::new_Block() does), and
 * what is read is compared with the original: the sizes, the coefficients,
 * the bounds, the sides, the values. The components are the rows
 * (FRowConstraint), the bounds (the OneVarConstraint family) and the
 * FRealObjective of an AbstractBlock, which travel as the LP file of the
 * model, the PolyhedralFunctionBlock, the BendersBFunction with its
 * sub-Block and the LagBFunction; the empty cases, zero rows or zero
 * variables, are there too. For the :Function that have a State, the State
 * goes the same way, both the one of get_State() and the one written by
 * serialize_State(), and put_State() of what is read back gives a Function
 * with the same global pool.
 *
 * All of this is done with the core alone: no Solver is needed, since the
 * global pools are filled by compute() for a PolyhedralFunction and by
 * reading a State written here for the LagBFunction and the
 * BendersBFunction, which would need a Solver of their sub-Block to compute
 * anything.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "BendersBFunction.h"
#include "ColVariableSolution.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "PolyhedralFunctionBlock.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <tuple>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using MultiVector = PolyhedralFunction::MultiVector;
using RealVector = PolyhedralFunction::RealVector;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the name of the netCDF file the tests write, removed at the end

static const std::string & nc_file( void )
{
 static const std::string f = ( std::filesystem::temp_directory_path() /
  ( "smspp_NetCDF_test_" + std::to_string(
       std::chrono::steady_clock::now().time_since_epoch().count() ) +
    ".nc4" ) ).string();
 return( f );
 }

/*--------------------------------------------------------------------------*/
/// the netCDF round trip of a Block through the factory

static Block * round_trip( const Block & b )
{
 {
  netCDF::NcFile f( nc_file() , netCDF::NcFile::replace );
  auto g = f.addGroup( "B" );
  b.serialize( g );
  }
 netCDF::NcFile f( nc_file() , netCDF::NcFile::read );
 return( Block::new_Block( f.getGroup( "B" ) ) );
 }

/*--------------------------------------------------------------------------*/
/// writes a State with the given writer and reads it back with the factory
/** \p write gets the group, and has to write in it a State. */

template< class W >
static State * state_round_trip( W write )
{
 {
  netCDF::NcFile f( nc_file() , netCDF::NcFile::replace );
  auto g = f.addGroup( "S" );
  write( g );
  }
 netCDF::NcFile f( nc_file() , netCDF::NcFile::read );
 return( State::new_State( f.getGroup( "S" ) ) );
 }

/*--------------------------------------------------------------------------*/
/// true if a and b are the same number, the infinite ones included

static bool near( double a , double b )
{
 if( a == b )
  return( true );
 return( std::abs( a - b ) <= 1e-12 * std::max( 1.0 , std::abs( a ) ) );
 }

/*--------------------------------------------------------------------------*/
/// the model an AbstractBlock describes, independent of how it is grouped
/** The columns in the order of the groups of Variable, each with its being
 * integer and its bounds, the ones the ColVariable has of its own and those
 * of the OneVarConstraint on it together; the rows as the sorted list of
 * the one-sided or equality rows, a row with two different finite sides
 * being two of them, as the LP format writes it; the sense, the
 * coefficients and the constant of the Objective. */

struct Model {
 using Row = std::tuple< std::vector< std::pair< Index , double > > ,
			 double , double >;

 std::vector< bool > integer;
 std::vector< double > lb;
 std::vector< double > ub;
 std::vector< Row > rows;
 int sense = Objective::eMin;
 std::vector< double > obj;
 double obj_const = 0;
 };

static Model model_of( const Block & b )
{
 Model m;
 std::map< const Variable * , Index > idx;

 b.for_each_variable_group( [ & ]( const BaseGroup & g ) {
   for( Index i = 0 ; i < g.get_num_elements() ; ++i ) {
    auto v = dynamic_cast< const ColVariable * >( g.get_Variable( i ) );
    assert( v );
    idx[ v ] = m.lb.size();
    m.integer.push_back( v->is_integer() );
    m.lb.push_back( v->is_positive() ? 0 : - Inf< double >() );
    m.ub.push_back( v->is_negative() ? 0 : Inf< double >() );
    }
   } );

 auto coefficients = [ & idx ]( const Function * f ) {
  std::vector< std::pair< Index , double > > c;
  auto lf = dynamic_cast< const LinearFunction * >( f );
  assert( lf );
  for( const auto & [ var , coeff ] : lf->get_v_var() )
   if( coeff != 0 )
    c.emplace_back( idx.at( var ) , coeff );
  std::sort( c.begin() , c.end() );
  return( c );
  };

 b.for_each_constraint_group( [ & ]( const BaseGroup & g ) {
   for( Index i = 0 ; i < g.get_num_elements() ; ++i ) {
    auto c = g.get_Constraint( i );
    if( auto o = dynamic_cast< const OneVarConstraint * >( c ) ) {
     auto k = idx.at( o->get_active_var( 0 ) );
     m.lb[ k ] = std::max( m.lb[ k ] , o->get_lhs() );
     m.ub[ k ] = std::min( m.ub[ k ] , o->get_rhs() );
     continue;
     }
    auto r = dynamic_cast< const FRowConstraint * >( c );
    assert( r );
    auto coeffs = coefficients( r->get_function() );
    const double lhs = r->get_lhs();
    const double rhs = r->get_rhs();
    if( lhs == rhs )
     m.rows.emplace_back( coeffs , lhs , rhs );
    else {
     if( lhs > - Inf< double >() )
      m.rows.emplace_back( coeffs , lhs , Inf< double >() );
     if( rhs < Inf< double >() )
      m.rows.emplace_back( coeffs , - Inf< double >() , rhs );
     }
    }
   } );
 std::sort( m.rows.begin() , m.rows.end() );

 m.obj.assign( m.lb.size() , 0 );
 if( auto o = dynamic_cast< const FRealObjective * >( b.get_objective() ) ) {
  m.sense = o->get_sense();
  for( auto & [ k , c ] : coefficients( o->get_function() ) )
   m.obj[ k ] = c;
  m.obj_const = static_cast< const LinearFunction * >( o->get_function() )
   ->get_constant_term();
  }

 return( m );
 }

/*--------------------------------------------------------------------------*/
/// true if the two models are the same

static bool same( const Model & a , const Model & b )
{
 if( ( a.integer != b.integer ) || ( a.lb.size() != b.lb.size() ) ||
     ( a.rows.size() != b.rows.size() ) || ( a.sense != b.sense ) ||
     ( ! near( a.obj_const , b.obj_const ) ) )
  return( false );
 for( std::size_t j = 0 ; j < a.lb.size() ; ++j )
  if( ( ! near( a.lb[ j ] , b.lb[ j ] ) ) ||
      ( ! near( a.ub[ j ] , b.ub[ j ] ) ) ||
      ( ! near( a.obj[ j ] , b.obj[ j ] ) ) )
   return( false );
 for( std::size_t i = 0 ; i < a.rows.size() ; ++i ) {
  const auto & [ ca , la , ra ] = a.rows[ i ];
  const auto & [ cb , lb , rb ] = b.rows[ i ];
  if( ( ca.size() != cb.size() ) || ( ! near( la , lb ) ) ||
      ( ! near( ra , rb ) ) )
   return( false );
  for( std::size_t k = 0 ; k < ca.size() ; ++k )
   if( ( ca[ k ].first != cb[ k ].first ) ||
       ( ! near( ca[ k ].second , cb[ k ].second ) ) )
    return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/// a LinearFunction with the given coefficients on the given ColVariable

static LinearFunction * linear( std::vector< ColVariable > & x ,
				const std::vector< double > & c ,
				double constant = 0 )
{
 LinearFunction::v_coeff_pair p;
 for( std::size_t j = 0 ; j < c.size() ; ++j )
  if( c[ j ] != 0 )
   p.push_back( { & x[ j ] , c[ j ] } );
 return( new LinearFunction( std::move( p ) , constant ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE TESTS --------------------------------*/
/*--------------------------------------------------------------------------*/
/* The rows, the bounds and the Objective of an AbstractBlock: an equality
 * row, a one-sided one and a two-sided one, a BoxConstraint, an
 * LBConstraint and an UBConstraint, an integer column, a free one, and an
 * Objective to be maximised. */

static void test_abstract_block( void )
{
 AbstractBlock block;

 auto x = new std::vector< ColVariable >( 4 );
 ( *x )[ 1 ].set_type( ColVariable::kNatural );
 block.add_static_variable( *x , "x" );

 auto rows = new std::vector< FRowConstraint >( 3 );
 ( *rows )[ 0 ].set_function( linear( *x , { 1 , 2 , 0 , 0 } ) );
 ( *rows )[ 0 ].set_both( 3 );
 ( *rows )[ 1 ].set_function( linear( *x , { 0 , 0 , -3 , 0.5 } ) );
 ( *rows )[ 1 ].set_lhs( - Inf< double >() );
 ( *rows )[ 1 ].set_rhs( 7 );
 ( *rows )[ 2 ].set_function( linear( *x , { 1 , 0 , -1 , 0 } ) );
 ( *rows )[ 2 ].set_lhs( 1 );
 ( *rows )[ 2 ].set_rhs( 4 );
 block.add_static_constraint( *rows , "r" );

 auto box = new BoxConstraint( & block , & ( *x )[ 0 ] , -1 , 5 );
 block.add_static_constraint( *box , "box" );
 auto lb = new LBConstraint( & block , & ( *x )[ 2 ] , -2 );
 block.add_static_constraint( *lb , "lb" );
 auto ub = new UBConstraint( & block , & ( *x )[ 3 ] , 8 );
 block.add_static_constraint( *ub , "ub" );
 auto lb3 = new LBConstraint( & block , & ( *x )[ 3 ] , -4 );
 block.add_static_constraint( *lb3 , "lb3" );

 auto obj = new FRealObjective( & block ,
				linear( *x , { 5 , -1 , 2 , 1 } ) );
 obj->set_sense( Objective::eMax , eNoMod );
 block.set_objective( obj , eNoMod );

 const auto original = model_of( block );
 assert( original.lb.size() == 4 );
 assert( original.rows.size() == 4 );

 auto read = round_trip( block );
 assert( dynamic_cast< AbstractBlock * >( read ) );
 const auto copy = model_of( *read );
 assert( same( copy , original ) );
 assert( copy.integer[ 1 ] && ( copy.lb[ 1 ] == 0 ) );
 assert( ( copy.lb[ 0 ] == -1 ) && ( copy.ub[ 0 ] == 5 ) );
 assert( ( copy.lb[ 2 ] == -2 ) && ( copy.ub[ 2 ] == Inf< double >() ) );
 assert( ( copy.lb[ 3 ] == -4 ) && ( copy.ub[ 3 ] == 8 ) );
 assert( copy.sense == Objective::eMax );

 // and a second trip gives the same again
 auto again = round_trip( *read );
 assert( again && same( model_of( *again ) , original ) );
 delete again;
 delete read;

 // a column with no lower bound and a finite upper bound u, which the LP
 // format has to be told is -inf <= x <= u, a lower bound that is not
 // written being 0 to it
 {
  AbstractBlock only_ub;
  auto y = new std::vector< ColVariable >( 1 );
  only_ub.add_static_variable( *y , "y" );
  auto u = new UBConstraint( & only_ub , & ( *y )[ 0 ] , 8 );
  only_ub.add_static_constraint( *u , "u" );
  only_ub.set_objective( new FRealObjective( & only_ub ,
					     linear( *y , { 1 } ) ) , eNoMod );
  auto r = round_trip( only_ub );
  assert( r && same( model_of( *r ) , model_of( only_ub ) ) );
  delete r;
  }

 // the AbstractBlock owns what was added to it, and deletes it
 std::cout << "rows, bounds and Objective of an AbstractBlock: OK"
	   << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The empty cases of an AbstractBlock: nothing at all, columns and no row,
 * an Objective and no row. */

static void test_empty_abstract_block( void )
{
 {
  AbstractBlock block;
  auto read = round_trip( block );
  assert( dynamic_cast< AbstractBlock * >( read ) );
  const auto m = model_of( *read );
  assert( m.lb.empty() && m.rows.empty() && ( ! read->get_objective() ) );
  assert( ! read->get_number_nested_Blocks() );
  delete read;
  }
 {
  AbstractBlock block;
  auto x = new std::vector< ColVariable >( 2 );
  block.add_static_variable( *x , "x" );
  auto obj = new FRealObjective( & block , linear( *x , { 1 , -1 } ) );
  block.set_objective( obj , eNoMod );

  auto read = round_trip( block );
  assert( read && same( model_of( *read ) , model_of( block ) ) );
  delete read;
  }
 {
  // an empty group of rows
  AbstractBlock block;
  auto x = new std::vector< ColVariable >( 2 );
  block.add_static_variable( *x , "x" );
  auto rows = new std::vector< FRowConstraint >();
  block.add_static_constraint( *rows , "none" );
  block.set_objective( new FRealObjective( & block ,
					   linear( *x , { 2 , 3 } ) ) ,
		       eNoMod );

  auto read = round_trip( block );
  assert( read && same( model_of( *read ) , model_of( block ) ) );
  delete read;
  }

 // a column that is in no row and has a zero coefficient in the Objective
 {
  AbstractBlock block;
  auto x = new std::vector< ColVariable >( 2 );
  block.add_static_variable( *x , "x" );
  block.set_objective( new FRealObjective( & block ,
					   linear( *x , { 0 , 3 } ) ) ,
		       eNoMod );

  auto read = round_trip( block );
  assert( read && same( model_of( *read ) , model_of( block ) ) );
  delete read;
  }

 // columns and no Objective
 {
  AbstractBlock block;
  auto x = new std::vector< ColVariable >( 2 );
  block.add_static_variable( *x , "x" );

  auto read = round_trip( block );
  assert( read && same( model_of( *read ) , model_of( block ) ) );
  delete read;
  }

 // an Objective whose coefficients are all zero, the columns being in a row
 // whose coefficients are all zero too: both are written as the constant 0
 {
  AbstractBlock block;
  auto x = new std::vector< ColVariable >( 2 );
  block.add_static_variable( *x , "x" );
  auto rows = new std::vector< FRowConstraint >( 2 );
  ( *rows )[ 0 ].set_function( linear( *x , { 1 , 1 } ) );
  ( *rows )[ 0 ].set_lhs( - Inf< double >() );
  ( *rows )[ 0 ].set_rhs( 4 );
  ( *rows )[ 1 ].set_function( linear( *x , { 0 , 0 } ) );
  ( *rows )[ 1 ].set_lhs( -1 );
  ( *rows )[ 1 ].set_rhs( Inf< double >() );
  block.add_static_constraint( *rows , "r" );
  block.set_objective( new FRealObjective( & block ,
					   linear( *x , { 0 , 0 } ) ) ,
		       eNoMod );

  auto read = round_trip( block );
  assert( read && same( model_of( *read ) , model_of( block ) ) );
  delete read;
  }

 // an LP file that ends before its End section, or has a word out of its
 // place, is refused rather than read forever
 for( const char * lp : { "Minimize\n obj: 0\n" ,
			  "Minimize\n obj: x\nSubject To\n c: x >= 1\n" ,
			  "Minimize\n obj: x\nSubject To\n c: x >= 1\n"
			  "Bounds\n x <= 3\n" ,
			  "Minimize\n obj: x\nSubject To\n c: x >= 1\n"
			  "Bounds\n x <= <= 3\nEnd\n" ,
			  "Minimize\n obj: x\nSubject To\n x >= 1\nEnd\n" ,
			  "Maximise\n obj: x\nSubject To\nEnd\n" } ) {
  AbstractBlock block;
  std::istringstream in( lp );
  bool refused = false;
  try { block.load( in , 'L' ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  }

 // what read_lp() takes that write_lp() does not write: a constant in the
 // Objective and in a row, a column named in the Bounds section alone, the
 // bounds with the sense turned, the default bounds of a column
 {
  AbstractBlock block;
  std::istringstream in( "Maximize\n obj: 2 x + 3 + y\nSubject To\n"
			 " c: x + 1 <= 5\n d: 0 >= -2\nBounds\n"
			 " 4 >= x >= -1\n z <= 6\nEnd\n" );
  block.load( in , 'L' );
  const auto m = model_of( block );
  assert( m.lb.size() == 3 );
  assert( ( m.obj[ 0 ] == 2 ) && ( m.obj[ 1 ] == 1 ) && ( m.obj[ 2 ] == 0 ) );
  assert( ( m.obj_const == 3 ) && ( m.sense == Objective::eMax ) );
  assert( ( m.lb[ 0 ] == -1 ) && ( m.ub[ 0 ] == 4 ) );
  assert( ( m.lb[ 1 ] == 0 ) && ( m.ub[ 1 ] == Inf< double >() ) );
  assert( ( m.lb[ 2 ] == 0 ) && ( m.ub[ 2 ] == 6 ) );
  assert( m.rows.size() == 2 );
  // sorted: the row with no coefficient comes first
  assert( std::get< 0 >( m.rows[ 0 ] ).empty() &&
	  ( std::get< 1 >( m.rows[ 0 ] ) == -2 ) );
  assert( ( std::get< 0 >( m.rows[ 1 ] ).size() == 1 ) &&
	  ( std::get< 2 >( m.rows[ 1 ] ) == 4 ) );
  }

 std::cout << "empty AbstractBlock: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* PolyhedralFunctionBlock: the matrix, the constants, the verse and the
 * bound of the PolyhedralFunction, and the empty cases. */

static void check_pf( PolyhedralFunction & a , PolyhedralFunction & b )
{
 assert( a.get_A() == b.get_A() );
 assert( a.get_b() == b.get_b() );
 assert( a.get_nrows() == b.get_nrows() );
 assert( a.is_convex() == b.is_convex() );
 assert( a.is_bound_set() == b.is_bound_set() );
 if( a.is_bound_set() )
  assert( a.get_global_bound() == b.get_global_bound() );
 }

static void test_polyhedral_function_block( void )
{
 std::vector< ColVariable > x( 3 );

 // a convex max with a bound, a concave min with none
 for( bool convex : { true , false } ) {
  auto pfb = new PolyhedralFunctionBlock();
  auto & pf = pfb->get_PolyhedralFunction();
  pf.set_variables( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] } );
  pf.set_PolyhedralFunction( { { 1 , 2 , 0 } , { -3 , 0.5 , 4 } } ,
			     { 0 , 1.5 } ,
			     convex ? -10 : Inf< double >() , convex ,
			     eNoMod );

  auto read = dynamic_cast< PolyhedralFunctionBlock * >( round_trip( *pfb ) );
  assert( read );
  auto & rpf = read->get_PolyhedralFunction();
  check_pf( rpf , pf );
  assert( rpf.get_A()[ 0 ].size() == 3 );

  // the Variable are not part of the format: those given afterwards must
  // be as many as the columns of A
  rpf.set_variables( { & x[ 0 ] , & x[ 1 ] , & x[ 2 ] } );
  assert( rpf.get_num_active_var() == 3 );
  delete read;
  delete pfb;
  }

 // zero rows
 {
  auto pfb = new PolyhedralFunctionBlock();
  auto & pf = pfb->get_PolyhedralFunction();
  pf.set_variables( { & x[ 0 ] , & x[ 1 ] } );
  pf.set_PolyhedralFunction( {} , {} , -1 , true , eNoMod );

  auto read = dynamic_cast< PolyhedralFunctionBlock * >( round_trip( *pfb ) );
  assert( read );
  check_pf( read->get_PolyhedralFunction() , pf );
  assert( ! read->get_PolyhedralFunction().get_nrows() );
  delete read;
  delete pfb;
  }

 // zero variables: a constant function, the max of the constants
 {
  auto pfb = new PolyhedralFunctionBlock();
  auto & pf = pfb->get_PolyhedralFunction();
  pf.set_PolyhedralFunction( { {} , {} } , { 2 , 3 } , - Inf< double >() ,
			     true , eNoMod );

  auto read = dynamic_cast< PolyhedralFunctionBlock * >( round_trip( *pfb ) );
  assert( read );
  check_pf( read->get_PolyhedralFunction() , pf );
  assert( read->get_PolyhedralFunction().get_nrows() == 2 );
  delete read;
  delete pfb;
  }

 // which rows are vertical linearizations goes in the file too
 {
  auto pfb = new PolyhedralFunctionBlock();
  auto & pf = pfb->get_PolyhedralFunction();
  pf.set_variables( { & x[ 0 ] , & x[ 1 ] } );
  pf.set_PolyhedralFunction( { { 1 , 0 } , { 0 , 1 } } , { 0 , -1 } ,
			     - Inf< double >() , true , eNoMod ,
			     { false , true } );
  auto read = dynamic_cast< PolyhedralFunctionBlock * >( round_trip( *pfb ) );
  assert( read );
  assert( read->get_PolyhedralFunction().get_is_vert() == pf.get_is_vert() );
  delete read;
  delete pfb;
  }

 std::cout << "PolyhedralFunctionBlock: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The State of a PolyhedralFunction: a global pool with two linearizations
 * and a combination of them, and the important linearization; the State of
 * get_State() and the one of serialize_State() both give it back. */

/// a PolyhedralFunction on x with the data used for the State

static void fill_pf( PolyhedralFunction & pf , std::vector< ColVariable > & x )
{
 pf.set_variables( { & x[ 0 ] , & x[ 1 ] } );
 pf.set_PolyhedralFunction( { { 1 , 2 } , { -1 , 1 } , { 0.5 , -3 } } ,
			    { 0 , 1 , 2 } , - Inf< double >() , true ,
			    eNoMod );
 pf.set_par( PolyhedralFunction::intGPMaxSz , 4 );
 }

/// true if the two PolyhedralFunction have the same global pool

static bool same_pool( PolyhedralFunction & a , PolyhedralFunction & b )
{
 if( a.get_int_par( PolyhedralFunction::intGPMaxSz ) !=
     b.get_int_par( PolyhedralFunction::intGPMaxSz ) )
  return( false );
 const Index n = a.get_int_par( PolyhedralFunction::intGPMaxSz );
 for( Index i = 0 ; i < n ; ++i ) {
  if( a.is_linearization_there( i ) != b.is_linearization_there( i ) )
   return( false );
  if( ! a.is_linearization_there( i ) )
   continue;
  if( a.is_linearization_vertical( i ) != b.is_linearization_vertical( i ) )
   return( false );
  if( ! near( a.get_linearization_constant( i ) ,
	      b.get_linearization_constant( i ) ) )
   return( false );
  std::vector< double > ga( 2 ) , gb( 2 );
  a.get_linearization_coefficients( ga.data() ,
				    PolyhedralFunction::Range( 0 , 2 ) , i );
  b.get_linearization_coefficients( gb.data() ,
				    PolyhedralFunction::Range( 0 , 2 ) , i );
  if( ga != gb )
   return( false );
  }
 return( a.get_important_linearization_coefficients() ==
	 b.get_important_linearization_coefficients() );
 }

static void test_polyhedral_function_state( void )
{
 // a new PolyhedralFunction has the default tolerance on the multipliers
 {
  PolyhedralFunction fresh;
  assert( fresh.get_dbl_par( PolyhedralFunction::dblAAccMlt ) ==
	  fresh.get_dflt_dbl_par( PolyhedralFunction::dblAAccMlt ) );
  }

 std::vector< ColVariable > x( 2 );
 PolyhedralFunction pf;
 fill_pf( pf , x );

 // an empty pool first
 {
  std::unique_ptr< State > s( pf.get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & s ]( auto & g ) {
			      s->serialize( g ); } ) );
  assert( dynamic_cast< PolyhedralFunctionState * >( r.get() ) );
  PolyhedralFunction other;
  fill_pf( other , x );
  other.put_State( *r );
  assert( same_pool( other , pf ) );
  }

 // two linearizations, in x = ( 1 , 1 ) and in x = ( -1 , 0 ), and their
 // combination with weights 1/2
 x[ 0 ].set_value( 1 );
 x[ 1 ].set_value( 1 );
 pf.compute();
 pf.store_linearization( 0 );
 x[ 0 ].set_value( -1 );
 x[ 1 ].set_value( 0 );
 pf.compute();
 pf.store_linearization( 1 );
 pf.store_combination_of_linearizations( { { 0 , 0.5 } , { 1 , 0.5 } } , 2 );
 pf.set_important_linearization( { { 0 , 0.25 } , { 2 , 0.75 } } );
 assert( pf.is_linearization_there( 0 ) && pf.is_linearization_there( 2 ) );
 assert( ! pf.is_linearization_there( 3 ) );

 // the State of get_State()
 {
  std::unique_ptr< State > s( pf.get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & s ]( auto & g ) {
			      s->serialize( g ); } ) );
  assert( r );
  PolyhedralFunction other;
  fill_pf( other , x );
  other.put_State( *r );
  assert( same_pool( other , pf ) );

  // and the one moved in
  PolyhedralFunction moved;
  fill_pf( moved , x );
  moved.put_State( std::move( *r ) );
  assert( same_pool( moved , pf ) );
  }

 // the State written by serialize_State()
 {
  std::unique_ptr< State > r( state_round_trip( [ & pf ]( auto & g ) {
			      pf.serialize_State( g ); } ) );
  assert( dynamic_cast< PolyhedralFunctionState * >( r.get() ) );
  PolyhedralFunction other;
  fill_pf( other , x );
  other.put_State( *r );
  assert( same_pool( other , pf ) );
  }

 std::cout << "State of a PolyhedralFunction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* BendersBFunction: the mapping, the sides, the paths to the RowConstraint
 * of the sub-Block and the sub-Block itself; zero rows and zero variables
 * too. */

/// the sub-Block: m rows on two ColVariable, and an Objective

static AbstractBlock * sub_Block( Index m ,
				  std::vector< RowConstraint * > & rows )
{
 auto b = new AbstractBlock();
 auto y = new std::vector< ColVariable >( 2 );
 ( *y )[ 0 ].is_positive( true , eNoMod );
 b->add_static_variable( *y , "y" );

 auto r = new std::vector< FRowConstraint >( m );
 for( Index i = 0 ; i < m ; ++i ) {
  ( *r )[ i ].set_function( linear( *y , { 1.0 + i , -1 } ) );
  ( *r )[ i ].set_lhs( - double( i ) );
  ( *r )[ i ].set_rhs( double( i ) + 1 );
  }
 b->add_static_constraint( *r , "r" );
 rows.clear();
 for( auto & row : *r )
  rows.push_back( & row );

 b->set_objective( new FRealObjective( b , linear( *y , { 1 , 2 } ) ) ,
		   eNoMod );
 return( b );
 }

/// the BendersBFunction and the data it was built with

struct Benders {
 std::vector< ColVariable > x;
 BendersBFunction * f;

 Benders( MultiVector A , RealVector b ,
	  BendersBFunction::ConstraintSideVector sides , Index nx = 2 )
  : x( nx ) {
  std::vector< RowConstraint * > rows;
  auto inner = sub_Block( A.size() , rows );
  BendersBFunction::VarVector vars;
  for( auto & v : x )
   vars.push_back( & v );
  f = new BendersBFunction( inner , std::move( vars ) , std::move( A ) ,
			    std::move( b ) , std::move( rows ) ,
			    std::move( sides ) );
  }

 ~Benders() { delete f; }

 /// a BendersBFunction on the same x, read out of the netCDF format of f
 BendersBFunction * read( void ) {
  {
   netCDF::NcFile file( nc_file() , netCDF::NcFile::replace );
   auto g = file.addGroup( "B" );
   f->serialize( g );
   }
  auto r = new BendersBFunction();
  BendersBFunction::VarVector vars;
  for( auto & v : x )
   vars.push_back( & v );
  r->set_variables( std::move( vars ) );
  netCDF::NcFile file( nc_file() , netCDF::NcFile::read );
  r->deserialize( file.getGroup( "B" ) );
  return( r );
  }
 };

static void check_benders( BendersBFunction & r , BendersBFunction & f )
{
 assert( r.get_A() == f.get_A() );
 assert( r.get_b() == f.get_b() );
 assert( r.get_sides() == f.get_sides() );
 assert( r.get_constraints().size() == f.get_constraints().size() );
 assert( r.get_num_active_var() == f.get_num_active_var() );
 for( Index j = 0 ; j < f.get_num_active_var() ; ++j )
  assert( r.get_active_var( j ) == f.get_active_var( j ) );
 assert( r.get_inner_block() && f.get_inner_block() );
 assert( same( model_of( *r.get_inner_block() ) ,
	       model_of( *f.get_inner_block() ) ) );
 }

static void test_benders_function( void )
{
 // A dense, both kinds of sides
 {
  Benders b( { { 1 , 2 } , { 3 , -4 } } , { 5 , 6 } ,
	     { BendersBFunction::eLHS , BendersBFunction::eBoth } );
  auto r = b.read();
  check_benders( *r , *b.f );
  delete r;
  }

 // zero rows
 {
  Benders b( {} , {} , {} );
  auto r = b.read();
  check_benders( *r , *b.f );
  assert( r->get_A().empty() );
  delete r;
  }

 // zero variables and zero rows
 {
  Benders b( {} , {} , {} , 0 );
  auto r = b.read();
  check_benders( *r , *b.f );
  delete r;
  }

 // A sparse enough to be written in the sparse format, with an empty row
 {
  Benders b( { { 1 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 } , { 0 , 0 , 2 , 0 } ,
	       { 0 , 0 , 0 , 3 } } , { 1 , 2 , 3 , 4 } ,
	     { BendersBFunction::eRHS , BendersBFunction::eRHS ,
	       BendersBFunction::eRHS , BendersBFunction::eRHS } , 4 );
  auto r = b.read();
  check_benders( *r , *b.f );
  delete r;
  }

 // the factory, which builds a BendersBFunction with no active Variable:
 // their number is then the one of the netCDF group, and set_variables()
 // afterwards has to give as many
 {
  Benders b( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  auto r = dynamic_cast< BendersBFunction * >( round_trip( *b.f ) );
  assert( r && ( r->get_A() == b.f->get_A() ) );
  assert( r->get_num_active_var() == 0 );
  bool refused = false;
  try { r->set_variables( { & b.x[ 0 ] } ); }
  catch( const std::logic_error & ) { refused = true; }
  assert( refused );
  r->set_variables( { & b.x[ 0 ] , & b.x[ 1 ] } );
  check_benders( *r , *b.f );
  delete r;
  }

 std::cout << "BendersBFunction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The State of a BendersBFunction: a global pool of two places, the first
 * holding a linearization and the second not, and the important
 * linearization. Filling the pool asks a Solver of the sub-Block, hence the
 * State is written here in the format BendersBFunctionState reads. */

static void write_benders_state( netCDF::NcGroup & g )
{
 g.putAtt( "type" , "BendersBFunctionState" );
 auto d = g.addDim( "BendersBFunction_MaxGlob" , 2 );
 std::vector< signed char > type = { 1 , 0 };
 g.addVar( "BendersBFunction_Type" , netCDF::NcByte() , d ).putVar(
							      type.data() );
 std::vector< double > constants = {
  1.5 , std::numeric_limits< double >::quiet_NaN() };
 g.addVar( "BendersBFunction_Constants" , netCDF::NcDouble() , d ).putVar(
							 constants.data() );
 auto c = g.addDim( "BendersBFunction_ImpCoeffNum" , 1 );
 g.addVar( "BendersBFunction_ImpCoeffInd" , netCDF::NcInt() , c ).putVar(
								  { 0 } , 0 );
 g.addVar( "BendersBFunction_ImpCoeffVal" , netCDF::NcDouble() , c ).putVar(
								{ 0 } , 1.0 );
 }

static void check_benders_pool( BendersBFunction & f )
{
 assert( f.is_linearization_there( 0 ) );
 assert( ! f.is_linearization_there( 1 ) );
 assert( ! f.is_linearization_vertical( 0 ) );
 assert( f.get_linearization_constant( 0 ) == 1.5 );
 assert( ( f.get_important_linearization_coefficients() ==
	   C05Function::LinearCombination( { { 0 , 1.0 } } ) ) );
 }

static void test_benders_state( void )
{
 Benders a( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
 std::unique_ptr< State > s( state_round_trip( write_benders_state ) );
 assert( dynamic_cast< BendersBFunctionState * >( s.get() ) );
 a.f->put_State( *s );
 check_benders_pool( *a.f );

 // the State of get_State()
 {
  std::unique_ptr< State > g( a.f->get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & g ]( auto & grp ) {
			      g->serialize( grp ); } ) );
  Benders b( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  b.f->put_State( *r );
  check_benders_pool( *b.f );
  }

 // the State written by serialize_State()
 {
  std::unique_ptr< State > r( state_round_trip( [ & a ]( auto & grp ) {
			      a.f->serialize_State( grp ); } ) );
  Benders b( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  b.f->put_State( *r );
  check_benders_pool( *b.f );
  }

 // an empty pool
 {
  Benders e( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  std::unique_ptr< State > g( e.f->get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & g ]( auto & grp ) {
			      g->serialize( grp ); } ) );
  a.f->put_State( *r );
  assert( a.f->get_important_linearization_coefficients().empty() );
  // the places of the pool past those of the State, which it keeps, hold
  // no linearization any more
  assert( a.f->get_int_par( C05Function::intGPMaxSz ) == 2 );
  assert( ! a.f->is_linearization_there( 0 ) );
  assert( ! a.f->is_linearization_there( 1 ) );
  }

 // a State moved in a BendersBFunction whose pool is smaller than the one
 // of the State, and one moved in a BendersBFunction whose pool is larger
 {
  std::unique_ptr< State > r( state_round_trip( write_benders_state ) );
  Benders b( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  b.f->put_State( std::move( *r ) );
  check_benders_pool( *b.f );

  Benders e( { { 1 , 2 } } , { 5 } , { BendersBFunction::eBoth } );
  std::unique_ptr< State > g( e.f->get_State() );
  b.f->put_State( std::move( *g ) );
  assert( b.f->get_int_par( C05Function::intGPMaxSz ) == 2 );
  assert( ! b.f->is_linearization_there( 0 ) );
  }

 std::cout << "State of a BendersBFunction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* LagBFunction: the inner Block and the Lagrangian term, and its State.
 * Filling the global pool asks a Solver of the inner Block, hence the State
 * is written here in the format LagBFunctionState reads, with a Solution of
 * the inner Block in the first place of the pool. */

/// a LagBFunction and its inner Block
/** With terms == 1, one Lagrangian multiplier y and g( x ) = 2 x_0 + x_1;
 * with terms == 2, y with g_0( x ) = -4 x_1 + 1.5 and y2 with g_1( x ) = -2,
 * a function with no term; with terms == 0, none. */

struct Lagrangian {
 ColVariable y;
 ColVariable y2;
 AbstractBlock * inner;
 std::vector< ColVariable > * x;
 LagBFunction * f;

 Lagrangian( int terms = 1 ) {
  inner = new AbstractBlock();
  x = new std::vector< ColVariable >( 2 );
  inner->add_static_variable( *x , "x" );
  inner->set_objective( new FRealObjective( inner ,
					    linear( *x , { 1 , -1 } ) ) ,
			eNoMod );
  f = new LagBFunction( inner );
  LagBFunction::v_dual_pair dp;
  if( terms == 1 )
   dp.emplace_back( & y , linear( *x , { 2 , 1 } ) );
  if( terms == 2 ) {
   dp.emplace_back( & y , linear( *x , { 0 , -4 } , 1.5 ) );
   dp.emplace_back( & y2 , linear( *x , { 0 , 0 } , -2 ) );
   }
  f->set_dual_pairs( std::move( dp ) );
  }

 ~Lagrangian() { delete f; }
 };

static void write_lagrangian_state( netCDF::NcGroup & g , Block * inner ,
				    bool with_value = true )
{
 g.putAtt( "type" , "LagBFunctionState" );
 auto d = g.addDim( "LagBFunction_MaxGlob" , 2 );
 std::vector< signed char > type = { 1 , 0 };
 g.addVar( "LagBFunction_Type" , netCDF::NcByte() , d ).putVar( type.data() );
 if( with_value ) {
  std::vector< double > value = { 2.5 , 0 };
  g.addVar( "LagBFunction_Value" , netCDF::NcDouble() , d ).putVar(
							       value.data() );
  std::vector< signed char > convexified = { 1 , 0 };
  g.addVar( "LagBFunction_Convexified" , netCDF::NcByte() , d ).putVar(
							 convexified.data() );
  }
 ColVariableSolution sol;
 sol.read( inner );
 auto sg = g.addGroup( "LagBFunction_Sol_0" );
 sol.serialize( sg );
 auto c = g.addDim( "LagBFunction_ImpCoeffNum" , 1 );
 g.addVar( "LagBFunction_ImpCoeffInd" , netCDF::NcInt() , c ).putVar(
								  { 0 } , 0 );
 g.addVar( "LagBFunction_ImpCoeffVal" , netCDF::NcDouble() , c ).putVar(
								{ 0 } , 1.0 );
 }

static void check_lagrangian_pool( LagBFunction & f )
{
 assert( f.is_linearization_there( 0 ) );
 assert( ! f.is_linearization_there( 1 ) );
 assert( f.get_linearization_constant( 0 ) == 2.5 );
 assert( ( f.get_important_linearization_coefficients() ==
	   C05Function::LinearCombination( { { 0 , 1.0 } } ) ) );
 }

static void test_lagrangian_function( void )
{
 Lagrangian a;
 ( *a.x )[ 0 ].set_value( 1 );
 ( *a.x )[ 1 ].set_value( 2 );

 std::unique_ptr< State > s( state_round_trip( [ & a ]( auto & g ) {
			     write_lagrangian_state( g , a.inner ); } ) );
 assert( dynamic_cast< LagBFunctionState * >( s.get() ) );
 a.f->put_State( *s );
 check_lagrangian_pool( *a.f );

 // the State of get_State()
 {
  std::unique_ptr< State > g( a.f->get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & g ]( auto & grp ) {
			      g->serialize( grp ); } ) );
  Lagrangian b;
  b.f->put_State( *r );
  check_lagrangian_pool( *b.f );
  Lagrangian c;
  c.f->put_State( std::move( *r ) );
  check_lagrangian_pool( *c.f );
  }

 // the State written by serialize_State(): the pool is the same
 {
  std::unique_ptr< State > r( state_round_trip( [ & a ]( auto & grp ) {
			      a.f->serialize_State( grp ); } ) );
  Lagrangian b;
  b.f->put_State( *r );
  check_lagrangian_pool( *b.f );
  }

 // a State with no LagBFunction_Value and LagBFunction_Convexified, which
 // are optional: the constants are 0
 {
  std::unique_ptr< State > r( state_round_trip( [ & a ]( auto & g ) {
			      write_lagrangian_state( g , a.inner , false );
			      } ) );
  Lagrangian b;
  b.f->put_State( *r );
  assert( b.f->is_linearization_there( 0 ) );
  assert( ! b.f->is_linearization_there( 1 ) );
  assert( b.f->get_linearization_constant( 0 ) == 0 );
  }

 // an empty pool
 {
  Lagrangian e;
  std::unique_ptr< State > g( e.f->get_State() );
  std::unique_ptr< State > r( state_round_trip( [ & g ]( auto & grp ) {
			      g->serialize( grp ); } ) );
  Lagrangian b;
  b.f->put_State( *r );
  assert( b.f->get_important_linearization_coefficients().empty() );
  }

 // a LagBFunction whose pool is larger than the one of the State, and has
 // been made so first: the constants are those of the State as well
 {
  Lagrangian b;
  b.f->set_par( LagBFunction::intGPMaxSz , 3 );
  std::unique_ptr< State > r( state_round_trip( [ & b ]( auto & g ) {
			      write_lagrangian_state( g , b.inner ); } ) );
  b.f->put_State( *r );
  check_lagrangian_pool( *b.f );
  assert( ! b.f->is_linearization_there( 2 ) );
  }

 // the netCDF format: the inner Block and the functions g_i( x ) of the
 // Lagrangian term, whose y are not in the format and are given afterwards
 {
  auto r = dynamic_cast< LagBFunction * >( round_trip( *a.f ) );
  assert( r && ( r->get_num_active_var() == 0 ) );
  assert( same( model_of( *r->get_inner_block() ) , model_of( *a.inner ) ) );

  ColVariable z;
  bool refused = false;
  try { r->set_variables( { & z , & a.y } ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );

  r->set_variables( { & z } );
  assert( ( r->get_num_active_var() == 1 ) &&
	  ( r->get_active_var( 0 ) == & z ) );
  auto g = static_cast< const LinearFunction * >(
					       r->get_Lagrangian_term( 0 ) );
  assert( g->get_num_active_var() == 2 );
  for( const auto & [ x , c ] : g->get_v_var() )
   assert( x->get_Block() == r->get_inner_block() );
  assert( ( g->get_v_var()[ 0 ].second == 2 ) &&
	  ( g->get_v_var()[ 1 ].second == 1 ) );
  assert( g->get_constant_term() == 0 );

  // and a second trip gives the same again
  auto again = dynamic_cast< LagBFunction * >( round_trip( *r ) );
  assert( again && same( model_of( *again->get_inner_block() ) ,
			 model_of( *a.inner ) ) );
  again->set_variables( { & z } );
  assert( again->get_num_active_var() == 1 );
  delete again;
  delete r;
  }

 // two Lagrangian terms, one with a constant and one with no term at all,
 // and none at all
 {
  Lagrangian t( 2 );
  auto r = dynamic_cast< LagBFunction * >( round_trip( *t.f ) );
  assert( r && ( r->get_num_active_var() == 0 ) );
  r->set_variables( { & t.y , & t.y2 } );
  auto g0 = static_cast< const LinearFunction * >(
					       r->get_Lagrangian_term( 0 ) );
  auto g1 = static_cast< const LinearFunction * >(
					       r->get_Lagrangian_term( 1 ) );
  assert( ( g0->get_num_active_var() == 1 ) &&
	  ( g0->get_v_var()[ 0 ].second == -4 ) &&
	  ( g0->get_constant_term() == 1.5 ) );
  assert( ( g1->get_num_active_var() == 0 ) &&
	  ( g1->get_constant_term() == -2 ) );
  delete r;

  Lagrangian e( 0 );
  auto n = dynamic_cast< LagBFunction * >( round_trip( *e.f ) );
  assert( n && ( n->get_num_active_var() == 0 ) );
  n->set_variables( {} );
  assert( n->get_num_active_var() == 0 );
  delete n;
  }

 std::cout << "LagBFunction: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_abstract_block();
 test_empty_abstract_block();
 test_polyhedral_function_block();
 test_polyhedral_function_state();
 test_benders_function();
 test_benders_state();
 test_lagrangian_function();

 std::filesystem::remove( nc_file() );

 std::cout << "all tests passed" << std::endl;

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File tests_NetCDF.cpp -----------------------*/
/*--------------------------------------------------------------------------*/

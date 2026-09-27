/** @file
 * Unit tests for Block and AbstractBlock.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Niccolo' Iardella, Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "FRowConstraint.h"
#include "ColVariable.h"
#include "ColVariableSolution.h"

#include "BlockInspection.h"
#include "FRealObjective.h"
#include "DQuadFunction.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"

#include <netcdf>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void TearDown( AbstractBlock * block )
{
 block->reset_static_constraints();
 assert( block->get_static_constraint_groups().empty() );

 block->reset_static_variables();
 assert( block->get_static_variable_groups().empty() );

 block->reset_dynamic_constraints();
 assert( block->get_dynamic_constraint_groups().empty() );

 block->reset_dynamic_variables();
 assert( block->get_dynamic_variable_groups().empty() );

 delete block;
}

/*--------------------------------------------------------------------------*/

void runAllTests()
{
 // test Sets_UpperBound
 double ub = 1.0;
 AbstractBlock * block = new AbstractBlock(); // SetUp
 assert( block->get_valid_upper_bound() == Inf< double >() );
 block->set_valid_upper_bound( ub , true );
 assert( block->get_valid_upper_bound( true ) == ub );
 TearDown( block ); // TearDown

 // test Sets_LowerBound
 double lb = -1.0;
 block = new AbstractBlock(); // SetUp
 assert( block->get_valid_lower_bound() == -Inf< double >() );
 block->set_valid_lower_bound( lb , true );
 assert( block->get_valid_lower_bound( true ) == lb );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_One
 block = new AbstractBlock(); // SetUp
 auto c = new FRowConstraint();
 block->add_static_constraint( *c );
 assert( block->get_static_constraint< FRowConstraint >( 0 ) == c );
 assert(
  block->get_static_constraint< FRowConstraint >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_Vector: the group is the vector, and each of
 // its elements knows the Block and where it sits in the group
 block = new AbstractBlock(); // SetUp
 auto v_c = new std::vector< FRowConstraint >( 5 );
 block->add_static_constraint( *v_c , "v_c" );
 assert( block->get_static_constraint_v< FRowConstraint >( 0 ) == v_c );
 assert( block->get_number_static_constraints() == 1 );
 assert( block->get_static_constraint_groups()[ 0 ]->get_num_elements() == 5 );
 assert( block->get_static_constraint_groups()[ 0 ]->get_rank() == 1 );
 for( Block::Index i = 0 ; i < v_c->size() ; ++i ) {
  assert( ( *v_c )[ i ].get_Block() == block );
  assert( inspection::get_element< FRowConstraint >( block , true , 0 , i )
	  == & ( *v_c )[ i ] );
  assert( std::get< 2 >( inspection::get_element_index( & ( *v_c )[ i ] ) )
	  == i );
  }
 Constraint::clear( *v_c );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_c = new boost::multi_array< FRowConstraint , 2 >;
 m_c->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_constraint( *m_c );
 assert( ( block->get_static_constraint< FRowConstraint , 2 >( 0 ) ) == m_c );
 for( auto i = m_c->data() ; i < ( m_c->data() + m_c->num_elements() ) ; ++i )
  assert( i->get_Block() == block );
 Constraint::clear( *m_c );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_One
 block = new AbstractBlock(); // SetUp
 auto v = new ColVariable();
 block->add_static_variable( *v );
 assert( block->get_static_variable< ColVariable >( 0 ) == v );
 assert( block->get_static_variable< ColVariable >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_Vector
 block = new AbstractBlock(); // SetUp
 auto v_v = new std::vector< ColVariable >( 5 );
 block->add_static_variable( *v_v );
 assert( block->get_static_variable_v< ColVariable >( 0 ) == v_v );
 for( const auto & i : *v_v )
  assert( i.get_Block() == block );
 assert( ColVariable::is_feasible( *v_v ) );
 TearDown( block ); // TearDown

 // test Adds_StaticVariables_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_v = new boost::multi_array< ColVariable , 2 >;
 m_v->resize( boost::extents[ 2 ][ 2 ] );
 block->add_static_variable( *m_v );
 assert( ( block->get_static_variable< ColVariable , 2 >( 0 ) ) == m_v );
 for( auto i = m_v->data() ; i < ( m_v->data() + m_v->num_elements() ) ; ++i )
  assert( i->get_Block() == block );
 assert( ColVariable::is_feasible( *m_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_List
 block = new AbstractBlock(); // SetUp
 auto l_c = new std::list< FRowConstraint >( 5 );
 block->add_dynamic_constraint( *l_c );
 assert( block->get_dynamic_constraint< FRowConstraint >( 0 ) == l_c );
 for( const auto & i : *l_c )
  assert( i.get_Block() == block );
 Constraint::clear( *l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_Vector
 block = new AbstractBlock(); // SetUp
 auto v_l_c = new std::vector< std::list< FRowConstraint > >( 5 );
 for( auto & i : *v_l_c )
  i.resize( 3 );
 block->add_dynamic_constraint( *v_l_c );
 assert( block->get_dynamic_constraint_v< FRowConstraint >( 0 ) == v_l_c );
 for( auto & i : *v_l_c )
  for( auto & j : i )
   assert( j.get_Block() == block );
 Constraint::clear( *v_l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_l_c = new boost::multi_array< std::list< FRowConstraint > , 2 >;
 m_l_c->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = m_l_c->data() ;
      i < ( m_l_c->data() + m_l_c->num_elements() ) ; ++i )
  i->resize( 3 );
 block->add_dynamic_constraint( *m_l_c );
 assert(
  ( block->get_dynamic_constraint< FRowConstraint , 2 >( 0 ) ) == m_l_c );
 for( auto i = m_l_c->data() ;
      i < ( m_l_c->data() + m_l_c->num_elements() ) ; ++i )
  for( auto & j : *i )
   assert( j.get_Block() == block );
 Constraint::clear( *m_l_c );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_List
 block = new AbstractBlock(); // SetUp
 auto l_v = new std::list< ColVariable >( 5 );
 block->add_dynamic_variable( *l_v );
 assert( block->get_dynamic_variable< ColVariable >( 0 ) == l_v );
 for( const auto & i : *l_v )
  assert( i.get_Block() == block );
 assert( ColVariable::is_feasible( *l_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_Vector
 block = new AbstractBlock(); // SetUp
 auto v_l_v = new std::vector< std::list< ColVariable > >( 5 );
 for( auto & i : *v_l_v )
  i.resize( 3 );
 block->add_dynamic_variable( *v_l_v );
 assert( block->get_dynamic_variable_v< ColVariable >( 0 ) == v_l_v );
 for( auto & i : *v_l_v )
  for( auto & j : i )
   assert( j.get_Block() == block );
 assert( ColVariable::is_feasible( *v_l_v ) );
 TearDown( block ); // TearDown

 // test Adds_DynamicVariables_MultiArray
 block = new AbstractBlock(); // SetUp
 auto m_l_v = new boost::multi_array< std::list< ColVariable > , 2 >;
 m_l_v->resize( boost::extents[ 2 ][ 2 ] );
 for( auto i = m_l_v->data() ;
      i < ( m_l_v->data() + m_l_v->num_elements() ) ; ++i )
  i->resize( 3 );
 block->add_dynamic_variable( *m_l_v );
 assert( ( block->get_dynamic_variable< ColVariable , 2 >( 0 ) ) == m_l_v );
 for( auto i = m_l_v->data() ;
      i < ( m_l_v->data() + m_l_v->num_elements() ) ; ++i )
  for( auto & j : *i )
   assert( j.get_Block() == block );
 assert( ColVariable::is_feasible( *m_l_v ) );
 TearDown( block ); // TearDown
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* A Solution written to a netCDF group and read back from it holds what it
 * held: the values of the static Variable, group by group, and those of the
 * dynamic ones, group by group and cell by cell. */

static void test_solution_round_trip( void )
{
 AbstractBlock block;

 std::vector< ColVariable > v( 3 );
 for( int i = 0 ; i < 3 ; ++i )
  v[ i ].set_value( 1.0 + i );
 block.add_static_variable( v , "x" );

 auto cells = new std::vector< std::list< ColVariable > >( 2 );
 ( *cells )[ 0 ].resize( 2 );
 ( *cells )[ 1 ].resize( 1 );
 double k = 10;
 for( auto & cell : *cells )
  for( auto & element : cell )
   element.set_value( k++ );
 block.add_dynamic_variable( *cells , "y" );

 ColVariableSolution written;
 written.read( & block );

 const char * const name = "tests_AbstractBlock_solution.nc4";
 {
  netCDF::NcFile file( name , netCDF::NcFile::replace );
  auto g = file.addGroup( "Solution" );
  written.serialize( g );
 }

 ColVariableSolution read;
 {
  netCDF::NcFile file( name , netCDF::NcFile::read );
  read.deserialize( file.getGroup( "Solution" ) );
 }
 std::remove( name );

 assert( read.get_static_variable_values() ==
	 written.get_static_variable_values() );
 assert( read.get_dynamic_variable_values() ==
	 written.get_dynamic_variable_values() );

 // the shape survives too: one group of 3, and one of 2 cells of 2 and 1
 assert( read.get_static_variable_values().size() == 1 );
 assert( read.get_static_variable_values()[ 0 ].size() == 3 );
 assert( read.get_dynamic_variable_values().size() == 1 );
 assert( read.get_dynamic_variable_values()[ 0 ].size() == 2 );
 assert( read.get_dynamic_variable_values()[ 0 ][ 0 ].size() == 2 );
 assert( read.get_dynamic_variable_values()[ 0 ][ 1 ].size() == 1 );

 delete cells;
 }

/*--------------------------------------------------------------------------*/
/* A Block written as an LP file and read back from it is the same model: the
 * same rows, the same bounds, the same columns declared integer. What is not
 * the same is the way it is grouped, since an LP file has no notion of
 * groups and read_lp() builds one group of columns and one of rows. */

static void test_lp_round_trip( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 3 );
 ( *cols )[ 0 ].set_type( ColVariable::kNatural );
 ( *cols )[ 1 ].is_positive( true , eNoMod );
 block.add_static_variable( *cols , "x" );

 auto rows = new std::vector< FRowConstraint >( 2 );
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 0 ] , 1.0 } );
  p.push_back( { & ( *cols )[ 1 ] , 2.0 } );
  ( *rows )[ 0 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 0 ].set_lhs( 1 );   // both sides finite: the LP file says it
  ( *rows )[ 0 ].set_rhs( 4 );   // in two rows, since it has no two-sided one
 }
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 2 ] , -3.0 } );
  ( *rows )[ 1 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 1 ].set_lhs( - Inf< double >() );
  ( *rows )[ 1 ].set_rhs( 7 );
 }
 block.add_static_constraint( *rows , "r" );

 LinearFunction::v_coeff_pair o;
 o.push_back( { & ( *cols )[ 0 ] , 5.0 } );
 o.push_back( { & ( *cols )[ 2 ] , -1.0 } );
 auto obj = new FRealObjective( & block , new LinearFunction( std::move( o ) ) );
 obj->set_sense( Objective::eMin , eNoMod );
 block.set_objective( obj , eNoMod );

 std::ostringstream written;
 block.write_lp( written );

 // the file says what the model is: the three columns are named after the
 // group they sit in, the row with both sides is there twice, and the one
 // integer column is in the Generals section
 const auto lp = written.str();
 assert( lp.find( "Minimize" ) != std::string::npos );
 assert( lp.find( "x_0" ) != std::string::npos );
 assert( lp.find( "r_0_up" ) != std::string::npos );
 assert( lp.find( "r_0_lo" ) != std::string::npos );
 assert( lp.find( "Generals" ) != std::string::npos );
 assert( lp.find( "End" ) != std::string::npos );

 // and what is written is read back into the same model
 const char * const name = "tests_AbstractBlock_model.nc4";
 block.Block::serialize( name , eBlockFile );

 auto read = dynamic_cast< AbstractBlock * >( Block::deserialize( name ) );
 std::remove( name );
 assert( read );

 std::ostringstream again;
 read->write_lp( again );
 delete read;

 // the LP of the copy has the same rows and the same bounds; the names of
 // the groups are those read_lp() gives, so the two files are not compared
 // character by character
 const auto lp2 = again.str();
 assert( lp2.find( "Minimize" ) != std::string::npos );
 assert( lp2.find( "Generals" ) != std::string::npos );
 assert( std::count( lp2.begin() , lp2.end() , '\n' ) ==
	 std::count( lp.begin() , lp.end() , '\n' ) );
 }

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* The same model written as an MPS file and read back from it. The format
 * says a two-sided row once, with its second side in RANGES, so the file has
 * one row per Constraint and not two as the LP one has. */

static void test_mps_round_trip( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 3 );
 ( *cols )[ 0 ].set_type( ColVariable::kNatural );
 ( *cols )[ 1 ].is_positive( true , eNoMod );
 block.add_static_variable( *cols , "x" );

 auto rows = new std::vector< FRowConstraint >( 2 );
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 0 ] , 1.0 } );
  p.push_back( { & ( *cols )[ 1 ] , 2.0 } );
  ( *rows )[ 0 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 0 ].set_lhs( 1 );       // both sides finite: RANGES says the
  ( *rows )[ 0 ].set_rhs( 4 );       // second one
 }
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 2 ] , -3.0 } );
  ( *rows )[ 1 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 1 ].set_lhs( - Inf< double >() );
  ( *rows )[ 1 ].set_rhs( 7 );
 }
 block.add_static_constraint( *rows , "r" );

 LinearFunction::v_coeff_pair o;
 o.push_back( { & ( *cols )[ 0 ] , 5.0 } );
 o.push_back( { & ( *cols )[ 2 ] , -1.0 } );
 auto obj = new FRealObjective( & block , new LinearFunction( std::move( o ) ) );
 obj->set_sense( Objective::eMin , eNoMod );
 block.set_objective( obj , eNoMod );

 std::ostringstream written;
 block.write_mps( written );

 const auto mps = written.str();
 assert( mps.find( "OBJSENSE" ) != std::string::npos );
 assert( mps.find( " L  r_0" ) != std::string::npos );
 assert( mps.find( "RANGES" ) != std::string::npos );
 assert( mps.find( "'INTORG'" ) != std::string::npos );
 assert( mps.find( "ENDATA" ) != std::string::npos );

 // read back: the same rows, the same bounds, the same integer column
 AbstractBlock again;
 std::istringstream in( mps );
 again.load( in , 'M' );

 const auto & gv = again.get_static_variable_groups();
 const auto & gc = again.get_static_constraint_groups();
 assert( gv.size() == 1 );
 assert( gc.size() == 2 );          // the rows, and the box of the columns
 assert( gv[ 0 ]->get_num_elements() == 3 );
 assert( gc[ 0 ]->get_num_elements() == 2 );  // one row each, not two

 // and what it says is what was written
 std::vector< double > lhs , rhs;
 gc[ 0 ]->for_each_as< FRowConstraint >(
  [ & lhs , & rhs ]( FRowConstraint & c ) {
   lhs.push_back( c.get_lhs() ); rhs.push_back( c.get_rhs() ); } );
 assert( lhs.size() == 2 );
 assert( lhs[ 0 ] == 1 );
 assert( rhs[ 0 ] == 4 );
 assert( lhs[ 1 ] <= - Inf< double >() );
 assert( rhs[ 1 ] == 7 );

 bool first = true;
 gv[ 0 ]->for_each_as< ColVariable >(
  [ & first ]( ColVariable & v ) {
   if( first ) { assert( v.is_integer() ); first = false; } } );

 /* Writing the copy does NOT give the first file back: an MPS file has no
  * notion of groups, so the copy has the one group of columns and the one of
  * rows read_mps() builds, and its elements are named after those. What does
  * hold is that from there on the file is a fixed point, which is what says
  * that nothing is lost or added at each trip. */

 std::ostringstream twice;
 again.write_mps( twice );
 assert( twice.str() != mps );

 AbstractBlock third;
 std::istringstream in2( twice.str() );
 third.load( in2 , 'M' );

 std::ostringstream thrice;
 third.write_mps( thrice );
 assert( thrice.str() == twice.str() );
 }

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* The rows a dual ray names. What a Solver does when it proves a model
 * unfeasible is writing the ray into the Block, one multiplier per
 * Constraint, so the ray is put there by hand here and what is checked is
 * that the printer names the rows that carry one and leaves the others
 * alone. The model is x >= 2 and x <= 1, which cannot both hold. */

static void test_is( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 2 );
 block.add_static_variable( *cols , "x" );

 auto rows = new std::vector< FRowConstraint >( 3 );
 for( int i = 0 ; i < 3 ; ++i ) {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ i == 2 ? 1 : 0 ] , 1.0 } );
  ( *rows )[ i ].set_function( new LinearFunction( std::move( p ) ) );
 }
 ( *rows )[ 0 ].set_lhs( 2 );                    // x_0 >= 2
 ( *rows )[ 0 ].set_rhs( Inf< double >() );
 ( *rows )[ 1 ].set_lhs( - Inf< double >() );    // x_0 <= 1
 ( *rows )[ 1 ].set_rhs( 1 );
 ( *rows )[ 2 ].set_lhs( - Inf< double >() );    // and one that has nothing
 ( *rows )[ 2 ].set_rhs( 9 );                    // to do with it
 block.add_static_constraint( *rows , "r" );

 // with no ray in the Block the printer says so rather than naming rows
 {
  std::ostringstream none;
  block.write_is( none );
  assert( none.str().find( "no multiplier" ) != std::string::npos );
 }

 // the ray a Solver would have written: the two rows that fight, and not
 // the third one
 ( *rows )[ 0 ].set_dual( 1 );
 ( *rows )[ 1 ].set_dual( -1 );
 ( *rows )[ 2 ].set_dual( 0 );

 std::ostringstream is;
 block.write_is( is );
 const auto said = is.str();

 assert( said.find( "r_2" ) == std::string::npos );
 assert( said.find( "no multiplier" ) == std::string::npos );

 // the line is read by somebody, so it is the whole line that is checked:
 // the multiplier, the name, and the row with its side where it belongs
 assert( said.find( " 1 * ( r_0: 2 <= x_0 )" ) != std::string::npos );
 assert( said.find( " -1 * ( r_1: x_0 <= 1 )" ) != std::string::npos );

 // eps leaves out what is smaller than it
 ( *rows )[ 1 ].set_dual( 1e-12 );
 std::ostringstream cut;
 block.write_is( cut , 1e-9 );
 assert( cut.str().find( "r_0" ) != std::string::npos );
 assert( cut.str().find( "r_1" ) == std::string::npos );
 }

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* What the two writers do with what is degenerate: no Objective at all, a
 * row every coefficient of which is zero, a free column, a fixed one, a
 * column that is in no row and in no Objective, an equality row, a row whose
 * two sides are both infinite, a group with no name, a coefficient of 1 and
 * one of -1, a number that wants all of its digits, and a Block that is
 * maximizing. Each of these is a branch of the two writers that the plain
 * model does not go through. */

static void test_writers_edge_cases( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 5 );
 // x_0 free, x_1 fixed, x_2 in nothing, x_3 and x_4 ordinary
 ( *cols )[ 0 ].is_positive( false , eNoMod );
 ( *cols )[ 0 ].is_negative( false , eNoMod );
 ( *cols )[ 1 ].set_value( 2.5 );
 ( *cols )[ 1 ].is_fixed( true , eNoMod );
 block.add_static_variable( *cols , "x" );

 // a group with no name: its elements are named after its index
 auto more = new std::vector< ColVariable >( 1 );
 block.add_static_variable( *more );

 auto rows = new std::vector< FRowConstraint >( 4 );
 {                                       // every coefficient zero
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 3 ] , 0.0 } );
  ( *rows )[ 0 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 0 ].set_lhs( - Inf< double >() );
  ( *rows )[ 0 ].set_rhs( 1 );
 }
 {                                       // an equality, and a 1 and a -1
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 3 ] , 1.0 } );
  p.push_back( { & ( *cols )[ 4 ] , -1.0 } );
  ( *rows )[ 1 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 1 ].set_both( 3 );
 }
 {                                       // both sides infinite
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 4 ] , 2.0 } );
  ( *rows )[ 2 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 2 ].set_lhs( - Inf< double >() );
  ( *rows )[ 2 ].set_rhs( Inf< double >() );
 }
 {                                       // a number that wants its digits
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *more )[ 0 ] , 0.1 } );
  ( *rows )[ 3 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 3 ].set_lhs( 1.0 / 3.0 );
  ( *rows )[ 3 ].set_rhs( Inf< double >() );
 }
 block.add_static_constraint( *rows , "r" );

 // ---- with no Objective at all -------------------------------------------

 std::ostringstream noobj;
 block.write_lp( noobj );
 assert( noobj.str().find( "Minimize" ) != std::string::npos );
 assert( noobj.str().find( " obj: 0" ) != std::string::npos );

 // ---- the LP file --------------------------------------------------------

 LinearFunction::v_coeff_pair o;
 o.push_back( { & ( *cols )[ 3 ] , 1.0 } );
 auto obj = new FRealObjective( & block ,
				new LinearFunction( std::move( o ) ) );
 obj->set_sense( Objective::eMax , eNoMod );
 block.set_objective( obj , eNoMod );

 std::ostringstream lps;
 block.write_lp( lps );
 const auto lp = lps.str();

 assert( lp.find( "Maximize" ) != std::string::npos );
 // a row with no coefficient left is still a row, and says 0
 assert( lp.find( " r_0_up: 0 <= 1" ) != std::string::npos );
 // an equality is one row, and a coefficient of 1 or -1 is not written
 assert( lp.find( " r_1: x_3 - x_4 = 3" ) != std::string::npos );
 // the free column says so, the fixed one is a point, and the group with
 // no name is named after its index
 assert( lp.find( " x_0 free" ) != std::string::npos );
 assert( lp.find( " v1_0" ) != std::string::npos );
 // the number is there whole
 assert( lp.find( "0.3333333333333333" ) != std::string::npos );
 assert( lp.find( "0.1 v1_0" ) != std::string::npos );

 // ---- the MPS file -------------------------------------------------------

 std::ostringstream mpss;
 block.write_mps( mpss );
 const auto mps = mpss.str();

 assert( mps.find( "OBJSENSE" ) != std::string::npos );
 assert( mps.find( "    MAX" ) != std::string::npos );
 assert( mps.find( " E  r_1" ) != std::string::npos );
 // the row with both sides infinite is not in the file at all
 assert( mps.find( "r_2" ) == std::string::npos );
 // the column that is in no row is, with a zero cost
 assert( mps.find( "    x_2  obj  0" ) != std::string::npos );
 // the fixed column is a point and the free one is free
 assert( mps.find( " FX BND  x_1  2.5" ) != std::string::npos );
 assert( mps.find( " FR BND  x_0" ) != std::string::npos );
 // no INTORG marker, nothing here being integer
 assert( mps.find( "INTORG" ) == std::string::npos );

 // and it still makes the trip: reading it and writing it again is a fixed
 // point, which is what says that none of these was lost on the way
 AbstractBlock again;
 std::istringstream in( mps );
 again.load( in , 'M' );

 std::ostringstream twice , thrice;
 again.write_mps( twice );

 /* The copy has no named group at all, and its group of columns and its
  * group of rows both have index 0: the two markers have to differ, or the
  * file would call a row and a column by the same name and no reader could
  * tell which of them a name means. */

 assert( twice.str().find( " E  c0_1" ) != std::string::npos );
 assert( twice.str().find( "    v0_1  obj" ) != std::string::npos );

 AbstractBlock third;
 std::istringstream in2( twice.str() );
 third.load( in2 , 'M' );
 third.write_mps( thrice );
 assert( thrice.str() == twice.str() );

 // ---- a Function that is not linear is refused, not written wrong --------

 auto quad = new std::vector< FRowConstraint >( 1 );
 {
  DQuadFunction::v_coeff_triple t;
  t.push_back( { & ( *cols )[ 3 ] , 1.0 , 1.0 } );
  ( *quad )[ 0 ].set_function( new DQuadFunction( std::move( t ) ) );
  ( *quad )[ 0 ].set_rhs( 1 );
 }
 block.add_static_constraint( *quad , "q" );

 bool refused = false;
 try { std::ostringstream no; block.write_lp( no ); }
 catch( const std::invalid_argument & ) { refused = true; }
 assert( refused );

 refused = false;
 try { std::ostringstream no; block.write_mps( no ); }
 catch( const std::invalid_argument & ) { refused = true; }
 assert( refused );
 }

/*--------------------------------------------------------------------------*/
/* What write_is() does at its own edges: a multiplier exactly equal to eps,
 * which the comparison leaves out; an equality row, which is written with
 * its one side; a row that is not linear, which has no place in a
 * certificate written this way; and a ray that names nothing at all. */

static void test_is_edge_cases( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 2 );
 block.add_static_variable( *cols , "y" );

 auto rows = new std::vector< FRowConstraint >( 2 );
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 0 ] , 1.0 } );
  ( *rows )[ 0 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 0 ].set_both( 4 );              // an equality
  ( *rows )[ 0 ].set_dual( 1e-9 );
 }
 {
  LinearFunction::v_coeff_pair p;
  p.push_back( { & ( *cols )[ 1 ] , 1.0 } );
  ( *rows )[ 1 ].set_function( new LinearFunction( std::move( p ) ) );
  ( *rows )[ 1 ].set_lhs( - Inf< double >() );
  ( *rows )[ 1 ].set_rhs( 2 );
  ( *rows )[ 1 ].set_dual( -1 );
 }
 block.add_static_constraint( *rows , "e" );

 // a multiplier exactly equal to eps is left out: the comparison is <=, so
 // the row on the threshold does not enter the certificate
 std::ostringstream at;
 block.write_is( at , 1e-9 );
 assert( at.str().find( "e_0" ) == std::string::npos );
 assert( at.str().find( "e_1" ) != std::string::npos );

 // just below eps it does enter, and an equality is written with its side
 std::ostringstream under;
 block.write_is( under , 1e-10 );
 assert( under.str().find( " 1e-09 * ( e_0: y_0 = 4 )" ) != std::string::npos );

 // a row that is not linear carries no multiplier into the certificate
 auto quad = new std::vector< FRowConstraint >( 1 );
 {
  DQuadFunction::v_coeff_triple t;
  t.push_back( { & ( *cols )[ 0 ] , 1.0 , 1.0 } );
  ( *quad )[ 0 ].set_function( new DQuadFunction( std::move( t ) ) );
  ( *quad )[ 0 ].set_rhs( 1 );
  ( *quad )[ 0 ].set_dual( 7 );
 }
 block.add_static_constraint( *quad , "q" );

 std::ostringstream withq;
 block.write_is( withq );
 assert( withq.str().find( "q_0" ) == std::string::npos );
 assert( withq.str().find( "e_1" ) != std::string::npos );

 // a ray that names nothing says so rather than writing an empty answer
 ( *rows )[ 0 ].set_dual( 0 );
 ( *rows )[ 1 ].set_dual( 0 );
 std::ostringstream none;
 block.write_is( none );
 assert( none.str().find( "no multiplier" ) != std::string::npos );
 }

/*--------------------------------------------------------------------------*/
/*------------------ SERIALIZATION, Objective AND mirror() -----------------*/
/*--------------------------------------------------------------------------*/

/// writes the Block into a netCDF file and reads it back, nullptr if it fails

static Block * through_a_file( const Block & block )
{
 const char * const name = "tests_AbstractBlock_round_trip.nc4";
 block.Block::serialize( name , eBlockFile );
 auto read = Block::deserialize( name );
 std::remove( name );
 return( read );
 }

/*--------------------------------------------------------------------------*/
/// a LinearFunction with coefficient 1 for each of the given Variable

static LinearFunction * sum_of( const std::vector< ColVariable * > & vars )
{
 LinearFunction::v_coeff_pair p;
 for( auto var : vars )
  p.push_back( { var , 1.0 } );
 return( new LinearFunction( std::move( p ) ) );
 }

/*--------------------------------------------------------------------------*/

/* An empty Block goes through a netCDF file and comes back as an empty
 * AbstractBlock: no group, no Objective, no nested Block. */

static void test_serialize_empty( void )
{
 AbstractBlock block;

 auto read = through_a_file( block );
 assert( read );
 assert( dynamic_cast< AbstractBlock * >( read ) );
 assert( read->get_static_variable_groups().empty() );
 assert( read->get_dynamic_variable_groups().empty() );
 assert( read->get_static_constraint_groups().empty() );
 assert( read->get_dynamic_constraint_groups().empty() );
 assert( ! read->get_objective() );
 assert( read->get_nested_Blocks().empty() );

 delete read;
 std::cout << "serialize an empty Block: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The dynamic groups travel as the model they are part of [see
 * AbstractBlock::serialize()]: what comes back has the same columns and the
 * same rows, in the static groups read_lp() builds. */

static void test_serialize_dynamic_groups( void )
{
 AbstractBlock block;

 auto cols = new std::list< ColVariable >( 3 );
 block.add_dynamic_variable( *cols , "y" );
 std::vector< ColVariable * > y;
 for( auto & col : *cols )
  y.push_back( & col );

 auto rows = new std::list< FRowConstraint >( 2 );
 double rhs = 5;
 for( auto & row : *rows ) {
  row.set_function( sum_of( { y[ 0 ] , y[ 1 ] } ) );
  row.set_lhs( - Inf< double >() );
  row.set_rhs( rhs++ );
  }
 block.add_dynamic_constraint( *rows , "d" );

 block.set_objective( new FRealObjective( & block , sum_of( y ) ) , eNoMod );

 auto read = through_a_file( block );
 assert( read );
 assert( read->get_dynamic_variable_groups().empty() );
 assert( read->get_dynamic_constraint_groups().empty() );

 const auto & sv = read->get_static_variable_groups();
 const auto & sc = read->get_static_constraint_groups();
 assert( sv.size() == 1 );
 assert( sv[ 0 ]->get_num_elements() == 3 );
 assert( sc.size() == 2 );                // the rows, and the box of columns
 assert( sc[ 0 ]->get_num_elements() == 2 );

 std::vector< double > got;
 sc[ 0 ]->for_each_as< FRowConstraint >( [ & got ]( FRowConstraint & c ) {
   assert( c.get_lhs() <= - Inf< double >() );
   got.push_back( c.get_rhs() ); } );
 assert( ( got == std::vector< double >( { 5 , 6 } ) ) );
 assert( read->get_objective() );

 delete read;
 std::cout << "serialize dynamic groups: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The bounds a :OneVarConstraint puts on a column travel as bounds of that
 * column; the Objective as it is. */

static void test_serialize_one_var_constraint( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 2 );
 block.add_static_variable( *cols , "x" );
 auto box = new std::vector< BoxConstraint >( 1 );
 ( *box )[ 0 ].set_variable( & ( *cols )[ 1 ] );
 ( *box )[ 0 ].set_lhs( 1 );
 ( *box )[ 0 ].set_rhs( 3 );
 block.add_static_constraint( *box , "b" );

 // both columns in the Objective, so that each of them is in the model
 // somewhere else than in the Bounds [see test_serialize_column_in_no_row()]
 block.set_objective( new FRealObjective( & block ,
				 sum_of( { & ( *cols )[ 0 ] , & ( *cols )[ 1 ] } ) ) ,
		      eNoMod );

 auto read = through_a_file( block );
 assert( read );

 // the bounds of the columns read back: x_0 is free, x_1 is in [ 1 , 3 ]
 std::vector< std::pair< double , double > > bounds;
 for( const auto & group : read->get_static_constraint_groups() )
  group->for_each_as< BoxConstraint >( [ & bounds ]( BoxConstraint & c ) {
    bounds.emplace_back( c.get_lhs() , c.get_rhs() ); } );
 assert( bounds.size() == 2 );
 assert( bounds[ 0 ].first <= - Inf< double >() );
 assert( bounds[ 0 ].second >= Inf< double >() );
 assert( ( bounds[ 1 ] == std::pair< double , double >( 1 , 3 ) ) );

 auto obj = dynamic_cast< FRealObjective * >( read->get_objective() );
 assert( obj );
 auto lf = dynamic_cast< LinearFunction * >( obj->get_function() );
 assert( lf && ( lf->get_num_active_var() == 2 ) );

 delete read;
 std::cout << "serialize a OneVarConstraint: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A column that is in no row and not in the Objective is still a column of
 * the model, and write_lp() writes it in the Bounds section: read_lp() has
 * to take it from there, or the Block cannot be read back at all. */

static void test_serialize_column_in_no_row( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 2 );
 block.add_static_variable( *cols , "x" );
 block.set_objective( new FRealObjective( & block ,
					  sum_of( { & ( *cols )[ 0 ] } ) ) ,
		      eNoMod );

 auto read = through_a_file( block );
 assert( read );
 const auto & sv = read->get_static_variable_groups();
 assert( sv.size() == 1 );
 assert( sv[ 0 ]->get_num_elements() == 2 );
 delete read;

 std::cout << "serialize a column in no row: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* An Objective that is not linear cannot be written as the LP file the
 * AbstractBlock is serialized into: it is refused, not written wrong. */

static void test_serialize_quadratic_objective( void )
{
 AbstractBlock block;

 auto cols = new std::vector< ColVariable >( 1 );
 block.add_static_variable( *cols , "x" );
 DQuadFunction::v_coeff_triple t;
 t.push_back( { & ( *cols )[ 0 ] , 1.0 , 2.0 } );
 block.set_objective( new FRealObjective( & block ,
				  new DQuadFunction( std::move( t ) ) ) ,
		      eNoMod );

 const char * const name = "tests_AbstractBlock_quadratic.nc4";
 bool refused = false;
 try {
  block.Block::serialize( name , eBlockFile );
  }
 catch( const std::invalid_argument & ) {
  refused = true;
  }
 std::remove( name );
 assert( refused );

 std::cout << "serialize a quadratic Objective: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The nested Block travel each in a group of its own, and come back nested
 * in the Block they were nested in, with their own model. */

static void test_serialize_nested( void )
{
 auto block = new AbstractBlock;
 auto x = new std::vector< ColVariable >( 1 );
 block->add_static_variable( *x , "x" );
 block->set_objective( new FRealObjective( block ,
					   sum_of( { & ( *x )[ 0 ] } ) ) , eNoMod );

 for( int k = 0 ; k < 2 ; ++k ) {
  auto inner = new AbstractBlock( block );
  auto z = new std::vector< ColVariable >( 2 );
  inner->add_static_variable( *z , "z" );
  auto rows = new std::vector< FRowConstraint >( 1 + k );
  for( auto & row : *rows ) {
   row.set_function( sum_of( { & ( *z )[ 0 ] , & ( *z )[ 1 ] } ) );
   row.set_lhs( - Inf< double >() );   // one side, hence one row of the LP
   row.set_rhs( 4 );
   }
  inner->add_static_constraint( *rows , "r" );
  inner->set_objective( new FRealObjective( inner ,
					    sum_of( { & ( *z )[ 1 ] } ) ) ,
			eNoMod );
  block->add_nested_Block( inner );
  }

 auto read = through_a_file( *block );
 assert( read );
 const auto & nested = read->get_nested_Blocks();
 assert( nested.size() == 2 );
 for( int k = 0 ; k < 2 ; ++k ) {
  assert( nested[ k ] );
  assert( nested[ k ]->get_static_variable_groups().size() == 1 );
  assert( nested[ k ]->get_static_variable_groups()[ 0 ]->
	  get_num_elements() == 2 );
  assert( nested[ k ]->get_static_constraint_groups()[ 0 ]->
	  get_num_elements() == Block::Index( 1 + k ) );
  assert( nested[ k ]->get_f_Block() == read );
  }

 delete read;
 delete block;
 std::cout << "serialize nested Blocks: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A Block with no Objective, whose LP file says " obj: 0", i.e., an
 * Objective made of a constant alone: it comes back as the same model, with
 * its row, and with an Objective that has no term. */

static void test_serialize_no_objective( void )
{
 AbstractBlock block;
 auto cols = new std::vector< ColVariable >( 1 );
 block.add_static_variable( *cols , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 ( *rows )[ 0 ].set_function( sum_of( { & ( *cols )[ 0 ] } ) );
 ( *rows )[ 0 ].set_lhs( - Inf< double >() );
 ( *rows )[ 0 ].set_rhs( 2 );
 block.add_static_constraint( *rows , "r" );

 std::ostringstream lp;
 block.write_lp( lp );
 assert( lp.str().find( " obj: 0" ) != std::string::npos );

 auto read = through_a_file( block );
 assert( read );
 const auto & sc = read->get_static_constraint_groups();
 assert( sc.size() == 2 );
 assert( sc[ 0 ]->get_num_elements() == 1 );
 sc[ 0 ]->for_each_as< FRowConstraint >( []( FRowConstraint & c ) {
   assert( c.get_rhs() == 2 ); } );
 auto obj = dynamic_cast< FRealObjective * >( read->get_objective() );
 assert( obj );
 auto lf = dynamic_cast< LinearFunction * >( obj->get_function() );
 assert( lf && ( lf->get_num_active_var() == 0 ) );

 delete read;
 std::cout << "serialize a Block with no Objective: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* The LP file of an empty model, as write_lp() writes it, is read back as a
 * model with no column and no row. */

static void test_read_lp_of_an_empty_model( void )
{
 AbstractBlock empty;
 std::ostringstream lp;
 empty.write_lp( lp );

 AbstractBlock again;
 std::istringstream in( lp.str() );
 again.load( in , 'L' );
 for( const auto & g : again.get_static_variable_groups() )
  assert( g->get_num_elements() == 0 );
 for( const auto & g : again.get_static_constraint_groups() )
  assert( g->get_num_elements() == 0 );

 std::cout << "read_lp of an empty model: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* What read_lp() does with what write_lp() writes at its edges: constants in
 * the Objective and in a row, which the first carries as its constant term
 * and the second moves to its side; a column whose lower bound is -infinity
 * and whose upper one is finite; a column that is only named among the
 * Generals. */

static void test_read_lp_edges( void )
{
 const std::string lp =
  "\\ a comment\n"
  "Maximize\n"
  " obj: 2 x + 3 - y\n"
  "Subject To\n"
  " r: 0 <= 4\n"
  " s: x + 1 >= 3\n"
  "Bounds\n"
  " -infinity <= x <= 5\n"
  " y free\n"
  "Generals\n"
  " z\n"
  "End\n";

 AbstractBlock block;
 std::istringstream in( lp );
 block.load( in , 'L' );

 const auto & sv = block.get_static_variable_groups();
 const auto & sc = block.get_static_constraint_groups();
 assert( sv.size() == 1 );
 assert( sv[ 0 ]->get_num_elements() == 3 );    // x , y and z
 assert( sc[ 0 ]->get_num_elements() == 2 );

 auto obj = dynamic_cast< FRealObjective * >( block.get_objective() );
 assert( obj && ( obj->get_sense() == Objective::eMax ) );
 auto lf = dynamic_cast< LinearFunction * >( obj->get_function() );
 assert( lf && ( lf->get_num_active_var() == 2 ) );
 assert( lf->get_linearization_constant() == 3 );

 std::vector< std::pair< double , double > > sides;
 sc[ 0 ]->for_each_as< FRowConstraint >( [ & sides ]( FRowConstraint & c ) {
   sides.emplace_back( c.get_lhs() , c.get_rhs() ); } );
 assert( sides[ 0 ].first <= - Inf< double >() );
 assert( sides[ 0 ].second == 4 );
 assert( sides[ 1 ].first == 2 );                // x + 1 >= 3 is x >= 2
 assert( sides[ 1 ].second >= Inf< double >() );

 std::vector< std::pair< double , double > > bounds;
 sc[ 1 ]->for_each_as< BoxConstraint >( [ & bounds ]( BoxConstraint & c ) {
   bounds.emplace_back( c.get_lhs() , c.get_rhs() ); } );
 assert( bounds[ 0 ].first <= - Inf< double >() );
 assert( bounds[ 0 ].second == 5 );

 bool integer = false;
 sv[ 0 ]->for_each_as< ColVariable >( [ & integer ]( ColVariable & v ) {
   integer = integer || v.is_integer(); } );
 assert( integer );

 // and the bound -infinity <= x <= 5 is written so that it is read so
 std::ostringstream again;
 block.write_lp( again );
 assert( again.str().find( "-infinity <= " ) != std::string::npos );

 std::cout << "read_lp at its edges: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Input that is not an LP file, or one that ends before its End, is refused
 * with an exception that says who refuses it, and does not keep read_lp()
 * reading forever. */

static void test_read_lp_malformed( void )
{
 for( const std::string lp : {
   std::string( "" ) ,
   std::string( "Maybe\n obj: x\nEnd\n" ) ,
   std::string( "Minimize\n obj: x\n" ) ,
   std::string( "Minimize\n obj: 0\n" ) ,
   std::string( "Minimize\n obj: x\nSubject To\n r: x <= 1\n" ) ,
   std::string( "Minimize\n obj: x\nSubject To\n r: x\n" ) ,
   std::string( "Minimize\n obj: x\nSubject To\nBounds\n x <= 1\n" ) ,
   std::string( "Minimize\n obj: x\nSubject To\nGenerals\n x\n" ) } ) {
  AbstractBlock block;
  std::istringstream in( lp );
  std::string what;
  try { block.load( in , 'L' ); }
  catch( const std::invalid_argument & e ) { what = e.what(); }
  assert( what.find( "AbstractBlock::read_lp: " ) == 0 );
  }

 std::cout << "read_lp of malformed input: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* set_objective() on a Block that has one replaces it: the new one is the
 * Objective of the Block and knows it, and the old one is left to whoever
 * made it, not deleted [see Block::set_objective()]. */

static void test_set_objective_replaces( void )
{
 AbstractBlock block;
 auto cols = new std::vector< ColVariable >( 2 );
 block.add_static_variable( *cols , "x" );

 auto first = new FRealObjective( & block , sum_of( { & ( *cols )[ 0 ] } ) );
 block.set_objective( first , eNoMod );
 assert( block.get_objective() == first );
 assert( first->get_Block() == & block );

 auto second = new FRealObjective( nullptr , sum_of( { & ( *cols )[ 1 ] } ) );
 second->set_sense( Objective::eMax , eNoMod );
 block.set_objective( second , eNoMod );
 assert( block.get_objective() == second );
 assert( second->get_Block() == & block );

 // the old one is still there to be deleted by whoever made it
 assert( first->get_function() );
 delete first;

 // and what the Block writes is the new one
 std::ostringstream lp;
 block.write_lp( lp );
 assert( lp.str().find( "Maximize" ) != std::string::npos );
 assert( lp.str().find( " obj: x_1" ) != std::string::npos );

 std::cout << "set_objective replaces: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* mirror() of an empty Block gives an empty copy that knows what it is a
 * copy of, with nothing to report and nothing to move; a second mirror(), a
 * mirror() of nothing and a mirror() into a Block that is not empty are
 * refused. */

static void test_mirror_of_an_empty_Block( void )
{
 AbstractBlock original;
 AbstractBlock copy;

 copy.mirror( & original );
 assert( copy.get_mirrored() == & original );
 assert( copy.get_mirror_issues().empty() );
 assert( copy.get_static_variable_groups().empty() );
 assert( copy.get_dynamic_variable_groups().empty() );
 assert( copy.get_static_constraint_groups().empty() );
 assert( copy.get_dynamic_constraint_groups().empty() );
 assert( copy.get_nested_Blocks().empty() );
 assert( ! copy.get_objective() );
 copy.mirror_read();
 copy.mirror_write();

 bool refused = false;
 try { copy.mirror( & original ); }
 catch( const std::logic_error & ) { refused = true; }
 assert( refused );

 AbstractBlock other;
 refused = false;
 try { other.mirror( nullptr ); }
 catch( const std::invalid_argument & ) { refused = true; }
 assert( refused );
 assert( ! other.get_mirrored() );

 AbstractBlock full;
 full.add_static_variable( * new std::vector< ColVariable >( 1 ) , "x" );
 refused = false;
 try { full.mirror( & original ); }
 catch( const std::logic_error & ) { refused = true; }
 assert( refused );

 std::cout << "mirror of an empty Block: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The edges of add_dynamic_*() and remove_dynamic_*(), for Variable and for
 * Constraint alike: adding nothing, adding to an empty list, a Range that is
 * empty (anywhere, even reversed or on an empty list), one to the end, one
 * that covers everything and one past the end (which, unlike the Range of a
 * LinearFunction, is refused rather than cut), an empty Subset (which means
 * "all of them"), an unordered one, an unordered one that is contiguous
 * (hence a Range), one that names every element and one with an index out
 * of the list. What is checked is the list, the Block and the group of the
 * elements, the size of the group, and the BlockModAdd / BlockModRmvRngd /
 * BlockModRmvSbst the Block is sent under eModBlck, which is sent whether or
 * not a Solver is listening. */

/// an AbstractBlock that records every Modification it is sent

class RecBlock : public AbstractBlock
{
 public:

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  v_seen.push_back( mod );
  AbstractBlock::add_Modification( mod , chnl );
  }

 Lst_sp_Mod v_seen;  ///< what the Block has been sent, in order
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the one Modification the Block has been sent, as a T, the record emptied

template< class T >
static std::shared_ptr< T > take( RecBlock * b )
{
 assert( b->v_seen.size() == 1 );
 auto mod = std::dynamic_pointer_cast< T >( b->v_seen.front() );
 assert( mod );
 b->v_seen.clear();
 return( mod );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the addresses of the elements of a list, in order

template< class T >
static std::vector< const T * > addrs( const std::list< T > & l )
{
 std::vector< const T * > rv;
 for( const auto & el : l )
  rv.push_back( & el );
 return( rv );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// the elements of v in the given positions

template< class T >
static std::vector< const T * > pick( const std::vector< const T * > & v ,
				      std::initializer_list< Block::Index >
				      pos )
{
 std::vector< const T * > rv;
 for( auto i : pos )
  rv.push_back( v[ i ] );
 return( rv );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// what tells Variable from Constraint, for the template below

template< class T >
static void add_d( Block * b , std::list< T > & l , std::list< T > & n ,
		   ModParam iM = eModBlck )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->add_dynamic_variables( l , n , iM );
 else
  b->add_dynamic_constraints( l , n , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l , Block::Range range ,
		   ModParam iM = eModBlck )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variables( l , range , iM , iM );
 else
  b->remove_dynamic_constraints( l , range , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l , Block::Subset && nms ,
		   bool ordered , ModParam iM = eModBlck )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variables( l , std::move( nms ) , ordered , iM , iM );
 else
  b->remove_dynamic_constraints( l , std::move( nms ) , ordered , iM );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< class T >
static void test_dynamic_edges_of( RecBlock * b , std::list< T > & l ,
				   const BaseGroup * group )
{
 using Range = Block::Range;
 using Subset = Block::Subset;
 static constexpr bool isvar = std::is_base_of_v< Variable , T >;

 // the list with n new elements, nothing recorded, their addresses
 auto fill = [ b , & l ]( Block::Index n ) {
  rmv_d( b , l , Subset() , false , eNoMod );
  std::list< T > nl( n );
  add_d( b , l , nl , eNoMod );
  b->v_seen.clear();
  return( addrs( l ) );
  };

 // a removal that leaves the positions left of the old list and sends a
 // BlockModRmvRngd with the Range, or a BlockModRmvSbst with the Subset if
 // the Range is empty, carrying the elements in the positions gone
 auto removed = [ b , & l ]( const std::vector< const T * > & old ,
			     std::initializer_list< Block::Index > left ,
			     std::initializer_list< Block::Index > gone ,
			     Range range , const Subset & subset ) {
  assert( addrs( l ) == pick( old , left ) );
  if( range.second > range.first ) {
   auto mod = take< BlockModRmvRngd< T > >( b );
   assert( & mod->whc() == & l );
   assert( ( ! mod->is_added() ) && ( mod->is_variable() == isvar ) );
   assert( mod->range() == range );
   assert( addrs( mod->removed() ) == pick( old , gone ) );
   }
  else {
   auto mod = take< BlockModRmvSbst< T > >( b );
   assert( & mod->whc() == & l );
   assert( ( ! mod->is_added() ) && ( mod->is_variable() == isvar ) );
   assert( mod->subset() == subset );
   assert( addrs( mod->removed() ) == pick( old , gone ) );
   }
  };

 const Range NR( 0 , 0 );  // not a Range: a Subset is expected

 // ---- adding -------------------------------------------------------------

 {                              // nothing, to a list with something in it
  auto old = fill( 2 );
  std::list< T > nl;
  add_d( b , l , nl );
  assert( addrs( l ) == old );
  assert( b->v_seen.empty() );
 }
 {                              // nothing to nothing
  fill( 0 );
  std::list< T > nl;
  add_d( b , l , nl );
  assert( l.empty() && b->v_seen.empty() );
 }
 {                              // to an empty list, then to a non-empty one
  fill( 0 );
  for( Block::Index first : { 0 , 3 } ) {
   std::list< T > nl( 3 );
   auto want = addrs( nl );
   add_d( b , l , nl );
   assert( nl.empty() );        // spliced, not copied
   assert( l.size() == first + 3 );
   auto now = addrs( l );
   assert( std::equal( want.begin() , want.end() , now.begin() + first ) );
   for( const auto & el : l ) {
    assert( el.get_Block() == b );
    assert( el.get_Group() == group );
    }
   assert( group->get_num_elements() == first + 3 );

   auto mod = take< BlockModAdd< T > >( b );
   assert( & mod->whc() == & l );
   assert( mod->is_added() && ( mod->is_variable() == isvar ) );
   assert( mod->first() == first );
   assert( mod->added().size() == 3 );
   for( Block::Index k = 0 ; k < 3 ; ++k )
    assert( mod->added()[ k ] == want[ k ] );
   }
 }

 // ---- removing by Range --------------------------------------------------

 {                              // empty, anywhere: nothing, and no throw
  auto old = fill( 3 );
  rmv_d( b , l , Range( 1 , 1 ) );
  rmv_d( b , l , Range( 3 , 3 ) );
  rmv_d( b , l , Range( 2 , 1 ) );       // reversed
  rmv_d( b , l , Range( 7 , 7 ) );       // past the end, but empty
  assert( addrs( l ) == old );
  assert( b->v_seen.empty() );
  fill( 0 );
  rmv_d( b , l , Range( 0 , 0 ) );       // on an empty list
  assert( l.empty() && b->v_seen.empty() );
 }
 {                              // to the end
  auto old = fill( 3 );
  rmv_d( b , l , Range( 1 , 3 ) );
  removed( old , { 0 } , { 1 , 2 } , Range( 1 , 3 ) , {} );
  assert( group->get_num_elements() == 1 );
 }
 {                              // all: said with an empty Subset
  auto old = fill( 3 );
  rmv_d( b , l , Range( 0 , 3 ) );
  removed( old , {} , { 0 , 1 , 2 } , NR , {} );
  assert( group->get_num_elements() == 0 );
 }
 {                              // past the end: refused, nothing done
  auto old = fill( 3 );
  bool thrown = false;
  try { rmv_d( b , l , Range( 1 , 4 ) ); }
  catch( const std::invalid_argument & ) { thrown = true; }
  assert( thrown );
  assert( addrs( l ) == old );
  assert( b->v_seen.empty() );
 }
 {                              // anything but nothing from an empty list
  fill( 0 );
  bool thrown = false;
  try { rmv_d( b , l , Range( 0 , 1 ) ); }
  catch( const std::invalid_argument & ) { thrown = true; }
  assert( thrown );
  assert( b->v_seen.empty() );
 }
 {                              // silently: done, and nothing sent
  auto old = fill( 3 );
  rmv_d( b , l , Range( 0 , 2 ) , eNoMod );
  assert( addrs( l ) == pick( old , { 2 } ) );
  assert( b->v_seen.empty() );
 }

 // ---- removing by Subset -------------------------------------------------

 {                              // empty: all of them
  auto old = fill( 3 );
  rmv_d( b , l , Subset() , true );
  removed( old , {} , { 0 , 1 , 2 } , NR , {} );
 }
 {                              // empty, on an empty list: nothing
  fill( 0 );
  rmv_d( b , l , Subset() , false );
  assert( l.empty() && b->v_seen.empty() );
 }
 {                              // unordered: ordered inside
  auto old = fill( 4 );
  rmv_d( b , l , Subset( { 3 , 0 } ) , false );
  removed( old , { 1 , 2 } , { 0 , 3 } , NR , { 0 , 3 } );
  assert( group->get_num_elements() == 2 );
 }
 {                              // unordered and contiguous: a Range
  auto old = fill( 4 );
  rmv_d( b , l , Subset( { 2 , 1 } ) , false );
  removed( old , { 0 , 3 } , { 1 , 2 } , Range( 1 , 3 ) , {} );
 }
 {                              // one element, the last one
  auto old = fill( 4 );
  rmv_d( b , l , Subset( { 3 } ) , true );
  removed( old , { 0 , 1 , 2 } , { 3 } , Range( 3 , 4 ) , {} );
 }
 {                              // every one by name: all of them
  auto old = fill( 3 );
  rmv_d( b , l , Subset( { 2 , 0 , 1 } ) , false );
  removed( old , {} , { 0 , 1 , 2 } , NR , {} );
 }
 {                              // an index out of the list: refused
  auto old = fill( 4 );
  bool thrown = false;
  try { rmv_d( b , l , Subset( { 4 , 0 } ) , false ); }
  catch( const std::invalid_argument & ) { thrown = true; }
  assert( thrown );
  assert( addrs( l ) == old );
  assert( b->v_seen.empty() );
 }
 {                              // anything but "all" from an empty list
  fill( 0 );
  bool thrown = false;
  try { rmv_d( b , l , Subset( { 0 } ) , true ); }
  catch( const std::invalid_argument & ) { thrown = true; }
  assert( thrown );
  assert( b->v_seen.empty() );
 }
 {                              // silently: done, and nothing sent
  auto old = fill( 4 );
  rmv_d( b , l , Subset( { 3 , 1 } ) , false , eNoMod );
  assert( addrs( l ) == pick( old , { 0 , 2 } ) );
  assert( b->v_seen.empty() );
 }

 fill( 0 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void test_dynamic_edges( void )
{
 auto b = new RecBlock;

 // one list of Variable and one of Constraint
 auto vars = new std::list< ColVariable >;
 b->add_dynamic_variable( *vars , "y" );
 auto rows = new std::list< FRowConstraint >;
 b->add_dynamic_constraint( *rows , "c" );

 test_dynamic_edges_of( b , *vars ,
			b->get_dynamic_variable_groups().back().get() );
 test_dynamic_edges_of( b , *rows ,
			b->get_dynamic_constraint_groups().back().get() );

 // ---- a vector of lists: one group, a list per cell ----------------------

 auto cells = new std::vector< std::list< ColVariable > >( 3 );
 b->add_dynamic_variable( *cells , "z" );
 const auto group = b->get_dynamic_variable_groups().back().get();
 assert( group->get_num_elements() == 0 );
 {
  std::list< ColVariable > nl( 2 );
  b->add_dynamic_variables( ( *cells )[ 1 ] , nl );
  auto mod = take< BlockModAdd< ColVariable > >( b );
  assert( & mod->whc() == & ( *cells )[ 1 ] );
  assert( mod->first() == 0 );         // the position in its own list
 }
 {
  std::list< ColVariable > nl( 1 );
  b->add_dynamic_variables( ( *cells )[ 2 ] , nl );
  b->v_seen.clear();
 }
 assert( ( *cells )[ 0 ].empty() );
 assert( group->get_num_elements() == 3 );
 for( auto & cell : *cells )
  for( auto & v : cell )
   assert( ( v.get_Block() == b ) && ( v.get_Group() == group ) );

 // removing from one cell leaves the others alone
 b->remove_dynamic_variables( ( *cells )[ 1 ] , Block::Subset() );
 assert( ( *cells )[ 1 ].empty() && ( ( *cells )[ 2 ].size() == 1 ) );
 assert( group->get_num_elements() == 1 );
 assert( take< BlockModRmvSbst< ColVariable > >( b )->subset().empty() );

 // ---- what goes away leaves the stuff it was in --------------------------

 auto x = new std::vector< ColVariable >( 1 );
 b->add_static_variable( *x , "x" );
 for( ModParam iM : { eNoMod , eModBlck } ) {
  // a dynamic Constraint on a static Variable: removing the Constraint
  // takes it off the active stuff of the Variable
  std::list< FRowConstraint > nl( 2 );
  for( auto & r : nl )
   r.set_function( new LinearFunction( { { & ( *x )[ 0 ] , 1.0 } } ) ,
		   eNoMod );
  b->add_dynamic_constraints( *rows , nl , eNoMod );
  assert( ( *x )[ 0 ].get_num_active() == 2 );
  b->remove_dynamic_constraints( *rows , Block::Range( 0 , 1 ) , iM );
  assert( ( *x )[ 0 ].get_num_active() == 1 );
  b->remove_dynamic_constraints( *rows , Block::Subset() , false , iM );
  assert( ( *x )[ 0 ].get_num_active() == 0 );
  assert( rows->empty() );
  b->v_seen.clear();
  }

 {
  // a dynamic Variable in a static Constraint: removing the Variable takes
  // it off the Function of the Constraint
  std::list< ColVariable > nl( 2 );
  b->add_dynamic_variables( *vars , nl , eNoMod );
  auto c = new std::vector< FRowConstraint >( 1 );
  LinearFunction::v_coeff_pair p;
  for( auto & v : *vars )
   p.push_back( { & v , 1.0 } );
  ( *c )[ 0 ].set_function( new LinearFunction( std::move( p ) ) , eNoMod );
  b->add_static_constraint( *c , "k" );
  assert( ( *c )[ 0 ].get_function()->get_num_active_var() == 2 );
  b->remove_dynamic_variables( *vars , Block::Range( 1 , 2 ) , eModBlck ,
			       eNoMod );
  assert( ( *c )[ 0 ].get_function()->get_num_active_var() == 1 );
  assert( ( *c )[ 0 ].get_function()->get_active_var( 0 ) ==
	  & vars->front() );
  b->remove_dynamic_variables( *vars , Block::Subset() , false , eNoMod ,
			       eNoMod );
  assert( ( *c )[ 0 ].get_function()->get_num_active_var() == 0 );
  b->v_seen.clear();
 }

 delete b;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 test_solution_round_trip();
 test_lp_round_trip();
 test_mps_round_trip();
 test_is();
 test_writers_edge_cases();
 test_is_edge_cases();

 test_serialize_empty();
 test_serialize_dynamic_groups();
 test_serialize_one_var_constraint();
 test_serialize_column_in_no_row();
 test_serialize_quadratic_objective();
 test_serialize_nested();
 test_serialize_no_objective();
 test_set_objective_replaces();
 test_mirror_of_an_empty_Block();
 test_read_lp_of_an_empty_model();
 test_read_lp_edges();
 test_read_lp_malformed();
 test_dynamic_edges();

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_AbstractBlock.cpp --------------------*/
/*--------------------------------------------------------------------------*/

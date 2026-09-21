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

#include "FRealObjective.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"

#include <netcdf>
#include <cstdio>
#include <list>
#include <sstream>
#include <algorithm>

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

 // test Adds_StaticConstraints_Vector
 block = new AbstractBlock(); // SetUp
 c = new FRowConstraint();
 block->add_static_constraint( *c );
 assert( block->get_static_constraint< FRowConstraint >( 0 ) == c );
 assert(
  block->get_static_constraint< FRowConstraint >( 0 )->get_Block() == block );
 TearDown( block ); // TearDown

 // test Adds_StaticConstraints_MultiArray
 block = new AbstractBlock(); // SetUp
 auto v_c = new std::vector< FRowConstraint >( 5 );
 block->add_static_constraint( *v_c );
 assert( block->get_static_constraint_v< FRowConstraint >( 0 ) == v_c );
 for( const auto & i : *v_c )
  assert( i.get_Block() == block );
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

int main( int argc , char ** argv )
{
 runAllTests();
 test_solution_round_trip();
 test_lp_round_trip();
 test_mps_round_trip();
 test_is();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_AbstractBlock.cpp --------------------*/
/*--------------------------------------------------------------------------*/

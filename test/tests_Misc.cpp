/*--------------------------------------------------------------------------*/
/*--------------------------- File tests_Misc.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the smaller pieces of the core that have no test of their
 * own: GlobalInformation and its Collection, SimpleDataMapping, Change and
 * GroupChange, BoxSolver, UpdateSolver, and what the base Solver class does
 * by itself (registration to a Block, the queue of Modification, the tables
 * of the parameters).
 *
 * BoxSolver is taken over box LPs and separable box QPs whose optimum is
 * known in closed form, in both senses, and over the two cases where there
 * is no optimum, a box that is empty and a cost pushing towards an infinite
 * bound, against what the Solver interface says has to be answered then.
 *
 * A check that fails because of a defect of the library prints what it
 * found and what it expected, and the test goes on with the next one; the
 * return value of main() says whether any failed.
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
#include "AbstractChange.h"
#include "AbstractPath.h"
#include "BoxSolver.h"
#include "Change.h"
#include "DataMapping.h"
#include "DQuadFunction.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "GlobalInformation.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "UpdateSolver.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using Range = Block::Range;
using Subset = Block::Subset;
using MF_dbl_it = Block::MF_dbl_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr double c_eps = 1e-10;
static constexpr double INF = Inf< double >();

/*--------------------------------------------------------------------------*/
/*---------------------------- A TEST Change -------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Change setting the value of one ColVariable of the first static group

class ValueChange : public Change {

 public:

 explicit ValueChange( Index i = 0 , double v = 0 )
  : Change() , f_i( i ) , f_v( v ) {}

 using Change::serialize;

 Change * apply( Block * block , bool doUndo = false ,
		 ModParam issueMod = eNoBlck ,
		 ModParam issueAMod = eNoBlck ) override {
  auto & x = ( *block->get_static_variable_v< ColVariable >( 0 ) )[ f_i ];
  Change * undo = doUndo ? new ValueChange( f_i , x.get_value() ) : nullptr;
  x.set_value( f_v );
  return( undo );
  }

 void serialize( netCDF::NcGroup & group ) const override {
  Change::serialize( group );
  group.putAtt( "index" , netCDF::NcUint() , f_i );
  group.putAtt( "value" , netCDF::NcDouble() , f_v );
  }

 void deserialize( const netCDF::NcGroup & group ) override {
  group.getAtt( "index" ).getValues( & f_i );
  group.getAtt( "value" ).getValues( & f_v );
  }

 [[nodiscard]] Index index( void ) const { return( f_i ); }
 [[nodiscard]] double value( void ) const { return( f_v ); }

 protected:

 void print( std::ostream & output ) const override {
  output << "ValueChange x[ " << f_i << " ] = " << f_v;
  }

 private:

 Index f_i;
 double f_v;

 SMSpp_insert_in_factory_h;
 };

SMSpp_insert_in_factory_cpp_0( ValueChange );

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failures = 0;  ///< number of failed checks of the library

/// records a failed check of the library, saying what was found

static void expect( bool ok , const std::string & what )
{
 if( ok )
  return;
 std::cout << "FAILED: " << what << std::endl;
 ++n_failures;
 }

/*--------------------------------------------------------------------------*/

static bool equal( double a , double b )
{
 if( std::isinf( b ) )
  return( a == b );
 return( std::abs( a - b ) <= c_eps * std::max( 1.0 , std::abs( b ) ) );
 }

/*--------------------------------------------------------------------------*/
/// true if calling f() throws an E

template< class E , class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( E & ) {
  return( true );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// an AbstractBlock with n ColVariable in its first static group, "x"

static AbstractBlock * block_with_x( Index n , Block * father = nullptr )
{
 auto block = new AbstractBlock( father );
 block->add_static_variable( *( new std::vector< ColVariable >( n ) ) , "x" );
 return( block );
 }

/*--------------------------------------------------------------------------*/

static std::vector< ColVariable > & x_of( Block * block )
{
 return( *block->get_static_variable_v< ColVariable >( 0 ) );
 }

/*--------------------------------------------------------------------------*/
/// the values of the ColVariable in the first static group

static std::vector< double > values_of( Block * block )
{
 std::vector< double > v;
 for( auto & x : x_of( block ) )
  v.push_back( x.get_value() );
 return( v );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- GlobalInformation ------------------------------*/
/*--------------------------------------------------------------------------*/
/* Collection are added, found by name and type, and removed; a name is
 * taken once whatever the type; the values of a Collection are read and
 * written, directly and through a functor; the atomic form gives out a
 * reference that stays valid as the map grows. */

static void test_GlobalInformation( void )
{
 GlobalInformation gi;

 assert( ! gi.exists( "pool" ) );
 assert( ! gi.get_from_Universe< int >( "pool" ) );

 gi.add_to_Universe< int >( "pool" );
 assert( gi.exists( "pool" ) );

 // a name is taken once, whatever the type
 assert( throws< std::runtime_error >( [ & ]() {
	  gi.add_to_Universe< int >( "pool" ); } ) );
 assert( throws< std::runtime_error >( [ & ]() {
	  gi.add_to_Universe< double >( "pool" ); } ) );

 // found with its type, not with another one
 auto pool = gi.get_from_Universe< int >( "pool" );
 assert( pool );
 assert( ! gi.get_from_Universe< double >( "pool" ) );
 assert( gi.get_from_Universe< int >( "pool" ) == pool );
 {
  const GlobalInformation & cgi = gi;
  assert( cgi.get_from_Universe< int >( "pool" ) == pool );
  }

 // reading and writing the values
 int v = -1;
 assert( pool->size() == 0 );
 assert( ! pool->read( "a" , v ) && ( v == -1 ) );
 pool->write( "a" , 3 );
 pool->write( "b" , 4 );
 pool->write( "a" , 5 );
 assert( pool->size() == 2 );
 assert( pool->contains( "a" ) && ( ! pool->contains( "c" ) ) );
 assert( pool->read( "a" , v ) && ( v == 5 ) );
 {
  auto keys = pool->keys();
  std::sort( keys.begin() , keys.end() );
  assert( ( keys == std::vector< std::string >( { "a" , "b" } ) ) );
  }
 int sum = 0;
 pool->for_each( [ & sum ]( const std::string & , int val ) { sum += val; } );
 assert( sum == 9 );

 // the functors run on the value found, and are not run on a missing key
 bool run = false;
 assert( pool->write_with( "b" , []( int & val ) { val *= 10; } ) );
 assert( pool->read_with( "b" , [ & v ]( const int & val ) { v = val; } ) &&
	 ( v == 40 ) );
 assert( ! pool->write_with( "c" , [ & run ]( int & ) { run = true; } ) );
 assert( ! pool->read_with( "c" , [ & run ]( const int & ) { run = true; } ) );
 assert( ! run );

 // the atomic form: operator[] creates the value as T{}, and the reference
 // it gives stays valid while the map grows past a rehash
 gi.add_to_Universe< std::atomic< double > >(
				       GlobalInformation::str_AtomicScalars );
 auto scalars = gi.get_from_Universe< std::atomic< double > >(
				       GlobalInformation::str_AtomicScalars );
 assert( scalars );
 auto & inc = ( *scalars )[ GlobalInformation::str_Incumbent ];
 assert( inc.load() == 0 );
 inc.store( 12.5 );
 for( int i = 0 ; i < 1000 ; ++i )
  scalars->write( "k" + std::to_string( i ) , i );
 assert( scalars->size() == 1001 );
 double d = 0;
 assert( scalars->read( GlobalInformation::str_Incumbent , d ) && ( d == 12.5 ) );
 assert( & ( *scalars )[ GlobalInformation::str_Incumbent ] == & inc );
 scalars->write( GlobalInformation::str_Incumbent , 7 );
 assert( inc.load() == 7 );
 assert( ! scalars->read( "missing" , d ) && ( d == 12.5 ) );

 // removal: whoever holds the Collection keeps it, and the name is free
 gi.remove_from_Universe( "pool" );
 assert( ! gi.exists( "pool" ) );
 assert( ! gi.get_from_Universe< int >( "pool" ) );
 assert( pool->read( "a" , v ) && ( v == 5 ) );
 gi.add_to_Universe< double >( "pool" );
 assert( gi.get_from_Universe< double >( "pool" ) );
 assert( gi.get_from_Universe< double >( "pool" )->size() == 0 );
 gi.remove_from_Universe( "nonesuch" );  // removing nothing is not an error

 std::cout << "GlobalInformation: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- DataMapping ---------------------------------*/
/*--------------------------------------------------------------------------*/
/// the method of the tests: sets the values of x[ range ]

static void set_x_range( Block * block , MF_dbl_it data , Range range ,
			 ModParam , ModParam )
{
 auto & x = x_of( block );
 for( Index i = range.first ; i < range.second ; ++i )
  x[ i ].set_value( *( data++ ) );
 }

/// the method of the tests: sets the values of x[ subset ]

static void set_x_subset( Block * block , MF_dbl_it data , Subset && subset ,
			  bool , ModParam , ModParam )
{
 auto & x = x_of( block );
 for( auto i : subset )
  x[ i ].set_value( *( data++ ) );
 }

/*--------------------------------------------------------------------------*/
/* SimpleDataMapping takes the SetFrom part of the data and gives it to the
 * SetTo part of the caller, broadcasting when SetFrom is the smaller; the
 * serialization gives back a mapping that does the same to the caller found
 * again from the reference Block, and the one whose sets cannot be matched
 * is refused when it is read. */

static void test_DataMapping( void )
{
 using FR = Block::FunctionType< MF_dbl_it , Range >;
 using FS = Block::FunctionType< MF_dbl_it , Subset && , bool >;
 Block::register_method( "tests_Misc::set_x" , new FR( & set_x_range ) );
 Block::register_method( "tests_Misc::set_x" , new FS( & set_x_subset ) );
 auto fr = Block::get_method< FR >( "tests_Misc::set_x" );
 auto fs = Block::get_method< FS >( "tests_Misc::set_x" );
 assert( fr && fs );
 assert( Block::get_method_name( fr ) == "tests_Misc::set_x" );

 // the caller is a sub-Block of the reference one
 auto outer = block_with_x( 1 );
 auto inner = block_with_x( 4 , outer );
 outer->add_nested_Block( inner );

 const std::vector< double > data = { 10 , 11 , 12 , 13 , 14 , 15 };

 SimpleDataMapping< Range , Range > rr( fr , inner , Range( 1 , 3 ) ,
					Range( 0 , 2 ) );
 rr.set_data( data.cbegin() );
 assert( ( values_of( inner ) == std::vector< double >( { 11 , 12 , 0 , 0 } ) ) );

 // subsets, SetTo not ordered: the i-th of SetFrom (an ordered multiset)
 // goes to the i-th of SetTo
 SimpleDataMapping< Subset , Subset > ss( fs , inner , Subset( { 0 , 5 } ) ,
					  Subset( { 3 , 1 } ) );
 ss.set_data( data.cbegin() );
 assert( ( values_of( inner ) == std::vector< double >( { 11 , 15 , 0 , 10 } ) ) );

 // one value broadcast over three
 SimpleDataMapping< Range , Range > bc( fr , inner , Range( 4 , 5 ) ,
					Range( 1 , 4 ) );
 bc.set_data( data.cbegin() );
 assert( ( values_of( inner ) == std::vector< double >( { 11 , 14 , 14 , 14 } ) ) );

 // the round trip
 const char * file = "Misc_test_DataMapping.nc4";
 {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  auto g1 = nc.addGroup( "RR" );
  rr.serialize( g1 , outer );
  auto g2 = nc.addGroup( "SS" );
  ss.serialize( g2 , outer );
  auto g3 = nc.addGroup( "BAD" );
  SimpleDataMapping< Range , Range > bad( fr , inner , Range( 0 , 2 ) ,
					  Range( 0 , 3 ) );
  bad.serialize( g3 , outer );
  }
 {
  netCDF::NcFile nc( file , netCDF::NcFile::read );
  auto rr2 = SimpleDataMappingFactory::deserialize( nc.getGroup( "RR" ) ,
						    outer );
  auto ss2 = SimpleDataMappingFactory::deserialize( nc.getGroup( "SS" ) ,
						    outer );
  assert( rr2 && ss2 );
  using SDM_RR = SimpleDataMapping< Range , Range >;
  using SDM_SS = SimpleDataMapping< Subset , Subset >;
  assert( dynamic_cast< SDM_RR * >( rr2 ) );
  assert( dynamic_cast< SDM_SS * >( ss2 ) );

  for( auto & x : x_of( inner ) )
   x.set_value( 0 );
  rr2->set_data( data.cbegin() );
  ss2->set_data( data.cbegin() );
  assert( ( values_of( inner ) ==
	    std::vector< double >( { 11 , 15 , 0 , 10 } ) ) );
  assert( ( values_of( outer ) == std::vector< double >( { 0 } ) ) );
  delete rr2;
  delete ss2;

  // two values over three: neither is a multiple of the other
  assert( throws< std::logic_error >( [ & ]() {
	   delete SimpleDataMappingFactory::deserialize( nc.getGroup( "BAD" ) ,
							 outer ); } ) );
  }
 std::remove( file );

 delete outer;

 std::cout << "SimpleDataMapping: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------ Change and GroupChange --------------------------*/
/*--------------------------------------------------------------------------*/
/* A Change applied with and without its undo; a GroupChange applies its
 * sub-Change in order and gives back the undo of each in the reverse order,
 * nested ones included; a Change is written to a file and read back through
 * the factory, also by its position in the file. */

static void test_Change( void )
{
 auto block = block_with_x( 2 );
 x_of( block )[ 0 ].set_value( 1 );
 x_of( block )[ 1 ].set_value( 2 );

 // the factory knows both, by name
 {
  auto c = Change::new_Change( "ValueChange" );
  assert( c && ( c->classname() == "ValueChange" ) );
  delete c;
  auto g = Change::new_Change( "GroupChange" );
  assert( g && ( g->classname() == "GroupChange" ) );
  delete g;
  assert( throws< std::invalid_argument >( []() {
	   delete Change::new_Change( "nonesuch" ); } ) );
  }

 // no undo is asked, none is given
 ValueChange c0( 0 , 5 );
 assert( c0.apply( block ) == nullptr );
 assert( ( values_of( block ) == std::vector< double >( { 5 , 2 } ) ) );

 // the undo gives back what was there
 auto undo = c0.apply( block , true );
 assert( undo );
 ValueChange( 0 , 9 ).apply( block );
 delete undo->apply( block );
 delete undo;
 assert( ( values_of( block ) == std::vector< double >( { 5 , 2 } ) ) );

 // a group: two Change on the same ColVariable, and a nested group
 GroupChange group;
 group.add( new ValueChange( 0 , 7 ) );
 group.add( new ValueChange( 0 , 8 ) );
 auto nested = new GroupChange();
 nested->add( new ValueChange( 1 , 3 ) );
 nested->add_front( new ValueChange( 1 , 4 ) );
 group.add( nested );
 assert( group.sub_Changes().size() == 3 );

 auto gundo = group.apply( block , true );
 assert( ( values_of( block ) == std::vector< double >( { 8 , 3 } ) ) );
 auto gu = dynamic_cast< GroupChange * >( gundo );
 assert( gu && ( gu->sub_Changes().size() == 3 ) );
 // the first undo is that of the last sub-Change, the nested group
 assert( dynamic_cast< GroupChange * >( gu->sub_Changes().front() ) );
 delete gundo->apply( block );
 delete gundo;
 assert( ( values_of( block ) == std::vector< double >( { 5 , 2 } ) ) );

 // written and read back
 const char * file = "Misc_test_Change.nc4";
 ValueChange( 1 , 6.5 ).serialize( std::string( file ) );
 {
  auto c = Change::deserialize( std::string( file ) );
  auto vc = dynamic_cast< ValueChange * >( c );
  assert( vc && ( vc->index() == 1 ) && ( vc->value() == 6.5 ) );
  delete vc->apply( block );
  assert( ( values_of( block ) == std::vector< double >( { 5 , 6.5 } ) ) );
  delete c;
  }

 // two in the same file, the second found by its position
 {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  ValueChange( 0 , 1 ).serialize_f( nc );
  ValueChange( 1 , 2 ).serialize_f( nc );
  }
 {
  auto c = Change::deserialize( std::string( file ) + "[1]" );
  auto vc = dynamic_cast< ValueChange * >( c );
  assert( vc && ( vc->index() == 1 ) && ( vc->value() == 2 ) );
  delete c;
  }

 // a GroupChange cannot be read back yet: the factory says so with nullptr
 {
  netCDF::NcFile nc( file , netCDF::NcFile::replace );
  group.serialize_f( nc );
  }
 {
  netCDF::NcFile nc( file , netCDF::NcFile::read );
  auto g = nc.getGroup( "Change_0" );
  std::string type;
  g.getAtt( "type" ).getValues( type );
  assert( type == "GroupChange" );
  assert( Change::new_Change( g ) == nullptr );
  }
 std::remove( file );

 delete block;

 std::cout << "Change and GroupChange: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BoxSolver --------------------------------*/
/*--------------------------------------------------------------------------*/
/// eFixX on a Variable fixed at another value, and the undo of both

static void test_AbstractChange_fix( void )
{
 auto block = block_with_x( 2 );
 auto & x = x_of( block );
 const AbstractPath path( & x[ 0 ] , block );

 auto u1 = AbstractChange( AbstractChange::eFixX , { 1 } , { path } )
            .apply( block , true );
 expect( x[ 0 ].is_fixed() && equal( x[ 0 ].get_value() , 1 ) ,
	 "eFixX: free, fixed at 1" );

 Change * u2 = nullptr;
 expect( ! throws< std::exception >( [ & ]() {
	  u2 = AbstractChange( AbstractChange::eFixX , { 2 } , { path } )
	        .apply( block , true ); } ) ,
	 "eFixX: fixed at 1, fixed again at 2" );
 expect( x[ 0 ].is_fixed() && equal( x[ 0 ].get_value() , 2 ) ,
	 "eFixX: fixed at 2" );

 if( u2 ) {
  delete u2->apply( block );
  delete u2;
  }
 expect( x[ 0 ].is_fixed() && equal( x[ 0 ].get_value() , 1 ) ,
	 "eFixX: the undo fixes it back at 1" );

 delete u1->apply( block );
 delete u1;
 expect( ! x[ 0 ].is_fixed() , "eFixX: the undo of the first unfixes it" );

 delete block;
 }

/*--------------------------------------------------------------------------*/
/// a box model: ColVariable with a BoxConstraint each, a separable Objective

struct BoxModel {
 AbstractBlock * block;
 std::vector< BoxConstraint > * box;
 FRealObjective * obj;
 BoxSolver * solver;

 /// linear if quad is empty, else the DQuadFunction sum quad x^2 + lin x

 BoxModel( const std::vector< double > & lb , const std::vector< double > & ub ,
	   const std::vector< double > & lin ,
	   const std::vector< double > & quad , int sense ) {
  const Index n = lb.size();
  block = block_with_x( n );
  auto & x = x_of( block );
  box = new std::vector< BoxConstraint >( n );
  block->add_static_constraint( *box , "box" );
  for( Index j = 0 ; j < n ; ++j ) {
   ( *box )[ j ].set_variable( & x[ j ] , eNoMod );
   ( *box )[ j ].set_lhs( lb[ j ] , eNoMod );
   ( *box )[ j ].set_rhs( ub[ j ] , eNoMod );
   }
  Function * f;
  if( quad.empty() ) {
   LinearFunction::v_coeff_pair cp;
   for( Index j = 0 ; j < n ; ++j )
    cp.push_back( { & x[ j ] , lin[ j ] } );
   f = new LinearFunction( std::move( cp ) );
   }
  else {
   DQuadFunction::v_coeff_triple ct;
   for( Index j = 0 ; j < n ; ++j )
    ct.push_back( { & x[ j ] , lin[ j ] , quad[ j ] } );
   f = new DQuadFunction( std::move( ct ) );
   }
  obj = new FRealObjective( block , f );
  obj->set_sense( sense , eNoMod );
  block->set_objective( obj , eNoMod );
  solver = new BoxSolver();
  block->register_Solver( solver );
  }

 /// the value of the Objective at the values in the ColVariable

 double objective_value( void ) {
  obj->compute();
  return( obj->value() );
  }

 ~BoxModel() {
  block->unregister_Solvers( true );
  delete block;
  }
 };

/*--------------------------------------------------------------------------*/
/* A 3-variable box LP, min and max: the value, the solution, the opposite
 * value, and the reduced costs on the BoxConstraint. */

static void test_BoxSolver_LP( void )
{
 const std::vector< double > lb = { 0 , -1 , 1 } , ub = { 2 , 1 , 3 };
 BoxModel m( lb , ub , { 1 , -2 , 0.5 } , {} , Objective::eMin );
 m.solver->set_par( BoxSolver::intPDSol , 2 );

 assert( throws< std::logic_error >( [ & ]() { (void) m.solver->get_lb(); } ) );
 assert( m.solver->compute() == Solver::kOK );
 assert( equal( m.solver->get_var_value() , -1.5 ) );
 assert( equal( m.solver->get_lb() , -1.5 ) &&
	 equal( m.solver->get_ub() , -1.5 ) );
 assert( equal( m.solver->get_opposite_value() , 5.5 ) );
 assert( m.solver->has_var_solution() && m.solver->has_dual_solution() );
 m.solver->get_var_solution();
 assert( ( values_of( m.block ) == std::vector< double >( { 0 , 1 , 1 } ) ) );
 assert( equal( m.objective_value() , -1.5 ) );
 m.solver->get_dual_solution();
 for( Index j = 0 ; j < 3 ; ++j )
  expect( equal( ( *m.box )[ j ].get_dual() , std::vector< double >(
		  { -1 , 2 , -0.5 } )[ j ] ) , "box LP, min: dual of box " +
	  std::to_string( j ) + " = " +
	  std::to_string( ( *m.box )[ j ].get_dual() ) + ", expected " +
	  std::to_string( std::vector< double >( { -1 , 2 , -0.5 } )[ j ] ) );

 // the sense changes: the ObjectiveMod makes it computed again
 m.obj->set_sense( Objective::eMax );
 assert( m.solver->compute() == Solver::kOK );
 assert( equal( m.solver->get_var_value() , 5.5 ) );
 assert( equal( m.solver->get_opposite_value() , -1.5 ) );
 m.solver->get_var_solution();
 {
  const auto x = values_of( m.block );
  expect( x == std::vector< double >( { 2 , -1 , 3 } ) ,
	  "box LP, min then max: get_var_solution() writes ( " +
	  std::to_string( x[ 0 ] ) + " , " + std::to_string( x[ 1 ] ) + " , " +
	  std::to_string( x[ 2 ] ) + " ), expected ( 2 , -1 , 3 )" );
  }

 // a changed bound makes it computed again
 ( *m.box )[ 2 ].set_rhs( 4 );
 assert( m.solver->compute() == Solver::kOK );
 assert( equal( m.solver->get_var_value() , 6 ) );

 std::cout << "BoxSolver, box LP: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A separable box QP, with a convex, a convex clipped and a concave term,
 * min and max: the values are the closed-form ones, and the solution written
 * has that value. */

static void test_BoxSolver_QP( void )
{
 // x0^2 - 2 x0 on [0,2], 2 x1^2 + 8 x1 on [-1,1], - x2^2 on [1,3]
 BoxModel m( { 0 , -1 , 1 } , { 2 , 1 , 3 } , { -2 , 8 , 0 } , { 1 , 2 , -1 } ,
	     Objective::eMin );
 m.solver->set_par( BoxSolver::intPDSol , 2 );

 assert( m.solver->compute() == Solver::kOK );
 assert( equal( m.solver->get_var_value() , -16 ) );

 // the dual values: x0 is interior, x1 is at its lower bound with
 // derivative 4 * ( -1 ) + 8 = 4, hence dual - 4, x2 is concave
 m.solver->get_dual_solution();
 assert( ( *m.box )[ 0 ].get_dual() == 0 );
 assert( equal( ( *m.box )[ 1 ].get_dual() , -4 ) );
 assert( ( *m.box )[ 2 ].get_dual() == 0 );

 assert( equal( m.solver->get_opposite_value() , 9 ) );
 m.solver->get_var_solution();
 assert( ( values_of( m.block ) == std::vector< double >( { 1 , -1 , 3 } ) ) );
 assert( equal( m.objective_value() , -16 ) );

 m.obj->set_sense( Objective::eMax );
 assert( m.solver->compute() == Solver::kOK );
 assert( equal( m.solver->get_var_value() , 9 ) );
 m.solver->get_var_solution();
 expect( equal( m.objective_value() , 9 ) , "box QP, min then max: the "
	 "solution written has value " + std::to_string( m.objective_value() )
	 + ", expected 9" );

 // the same in a maximization from the start: x2 is at its lower bound 1
 // with derivative - 2, and the multiplier w of the lower bound is 2
 {
  BoxModel mx( { 0 , -1 , 1 } , { 2 , 1 , 3 } , { -2 , 8 , 0 } ,
	       { 1 , 2 , -1 } , Objective::eMax );
  mx.solver->set_par( BoxSolver::intPDSol , 2 );
  assert( mx.solver->compute() == Solver::kOK );
  assert( equal( mx.solver->get_var_value() , 9 ) );
  mx.solver->get_dual_solution();
  expect( equal( ( *mx.box )[ 2 ].get_dual() , 2 ) , "box QP, max: dual of "
	  "the lower bound of - x2^2 on [ 1 , 3 ] = " +
	  std::to_string( ( *mx.box )[ 2 ].get_dual() ) + ", expected 2" );
  }

 std::cout << "BoxSolver, box QP: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A lower and an upper bound given by two different OneVarConstraint: the
 * dual value goes to the one that is tight at the optimum, and the other
 * one gets 0, whether the dual solution is produced in compute() or asked
 * for afterwards. The sign is that of RowConstraint: the coefficient of the
 * row in the Lagrangian function, i.e., minus the cost in both senses (z - w
 * for a minimization, w - z for a maximization, z the multiplier of the
 * upper bound and w that of the lower one). */

static void test_BoxSolver_duals( void )
{
 struct Case {
  int sense;
  double cost;
  bool lb_tight;
  };
 const std::vector< Case > cases = { { Objective::eMin , 1 , true } ,
				     { Objective::eMin , -1 , false } ,
				     { Objective::eMax , 1 , false } ,
				     { Objective::eMax , -1 , true } };

 for( const auto & c : cases )
  for( int when = 0 ; when < 2 ; ++when ) {
   auto block = block_with_x( 1 );
   auto & x = x_of( block );
   auto lbc = new std::vector< LBConstraint >( 1 );
   auto ubc = new std::vector< UBConstraint >( 1 );
   block->add_static_constraint( *lbc , "lb" );
   block->add_static_constraint( *ubc , "ub" );
   ( *lbc )[ 0 ].set_variable( & x[ 0 ] , eNoMod );
   ( *lbc )[ 0 ].set_lhs( 0 , eNoMod );
   ( *ubc )[ 0 ].set_variable( & x[ 0 ] , eNoMod );
   ( *ubc )[ 0 ].set_rhs( 2 , eNoMod );
   auto obj = new FRealObjective( block , new LinearFunction(
			  LinearFunction::v_coeff_pair( { { & x[ 0 ] , c.cost } } ) ) );
   obj->set_sense( c.sense , eNoMod );
   block->set_objective( obj , eNoMod );
   auto solver = new BoxSolver();
   block->register_Solver( solver );

   // when == 0: produced in compute(); when == 1: asked for afterwards
   if( when == 0 )
    solver->set_par( BoxSolver::intPDSol , 3 );
   assert( solver->compute() == Solver::kOK );
   if( when == 1 )
    solver->set_par( BoxSolver::intPDSol , 3 );
   solver->get_var_solution();
   assert( x[ 0 ].get_value() == ( c.lb_tight ? 0 : 2 ) );
   const std::string where = std::string( c.sense == Objective::eMin ?
					  "min " : "max " ) +
                             std::to_string( c.cost ) + " x on [ 0 , 2 ], "
                             + ( when ? "asked after" : "made in" ) +
                             " compute(): ";
   try {
    solver->get_dual_solution();
    const double dl = ( *lbc )[ 0 ].get_dual();
    const double du = ( *ubc )[ 0 ].get_dual();
    const double want = - c.cost;
    expect( ( dl == ( c.lb_tight ? want : 0 ) ) &&
	    ( du == ( c.lb_tight ? 0 : want ) ) , where + "dual of LB = " +
	    std::to_string( dl ) + ", of UB = " + std::to_string( du ) +
	    ", expected " + std::to_string( c.lb_tight ? want : 0 ) +
	    " and " + std::to_string( c.lb_tight ? 0 : want ) );
    }
   catch( std::exception & e ) {
    expect( false , where + "get_dual_solution() throws: " + e.what() );
    }

   block->unregister_Solvers( true );
   delete block;
   }

 // a maximization bounded by an upper bound alone, the lower one being
 // infinite: the dual solution asked for after compute() is there
 {
  auto block = block_with_x( 1 );
  auto & x = x_of( block );
  auto ubc = new std::vector< UBConstraint >( 1 );
  block->add_static_constraint( *ubc , "ub" );
  ( *ubc )[ 0 ].set_variable( & x[ 0 ] , eNoMod );
  ( *ubc )[ 0 ].set_rhs( 2 , eNoMod );
  auto obj = new FRealObjective( block , new LinearFunction(
			  LinearFunction::v_coeff_pair( { { & x[ 0 ] , 1.0 } } ) ) );
  obj->set_sense( Objective::eMax , eNoMod );
  block->set_objective( obj , eNoMod );
  auto solver = new BoxSolver();
  block->register_Solver( solver );
  assert( solver->compute() == Solver::kOK );
  assert( equal( solver->get_var_value() , 2 ) );
  solver->set_par( BoxSolver::intPDSol , 2 );
  try {
   solver->get_dual_solution();
   expect( ( *ubc )[ 0 ].get_dual() == -1 , "max x, x <= 2: dual of UB = " +
	   std::to_string( ( *ubc )[ 0 ].get_dual() ) + ", expected -1" );
   }
  catch( std::exception & e ) {
   expect( false , std::string( "max x, x <= 2, x free below: "
				"get_dual_solution() throws: " ) + e.what() );
   }
  block->unregister_Solvers( true );
  delete block;
  }

 std::cout << "BoxSolver, dual values: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A box that is empty ( lb > ub ) makes the problem infeasible in both
 * senses, even for a ColVariable with zero cost and even if another one is
 * unbounded; get_lb() and get_ub() then say what the Solver interface says
 * they say for kInfeasible. */

static void test_BoxSolver_empty_box( void )
{
 for( int sense : { Objective::eMin , Objective::eMax } ) {
  const std::string where = sense == Objective::eMin ? "min" : "max";

  // with a cost, and with zero cost
  for( double cost : { 1.0 , 0.0 } ) {
   BoxModel m( { 0 , 3 } , { 2 , 1 } , { 1 , cost } , {} , sense );
   assert( m.solver->compute() == Solver::kInfeasible );
   assert( ! m.solver->has_var_solution() );
   const double want = sense == Objective::eMin ? INF : -INF;
   expect( ( m.solver->get_lb() == want ) && ( m.solver->get_ub() == want ) ,
	   where + " over an empty box, cost " + std::to_string( cost ) +
	   ": get_lb() = " + std::to_string( m.solver->get_lb() ) +
	   ", get_ub() = " + std::to_string( m.solver->get_ub() ) +
	   ", expected both " + std::to_string( want ) );
   }

  // an empty box beats an unbounded direction
  BoxModel m( { 0 , 3 } , { INF , 1 } ,
	      { sense == Objective::eMin ? -1.0 : 1.0 , 1 } , {} , sense );
  assert( m.solver->compute() == Solver::kInfeasible );
  }

 std::cout << "BoxSolver, empty box: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* A cost pushing towards an infinite bound makes the problem unbounded:
 * get_lb() and get_ub() are both infinite on the side of the sense, the
 * opposite value is finite, the direction is along the infinite bound, and
 * the solution, said to be there, can be read. */

static void test_BoxSolver_unbounded( void )
{
 for( int sense : { Objective::eMin , Objective::eMax } ) {
  const std::string where = sense == Objective::eMin ? "min" : "max";
  // min: - 2 x1 with x1 <= INF; max: - 2 x1 with x1 >= -INF
  const bool mn = ( sense == Objective::eMin );
  BoxModel m( { 0 , mn ? -1 : -INF , 1 } , { 2 , mn ? INF : 1 , 3 } ,
	      { 1 , -2 , 0.5 } , {} , sense );

  assert( m.solver->compute() == Solver::kUnbounded );
  const double want = mn ? -INF : INF;
  assert( ( m.solver->get_lb() == want ) && ( m.solver->get_ub() == want ) );
  assert( equal( m.solver->get_opposite_value() , mn ? 5.5 : -1.5 ) );

  assert( m.solver->has_var_direction() );
  m.solver->get_var_direction();
  assert( ( values_of( m.block ) ==
	    std::vector< double >( { 0 , mn ? 1.0 : -1.0 , 0 } ) ) );

  assert( m.solver->has_var_solution() );
  try {
   m.solver->get_var_solution();
   auto x = values_of( m.block );
   bool feasible = true;
   for( Index j = 0 ; j < 3 ; ++j )
    feasible = feasible && ( x[ j ] >= ( *m.box )[ j ].get_lhs() ) &&
               ( x[ j ] <= ( *m.box )[ j ].get_rhs() );
   expect( feasible , where + ", unbounded: the solution is not feasible" );
   }
  catch( std::exception & e ) {
   expect( false , where + ", unbounded: has_var_solution() is true but "
	   "get_var_solution() throws: " + e.what() );
   }
  }

 std::cout << "BoxSolver, unbounded: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ UpdateSolver ------------------------------*/
/*--------------------------------------------------------------------------*/
/* An UpdateSolver attached to a Block passes the Modification it receives,
 * unchanged, to another Block, and from there to its Solver; the options
 * filter them by the Block they come from and by concerns_Block(). Mapping
 * them is left to map_forward_Modification(), which the base Block does
 * not implement, and then nothing arrives and the other Block is not left
 * locked. */

static void test_UpdateSolver( void )
{
 assert( throws< std::invalid_argument >( []() { UpdateSolver( nullptr ); } ) );

 struct Case {
  int options;
  bool from_root_mod , from_root_nomod , from_son;
  };
 // 2: pass unchanged; 4: only from the Block itself; 8: only from its sons;
 // 16: only concerns_Block(); 32: only ! concerns_Block()
 const std::vector< Case > cases = { { 2 , true , true , true } ,
				     { 2 | 4 , true , true , false } ,
				     { 2 | 8 , false , false , true } ,
				     { 2 | 16 , true , false , true } ,
				     { 2 | 32 , false , true , false } ,
				     { 0 , false , false , false } };

 for( const auto & c : cases ) {
  auto a = block_with_x( 2 );
  auto son = block_with_x( 1 , a );
  a->add_nested_Block( son );
  auto b = block_with_x( 1 );
  auto us = new UpdateSolver( b , nullptr , c.options );
  a->register_Solver( us );
  auto fake = new FakeSolver();
  b->register_Solver( fake );
  auto & mods = fake->get_Modification_list();
  mods.clear();

  x_of( a )[ 0 ].is_fixed( true , eModBlck );  // concerns_Block()
  x_of( a )[ 1 ].is_fixed( true , eNoBlck );   // ! concerns_Block()
  x_of( son )[ 0 ].is_fixed( true , eModBlck );

  std::vector< Variable * > expected;
  if( c.from_root_mod )
   expected.push_back( & x_of( a )[ 0 ] );
  if( c.from_root_nomod )
   expected.push_back( & x_of( a )[ 1 ] );
  if( c.from_son )
   expected.push_back( & x_of( son )[ 0 ] );

  assert( mods.size() == expected.size() );
  Index k = 0;
  for( auto & mod : mods ) {
   auto vmod = std::dynamic_pointer_cast< VariableMod >( mod );
   assert( vmod && ( vmod->variable() == expected[ k++ ] ) );
   }
  assert( b->is_owned_by( nullptr ) );  // not left locked

  // an inhibited UpdateSolver passes nothing
  mods.clear();
  us->inhibit_Modification( true );
  x_of( a )[ 0 ].is_fixed( false , eModBlck );
  assert( mods.empty() );

  a->unregister_Solvers( true );
  b->unregister_Solvers( true );
  delete a;
  delete b;
  }

 std::cout << "UpdateSolver: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ Solver base -------------------------------*/
/*--------------------------------------------------------------------------*/
/* The list of Solver registered to a Block: in the order they came, at the
 * front when so asked, a Solver registered twice being there once; each
 * Solver knows its Block, and forgets it when it is unregistered or
 * replaced; the sub-Block know that someone is listening as long as a
 * Solver is there. */

static void test_Solver_registration( void )
{
 auto a = block_with_x( 1 );
 auto son = block_with_x( 1 , a );
 a->add_nested_Block( son );
 auto s1 = new FakeSolver() , s2 = new FakeSolver() , s3 = new FakeSolver();

 assert( ! son->anyone_there() );
 a->register_Solver( s1 );
 a->register_Solver( s2 );
 a->register_Solver( s3 , true );
 a->register_Solver( s1 );
 assert( son->anyone_there() );
 {
  const auto & lst = a->get_registered_solvers();
  assert( ( std::vector< Solver * >( lst.begin() , lst.end() ) ==
	    std::vector< Solver * >( { s3 , s1 , s2 } ) ) );
  }
 assert( ( s1->get_Block() == a ) && ( s2->get_Block() == a ) &&
	 ( s3->get_Block() == a ) );
 assert( throws< std::invalid_argument >( [ & ]() {
	  a->register_Solver( nullptr ); } ) );

 // unregistered, not deleted; unregistering it again does nothing
 a->unregister_Solver( s1 );
 assert( s1->get_Block() == nullptr );
 a->unregister_Solver( s1 );
 assert( a->get_registered_solvers().size() == 2 );

 // s1 takes the place of s3
 a->replace_Solver( s1 , a->get_registered_solvers().begin() );
 {
  const auto & lst = a->get_registered_solvers();
  assert( ( std::vector< Solver * >( lst.begin() , lst.end() ) ==
	    std::vector< Solver * >( { s1 , s2 } ) ) );
  }
 assert( ( s1->get_Block() == a ) && ( s3->get_Block() == nullptr ) );
 delete s3;

 a->unregister_Solver( s2 , true );
 a->unregister_Solver( s1 , true );
 assert( a->get_registered_solvers().empty() );
 assert( ! son->anyone_there() );

 delete a;

 std::cout << "Solver registration: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The queue of Modification a Solver sees: in the order they are issued,
 * from the Block and from its sons; a NBModification wipes what came
 * before; the Modification sent on a channel arrive as one GroupModification
 * when the channel is closed, with the sub-Modification in their order; an
 * inhibited Solver sees nothing. */

static void test_Solver_Modification_queue( void )
{
 auto a = block_with_x( 3 );
 auto son = block_with_x( 1 , a );
 a->add_nested_Block( son );
 auto fake = new FakeSolver();
 a->register_Solver( fake );
 auto & mods = fake->get_Modification_list();
 mods.clear();

 auto & x = x_of( a );
 x[ 2 ].is_fixed( true );
 x_of( son )[ 0 ].is_fixed( true );
 x[ 0 ].is_fixed( true );
 {
  std::vector< Variable * > expected = { & x[ 2 ] , & x_of( son )[ 0 ] ,
					 & x[ 0 ] };
  assert( mods.size() == 3 );
  Index k = 0;
  for( auto & mod : mods ) {
   auto vmod = std::dynamic_pointer_cast< VariableMod >( mod );
   assert( vmod && ( vmod->variable() == expected[ k++ ] ) );
   }
  }

 // a NBModification wipes the queue
 a->add_Modification( std::make_shared< NBModification >( a ) );
 assert( mods.size() == 1 );
 assert( std::dynamic_pointer_cast< NBModification >( mods.front() ) );
 mods.clear();

 // a channel: nothing arrives until it is closed, then one group
 auto chnl = a->open_channel();
 x[ 1 ].is_fixed( true , Observer::make_par( eModBlck , chnl ) );
 x[ 0 ].is_fixed( false , Observer::make_par( eModBlck , chnl ) );
 assert( mods.empty() );
 a->close_channel( chnl );
 assert( mods.size() == 1 );
 {
  auto gmod = std::dynamic_pointer_cast< GroupModification >( mods.front() );
  assert( gmod && ( gmod->sub_Modifications().size() == 2 ) );
  auto m1 = std::dynamic_pointer_cast< VariableMod >(
				       gmod->sub_Modifications().front() );
  auto m2 = std::dynamic_pointer_cast< VariableMod >(
				       gmod->sub_Modifications().back() );
  assert( m1 && m2 && ( m1->variable() == & x[ 1 ] ) &&
	  ( m2->variable() == & x[ 0 ] ) );
  }
 mods.clear();

 // an inhibited Solver sees nothing, and then sees again
 fake->inhibit_Modification( true );
 x[ 1 ].is_fixed( false );
 assert( mods.empty() );
 fake->inhibit_Modification( false );
 x[ 1 ].is_fixed( true );
 assert( mods.size() == 1 );

 a->unregister_Solvers( true );
 delete a;

 std::cout << "Solver Modification queue: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/* The tables of the parameters of Solver, CDASolver and BoxSolver: every
 * index has a name that is read back as the same index, the defaults are
 * those documented, a name that is no parameter is answered with Inf, and
 * an index that is no parameter is thrown at. */

template< class S >
static void check_tables( S & s , const std::string & who )
{
 using idx_type = Solver::idx_type;

 for( idx_type i = 0 ; i < s.get_num_int_par() ; ++i ) {
  assert( s.int_par_str2idx( s.int_par_idx2str( i ) ) == i );
  assert( s.get_int_par( i ) == s.get_dflt_int_par( i ) );
  }
 for( idx_type i = 0 ; i < s.get_num_dbl_par() ; ++i ) {
  assert( s.dbl_par_str2idx( s.dbl_par_idx2str( i ) ) == i );
  const double v = s.get_dbl_par( i ) , d = s.get_dflt_dbl_par( i );
  assert( ( v == d ) || ( std::isnan( v ) && std::isnan( d ) ) );
  }
 for( idx_type i = 0 ; i < s.get_num_str_par() ; ++i ) {
  assert( s.str_par_str2idx( s.str_par_idx2str( i ) ) == i );
  assert( s.get_str_par( i ) == s.get_dflt_str_par( i ) );
  }

 assert( s.int_par_str2idx( "" ) == Inf< idx_type >() );
 assert( s.int_par_str2idx( "intMaxIte" ) == Inf< idx_type >() );
 assert( s.int_par_str2idx( "dblRelAcc" ) == Inf< idx_type >() );
 assert( s.dbl_par_str2idx( "intMaxIter" ) == Inf< idx_type >() );
 assert( s.str_par_str2idx( "nonesuch" ) == Inf< idx_type >() );

 assert( throws< std::invalid_argument >( [ & ]() {
	  (void) s.int_par_idx2str( s.get_num_int_par() ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
	  (void) s.dbl_par_idx2str( s.get_num_dbl_par() ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
	  (void) s.str_par_idx2str( s.get_num_str_par() ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
	  (void) s.get_dflt_int_par( s.get_num_int_par() ); } ) );
 assert( throws< std::invalid_argument >( [ & ]() {
	  (void) s.get_dflt_dbl_par( s.get_num_dbl_par() ); } ) );

 std::cout << who << " parameter tables: done" << std::endl;
 }

static void test_Solver_parameters( void )
{
 FakeSolver fake;
 assert( fake.get_num_int_par() == Solver::intLastAlgPar );
 assert( fake.get_num_dbl_par() == Solver::dblLastAlgPar );
 assert( fake.get_num_str_par() == Solver::strLastAlgPar );
 assert( fake.get_dflt_int_par( Solver::intMaxIter ) == Inf< int >() );
 assert( fake.get_dflt_int_par( Solver::intMaxThread ) == 0 );
 assert( fake.get_dflt_int_par( Solver::intEverykIt ) == 0 );
 assert( fake.get_dflt_int_par( Solver::intMaxSol ) == 1 );
 assert( fake.get_dflt_int_par( Solver::intLogVerb ) == 0 );
 assert( fake.get_dflt_dbl_par( Solver::dblMaxTime ) == INF );
 assert( fake.get_dflt_dbl_par( Solver::dblRelAcc ) == 1e-6 );
 assert( fake.get_dflt_dbl_par( Solver::dblAbsAcc ) == INF );
 assert( fake.get_dflt_dbl_par( Solver::dblUpCutOff ) == INF );
 assert( fake.get_dflt_dbl_par( Solver::dblLwCutOff ) == -INF );
 assert( fake.get_dflt_dbl_par( Solver::dblFAccSol ) == 0 );
 assert( fake.int_par_str2idx( "intMaxThread" ) == Solver::intMaxThread );
 assert( fake.str_par_str2idx( "strLogFileName" ) == Solver::strLogFileName );
 check_tables( fake , "Solver" );

 BoxSolver box;
 assert( box.get_num_int_par() == BoxSolver::intLastParBoxS );
 assert( box.get_num_dbl_par() == CDASolver::dblLastParCDAS );
 assert( box.int_par_str2idx( "intPDSol" ) == BoxSolver::intPDSol );
 assert( box.int_par_str2idx( "intMaxDSol" ) == CDASolver::intMaxDSol );
 assert( box.get_dflt_int_par( BoxSolver::intPDSol ) == 0 );
 assert( box.get_dflt_int_par( CDASolver::intMaxDSol ) == 1 );
 assert( box.get_dflt_dbl_par( CDASolver::dblRAccDSol ) == INF );
 assert( box.get_dflt_dbl_par( CDASolver::dblFAccDSol ) == 0 );
 check_tables( box , "BoxSolver" );

 // intPDSol is kept to its three bits
 box.set_par( BoxSolver::intPDSol , 13 );
 assert( box.get_int_par( BoxSolver::intPDSol ) == 5 );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_GlobalInformation();
 test_DataMapping();
 test_Change();
 test_AbstractChange_fix();
 test_BoxSolver_LP();
 test_BoxSolver_QP();
 test_BoxSolver_duals();
 test_BoxSolver_empty_box();
 test_BoxSolver_unbounded();
 test_UpdateSolver();
 test_Solver_registration();
 test_Solver_Modification_queue();
 test_Solver_parameters();

 if( n_failures ) {
  std::cout << "Misc_unit_test: " << n_failures << " check(s) failed"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File tests_Misc.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

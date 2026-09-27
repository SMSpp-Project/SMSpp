/*--------------------------------------------------------------------------*/
/*---------------------- File tests_BlockModification.cpp ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the Modification a Block issues and for the way they reach
 * the Solver registered to it or to one of its ancestors.
 *
 * A FakeSolver is registered to an AbstractBlock and the list of the
 * Modification it has received is inspected after each change: the tests go
 * over the addition and the removal of dynamic Variable and Constraint (with
 * the empty Range and the empty Subset, which the documentation gives
 * opposite meanings), over the arithmetic of the parameter that says if, how
 * and where a Modification is issued, over the channels that pack them into
 * a GroupModification (nested, forced, empty, emptied, discarded, and
 * hijacking the default channel), and over the path of a Modification from a sub-Block to the
 * Solver of its father.
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
#include "ColVariable.h"
#include "FakeSolver.h"
#include "FRowConstraint.h"
#include "LinearFunction.h"

#include <iostream>
#include <list>
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
using ChnlName = Observer::ChnlName;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// number of failed checks of the documented contract

static int n_failed = 0;

/// records and prints a failed check of the documented contract
/** Used where the documentation of the library promises something that the
 * test found to be false: the failure is printed and counted, and the other
 * cases still run. */

static void expect( bool ok , const char * what )
{
 if( ok )
  return;
 std::cout << "FAILED: " << what << std::endl;
 ++n_failed;
 }

/*--------------------------------------------------------------------------*/
/// true if calling f() throws std::invalid_argument

template< class F >
static bool throws( F f )
{
 try {
  f();
  }
 catch( std::invalid_argument & ) {
  return( true );
  }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// the Modification downcast to M, nullptr if it is not one

template< class M >
static std::shared_ptr< M > as( const sp_Mod & mod )
{
 return( std::dynamic_pointer_cast< M >( mod ) );
 }

/*--------------------------------------------------------------------------*/
/// an AbstractBlock that records every Modification it is given
/** It records what reaches its add_Modification() before handing it to the
 * base class, i.e., what the Block itself "sees", as opposed to what its
 * Solver receive. */

class PeepingBlock : public AbstractBlock {
 public:

 explicit PeepingBlock( Block * father = nullptr )
  : AbstractBlock( father ) {}

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  seen.push_back( mod );
  AbstractBlock::add_Modification( mod , chnl );
  }

 std::vector< sp_Mod > seen;  ///< what the Block has been given
 };

/*--------------------------------------------------------------------------*/
/// true if the stuff is among the active stuff of the Variable, by scanning

static bool listed( const Variable & v , const ThinVarDepInterface * stuff )
{
 for( Index i = 0 ; i < v.get_num_active() ; ++i )
  if( v.get_active( i ) == stuff )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// the values of the ColVariable of a list, in list order

static std::vector< double > values_of( const std::list< ColVariable > & l )
{
 std::vector< double > rv;
 for( const auto & v : l )
  rv.push_back( v.get_value() );
 return( rv );
 }

/*--------------------------------------------------------------------------*/
/// a list of n ColVariable whose values are start, start + 1, ...

static void number( std::list< ColVariable > & l , double start )
{
 for( auto & v : l )
  v.set_value( start++ );
 }

/*--------------------------------------------------------------------------*/
/// gives each row of the list the Function 1 * var and the given RHS
/** The RHS are start, start + 1, ... in list order, which is what tells the
 * rows apart once they have been moved into a Modification. */

static void fill( std::list< FRowConstraint > & rows , ColVariable & var ,
		  double start )
{
 for( auto & row : rows ) {
  row.set_function( new LinearFunction( { { & var , 1.0 } } ) , eNoMod );
  row.set_lhs( - Inf< double >() , eNoMod );
  row.set_rhs( start++ , eNoMod );
  }
 }

/*--------------------------------------------------------------------------*/
/// the RHS of the rows of a list, in list order

static std::vector< double > rhs_of( const std::list< FRowConstraint > & l )
{
 std::vector< double > rv;
 for( const auto & r : l )
  rv.push_back( r.get_rhs() );
 return( rv );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* The arithmetic of the composite parameter: make_par( iM , 0 ) == iM, the
 * mode and the channel come back out of it untouched for every mode and for
 * the extreme channel names, un_ModBlock() / not_ModBlock() downgrade only
 * eModBlck and keep the channel, and par2concern() / not_dry_run() read the
 * mode alone. */

static void test_ModParam_arithmetic( void )
{
 const ModParam modes[] = { eDryRun , eNoMod , eNoBlck , eModBlck };
 const ChnlName chnls[] = { 0 , 1 , 2 , 1000 , Observer::max_channel_name };

 for( auto iM : modes ) {
  assert( Observer::make_par( iM , 0 ) == iM );
  for( auto ch : chnls ) {
   const auto par = Observer::make_par( iM , ch );
   assert( Observer::par2mod( par ) == iM );
   assert( Observer::par2chnl( par ) == ch );
   assert( Observer::par2concern( par ) == ( iM == eModBlck ) );
   assert( Observer::not_dry_run( par ) == ( iM != eDryRun ) );

   const auto down = Observer::un_ModBlock( par );
   assert( Observer::par2chnl( down ) == ch );
   assert( Observer::par2mod( down ) == ( iM == eModBlck ? eNoBlck : iM ) );

   ModParam inplace = par;
   Observer::not_ModBlock( inplace );
   assert( inplace == down );
   }
  }

 // the greatest composite value fits a ModParam without wrapping
 const auto top = Observer::make_par( eModBlck , Observer::max_channel_name );
 assert( Observer::par2chnl( top ) == Observer::max_channel_name );
 assert( Observer::par2mod( top ) == eModBlck );

 std::cout << "ModParam arithmetic: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* issue_mod() and issue_pmod() against anyone_there(): eModBlck always
 * issues an "abstract" Modification and eNoBlck only if someone listens,
 * while a "physical" one treats eModBlck as eNoBlck; eDryRun and eNoMod
 * never issue anything. The channel part of the parameter is irrelevant. */

static void test_issue_mod( void )
{
 auto block = new AbstractBlock;
 for( int there = 0 ; there < 2 ; ++there ) {
  if( there )
   block->register_Solver( new FakeSolver() );
  assert( block->anyone_there() == bool( there ) );

  for( ChnlName ch : { ChnlName( 0 ) , ChnlName( 5 ) } ) {
   assert( ! block->issue_mod( Observer::make_par( eDryRun , ch ) ) );
   assert( ! block->issue_mod( Observer::make_par( eNoMod , ch ) ) );
   assert( block->issue_mod( Observer::make_par( eNoBlck , ch ) ) ==
	   bool( there ) );
   assert( block->issue_mod( Observer::make_par( eModBlck , ch ) ) );

   assert( ! block->issue_pmod( Observer::make_par( eDryRun , ch ) ) );
   if( block->issue_pmod( Observer::make_par( eNoMod , ch ) ) ) {
    std::cout << "        issue_pmod( make_par( eNoMod , " << ch
	      << " ) ) is true with " << ( there ? "a" : "no" )
	      << " Solver" << std::endl;
    expect( false , "issue_pmod() is false for eNoMod (Observer.h: "
	    "\"eNoMod the Modification is *not* issued\")" );
    }
   assert( block->issue_pmod( Observer::make_par( eNoBlck , ch ) ) ==
	   bool( there ) );
   assert( block->issue_pmod( Observer::make_par( eModBlck , ch ) ) ==
	   bool( there ) );
   }
  }

 block->unregister_Solvers( true );
 assert( ! block->anyone_there() );
 delete block;
 std::cout << "issue_mod / issue_pmod: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* open_if_needed() opens a channel only if at least two Modification are
 * going to be issued and the mode issues them at all, nesting it into the
 * channel of the parameter if there is one; close_if_needed() closes only
 * what the former has opened. */

static void test_open_if_needed( void )
{
 auto block = new AbstractBlock;
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();

 // nothing is opened for less than two, nor for eNoMod
 assert( block->open_if_needed( eModBlck , 1 ) == eModBlck );
 assert( block->open_if_needed( eModBlck , 0 ) == eModBlck );
 assert( block->open_if_needed( eNoMod , 5 ) == eNoMod );
 block->close_if_needed( eModBlck , 1 );
 block->close_if_needed( eNoMod , 5 );
 assert( mods.empty() );

 // a new channel, keeping the mode
 auto par = block->open_if_needed( eNoBlck , 2 );
 const auto ch = Observer::par2chnl( par );
 assert( ch != 0 );
 assert( Observer::par2mod( par ) == eNoBlck );

 // nested into the one of the parameter: the name does not change
 auto inner = block->open_if_needed( Observer::make_par( eModBlck , ch ) , 3 );
 assert( Observer::par2chnl( inner ) == ch );
 assert( Observer::par2mod( inner ) == eModBlck );
 block->close_if_needed( inner , 3 );
 assert( mods.empty() );
 block->close_if_needed( par , 2 );

 // the two levels were both closed: one GroupModification holding the
 // empty inner one
 assert( mods.size() == 1 );
 auto gm = as< GroupModification >( mods.front() );
 assert( gm && ( gm->father() == nullptr ) );
 assert( gm->sub_Modifications().size() == 1 );
 assert( as< GroupModification >( gm->sub_Modifications().front() ) );
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "open_if_needed / close_if_needed: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Adding dynamic Variable: one BlockModAdd< ColVariable > whose first() is
 * the size of the list before the addition and whose added() are, in order,
 * the addresses the new Variable have in the list; eNoBlck gives
 * concerns_Block() == false, eNoMod and an empty list to add give nothing,
 * and in all cases the new Variable belong to the Block. */

static void test_add_dynamic_variables( void )
{
 auto block = new AbstractBlock;
 auto x = new std::list< ColVariable >( 2 );
 block->add_dynamic_variable( *x , "x" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 std::list< ColVariable > more( 3 );
 std::vector< ColVariable * > addr;
 for( auto & v : more )
  addr.push_back( & v );

 block->add_dynamic_variables( *x , more );
 assert( more.empty() && ( x->size() == 5 ) );
 assert( mods.size() == 1 );
 auto add = as< BlockModAdd< ColVariable > >( mods.front() );
 assert( add );
 assert( add->is_variable() && add->is_added() && add->concerns_Block() );
 assert( ( & add->whc() == x ) && ( add->first() == 2 ) );
 assert( add->added() == addr );
 assert( add->get_Block() == block );
 {
  // the addresses did not change in the splice: they are x[ 2 ], ... x[ 4 ]
  auto it = std::next( x->begin() , 2 );
  for( auto p : addr )
   assert( & *( it++ ) == p );
  for( auto p : addr )
   assert( p->get_Block() == block );

  std::vector< Variable * > vv;
  add->get_elements( vv );
  assert( vv.size() == 3 );
  std::vector< Constraint * > cv( 7 );
  add->get_elements( cv );
  assert( cv.empty() );
  }

 // eNoBlck: issued, but not a concern of the Block
 mods.clear();
 std::list< ColVariable > one( 1 );
 block->add_dynamic_variables( *x , one , eNoBlck );
 assert( mods.size() == 1 );
 add = as< BlockModAdd< ColVariable > >( mods.front() );
 assert( add && ( ! add->concerns_Block() ) && ( add->first() == 5 ) );

 // eNoMod: done, not issued
 mods.clear();
 std::list< ColVariable > two( 2 );
 block->add_dynamic_variables( *x , two , eNoMod );
 assert( mods.empty() && ( x->size() == 8 ) );
 assert( x->back().get_Block() == block );

 // nothing to add: nothing done, nothing issued
 std::list< ColVariable > none;
 block->add_dynamic_variables( *x , none );
 assert( mods.empty() && ( x->size() == 8 ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "add dynamic Variable: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Adding dynamic Constraint: the same as for Variable, with the new
 * Constraint belonging to the Block and a BlockModAdd< FRowConstraint > that
 * gives no Variable in get_elements(). */

static void test_add_dynamic_constraints( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "v" );
 auto rows = new std::list< FRowConstraint >( 1 );
 fill( *rows , *var , 0 );
 block->add_dynamic_constraint( *rows , "rows" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 std::list< FRowConstraint > more( 2 );
 fill( more , *var , 1 );
 std::vector< FRowConstraint * > addr;
 for( auto & r : more )
  addr.push_back( & r );

 block->add_dynamic_constraints( *rows , more );
 assert( more.empty() && ( rows->size() == 3 ) );
 assert( ( rhs_of( *rows ) == std::vector< double >( { 0 , 1 , 2 } ) ) );
 assert( mods.size() == 1 );
 auto add = as< BlockModAdd< FRowConstraint > >( mods.front() );
 assert( add && ( ! add->is_variable() ) && add->is_added() );
 assert( ( & add->whc() == rows ) && ( add->first() == 1 ) );
 assert( add->added() == addr );
 assert( add->get_Block() == block );
 for( auto p : addr )
  assert( p->get_Block() == block );
 {
  std::vector< Constraint * > cv;
  add->get_elements( cv );
  assert( ( cv.size() == 2 ) && ( cv[ 0 ] == addr[ 0 ] ) );
  std::vector< Variable * > vv( 4 );
  add->get_elements( vv );
  assert( vv.empty() );
  }

 // the base BlockModAD is what a Solver not knowing the type catches
 assert( as< BlockModAD >( mods.front() ) );

 mods.clear();
 std::list< FRowConstraint > none;
 block->add_dynamic_constraints( *rows , none );
 assert( mods.empty() );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "add dynamic Constraint: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing a Range of dynamic Variable: a BlockModRmvRngd with that range
 * and the removed Variable, in their order, still alive inside it; an empty
 * or reversed Range removes and issues nothing; a Range covering the whole
 * list gives a BlockModRmvSbst with an empty subset(); a Range past the end
 * throws. */

static void test_remove_range( void )
{
 auto block = new AbstractBlock;
 auto x = new std::list< ColVariable >( 6 );
 number( *x , 0 );
 block->add_dynamic_variable( *x , "x" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 block->remove_dynamic_variables( *x , Range( 1 , 3 ) );
 assert( ( values_of( *x ) == std::vector< double >( { 0 , 3 , 4 , 5 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvRngd< ColVariable > >( mods.front() );
  assert( rmv && rmv->is_variable() && ( ! rmv->is_added() ) );
  assert( rmv->concerns_Block() && ( & rmv->whc() == x ) );
  assert( rmv->range() == Range( 1 , 3 ) );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 1 , 2 } ) ) );
  assert( rmv->get_Block() == block );
  std::vector< Variable * > vv;
  rmv->get_elements( vv );
  assert( ( vv.size() == 2 ) && ( vv[ 0 ] == & rmv->removed().front() ) );
  }

 // empty and reversed Range: nothing at all
 mods.clear();
 block->remove_dynamic_variables( *x , Range( 2 , 2 ) );
 block->remove_dynamic_variables( *x , Range( 3 , 1 ) );
 assert( mods.empty() && ( x->size() == 4 ) );

 // past the end: an error, and the list is left alone
 assert( throws( [ & ]() {
	  block->remove_dynamic_variables( *x , Range( 2 , 9 ) ); } ) );
 assert( mods.empty() && ( x->size() == 4 ) );

 // eNoMod: done, not issued
 block->remove_dynamic_variables( *x , Range( 0 , 1 ) , eNoMod );
 assert( mods.empty() );
 assert( ( values_of( *x ) == std::vector< double >( { 3 , 4 , 5 } ) ) );

 // the whole list: a BlockModRmvSbst with empty subset()
 block->remove_dynamic_variables( *x , Range( 0 , 3 ) , eNoBlck );
 assert( x->empty() && ( mods.size() == 1 ) );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && rmv->subset().empty() && ( ! rmv->concerns_Block() ) );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 3 , 4 , 5 } ) ) );
  }

 // an empty list with an empty Range is fine, a non-empty Range throws
 mods.clear();
 block->remove_dynamic_variables( *x , Range( 0 , 0 ) );
 assert( throws( [ & ]() {
	  block->remove_dynamic_variables( *x , Range( 0 , 1 ) ); } ) );
 assert( mods.empty() );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "remove a Range: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing a Subset of dynamic Variable: an unordered Subset is ordered
 * inside and the BlockModRmvSbst gives it ordered, with removed() in the
 * same order; a contiguous Subset is a Range, and either Modification must
 * say the same positions; a Subset that is the whole list gives an empty
 * subset(); the empty Subset removes everything, and on an empty list it
 * removes and issues nothing; a position past the end throws. */

static void test_remove_subset( void )
{
 auto block = new AbstractBlock;
 auto x = new std::list< ColVariable >( 8 );
 number( *x , 0 );
 block->add_dynamic_variable( *x , "x" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 block->remove_dynamic_variables( *x , Subset( { 6 , 1 , 4 } ) );
 assert( ( values_of( *x ) ==
	   std::vector< double >( { 0 , 2 , 3 , 5 , 7 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && rmv->concerns_Block() && ( & rmv->whc() == x ) );
  assert( ( rmv->subset() == Subset( { 1 , 4 , 6 } ) ) );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 1 , 4 , 6 } ) ) );
  }

 // a contiguous Subset: whatever the Modification, the same positions
 mods.clear();
 block->remove_dynamic_variables( *x , Subset( { 2 , 1 } ) );
 assert( ( values_of( *x ) == std::vector< double >( { 0 , 5 , 7 } ) ) );
 assert( mods.size() == 1 );
 if( auto rng = as< BlockModRmvRngd< ColVariable > >( mods.front() ) )
  assert( rng->range() == Range( 1 , 3 ) );
 else {
  auto sbst = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( sbst && ( sbst->subset() == Subset( { 1 , 2 } ) ) );
  }

 // past the end: an error, and the list is left alone
 mods.clear();
 assert( throws( [ & ]() {
	  block->remove_dynamic_variables( *x , Subset( { 0 , 3 } ) ); } ) );
 assert( mods.empty() && ( x->size() == 3 ) );

 // the whole list in disorder: nothing left, empty subset()
 block->remove_dynamic_variables( *x , Subset( { 2 , 0 , 1 } ) );
 assert( x->empty() && ( mods.size() == 1 ) );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && rmv->subset().empty() );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 0 , 5 , 7 } ) ) );
  }

 // the empty Subset: "remove all"
 std::list< ColVariable > more( 4 );
 number( more , 10 );
 block->add_dynamic_variables( *x , more , eNoMod );
 mods.clear();
 block->remove_dynamic_variables( *x , Subset() );
 assert( x->empty() && ( mods.size() == 1 ) );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && rmv->subset().empty() );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 10 , 11 , 12 , 13 } ) ) );
  assert( rmv->get_Block() == block );
  }

 // the empty Subset of an empty list: nothing to remove, nothing issued
 mods.clear();
 block->remove_dynamic_variables( *x , Subset() );
 assert( mods.empty() );

 // a non-empty Subset of an empty list is an error
 assert( throws( [ & ]() {
	  block->remove_dynamic_variables( *x , Subset( { 0 } ) ); } ) );

 // the empty Subset with eNoMod: everything goes, nothing is issued
 std::list< ColVariable > again( 2 );
 block->add_dynamic_variables( *x , again , eNoMod );
 block->remove_dynamic_variables( *x , Subset() , false , eNoMod );
 assert( x->empty() && mods.empty() );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "remove a Subset: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing single elements and vectors of iterators: one element in the
 * middle is the Range [ i , i + 1 ), the last one left is the empty
 * subset(); ordered iterators give the positions they had, a contiguous run
 * of them a Range, and unordered ones are an error when the Modification is
 * issued. */

static void test_remove_iterators( void )
{
 auto block = new AbstractBlock;
 auto x = new std::list< ColVariable >( 6 );
 number( *x , 0 );
 block->add_dynamic_variable( *x , "x" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 block->remove_dynamic_variable( *x , std::next( x->begin() , 2 ) );
 assert( ( values_of( *x ) ==
	   std::vector< double >( { 0 , 1 , 3 , 4 , 5 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvRngd< ColVariable > >( mods.front() );
  assert( rmv && ( rmv->range() == Range( 2 , 3 ) ) );
  assert( rmv->removed().front().get_value() == 2 );
  }

 // ordered, not contiguous: the positions they had
 mods.clear();
 {
  std::vector< std::list< ColVariable >::iterator > its =
   { std::next( x->begin() , 1 ) , std::next( x->begin() , 3 ) };
  block->remove_dynamic_variables( *x , its );
  }
 assert( ( values_of( *x ) == std::vector< double >( { 0 , 3 , 5 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && ( rmv->subset() == Subset( { 1 , 3 } ) ) );
  assert( ( values_of( rmv->removed() ) ==
	    std::vector< double >( { 1 , 4 } ) ) );
  }

 // ordered and contiguous: a Range
 mods.clear();
 {
  std::vector< std::list< ColVariable >::iterator > its =
   { std::next( x->begin() , 1 ) , std::next( x->begin() , 2 ) };
  block->remove_dynamic_variables( *x , its );
  }
 assert( ( values_of( *x ) == std::vector< double >( { 0 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvRngd< ColVariable > >( mods.front() );
  assert( rmv && ( rmv->range() == Range( 1 , 3 ) ) );
  }

 // the last one: the empty subset()
 mods.clear();
 block->remove_dynamic_variable( *x , x->begin() );
 assert( x->empty() && ( mods.size() == 1 ) );
 {
  auto rmv = as< BlockModRmvSbst< ColVariable > >( mods.front() );
  assert( rmv && rmv->subset().empty() && ( rmv->removed().size() == 1 ) );
  }

 // an empty vector of iterators: nothing
 mods.clear();
 {
  std::vector< std::list< ColVariable >::iterator > its;
  block->remove_dynamic_variables( *x , its );
  }
 assert( mods.empty() );

 // unordered iterators are an error when the Modification is issued; the
 // list they are taken from is a throwaway one, since the documentation
 // says nothing about what is left of it
 auto y = new std::list< ColVariable >( 4 );
 block->add_dynamic_variable( *y , "y" );
 {
  std::vector< std::list< ColVariable >::iterator > its =
   { std::next( y->begin() , 3 ) , std::next( y->begin() , 1 ) };
  assert( throws( [ & ]() { block->remove_dynamic_variables( *y , its ); } ) );
  }

 block->unregister_Solvers( true );
 delete block;
 std::cout << "remove by iterators: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing dynamic Constraint: the Variable they were active in no longer
 * list them, whether or not the Modification is issued, and the rows in
 * the Modification are the removed ones in their order; the empty Subset
 * removes all of them. */

static void test_remove_constraints( void )
{
 auto block = new AbstractBlock;
 auto var = new ColVariable;
 block->add_static_variable( *var , "v" );
 auto rows = new std::list< FRowConstraint >( 6 );
 fill( *rows , *var , 0 );
 block->add_dynamic_constraint( *rows , "rows" );
 assert( var->get_num_active() == 6 );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 std::vector< const FRowConstraint * > gone;
 for( auto it = std::next( rows->begin() , 1 ) ;
      it != std::next( rows->begin() , 3 ) ; ++it )
  gone.push_back( & *it );

 block->remove_dynamic_constraints( *rows , Range( 1 , 3 ) );
 assert( ( rhs_of( *rows ) == std::vector< double >( { 0 , 3 , 4 , 5 } ) ) );
 assert( var->get_num_active() == 4 );
 for( auto c : gone ) {
  assert( ! listed( *var , c ) );
  // is_active() of a stuff that is not there is >= get_num_active()
  if( var->is_active( c ) < var->get_num_active() )
   std::cout << "        ColVariable::is_active() of a removed row gives "
	     << var->is_active( c ) << " < get_num_active() = "
	     << var->get_num_active() << std::endl;
  expect( var->is_active( c ) >= var->get_num_active() ,
	  "ColVariable::is_active() of a stuff it is not active in is >= "
	  "get_num_active() (Variable.h)" );
  }
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvRngd< FRowConstraint > >( mods.front() );
  assert( rmv && ( ! rmv->is_variable() ) && ( rmv->range() == Range( 1 , 3 ) ) );
  assert( ( rhs_of( rmv->removed() ) == std::vector< double >( { 1 , 2 } ) ) );
  assert( & rmv->removed().front() == gone[ 0 ] );
  }

 // a Subset
 mods.clear();
 block->remove_dynamic_constraints( *rows , Subset( { 3 , 0 } ) );
 assert( ( rhs_of( *rows ) == std::vector< double >( { 3 , 4 } ) ) );
 assert( var->get_num_active() == 2 );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvSbst< FRowConstraint > >( mods.front() );
  assert( rmv && ( rmv->subset() == Subset( { 0 , 3 } ) ) );
  assert( ( rhs_of( rmv->removed() ) == std::vector< double >( { 0 , 5 } ) ) );
  }

 // eNoMod: the Variable is told all the same
 mods.clear();
 block->remove_dynamic_constraints( *rows , Range( 0 , 1 ) , eNoMod );
 assert( mods.empty() && ( rows->size() == 1 ) );
 assert( var->get_num_active() == 1 );

 // the empty Subset: all of them
 block->remove_dynamic_constraints( *rows , Subset() );
 assert( rows->empty() && ( var->get_num_active() == 0 ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvSbst< FRowConstraint > >( mods.front() );
  assert( rmv && rmv->subset().empty() && ( rmv->removed().size() == 1 ) );
  }

 // the single-element version
 std::list< FRowConstraint > more( 3 );
 fill( more , *var , 10 );
 block->add_dynamic_constraints( *rows , more , eNoMod );
 mods.clear();
 block->remove_dynamic_constraint( *rows , std::next( rows->begin() ) );
 assert( ( rhs_of( *rows ) == std::vector< double >( { 10 , 12 } ) ) );
 assert( mods.size() == 1 );
 {
  auto rmv = as< BlockModRmvRngd< FRowConstraint > >( mods.front() );
  assert( rmv && ( rmv->range() == Range( 1 , 2 ) ) );
  }

 block->unregister_Solvers( true );
 delete block;
 std::cout << "remove dynamic Constraint: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* What the removal of a Constraint relies on in the Variable: is_active()
 * of a stuff the Variable is not active in is >= get_num_active() (also
 * when that stuff would sort before one it is active in), and
 * remove_active() of such a stuff does not remove another one. The two
 * rows are elements of the same vector, hence the first has the smaller
 * address. */

static void test_active_of_absent_stuff( void )
{
 std::vector< FRowConstraint > r( 2 );
 ColVariable v;
 v.add_active( & r[ 1 ] );
 assert( ( v.get_num_active() == 1 ) && listed( v , & r[ 1 ] ) );
 assert( v.is_active( & r[ 1 ] ) == 0 );

 const auto pos = v.is_active( & r[ 0 ] );
 if( pos < v.get_num_active() )
  std::cout << "        ColVariable::is_active() of a stuff it is not "
	    << "active in gives " << pos << std::endl;
 expect( pos >= v.get_num_active() ,
	 "ColVariable::is_active() of a stuff it is not active in, sorting "
	 "before one it is active in, is >= get_num_active() (Variable.h)" );

 try {
  v.remove_active( & r[ 0 ] );
  }
 catch( std::invalid_argument & ) {}
 if( ! listed( v , & r[ 1 ] ) )
  std::cout << "        ColVariable::remove_active( &r[ 0 ] ) removed "
	    << "&r[ 1 ] instead" << std::endl;
 expect( listed( v , & r[ 1 ] ) ,
	 "ColVariable::remove_active() of a stuff it is not active in leaves "
	 "the other active stuff alone" );

 std::cout << "active stuff of a Variable: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing a dynamic Variable active in a Constraint: the Constraint loses
 * it in any case; with issueindMod == eNoMod only the BlockModRmv is issued,
 * otherwise the Modification of the Constraint come first. */

static void test_remove_active_variable( void )
{
 auto block = new AbstractBlock;
 auto x = new std::list< ColVariable >( 2 );
 block->add_dynamic_variable( *x , "x" );
 auto rows = new std::vector< FRowConstraint >( 1 );
 block->add_static_constraint( *rows , "r" );
 auto & row = ( *rows )[ 0 ];
 row.set_function( new LinearFunction( { { & x->front() , 1.0 } ,
					  { & x->back() , 2.0 } } ) , eNoMod );
 assert( row.get_num_active_var() == 2 );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 // the individual Modification switched off
 block->remove_dynamic_variables( *x , Range( 1 , 2 ) , eModBlck , eNoMod );
 assert( row.get_num_active_var() == 1 );
 assert( row.get_active_var( 0 ) == & x->front() );
 assert( mods.size() == 1 );
 assert( as< BlockModRmvRngd< ColVariable > >( mods.front() ) );

 // the individual Modification issued: they precede the BlockModRmv
 mods.clear();
 block->remove_dynamic_variables( *x , Subset() );
 assert( row.get_num_active_var() == 0 );
 assert( mods.size() >= 2 );
 assert( as< BlockModRmvSbst< ColVariable > >( mods.back() ) );
 for( auto it = mods.begin() ; it != std::prev( mods.end() ) ; ++it )
  assert( ! as< BlockModAD >( *it ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "remove an active Variable: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Who sees what according to the mode, with and without a Solver: eModBlck
 * is issued even if nobody listens, and the Block sees it, but it goes no
 * further; eNoBlck is not issued at all when nobody listens; eNoMod is never
 * issued. VariableMod carries the state before and after the change, a
 * change that changes nothing issues nothing, and set_value() is not a
 * Modification at all. */

static void test_modes_and_listening( void )
{
 auto block = new PeepingBlock;
 auto s = new std::vector< ColVariable >( 2 );
 block->add_static_variable( *s , "s" );
 auto & v = ( *s )[ 0 ];

 // nobody listening
 v.is_fixed( true , eNoBlck );
 assert( v.is_fixed() && block->seen.empty() );
 v.is_fixed( false );
 assert( ( ! v.is_fixed() ) && ( block->seen.size() == 1 ) );
 v.is_fixed( true , eNoMod );
 assert( v.is_fixed() && ( block->seen.size() == 1 ) );

 // a Solver listening
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();
 block->seen.clear();

 v.is_fixed( false , eNoBlck );
 assert( ( block->seen.size() == 1 ) && ( mods.size() == 1 ) );
 assert( mods.front() == block->seen.front() );
 {
  auto vm = as< VariableMod >( mods.front() );
  assert( vm && ( vm->variable() == & v ) && ( ! vm->concerns_Block() ) );
  assert( ( vm->old_state() & 1 ) && ! ( vm->new_state() & 1 ) );
  assert( vm->get_Block() == block );
  }

 mods.clear();
 v.is_fixed( true );
 assert( mods.size() == 1 );
 {
  auto vm = as< VariableMod >( mods.front() );
  assert( vm && vm->concerns_Block() && ( vm->new_state() & 1 ) );
  }

 // nothing changes, nothing issued
 mods.clear();
 v.is_fixed( true );
 v.is_integer( false );
 assert( mods.empty() );

 // is_integer() reports a change of the "type" in the same way
 v.is_integer( true );
 assert( mods.size() == 1 );
 {
  auto vm = as< VariableMod >( mods.front() );
  assert( vm && ( vm->old_state() != vm->new_state() ) );
  assert( ( vm->old_state() & 1 ) && ( vm->new_state() & 1 ) );
  }

 // eNoMod: nothing
 mods.clear();
 block->seen.clear();
 v.is_fixed( false , eNoMod );
 assert( mods.empty() && block->seen.empty() );

 // the value is not a Modification
 v.set_value( 42 );
 ( *s )[ 1 ].set_value( -1 );
 assert( mods.empty() && block->seen.empty() );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "modes and anyone_there(): OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A channel: what is sent to it reaches the Solver as exactly one
 * GroupModification when it is closed, in the order it was sent, while the
 * Block itself sees each Modification naked when it is issued; what is sent
 * to channel 0 meanwhile goes through at once; the GroupModification
 * concerns the Block if any of its elements does; a closed channel cannot be
 * closed or sent to again; two open channels have distinct names. */

static void test_channel( void )
{
 auto block = new PeepingBlock;
 auto s = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();

 const auto ch = block->open_channel();
 assert( ( ch != 0 ) && ( ch <= Observer::max_channel_name ) );
 const auto other = block->open_channel();
 assert( ( other != 0 ) && ( other != ch ) );
 block->close_channel( other );
 mods.clear();

 ( *s )[ 0 ].is_fixed( true , Observer::make_par( eNoBlck , ch ) );
 ( *s )[ 1 ].is_fixed( true );    // the default channel: at once
 ( *s )[ 2 ].is_fixed( true , Observer::make_par( eNoBlck , ch ) );
 assert( mods.size() == 1 );
 assert( as< VariableMod >( mods.front() )->variable() == & ( *s )[ 1 ] );
 assert( block->seen.size() == 3 );
 assert( as< VariableMod >( block->seen[ 0 ] ) );

 mods.clear();
 block->seen.clear();
 block->close_channel( ch );
 assert( mods.size() == 1 );
 auto gm = as< GroupModification >( mods.front() );
 assert( gm && ( gm->father() == nullptr ) );
 assert( gm->sub_Modifications().size() == 2 );
 assert( as< VariableMod >( gm->sub_Modifications().front() )->variable() ==
	 & ( *s )[ 0 ] );
 assert( as< VariableMod >( gm->sub_Modifications().back() )->variable() ==
	 & ( *s )[ 2 ] );
 assert( ! gm->concerns_Block() );   // all of them were eNoBlck
 assert( gm->get_Block() == block );

 // the Block where the channel is defined never sees the GroupModification
 assert( block->seen.empty() );

 // closed: neither closing it nor sending to it is possible any longer
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );
 assert( throws( [ & ]() {
	  ( *s )[ 0 ].is_fixed( false , Observer::make_par( eNoBlck , ch ) );
	  } ) );
 assert( throws( [ & ]() { block->close_channel( 0 ); } ) );

 // one element concerning the Block is enough for the whole group
 mods.clear();
 const auto ch2 = block->open_channel();
 ( *s )[ 1 ].is_fixed( false , Observer::make_par( eNoBlck , ch2 ) );
 ( *s )[ 2 ].is_fixed( false , Observer::make_par( eModBlck , ch2 ) );
 block->close_channel( ch2 );
 assert( mods.size() == 1 );
 gm = as< GroupModification >( mods.front() );
 assert( gm && gm->concerns_Block() && ( gm->sub_Modifications().size() == 2 ) );

 // nesting into a channel that is not open is an error
 assert( throws( [ & ]() { block->open_channel( ch2 ); } ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "a channel: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Nested channels: open_channel( ch ) returns ch, what follows goes into
 * the inner GroupModification, close_channel( ch ) goes back to the outer
 * one without shipping anything, and only the last close ships the whole
 * tree; concerns_Block() of an inner group is propagated to the outer one. */

static void test_nested_channels( void )
{
 auto block = new AbstractBlock;
 auto s = new std::vector< ColVariable >( 4 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 const auto ch = block->open_channel();
 const auto par = Observer::make_par( eNoBlck , ch );
 ( *s )[ 0 ].is_fixed( true , par );
 assert( block->open_channel( ch ) == ch );
 ( *s )[ 1 ].is_fixed( true , par );
 ( *s )[ 2 ].is_fixed( true , Observer::make_par( eModBlck , ch ) );
 block->close_channel( ch );
 assert( mods.empty() );
 ( *s )[ 3 ].is_fixed( true , par );
 assert( mods.empty() );
 block->close_channel( ch );

 assert( mods.size() == 1 );
 auto outer = as< GroupModification >( mods.front() );
 assert( outer && ( outer->father() == nullptr ) );
 const auto & subs = outer->sub_Modifications();
 assert( subs.size() == 3 );
 auto it = subs.begin();
 assert( as< VariableMod >( *it )->variable() == & ( *s )[ 0 ] );
 auto inner = as< GroupModification >( *( ++it ) );
 assert( inner && ( inner->father() == outer.get() ) );
 assert( inner->sub_Modifications().size() == 2 );
 assert( as< VariableMod >( inner->sub_Modifications().back() )->variable()
	 == & ( *s )[ 2 ] );
 assert( as< VariableMod >( *( ++it ) )->variable() == & ( *s )[ 3 ] );

 // the eModBlck inside the inner group makes both groups concern the Block
 assert( inner->concerns_Block() && outer->concerns_Block() );

 // the channel is closed after as many close as open
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "nested channels: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* close_channel( ch , true ) at depth 3 ships the outermost
 * GroupModification of the channel, the one with no father that holds the
 * whole tree, and closes the channel. */

static void test_forced_close( void )
{
 auto block = new AbstractBlock;
 auto s = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 const auto ch = block->open_channel();
 const auto par = Observer::make_par( eNoBlck , ch );
 ( *s )[ 0 ].is_fixed( true , par );
 block->open_channel( ch );
 ( *s )[ 1 ].is_fixed( true , par );
 block->open_channel( ch );
 ( *s )[ 2 ].is_fixed( true , Observer::make_par( eModBlck , ch ) );
 block->close_channel( ch , true );

 assert( mods.size() == 1 );
 auto gm = as< GroupModification >( mods.front() );
 assert( gm );
 expect( gm->father() == nullptr ,
	 "close_channel( ch , true ) ships the outermost GroupModification "
	 "of the channel (father() == nullptr)" );
 expect( gm->sub_Modifications().size() == 2 ,
	 "the forcibly shipped GroupModification holds the first "
	 "Modification and the nested group (2 elements)" );
 if( gm->father() != nullptr )
  std::cout << "        got a GroupModification with father() != nullptr "
	    << "and " << gm->sub_Modifications().size() << " element(s): "
	    << "the innermost level, the outer ones are never shipped"
	    << std::endl;
 assert( gm->concerns_Block() );

 // the channel is closed
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );
 assert( throws( [ & ]() { ( *s )[ 0 ].is_fixed( false , par ); } ) );

 // forcing a channel in "root mode" is the same as closing it
 mods.clear();
 const auto ch2 = block->open_channel();
 ( *s )[ 1 ].is_fixed( false , Observer::make_par( eNoBlck , ch2 ) );
 block->close_channel( ch2 , true );
 assert( mods.size() == 1 );
 gm = as< GroupModification >( mods.front() );
 assert( gm && ( gm->father() == nullptr ) &&
	 ( gm->sub_Modifications().size() == 1 ) );

 block->unregister_Solvers( true );
 delete block;
 std::cout << "forced close: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* An empty channel: the documentation says the GroupModification is shipped
 * on closing, and does not say that an empty one is not; if one arrives it
 * has no elements, and its get_Block() is then nullptr. A channel opened
 * with nobody listening holds nothing, since nothing is issued, and closing
 * it delivers nothing to anybody. */

static void test_empty_channel( void )
{
 auto block = new AbstractBlock;
 auto s = new std::vector< ColVariable >( 1 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 block->close_channel( block->open_channel() );
 assert( mods.size() <= 1 );
 if( ! mods.empty() ) {
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && gm->sub_Modifications().empty() );
  assert( gm->get_Block() == nullptr );
  assert( ! gm->concerns_Block() );
  }
 std::cout << "an empty channel delivers " << mods.size()
	   << " Modification" << std::endl;

 // a nested empty level is still an element of the outer one
 mods.clear();
 const auto ch = block->open_channel();
 block->open_channel( ch );
 block->close_channel( ch );
 ( *s )[ 0 ].is_fixed( true , Observer::make_par( eNoBlck , ch ) );
 block->close_channel( ch );
 assert( mods.size() == 1 );
 {
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 2 ) );
  assert( as< GroupModification >( gm->sub_Modifications().front() ) );
  // get_Block() of a group is that of its first element, here the empty
  // group, hence nullptr as documented
  assert( gm->get_Block() == nullptr );
  }

 // nobody listening while the channel is open: nothing gets into it, and
 // a Solver registered before it is closed receives it empty
 block->unregister_Solvers( true );
 const auto ch2 = block->open_channel();
 ( *s )[ 0 ].is_fixed( false , Observer::make_par( eNoBlck , ch2 ) );
 ( *s )[ 0 ].is_fixed( true , Observer::make_par( eModBlck , ch2 ) );
 auto late = new FakeSolver();
 block->register_Solver( late );
 block->close_channel( ch2 );
 {
  auto & lmods = late->get_Modification_list();
  assert( lmods.size() <= 1 );
  if( ! lmods.empty() ) {
   auto gm = as< GroupModification >( lmods.front() );
   assert( gm && gm->sub_Modifications().empty() );
   }
  }
 block->unregister_Solvers( true );

 delete block;
 std::cout << "an empty channel: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* set_default_channel( ch ) sends to ch whatever is sent to channel 0 until
 * it is set back to 0 or ch is closed; setting it to a name that is not an
 * open channel is documented as an error that throws. */

static void test_default_channel( void )
{
 auto block = new AbstractBlock;
 auto s = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();

 const auto ch = block->open_channel();
 block->set_default_channel( ch );
 ( *s )[ 0 ].is_fixed( true );
 ( *s )[ 1 ].is_fixed( true );
 assert( mods.empty() );
 block->set_default_channel( 0 );
 ( *s )[ 2 ].is_fixed( true );
 assert( mods.size() == 1 );
 block->set_default_channel( ch );
 ( *s )[ 2 ].is_fixed( false );
 assert( mods.size() == 1 );

 // closing ch ends the hijacking, and ships the three hijacked ones
 mods.clear();
 block->close_channel( ch );
 assert( mods.size() == 1 );
 auto gm = as< GroupModification >( mods.front() );
 assert( gm && ( gm->sub_Modifications().size() == 3 ) );
 mods.clear();
 ( *s )[ 0 ].is_fixed( false );
 assert( ( mods.size() == 1 ) && as< VariableMod >( mods.front() ) );

 // a name that is not an open channel
 bool threw = false;
 try {
  block->set_default_channel( ch );    // closed by now
  }
 catch( std::exception & ) {
  threw = true;
  }
 expect( threw , "set_default_channel() with the name of a closed channel "
	 "throws (Observer.h: \"is an error and should throw exception\")" );
 if( ! threw ) {
  // the default channel now points nowhere: what is sent to 0 is lost
  bool lost = throws( [ & ]() { ( *s )[ 0 ].is_fixed( true ); } );
  std::cout << "        and then a Modification on channel 0 "
	    << ( lost ? "throws \"wrong channel name\"" : "goes through" )
	    << std::endl;
  block->set_default_channel( 0 );
  }

 // a sub-Block can hijack its channel 0 into a channel of its father, not
 // into one of another Block
 auto sub = new AbstractBlock( block );
 block->add_nested_Block( sub );
 auto other = new AbstractBlock;
 other->register_Solver( new FakeSolver() );
 const auto fch = block->open_channel();
 const auto och = other->open_channel();
 sub->set_default_channel( fch );
 assert( throws( [ & ]() { sub->set_default_channel( och ); } ) );
 assert( throws( [ & ]() { block->set_default_channel( och ); } ) );
 sub->set_default_channel( 0 );
 other->close_channel( och );
 block->close_channel( fch );
 other->unregister_Solvers( true );
 delete other;

 block->unregister_Solvers( true );
 delete block;
 std::cout << "default channel: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Emptying and discarding a channel: clear_channel() deletes what the
 * current level holds and leaves it open, so that what is sent afterwards
 * is all that is shipped, and concerns_Block() is only that of what is left;
 * on a nested channel it empties the inner level and leaves the outer one
 * alone, while on the outer one it also deletes the inner levels closed in
 * it; close_channel( ch , force , true ) delivers nothing, deleting the
 * inner level (which the outer one no longer holds) or, in "root mode" or
 * forced, the whole channel, which is then closed and is no longer the
 * default one; both pass up from a sub-Block to the father that defined the
 * channel, whose Solver receives nothing while that of the sub-Block
 * receives everything naked; 0 and a name that is not open are errors. */

static void test_clear_and_discard( void )
{
 auto block = new AbstractBlock;
 auto s = new std::vector< ColVariable >( 4 );
 block->add_static_variable( *s , "s" );
 auto solver = new FakeSolver();
 block->register_Solver( solver );
 auto & mods = solver->get_Modification_list();
 mods.clear();
 const auto nb = [ & ]( ChnlName c ) { return( Observer::make_par( eNoBlck ,
								c ) ); };

 // an empty channel: clearing it changes nothing, discarding it ships
 // nothing and closes it
 auto ch = block->open_channel();
 block->clear_channel( ch );
 block->close_channel( ch , false , true );
 assert( mods.empty() );
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );
 assert( throws( [ & ]() { block->clear_channel( ch ); } ) );

 // clear, then add: only what comes after the clear is shipped, and an
 // eModBlck cleared away no longer makes the group concern the Block
 ch = block->open_channel();
 ( *s )[ 0 ].is_fixed( true , Observer::make_par( eModBlck , ch ) );
 ( *s )[ 1 ].is_fixed( true , nb( ch ) );
 block->clear_channel( ch );
 assert( mods.empty() );
 ( *s )[ 2 ].is_fixed( true , nb( ch ) );
 block->close_channel( ch );
 assert( mods.size() == 1 );
 {
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 1 ) );
  assert( as< VariableMod >( gm->sub_Modifications().front() )->variable()
	  == & ( *s )[ 2 ] );
  assert( ! gm->concerns_Block() );
  }

 // discarding a channel with something in it, the default one: nothing is
 // shipped, the channel is closed, and channel 0 goes through again
 mods.clear();
 ch = block->open_channel();
 block->set_default_channel( ch );
 assert( block->get_default_channel() == ch );
 ( *s )[ 0 ].is_fixed( false );
 ( *s )[ 1 ].is_fixed( false , nb( ch ) );
 block->close_channel( ch , false , true );
 assert( mods.empty() );
 assert( block->get_default_channel() == 0 );
 ( *s )[ 0 ].is_fixed( true );
 assert( ( mods.size() == 1 ) && as< VariableMod >( mods.front() ) );

 // nested: the clear of the inner level leaves the outer one as it is,
 // the discard of the inner level takes it out of the outer one, and the
 // outer one is shipped with what it had and what came after
 mods.clear();
 ch = block->open_channel();
 ( *s )[ 0 ].is_fixed( false , nb( ch ) );
 block->open_channel( ch );
 ( *s )[ 1 ].is_fixed( true , Observer::make_par( eModBlck , ch ) );
 block->clear_channel( ch );
 ( *s )[ 2 ].is_fixed( false , nb( ch ) );
 block->close_channel( ch , false , true );
 assert( mods.empty() );
 ( *s )[ 3 ].is_fixed( true , nb( ch ) );
 block->close_channel( ch );
 assert( mods.size() == 1 );
 {
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->father() == nullptr ) );
  const auto & subs = gm->sub_Modifications();
  assert( subs.size() == 2 );
  assert( as< VariableMod >( subs.front() )->variable() == & ( *s )[ 0 ] );
  assert( as< VariableMod >( subs.back() )->variable() == & ( *s )[ 3 ] );
  assert( ! gm->concerns_Block() );
  }

 // the clear of the outer level also deletes the inner ones closed in it
 mods.clear();
 ch = block->open_channel();
 block->open_channel( ch );
 ( *s )[ 0 ].is_fixed( true , nb( ch ) );
 block->close_channel( ch );
 block->clear_channel( ch );
 ( *s )[ 1 ].is_fixed( false , nb( ch ) );
 block->close_channel( ch );
 assert( mods.size() == 1 );
 {
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 1 ) );
  assert( as< VariableMod >( gm->sub_Modifications().front() ) );
  }

 // a forced discard at depth 3 deletes the whole channel and closes it
 mods.clear();
 ch = block->open_channel();
 ( *s )[ 0 ].is_fixed( false , nb( ch ) );
 block->open_channel( ch );
 ( *s )[ 1 ].is_fixed( true , nb( ch ) );
 block->open_channel( ch );
 ( *s )[ 2 ].is_fixed( true , nb( ch ) );
 block->close_channel( ch , true , true );
 assert( mods.empty() );
 assert( throws( [ & ]() { block->close_channel( ch ); } ) );

 // errors: channel 0 and a name that is not open
 assert( throws( [ & ]() { block->clear_channel( 0 ); } ) );
 assert( throws( [ & ]() { block->close_channel( 0 , false , true ); } ) );
 assert( throws( [ & ]() { block->close_channel( ch , true , true ); } ) );

 // from a sub-Block, on the default channel of the father: the Solver of
 // the sub-Block receives everything naked, the one of the father nothing
 auto sub = new AbstractBlock( block );
 block->add_nested_Block( sub );
 auto t = new std::vector< ColVariable >( 1 );
 sub->add_static_variable( *t , "t" );
 auto ssolver = new FakeSolver();
 sub->register_Solver( ssolver );
 auto & smods = ssolver->get_Modification_list();
 smods.clear();
 mods.clear();
 ch = block->open_channel();
 block->set_default_channel( ch );
 for( int i = 0 ; i < 10 ; ++i ) {
  sub->clear_channel( ch );
  ( *t )[ 0 ].is_fixed( i % 2 == 0 );
  }
 assert( smods.size() == 10 );
 assert( mods.empty() );
 sub->close_channel( ch , false , true );
 assert( mods.empty() && ( smods.size() == 10 ) );
 assert( block->get_default_channel() == 0 );
 assert( throws( [ & ]() { sub->clear_channel( ch ); } ) );

 sub->unregister_Solvers( true );
 block->unregister_Solvers( true );
 delete block;
 std::cout << "clear and discard: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A sub-Block: a sub-Block with no Solver of its own knows someone listens
 * to its father, also when it is nested after the Solver was registered,
 * and forgets it when the Solver goes; its Modification reach the Solver of
 * both, the very same object; on a channel defined in the father, the
 * Solver of the sub-Block receives them naked and that of the father only
 * inside the GroupModification; on a channel defined in the sub-Block, the
 * father receives the GroupModification when it is closed; sending from the
 * father to a channel of the sub-Block is an error. */

static void test_sub_Block( void )
{
 auto father = new PeepingBlock;
 auto early = new PeepingBlock( father );
 father->add_nested_Block( early );
 auto s = new std::vector< ColVariable >( 2 );
 early->add_static_variable( *s , "s" );
 auto & v = ( *s )[ 0 ];
 assert( ! early->anyone_there() );

 auto fslv = new FakeSolver();
 father->register_Solver( fslv );
 auto & fmods = fslv->get_Modification_list();
 assert( early->anyone_there() );

 // nested after the Solver of the father was registered
 auto late = new AbstractBlock( father );
 father->add_nested_Block( late );
 assert( late->anyone_there() );

 fmods.clear();
 v.is_fixed( true , eNoBlck );
 assert( ( fmods.size() == 1 ) && ( early->seen.size() == 1 ) &&
	 ( father->seen.size() == 1 ) );
 assert( as< VariableMod >( fmods.front() )->get_Block() == early );

 // a Solver of its own: the same object reaches both
 auto sslv = new FakeSolver();
 early->register_Solver( sslv );
 auto & smods = sslv->get_Modification_list();
 fmods.clear();
 v.is_fixed( false , eNoBlck );
 assert( ( fmods.size() == 1 ) && ( smods.size() == 1 ) );
 assert( fmods.front() == smods.front() );

 // a channel of the father: naked below, packed above
 fmods.clear();
 smods.clear();
 early->seen.clear();
 father->seen.clear();
 const auto fch = father->open_channel();
 v.is_fixed( true , Observer::make_par( eNoBlck , fch ) );
 ( *s )[ 1 ].is_fixed( true , Observer::make_par( eNoBlck , fch ) );
 assert( ( smods.size() == 2 ) && fmods.empty() );
 assert( ( early->seen.size() == 2 ) && ( father->seen.size() == 2 ) );
 father->close_channel( fch );
 assert( fmods.size() == 1 );
 {
  auto gm = as< GroupModification >( fmods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 2 ) );
  assert( gm->sub_Modifications().front() == smods.front() );
  assert( gm->get_Block() == early );
  }
 assert( smods.size() == 2 );

 // the sub-Block can nest into the channel of its father, and close it
 fmods.clear();
 smods.clear();
 const auto fch2 = father->open_channel();
 assert( early->open_channel( fch2 ) == fch2 );
 v.is_fixed( false , Observer::make_par( eNoBlck , fch2 ) );
 early->close_channel( fch2 );
 early->close_channel( fch2 );
 assert( ( smods.size() == 1 ) && ( fmods.size() == 1 ) );
 {
  auto gm = as< GroupModification >( fmods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 1 ) );
  auto in = as< GroupModification >( gm->sub_Modifications().front() );
  assert( in && ( in->sub_Modifications().size() == 1 ) );
  }

 // a channel of the sub-Block: the father sees the GroupModification
 fmods.clear();
 smods.clear();
 father->seen.clear();
 const auto sch = early->open_channel();
 v.is_fixed( true , Observer::make_par( eNoBlck , sch ) );
 assert( fmods.empty() && smods.empty() && father->seen.empty() );
 assert( throws( [ & ]() { father->close_channel( sch ); } ) );
 {
  auto fv = new ColVariable;
  father->add_static_variable( *fv , "f" );
  assert( throws( [ & ]() {
	   fv->is_fixed( true , Observer::make_par( eNoBlck , sch ) ); } ) );
  }
 father->seen.clear();
 early->close_channel( sch );
 assert( ( fmods.size() == 1 ) && ( smods.size() == 1 ) );
 assert( fmods.front() == smods.front() );
 assert( ( father->seen.size() == 1 ) &&
	 as< GroupModification >( father->seen.front() ) );

 // the Solver of the father goes: the sub-Block with none of its own
 // no longer has anyone listening, the other still has its own
 father->unregister_Solvers( true );
 assert( ! late->anyone_there() );
 assert( early->anyone_there() );
 early->unregister_Solvers( true );
 assert( ! early->anyone_there() );

 delete father;
 std::cout << "sub-Block: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_ModParam_arithmetic();
 test_issue_mod();
 test_open_if_needed();
 test_add_dynamic_variables();
 test_add_dynamic_constraints();
 test_remove_range();
 test_remove_subset();
 test_remove_iterators();
 test_remove_constraints();
 test_active_of_absent_stuff();
 test_remove_active_variable();
 test_modes_and_listening();
 test_channel();
 test_nested_channels();
 test_forced_close();
 test_empty_channel();
 test_default_channel();
 test_clear_and_discard();
 test_sub_Block();

 if( n_failed ) {
  std::cout << n_failed << " check(s) of the documented contract FAILED"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File tests_BlockModification.cpp ------------------*/
/*--------------------------------------------------------------------------*/

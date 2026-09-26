/*--------------------------------------------------------------------------*/
/*-------------------------- File tests_Objective.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for Objective, RealObjective and FRealObjective, and for the
 * Modification they issue.
 *
 * The FRealObjective is the Observer of its Function and is registered as
 * active in the Variable of the Function: the tests go over the change of
 * sense (and the changes that change nothing), over an FRealObjective with
 * no Function, over the replacement of the Function (who observes what, who
 * is active where, and who deletes what), over the Modification of the
 * Function reaching the Solver of the Block, and over the removal of the
 * Variable of the Function through the FRealObjective, which has to keep
 * the Variable in step whether or not a Modification is issued.
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
#include "C05Function.h"
#include "ColVariable.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "LinearFunction.h"

#include <iostream>
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
/// the Modification downcast to M, nullptr if it is not one

template< class M >
static std::shared_ptr< M > as( const sp_Mod & mod )
{
 return( std::dynamic_pointer_cast< M >( mod ) );
 }

/*--------------------------------------------------------------------------*/
/// a LinearFunction that says when it is deleted

class TracedFunction : public LinearFunction {
 public:

 TracedFunction( v_coeff_pair && vars , bool & dead , FunctionValue ct = 0 )
  : LinearFunction( std::move( vars ) , ct ) , f_dead( dead ) {
  f_dead = false;
  }

 ~TracedFunction() override { f_dead = true; }

 bool & f_dead;
 };

/*--------------------------------------------------------------------------*/
/// an AbstractBlock that records every Modification it is given

class PeepingBlock : public AbstractBlock {
 public:

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  seen.push_back( mod );
  AbstractBlock::add_Modification( mod , chnl );
  }

 std::vector< sp_Mod > seen;  ///< what the Block has been given
 };

/*--------------------------------------------------------------------------*/
/// true if the Objective is among the active stuff of the Variable
/** The list is scanned element by element rather than asked with
 * is_active(), so that the check does not depend on the latter. */

static bool listed( const ColVariable & v , const Objective * obj )
{
 for( Index i = 0 ; i < v.get_num_active() ; ++i )
  if( v.get_active( i ) == obj )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// a Block with n static ColVariable and a FakeSolver
/** The FRealObjective is set as the Objective of the Block with eNoMod, so
 * that the list of the FakeSolver starts empty. */

struct Fixture {
 Fixture( Index n , bool with_solver ) {
  block = new PeepingBlock;
  x = new std::vector< ColVariable >( n );
  block->add_static_variable( *x , "x" );
  obj = new FRealObjective( block );
  block->set_objective( obj , eNoMod );
  if( with_solver ) {
   solver = new FakeSolver();
   block->register_Solver( solver );
   }
  }

 ~Fixture() {
  block->unregister_Solvers( true );
  block->reset_objective();
  delete obj;
  delete block;
  }

 Lst_sp_Mod & mods( void ) { return( solver->get_Modification_list() ); }

 PeepingBlock * block = nullptr;
 std::vector< ColVariable > * x = nullptr;
 FRealObjective * obj = nullptr;
 FakeSolver * solver = nullptr;
 };

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* set_sense(): an Objective starts to be minimized; min -> max issues one
 * ObjectiveMod of type eSetMax, the Objective and its Block in it; setting
 * the same sense issues nothing; eNoMod changes the sense and issues
 * nothing; eNoBlck gives concerns_Block() == false and is not issued when
 * nobody listens, while eModBlck is still given to the Block; on a channel
 * it arrives packed; without a Block the sense just changes. */

static void test_sense( void )
{
 {
  Fixture f( 1 , true );
  auto & mods = f.mods();
  assert( f.obj->get_sense() == Objective::eMin );

  f.obj->set_sense( Objective::eMax );
  assert( f.obj->get_sense() == Objective::eMax );
  assert( mods.size() == 1 );
  auto om = as< ObjectiveMod >( mods.front() );
  assert( om && ( om->type() == ObjectiveMod::eSetMax ) );
  assert( ( om->of() == f.obj ) && ( om->get_Block() == f.block ) );
  assert( om->concerns_Block() );
  assert( ! as< FRealObjectiveMod >( mods.front() ) );

  // the same sense: nothing
  mods.clear();
  f.obj->set_sense( Objective::eMax );
  assert( mods.empty() );

  // eNoMod: changed, not issued
  f.obj->set_sense( Objective::eMin , eNoMod );
  assert( ( f.obj->get_sense() == Objective::eMin ) && mods.empty() );

  // eNoBlck
  f.obj->set_sense( Objective::eMax , eNoBlck );
  assert( mods.size() == 1 );
  om = as< ObjectiveMod >( mods.front() );
  assert( om && ( om->type() == ObjectiveMod::eSetMax ) &&
	  ( ! om->concerns_Block() ) );

  // max -> min
  mods.clear();
  f.obj->set_sense( Objective::eMin );
  assert( mods.size() == 1 );
  assert( as< ObjectiveMod >( mods.front() )->type() ==
	  ObjectiveMod::eSetMin );

  // on a channel: packed until closed
  mods.clear();
  const auto ch = f.block->open_channel();
  f.obj->set_sense( Objective::eMax , Observer::make_par( eModBlck , ch ) );
  f.obj->set_sense( Objective::eMin , Observer::make_par( eModBlck , ch ) );
  assert( mods.empty() );
  f.block->close_channel( ch );
  assert( mods.size() == 1 );
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 2 ) );
  }

 {
  // nobody listening: eNoBlck is not even given to the Block, eModBlck is
  Fixture f( 1 , false );
  f.obj->set_sense( Objective::eMax , eNoBlck );
  assert( ( f.obj->get_sense() == Objective::eMax ) && f.block->seen.empty() );
  f.obj->set_sense( Objective::eMin );
  assert( f.block->seen.size() == 1 );
  assert( as< ObjectiveMod >( f.block->seen.front() ) );
  }

 {
  // no Block at all
  FRealObjective loose;
  assert( loose.get_Block() == nullptr );
  loose.set_sense( Objective::eMax );
  assert( loose.get_sense() == Objective::eMax );
  }

 std::cout << "sense: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* An FRealObjective with no Function: nothing to compute, no active
 * Variable, a constant term of 0 (the default of RealObjective), and it is
 * "always listening" as documented. */

static void test_no_function( void )
{
 Fixture f( 2 , true );
 auto obj = f.obj;
 assert( obj->get_function() == nullptr );
 assert( obj->compute() == FRealObjective::kUnEval );
 assert( obj->value() == Inf< RealObjective::OFValue >() );
 assert( obj->get_constant_term() == 0 );
 assert( obj->get_num_active_var() == 0 );
 assert( obj->is_active( & ( *f.x )[ 0 ] ) >= obj->get_num_active_var() );
 assert( obj->get_active_var( 0 ) == nullptr );
 assert( obj->v_begin() == nullptr );
 assert( obj->anyone_there() );
 assert( obj->get_Block() == f.block );

 // removing Variable of no Function does nothing
 obj->remove_variable( 0 );
 obj->remove_variables( Range( 0 , 2 ) );
 obj->remove_variables( Subset() );
 assert( f.mods().empty() );

 // setting no Function over no Function changes nothing, hence says nothing
 obj->set_function( nullptr );
 assert( f.mods().empty() );

 std::cout << "no Function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* compute() and value() with a Function: the value is that of the Function
 * at the values of the Variable, constant term included, and it changes
 * only at the next compute(); get_constant_term() is that of the Function,
 * and a change of it is a Modification of the Function reaching the Solver. */

static void test_value_and_constant( void )
{
 Fixture f( 2 , true );
 auto & x = *f.x;
 auto lf = new LinearFunction( { { & x[ 0 ] , 2.0 } , { & x[ 1 ] , -1.0 } } ,
			       3.0 );
 f.obj->set_function( lf , eNoMod );
 assert( f.mods().empty() );
 assert( f.obj->get_constant_term() == 3.0 );
 x[ 0 ].set_value( 5 );
 x[ 1 ].set_value( 4 );
 assert( f.obj->compute() == FRealObjective::kOK );
 assert( f.obj->value() == 2 * 5 - 4 + 3 );

 // the value is the one of the last compute()
 x[ 0 ].set_value( 0 );
 assert( f.obj->value() == 9 );
 f.obj->compute();
 assert( f.obj->value() == -1 );

 lf->set_constant_term( 10 );
 assert( f.obj->get_constant_term() == 10 );
 assert( f.mods().size() == 1 );
 {
  auto fm = as< FunctionMod >( f.mods().front() );
  auto om = as< FRealObjectiveMod >( f.mods().front() );
  assert( ( fm && ( fm->function() == lf ) ) || ( om && ( om->of() == f.obj ) ) );
  if( fm )
   assert( fm->shift() == 7 );
  }

 std::cout << "value and constant term: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* set_function(): the new Function is observed by the FRealObjective and the
 * FRealObjective is active exactly in its Variable, while the old one is no
 * longer observed and the FRealObjective leaves its Variable; the old one is
 * deleted if deleteold and left alone otherwise; one FRealObjectiveMod of
 * type eFunctionChanged is issued according to issueMod, and none when the
 * Function is the same; set_function( nullptr ) empties it. */

static void test_set_function( void )
{
 Fixture f( 3 , true );
 auto & x = *f.x;
 auto & mods = f.mods();

 bool dead1 , dead2 , dead3;
 auto f1 = new TracedFunction( { { & x[ 0 ] , 1.0 } , { & x[ 1 ] , 1.0 } } ,
			       dead1 );
 f.obj->set_function( f1 );
 assert( f1->get_Observer() == f.obj );
 assert( listed( x[ 0 ] , f.obj ) && listed( x[ 1 ] , f.obj ) &&
	 ( ! listed( x[ 2 ] , f.obj ) ) );
 assert( mods.size() == 1 );
 {
  auto om = as< FRealObjectiveMod >( mods.front() );
  assert( om && ( om->type() == FRealObjectiveMod::eFunctionChanged ) );
  assert( ( om->of() == f.obj ) && om->concerns_Block() );
  assert( om->get_Block() == f.block );
  }

 // the same Function: nothing
 mods.clear();
 f.obj->set_function( f1 );
 assert( mods.empty() && ( ! dead1 ) );

 // replaced, the old one kept
 auto f2 = new TracedFunction( { { & x[ 1 ] , 1.0 } , { & x[ 2 ] , 1.0 } } ,
			       dead2 );
 f.obj->set_function( f2 , eNoBlck , false );
 assert( ! dead1 );
 assert( f1->get_Observer() == nullptr );
 assert( f2->get_Observer() == f.obj );
 assert( ( ! listed( x[ 0 ] , f.obj ) ) && listed( x[ 1 ] , f.obj ) &&
	 listed( x[ 2 ] , f.obj ) );
 // x[ 1 ] is in both: it lists the FRealObjective once
 {
  Index n = 0;
  for( Index i = 0 ; i < x[ 1 ].get_num_active() ; ++i )
   if( x[ 1 ].get_active( i ) == f.obj )
    ++n;
  assert( n == 1 );
  }
 assert( f.obj->get_num_active_var() == 2 );
 assert( mods.size() == 1 );
 assert( ! as< FRealObjectiveMod >( mods.front() )->concerns_Block() );

 // a Modification of the Function no longer observed goes nowhere
 mods.clear();
 f1->modify_coefficient( 0 , 5.0 );
 assert( mods.empty() );
 delete f1;
 assert( dead1 );

 // replaced with eNoMod, the old one deleted
 auto f3 = new TracedFunction( { { & x[ 0 ] , 1.0 } } , dead3 );
 f.obj->set_function( f3 , eNoMod );
 assert( dead2 && ( ! dead3 ) );
 assert( mods.empty() );
 assert( listed( x[ 0 ] , f.obj ) && ( ! listed( x[ 1 ] , f.obj ) ) &&
	 ( ! listed( x[ 2 ] , f.obj ) ) );

 // emptied: no Function, no active Variable
 f.obj->set_function( nullptr );
 assert( dead3 && ( f.obj->get_function() == nullptr ) );
 assert( mods.size() == 1 );
 for( auto & v : x )
  assert( ! listed( v , f.obj ) );

 // the destructor deletes the Function it has
 bool dead4;
 auto f4 = new TracedFunction( { { & x[ 2 ] , 1.0 } } , dead4 );
 {
  FRealObjective tmp( nullptr , f4 );
  assert( listed( x[ 2 ] , & tmp ) );
  }
 assert( dead4 && ( ! listed( x[ 2 ] , f.obj ) ) );
 assert( x[ 2 ].get_num_active() == 0 );

 std::cout << "set_function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* A Modification of the Function: it reaches the Solver of the Block of the
 * FRealObjective exactly once, either as it is or repackaged by the
 * FRealObjective (the documentation allows both); eNoMod gives nothing; with
 * nobody listening to the Block it does not even reach the Block; a
 * Variable added to the Function makes the FRealObjective active in it also
 * when nobody listens, since the FRealObjective "is always listening". */

static void test_function_Modification( void )
{
 {
  Fixture f( 3 , true );
  auto & x = *f.x;
  auto & mods = f.mods();
  auto lf = new LinearFunction( { { & x[ 0 ] , 1.0 } , { & x[ 1 ] , 2.0 } } );
  f.obj->set_function( lf , eNoMod );

  lf->modify_coefficient( 1 , 4.0 );
  assert( mods.size() == 1 );
  {
   auto fm = as< FunctionMod >( mods.front() );
   auto om = as< FRealObjectiveMod >( mods.front() );
   assert( ( fm && ( fm->function() == lf ) ) ||
	   ( om && ( om->of() == f.obj ) ) );
   assert( mods.front()->get_Block() == f.block );
   if( auto lm = as< C05FunctionModLinRngd >( mods.front() ) ) {
    assert( lm->range() == Range( 1 , 2 ) );
    assert( ( lm->delta().size() == 1 ) && ( lm->delta()[ 0 ] == 2.0 ) );
    }
   }

  // eNoMod
  mods.clear();
  lf->modify_coefficient( 0 , 3.0 , eNoMod );
  assert( mods.empty() );

  // a Variable added: the FunctionModVars arrives, the FRealObjective is
  // active in it
  lf->add_variable( & x[ 2 ] , 1.0 );
  assert( listed( x[ 2 ] , f.obj ) );
  assert( mods.size() == 1 );
  {
   auto vm = as< FunctionModVars >( mods.front() );
   assert( vm && vm->added() && ( vm->vars().size() == 1 ) &&
	   ( vm->vars()[ 0 ] == & x[ 2 ] ) );
   }

  // on a channel of the Block, through the Function
  mods.clear();
  const auto ch = f.obj->open_channel();
  lf->modify_coefficient( 0 , 1.0 , Observer::make_par( eModBlck , ch ) );
  lf->modify_coefficient( 1 , 1.0 , Observer::make_par( eModBlck , ch ) );
  assert( mods.empty() );
  f.obj->close_channel( ch );
  assert( mods.size() == 1 );
  auto gm = as< GroupModification >( mods.front() );
  assert( gm && ( gm->sub_Modifications().size() == 2 ) );
  }

 {
  // nobody listening: a Modification that concerns the Block still reaches
  // it, so that its physical representation follows the Objective, while
  // one that does not concern it is not issued at all
  Fixture f( 3 , false );
  auto & x = *f.x;
  auto lf = new LinearFunction( { { & x[ 0 ] , 1.0 } } );
  f.obj->set_function( lf , eNoMod );
  f.block->seen.clear();

  lf->modify_coefficient( 0 , 4.0 );
  assert( f.block->seen.size() == 1 );
  f.block->seen.clear();

  lf->add_variable( & x[ 1 ] , 1.0 , eNoBlck );
  assert( listed( x[ 1 ] , f.obj ) );
  assert( f.block->seen.empty() );

  // a Solver registered later hears the next ones
  f.solver = new FakeSolver();
  f.block->register_Solver( f.solver );
  lf->modify_coefficient( 0 , 5.0 );
  assert( f.mods().size() == 1 );
  }

 std::cout << "Modification of the Function: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

/* Removing Variable through the FRealObjective: the Variable removed no
 * longer list the FRealObjective, both when the Modification is issued and
 * when it is not (eNoMod, or nobody listening), for one Variable, a Range
 * and a Subset, including the empty Subset, which removes all of them. */

static void test_remove_variables( void )
{
 for( int there = 0 ; there < 2 ; ++there )
  for( ModParam mode : { ModParam( eModBlck ) , ModParam( eNoMod ) } ) {
   Fixture f( 6 , there );
   auto & x = *f.x;
   LinearFunction::v_coeff_pair p;
   for( auto & v : x )
    p.push_back( { & v , 1.0 } );
   f.obj->set_function( new LinearFunction( std::move( p ) ) , eNoMod );
   for( auto & v : x )
    assert( listed( v , f.obj ) );

   // one: x[ 0 ]
   f.obj->remove_variable( 0 , mode );
   assert( f.obj->get_num_active_var() == 5 );
   assert( ! listed( x[ 0 ] , f.obj ) );

   // a Range: x[ 1 ], x[ 2 ]
   f.obj->remove_variables( Range( 0 , 2 ) , mode );
   assert( f.obj->get_num_active_var() == 3 );
   assert( ( ! listed( x[ 1 ] , f.obj ) ) && ( ! listed( x[ 2 ] , f.obj ) ) );
   assert( f.obj->get_active_var( 0 ) == & x[ 3 ] );

   // a Subset: x[ 5 ]
   f.obj->remove_variables( Subset( { 2 } ) , false , mode );
   assert( f.obj->get_num_active_var() == 2 );
   assert( ! listed( x[ 5 ] , f.obj ) );
   assert( listed( x[ 3 ] , f.obj ) && listed( x[ 4 ] , f.obj ) );

   // the empty Subset: all the rest
   f.obj->remove_variables( Subset() , false , mode );
   assert( f.obj->get_num_active_var() == 0 );
   const bool left = listed( x[ 3 ] , f.obj ) || listed( x[ 4 ] , f.obj );
   if( left )
    std::cout << "        with " << ( there ? "a" : "no" ) << " Solver and "
	      << ( mode == eNoMod ? "eNoMod" : "eModBlck" ) << ", x[ 3 ] and "
	      << "x[ 4 ] still list the FRealObjective" << std::endl;
   expect( ! left , "FRealObjective::remove_variables( Subset() ) leaves "
	   "no removed Variable with the FRealObjective among its active "
	   "stuff" );

   if( there && ( mode == eModBlck ) )
    assert( f.mods().size() == 4 );
   if( there && ( mode == eNoMod ) )
    assert( f.mods().empty() );

   // cleanup the stale registrations, if any, before the Variable go
   for( auto & v : x )
    if( listed( v , f.obj ) )
     v.remove_active( f.obj );
   }

 // an FRealObjective with no Block: the Variable are removed and left
 {
  ColVariable a , b , c;
  FRealObjective loose( nullptr ,
			new LinearFunction( { { & a , 1.0 } , { & b , 1.0 } ,
					      { & c , 1.0 } } ) );
  loose.remove_variable( 0 );
  assert( ( loose.get_num_active_var() == 2 ) && ( ! listed( a , & loose ) ) );
  loose.remove_variables( Range( 0 , 1 ) );
  assert( ( loose.get_num_active_var() == 1 ) && ( ! listed( b , & loose ) ) );
  loose.remove_variables( Subset() );
  assert( ( loose.get_num_active_var() == 0 ) && ( ! listed( c , & loose ) ) );
  }

 std::cout << "remove Variable: done" << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 test_sense();
 test_no_function();
 test_value_and_constant();
 test_set_function();
 test_function_Modification();
 test_remove_variables();

 if( n_failed ) {
  std::cout << n_failed << " check(s) of the documented contract FAILED"
	    << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File tests_Objective.cpp ----------------------*/
/*--------------------------------------------------------------------------*/

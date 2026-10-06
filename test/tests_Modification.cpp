/*--------------------------------------------------------------------------*/
/*----------------------- File tests_Modification.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the Modification issued by the modifying methods of the
 * core components: ColVariable, FRowConstraint, the OneVarConstraint family,
 * FRealObjective, LinearFunction within an FRowConstraint and within an
 * FRealObjective, and the dynamic Variable and Constraint of a Block; and
 * that under eDryRun those of DQuadFunction, QuadFunction,
 * PolyhedralFunction, LagBFunction, BendersBFunction and C05SumFunction
 * change and issue nothing, those that can be exercised without a Solver
 * being also checked to do the change under eNoMod.
 *
 * For each method the test checks what Observer::make_par() says of the
 * ModParam: under eDryRun the change is not done and nothing is issued;
 * under eNoMod the change is done and nothing is issued; under
 * eNoBlck the Modification is issued, with concerns_Block() == false, only
 * if a Solver is listening; under eModBlck it is issued, with
 * concerns_Block() == true, whether or not a Solver is listening; on a
 * channel of the Block it arrives packed in the GroupModification that
 * closing the channel dispatches. It then checks the type of the
 * Modification and what it says (the object it refers to, the type code,
 * the range or subset, the elements added or removed), and the edge cases:
 * a call that changes nothing, empty and full Range, Range at the
 * boundaries, empty Subset (which for the removing methods means "all"),
 * unordered Subset, adding nothing and removing everything. Finally, it
 * checks what changes() says of the Modification of the core and what the
 * static methods of Modification read from it, and that a Block passes a
 * Solver only the kinds of Modification it reads [see
 * Solver::concerned_by()], and what Solution::adapt() answers to them.
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

#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "AbstractBlock.h"
#include "BendersBFunction.h"
#include "C05SumFunction.h"
#include "ColVariable.h"
#include "ColVariableSolution.h"
#include "DQuadFunction.h"
#include "FakeSolver.h"
#include "FRealObjective.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "LinearFunction.h"
#include "OneVarConstraint.h"
#include "PolyhedralFunction.h"
#include "QuadFunction.h"

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;
using Range = Block::Range;
using Subset = Block::Subset;
using var_type = Variable::var_type;
using RHSValue = RowConstraint::RHSValue;
using v_coeff_pair = LinearFunction::v_coeff_pair;
using Vec_FV = Function::Vec_FunctionValue;
using Addrs = std::vector< const void * >;

static constexpr RHSValue INF = RowConstraint::RHSINF;

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASSES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an AbstractBlock that records every Modification it is sent
/** The Block is sent a Modification before looking at whether anyone is
 * listening, which is what tells eModBlck from eNoBlck when no Solver is
 * registered. */

class RecBlock : public AbstractBlock
{
 public:

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  v_seen.push_back( mod );
  AbstractBlock::add_Modification( mod , chnl );
  }

 Lst_sp_Mod v_seen;  ///< what the Block has been sent, in order
 };

/*--------------------------------------------------------------------------*/
/// the Block under test, with the FakeSolver that records what it receives

struct Rig
{
 Rig( void ) : block( new RecBlock ) , solver( new FakeSolver ) {
  block->register_Solver( solver );
  }

 ~Rig() {
  clear();
  block->unregister_Solvers( true );
  delete block;       // it clear()-s its Objective, hence before this
  delete objective;
  }

 Lst_sp_Mod & got( void ) { return( solver->get_Modification_list() ); }

 Lst_sp_Mod & seen( void ) { return( block->v_seen ); }

 void clear( void ) {
  got().clear();
  seen().clear();
  }

 void listen( bool yn ) {
  if( yn )
   block->register_Solver( solver );   // does nothing if it is there
  else
   block->unregister_Solver( solver );
  }

 RecBlock * block;
 FakeSolver * solver;
 Objective * objective = nullptr;  ///< owned here, the Block does not
 };

/*--------------------------------------------------------------------------*/
/// one modifying call and what it has to do
/** reset() puts the object in the state before the call without issuing
 * anything, call() does the call under the given ModParam, before() and
 * after() tell the two states apart, and check() looks at the type and the
 * content of the one Modification the call issues. */

struct Case
{
 std::function< void( void ) > reset;
 std::function< void( ModParam ) > call;
 std::function< bool( void ) > before;
 std::function< bool( void ) > after;
 std::function< void( const sp_Mod & ) > check;
 };

/*--------------------------------------------------------------------------*/
/// a FakeSolver that reads only some kinds of Modification

class ReadsSolver : public FakeSolver
{
 public:

 explicit ReadsSolver( Modification::ModConcern reads ) : f_reads( reads ) {}

 [[nodiscard]] Modification::ModConcern concerned_by( void ) const override {
  return( f_reads );
  }

 Modification::ModConcern f_reads;
 };

/*--------------------------------------------------------------------------*/
/// a :Solution that says nothing of what it holds

class GenericSolution : public Solution
{
 public:

 GenericSolution( void ) : Solution() {}
 };

/*--------------------------------------------------------------------------*/
/// a physical Modification that says nothing more of itself

class PhysMod : public Modification
{
 public:

 explicit PhysMod( Block * b ) : f_block( b ) {}

 [[nodiscard]] Block * get_Block( void ) const override { return( f_block ); }

 protected:

 void print( std::ostream & output ) const override {
  output << "PhysMod" << std::endl;
  }

 Block * f_block;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
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
/// true if s is among the stuff in which v is active
/** The list is scanned, so that the checks do not rely on
 * ColVariable::is_active(), which test_active_list() checks apart. */

static bool active_in( const Variable & v , const ThinVarDepInterface * s )
{
 for( Index i = 0 ; i < v.get_num_active() ; ++i )
  if( v.get_active( i ) == s )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// true if exactly the Variable of p among those of x see s as active

static bool registered( const std::vector< ColVariable > & x ,
			const ThinVarDepInterface * s ,
			const v_coeff_pair & p )
{
 for( const auto & v : x ) {
  bool in = false;
  for( const auto & pr : p )
   if( pr.first == & v )
    in = true;
  if( active_in( v , s ) != in )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/// true if the LinearFunction has exactly the pairs in p, in that order

static bool is_lf( const Function * f , const v_coeff_pair & p )
{
 auto lf = dynamic_cast< const LinearFunction * >( f );
 if( ( ! lf ) || ( lf->get_num_active_var() != p.size() ) )
  return( false );
 for( Index i = 0 ; i < p.size() ; ++i )
  if( ( lf->get_active_var( i ) != p[ i ].first ) ||
      ( lf->get_coefficient( i ) != p[ i ].second ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/// gives s (an FRowConstraint or FRealObjective) a new LinearFunction on p
/** A LinearFunction changed under eNoMod leaves s out of step with its
 * Variable, since s learns of added and removed Variable from the
 * FunctionModVars: the old Function is emptied first, so that nothing is
 * unregistered through it, and the Variable are then told by hand. */

template< class S >
static void rebuild( S & s , std::vector< ColVariable > & x , v_coeff_pair p )
{
 s.clear();
 for( auto & v : x )
  if( active_in( v , & s ) )
   v.remove_active( & s );
 s.set_function( new LinearFunction( std::move( p ) ) , eNoMod );
 }

/*--------------------------------------------------------------------------*/
/// the addresses of the elements of a container, in order

template< class L >
static Addrs addresses( const L & l )
{
 Addrs rv;
 for( const auto & el : l )
  rv.push_back( & el );
 return( rv );
 }

/*--------------------------------------------------------------------------*/
/// the elements of v in the given positions

static Addrs pick( const Addrs & v , std::initializer_list< Index > pos )
{
 Addrs rv;
 for( auto i : pos )
  rv.push_back( v[ i ] );
 return( rv );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ CHECKING ONE Case -------------------------------*/
/*--------------------------------------------------------------------------*/

/// resets the Case with or without a Solver listening, nothing recorded

static void prepare( Rig & r , const Case & c , bool listening )
{
 r.clear();
 r.listen( listening );
 c.reset();
 r.clear();
 assert( c.before() );
 }

/*--------------------------------------------------------------------------*/
/// eNoMod: the change is done, nothing is issued

static void check_nomod( Rig & r , const Case & c )
{
 prepare( r , c , true );
 c.call( eNoMod );
 assert( c.after() );
 assert( r.got().empty() && r.seen().empty() );
 }

/*--------------------------------------------------------------------------*/
/// with a Solver listening, one Modification with the given concerns_Block

static void check_issued( Rig & r , const Case & c , ModParam iM , bool cB )
{
 prepare( r , c , true );
 c.call( iM );
 assert( c.after() );
 assert( ( r.got().size() == 1 ) && ( r.seen().size() == 1 ) );
 const auto & mod = r.got().front();
 assert( mod == r.seen().front() );
 assert( mod->concerns_Block() == cB );
 assert( mod->get_Block() == r.block );
 c.check( mod );
 }

/*--------------------------------------------------------------------------*/
/// eNoBlck and eModBlck with a Solver listening

static void check_listened( Rig & r , const Case & c )
{
 check_nomod( r , c );
 check_issued( r , c , eNoBlck , false );
 check_issued( r , c , eModBlck , true );
 }

/*--------------------------------------------------------------------------*/
/// eNoBlck with no Solver listening: the change is done, nothing is issued

static void check_unheard_noblck( Rig & r , const Case & c )
{
 prepare( r , c , false );
 c.call( eNoBlck );
 assert( c.after() );
 assert( r.seen().empty() );
 r.listen( true );
 }

/*--------------------------------------------------------------------------*/
/// eModBlck with no Solver listening: the Block is sent the Modification

static void check_unheard_modblck( Rig & r , const Case & c )
{
 prepare( r , c , false );
 c.call( eModBlck );
 assert( c.after() );
 assert( r.seen().size() == 1 );
 const auto & mod = r.seen().front();
 assert( mod->concerns_Block() );
 assert( mod->get_Block() == r.block );
 c.check( mod );
 r.clear();
 r.listen( true );
 }

/*--------------------------------------------------------------------------*/
/// on a channel: nothing until it is closed, then one GroupModification

static void check_channel( Rig & r , const Case & c )
{
 prepare( r , c , true );
 auto chnl = r.block->open_channel();
 c.call( Observer::make_par( eModBlck , chnl ) );
 assert( c.after() );
 assert( r.got().empty() );
 assert( r.seen().size() == 1 );
 r.block->close_channel( chnl );
 assert( r.got().size() == 1 );
 auto gmod = std::dynamic_pointer_cast< GroupModification >(
						      r.got().front() );
 assert( gmod && ( gmod->sub_Modifications().size() == 1 ) );
 const auto & mod = gmod->sub_Modifications().front();
 assert( mod == r.seen().front() );
 assert( mod->concerns_Block() );
 c.check( mod );
 }

/*--------------------------------------------------------------------------*/
/// eDryRun: the change is not done, nothing is issued

static void check_dry_run( Rig & r , const Case & c )
{
 prepare( r , c , true );
 c.call( eDryRun );
 assert( c.before() );
 assert( r.got().empty() && r.seen().empty() );
 }

/*--------------------------------------------------------------------------*/
/// all the above but eDryRun

static void check_all( Rig & r , const Case & c )
{
 check_listened( r , c );
 check_unheard_noblck( r , c );
 check_unheard_modblck( r , c );
 check_channel( r , c );
 }

/*--------------------------------------------------------------------------*/
/// a call that changes nothing issues nothing, whatever the ModParam

static void check_nothing( Rig & r , const Case & c )
{
 for( bool listening : { true , false } )
  for( ModParam iM : { eDryRun , eNoMod , eNoBlck , eModBlck } ) {
   prepare( r , c , listening );
   c.call( iM );
   assert( c.before() );
   assert( r.got().empty() && r.seen().empty() );
   }
 r.listen( true );
 }

/*--------------------------------------------------------------------------*/
/// a call that throws, changes nothing and issues nothing

static void check_throws( Rig & r , const Case & c )
{
 for( ModParam iM : { eDryRun , eNoMod , eNoBlck , eModBlck } ) {
  prepare( r , c , true );
  assert( throws( [ & ]() { c.call( iM ); } ) );
  assert( c.before() );
  assert( r.got().empty() && r.seen().empty() );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/* is_fixed(), set_type(), is_integer(), is_positive(), is_negative() and
 * is_unitary() of a ColVariable: a VariableMod with the old and the new
 * state, the fixed bit being kept by the type setters and the type by
 * is_fixed(). */

static void test_ColVariable( void )
{
 Rig r;
 auto x = new ColVariable;
 r.block->add_static_variable( *x , "x" );

 // the state as ColVariable encodes it: the type above the fixed bit
 auto st = []( bool fixed , int type ) {
  return( var_type( type * 2 + ( fixed ? 1 : 0 ) ) );
  };

 auto make = [ & ]( bool f0 , int t0 , bool f1 , int t1 ,
		    std::function< void( ModParam ) > call ) {
  return( Case{ [ = ]() {
                 x->is_fixed( f0 , eNoMod );
                 x->set_type( t0 , eNoMod );
                 } ,
		call ,
		[ = ]() { return( x->get_state() == st( f0 , t0 ) ); } ,
		[ = ]() { return( x->get_state() == st( f1 , t1 ) ); } ,
		[ = ]( const sp_Mod & mod ) {
		 auto vmod = std::dynamic_pointer_cast< VariableMod >( mod );
		 assert( vmod && ( vmod->variable() == x ) );
		 assert( vmod->old_state() == st( f0 , t0 ) );
		 assert( vmod->new_state() == st( f1 , t1 ) );
		 } } );
  };

 const int C = ColVariable::kContinuous;
 const int I = ColVariable::kInteger;
 const int B = ColVariable::kBinary;

 std::vector< Case > changes = {
  make( false , C , true , C , [ = ]( ModParam iM ) {
                                x->is_fixed( true , iM ); } ) ,
  make( true , B , false , B , [ = ]( ModParam iM ) {
                                x->is_fixed( false , iM ); } ) ,
  // the type changes, the fixed bit stays
  make( true , C , true , B , [ = ]( ModParam iM ) {
                               x->set_type( B , iM ); } ) ,
  make( false , I , false , C , [ = ]( ModParam iM ) {
                                 x->set_type( C , iM ); } ) ,
  make( true , ColVariable::kNonNegative , true , ColVariable::kNatural ,
	[ = ]( ModParam iM ) { x->is_integer( true , iM ); } ) ,
  make( false , B , false , B & ~I ,
	[ = ]( ModParam iM ) { x->is_integer( false , iM ); } ) ,
  make( false , C , false , ColVariable::kNonNegative ,
	[ = ]( ModParam iM ) { x->is_positive( true , iM ); } ) ,
  make( false , B , false , B & ~ColVariable::kNonNegative ,
	[ = ]( ModParam iM ) { x->is_positive( false , iM ); } ) ,
  make( false , I , false , ColVariable::kNegative ,
	[ = ]( ModParam iM ) { x->is_negative( true , iM ); } ) ,
  make( true , C , true , ColVariable::kUnitary ,
	[ = ]( ModParam iM ) { x->is_unitary( true , iM ); } ) ,
  make( false , B , false , B & ~ColVariable::kUnitary ,
	[ = ]( ModParam iM ) { x->is_unitary( false , iM ); } ) };

 // a Case that changes nothing: before() == after()
 auto same = [ & ]( bool f0 , int t0 ,
		    std::function< void( ModParam ) > call ) {
  return( make( f0 , t0 , f0 , t0 , call ) );
  };

 std::vector< Case > noops = {
  same( true , C , [ = ]( ModParam iM ) { x->is_fixed( true , iM ); } ) ,
  same( false , C , [ = ]( ModParam iM ) { x->is_fixed( false , iM ); } ) ,
  same( false , B , [ = ]( ModParam iM ) { x->set_type( B , iM ); } ) ,
  same( false , B , [ = ]( ModParam iM ) { x->is_integer( true , iM ); } ) ,
  same( false , C , [ = ]( ModParam iM ) { x->is_positive( false , iM ); } ) ,
  same( false , C , [ = ]( ModParam iM ) { x->is_negative( false , iM ); } ) ,
  same( false , B , [ = ]( ModParam iM ) { x->is_unitary( true , iM ); } ) };

 for( const auto & c : changes )
  check_all( r , c );
 for( const auto & c : noops )
  check_nothing( r , c );

 for( const auto & c : changes )
  check_dry_run( r , c );

 // a ColVariable of no Block changes all the same, and has none to tell
 ColVariable y;
 for( ModParam iM : { eNoMod , eNoBlck , eModBlck } ) {
  y.is_fixed( false , eNoMod );
  y.set_type( C , eNoMod );
  y.is_fixed( true , iM );
  y.set_type( B , iM );
  assert( y.is_fixed() && ( y.get_type() == B ) );
  }
 }

/*--------------------------------------------------------------------------*/
/* set_lhs(), set_rhs(), set_both() and relax() of an FRowConstraint, and
 * set_function(): a RowConstraintMod with the type of the change, a
 * ConstraintMod telling relaxing from enforcing, an FRowConstraintMod. */

static void test_RowConstraint( void )
{
 Rig r;
 auto x = new std::vector< ColVariable >( 2 );
 r.block->add_static_variable( *x , "x" );
 auto X = [ x ]( Index i ) { return( & ( *x )[ i ] ); };
 auto c = new FRowConstraint( nullptr , 0 , 0 ,
			      new LinearFunction( { { X( 0 ) , 1 } ,
						    { X( 1 ) , 2 } } ) );
 r.block->add_static_constraint( *c , "c" );

 auto bounds = [ c ]( RHSValue l , RHSValue u ) {
  return( ( c->get_lhs() == l ) && ( c->get_rhs() == u ) );
  };

 auto make = [ & ]( RHSValue l0 , RHSValue u0 , RHSValue l1 , RHSValue u1 ,
		    int type , std::function< void( ModParam ) > call ) {
  return( Case{ [ = ]() {
                 c->set_lhs( l0 , eNoMod );
                 c->set_rhs( u0 , eNoMod );
                 } ,
		call ,
		[ = ]() { return( bounds( l0 , u0 ) ); } ,
		[ = ]() { return( bounds( l1 , u1 ) ); } ,
		[ = ]( const sp_Mod & mod ) {
		 auto cmod = std::dynamic_pointer_cast< RowConstraintMod >(
									mod );
		 assert( cmod && ( cmod->constraint() == c ) );
		 assert( cmod->type() == type );
		 assert( ! std::dynamic_pointer_cast< OneVarConstraintMod >(
									mod ) );
		 assert( ! std::dynamic_pointer_cast< FRowConstraintMod >(
									mod ) );
		 } } );
  };

 const auto LHS = RowConstraintMod::eChgLHS;
 const auto RHS = RowConstraintMod::eChgRHS;
 const auto BTS = RowConstraintMod::eChgBTS;

 std::vector< Case > changes = {
  make( 0 , 5 , -1 , 5 , LHS , [ = ]( ModParam iM ) {
                                c->set_lhs( -1 , iM ); } ) ,
  make( 0 , 5 , 0 , 7 , RHS , [ = ]( ModParam iM ) {
                               c->set_rhs( 7 , iM ); } ) ,
  make( 0 , 5 , 0 , INF , RHS , [ = ]( ModParam iM ) {
                                 c->set_rhs( INF , iM ); } ) ,
  make( 0 , 5 , -INF , 5 , LHS , [ = ]( ModParam iM ) {
                                  c->set_lhs( -INF , iM ); } ) ,
  make( -1 , 5 , 3 , 3 , BTS , [ = ]( ModParam iM ) {
                                c->set_both( 3 , iM ); } ) ,
  // one of the two is already there, both are said to change
  make( 0 , 3 , 3 , 3 , BTS , [ = ]( ModParam iM ) {
                               c->set_both( 3 , iM ); } ) };

 std::vector< Case > noops = {
  make( 0 , 5 , 0 , 5 , LHS , [ = ]( ModParam iM ) {
                               c->set_lhs( 0 , iM ); } ) ,
  make( 0 , 5 , 0 , 5 , RHS , [ = ]( ModParam iM ) {
                               c->set_rhs( 5 , iM ); } ) ,
  make( 3 , 3 , 3 , 3 , BTS , [ = ]( ModParam iM ) {
                               c->set_both( 3 , iM ); } ) };

 auto relax = [ & ]( bool from , bool to , int type ) {
  return( Case{ [ = ]() { c->relax( from , eNoMod ); } ,
		[ = ]( ModParam iM ) { c->relax( to , iM ); } ,
		[ = ]() { return( c->is_relaxed() == from ); } ,
		[ = ]() { return( c->is_relaxed() == to ); } ,
		[ = ]( const sp_Mod & mod ) {
		 auto cmod = std::dynamic_pointer_cast< ConstraintMod >( mod );
		 assert( cmod && ( cmod->constraint() == c ) );
		 assert( cmod->type() == type );
		 assert( ! std::dynamic_pointer_cast< RowConstraintMod >(
									mod ) );
		 } } );
  };

 changes.push_back( relax( false , true , ConstraintMod::eRelaxConst ) );
 changes.push_back( relax( true , false , ConstraintMod::eEnforceConst ) );
 noops.push_back( relax( true , true , ConstraintMod::eRelaxConst ) );
 noops.push_back( relax( false , false , ConstraintMod::eEnforceConst ) );

 // the whole Function changes, and with it the Variable c is active in
 changes.push_back( Case{
  [ = ]() {
   rebuild( *c , *x , { { X( 0 ) , 1 } } );
   } ,
  [ = ]( ModParam iM ) {
   auto f = new LinearFunction( { { X( 1 ) , 3 } } );
   c->set_function( f , iM );
   if( c->get_function() != f )  // not taken, as under eDryRun
    delete f;
   } ,
  [ = ]() {
   return( is_lf( c->get_function() , { { X( 0 ) , 1 } } ) &&
	   registered( *x , c , { { X( 0 ) , 1 } } ) );
   } ,
  [ = ]() {
   return( is_lf( c->get_function() , { { X( 1 ) , 3 } } ) &&
	   registered( *x , c , { { X( 1 ) , 3 } } ) );
   } ,
  [ = ]( const sp_Mod & mod ) {
   auto cmod = std::dynamic_pointer_cast< FRowConstraintMod >( mod );
   assert( cmod && ( cmod->constraint() == c ) );
   assert( cmod->type() == FRowConstraintMod::eFunctionChanged );
   } } );

 for( const auto & cs : changes )
  check_all( r , cs );
 for( const auto & cs : noops )
  check_nothing( r , cs );

 for( const auto & cs : changes )
  check_dry_run( r , cs );
 }

/*--------------------------------------------------------------------------*/
/* set_lhs(), set_rhs(), set_both() of BoxConstraint, LBConstraint,
 * UBConstraint and LB0Constraint, and set_variable() and remove_variable*()
 * of a OneVarConstraint: a OneVarConstraintMod with the RowConstraintMod
 * type of the change, or eVariableChanged; a bound that the class does not
 * have throws, changing and issuing nothing. */

static void test_OneVarConstraint( void )
{
 Rig r;
 auto x = new std::vector< ColVariable >( 2 );
 r.block->add_static_variable( *x , "x" );
 auto X = [ x ]( Index i ) { return( & ( *x )[ i ] ); };

 auto box = new BoxConstraint( nullptr , X( 0 ) , 0 , 10 );
 r.block->add_static_constraint( *box , "box" );
 auto lb = new LBConstraint( nullptr , X( 0 ) , 0 );
 r.block->add_static_constraint( *lb , "lb" );
 auto ub = new UBConstraint( nullptr , X( 0 ) , 10 );
 r.block->add_static_constraint( *ub , "ub" );
 auto lb0 = new LB0Constraint( nullptr , X( 1 ) );
 r.block->add_static_constraint( *lb0 , "lb0" );

 // the OneVarConstraintMod of cnst of the given type
 auto ovc_mod = []( OneVarConstraint * cnst , int type ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto cmod = std::dynamic_pointer_cast< OneVarConstraintMod >( mod );
   assert( cmod && ( cmod->constraint() == cnst ) );
   assert( cmod->type() == type );
   } );
  };

 auto make = [ & ]( OneVarConstraint * cnst ,
		    RHSValue l0 , RHSValue u0 , RHSValue l1 , RHSValue u1 ,
		    int type , std::function< void( ModParam ) > call ) {
  return( Case{ [ = ]() {
                 // the bound a class does not have is left alone
                 if( cnst->get_lhs() != l0 )
                  cnst->set_lhs( l0 , eNoMod );
                 if( cnst->get_rhs() != u0 )
                  cnst->set_rhs( u0 , eNoMod );
                 } ,
		call ,
		[ = ]() {
		 return( ( cnst->get_lhs() == l0 ) &&
			 ( cnst->get_rhs() == u0 ) ); } ,
		[ = ]() {
		 return( ( cnst->get_lhs() == l1 ) &&
			 ( cnst->get_rhs() == u1 ) ); } ,
		ovc_mod( cnst , type ) } );
  };

 const auto LHS = RowConstraintMod::eChgLHS;
 const auto RHS = RowConstraintMod::eChgRHS;
 const auto BTS = RowConstraintMod::eChgBTS;

 std::vector< Case > changes = {
  make( box , 0 , 10 , -2 , 10 , LHS , [ = ]( ModParam iM ) {
                                        box->set_lhs( -2 , iM ); } ) ,
  make( box , 0 , 10 , 0 , 12 , RHS , [ = ]( ModParam iM ) {
                                       box->set_rhs( 12 , iM ); } ) ,
  make( box , 0 , 10 , 0 , INF , RHS , [ = ]( ModParam iM ) {
                                        box->set_rhs( INF , iM ); } ) ,
  make( box , 0 , 10 , 4 , 4 , BTS , [ = ]( ModParam iM ) {
                                      box->set_both( 4 , iM ); } ) ,
  make( box , 0 , 4 , 4 , 4 , BTS , [ = ]( ModParam iM ) {
                                     box->set_both( 4 , iM ); } ) ,
  make( lb , 0 , INF , 2 , INF , LHS , [ = ]( ModParam iM ) {
                                        lb->set_lhs( 2 , iM ); } ) ,
  make( lb , 0 , INF , -INF , INF , LHS , [ = ]( ModParam iM ) {
                                           lb->set_lhs( -INF , iM ); } ) ,
  make( ub , -INF , 10 , -INF , 8 , RHS , [ = ]( ModParam iM ) {
                                           ub->set_rhs( 8 , iM ); } ) ,
  make( lb0 , 0 , INF , 0 , 4 , RHS , [ = ]( ModParam iM ) {
                                       lb0->set_rhs( 4 , iM ); } ) };

 std::vector< Case > noops = {
  make( box , 0 , 10 , 0 , 10 , LHS , [ = ]( ModParam iM ) {
                                       box->set_lhs( 0 , iM ); } ) ,
  make( box , 4 , 4 , 4 , 4 , BTS , [ = ]( ModParam iM ) {
                                     box->set_both( 4 , iM ); } ) ,
  // the bound a class does not have can be "set" to what it is
  make( lb , 0 , INF , 0 , INF , RHS , [ = ]( ModParam iM ) {
                                        lb->set_rhs( INF , iM ); } ) ,
  make( ub , -INF , 10 , -INF , 10 , LHS , [ = ]( ModParam iM ) {
                                            ub->set_lhs( -INF , iM ); } ) ,
  make( lb0 , 0 , 4 , 0 , 4 , LHS , [ = ]( ModParam iM ) {
                                     lb0->set_lhs( 0 , iM ); } ) };

 // and not to anything else
 std::vector< Case > throwing = {
  make( lb , 0 , INF , 0 , INF , RHS , [ = ]( ModParam iM ) {
                                        lb->set_rhs( 5 , iM ); } ) ,
  make( lb , 0 , INF , 0 , INF , BTS , [ = ]( ModParam iM ) {
                                        lb->set_both( 5 , iM ); } ) ,
  make( ub , -INF , 10 , -INF , 10 , LHS , [ = ]( ModParam iM ) {
                                            ub->set_lhs( 1 , iM ); } ) ,
  make( ub , -INF , 10 , -INF , 10 , BTS , [ = ]( ModParam iM ) {
                                            ub->set_both( 1 , iM ); } ) ,
  make( lb0 , 0 , 4 , 0 , 4 , LHS , [ = ]( ModParam iM ) {
                                     lb0->set_lhs( 1 , iM ); } ) };

 // the ColVariable of box: from X( 0 ) to what the call leaves
 auto var = [ & ]( ColVariable * to ,
		   std::function< void( ModParam ) > call ) {
  return( Case{ [ = ]() { box->set_variable( X( 0 ) , eNoMod ); } ,
		call ,
		[ = ]() {
		 return( ( box->get_active_var( 0 ) == X( 0 ) ) &&
			 active_in( *X( 0 ) , box ) &&
			 ( ! active_in( *X( 1 ) , box ) ) ); } ,
		[ = ]() {
		 return( ( box->get_active_var( 0 ) == to ) &&
			 ( box->get_num_active_var() == ( to ? 1 : 0 ) ) &&
			 ( active_in( *X( 0 ) , box ) == ( to == X( 0 ) ) ) &&
			 ( active_in( *X( 1 ) , box ) == ( to == X( 1 ) ) ) ); } ,
		ovc_mod( box , OneVarConstraintMod::eVariableChanged ) } );
  };

 changes.push_back( var( X( 1 ) , [ = ]( ModParam iM ) {
                                   box->set_variable( X( 1 ) , iM ); } ) );
 changes.push_back( var( nullptr , [ = ]( ModParam iM ) {
                                    box->set_variable( nullptr , iM ); } ) );
 changes.push_back( var( nullptr , [ = ]( ModParam iM ) {
                                    box->remove_variable( 0 , iM ); } ) );
 changes.push_back( var( nullptr , [ = ]( ModParam iM ) {
			  box->remove_variables( Range( 0 , 1 ) , iM ); } ) );
 // the empty Subset means all of them
 changes.push_back( var( nullptr , [ = ]( ModParam iM ) {
			  box->remove_variables( Subset() , false , iM ); } ) );
 changes.push_back( var( nullptr , [ = ]( ModParam iM ) {
			  box->remove_variables( Subset( { 0 } ) , true ,
						 iM ); } ) );
 noops.push_back( var( X( 0 ) , [ = ]( ModParam iM ) {
                                 box->set_variable( X( 0 ) , iM ); } ) );

 for( const auto & c : changes )
  check_all( r , c );
 for( const auto & c : noops )
  check_nothing( r , c );
 for( const auto & c : throwing )
  check_throws( r , c );

 for( const auto & c : changes )
  check_dry_run( r , c );
 }

/*--------------------------------------------------------------------------*/
/* set_sense() of an FRealObjective and set_function(), and set_objective()
 * of the Block: an ObjectiveMod eSetMin / eSetMax, an FRealObjectiveMod, a
 * BlockMod. */

static void test_Objective( void )
{
 Rig r;
 auto x = new std::vector< ColVariable >( 2 );
 r.block->add_static_variable( *x , "x" );
 auto X = [ x ]( Index i ) { return( & ( *x )[ i ] ); };
 auto obj = new FRealObjective( nullptr ,
				new LinearFunction( { { X( 0 ) , 1 } } ) );
 r.objective = obj;
 r.block->set_objective( obj , eNoMod );

 auto sense = [ & ]( int from , int to , int type ) {
  return( Case{ [ = ]() { obj->set_sense( from , eNoMod ); } ,
		[ = ]( ModParam iM ) { obj->set_sense( to , iM ); } ,
		[ = ]() { return( obj->get_sense() == from ); } ,
		[ = ]() { return( obj->get_sense() == to ); } ,
		[ = ]( const sp_Mod & mod ) {
		 auto omod = std::dynamic_pointer_cast< ObjectiveMod >( mod );
		 assert( omod && ( omod->of() == obj ) );
		 assert( omod->type() == type );
		 assert( ! std::dynamic_pointer_cast< FRealObjectiveMod >(
									mod ) );
		 } } );
  };

 std::vector< Case > changes = {
  sense( Objective::eMin , Objective::eMax , ObjectiveMod::eSetMax ) ,
  sense( Objective::eMax , Objective::eMin , ObjectiveMod::eSetMin ) ,
  Case{ [ = ]() { rebuild( *obj , *x , { { X( 0 ) , 1 } } ); } ,
	[ = ]( ModParam iM ) {
	 auto f = new LinearFunction( { { X( 1 ) , 2 } } );
	 obj->set_function( f , iM );
	 if( obj->get_function() != f )  // not taken, as under eDryRun
	  delete f;
	 } ,
	[ = ]() {
	 return( is_lf( obj->get_function() , { { X( 0 ) , 1 } } ) &&
		 registered( *x , obj , { { X( 0 ) , 1 } } ) ); } ,
	[ = ]() {
	 return( is_lf( obj->get_function() , { { X( 1 ) , 2 } } ) &&
		 registered( *x , obj , { { X( 1 ) , 2 } } ) ); } ,
	[ = ]( const sp_Mod & mod ) {
	 auto omod = std::dynamic_pointer_cast< FRealObjectiveMod >( mod );
	 assert( omod && ( omod->of() == obj ) );
	 assert( omod->type() == FRealObjectiveMod::eFunctionChanged );
	 } } };

 std::vector< Case > noops = {
  sense( Objective::eMin , Objective::eMin , ObjectiveMod::eSetMin ) ,
  sense( Objective::eMax , Objective::eMax , ObjectiveMod::eSetMax ) };

 for( const auto & c : changes )
  check_all( r , c );
 for( const auto & c : noops )
  check_nothing( r , c );

 for( const auto & c : changes )
  check_dry_run( r , c );

 // the Objective of the Block changes whole
 FRealObjective other;
 Case swap{ [ & ]() { r.block->set_objective( obj , eNoMod ); } ,
	    [ & ]( ModParam iM ) { r.block->set_objective( & other , iM ); } ,
	    [ & ]() { return( r.block->get_objective() == obj ); } ,
	    [ & ]() {
	     return( ( r.block->get_objective() == & other ) &&
		     ( other.get_Block() == r.block ) ); } ,
	    [ & ]( const sp_Mod & mod ) {
	     auto bmod = std::dynamic_pointer_cast< BlockMod >( mod );
	     assert( bmod && ( bmod->get_Block() == r.block ) );
	     } };

 check_listened( r , swap );
 check_unheard_noblck( r , swap );
 check_unheard_modblck( r , swap );

 check_channel( r , swap );
 check_dry_run( r , swap );

 r.block->set_objective( obj , eNoMod );
 r.clear();
 }

/*--------------------------------------------------------------------------*/
/* add_variable(), add_variables(), modify_coefficient(),
 * modify_coefficients() by Range and by Subset, set_constant_term() of a
 * LinearFunction within an FRowConstraint of the Block, and the
 * remove_variable() and remove_variables() by Range and by Subset of the
 * FRowConstraint, which are how the Variable of its Function are removed:
 * the LinearFunctionModVarsAddd, C05FunctionModVarsRngd / Sbst,
 * C05FunctionModLinRngd / Sbst and C05FunctionMod they issue, which reach
 * the Block through the FRowConstraint. The same for a few of them within
 * an FRealObjective.
 *
 * Adding to the LinearFunction under eNoMod leaves the FRowConstraint
 * unaware of the new Variable, which is what eNoMod means (neither Block nor
 * Solver will need it); the registration is thus checked only when the
 * Modification is issued. Removing through the FRowConstraint keeps it in
 * step under every ModParam. */

static void test_LinearFunction( void )
{
 Rig r;
 auto x = new std::vector< ColVariable >( 4 );
 r.block->add_static_variable( *x , "x" );
 auto X = [ x ]( Index i ) { return( & ( *x )[ i ] ); };
 auto c = new FRowConstraint;
 r.block->add_static_constraint( *c , "c" );
 auto lf = [ c ]() {
  return( static_cast< LinearFunction * >( c->get_function() ) );
  };

 const v_coeff_pair P1 = { { X( 0 ) , 1 } };
 const v_coeff_pair P2 = { { X( 0 ) , 1 } , { X( 1 ) , 2 } };
 const v_coeff_pair P3 = { { X( 0 ) , 1 } , { X( 1 ) , 2 } , { X( 2 ) , 3 } };
 const v_coeff_pair P4 = { { X( 0 ) , 1 } , { X( 1 ) , 2 } , { X( 2 ) , 3 } ,
			   { X( 3 ) , 4 } };

 // from the pairs p0 to the pairs p1; the registration of c with its
 // Variable is part of the state after if the call is by c itself
 auto make = [ & ]( const v_coeff_pair & p0 , const v_coeff_pair & p1 ,
		    bool by_c , std::function< void( ModParam ) > call ,
		    std::function< void( const sp_Mod & ) > check ) {
  return( Case{ [ = ]() { rebuild( *c , *x , p0 ); } ,
		call ,
		[ = ]() { return( is_lf( c->get_function() , p0 ) &&
				  registered( *x , c , p0 ) ); } ,
		[ = ]() {
		 return( is_lf( c->get_function() , p1 ) &&
			 ( ( ! by_c ) || registered( *x , c , p1 ) ) ); } ,
		[ = ]( const sp_Mod & mod ) {
		 // the Modification went through c, which knows by now
		 assert( registered( *x , c , p1 ) );
		 check( mod );
		 } } );
  };

 // the LinearFunctionModVarsAddd of the given Variable and coefficients
 auto added = [ = ]( Vec_p_Var vars , LinearFunction::v_coeff coeff ,
		     Index first ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< LinearFunctionModVarsAddd >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( fmod->added() );
   assert( ( fmod->vars() == vars ) && ( fmod->coeff() == coeff ) );
   assert( fmod->first() == first );
   assert( fmod->shift() == 0 );
   } );
  };

 // the C05FunctionModVarsRngd of the given Variable and Range
 auto rmvd_rngd = [ = ]( Vec_p_Var vars , Range range ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< C05FunctionModVarsRngd >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( ! fmod->added() );
   assert( ( fmod->vars() == vars ) && ( fmod->range() == range ) );
   assert( fmod->shift() == 0 );
   } );
  };

 // the C05FunctionModVarsSbst of the given Variable and Subset
 auto rmvd_sbst = [ = ]( Vec_p_Var vars , Subset subset ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< C05FunctionModVarsSbst >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( ! fmod->added() );
   assert( ( fmod->vars() == vars ) && ( fmod->subset() == subset ) );
   assert( fmod->shift() == 0 );
   } );
  };

 // the C05FunctionModLinRngd of the given Variable, deltas and Range
 auto lin_rngd = [ = ]( Vec_p_Var vars , Vec_FV delta , Range range ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< C05FunctionModLinRngd >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( ( fmod->vars() == vars ) && ( fmod->delta() == delta ) );
   assert( fmod->range() == range );
   assert( std::isnan( fmod->shift() ) );
   } );
  };

 // the C05FunctionModLinSbst of the given Variable, deltas and Subset,
 // matched as triples whatever the order the Modification keeps them in
 auto lin_sbst = [ = ]( Vec_p_Var vars , Vec_FV delta , Subset subset ) {
  return( [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< C05FunctionModLinSbst >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( std::isnan( fmod->shift() ) );
   assert( fmod->subset().size() == subset.size() );
   assert( ( fmod->vars().size() == vars.size() ) &&
	   ( fmod->delta().size() == delta.size() ) );
   for( Index k = 0 ; k < subset.size() ; ++k ) {
    Index h = 0;
    while( ( h < subset.size() ) && ( fmod->subset()[ h ] != subset[ k ] ) )
     ++h;
    assert( h < subset.size() );
    assert( ( fmod->vars()[ h ] == vars[ k ] ) &&
	    ( fmod->delta()[ h ] == delta[ k ] ) );
    }
   } );
  };

 std::vector< Case > changes = {
  // adding
  make( P2 , P3 , false , [ = ]( ModParam iM ) {
                           lf()->add_variable( X( 2 ) , 3 , iM ); } ,
	added( { X( 2 ) } , { 3 } , 2 ) ) ,
  make( P2 , P4 , false , [ = ]( ModParam iM ) {
                           lf()->add_variables( { { X( 2 ) , 3 } ,
                                                  { X( 3 ) , 4 } } , iM ); } ,
	added( { X( 2 ) , X( 3 ) } , { 3 , 4 } , 2 ) ) ,
  // to an empty LinearFunction
  make( {} , P2 , false , [ = ]( ModParam iM ) {
                           lf()->add_variables( v_coeff_pair( P2 ) , iM ); } ,
	added( { X( 0 ) , X( 1 ) } , { 1 , 2 } , 0 ) ) ,
  // changing coefficients
  make( P3 , { { X( 0 ) , 1 } , { X( 1 ) , 5 } , { X( 2 ) , 3 } } , false ,
	[ = ]( ModParam iM ) { lf()->modify_coefficient( 1 , 5 , iM ); } ,
	lin_rngd( { X( 1 ) } , { 3 } , Range( 1 , 2 ) ) ) ,
  // the full Range
  make( P3 , { { X( 0 ) , 7 } , { X( 1 ) , 8 } , { X( 2 ) , 9 } } , false ,
	[ = ]( ModParam iM ) {
	 lf()->modify_coefficients( { 7 , 8 , 9 } , Range( 0 , 3 ) , iM ); } ,
	lin_rngd( { X( 0 ) , X( 1 ) , X( 2 ) } , { 6 , 6 , 6 } ,
		  Range( 0 , 3 ) ) ) ,
  // the lower boundary
  make( P3 , { { X( 0 ) , -1 } , { X( 1 ) , 2 } , { X( 2 ) , 3 } } , false ,
	[ = ]( ModParam iM ) {
	 lf()->modify_coefficients( { -1 } , Range( 0 , 1 ) , iM ); } ,
	lin_rngd( { X( 0 ) } , { -2 } , Range( 0 , 1 ) ) ) ,
  // a Range past the end is cut to it, and the Modification says so
  make( P3 , { { X( 0 ) , 1 } , { X( 1 ) , 8 } , { X( 2 ) , 9 } } , false ,
	[ = ]( ModParam iM ) {
	 lf()->modify_coefficients( { 8 , 9 } , Range( 1 , Inf< Index >() ) ,
				    iM ); } ,
	lin_rngd( { X( 1 ) , X( 2 ) } , { 6 , 6 } , Range( 1 , 3 ) ) ) ,
  // an unordered Subset
  make( P3 , { { X( 0 ) , 7 } , { X( 1 ) , 2 } , { X( 2 ) , 9 } } , false ,
	[ = ]( ModParam iM ) {
	 lf()->modify_coefficients( { 9 , 7 } , Subset( { 2 , 0 } ) , false ,
				    iM ); } ,
	lin_sbst( { X( 2 ) , X( 0 ) } , { 6 , 6 } , Subset( { 2 , 0 } ) ) ) ,
  make( P3 , { { X( 0 ) , 1 } , { X( 1 ) , 4 } , { X( 2 ) , 3 } } , false ,
	[ = ]( ModParam iM ) {
	 lf()->modify_coefficients( { 4 } , Subset( { 1 } ) , true , iM ); } ,
	lin_sbst( { X( 1 ) } , { 2 } , Subset( { 1 } ) ) ) ,
  // removing, through c
  make( P3 , { { X( 0 ) , 1 } , { X( 2 ) , 3 } } , true ,
	[ = ]( ModParam iM ) { c->remove_variable( 1 , iM ); } ,
	rmvd_rngd( { X( 1 ) } , Range( 1 , 2 ) ) ) ,
  // the only one
  make( P1 , {} , true ,
	[ = ]( ModParam iM ) { c->remove_variable( 0 , iM ); } ,
	rmvd_rngd( { X( 0 ) } , Range( 0 , 1 ) ) ) ,
  make( P4 , { { X( 0 ) , 1 } , { X( 3 ) , 4 } } , true ,
	[ = ]( ModParam iM ) { c->remove_variables( Range( 1 , 3 ) , iM ); } ,
	rmvd_rngd( { X( 1 ) , X( 2 ) } , Range( 1 , 3 ) ) ) ,
  // the boundaries
  make( P4 , { { X( 1 ) , 2 } , { X( 2 ) , 3 } , { X( 3 ) , 4 } } , true ,
	[ = ]( ModParam iM ) { c->remove_variables( Range( 0 , 1 ) , iM ); } ,
	rmvd_rngd( { X( 0 ) } , Range( 0 , 1 ) ) ) ,
  make( P4 , P3 , true ,
	[ = ]( ModParam iM ) { c->remove_variables( Range( 3 , 4 ) , iM ); } ,
	rmvd_rngd( { X( 3 ) } , Range( 3 , 4 ) ) ) ,
  // the full Range is said as an empty Subset
  make( P4 , {} , true ,
	[ = ]( ModParam iM ) { c->remove_variables( Range( 0 , 4 ) , iM ); } ,
	rmvd_sbst( { X( 0 ) , X( 1 ) , X( 2 ) , X( 3 ) } , Subset() ) ) ,
  // an unordered Subset comes out ordered
  make( P4 , { { X( 0 ) , 1 } , { X( 2 ) , 3 } } , true ,
	[ = ]( ModParam iM ) {
	 c->remove_variables( Subset( { 3 , 1 } ) , false , iM ); } ,
	rmvd_sbst( { X( 1 ) , X( 3 ) } , Subset( { 1 , 3 } ) ) ) ,
  // all of them, by name
  make( P4 , {} , true ,
	[ = ]( ModParam iM ) {
	 c->remove_variables( Subset( { 0 , 1 , 2 , 3 } ) , true , iM ); } ,
	rmvd_sbst( { X( 0 ) , X( 1 ) , X( 2 ) , X( 3 ) } ,
		   Subset( { 0 , 1 , 2 , 3 } ) ) ) };

 // the constant term
 changes.push_back( Case{
  [ = ]() { rebuild( *c , *x , P2 ); } ,
  [ = ]( ModParam iM ) { lf()->set_constant_term( 4 , iM ); } ,
  [ = ]() { return( lf()->get_constant_term() == 0 ); } ,
  [ = ]() { return( lf()->get_constant_term() == 4 ); } ,
  [ = ]( const sp_Mod & mod ) {
   auto fmod = std::dynamic_pointer_cast< C05FunctionMod >( mod );
   assert( fmod && ( fmod->function() == lf() ) );
   assert( fmod->type() == C05FunctionMod::NothingChanged );
   assert( fmod->which().empty() );
   assert( fmod->shift() == 4 );
   assert( ! std::dynamic_pointer_cast< C05FunctionModRngd >( mod ) );
   assert( ! std::dynamic_pointer_cast< C05FunctionModSbst >( mod ) );
   } } );

 auto none = []( const sp_Mod & ) { assert( false ); };

 std::vector< Case > noops = {
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->add_variables( {} , iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->add_variable( nullptr , 1 , iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->modify_coefficient( 1 , 2 , iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->modify_coefficients( {} , Range( 0 , 0 ) ,
                                                     iM ); } , none ) ,
  // a Range all past the end is empty
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->modify_coefficients( { 1 } , Range( 2 , 5 ) ,
                                                     iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->modify_coefficients( {} , Subset() , false ,
                                                     iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          lf()->set_constant_term( 0 , iM ); } , none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          c->remove_variables( Range( 0 , 0 ) , iM ); } ,
	none ) ,
  make( P2 , P2 , true , [ = ]( ModParam iM ) {
                          c->remove_variables( Range( 2 , 2 ) , iM ); } ,
	none ) };

 for( const auto & cs : changes ) {
  check_listened( r , cs );
  check_unheard_noblck( r , cs );
  check_channel( r , cs );
  }
 for( const auto & cs : noops )
  check_nothing( r , cs );

 // the empty Subset means all of them
 Case all_sbst = make( P3 , {} , true ,
		       [ = ]( ModParam iM ) {
			c->remove_variables( Subset() , true , iM ); } ,
		       rmvd_sbst( { X( 0 ) , X( 1 ) , X( 2 ) } , Subset() ) );
 check_issued( r , all_sbst , eNoBlck , false );
 check_issued( r , all_sbst , eModBlck , true );
 check_channel( r , all_sbst );

 // a Range past the end is cut to it, and the Modification says so
 Case past_end = make( P4 , { { X( 0 ) , 1 } , { X( 1 ) , 2 } } , true ,
		       [ = ]( ModParam iM ) {
			c->remove_variables( Range( 2 , Inf< Index >() ) ,
					     iM ); } ,
		       rmvd_rngd( { X( 2 ) , X( 3 ) } , Range( 2 , 4 ) ) );
 check_issued( r , past_end , eNoBlck , false );
 check_issued( r , past_end , eModBlck , true );
 check_channel( r , past_end );

 // removing without a Modification, c lets go of all the Variable the
 // empty Subset stands for, and of those the Range cut to the end has
 check_nomod( r , all_sbst );
 check_unheard_noblck( r , all_sbst );
 check_nomod( r , past_end );
 check_unheard_noblck( r , past_end );

 // under eModBlck the Modification reaches the Block even if no Solver is
 // listening, also when the removal goes through c
 for( const auto & cs : changes )
  check_unheard_modblck( r , cs );
 check_unheard_modblck( r , all_sbst );
 check_unheard_modblck( r , past_end );

 for( const auto & cs : changes )
  check_dry_run( r , cs );
 check_dry_run( r , all_sbst );
 check_dry_run( r , past_end );

 // the same within an FRealObjective of the Block
 auto obj = new FRealObjective;
 r.objective = obj;
 r.block->set_objective( obj , eNoMod );
 auto of = [ obj ]() {
  return( static_cast< LinearFunction * >( obj->get_function() ) );
  };

 auto omake = [ & ]( const v_coeff_pair & p0 , const v_coeff_pair & p1 ,
		     bool by_o , std::function< void( ModParam ) > call ,
		     std::function< void( const sp_Mod & ) > check ) {
  return( Case{ [ = ]() { rebuild( *obj , *x , p0 ); } ,
		call ,
		[ = ]() { return( is_lf( obj->get_function() , p0 ) &&
				  registered( *x , obj , p0 ) ); } ,
		[ = ]() {
		 return( is_lf( obj->get_function() , p1 ) &&
			 ( ( ! by_o ) || registered( *x , obj , p1 ) ) ); } ,
		[ = ]( const sp_Mod & mod ) {
		 assert( registered( *x , obj , p1 ) );
		 auto fmod = std::dynamic_pointer_cast< FunctionMod >( mod );
		 auto vmod = std::dynamic_pointer_cast< FunctionModVars >( mod );
		 assert( ( fmod && ( fmod->function() == of() ) ) ||
			 ( vmod && ( vmod->function() == of() ) ) );
		 check( mod );
		 } } );
  };

 std::vector< Case > ochanges = {
  omake( P1 , P2 , false , [ = ]( ModParam iM ) {
                            of()->add_variable( X( 1 ) , 2 , iM ); } ,
	 []( const sp_Mod & mod ) {
	  auto fmod = std::dynamic_pointer_cast< LinearFunctionModVarsAddd >(
									mod );
	  assert( fmod && ( fmod->first() == 1 ) );
	  } ) ,
  omake( P2 , { { X( 1 ) , 2 } } , true ,
	 [ = ]( ModParam iM ) { obj->remove_variable( 0 , iM ); } ,
	 [ = ]( const sp_Mod & mod ) {
	  auto fmod = std::dynamic_pointer_cast< C05FunctionModVarsRngd >(
									mod );
	  assert( fmod && ( fmod->range() == Range( 0 , 1 ) ) );
	  assert( fmod->vars() == Vec_p_Var( { X( 0 ) } ) );
	  } ) ,
  omake( P3 , { { X( 1 ) , 2 } } , true ,
	 [ = ]( ModParam iM ) {
	  obj->remove_variables( Subset( { 2 , 0 } ) , false , iM ); } ,
	 [ = ]( const sp_Mod & mod ) {
	  auto fmod = std::dynamic_pointer_cast< C05FunctionModVarsSbst >(
									mod );
	  assert( fmod && ( fmod->subset() == Subset( { 0 , 2 } ) ) );
	  assert( fmod->vars() == Vec_p_Var( { X( 0 ) , X( 2 ) } ) );
	  } ) ,
  omake( P2 , P2 , false , [ = ]( ModParam iM ) {
                            of()->set_constant_term( -3 , iM ); } ,
	 []( const sp_Mod & mod ) {
	  auto fmod = std::dynamic_pointer_cast< C05FunctionMod >( mod );
	  assert( fmod && ( fmod->shift() == -3 ) );
	  assert( fmod->type() == C05FunctionMod::NothingChanged );
	  } ) };

 // set_constant_term() changes what is_lf() does not look at
 ochanges.back().before = [ = ]() {
  return( of()->get_constant_term() == 0 ); };
 ochanges.back().after = [ = ]() {
  return( of()->get_constant_term() == -3 ); };

 for( const auto & cs : ochanges ) {
  check_listened( r , cs );
  check_unheard_noblck( r , cs );
  check_channel( r , cs );
  }

 for( const auto & cs : ochanges ) {
  check_unheard_modblck( r , cs );
  check_dry_run( r , cs );
  }
 }

/*--------------------------------------------------------------------------*/
/* add_dynamic_variables() / add_dynamic_constraints() and the removing
 * methods of dynamic Variable / Constraint of a Block, by iterator, by
 * iterators, by Range and by Subset: a BlockModAdd with the list, the added
 * elements and the name of the first, a BlockModRmvRngd with the Range, or
 * a BlockModRmvSbst with the Subset (empty when the list has been emptied),
 * with the removed elements, whose addresses are those they had. */

// what tells Variable from Constraint, for the template below

static void add_d( Block * b , std::list< ColVariable > & l ,
		   std::list< ColVariable > & n , ModParam iM )
{
 b->add_dynamic_variables( l , n , iM );
 }

static void add_d( Block * b , std::list< FRowConstraint > & l ,
		   std::list< FRowConstraint > & n , ModParam iM )
{
 b->add_dynamic_constraints( l , n , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l , Range range ,
		   ModParam iM )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variables( l , range , iM , iM );
 else
  b->remove_dynamic_constraints( l , range , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l , Subset && nms ,
		   bool ordered , ModParam iM )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variables( l , std::move( nms ) , ordered , iM , iM );
 else
  b->remove_dynamic_constraints( l , std::move( nms ) , ordered , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l ,
		   typename std::list< T >::iterator it , ModParam iM )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variable( l , it , iM , iM );
 else
  b->remove_dynamic_constraint( l , it , iM );
 }

template< class T >
static void rmv_d( Block * b , std::list< T > & l ,
		   std::vector< typename std::list< T >::iterator > & its ,
		   ModParam iM )
{
 if constexpr( std::is_base_of_v< Variable , T > )
  b->remove_dynamic_variables( l , its , iM , iM );
 else
  b->remove_dynamic_constraints( l , its , iM );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

template< class T >
static void test_dynamic( Rig & r , std::list< T > & l )
{
 const bool isvar = std::is_base_of_v< Variable , T >;
 auto b = r.block;
 Addrs old;  // the elements of the list after the reset

 auto refill = [ & , b ]( Index n ) {
  rmv_d( b , l , Subset() , true , eNoMod );
  std::list< T > nl( n );
  add_d( b , l , nl , eNoMod );
  old = addresses( l );
  };

 auto its = [ & ]( std::initializer_list< Index > pos ) {
  std::vector< typename std::list< T >::iterator > rv;
  for( auto i : pos )
   rv.push_back( std::next( l.begin() , i ) );
  return( rv );
  };

 // adding n elements to a list of n0
 auto add = [ & , b ]( Index n0 , Index n ) {
  return( Case{ [ & , n0 ]() { refill( n0 ); } ,
		[ & , b , n ]( ModParam iM ) {
		 std::list< T > nl( n );
		 add_d( b , l , nl , iM );
		 } ,
		[ & , n0 ]() { return( l.size() == n0 ); } ,
		[ & , b , n0 , n ]() {
		 if( l.size() != n0 + n )
		  return( false );
		 auto now = addresses( l );
		 for( Index i = 0 ; i < n0 ; ++i )
		  if( now[ i ] != old[ i ] )
		   return( false );
		 for( auto & el : l )
		  if( el.get_Block() != b )
		   return( false );
		 return( true );
		 } ,
		[ & , n0 , n ]( const sp_Mod & mod ) {
		 auto bmod = std::dynamic_pointer_cast< BlockModAdd< T > >( mod );
		 assert( bmod && ( & bmod->whc() == & l ) );
		 assert( bmod->is_added() && ( bmod->is_variable() == isvar ) );
		 assert( bmod->first() == n0 );
		 auto now = addresses( l );
		 assert( bmod->added().size() == n );
		 for( Index k = 0 ; k < n ; ++k )
		  assert( bmod->added()[ k ] == now[ n0 + k ] );
		 } } );
  };

 // removing from a list of n0 elements, leaving those in the positions
 // left; the Modification removes those in the positions gone and says it
 // with the Range, or with the Subset if range is empty
 auto rmv = [ & ]( Index n0 , std::function< void( ModParam ) > call ,
		   std::initializer_list< Index > left ,
		   std::initializer_list< Index > gone ,
		   Range range , Subset subset ) {
  std::vector< Index > lft( left ) , gn( gone );
  return( Case{ [ & , n0 ]() { refill( n0 ); } ,
		call ,
		[ & , n0 ]() { return( l.size() == n0 ); } ,
		[ & , lft ]() {
		 Addrs want;
		 for( auto i : lft )
		  want.push_back( old[ i ] );
		 return( addresses( l ) == want );
		 } ,
		[ & , gn , range , subset ]( const sp_Mod & mod ) {
		 Addrs want;
		 for( auto i : gn )
		  want.push_back( old[ i ] );
		 if( range.second > range.first ) {
		  auto bmod = std::dynamic_pointer_cast< BlockModRmvRngd< T > >(
									mod );
		  assert( bmod && ( & bmod->whc() == & l ) );
		  assert( ( ! bmod->is_added() ) &&
			  ( bmod->is_variable() == isvar ) );
		  assert( bmod->range() == range );
		  assert( addresses( bmod->removed() ) == want );
		  }
		 else {
		  auto bmod = std::dynamic_pointer_cast< BlockModRmvSbst< T > >(
									mod );
		  assert( bmod && ( & bmod->whc() == & l ) );
		  assert( ( ! bmod->is_added() ) &&
			  ( bmod->is_variable() == isvar ) );
		  assert( bmod->subset() == subset );
		  assert( addresses( bmod->removed() ) == want );
		  }
		 } } );
  };

 const Range NR( 0 , 0 );  // not a Range: a Subset is expected

 std::vector< Case > changes = {
  add( 3 , 2 ) ,
  add( 0 , 1 ) ,  // to an empty list
  // one element in the middle
  rmv( 3 , [ & , b ]( ModParam iM ) {
            rmv_d( b , l , std::next( l.begin() , 1 ) , iM ); } ,
       { 0 , 2 } , { 1 } , Range( 1 , 2 ) , {} ) ,
  // the only one: the list is emptied
  rmv( 1 , [ & , b ]( ModParam iM ) { rmv_d( b , l , l.begin() , iM ); } ,
       {} , { 0 } , NR , {} ) ,
  // by iterators
  rmv( 4 , [ & , b ]( ModParam iM ) {
            auto v = its( { 0 , 2 } );
            rmv_d( b , l , v , iM ); } ,
       { 1 , 3 } , { 0 , 2 } , NR , { 0 , 2 } ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) {
            auto v = its( { 1 , 2 } );
            rmv_d( b , l , v , iM ); } ,
       { 0 , 3 } , { 1 , 2 } , Range( 1 , 3 ) , {} ) ,
  rmv( 3 , [ & , b ]( ModParam iM ) {
            auto v = its( { 0 , 1 , 2 } );
            rmv_d( b , l , v , iM ); } ,
       {} , { 0 , 1 , 2 } , NR , {} ) ,
  // by Range, in the middle, at the boundaries, the full one
  rmv( 4 , [ & , b ]( ModParam iM ) { rmv_d( b , l , Range( 1 , 3 ) , iM ); } ,
       { 0 , 3 } , { 1 , 2 } , Range( 1 , 3 ) , {} ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) { rmv_d( b , l , Range( 0 , 1 ) , iM ); } ,
       { 1 , 2 , 3 } , { 0 } , Range( 0 , 1 ) , {} ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) { rmv_d( b , l , Range( 3 , 4 ) , iM ); } ,
       { 0 , 1 , 2 } , { 3 } , Range( 3 , 4 ) , {} ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) { rmv_d( b , l , Range( 0 , 4 ) , iM ); } ,
       {} , { 0 , 1 , 2 , 3 } , NR , {} ) ,
  // by Subset, unordered, contiguous (hence a Range), all by name, empty
  rmv( 4 , [ & , b ]( ModParam iM ) {
            rmv_d( b , l , Subset( { 3 , 1 } ) , false , iM ); } ,
       { 0 , 2 } , { 1 , 3 } , NR , { 1 , 3 } ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) {
            rmv_d( b , l , Subset( { 2 , 1 } ) , false , iM ); } ,
       { 0 , 3 } , { 1 , 2 } , Range( 1 , 3 ) , {} ) ,
  rmv( 4 , [ & , b ]( ModParam iM ) {
            rmv_d( b , l , Subset( { 0 , 1 , 2 , 3 } ) , true , iM ); } ,
       {} , { 0 , 1 , 2 , 3 } , NR , {} ) ,
  rmv( 3 , [ & , b ]( ModParam iM ) {
            rmv_d( b , l , Subset() , true , iM ); } ,
       {} , { 0 , 1 , 2 } , NR , {} ) };

 std::vector< Case > noops = {
  // adding nothing
  Case{ [ & ]() { refill( 2 ); } ,
	[ & , b ]( ModParam iM ) {
	 std::list< T > nl;
	 add_d( b , l , nl , iM ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[]( const sp_Mod & ) { assert( false ); } } ,
  // removing nothing
  Case{ [ & ]() { refill( 2 ); } ,
	[ & , b ]( ModParam iM ) { rmv_d( b , l , Range( 1 , 1 ) , iM ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[]( const sp_Mod & ) { assert( false ); } } ,
  Case{ [ & ]() { refill( 2 ); } ,
	[ & , b ]( ModParam iM ) {
	 std::vector< typename std::list< T >::iterator > v;
	 rmv_d( b , l , v , iM ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[ & ]() { return( addresses( l ) == old ); } ,
	[]( const sp_Mod & ) { assert( false ); } } ,
  // "all" of an empty list
  Case{ [ & ]() { refill( 0 ); } ,
	[ & , b ]( ModParam iM ) { rmv_d( b , l , Subset() , false , iM ); } ,
	[ & ]() { return( l.empty() ); } ,
	[ & ]() { return( l.empty() ); } ,
	[]( const sp_Mod & ) { assert( false ); } } };

 for( const auto & c : changes )
  check_all( r , c );
 for( const auto & c : noops )
  check_nothing( r , c );

 for( const auto & c : changes )
  check_dry_run( r , c );

 r.clear();
 refill( 0 );
 }

/*--------------------------------------------------------------------------*/

static void test_AbstractBlock( void )
{
 Rig r;
 auto vars = new std::list< ColVariable >;
 r.block->add_dynamic_variable( *vars , "y" );
 auto rows = new std::list< FRowConstraint >;
 r.block->add_dynamic_constraint( *rows , "c" );

 test_dynamic( r , *vars );
 test_dynamic( r , *rows );
 }

/*--------------------------------------------------------------------------*/
/* A dynamic Variable that is removed is removed first from the stuff it is
 * active in, which issues the Modification of that stuff under issueindMod
 * before the BlockModRmv* under issueMod. */

static void test_removed_Variable_in_stuff( void )
{
 Rig r;
 auto vars = new std::list< ColVariable >;
 r.block->add_dynamic_variable( *vars , "y" );
 auto rows = new std::vector< FRowConstraint >( 2 );
 r.block->add_static_constraint( *rows , "c" );
 auto c0 = & ( *rows )[ 0 ];
 auto c1 = & ( *rows )[ 1 ];

 // one Variable, active in c0 (and in c1 if both)
 auto setup = [ & ]( bool both ) {
  std::list< ColVariable > nl( 1 );
  r.block->add_dynamic_variables( *vars , nl , eNoMod );
  auto y = & vars->front();
  c0->set_function( new LinearFunction( { { y , 1 } } ) , eNoMod );
  c1->set_function( new LinearFunction( both ? v_coeff_pair{ { y , 2 } }
					     : v_coeff_pair{} ) , eNoMod );
  r.clear();
  return( y );
  };

 // issueindMod = eNoMod: only the BlockModRmv*, and c0 lets y go all the same
 {
  auto y = setup( false );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eModBlck ,
				    eNoMod );
  assert( vars->empty() );
  assert( c0->get_function()->get_num_active_var() == 0 );
  assert( r.got().size() == 1 );
  auto bmod = std::dynamic_pointer_cast< BlockModRmvSbst< ColVariable > >(
							   r.got().front() );
  assert( bmod && bmod->subset().empty() &&
	  ( & bmod->removed().front() == y ) );
  r.clear();
  }

 // issueindMod = eModBlck: first the Modification of c0, then the Block's
 {
  auto y = setup( false );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eModBlck ,
				    eModBlck );
  assert( vars->empty() );
  assert( c0->get_function()->get_num_active_var() == 0 );
  assert( r.got().size() == 2 );
  auto fmod = std::dynamic_pointer_cast< C05FunctionModVarsRngd >(
							   r.got().front() );
  assert( fmod && ( fmod->function() == c0->get_function() ) );
  assert( ( fmod->vars() == Vec_p_Var( { y } ) ) &&
	  ( fmod->range() == Range( 0 , 1 ) ) );
  assert( fmod->concerns_Block() );
  auto bmod = std::dynamic_pointer_cast< BlockModRmvSbst< ColVariable > >(
							    r.got().back() );
  assert( bmod && ( & bmod->removed().front() == y ) );
  r.clear();
  }

 // issueindMod = eNoBlck: the same, but the first does not concern the Block
 {
  setup( false );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eModBlck ,
				    eNoBlck );
  assert( r.got().size() == 2 );
  assert( ! r.got().front()->concerns_Block() );
  assert( r.got().back()->concerns_Block() );
  r.clear();
  }

 // issueMod = eDryRun: nothing at all, whatever issueindMod is
 for( ModParam iiM : { eDryRun , eNoMod , eNoBlck , eModBlck } ) {
  auto y = setup( true );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eDryRun , iiM );
  assert( ( vars->size() == 1 ) && ( & vars->front() == y ) );
  assert( c0->get_function()->get_num_active_var() == 1 );
  assert( c1->get_function()->get_num_active_var() == 1 );
  assert( active_in( *y , c0 ) && active_in( *y , c1 ) );
  assert( r.got().empty() && r.seen().empty() );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eNoMod , eNoMod );
  r.clear();
  }

 // a dynamic Variable active in two stuff is removed from both, although
 // each removal takes the stuff out of the active list of the Variable
 // that Block::remove_variable_from_stuff() walks
 {
  setup( true );
  r.block->remove_dynamic_variable( *vars , vars->begin() , eModBlck ,
				    eModBlck );
  assert( vars->empty() );
  assert( c0->get_function()->get_num_active_var() == 0 );
  assert( c1->get_function()->get_num_active_var() == 0 );
  assert( r.got().size() == 3 );
  r.clear();
  }
 }

/*--------------------------------------------------------------------------*/
/* The modifying methods of a DQuadFunction and of a QuadFunction within an
 * FRowConstraint of the Block and of a PolyhedralFunction within an
 * FRealObjective of the Block, including the removal of Variable through
 * the stuff: under eDryRun each of them leaves the data, the Variable and
 * their registration as they were, and issues nothing. */

static void test_dry_run( void )
{
 Rig r;
 auto x = new std::vector< ColVariable >( 3 );
 r.block->add_static_variable( *x , "x" );
 auto X = [ x ]( Index i ) { return( & ( *x )[ i ] ); };
 auto c = new FRowConstraint;
 r.block->add_static_constraint( *c , "c" );
 auto obj = new FRealObjective;
 r.objective = obj;
 r.block->set_objective( obj , eNoMod );

 // the state of a stuff s with Function f: the active Variable of f, which
 // of x see s as active, and the data of f that the family gives
 using State = std::pair< Addrs , std::vector< double > >;
 std::function< std::vector< double >( void ) > data;
 auto state = [ & ]( const ThinVarDepInterface * s , const Function * f ) {
  State st;
  for( Index i = 0 ; i < f->get_num_active_var() ; ++i )
   st.first.push_back( f->get_active_var( i ) );
  for( const auto & v : *x )
   st.second.push_back( active_in( v , s ) ? 1 : 0 );
  auto d = data();
  st.second.insert( st.second.end() , d.begin() , d.end() );
  return( st );
  };

 // a Case that gives s a new Function, and is to find it unchanged
 State snap;
 auto dry = [ & ]( auto * s , std::function< Function * ( void ) > make ,
		   std::function< void( ModParam ) > call ) {
  return( Case{ [ & , s , make ]() {
                 s->set_function( make() , eNoMod );
                 snap = state( s , s->get_function() );
                 } ,
		call ,
		[ & , s ]() {
		 return( state( s , s->get_function() ) == snap ); } ,
		nullptr , nullptr } );
  };

 // DQuadFunction - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 auto dq = [ c ]() {
  return( static_cast< DQuadFunction * >( c->get_function() ) );
  };
 auto dq_new = [ X ]() -> Function * {
  return( new DQuadFunction( { { X( 0 ) , 1 , 2 } , { X( 1 ) , 3 , 4 } } ,
			     5 ) );
  };
 auto dq_data = [ dq ]() {
  std::vector< double > d;
  for( Index i = 0 ; i < dq()->get_num_active_var() ; ++i ) {
   d.push_back( dq()->get_linear_coefficient( i ) );
   d.push_back( dq()->get_quadratic_coefficient( i ) );
   }
  d.push_back( dq()->get_constant_term() );
  return( d );
  };

 const DQuadFunction::v_coeff NQ = { 7 , 8 };
 const DQuadFunction::v_coeff NL = { 9 , 6 };

 std::vector< Case > dq_cases = {
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->add_variables( { { X( 2 ) , 6 , 7 } } , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->add_variable( X( 2 ) , 6 , 7 , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_term( 0 , 8 , 9 , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_linear_coefficient( 1 , 8 , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_terms( NQ.begin() , NL.begin() ,
                                         Range( 0 , 2 ) , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_terms( NQ.begin() , NL.begin() ,
                                         Subset( { 1 , 0 } ) , false ,
                                         iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_linear_coefficients( { 9 , 6 } ,
                                                       Range( 0 , 2 ) ,
                                                       iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->modify_linear_coefficients( { 9 } ,
                                                       Subset( { 1 } ) ,
                                                       true , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     dq()->set_constant_term( 0 , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) { c->remove_variable( 0 , iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     c->remove_variables( Range( 0 , Inf< Index >() ) ,
                                          iM ); } ) ,
  dry( c , dq_new , [ = ]( ModParam iM ) {
                     c->remove_variables( Subset() , false , iM ); } ) };

 data = dq_data;
 for( const auto & cs : dq_cases )
  check_dry_run( r , cs );

 // QuadFunction- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 auto qf = [ c ]() {
  return( static_cast< QuadFunction * >( c->get_function() ) );
  };
 auto qf_new = [ X ]() -> Function * {
  return( new QuadFunction( { { X( 0 ) , 1 , 2 } , { X( 1 ) , 3 , 4 } } ,
			    { { 1 , 0 , 5 } } ) );
  };
 auto qf_data = [ qf ]() {
  std::vector< double > d;
  const Index n = qf()->get_num_active_var();
  for( Index i = 0 ; i < n ; ++i ) {
   d.push_back( qf()->get_linear_coefficient( i ) );
   for( Index j = 0 ; j < n ; ++j )
    d.push_back( qf()->get_quadratic_coefficient( i , j ) );
   }
  return( d );
  };

 std::vector< Case > qf_cases = {
  dry( c , qf_new , [ = ]( ModParam iM ) {
                     qf()->add_variables( { { X( 2 ) , 6 , 7 } } ,
                                          { { 2 , 0 , 8 } } , iM ); } ) ,
  dry( c , qf_new , [ = ]( ModParam iM ) {
                     qf()->add_nd_term( X( 1 ) , X( 2 ) , 8 , iM ); } ) ,
  dry( c , qf_new , [ = ]( ModParam iM ) {
                     qf()->modify_term( 0 , 1 , 9 , iM ); } ) ,
  dry( c , qf_new , [ = ]( ModParam iM ) { c->remove_variable( 1 , iM ); } ) ,
  dry( c , qf_new , [ = ]( ModParam iM ) {
                     c->remove_variables( Range( 0 , 2 ) , iM ); } ) ,
  dry( c , qf_new , [ = ]( ModParam iM ) {
                     c->remove_variables( Subset( { 0 } ) , true ,
                                          iM ); } ) };

 data = qf_data;
 for( const auto & cs : qf_cases )
  check_dry_run( r , cs );

 // PolyhedralFunction- - - - - - - - - - - - - - - - - - - - - - - - - - - -
 auto pf = [ obj ]() {
  return( static_cast< PolyhedralFunction * >( obj->get_function() ) );
  };
 auto pf_new = [ X ]() -> Function * {
  return( new PolyhedralFunction( { X( 0 ) , X( 1 ) } ,
				  { { 1 , 2 } , { 3 , 4 } } , { 5 , 6 } ,
				  -10 , true ) );
  };
 auto pf_data = [ pf ]() {
  std::vector< double > d;
  for( const auto & row : pf()->get_A() )
   d.insert( d.end() , row.begin() , row.end() );
  d.insert( d.end() , pf()->get_b().begin() , pf()->get_b().end() );
  d.push_back( pf()->is_convex() ? 1 : 0 );
  d.push_back( pf()->get_global_lower_bound() );
  d.push_back( pf()->get_global_upper_bound() );
  return( d );
  };

 std::vector< Case > pf_cases = {
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->set_is_convex( false , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->add_variable( X( 2 ) , { 7 , 8 } , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->modify_constant( 0 , 9 , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->modify_bound( -20 , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->add_row( { 7 , 8 } , 9 , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       pf()->delete_row( 0 , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       obj->remove_variable( 0 , iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       obj->remove_variables( Range( 1 , Inf< Index >() ) ,
                                              iM ); } ) ,
  dry( obj , pf_new , [ = ]( ModParam iM ) {
                       obj->remove_variables( Subset() , false , iM ); } ) };

 data = pf_data;
 for( const auto & cs : pf_cases )
  check_dry_run( r , cs );
 }

/*--------------------------------------------------------------------------*/
/* add_dual_pairs(), remove_variable() and remove_variables() of a
 * LagBFunction, whose inner Block is an AbstractBlock with a linear
 * Objective, and the methods of its global pool: under eDryRun each of them
 * leaves the Lagrangian pairs, their terms and the global pool as they
 * were, and issues nothing, while under eNoMod each of them does the change.
 * No Solver is attached to the inner Block: the global pool is filled with
 * intNoSol, under which a combination stands for a Solution, and
 * store_linearization(), which takes the Solution of the Solver, is only
 * checked under eDryRun. */

static void test_dry_run_LagBFunction( void )
{
 Rig r;
 std::vector< ColVariable > y( 3 );          // the multipliers
 std::vector< ColVariable > * z = nullptr;  // the Variable of the inner Block
 auto Z = [ & z ]( Index i ) { return( & ( *z )[ i ] ); };
 LagBFunction * lbf = nullptr;
 FRealObjective * iobj = nullptr;            // Objective of inner Block

 // the pairs, their terms and which names the global pool holds
 using State = std::pair< Addrs , std::vector< double > >;
 auto state = [ & ]() {
  State st;
  for( Index i = 0 ; i < lbf->get_num_active_var() ; ++i ) {
   auto lf = static_cast< const LinearFunction * >(
					      lbf->get_Lagrangian_term( i ) );
   st.first.push_back( lbf->get_active_var( i ) );
   st.first.push_back( lf );
   for( const auto & p : lf->get_v_var() ) {
    st.first.push_back( p.first );
    st.second.push_back( p.second );
    }
   st.second.push_back( lf->get_constant_term() );
   }
  for( Index n = 0 ; n < 3 ; ++n )
   st.second.push_back( lbf->is_linearization_there( n ) ? 1 : 0 );
  return( st );
  };

 // the LagBFunction goes, and the inner Block with it; the Objective, which
 // the AbstractBlock does not own, has been clear()-ed by it
 auto drop = [ & ]() {
  if( ! lbf )
   return;
  delete lbf;
  iobj->set_Block( nullptr );
  delete iobj;
  lbf = nullptr;
  };

 // a new LagBFunction each time, two pairs and linearization 0 in the pool
 State snap;
 auto make = [ & ]() {
  drop();
  auto inner = new AbstractBlock;
  z = new std::vector< ColVariable >( 2 );
  inner->add_static_variable( *z , "z" );
  iobj = new FRealObjective( inner , new LinearFunction( v_coeff_pair{
				      { Z( 0 ) , 1 } , { Z( 1 ) , 2 } } ) );
  inner->set_objective( iobj , eNoMod );

  lbf = new LagBFunction( inner );
  LagBFunction::v_dual_pair dp;
  dp.emplace_back( & y[ 0 ] , new LinearFunction(
				    v_coeff_pair{ { Z( 0 ) , 1 } } , 2 ) );
  dp.emplace_back( & y[ 1 ] , new LinearFunction( v_coeff_pair{
				     { Z( 0 ) , 3 } , { Z( 1 ) , 4 } } ) );
  lbf->set_dual_pairs( std::move( dp ) );
  lbf->set_par( LagBFunction::intNoSol , 1 );
  lbf->set_par( LagBFunction::intGPMaxSz , 3 );
  lbf->store_combination_of_linearizations( { { 0 , 1 } } , 0 , eNoMod );
  lbf->register_Observer( r.block );
  snap = state();
  };

 auto lbf_case = [ & ]( std::function< void( ModParam ) > call ,
			std::function< bool( void ) > after ) {
  return( Case{ make , call , [ & ]() { return( state() == snap ); } ,
		after , nullptr } );
  };

 auto n_is = [ & ]( Index n ) {
  return( lbf->get_num_active_var() == n );
  };

 std::vector< Case > cases = {
  lbf_case( [ & ]( ModParam iM ) {
             LagBFunction::v_dual_pair dp;
             dp.emplace_back( & y[ 2 ] , new LinearFunction(
                                          v_coeff_pair{ { Z( 1 ) , 5 } } ) );
             auto term = dp.front().second;
             lbf->add_dual_pairs( std::move( dp ) , iM );
             if( lbf->get_num_active_var() < 3 )  // not taken, still ours
              delete term;
             } ,
	    [ & ]() { return( n_is( 3 ) &&
			      ( lbf->get_active_var( 2 ) == & y[ 2 ] ) );
	     } ) ,
  lbf_case( [ & ]( ModParam iM ) { lbf->remove_variable( 0 , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( lbf->get_active_var( 0 ) == & y[ 1 ] ) );
	     } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->remove_variables( Range( 1 , Inf< Index >() ) , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( lbf->get_active_var( 0 ) == & y[ 0 ] ) );
	     } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->remove_variables( Range( 0 , Inf< Index >() ) , iM ); } ,
	    [ & ]() { return( n_is( 0 ) ); } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->remove_variables( Subset( { 0 } ) , true , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( lbf->get_active_var( 0 ) == & y[ 1 ] ) );
	     } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->remove_variables( Subset() , false , iM ); } ,
	    [ & ]() { return( n_is( 0 ) ); } ) ,
  lbf_case( [ & ]( ModParam iM ) { lbf->store_linearization( 1 , iM ); } ,
	    nullptr ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->store_combination_of_linearizations( { { 0 , 1 } } , 1 ,
                                                       iM ); } ,
	    [ & ]() { return( lbf->is_linearization_there( 1 ) ); } ) ,
  lbf_case( [ & ]( ModParam iM ) { lbf->delete_linearization( 0 , iM ); } ,
	    [ & ]() { return( ! lbf->is_linearization_there( 0 ) ); } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->delete_linearizations( Subset( { 0 } ) , true , iM ); } ,
	    [ & ]() { return( ! lbf->is_linearization_there( 0 ) ); } ) ,
  lbf_case( [ & ]( ModParam iM ) {
             lbf->delete_linearizations( Subset() , true , iM ); } ,
	    [ & ]() { return( ! lbf->is_linearization_there( 0 ) ); } ) };

 for( const auto & cs : cases ) {
  check_dry_run( r , cs );
  if( cs.after )
   check_nomod( r , cs );
  }

 drop();
 }

/*--------------------------------------------------------------------------*/
/* The modifying methods of a BendersBFunction whose inner Block is an
 * AbstractBlock holding the RowConstraint of the mapping: under eDryRun
 * each of them leaves the Variable, the matrix, the constants, the
 * RowConstraint with their sides and the global pool as they were, and
 * issues nothing, while under eNoMod each of those that change the mapping
 * does the change. No Solver is attached to the inner Block, hence the
 * global pool cannot be filled: store_linearization(),
 * store_combination_of_linearizations() and the deleting methods are only
 * checked under eDryRun, which also has to leave alone the netCDF::NcGroup
 * given to deserialize(), a null one here. */

static void test_dry_run_BendersBFunction( void )
{
 Rig r;
 std::vector< ColVariable > x( 3 );
 auto X = [ & x ]( Index i ) { return( & x[ i ] ); };
 std::vector< FRowConstraint > * c = nullptr;  // the rows of the inner Block
 auto C = [ & c ]( Index i ) { return( & ( *c )[ i ] ); };
 BendersBFunction * bbf = nullptr;

 // the Variable, the RowConstraint, the matrix row by row, the constants,
 // the sides and which names the global pool holds
 using State = std::pair< Addrs , std::vector< double > >;
 auto state = [ & ]() {
  State st;
  for( Index i = 0 ; i < bbf->get_num_active_var() ; ++i )
   st.first.push_back( bbf->get_active_var( i ) );
  for( auto cn : bbf->get_constraints() )
   st.first.push_back( cn );
  for( const auto & row : bbf->get_A() ) {
   st.second.push_back( row.size() );
   st.second.insert( st.second.end() , row.begin() , row.end() );
   }
  st.second.insert( st.second.end() , bbf->get_b().begin() ,
		    bbf->get_b().end() );
  for( auto s : bbf->get_sides() )
   st.second.push_back( s );
  for( Index n = 0 ; n < 2 ; ++n )
   st.second.push_back( bbf->is_linearization_there( n ) ? 1 : 0 );
  return( st );
  };

 // a new BendersBFunction each time, on x[ 0 ] and x[ 1 ] with two rows
 State snap;
 auto make = [ & ]() {
  delete bbf;  // and its inner Block with it
  auto inner = new AbstractBlock;
  c = new std::vector< FRowConstraint >( 3 );
  inner->add_static_constraint( *c , "c" );
  bbf = new BendersBFunction( inner , { X( 0 ) , X( 1 ) } ,
			      { { 1 , 2 } , { 3 , 4 } } , { 5 , 6 } ,
			      { C( 0 ) , C( 1 ) } ,
			      { BendersBFunction::eLHS ,
				BendersBFunction::eRHS } );
  bbf->set_par( BendersBFunction::intGPMaxSz , 2 );
  bbf->register_Observer( r.block );
  snap = state();
  };

 auto bbf_case = [ & ]( std::function< void( ModParam ) > call ,
			std::function< bool( void ) > after ) {
  return( Case{ make , call , [ & ]() { return( state() == snap ); } ,
		after , nullptr } );
  };

 auto n_is = [ & ]( Index n ) {
  return( bbf->get_num_active_var() == n );
  };
 auto rows_are = [ & ]( Index m ) {
  return( ( bbf->get_A().size() == m ) && ( bbf->get_b().size() == m ) &&
	  ( bbf->get_constraints().size() == m ) &&
	  ( bbf->get_sides().size() == m ) );
  };
 auto b_is = [ & ]( Index i , double v ) {
  return( bbf->get_b()[ i ] == v );
  };

 const BendersBFunction::RealVector NB = { 9 , 10 };

 std::vector< Case > cases = {
  bbf_case( [ & ]( ModParam iM ) {
             bbf->set_mapping( { { 7 , 8 } } , { 9 } , { C( 2 ) } ,
                               { BendersBFunction::eBoth } , iM ); } ,
	    [ & ]() { return( rows_are( 1 ) && b_is( 0 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->add_variables( { X( 2 ) } , { { 7 } , { 8 } } , iM ); } ,
	    [ & ]() { return( n_is( 3 ) &&
			      ( bbf->get_A()[ 1 ][ 2 ] == 8 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->add_variable( X( 2 ) , { 7 , 8 } , iM ); } ,
	    [ & ]() { return( n_is( 3 ) &&
			      ( bbf->get_A()[ 1 ][ 2 ] == 8 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->remove_variable( 0 , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( bbf->get_active_var( 0 ) == X( 1 ) ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->remove_variables( Range( 0 , 1 ) , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( bbf->get_active_var( 0 ) == X( 1 ) ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->remove_variables( Range( 0 , Inf< Index >() ) , iM ); } ,
	    [ & ]() { return( n_is( 0 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->remove_variables( Subset( { 1 } ) , true , iM ); } ,
	    [ & ]() { return( n_is( 1 ) &&
			      ( bbf->get_active_var( 0 ) == X( 0 ) ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->remove_variables( Subset() , false , iM ); } ,
	    [ & ]() { return( n_is( 0 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_rows( { { 7 , 8 } } , { 9 } , Range( 0 , 1 ) ,
                               iM ); } ,
	    [ & ]() { return( b_is( 0 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_rows( { { 7 , 8 } } , { 9 } , Subset( { 1 } ) ,
                               true , iM ); } ,
	    [ & ]() { return( b_is( 1 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_row( 0 , { 7 , 8 } , 9 , iM ); } ,
	    [ & ]() { return( b_is( 0 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_constants( NB , Range( 0 , 2 ) , iM ); } ,
	    [ & ]() { return( b_is( 0 , 9 ) && b_is( 1 , 10 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_constants( NB.cbegin() , Range( 0 , 2 ) , iM ,
                                    iM ); } ,
	    [ & ]() { return( b_is( 0 , 9 ) && b_is( 1 , 10 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_constants( NB , Subset( { 1 , 0 } ) , false ,
                                    iM ); } ,
	    [ & ]() { return( b_is( 1 , 9 ) && b_is( 0 , 10 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->modify_constants( NB.cbegin() , Subset( { 1 } ) , true ,
                                    iM , iM ); } ,
	    [ & ]() { return( b_is( 1 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->modify_constant( 1 , 9 , iM ); } ,
	    [ & ]() { return( b_is( 1 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->add_rows( { { 7 , 8 } } , { 9 } , { C( 2 ) } ,
                            { BendersBFunction::eBoth } , iM ); } ,
	    [ & ]() { return( rows_are( 3 ) && b_is( 2 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->add_row( { 7 , 8 } , 9 , C( 2 ) , BendersBFunction::eBoth ,
                           iM ); } ,
	    [ & ]() { return( rows_are( 3 ) && b_is( 2 , 9 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->delete_rows( Range( 0 , Inf< Index >() ) , iM ); } ,
	    [ & ]() { return( rows_are( 0 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->delete_rows( Range( 1 , 2 ) , iM ); } ,
	    [ & ]() { return( rows_are( 1 ) && b_is( 0 , 5 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->delete_rows( Subset( { 1 , 0 } ) , false , iM ); } ,
	    [ & ]() { return( rows_are( 0 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->delete_row( 0 , iM ); } ,
	    [ & ]() { return( rows_are( 1 ) && b_is( 0 , 6 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->delete_rows( iM ); } ,
	    [ & ]() { return( rows_are( 0 ) ); } ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->store_linearization( 0 , iM ); } ,
	    nullptr ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->store_combination_of_linearizations( { { 0 , 1 } } , 1 ,
                                                       iM ); } ,
	    nullptr ) ,
  bbf_case( [ & ]( ModParam iM ) { bbf->delete_linearization( 0 , iM ); } ,
	    nullptr ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->delete_linearizations( Subset() , true , iM ); } ,
	    nullptr ) ,
  bbf_case( [ & ]( ModParam iM ) {
             bbf->deserialize( netCDF::NcGroup() , iM ); } ,
	    nullptr ) };

 for( const auto & cs : cases ) {
  check_dry_run( r , cs );
  if( cs.after )
   check_nomod( r , cs );
  }

 delete bbf;
 }

/*--------------------------------------------------------------------------*/
/* The methods of the global pool of a C05SumFunction of two
 * PolyhedralFunction: under eDryRun each of them leaves the global pool of
 * the sum and those of the members as they were, and issues nothing, while
 * under eNoMod each of them does the change. remove_variable() is left out,
 * since it throws whatever the ModParam, the Variable of a sum being those
 * of its members. */

static void test_dry_run_C05SumFunction( void )
{
 Rig r;
 std::vector< ColVariable > x( 2 );
 x[ 0 ].set_value( 1 );
 x[ 1 ].set_value( 2 );

 auto member = [ & x ]( double c ) {
  PolyhedralFunction::VarVector vars( { & x[ 0 ] , & x[ 1 ] } );
  PolyhedralFunction::MultiVector A( { { c , 0 } , { 0 , c } } );
  PolyhedralFunction::RealVector b( { 0 , 1 } );
  return( new PolyhedralFunction( std::move( vars ) , std::move( A ) ,
				  std::move( b ) ) );
  };

 std::vector< PolyhedralFunction * > members{ member( 1 ) , member( 2 ) };
 auto sum = new C05SumFunction( std::vector< C05Function * >(
					  members.begin() , members.end() ) );
 sum->set_par( C05Function::intGPMaxSz , 3 );
 sum->register_Observer( r.block );

 // which names the sum and the members hold, and what the sum has there
 auto state = [ & ]() {
  std::vector< double > st;
  for( Index n = 0 ; n < 3 ; ++n ) {
   for( auto m : members )
    st.push_back( m->is_linearization_there( n ) ? 1 : 0 );
   if( ! sum->is_linearization_there( n ) ) {
    st.push_back( 0 );
    continue;
    }
   Vec_FV g( 2 );
   sum->get_linearization_coefficients( g.data() , Range( 0 , 2 ) , n );
   st.insert( st.end() , g.begin() , g.end() );
   st.push_back( sum->get_linearization_constant( n ) );
   }
  return( st );
  };

 // the pool emptied, and the linearization at x stored in name 0
 std::vector< double > snap;
 auto reset = [ & ]() {
  sum->delete_linearizations( Subset() , true , eNoMod );
  sum->compute();
  sum->has_linearization( true );
  sum->store_linearization( 0 , eNoMod );
  snap = state();
  };

 auto sum_case = [ & ]( std::function< void( ModParam ) > call ,
			std::function< bool( void ) > after ) {
  return( Case{ reset , call , [ & ]() { return( state() == snap ); } ,
		after , nullptr } );
  };

 std::vector< Case > cases = {
  sum_case( [ & ]( ModParam iM ) { sum->store_linearization( 1 , iM ); } ,
	    [ & ]() { return( sum->is_linearization_there( 1 ) ); } ) ,
  sum_case( [ & ]( ModParam iM ) {
             sum->store_combination_of_linearizations( { { 0 , 1 } } , 2 ,
                                                       iM ); } ,
	    [ & ]() { return( sum->is_linearization_there( 2 ) ); } ) ,
  sum_case( [ & ]( ModParam iM ) { sum->delete_linearization( 0 , iM ); } ,
	    [ & ]() { return( ! sum->is_linearization_there( 0 ) ); } ) ,
  sum_case( [ & ]( ModParam iM ) {
             sum->delete_linearizations( Subset( { 0 } ) , true , iM ); } ,
	    [ & ]() { return( ! sum->is_linearization_there( 0 ) ); } ) ,
  sum_case( [ & ]( ModParam iM ) {
             sum->delete_linearizations( Subset() , true , iM ); } ,
	    [ & ]() { return( ! sum->is_linearization_there( 0 ) ); } ) };

 // the snapshot has something to lose: name 0 is there, in both members
 reset();
 assert( sum->is_linearization_there( 0 ) );
 assert( members[ 0 ]->is_linearization_there( 0 ) &&
	 members[ 1 ]->is_linearization_there( 0 ) );

 for( const auto & cs : cases ) {
  check_dry_run( r , cs );
  check_nomod( r , cs );
  }

 delete sum;  // it gives the members back their Observer
 for( auto m : members )
  delete m;
 }

/*--------------------------------------------------------------------------*/
/* is_active() and remove_active() of a ColVariable with a stuff that is not
 * in its active list: whichever the place the stuff would take in the list,
 * which is ordered by address, is_active() gives Inf and remove_active()
 * throws, leaving the list as it is */

static void test_active_list( void )
{
 std::vector< FRowConstraint > stuff( 5 );
 for( std::size_t k = 0 ; k < stuff.size() ; ++k ) {
  // the active list holds all the stuff but the k-th one
  ColVariable v;
  for( std::size_t h = 0 ; h < stuff.size() ; ++h )
   if( h != k )
    v.add_active( & stuff[ h ] );

  assert( v.get_num_active() == stuff.size() - 1 );
  assert( v.is_active( & stuff[ k ] ) == Inf< Index >() );
  for( std::size_t h = 0 ; h < stuff.size() ; ++h )
   if( h != k )
    assert( v.get_active( v.is_active( & stuff[ h ] ) ) == & stuff[ h ] );

  assert( throws( [ & ]() { v.remove_active( & stuff[ k ] ); } ) );
  assert( v.get_num_active() == stuff.size() - 1 );
  for( std::size_t h = 0 ; h < stuff.size() ; ++h )
   assert( active_in( v , & stuff[ h ] ) == ( h != k ) );

  // and removing the ones that are there empties it
  for( std::size_t h = 0 ; h < stuff.size() ; ++h )
   if( h != k )
    v.remove_active( & stuff[ h ] );
  assert( v.get_num_active() == 0 );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- MAIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* What changes() says of the Modification of the core, both built directly
 * and issued by the Block, and what the static methods of Modification read
 * from a ModConcern [see Modification::ModConcern]. */

static void test_changes( void )
{
 using M = Modification;
 Rig r;
 auto x = new ColVariable;
 r.block->add_static_variable( *x , "x" );

 // the data of a Variable: fixing it shrinks the region, unfixing it makes
 // it grow, changing its sign may do either; its integrality is a kind of
 // its own, becoming integer shrinking the region and continuous making it
 // grow, and when it changes with something else both kinds are said
 const var_type free = 0 , fixed = 1;
 const var_type integer = var_type( ColVariable::kInteger * 2 );
 const var_type nonneg = var_type( ColVariable::kNonNegative * 2 );
 const var_type natural = var_type( ColVariable::kNatural * 2 );
 assert( VariableMod( x , free , fixed ).changes() ==
	 ( M::eModVarData | M::eRegnShrink ) );
 assert( VariableMod( x , fixed , free ).changes() ==
	 ( M::eModVarData | M::eRegnGrow ) );
 assert( VariableMod( x , free , integer ).changes() ==
	 ( M::eModVarType | M::eRegnShrink ) );
 assert( VariableMod( x , integer , free ).changes() ==
	 ( M::eModVarType | M::eRegnGrow ) );
 assert( VariableMod( x , free , nonneg ).changes() ==
	 ( M::eModVarData | M::eRegnShrink | M::eRegnGrow ) );
 assert( VariableMod( x , free , natural ).changes() ==
	 ( M::eModVarType | M::eModVarData | M::eRegnShrink | M::eRegnGrow ) );
 assert( VariableMod( x , free , var_type( integer + 1 ) ).changes() ==
	 ( M::eModVarType | M::eModVarData | M::eRegnShrink ) );
 assert( VariableMod( x , var_type( integer + 1 ) , integer ).changes() ==
	 ( M::eModVarData | M::eRegnGrow ) );
 assert( M::changes_integrality( M::eModVarType ) &&
	 M::changes_variables( M::eModVarType ) &&
	 ( ! M::changes_integrality( M::eModVarData ) ) );
 assert( VariableMod( x , free , integer ).changes_integrality() &&
	 ( ! VariableMod( x , free , fixed ).changes_integrality() ) );

 // the data of a Constraint: relaxing it makes the region grow, enforcing
 // it shrinks it, changing its sides may do either
 FRowConstraint c;
 assert( ConstraintMod( & c , ConstraintMod::eRelaxConst ).changes() ==
	 ( M::eModCnsSide | M::eRegnGrow ) );
 assert( ConstraintMod( & c , ConstraintMod::eEnforceConst ).changes() ==
	 ( M::eModCnsSide | M::eRegnShrink ) );
 assert( RowConstraintMod( & c , RowConstraintMod::eChgRHS ).changes() ==
	 ( M::eModCnsSide | M::eRegnShrink | M::eRegnGrow ) );

 // the Objective: the region stays, the objective may move either way
 FRealObjective o;
 assert( ObjectiveMod( & o , ObjectiveMod::eSetMax ).changes() ==
	 ( M::eModObj | M::eObjUp | M::eObjDown ) );

 // physical: any effect; nuclear: everything
 assert( PhysMod( r.block ).changes() == ( M::eModPhys | M::eEffAny ) );
 assert( NBModification( r.block ).changes() == M::eModAll );

 // the set of the dynamic Variable and Constraint, as the Block issues it
 auto vars = new std::list< ColVariable >;
 r.block->add_dynamic_variable( *vars , "y" );
 auto rows = new std::list< FRowConstraint >;
 r.block->add_dynamic_constraint( *rows , "c" );

 auto one = [ & ]( std::function< void( void ) > call , M::ModConcern mc ) {
  r.clear();
  call();
  assert( r.got().size() == 1 );
  assert( r.got().front()->changes() == mc );
  };

 one( [ & ]() {
       std::list< ColVariable > n( 2 );
       add_d( r.block , *vars , n , eModBlck );
       } , M::eModVarSet | M::eRegnGrow );
 one( [ & ]() { rmv_d( r.block , *vars , Range( 0 , 1 ) , eModBlck ); } ,
      M::eModVarSet | M::eRegnShrink );
 one( [ & ]() {
       std::list< FRowConstraint > n( 2 );
       add_d( r.block , *rows , n , eModBlck );
       } , M::eModCnsSet | M::eRegnShrink );
 one( [ & ]() { rmv_d( r.block , *rows , Range( 0 , 1 ) , eModBlck ); } ,
      M::eModCnsSet | M::eRegnGrow );

 // a GroupModification says the or of what its sub-Modification say
 one( [ & ]() {
       auto chnl = r.block->open_channel();
       x->is_fixed( true , Observer::make_par( eModBlck , chnl ) );
       std::list< FRowConstraint > n( 1 );
       add_d( r.block , *rows , n , Observer::make_par( eModBlck , chnl ) );
       r.block->close_channel( chnl );
       } , M::eModVarData | M::eModCnsSet | M::eRegnShrink );

 // the static methods
 const auto phys = M::ModConcern( M::eModPhys | M::eEffAny );
 assert( M::is_physical( phys ) && ( ! M::is_abstract( phys ) ) );
 assert( M::is_abstract( M::eModObj ) && ( ! M::is_physical( M::eModObj ) ) );
 assert( M::changes_variables( M::eModVarSet ) &&
	 M::changes_variables( M::eModVarData ) &&
	 ( ! M::changes_variables( M::eModCnsSide ) ) );
 assert( M::changes_constraints( M::eModCnsSet ) &&
	 ( ! M::changes_constraints( M::eModObj ) ) );
 assert( M::changes_objective( M::eModObj ) &&
	 ( ! M::changes_objective( M::eModVarData ) ) );
 assert( M::changes_structure( M::eModVarSet ) &&
	 ( ! M::changes_structure( M::eModVarData ) ) );
 assert( M::changes_only_sides( M::eModCnsSide | M::eRegnShrink ) &&
	 M::changes_only_sides( M::eModPhys | M::eModCnsSide ) &&
	 ( ! M::changes_only_sides( M::eModCnsSide | M::eModCnsCoef ) ) &&
	 ( ! M::changes_only_sides( M::eModPhys | M::eEffAny ) ) );
 assert( M::is_physical( M::eModPhys | M::eModCnsSide ) &&
	 ( ! M::is_abstract( M::eModPhys | M::eModCnsSide ) ) );

 // a region that only shrinks keeps the lower bounds, one that only grows
 // the upper bounds, and an objective that may decrease keeps neither of
 // the lower bounds
 const auto shrink = M::ModConcern( M::eModCnsSet | M::eRegnShrink );
 const auto grow = M::ModConcern( M::eModVarSet | M::eRegnGrow );
 const auto obj = M::ModConcern( M::eModObj | M::eObjUp | M::eObjDown );
 assert( M::lower_bound_stays_valid( shrink ) &&
	 ( ! M::upper_bound_stays_valid( shrink ) ) &&
	 ( ! M::solution_stays_feasible( shrink ) ) &&
	 M::may_shrink_region( shrink ) && ( ! M::may_grow_region( shrink ) ) );
 assert( M::upper_bound_stays_valid( grow ) &&
	 ( ! M::lower_bound_stays_valid( grow ) ) &&
	 M::solution_stays_feasible( grow ) );
 assert( M::solution_stays_feasible( obj ) &&
	 ( ! M::lower_bound_stays_valid( obj ) ) &&
	 ( ! M::upper_bound_stays_valid( obj ) ) );
 assert( ! M::lower_bound_stays_valid( M::eModAll ) );
 assert( ! M::upper_bound_stays_valid( M::eModAll ) );
 }

/*--------------------------------------------------------------------------*/
/* What a Block passes to a Solver that reads only some kinds of
 * Modification [see Solver::concerned_by()], and what concerned() and
 * anyone_there_for() say on a Block, on its son, and with and without
 * Solver. */

static void test_concerned( void )
{
 using M = Modification;
 Rig r;
 auto x = new ColVariable;
 r.block->add_static_variable( *x , "x" );
 auto rows = new std::list< FRowConstraint >;
 r.block->add_dynamic_constraint( *rows , "c" );
 auto son = new AbstractBlock( r.block );
 r.block->add_nested_Block( son );

 // the FakeSolver of the Rig reads all the kinds, the other only the data
 // of the Variable
 auto reads = new ReadsSolver( M::eModVarData );
 r.block->register_Solver( reads );
 assert( r.block->concerned() == M::eModAnything );
 assert( son->concerned() == M::eModAnything );

 // fixing a Variable reaches both, adding a Constraint only the first
 r.clear();
 reads->get_Modification_list().clear();
 x->is_fixed( true , eModBlck );
 assert( r.got().size() == 1 );
 assert( reads->get_Modification_list().size() == 1 );

 // making it integer reaches only the first: the other is the Solver of a
 // continuous relaxation, which reads the data of the Variable but not their
 // integrality
 r.clear();
 reads->get_Modification_list().clear();
 x->is_integer( true , eModBlck );
 assert( r.got().size() == 1 );
 assert( reads->get_Modification_list().empty() );
 x->is_integer( false , eNoMod );

 r.clear();
 reads->get_Modification_list().clear();
 {
  std::list< FRowConstraint > n( 1 );
  add_d( r.block , *rows , n , eModBlck );
  }
 assert( r.got().size() == 1 );
 assert( reads->get_Modification_list().empty() );

 // a GroupModification reaches whoever reads one of its sub-Modification
 r.clear();
 reads->get_Modification_list().clear();
 {
  auto chnl = r.block->open_channel();
  x->is_fixed( false , Observer::make_par( eModBlck , chnl ) );
  std::list< FRowConstraint > n( 1 );
  add_d( r.block , *rows , n , Observer::make_par( eModBlck , chnl ) );
  r.block->close_channel( chnl );
  }
 assert( r.got().size() == 1 );
 assert( reads->get_Modification_list().size() == 1 );

 // without the FakeSolver of the Rig, only the data of the Variable are read,
 // by the Block and by its son alike
 r.listen( false );
 assert( r.block->concerned() == M::eModVarData );
 assert( son->concerned() == M::eModVarData );
 assert( r.block->anyone_there_for( M::eModVarData ) );
 assert( ! r.block->anyone_there_for( M::eModCnsSet ) );
 assert( son->anyone_there_for( M::eModVarData | M::eModCnsSet ) );
 assert( r.block->issue_mod( eNoBlck , M::eModVarData ) );
 assert( ! r.block->issue_mod( eNoBlck , M::eModCnsSet ) );
 assert( r.block->issue_mod( eModBlck , M::eModCnsSet ) );
 assert( ! r.block->issue_pmod( eNoBlck , M::eModPhys ) );

 // with no Solver at all, nobody reads anything
 r.block->unregister_Solver( reads , true );
 assert( r.block->concerned() == 0 );
 assert( son->concerned() == 0 );
 assert( ! son->anyone_there_for( M::eModAnything ) );
 r.listen( true );
 }

/*--------------------------------------------------------------------------*/
/* What Solution::adapt() answers: a ColVariableSolution drops the value of
 * a removed dynamic Variable and is unchanged by what does not touch what it
 * holds, a NModification invalidates it, a GroupModification combines the
 * answers, and a :Solution that says nothing of itself cannot be adapted to
 * a removal. */

static void test_adapt( void )
{
 Rig r;
 auto x = new ColVariable;
 r.block->add_static_variable( *x , "x" );
 auto vars = new std::list< ColVariable >;
 r.block->add_dynamic_variable( *vars , "y" );
 auto rows = new std::list< FRowConstraint >;
 r.block->add_dynamic_constraint( *rows , "c" );
 {
  std::list< ColVariable > n( 3 );
  add_d( r.block , *vars , n , eNoMod );
  std::list< FRowConstraint > m( 2 );
  add_d( r.block , *rows , m , eNoMod );
  }
 double val = 1;
 for( auto & v : *vars )
  v.set_value( val++ );

 ColVariableSolution sol;
 sol.read( r.block );
 std::vector< double > dropped;

 // the last Modification the Block issued, after the given call
 auto last = [ & ]( std::function< void( void ) > call ) {
  r.clear();
  call();
  assert( r.got().size() == 1 );
  return( r.got().front() );
  };

 // the dynamic Variable in the middle goes, and its value with it
 auto rmv = last( [ & ]() {
		   rmv_d( r.block , *vars , Range( 1 , 2 ) , eModBlck ); } );
 assert( sol.adapt( r.block , *rmv , dropped ) == Solution::kAdapted );
 assert( ( dropped.size() == 1 ) && ( dropped[ 0 ] == 2 ) );
 sol.write( r.block );
 assert( ( vars->front().get_value() == 1 ) &&
	 ( vars->back().get_value() == 3 ) );

 // a Constraint that goes, the fixing of a Variable and a physical
 // Modification do not touch what a ColVariableSolution holds
 dropped.clear();
 auto rmc = last( [ & ]() {
		   rmv_d( r.block , *rows , Range( 0 , 1 ) , eModBlck ); } );
 assert( sol.adapt( r.block , *rmc , dropped ) == Solution::kUnchanged );
 auto fix = last( [ & ]() { x->is_fixed( true , eModBlck ); } );
 assert( sol.adapt( r.block , *fix , dropped ) == Solution::kUnchanged );
 assert( sol.adapt( r.block , PhysMod( r.block ) , dropped ) ==
	 Solution::kUnchanged );
 assert( dropped.empty() );

 // after a NModification nothing of the Solution can be trusted
 assert( sol.adapt( r.block , NBModification( r.block ) , dropped ) ==
	 Solution::kInvalid );

 // a GroupModification: a removal and a fixing make an adapted Solution
 auto grp = last( [ & ]() {
		   auto chnl = r.block->open_channel();
		   x->is_fixed( false , Observer::make_par( eModBlck , chnl ) );
		   rmv_d( r.block , *vars , Range( 0 , 1 ) ,
			  Observer::make_par( eModBlck , chnl ) );
		   r.block->close_channel( chnl );
		   } );
 assert( sol.adapt( r.block , *grp , dropped ) == Solution::kAdapted );
 assert( ( dropped.size() == 1 ) && ( dropped[ 0 ] == 1 ) );

 // a :Solution that says nothing of itself cannot be adapted to a removal,
 // and is unchanged by the rest
 GenericSolution gen;
 dropped.clear();
 auto rmv2 = last( [ & ]() {
		    rmv_d( r.block , *vars , Range( 0 , 1 ) , eModBlck ); } );
 assert( gen.adapt( r.block , *rmv2 , dropped ) == Solution::kInvalid );
 assert( gen.adapt( r.block , *fix , dropped ) == Solution::kUnchanged );
 }

/*--------------------------------------------------------------------------*/

int main( void )
{
 test_ColVariable();
 test_active_list();
 test_RowConstraint();
 test_OneVarConstraint();
 test_Objective();
 test_LinearFunction();
 test_AbstractBlock();
 test_removed_Variable_in_stuff();
 test_dry_run();
 test_dry_run_LagBFunction();
 test_dry_run_BendersBFunction();
 test_dry_run_C05SumFunction();
 test_changes();
 test_concerned();
 test_adapt();

 std::cout << "Modification_test: all tests passed" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_Modification.cpp --------------------*/
/*--------------------------------------------------------------------------*/

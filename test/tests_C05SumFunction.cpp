/** @file
 * Unit tests for C05SumFunction as the Observer of its members.
 *
 * The sum takes the place of the Observer of each of its members, so that a
 * change of a member is a change of the sum: what the tests check is the
 * translation, i.e. that a Modification of a member is reported as one of
 * the sum with the same names and the same shift, that the original is still
 * handed to the Observer the member had, and that a GroupModification is
 * looked into rather than passed on untranslated, the translations of the
 * sub-Modification that say the same thing about the sum being merged into
 * one. The members are PolyhedralFunction, i.e. the very C05Function the
 * master problem of a bundle method is made of.
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
#include "C05SumFunction.h"
#include "ColVariable.h"
#include "PolyhedralFunction.h"

#include <iostream>
#include <memory>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Function::Index;
using Subset = Function::Subset;
using FunctionValue = Function::FunctionValue;

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an Observer that writes down what it is told, and nothing else
/** The Observer of the sum in the tests, and the one the members had before
 * the sum took their place: it keeps the Modification it receives, so that
 * what the sum says can be read off it. */

class Recorder : public Observer
{
 public:

 [[nodiscard]] Block * get_Block( void ) const override { return( nullptr ); }

 [[nodiscard]] bool anyone_there( void ) const override { return( true ); }

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  v_mod.push_back( std::move( mod ) );
  }

 ChnlName open_channel( ChnlName chnl = 0 ,
                        GroupModification * gmpmod = nullptr ) override {
  return( ++f_chnl );
  }

 void close_channel( ChnlName chnl , bool force = false ) override {}

 void set_default_channel( ChnlName chnl = 0 ) override {}

 void clear( void ) { v_mod.clear(); }

 [[nodiscard]] std::size_t size( void ) const { return( v_mod.size() ); }

 [[nodiscard]] const sp_Mod & operator[]( std::size_t i ) const {
  return( v_mod[ i ] );
  }

 private:

 std::vector< sp_Mod > v_mod;    ///< what the Observer has been told
 ChnlName f_chnl = 0;            ///< the last channel name given out
 };

/*--------------------------------------------------------------------------*/

/// a Block that hands to a given Observer whatever comes up to it
/** A GroupModification is built by the channel machinery of a Block, and
 * this is how the tests get one: a channel is opened on the child, the
 * Modification of the members are sent on it, and closing it delivers the
 * whole group to the father, which passes it on to the sum. */

class Relay : public AbstractBlock
{
 public:

 explicit Relay( Observer * to ) : AbstractBlock() , f_to( to ) {}

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override {
  f_to->add_Modification( std::move( mod ) , chnl );
  }

 [[nodiscard]] bool anyone_there( void ) const override { return( true ); }

 private:

 Observer * f_to;   ///< where the Modification go
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the type and the names of a Modification of the sum, if it is one

static bool is_of( const sp_Mod & mod , const Function * f , int type ,
                   const Subset & which )
{
 const auto cmod = std::dynamic_pointer_cast< C05FunctionMod >( mod );
 if( ( ! cmod ) || ( cmod->function() != f ) )
  return( false );
 return( ( cmod->type() == type ) && ( cmod->which() == which ) );
 }

/*--------------------------------------------------------------------------*/

/// one linearization changed in member h, as the member would say it

static sp_Mod changed( PolyhedralFunction & member , Index name ,
                       FunctionValue shift = 0 )
{
 return( std::make_shared< C05FunctionMod >( & member ,
                            C05FunctionMod::AllLinearizationChanged ,
                            Subset( { name } ) , shift ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( void )
{
 // the members: three PolyhedralFunction on the same two Variable - - - - - -

 std::vector< ColVariable > x( 2 );

 auto make = []( std::vector< ColVariable > & x , double c ) {
  PolyhedralFunction::VarVector vars( { & x[ 0 ] , & x[ 1 ] } );
  PolyhedralFunction::MultiVector A( { { c , 0 } , { 0 , c } } );
  PolyhedralFunction::RealVector b( { 0 , 0 } );
  return( new PolyhedralFunction( std::move( vars ) , std::move( A ) ,
                                  std::move( b ) ) );
  };

 std::vector< PolyhedralFunction * > members{ make( x , 1 ) , make( x , 2 ) ,
                                              make( x , 3 ) };

 // the Observer the members had before the sum took their place
 Recorder before;
 for( auto m : members )
  m->register_Observer( & before );

 std::vector< C05Function * > cmp( members.begin() , members.end() );
 C05SumFunction sum( std::move( cmp ) , true );

 Recorder after;             // the Observer of the sum
 sum.register_Observer( & after );

 // one Modification of one member is one Modification of the sum - - - - - -

 before.clear();
 after.clear();
 sum.add_Modification( changed( *members[ 0 ] , 1 ) );

 assert( after.size() == 1 );
 assert( is_of( after[ 0 ] , & sum ,
                C05FunctionMod::AllLinearizationChanged , Subset( { 1 } ) ) );
 assert( before.size() == 1 );   // the original still goes to the member's
 assert( std::dynamic_pointer_cast< FunctionMod >( before[ 0 ]
                                       )->function() == members[ 0 ] );

 // three of them, one per member, are three of the sum, since the sum is - -
 // told of them one at a time and cannot know that more are coming

 before.clear();
 after.clear();
 for( Index h = 0 ; h < 3 ; ++h )
  sum.add_Modification( changed( *members[ h ] , 1 ) );

 assert( after.size() == 3 );
 assert( before.size() == 3 );

 // the same three inside a GroupModification are one of the sum - - - - - - -

 before.clear();
 after.clear();
 {
  Relay relay( & sum );
  auto child = new AbstractBlock( & relay );
  relay.add_nested_Block( child );

  const auto chnl = child->open_channel();
  for( Index h = 0 ; h < 3 ; ++h )
   child->add_Modification( changed( *members[ h ] , 1 ) , chnl );
  child->close_channel( chnl );
  }

 assert( after.size() == 1 );
 assert( is_of( after[ 0 ] , & sum ,
                C05FunctionMod::AllLinearizationChanged , Subset( { 1 } ) ) );
 assert( before.size() == 1 );   // the group goes on whole, and once
 assert( std::dynamic_pointer_cast< GroupModification >( before[ 0 ] ) );

 // a group whose members do not say the same thing is not merged - - - - - -

 before.clear();
 after.clear();
 {
  Relay relay( & sum );
  auto child = new AbstractBlock( & relay );
  relay.add_nested_Block( child );

  const auto chnl = child->open_channel();
  child->add_Modification( changed( *members[ 0 ] , 1 ) , chnl );
  child->add_Modification( changed( *members[ 1 ] , 2 ) , chnl );
  child->close_channel( chnl );
  }

 assert( after.size() == 2 );
 assert( is_of( after[ 0 ] , & sum ,
                C05FunctionMod::AllLinearizationChanged , Subset( { 1 } ) ) );
 assert( is_of( after[ 1 ] , & sum ,
                C05FunctionMod::AllLinearizationChanged , Subset( { 2 } ) ) );

 // the shift of the merged one is the sum of theirs, a shift of one - - - - -
 // addend being a shift of the sum

 before.clear();
 after.clear();
 {
  Relay relay( & sum );
  auto child = new AbstractBlock( & relay );
  relay.add_nested_Block( child );

  const auto chnl = child->open_channel();
  for( Index h = 0 ; h < 3 ; ++h )
   child->add_Modification( changed( *members[ h ] , 1 , 2.5 ) , chnl );
  child->close_channel( chnl );
  }

 assert( after.size() == 1 );
 {
  const auto fmod = std::dynamic_pointer_cast< FunctionMod >( after[ 0 ] );
  assert( fmod );
  assert( fmod->shift() == 7.5 );
  }

 // the "active" Variable of the sum are the union of those of the members - -

 ColVariable y;                   // a Variable no member has
 assert( sum.get_num_active_var() == 2 );

 before.clear();
 after.clear();
 members[ 1 ]->add_variable( & y , PolyhedralFunction::RealVector( { 1 , 1 } ) );

 assert( sum.get_num_active_var() == 3 );      // the sum has gained it
 assert( sum.is_active( & y ) == 2 );
 {
  const auto vmod = std::dynamic_pointer_cast< FunctionModVarsAddd >(
                                                                after[ 0 ] );
  assert( vmod );
  assert( vmod->function() == & sum );
  assert( vmod->first() == 2 );
  assert( vmod->vars().size() == 1 );
  assert( vmod->vars().front() == & y );
  }

 // the same Variable added to another member is not new for the sum - - - - -

 before.clear();
 after.clear();
 members[ 2 ]->add_variable( & y , PolyhedralFunction::RealVector( { 1 , 1 } ) );

 assert( sum.get_num_active_var() == 3 );      // nothing has changed
 for( std::size_t i = 0 ; i < after.size() ; ++i )
  assert( ! std::dynamic_pointer_cast< FunctionModVars >( after[ i ] ) );

 // and it leaves the sum only when no member has it any more - - - - - - - -

 before.clear();
 after.clear();
 members[ 1 ]->remove_variable( members[ 1 ]->is_active( & y ) );

 assert( sum.get_num_active_var() == 3 );      // member 2 still has it
 for( std::size_t i = 0 ; i < after.size() ; ++i )
  assert( ! std::dynamic_pointer_cast< FunctionModVars >( after[ i ] ) );

 before.clear();
 after.clear();
 members[ 2 ]->remove_variable( members[ 2 ]->is_active( & y ) );

  assert( sum.get_num_active_var() == 2 );      // now nobody has it
 assert( sum.is_active( & y ) == Inf< Index >() );   // not "active" any more
 {
  bool said = false;
  for( std::size_t i = 0 ; i < after.size() ; ++i )
   if( const auto vmod = std::dynamic_pointer_cast< FunctionModVars >(
                                                              after[ i ] ) ) {
    assert( vmod->function() == & sum );
    assert( ! vmod->added() );
    assert( vmod->vars().size() == 1 );
    assert( vmod->vars().front() == & y );
    said = true;
    }
  assert( said );
  }

 // a Modification of something else is passed on as it is - - - - - - - - - -

 PolyhedralFunction stranger;
 before.clear();
 after.clear();
 sum.add_Modification( std::make_shared< FunctionMod >( & stranger , 1.0 ) );

 assert( after.size() == 1 );
 {
  const auto fmod = std::dynamic_pointer_cast< FunctionMod >( after[ 0 ] );
  assert( fmod );
  assert( fmod->function() == & stranger );   // not translated
  }
 assert( before.size() == 0 );

 std::cout << "All tests passed" << std::endl;

 for( auto m : members )
  m->register_Observer( nullptr );

 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File tests_C05SumFunction.cpp ------------------*/
/*--------------------------------------------------------------------------*/

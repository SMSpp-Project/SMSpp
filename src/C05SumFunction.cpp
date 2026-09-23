/*--------------------------------------------------------------------------*/
/*----------------------- File C05SumFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the C05SumFunction class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05SumFunction.h"

#include "Solver.h"

#include <algorithm>

#include <cmath>

#include <future>

#include <limits>

#include <numeric>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

// the compensated sum of the values of a group, an infinite one absorbing the
// others: the members are summed with the Kahan-Babuska correction, so that
// the error does not grow with their number, which matters because the value
// of a group is often much smaller than those of its members

class group_sum
{
 public:

 using FunctionValue = Function::FunctionValue;

 void operator+=( FunctionValue v ) {
  static constexpr auto INF = Inf< FunctionValue >();
  if( ( v == INF ) || ( v == -INF ) ) {
   if( f_inf && ( f_inf != v ) )
    f_nan = true;
   f_inf = v;
   return;
   }

  const auto t = f_sum + v;
  f_c += ( std::abs( f_sum ) >= std::abs( v ) ) ? ( f_sum - t ) + v
                                                : ( v - t ) + f_sum;
  f_sum = t;
  }

 operator FunctionValue( void ) const {
  if( f_nan )
   return( std::nan( "" ) );
  return( f_inf ? f_inf : f_sum + f_c );
  }

 private:

 FunctionValue f_sum = 0;   ///< the sum so far
 FunctionValue f_c = 0;     ///< what the sum has lost so far
 FunctionValue f_inf = 0;   ///< the infinite value, if any
 bool f_nan = false;        ///< true if both infinities are there
 };

}  // end( anonymous namespace )

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS OF C05SumFunction ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

C05SumFunction::C05SumFunction( std::vector< C05Function * > && members ,
                                std::vector< ColVariable * > && vars ,
                                bool observe )
 : C05Function() , v_members( std::move( members ) ) ,
   v_vars( std::move( vars ) )
{
 if( v_members.empty() )
  throw( std::invalid_argument( "C05SumFunction: the sum has no members" ) );

 // when the "active" Variable are not given, they are the union of those of
 // the members, in the order in which the members give them
 if( v_vars.empty() )
  for( auto m : v_members ) {
   const auto n = m->get_num_active_var();
   for( Index i = 0 ; i < n ; ++i ) {
    const auto var = static_cast< ColVariable * >( m->get_active_var( i ) );
    if( std::find( v_vars.begin() , v_vars.end() , var ) == v_vars.end() )
     v_vars.push_back( var );
    }
   }

 f_var2idx.reserve( v_vars.size() );
 for( Index i = 0 ; i < v_vars.size() ; ++i )
  f_var2idx.emplace( v_vars[ i ] , i );

 // the vectors of entries are drawn from a generator of the group's own, so
 // that a run is repeatable: the seed is the size of the group, which is
 // what tells one group of a run from another
 f_rnd.seed( std::mt19937::result_type( v_members.size() ) );

 f_convex = v_members.front()->is_convex();
 f_concave = v_members.front()->is_concave();

 v_map.resize( v_members.size() );
 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  auto m = v_members[ h ];
  if( ( m->is_convex() != f_convex ) || ( m->is_concave() != f_concave ) )
   throw( std::invalid_argument( "C05SumFunction: the members are not all "
                                 "convex or all concave" ) );

  const auto n = m->get_num_active_var();
  v_map[ h ].resize( n );
  for( Index i = 0 ; i < n ; ++i ) {
   const auto it = f_var2idx.find( m->get_active_var( i ) );
   if( it == f_var2idx.end() )
    throw( std::invalid_argument( "C05SumFunction: a Variable of a member "
                                  "is not among those of the group" ) );
   v_map[ h ][ i ] = it->second;
   }
  }

 v_part.assign( v_members.size() , 0 );
 v_last.assign( v_members.size() ,
                std::numeric_limits< FunctionValue >::quiet_NaN() );
 v_flat.resize( v_members.size() );

 if( observe )
  steal_the_Observers();
 }

/*--------------------------------------------------------------------------*/

C05SumFunction::C05SumFunction( std::vector< C05Function * > && members ,
                                bool observe )
 : C05SumFunction( std::move( members ) , std::vector< ColVariable * >() ,
                   observe )
{}

/*--------------------------------------------------------------------------*/

C05SumFunction::~C05SumFunction()
{
 // the members are given back the Observer they had, if it was taken
 for( Index h = 0 ; h < v_prev_obs.size() ; ++h )
  v_members[ h ]->register_Observer( v_prev_obs[ h ] );
 }

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR BEING THE Observer OF THE MEMBERS --------------*/
/*--------------------------------------------------------------------------*/

void C05SumFunction::steal_the_Observers( void )
{
 v_prev_obs.resize( v_members.size() );
 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  v_prev_obs[ h ] = v_members[ h ]->get_Observer();
  v_members[ h ]->register_Observer( this );
  }
 }

/*--------------------------------------------------------------------------*/

C05SumFunction::Index C05SumFunction::member_of( const sp_Mod & mod ) const
{
 /* A FunctionModVars is not a FunctionMod, the two hierarchies being apart
  * although both say which Function they come from: which one it is has to
  * be asked of both. */

 const Function * f = nullptr;
 if( const auto fmod = std::dynamic_pointer_cast< FunctionMod >( mod ) )
  f = fmod->function();
 else
  if( const auto vmod = std::dynamic_pointer_cast< FunctionModVars >( mod ) )
   f = vmod->function();

 if( f )
  for( Index k = 0 ; k < v_members.size() ; ++k )
   if( v_members[ k ] == f )
    return( k );

 return( Inf< Index >() );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::add_Modification( sp_Mod mod , ChnlName chnl )
{
 /* A GroupModification is looked into: the Observer of a Function is
  * entitled to receive one, as FRealObjective (the Observer whose place the
  * sum takes) does, and what it says about the sum is what its
  * sub-Modification say together. */

 if( const auto gmod = std::dynamic_pointer_cast< GroupModification >( mod ) ) {
  group_Modification( gmod , chnl );
  return;
  }

 // which member is speaking: a Modification of anything else is passed on
 // as it is, the sum having nothing to say about it
 const Index h = member_of( mod );

 if( h == Inf< Index >() ) {
  if( f_Observer )
   f_Observer->add_Modification( std::move( mod ) , chnl );
  return;
  }

 /* While the sum is changing the global pools of the members itself, what
  * arrives from a member is the echo of that, and it is only passed on to
  * the Observer the member had: the sum reports for itself. */

 if( f_own_op ) {
  if( v_prev_obs[ h ] )
   v_prev_obs[ h ]->add_Modification( std::move( mod ) , chnl );
  return;
  }

 member_Modification( mod , v_prev_obs[ h ] , chnl );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::issue_pool_Modification( int type , Subset && which ,
                                              ModParam issueMod )
{
 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< C05FunctionMod >( this ,
                                 type , std::move( which ) , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::member_Modification( const sp_Mod & mod ,
                                          Observer * previous ,
                                          ChnlName chnl )
{
 translate_Modification( mod , chnl );

 // last, the member speaks to the Observer it had, which is what the Solver
 // attached to its Block are waiting for
 if( previous )
  previous->add_Modification( sp_Mod( mod ) , chnl );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::group_Modification(
                       const std::shared_ptr< GroupModification > & gmod ,
                       ChnlName chnl )
{
 /* The sub-Modification that belong to the members are translated, the
  * others are passed on as they are; the translations are held and merged,
  * so that a group in which every member changes in the same way becomes
  * one Modification of the sum rather than one per member. */

 std::vector< Observer * > previous;  // the Observer of the members involved

 {
  bunching mine( *this , chnl );

  for( const auto & sub : gmod->sub_Modifications() ) {
   if( const auto sgmod =
       std::dynamic_pointer_cast< GroupModification >( sub ) ) {
    group_Modification( sgmod , chnl );   // a nested group is looked into
    continue;
    }

   const Index h = member_of( sub );
   if( h == Inf< Index >() ) {            // not a member: not ours to say
    if( f_Observer )
     f_Observer->add_Modification( sub , chnl );
    continue;
    }

   if( std::find( previous.begin() , previous.end() , v_prev_obs[ h ] ) ==
       previous.end() )
    previous.push_back( v_prev_obs[ h ] );

   // while the sum is changing the members itself, what they say is the
   // echo of that, and the sum reports for itself
   if( ! f_own_op )
    translate_Modification( sub , chnl );
   }
  }

 // the group goes on, whole, to the Observer the members had: it is one
 // change of theirs, and the Solver attached to their Block see it as such
 for( auto obs : previous )
  if( obs )
   obs->add_Modification( gmod , chnl );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::emit( sp_Mod out , ChnlName chnl )
{
 if( f_bunching )
  v_pending.push_back( std::move( out ) );
 else
  if( f_Observer )
   f_Observer->add_Modification( std::move( out ) , chnl );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::flush_pending( ChnlName chnl )
{
 if( ! f_Observer ) {
  v_pending.clear();
  return;
  }

 /* Two translations say the same thing about the sum if they are the same
  * kind of Modification about the same linearizations: they are then merged
  * into one, whose shift is the sum of theirs, a shift of one addend being
  * a shift of the sum, and which concerns the Block if either does. */

 auto same = []( const sp_Mod & a , const sp_Mod & b ) -> bool {
  const auto ca = std::dynamic_pointer_cast< C05FunctionMod >( a );
  const auto cb = std::dynamic_pointer_cast< C05FunctionMod >( b );
  if( bool( ca ) != bool( cb ) )
   return( false );
  if( ! ca )
   return( true );          // both plain FunctionMod: only the shift
  return( ( ca->type() == cb->type() ) && ( ca->which() == cb->which() ) );
  };

 auto add_shift = []( FunctionValue a , FunctionValue b ) -> FunctionValue {
  if( ( a == FunctionMod::INFshift ) || ( b == FunctionMod::INFshift ) )
   return( FunctionMod::INFshift );  // unpredictable stays unpredictable
  return( a + b );          // NaN, i.e. "no shift known", propagates
  };

 while( ! v_pending.empty() ) {
  auto out = v_pending.front();
  v_pending.pop_front();
  auto fout = std::static_pointer_cast< FunctionMod >( out );
  FunctionValue shift = fout->shift();
  bool cB = fout->concerns_Block();
  bool merged = false;

  while( ( ! v_pending.empty() ) && same( out , v_pending.front() ) ) {
   auto nxt = std::static_pointer_cast< FunctionMod >( v_pending.front() );
   shift = add_shift( shift , nxt->shift() );
   cB = cB || nxt->concerns_Block();
   v_pending.pop_front();
   merged = true;
   }

  if( merged ) {     // the merged one is built anew, the held ones are gone
   if( const auto cout = std::dynamic_pointer_cast< C05FunctionMod >( out ) )
    out = std::make_shared< C05FunctionMod >( this , cout->type() ,
                                              Subset( cout->which() ) ,
                                              shift , cB );
   else
    out = std::make_shared< FunctionMod >( this , shift , cB );
   }

  f_Observer->add_Modification( std::move( out ) , chnl );
  }
 }


/*--------------------------------------------------------------------------*/

void C05SumFunction::rebuild_map( Index h )
{
 auto m = v_members[ h ];
 const auto n = m->get_num_active_var();
 v_map[ h ].resize( n );
 for( Index i = 0 ; i < n ; ++i ) {
  const auto it = f_var2idx.find( m->get_active_var( i ) );
  if( it == f_var2idx.end() )
   throw( std::logic_error( "C05SumFunction::rebuild_map: a Variable of a "
                            "member is not among those of the group" ) );
  v_map[ h ][ i ] = it->second;
  }
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::member_Variables_changed(
                        const std::shared_ptr< FunctionModVars > & vmod ,
                        ChnlName chnl )
{
 const auto h = member_of( vmod );
 auto member = v_members[ h ];

 /* The "active" Variable of the sum are the union of those of the members,
  * so that a Variable added to a member is one of the sum only if no other
  * member had it, and one removed from a member leaves the sum only if no
  * other member still has it. The linearizations of the sum are combined
  * through v_map at each request, and not stored, so that rebuilding the
  * map is all there is to do on this side. */

 Vec_p_Var changed;   // the Variable that are added to, or leave, the sum
 Subset where;        // where they were, for a removal

 if( vmod->added() ) {
  const Index first = v_vars.size();

  for( auto v : vmod->vars() ) {
   auto var = static_cast< ColVariable * >( v );
   if( f_var2idx.find( var ) != f_var2idx.end() )
    continue;                      // some other member already had it
   f_var2idx.emplace( var , v_vars.size() );
   v_vars.push_back( var );
   changed.push_back( var );
   }

  rebuild_map( h );

  if( changed.empty() )            // nothing new for the sum
   return;

  if( f_Observer && anyone_there() ) {
   sp_Mod out;
      if( std::dynamic_pointer_cast< C05FunctionModVarsAddd >( vmod ) )
    out = std::make_shared< C05FunctionModVarsAddd >( this ,
                                     std::move( changed ) , first ,
                                     vmod->shift() , vmod->concerns_Block() );
   else
    out = std::make_shared< FunctionModVarsAddd >( this ,
                                     std::move( changed ) , first ,
                                     vmod->shift() , vmod->concerns_Block() );
   emit( std::move( out ) , chnl );
   }

  return;
  }

 /* A removal: a Variable leaves the sum only if no member still has it,
  * which is asked of the members as they are now, the one that has changed
  * included. The Variable that leave are taken out of v_vars, whence the
  * positions of those after them change, and every map is rebuilt. */

 for( auto v : vmod->vars() ) {
  auto var = static_cast< ColVariable * >( v );
  const auto it = f_var2idx.find( var );
  if( it == f_var2idx.end() )
   continue;                       // it was not one of the sum anyway

  bool someone = false;
  for( Index k = 0 ; ( k < v_members.size() ) && ( ! someone ) ; ++k )
   if( v_members[ k ]->is_active( var ) <
       v_members[ k ]->get_num_active_var() )
    someone = true;

  if( someone )                    // some member still has it
   continue;

  changed.push_back( var );
  where.push_back( it->second );
  }

 if( changed.empty() ) {           // no Variable leaves the sum
  rebuild_map( h );
  return;
  }

 // the positions are given in increasing order, as a Subset is expected to
 Subset ord( where.size() );
 std::iota( ord.begin() , ord.end() , 0 );
 std::sort( ord.begin() , ord.end() ,
            [ & where ]( Index a , Index b ) {
             return( where[ a ] < where[ b ] );
             } );

 Vec_p_Var sorted( changed.size() );
 Subset positions( where.size() );
 for( Index i = 0 ; i < ord.size() ; ++i ) {
  sorted[ i ] = changed[ ord[ i ] ];
  positions[ i ] = where[ ord[ i ] ];
  }

 // take them out, keeping the order of the ones that stay
 auto gone = [ & sorted ]( const ColVariable * v ) {
  return( std::find( sorted.begin() , sorted.end() , v ) != sorted.end() );
  };
 v_vars.erase( std::remove_if( v_vars.begin() , v_vars.end() , gone ) ,
               v_vars.end() );

 f_var2idx.clear();
 f_var2idx.reserve( v_vars.size() );
 for( Index i = 0 ; i < v_vars.size() ; ++i )
  f_var2idx.emplace( v_vars[ i ] , i );

 for( Index k = 0 ; k < v_members.size() ; ++k )
  rebuild_map( k );

 if( f_Observer && anyone_there() ) {
  sp_Mod out;
    const bool strong =
   std::dynamic_pointer_cast< C05FunctionModVarsRngd >( vmod ) ||
   std::dynamic_pointer_cast< C05FunctionModVarsSbst >( vmod );
  // a run of consecutive positions is a range, which is cheaper to read
  const bool range = ( positions.back() - positions.front() + 1 ) ==
                     positions.size();
  if( range ) {
   Function::Range rng( positions.front() , positions.back() + 1 );
   if( strong )
    out = std::make_shared< C05FunctionModVarsRngd >( this ,
                                     std::move( sorted ) , rng ,
                                     vmod->shift() , vmod->concerns_Block() );
   else
    out = std::make_shared< FunctionModVarsRngd >( this ,
                                     std::move( sorted ) , rng ,
                                     vmod->shift() , vmod->concerns_Block() );
   }
  else
   if( strong )
    out = std::make_shared< C05FunctionModVarsSbst >( this ,
                                     std::move( sorted ) ,
                                     std::move( positions ) , true ,
                                     vmod->shift() , vmod->concerns_Block() );
   else
    out = std::make_shared< FunctionModVarsSbst >( this ,
                                     std::move( sorted ) ,
                                     std::move( positions ) , true ,
                                     vmod->shift() , vmod->concerns_Block() );

  emit( std::move( out ) , chnl );
  }
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::translate_Modification( const sp_Mod & mod ,
                                             ChnlName chnl )
{
 /* A Variable added to or removed from a member changes the "active"
  * Variable of the sum, which are their union, and the map between the two:
  * this is dealt with apart, since what the sum has to say about it is not
  * a Modification of its linearizations but one of its Variable. */

 if( const auto vmod = std::dynamic_pointer_cast< FunctionModVars >( mod ) ) {
  member_Variables_changed( vmod , chnl );
  return;
  }

 const auto fmod = std::static_pointer_cast< FunctionMod >( mod );
 const auto member = static_cast< C05Function * >( fmod->function() );

 /* The global pool of the sum is kept to what the Modification of the
  * members say it is: a linearization that a member no longer has is no
  * longer a linearization of the sum, and a member that has changed may have
  * changed its global bound, which makes stale the horizontal linearizations
  * the sum records for it. Both are dealt with as removed from the global
  * pool of the sum, and reported as such. */

 Subset gone;
 if( const auto cmod = std::dynamic_pointer_cast< C05FunctionMod >( fmod ) )
  if( cmod->type() == C05FunctionMod::GlobalPoolRemoved ) {
   gone = cmod->which();
   delete_linearizations( Subset( gone ) , true , eNoMod );
   }

 auto stale = remove_stale_flat_linearizations( member );

 /* What the sum tells its own Observer. A Modification of a member is a
  * Modification of the sum with the same names, the linearization called i
  * in the sum being the sum of those called i in the members, and with the
  * same shift, a shift of one addend being a shift of the sum. The types
  * that carry the changed coefficients with them are reported as
  * AllLinearizationChanged with no names, i.e. "check them all", which is
  * coarser but true. */

 if( f_Observer && anyone_there() ) {
  sp_Mod out;
  if( const auto cmod = std::dynamic_pointer_cast< C05FunctionMod >( fmod ) ) {
   if( std::dynamic_pointer_cast< C05FunctionModRngd >( fmod ) ||
       std::dynamic_pointer_cast< C05FunctionModSbst >( fmod ) )
    out = std::make_shared< C05FunctionMod >( this ,
                                 C05FunctionMod::AllLinearizationChanged ,
                                 Subset() , cmod->shift() ,
                                 cmod->concerns_Block() );
   else
    out = std::make_shared< C05FunctionMod >( this , cmod->type() ,
                                              Subset( cmod->which() ) ,
                                              cmod->shift() ,
                                              cmod->concerns_Block() );
   }
  else
   if( std::dynamic_pointer_cast< C05FunctionModLin >( fmod ) )
    out = std::make_shared< C05FunctionMod >( this ,
                                 C05FunctionMod::AllLinearizationChanged ,
                                 Subset() , fmod->shift() ,
                                 fmod->concerns_Block() );
   else
    out = std::make_shared< FunctionMod >( this , fmod->shift() ,
                                           fmod->concerns_Block() );

    emit( std::move( out ) , chnl );

  if( ! stale.empty() )
   emit( std::make_shared< C05FunctionMod >( this ,
                                 C05FunctionMod::GlobalPoolRemoved ,
                                 std::move( stale ) , 0 ) , chnl );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------- METHODS FOR HANDLING THE PARAMETERS ----------------------*/
/*--------------------------------------------------------------------------*/

void C05SumFunction::set_par( idx_type par , int value )
{
 // the size of the global pool is the range of names the members are asked
 // about when a combination of what they hold is looked for
 if( par == intGPMaxSz )
  f_gp_size = value > 0 ? Index( value ) : 0;

 /* intMaxThread is handed down as everything else is: the threads a member
  * spends on itself are its own business. How many members the group
  * evaluates at once is a different number, which only the solver driving
  * the group knows, and it says so through set_members_at_once(). */

 for( auto m : v_members )
  m->set_par( par , value );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::set_par( idx_type par , double value )
{
 // a target on the sum cannot be split among the members
 if( ( par == dblUpCutOff ) || ( par == dblLwCutOff ) )
  return;

 // the time is not given to every member, or the group would be allowed
 // as many times it as it has members: it is kept here and handed down one
 // member at a time, each of them getting what the previous ones have left
 if( par == dblMaxTime ) {
  f_max_time = value;
  return;
  }

 /* The errors of the members add up in the group, so each of them can only
  * be allowed the share of an absolute accuracy that is its own; the share
  * is not the same for everybody, since dividing by the number of members
  * assumes that they are worth the same, which they need not be. The
  * accuracy is therefore recorded here and shared out at each compute(),
  * where what each member was worth at its last evaluation is known. The
  * relative accuracies are passed on as they are, the value of a member
  * having nothing to do with that of the group. */

 if( ( par == dblAbsAcc ) || ( par == dblAAccLin ) || ( par == dblAAccMlt ) ) {
  f_abs_par[ par ] = value;
  return;
  }

 for( auto m : v_members )
  m->set_par( par , value );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::set_par( idx_type par , std::string && value )
{
 for( auto m : v_members )
  m->set_par( par , std::string( value ) );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::set_ComputeConfig( const ComputeConfig * scfg )
{
 for( auto m : v_members )
  m->set_ComputeConfig( scfg );
 }

/*--------------------------------------------------------------------------*/
/*----------- METHODS FOR HANDLING THE "ACTIVE" Variable ------------------*/
/*--------------------------------------------------------------------------*/

void C05SumFunction::remove_variable( Index i , ModParam issueMod )
{
 throw( std::logic_error( "C05SumFunction::remove_variable: the Variable "
                          "of a group cannot be changed" ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS FOR COMPUTING THE FUNCTION ------------------*/
/*--------------------------------------------------------------------------*/

std::vector< double > C05SumFunction::accuracy_shares( void )
 const
{
 /* An absolute accuracy asked of the group has to be split among its
  * members, whose errors add up; splitting it in equal parts assumes that
  * the members are worth the same, and a member worth 1e9 next to one worth
  * 1 would be asked for an accuracy it has no way of delivering while the
  * small one is asked for far more than it is worth. The share is therefore
  * proportional to what the member was worth at its last evaluation, and
  * equal parts are used until there is one, or when what the members are
  * worth says nothing (one of them is infinite, or they are all worth
  * nothing). No member is left without a share, whatever it is worth: a
  * share of zero would ask it for an exact answer. */

 const auto k = v_members.size();
 std::vector< double > shares( k , 1.0 / double( k ) );

 if( v_last.size() != k )
  return( shares );

 double total = 0;
 for( const auto v : v_last ) {
  if( ! std::isfinite( v ) )
   return( shares );
  total += std::abs( v );
  }

 if( ! ( total > 0 ) )
  return( shares );

 const double least = 1.0 / ( 10.0 * double( k ) );  // the floor of a share
 double sum = 0;
 for( Index h = 0 ; h < k ; ++h ) {
  shares[ h ] = std::max( std::abs( v_last[ h ] ) / total , least );
  sum += shares[ h ];
  }

 for( auto & share : shares )   // the floors have to be paid for
  share /= sum;

 return( shares );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::update_member_time(
                const std::chrono::system_clock::time_point & from ,
                Index howmany )
{
 if( ! howmany )
  return;

 const auto now = std::chrono::system_clock::now();
 const double each = std::chrono::duration< double >( now - from ).count() /
                     double( howmany );

 // an exponential average, so that a group whose members become expensive
 // (or cheap) is followed rather than remembered
 f_member_time = f_member_time > 0 ? 0.7 * f_member_time + 0.3 * each : each;
 }

/*--------------------------------------------------------------------------*/

int C05SumFunction::compute_parallel( bool changedvars ,
                                       const std::vector< double > & shares )
{
 /* The members are evaluated as many at a time as the threads the group has
    * been allowed to spend. The time cannot be handed down one member at a
  * time here,
  * they being computed together: each of them is given all of what the group
  * has left, which is the only thing the group has to respect anyway, and
  * the ones that run together share it rather than adding up to it. */

 const Index k = v_members.size();
 const Index at_once = std::min( Index( f_max_thread ) , k );

 int status = kOK;
 auto start = std::chrono::system_clock::now();

 for( Index from = 0 ; from < k ; from += at_once ) {
  const Index to = std::min( from + at_once , k );

  if( f_max_time < Inf< double >() ) {
   const auto now = std::chrono::system_clock::now();
   const auto left = f_max_time -
                     std::chrono::duration< double >( now - start ).count();
   if( left <= 0 )     // the time is up: what is left is not computed
    return( Solver::kStopTime );
   for( Index h = from ; h < to ; ++h )
    v_members[ h ]->set_par( dblMaxTime , left );
   }

    /* The members but the first are handed to whoever runs them, and the
   * first is run here: the thread that would otherwise only wait for them
   * does one of them, which is one thread less to find and, when the
   * threads come from a pool shared with the solver driving the group, is
   * what keeps a thread of that pool from being held doing nothing. */

  std::vector< std::future< int > > running;
  running.reserve( to - from - 1 );
  for( Index h = from ; h < to ; ++h ) {
   for( const auto & [ par , value ] : f_abs_par )
    v_members[ h ]->set_par( par , value * shares[ h ] );
   v_members[ h ]->set_par( intMaxThread , 1 );
   ++f_member_evals;
   if( h == from )
    continue;
   running.push_back( f_submit ? f_submit( v_members[ h ] , changedvars )
                               : v_members[ h ]->compute_async( changedvars )
                      );
   }

  const int first = v_members[ from ]->compute( changedvars );

  // whatever happens, every member that has been started is waited for:
  // returning while one of them is still writing its own Block would leave
  // the group reading a value that is being changed under it
  int bad = kOK;
  for( Index h = from ; h < to ; ++h ) {
   const int s = ( h == from ) ? first : running[ h - from - 1 ].get();
   if( ( s <= kUnEval ) || ( ( s >= kError ) && ( s != Solver::kLowPrecision ) ) )
    bad = s;
   else if( s == Solver::kLowPrecision )
    status = Solver::kLowPrecision;
   else if( ( status == kOK ) && ( s != kOK ) )
    status = s;
   v_last[ h ] = v_members[ h ]->get_value();
   }

  if( bad != kOK )   // an error ends it all, but only once everybody is in
   return( bad );
  }

 // what the members have taken together is not what one of them takes, but
 // the threads have been spent all the same: the average is refreshed on
 // the wall-clock time divided by how many have run at once
 update_member_time( start , ( k + at_once - 1 ) / at_once );

 return( status );
 }

/*--------------------------------------------------------------------------*/

int C05SumFunction::compute( bool changedvars )
{
 const bool timed = ( f_max_time < Inf< double >() );
 auto left = f_max_time;
 auto start = std::chrono::system_clock::now();

 // the share of the absolute accuracies each member is given
 const auto shares = accuracy_shares();

  /* The members are evaluated together only if they are worth a thread each:
  * spawning one and waiting for it costs of the order of 100 microseconds,
  * so a group whose members are answered by a dynamic program in ten of
  * them would spend on the threads several times what the evaluation takes.
  * What a member has cost so far is therefore what decides, and since it is
  * not known before the first evaluation, that one is done one member at a
  * time. When the threads come from a pool [see set_submitter()] handing a
  * member over is a queue and not a thread, which costs two orders of
  * magnitude less, and the members that are worth it are accordingly
  * cheaper ones. */

 const double worth = f_submit ? 1e-5 : 1e-3;

 if( ( f_max_thread > 1 ) && ( f_member_time > worth ) )
  return( compute_parallel( changedvars , shares ) );

 const auto t0 = std::chrono::system_clock::now();

 int status = kOK;
 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  auto m = v_members[ h ];
  if( timed ) {
   if( left <= 0 )  // the time is up: what is left is not computed
    return( Solver::kStopTime );
   m->set_par( dblMaxTime , left );
   }

  for( const auto & [ par , value ] : f_abs_par )
   m->set_par( par , value * shares[ h ] );

  ++f_member_evals;
  const int s = m->compute( changedvars );

  // kLowPrecision is not an error: it says that the member is worth what it
  // returns only up to the accuracy it was given, which is what an inexact
  // oracle is, and the group is inexact with it; stopping here would leave
  // the other members unevaluated and the group without a value
  if( ( s <= kUnEval ) || ( ( s >= kError ) && ( s != Solver::kLowPrecision ) ) )
   return( s );                    // an error ends it all
  if( s == Solver::kLowPrecision )  // an inexact member makes the sum inexact
   status = Solver::kLowPrecision;
  else if( status == kOK )
   status = s;

  v_last[ h ] = m->get_value();

  if( timed ) {  // what this member has taken is not there for the others
   const auto now = std::chrono::system_clock::now();
   left = f_max_time -
          std::chrono::duration< double >( now - start ).count();
   }
  }

 update_member_time( t0 , v_members.size() );

 return( status );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_value( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_value();
 return( v );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_lower_estimate( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_lower_estimate();
 return( v );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_upper_estimate( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_upper_estimate();
 return( v );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_global_lower_bound( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_global_lower_bound();
 return( v );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_global_upper_bound( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_global_upper_bound();
 return( v );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_Lipschitz_constant( void )
{
 group_sum v;
 for( auto m : v_members )
  v += m->get_Lipschitz_constant();
 return( v );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR LINEARIZATIONS ------------------------*/
/*--------------------------------------------------------------------------*/

bool C05SumFunction::random_combination( void )
{
 /* When the members have nothing new to give, what they have given before is
  * still there: each of them holds in its global pool the linearizations of
  * the entries the group has stored, and a vector that takes one of them from
  * each member is again a linearization of the group, a sum of lower models
  * of the members being a lower model of their sum. The vectors are drawn at
  * random, without giving the same one twice since the last evaluation, and
  * the vertical entries are left out of the draw, a constraint of the domain
  * not being a model of the function. */

 const auto k = v_members.size();
 if( ! k )
  return( false );

 // the size of the pool is asked of the members when it has not been set
 // through the group, which happens when they had a large enough one of
 // their own and nobody had to change it
 if( ! f_gp_size ) {
  const auto gps = get_int_par( intGPMaxSz );
  if( gps <= 0 )
   return( false );
  f_gp_size = Index( gps );
  }

 std::vector< std::vector< Index > > held( k );
 Index most = 0;
 for( Index h = 0 ; h < k ; ++h ) {
  const auto m = v_members[ h ];
  for( Index n = 0 ; n < f_gp_size ; ++n )
   if( m->is_linearization_there( n ) && ( ! m->is_linearization_vertical( n ) ) )
    held[ h ].push_back( n );

  if( held[ h ].empty() )   // a member with nothing to contribute leaves the
   return( false );         // group without a combination to make
  most = std::max( most , Index( held[ h ].size() ) );
  }

 if( most < 2 )   // there is one vector only, and it has been given already
  return( false );

 for( Index trial = 0 ; trial < 10 ; ++trial ) {
  std::vector< Index > pick( k );
  for( Index h = 0 ; h < k ; ++h )
   pick[ h ] = held[ h ][ f_rnd() % held[ h ].size() ];

  if( f_given.count( pick ) )   // this one has been given already
   continue;

  f_given.insert( pick );
  v_pick = std::move( pick );
  v_part.assign( k , 1 );
  return( true );
  }

 return( false );   // ten draws and nothing new: enough for this evaluation
 }

/*--------------------------------------------------------------------------*/

bool C05SumFunction::compute_new_linearization( bool diagonal )
{
 if( diagonal ) {
  /* A new linearization of the group is a new combination of those of its
   * members, and the first new combination to look for is the one the
   * members themselves can produce: each of them is asked for one more, and
   * those that have one move to it while the others stay where they are, so
   * that the sum is a linearization the group has not reported yet. A member
   * sitting at its bound has nothing else to give, the flat subgradient
   * being the only one it has there. */

  bool any = false;
  for( Index h = 0 ; h < v_members.size() ; ++h )
   if( ( v_part[ h ] == 1 ) &&
       v_members[ h ]->compute_new_linearization( true ) )
    any = true;

  if( any )
   return( true );

  // the members have nothing new: what they have given before is still in
  // their global pool, and a vector taking one entry from each of them is a
  // linearization of the group that has not been reported yet
  return( random_combination() );
  }

 /* The vertical ones are a different matter: their sum is a valid inequality
  * of the domain, but a weaker one than any of its terms, so it is worth
  * reporting only when a single linearization is asked for. Each further
  * request is therefore answered with the vertical linearization of one
  * member alone, the members being walked in order; when they have all been
  * handed out, they are asked for a new one each and the walk starts again
  * over those that have one. */

 if( v_vert.size() != v_members.size() )  // no vertical round is on
  return( false );

 for( ; ; ) {
  const Index from = ( f_solo == Inf< Index >() ? 0 : f_solo + 1 );
  for( Index h = from ; h < v_members.size() ; ++h )
   if( v_vert[ h ] ) {
    f_solo = h;
    v_part.assign( v_members.size() , 0 );
    v_part[ h ] = 1;
    return( true );
    }

  // they have all been handed out: ask each of them for one more
  bool any = false;
  for( Index h = 0 ; h < v_members.size() ; ++h )
   if( ( v_vert[ h ] = v_members[ h ]->compute_new_linearization( false ) ) )
    any = true;

  if( ! any )
   return( false );

  f_solo = Inf< Index >();
  }
 }

/*--------------------------------------------------------------------------*/

bool C05SumFunction::has_linearization( bool diagonal )
{
 f_solo = Inf< Index >();  // whatever is asked for, the round starts again
 v_pick.clear();           // and so does the drawing of the vectors
 f_given.clear();

 if( diagonal ) {
  // every member gives one, or is horizontal at its finite bound
  bool own = false;
  for( Index h = 0 ; h < v_members.size() ; ++h )
   if( v_members[ h ]->has_linearization( true ) ) {
    v_part[ h ] = 1;
    own = true;
    }
   else {
    const auto b = bound_of( h );
    if( ( b == Inf< FunctionValue >() ) || ( b == -Inf< FunctionValue >() ) )
     return( false );
    v_part[ h ] = 2;
    }

  return( own );  // with every member at its bound, the bound says it all
  }

 // the members that give a vertical one make it, the others take no part;
 // when only one of them does, the sum is that member's own linearization,
 // so the round of the single ones starts after it rather than handing the
 // same row back a second time [see compute_new_linearization]
 bool any = false;
 f_solo = Inf< Index >();
 Index only = Inf< Index >();
 Index howmany = 0;
 v_vert.assign( v_members.size() , 0 );
 for( Index h = 0 ; h < v_members.size() ; ++h )
  if( ( v_vert[ h ] = v_part[ h ] = v_members[ h ]->has_linearization( false ) ) ) {
   any = true;
   only = h;
   ++howmany;
   }

 if( howmany == 1 )    // the sum is the linearization of that member alone
  f_solo = only;       // which is therefore already given

 return( any );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::set_flat( Index h , Index name ,
                                              ModParam issueMod )
{
 if( v_members[ h ]->is_linearization_there( name ) )
  v_members[ h ]->delete_linearization( name , issueMod );
 v_flat[ h ][ name ] = bound_of( h );
 }

/*--------------------------------------------------------------------------*/

C05SumFunction::Subset C05SumFunction::
                 remove_stale_flat_linearizations( const C05Function * member )
{
 Subset stale;
 const auto it = std::find( v_members.begin() , v_members.end() , member );
 if( it == v_members.end() )
  return( stale );

 const Index h = std::distance( v_members.begin() , it );
 const auto b = bound_of( h );
 for( const auto & f : v_flat[ h ] )
  if( f.second != b )
   stale.push_back( f.first );

 std::sort( stale.begin() , stale.end() );
 for( auto name : stale )
  delete_linearization( name , eNoMod );

 return( stale );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::unset( Index h , Index name ,
                                           ModParam issueMod )
{
 if( v_members[ h ]->is_linearization_there( name ) )
  v_members[ h ]->delete_linearization( name , issueMod );
 v_flat[ h ].erase( name );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::store_linearization( Index name , ModParam issueMod )
{
 {
  own_operation mine( f_own_op );

  for( Index h = 0 ; h < v_members.size() ; ++h )
   switch( v_part[ h ] ) {
    case( 1 ):
     // when the current linearization is a vector over what the members
     // hold, each of them copies the entry it contributes into the new name
     // rather than storing the one it has just computed
     // [see random_combination]
     if( v_pick.empty() )
      v_members[ h ]->store_linearization( name , issueMod );
     else if( v_pick[ h ] != name ) {
      LinearCombination one( { std::pair( v_pick[ h ] ,
                                          FunctionValue( 1 ) ) } );
      v_members[ h ]->store_combination_of_linearizations( one , name ,
                                                           issueMod );
      }
     v_flat[ h ].erase( name );
     break;
    case( 2 ): set_flat( h , name , issueMod ); break;
    default:   unset( h , name , issueMod );
    }
  }

 issue_pool_Modification( C05FunctionMod::GlobalPoolAdded ,
                          Subset( { name } ) , issueMod );
 }

/*--------------------------------------------------------------------------*/

bool C05SumFunction::is_linearization_there( Index name ) const
{
 for( Index h = 0 ; h < v_members.size() ; ++h )
  if( v_members[ h ]->is_linearization_there( name ) || is_flat( h , name ) )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/

bool C05SumFunction::is_linearization_vertical( Index name ) const
{
 // a diagonal linearization is in every member, possibly as the horizontal
 // one at its bound, a vertical one only in the members that made it, and
 // all of them hold a vertical one
 bool there = false;
 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  if( is_flat( h , name ) )
   return( false );
  const auto m = v_members[ h ];
  if( m->is_linearization_there( name ) ) {
   if( ! m->is_linearization_vertical( name ) )
    return( false );
   there = true;
   }
  }
 return( there );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::store_combination_of_linearizations(
                                          c_LinearCombination & coefficients ,
                                          Index name , ModParam issueMod )
{
 // the member combines those it holds, and gives the horizontal linearization
 // at its bound to the mass left by its diagonal ones; if it holds no
 // diagonal one, the group records the horizontal one if any is combined
 {
  own_operation mine( f_own_op );

  LinearCombination own;
  for( Index h = 0 ; h < v_members.size() ; ++h ) {
   const auto m = v_members[ h ];
   own.clear();
   bool own_diag = false;
   bool flat = false;
   for( const auto & c : coefficients )
    if( m->is_linearization_there( c.first ) ) {
     own.push_back( c );
     if( ! m->is_linearization_vertical( c.first ) )
      own_diag = true;
     }
    else
     if( is_flat( h , c.first ) )
      flat = true;

   if( ! own.empty() )
    m->store_combination_of_linearizations( own , name , issueMod );
   else
    if( m->is_linearization_there( name ) )
     m->delete_linearization( name , issueMod );

   if( flat && ! own_diag )
    v_flat[ h ][ name ] = bound_of( h );
   else
    v_flat[ h ].erase( name );
   }
  }

 issue_pool_Modification( C05FunctionMod::GlobalPoolAdded ,
                          Subset( { name } ) , issueMod );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::set_important_linearization(
                                         LinearCombination && coefficients )
{
 LinearCombination own;
 for( auto m : v_members ) {
  own.clear();
  for( const auto & c : coefficients )
   if( m->is_linearization_there( c.first ) )
    own.push_back( c );
  m->set_important_linearization( std::move( own ) );
  }

 f_important = std::move( coefficients );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::delete_linearization( Index name , ModParam issueMod )
{
 {
  own_operation mine( f_own_op );

  for( Index h = 0 ; h < v_members.size() ; ++h )
   unset( h , name , issueMod );
  }

 issue_pool_Modification( C05FunctionMod::GlobalPoolRemoved ,
                          Subset( { name } ) , issueMod );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::delete_linearizations( Subset && which , bool ordered ,
                                              ModParam issueMod )
{
 {
  own_operation mine( f_own_op );

  if( which.empty() ) {  // all of them, in every member
   for( auto m : v_members )
    m->delete_linearizations( Subset() , ordered , issueMod );
   for( auto & f : v_flat )
    f.clear();
   }
  else
   for( Index h = 0 ; h < v_members.size() ; ++h ) {
    const auto m = v_members[ h ];
    Subset own;
    for( auto name : which ) {
     if( m->is_linearization_there( name ) )
      own.push_back( name );
     v_flat[ h ].erase( name );
     }
    if( ! own.empty() )
     m->delete_linearizations( std::move( own ) , ordered , issueMod );
    }
  }

 issue_pool_Modification( C05FunctionMod::GlobalPoolRemoved ,
                          std::move( which ) , issueMod );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::full_coefficients( Index name )
{
 f_g.assign( v_vars.size() , 0 );

 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  if( ! takes_part( h , name ) )
   continue;

  const auto & map = v_map[ h ];
  f_gh.resize( map.size() );
  v_members[ h ]->get_linearization_coefficients( f_gh.data() ,
                                                  Range( 0 , map.size() ) ,
                                                  name_of( h , name ) );
  for( Index i = 0 ; i < map.size() ; ++i )
   f_g[ map[ i ] ] += f_gh[ i ];
  }
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::get_linearization_coefficients( FunctionValue * g ,
                                                       Range range ,
                                                       Index name )
{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 full_coefficients( name );
 std::copy( f_g.begin() + range.first , f_g.begin() + range.second , g );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::get_linearization_coefficients( FunctionValue * g ,
                                                       c_Subset & subset ,
                                                       bool ordered ,
                                                       Index name )
{
 if( subset.empty() )
  return;

 full_coefficients( name );
 for( auto i : subset )
  *(g++) = f_g[ i ];
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue C05SumFunction::get_linearization_constant(
                                                                 Index name )
{
 group_sum a;
 for( Index h = 0 ; h < v_members.size() ; ++h ) {
  if( takes_part( h , name ) )
   a += v_members[ h ]->get_linearization_constant( name_of( h , name ) );
  if( name == Inf< Index >() ) {
   if( v_part[ h ] == 2 )
    a += bound_of( h );
   }
  else {
   const auto it = v_flat[ h ].find( name );
   if( it != v_flat[ h ].end() )
    a += it->second;
   }
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS FOR THE State --------------------------*/
/*--------------------------------------------------------------------------*/

State * C05SumFunction::get_State( void ) const
{
 throw( std::logic_error( "C05SumFunction::get_State: the State of a "
                          "C05SumFunction is not supported" ) );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::put_State( const State & state )
{
 throw( std::logic_error( "C05SumFunction::put_State: the State of a "
                          "C05SumFunction is not supported" ) );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::put_State( State && state )
{
 throw( std::logic_error( "C05SumFunction::put_State: the State of a "
                          "C05SumFunction is not supported" ) );
 }

/*--------------------------------------------------------------------------*/

void C05SumFunction::serialize_State( netCDF::NcGroup & group ,
                                        const std::string & sub_group_name )
 const
{
 throw( std::logic_error( "C05SumFunction::serialize_State: the State of a"
                          " C05SumFunction is not supported" ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File C05SumFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------ File C05SumFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the C05SumFunction class, the C05Function that is the sum
 * of a given set of C05Function, which it computes and whose linearizations
 * it combines, while being their Observer so that what happens to them is
 * seen as happening to the sum.
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
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __C05SumFunction
 #define __C05SumFunction
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05Function.h"

#include "ColVariable.h"

#include "Observer.h"

#include <chrono>

#include <functional>

#include <future>

#include <list>

#include <map>

#include <random>

#include <set>

#include <unordered_map>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup C05SumFunction_CLASSES Classes in C05SumFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS C05SumFunction ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// the C05Function that is the sum of a set of C05Function
/** The class C05SumFunction represents the sum
 *
 *     f( x ) = \sum_{ h \in G } f_h( x )
 *
 * of a group G of C05Function, the members, which are neither owned nor
 * changed in any other way than by being computed and by having their
 * global pools used. Its "active" Variable are the union of those of the
 * members, in the order given to the constructor.
 *
 * A linearization of the group is a sum of linearizations of the members
 * stored under the *same* name in their global pools, which therefore must
 * all have (at least) the size of the one of the group:
 *
 * - a diagonal one is the sum of one diagonal linearization per member,
 *   all of them computed at the same point; a member that gives none there
 *   although it has a finite global lower [upper] bound, as a convex
 *   [concave] C05Function is allowed to do at its minimum [maximum], takes
 *   part with the horizontal linearization at that bound, which the group
 *   records for that name instead of storing it in the member, and whose
 *   constant is the bound when it is read; if every member is at its bound
 *   there is no diagonal linearization, as for a single member;
 *
 * - a vertical one is the sum of the vertical linearizations of the members
 *   that have one, since for each of them 0 >= alpha_h + g_h x holds; the
 *   members that do not have one take no part in it, and do not hold that
 *   name in their global pool.
 *
 * Accordingly, a combination of linearizations of the group is the
 * combination, member by member, of those among them that the member holds.
 * A member that holds diagonal ones among them is expected to give to the
 * mass their multipliers leave the horizontal linearization at its bound, as
 * PolyhedralFunction does; a member that holds none takes part in a diagonal
 * combination with the horizontal one, if any of the combined ones is such.
 *
 * The value, the estimates, the global bounds and the Lipschitz constant are
 * the sums of those of the members. Parameters are passed on to every member
 * as they are, save for the targets dblUpCutOff and dblLwCutOff, which cannot
 * be split among them and are therefore not; note that this means that a time
 * limit is given to each member, and not to the group as a whole.
 *
 * The State of a C05SumFunction is not supported, and neither is changing
 * its Variable: the members are Observed as usual by whoever Observes them,
 * and it is up to that one to know that they are part of a group. */

class C05SumFunction : public C05Function , public Observer
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC TYPES OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// iterator over the "active" Variable
 class v_iterator : public ThinVarDepInterface::v_iterator
 {
  public:

  explicit v_iterator( std::vector< ColVariable * >::iterator itr )
   : itr_( itr ) {}

  v_iterator * clone( void ) override final {
   return( new v_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++itr_; }

  reference operator*( void ) const override final { return( **itr_ ); }

  pointer operator->( void ) const override final { return( *itr_ ); }

  bool operator==( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   auto tmp = dynamic_cast< const C05SumFunction::v_iterator * >( & rhs );
   return( tmp ? itr_ == tmp->itr_ : false );
   }

  bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
   const override final {
   auto tmp = dynamic_cast< const C05SumFunction::v_iterator * >( & rhs );
   return( tmp ? itr_ != tmp->itr_ : true );
   }

  private:

  std::vector< ColVariable * >::iterator itr_;
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// const iterator over the "active" Variable

 class v_const_iterator : public ThinVarDepInterface::v_const_iterator
 {
  public:

  explicit v_const_iterator(
                       std::vector< ColVariable * >::const_iterator itr )
   : itr_( itr ) {}

  v_const_iterator * clone( void ) override final {
   return( new v_const_iterator( itr_ ) );
   }

  void operator++( void ) override final { ++itr_; }

  reference operator*( void ) const override final { return( **itr_ ); }

  pointer operator->( void ) const override final { return( *itr_ ); }

  bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   auto tmp =
    dynamic_cast< const C05SumFunction::v_const_iterator * >( & rhs );
   return( tmp ? itr_ == tmp->itr_ : false );
   }

  bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
   const override final {
   auto tmp =
    dynamic_cast< const C05SumFunction::v_const_iterator * >( & rhs );
   return( tmp ? itr_ != tmp->itr_ : true );
   }

  private:

  std::vector< ColVariable * >::const_iterator itr_;
  };

/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING C05SumFunction ------------*/
/*--------------------------------------------------------------------------*/

 /// constructor: the members, and the union of their "active" Variable
 /** Constructs the sum of the given members, whose "active" Variable are
  * vars; each "active" Variable of each member must be in vars, which
  * dictates their order in the sum. The members must be all convex or all
  * concave. The sum becomes the Observer of each member, keeping the one it
  * had, to which the Modification of that member are passed on after the sum
  * has dealt with them [see add_Modification()]. This is what observe asks
  * for, and it is what a sum standing on its own needs; whoever drives both
  * the sum and the members, as a Solver that builds the sum out of the
  * components of the Block it is solving does, already sees the Modification
  * of the members and can pass observe = false, the sum then only being
  * computed and asked for linearizations. */

 C05SumFunction( std::vector< C05Function * > && members ,
                 std::vector< ColVariable * > && vars ,
                 bool observe = true );

/*--------------------------------------------------------------------------*/
 /// constructor: the members alone, the "active" Variable being their union
 /** As the other constructor, save that the "active" Variable are computed
  * here as the union of those of the members, in the order in which the
  * members give them. */

 explicit C05SumFunction( std::vector< C05Function * > && members ,
                          bool observe = true );

/*--------------------------------------------------------------------------*/
 /// destructor: gives the members back the Observer they had

 ~C05SumFunction() override;

/*--------------------------------------------------------------------------*/
/*-------------------------------- GETTERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the members of the group
 /// how many members the group evaluates at once
 /** Sets how many of its members the group is allowed to evaluate at the
  * same time, which is one unless somebody says otherwise. The number is
  * not read from intMaxThread because the threads are either spent by the
  * group on its members or by each member on itself: the solver that drives
  * the group is the only one that knows how many of them are not already
  * spent on evaluating the groups themselves. */

  void set_members_at_once( int n ) {
  f_max_thread = n > 1 ? n : 1;
  }

/*--------------------------------------------------------------------------*/
 /// the threads the group is given, if they come from a pool
 /** The type of what runs a member on a thread that is not the caller's: it
  * takes the member and whether its Variable have changed, and returns the
  * future of its compute(), i.e., it is what compute_async() does, done by
  * whoever owns the threads. */

 using Submitter = std::function< std::future< int >(
                                   ThinComputeInterface * , bool ) >;

/*--------------------------------------------------------------------------*/
 /// gives the group the threads of whoever drives it
 /** Sets what the group uses to evaluate a member on another thread. On its
  * own the group starts a thread for each of them, which costs of the order
  * of 10^-4 seconds and is therefore worth it only for members that cost
  * more than that; a solver that keeps a pool of threads alive can hand it
  * over here, the cost of using one becoming that of a queue and the
  * threads of the two levels being the same ones, so that they are not
  * contended. Passing nothing puts the group back to starting its own. */

 void set_submitter( Submitter submit = {} ) {
  f_submit = std::move( submit );
  }

 const std::vector< C05Function * > & get_members( void ) const {
  return( v_members );
  }

/*--------------------------------------------------------------------------*/
 /// how many members have been computed since the group was built
 /** The number of times a member of the group has been computed, i.e., the
  * number of evaluations of the original components that the evaluations of
  * the group have cost. Computing the group computes every member, hence
  * this is the number of evaluations of the group times the number of its
  * members, and it is what has to be compared with the evaluations of a
  * solver that sees the components one by one. */

 [[nodiscard]] long get_member_evaluations( void ) const {
  return( f_member_evals );
  }

 /// removes the linearizations where member is horizontal at a stale bound
 /** Removes from the global pool of the group, without issuing any
  * Modification, the linearizations where member takes part with the
  * horizontal linearization at a bound that is no longer its global bound,
  * and returns their names, ordered. The caller is expected to call it
  * whenever member has changed, and to deal with them as removed. */

 Subset remove_stale_flat_linearizations( const C05Function * member );

/*--------------------------------------------------------------------------*/
/*--------------- METHODS FOR HANDLING THE PARAMETERS ----------------------*/
/*--------------------------------------------------------------------------*/

 void set_par( idx_type par , int value ) override;

 void set_par( idx_type par , double value ) override;

 void set_par( idx_type par , std::string && value ) override;

 void set_ComputeConfig( const ComputeConfig * scfg = nullptr ) override;

 [[nodiscard]] int get_int_par( idx_type par ) const override {
  return( v_members.front()->get_int_par( par ) );
  }

 [[nodiscard]] double get_dbl_par( idx_type par ) const override {
  if( par == dblMaxTime )
   return( f_max_time );
  return( v_members.front()->get_dbl_par( par ) );
  }

/*--------------------------------------------------------------------------*/
/*----------- METHODS FOR HANDLING THE "ACTIVE" Variable ------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] Block * get_Block( void ) const override {
  return( f_Observer ? f_Observer->get_Block() : nullptr );
  }

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR BEING THE Observer OF THE MEMBERS --------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for being the Observer of the members
 *
 * The sum has to see what happens to its members, since a change in one of
 * them is a change in the sum, and the only way to see it is to be their
 * Observer. It therefore takes that role at construction and gives it back
 * at destruction, keeping the Observer each member had: a Modification of a
 * member is first used to keep the global pool of the sum what the
 * Modification say it is, then reported to the Observer of the sum as a
 * Modification of the sum, and finally passed on to the Observer the member
 * had, which is what the Solver attached to the Block of that member are
 * waiting for.
 *  @{ */

 [[nodiscard]] bool anyone_there( void ) const override {
  return( f_Observer ? f_Observer->anyone_there() : false );
  }

/*--------------------------------------------------------------------------*/
 /// a member has changed: see the class comments

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

/*--------------------------------------------------------------------------*/

 ChnlName open_channel( ChnlName chnl = 0 ,
                        GroupModification * gmpmod = nullptr ) override {
  return( f_Observer ? f_Observer->open_channel( chnl , gmpmod ) : 0 );
  }

/*--------------------------------------------------------------------------*/

 void close_channel( ChnlName chnl , bool force = false ) override {
  if( f_Observer )
   f_Observer->close_channel( chnl , force );
  }

/*--------------------------------------------------------------------------*/

 void set_default_channel( ChnlName chnl = 0 ) override {
  if( f_Observer )
   f_Observer->set_default_channel( chnl );
  }

/** @} ---------------------------------------------------------------------*/

 [[nodiscard]] Index get_num_active_var( void ) const override {
  return( Index( v_vars.size() ) );
  }

 Index is_active( const Variable * var ) const override {
  const auto it = f_var2idx.find( var );
  return( it == f_var2idx.end() ? Inf< Index >() : it->second );
  }

 [[nodiscard]] Variable * get_active_var( Index i ) const override {
  return( v_vars[ i ] );
  }

 v_iterator * v_begin( void ) override {
  return( new v_iterator( v_vars.begin() ) );
  }

 [[nodiscard]] v_const_iterator * v_begin( void ) const override {
  return( new v_const_iterator( v_vars.cbegin() ) );
  }

 v_iterator * v_end( void ) override {
  return( new v_iterator( v_vars.end() ) );
  }

 [[nodiscard]] v_const_iterator * v_end( void ) const override {
  return( new v_const_iterator( v_vars.cend() ) );
  }

 void remove_variable( Index i , ModParam issueMod = eModBlck ) override;

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS FOR COMPUTING THE FUNCTION ------------------*/
/*--------------------------------------------------------------------------*/

 /// computes all the members, and returns the first status that is not kOK
 int compute( bool changedvars = true ) override;

 [[nodiscard]] FunctionValue get_value( void ) override;

 [[nodiscard]] FunctionValue get_lower_estimate( void ) override;

 [[nodiscard]] FunctionValue get_upper_estimate( void ) override;

 FunctionValue get_global_lower_bound( void ) override;

 FunctionValue get_global_upper_bound( void ) override;

 FunctionValue get_Lipschitz_constant( void ) override;

 [[nodiscard]] bool is_convex( void ) override { return( f_convex ); }

 [[nodiscard]] bool is_concave( void ) override { return( f_concave ); }

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR LINEARIZATIONS ------------------------*/
/*--------------------------------------------------------------------------*/

 bool has_linearization( bool diagonal = true ) override;

 bool compute_new_linearization( bool diagonal = true ) override;

 void store_linearization( Index name , ModParam issueMod = eModBlck )
  override;

 [[nodiscard]] bool is_linearization_there( Index name ) const override;

 [[nodiscard]] bool is_linearization_vertical( Index name ) const override;

 void store_combination_of_linearizations(
                         c_LinearCombination & coefficients , Index name ,
                         ModParam issueMod = eModBlck ) override;

 void set_important_linearization( LinearCombination && coefficients )
  override;

 [[nodiscard]] c_LinearCombination & get_important_linearization_coefficients(
                                                        void ) const override {
  return( f_important );
  }

 void delete_linearization( Index name , ModParam issueMod = eModBlck )
  override;

 void delete_linearizations( Subset && which , bool ordered = true ,
                             ModParam issueMod = eModBlck ) override;

 void get_linearization_coefficients( FunctionValue * g ,
                                      Range range = INFRange ,
                                      Index name = Inf< Index >() ) override;

 void get_linearization_coefficients( FunctionValue * g ,
                                      c_Subset & subset ,
                                      bool ordered = false ,
                                      Index name = Inf< Index >() ) override;

 FunctionValue get_linearization_constant( Index name = Inf< Index >() )
  override;

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS FOR THE State --------------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] State * get_State( void ) const override;

 void put_State( const State & state ) override;

 void put_State( State && state ) override;

 void serialize_State( netCDF::NcGroup & group ,
                       const std::string & sub_group_name = "" )
  const override;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 /// the coefficients of the linearization name of the group, all of them
 void full_coefficients( Index name );

 /// true if member h gives coefficients to the linearization name
 [[nodiscard]] bool takes_part( Index h , Index name ) const {
  return( name == Inf< Index >() ? ( v_part[ h ] == 1 )
                                 : v_members[ h ]->is_linearization_there(
                                                                   name ) );
  }

 /// true if member h is horizontal at its bound in the linearization name
 [[nodiscard]] bool is_flat( Index h , Index name ) const {
  return( name == Inf< Index >() ? ( v_part[ h ] == 2 )
                                 : ( v_flat[ h ].count( name ) > 0 ) );
  }

 /// the global lower [upper] bound of member h, if convex [concave]
 [[nodiscard]] FunctionValue bound_of( Index h ) const {
  return( f_convex ? v_members[ h ]->get_global_lower_bound()
                   : v_members[ h ]->get_global_upper_bound() );
  }

 /// records member h as horizontal at its current bound in name
 void set_flat( Index h , Index name , ModParam issueMod );

 /// removes from member h the linearization name, if any
 void unset( Index h , Index name , ModParam issueMod );

 /// how the absolute accuracies are shared out among the members
 std::vector< double > accuracy_shares( void ) const;

 /// takes one linearization of each member at random and makes it the
 /// current one of the group [see compute_new_linearization()]
 bool random_combination( void );

 /// refreshes what a member costs, which is what decides whether they are
 /// evaluated together [see compute()]
 void update_member_time( const std::chrono::system_clock::time_point & from ,
                          Index howmany );

 /// evaluates the members together, as many at a time as the threads the
 /// group is allowed [see compute()]
 int compute_parallel( bool changedvars ,
                       const std::vector< double > & shares );

 /// the name member h contributes to the linearization name of the group
 [[nodiscard]] Index name_of( Index h , Index name ) const {
  return( ( name == Inf< Index >() ) && ( ! v_pick.empty() ) ? v_pick[ h ]
                                                             : name );
  }

 /// the Modification of a member, seen by the sum [see add_Modification()]
 void member_Modification( const sp_Mod & mod , Observer * previous ,
                           ChnlName chnl );

  /// which member a Modification speaks for, Inf< Index >() if none
 [[nodiscard]] Index member_of( const sp_Mod & mod ) const;

 /// redoes the map between the Variable of member h and those of the sum
 void rebuild_map( Index h );

 /// a member has changed its "active" Variable, hence maybe the sum has
 /** The "active" Variable of the sum are the union of those of the members:
  * this takes in one added to or removed from a member, gives the sum the
  * ones that no other member has, redoes the map between the two, and says
  * so to the Observer of the sum. */

 void member_Variables_changed(
                     const std::shared_ptr< FunctionModVars > & vmod ,
                     ChnlName chnl );

 /// what a Modification of a member does to the sum, told to its Observer
 /** The part of member_Modification() that concerns the sum: the cascade on
  * the global pool and the Modification of the sum that the one of the
  * member translates into. It does not pass the original on to the Observer
  * the member had, which the caller does. */

 void translate_Modification( const sp_Mod & mod , ChnlName chnl );

 /// a GroupModification of the members, seen by the sum
 /** The Observer of a Function is entitled to receive a GroupModification,
  * and has to look inside it: this does so, translating each of the
  * sub-Modification that belongs to a member, and says once what the whole
  * group does to the sum whenever the translations agree. The group itself
  * is passed on, whole, to the Observer the members had. */

 void group_Modification( const std::shared_ptr< GroupModification > & gmod ,
                          ChnlName chnl );

 /// tells the Observer of the sum, or holds it while a group is open
 void emit( sp_Mod out , ChnlName chnl );

 /// says once what a whole group of Modification of the members does
 /** Marks the span in which the translations of the sum are held instead of
  * being told one by one: at the end of it the ones that say the same thing
  * about the sum are merged, i.e., a group in which every member changes in
  * the same way becomes one Modification of the sum and not one per member. */

 class bunching
 {
  public:
  explicit bunching( C05SumFunction & f , ChnlName chnl )
   : f_f( f ) , f_chnl( chnl ) { ++f_f.f_bunching; }
  ~bunching() { if( ! --f_f.f_bunching ) f_f.flush_pending( f_chnl ); }
  private:
  C05SumFunction & f_f;
  ChnlName f_chnl;
  };

 /// merges the held translations and tells them to the Observer
 void flush_pending( ChnlName chnl );

 /// tells the Observer of the sum what has happened to its global pool
 void issue_pool_Modification( int type , Subset && which ,
                               ModParam issueMod );

 /// while the sum is changing the members itself, what they say is theirs
 /** Marks the methods where the sum changes the global pools of the members
  * on purpose: a Modification arriving from a member while this is set is
  * the echo of what the sum is doing, and it is only passed on to the
  * Observer the member had, the sum reporting for itself. */

 class own_operation
 {
  public:
  explicit own_operation( bool & flag ) : f_flag( flag ) { f_flag = true; }
  ~own_operation() { f_flag = false; }
  private:
  bool & f_flag;
  };

 /// takes the role of Observer of every member, keeping the one they had
 void steal_the_Observers( void );

 std::vector< C05Function * > v_members;  ///< the members

 std::vector< Observer * > v_prev_obs;
 ///< the Observer each member had, to which its Modification are passed on

 bool f_own_op = false;
 ///< true while the sum is changing the global pools of the members itself
 ///< [see own_operation]

 unsigned int f_bunching = 0;
 ///< how many groups of Modification of the members are being looked into,
 ///< the translations being held until the outermost one is done
 ///< [see bunching]

 std::list< sp_Mod > v_pending;
 ///< the translations held while a group is being looked into [see emit()]

 std::vector< ColVariable * > v_vars;     ///< the "active" Variable

 std::unordered_map< const Variable * , Index > f_var2idx;
 ///< the position of each "active" Variable

 std::vector< std::vector< Index > > v_map;
 ///< v_map[ h ][ i ] = position in v_vars of the i-th Variable of member h

 std::vector< char > v_part;
 ///< how the members take part in the last computed linearization: 0 not
 ///< at all, 1 with their own, 2 with the horizontal one at their bound

 double f_max_time = Inf< double >();
 ///< the time the group is given, which it hands down one member at a time

 long f_member_evals = 0;
 ///< how many members have been computed [see get_member_evaluations()]

 std::map< idx_type , FunctionValue > f_abs_par;
 ///< the absolute accuracies asked of the group, which it shares out among
 ///< the members at each compute() [see set_par( idx_type , double )]

 double f_member_time = 0;
 ///< what a member has taken, on average, at the evaluations so far: it is
 ///< what says whether evaluating them together is worth a thread each
 ///< [see compute()]

  int f_max_thread = 1;
 ///< how many members the group evaluates at once: one, i.e. one at a time,
 ///< unless whoever drives the group says otherwise
 ///< [see set_members_at_once()]

 Submitter f_submit;
 ///< what runs a member on a thread of whoever drives the group, if any:
 ///< when it is not there the group starts a thread of its own
 ///< [see set_submitter()]

 Index f_gp_size = 0;
 ///< how many names the global pool of each member holds [intGPMaxSz]

 std::vector< Index > v_pick;
 ///< when not empty, the current linearization of the group is the vector
 ///< that takes the linearization v_pick[ h ] of each member h, rather than
 ///< the one each of them holds as its own current

 std::set< std::vector< Index > > f_given;
 ///< the vectors already handed out since the last evaluation, so that the
 ///< same combination is not reported twice

 std::mt19937 f_rnd;
 ///< the generator of the vectors, seeded as the grouping is

 Index f_solo = Inf< Index >();
 ///< when < Inf, the current linearization is the vertical one of this
 ///< member alone, the group handing the vertical ones out one at a time
 ///< after the first request [see compute_new_linearization()]

 std::vector< char > v_vert;
 ///< which members still have a vertical linearization to hand out

 std::vector< FunctionValue > v_last;
 ///< v_last[ h ] = what member h was worth at its last evaluation, which is
 ///< the share of the absolute accuracies it gets; empty until the first
 ///< compute(), where the shares are equal

 std::vector< std::unordered_map< Index , FunctionValue > > v_flat;
 ///< v_flat[ h ][ name ] = the bound member h is horizontal at in name

 bool f_convex;   ///< true if the members are convex

 bool f_concave;  ///< true if the members are concave

 LinearCombination f_important;  ///< the important linearization

 std::vector< FunctionValue > f_g;    ///< scratch: the group coefficients

 std::vector< FunctionValue > f_gh;   ///< scratch: those of a member

/*--------------------------------------------------------------------------*/

 };  // end( class( C05SumFunction ) )

/** @} end( group( C05SumFunction_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* C05SumFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File C05SumFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/

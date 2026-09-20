/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the AbstractBlock class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <boost/algorithm/string.hpp> // Used in read()

#include "AbstractBlock.h"

#include "BlockInspection.h"

#include <iomanip>
#include <map>
#include <sstream>


#include "ColVariable.h"

#include "LinearFunction.h"
#include "QuadFunction.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

#include "FRealObjective.h"

#include "ColVariableSolution.h"
#include "RowConstraintSolution.h"
#include "ColRowSolution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using v_coeff_pair = LinearFunction::v_coeff_pair;
using v_coeff_triple = DQuadFunction::v_coeff_triple;
using v_off_diag_term = QuadFunction::v_off_diag_term;

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// says that the container a group views belongs to the Block, and how it goes

template< class C >
static void own_storage( const std::unique_ptr< BaseGroup > & group , C * c )
{
 if( group )
  group->set_storage_deleter( [ c ]( void ) { delete c; } );
 }

/*--------------------------------------------------------------------------*/
/// calls the right function on each element of a group of :RowConstraint
/** Calls frow() on each element of the group if these are FRowConstraint, and
 * onevar() on each of them if these are one of the concrete
 * :OneVarConstraint of the core; returns false, having done nothing, if the
 * elements are of none of those types. */

template< class FR , class FO >
bool for_each_RowConstraint( const BaseGroup & group , FR frow , FO onevar )
{
 return( group.for_each_as< FRowConstraint >( frow ) ||
	 for_each_as_any_of< BoxConstraint , LB0Constraint , UB0Constraint ,
			     LBConstraint , UBConstraint , NNConstraint ,
			     NPConstraint , ZOConstraint >( group , onevar ) );
 }

/*--------------------------------------------------------------------------*/
/// as for_each_RowConstraint(), with the name of each element beside it
/** Calls frow( name , element ) or onevar( name , element ), the name being
 * the one the group gives the element [see inspection::name_of()]. */

template< class FR , class FO >
bool for_each_named_RowConstraint( const BaseGroup & group , FR frow ,
				   FO onevar )
{
 return( inspection::for_each_named_as< FRowConstraint >( group , frow ) ||
	 inspection::for_each_named_as_any_of<
	  FO , BoxConstraint , LB0Constraint , UB0Constraint , LBConstraint ,
	  UBConstraint , NNConstraint , NPConstraint ,
	  ZOConstraint >( group , onevar ) );
 }

}  // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register AbstractBlock to the Block factory

SMSpp_insert_in_factory_cpp_1( AbstractBlock );

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS of AbstractBlock --------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock::~AbstractBlock()
{
 // first, clear() all Constraint: each group says what it holds, hence no
 // type has to be enumerated here
 auto clear_them = [ this ]( const Vec_Group & groups , Index first ) {
  for( auto & group : groups )
   if( group && ( ! group->is_indirect() ) &&
       ( group->get_index() >= first ) )
    group->for_each( []( Constraint & cnst ) { cnst.clear(); } );
  };

 clear_them( get_static_constraint_groups() , get_first_static_Constraint() );
 clear_them( get_dynamic_constraint_groups() ,
	     get_first_dynamic_Constraint() );

 // then clear the Objective
 if( ( ! is_Objective_reserved() ) && get_objective() )
  get_objective()->clear();

 // now delete all the inner Block
 for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
  delete v_Block[ i ];

 v_Block.clear();

 // now delete the containers this Block owns: each of them was registered
 // here, and its group was told then how to dispose of it, so nothing has to
 // be said here about their types. A container somebody else owns has no
 // deleter and is left alone
 auto dispose_of = []( const Vec_Group & groups , Index first ) {
  for( auto & group : groups )
   if( group && ( group->get_index() >= first ) )
    group->delete_storage();
  };

 dispose_of( get_static_constraint_groups() , get_first_static_Constraint() );
 dispose_of( get_dynamic_constraint_groups() ,
	     get_first_dynamic_Constraint() );
 dispose_of( get_static_variable_groups() , get_first_static_Variable() );
 dispose_of( get_dynamic_variable_groups() , get_first_dynamic_Variable() );

 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::load( std::istream & input , char frmt )
{
 if( ( ! frmt ) || ( frmt == 'M' ) ) {
  read_mps( input );
  return;
  }

 if( frmt == 'L' ) {
  read_lp( input );
  return;
  }

 throw( std::invalid_argument( "AbstractBlock::read: unsupported file format"
                               ) );
 }

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract , Configuration * fsbc )
{
 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 double eps = 0;
 bool rel_viol = true;

 // Try to extract, from "c", the parameters that determine feasibility.
 // If it succeeds, it sets the values of the parameters and returns
 // true. Otherwise, it returns false.
 auto extract_parameters = [ & eps , & rel_viol ]( Configuration * c )
  -> bool {
  if( auto tc = dynamic_cast< SimpleConfiguration< double > * >( c ) ) {
   eps = tc->f_value;
   return( true );
   }
  if( auto tc = dynamic_cast< SimpleConfiguration<
      std::pair< double , int > > * >( c ) ) {
   eps = tc->f_value.first;
   rel_viol = tc->f_value.second;
   return( true );
   }
  return( false );
  };

 if( ( ! extract_parameters( fsbc ) ) && f_BlockConfig )
  // if the given Configuration is not valid, try the one from the BlockConfig
  extract_parameters( f_BlockConfig->f_is_feasible_Configuration );

 bool feas = true;

 // a direction is feasible if it satisfies the homogeneous version of the
 // Constraint: with a finite right-hand side the row must not grow along the
 // ray, with a finite left-hand side it must not shrink, and an infinite
 // side asks nothing at all
 auto check_direction = [ & feas , eps ]( const RowConstraint & cnst ,
                                          double value ) {
                         double viol = 0;
                         if( cnst.get_rhs() < RowConstraint::RHSINF )
                          viol = value;
                         if( cnst.get_lhs() > - RowConstraint::RHSINF )
                          viol = std::max( viol , - value );
                         feas = ( viol <= eps ); };

 // check if a OneVarConstraint is satisfied, but without computing it; the
 // row of a OneVarConstraint is the Variable itself, hence its homogeneous
 // version is the value of the Variable
 auto check_feasibility = [ & feas , eps , rel_viol , this ,
                            & check_direction ]( auto & cnst ) {
                           if( ( ! feas ) || cnst.is_relaxed() )
                            return;
                           if( f_is_direction ) {
                            check_direction( cnst , static_cast< ColVariable * >(
                             cnst.get_active_var( 0 ) )->get_value() );
                            return;
                            }
                           feas = ( ( rel_viol ? cnst.rel_viol() :
                                      cnst.abs_viol() ) <= eps ); };

 // a direction is checked against the homogeneous version of the row. For a
 // linear one that is the row without its constant term. For a quadratic one
 // the row is worth ( q . d ) alpha + ( d^T Q d ) alpha^2 along the ray, hence
 // the sign of d^T Q d decides: negative, the row goes to minus infinity and
 // only an infinite left-hand side survives it; positive, it goes to plus
 // infinity and only an infinite right-hand side does; zero, what is left is
 // the linear term and the rule is the linear one. Anything else has no
 // homogeneous version to be checked against
 auto check_direction_frow = [ & feas , eps , & check_direction ]
                             ( FRowConstraint & cnst ) {
  auto f = cnst.get_function();

  if( auto lf = dynamic_cast< LinearFunction * >( f ) ) {
   check_direction( cnst , lf->get_value() - lf->get_constant_term() );
   return;
   }

  if( auto qf = dynamic_cast< DQuadFunction * >( f ) ) {
   double quad = 0;
   double lin = 0;
   for( Index i = 0 ; i < qf->get_num_active_var() ; ++i ) {
    const double di = static_cast< const ColVariable * >(
                                    qf->get_active_var( i ) )->get_value();
    quad += qf->get_quadratic_coefficient( i ) * di * di;
    lin += qf->get_linear_coefficient( i ) * di;
    }

   if( quad < - eps ) {
    feas = ( cnst.get_lhs() <= - RowConstraint::RHSINF );
    return;
    }
   if( quad > eps ) {
    feas = ( cnst.get_rhs() >= RowConstraint::RHSINF );
    return;
    }

   check_direction( cnst , lin );
   return;
   }

  throw( std::logic_error( "AbstractBlock::is_feasible: a direction is only "
                           "checked against linear or diagonal quadratic "
                           "Constraint" ) );
  };

 // check if a FRowConstraint is satisfied
 auto check_frow = [ & feas , eps , rel_viol , this ,
                     & check_direction_frow ]
                   ( FRowConstraint & cnst ) {
                    if( ( ! feas ) || cnst.is_relaxed() )
                     return;
                    if( auto ret = cnst.compute() ;
                        ( ret <= FRowConstraint::kUnEval ) ||
                        ( ret > FRowConstraint::kOK ) ) {
                     feas = false;
                     return;
                     }
                    if( f_is_direction ) {
                     check_direction_frow( cnst );
                     return;
                     }
                    feas = ( ( rel_viol ? cnst.rel_viol() :
                               cnst.abs_viol() ) <= eps ); };

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 // note: AbstractBlock::is_feasible() is now checking *all* the abstract
 //       representation, both the "reserved" part and all the rest. another
 //       approach would be to leave the "reserved" part to derived classes
 //       and only test here the "non reserved" part. This might be
 //       fractionally more efficient but it would require every derived
 //       class to implement is_feasible(); so far we prefer the general
 //       even if possibly slower solution
 for( const auto & group : get_static_constraint_groups() ) {
  if( ! group )
   continue;
  if( ! for_each_RowConstraint( *group , check_frow , check_feasibility ) )
   throw( std::logic_error(
       "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  if( ! feas )
   return( false );
  }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 // see above for comments
 auto check_variable = [ & feas , eps ]( ColVariable & var ) {
  feas = feas && var.is_feasible( eps ); };

 for( const auto & group : get_static_variable_groups() ) {
  if( ! group )
   continue;
  if( ! group->for_each_as< ColVariable >( check_variable ) )
   throw( std::logic_error( "some static Variable not ColVariable" ) );
  if( ! feas )
   return( false );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 // see above for comments
 for( const auto & group : get_dynamic_constraint_groups() ) {
  if( ! group )
   continue;
  if( ! for_each_RowConstraint( *group , check_frow , check_feasibility ) )
   throw( std::logic_error(
    "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
  if( ! feas )
   return( false );
  }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 // see above for comments
 for( const auto & group : get_dynamic_variable_groups() ) {
  if( ! group )
   continue;
  if( ! group->for_each_as< ColVariable >( check_variable ) )
   throw( std::logic_error( "some dynamic Variable not ColVariable" ) );
  if( ! feas )
   return( false );
  }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 //!! for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
 // see above for comments
 for( auto bi : v_Block )
  if( ! bi->is_feasible( useabstract ) )
   return( false );

 return( true );

 }  // end( AbstractBlock::is_feasible )

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Variable( Variable * var )
{
 if( var->get_Block() != this )
  std::cout << std::endl << "Variable " << var
            << " not of the right Block";

 for( Index as = 0 ; as < var->get_num_active() ; ++as ) {
  auto tvdi = var->get_active( as );
  if( tvdi->is_active( var ) >= tvdi->get_num_active_var() )
   std::cout << std::endl << "Variable " << var
             << " not active in its " << as << "-th active stuff";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Constraint( Constraint * cnst )
{
 if( cnst->get_Block() != this )
  std::cout << std::endl << "Constraint " << cnst
            << " not of the right Block";

 for( Index av = 0 ; av < cnst->get_num_active_var() ; ++av ) {
  auto var = cnst->get_active_var( av );
  if( var->is_active( cnst ) >= var->get_num_active() )
   std::cout << std::endl << "Constraint " << cnst
             << " not active in its " << av << "-th active Variable";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Objective( Objective * obj )
{
 if( obj->get_Block() != this )
  std::cout << std::endl << "Objective " << obj << " not of the right Block";

 for( Index av = 0 ; av < obj->get_num_active_var() ; ++av ) {
  auto var = obj->get_active_var( av );
  if( var->is_active( obj ) >= var->get_num_active() )
   std::cout << std::endl << "Objective " << obj
             << " not active in its " << av << "-th active Variable";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::is_correct( void )
{
 auto check_var = [ this ]( ColVariable & var ) { check_Variable( & var ); };

 auto check_cnst = [ this ]( auto & cnst ) { check_Constraint( & cnst ); };

 // the Variables of the Block- - - - - - - - - - - - - - - - - - - - - - - -
 for_each_variable_group( [ & check_var ]( const BaseGroup & group ) {
   if( ! group.for_each_as< ColVariable >( check_var ) )
    throw( std::logic_error( std::string( "some " ) +
			     ( group.is_dynamic() ? "dynamic" : "static" ) +
			     " Variable not ColVariable" ) );
   } );

 // the Constraints of the Block- - - - - - - - - - - - - - - - - - - - - - -
 for_each_constraint_group( [ & check_cnst ]( const BaseGroup & group ) {
   if( ! for_each_RowConstraint( group , check_cnst , check_cnst ) )
    throw( std::logic_error( std::string( "some " ) +
			     ( group.is_dynamic() ? "dynamic" : "static" ) +
			     " Constraint not FRowConstraint or"
			     " :OneVarConstraint" ) );
   } );

 // the Objective of the Block- - - - - - - - - - - - - - - - - - - - - - - -
 if( auto obj = get_objective() )
  check_Objective( obj );

 // check every sub-Block of AbstractBlock - - - - - - - - - - - - - - - - -

 for( Index i = 0 ; i < get_number_nested_Blocks() ; ++i ) {
  auto sb = get_nested_Block( i );
  if( sb->get_f_Block() != this )
   std::cout << std::endl << "sub-Block " << i << " has wrong father";
  if( auto asb = dynamic_cast< AbstractBlock * >( sb ) )
   asb->is_correct();
  }
 }  // end( AbstractBlock::is_correct )

/*--------------------------------------------------------------------------*/

Solution * AbstractBlock::get_Solution( Configuration * csolc, bool emptys )
{

 auto config = dynamic_cast< SimpleConfiguration< int > * >( csolc );

 if( ( ! config ) && f_BlockConfig )
  config = dynamic_cast< SimpleConfiguration< int > * >(
   f_BlockConfig->f_solution_Configuration );

 auto solution_type = config ? config->f_value : 0;

 Solution * sol = nullptr;
 switch( solution_type ) {
  case 1:
   sol = new RowConstraintSolution;
   break;
  case 2:
   sol = new ColRowSolution;
   break;
  default:
   sol = new ColVariableSolution;
  }

 if( ! emptys )
  sol->read( this );
 
 return( sol );
 }

/*--------------------------------------------------------------------------*/

/* Writes v with the fewest digits that read back as v: a model file is read
 * by somebody else, so a number in it has to be the number that was written
 * and not the six digits the default precision of a stream gives. */

static void put_double( std::ostream & output , double v )
{
 std::ostringstream s;
 for( int p = 15 ; p <= 17 ; ++p ) {
  s.str( std::string() );
  s.clear();
  s << std::setprecision( p ) << v;
  if( std::stod( s.str() ) == v )
   break;
  }
 output << s.str();
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::file_model( std::vector< f_column > & columns ,
				std::vector< f_row > & rows ) const
{
 /* A model file has no notion of groups: every column and every row is one
  * of a list, and what says which is which is its name. The names are those
  * the groups give [see inspection::name_of()], with the indices joined by
  * underscores, since neither format takes brackets or spaces inside a name.
  * The order is the order the elements are stored in, so that writing the
  * same Block twice gives the same file and reading one back gives the
  * columns in the order they had. */

 const inspection::name_format fmt = { "_" , "" , "g" , "" };

 auto do_columns = [ & columns , & fmt ]( const Vec_Group & groups ) {
  for( const auto & group : groups ) {
   if( ! group )
    continue;
   inspection::for_each_named_as< ColVariable >( *group ,
    [ & columns ]( const std::string & n , ColVariable & v ) {
     columns.emplace_back( & v , n ); } , fmt );
   }
  };

 do_columns( get_static_variable_groups() );
 do_columns( get_dynamic_variable_groups() );

 auto do_rows = [ & rows , & fmt ]( const Vec_Group & groups ) {
  for( const auto & group : groups ) {
   if( ! group )
    continue;
   inspection::for_each_named_as< FRowConstraint >( *group ,
    [ & rows ]( const std::string & n , FRowConstraint & c ) {
     rows.emplace_back( & c , n ); } , fmt );
   }
  };

 do_rows( get_static_constraint_groups() );
 do_rows( get_dynamic_constraint_groups() );

 }  // end( AbstractBlock::file_model )

/*--------------------------------------------------------------------------*/

void AbstractBlock::file_bounds( const std::vector< f_column > & columns ,
				 std::vector< f_bound > & bounds ) const
{
 /* What a column has of its own, tightened by the :OneVarConstraint that are
  * written on it: both formats say the bounds of a column in one place of
  * their own, and not as a row. */

 std::map< const ColVariable * , Index > where;
 bounds.resize( columns.size() );

 for( Index i = 0 ; i < columns.size() ; ++i ) {
  where[ columns[ i ].first ] = i;
  bounds[ i ] = { columns[ i ].first->get_lb() ,
		  columns[ i ].first->get_ub() };
  }

 auto tighten = [ & where , & bounds ]( const Vec_Group & groups ) {
  for( const auto & group : groups ) {
   if( ! group )
    continue;
   for_each_as_any_of< BoxConstraint , LB0Constraint , UB0Constraint ,
		       LBConstraint , UBConstraint , NNConstraint ,
		       NPConstraint , ZOConstraint >( *group ,
    [ & where , & bounds ]( OneVarConstraint & c ) {
     auto it = where.find( static_cast< const ColVariable * >(
					       c.get_active_var( 0 ) ) );
     if( it == where.end() )
      return;
     auto & b = bounds[ it->second ];
     b.first = std::max( b.first , double( c.get_lhs() ) );
     b.second = std::min( b.second , double( c.get_rhs() ) );
     } );
   }
  };

 tighten( get_static_constraint_groups() );
 tighten( get_dynamic_constraint_groups() );

 }  // end( AbstractBlock::file_bounds )

/*--------------------------------------------------------------------------*/

/* The LinearFunction a row or the Objective is written on, refusing anything
 * else: what the two formats can say is a linear expression, so a Function
 * that is not one has to be reported rather than silently written wrong. */

static const LinearFunction * linear_of( const Function * f ,
					 const std::string & what )
{
 auto lf = dynamic_cast< const LinearFunction * >( f );
 if( ! lf )
  throw( std::invalid_argument( "AbstractBlock::write: " + what +
				" is not written on a LinearFunction" ) );
 return( lf );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::write_lp( std::ostream & output ) const
{
 std::vector< f_column > columns;
 std::vector< f_row > rows;
 file_model( columns , rows );

 std::map< const ColVariable * , const std::string * > name;
 for( const auto & [ var , n ] : columns )
  name[ var ] = & n;

 auto write_linear = [ & output , & name ]( const LinearFunction * lf ) {
  bool first = true;
  for( const auto & [ var , coeff ] : lf->get_v_var() ) {
   if( coeff == 0 )
    continue;
   const auto it = name.find( var );
   if( it == name.end() )
    throw( std::logic_error( "AbstractBlock::write_lp: a Variable of the "
			     "model is not in any group of this Block" ) );
   if( first ) {
    output << ( coeff < 0 ? "- " : "" );
    first = false;
    }
   else
    output << ( coeff < 0 ? " - " : " + " );
   const auto a = std::abs( coeff );
   if( a != 1 ) {
    put_double( output , a );
    output << " ";
    }
   output << *( it->second );
   }
  if( first )       // every coefficient is zero: the expression is empty,
   output << "0";   // and the format wants something there
  };

 // the Objective - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 output << "\\ written by AbstractBlock::write_lp()" << std::endl;

 auto obj = dynamic_cast< const FRealObjective * >( get_objective() );
 output << ( ( obj && ( obj->get_sense() == Objective::eMax ) )
	     ? "Maximize" : "Minimize" ) << std::endl << " obj: ";
 if( obj )
  write_linear( linear_of( obj->get_function() , "the Objective" ) );
 else
  output << "0";
 output << std::endl;

 // the rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // a row with both sides finite and different is written twice, since the
 // format has no two-sided row; one with both sides equal is an equality

 output << "Subject To" << std::endl;

 for( const auto & [ row , n ] : rows ) {
  auto lf = linear_of( row->get_function() , "the row " + n );
  const auto lhs = row->get_lhs();
  const auto rhs = row->get_rhs();
  if( lhs == rhs ) {
   output << " " << n << ": ";
   write_linear( lf );
   output << " = ";
   put_double( output , rhs );
   output << std::endl;
   continue;
   }
  if( rhs < RowConstraint::RHSINF ) {
   output << " " << n << "_up: ";
   write_linear( lf );
   output << " <= ";
   put_double( output , rhs );
   output << std::endl;
   }
  if( lhs > - RowConstraint::RHSINF ) {
   output << " " << n << "_lo: ";
   write_linear( lf );
   output << " >= ";
   put_double( output , lhs );
   output << std::endl;
   }
  }

 // the bounds - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::vector< f_bound > bnd;
 file_bounds( columns , bnd );

 output << "Bounds" << std::endl;
 for( Index i = 0 ; i < columns.size() ; ++i ) {
  const auto [ lb , ub ] = bnd[ i ];
  const auto & n = columns[ i ].second;
  if( ( lb <= - Inf< double >() ) && ( ub >= Inf< double >() ) ) {
   output << " " << n << " free" << std::endl;
   continue;
   }
  if( lb > - Inf< double >() ) {
   put_double( output , lb );
   output << " <= ";
   }
  output << n;
  if( ub < Inf< double >() ) {
   output << " <= ";
   put_double( output , ub );
   }
  output << std::endl;
  }

 // the integer columns- - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool any_integer = false;
 for( const auto & [ var , n ] : columns )
  if( var->is_integer() ) {
   if( ! any_integer ) {
    output << "Generals" << std::endl;
    any_integer = true;
    }
   output << " " << n << std::endl;
   }

 output << "End" << std::endl;

 }  // end( AbstractBlock::write_lp )

/*--------------------------------------------------------------------------*/

void AbstractBlock::write_mps( std::ostream & output ) const
{
 std::vector< f_column > columns;
 std::vector< f_row > rows;
 file_model( columns , rows );

 std::vector< f_bound > bnd;
 file_bounds( columns , bnd );

 /* The MPS file is written by column, which is the opposite of how a Block
  * holds the model: a row knows the columns it is written on, a column knows
  * nothing. So the rows are walked once and what each column appears in is
  * collected, the entries of a column staying together as the format wants.
  * The name of a row is its position here, the objective being -1. */

 std::map< const ColVariable * , std::vector< std::pair< int , double > > >
  entries;
 for( const auto & [ var , n ] : columns )
  entries[ var ];   // a column with no entry at all is still a column

 auto collect = [ & entries ]( const LinearFunction * lf , int r ,
			       const std::string & what ) {
  for( const auto & [ var , coeff ] : lf->get_v_var() ) {
   if( coeff == 0 )
    continue;
   auto it = entries.find( var );
   if( it == entries.end() )
    throw( std::logic_error( "AbstractBlock::write_mps: a Variable of " +
			     what + " is not in any group of this Block" ) );
   it->second.emplace_back( r , coeff );
   }
  };

 auto obj = dynamic_cast< const FRealObjective * >( get_objective() );
 if( obj )
  collect( linear_of( obj->get_function() , "the Objective" ) , -1 ,
	   "the Objective" );

 /* A row whose two sides are both infinite constrains nothing, and the
  * format has no way of saying it other than a second objective row, so it
  * is left out: what goes is the row, not anything the model says. */

 std::vector< Index > kept;
 for( Index r = 0 ; r < rows.size() ; ++r ) {
  const auto lhs = rows[ r ].first->get_lhs();
  const auto rhs = rows[ r ].first->get_rhs();
  if( ( lhs <= - RowConstraint::RHSINF ) && ( rhs >= RowConstraint::RHSINF ) )
   continue;
  collect( linear_of( rows[ r ].first->get_function() ,
		      "the row " + rows[ r ].second ) , int( kept.size() ) ,
	   "the row " + rows[ r ].second );
  kept.push_back( r );
  }

 auto row_name = [ & rows , & kept ]( int r ) -> const std::string & {
  static const std::string objective = "obj";
  return( r < 0 ? objective : rows[ kept[ r ] ].second );
  };

 // the header and the rows- - - - - - - - - - - - - - - - - - - - - - - - -
 // G means rhs <= f(), L means f() <= rhs, E means both, and the second side
 // of a two-sided row travels in RANGES

 output << "NAME" << std::endl;
 output << "OBJSENSE" << std::endl << "    "
	<< ( ( obj && ( obj->get_sense() == Objective::eMax ) )
	     ? "MAX" : "MIN" ) << std::endl;

 output << "ROWS" << std::endl;
 output << " N  obj" << std::endl;

 auto sense_of = []( const FRowConstraint * row ) {
  const auto lhs = row->get_lhs();
  const auto rhs = row->get_rhs();
  if( lhs == rhs )
   return( 'E' );
  return( rhs < RowConstraint::RHSINF ? 'L' : 'G' );
  };

 for( auto r : kept )
  output << " " << sense_of( rows[ r ].first ) << "  " << rows[ r ].second
	 << std::endl;

 // the columns- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // the integer ones are the ones between an INTORG marker and an INTEND one,
 // and a column that is in no row at all is given a zero cost so that it is
 // in the file, the format having no place where a column is just named

 output << "COLUMNS" << std::endl;

 bool integer = false;
 int marker = 0;
 for( const auto & [ var , n ] : columns ) {
  if( var->is_integer() != integer ) {
   integer = ! integer;
   output << "    M" << marker++ << "  'MARKER'  '"
	  << ( integer ? "INTORG" : "INTEND" ) << "'" << std::endl;
   }

  auto & mine = entries[ var ];
  if( mine.empty() )
   mine.emplace_back( -1 , 0.0 );

  for( Index k = 0 ; k < mine.size() ; k += 2 ) {
   output << "    " << n;
   output << "  " << row_name( mine[ k ].first ) << "  ";
   put_double( output , mine[ k ].second );
   if( k + 1 < mine.size() ) {
    output << "  " << row_name( mine[ k + 1 ].first ) << "  ";
    put_double( output , mine[ k + 1 ].second );
    }
   output << std::endl;
   }
  }

 if( integer )
  output << "    M" << marker << "  'MARKER'  'INTEND'" << std::endl;

 // the right-hand sides and the ranges- - - - - - - - - - - - - - - - - - -

 output << "RHS" << std::endl;
 for( Index r = 0 ; r < kept.size() ; ++r ) {
  const auto * row = rows[ kept[ r ] ].first;
  const double value = ( sense_of( row ) == 'G' ) ? double( row->get_lhs() )
						  : double( row->get_rhs() );
  output << "    RHS  " << rows[ kept[ r ] ].second << "  ";
  put_double( output , value );
  output << std::endl;
  }

 bool any_range = false;
 for( Index r = 0 ; r < kept.size() ; ++r ) {
  const auto * row = rows[ kept[ r ] ].first;
  const auto lhs = row->get_lhs();
  const auto rhs = row->get_rhs();
  if( ( lhs == rhs ) || ( lhs <= - RowConstraint::RHSINF ) ||
      ( rhs >= RowConstraint::RHSINF ) )
   continue;
  if( ! any_range ) {
   output << "RANGES" << std::endl;
   any_range = true;
   }
  output << "    RNG  " << rows[ kept[ r ] ].second << "  ";
  put_double( output , double( rhs ) - double( lhs ) );
  output << std::endl;
  }

 // the bounds - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // the default of the format is 0 <= x < +infinity, so only what differs is
 // written; MI is not used, as what it means has changed over time, and a
 // column with no lower bound is freed and then given its upper one

 bool any_bound = false;
 auto bound_line = [ & output , & any_bound ]( const char * type ,
					       const std::string & n ) {
  if( ! any_bound ) {
   output << "BOUNDS" << std::endl;
   any_bound = true;
   }
  output << " " << type << " BND  " << n;
  };

 for( Index i = 0 ; i < columns.size() ; ++i ) {
  const auto [ lb , ub ] = bnd[ i ];
  const auto & n = columns[ i ].second;
  const auto * var = columns[ i ].first;

  if( var->is_fixed() ) {
   bound_line( "FX" , n );
   output << "  ";
   put_double( output , var->get_value() );
   output << std::endl;
   continue;
   }

  if( ( lb == 0 ) && ( ub >= Inf< double >() ) )
   continue;

  if( lb == ub ) {
   bound_line( "FX" , n );
   output << "  ";
   put_double( output , lb );
   output << std::endl;
   continue;
   }

  if( lb <= - Inf< double >() ) {
   bound_line( "FR" , n );
   output << std::endl;
   }
  else
   if( lb != 0 ) {
    bound_line( "LO" , n );
    output << "  ";
    put_double( output , lb );
    output << std::endl;
    }

  if( ub < Inf< double >() ) {
   bound_line( "UP" , n );
   output << "  ";
   put_double( output , ub );
   output << std::endl;
   }
  }

 output << "ENDATA" << std::endl;

 }  // end( AbstractBlock::write_mps )

/*--------------------------------------------------------------------------*/

void AbstractBlock::print( std::ostream & output , char vlvl ) const
{
 if( vlvl == 'M' ) {
  write_mps( output );
  return;
  }

 if( vlvl == 'L' ) {
  write_lp( output );
  return;
  }
 
 output << std::endl << "AbstractBlock with: ";
 output << std::endl << get_static_variable_groups().size()
        << " types of static Variables, "
        << get_dynamic_variable_groups().size()
        << " types of dynamic Variables, "
        << std::endl << get_static_constraint_groups().size()
        << " types of static Constraints, "
        << get_dynamic_constraint_groups().size()
        << " types of dynamic Constraints, "
        << std::endl << v_Block.size() << " inner Blocks" << std::endl;

 if( vlvl ) {
  auto header = [ & output ]( const BaseGroup & group ) {
   output << group.get_index();
   if( group.get_name().empty() )
    output << ": ";
   else
    output << " (" << group.get_name() << "): ";
   };

  auto print_it = [ & output ]( const std::string & name , auto & element ) {
   output << name << ": " << element << std::endl; };

  auto print_constraints = [ & output , & header , & print_it ]
			   ( const Vec_Group & groups , Index first ) {
   for( const auto & group : groups ) {
    if( ( ! group ) || ( group->get_index() < first ) )
     continue;
    header( *group );
    output << std::endl;
    if( ! for_each_named_RowConstraint( *group , print_it , print_it ) )
     throw( std::logic_error( std::string( "some " ) +
			      ( group->is_dynamic() ? "dynamic" : "static" ) +
			      " Constraint not FRowConstraint or"
			      " :OneVarConstraint" ) );
    }
   };

  // each Variable is printed with the name its group gives it, which is
  // what tells which one of them a row of the model is written on
  auto print_named = [ & output ]( const std::string & name ,
				   ColVariable & variable ) {
   output << name << ": " << variable << std::endl; };

  auto print_variables = [ & output , & header , & print_named ]
			 ( const Vec_Group & groups , Index first ) {
   for( const auto & group : groups ) {
    if( ( ! group ) || ( group->get_index() < first ) )
     continue;
    header( *group );
    output << std::endl;
    if( ! inspection::for_each_named_as< ColVariable >( *group ,
						       print_named ) )
     throw( std::logic_error( std::string( "some " ) +
			      ( group->is_dynamic() ? "dynamic" : "static" ) +
			      " Variable not ColVariable" ) );
    }
   };

  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << std::endl;
  print_constraints( get_static_constraint_groups() ,
		     get_first_static_Constraint() );

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << std::endl;
  print_variables( get_static_variable_groups() ,
		   get_first_static_Variable() );

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << std::endl;
  print_constraints( get_dynamic_constraint_groups() ,
		     get_first_dynamic_Constraint() );

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << std::endl;
  print_variables( get_dynamic_variable_groups() ,
		   get_first_dynamic_Variable() );

  // the Objective of the Block - - - - - - - - - - - - - - - - - - - - - - -
  if( ! is_Objective_reserved() )
   output << "Objective:" << *get_objective() << std::endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output << std::endl << "Nested Blocks:" << std::endl;
  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
   output << *v_Block[ i ];
  }
 }  // end( AbstractBlock::print )

/*--------------------------------------------------------------------------*/

void AbstractBlock::serialize( netCDF::NcGroup & group ) const
{
 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -

 Block::serialize( group );

 // now the AbstractBlock data- - - - - - - - - - - - - - - - - - - - - - - -

 auto & sc = get_static_constraint_groups();
 auto & sv = get_static_variable_groups();
 auto & dc = get_dynamic_constraint_groups();
 auto & dv = get_dynamic_variable_groups();

 /* What is not reserved to a derived class is written as the LP file
  * deserialize() reads back out of Model, with ModelType saying which of the
  * two formats it is [see guts_of_deserialize()]. An LP file has no notion
  * of groups, so what travels is the model and not the way it is grouped:
  * reading it back gives one group of columns and one of rows, as read_lp()
  * builds them. */

 if( ( sc.size() > get_first_static_Constraint() ) ||
     ( dc.size() > get_first_dynamic_Constraint() ) ||
     ( sv.size() > get_first_static_Variable() ) ||
     ( dv.size() > get_first_dynamic_Variable() ) ||
     ( get_objective() && ( ! is_Objective_reserved() ) ) ) {
  std::ostringstream model;
  write_lp( model );
  const auto str = model.str();
  const char * c_str = str.c_str();

  auto ncVar = group.addVar( "Model" , netCDF::NcString() );
  ncVar.putVar( & c_str );
  ncVar.putAtt( "ModelType" , netCDF::NcChar() , 1 , "L" );
  }

 if( v_Block.size() > get_first_inner_Block() ) {
  group.addDim( "NumberInnerBlock", v_Block.size() );

  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i ) {
   auto gi = group.addGroup( "Block_" + std::to_string( i ) );
   v_Block[ i ]->serialize( gi );
   }
  }
 }  // end( AbstractBlock::serialize )

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_mps( std::istream & file )
{
 auto dbl_val = []( std::string & s ) {
  assert( ! s.empty() );
  if( s[ 0 ] == '.' )
   s.insert( 0 , "0" );
  else
   if( ( s[ 0 ] == '-' ) && ( s[ 1 ] == '.' ) )
    s.insert( 1 , "0" );

  if( s.back() == '.' )
   s.pop_back();

  return( std::stod( s ) );
  };

 std::string problem_name;
 int num_rows = 0;
 int num_cols = 0;

 auto * of = new FRealObjective();
 std::string of_name;

 std::vector< FRowConstraint > * rows;
 std::vector< std::string > row_names;
 std::vector< char > row_type;

 std::vector< ColVariable > * cols;
 std::vector< std::string > col_names;
 std::vector< BoxConstraint > * bounds;

 std::string rhs_name; // Only one RHS vector is supported
 std::string rng_name; // Only one RANGES vector is supported
 std::string bnd_name; // Only one BOUNDS vector is supported
 std::vector< double > rhs;
 std::vector< double > rng;

 std::string word;
 auto max = std::numeric_limits< std::streamsize >::max();

 // Eat initial comments
 while( file.peek() == file.widen( '*' ) ) {
  file.ignore( max, '\n' );
 }

 auto trim = []( std::string & s ) {
  s.erase( 0 , s.find_first_not_of( " \t\r" ) );
  s.erase( s.find_last_not_of( " \t\r" ) + 1 );
  };

 // Read NAME; the problem name, which may well be empty, is whatever
 // remains on the same line
 file >> word;
 if( word != "NAME" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 std::getline( file , problem_name );
 trim( problem_name );

 file >> word;

 // Read the optional OBJSENSE section; the sense is found either on the
 // same line (as Gurobi and HiGHS write it) or on the following one, and
 // it defaults to minimization
 int of_sense = Objective::eMin;
 if( word == "OBJSENSE" ) {
  std::string sense;
  std::getline( file , sense );
  trim( sense );
  if( sense.empty() )
   file >> sense;
  if( ( sense == "MAX" ) || ( sense == "MAXIMIZE" ) )
   of_sense = Objective::eMax;
  else
   if( ( sense != "MIN" ) && ( sense != "MINIMIZE" ) )
    throw( std::invalid_argument( "Invalid OBJSENSE in MPS file" ) );
  file >> word;
 }

 /*
  * First pass: get rows and columns number
  */

 // Read ROWS
 if( word != "ROWS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 auto pos = file.tellg(); // Save it for later
 while( file.peek() == file.widen( ' ' ) ) {
  file >> word;
  if( word == "E" || word == "L" || word == "G" ) {
   ++num_rows;
   file.ignore( max, '\n' ); // Skip row name for now
  } else if( word == "N" ) {
   file.ignore( max, '\n' ); // Skip row name for now
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }
 }

 // Read COLUMNS
 file >> word;
 if( word != "COLUMNS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 std::string tmp;
 while( file.peek() == file.widen( ' ' ) ) {
  std::string row;

  file >> word;
  file >> row;

  if( row == "\'MARKER\'" ) {
   file.ignore( max, '\n' );
   continue; // Skip integrality markers for now
  }

  if( word != tmp ) {
   tmp = word;
   ++num_cols;
  }
  file.ignore( max, '\n' );
 }

 /*
  * Initialize stuff
  */

 row_names.resize( num_rows );
 row_type.resize( num_rows );
 rows = new std::vector< FRowConstraint >( num_rows );

 for( auto & r: *rows ) {
  r.set_function( new LinearFunction(), eNoMod );
  r.set_Block( this );
 }
 of->set_function( new LinearFunction(), eNoMod );

 cols = new std::vector< ColVariable >( num_cols );
 col_names.resize( num_cols );
 bounds = new std::vector< BoxConstraint >( num_cols );

 for( int i = 0; i < num_cols; ++i ) {
  ( *bounds )[ i ].set_variable( &( *cols )[ i ], eNoMod );
  ( *bounds )[ i ].set_Block( this );
  ( *cols )[ i ].set_Block( this );
 }

 rhs.resize( num_rows, 0 );
 rng.resize( num_rows, Inf< double >() );

 /*
  * Second pass: fill data
  */

 // Read ROWS
 file.seekg( pos, file.beg ); // Go back to line after "ROWS"
 int i = 0;
 while( file.peek() == file.widen( ' ' ) ) {

  file >> word;
  if( word == "E" || word == "L" || word == "G" ) {
   row_type[ i ] = word[ 0 ];
   file >> row_names[ i ];
   ++i;
   file.ignore( max, '\n' );
  } else if( word == "N" ) {
   if( of_name.empty() ) {
    file >> of_name;
   }
   // FIXME: Other N rows are ignored
   file.ignore( max, '\n' );
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }
 }

 // Read COLUMNS
 file.ignore( max, '\n' ); // Ignore "COLUMNS" line
 i = 0;
 bool marker = false;
 tmp.clear();
 ColVariable * v;
 LinearFunction * f;
 while( file.peek() == file.widen( ' ' ) ) {
  std::string row;
  std::string value;

  file >> word;
  file >> row;
  file >> value;

  // Check integrality marker
  if( row == "\'MARKER\'" ) {
   if( value == "\'INTORG\'" ) {
    marker = true;
   } else if( value == "\'INTEND\'" ) {
    marker = false;
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
   file.ignore( max, '\n' );
   continue;
  }

  // Column name
  if( word != tmp ) {
   tmp = word;
   col_names[ i ] = tmp;
   v = &( *cols )[ i ];
   if( marker ) {
    v->set_type( ColVariable::kInteger, eNoMod );
   }
   ++i;
  }

  // First name/value pair
  if( row == of_name ) {
   f = static_cast< LinearFunction * >(of->get_function());
  } else {
   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    f = static_cast< LinearFunction * >(( *rows )[ j ].get_function());
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
  }
  f->add_variable( v, dbl_val( value ) );

  // Optional second name/value pair
  if( file.peek() != file.widen( '\n' ) ) {
   file >> row;
   file >> value;
   if( row == of_name ) {
    // Will add to OF
    f = static_cast< LinearFunction * >(of->get_function());
   } else {
    // Will add to a constraint
    auto it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     f = static_cast< LinearFunction * >(( *rows )[ j ].get_function());
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   }
   f->add_variable( v, dbl_val( value ) );
  }
  file.ignore( max, '\n' );
 }

 /*
  * Continue with RHS and RANGES
  */

 // Read RHS
 file >> word;
 if( word != "RHS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 while( file.peek() == file.widen( ' ' ) ) {

  // RHS name
  file >> word;
  if( rhs_name.empty() ) {
   rhs_name = word;
  } else if( word != rhs_name ) {
   throw( std::invalid_argument( "Only one RHS vector is supported" ) );
  }

  std::string row;
  std::string value;

  // First name/value pair
  file >> row;
  file >> value;
  auto it = std::find( row_names.begin(), row_names.end(), row );
  if( it != row_names.end() ) {
   auto j = std::distance( row_names.begin(), it );
   rhs[ j ] = dbl_val( value );
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }

  // Optional second name/value pair
  if( file.peek() != file.widen( '\n' ) ) {
   file >> row;
   file >> value;
   it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rhs[ j ] = dbl_val( value );
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
  }
  file.ignore( max, '\n' );
 }

 // Read RANGES (optional)
 file >> word;
 if( word == "RANGES" ) {
  file.ignore( max, '\n' );
  while( file.peek() == file.widen( ' ' ) ) {

   // RANGES name
   file >> word;
   if( rng_name.empty() ) {
    rng_name = word;
   } else if( word != rng_name ) {
    throw( std::invalid_argument( "Only one RANGE vector is supported" ) );
   }

   std::string row;
   std::string value;

   // First name/value pair
   file >> row;
   file >> value;
   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rng[ j ] = dbl_val( value );
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }

   // Optional second name/value pair
   if( file.peek() != file.widen( '\n' ) ) {
    file >> row;
    file >> value;
    it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     rng[ j ] = dbl_val( value );
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   }
   file.ignore( max, '\n' );
  }
  file >> word;
 }

 // Process RHS and ranges
 for( int r = 0; r < num_rows; ++r ) {
  auto & row = ( *rows )[ r ];

  switch( row_type[ r ] ) {
   case 'G' :
    // G: rhs =< f() =< rhs + |rng|
    row.set_lhs( rhs[ r ], eNoMod );
    if( rng[ r ] == Inf< double >() ) {
     row.set_rhs( Inf< double >(), eNoMod );
    } else {
     row.set_rhs( rhs[ r ] + std::abs( rng[ r ] ), eNoMod );
    }
    break;

   case 'L' :
    // L: rhs - |rng| =< f() =< rhs
    row.set_rhs( rhs[ r ], eNoMod );
    if( rng[ r ] == Inf< double >() ) {
     row.set_lhs( -Inf< double >(), eNoMod );
    } else {
     row.set_lhs( rhs[ r ] - std::abs( rng[ r ] ), eNoMod );
    }
    break;

   case 'E' :
    if( rng[ r ] == Inf< double >() || rng[ r ] == 0 ) {
     // E (no range): rhs =< f() =< rhs
     row.set_both( rhs[ r ], eNoMod );
    } else if( rng[ r ] > 0 ) {
     // E+: rhs + rng =< f() =< rhs
     row.set_lhs( rhs[ r ] + rng[ r ], eNoMod );
     row.set_rhs( rhs[ r ], eNoMod );
    } else {
     // E-: rhs =< f() =< rhs + rng
     row.set_lhs( rhs[ r ], eNoMod );
     row.set_rhs( rhs[ r ] + rng[ r ], eNoMod );
    }
    break;
   default:;
  }
 }

 /*
  * Continue with BOUNDS
  */
 if( word == "BOUNDS" ) {
  file.ignore( max, '\n' );
  while( file.peek() == file.widen( ' ' ) ) {

   std::string type;
   file >> type;

   // BOUNDS name
   file >> word;
   if( bnd_name.empty() ) {
    bnd_name = word;
   } else if( word != bnd_name ) {
    throw( std::invalid_argument( "Only one BOUNDS vector is supported" ) );
   }

   std::string col;
   std::string value;
   file >> col;

   auto it = std::find( col_names.begin(), col_names.end(), col );
   if( it != col_names.end() ) {
    auto j = std::distance( col_names.begin(), it );
    auto & b = ( *bounds )[ j ];
    auto & c = ( *cols )[ j ];
    if( type == "LO" ) {        // Lower bound
     file >> value;
     b.set_lhs( dbl_val( value ), eNoMod );
    } else if( type == "UP" ) { // Upper bound
     file >> value;
     b.set_rhs( dbl_val( value ), eNoMod );
    } else if( type == "FX" ) { // Fixed variable
     file >> value;
     b.set_both( dbl_val( value ), eNoMod );
     c.set_value( dbl_val( value ) );
     c.is_fixed( true, eNoMod );
    } else if( type == "FR" ) { // Free variable
     b.set_lhs( -Inf< double >(), eNoMod );
     b.set_rhs( Inf< double >(), eNoMod );
    } else if( type == "MI" ) { // Lower bound -inf
     b.set_lhs( -Inf< double >(), eNoMod );
     b.set_rhs( 0, eNoMod );
    } else if( type == "PL" ) { // Upper bound +inf
     b.set_lhs( 0, eNoMod );
     b.set_rhs( Inf< double >(), eNoMod );
    } else if( type == "BV" ) { // Binary variable
     c.set_type( ColVariable::kBinary, eNoMod );
    } else if( type == "LI" ) { // Integer variable
     file >> value;
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_lhs( dbl_val( value ), eNoMod );
    } else if( type == "UI" ) { // Integer variable
     file >> value;
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_rhs( dbl_val( value ), eNoMod );
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
   file.ignore( max, '\n' );
  }
  file >> word;
 }

 if( word != "ENDATA" )
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );

 of->set_sense( of_sense , eNoMod );

 // Reset and set abstract representation
 reset_static_constraints();
 reset_static_variables();
 reset_objective();

 set_objective( of, eNoMod );
 add_static_variable( *cols );
 add_static_constraint( *rows );
 add_static_constraint( *bounds );

 // these three containers are ours, and the groups are told how to dispose
 // of them, so that the destructor does not have to know their type
 own_storage( get_static_variable_groups().back() , cols );
 own_storage( get_static_constraint_groups()[
			  get_static_constraint_groups().size() - 2 ] , rows );
 own_storage( get_static_constraint_groups().back() , bounds );

 // Issue the NBModification
 if( anyone_there() )
  add_Modification( std::make_shared< NBModification >( this ) );

 }  // end( AbstractBlock::read_mps )

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_lp( std::istream & file )
{

 // function to convert a float value written in a string in a double
 auto dbl_val = []( std::string & s ) {
 assert( ! s.empty() );
 if( s == "-infinity")
   return( -Inf< double >() );
 else if( s == "infinity")
   return( Inf< double >() );

 if( s[ 0 ] == '.' )
  s.insert( 0 , "0" );
 else
  if( ( s[ 0 ] == '-' ) && ( s[ 1 ] == '.' ) )
    s.insert( 1 , "0" );

 if( s.back() == '.' )
  s.pop_back();

 return( std::stod( s ) );
 };

 struct compare_words
 {
    std::string key_s;
    compare_words(std::string const &s): key_s(s) {}
 
    bool operator()(std::string const &s) {
        return boost::iequals( s , key_s);
    }
 };

 std::string problem_name;
 int num_rows = 0;
 int num_cols = 0;

 auto * of = new FRealObjective();
 std::string of_name;
 int of_sense = 0;
 bool is_qp = 0;

 std::vector< FRowConstraint > * rows;
 std::vector< std::string > row_names;
 std::vector< char > row_type;
 std::vector< bool > is_row_q;

 std::vector< ColVariable > * cols;
 std::vector< std::string > col_names;
 std::vector< BoxConstraint > * bounds;

 std::string rhs_name; // Only one RHS vector is supported
 std::string rng_name; // Only one RANGES vector is supported
 std::string bnd_name; // Only one BOUNDS vector is supported

 std::string word;
 auto max = std::numeric_limits< std::streamsize >::max();

 std::vector< std::string > minfinity_names = { "-inf" , 
                              "-infinity" };

 // int value used to store the section currently scanned
 int current_section;

 /*---------------------------------------*/
 /* HERE WE ARE STARTING TO READ THE FILE */
 /*---------------------------------------*/
  
  // Eat initial comments
 while( file.peek() == file.widen( '\\' )  || 
            file.peek() ==  '\n' ) {
  file.ignore( max, '\n' );
 }

 /*---------------------------------------*/
 /*----------- READ OBJECTIVE ------------*/
 /*---------------------------------------*/
 
 // Read Objective sense
 file >> word;
 if( boost::iequals(word, "maximize") || boost::iequals(word, "max") )
    of_sense = 1;
 else if( boost::iequals(word, "minimize") || boost::iequals(word, "min") )
    of_sense = -1;
 else
    throw( std::invalid_argument( "Invalid objective sense in" 
        " LP file" ) );
  
 // Get objective function data
 file >> word;
 int pos = word.find( ":" );
 of_name = word.substr( 0 , pos );

 // Function name and row name should always end with a ":" in
 // .lp format. Thus, if no ":" has been found, it means we
 // are already reading the formula. Otherwise, we can skip to the next
 // next word.
 if( pos != -1 )
  file >> word;

 std::string first_obj_word = word;

 current_section = LP_sections::LP_LINOBJECTIVE;
 sec_reached( &current_section , word );
 auto pos_start_objective = file.tellg(); // Save it for later

 /*---------------------------------------*/
 /*------------- FIRST SCAN --------------*/
 /*---------------------------------------*/

 // First pass: just get number of columns and rows

 while( current_section == LP_sections::LP_LINOBJECTIVE ) {
  std::string column;
  ColVariable * v;
  
  char first_char = word[0];
  int len_word = word.length();
  bool read_sign = ( first_char == '-'  || first_char == '+' );
  
  // we can either read the sign, the coefficient or directly the 
  // variable ( i.e., the coefficient is 1). In Highs usually the
  // coefficient and the sign are grouped.
  // NOTE: in the QP formulation, we have also the possibility to
  // read a * or ^, but we expect to find a [ at the beginning of the
  // quadratic part.
  file.get(); // eat space
  if( file.peek() !=  '[' ) {
    if( len_word == 1 && read_sign ) {
    file >> word; // reading the coefficient
    if( std::isdigit( word[0] ) )
      // we read the coefficient
      file >> column; // reading the variable name
    else
      column = word;
    }
    else if( std::isdigit( first_char ) || read_sign ) {
      // we already read the coefficient
      file >> column; // reading the variable name
    }
    else{ // the only possibility left is that we read the variable name
      column = word;
    }
  
    // When reading the active variable in the objective function, no variable
    // have been added before
    col_names.push_back( column );
    ++num_cols;
  }
  
  file >> word;
  sec_reached( &current_section , word );
 }

 /*---------------------------------------*/
 /*------ READ QUADRATIC OBJECTIVE -------*/
 /*---------------------------------------*/

 while( current_section == LP_sections::LP_QUADOBJECTIVE ) {
  file >> word;

  if( is_qp == 0)
    is_qp = 1;

  std::string column;
  ColVariable * v;
  
  char first_char = word[0];
  int len_word = word.length();
  bool read_sign = ( first_char == '-'  || first_char == '+' );

  if( first_char == ']' ) {
    // we reached the end of the quadratic section.
    file.ignore( max, '\n' );

    file >> word;
    sec_reached( &current_section , word );

    break;
  }
  
  // we can either read the sign, the coefficient or directly the 
  // variable ( i.e., the coefficient is 1 or we are reading the 
  // second variable in term x_i*x_j). In Highs usually the
  // coefficient and the sign are grouped.

  if( len_word == 1 && read_sign ) {
   file >> word; // reading the coefficient
   if( std::isdigit( word[0] ) )
      // we read the coefficient
      file >> column; // reading the variable name
    else
      column = word;
  }
  else if( std::isdigit( first_char ) || read_sign ) {
    // we already read the coefficient
    file >> column; // reading the variable name
  }
  else{ // the only possibility left is that we read the variable name
    column = word;
  }

  file.get(); // eat white space
  if( column[ column.length() - 2 ] == '^' ) {
    // We are reading the quadratic term x^2. Thus the real name of the
    // variable is obtained by removing the last two character
    column = column.substr( 0 , column.length() - 2 );
  }
  else if( file.peek() == '*' ) {
    // We read only the first term. In this first scan, simply skip to
    // the second
    file >> word;
  }
  
  // Now we have to check if the variable considered has been already found
  // in the linear objective function or in a precedent quadratic term
  auto it = std::find( col_names.begin(), col_names.end(), column );
  if( it == col_names.end() ) {
    col_names.push_back( column );
    ++num_cols;
  }
 }
 
 /*---------------------------------------*/
 /*------------- READ ROWS ---------------*/
 /*---------------------------------------*/

 file.ignore( max, '\n' );

 file >> word;
 sec_reached( &current_section , word );
 
 while( current_section == LP_sections::LP_ROW ) {
  // All row name must end with a ":"
  pos = word.find( ":" );
  
  if( pos != -1 ) { // we actually found a new row
   is_row_q.push_back( false );
   ++num_rows;
   std::string row_name = word.substr( 0 , pos );
   row_names.push_back( row_name );
   }
 
  file >> word;
  char first_char = word[0];

  // we can read symbols until we get to the sign, i.e., we are reading variables
  // and coefficients
  while( first_char != '<' &&  first_char != '>' && first_char != '=' ) {
   std::string column;
   int len_word = word.length();
   bool read_sign = ( first_char == '-'  || first_char == '+' );

   // we can either read the sign, the coefficient or directly the 
   // variable ( i.e., the coefficient is 1)

   if( ( len_word == 1 && read_sign ) || word[0] == '[' ) {
    // We take into account the strange case where we don't have any linear
    // coefficient. Thus, no sign will be found before the quadratic part as
    // we expected.
    if( word[0] != '[' )
      file >> word; // Read next word after the sign
    
    if( word[0] == '[' ) {
      // we reached the quadratic part of the row.
      is_row_q[ num_rows - 1 ] = true; // Update row type
      
      file >> word;
      first_char = word[0];
      read_sign = ( first_char == '-'  || first_char == '+' );
      len_word = word.length();
      if( len_word == 1 && read_sign )
        file >> word; // Read next word after the sign
    }

    if( std::isdigit( word[0] ) )
      // we read the coefficient
      file >> column; // reading the variable name
    else
      column = word;
    }  
   else if( std::isdigit( first_char ) || read_sign ) {
    // we already read the coefficient
    file >> column; // reading the variable name
    }
   else{ // the only possibility left is that we read the variable name
    column = word;
    }

   // Options to check if we are in the quadratic part
   file.get(); // eat white space
   if( column[ column.length() - 2 ] == '^' ) {
    // We are reading the quadratic term x^2. Thus the real name of the
    // variable is obtained by removing the last two character
    column = column.substr( 0 , column.length() - 2 );
   }
   else if( file.peek() == '*' ) {
    // We read only the first term. In this first scan, simply skip to
    // the second
    file >> word;
   }

   // Now we have to check if the variable considered has been already found
   // in the objective function or in a precedent row
   auto it = std::find( col_names.begin(), col_names.end(), column );
   if( it == col_names.end() ) {
    col_names.push_back( column );
    ++num_cols;
    }

   file >> word;
   first_char = word[0];

   // Check if we reached the end of the quadratic part
   if( first_char == ']'){
    //Simply skip to the sense
    file >> word;
    first_char = word[0];
   }
  }

  // Now skip sense and rhs
  file >> word;
  file >> word;
  sec_reached( &current_section , word );
 }

 /*---------------------------------------*/
 /*---------- INITIALIZE STUFF -----------*/
 /*---------------------------------------*/

 v_coeff_pair lin_var;    // vector of (var-coeff) used to declare
                          // the linear function
 v_coeff_triple qd_var;  // vector of (var-lincoeff-quadcoeff) used 
                          // to declare a quadratic function with diagonal terms
 v_off_diag_term qod_var; // vector of (var-var-qcoeff) used to declare 
                          // a quadratic function with off diagonal terms
 
 // Vector used to store local active var in a certain constrain/objective
 std::vector< std::string > local_active_var; 

 rows = new std::vector< FRowConstraint >( num_rows );

 cols = new std::vector< ColVariable >( num_cols );
 bounds = new std::vector< BoxConstraint >( num_cols );

 for( int i = 0; i < num_cols; ++i ) {
  ( *bounds )[ i ].set_variable( &( *cols )[ i ], eNoMod );
  ( *bounds )[ i ].set_Block( this );
  ( *cols )[ i ].set_Block( this );
 }

 /*---------------------------------------*/
 /*------------ SECOND SCAN --------------*/
 /*---------------------------------------*/
 
 file.seekg( pos_start_objective, file.beg ); // Go back to objective section
 current_section = LP_sections::LP_LINOBJECTIVE;
 word = first_obj_word;

 while( current_section == LP_sections::LP_LINOBJECTIVE ) {
  std::string column;
  std::string value = "1";
  std::string value_sense = "+";
  ColVariable * v;
  
  char first_char = word[0];
  int len_word = word.length();
  bool read_sign = ( first_char == '-'  || first_char == '+' );

  // we can either read the sign, the coefficient or directly the 
  // variable ( i.e., the coefficient is 1). In Highs usually the
  // coefficient and the sign are grouped
  file.get(); // eat space
  if( file.peek() !=  '[' ) {
    if( len_word == 1 && read_sign ) {
    value_sense = word;
    file >> word; // reading the coefficient
    if( std::isdigit( word[0] ) ) {
      // we read the coefficient
      value = word;
      file >> column; // reading the variable name
    }
    else
      column = word;

    value = value_sense + value;
    }
    else if( std::isdigit( first_char ) || read_sign ) {
      // we already read the coefficient
      value = word;
      file >> column; // reading the variable name
    }
    else{ // the only possibility left is that we read the variable name
      value = std::to_string( 1 );
      column = word;
    }

    // Update active variable in the objective function
    local_active_var.push_back( column );

    auto it = std::find( col_names.begin(), col_names.end(), column );
    auto j = std::distance( col_names.begin(), it );
    v = &( *cols )[ j ];
    
    if( ! is_qp ) {
      // LinearFunction Modification
      lin_var.push_back( std::make_pair( v , dbl_val( value ) ) );
    }
    else{
      // DQuadFunction Modification (also consider a 
      // quadratic coefficient equal to 0)
      qd_var.push_back( std::make_tuple( v , dbl_val( value ) , 0 ) );
    }
  }
  file >> word;
  sec_reached( &current_section , word );
 }

 /*---------------------------------------*/
 /*------ READ QUADRATIC OBJECTIVE -------*/
 /*---------------------------------------*/

 while( current_section == LP_sections::LP_QUADOBJECTIVE ) {
  file >> word;

  std::string column;
  std::string column2;
  std::string value = "1";
  std::string value_sense = "+";
  ColVariable * v;
  ColVariable * v2;
  
  char first_char = word[0];
  int len_word = word.length();
  bool read_sign = ( first_char == '-'  || first_char == '+' );

  if( first_char == ']' ) {
    // we reached the end of the quadratic section.
    file.ignore( max, '\n' );

    file >> word;
    sec_reached( &current_section , word );

    break;
  }
  
  // we can either read the sign, the coefficient or directly the 
  // variable ( i.e., the coefficient is 1). In Highs usually the
  // coefficient and the sign are grouped.

  if( len_word == 1 && read_sign ) {
   value_sense = word;
   file >> word; // reading the coefficient
   if( std::isdigit( word[0] ) ) {
      // we read the coefficient
      value = word;
      file >> column; // reading the variable name
    }
    else
      column = word;
  
   value = value_sense + value;
  }
  else if( std::isdigit( first_char ) || read_sign ) {
    // we already read the coefficient
    value = word;
    file >> column; // reading the variable name
  }
  else{ // the only possibility left is that we read the variable name
    value = std::to_string( 1 );
    column = word;
  }

  // eat white space
  file.get();
  if( column[ column.length() - 2 ] == '^' ) {
    // We are reading the quadratic term x^2. Thus the real name of the
    // variable is obtained by removing the last two character
    column = column.substr( 0 , column.length() - 2 );

    // Map locally the column in the active variable for the objective
    auto it_local = std::find( local_active_var.begin(), local_active_var.end(), 
                                column );
    auto idx_local = std::distance( local_active_var.begin(), it_local );

    auto it = std::find( col_names.begin(), col_names.end(), column );
    auto j = std::distance( col_names.begin(), it );
    v = &( *cols )[ j ];

    if( it_local != local_active_var.end() ) {
      // Var v had already a linear coefficient set
      // DQuadFunction modification (nothing to be done on the linear term)
      std::get<2>( qd_var[idx_local] ) = dbl_val( value )/2;
    }
    else{
      // Var v doesn't have a linear coefficient
      // DQuadFunction modification (nothing to be done on the linear term)
      local_active_var.push_back( column );

      auto it_global = std::find( col_names.begin(), col_names.end(), column );
      auto idx_global = std::distance( col_names.begin(), it_global );
      v = &( *cols )[ idx_global ];

      qd_var.push_back( std::make_tuple( v , 0 , dbl_val( value )/2 ) );
    }
  }
  else if( file.peek() == '*' ) {
    // We read only the first term. Now skip the * and read the second
    file >> word; // *
    file >> column2;

    // We have to check that both variables are active locally (first var)
    auto it_local1 = std::find( local_active_var.begin(), local_active_var.end(), 
                                  column );
    auto idx_local1 = std::distance( local_active_var.begin(), it_local1 );

    // If one of the variable is not active locally, we have to insert it in the
    // set of active var of the constraint
    if( it_local1 == local_active_var.end() ) {
      // Map globally the column in the set of all the variable  
      auto it_global1 = std::find( col_names.begin(), col_names.end(), column );
      auto idx_global1 = std::distance( col_names.begin(), it_global1 );
      v = &( *cols )[ idx_global1 ];

      local_active_var.push_back( column );
      qd_var.push_back( std::make_tuple( v , 0 , 0 ) );
    }
    
    // We have to check that both variables are active locally (second var)
    auto it_local2 = std::find( local_active_var.begin(), local_active_var.end(), 
                                  column2 );
    auto idx_local2 = std::distance( local_active_var.begin(), it_local2 );

    // If one of the variable is not active locally, we have to insert it in the
    // set of active var of the constraint
    if( it_local2 == local_active_var.end() ) {
      // Map globally the column in the set of all the variable  
      auto it_global2 = std::find( col_names.begin(), col_names.end(), column2 );
      auto idx_global2 = std::distance( col_names.begin(), it_global2 );
      v2 = &( *cols )[ idx_global2 ];

      local_active_var.push_back( column2 );
      qd_var.push_back( std::make_tuple( v2 , 0 , 0 ) );
    }

    // NOTE: we store the indexes in a way that we are actually preserving only the 
    // lower traingul part of the matrix
    qod_var.push_back( std::make_tuple( std::max( idx_local1 , idx_local2 ) ,
                          std::min( idx_local1 , idx_local2 ) , dbl_val( value )/2 ) );
  }
  else{
    std::stringstream ss;
    ss << "Error while reading the quadratic part of objective function in" << 
    "AbstractBlock::read_lp(). Expected ^ (got " << column[ column.length() - 2 ] 
    << ") or * (got " << file.peek() << ")";

    std::string error = ss.str();
    throw( std::runtime_error( error ) );
  }
 }

 // Intizialize objective function
 if( is_qp == 0 ) {
  // Linear Function
  of->set_function( new LinearFunction( std::move( lin_var ) ) , eNoMod );
 }
 else{
  // Quadratic Function
  if( qod_var.size() == 0 )
    of->set_function( new DQuadFunction( std::move( qd_var ) ) , eNoMod );
  else
    of->set_function( new QuadFunction( std::move( qd_var ) , std::move( qod_var ) ), eNoMod );
 }

 of->set_sense( of_sense, eNoMod );
 
 /*---------------------------------------*/
 /*------------- READ ROWS ---------------*/
 /*---------------------------------------*/

 file.ignore( max, '\n' );

 file >> word;
 sec_reached( &current_section , word );
 
 while( current_section == LP_sections::LP_ROW ) {
  std::string row_name;
  std::string rhs;
   
  pos = word.find( ":" );
  row_name = word.substr( 0 , pos );
  auto it_row = std::find( row_names.begin(), row_names.end(), row_name );
  auto r = std::distance( row_names.begin(), it_row );

  // Reset vectors to store constraint information
  lin_var.clear();    // vector of (var-coeff) used to declare
                          // the linear function
  qd_var.clear();  // vector of (var-lincoeff-quadcoeff) used 
                          // to declare a quadratic function with diagonal terms
  qod_var.clear(); // vector of (var-var-qcoeff) used to declare 
                          // a quadratic function with off diagonal terms

  // Vector to map the active variable in a specific row
  local_active_var.clear();

  file >> word;
  char first_char = word[0];

  // we can read symbols until we get to the sign, i.e., we are reading variables
  // and coefficients
  while( first_char != '<' &&  first_char != '>' && first_char != '=' ) {
   std::string column;
   std::string column2;
   std::string value = "1";
   std::string value_sense = "+";
   ColVariable * v;
   ColVariable * v2;

   int len_word = word.length();
   bool read_sign = ( first_char == '-'  || first_char == '+' );

   // we can either read the sign, the coefficient or directly the 
   // variable ( i.e., the coefficient is 1)

   if( len_word == 1 && read_sign || word[0] == '[' ) {
    // We take into account the strange case where we don't have any linear
    // coefficient. Thus, no sign will be found before the quadratic part as
    // we expected.
    if( word[0] != '[' ) {
      value_sense = word;
      file >> word; // Read next word after the sign
    }

    if( word[0] == '[' ) {
      // we reached the quadratic part of the row.
      file >> word;
      first_char = word[0];
      read_sign = ( first_char == '-'  || first_char == '+' );
      len_word = word.length();
      if( len_word == 1 && read_sign ) {
        value_sense = word;
        file >> word; // Read next word after the sign
      }
    }

    if( std::isdigit( word[0] ) ) {
      // we read the coefficient
      value = word;
      file >> column; // reading the variable name
    }
    else
      column = word;

    value = value_sense + value;
   }  
   else if( std::isdigit( first_char ) || read_sign ) {
    // we already read the coefficient
    value = word;
    file >> column; // reading the variable name
   }
   else{ // the only possibility left is that we read the variable name
    value = std::to_string( 1 );
    column = word;
   }

   // Check if we are in the quadratic part!
   file.get(); // eat white space
   if( column[ column.length() - 2 ] == '^' ) {
    // We are reading the quadratic term x^2. Thus the real name of the
    // variable is obtained by removing the last two character
    column = column.substr( 0 , column.length() - 2 );
    
    // Map locally the column in the active variable for the row
    auto it_local = std::find( local_active_var.begin(), local_active_var.end(), 
                                column );
    auto idx_local = std::distance( local_active_var.begin(), it_local );

    if( it_local != local_active_var.end() ) {
      // Var v had already a linear coefficient set
      // DQuadFunction modification (nothing to be done on the linear term)
      std::get<2>( qd_var[idx_local] ) = dbl_val( value );
    }
    else{
      // Var v doesn't have a linear coefficient
      // DQuadFunction modification (nothing to be done on the linear term)
      // Map globally the column in the set of all the variable  
      auto it_global = std::find( col_names.begin(), col_names.end(), column );
      auto idx_global = std::distance( col_names.begin(), it_global );
      v = &( *cols )[ idx_global ];

      local_active_var.push_back( column ); // Update set of local active var
      qd_var.push_back( std::make_tuple( v , 0 , dbl_val( value ) ) );
    }
   }
   else if( file.peek() == '*' ) {
    // We read only the first term. Now skip the * and read the second
    file >> word; // *
    file >> column2;

    // We have to check that both variables are active locally (first var)
    auto it_local1 = std::find( local_active_var.begin(), local_active_var.end(), 
                                  column );
    auto idx_local1 = std::distance( local_active_var.begin(), it_local1 );

    // If one of the variable is not active locally, we have to insert it in the
    // set of active var of the constraint
    if( it_local1 == local_active_var.end() ) {
      // Map globally the column in the set of all the variable  
      auto it_global1 = std::find( col_names.begin(), col_names.end(), column );
      auto idx_global1 = std::distance( col_names.begin(), it_global1 );
      v = &( *cols )[ idx_global1 ];

      local_active_var.push_back( column );
      qd_var.push_back( std::make_tuple( v , 0 , 0 ) );
    }
    
    // We have to check that both variables are active locally (second var)
    auto it_local2 = std::find( local_active_var.begin(), local_active_var.end(), 
                                  column2 );
    auto idx_local2 = std::distance( local_active_var.begin(), it_local2 );

    // If one of the variable is not active locally, we have to insert it in the
    // set of active var of the constraint
    if( it_local2 == local_active_var.end() ) {
      // Map globally the column in the set of all the variable  
      auto it_global2 = std::find( col_names.begin(), col_names.end(), column2 );
      auto idx_global2 = std::distance( col_names.begin(), it_global2 );
      v2 = &( *cols )[ idx_global2 ];

      local_active_var.push_back( column2 );
      qd_var.push_back( std::make_tuple( v2 , 0 , 0 ) );
    }

    // QuadFunction modification
    // NOTE: we store the indexes in a way that we are actually preserving only the 
    // lower traingul part of the matrix
    qod_var.push_back( std::make_tuple( std::max( idx_local1 , idx_local2 ) ,
                          std::min( idx_local1 , idx_local2 ) , dbl_val( value ) ) );
   }
   else{
    // We read a simple linear coefficient

    // In the linear part we expect never to find a variable that was already active in the 
    // scanned constraint.
    local_active_var.push_back( column );

    auto it_global = std::find( col_names.begin(), col_names.end(), column );
    auto idx_global = std::distance( col_names.begin(), it_global );
    v = &( *cols )[ idx_global ];

    if( ! is_row_q[ r ] ) {
      // LinearFunction Modification
      lin_var.push_back( std::make_pair( v , dbl_val( value ) ) );
    }
    else{
      // DQuadFunction Modification (also consider a 
      // quadratic coefficient equal to 0)
      qd_var.push_back( std::make_tuple( v , dbl_val( value ) , 0 ) );
    }
   }

   file >> word;
   first_char = word[0];

   // Check if we reached the end of the quadratic part
   if( first_char == ']'){
    //Simply skip to the sense
    file >> word;
    first_char = word[0];
   }
  }
  
  // Now we should be reading the rhs
  file >> rhs;
  auto & row = (*rows)[ r ];

  // Initialize row with data collected
  if( ! is_row_q[ r ] )
    row.set_function( new LinearFunction( std::move( lin_var ) ) , eNoMod );
  else
    row.set_function( new QuadFunction( std::move( qd_var ) , std::move( qod_var ) ), eNoMod );
  
  row.set_Block( this );

  switch( first_char ) {
   case '<' :
    // G: -inf =< f() =< rhs
    row.set_lhs( - Inf< double >(), eNoMod );
    row.set_rhs( dbl_val( rhs ), eNoMod );
    break;

   case '>' :
    // L: rhs =< f() =< +inf
    row.set_lhs( dbl_val( rhs ), eNoMod );
    row.set_rhs( Inf< double >(), eNoMod );
    break;

   case '=' :
    // E (no range): rhs =< f() =< rhs
    row.set_both( dbl_val( rhs ), eNoMod );
    break;

   default:
    throw( std::invalid_argument( "Invalid row sense in" 
        " LP file" ) );
   }

  file >> word;
  sec_reached( &current_section , word );
  }

 /*---------------------------------------*/
 /*------------- READ BOUNDS -------------*/
 /*---------------------------------------*/
 
 if( current_section == LP_sections::LP_BOUND ) {
  file >> word;
  sec_reached( &current_section , word );
 }
 
 // In this case we have to control both for the general and binary section,
 // because they can come in any order.
 while( current_section == LP_sections::LP_BOUND ) {
  
  std::string column;
  std::string lhs_value = "0"; // default lhs value in .lp file
  std::string rhs_value = "infinity"; // default rhs value in .lp file
  
  char first_char = word[0];
  
  if( std::isdigit( first_char ) || first_char == '-' || first_char == '.' ) {
   // we read the lhs
   lhs_value = word;
   file >> word; // we can skip the <=
   file >> column;
   }
  else // we should have found the variable
   column = word;

  file >> word; // We expect to be reading the sense
  first_char = word[0];
  
  if( first_char == '<' ) { // now reading rhs
   file >> rhs_value;
   file >> word;
  }
  else if( first_char == '>' ) { // now reading lhs
   file >> lhs_value;
   file >> word;
  }
  else if( first_char == '=' ) { // reading both
   file >> rhs_value;
   lhs_value = rhs_value;
   file >> word;
  }
  else if( boost::iequals( word , "free" ) ) { // free variable
   lhs_value = "-infinity";
   file >> word;
  }

  auto it = std::find( col_names.begin(), col_names.end(), column );
  if( it != col_names.end() ) {
   auto j = std::distance( col_names.begin(), it );
   auto & b = ( *bounds )[ j ];
   auto & c = ( *cols )[j];
   b.set_lhs( dbl_val( lhs_value ), eNoMod );
   b.set_rhs( dbl_val( rhs_value ), eNoMod );
   
   if( lhs_value == rhs_value )
    c.is_fixed( true, eNoMod );

   } 
  else
   throw( std::invalid_argument( "Invalid syntax in LP file" ) );

  sec_reached( &current_section , word );
  }

 /*---------------------------------------*/
 /*-------------- READ TYPES -------------*/
 /*---------------------------------------*/

 file >> word;
 sec_reached( &current_section , word );
 while( current_section == LP_sections::LP_GENERAL || 
         current_section == LP_sections::LP_BINARY ) {
   
   std::string column;   
   column = word; // read new variable
   auto it = std::find( col_names.begin(), col_names.end(), column );
   if( it != col_names.end() ) {
    auto j = std::distance( col_names.begin(), it );
    auto & c = ( *cols )[j];
    if( current_section == LP_sections::LP_GENERAL )
     c.set_type( ColVariable::kInteger, eNoMod );
    else // Binary
     c.set_type( ColVariable::kBinary, eNoMod );
    }
   else // we already switched to a new section
    sec_reached( &current_section , column );
   
   // In any case, now we can read a new word and update the section
   file >> word;
   sec_reached( &current_section , word );
   }

 /*---------------------------------------*/
 /*------- READ REMAINING SECTIONS -------*/
 /*---------------------------------------*/

 if( current_section == LP_sections::LP_SEMI_CON ) {
  // TODO
 }
 else if( current_section == LP_sections::LP_SOS ) {
  // TODO
 }
 else if( current_section == LP_sections::LP_END ) {
   // Nothing to do
 }
 else{
   throw( std::invalid_argument( "Invalid syntax in LP file" ) );
 }

 // Reset and set abstract representation
 reset_static_constraints();
 reset_static_variables();
 reset_objective();

 set_objective( of, eNoMod );
 add_static_variable( *cols );
 add_static_constraint( *rows );
 add_static_constraint( *bounds );

 // these three containers are ours, and the groups are told how to dispose
 // of them, so that the destructor does not have to know their type
 own_storage( get_static_variable_groups().back() , cols );
 own_storage( get_static_constraint_groups()[
			  get_static_constraint_groups().size() - 2 ] , rows );
 own_storage( get_static_constraint_groups().back() , bounds );

 // Issue the NBModification
 if( anyone_there() )
   add_Modification( std::make_shared< NBModification >( this ) );

 }  // end( AbstractBlock::read_lp )

/*--------------------------------------------------------------------------*/

 void AbstractBlock::sec_reached( int * actual_sec , std::string word ) {

  /* Here are listed all of the possible names of each section of a .lp file. */
 const std::vector< std::string > QOBJ_SECTION = { "[" };

 const std::vector< std::string > ROW_SECTION = { "subject" , "such" , 
                              "st" , "S.T." , "ST." };
 
 const std::vector< std::string > BOUND_SECTION = { "bounds" , "bound" };

 const std::vector< std::string > GENERAL_SECTION = { "general" , "generals" ,
                              "gen" };

 const std::vector< std::string > BINARY_SECTION = { "binary" , "binaries" ,
                              "bin" };

 const std::vector< std::string > SEMI_CON_SECTION = { "semi-continuos" , "semi" ,
                              "semis" };           

 const std::vector< std::string > SOS_SECTION = { "sos" }; 

 const std::vector< std::string > END_SECTION = { "end" };

 // Struct used to return true if the actual word read is in the specific 
 // following section.
 struct compare_section
 {
    std::string key_s;
    compare_section(std::string const &s): key_s(s) {}
 
    bool operator()(std::string const &s) {
        return boost::iequals( s , key_s);
    }
 };

 assert( *actual_sec <= LP_sections::LP_END );
 
 switch( *actual_sec ) {
   case( LP_sections::LP_LINOBJECTIVE ) :
    if( std::any_of( QOBJ_SECTION.begin() , QOBJ_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the quadratic objective section
      *actual_sec = LP_sections::LP_QUADOBJECTIVE;
      return;
    }
    case( LP_sections::LP_QUADOBJECTIVE ) :
     if( std::any_of( ROW_SECTION.begin() , ROW_SECTION.end() ,
                      compare_section( word ) ) ) {
      // we reached the row section
      *actual_sec = LP_sections::LP_ROW;
      return;
    }
    // In any case, there is no reason to continue searching because 
    // the row section is mandatory
    break;
   case( LP_sections::LP_ROW ) :
    if( std::any_of( BOUND_SECTION.begin() , BOUND_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the bound section
      *actual_sec = LP_sections::LP_BOUND;
      return;
    }
   case( LP_sections::LP_BOUND ) :
   case( LP_sections::LP_GENERAL ) :
   case( LP_sections::LP_BINARY ) :
    if( std::any_of( GENERAL_SECTION.begin() , GENERAL_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the general section
      *actual_sec = LP_sections::LP_GENERAL;
      return;
    }
    if( std::any_of( BINARY_SECTION.begin() , BINARY_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the general section
      *actual_sec = LP_sections::LP_BINARY;
      return;
    }
    if( std::any_of( SEMI_CON_SECTION.begin() , SEMI_CON_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the semi-continuous section
      *actual_sec = LP_sections::LP_SEMI_CON;
      return;
    }
   case( LP_sections::LP_SEMI_CON ) :
    if( std::any_of( SOS_SECTION.begin() , SOS_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the sos section
      *actual_sec = LP_sections::LP_SOS;
      return;
    }
   case( LP_sections::LP_SOS ) :
    if( std::any_of( END_SECTION.begin() , END_SECTION.end() ,
                     compare_section( word ) ) ) {
      // we reached the sos section
      *actual_sec = LP_sections::LP_END;
      return;
    }
    break;
   case( LP_sections::LP_END ) :
    break; // nothing to do
   default : 
    throw( std::logic_error(
       "Error in reading LP file" ) );
  }

 // No new section has been reached. Nothing to do.
 return;
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::guts_of_deserialize( const netCDF::NcGroup & group )
{
 // first deserialise the abstract representation out of a LP or MPS file
 auto mod = group.getVar( "Model" );
 
 // check if the LP/MPS representation of the block is provided 
 if( ! mod.isNull() ) {
  // Prepare the stream of the file to be read
  std::string str;
  mod.getVar( {0} , &str );
  std::istringstream file( str.data() );

  /* Get the format of the file provided. Possible values for this attribute
  *  are:
  *    'L' if we have to read the file from an .lp file; 
  *    'M' if we have to read the file from an .mps file;
  *  
  *  If no format is provided, we suppose that the file is provided in a .lp 
  *  format. */
  char type;
  auto gtype = mod.getAtt( "ModelType" );
  if( gtype.isNull() )
   type = 'L';
  else
   gtype.getValues( &type );

  if( type == 'L' )
   read_lp( file ); // read the .lp file
  else
   read_mps( file ); // read the .mps file
  }

 // deserialize the "abstract only inner Block"
 netCDF::NcDim nib = group.getDim( "NumberInnerBlock" );
 if( nib.isNull() )
  return;

 auto nibs = nib.getSize();

 if( v_Block.size() < nibs )
  v_Block.resize( nibs, nullptr );

 for( auto i = get_first_inner_Block() ; i < nibs ; ++i ) {
  auto bi = group.getGroup( "Block_" + std::to_string( i ) );
  if( bi.isNull() )
   throw( std::invalid_argument( "inner Block not found" ) );
  v_Block[ i ] = new_Block( bi );
  }
 }  // end( AbstractBlock::guts_of_deserialize )

/*--------------------------------------------------------------------------*/

#ifndef NDEBUG

std::vector< std::string > AbstractBlock::expected_dims( void ) const {
 auto ret = Block::expected_dims();
 ret.push_back( "NumberInnerBlock" );

 return( ret );
 }

/*--------------------------------------------------------------------------*/

std::vector< std::string > AbstractBlock::expected_vars( void ) const {
 auto ret = Block::expected_vars();
 ret.push_back( "Model" );

 return( ret );
 }

#endif


/*--------------------------------------------------------------------------*/
/*--------------------- MIRRORING ANOTHER Block ----------------------------*/
/*--------------------------------------------------------------------------*/

/* The copy of a group is made by the group itself, which knows the type of
 * the container it views and can therefore say how to build another one of
 * the same shape in the copy; what is left to do here is to pair the objects
 * of the two, which come in the same order, the storage order being what a
 * group promises [see BaseGroup]. */

template< class C , class F >
static bool mirror_elements( const BaseGroup & src , const BaseGroup & dst ,
                             F f )
{
 if( ! src.elements_are< C >() )
  return( false );  // the group is of another type, the caller tries on

 std::vector< C * > copy;
 copy.reserve( dst.get_num_elements() );
 dst.for_each_as< C >( [ & copy ]( C & d ) { copy.push_back( & d ); } );

 Block::Index i = 0;
 src.for_each_as< C >( [ & f , & copy , & i ]( C & o ) {
   if( i < copy.size() )
    f( o , * copy[ i ] );
   ++i;
   } );

 return( true );
 }

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

bool AbstractBlock::check_count( std::size_t src , std::size_t dst ,
                                 const std::string & what )
{
 if( src == dst )
  return( true );

 v_issues.push_back( what + " holds " + std::to_string( src ) +
                     " objects and its copy " + std::to_string( dst ) +
                     ": the shape of the group is one the copy does not "
                     "reproduce" );
 return( true );   // the group was of that type all the same
 }

/*--------------------------------------------------------------------------*/

Function * AbstractBlock::mirror_Function( const Function * fnct )
{
 if( ! fnct )
  return( nullptr );

 auto var_of = [ this ]( ColVariable * v ) -> ColVariable * {
  auto it = f_v_map.find( v );
  return( it != f_v_map.end() ? it->second : v );
  };

 if( auto lf = dynamic_cast< const LinearFunction * >( fnct ) ) {
  LinearFunction::v_coeff_pair cp;
  cp.reserve( lf->get_v_var().size() );
  for( auto & p : lf->get_v_var() )
   cp.emplace_back( var_of( p.first ) , p.second );
  return( new LinearFunction( std::move( cp ) , lf->get_constant_term() ) );
  }

 if( auto qf = dynamic_cast< const DQuadFunction * >( fnct ) ) {
  DQuadFunction::v_coeff_triple ct;
  ct.reserve( qf->get_v_var().size() );
  for( auto & tr : qf->get_v_var() )
   ct.emplace_back( var_of( std::get< 0 >( tr ) ) , std::get< 1 >( tr ) ,
                    std::get< 2 >( tr ) );
  return( new DQuadFunction( std::move( ct ) , qf->get_constant_term() ) );
  }

 return( nullptr );   // a Function that cannot be written on other Variable
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::mirror_variables( Block * src , AbstractBlock * dst )
{
 /* The type of a ColVariable carries everything about it but the fixing,
  * which is a state of its own and goes with the value it fixes it at. */

 auto take = [ this , dst ]( ColVariable & s , ColVariable & d ) {
  d.set_Block( dst );
  d.set_type( s.get_type() , eNoMod );
  d.set_value( s.get_value() );
  if( s.is_fixed() )
   d.is_fixed( true , eNoMod );
  f_v_map[ & s ] = & d;
  f_v_rmap[ & d ] = & s;
  };

 // each group of the original makes a group of the copy, of the same type
 // and shape, and the objects are paired in storage order
 auto mirror_group = [ & ]( const BaseGroup & group , bool dynamic ) {
  const std::string what = std::string( dynamic ? "dynamic" : "static" ) +
                           " Variable group " +
                           std::to_string( group.get_index() ) + " of " +
                           src->name();

  if( ! group.clone_into( dst , std::string( group.get_name() ) ) ) {
   v_issues.push_back( what + " is one the copy cannot make, it is empty "
                       "in the copy" );
   dynamic ? dst->add_dynamic_variable() : dst->add_static_variable();
   return;
   }

  const auto & copy = dynamic ? dst->get_dynamic_variable_groups().back()
                              : dst->get_static_variable_groups().back();

  if( ! mirror_elements< ColVariable >( group , *copy , take ) )
   v_issues.push_back( what + " is not made of ColVariable" );
  else
   check_count( group.get_num_elements() , copy->get_num_elements() , what );
  };

 for( const auto & group : src->get_static_variable_groups() )
  if( group )
   mirror_group( *group , false );
  else
   dst->add_static_variable();

 for( const auto & group : src->get_dynamic_variable_groups() )
  if( group )
   mirror_group( *group , true );
  else
   dst->add_dynamic_variable();

 // the inner Block: a Constraint of any of them may use their Variable
 for( Index i = 0 ; i < src->get_number_nested_Blocks() ; ++i ) {
  auto inner = new AbstractBlock( dst );
  inner->set_name( std::string( src->get_nested_Block( i )->name() ) );
  dst->add_nested_Block( inner );
  mirror_variables( src->get_nested_Block( i ) , inner );
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::mirror_constraints( Block * src , AbstractBlock * dst )
{
 auto var_of = [ this ]( ColVariable * v ) -> ColVariable * {
  auto it = f_v_map.find( v );
  return( it != f_v_map.end() ? it->second : v );
  };

 /* A FRowConstraint whose Function cannot be written on the Variable of the
  * copy is left there with an empty Function and free sides, i.e., as the
  * row that says nothing: the copy is then a relaxation of the original, and
  * the issue says which row made it one. */

 auto take_frow = [ & ]( FRowConstraint & s , FRowConstraint & d ) {
  d.set_Block( dst );
  auto nf = mirror_Function( s.get_function() );
  if( ! nf ) {
   v_issues.push_back( "the Function of a FRowConstraint of " + src->name() +
                       " cannot be written on other Variable, the row is "
                       "relaxed" );
   d.set_function( new LinearFunction() , eNoMod );
   d.set_lhs( -Inf< RowConstraint::RHSValue >() , eNoMod );
   d.set_rhs( Inf< RowConstraint::RHSValue >() , eNoMod );
   return;
   }
  d.set_function( nf , eNoMod );
  d.set_lhs( s.get_lhs() , eNoMod );
  d.set_rhs( s.get_rhs() , eNoMod );
  f_c_map[ & s ] = & d;
  };

 /* The OneVarConstraint family: the concrete type is what says which sides
  * the constraint has, so the copy is of the very same type and only the
  * sides that the type does not fix are set. */

 auto take_one = [ & ]( OneVarConstraint & s , OneVarConstraint & d ) {
  d.set_Block( dst );
  d.set_variable( var_of( static_cast< ColVariable * >(
                                         s.get_active_var( 0 ) ) ) , eNoMod );
  f_c_map[ & s ] = & d;
  };

 auto box = [ & ]( BoxConstraint & s , BoxConstraint & d ) {
  take_one( s , d );
  d.set_lhs( s.get_lhs() , eNoMod );
  d.set_rhs( s.get_rhs() , eNoMod );
  };
 auto lb0 = [ & ]( LB0Constraint & s , LB0Constraint & d ) {
  take_one( s , d ); d.set_rhs( s.get_rhs() , eNoMod ); };
 auto ub0 = [ & ]( UB0Constraint & s , UB0Constraint & d ) {
  take_one( s , d ); d.set_lhs( s.get_lhs() , eNoMod ); };
 auto lbc = [ & ]( LBConstraint & s , LBConstraint & d ) {
  take_one( s , d ); d.set_lhs( s.get_lhs() , eNoMod ); };
 auto ubc = [ & ]( UBConstraint & s , UBConstraint & d ) {
  take_one( s , d ); d.set_rhs( s.get_rhs() , eNoMod ); };
 auto nnc = [ & ]( NNConstraint & s , NNConstraint & d ) { take_one( s , d ); };
 auto npc = [ & ]( NPConstraint & s , NPConstraint & d ) { take_one( s , d ); };
 auto zoc = [ & ]( ZOConstraint & s , ZOConstraint & d ) { take_one( s , d ); };

 // the concrete type of a group says which of the lambdas above applies to
 // it; the first that claims it is the one, as the group is homogeneous
 auto copy_rows = [ & ]( const BaseGroup & s , const BaseGroup & d ) {
  return( mirror_elements< FRowConstraint >( s , d , take_frow ) ||
	  mirror_elements< BoxConstraint >( s , d , box ) ||
	  mirror_elements< LB0Constraint >( s , d , lb0 ) ||
	  mirror_elements< UB0Constraint >( s , d , ub0 ) ||
	  mirror_elements< LBConstraint >( s , d , lbc ) ||
	  mirror_elements< UBConstraint >( s , d , ubc ) ||
	  mirror_elements< NNConstraint >( s , d , nnc ) ||
	  mirror_elements< NPConstraint >( s , d , npc ) ||
	  mirror_elements< ZOConstraint >( s , d , zoc ) );
  };

 auto mirror_group = [ & ]( const BaseGroup & group , bool dynamic ) {
  const std::string what = std::string( dynamic ? "dynamic" : "static" ) +
                           " Constraint group " +
                           std::to_string( group.get_index() ) + " of " +
                           src->name();

  if( ! group.clone_into( dst , std::string( group.get_name() ) ) ) {
   v_issues.push_back( what + " is one the copy cannot make, it is empty "
                       "in the copy" );
   dynamic ? dst->add_dynamic_constraint() : dst->add_static_constraint();
   return;
   }

  const auto & copy = dynamic ? dst->get_dynamic_constraint_groups().back()
                              : dst->get_static_constraint_groups().back();

  if( ! copy_rows( group , *copy ) )
   v_issues.push_back( what + " is of a type the mirror does not know, the "
                       "group is empty in the copy" );
  else
   check_count( group.get_num_elements() , copy->get_num_elements() , what );
  };

 for( const auto & group : src->get_static_constraint_groups() )
  if( group )
   mirror_group( *group , false );
  else
   dst->add_static_constraint();

 for( const auto & group : src->get_dynamic_constraint_groups() )
  if( group )
   mirror_group( *group , true );
  else
   dst->add_dynamic_constraint();

 // the Objective, which the copy has only if the original has one
 if( auto obj = src->get_objective() ) {
  auto fobj = dynamic_cast< FRealObjective * >( obj );
  Function * nf = fobj ? mirror_Function( fobj->get_function() ) : nullptr;
  if( nf ) {
   auto nobj = new FRealObjective( dst , nf );
   nobj->set_sense( obj->get_sense() , eNoMod );
   dst->set_objective( nobj , eNoMod );
   }
  else
   v_issues.push_back( "the Objective of " + src->name() + " cannot be "
                       "written on other Variable, the copy has none" );
  }

 for( Index i = 0 ; i < src->get_number_nested_Blocks() ; ++i )
  mirror_constraints( src->get_nested_Block( i ) ,
                      static_cast< AbstractBlock * >(
                                              dst->get_nested_Block( i ) ) );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::mirror( Block * blck )
{
 static const std::string _prfx = "AbstractBlock::mirror: ";

 if( ! blck )
  throw( std::invalid_argument( _prfx + "no Block to mirror" ) );

 if( f_mirrored )
  throw( std::logic_error( _prfx + "this AbstractBlock mirrors one already" ) );

 if( get_number_static_variables() || get_number_dynamic_variables() ||
     get_number_static_constraints() || get_number_dynamic_constraints() ||
     get_number_nested_Blocks() || get_objective() )
  throw( std::logic_error( _prfx + "the AbstractBlock is not empty" ) );

 f_mirrored = blck;

 /* Two passes over the subtree: a Constraint of any of its Block can be
  * written in the Variable of any other, hence every Variable has to have
  * its copy before the first Constraint is copied. */

 mirror_variables( blck , this );
 mirror_constraints( blck , this );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::mirror_read( void )
{
 for( auto & el : f_v_map )
  el.second->set_value( el.first->get_value() );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::mirror_write( void )
{
 for( auto & el : f_v_map )
  const_cast< ColVariable * >( el.first )->set_value( el.second->get_value() );
 }

/*--------------------------------------------------------------------------*/

bool AbstractBlock::mirror_Function_changed( const Function * fnct )
{
 /* Which Constraint, or Objective, the Function belongs to is not said by
  * the Modification: it is the Observer of the Function. The copy of the
  * Function is rebuilt whole, which costs the length of the row and spares
  * the reading of which coefficients the Modification carries, there being
  * one such Modification per change and not per coefficient. */

 if( ! fnct )
  return( false );

 auto obs = fnct->get_Observer();

 if( auto cns = dynamic_cast< const FRowConstraint * >( obs ) ) {
  auto dst = dynamic_cast< FRowConstraint * >( mirror_of( cns ) );
  if( ! dst )
   return( false );
  auto nf = mirror_Function( fnct );
  if( ! nf )
   return( false );
  dst->set_function( nf , eNoMod , true );
  return( true );
  }

 if( dynamic_cast< const FRealObjective * >( obs ) ) {
  auto dst = dynamic_cast< FRealObjective * >( get_objective() );
  if( ! dst )
   return( false );
  auto nf = mirror_Function( fnct );
  if( ! nf )
   return( false );
  dst->set_function( nf , eNoMod , true );
  return( true );
  }

 return( false );
 }

/*--------------------------------------------------------------------------*/

bool AbstractBlock::mirror_forward_Modification( c_p_Mod mod )
{
 if( ( ! f_mirrored ) || ( ! mod ) )
  return( false );

 if( auto gm = dynamic_cast< const GroupModification * >( mod ) ) {
  for( auto & sm : gm->sub_Modifications() )
   if( ! mirror_forward_Modification( sm.get() ) )
    return( false );
  return( true );
  }

 // the coefficients of a Function, whatever kind of change it is
 if( auto fm = dynamic_cast< const FunctionMod * >( mod ) )
  return( mirror_Function_changed( fm->function() ) );

 // a side of a RowConstraint
 if( auto cm = dynamic_cast< const RowConstraintMod * >( mod ) ) {
  auto src = dynamic_cast< RowConstraint * >( cm->constraint() );
  if( ! src )
   return( false );
  auto dst = mirror_of( src );
  if( ! dst )
   return( false );

  switch( cm->type() ) {
   case( RowConstraintMod::eChgLHS ):
    dst->set_lhs( src->get_lhs() , eNoMod );
    break;
   case( RowConstraintMod::eChgRHS ):
    dst->set_rhs( src->get_rhs() , eNoMod );
    break;
   case( RowConstraintMod::eChgBTS ):
    dst->set_lhs( src->get_lhs() , eNoMod );
    dst->set_rhs( src->get_rhs() , eNoMod );
    break;
   default:
    return( false );
   }

  return( true );
  }

 // the type or the fixing of a ColVariable
 if( auto vm = dynamic_cast< const VariableMod * >( mod ) ) {
  auto src = dynamic_cast< const ColVariable * >( vm->variable() );
  if( ! src )
   return( false );
  auto dst = mirror_of( src );
  if( ! dst )
   return( false );
  dst->set_type( src->get_type() , eNoMod );
  dst->is_fixed( src->is_fixed() , eNoMod );
  if( src->is_fixed() )
   dst->set_value( src->get_value() );
  return( true );
  }

 // the sense of the Objective
 if( auto om = dynamic_cast< const ObjectiveMod * >( mod ) ) {
  auto dst = get_objective();
  if( ( ! dst ) || ( ! om->of() ) )
   return( false );
  dst->set_sense( om->of()->get_sense() , eNoMod );
  return( true );
  }

 return( false );   // a change of the shape, or one this does not know
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

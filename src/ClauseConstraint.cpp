/*--------------------------------------------------------------------------*/
/*----------------------- File ClauseConstraint.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the ClauseConstraint class.
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

#include <unordered_set>

#include "Block.h"
#include "ClauseConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

ClauseConstraint::~ClauseConstraint()
{
 for( auto & literal : v_literals )
  literal.first->remove_active( this );
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::check_literals( c_v_Literal & literals ,
				       bool with_current ) const
{
 std::unordered_set< const BooleanVariable * > seen;
 if( with_current )
  for( const auto & literal : v_literals )
   seen.insert( literal.first );

 for( const auto & literal : literals ) {
  if( ! literal.first )
   throw( std::invalid_argument(
		      "ClauseConstraint::check_literals: null BooleanVariable" ) );
  if( ! seen.insert( literal.first ).second )
   throw( std::invalid_argument( "ClauseConstraint::check_literals: "
				 "a BooleanVariable appears twice" ) );
  }
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::issue( int type , ModParam issueMod )
{
 if( ( ! get_Block() ) || ( ! get_Block()->issue_mod( issueMod ) ) )
  return;

 get_Block()->add_Modification( std::make_shared< ClauseConstraintMod >(
			     this , type , Observer::par2concern( issueMod ) ) ,
			    Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::set_literals( v_Literal && literals ,
				     ModParam issueMod )
{
 check_literals( literals , false );

 for( auto & literal : v_literals )
  literal.first->remove_active( this );

 v_literals = std::move( literals );

 for( auto & literal : v_literals )
  literal.first->add_active( this );

 issue( ClauseConstraintMod::eLiteralsChanged , issueMod );
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::add_literals( v_Literal && literals ,
				     ModParam issueMod )
{
 if( literals.empty() )
  return;

 check_literals( literals , true );

 for( auto & literal : literals ) {
  literal.first->add_active( this );
  v_literals.push_back( literal );
  }

 issue( ClauseConstraintMod::eLiteralsAdded , issueMod );
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::remove_variable( Index i , ModParam issueMod )
{
 if( i >= v_literals.size() )
  throw( std::invalid_argument(
		 "ClauseConstraint::remove_variable: wrong Variable index" ) );

 v_literals[ i ].first->remove_active( this );
 v_literals.erase( v_literals.begin() + i );

 issue( ClauseConstraintMod::eLiteralsRemoved , issueMod );
 }

/*--------------------------------------------------------------------------*/

int ClauseConstraint::compute( bool changedvars )
{
 f_num_true = std::count_if( v_literals.begin() , v_literals.end() ,
			     literal_value );
 return( kOK );
 }

/*--------------------------------------------------------------------------*/

void ClauseConstraint::print( std::ostream & output ) const
{
 output << "ClauseConstraint [" << this << "] of Block [" << get_Block()
	<< "] with " << v_literals.size() << " literals:";
 for( const auto & literal : v_literals )
  output << " " << ( literal.second ? "~" : "" ) << literal.first;
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File ClauseConstraint.cpp ----------------------*/
/*--------------------------------------------------------------------------*/

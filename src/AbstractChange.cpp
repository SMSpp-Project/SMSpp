/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractChange.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the class AbstractChange, a Change of the abstract
 * representation of a Block.
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Filippo Magi, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractChange.h"

#include "ColVariable.h"

#include "DQuadFunction.h"

#include "FRealObjective.h"

#include "LinearFunction.h"

#include "OneVarConstraint.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

using Index = Block::Index;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register AbstractChange to the Change factory

SMSpp_insert_in_factory_cpp_0( AbstractChange );

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS OF AbstractChange ---------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR LOADING, PRINTING & SAVING --------------------*/
/*--------------------------------------------------------------------------*/

void AbstractChange::deserialize( const netCDF::NcGroup & group )
{
 auto type = group.getAtt( "AbstractChange_type" );
 if( type.isNull() )
  throw( std::invalid_argument( "AbstractChange::deserialize: the "
				"AbstractChange_type attribute is missing" ) );
 type.getValues( & f_type );

 auto data = group.getVar( "Data" );
 if( data.isNull() )
  v_data.clear();
 else {
  v_data.resize( group.getDim( "dim" ).getSize() );
  data.getVar( v_data.data() );
  }

 auto paths = group.getGroup( "VariablesPath" );
 if( paths.isNull() )
  v_paths.clear();
 else
  v_paths = AbstractPath::vector_deserialize( paths );

 }  // end( AbstractChange::deserialize )

/*--------------------------------------------------------------------------*/

void AbstractChange::serialize( netCDF::NcGroup & group ) const
{
 Change::serialize( group );  // always call the method of the base class

 group.putAtt( "AbstractChange_type" , netCDF::NcInt() , f_type );

 if( ! v_data.empty() ) {
  auto dim = group.addDim( "dim" , v_data.size() );
  ( group.addVar( "Data" , netCDF::NcDouble() , dim ) ).putVar(
							      v_data.data() );
  }

 if( ! v_paths.empty() ) {
  auto paths = group.addGroup( "VariablesPath" );
  AbstractPath::serialize( v_paths , paths );
  }

 }  // end( AbstractChange::serialize )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

Change * AbstractChange::apply( Block * block , bool doUndo ,
				ModParam issueMod , ModParam issueAMod )
{
 // the ColVariable of the first path, which most types act upon
 auto variable = [ & ]( void ) {
  if( v_paths.empty() )
   throw( std::invalid_argument( "AbstractChange::apply: no path" ) );
  auto pv = v_paths[ 0 ].get_element< ColVariable >( block );
  if( ! pv )
   throw( std::invalid_argument( "AbstractChange::apply: the path is not "
				 "that of a ColVariable" ) );
  return( pv );
  };

 // the first datum, which most types need
 auto datum = [ & ]( void ) {
  if( v_data.empty() )
   throw( std::invalid_argument( "AbstractChange::apply: no data" ) );
  return( v_data[ 0 ] );
  };

 const auto undo = [ & ]( int type , std::vector< double > data ) {
  return( new AbstractChange( type , std::move( data ) , v_paths ) );
  };

 Change * ret = nullptr;

 switch( f_type ) {
  case( eChgObj ): {
   ColVariable * pv = nullptr;
   Function * fobj = nullptr;
   if( v_paths.size() != 2 )
    throw( std::invalid_argument( "AbstractChange::apply: eChgObj needs "
				  "the paths of a variable and of the "
				  "objective" ) );
   for( const auto & path : v_paths ) {
    const auto type = path.get_last_node( block ).type;
    if( ( type == 'V' ) || ( type == 'v' ) )
     pv = path.get_element< ColVariable >( block );
    else
     if( type == 'O' )
      if( auto obj = dynamic_cast< FRealObjective * >(
				       path.get_element< Objective >( block ) ) )
       fobj = obj->get_function();
    }
   if( ( ! pv ) || ( ! fobj ) )
    throw( std::invalid_argument( "AbstractChange::apply: eChgObj needs "
				  "the paths of a variable and of the "
				  "objective" ) );

   if( auto qf = dynamic_cast< DQuadFunction * >( fobj ) ) {
    if( v_data.size() != 2 )
     throw( std::invalid_argument( "AbstractChange::apply: eChgObj on a "
				   "DQuadFunction needs 2 data" ) );
    const auto pos = qf->is_active( pv );
    double old_c1 = 0 , old_c2 = 0;
    if( pos < qf->get_num_active_var() ) {
     old_c1 = qf->get_linear_coefficient( pos );
     old_c2 = qf->get_quadratic_coefficient( pos );
     qf->modify_term( pos , v_data[ 0 ] , v_data[ 1 ] , issueAMod );
     }
    else
     qf->add_variable( pv , v_data[ 0 ] , v_data[ 1 ] , issueAMod );
    if( doUndo )
     ret = undo( eChgObj , { old_c1 , old_c2 } );
    }
   else
    if( auto lf = dynamic_cast< LinearFunction * >( fobj ) ) {
     if( v_data.size() != 1 )
      throw( std::invalid_argument( "AbstractChange::apply: eChgObj on a "
				    "LinearFunction needs 1 datum" ) );
     const auto pos = lf->is_active( pv );
     double old_c1 = 0;
     if( pos < lf->get_num_active_var() ) {
      old_c1 = lf->get_coefficient( pos );
      lf->modify_coefficient( pos , v_data[ 0 ] , issueAMod );
      }
     else
      lf->add_variable( pv , v_data[ 0 ] , issueAMod );
     if( doUndo )
      ret = undo( eChgObj , { old_c1 } );
     }
    else
     throw( std::invalid_argument( "AbstractChange::apply: eChgObj on an "
				   "objective Function not supported" ) );
   break;
   }

  case( eChgSense ): {
   if( v_paths.empty() )
    throw( std::invalid_argument( "AbstractChange::apply: no path" ) );
   auto obj = v_paths[ 0 ].get_element< Objective >( block );
   if( ! obj )
    throw( std::invalid_argument( "AbstractChange::apply: the path of "
				  "eChgSense is not that of an Objective" ) );
   if( doUndo )
    ret = undo( eChgSense , { double( obj->get_sense() ) } );
   obj->set_sense( int( datum() ) , issueAMod );
   break;
   }

  case( eChgIntegrality ): {
   auto pv = variable();
   const bool was_integer = pv->is_integer();
   pv->is_integer( datum() != 0 , issueAMod );
   if( doUndo )
    ret = undo( eChgIntegrality , { was_integer ? 1.0 : 0.0 } );
   break;
   }

  case( eFixX ): {
   auto pv = variable();
   const double value = datum();
   if( doUndo )
    ret = pv->is_fixed() ? undo( eFixX , { pv->get_value() } )
	                 : undo( eUnfixX , {} );
   pv->set_value( value );
   pv->is_fixed( true , issueAMod );
   break;
   }

  case( eUnfixX ): {
   auto pv = variable();
   pv->is_fixed( false , issueAMod );
   if( doUndo )
    ret = undo( eFixX , { pv->get_value() } );
   break;
   }

  case( eChgLB ):
  case( eChgUB ): {
   auto pv = variable();
   const double value = datum();
   const bool lb = ( f_type == eChgLB );
   bool found = false;
   for( Index i = 0 ; i < pv->get_num_active() ; ++i ) {
    auto dep = pv->get_active( i );
    if( auto c = dynamic_cast< BoxConstraint * >( dep ) ) {
     if( doUndo )
      ret = undo( f_type , { lb ? c->get_lhs() : c->get_rhs() } );
     if( lb )
      c->set_lhs( value , issueAMod );
     else
      c->set_rhs( value , issueAMod );
     found = true;
     break;
     }
    if( lb )
     if( auto c = dynamic_cast< LBConstraint * >( dep ) ) {
      if( doUndo )
       ret = undo( f_type , { c->get_lhs() } );
      c->set_lhs( value , issueAMod );
      found = true;
      break;
      }
    if( ! lb )
     if( auto c = dynamic_cast< UBConstraint * >( dep ) ) {
      if( doUndo )
       ret = undo( f_type , { c->get_rhs() } );
      c->set_rhs( value , issueAMod );
      found = true;
      break;
      }
    }
   if( ! found )
    throw( std::invalid_argument( std::string( "AbstractChange::apply: " ) +
				  ( lb ? "eChgLB" : "eChgUB" ) +
				  " needs a variable with a BoxConstraint or "
				  "a " + ( lb ? "LB" : "UB" ) + "Constraint" ) );
   break;
   }

  case( eEmpty ):
   throw( std::invalid_argument( "AbstractChange::apply: an empty "
				 "AbstractChange cannot be applied" ) );

  default:
   throw( std::invalid_argument( "AbstractChange::apply: unknown type " +
				 std::to_string( f_type ) ) );
  }

 return( ret );

 }  // end( AbstractChange::apply )

/*--------------------------------------------------------------------------*/
/*---------------------- End File AbstractChange.cpp -----------------------*/
/*--------------------------------------------------------------------------*/

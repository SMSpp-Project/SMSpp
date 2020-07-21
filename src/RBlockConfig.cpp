/*--------------------------------------------------------------------------*/
/*------------------------- File RBlockConfig.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the RBlockConfig class.
 *
 * \version 0.10
 *
 * \date 15 - 07 - 2020
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BlockInspection.h"
#include "RBlockConfig.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register RBlockConfig and ERBlockConfig to the Configuration factory

SMSpp_insert_in_factory_cpp_0( RBlockConfig );
SMSpp_insert_in_factory_cpp_0( ERBlockConfig );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// Auxiliary functions for RBlockConfig.cpp not exported as methods of
// the class

namespace {

/*--------------------------------------------------------------------------*/

/// returns the BlockConfig associated with the given element
/** Returns a pointer to the BlockConfig associated with the given \p
 * element. The given \p element must be a pointer to an object whose base
 * class is either FRowConstraint or FRealObjective. If the function
 * associated with this element is a BendersBFunction or a LagBFunction, then
 * a pointer to the BlockConfig of the inner Block of that Function is
 * returned. Otherwise, a nullptr is returned.
 *
 * @param element A pointer to the element whose associated BlockConfig
 *        is desired.
 *
 * @return A pointer to the BlockConfig associated with the given
 *         element (if there is one); or nullptr otherwise.
 */
template< class S = ERBlockConfig , class T >
static std::enable_if_t< std::is_base_of_v< BlockConfig , S > &&
                         ( std::is_base_of_v< FRowConstraint , T > ||
                           std::is_base_of_v< FRealObjective , T > ) ,
                         S * >
extract_BlockConfig( const T * const element ) {
 if( element )
  if( auto block =
      inspection::get_indirect_sub_Block( element->get_function() ) )
   return new S( block );
 return nullptr;
}

/*--------------------------------------------------------------------------*/

template< class S = ERBlockConfig >
S * extract_BlockConfig( Objective * objective ) {
 return extract_BlockConfig< S >
  ( dynamic_cast<FRealObjective *>( objective ) );
}

/*--------------------------------------------------------------------------*/

/// writes in bc the BlockConfig associated with Constraint
/** Writes in \p bc the BlockConfig associated with Constraint.
 *
 * @param block A pointer to the Block whose BlockConfig associated with the
 *        Constraint will be written in \p bc.
 *
 * @param bc A pointer to the BlockConfig in which the BlockConfig
 *        associated with Constraints will be written.
 */
void extract_BlockConfig_Constraint( const Block * const block ,
                                     ERBlockConfig * bc ) {

 auto base_lambda = [ bc ]( const auto & group , const auto group_index ,
                            const auto block , const auto num_static_groups ,
                            const auto is_static ) {
  return
   [ bc , & group , group_index , block , num_static_groups ,
     is_static ] ( FRowConstraint & constraint ) {

    auto sub_bc = extract_BlockConfig( & constraint );

    if( ! sub_bc )
     return;

    auto constraint_index = inspection::get_index( & constraint ,
                                                   group , is_static );

    if( constraint_index == Inf<Block::Index>() ) {
     std::stringstream message;
     message << "RBlockConfig::get: index of Constraint " <<
      static_cast<const void*>( & constraint ) << " in " <<
      ( is_static ? "static" : "dynamic" ) << " group " +
      std::to_string( group_index ) + " of Block " <<
      static_cast<const void*>( block ) << " was not found";
     throw( std::logic_error( message.str() ) );
    }

    auto constraint_id = Block::ConstraintID (
     ( is_static ? group_index : group_index + num_static_groups ) ,
     constraint_index );

    bc->add_Config_Constraint( sub_bc , constraint_id );

   };
 };

 // BlockConfig for static Constraint

 const auto & static_constraints = block->get_static_constraints();
 const auto num_static_groups = static_constraints.size();
 auto group_index = 0;

 for( const auto & group : static_constraints ) {
  auto lambda = base_lambda( group , group_index , block ,
                             num_static_groups , true );
  un_any_const_static( group , lambda , un_any_type<FRowConstraint>() );
  ++group_index;
 }

 // BlockConfig for dynamic Constraint

 const auto & dynamic_constraints = block->get_dynamic_constraints();
 group_index = 0;
 for( const auto & group : dynamic_constraints ) {
  auto lambda = base_lambda( group , group_index , block ,
                             num_static_groups , false );
  un_any_const_static( group , lambda , un_any_type<FRowConstraint>() );
  ++group_index;
 }
}

} // end( unnamed namespace )

/*--------------------------------------------------------------------------*/
/*------------------------- METHODS of RBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

RBlockConfig::RBlockConfig( const RBlockConfig &old ) : BlockConfig( old )
{
 v_sub_BlockConfig.resize( old.v_sub_BlockConfig.size() , nullptr );
 for( std::size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( old.v_sub_BlockConfig[ i ] )
   v_sub_BlockConfig[ i ] = old.v_sub_BlockConfig[ i ]->clone();
 }

/*--------------------------------------------------------------------------*/

void RBlockConfig::get( Block * block ) {

 BlockConfig::get( block );

 for( auto & sBC : v_sub_BlockConfig )
  delete sBC;

 if( ! block ) {
  v_sub_BlockConfig.clear();
  return;
  }

 auto & nested_blocks = block->get_nested_Blocks();
 v_sub_BlockConfig.resize( nested_blocks.size() );

 auto nbit = nested_blocks.begin();
 for( c_Vec_Block::size_type i = 0 ; i < nested_blocks.size() ; ++i )
  v_sub_BlockConfig[ i ] = new ERBlockConfig( *(nbit++) );
 }  // end( RBlockConfig::get )

/*--------------------------------------------------------------------------*/

void RBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 // set the configurations for the Block -------------------------------------

 BlockConfig::apply( block , deleteold );

 // set the configurations for the sub-Block ---------------------------------

 auto & nb = block->get_nested_Blocks();
 auto bit = nb.begin();
 auto sbcit = v_sub_BlockConfig.begin();

 // only set non-nullptr configurations, hence only up until the list of
 // BlockConfigs ends
 for( ; ( bit != nb.end() ) &&
        ( sbcit != v_sub_BlockConfig.end() ) ;
        ++bit , ++sbcit )
  if( *sbcit )
   ( *sbcit )->apply( *bit , deleteold );
 }  // end( RBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void RBlockConfig::print( std::ostream &output ) const
{
 BlockConfig::print( output );
 for( const auto cfg : v_sub_BlockConfig )
  if( cfg )
   output << *cfg;
 output << std::endl;

 }  // end( RBlockConfig::print )

/*--------------------------------------------------------------------------*/

void RBlockConfig::load( std::istream & input ) {

 BlockConfig::load( input );

 input >> eatcomments >> f_diff;

 int k;
 input >> eatcomments >> k;
 v_sub_BlockConfig.resize( k );
 for( int i = 0; i < k; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) ) {
   v_sub_BlockConfig[ i ] = nullptr;
   input.ignore( std::numeric_limits< std::streamsize >::max(),
                 input.widen( '\n' ) );
  } else {
   std::string cname;
   input >> cname;
   v_sub_BlockConfig[ i ] =
    dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cname ) );
   if( ! v_sub_BlockConfig[ i ] )
    throw ( std::invalid_argument( "RBlockConfig::load: invalid Configuration"
                                   " for the sub-Block " +
                                   std::to_string( i ) + "." ) );
   input >> *v_sub_BlockConfig[ i ];
  }
 }
}  // end( RBlockConfig::load )

/*--------------------------------------------------------------------------*/

void RBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockConfig::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt() , f_diff );

 group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 for( size_t i = 0 ; i < v_sub_BlockConfig.size() ; ++i )
  if( v_sub_BlockConfig[ i ] ) {
   auto cg =  group.addGroup( "sub-BlockConfig_" + std::to_string( i ) );
   v_sub_BlockConfig[ i ]->serialize( cg );
   }

 }  // end( RBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockConfig::deserialize( netCDF::NcGroup & group )
{

 if( ! v_sub_BlockConfig.empty() )
  throw( std::logic_error( "deserializing a non-empty RBlockConfig" ) );

 BlockConfig::deserialize( group );

 auto diff_att = group.getAtt( "diff" );
 if( ! diff_att.isNull() )
  diff_att.getValues( & f_diff );
 else
  f_diff = false;

 group.addDim( "n_sub_Block" , v_sub_BlockConfig.size() );

 size_t size = ( group.getDim( "n_sub_Block" ) ).getSize();

 v_sub_BlockConfig.resize( size );

 for( size_t i = 0 ; i < size ; ++i ) {
  auto cg = group.getGroup( "sub-BlockConfig_" + std::to_string( i ) );
  v_sub_BlockConfig[ i ] =
   dynamic_cast< BlockConfig * >( new_Configuration( cg ) );
  }
 }  // end( RBlockConfig::deserialize( group ) )


/*--------------------------------------------------------------------------*/
/*------------------------ METHODS of ERBlockConfig ------------------------*/
/*--------------------------------------------------------------------------*/

ERBlockConfig::ERBlockConfig( const ERBlockConfig &old ) : RBlockConfig( old )
{
 v_ConstraintID.resize( old.v_ConstraintID.size() );
 for( std::size_t i = 0 ; i < v_ConstraintID.size() ; ++i )
   v_ConstraintID[ i ] = old.v_ConstraintID[ i ];

 v_Config_Constraints.resize
  ( old.v_Config_Constraints.size() , nullptr );
 for( std::size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
  v_Config_Constraints[ i ] = nullptr;
  if( old.v_Config_Constraints[ i ] )
   v_Config_Constraints[ i ] =
    old.v_Config_Constraints[ i ]->clone();
  }

 f_Config_Objective = nullptr;
 if( old.f_Config_Objective )
  f_Config_Objective = old.f_Config_Objective->clone();
 }

/*--------------------------------------------------------------------------*/

void ERBlockConfig::get( Block * block ) {

 RBlockConfig::get( block );

 for( auto sCC : v_Config_Constraints )
  delete sCC;

 delete f_Config_Objective;
 f_Config_Objective = nullptr;

 if( ! block ) {
  v_Config_Constraints.clear();
  return;
  }

 // BlockConfig Constraint

 extract_BlockConfig_Constraint( block , this );

 // BlockConfig for Objective

 f_Config_Objective = extract_BlockConfig( block->get_objective() );

 }  // end( ERBlockConfig::get )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::apply( Block * block , bool deleteold ) {

 if( ! block )
  return;

 RBlockConfig::apply( block , deleteold );

 // set the configurations for the Block associated with Constraint ----------
 //---------------------------------------------------------------------------

 for( std::size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
  if( v_Config_Constraints[ i ] )
   v_Config_Constraints[ i ]->apply
    ( inspection::get_indirect_sub_Block( block , v_ConstraintID[ i ] ) ,
      deleteold );
  }

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( f_Config_Objective )
  f_Config_Objective->apply( inspection::get_indirect_sub_Block( block ) ,
                             deleteold );

 }  // end( ERBlockConfig::apply )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::print( std::ostream &output ) const
{
 RBlockConfig::print( output );

 for( const auto cfgcstr : v_Config_Constraints )
  if( cfgcstr )
   output << *cfgcstr;
 if( f_Config_Objective )
  output << *f_Config_Objective;
 output << std::endl;
 }  // end( ERBlockConfig::print )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::load( std::istream & input ) {
 RBlockConfig::load( input );

 // Configuration for Constraint

 int k;
 input >> eatcomments >> k;
 v_Config_Constraints.resize( k );
 v_ConstraintID.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  Block::Index group_index, constraint_index;
  input >> eatcomments;
  input >> group_index >> constraint_index;
  v_ConstraintID[ i ] = Block::ConstraintID( group_index , constraint_index );
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_Config_Constraints[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   v_Config_Constraints[ i ] =
    dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cname ) );
   if( ! v_Config_Constraints[ i ] )
    throw ( std::invalid_argument( "ERBlockConfig::load: invalid Configuration"
                                   " for the Constraint " +
                                   std::to_string( i ) + "." ) );
   input >> *v_Config_Constraints[ i ];
   }
  }

 // Configuration for Objective

 input >> eatcomments;
 if( input.peek() == input.widen( '*' ) )
  f_Config_Objective = nullptr;
 else {
  std::string cname;
  input >> cname;
  f_Config_Objective =
   dynamic_cast< BlockConfig * >( Configuration::new_Configuration( cname ) );
  if( ! f_Config_Objective )
   throw ( std::invalid_argument( "ERBlockConfig::load: invalid Configuration"
                                  " for the Objective." ) );
  input >> *f_Config_Objective;
  }
 }  // end( ERBlockConfig::load )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::serialize( netCDF::NcGroup & group ) const
{
 RBlockConfig::serialize( group );

 // Configuration for Constraint

 if( ! v_Config_Constraints.empty() ) {

  group.addDim( "n_Config_Constraint" ,
                v_Config_Constraints.size() );

  for( size_t i = 0 ; i < v_Config_Constraints.size() ; ++i ) {
   if( v_Config_Constraints[ i ] ) {
    auto bcc = group.addGroup( "Config_Constraint_" + std::to_string( i ) );
    v_Config_Constraints[ i ]->serialize( bcc );
    }
   }

  auto ConstraintID_dim = group.addDim( "ConstraintID_dim" ,
                                        2 * v_ConstraintID.size() );

  auto ConstraintID_var = group.addVar( "ConstraintID" , netCDF::NcUint() ,
                                        ConstraintID_dim );

  for( size_t i = 0 ; i < v_ConstraintID.size() ; ++i ) {
   ConstraintID_var.putVar( { 2 * i } , { 2 } , std::vector<Block::Index>
                            { v_ConstraintID[ i ].first ,
                              v_ConstraintID[ i ].second }.data() );
   }
  }

 // Configuration for Objective

 if( f_Config_Objective ) {
  auto bcobj = group.addGroup( "Config_Objective" );
  f_Config_Objective->serialize( bcobj );
  }
 }  // end( ERBlockConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void ERBlockConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_Config_Constraints.size() || v_ConstraintID.size() ||
     f_Config_Objective )
  throw( std::logic_error( "deserializing a non-empty ERBlockConfig" ) );

 RBlockConfig::deserialize( group );

 // Configuration for Constraint

 auto constrdim = group.getDim( "n_Config_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_Config_Constraints.resize( constrsize );
 v_ConstraintID.resize( constrsize );

 std::vector<Block::Index> var_ConstraintID;
 if( constrsize > 0 ) {
  ::deserialize( group , "ConstraintID" , 2 * constrsize ,
                 var_ConstraintID , false , false );
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto bcc = group.getGroup( "Config_Constraint_" +
                              std::to_string( i ) );
  v_Config_Constraints[ i ] =
   dynamic_cast< BlockConfig * >( new_Configuration( bcc ) );
  v_ConstraintID[ i ] = Block::ConstraintID( var_ConstraintID[ 2 * i ] ,
                                             var_ConstraintID[ 2 * i + 1 ] );
  }

 // Configuration for Objective

 auto bcobj = group.getGroup( "Config_Objective" );
 if( ! bcobj.isNull() ) {
  f_Config_Objective =
   dynamic_cast< BlockConfig * >( new_Configuration( bcobj ) );
  }
 }  // end( ERBlockConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Block.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/

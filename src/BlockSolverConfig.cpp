/*--------------------------------------------------------------------------*/
/*---------------------- File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BlockSolverConfig, RBlockSolverConfig, and
 * ERBlockSolverConfig classes.
 *
 * \version 0.10
 *
 * \date 16 - 06 - 2020
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

#include "AbstractPath.h"
#include "BendersBFunction.h"
#include "Block.h"
#include "BlockSolverConfig.h"
#include "Constraint.h"
#include "FRowConstraint.h"
#include "LagBFunction.h"
#include "Objective.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BlockSolverConfig, RBlockSolverConfig, and ERBlockSolverConfig to
// the Configuration factory

SMSpp_insert_in_factory_cpp_0( BlockSolverConfig );
SMSpp_insert_in_factory_cpp_0( RBlockSolverConfig );
SMSpp_insert_in_factory_cpp_0( ERBlockSolverConfig );

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
// Auxiliary functions for BlockSolverConfig.cpp not exported as methods of
// the class

/// returns a pointer to the element at the given index in the given group
/** Returns a pointer to the element of type \p T located at the given
 * position \p index in the given boost::any \p group. If the element is not
 * found, nullptr is returned. The parameter \p is_static indicates whether
 * the given group must be considered static or dynamic.
 *
 * @param group A boost::any.
 *
 * @param index The index of the element in the given group.
 *
 * @param is_static Indicates whether the given group is static or dynamic.
 *
 * @return If an element of type T is found at position \p index in the given
 *         \p group, then a pointer to this element is returned. Otherwise,
 *         nullptr is returned.
 */
template< class T >
T * get_element( const boost::any & group , const Block::Index index ,
                 const bool is_static ) {
 if( is_static )
  return AbstractPath::get_static_element< T, T >( group , index );
 else
  return AbstractPath::get_dynamic_element< T, T >( group , index );
}

/*--------------------------------------------------------------------------*/

/// returns a pointer to the Constraint identified by the given \p id
/** Returns a pointer to the Constraint identified by the given \p id in the
 * given \p block. The desired Constraint must have exactly the type T. If no
 * Constraint of type T with the given \p id is found, nullptr is returned.
 *
 * @param block The Block to which the desired Constraint belongs.
 *
 * @param id The Block::ConstraintID identifying the Constraint in the given
 *        \p block.
 *
 * @return If there is a Constraint of type T with the given \p id in the
 *         given \p Block, then a pointer to this Constraint is
 *         returned. Otherwise, nullptr is returned.
 */
template< class T >
T * get_Constraint( const Block * const block ,
                    const Block::ConstraintID id ) {
 const auto & static_constraints = block->get_static_constraints();
 const auto num_static_groups = static_constraints.size();
 auto group_index = id.first;
 auto constraint_index = id.second;

 if( group_index < num_static_groups ) {
  // A static Constraint
  auto any_group = static_constraints[ group_index ];
  return get_element< T >( any_group , constraint_index , true );
 }
 else {
  // A dynamic Constraint
  group_index = id.first - num_static_groups;
  const auto & dynamic_constraints = block->get_dynamic_constraints();

  if( group_index >= dynamic_constraints.size() )
   throw( std::logic_error( "BlockSolverConfig::get_Constraint: invalid "
                            "dynamic Constraint group index: " +
                            std::to_string( group_index ) ) );

  auto any_group = dynamic_constraints[ group_index ];
  return get_element< T >( any_group , constraint_index , false );
 }
}

/*--------------------------------------------------------------------------*/

/// returns a pointer to the inner Block associated with the given Function
/** Returns a pointer to the inner Block associated with the given \p
 * function. The given Function is expected to be either a BendersBFunction or
 * a LagBFunction. If the given Function is not of any of these two types,
 * then nullptr is returned.
 *
 * @param function A pointer to the Function whose inner Block is desired.
 *
 * @return The inner Block associated with the given Function. If the given
 *         Function has no associated inner Block, then nullptr is returned.
 */
Block * get_indirect_sub_Block( const Function * const function ) {
 if( ! function )
  return nullptr;
 if( auto f = dynamic_cast< const BendersBFunction * >( function ) )
  return f->get_inner_block();
 else if( auto f = dynamic_cast< const LagBFunction * >( function ) )
  return f->get_inner_block();
 return nullptr;
}

/*--------------------------------------------------------------------------*/

/// returns the inner Block associated with the Objective of the given Block
/** Returns a pointer to the inner Block associated with the given Objective
 * of \p block. If the Objective of the given Block is not an FRealObjective,
 * then nullptr is returned.
 *
 * @param block A pointer to the Block containing an Objective whose
 *        associated inner Block is desired.
 *
 * @return A pointer to the inner Block associated with the Objective of the
 *         given Block. If the Objective of the given Block is not an
 *         FRealObjective, nullptr is returned.
 */
Block * get_indirect_sub_Block( const Block * const block ) {
 if( auto objective = dynamic_cast<FRealObjective *>( block->get_objective() ) )
  return get_indirect_sub_Block( objective->get_function() );
 return nullptr;
}

/*--------------------------------------------------------------------------*/

/// returns a pointer to the inner Block associated with the given Constraint
/** Returns a pointer to the inner Block associated with the given \p
 * constraint. If the given Constraint has no inner Block associated with it,
 * nullptr is returned.
 *
 * @param constraint A pointer to the Constraint whose associated inner Block
 *        is desired.
 *
 * @return The inner Block associated with the given Constraint. If the given
 *         Constraint has no associated inner Block, then nullptr is returned.
 */
Block * get_indirect_sub_Block( const Constraint * const constraint ) {
 if( auto frowc = dynamic_cast< const FRowConstraint * >( constraint ) )
  return get_indirect_sub_Block( frowc->get_function() );
 return nullptr;
}

/*--------------------------------------------------------------------------*/

/// returns a pointer to the inner Block associated with the given Constraint
/** Returns a pointer to the inner Block associated with the Constraint,
 * belonging to \p block, specified by the given ConstraintID \p id (if any).
 *
 * @param block The Block to which the Constraint belongs.
 *
 * @param id The Block::ConstraintID identifying the Constraint in the given
 *        \p block.
 *
 * @return The inner Block associated with the given Constraint.
 */
Block * get_indirect_sub_Block( const Block * const block ,
                                const Block::ConstraintID id ) {
 auto constraint = get_Constraint< FRowConstraint >( block , id );
 return get_indirect_sub_Block( constraint );
}

/*--------------------------------------------------------------------------*/

/// returns the BlockSolverConfig associated with the given element
/** Returns a pointer to the BlockSolverConfig associated with the given \p
 * element. The given \p element must be a pointer to an object whose base
 * class is either FRowConstraint or FRealObjective. If the function
 * associated with this element is a BendersBFunction or a LagBFunction, then
 * a pointer to the BlockSolverConfig of the inner Block of that Function is
 * returned. Otherwise, a nullptr is returned.
 *
 * @param element A pointer to the element whose associated BlockSolverConfig
 *        is desired.
 *
 * @return A pointer to the BlockSolverConfig associated with the given
 *         element (if there is one); or nullptr otherwise.
 *
 * @param clear It indicates whether a "clear" BlockSolverConfig must be
 *        constructed.
 */
template< class S = ERBlockSolverConfig , class T >
static std::enable_if_t< std::is_base_of_v< BlockSolverConfig , S > &&
                         ( std::is_base_of_v< FRowConstraint , T > ||
                           std::is_base_of_v< FRealObjective , T > ) ,
                         S * >
extract_BlockSolverConfig( const T * const element , bool clear ) {
 if( element )
  if( auto block = get_indirect_sub_Block( element->get_function() ) )
   return new S( block , false , clear );
 return nullptr;
}

/*--------------------------------------------------------------------------*/

template< class S = ERBlockSolverConfig >
S * extract_BlockSolverConfig( Objective * objective , bool clear ) {
 return extract_BlockSolverConfig< S >
  ( dynamic_cast<FRealObjective *>( objective ) , clear );
}

/*--------------------------------------------------------------------------*/

/// returns the index of the given element in the given boost::any group
/** Returns the index of the given \p element in the given boost::any \p
 * group.
 *
 * @param element A pointer to the element whose index in \p group is desired.
 *
 * @param group The group containing the element.
 *
 * @param is_static Indicates whether the given group is static or dynamic.
 *
 * @return The index of the given element in the given group.
 */
template< class T >
Block::Index get_index( const T * element , const boost::any & group ,
                        const bool is_static ) {
 if( is_static )
  return AbstractPath::get_static_index< T, T >( element , group );
 else
  return AbstractPath::get_dynamic_index< T, T >( element , group );
}

/*--------------------------------------------------------------------------*/

/// writes in bsc the BlockSolverConfig associated with Constraint
/** Writes in \p bsc the BlockSolverConfig associated with Constraint.
 *
 * @param block A pointer to the Block whose BlockSolverConfig associated with
 *        the Constraint will be written in \p bsc.
 *
 * @param bsc A pointer to the BlockSolverConfig in which the
 *        BlockSolverConfig associated with Constraints will be written.
 *
 * @param clear It indicates whether a "clear" BlockSolverConfig must be
 *        constructed for the inner Block of the Constraint.
 */
void extract_BlockSolverConfig_Constraint
( const Block * const block , ERBlockSolverConfig * bsc , bool clear ) {

 auto base_lambda = [ bsc ]( const auto & group , const auto group_index ,
                             const auto block , const auto num_static_groups ,
                             const auto is_static , const auto clear ) {
  return
   [ bsc , & group , group_index , block , num_static_groups ,
     is_static , clear ] ( FRowConstraint & constraint ) {

    auto sub_bsc = extract_BlockSolverConfig( & constraint , clear );

    if( ! sub_bsc )
     return;

    auto constraint_index = get_index( & constraint , group , is_static );

    if( constraint_index == Inf<Block::Index>() ) {
     std::stringstream message;
     message << "BlockSolverConfig::get: index of Constraint " <<
      static_cast<const void*>( & constraint ) << " in " <<
      ( is_static ? "static" : "dynamic" ) << " group " +
      std::to_string( group_index ) + " of Block " <<
      static_cast<const void*>( block ) << " was not found";
     delete bsc;
     throw( std::logic_error( message.str() ) );
    }

    auto constraint_id = Block::ConstraintID (
     ( is_static ? group_index : group_index + num_static_groups ) ,
     constraint_index );

    bsc->add_BlockSolverConfig_Constraint( sub_bsc , constraint_id );

   };
 };

 // BlockSolverConfig for static Constraint

 const auto & static_constraints = block->get_static_constraints();
 const auto num_static_groups = static_constraints.size();
 auto group_index = 0;

 for( const auto & group : static_constraints ) {
  auto lambda = base_lambda( group , group_index , block ,
                             num_static_groups , true , clear );
  un_any_const_static( group , lambda , un_any_type<FRowConstraint>() );
  ++group_index;
 }

 // BlockSolverConfig for dynamic Constraint

 const auto & dynamic_constraints = block->get_dynamic_constraints();
 group_index = 0;
 for( const auto & group : dynamic_constraints ) {
  auto lambda = base_lambda( group , group_index , block ,
                             num_static_groups , false , clear );
  un_any_const_static( group , lambda , un_any_type<FRowConstraint>() );
  ++group_index;
 }
}

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of BlockSolverConfig ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING BlockSolverConfig --------------*/
/*--------------------------------------------------------------------------*/

BlockSolverConfig::BlockSolverConfig( const BlockSolverConfig &old )
 : Configuration() {

 f_diff = old.f_diff;
 v_SolverNames = old.v_SolverNames;

 v_SolverConfigs.resize( old.v_SolverConfigs.size() );
 for( std::size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  v_SolverConfigs[ i ] = nullptr;
  if( old.v_SolverConfigs[ i ] )
   v_SolverConfigs[ i ] = old.v_SolverConfigs[ i ]->clone();
  }
 }

/*--------------------------------------------------------------------------*/

BlockSolverConfig * BlockSolverConfig::deserialize( netCDF::NcFile & f ,
                                                    const unsigned int idx )
{
 try {
  netCDF::NcGroupAtt ftype = f.getAtt( "SMS++_file_type" );
  if( ftype.isNull() )
   return( nullptr );

  int type;
  ftype.getValues( & type );

  if( ( type != eProbFile ) && ( type != eConfigFile ) )
   return( nullptr );

  netCDF::NcGroup cg;

  if( type == eProbFile ) {
   netCDF::NcGroup dg = f.getGroup( "Config_" + std::to_string( idx ) );
   if( dg.isNull() )
    return( nullptr );

   cg = dg.getGroup( "SolverConfig" );
   }
  else
   cg = f.getGroup( "Config_" + std::to_string( idx ) );

  auto result = new_Configuration( cg );
  auto bcresult = dynamic_cast< BlockSolverConfig * >( result );
  if( ! bcresult ) {
   delete result;
   return( nullptr );
   }

  return( bcresult );
  }
 catch( netCDF::exceptions::NcException & e ) {
  std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
  }
 catch( std::exception & e ) {
  std::cerr << "error " << e.what() << " in deserialize" << std::endl;
  }
 catch( ... ) {
  std::cerr << "unknown error in deserialize" << std::endl;
  }

 return( nullptr );

 } // end( BlockSolverConfig::deserialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_SolverNames.size() || v_SolverConfigs.size() )
  throw( std::logic_error( "deserializing a non-empty BlockSolverConfig" ) );

 Configuration::deserialize( group );

 netCDF::NcGroupAtt diff = group.getAtt( "diff" );
 if( diff.isNull() )
  f_diff = false;
 else {
  int diffint;
  diff.getValues( &diffint );
  f_diff = diffint > 0;
 }

 size_t slvsize = 0;
 auto dim = group.getDim( "n_SolverConfig" );
 if( ! dim.isNull() )
  slvsize = dim.getSize();

 v_SolverNames.resize( slvsize );
 v_SolverConfigs.resize( slvsize );

 netCDF::NcVar slvnms = group.getVar( "SolverNames" );

 if( slvsize > 0 && slvnms.isNull() )
  throw( std::logic_error( "BlockSolverConfig::deserialize: 'SolverNames' "
                           "was not provided." ) );

 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < slvsize ; ++i ) {
  idx[ 0 ] = i;
  char * str;
  slvnms.getVar( idx , & str );
  v_SolverNames[ i ] = std::string( str );
  auto sc = group.getGroup( "SolverConfig_" + std::to_string( i ) );
  v_SolverConfigs[ i ] = dynamic_cast< ComputeConfig * >
   ( new_Configuration( sc ) );
  }
 }  // end( BlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS BlockSolverConfig ----------------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::get( Block * block , bool clear ) {

 if( clear ) {
  this->clear();
  return;
  }

 for( auto sSC : v_SolverConfigs )
  delete sSC;

 if( ! block ) {
  v_SolverConfigs.clear();
  return;
  }

 c_Lst_Solver & ls = block->get_registered_solvers();

 v_SolverNames.resize( ls.size() );
 v_SolverConfigs.resize( ls.size() );

 auto lsit = ls.begin();
 for( c_Lst_Solver::size_type i = 0 ; i < ls.size() ; ++i , ++lsit ) {
  if( *lsit ) {
   v_SolverNames[ i ] = (*lsit)->classname();
   v_SolverConfigs[ i ] = (*lsit)->get_ComputeConfig();
   }
  else {
   v_SolverNames[ i ] = "";
   v_SolverConfigs[ i ] = nullptr;
   }
  }
 }  // end( BlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE BlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 // set the configurations for the Solver of the Block -----------------------
 //---------------------------------------------------------------------------
 auto & solvers = block->get_registered_solvers();
 auto sit = solvers.begin();
 auto nit = v_SolverNames.begin();
 auto cit = v_SolverConfigs.begin();

 // process existing Solvers -------------------------------------------------

 if( f_diff ) {  // differential mode ----------------------------------
  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
       ++sit , ++nit ) {

   if( ! (*nit).empty() ) {  // if the name is empty do nothing
    Solver *oldS = *sit;
    block->replace_Solver( Solver::new_Solver( *nit ) , sit );
    delete oldS;
    }

   if( cit != v_SolverConfigs.end() ) {
    if( *cit )  // if the configurtion is empty, do nothing
     (*sit)->set_ComputeConfig( *cit );
    ++cit;
    }
   }
  }
 else {                // setting mode ---------------------------------------
  for( ; ( sit != solvers.end() ) && ( nit != v_SolverNames.end() ) ;
       ++nit ) {

   Solver *oldS = *sit;

   if( (*nit).empty() ) {  // empty name
    block->unregister_Solver( sit );
    if( cit != v_SolverConfigs.end() )
     ++cit;  // ignore corresponding configuration
    // note: sit is not increased because the list is shortened
    }
   else {                   // non-empty name
    block->replace_Solver( Solver::new_Solver( *nit ) , sit );
    if( cit != v_SolverConfigs.end() ) {
     (*sit)->set_ComputeConfig( *cit );
     ++cit;
     }
    ++sit;
    }

   delete oldS;
   }
  }                    // end setting mode -----------------------------------

 // process Solvers in the Configuration but not in the Block-----------------

 for( ; nit != v_SolverNames.end() ; ++nit )
  if( (*nit).empty() ) {  // if the name is empty
   if( cit != v_SolverConfigs.end() )
    ++cit;
   }
  else {                   // the name is non-empty
   // important note: the order is
   // - first the Solver is created;
   // - then it is ComputeConfig-ured
   // - only then it is passed to the Block

   auto slvr = Solver::new_Solver( *nit );

   if( cit != v_SolverConfigs.end() ) {
    if( *cit )  // if the configurtion is empty, do nothing
     slvr->set_ComputeConfig( *cit );
    ++cit;
    }

   block->register_Solver( slvr );
   }
 }  // end( BlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::reset_Solver( Block * block ) const {
 // unregister Solver in reverse order
 auto & solvers = block->get_registered_solvers();
 for( auto it = solvers.rbegin() ; it != solvers.rend() ; ++it ) {
  Solver *oldS = *it;
  block->unregister_Solver( --(it.base()) );  // convert backward into forward
  delete oldS;
  }
 }  // end( BlockSolverConfig::reset_Solver )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE BlockSolverConfig ------*/
/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcFile & f , const int type )
 const
{
 if( type == eConfigFile ) {
  Configuration::serialize( f , type );
  return;
  }

 auto cg = ( f.addGroup( "Config_" + std::to_string( f.getGroupCount() )
                         ) ).addGroup( "SolverConfig" );
 serialize( cg );

 }  // end( BlockSolverConfig::serialize( file ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 Configuration::serialize( group );

 group.putAtt( "diff" , netCDF::NcInt() , int( f_diff ) );

 netCDF::NcDim sd = group.addDim( "n_SolverConfig" , v_SolverConfigs.size() );

 netCDF::NcVar slvnms = group.addVar( "SolverNames" , netCDF::NcString() ,
                                      sd );
 std::vector<size_t> idx( 1 );
 for( size_t i = 0 ; i < v_SolverConfigs.size() ; ++i ) {
  idx[ 0 ] = i;
  slvnms.putVar( idx , v_SolverNames[ i ] );
  if( v_SolverConfigs[ i ] ) {
   auto sc = group.addGroup( "SolverConfig_" + std::to_string( i ) );
   v_SolverConfigs[ i ]->serialize( sc );
   }
  }
 }  // end( BlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::print( std::ostream &output ) const
{
 output << "BlockSolverConfig";
 if( f_diff ) output << "[diff]";
 output << ": " << std::endl;
 for( std::size_t i = 0 ; i < v_SolverNames.size() ; ++i )
  output << v_SolverNames[ i ] << ": " << v_SolverConfigs[ i ];
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void BlockSolverConfig::load( std::istream &input )
{
 input >> eatcomments >> f_diff;

 int k;
 input >> eatcomments >> k;
 v_SolverNames.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_SolverNames[ i ] = nullptr;
  else
   input >> v_SolverNames[ i ];
  }

 input >> eatcomments >> k;
 v_SolverConfigs.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_SolverConfigs[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   ComputeConfig *tmpbc = dynamic_cast<ComputeConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a ComputeConfig object" ) );
   v_SolverConfigs[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }
 }  // end( BlockSolverConfig::load )


/*--------------------------------------------------------------------------*/
/*-------------------- METHODS of RBlockSolverConfig -----------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING RBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/

RBlockSolverConfig::RBlockSolverConfig( const RBlockSolverConfig &old )
 : BlockSolverConfig( old ) {

 v_BlockSolverConfigs.resize( old.v_BlockSolverConfigs.size() );
 for( std::size_t i = 0 ; i < v_BlockSolverConfigs.size() ; ++i ) {
  v_BlockSolverConfigs[ i ] = nullptr;
  if( old.v_BlockSolverConfigs[ i ] )
   v_BlockSolverConfigs[ i ] = old.v_BlockSolverConfigs[ i ]->clone();
  }

 }  // end( RBlockSolverConfig::RBlockSolverConfig )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_BlockSolverConfigs.size() )
  throw( std::logic_error( "deserializing a non-empty BlockSolverConfig" ) );

 BlockSolverConfig::deserialize( group );

 // BlockSolverConfig for sub-Block

 size_t blkslvsize = 0;

 auto dim = group.getDim( "n_BlockSolverConfig" );
 if( ! dim.isNull() )
  blkslvsize = dim.getSize();

 v_BlockSolverConfigs.resize( blkslvsize );

 for( size_t i = 0 ; i < v_BlockSolverConfigs.size() ; ++i ) {
  auto bc = group.getGroup( "BlockSolverConfig_" + std::to_string( i ) );
  v_BlockSolverConfigs[ i ] = dynamic_cast< BlockSolverConfig * >(
                                                    new_Configuration( bc ) );
  }
 }  // end( RBlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*----------------- OTHER INITIALIZATIONS RBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::get( Block * block , bool clear ) {

 BlockSolverConfig::get( block , clear );

 for( auto sBSC : v_BlockSolverConfigs )
  delete sBSC;

 if( ! block ) {
  v_BlockSolverConfigs.clear();
  return;
  }

 auto & nested_blocks = block->get_nested_Blocks();
 v_BlockSolverConfigs.resize( nested_blocks.size() );

 auto nbit = nested_blocks.begin();
 for( c_Vec_Block::size_type i = 0 ; i < nested_blocks.size() ; ++i )
  v_BlockSolverConfigs[ i ] = new ERBlockSolverConfig( *(nbit++) ,
                                                       false , clear );
 }  // end( RBlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::apply( Block * block ) const
{
 if( ! block )
  return;

 // set the configurations for the Solver of the Block -----------------------
 //---------------------------------------------------------------------------

 BlockSolverConfig::apply( block );

 // set the configurations for the sub-Block ---------------------------------
 //---------------------------------------------------------------------------
 auto & nb = block->get_nested_Blocks();
 auto bit = nb.begin();
 auto bsit = v_BlockSolverConfigs.begin();

 // only set non-nullptr configurations, hence only up until the list of
 // BlockSolverConfigs ends
 for( ; ( bit != nb.end() ) &&
        ( bsit != v_BlockSolverConfigs.end() ) ;
        ++bit , ++bsit )
  if( *bsit )
   ( *bsit )->apply( *bit );
 }  // end( RBlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::reset_Solver( Block * block ) const {
 BlockSolverConfig::reset_Solver( block );

 // reset all Solver in all sub-Block
 const auto & blocks = block->get_nested_Blocks();
 auto block_it = blocks.cbegin();
 auto config_it = v_BlockSolverConfigs.cbegin();
 assert( v_BlockSolverConfigs.size() <= blocks.size() );

 for( ; ( block_it != blocks.cend() ) &&
       ( config_it != v_BlockSolverConfigs.cend() ) ;
      ++block_it , ++config_it ) {
  if( *config_it )
   ( *config_it )->reset_Solver( *block_it );
  }
 }  // end( RBlockSolverConfig::reset_Solver )

/*--------------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE RBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::serialize( netCDF::NcGroup & group ) const
{
 BlockSolverConfig::serialize( group );

 // BlockSolverConfig for sub-Block

 group.addDim( "n_BlockSolverConfig" , v_BlockSolverConfigs.size() );

 for( size_t i = 0 ; i < v_BlockSolverConfigs.size() ; ++i ) {
  if( v_BlockSolverConfigs[ i ] ) {
   auto bc = group.addGroup( "BlockSolverConfig_" + std::to_string( i ) );
   v_BlockSolverConfigs[ i ]->serialize( bc );
   }
  }

 }  // end( RBlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::print( std::ostream &output ) const
{
 BlockSolverConfig::print( output );

 for( const auto cfg : v_BlockSolverConfigs )
  if( cfg )
   output << *cfg;
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void RBlockSolverConfig::load( std::istream &input )
{

 BlockSolverConfig::load( input );

 // BlockSolverConfig for sub-Block

 int k;
 input >> eatcomments >> k;
 v_BlockSolverConfigs.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_BlockSolverConfigs[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockSolverConfig *tmpbc = dynamic_cast<BlockSolverConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a BlockSolverConfig object" ) );
   v_BlockSolverConfigs[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }

 }  // end( RBlockSolverConfig::load )


/*--------------------------------------------------------------------------*/
/*------------------- METHODS of ERBlockSolverConfig -----------------------*/
/*--------------------------------------------------------------------------*/
/*----------- CONSTRUCTING AND DESTRUCTING ERBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/

ERBlockSolverConfig::ERBlockSolverConfig( const ERBlockSolverConfig &old )
 : RBlockSolverConfig( old ) {

 v_ConstraintID.resize( old.v_ConstraintID.size() );
 for( std::size_t i = 0 ; i < v_ConstraintID.size() ; ++i )
  v_ConstraintID[ i ] = old.v_ConstraintID[ i ];

 v_BlockSolverConfig_Constraints.resize
  ( old.v_BlockSolverConfig_Constraints.size() );
 for( std::size_t i = 0 ; i < v_BlockSolverConfig_Constraints.size() ; ++i ) {
  v_BlockSolverConfig_Constraints[ i ] = nullptr;
  if( old.v_BlockSolverConfig_Constraints[ i ] )
   v_BlockSolverConfig_Constraints[ i ] =
    old.v_BlockSolverConfig_Constraints[ i ]->clone();
  }

 f_BlockSolverConfig_Objective = nullptr;
 if( old.f_BlockSolverConfig_Objective )
  f_BlockSolverConfig_Objective = old.f_BlockSolverConfig_Objective->clone();

 }  // end( ERBlockSolverConfig::ERBlockSolverConfig )

/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::deserialize( netCDF::NcGroup & group )
{
 if( v_BlockSolverConfig_Constraints.size() || v_ConstraintID.size() ||
     f_BlockSolverConfig_Objective )
  throw( std::logic_error( "deserializing a non-empty BlockSolverConfig" ) );

 RBlockSolverConfig::deserialize( group );

 // BlockSolverConfig for Constraint

 auto constrdim = group.getDim( "n_BlockSolverConfig_Constraint" );
 size_t constrsize = constrdim.isNull() ? 0 : constrdim.getSize();

 v_BlockSolverConfig_Constraints.resize( constrsize );
 v_ConstraintID.resize( constrsize );

 std::vector<Block::Index> var_ConstraintID;
 if( constrsize > 0 ) {
  ::deserialize( group , "ConstraintID" , 2 * constrsize ,
                 var_ConstraintID , false , false );
  }

 for( size_t i = 0 ; i < constrsize ; ++i ) {
  auto bscc = group.getGroup( "BlockSolverConfig_Constraint_" +
                              std::to_string( i ) );
  v_BlockSolverConfig_Constraints[ i ] = dynamic_cast< BlockSolverConfig * >(
                                                  new_Configuration( bscc ) );
  v_ConstraintID[ i ] = Block::ConstraintID( var_ConstraintID[ 2 * i ] ,
                                             var_ConstraintID[ 2 * i + 1 ] );
  }

 // BlockSolverConfig for Objective

 auto bscobj = group.getGroup( "BlockSolverConfig_Objective" );
 if( ! bscobj.isNull() ) {
  f_BlockSolverConfig_Objective = dynamic_cast< BlockSolverConfig * >(
                                                new_Configuration( bscobj ) );
  }
 }  // end( ERBlockSolverConfig::deserialize( group ) )

/*--------------------------------------------------------------------------*/
/*---------------- OTHER INITIALIZATIONS ERBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::get( Block * block , bool clear ) {

 RBlockSolverConfig::get( block , clear );

 for( auto sBSCC : v_BlockSolverConfig_Constraints )
  delete sBSCC;
 delete f_BlockSolverConfig_Objective;

 if( ! block ) {
  v_BlockSolverConfig_Constraints.clear();
  return;
  }

 // BlockSolverConfig Constraint

 extract_BlockSolverConfig_Constraint( block , this , clear );

 // BlockSolverConfig for Objective

 f_BlockSolverConfig_Objective =
  extract_BlockSolverConfig( block->get_objective() , clear );

 }  // end( ERBlockSolverConfig::get )

/*--------------------------------------------------------------------------*/
/*------- METHODS DESCRIBING THE BEHAVIOR OF THE ERBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::apply( Block * block ) const {

 if( ! block )
  return;

 RBlockSolverConfig::apply( block );

 // set the configurations for the Block associated with Constraint ----------
 //---------------------------------------------------------------------------

 for( std::size_t i = 0 ; i < v_BlockSolverConfig_Constraints.size() ; ++i ) {
  if( v_BlockSolverConfig_Constraints[ i ] )
   v_BlockSolverConfig_Constraints[ i ]->apply
    ( get_indirect_sub_Block( block , v_ConstraintID[ i ] ) );
  }

 // set the configurations for the Block associated with Objective -----------
 //---------------------------------------------------------------------------

 if( f_BlockSolverConfig_Objective )
  f_BlockSolverConfig_Objective->apply( get_indirect_sub_Block( block ) );

 }  // end( ERBlockSolverConfig::apply )

/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::reset_Solver( Block * block ) const {

 RBlockSolverConfig::reset_Solver( block );

 auto id_it = v_ConstraintID.cbegin();
 auto config_it = v_BlockSolverConfig_Constraints.cbegin();

 for( ; ( id_it != v_ConstraintID.cend() ) &&
       ( config_it != v_BlockSolverConfig_Constraints.cend() ) ;
      ++id_it , ++config_it ) {

  if( ! ( *config_it ) )
   continue;

  auto constraint = get_Constraint< FRowConstraint >( block , *id_it );

  if( ! constraint )
   throw( std::logic_error( "ERBlockSolverConfig::reset_Solver: invalid "
                            "ConstraintID: ( " + std::to_string
                            ( id_it->first ) + ", " + std::to_string
                            ( id_it->second ) + ")." ) );

  auto sub_Block = get_indirect_sub_Block( constraint );
  ( *config_it )->reset_Solver( sub_Block );
  }
 }  // end( ERBlockSolverConfig::reset_Solver )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR LOADING, PRINTING & SAVING THE ERBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::serialize( netCDF::NcGroup & group ) const {

 RBlockSolverConfig::serialize( group );

 // BlockSolverConfig for Constraint

 if( ! v_BlockSolverConfig_Constraints.empty() ) {

  group.addDim( "n_BlockSolverConfig_Constraint" ,
                v_BlockSolverConfig_Constraints.size() );

  for( size_t i = 0 ; i < v_BlockSolverConfig_Constraints.size() ; ++i ) {
   if( v_BlockSolverConfig_Constraints[ i ] ) {
    auto bscc = group.addGroup( "BlockSolverConfig_Constraint_"
                                + std::to_string( i ) );
    v_BlockSolverConfig_Constraints[ i ]->serialize( bscc );
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

 // BlockSolverConfig for Objective

 if( f_BlockSolverConfig_Objective ) {
  auto bscobj = group.addGroup( "BlockSolverConfig_Objective" );
  f_BlockSolverConfig_Objective->serialize( bscobj );
  }
 }  // end( ERBlockSolverConfig::serialize( group ) )

/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::print( std::ostream &output ) const
{
 RBlockSolverConfig::print( output );

 for( const auto cfgcstr : v_BlockSolverConfig_Constraints )
  if( cfgcstr )
   output << *cfgcstr;
 if( f_BlockSolverConfig_Objective )
  output << *f_BlockSolverConfig_Objective;
 output << std::endl;
 }

/*--------------------------------------------------------------------------*/

void ERBlockSolverConfig::load( std::istream &input )
{

 RBlockSolverConfig::load( input );

 // BlockSolverConfig for Constraint

 int k;
 input >> eatcomments >> k;
 v_BlockSolverConfig_Constraints.resize( k );
 v_ConstraintID.resize( k );
 for( int i = 0 ; i < k ; ++i ) {
  Block::Index group_index, constraint_index;
  input >> eatcomments;
  input >> group_index >> constraint_index;
  v_ConstraintID[ i ] = Block::ConstraintID( group_index , constraint_index );
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   v_BlockSolverConfig_Constraints[ i ] = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockSolverConfig *tmpbc = dynamic_cast<BlockSolverConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a BlockSolverConfig object" ) );
   v_BlockSolverConfig_Constraints[ i ] = tmpbc;
   input >> *tmpbc;
   }
  }

 // BlockSolverConfig for Objective

 bool objective = false;
 input >> eatcomments >> objective;

 if( objective ) {
  input >> eatcomments;
  if( input.peek() == input.widen( '*' ) )
   f_BlockSolverConfig_Objective = nullptr;
  else {
   std::string cname;
   input >> cname;
   Configuration *tmpc = Configuration::new_Configuration( cname );
   BlockSolverConfig *tmpbc = dynamic_cast<BlockSolverConfig *>( tmpc );
   if( ! tmpbc )
    throw( std::invalid_argument( "not a BlockSolverConfig object" ) );
   f_BlockSolverConfig_Objective = tmpbc;
   input >> *tmpbc;
   }
  }

 }  // end( ERBlockSolverConfig::load )

/*--------------------------------------------------------------------------*/
/*------------------ End File BlockSolverConfig.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

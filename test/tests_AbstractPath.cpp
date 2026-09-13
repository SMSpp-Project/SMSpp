/*--------------------------------------------------------------------------*/
/*---------------------- File tests_AbstractPath.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the tests for AbstractPath.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlockGenerator.h"
#include "AbstractPath.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace SMSpp_di_unipi_it::tests;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

void test_serialization( const AbstractPath & path ) {
 netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
 auto group = ncFile.addGroup("Path");
 path.serialize( group );
 AbstractPath deserialized_path( group );
 assert( path == deserialized_path );
}

/*--------------------------------------------------------------------------*/

void test_paths( Block * block , Block * reference_block ) {

 for( const auto & group : block->get_static_variables() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Variable >( reference_block ) );
             test_serialization( path );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             test_serialization( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             test_serialization( path );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_variables() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( ColVariable & v ) {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Variable >( reference_block ) );
             test_serialization( path );
            } ,
            un_any_type< ColVariable >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             test_serialization( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             test_serialization( path );
             }
            } ,
            un_any_type< FRowConstraint >() ) );

 {
  auto objective = block->get_objective();
  AbstractPath path( objective , reference_block );
  auto e = path.get_element< Objective >( reference_block );
  assert( objective == e );
  test_serialization( path );
 }

 {
  Function * function = nullptr;
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   function = objective->get_function();
  }
  AbstractPath path( function , reference_block );
  auto e = path.get_element< Function >( reference_block );
  assert( function == e );
  test_serialization( path );
 }

 {
  AbstractPath path( block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == block );
  test_serialization( path );
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  AbstractPath path( nested_block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == nested_block );
  test_serialization( path );
 }

 if( const auto pfb = dynamic_cast< PolyhedralFunctionBlock * >( block ) ) {
  const auto & function = pfb->get_PolyhedralFunction();
  AbstractPath path( & function , reference_block );
  const auto retrieved_function =
   path.get_element< Function >( reference_block );
  assert( retrieved_function == & function );
  test_serialization( path );
 }
}

/*--------------------------------------------------------------------------*/

void test_variable_multi_selection( Block * reference ) {
 using Index = Block::Index;

 // Find a static ColVariable group of size >= 2 in the reference Block;
 // skip the test if no such group is available.
 const auto & static_vars = reference->get_static_variables();
 for( Index g = 0 ; g < static_vars.size() ; ++g ) {
  const auto group_size = inspection::get_static_element_size<
   ColVariable , ColVariable >( static_vars[ g ] );
  if( ( group_size == Inf< Index >() ) || ( group_size < 2 ) )
   continue;

  auto * first = inspection::get_element< ColVariable >(
   reference , /*is_static*/ true , g , /*element_index*/ 0 );
  if( ! first )
   continue;

  AbstractPath path( first , reference );

  // ---- contiguous element range [ 0 , group_size ) ------------------------
  {
   AbstractPath rpath = path;
   rpath.set_last_node_range( 0 , group_size );
   assert( rpath.get_number_elements< ColVariable >( reference ) == group_size );
   for( Index j = 0 ; j < group_size ; ++j )
    assert( rpath.get_element< ColVariable >( reference , j ) ==
            inspection::get_element< ColVariable >( reference , true , g , j ) );

   const auto indices = rpath.get_resolved_indices< ColVariable >( reference );
   assert( indices.size() == group_size );
   for( Index j = 0 ; j < group_size ; ++j )
    assert( indices[ j ] == j );

   netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
   auto ncgroup = ncFile.addGroup( "Path" );
   rpath.serialize( ncgroup );
   AbstractPath round_trip( ncgroup );
   assert( round_trip == rpath );
   assert( round_trip.get_number_elements< ColVariable >( reference ) ==
           group_size );
  }

  // ---- explicit, possibly non-contiguous, element subset -------------------
  {
   // pick a subset with at least one "gap": { 0 , group_size - 1 }
   std::vector< Index > subset = { 0 , group_size - 1 };
   AbstractPath spath = path;
   spath.set_last_node_subset( subset );

   assert( spath.get_number_elements< ColVariable >( reference ) ==
           subset.size() );
   assert( spath.get_element< ColVariable >( reference , 0 ) == first );
   assert( spath.get_element< ColVariable >( reference , 1 ) ==
           inspection::get_element< ColVariable >( reference , true , g ,
                                                  group_size - 1 ) );

   const auto indices = spath.get_resolved_indices< ColVariable >( reference );
   assert( indices == subset );

   netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
   auto ncgroup = ncFile.addGroup( "Path" );
   spath.serialize( ncgroup );
   AbstractPath round_trip( ncgroup );
   assert( round_trip == spath );
   assert( round_trip.get_number_elements< ColVariable >( reference ) ==
           subset.size() );
   assert( round_trip.get_resolved_indices< ColVariable >( reference ) ==
           subset );
  }

  return;  // at least one group exercised
  }
}

/*--------------------------------------------------------------------------*/

void test_block_multi_selection( Block * reference ) {
 using Index = Block::Index;

 const auto & nested = reference->get_nested_Blocks();
 if( nested.size() < 2 )
  return;

 // ---- contiguous Block range [ 0 , nested.size() ) on the last 'B' node ----
 {
  AbstractPath path( nested[ 0 ] , reference );
  path.set_last_node_range( 0 , nested.size() );

  assert( path.get_number_elements< Block >( reference ) == nested.size() );
  for( Index j = 0 ; j < nested.size() ; ++j )
   assert( path.get_element< Block >( reference , j ) == nested[ j ] );

  const auto indices = path.get_resolved_indices< Block >( reference );
  assert( indices.size() == nested.size() );
  for( Index j = 0 ; j < nested.size() ; ++j )
   assert( indices[ j ] == j );

  test_serialization( path );
  AbstractPath round_trip;
  {
   netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
   auto group = ncFile.addGroup( "Path" );
   path.serialize( group );
   round_trip = AbstractPath( group );
  }
  assert( round_trip == path );
  assert( round_trip.get_number_elements< Block >( reference ) ==
          nested.size() );
  for( Index j = 0 ; j < nested.size() ; ++j )
   assert( round_trip.get_element< Block >( reference , j ) == nested[ j ] );
 }

 // ---- explicit, possibly non-contiguous, Block subset on the last node -----
 {
  std::vector< Index > subset;
  for( Index k = 0 ; k < nested.size() ; k += 2 )
   subset.push_back( k );

  AbstractPath path( nested[ 0 ] , reference );
  path.set_last_node_subset( subset );

  assert( path.get_number_elements< Block >( reference ) == subset.size() );
  for( Index j = 0 ; j < subset.size() ; ++j )
   assert( path.get_element< Block >( reference , j ) == nested[ subset[ j ] ] );

  assert( path.get_resolved_indices< Block >( reference ) == subset );

  AbstractPath round_trip;
  {
   netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
   auto group = ncFile.addGroup( "Path" );
   path.serialize( group );
   round_trip = AbstractPath( group );
  }
  assert( round_trip == path );
  assert( round_trip.get_number_elements< Block >( reference ) ==
          subset.size() );
  for( Index j = 0 ; j < subset.size() ; ++j )
   assert( round_trip.get_element< Block >( reference , j ) ==
           nested[ subset[ j ] ] );
 }
}

/*--------------------------------------------------------------------------*/

std::set< std::pair< Block * , Block * > > visited_blocks;

bool visited( Block * block , Block * reference_block ) {
 if( visited_blocks.find( std::make_pair( block , reference_block ) ) !=
     visited_blocks.end() )
  return( true );
 visited_blocks.insert( std::make_pair( block , reference_block ) );
 return( false );
}

/*--------------------------------------------------------------------------*/

void test( Block * block , Block * reference_block ) {
 if( visited( block , reference_block ) )
  return;

 test_paths( block , reference_block );

 if( const auto objective =
     dynamic_cast< FRealObjective * >( block->get_objective() ) ) {
  if( const auto function =
      dynamic_cast< BendersBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    test( inner_block , reference_block );
    test( inner_block , inner_block );
    if( reference_block != block ) {
     test( inner_block , block );
    }
   }
  }
  else if( const auto function =
           dynamic_cast< LagBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    test( inner_block , reference_block );
    test( inner_block , inner_block );
    if( reference_block != block ) {
     test( inner_block , block );
    }
   }
  }
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  test( nested_block , reference_block );
  if( block != reference_block )
   test( nested_block , block );
  test( nested_block , nested_block );
 }
}

/*--------------------------------------------------------------------------*/

void test_everyone_has_function( Block * block ) {

 for( const auto & group : block->get_static_constraints() )
  assert( un_any_const_static
          ( group ,
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            } ,
            un_any_type< FRowConstraint >() ) );

 for( const auto & group : block->get_dynamic_constraints() )
  assert( un_any_const_dynamic
          ( group ,
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            } ,
            un_any_type< FRowConstraint >() ) );

 {
  auto objective = static_cast< FRealObjective * >( block->get_objective() );
  if( objective ) {
   assert( objective->get_function() != nullptr );
  }
 }

 for( const auto nested_block : block->get_nested_Blocks() )
  test_everyone_has_function( nested_block );
}

/*--------------------------------------------------------------------------*/

void print_tree( Block * block , std::string spaces = "" ) {
 std::cout << block << std::endl;

 if( const auto objective =
     dynamic_cast< FRealObjective * >( block->get_objective() ) ) {
  if( const auto function =
      dynamic_cast< BendersBFunction * >( objective->get_function() ) ) {
   if( const auto inner_block = function->get_inner_block() ) {
    std::cout << spaces << "-> BendersBFunction " << function << std::endl;
    std::cout << spaces + "   " << "-> ";
    print_tree( inner_block , spaces + "   "  + "   " );
   }
  }
  else if( const auto function =
           dynamic_cast< LagBFunction * >( objective->get_function() ) )
   if( const auto inner_block = function->get_inner_block() ) {
    std::cout << spaces << "-> LagBFunction " << function << std::endl;
    std::cout << spaces + "   " << "-> ";
    print_tree( inner_block , spaces + "   "  + "   " );
   }
 }

 auto & nested_blocks = block->get_nested_Blocks();
 if( ! nested_blocks.empty() ) {
  for( auto son : nested_blocks ) {
   std::cout << spaces << "-> ";
   print_tree( son , spaces + "   " );
  }
 }
}

/*--------------------------------------------------------------------------*/

void simple_full_test() {

 AbstractBlockRandomNumberGenerator generator;

 generator.static_constraint_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } , { 3 , 6 } , { 4 , 7 } , 0 );

 generator.static_variable_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } , { 3 , 6 } , { 4 , 7 } , 2 );

 generator.dynamic_constraint_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 3 } , { 3 , 6 } , { 4 , 7 } , 3 );

 generator.dynamic_variable_generator =
  new ElementGenerator< std::mt19937 , Int >
  ( { 4 , 7 } , { 0 , 2 } , { 4 , 7 } , { 2 , 4 } , { 3 , 6 } , { 4 , 7 } , 4 );

 generator.function_generator =
  new FunctionGenerator< std::mt19937 , Int >( { 0 , 2 } , 5 );

 generator.num_nested_block_generator =
  new NumNestedBlockGenerator< std::mt19937 , Int >( { 4 , 7 } , 6 );

 AbstractBlockGenerator ab_generator( & generator );
 auto block = ab_generator.generate( 2 );

 test_everyone_has_function( block );

 test( block , block );

 test_block_multi_selection( block );
 test_variable_multi_selection( block );

 delete block;
}

/*--------------------------------------------------------------------------*/

// writes into group the path with the given node types, element and range
// indices, and the given (string) group indices

void write_named_path( netCDF::NcGroup group ,
                       const std::vector< char > & types ,
                       const std::vector< std::string > & names ,
                       const std::vector< unsigned int > & elements ,
                       const std::vector< unsigned int > & ranges ) {
 auto dim = group.addDim( "PathTotalLength" , types.size() );
 group.addVar( "PathNodeTypes" , netCDF::NcChar() , dim ).putVar(
                                                               types.data() );
 std::vector< const char * > cnames( names.size() );
 for( std::size_t i = 0 ; i < names.size() ; ++i )
  cnames[ i ] = names[ i ].c_str();
 group.addVar( "PathGroupIndices" , netCDF::NcString() , dim ).putVar(
                                                              cnames.data() );
 group.addVar( "PathElementIndices" , netCDF::NcUint() , dim ).putVar(
                                                            elements.data() );
 group.addVar( "PathRangeIndices" , netCDF::NcUint() , dim ).putVar(
                                                              ranges.data() );
 }

/*--------------------------------------------------------------------------*/

// paths addressing nested Blocks and groups of Variable by name, by index,
// or by a mix of the two

void test_names( void ) {

 auto root = new AbstractBlock();
 std::vector< ColVariable > * target_group = nullptr;
 for( const std::string name : { "alpha" , "beta" } ) {
  auto son = new AbstractBlock( root );
  son->set_name( std::string( name ) );
  son->add_static_variable( * new std::vector< ColVariable >( 2 ) , "x" );
  target_group = new std::vector< ColVariable >( 3 );
  son->add_static_variable( * target_group , "y" );
  root->add_nested_Block( son );
  }
 // the target is the last element of group "y" of the nested Block "beta"
 const Variable * target = & target_group->back();

 netCDF::NcFile ncFile( "ncfile_path_names_test.txt" ,
                        netCDF::NcFile::replace );

 // the numeric path, whose node types and element indices are reused
 AbstractPath numeric( target , root );
 assert( numeric.get_element< Variable >( root ) == target );
 auto ng = ncFile.addGroup( "Numeric" );
 numeric.serialize( ng );

 const auto n = ng.getDim( "PathTotalLength" ).getSize();
 std::vector< char > types( n );
 std::vector< unsigned int > elements( n ) , ranges( n );
 ng.getVar( "PathNodeTypes" ).getVar( types.data() );
 ng.getVar( "PathElementIndices" ).getVar( elements.data() );
 ng.getVar( "PathRangeIndices" ).getVar( ranges.data() );

 // the string group indices: bname for the 'B' node, vname for the 'V' one
 auto names = [ & types ]( const std::string & bname ,
                           const std::string & vname ) {
  std::vector< std::string > result;
  for( auto t : types ) {
   assert( ( t == 'B' ) || ( t == 'V' ) );
   result.push_back( t == 'B' ? bname : vname );
   }
  return( result );
  };

 const std::vector< std::pair< std::string , std::string > > cases = {
  { "beta" , "y" } , { "1" , "y" } , { "beta" , "1" } , { "1" , "1" } };

 std::vector< AbstractPath > paths;
 for( std::size_t k = 0 ; k < cases.size() ; ++k ) {
  auto g = ncFile.addGroup( "Named" + std::to_string( k ) );
  write_named_path( g , types , names( cases[ k ].first , cases[ k ].second ) ,
                    elements , ranges );
  paths.emplace_back( g );
  assert( paths.back().get_element< Variable >( root ) == target );
  }

 // a name that no nested Block has
 {
  auto g = ncFile.addGroup( "Unknown" );
  write_named_path( g , types , names( "gamma" , "y" ) , elements , ranges );
  AbstractPath path( g );
  bool thrown = false;
  try {
   path.get_element< Variable >( root );
   }
  catch( const std::invalid_argument & ) {
   thrown = true;
   }
  assert( thrown );
  }

 // a path with names keeps them through serialize() and deserialize()
 {
  auto g = ncFile.addGroup( "RoundTrip" );
  paths.front().serialize( g );
  AbstractPath path( g );
  assert( path == paths.front() );
  assert( path.get_element< Variable >( root ) == target );
  }

 // a vector mixing a path with names and a numeric one: the unnamed nodes
 // are written as the decimal form of their index
 {
  auto g = ncFile.addGroup( "Vector" );
  AbstractPath::serialize( std::vector< AbstractPath >{ paths.front() ,
                                                        numeric } , g );
  const auto read = AbstractPath::vector_deserialize( g );
  assert( read.size() == 2 );
  for( const auto & path : read )
   assert( path.get_element< Variable >( root ) == target );
  }

 delete root;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_names();
 simple_full_test();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_AbstractPath.cpp ---------------------*/
/*--------------------------------------------------------------------------*/

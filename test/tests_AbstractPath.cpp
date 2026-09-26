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
#include "OneVarConstraint.h"

#include <iostream>
#include <list>
#include <memory>
#include <string>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

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
/// the same round trip for many paths at once, through one file
/** Creating a netCDF file costs far more than writing a path into it, so the
 * paths go through one file in many thousands at a time, in the format of a
 * vector of AbstractPath, and each of them is compared with its own
 * reading. */

void test_serialization( const std::vector< AbstractPath > & paths ) {
 netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
 auto group = ncFile.addGroup( "Paths" );
 AbstractPath::serialize( paths , group );
 const auto read = AbstractPath::vector_deserialize( group );
 assert( read.size() == paths.size() );
 for( decltype( paths.size() ) i = 0 ; i < paths.size() ; ++i )
  assert( read[ i ] == paths[ i ] );
}

/*--------------------------------------------------------------------------*/

/// the paths made so far and not yet written and read back
std::vector< AbstractPath > pending_paths;

/// writes and reads back the pending paths, and forgets them
void flush_paths( void ) {
 test_serialization( pending_paths );
 pending_paths.clear();
}

/// adds one path in 8 to the pending ones, flushing them when they are many
/** Every path is checked by get_element() where it is made, which costs
 * little; the round trip through netCDF costs a few calls to the library per
 * path, and there are hundreds of thousands of them, so it is done for one
 * path in 8, at a fixed stride over the order in which they are made. */
void add_path( const AbstractPath & path ) {
 static unsigned long made = 0;
 if( made++ % 8 )
  return;
 pending_paths.push_back( path );
 if( pending_paths.size() >= 16384 )
  flush_paths();
}

/*--------------------------------------------------------------------------*/

void test_paths( Block * block , Block * reference_block ) {

 for( const auto & group : block->get_static_variable_groups() )
  assert( group->for_each_as< ColVariable >(
           [ reference_block ]( ColVariable & v ) {
            AbstractPath path( & v , reference_block );
            assert( & v == path.get_element< Variable >( reference_block ) );
            add_path( path );
            } ) );

 for( const auto & group : block->get_static_constraint_groups() )
  assert( group->for_each_as< FRowConstraint >(
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             add_path( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             add_path( path );
             }
            }  ) );

 for( const auto & group : block->get_dynamic_variable_groups() )
  assert( group->for_each_as< ColVariable >(
            [ reference_block ]( ColVariable & v ) {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Variable >( reference_block ) );
             add_path( path );
            }  ) );

 for( const auto & group : block->get_dynamic_constraint_groups() )
  assert( group->for_each_as< FRowConstraint >(
            [ reference_block ]( FRowConstraint & v ) {
             {
             AbstractPath path( & v , reference_block );
             assert( & v == path.get_element< Constraint >( reference_block ) );
             add_path( path );
             }

             {
             auto function = v.get_function();
             AbstractPath path( function , reference_block );
             assert( function == path.get_element< Function >
                     ( reference_block ) );
             add_path( path );
             }
            }  ) );

 {
  auto objective = block->get_objective();
  AbstractPath path( objective , reference_block );
  auto e = path.get_element< Objective >( reference_block );
  assert( objective == e );
  add_path( path );
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
  add_path( path );
 }

 {
  AbstractPath path( block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == block );
  add_path( path );
 }

 for( const auto nested_block : block->get_nested_Blocks() ) {
  AbstractPath path( nested_block , reference_block );
  const auto retrieved_block = path.get_element< Block >( reference_block );
  assert( retrieved_block == nested_block );
  add_path( path );
 }

 if( const auto pfb = dynamic_cast< PolyhedralFunctionBlock * >( block ) ) {
  const auto & function = pfb->get_PolyhedralFunction();
  AbstractPath path( & function , reference_block );
  const auto retrieved_function =
   path.get_element< Function >( reference_block );
  assert( retrieved_function == & function );
  add_path( path );
 }
}

/*--------------------------------------------------------------------------*/

void test_variable_multi_selection( Block * reference ) {
 using Index = Block::Index;

 // Find a static ColVariable group of size >= 2 in the reference Block;
 // skip the test if no such group is available.
 const auto & static_vars = reference->get_static_variable_groups();
 for( Index g = 0 ; g < static_vars.size() ; ++g ) {
  const auto group_size = inspection::get_element_size< ColVariable >(
					       reference , true , g );
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

 for( const auto & group : block->get_static_constraint_groups() )
  assert( group->for_each_as< FRowConstraint >(
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            }  ) );

 for( const auto & group : block->get_dynamic_constraint_groups() )
  assert( group->for_each_as< FRowConstraint >(
            []( FRowConstraint & v ) {
             assert( v.get_function() != nullptr );
            }  ) );

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
 flush_paths();

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
/*------------------------- THE EDGES OF A PATH ----------------------------*/
/*--------------------------------------------------------------------------*/

/// writes a path and reads it back

static AbstractPath round_trip( const AbstractPath & path )
{
 netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
 auto group = ncFile.addGroup( "Path" );
 path.serialize( group );
 return( AbstractPath( group ) );
 }

/*--------------------------------------------------------------------------*/

// writes into group the path with the given node types and numeric group,
// element and range indices

void write_numeric_path( netCDF::NcGroup group ,
                         const std::vector< char > & types ,
                         const std::vector< unsigned int > & groups ,
                         const std::vector< unsigned int > & elements ,
                         const std::vector< unsigned int > & ranges ) {
 auto dim = group.addDim( "PathTotalLength" , types.size() );
 group.addVar( "PathNodeTypes" , netCDF::NcChar() , dim ).putVar(
                                                               types.data() );
 group.addVar( "PathGroupIndices" , netCDF::NcUint() , dim ).putVar(
                                                              groups.data() );
 group.addVar( "PathElementIndices" , netCDF::NcUint() , dim ).putVar(
                                                            elements.data() );
 group.addVar( "PathRangeIndices" , netCDF::NcUint() , dim ).putVar(
                                                              ranges.data() );
 }

/*--------------------------------------------------------------------------*/

// an empty contiguous range on the last node selects nothing, and says so
// before and after the round trip through netCDF

void test_empty_range( void ) {
 AbstractBlock block;
 auto x = new std::vector< ColVariable >( 4 );
 block.add_static_variable( *x , "x" );

 AbstractPath path( & ( *x )[ 0 ] , & block );
 path.set_last_node_range( 2 , 2 );
 assert( path.get_number_elements< ColVariable >( & block ) == 0 );
 assert( path.get_resolved_indices< ColVariable >( & block ).empty() );

 const auto back = round_trip( path );
 assert( back == path );
 assert( back.get_number_elements< ColVariable >( & block ) == 0 );
 assert( back.get_resolved_indices< ColVariable >( & block ).empty() );

 // the empty range at the end of the group, and one that is not empty
 path.set_last_node_range( 4 , 4 );
 assert( path.get_number_elements< ColVariable >( & block ) == 0 );
 path.set_last_node_range( 1 , 4 );
 assert( path.get_number_elements< ColVariable >( & block ) == 3 );
 assert( path.get_element< ColVariable >( & block , 2 ) == & ( *x )[ 3 ] );

 // an end before the start is refused
 bool refused = false;
 try { path.set_last_node_range( 3 , 1 ); }
 catch( const std::logic_error & ) { refused = true; }
 assert( refused );

 std::cout << "empty range: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

// an empty explicit subset on the last node: after set_last_node_subset(),
// get_number_elements() is the size of the subset [see the method], i.e., 0

void test_empty_subset( void ) {
 AbstractBlock block;
 auto x = new std::vector< ColVariable >( 4 );
 block.add_static_variable( *x , "x" );

 AbstractPath path( & ( *x )[ 1 ] , & block );
 path.set_last_node_subset( { 0 , 3 } );
 assert( path.get_number_elements< ColVariable >( & block ) == 2 );

 path.set_last_node_subset( {} );
 assert( path.get_number_elements< ColVariable >( & block ) == 0 );
 assert( path.get_resolved_indices< ColVariable >( & block ).empty() );

 // and the path goes through netCDF selecting nothing
 const auto back = round_trip( path );
 assert( back == path );
 assert( back.get_number_elements< ColVariable >( & block ) == 0 );

 // the same on a 'B' node, and on one that targets the reference Block
 auto root = new AbstractBlock;
 root->add_nested_Block( new AbstractBlock( root ) );
 root->add_nested_Block( new AbstractBlock( root ) );
 AbstractPath to_son( root->get_nested_Blocks()[ 1 ] , root );
 to_son.set_last_node_subset( {} );
 assert( to_son.get_number_elements< Block >( root ) == 0 );
 assert( to_son.get_resolved_indices< Block >( root ).empty() );
 assert( round_trip( to_son ).get_number_elements< Block >( root ) == 0 );
 AbstractPath to_root( root , root );
 to_root.set_last_node_subset( {} );
 assert( to_root.get_number_elements< Block >( root ) == 0 );
 delete root;

 std::cout << "empty subset: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

// indices that are not there: a group index past the groups of the Block is
// refused when the path is resolved, an element index past the elements of
// its group resolves to nothing, and so does a nested Block past the last
// one on the last node

void test_out_of_range( void ) {
 auto block = new AbstractBlock;
 auto x = new std::vector< ColVariable >( 3 );
 block->add_static_variable( *x , "x" );
 block->add_nested_Block( new AbstractBlock( block ) );

 netCDF::NcFile ncFile( "ncfile_path_range_test.txt" ,
                        netCDF::NcFile::replace );
 const auto inf = Inf< unsigned int >();

 // the group is not there
 {
  auto g = ncFile.addGroup( "Group" );
  write_numeric_path( g , { 'V' } , { 7 } , { 0 } , { 1 } );
  AbstractPath path( g );
  bool refused = false;
  try { path.get_element< Variable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  }

 // the group is not there, and the range goes to its end: counting the
 // elements asks the group, which is refused
 {
  auto g = ncFile.addGroup( "ToTheEnd" );
  write_numeric_path( g , { 'V' } , { 7 } , { 0 } , { inf } );
  AbstractPath path( g );
  bool refused = false;
  try { path.get_number_elements< ColVariable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  refused = false;
  try { path.get_resolved_indices< ColVariable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  }

 // the group is not there, and it is named by its index
 {
  auto g = ncFile.addGroup( "Named" );
  write_named_path( g , { 'V' } , { "7" } , { 0 } , { 1 } );
  AbstractPath path( g );
  bool refused = false;
  try { path.get_element< Variable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  }

 // the element is not there
 {
  auto g = ncFile.addGroup( "Element" );
  write_numeric_path( g , { 'V' } , { 0 } , { 3 } , { 4 } );
  AbstractPath path( g );
  assert( ! path.get_element< Variable >( block ) );
  assert( path.get_element< Variable >( block , 0 ) == nullptr );
  }

 // a nested Block that is not there in the middle of the path
 {
  auto g = ncFile.addGroup( "Middle" );
  write_numeric_path( g , { 'B' , 'V' } , { 3 , 0 } , { inf , 0 } ,
		      { inf , 1 } );
  AbstractPath path( g );
  bool refused = false;
  try { path.get_element< Variable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  refused = false;
  try { path.get_number_elements< ColVariable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  refused = false;
  try { path.get_resolved_indices< ColVariable >( block ); }
  catch( const std::invalid_argument & ) { refused = true; }
  assert( refused );
  }

 // the nested Block is not there
 {
  auto g = ncFile.addGroup( "Block" );
  write_numeric_path( g , { 'B' } , { 1 } , { inf } , { inf } );
  AbstractPath path( g );
  assert( ! path.get_element< Block >( block ) );
  AbstractPath first( block->get_nested_Blocks()[ 0 ] , block );
  assert( first.get_element< Block >( block ) ==
	  block->get_nested_Blocks()[ 0 ] );
  first.set_last_node_range( 0 , 2 );
  assert( first.get_element< Block >( block , 1 ) == nullptr );
  }

 delete block;
 std::cout << "indices out of range: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

// paths to the OneVarConstraint, static and dynamic: each resolves to its
// element, as a Constraint and as its own type, before and after netCDF

void test_one_var_constraint( void ) {
 AbstractBlock block;
 auto x = new std::vector< ColVariable >( 3 );
 block.add_static_variable( *x , "x" );

 auto boxes = new std::vector< BoxConstraint >( 3 );
 for( Block::Index i = 0 ; i < 3 ; ++i ) {
  ( *boxes )[ i ].set_variable( & ( *x )[ i ] );
  ( *boxes )[ i ].set_lhs( 0 );
  ( *boxes )[ i ].set_rhs( 1 + i );
  }
 block.add_static_constraint( *boxes , "box" );

 auto lbs = new std::list< LBConstraint >( 2 );
 for( auto & lb : *lbs ) {
  lb.set_variable( & ( *x )[ 0 ] );
  lb.set_lhs( -1 );
  }
 block.add_dynamic_constraint( *lbs , "lb" );

 for( auto & box : *boxes ) {
  AbstractPath path( & box , & block );
  assert( path.get_element< Constraint >( & block ) == & box );
  assert( path.get_element< BoxConstraint >( & block ) == & box );
  assert( path.get_element< OneVarConstraint >( & block ) == & box );
  const auto back = round_trip( path );
  assert( back == path );
  assert( back.get_element< BoxConstraint >( & block ) == & box );
  }

 AbstractPath all( & boxes->front() , & block );
 all.set_last_node_range( 0 , 3 );
 assert( all.get_number_elements< BoxConstraint >( & block ) == 3 );
 assert( all.get_element< BoxConstraint >( & block , 2 ) == & boxes->back() );

 for( auto & lb : *lbs ) {
  AbstractPath path( & lb , & block );
  assert( path.get_element< LBConstraint >( & block ) == & lb );
  assert( round_trip( path ).get_element< Constraint >( & block ) == & lb );
  }

 std::cout << "OneVarConstraint: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

// a path to a dynamic element is by position [see deserialize()]: after a
// removal before it, the path gives the element that is now in that
// position, and a path past the end of what is left gives nothing

void test_after_a_dynamic_removal( void ) {
 AbstractBlock block;
 auto y = new std::list< ColVariable >( 4 );
 block.add_dynamic_variable( *y , "y" );
 std::vector< ColVariable * > was;
 for( auto & v : *y )
  was.push_back( & v );

 AbstractPath second( was[ 2 ] , & block );
 AbstractPath last( was[ 3 ] , & block );
 assert( second.get_element< ColVariable >( & block ) == was[ 2 ] );
 assert( last.get_element< ColVariable >( & block ) == was[ 3 ] );

 block.remove_dynamic_variables( *y , Block::Subset( { 1 } ) , true , eNoMod );
 assert( y->size() == 3 );

 assert( second.get_element< ColVariable >( & block ) == was[ 3 ] );
 assert( ! last.get_element< ColVariable >( & block ) );

 // the path made now to the same element says its new position
 AbstractPath again( was[ 2 ] , & block );
 assert( again.get_element< ColVariable >( & block ) == was[ 2 ] );
 assert( again.get_resolved_indices< ColVariable >( & block ) ==
	 std::vector< Block::Index >( { 1 } ) );
 assert( ! ( again == second ) );

 // the range to the end of the group follows its size
 AbstractPath tail( was[ 0 ] , & block );
 tail.set_last_node_range( 0 , Inf< Block::Index >() );
 assert( tail.get_number_elements< ColVariable >( & block ) == 3 );

 std::cout << "path after a dynamic removal: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

// a vector of no path goes through netCDF and comes back as a vector of no
// path, in both forms of vector_deserialize()

void test_empty_vector( void ) {
 test_serialization( std::vector< AbstractPath >() );

 netCDF::NcFile ncFile( "ncfile_path_test.txt" , netCDF::NcFile::replace );
 auto group = ncFile.addGroup( "Paths" );
 AbstractPath::serialize( std::vector< AbstractPath >() , group );
 std::vector< std::unique_ptr< AbstractPath > > read;
 read.emplace_back( std::make_unique< AbstractPath >() );
 AbstractPath::vector_deserialize( group , read );
 assert( read.empty() );

 std::cout << "empty vector of paths: OK" << std::endl;
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 test_names();
 test_empty_range();
 test_empty_subset();
 test_out_of_range();
 test_one_var_constraint();
 test_after_a_dynamic_removal();
 test_empty_vector();
 simple_full_test();

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_AbstractPath.cpp ---------------------*/
/*--------------------------------------------------------------------------*/

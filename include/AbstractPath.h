/*--------------------------------------------------------------------------*/
/*-------------------------- File AbstractPath.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the AbstractPath class, which represent a path from a Block
 * to one of the elements of its "abstract" representation: a Block (which
 * is actually also an element of the "physical" representation), a
 * Constraint, a Variable, an Objective, or a Function. The element can
 * directly belong to the Block itself (even be the Block itself) or belong
 * to any of its sub-Block, recursively.
 *
 * Since AbstractPath has to scan the "abstract" representation to work, it
 * has to boost::any_cast<> (in particular, Constraint and Variable), and
 * therefore it has to have a list of the kind of types that they may have.
 * Hence, this class at any point in time works only with a specific subset
 * of those classes, and if new types need to be handled, then the class has to
 * be manually updated. This is made a bit easier by the two macros
 *
 *     Constraint_Derived_Classes
 *     Variable_Derived_Classes
 *
 * defined in this header file (and immediately un-defined at the end).
 *
 * Also, AbstractPath has a specific management for:
 *
 * - Function that appears in the Constraint and Objective, i.e.,
 *   FRealObjective and FRowConstraint;
 *
 * - Function that have a Block (are a Block), i.e., BendersBFunction and
 *   LagBFunction.
 *
 * - PolyhedralFunction, if it is part of a PolyhedralFunctionBlock.
 *
 * Again, should more type of this kind of stuff be defined that needs
 * handling, this class would have to properly manually extend.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato, Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AbstractPath
 #define __AbstractPath
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"
#include "BlockInspection.h"
#include "PolyhedralFunctionBlock.h"
#include "SMSTypedefs.h"

#include <iterator>
#include <vector>
#include <netcdf>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS AbstractPath ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// A path from a Block to one of its elements
/** The AbstractPath represents a path from a Block, here referred to as the
 * reference Block, to one of its elements (the target element): a Block, a
 * Constraint, a Variable, an Objective, or a Function. The target element can
 * directly belong to the reference Block itself (even be the reference Block
 * itself) or belong to any of its sons, recursively. The reference Block is
 * not explicitly represented in the path. In fact, the representation of the
 * path is independent from the reference Block. For the path to be
 * meaningful, the reference Block should be clear from the context. The
 * reference Block must be available when the path is constructed and when the
 * target element is retrieved. Furthermore, the type of the target element
 * cannot always be inferred from the path. The type of the target element,
 * therefore, must also be known from the context.
 *
 * The AbstractPath is particularly useful for the serialization and
 * deserialization of pointers to objects. If an object stores pointers to
 * other objects (for example, a Function has a set of pointers to Variable,
 * others have pointers to Block), these pointers cannot be serialized and
 * deserialized as such. Rather, an "abstract representation" of these
 * pointers has to be serialized, from which the pointers can be
 * reconstructed at deserialization time; this is the facility that
 * AbstractPath offers. The fact that the representation of the path is
 * independent from the reference Block facilitates its serialization and
 * deserialization, and makes it possible to use the same path to target
 * different objects (say, two Constraint "in the same position" in two
 * "twin" Block can be represented by the same AbstractPath).
 *
 * Since AbstractPath has to scan the "abstract" representation to work, it
 * has to boost::any_cast<> (in particular, Constraint and Variable), and
 * therefore it has to have a list of the kind of types that they may have.
 * Hence, this class at any point in time works only with a specific subset
 * of those classes, and if new types need be handled than the class has to
 * be manually updated. This is made a bit easier by the two macros
 *
 *     Constraint_Derived_Classes
 *     Variable_Derived_Classes
 *
 * defined in this header file (and immediately un-defined at the end).
 *
 * Also, AbstractPath has a specific management for:
 *
 * - Function that appear in the Constraint and Objective, i.e.,
 *   FRealObjective and FRowConstraint;
 *
 * - Function that have a Block (are a Block), i.e., BendersBFunction and
 *   LagBFunction.
 *
 * - PolyhedralFunction, if it is part of a PolyhedralFunctionBlock.
 *
 * Again, should more type of this kind of stuff be defined that needs
 * handling, this class would have to properly manually extended.
 *
 * The path is defined as a sequence of nodes. Each node has one of the
 * following types:
 *
 * - 'O', if the node is associated with an Objective;
 *
 * - 'B', if the node is associated with a Block;
 *
 * - 'C', if the node is associated with a static Constraint;
 *
 * - 'c', if the node is associated with a dynamic Constraint;
 *
 * - 'V', if the node is associated with a static Variable;
 *
 * - 'v', if the node is associated with a dynamic Variable;
 *
 * Notice that, for Constraint and Variable, an upper case letter indicates
 * that the element is static, while a lower case letter indicates that the
 * element is dynamic. Also notice that there is no node associated with a
 * Function, but this does not prevent one from constructing a path to a
 * Function. Although the nodes in the path are stored in forward order, i.e.,
 * the first node is the origin of the path and the last one is the
 * destination, it is easier to understand the path if we look at it
 * backwards.
 *
 * Consider the path from some reference Block to some Variable. The last node
 * in this path necessarily has the 'V' or 'v' type, indicating this is a path
 * to a Variable. This Variable belongs to some Block and it is either static
 * or dynamic. This last node has all information needed to retrieve this
 * Variable from its father Block: an indication of whether the Variable is
 * static or dynamic, the index of the group to which it belongs, and the
 * index of the Variable within that group.
 *
 * Note: The index of a Variable (or Constraint) within a group is a single
 *       number and may not be entirely obvious which number it should be
 *       (specially for multi-dimensional arrays and (multi-dimensional)
 *       arrays of lists). Please refer to the comments of the deserialize()
 *       method for an explanation of the index of a Variable (or
 *       Constraint) within a group.
 *
 * A 'V' or 'v' node is always preceded by a 'B' node, unless it is the only
 * node in the path. If the path has only a single node, which is a 'V' or 'v'
 * node, then the Variable this path refers to is defined in the reference
 * Block. In other words, if the target Variable of the path belongs to the
 * reference Block, then the path is formed by a single node whose type is
 * either 'V' or 'v'.
 *
 * A 'B' node is associated with a Block and may contain the index of this
 * Block in the vector of nested Blocks of its father Block. There is one case
 * in which this node does not have this index, which is when the node is
 * associated with the reference Block. Since the reference Block is the root
 * of the three that contains the path, no allusion to the father of the
 * reference Block must be made. In this case, the index has the value
 * +Inf. If the index of a `B' node is not +Inf, then this node is necessarily
 * preceded by another `B' node, which is associated with the father of that
 * Block.
 *
 * An 'O' node, which is associated with an Objective, is either preceded by
 * a 'B' node (which is associated with the Block that owns that Objective) or
 * is the only node in the path. If it is the last node in the path, then the
 * target element is either this Objective or the Function in that Objective
 * (in which case that Objective is an FRealObjective). The type of the target
 * element must be known from the context. If this is not the last node in the
 * path, then the type of the next node in the path is 'B' and it is
 * associated with the inner Block of either a BendersBFunction or a
 * LagBFunction which is the Function of that Objective (and thus that
 * Objective is actually an FRealObjective).
 *
 * Finally, a 'C' or 'c' node, which is associated with a Constraint, has
 * characteristics pertaining both the 'V' (and 'v') and the 'O' nodes. Like
 * the 'V' (and 'v') node, it has an indication of whether it is static or
 * not, the index of the group to which it belongs, and the index of the
 * Constraint within that group (exactly as defined for Variables). Like an
 * 'O' node, a 'C' or 'c' node is either preceded by a 'B' node (which is
 * associated with the Block that owns that Constraint) or is the only node in
 * the path. If it is the last node in the path, then the target element is
 * either this Constraint or the Function in that Constraint (in which case
 * that Constraint is an FRowConstraint). The type of the target element must
 * be known from the context. If this is not the last node in the path, then
 * the type of the next node in the path is 'B' and it is associated with the
 * inner Block of either a BendersBFunction or a LagBFunction which is the
 * Function of that Constraint (and thus that Constraint is actually an
 * FRowConstraint).
 *
 * Multi-element selection on the LAST node
 * -----------------------------------------
 *
 * The last node of a path can optionally select more than one element of the
 * same kind, instead of a single one. This is supported in two equivalent
 * forms:
 *
 * - a contiguous range. On a 'V'/'v' or 'C'/'c' node the range is
 *   [ element_index , range_index ) over the element indices within the
 *   group; on a 'B' node it is [ group_index , range_index ) over the
 *   nested-Block indices of the Block that owns them. The legacy convention
 *   range_index == +Inf still means "a single element" for 'B' nodes (so that
 *   existing paths keep their meaning), while for 'V'/'v' or 'C'/'c' nodes
 *   range_index == +Inf means "to the end of the group".
 *
 * - an explicit, possibly non-contiguous, subset of element/Block indices.
 *   The subset is stored in the optional netCDF variables PathSubset and
 *   PathSubsetSizes (see deserialize()); when present on a node it takes
 *   precedence over the contiguous range. This is useful e.g. to drive a
 *   data mapping over a non-contiguous set of sibling Blocks.
 *
 * In both forms, get_number_elements() returns the number of selected
 * elements/Blocks and get_element( reference , offset ) returns the
 * offset-th one. Setters set_last_node_range() and set_last_node_subset()
 * make authoring these selections from C++ straightforward.
 */

class AbstractPath
{
/*--------------------------------------------------------------------------*/
/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*----------------------------- PRIVATE TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Types
    @{ */

 using Index = Block::Index;

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Static Fields
    @{ */

 /// Name of the netCDF dimension that stores the number of nodes in the path
 inline static const std::string path_dim_name = "PathDim";

 /// Name of the netCDF dimension that stores the sum of the lengths of the paths
 inline static const std::string path_total_length_dim_name = "PathTotalLength";

 /// Name of the netCDF variable that stores the starting positions of each path
 inline static const std::string path_start_name = "PathStart";

 /// Name of the netCDF variable that stores the array of types of nodes
 inline static const std::string node_type_name = "PathNodeTypes";

 /// Name of the netCDF variable that stores the array with group indices
 inline static const std::string group_index_name = "PathGroupIndices";

 /// Name of the netCDF variable that stores the array of first element indices
 inline static const std::string element_index_name = "PathElementIndices";

 /// Name of the netCDF variable that stores the array of last element indices
 inline static const std::string element_range_name = "PathRangeIndices";

 /// Name of the netCDF variable that stores, for each node, the size of the
 /// (optional) explicit subset of nested-Block indices selected by that node
 inline static const std::string subset_size_name = "PathSubsetSizes";

 /// Name of the netCDF dimension that stores the total number of elements in
 /// the (optional) explicit Block subsets of all nodes
 inline static const std::string subset_total_length_dim_name =
                                                       "PathSubsetTotalLength";

 /// Name of the netCDF variable that stores the concatenation of the
 /// (optional) explicit subsets of nested-Block indices of all nodes
 inline static const std::string subset_name = "PathSubset";

/** @} ---------------------------------------------------------------------*/
/*-------------------------- PRIVATE CLASSES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Classes
    @{ */

 /// Node represents a node in the path
 /** This class is used to represent a node in the path. A Node can be of four
  * main types, defined by the NodeType enum:
  *
  * 1. 'B', a Block
  * 2. 'C' or 'c', a Constraint
  * 3. 'V' or 'v', a Variable
  * 4. 'O', an Objective.
  *
  * A 'C' node is associated with a static Constraint, while a 'c' node is
  * associated with a dynamic Constraint. Likewise, a 'V' node is associated
  * with a static Variable, while a 'v' node is associated with a dynamic
  * Variable. */

 class Node {

 public:

/*--------------------------------------------------------------------------*/

  using NodeType = char;

  static constexpr NodeType eBlock = 'B';
  static constexpr NodeType eConstraint = 'C';
  static constexpr NodeType eVariable = 'V';
  static constexpr NodeType eObjective = 'O';

/*--------------------------------------------------------------------------*/

  Node() : type( 'N' ) , group_index( Inf< Index >() ) ,
           element_index( Inf< Index >() ) , range_index( Inf< Index >() ) {}

/*--------------------------------------------------------------------------*/

  Node( NodeType type , Index group_index , Index element_index ,
	Index range_index )
   : type( type ) , group_index( group_index ) ,
     element_index( element_index ) , range_index( range_index ) {}

/*--------------------------------------------------------------------------*/

  bool is_static( void ) const {
   if( type == 'c' || type == 'v' )
    return( false );
   return( true );
   }

/*--------------------------------------------------------------------------*/

  static NodeType to_static( NodeType type ) {
   return( std::toupper( type ) );
   }

/*--------------------------------------------------------------------------*/

  static NodeType to_dynamic( NodeType type ) {
   if( type == 'C' || type == 'V' )
    return( std::tolower( type ) );
   return( type );
   }

/*--------------------------------------------------------------------------*/

  static bool is_static( NodeType type ) {
   if( type == 'c' || type == 'v' )
    return( false );
   return( true );
   }

/*--------------------------------------------------------------------------*/

  static bool is_constraint( NodeType type ) {
   if( type == 'c' || type == 'C' )
    return( true );
   return( false );
   }

/*--------------------------------------------------------------------------*/

  static bool is_variable( NodeType type ) {
   if( type == 'v' || type == 'V' )
    return( true );
   return( false );
   }

/*--------------------------------------------------------------------------*/

  static bool is_block( NodeType type ) {
   return( type == eBlock );
   }

/*--------------------------------------------------------------------------*/

  static bool has_range( NodeType type ) {
   return( is_variable( type ) || is_constraint( type ) );
   }

/*--------------------------------------------------------------------------*/

  NodeType type;
  Index group_index;
  Index element_index;
  Index range_index;

 };

/** @} ---------------------------------------------------------------------*/
/*----------------------------- PRIVATE METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/

 template< class T >
 void add_node( const T * t , Node::NodeType type ) {
  const auto triple = inspection::get_element_index( t );
  const auto group_index = std::get< 1 >( triple );
  if( group_index < Inf< Index >() ) {
   group_indices.push_back( group_index );

   const auto element_index = std::get< 2 >( triple );
   element_indices.push_back( element_index );

   const auto is_static = std::get< 0 >( triple );
   if( is_static )
    type = Node::to_static( type );
   else
    type = Node::to_dynamic( type );
   node_types.push_back( type );

   // For nodes that support a range, range_index is the absolute one-past-last
   // element index, so a single element at position k has range_index = k + 1
   // (see get_number_elements()). For nodes that do not, range_index is +Inf.
   if( Node::has_range( type ) )
    range_indices.push_back( element_index + 1 );
   else
    range_indices.push_back( Inf< Index >() );
  }
  else
   throw( std::logic_error( "AbstractPath::add_node: Element not found." ) );
 }

/*--------------------------------------------------------------------------*/

 /// adds a node to this AbstractPath
 void add_node( Node::NodeType type ,
                Index group_index = Inf< Index >() ,
                Index element_index = Inf< Index >() ,
                Index range_index = Inf< Index >() ) {
  node_types.push_back( type );
  group_indices.push_back( group_index );
  element_indices.push_back( element_index );
  range_indices.push_back( range_index );
 }

/*--------------------------------------------------------------------------*/

 /// reverses this AbstractPath
 void reverse( void ) {
  std::reverse( std::begin( node_types ), std::end( node_types ) );
  std::reverse( std::begin( group_indices ), std::end( group_indices ) );
  std::reverse( std::begin( group_index_names ) , std::end( group_index_names ) );
  std::reverse( std::begin( element_indices ), std::end( element_indices ) );
  std::reverse( std::begin( range_indices ), std::end( range_indices) );
  std::reverse( std::begin( node_subsets ), std::end( node_subsets ) );
 }

/*--------------------------------------------------------------------------*/

 /// returns the length of the given AbstractPath
 static Index length( const AbstractPath & path ) {
  return( path.length() );
 }

/*--------------------------------------------------------------------------*/

 /// returns the length of the given AbstractPath
 static Index length( const std::unique_ptr< AbstractPath > & path ) {
  if( path )
   return( path->length() );
  return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS  ---------------------------*/
/*--------------------------------------------------------------------------*/

 /// node_types[i] is the type of the i-th node in the path
 std::vector< Node::NodeType > node_types;

 /// group_indices[i] is the group index of the i-th node in the path
 std::vector< Index > group_indices;
 std::vector< std::string > group_index_names;

 /// element_indices[i] is the first element index of the i-th node in the path
 std::vector< Index > element_indices;

 /// range_indices[i] is the last element index of the i-th node in the path
 std::vector< Index > range_indices;

 /// node_subsets[i] is the (possibly empty) explicit subset of nested-Block
 /// indices selected by the i-th node, when it is a 'B' node that targets a
 /// non-contiguous set of sub-Blocks. When node_subsets[i] is non-empty, it
 /// takes precedence over the [ group_indices[ i ] , range_indices[ i ] )
 /// contiguous Block range. It is empty for every node that selects a single
 /// element or a contiguous range.
 std::vector< std::vector< Index > > node_subsets;

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*--------------------------- PUBLIC CLASSES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Classes
    @{ */

 /// APnetCDF is a struct to store netCDF dimensions and variables of paths
 /** This struct is used simply to store the netCDF dimensions and variables
  * used to represent an AbstractPath or a vector of AbstractPath. */

 struct APnetCDF {

  /// Number of paths
  Index NumPaths;

  /// Optional dimension indicating the number of paths
  netCDF::NcDim PathDim;

  /// PathStart[i] = the index where the i-th path starts
  netCDF::NcVar PathStart;

  /// Sum of the lengths of all paths
  netCDF::NcDim PathTotalLength;

  /// Variable storing the types of the nodes
  netCDF::NcVar PathNodeTypes;

  /// Variable storing the group indices
  netCDF::NcVar PathGroupIndices;

  /// Variable storing the first element indices
  netCDF::NcVar PathElementIndices;

  /// Variable storing the last element indices
  netCDF::NcVar PathRangeIndices;

  /// Variable storing, per node, the size of the explicit Block subset (0 if
  /// the node does not select an explicit subset of nested Blocks)
  netCDF::NcVar PathSubsetSizes;

  /// Sum of the sizes of the explicit Block subsets of all nodes
  netCDF::NcDim PathSubsetTotalLength;

  /// Variable storing the concatenation of the explicit Block subsets
  netCDF::NcVar PathSubset;
  };

/** @} ---------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING AbstractPath ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing AbstractPath
 *  @{ */

 AbstractPath() { }

/*--------------------------------------------------------------------------*/

 template< class T >
 AbstractPath( const T * t , const Block * reference_block ) {
  build( t , reference_block );
  }

/*--------------------------------------------------------------------------*/

 AbstractPath( const netCDF::NcGroup & group ) {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/

 AbstractPath( Index path_index , const APnetCDF & netCDFvars ) {
  deserialize( path_index , netCDFvars );
  }

/*--------------------------------------------------------------------------*/

 virtual ~AbstractPath() { }

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PUBLIC METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Methods
    @{ */

 /// returns the length of this AbstractPath
 /** This function returns the length of this AbstractPath, i.e., the number
  * of nodes in the path. */

 Index length( void ) const { return( node_types.size() ); }

/*--------------------------------------------------------------------------*/
 /// returns true if and only if this AbstractPath is empty
 /** This function returns true if and only if this AbstractPath is empty,
  * i.e., it does not represent a path to any object. */

 bool empty( void ) const { return( length() == 0 ); }

/*--------------------------------------------------------------------------*/
 /// clears this AbstractPath
 /** This function clears this AbstractPath making it an empty path. */

 void clear( void ) {
  node_types.clear();
  group_indices.clear();
  group_index_names.clear();
  element_indices.clear();
  range_indices.clear();
  node_subsets.clear();
  }

/*--------------------------------------------------------------------------*/
 /// returns the Node representation of the i-th node in this AbstractPath
 /** This function returns the Node representation of the i-th node in this
  * AbstractPath.
  *
  * @param block The pointer to the reference Block.
  *
  * @param i The index of the node of this AbstractPath whose Node
  *        representation is required.
  *
  * @return The Node representation of the i-th node in this AbstractPath.
  */

 Node get_node( const Block * block , const Index i ) const {
  assert( i < length() );

  Index group_index;
  if( ( ! group_index_names.empty() ) &&
      ( ! group_index_names[ i ].empty() ) ) {

   const bool is_static = Node::is_static( node_types[ i ] );

   if( Node::is_variable( node_types[ i ] ) )
    if( is_static )
    group_index = block->get_s_var_index( group_index_names[ i ] );
    else
     group_index = block->get_d_var_index( group_index_names[ i ] );

   else if( Node::is_constraint( node_types[ i ] ) )
    if( is_static )
     group_index = block->get_s_const_index( group_index_names[ i ] );
    else
     group_index = block->get_d_const_index( group_index_names[ i ] );

   else
    group_index = static_cast< Index >( std::stoul( group_index_names[ i ] ) );
  }
  else
   group_index = group_indices[ i ];

  return( Node( node_types[ i ] ,
                group_index ,
                element_indices[ i ] ,
                range_indices[ i ] ) );
 }

/*--------------------------------------------------------------------------*/

 /// returns the Node representation of the last node in this AbstractPath
 /** This function returns the Node representation of the last node in this
  * AbstractPath.
  *
  * @param block The pointer to the reference Block.
  *
  * @return The Node representation of the last node in this AbstractPath.
  */
 Node get_last_node( Block * block ) const {
  return( get_node( block , length() - 1 ) );
 }

/*--------------------------------------------------------------------------*/
 /// makes the last node of this path select a contiguous range of elements
 /** Makes the last node of this AbstractPath select the contiguous range
  * [ \p start , \p end ) of elements. The last node must be a 'B' node (in
  * which case the range is over the nested-Block indices of the Block that
  * owns them) or a Variable/Constraint node (in which case the range is over
  * the element indices within its group). After this call,
  * get_number_elements< T >() returns ( \p end - \p start ) and
  * get_element< T >( reference , k ) returns the ( \p start + k )-th element,
  * for k in { 0, ..., end - start - 1 }. Any explicit subset previously set on
  * the last node is cleared. */
 void set_last_node_range( Index start , Index end ) {
  if( empty() ||
      ( ! Node::is_block( node_types.back() ) &&
        ! Node::has_range( node_types.back() ) ) )
   throw( std::logic_error( "AbstractPath::set_last_node_range: the last node "
                            "does not support a range." ) );
  if( end < start )
   throw( std::logic_error( "AbstractPath::set_last_node_range: end < start." )
          );
  if( Node::is_block( node_types.back() ) )
   group_indices.back() = start;
  else
   element_indices.back() = start;
  range_indices.back() = end;
  if( ! node_subsets.empty() )
   node_subsets.back().clear();
 }

/*--------------------------------------------------------------------------*/
 /// makes the last node of this path select an explicit subset of elements
 /** Makes the last node of this AbstractPath select the explicit (possibly
  * non-contiguous) \p subset of indices. The last node must be a 'B' node (in
  * which case the indices are nested-Block indices of the Block that owns
  * them) or a Variable/Constraint node (in which case they are element indices
  * within its group). After this call, get_number_elements< T >() returns
  * subset.size() and get_element< T >( reference , k ) returns the element
  * with index subset[ k ]. An explicit subset takes precedence over any
  * contiguous range that may have been set on the same node; the first subset
  * element is also recorded as the node's "start" index so that consumers that
  * ignore subsets still resolve to the first selected element. */
 void set_last_node_subset( std::vector< Index > subset ) {
  if( empty() ||
      ( ! Node::is_block( node_types.back() ) &&
        ! Node::has_range( node_types.back() ) ) )
   throw( std::logic_error( "AbstractPath::set_last_node_subset: the last node "
                            "does not support a subset." ) );
  if( node_subsets.size() < length() )
   node_subsets.resize( length() );
  if( ! subset.empty() ) {
   if( Node::is_block( node_types.back() ) )
    group_indices.back() = subset.front();
   else
    element_indices.back() = subset.front();
  }
  node_subsets.back() = std::move( subset );
 }

/*--------------------------------------------------------------------------*/
 /// returns the explicit Block subset of the i-th node (empty if none)

 const std::vector< Index > & get_node_subset( Index i ) const {
  static const std::vector< Index > empty_subset;
  if( i < node_subsets.size() )
   return( node_subsets[ i ] );
  return( empty_subset );
 }

/*--------------------------------------------------------------------------*/

 /// constructs an AbstractPath for the given element
 /** This function constructs and AbstractPath for the given target element \p
  * t with respect to the given reference Block \p reference_block.
  *
  * @param t The pointer to the target element.
  *
  * @param reference_block The pointer to the reference Block.
  *
  * @return Returns the AbstractPath for the given target element \p t with
  *         respect to the given \p reference_block.
  */
 template< class T >
 void build( const T * t , const Block * reference_block ) {

  static_assert( std::is_base_of_v< Block , T > ||
                 std::is_base_of_v< Constraint , T > ||
                 std::is_base_of_v< Function , T > ||
                 std::is_base_of_v< Objective , T > ||
                 std::is_base_of_v< Variable , T > ,
                 "AbstractPath::build: the element type must be one of: "
                 "Block, Constraint, Function, Objective, Variable." );

  clear();

  if( ! t )
   return;

  if constexpr( std::is_base_of_v< Constraint , T > ) {
   add_node( t , Node::eConstraint );
  }
  else if constexpr( std::is_base_of_v< Variable , T > ) {
   add_node( t , Node::eVariable );
  }
  else if constexpr( std::is_base_of_v< Objective , T > ) {
   add_node( Node::eObjective );
  }
  else if constexpr( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( const auto constraint = dynamic_cast< FRowConstraint * >( observer ) ) {
    add_node( constraint , Node::eConstraint );
   }
   else if( dynamic_cast< FRealObjective * >( observer ) ) {
    add_node( Node::eObjective );
   }
   else if( dynamic_cast< PolyhedralFunctionBlock * >( observer ) ) {
    if( observer == reference_block )
     add_node( Node::eBlock );
   }
   else
    throw( std::logic_error( "AbstractPath::build: Unknown Observer of "
                             "given Function." ) );
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   if( t == reference_block ) {
    add_node( Node::eBlock , Inf< Index >() );
    return;
   }
   auto group_index = inspection::get_block_index( t );
   add_node( Node::eBlock , group_index );
  }

  Block * block;
  if constexpr( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( ! observer )
    throw( std::logic_error( "AbstractPath::build: Path not found. "
                             "Function has no Observer." ) );
   block = observer->get_Block();
  }
  else if constexpr( std::is_base_of_v< Block , T > )
   block = t->get_f_Block();
  else
   block = t->get_Block();

  while( block != reference_block ) {

   auto index = inspection::get_block_index( block );

   if( index < Inf< Index >() ) {
    // block has a father Block
    add_node( Node::eBlock , index );
    block = block->get_f_Block();
   }

   else {
    // block has no father. So, block must be either a BendersBFunction or a
    // LagBFunction.

    Observer * observer;

    if( const auto benders = dynamic_cast< BendersBFunction * >( block ) )
     observer = benders->get_Observer();
    else if( const auto lag = dynamic_cast< LagBFunction * >( block ) )
     observer = lag->get_Observer();
    else
     throw( std::logic_error( "AbstractPath::build: Path not found." ) );

    if( const auto frc = dynamic_cast< FRowConstraint * >( observer ) ) {
     add_node( frc , Node::eConstraint );
     block = frc->get_Block();
    }
    else if( const auto fro = dynamic_cast< FRealObjective * >( observer ) ) {
     add_node( Node::eObjective );
     block = fro->get_Block();
    }
    else
     throw( std::logic_error( "AbstractPath::build: Path not found." ) );
   }
  }

  reverse();
 }

/*--------------------------------------------------------------------------*/

 /**
 * Returns how many Variables(s) or Constraint(s) are specified by the range
 * in this AbstractPath.
 *
 * Interprets element_indices[ i ] as "start" and range_indices[ i ] as "end",
 * returning "end" - "start" if the node is of a Variable or Constraint type,
 * 1 otherwise.
 * Consider the dimension of the pointed Variables(s) or Constraint(s)
 * structure as "end" value, if range_indices[ i ] is Inf< Index >().
 */
 template< class T >
 Index get_number_elements( Block * reference ) const {

  if( length() == 0 )
   return( Inf< Index >() );

  auto block = reference;

  for( Index i = 0 ; i < length() - 1 ; ++i ) {

   const auto node = get_node( block , i );

   // Intermediate nodes can be: Block, Constraint, or Objective.

   if( node.type == Node::eBlock ) {
    assert( node.group_index < block->get_nested_Blocks().size() );
    block = block->get_nested_Blocks()[ node.group_index ];
   }
   else if( Node::is_constraint( node.type ) ) {
    auto constraint = inspection::get_element< Constraint >
     ( block , node.is_static() , node.group_index , node.element_index );

    if( const auto frowc =
        dynamic_cast< const FRowConstraint * >( constraint ) ) {
     auto function = frowc->get_function();

     if( const auto benders =
         dynamic_cast< const BendersBFunction * >( function ) )
      block = benders->get_inner_block();
     else if( const auto lag =
              dynamic_cast< const LagBFunction * >( function ) )
      block = lag->get_inner_block();
     else // not found
      return( Inf< Index >() );
        }
    else // not found
     return( Inf< Index >() );
   }
   else if( node.type == Node::eObjective ) {
    return( Inf< Index >() );
   }
  } // end for

  // Now, we analyze the last node in the path.

  const auto node = get_last_node( block );

  // A 'B' node may select a single Block (legacy), a contiguous range of
  // nested Blocks [ group_index , range_index ), or an explicit subset of
  // nested-Block indices (which takes precedence over the range).
  if( Node::is_block( node.type ) ) {
   const auto & subset = get_node_subset( length() - 1 );
   if( ! subset.empty() )
    return( subset.size() );
   if( node.range_index == Inf< Index >() )
    return( 1 );
   if( node.range_index < node.group_index )
    throw( std::logic_error(
     "AbstractPath::get_number_elements: invalid Block range for node ["
     + std::to_string( length() - 1 ) + "] since the ending index, i.e., the "
     "range_indices[" + std::to_string( length() - 1 ) + "], is less than the "
     "starting one, i.e., the group_indices[" + std::to_string( length() - 1 )
     + "]." ) );
   return( node.range_index - node.group_index );
  }

  if( ! Node::has_range( node.type ) )
   return( 1 );

  // A 'V'/'v' or 'C'/'c' node may select a single element, a contiguous range
  // [ element_index , range_index ) of elements within its group, or an
  // explicit (possibly non-contiguous) subset of element indices within its
  // group (which takes precedence over the range).
  const auto & subset = get_node_subset( length() - 1 );
  if( ! subset.empty() )
   return( subset.size() );

  Index start = node.element_index;
  Index end = node.range_index;

  if( end == Inf< Index >() )
   end = inspection::get_element_size< T >( block ,
                                            node.is_static() ,
                                            node.group_index );

  if( end < start )
   throw( std::logic_error(
    "AbstractPath::get_number_elements: invalid range provided for node ["
    + std::to_string( node_types.size() - 1 ) + "]" + " of type " +
    std::string( 1 , node.type ) + " since the ending range index, " +
    "i.e., the range_indices[" + std::to_string( range_indices.size() - 1 ) +
    "], is less than the starting one, i.e., the element_indices[" +
    std::to_string( element_indices.size() - 1 ) + "]." ) );

  return( end - start );
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the target element of the given AbstractPath
 /** This function returns a pointer to the object of type T that is the
  * target element of the given \p path with respect to the given \p reference
  * Block.
  *
  * @param path The AbstractPath to the target element with respect to the
  *        given \p reference Block.
  *
  * @param reference The pointer to the reference Block.
  *
  * @param The pointer to the target element.
  */
 template< class T >
 T * get_element( Block * reference , Index offset = 0 ) const {

  if( length() == 0 )
   return( nullptr );

  auto block = reference;

  for( Index i = 0 ; i < length() - 1 ; ++i ) {

   const auto node = get_node( block , i );

   // Intermediate nodes can be: Block, Constraint, or Objective.

   if( node.type == Node::eBlock ) {
    assert( node.group_index < block->get_nested_Blocks().size() );
    block = block->get_nested_Blocks()[ node.group_index ];
   }
   else if( Node::is_constraint( node.type ) ) {
    auto constraint = inspection::get_element< Constraint >
     ( block , node.is_static() , node.group_index , node.element_index );

    if( const auto frowc =
        dynamic_cast< const FRowConstraint * >( constraint ) ) {
     auto function = frowc->get_function();

     if( const auto benders =
         dynamic_cast< const BendersBFunction * >( function ) )
      block = benders->get_inner_block();
     else if( const auto lag =
              dynamic_cast< const LagBFunction * >( function ) )
      block = lag->get_inner_block();
     else // not found
      return( nullptr );
    }
    else // not found
     return( nullptr );
   }
   else if( node.type == Node::eObjective ) {
    auto objective = block->get_objective();

    if( const auto fro = dynamic_cast< const FRealObjective * >( objective ) ) {
     auto function = fro->get_function();
     if( dynamic_cast< BendersBFunction * >( function ) ||
         dynamic_cast< LagBFunction * >( function ) )
      block = dynamic_cast< Block * >( function );
     else // not found
      return( nullptr );
    }
    else // not found
     return( nullptr );
   }
   else // not found
    return( nullptr );
  } // end for

  // Now, we analyze the last node in the path.

  const auto node = get_last_node( block );

  if constexpr( std::is_base_of_v< Constraint , T > ||
                std::is_base_of_v< Variable , T > ) {
   if constexpr( std::is_base_of_v< Constraint , T > )
    assert( Node::is_constraint( node.type ) );
   else
    assert( Node::is_variable( node.type ) );

   // An explicit subset of element indices, when present, takes precedence
   // over the [ element_index , range_index ) contiguous element range.
   const auto & subset = get_node_subset( length() - 1 );
   Index element_index;
   if( ! subset.empty() ) {
    if( offset >= subset.size() )
     return( nullptr );
    element_index = subset[ offset ];
   }
   else
    element_index = node.element_index + offset;

   return( inspection::get_element< T >( block ,
                                         node.is_static() ,
                                         node.group_index ,
                                         element_index ) );
  }
  else if constexpr( std::is_base_of_v< Objective , T > ) {
   assert( node.type == Node::eObjective );
   return( block->get_objective() );
  }
  else if constexpr( std::is_base_of_v< Function , T > ) {
   if( Node::is_constraint( node.type ) ) {
    // It must be an FRowConstraint
    auto constraint = inspection::get_element< FRowConstraint >
     ( block , node.is_static() , node.group_index , node.element_index );
    if( constraint )
     return( dynamic_cast< T * >( constraint->get_function() ) );
    return( nullptr );
   }
   if( node.type == Node::eObjective ) {
    // It must be an FRealObjective
    auto objective = dynamic_cast< FRealObjective * >( block->get_objective() );
    if( objective )
     return( dynamic_cast< T * >( objective->get_function() ) );
    return( nullptr );
   }
   if( node.type == Node::eBlock ) {
    // It must be a PolyhedralFunctionBlock
    PolyhedralFunctionBlock * pfb = nullptr;
    if( node.group_index == Inf< Index >() )
     pfb = dynamic_cast< PolyhedralFunctionBlock * >( block );
    else
     pfb = dynamic_cast< PolyhedralFunctionBlock * >
      ( block->get_nested_Blocks()[ node.group_index ] );
    if( pfb )
     return( dynamic_cast< T * >( & ( pfb->get_PolyhedralFunction() ) ) );
    return( nullptr );
   }
   return( nullptr );
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   assert( node.type == Node::eBlock );

   // A 'B' node may select a single nested Block (via group_index), a
   // contiguous range [ group_index , range_index ) of nested Blocks, or an
   // explicit subset of nested-Block indices. In the latter two cases, the
   // offset selects which Block of the range/subset is returned.
   const auto & subset = get_node_subset( length() - 1 );

   if( subset.empty() && ( node.group_index == Inf< Index >() ) )
    // the path targets the reference Block itself
    return( offset == 0 ? block : nullptr );

   const auto & nested_blocks = block->get_nested_Blocks();

   Index block_index;
   if( ! subset.empty() ) {
    if( offset >= subset.size() )
     return( nullptr );
    block_index = subset[ offset ];
   }
   else
    block_index = node.group_index + offset;

   if( block_index >= nested_blocks.size() )
    return( nullptr );
   return( nested_blocks[ block_index ] );
  }
  else
   return( nullptr );
 }

/*--------------------------------------------------------------------------*/

 /// returns the resolved indices selected by the last node of this path
 /** Returns the (possibly empty) list of indices that the last node of this
  * AbstractPath selects on its container, with all three notations - single
  * element, contiguous range, explicit subset - resolved to the same explicit
  * list. The returned indices are:
  *
  * - nested-Block indices (relative to the Block that owns them) if the last
  *   node is a 'B' node;
  *
  * - element indices within the group (relative to the Block that owns it)
  *   if the last node is a Variable or Constraint node.
  *
  * For any other node type (e.g. an Objective node), or when this path is
  * empty, or when its last 'B' node targets the reference Block itself
  * (group_index == +Inf), the empty vector is returned. The template
  * parameter \p T is used only to size open-ended Variable/Constraint ranges
  * (range_index == +Inf), in the same way as in get_number_elements< T >().
  *
  * This helper makes it easy to consume the multi-element selection from
  * outside the class (introspection tools, equivalent-form detection, etc.)
  * without re-implementing the subset / range / single dispatch.
  *
  * @param reference The reference Block against which this path is resolved.
  *
  * @return The list of indices selected by the last node, or an empty vector
  *         if no indices apply. */

 template< class T >
 std::vector< Index > get_resolved_indices( Block * reference ) const {

  std::vector< Index > result;

  if( length() == 0 )
   return( result );

  // Walk the intermediate nodes exactly as get_element()/get_number_elements
  // do, to land `block` on the owner of the last node's container.
  auto block = reference;
  for( Index i = 0 ; i < length() - 1 ; ++i ) {
   const auto node = get_node( block , i );
   if( node.type == Node::eBlock ) {
    assert( node.group_index < block->get_nested_Blocks().size() );
    block = block->get_nested_Blocks()[ node.group_index ];
    }
   else if( Node::is_constraint( node.type ) ) {
    auto constraint = inspection::get_element< Constraint >
     ( block , node.is_static() , node.group_index , node.element_index );
    if( const auto frowc =
        dynamic_cast< const FRowConstraint * >( constraint ) ) {
     auto function = frowc->get_function();
     if( const auto benders =
         dynamic_cast< const BendersBFunction * >( function ) )
      block = benders->get_inner_block();
     else if( const auto lag =
              dynamic_cast< const LagBFunction * >( function ) )
      block = lag->get_inner_block();
     else
      return( result );
     }
    else
     return( result );
    }
   else if( node.type == Node::eObjective ) {
    auto objective = block->get_objective();
    if( const auto fro = dynamic_cast< const FRealObjective * >( objective ) ) {
     auto function = fro->get_function();
     if( dynamic_cast< BendersBFunction * >( function ) ||
         dynamic_cast< LagBFunction * >( function ) )
      block = dynamic_cast< Block * >( function );
     else
      return( result );
     }
    else
     return( result );
    }
   else
    return( result );
   }

  // Resolve the last node.
  const auto node = get_last_node( block );
  const auto & subset = get_node_subset( length() - 1 );

  if( ! subset.empty() )
   return( subset );

  if( Node::is_block( node.type ) ) {
   if( node.range_index == Inf< Index >() ) {
    if( node.group_index == Inf< Index >() )
     return( result );             // path targets the reference Block itself
    result.push_back( node.group_index );
    return( result );
    }
   if( node.range_index < node.group_index )
    throw( std::logic_error( "AbstractPath::get_resolved_indices: invalid Block "
                             "range, end < start." ) );
   result.reserve( node.range_index - node.group_index );
   for( Index k = node.group_index ; k < node.range_index ; ++k )
    result.push_back( k );
   return( result );
   }

  if( ! Node::has_range( node.type ) )
   return( result );                // Objective: no indices

  // Variable or Constraint.
  Index start = node.element_index;
  Index end   = node.range_index;
  if( end == Inf< Index >() )
   end = inspection::get_element_size< T >( block , node.is_static() ,
                                            node.group_index );
  if( end < start )
   throw( std::logic_error( "AbstractPath::get_resolved_indices: invalid "
                            "element range, end < start." ) );
  result.reserve( end - start );
  for( Index k = start ; k < end ; ++k )
   result.push_back( k );
  return( result );
  }

/*--------------------------------------------------------------------------*/
 /// serializes the given AbstractPath into the given netCDF::NcGroup
 /** This function serializes the given AbstractPath into the given
  * netCDF::NcGroup. Please refer to the comments to deserialize() for the
  * specification of the format of an AbstractPath.
  *
  * @param path The AbstractPath to be serialized.
  *
  * @param group The group in which the path will be serialized. */

 void serialize( netCDF::NcGroup & group ) const {
  auto dim = group.addDim( path_total_length_dim_name , length() );
  using ::SMSpp_di_unipi_it::serialize;
  serialize( group , node_type_name , netCDF::NcChar() ,
             dim , node_types );
  serialize( group , group_index_name , netCDF::NcUint() ,
             dim , group_indices );
  serialize( group , element_index_name , netCDF::NcUint() ,
             dim , element_indices );
  serialize( group , element_range_name , netCDF::NcUint() ,
             dim , range_indices );

  // The explicit per-node subsets of element/Block indices are optional: they
  // are serialized only when at least one node actually selects a subset, so
  // that paths without subsets keep exactly the same netCDF representation as
  // before.
  std::vector< Index > subset_sizes( length() , 0 );
  std::vector< Index > subset_elements;
  for( Index i = 0 ; i < length() ; ++i )
   if( ( i < node_subsets.size() ) && ( ! node_subsets[ i ].empty() ) ) {
    subset_sizes[ i ] = node_subsets[ i ].size();
    subset_elements.insert( subset_elements.end() ,
                            node_subsets[ i ].begin() ,
                            node_subsets[ i ].end() );
    }

  if( ! subset_elements.empty() ) {
   serialize( group , subset_size_name , netCDF::NcUint() , dim , subset_sizes );
   auto subset_dim = group.addDim( subset_total_length_dim_name ,
                                   subset_elements.size() );
   serialize( group , subset_name , netCDF::NcUint() , subset_dim ,
              subset_elements );
   }
  }

/*--------------------------------------------------------------------------*/
 /// serialize a vector of AbstractPath into the given NcGroup
 /** This function serializes a vector of AbstractPath in the given \p
  * group. Please refer to the comments to vector_deserialize() for a
  * description of the format used to represent a vector of AbstractPath in
  * netCDF.
  *
  * @param paths The vector of AbstractPath to be serialized.
  *
  * @param group The netCDF::NcGroup in which the vector of AbstractPath will
  *        be stored. */

 static void serialize( const std::vector< AbstractPath > & paths ,
			netCDF::NcGroup & group ) {
  APnetCDF netCDFvars;
  pre_serialize( paths , netCDFvars , group );
  for( Index i = 0 ; i < paths.size() ; ++i ) {
   paths[ i ].serialize( i , netCDFvars );
   }
  }

/*--------------------------------------------------------------------------*/
 /// serialize a vector of AbstractPath into the given NcGroup
 /** This function serializes a vector of (unique_ptr to) AbstractPath in the
  * given \p group. Please refer to the comments to vector_deserialize() for a
  * description of the format used to represent a vector of AbstractPath in
  * netCDF.
  *
  * @param paths The vector of (unique_ptr to the) AbstractPath to be
  *              serialized.
  *
  * @param group The netCDF::NcGroup in which the vector of AbstractPath will
  *        be stored. */

 static void serialize(
	      const std::vector< std::unique_ptr< AbstractPath > > & paths ,
	      netCDF::NcGroup & group ) {
  APnetCDF netCDFvars;
  pre_serialize( paths , netCDFvars , group );
  for( Index i = 0 ; i < paths.size() ; ++i ) {
   paths[ i ]->serialize( i , netCDFvars );
   }
  }

/*--------------------------------------------------------------------------*/
 /// deserializes an AbstractPath from a netCDF::NcGroup and returns it
 /** This function constructs and returns an AbstractPath by deserializing it
  * from the given \p group. This \p group has a dimension called
  * "TotalLength" that contains the number N of nodes in the path.
  *
  *          ALL INDICES MENTIONED HERE BELONG TO ZERO-BASED NUMBERED
  *          SEQUENCES, I.E., SEQUENCES WHOSE FIRST ELEMENT IS 0.
  *
  * This \p group also has three one-dimensional arrays, whose dimensions are
  * N, that store the types of the nodes, the group indices, and the element
  * indices. The names of the netCDF variables storing these arrays are given
  * by the "PathNodeTypes", "PathGroupIndices", and "PathElementIndices"
  * parameters, respectively. Here in the comments, we call these variables
  * node_type, group_index, and element_index, respectively, and refer to the
  * value stored at position i of these arrays by appending [i] to their
  * denominations, i.e., node_type[i] is the type of node i. The i-th element
  * in each of these arrays is associated with the i-th node in the path, for
  * i in {0, ..., N-1}. The type of the variable node_type is
  * netCDF::NcChar(), while the type of the variables group_index and
  * element_index is netCDF::NcUint(). The type of a node may be indicated
  * by any of the following letters:
  *
  * - 'O', if the node is associated with an Objective;
  *
  * - 'B', if the node is associated with a Block;
  *
  * - 'C', if the node is associated with a static Constraint;
  *
  * - 'c', if the node is associated with a dynamic Constraint;
  *
  * - 'V', if the node is associated with a static Variable;
  *
  * - 'v', if the node is associated with a dynamic Variable.
  *
  * Notice that, for Constraint and Variable, an upper case letter indicates
  * that the element is static, while a lower case letter indicates that the
  * element is dynamic.
  *
  * For a node whose type is 'O', i.e., representing a Node associated with an
  * Objective, the values stored in the variables group_index and
  * element_index are meaningless. If it is the last node in the path, then
  * the path refers to an Objective. Otherwise, the next node in the path must
  * be of type 'B'.
  *
  * For node whose type is 'B', i.e., representing a Node associated with a
  * Block B, the values stored in the variable element_index are
  * meaningless. If this is the i-th node in the path with i < N - 1, i.e., it
  * is not the last node of the path, the value stored in group_index[i] is
  * the index of the sub-Block of Block B which is the next node in the
  * path. If i = N-1, i.e., it is the last node in the path, then the
  * destination of the path is a Block which must be
  *
  * - the Block B itself if group_index[i] = +Inf;
  *
  * - the j-th sub-Block of Block B, where j = group_index[i].
  *
  * If the i-th node has type 'C' or 'c', representing a Constraint, or 'V' or
  * 'v', representing a Variable, the variable group_index[i] stores the
  * index of the (static or dynamic) group to which the element belongs. The
  * variable element_index[i] stores the index of the element in that group.
  *
  * A static group can be one of three types:
  *
  * 1. It is a single Constraint/Variable;
  *
  * 2. It is a vector of Constraint/Variable;
  *
  * 3. It is a multidimensional array of Constraint/Variable.
  *
  * In the first case, in which the group is a single Constraint/Variable, the
  * value of element_index[i] is 0. In the second case, in which the group is
  * a vector of Constraint/Variable, element_index[i] is the index of the
  * element in that vector. In the last case, in which the group is a
  * multidimensional array of Constraint/Variable, element_index[i] is the
  * index of the element in the vectorized multidimensional array in row-major
  * layout. For instance, if the multidimensional array has two dimensions
  * with sizes m and n, respectively, then the element at position (p, q)
  * would have an element index equal to "n * p + q" (recall the indices start
  * from 0). In general, for a multidimensional array with k dimensions with
  * sizes (n_0, ..., n_{k-1}), the element at position (i_0, ..., i_{k-1})
  * would have an element index equal to
  * \f[
  *    \sum_{r = 0}^{k-1} ( \prod_{s = r + 1}^{k-1} n_s ) i_r.
  * \f]
  *
  * A dynamic group can be one of three types:
  *
  * 1. It is list of Constraint/Variable;
  *
  * 2. It is a vector of lists of Constraint/Variable;
  *
  * 3. It is a multidimensional array of lists of Constraint/Variable.
  *
  * In the first case, in which the group is a list of Constraint/Variable,
  * element_index[i] is the index of the element in that list. In the second
  * case, in which the group is a vector of lists of Constraint/Variable, for
  * an element at position j of the k-th list of the vector, element_index[i]
  * is given by
  * \f[
  *    j + \sum_{t = 0}^{k-1} s_t
  * \f]
  *
  * where s_t is the number of elements in the t-th list of the vector. The
  * last case is analogous.
  *
  * Multi-element selection on the last node
  * ----------------------------------------
  *
  * The last node of a path can optionally select more than one element of the
  * same kind. This is supported in two forms, both backward-compatible (every
  * existing serialized path keeps its previous meaning):
  *
  * - a contiguous range. On a 'V'/'v' or 'C'/'c' node the range is given by
  *   the existing [ element_index[i] , range_index[i] ) interval over the
  *   element indices within the group; on a 'B' node it is the interval
  *   [ group_index[i] , range_index[i] ) over the nested-Block indices of the
  *   Block that owns them. For 'B' nodes, range_index[i] == +Inf still means
  *   "a single Block" (the legacy semantics).
  *
  * - an explicit, possibly non-contiguous, subset of element/Block indices.
  *   The subset is stored in two optional netCDF variables:
  *
  *   * "PathSubsetSizes", of type netCDF::NcUint and indexed over the same
  *     dimension as PathNodeTypes: PathSubsetSizes[i] is the number K_i of
  *     elements/Blocks selected by node i, or 0 if node i does not select an
  *     explicit subset;
  *
  *   * "PathSubset", of type netCDF::NcUint and indexed over its own
  *     dimension "PathSubsetTotalLength" (equal to the sum of PathSubsetSizes):
  *     it contains the concatenation, in node order, of the explicit subsets
  *     of all nodes (only the nodes with PathSubsetSizes[i] > 0 contribute,
  *     in their original order).
  *
  *   When PathSubsetSizes[i] > 0, the subset takes precedence over the
  *   contiguous range that may also be present on node i. When both
  *   PathSubsetSizes and PathSubset are absent (or all PathSubsetSizes are 0)
  *   no node selects an explicit subset and the path behaves as before.
  *
  * @param group The netCDF::NcGroup containing the path.
  *
  * @return The AbstractPath corresponding to the given group. */

 void deserialize( const netCDF::NcGroup & group ) {
  deserialize( 0 , pre_deserialize( group ) );
  }

/*--------------------------------------------------------------------------*/
 /// deserializes a vector of AbstractPath
 /** This function deserializes a vector of AbstractPath and returns it. The
  * format is similar to that described in the comments to deserialize() with
  * a few differences. Firstly, there is a dimension called "PathDim" which
  * contains the number of paths that are described in that group. Secondly,
  * there is a one-dimensional array called "PathStart", indexed over
  * "PathDim", which contains the index where the description of each
  * AbstractPath starts. Then there are the one-dimensional arrays
  * "PathNodeTypes", "PathGroupIndices", and "PathElementIndices", which store
  * the types of nodes in the paths, the group indices, and the element
  * indices, respectively. The description of the k-th AbstractPath, for k <
  * PathDim - 1 is given in those arrays between the indices PathStart[k] and
  * PathStart[k+1], i.e., in
  *
  * ( PathNodeTypes[ PathStart[ k ] ] , ... ,
  *                   PathNodeTypes[ PathStart[ k+1 ] - 1 ] )
  *
  * ( PathGroupIndices[ PathStart[ k ] ] , ... ,
  *                   PathGroupIndices[ PathStart[ k+1 ] - 1 ] )
  *
  * and
  *
  * ( PathElementIndices[ PathStart[ k ] ] , ... ,
  *                   PathElementIndices[ PathStart[ k+1 ] - 1 ] )
  *
  * The description of the last path is given between the indices
  * PathStart[PathDim - 1] and TotalLength - 1, where "TotalLength" is a
  * dimension containing the size of the arrays "PathNodeTypes",
  * "PathGroupIndices", and "PathElementIndices" (and, therefore, these arrays
  * are indexed over "TotalLength").
  *
  * The paths are represented in the same format as specified in the comments
  * to deserialize(), except that here they are concatenated in the arrays
  * "PathNodeTypes", "PathGroupIndices", and "PathElementIndices".
  *
  * @param group The NcGroup that contains the description of the AbstractPaths
  *              to be deserialized.
  *
  * @return A vector with the AbstractPaths. */

 static std::vector< AbstractPath > vector_deserialize(
					    const netCDF::NcGroup & group ) {
  std::vector< AbstractPath > paths;
  if( group.isNull() )
   return( paths );
  const auto netCDFvars = pre_deserialize( group );
  paths.reserve( netCDFvars.NumPaths );
  for( Index i = 0 ; i < netCDFvars.NumPaths ; ++i )
   paths.emplace_back( i , netCDFvars );
  return( paths );
  }
/*--------------------------------------------------------------------------*/
 /// deserializes a vector of AbstractPath
 /** This function deserializes a vector of (unique_ptr) AbstractPath and
  * returns it. The format of the netCDF \p group is specified in
  * vector_deserialize( const netCDF::NcGroup & ).
  *
  * @param group The NcGroup that contains the description of the vector of
  *              AbstractPath to be deserialized.
  *
  * @return A vector with the AbstractPaths. */

 static void vector_deserialize( const netCDF::NcGroup & group ,
		  std::vector< std::unique_ptr< AbstractPath > > & paths ) {
  paths.clear();
  if( group.isNull() )
   return;
  const auto netCDFvars = pre_deserialize( group );
  paths.reserve( netCDFvars.NumPaths );
  for( Index i = 0 ; i < netCDFvars.NumPaths ; ++i )
   paths.emplace_back( std::make_unique< AbstractPath >( i , netCDFvars ) );
  }

/*--------------------------------------------------------------------------*/
 /// pre-deserializes a vector of AbstractPath
 /** This function pre-deserializes a vector of AbstractPath and returns an
  * APnetCDF. The format of the netCDF \p group is specified in
  * vector_deserialize( const netCDF::NcGroup & ).
  *
  * @param group The NcGroup that contains the description of the vector of
  *              AbstractPath to be pre-deserialized.
  *
  * @return An APnetCDF for the vector of AbstractPath described in \p group.
  */

 static APnetCDF pre_deserialize( const netCDF::NcGroup & group ) {
  APnetCDF netCDFvars;
  netCDFvars.NumPaths = 1;
  netCDFvars.PathDim = group.getDim( path_dim_name );
  netCDFvars.PathStart = group.getVar( path_start_name );
  netCDFvars.PathNodeTypes = group.getVar( node_type_name );
  netCDFvars.PathGroupIndices = group.getVar( group_index_name );
  netCDFvars.PathElementIndices = group.getVar( element_index_name );
  netCDFvars.PathRangeIndices = group.getVar( element_range_name );
  netCDFvars.PathSubsetSizes = group.getVar( subset_size_name );
  netCDFvars.PathSubset = group.getVar( subset_name );

  /* The dimension PathDim is optional. If it is not present, then there is
   * only one path and PathStart is ignored. If PathDim is present, then it
   * indicates the number of paths and PathStart must be present and indexed
   * over PathDim. PathStart[i] is the index where the i-th path starts, for i
   * in {0, ..., PathDim - 1}. */

  if( ! netCDFvars.PathDim.isNull() ) {
   netCDFvars.NumPaths = netCDFvars.PathDim.getSize();

   if( netCDFvars.PathStart.isNull() )
    throw( std::invalid_argument( "AbstractPath::pre_deserialize: variable " +
                                  path_start_name + " is not present in the "
                                  "given group. " ) );

   if( netCDFvars.PathStart.getDimCount() != 1 ||
       netCDFvars.PathStart.getDim( 0 ) != netCDFvars.PathDim )
    throw( std::invalid_argument( "AbstractPath::pre_deserialize: variable " +
                                  path_start_name + " must be index over "
                                  "dimension " + path_dim_name + "." ) );
  }

  if( netCDFvars.PathNodeTypes.isNull() ||
      netCDFvars.PathGroupIndices.isNull() )
   throw( std::invalid_argument( "AbstractPath::pre_deserialize: variables " +
                                 node_type_name + " and " + group_index_name +
                                 " must all be present in the "
                                 "given NcGroup." ) );

  if( netCDFvars.PathNodeTypes.getDimCount() != 1 ||
      netCDFvars.PathGroupIndices.getDimCount() != 1 ||
      ( ! netCDFvars.PathElementIndices.isNull() &&
        netCDFvars.PathElementIndices.getDimCount() != 1 ) )
   throw( std::invalid_argument( "AbstractPath::pre_deserialize: variables " +
                                 node_type_name + ", " + group_index_name + ", "
                                 "and " + element_index_name + " must have "
                                 "a single dimension." ) );

  if( ( netCDFvars.PathNodeTypes.getDim( 0 ) !=
        netCDFvars.PathGroupIndices.getDim( 0 ) ) ||
      ( ! netCDFvars.PathElementIndices.isNull() &&
        netCDFvars.PathNodeTypes.getDim( 0 ) !=
        netCDFvars.PathElementIndices.getDim( 0 ) ) )
   throw( std::invalid_argument( "AbstractPath::pre_deserialize: variables " +
                                 node_type_name + ", " + group_index_name + ", "
                                 "and " + element_index_name + " must be "
                                 "indexed over the same dimension." ) );

  return( netCDFvars );
 }

/*--------------------------------------------------------------------------*/
 /// deserialize the AbstractPath with given index
 /** This function deserializes the AbstractPath whose index is \p path_index
  * from the given netCDF dimensions and variables in \p netCDFvars. The
  * variables in \p netCDFvars may contain the description of a single
  * AbstractPath or a vector of AbstractPath. If the dimension "PathDim" is
  * not present, then \p netCDFvars contains the description of a single
  * AbstractPath. In this case, \p path_index must be 0; otherwise, an
  * exception is thrown. If the dimension "PathDim" is present, then it
  * indicates the number N of stored AbstractPath. The value for \p path_index
  * must be between 0 and N - 1; otherwise, an exception is thrown. Please
  * refer to the comments of the other deserialize() function for a
  * description of the format of an AbstractPath.
  *
  * @param path_index Index of the AbstractPath to be deserialized.
  *
  * @param netCDFvars The struct containing the netCDF dimensions and
  *        variables describing the vector of AbstractPath.
  *
  * @return The AbstractPath with index \p path_index. */

 void deserialize( Index path_index , const APnetCDF & netCDFvars ) {

  Index path_start = 0;
  Index path_end = netCDFvars.PathNodeTypes.getDim( 0 ).getSize();

  if( ! netCDFvars.PathDim.isNull() ) { // a vector of AbstractPath
   if( path_index >= netCDFvars.PathDim.getSize() )
    throw( std::invalid_argument( "AbstractPath::deserialize: AbstractPath "
                                  "number " + std::to_string( path_index ) +
                                  " is not present in the given NcGroup" ) );

   netCDFvars.PathStart.getVar( { path_index } , & path_start );

   if( path_index < netCDFvars.PathDim.getSize() - 1 )
    netCDFvars.PathStart.getVar( { path_index + 1 } , & path_end );

   if( path_start > path_end )
    throw( std::invalid_argument( "AbstractPath::deserialize: invalid "
                                  "PathStart index" ) );
   }
  else
   if( path_index != 0 ) {
    // There is only one path. So the path index must be 0.
    throw( std::invalid_argument( "AbstractPath::deserialize: invalid path "
				  "index " + std::to_string( path_index ) +
				  ", the NcGroup has a single AbstractPath" )
	   );
    }

  const auto num_nodes = path_end - path_start;

  node_types.resize( num_nodes );
  element_indices.resize( num_nodes );
  range_indices.resize( num_nodes );

  netCDFvars.PathNodeTypes.getVar( { path_start } , { num_nodes } ,
                                   node_types.data() );

  if( netCDFvars.PathGroupIndices.getType().getTypeClass() ==
      netCDF::NcType::nc_STRING ) {
   group_index_names.resize( num_nodes );
   std::vector< char * > tmp( num_nodes );
   netCDFvars.PathGroupIndices.getVar( { path_start } , { num_nodes } ,
                                       tmp.data() );
   std::transform( tmp.begin() , tmp.end() , group_index_names.begin() ,
                   []( char * s ) { return( std::string( s ) ); } );
   for( char * s : tmp ) free( s );
  }
  else if( netCDFvars.PathGroupIndices.getType().getTypeClass() ==
           netCDF::NcType::nc_UINT ) {
   group_indices.resize( num_nodes );
   netCDFvars.PathGroupIndices.getVar( { path_start } , { num_nodes } ,
                                       group_indices.data() );
  }
  else
   throw( std::logic_error( "AbstractPath::deserialize: unsupported type "
			    "for PathGroupIndices" ) );

  if( ! netCDFvars.PathElementIndices.isNull() )
   netCDFvars.PathElementIndices.getVar( { path_start } , { num_nodes } ,
                                         element_indices.data() );
  else {
   element_indices.resize( num_nodes );
   element_indices.assign( element_indices.size() , Inf< Index >() );
   }

  if( ! netCDFvars.PathRangeIndices.isNull() ) {
   netCDFvars.PathRangeIndices.getVar( { path_start } , { num_nodes } ,
                                       range_indices.data() );

   for( Index i = 0 ; i < num_nodes ; ++i ) {
    const auto type = node_types[ i ];
    // A 'B' node may carry a contiguous Block range [ group_index ,
    // range_index ); the consistency of its bounds is checked at use time
    // (see get_number_elements() / get_element()).
    if( Node::is_block( type ) )
     continue;
    if( ! Node::has_range( type ) ) {
     if( range_indices[ i ] != Inf< Index >() )
      throw( std::logic_error(
       "AbstractPath::deserialize: range provided for node ["
       + std::to_string( i ) + "]" + " of type " +
       std::string( 1 , type ) + " that does not support range" ) );
    }
    else if( range_indices[ i ] < element_indices[ i ] )
     if( ( range_indices[ i ] != 0 ) && ( range_indices[ i ] != 1 ) )
      throw( std::logic_error(
       "AbstractPath::deserialize: range_indices[" + std::to_string( i )
       + "] < element_indices[" + std::to_string( i ) + "] not allowed, " +
       "unless range_indices[" + std::to_string( i ) + "] is 0 or 1" ) );
    }
  }
  else
   for( Index i = 0 ; i < num_nodes ; ++i )
    if( Node::has_range( node_types[ i ] ) )
     range_indices[ i ] = 1;
    else
     range_indices[ i ] = Inf< Index >();

  // Optional explicit per-node subsets of element/Block indices. They are
  // present only in netCDF descriptions that use them; when absent, every node
  // selects a single element or a contiguous range, as before.
  node_subsets.assign( num_nodes , {} );
  if( ( ! netCDFvars.PathSubsetSizes.isNull() ) &&
      ( ! netCDFvars.PathSubset.isNull() ) ) {
   const auto total_nodes = netCDFvars.PathSubsetSizes.getDim( 0 ).getSize();
   std::vector< Index > all_sizes( total_nodes );
   netCDFvars.PathSubsetSizes.getVar( { 0 } , { total_nodes } ,
                                      all_sizes.data() );

   // offset into PathSubset where this path's subset elements begin
   Index subset_offset = 0;
   for( Index k = 0 ; k < path_start ; ++k )
    subset_offset += all_sizes[ k ];

   for( Index i = 0 ; i < num_nodes ; ++i ) {
    const Index sz = all_sizes[ path_start + i ];
    if( sz > 0 ) {
     node_subsets[ i ].resize( sz );
     netCDFvars.PathSubset.getVar( { subset_offset } , { sz } ,
                                   node_subsets[ i ].data() );
     }
    subset_offset += sz;
    }
   }
  }

/*--------------------------------------------------------------------------*/

#ifndef NDEBUG
 /// mimics Block::expected_dims()

 virtual std::vector< std::string > expected_dims( void ) const {
  static const std::vector< std::string > ed =
  { path_dim_name , path_total_length_dim_name , subset_total_length_dim_name };

  return( ed );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// mimics Block::expected_vars()

 virtual std::vector< std::string > expected_vars( void ) const {
  static const std::vector< std::string > ed =
  { path_start_name , node_type_name , group_index_name ,
    element_index_name , element_range_name , subset_size_name , subset_name };
  return( ed );
  }

#endif

/*--------------------------------------------------------------------------*/

 template< class T >
 static void pre_serialize( const std::vector< T > & paths ,
                            APnetCDF & netCDFvars , netCDF::NcGroup & group ) {

  netCDFvars.NumPaths = paths.size();
  netCDFvars.PathDim = group.addDim( path_dim_name , netCDFvars.NumPaths );
  netCDFvars.PathStart = group.addVar( path_start_name , netCDF::NcUint() ,
                                       netCDFvars.PathDim );

  Index total_length = 0;
  for( Index i = 0 ; i < paths.size() ; ++i ) {
   netCDFvars.PathStart.putVar( { i } , total_length );
   total_length += length( paths[ i ] );
   }

  netCDFvars.PathTotalLength = group.addDim( path_total_length_dim_name ,
                                             total_length );

  netCDFvars.PathNodeTypes = group.addVar( node_type_name , netCDF::NcChar() ,
                                           netCDFvars.PathTotalLength );

  netCDFvars.PathGroupIndices = group.addVar( group_index_name ,
                                              netCDF::NcUint() ,
                                              netCDFvars.PathTotalLength );

  netCDFvars.PathElementIndices = group.addVar( element_index_name ,
                                                netCDF::NcUint() ,
                                                netCDFvars.PathTotalLength );

  netCDFvars.PathRangeIndices = group.addVar( element_range_name ,
                                              netCDF::NcUint() ,
                                              netCDFvars.PathTotalLength );
  }

/*--------------------------------------------------------------------------*/

 static void pre_serialize( Index number_paths , APnetCDF & netCDFvars ,
                            netCDF::NcGroup & group ) {

  netCDFvars.NumPaths = number_paths;
  netCDFvars.PathDim = group.addDim( path_dim_name , netCDFvars.NumPaths );
  netCDFvars.PathStart = group.addVar( path_start_name , netCDF::NcUint() ,
                                       netCDFvars.PathDim );

  netCDFvars.PathTotalLength = group.addDim( path_total_length_dim_name );

  netCDFvars.PathNodeTypes = group.addVar( node_type_name , netCDF::NcChar() ,
                                           netCDFvars.PathTotalLength );

  netCDFvars.PathGroupIndices = group.addVar( group_index_name ,
                                              netCDF::NcUint() ,
                                              netCDFvars.PathTotalLength );

  netCDFvars.PathElementIndices = group.addVar( element_index_name ,
                                                netCDF::NcUint() ,
                                                netCDFvars.PathTotalLength );

  netCDFvars.PathRangeIndices = group.addVar( element_range_name ,
                                              netCDF::NcUint() ,
                                              netCDFvars.PathTotalLength );
 }

/*--------------------------------------------------------------------------*/

 void serialize( const Index path_index , APnetCDF & netCDFvars ) const {

  // Explicit per-node subsets are not (yet) representable when serializing a
  // whole vector of AbstractPath in a single shared set of netCDF variables;
  // fail loudly rather than silently dropping them. In practice, the paths
  // serialized this way are always built (via build()) and thus select a
  // single element, so this never triggers.
  for( const auto & subset : node_subsets )
   if( ! subset.empty() )
    throw( std::logic_error(
     "AbstractPath::serialize: serializing a vector of AbstractPath that "
     "contains explicit element/Block subsets is not supported; serialize such "
     "paths individually instead." ) );

  const auto num_nodes = length();

  Index path_start;
  netCDFvars.PathStart.getVar( { path_index } , & path_start );

  netCDFvars.PathNodeTypes.putVar( { path_start } , { num_nodes } ,
                                   node_types.data() );
  netCDFvars.PathGroupIndices.putVar( { path_start } , { num_nodes } ,
                                      group_indices.data() );
  netCDFvars.PathElementIndices.putVar( { path_start } , { num_nodes } ,
                                        element_indices.data() );
  netCDFvars.PathRangeIndices.putVar( { path_start } , { num_nodes } ,
                                      range_indices.data() );
 }

/*--------------------------------------------------------------------------*/

 bool operator==( const AbstractPath & path ) const {
  if( length() != path.length() )
   return( false );

  // group_indices (numeric) and group_index_names (string) are mutually
  // exclusive per-node: a path that comes from a string-typed
  // PathGroupIndices netCDF variable populates only group_index_names and
  // leaves group_indices empty (and vice-versa). Compare both safely, using
  // an Inf / "" default for the missing slot.
  auto numeric = []( const std::vector< Index > & v , Index i ) {
   return( i < v.size() ? v[ i ] : Inf< Index >() );
   };
  auto named = []( const std::vector< std::string > & v , Index i ) {
   return( i < v.size() ? v[ i ] : std::string() );
   };

  for( Index i = 0 ; i < length() ; ++i )
   if( node_types[ i ] != path.node_types[ i ] ||
       numeric( group_indices , i ) != numeric( path.group_indices , i ) ||
       named( group_index_names , i ) != named( path.group_index_names , i ) ||
       element_indices[ i ] != path.element_indices[ i ] ||
       range_indices[ i ] != path.range_indices[ i ] ||
       get_node_subset( i ) != path.get_node_subset( i ) )
    return( false );

  return( true );
  }

/*--------------------------------------------------------------------------*/

 void print( void ) const {
  for( Index i = 0 ; i < length() ; ++i ) {
   std::cout << node_types[ i ] << "( ";
   // Prefer the string-typed group identifier when present (paths
   // deserialized from a string-typed PathGroupIndices have an empty
   // group_indices vector).
   if( ( i < group_index_names.size() ) && ( ! group_index_names[ i ].empty() ) )
    std::cout << group_index_names[ i ];
   else if( i < group_indices.size() )
    std::cout << group_indices[ i ];
   else
    std::cout << "?";
   std::cout << " , " << element_indices[ i ] << " , " << range_indices[ i ]
             << " )";
   const auto & subset = get_node_subset( i );
   if( ! subset.empty() ) {
    std::cout << "{ ";
    for( const auto e : subset )
     std::cout << e << " ";
    std::cout << "}";
    }
   if( i < length() - 1 )
    std::cout << " -> ";
   }
  std::cout << std::endl;
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

};  // end( class( AbstractPath ) )

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* AbstractPath.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File AbstractPath.h --------------------------*/
/*--------------------------------------------------------------------------*/

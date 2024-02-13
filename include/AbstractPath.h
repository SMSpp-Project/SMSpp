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

#include <cstddef>
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
 * - 'O', if the node is associated with an Objective.
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
 * If the index of a 'B' node is not +Inf, then this node is necessarily
 * preceded by another 'B' node, which is associated with the father of that
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
 * To help understand how a path is defined, we present some examples. TODO
 */

class AbstractPath {

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

 /// Name of the netCDF variable that stores the array of element indices
 inline static const std::string element_index_name = "PathElementIndices";

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
  * Variable.
  */
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
           element_index( Inf< Index >() ) {}

/*--------------------------------------------------------------------------*/

  Node( NodeType type , Index group_index , Index element_index )
   : type( type ) , group_index( group_index ) ,
     element_index( element_index ) {}

/*--------------------------------------------------------------------------*/

  bool is_static() const {
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

  NodeType type;
  Index group_index;
  Index element_index;

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
  }
  else
   throw( std::logic_error( "AbstractPath::add_node: Element not found." ) );
 }

/*--------------------------------------------------------------------------*/

 /// adds a node to this AbstractPath
 void add_node( Node::NodeType type , Index group_index = Inf< Index >() ,
                Index element_index = Inf< Index >() ) {
  node_types.push_back( type );
  group_indices.push_back( group_index );
  element_indices.push_back( element_index );
 }

/*--------------------------------------------------------------------------*/

 /// reverses this AbstractPath
 void reverse() {
  std::reverse( std::begin( node_types ), std::end( node_types ) );
  std::reverse( std::begin( group_indices ), std::end( group_indices ) );
  std::reverse( std::begin( element_indices ), std::end( element_indices ) );
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

 /// element_indices[i] is the element index of the i-th node in the path
 std::vector< Index > element_indices;

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
  * used to represent an AbstractPath or a vector of AbstractPath.
  */
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

  /// Variable storing the element indices
  netCDF::NcVar PathElementIndices;
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
  * of nodes in the path.
  */
 inline Index length() const {
  return( node_types.size() );
 }

/*--------------------------------------------------------------------------*/

 /// returns true if and only if this AbstractPath is empty
 /** This function returns true if and only if this AbstractPath is empty,
  * i.e., it does not represent a path to any object.
  */
 inline bool empty() const {
  return( length() == 0 );
 }

/*--------------------------------------------------------------------------*/

 /// clears this AbstractPath
 /** This function clears this AbstractPath making it an empty path.
  */
 void clear() {
  node_types.clear();
  group_indices.clear();
  element_indices.clear();
 }

/*--------------------------------------------------------------------------*/

 /// returns the Node representation of the i-th node in this AbstractPath
 /** This function returns the Node representation of the i-th node in this
  * AbstractPath.
  *
  * @param i The index of the node of this AbstractPath whose Node
  *        representation is required.
  *
  * @return The Node representation of the i-th node in this AbstractPath.
  */
 inline Node get_node( Index i ) const {
  assert( i < this->length() );
  return( Node( node_types[ i ] , group_indices[ i ] , element_indices[ i ] ) );
 }

/*--------------------------------------------------------------------------*/

 /// returns the Node representation of the last node in this AbstractPath
 /** This function returns the Node representation of the last node in this
  * AbstractPath.
  *
  * @return The Node representation of the last node in this AbstractPath.
  */
 inline Node get_last_node() const {
  const auto length = this->length();
  assert( length > 0 );
  return( Node( node_types[ length - 1 ] , group_indices[ length - 1 ] ,
                element_indices[ length - 1 ] ) );
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

  this->clear();

  if( ! t )
   return;

  if constexpr( std::is_base_of_v< Constraint , T > ) {
   this->add_node( t , Node::eConstraint );
  }
  else if constexpr( std::is_base_of_v< Variable , T > ) {
   this->add_node( t , Node::eVariable );
  }
  else if constexpr( std::is_base_of_v< Objective , T > ) {
   this->add_node( Node::eObjective );
  }
  else if constexpr( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( const auto constraint = dynamic_cast< FRowConstraint * >( observer ) ) {
    this->add_node( constraint , Node::eConstraint );
   }
   else if( dynamic_cast< FRealObjective * >( observer ) ) {
    this->add_node( Node::eObjective );
   }
   else if( dynamic_cast< PolyhedralFunctionBlock * >( observer ) ) {
    if( observer == reference_block )
     this->add_node( Node::eBlock );
   }
   else
    throw( std::logic_error( "AbstractPath::build: Unknown Observer of "
                             "given Function." ) );
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   if( t == reference_block ) {
    this->add_node( Node::eBlock , Inf< Index >() );
    return;
   }
   auto group_index = inspection::get_block_index( t );
   this->add_node( Node::eBlock , group_index );
  }

  Block * block;
  if constexpr ( std::is_base_of_v< Function , T > ) {
   auto observer = t->get_Observer();
   if( ! observer )
    throw( std::logic_error( "AbstractPath::build: Path not found. "
                             "Function has no Observer." ) );
   block = observer->get_Block();
  }
  else if constexpr ( std::is_base_of_v< Block , T > )
   block = t->get_f_Block();
  else
   block = t->get_Block();

  while( block != reference_block ) {

   auto index = inspection::get_block_index( block );

   if( index < Inf< Index >() ) {
    // block has a father Block
    this->add_node( Node::eBlock , index );
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
     this->add_node( frc , Node::eConstraint );
     block = frc->get_Block();
    }
    else if( const auto fro = dynamic_cast< FRealObjective * >( observer ) ) {
     this->add_node( Node::eObjective );
     block = fro->get_Block();
    }
    else
     throw( std::logic_error( "AbstractPath::build: Path not found." ) );
   }
  }

  this->reverse();
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
 T * get_element( Block * reference ) const {

  if( this->length() == 0 )
   return( nullptr );

  auto block = reference;

  for( Index i = 0 ; i < this->length() - 1 ; ++i ) {

   const auto node = this->get_node( i );

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

  // Now, we analyse the last node in the path.

  const auto node = this->get_last_node();

  if constexpr( std::is_base_of_v< Constraint , T > ) {
   assert( Node::is_constraint( node.type ) );
   return( inspection::get_element< T >( block ,
                                         node.is_static() ,
                                         node.group_index ,
                                         node.element_index ) );
  }
  else if constexpr( std::is_base_of_v< Variable , T > ) {
   assert( Node::is_variable( node.type ) );
   return( inspection::get_element< T >( block ,
                                         node.is_static() ,
                                         node.group_index ,
                                         node.element_index ) );
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
    else
     return( nullptr );
   }
   else if( node.type == Node::eObjective ) {
    // It must be an FRealObjective
    auto objective = dynamic_cast< FRealObjective * >( block->get_objective() );
    if( objective )
     return( dynamic_cast< T * >( objective->get_function() ) );
    else
     return( nullptr );
   }
   else if( node.type == Node::eBlock ) {
    // It must be a PolyhedralFunctionBlock
    PolyhedralFunctionBlock * pfb = nullptr;
    if( node.group_index == Inf< Index >() )
     pfb = dynamic_cast< PolyhedralFunctionBlock * >( block );
    else
     pfb = dynamic_cast< PolyhedralFunctionBlock * >
       ( block->get_nested_Blocks()[ node.group_index ] );
    if( pfb )
     return( dynamic_cast< T * >( & ( pfb->get_PolyhedralFunction() ) ) );
    else
     return( nullptr );
   }
   else
    return( nullptr );
  }
  else if constexpr( std::is_base_of_v< Block , T > ) {
   assert( node.type == Node::eBlock );

   if( node.group_index == Inf< Index >() )
    return( block );

   const auto & nested_blocks = block->get_nested_Blocks();
   if( node.group_index >= nested_blocks.size() )
    return( nullptr );
   return( nested_blocks[ node.group_index ] );
  }
  else
   return( nullptr );
 }

/*--------------------------------------------------------------------------*/

 /// serializes the given AbstractPath into the given netCDF::NcGroup
 /** This function serializes the given AbstractPath into the given
  * netCDF::NcGroup. Please refer to the comments to deserialize() for the
  * specification of the format of an AbstractPath.
  *
  * @param path The AbstractPath to be serialized.
  *
  * @param group The group in which the path will be serialized.
  */

 void serialize( netCDF::NcGroup & group ) const {

  auto dim = group.addDim( path_total_length_dim_name , this->length() );

  using ::SMSpp_di_unipi_it::serialize;
  serialize( group , node_type_name , netCDF::NcChar() ,
             dim , this->node_types );
  serialize( group , group_index_name , netCDF::NcUint() ,
             dim , this->group_indices );
  serialize( group , element_index_name , netCDF::NcUint() ,
             dim , this->element_indices );
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
  *        be stored.
  */
 static void serialize
 ( const std::vector< AbstractPath > & paths , netCDF::NcGroup & group ) {
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
  *        be stored.
  */
 static void serialize
 ( const std::vector< std::unique_ptr< AbstractPath > > & paths ,
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
  *
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
  *
  * \f[
  *    j + \sum_{t = 0}^{k-1} s_t
  * \f]
  *
  * where s_t is the number of elements in the t-th list of the vector. The
  * last case is analogous.
  *
  * @param group The netCDF::NcGroup containing the path.
  *
  * @return The AbstractPath corresponding to the given group.
  */

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
  * @return A vector with the AbstractPaths.
  */

 static std::vector< AbstractPath > vector_deserialize
 ( const netCDF::NcGroup & group ) {
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
  * @return A vector with the AbstractPaths.
  */
 static void vector_deserialize
 ( const netCDF::NcGroup & group ,
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

  /* The dimension PathDim is optional. If it is not present, then there is
   * only one path and PathStart is ignored. If PathDim is present, then it
   * indicates the number of paths and PathStart must be present and indexed
   * over PathDim. PathStart[i] is the index where the i-th path starts, for i
   * in {0, ..., PathDim - 1}.
   */

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
  * @return The AbstractPath with index \p path_index.
  */

 void deserialize( Index path_index , const APnetCDF & netCDFvars ) {

  Index path_start = 0;
  Index path_end = netCDFvars.PathNodeTypes.getDim( 0 ).getSize();

  if( ! netCDFvars.PathDim.isNull() ) { // a vector of AbstractPath
   if( path_index >= netCDFvars.PathDim.getSize() )
    throw( std::invalid_argument( "AbstractPath::deserialize: AbstractPath "
                                  "number " + std::to_string( path_index ) +
                                  " is not present in the given NcGroup." ) );

   netCDFvars.PathStart.getVar( { path_index } , & path_start );

   if( path_index < netCDFvars.PathDim.getSize() - 1 )
    netCDFvars.PathStart.getVar( { path_index + 1 } , & path_end );


   if( path_start > path_end )
    throw( std::invalid_argument( "AbstractPath::deserialize: invalid "
                                  "PathStart index." ) );
  }
  else if( path_index != 0 ) {
   // There is only one path. So the path index must be 0.
   throw( std::invalid_argument( "AbstractPath::deserialize: invalid path "
                                 "index " + std::to_string( path_index ) + "."
                                 "Given NcGroup has a single AbstractPath." ) );
  }

  const auto num_nodes = path_end - path_start;

  this->node_types.resize( num_nodes );
  this->group_indices.resize( num_nodes );
  this->element_indices.resize( num_nodes );

  netCDFvars.PathNodeTypes.getVar( { path_start } , { num_nodes } ,
                                   this->node_types.data() );
  netCDFvars.PathGroupIndices.getVar( { path_start } , { num_nodes } ,
                                      this->group_indices.data() );

  if( ! netCDFvars.PathElementIndices.isNull() ) {
   netCDFvars.PathElementIndices.getVar( { path_start } , { num_nodes } ,
                                         this->element_indices.data() );
  }
  else {
   this->element_indices.resize( num_nodes );
   this->element_indices.assign( this->element_indices.size() , Inf< Index >() );
  }
 }

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
 }

/*--------------------------------------------------------------------------*/

 void serialize( const Index path_index , APnetCDF & netCDFvars ) const {

  const auto num_nodes = this->length();

  Index path_start;
  netCDFvars.PathStart.getVar( { path_index } , & path_start );

  netCDFvars.PathNodeTypes.putVar( { path_start } , { num_nodes } ,
                                   this->node_types.data() );
  netCDFvars.PathGroupIndices.putVar( { path_start } , { num_nodes } ,
                                      this->group_indices.data() );
  netCDFvars.PathElementIndices.putVar( { path_start } , { num_nodes } ,
                                        this->element_indices.data() );
 }

/*--------------------------------------------------------------------------*/

 bool operator==( const AbstractPath & path ) const {
  if( this->length() != path.length() )
   return( false );

  for( Index i = 0 ; i < this->length() ; ++i ) {
   if( node_types[ i ] != path.node_types[ i ] ||
       group_indices[ i ] != path.group_indices[ i ] ||
       element_indices[ i ] != path.element_indices[ i ] )
    return( false );
  }

  return( true );
 }

/*--------------------------------------------------------------------------*/

 void print() const {
  for( Index i = 0 ; i < this->length() ; ++i ) {
   std::cout << node_types[ i ] << "( " << group_indices[ i ] << " , " <<
    element_indices[ i ] << " )";
   if( i < this->length() - 1 )
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

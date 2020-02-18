/*--------------------------------------------------------------------------*/
/*------------------------ File DataMapping.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the abstract class DataMapping that defines an interface
 * for all class that implements a mechanism for mapping some data into the
 * data of some object. A concrete class called SimpleDataMapping is also
 * defined for some common kinds of data mapping.
 *
 * \version 0.1
 *
 * \date 18 - 02 - 2020
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __DataMapping
#define __DataMapping
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractPath.h"
#include "Block.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup DataMapping_CLASSES Classes in DataMapping.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DataMapping ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// DataMapping defines an interface for all types of data mappings.
/** DataMapping defines an interface for all types of data mappings. The idea
 * of a data mapping is to allow, in particular, to map the values given by a
 * vector of double into the data of some object. It has four pure virtual
 * functions. The first two are set_data(), which have the following
 * signature:
 *
 *    virtual void set_data( const std::vector< double > & data ,
 *                           c_ModParam issueMod = eModBlck ,
 *                           c_ModParam issueAMod = eModBlck ) const;
 *
 *    virtual void set_data( const Eigen::ArrayXd & data ,
 *                           c_ModParam issueMod = eModBlck ,
 *                           c_ModParam issueAMod = eModBlck ) const;
 *
 * The idea of these functions is that the values of some data of an object can
 * be modified considering the given "data" parameter. The other two functions
 * are for serializing and deserializing a DataMapping. Typically, a
 * DataMapping could be used to set the data of a Block. In this case, a
 * pointer to that Block must be available. Pointers to a Block can be
 * serialized and deserialized considering its AbstractPath, which is relative
 * to some reference Block. For this reason, the serialize() and deserialize()
 * functions have a parameter which is a pointer to the reference Block:
 *
 *  virtual void serialize( netCDF::NcGroup & group ,
 *                          Block * block_reference = nullptr ) const;
 *
 *  virtual void deserialize( const netCDF::NcGroup & group ,
 *                            Block * block_reference = nullptr );
 */

class DataMapping {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING DataMapping -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing DataMapping
 *  @{ */

 /// constructor
 DataMapping() = default;

/*--------------------------------------------------------------------------*/

 /// destructor
 virtual ~DataMapping() {}

/*--------------------------------------------------------------------------*/

 /// deserializes this DataMapping
 /** This function deserializes this DataMapping out of the given NcGroup \p
  * group.
  *
  * @param group The NcGroup that contains the data describing this
  *        DataMapping.
  *
  * @param block_reference A pointer to the Block that may be used as
  *        reference when deserializing an AbstractPath.
  */

 virtual void deserialize( const netCDF::NcGroup & group ,
                           Block * block_reference = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF THE DataMapping ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the DataMapping
 *  @{ */

 /// sets the value of the associated data
 /** This function sets the value of the data associated with this
  * DataMapping.
  *
  * @param data a (const) reference to the vector that stores the value of the
  *        data that will be set.
  *
  * @param issueMod indicates if and how the a "physical" Modification should
  *        be issued.
  *
  * @param issueAMod indicates if and how the an "abstract" Modification
  *        should be issued.
  */

 virtual void set_data( const std::vector< double > & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const = 0;

/*--------------------------------------------------------------------------*/

 /// sets the value of the associated data
 /** This function sets the value of the data associated with this
  * DataMapping.
  *
  * @param data a (const) reference to the Eigen::ArrayXd that stores the
  *        value of the data that will be set.
  *
  * @param issueMod indicates if and how the a "physical" Modification should
  *        be issued.
  *
  * @param issueAMod indicates if and how the an "abstract" Modification
  *        should be issued.
  */

 virtual void set_data( const Eigen::ArrayXd & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const = 0;

/*--------------------------------------------------------------------------*/

 /// serializes this DataMapping
 /** This function serializes this DataMapping into the given NcGroup \p
  * group.
  *
  * @param group The NcGroup in which this DataMapping will be serialized.
  *
  * @param block_reference A pointer to the Block that may be used as
  *        reference when serializing an AbstractPath.
  */

 virtual void serialize( netCDF::NcGroup & group ,
                         Block * block_reference = nullptr ) const = 0;

/**@} ----------------------------------------------------------------------*/

};  // end( class( DataMapping ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SimpleDataMappingBase -----------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- GENERAL NOTES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// SimpleDataMappingBase derives from DataMapping
/**
 * SimpleDataMappingBase is a class intended to be the base class for all
 * SimpleDataMapping. It provides (pure virtual) function for setting the
 * SetFrom and SetTo sets, the caller object, and the function associated with
 * the SimpleDataMapping. See SimpleDataMapping for details.
 */

class SimpleDataMappingBase : public DataMapping {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 using Index = Block::Index;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE SimpleDataMapping --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the SimpleDataMappingBase
 *  @{ */

 /** This function sets the elements of the SetFrom set. It receives the \p
  * set_elements vector containing the elements defining the SetFrom set. If
  * the SetFrom set is a Range, let say representing the interval [a, b), then
  * \p set_elements must have "a" as its first element and "b" as its second
  * element. If SetFrom set is a Subset, then \p set_elements contains the
  * elements of the Subset.
  *
  * @param set_elements The vector containing the elements defining the
  *        SetFrom set.
  */
 virtual void set_set_from( const std::vector< Index > & set_elements ) = 0;

/*--------------------------------------------------------------------------*/

 /** This function sets the elements of the SetTo set. It receives the \p
  * set_elements vector containing the elements defining the SetTo set. If the
  * SetTo set is a Range, let say representing the interval [a, b), then \p
  * set_elements must have "a" as its first element and "b" as its second
  * element. If SetTo set is a Subset, then \p set_elements contains the
  * elements of the Subset.
  *
  * @param set_elements The vector containing the elements defining the
  *        SetTo set.
  */
 virtual void set_set_to( const std::vector< Index > & set_elements ) = 0;

/*--------------------------------------------------------------------------*/

/** This function sets the caller object based on the given AbstractPath.
 *
 * @param path The AbstractPath from the reference Block to the caller
 *        object.
 *
 * @param block_reference A pointer to the Block that serves as the reference
 *        Block in the path to the caller object.
 */
 virtual void set_caller( const AbstractPath & path ,
                          Block * block_reference ) = 0;

/*--------------------------------------------------------------------------*/

/** It sets the function associated with this SimpleDataMappingBase. The
 * function is retrieved from the methods factory based on its name.
 *
 * @param function_name The name of the function as registered in the methods
 *        factory.
 */
 virtual void set_function( const std::string & function_name ) = 0;

/**@} ----------------------------------------------------------------------*/

};  // end( class( SimpleDataMappingBase ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS SimpleDataMapping -------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- GENERAL NOTES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// SimpleDataMapping derives from SimpleDataMappingBase
/**
 * SimpleDataMapping is a template class that derives from
 * SimpleDataMappingBase and is used to define some common kinds of data
 * mapping. We define two vectors: the large one and the small one. The large
 * vector refers to the vector that is given as input to the set_data()
 * method. This is the vector containing all the data that can be used by the
 * SimpleDataMapping. The small vector is a vector formed by a subset of the
 * elements of the large vector. This is the vector that will effectively be
 * used to perform some computation. This computation is typically the task of
 * changing the data of some object based on this small vector. There is a
 * mapping defined by the SetFrom set that specifies which elements of the
 * large vector are used to compose the small vector. This SetFrom set
 * contains the indices of these elements in the large vector. The small
 * vector is the one that will typically impact the data of some object. The
 * SetTo set can be used to specify which part of this data is affected.
 *
 * As an example, consider the case in which the large vector contains data
 * related to costs and capacities of arcs of a network. Suppose this network
 * is represented by a class Network. A SimpleDataMapping could be used to set
 * the capacities of the arcs of a Network object considering the data
 * provided by this large vector. Some elements of this large vector would be
 * extracted and form a small vector containing the capacities of some
 * arcs. The indices of the elements that are extracted from the large vector
 * are specified by the SetFrom set. This set could be, for instance, the set
 * {0, 3, 8, 11}. This means that the elements at positions 0, 3, 8, and 11 in
 * the large vector are selected to form a small vector with four
 * elements. This small vector would then be used to change the capacities of
 * some arcs of the Network object. The arcs whose capacities would be
 * modified could be specified by the SetTo set. This could be the set [2, 6),
 * for instance, stating that the arcs with indices 2, 3, 4, and 5 would have
 * their capacities changed according to the small vector.
 *
 * Besides the SetFrom and SetTo sets, the SimpleDataMapping also has a
 * pointer to a function, which is invoked within the set_data() method. This
 * is a function that receives, in particular, a pointer to a Block, the small
 * vector, and the SetTo set. If the SetTo set is a Block::Subset, then the
 * type of this function is
 *
 *    Block::FunctionType< typename std::vector< DataType >::const_iterator ,
 *                         SetTo && , bool >
 *
 * If the SetTo set is a Block::Range, then the type of this function is
 *
 *    Block::FunctionType< typename std::vector< DataType >::const_iterator ,
 *                         const SetTo & >
 *
 * Please refer to the definition of Block::FunctionType for completely
 * understanding the type of this function.
 *
 * In the network example above, this function could be, for instance,
 *
 * void set_capacities( Network * network ,
 *                      std::vector<double>::const_iterator capacities ,
 *                      const Range & indices ,
 *                      c_ModParam , c_ModParam );
 *
 * Ignoring the details of the type of this function, this is a function that
 * receives a pointer to a Network object, a vector of capacities, and a Range
 * of indices. This function could be responsible for changing the capacities
 * of the arcs (of the given Network object) specified by the "indices"
 * parameter according to the given capacities.
 *
 * As you can see, the function associated with a SimpleDataMapping receives a
 * pointer to a Block as its first parameter. This is a pointer to the caller
 * object; the object that "invokes" the function.
 *
 * Finally, the SimpleDataMapping is also determined by the type of the data
 * of the small vector, the DataType.
 *
 * Notice that a SimpleDataMapping is general enough in the sense that it is
 * not only meant to change the data of some object, but perform arbitrary
 * computation defined by the function associated with this SimpleDataMapping.
 *
 * In summary, a SimpleDataMapping has the following template parameters:
 *
 * - SetFrom: This is the type of the set that selects the appropriate data
 *            from data vector. It must be either Block::Range or
 *            Block::Subset.
 *
 * - SetTo: This is the type of the set that indicates which part of the data
 *          of the caller object that is affected. It must be either
 *          Block::Range or Block::Set.
 *
 * - DataType: This is the type of the data of the "small" vector (typically
 *             the type of the data that will be set in the caller object).
 *
 * - Caller: This is the type of the caller object, which is the object that
 *           will "invoke" the function. By default, Caller is Block.
 */

template< class SetFrom = Block::Range , class SetTo = Block::Range ,
          class DataType = double , class Caller = Block >
class SimpleDataMapping : public SimpleDataMappingBase {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 using Range = Block::Range;
 using Subset = Block::Subset;

 using F = std::conditional_t< std::is_same_v< SetTo , Subset > ,
    Block::FunctionType< typename std::vector< DataType >::const_iterator ,
                         SetTo && , bool > ,
    Block::FunctionType< typename std::vector< DataType >::const_iterator ,
                         const SetTo & > >;

/**@} ----------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING SimpleDataMapping --------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing SimpleDataMapping
 *  @{ */

 /// constructor of SimpleDataMapping
 /** The constructor of SimpleDataMapping must receive the following
  * parameters:
  *
  * @param function The pointer to the to be invoked.
  *
  * @param caller The pointer to the object that will be the first argument
  *        when the given \p function is invoked. If \p function is a member
  *        function of Caller, then \p caller is the object that will invoke
  *        the function.
  *
  * @param set_from The set specifying which part of the input data should be
  *                 considered.
  *
  * @param set_to The set specifying which part of the data that will change.
  */
 SimpleDataMapping( const F * function = nullptr , Caller * caller = nullptr ,
              const SetFrom & set_from = {} , const SetTo & set_to = {} ) :
  function( function ) , caller( caller ) , set_from( set_from ) ,
  set_to( set_to ) {

  ordered = true;

  if constexpr( std::is_same_v< SetTo , Subset > ) {
   ordered = std::is_sorted( std::begin( set_to ), std::end( set_to ) );
  }
 }

/*--------------------------------------------------------------------------*/

 /// destructor
 virtual ~SimpleDataMapping() {}

/*--------------------------------------------------------------------------*/

 /// deserialize a SimpleDataMapping from a netCDF::NcGroup
 /** Deserialize a SimpleDataMapping from a netCDF::NcGroup. The format is
  * specified in the comments of the serialize() method.
  *
  * @param group The netCDF::NcGroup from which to read the data.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller together with its AbstractPath.
  */

 virtual void deserialize( const netCDF::NcGroup & group ,
                           Block * block_reference ) override {

  // FunctionName

  std::string function_name;
  ::SMSpp_di_unipi_it::deserialize< std::string >( group , "FunctionName" ,
                                                   & function_name , false );
  function = Block::get_method< F >( function_name );

  // AbstractPath

  {
   auto path_group = group.getGroup( "AbstractPath" );
   if( path_group.isNull() )
    std::logic_error( "SimpleDataMapping::deserialize: group 'AbstractPath' "
                      "was not found." );

   const auto path = AbstractPath::deserialize( path_group );

   caller = AbstractPath::get_element< Caller >( path , block_reference );
  }

  // SetFrom and SetTo

  {
   std::vector< Index > set_size;
   ::SMSpp_di_unipi_it::deserialize( group , "SetSize" , set_size , false );

   if( set_size.size() != 2 )
    throw( std::logic_error( "SimpleDataMapping::deserialize: array 'SetSize' "
                             "must have size 2." ) );

   std::vector< Index > set_elements;
   ::SMSpp_di_unipi_it::deserialize( group , "SetElements" , set_elements , false );

   Index next_index = 0;
   if constexpr( std::is_same_v< SetFrom , Range > ) {
    if( set_elements.size() < 3 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'SetElements' array." ) );
    set_from = Range( set_elements[ 0 ] , set_elements[ 1 ] );
    next_index = 2;
   }
   else {
    if( set_elements.size() < set_size[ 0 ] + 1 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'SetElements' array." ) );
    set_from.resize( set_size[ 0 ] );
    for( Index i = 0; i < set_size[ 0 ]; ++i )
     set_from[ i ] = set_elements[ i ];
    next_index = set_size[ 0 ];
   }

   ordered = true;
   if constexpr( std::is_same_v< SetTo , Range > ) {
    if( set_elements.size() < next_index + 2 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'SetElements' array." ) );
    set_to = Range( set_elements[ next_index ] , set_elements[ next_index + 1 ] );
   }
   else {
    if( set_elements.size() < next_index + set_size[ 1 ] )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'SetElements' array." ) );
    set_to.resize( set_size[ 1 ] );
    for( Index i = 0; i < set_size[ 1 ]; ++i )
     set_to[ i ] = set_elements[ next_index + i ];
    ordered = std::is_sorted( std::begin( set_to ), std::end( set_to ) );
   }
  }

  if( cardinality( set_from ) != cardinality( set_to ) ) {
   throw( std::logic_error( "SimpleDataMapping::deserialize: 'SetFrom' and "
                            "'SetTo' must have the same cardinality." ) );
  }
 }

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE SimpleDataMapping --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the SimpleDataMapping
 *  @{ */

 virtual void set_data( const std::vector< double > & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const override {
  auto sub_data = extract< DataType >( data, set_from );

  if constexpr( std::is_same_v< SetTo , Subset > )
   std::invoke( * function , caller , sub_data.cbegin() ,
                Subset( set_to ) , ordered , issueMod , issueAMod );
  else
   std::invoke( * function , caller , sub_data.cbegin() ,
                set_to , issueMod , issueAMod );

 }

/*--------------------------------------------------------------------------*/

 virtual void set_data( const Eigen::ArrayXd & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const override {
  auto sub_data = extract< DataType >( data, set_from );

  if constexpr( std::is_same_v< SetTo , Subset > )
   std::invoke( * function , caller , sub_data.cbegin() ,
                Subset( set_to ) , ordered , issueMod , issueAMod );
  else
   std::invoke( * function , caller , sub_data.cbegin() ,
                set_to , issueMod , issueAMod );
 }

/*--------------------------------------------------------------------------*/

 /// serialize a SimpleDataMapping into a netCDF::NcGroup
 /** Serialize a SimpleDataMapping into a netCDF::NcGroup with the following
  * format:
  *
  * - The one-dimensional variable "SetSize", an array of type
  *   netCDF::NcUint64 with two elements indicating the sizes (or types) of
  *   the "SetFrom" and "SetTo" sets. SetSize[0] indicates the size (or type)
  *   of the "SetFrom" set and SetSize[1] indicates the size (or type) of the
  *   "SetTo" set. For each i in {0,1}, if SetSize[i] == 0, then the
  *   corresponding set is a Range. Otherwise, if SetSize[i] != 0, then the
  *   corresponding set is a Subset whose size is SetSize[i]. Notice,
  *   therefore, that SetSize[i] is not the size of the corresponding set when
  *   SetSize[i] == 0. In this case, it only indicates that the set is a
  *   Range, whose size (and elements) can be determined by the "SetElements"
  *   variable. This variable is optional. If it is not provided, then the
  *   "SetFrom" and "SetTo" sets are assumed to be Range.
  *
  * - The one-dimensional variable "SetElements", of type netCDF::NcUint64,
  *   containing the concatenation of the representations of the sets
  *   "SetFrom" and "SetTo". A Subset is represented by a sequence of indices
  *   (which are the elements of the Subset); while a Range is represented by
  *   two indices "a" and "b" such that the Range set is given by the integers
  *   in the closed-open interval [a, b). For instance, if "SetFrom" is the
  *   Subset {3, 6, 8} and "SetTo" is the Range [2, 5), then "SetElements"
  *   would be the array (3, 6, 8, 2, 5).
  *
  * - The variable "FunctionName", whose type is netCDF::NcString, containing
  *   the name of the function as it is registered in the methods factory.
  *
  * - The variable "DataType", of type netCDF::NcChar, specifying the type of
  *   the data that is associated with this SimpleDataMapping. This is the
  *   type of the data that can be set by this SimpleDataMapping (i.e., the
  *   DataType template parameter of SimpleDataMapping). This variable is
  *   optional. If it is not present, then the data type associated with the
  *   SimpleDataMapping is assumed to be "double". If it is present, it can
  *   be either 'I' or 'D', indicating that the type of the data is
  *   "int" or "double", respectively.
  *
  * - The variable "Caller", of type netCDF::NcChar, containing the type of
  *   the caller object associated with this SimpleDataMapping. It and can be
  *   either 'B', indicating that the caller is a Block, or 'F', indicating
  *   that the caller is a Function. This variable is optional. If it is not
  *   provided, then we assume that Caller = 'B', that is, we assume that the
  *   caller is a Block.
  *
  * - The group "AbstractPath" containing the description of the AbstractPath
  *   representing the path to the caller object.
  *
  * @param group The netCDF::NcGroup into which this SimpleDataMapping will be
  *        serialized.
  *
  * @param block_reference The pointer to the reference Block that is used to
  *        construct the AbstractPath to the caller object.
  */

 virtual void serialize( netCDF::NcGroup & group ,
                         Block * block_reference ) const override {

  // FunctionName

  auto function_name = Block::get_method_name( function );
  ::SMSpp_di_unipi_it::serialize< std::string >
   ( group , "FunctionName" , netCDF::NcString() , function_name );

  // AbstractPath

  if constexpr( std::is_base_of_v< Function , Caller > ) {
   const auto path = AbstractPath::build_path< Function >( caller ,
                                                           block_reference );
   auto path_group = group.addGroup( "AbstractPath" );
   AbstractPath::serialize( path , path_group );
  }
  else {
   const auto path = AbstractPath::build_path< Caller >( caller ,
                                                         block_reference );
   auto path_group = group.addGroup( "AbstractPath" );
   AbstractPath::serialize( path , path_group );
  }

  // SetFrom and SetTo (SetSize and SetElements)

  Index set_elements_size = 0;
  std::vector< Index > set_size( 2 );
  if constexpr( std::is_same_v< Range , SetFrom > ) {
   set_size[ 0 ] = 0;
   set_elements_size = 2;
  }
  else {
   set_size[ 0 ] = set_from.size();
   set_elements_size = set_from.size();
  }

  if constexpr( std::is_same_v< Range , SetTo > ) {
   set_size[ 1 ] = 0;
   set_elements_size += 2;
  }
  else {
   set_size[ 1 ] = set_to.size();
   set_elements_size += set_to.size();
  }

  auto SetSize_dim = group.addDim( "SetSize_dim" , set_size.size() );

  ::SMSpp_di_unipi_it::serialize( group , "SetSize" , netCDF::NcUint64() ,
                                  SetSize_dim , set_size , false );

  std::vector< Index > set_elements( set_elements_size );
  Index next_index = 0;
  if constexpr( std::is_same_v< Range , SetFrom > ) {
   set_elements[ 0 ] = set_from.first;
   set_elements[ 1 ] = set_from.second;
   next_index = 2;
  }
  else {
   for( Index i = 0; i < set_from.size(); ++i )
    set_elements[ i ] = set_from[ i ];
   next_index = set_from.size();
  }

  if constexpr( std::is_same_v< Range , SetTo > ) {
   set_elements[ next_index ] = set_to.first;
   set_elements[ next_index + 1 ] = set_to.second;
  }
  else {
   for( Index i = 0; i < set_to.size(); ++i )
    set_elements[ next_index + i ] = set_to[ i ];
  }

  auto SetElements_dim = group.addDim( "SetElements_dim" ,
                                       set_elements.size() );

  ::SMSpp_di_unipi_it::serialize( group , "SetElements" , netCDF::NcUint64() ,
                                  SetElements_dim , set_elements , false );


  // DataType

  ::SMSpp_di_unipi_it::serialize( group , "DataType" , netCDF::NcChar() ,
                                  get_id< DataType >() );

  // Caller type

  char caller_type = 'B';
  if constexpr( std::is_base_of_v< Function , Caller > )
   caller_type = 'F';

  ::SMSpp_di_unipi_it::serialize( group , "Caller" , netCDF::NcChar() ,
                                  caller_type );
 }

/*--------------------------------------------------------------------------*/

 /// sets the caller of this DataMapping
 /** Defines the given \p caller as the caller of this DataMapping.
  *
  * @param caller The new caller of this DataMapping.
  */

 void set_caller( Caller * caller ) {
  this->caller = caller;
 }

/*--------------------------------------------------------------------------*/

 virtual void set_set_from( const std::vector< Index > & set_elements )
  override {
  if constexpr( std::is_base_of_v< Range , SetFrom > ) {
   if( set_elements.size() < 2 )
    throw( std::invalid_argument
           ( "SimpleDataMapping::set_set_from(): the size of 'set_elements' "
             "must be at least two.") );
   set_from = Range( set_elements[ 0 ] , set_elements[ 1 ] );
  }
  else {
   set_from = Subset( set_elements );
  }
 }

/*--------------------------------------------------------------------------*/

 virtual void set_set_to( const std::vector< Index > & set_elements ) override {
  if constexpr( std::is_base_of_v< Range , SetTo > ) {
   if( set_elements.size() < 2 )
    throw( std::invalid_argument
           ( "SimpleDataMapping::set_set_to(): the size of 'set_elements' "
             "must be at least two.") );
   set_to = Range( set_elements[ 0 ] , set_elements[ 1 ] );
  }
  else {
   set_to = Subset( set_elements );
  }
 }

/*--------------------------------------------------------------------------*/

 virtual void set_caller( const AbstractPath & path , Block * block_reference )
  override {
  caller = AbstractPath::get_element< Caller >( path , block_reference );
 }

/*--------------------------------------------------------------------------*/

 virtual void set_function( const std::string & function_name ) override {
  function = Block::get_method< F >( function_name );
 }

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR READING THE DATA OF THE SimpleDataMapping -----------*/
/*--------------------------------------------------------------------------*/

 /// returns a pointer to the function
 /** Returns a pointer to the function that is associated with this
  * SimpleDataMapping.
  */

 const F * get_function() const {
  return function;
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the caller of the function
 /** Returns a pointer to the caller of the function that is associated with
  * this SimpleDataMapping.
  */

 Caller * get_caller() const {
  return caller;
 }

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*------------------------- PROTECTED METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

 template< class T >
 static constexpr char get_id();

/*--------------------------------------------------------------------------*/

 template<>
 static constexpr char get_id< Block::Range >() { return 'R'; }

/*--------------------------------------------------------------------------*/

 template<>
 static constexpr char get_id< Block::Subset >() { return 'S'; }

/*--------------------------------------------------------------------------*/

 template<>
 static constexpr char get_id< double >() { return 'D'; }

/*--------------------------------------------------------------------------*/

 template<>
 static constexpr char get_id< int >() { return 'I'; }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract( const std::vector< T > & data ,
                                  const Block::Range & range ) {
  if( range.first >= range.second )
   return {};
  assert( range.second <= data.size() );
  return std::vector< S >
   ( data.begin() + range.first ,
     data.begin() + std::min<decltype( data.size() )>( range.second ,
                                                       data.size() ) );
 }

/*--------------------------------------------------------------------------*/

 template< class S = double >
 static std::vector< S > extract( const Eigen::ArrayXd & data ,
                                  const Block::Range & range ) {
  if( range.first >= range.second )
   return {};
  assert( range.second <= data.size() );
  return std::vector< S >
   ( data.data() + range.first ,
     data.data() + std::min<decltype( data.size() )>( range.second ,
                                                      data.size() ) );
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract
 ( const typename std::vector< T >::const_iterator & begin ,
   const Block::Range & range ) {
  if( range.first >= range.second )
   return {};
  return std::vector< S >( begin + range.first , begin + range.second );
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract( const std::vector< T > & data ,
                                  const Block::Subset & subset ) {
  std::size_t size = subset.size();
  std::vector< S > output( size );
  for(std::size_t i = 0; i < size; ++i) {
   assert( subset[ i ] >= 0 && subset[ i ] < data.size() );
   output[ i ] = data[ subset[ i ] ];
  }
  return output;
 }

/*--------------------------------------------------------------------------*/

 template< class S = double >
 static std::vector< S > extract( const Eigen::ArrayXd & data ,
                                  const Block::Subset & subset ) {
  std::size_t size = subset.size();
  std::vector< S > output( size );
  for(std::size_t i = 0; i < size; ++i) {
   assert( subset[ i ] >= 0 && subset[ i ] < data.size() );
   output[ i ] = data( subset[ i ] );
  }
  return output;
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract_from_ordered_subset
 ( const typename std::vector< T >::const_iterator & begin ,
   const Block::Subset & subset ) {
  std::size_t size = subset.size();
  std::vector< S > output( size );
  std::size_t j = 0;
  auto it = begin;
  for( std::size_t i = 0 ; i < size ; ++i ) {
   assert( j <= subset[ i ] );
   while( j++ < subset[ i ] ) ++it;
   output[ i ] = *it++;
  }
  return output;
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract
 ( const typename std::vector< T >::const_iterator & begin ,
   const Block::Subset & subset, bool subset_is_ordered = false ) {
  if( subset_is_ordered )
   return extract_from_ordered_subset< S , T >( begin , subset );
  else {
   auto ordered_subset = Block::Subset( subset );
   std::sort( ordered_subset.begin() , ordered_subset.end() );
   return extract_from_ordered_subset< S , T >( begin , ordered_subset );
  }
 }

/*--------------------------------------------------------------------------*/

 bool deserialize( const netCDF::NcGroup & group , const std::string & suffix ,
                   Block::Range & range , bool optional = true ) {

  {
   auto ncVar = group.getVar( "First" + suffix );
   if( ncVar.isNull() ) {
    if( optional )
     return false;
    throw( std::invalid_argument
           ( "SimpleDataMapping::deserialize(): variable 'First" + suffix +
             "' is not present in group '" + group.getName() + "'." ) );
   }
   ncVar.getVar( & range.first );
  }

  {
   auto ncVar = group.getVar( "Second" + suffix );
   if( ncVar.isNull() ) {
    if( optional )
     return false;
    throw( std::invalid_argument
           ( "deserialize(): variable 'Second" + suffix + "' is not present "
             "in group '" + group.getName() + "'." ) );
   }
   ncVar.getVar( & range.second );
  }

  return true;
 }

/*--------------------------------------------------------------------------*/

 bool deserialize( const netCDF::NcGroup & group , const std::string & suffix ,
                   Block::Subset & subset , bool optional = true ) {
  return ::SMSpp_di_unipi_it::deserialize( group , "Subset" + suffix ,
                                           subset , optional );
 }

/*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ,
                         const std::string & suffix ,
                         const Block::Range & range ) const {
  ::SMSpp_di_unipi_it::serialize( group , "First" + suffix ,
                                  netCDF::NcUint64() , range.first );
  ::SMSpp_di_unipi_it::serialize( group , "Second" + suffix ,
                                  netCDF::NcUint64() , range.second );
 }

/*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ,
                         const std::string & suffix ,
                         const Block::Subset & subset ) const {
  auto dim = group.addDim( "Size" + suffix , subset.size() );
  ::SMSpp_di_unipi_it::serialize( group , "Subset" + suffix ,
                                  netCDF::NcUint64() , dim , subset , false );
 }

/*--------------------------------------------------------------------------*/

 static Index cardinality( const Range & range ) {
  if( range.second > range.first )
   return range.second - range.first;
  return 0;
 }

/*--------------------------------------------------------------------------*/

 static Index cardinality( const Subset & subset ) {
  return subset.size();
 }

/*--------------------------------------------------------------------------*/

 static void fill( Range & range , Index size ) {
  range.first = 0;
  range.second = size;
 }

/*--------------------------------------------------------------------------*/

 static void fill( Subset & subset , Index size ) {
  subset.resize( size );
  for( Index i = 0 ; i < size ; ++i )
   subset[ i ] = i;
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// Pointer to the function that will be invoked
 const F * function;

 /// Pointer to the object that will invoke the function
 Caller * caller;

 /// The set specifying which subset of the given data should be considered
 SetFrom set_from;

 /// The set that must be passed as argument to the function being invoked
 SetTo set_to;

 /// Indicates whether the SetTo set is ordered
 bool ordered;

};  // end( class( SimpleDataMapping ) )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS SimpleDataMappingFactory ---------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- GENERAL NOTES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// class to provide a simple factory for SimpleDataMapping
/** This class is intended to provide a simple way of constructing a
 * SimpleDataMapping, specially when a SimpleDataMapping is needed during
 * deserialization.
 */

class SimpleDataMappingFactory {

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 using Index = Block::Index;
 using Range = Block::Range;
 using Subset = Block::Subset;

/*--------------------------------------------------------------------------*/
/*--------------------------- PUBLIC METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
 /// constructs a SimpleDataMapping
 /** This function constructs a SimpleDataMapping whose template arguments are
  * given by the given \p types string. We consider a SimpleDataMapping with
  * four template parameters: SetFrom, SetTo, DataType, and Caller. The first
  * one is the type of the set that specifies the relevant entries of the data
  * vector. The second one, SetTo, is the type of the set that specifies which
  * part of the object's data that must be modified or is affected. The
  * DataType parameter indicates the numerical type of the data that the
  * method that modifies the object expects. The Caller parameter indicates
  * the type of the caller object.
  *
  * The argument for each of these template parameters is given by a
  * character. The supported sets for SetFrom and SetTo are Range and
  * Subset. These sets are identified by the characters 'R' and 'S',
  * respectively. The DataType can be either int or double, which are
  * identified by the characters 'I' and 'D', respectively. The Caller can be
  * either a Block or a Function, which are identified by the characters 'I'
  * and 'D', respectively.
  *
  * The \p types string can have size 3 or 4. If it has size 4, then we assume
  * it provides the types for SetFrom, SetTo, DataType, and Caller (in this
  * order).  If it has size 3, then we assume it provides the types for
  * SetFrom, SetTo, and DataType (in this order) and that the type of Caller
  * is Block.
  *
  * For instance, to create a SimpleDataMapping having SetFrom as Range, SetTo
  * as Subset, DataType as double, and Caller as Block, one could pass either
  * the "RSD" or the "RSDB" string to this function. To create a
  * SimpleDataMapping having SetFrom as Range, SetTo as Subset, DataType as
  * double, and Caller as Function, one must pass the "RSDF" string to this
  * function.
  *
  * If a non-supported type is given, an exception is thrown.
  *
  * @param types A string indicating the template arguments of the
  *              SimpleDataMapping.
  *
  * @return A pointer to the SimpleDataMapping that was constructed.
  */

 static SimpleDataMappingBase * new_SimpleDataMapping
 ( const std::string & types ) {

  if( types.size() == 4 && types[ 3 ] == 'F' ) {
   const auto t = types.substr( 0, 3 );
   if( t == "RRD" )
    return new SimpleDataMapping<Range ,Range ,double,BendersBFunction>;
   else if( t == "RRI" )
    return new SimpleDataMapping<Range ,Range ,int   ,BendersBFunction>;
   else if( t == "RSD" )
    return new SimpleDataMapping<Range ,Subset,double,BendersBFunction>;
   else if( t == "RSI" )
    return new SimpleDataMapping<Range ,Subset,int   ,BendersBFunction>;
   else if( t == "SRD" )
    return new SimpleDataMapping<Subset,Range ,double,BendersBFunction>;
   else if( t == "SRI" )
    return new SimpleDataMapping<Subset,Range ,int   ,BendersBFunction>;
   else if( t == "SSD" )
    return new SimpleDataMapping<Subset,Subset,double,BendersBFunction>;
   else if( t == "SSI" )
    return new SimpleDataMapping<Subset,Subset,int   ,BendersBFunction>;
   else
    throw std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                 "parameter types string: " + types );
  }
  else if( types.size() == 3 || ( types.size() == 4 && types[ 3 ] == 'B' ) ) {
   const auto t = types.substr( 0, 3 );
        if( t == "RRD" ) return new SimpleDataMapping<Range ,Range ,double>;
   else if( t == "RRI" ) return new SimpleDataMapping<Range ,Range ,int>;
   else if( t == "RSD" ) return new SimpleDataMapping<Range ,Subset,double>;
   else if( t == "RSI" ) return new SimpleDataMapping<Range ,Subset,int>;
   else if( t == "SRD" ) return new SimpleDataMapping<Subset,Range ,double>;
   else if( t == "SRI" ) return new SimpleDataMapping<Subset,Range ,int>;
   else if( t == "SSD" ) return new SimpleDataMapping<Subset,Subset,double>;
   else if( t == "SSI" ) return new SimpleDataMapping<Subset,Subset,int>;
   else
    throw std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                 "parameter types string: " + types );
  }
  else
   throw std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                "parameter types string: " + types );
 }

/*--------------------------------------------------------------------------*/

 /// deserializes a vector of SimpleDataMapping
 /** This function deserializes a vector of SimpleDataMapping and returns
  * it. A vector of SimpleDataMapping is specified as follows.
  *
  * - The "NumberDataMappings" dimension indicates the number of
  *   SimpleDataMapping that is present in the vector of SimpleDataMapping.
  *
  * - The one-dimensional variable "DataType" indexed over the
  *   "NumberDataMappings" dimension is an array of type netCDF::NcChar that
  *   specifies the type of the data that is associated with each
  *   SimpleDataMapping of the vector. This is the type of the data that can
  *   be set by the SimpleDataMapping (i.e., the DataType template parameter
  *   of SimpleDataMapping). This variable is optional. If it is not present,
  *   then the data type associated with each SimpleDataMapping in this vector
  *   is assumed to be double. If it is present then, for each i in {0, ...,
  *   NumberDataMappings-1}, DataType[ i ] is the type of the data associated
  *   with the i-th SimpleDataMapping and can be either 'I' or 'D', indicating
  *   that the type of the data is int or double, respectively.
  *
  * - The one-dimensional variable "SetSize" is an array of type
  *   netCDF::NcUint64 with size (2 * NumberDataMappings) and indicates the
  *   size of the sets that define each SimpleDataMapping (the "SetFrom" and
  *   "SetTo" sets). This variable is optional. If it is not present, then all
  *   sets are assumed to be Range. If it is present, then SetSize[ 2i + k ]
  *   is the size of the SetFrom set of the i-th SimpleDataMapping if k = 0 or
  *   the size of the SetTo set of the i-th SimpleDataMapping if k = 1. If
  *   SetSize[ j ] == 0, then the corresponding set is a Range. Otherwise, the
  *   corresponding set is a Subset of size SetSize[ j ].
  *
  * - The one-dimensional variable "SetElements", of type netCDF::NcUint64, is
  *   an array containing the concatenation of the representations of the sets
  *   SetFrom and SetTo. A Subset is represented by a sequence of indices
  *   (which are the elements of the Subset); while a Range is represented by
  *   two indices a and b such that the Range set is given by the integers in
  *   the closed-open interval [a, b). If we let SetFrom_i and SetTo_i denote
  *   the representations of the SetFrom and SetTo sets of the i-th
  *   SimpleDataMapping, then "SetElements" is the array
  *
  *   ( SetFrom_0 , SetTo_0 , SetFrom_1 , SetTo_1 , ..., SetFrom_N, SetTo_N )
  *
  *   where N = NumberDataMappings - 1.
  *
  * - The one-dimensional variable "FunctionName" of type netCDF::NcString and
  *   indexed over "NumberDataMappings" contains the names of the functions
  *   associated with each SimpleDataMapping. FunctionName[ i ] gives the name
  *   of the function (as registered in the methods factory) associated with
  *   the i-th SimpleDataMapping.
  *
  * - A sub-group called "AbstractPath", containing a vector of AbstractPath
  *   with the paths to the Block. The i-th path in this vector of
  *   AbstractPath is the path to the Block associated with the i-th
  *   SimpleDataMapping.
  *
  * - The one-dimensional variable "Caller", of type netCDF::NcChar and
  *   indexed over "NumberDataMappings", containing the types of the caller
  *   objects associated with each SimpleDataMapping. Caller[ i ] gives the
  *   type of the caller object associated with the i-th SimpleDataMapping and
  *   can be either 'B', indicating that the caller is a Block, or 'F',
  *   indicating that the caller is a Function. This variable is optional. If
  *   it is not provided, then we assume that Caller[ i ] = 'B' for each i in
  *   {0, ..., NumberDataMappings - 1}, that is, we assume that all callers
  *   are Block.
  *
  * @param group The NcGroup that contains the description of the
  *              SimpleDataMappings to be deserialized.
  *
  * @param data_mappings The vector to which the pointers to the
  *        SimpleDataMapping will be added.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller together with its AbstractPath.
  */

 static void vector_deserialize
 ( const netCDF::NcGroup & group ,
   std::vector< std::unique_ptr< DataMapping > > & data_mappings ,
   Block * block_reference ) {

  Index num_data_mappings;
  ::SMSpp_di_unipi_it::deserialize_dim( group , "NumberDataMappings" ,
                                        num_data_mappings , false );

  auto data_type_var = group.getVar( "DataType" );
  if( ! data_type_var.isNull() &&
      ( data_type_var.getDimCount() != 1 ||
        data_type_var.getDim( 0 ).getSize() != num_data_mappings ) )
    throw( std::invalid_argument
           ( "SimpleDataMappingFactory::vector_deserialize: 'DataType' must"
             " be a one-dimensional array with size 'NumberDataMappings'." ) );

  auto caller_type_var = group.getVar( "Caller" );
  if( ! caller_type_var.isNull() &&
      ( caller_type_var.getDimCount() != 1 ||
        caller_type_var.getDim( 0 ).getSize() != num_data_mappings ) )
    throw( std::invalid_argument
           ( "SimpleDataMappingFactory::vector_deserialize: 'Caller' must"
             " be a one-dimensional array with size 'NumberDataMappings'." ) );

  auto function_name_var = group.getVar( "FunctionName" );
  if( function_name_var.isNull() || function_name_var.getDimCount() != 1 ||
      function_name_var.getDim( 0 ).getSize() != num_data_mappings )
    throw( std::invalid_argument
           ( "SimpleDataMappingFactory::vector_deserialize: 'FunctionName' must"
             " be a one-dimensional array with size 'NumberDataMappings'." ) );

  auto set_size_var = group.getVar( "SetSize" );
  if( ! set_size_var.isNull() ) {
   if( set_size_var.getDimCount() != 1 )
    throw( std::invalid_argument
           ( "SimpleDataMappingFactory::vector_deserialize: 'SetSize' must be "
             "a one-dimensional array." ) );

   if( set_size_var.getDim( 0 ).getSize() != 2 * num_data_mappings )
    throw( std::invalid_argument
           ( "SimpleDataMappingFactory::vector_deserialize: 'SetSize' must be "
             "a one-dimensional array with size 2*NumberDataMappings." ) );
  }

  auto set_elements_var = group.getVar( "SetElements" );
  if( set_elements_var.isNull() ) {
   throw( std::invalid_argument( "SimpleDataMappingFactory::vector_deserialize:"
                                 " 'SetElements' is not present." ) );
  }

  auto path_group = group.getGroup( "AbstractPath" );
  if( path_group.isNull() )
   throw( std::invalid_argument
          ( "SimpleDataMappingFactory::vector_deserialize: group 'AbstractPath'"
            " is not present." ) );

  auto paths =  AbstractPath::vector_deserialize( path_group );

  if( paths.size() != num_data_mappings )
   throw( std::invalid_argument
          ( "SimpleDataMappingFactory::vector_deserialize: group 'AbstractPath'"
            " must contain 'NumberDataMappings' paths." ) );

  Index next_index = 0;
  for( Index i = 0 ; i < num_data_mappings ; ++i ) {

   char set_from_type, set_to_type;
   std::vector< Index > set_from, set_to;
   {
    Index set_from_size , set_to_size;
    get_sets_type( set_size_var , set_from_type, set_to_type ,
                   set_from_size , set_to_size , i );

    if( set_from_size == 0 )
     set_from.resize( 2 );
    else
     set_from.resize( set_from_size );

    if( set_to_size == 0 )
     set_to.resize( 2 );
    else
     set_to.resize( set_to_size );
   }

   set_elements_var.getVar( { next_index } , { set_from.size() } ,
                            set_from.data() );
   next_index += set_from.size();

   set_elements_var.getVar( { next_index } , { set_to.size() } ,
                            set_to.data() );
   next_index += set_to.size();

   // DataType
   char data_type;
   data_type_var.getVar( { i } , { 1 } , & data_type );

   // Caller type
   char caller_type;
   caller_type_var.getVar( { i } , { 1 } , & caller_type );

   // FunctionName
   std::string function_name;
   function_name_var.getVar( { i } , { 1 } , & function_name );

   auto data_mapping = new_SimpleDataMapping( { set_from_type , set_to_type ,
                                                data_type , caller_type } );

   data_mapping->set_set_from( set_from );
   data_mapping->set_set_to( set_to );
   data_mapping->set_function( function_name );
   data_mapping->set_caller( paths[ i ] , block_reference );

   data_mappings.emplace_back( data_mapping );
  }
 }

/*--------------------------------------------------------------------------*/

 static void vector_serialize( netCDF::NcGroup & group ,
             const std::vector< std::unique_ptr< DataMapping > > & data_mappings ) {
  // TODO
 }

/*--------------------------------------------------------------------------*/

 /// deserialize a SimpleDataMapping from a netCDF::NcGroup
 /** Deserialize a SimpleDataMapping from a netCDF::NcGroup, with the
  * following format described in SimpleDataMapping::serialize().
  *
  * @param group The netCDF::NcGroup from which to read the data.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller together with its AbstractPath.
  */

 static DataMapping * deserialize( const netCDF::NcGroup & group ,
                                   Block * block_reference ) {

  // DataType

  char data_type;
  if( ! ::SMSpp_di_unipi_it::deserialize( group , "DataType" ,
                                          & data_type , true ) )
   data_type = 'D';

  // Caller type

  char caller_type;
  if( ! ::SMSpp_di_unipi_it::deserialize( group , "Caller" ,
                                          & caller_type , true ) )
   caller_type = 'B';

  char set_from_type, set_to_type;
  get_sets_type( group , set_from_type , set_to_type );

  auto data_mapping = new_SimpleDataMapping( { set_from_type , set_to_type ,
                                               data_type , caller_type } );

  data_mapping->deserialize( group , block_reference );

  return data_mapping;
 }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 static void get_sets_type( const netCDF::NcGroup & group ,
                            char & set_from_type , char & set_to_type ) {

  std::vector< Index > set_size;
  ::SMSpp_di_unipi_it::deserialize( group , "SetSize" , set_size , false );

  if( set_size.size() != 2 )
   throw( std::logic_error( "SimpleDataMappingFactory::get_sets_type: array "
                            "'SetSize' must have size 2." ) );

  set_from_type = set_size[ 0 ] > 0 ? 'S' : 'R';
  set_to_type   = set_size[ 1 ] > 0 ? 'S' : 'R';
 }

/*--------------------------------------------------------------------------*/

 static void get_sets_type( const netCDF::NcVar & set_size_var ,
                            char & set_from_type , char & set_to_type ,
                            Index & set_from_size , Index & set_to_size ,
                            const Index index ) {
  std::vector< Index > set_size( 2 );
  set_size_var.getVar( { 2 * index } , { 2 } , set_size.data() );

  set_from_type = set_size[ 0 ] > 0 ? 'S' : 'R';
  set_to_type   = set_size[ 1 ] > 0 ? 'S' : 'R';

  set_from_size = set_size[ 0 ];
  set_to_size   = set_size[ 1 ];
 }

/*--------------------------------------------------------------------------*/

};  // end( class( SimpleDataMappingFactory ) )

/** @} end( group( DataMapping_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* DataMapping.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File DataMapping.h ---------------------------*/
/*--------------------------------------------------------------------------*/

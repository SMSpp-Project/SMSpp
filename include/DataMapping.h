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
 * \date 09 - 01 - 2020
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
/*------------------------ CLASS SimpleDataMapping -------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- GENERAL NOTES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// SimpleDataMapping derives from DataMapping
/**
 * SimpleDataMapping is a template class that derives from DataMapping and is
 * used to define some common kinds of data mapping. We define two vectors:
 * the large one and the small one. The large vector refers to the vector that
 * is given as input to the set_data() method. This is the vector containing
 * all the data that can be used by the SimpleDataMapping. The small vector is
 * a vector formed by a subset of the elements of the large vector. This is
 * the vector that will effectively be used to perform some computation. This
 * computation is typically the task of changing the data of some object based
 * on this small vector. There is a mapping defined by the SetFrom set that
 * specifies which elements of the large vector are used to compose the small
 * vector. This SetFrom set contains the indices of these elements in the
 * large vector. The small vector is the one that will typically impact the
 * data of some object. The SetTo set can be used to specify which part of
 * this data is affected.
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
class SimpleDataMapping : public DataMapping {

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
 /** Deserialize a SimpleDataMapping from a netCDF::NcGroup, with the
  * following format:
  *
  * - The variable "FunctionName", whose type is netCDF::NcString, contains
  *   the name of the function as it is registered in the methods factory.
  *
  * - All the dimensions and variables that are necessary to describe an
  *   AbstractPath to the caller as specified in the comments to
  *   AbstractPath::deserialize().
  *
  * - All that is necessary to describe the SetFrom set. If SetFrom is a
  *   Range, then this set is specified by the variables "FirstFrom" and
  *   "SecondFrom", both of type netCDF::NcUint64, which contain the limits of
  *   the closed-open interval defining the Range. So, if the Range is the set
  *   [a, b), then FirstFrom must contain "a" and SecondFrom must contain
  *   "b". If SetFrom is a Subset, then this set is specified by the dimension
  *   "SizeFrom" indicating the size of the set and the one-dimensional
  *   variable "SubsetFrom", of type netCDF::NcUint64 and indexed over
  *   "SizeFrom", containing the elements of the set.
  *
  * - All that is necessary to describe the SetTo set. Its representation is
  *   the same as that of the SetFrom set, except for the names of the
  *   variables, which should be "FirstTo" and "SecondTo" if SetTo is a Range,
  *   and "SubsetTo" if the set is a Subset (accompanied by the dimension
  *   "SizeTo"). This set is optional. If it is not provided, then the full
  *   set of indices [ 0 , N ) is considered, where N is the size of the
  *   SetFrom set.
  *
  * - The attribute "TemplateParameterTypes" is a string with three characters
  *   that indicate the types of the template parameters of this class. The
  *   first and second characters indicate the type of the SetFrom and SetTo
  *   template parameters. Each of these two first characters can be either
  *   'R', indicating the set has type Block::Range, or 'S', indicating the
  *   set has type Block::Subset. The third character indicates the type of
  *   the data that this SimpleDataMapping sets. This character can be either
  *   'I', indicating the type of the data is int, or 'D', indicating the type
  *   of the data is double. This variable is optional. If it is not present,
  *   then "RRD" is considered, meaning that the SetFrom and SetTo sets are
  *   Range and the type of the data is double.
  *
  * @param group The netCDF::NcGroup from which to read the data.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller together with its AbstractPath.
  */

 virtual void deserialize( const netCDF::NcGroup & group ,
                           Block * block_reference ) override {

  std::string function_name;
  ::SMSpp_di_unipi_it::deserialize< std::string >( group , "FunctionName" ,
                                                   & function_name , false );
  function = Block::get_method< F >( function_name );

  if constexpr( std::is_base_of_v< Block , Caller > ) {
   const auto path = AbstractPath::deserialize( group );
   caller = AbstractPath::get_element< Block >( path , block_reference );
  }

  this->deserialize( group , "From" , set_from , false );

  ordered = true;
  if( ! this->deserialize( group , "To" , set_to , true ) ) {
   // SetTo was not specified. Therefore, we consider the set [0, N), where N
   // is the size of the SetFrom set.
   fill( set_to , cardinality( set_from ) );
  }
  else if constexpr( std::is_same_v< SetTo , Subset > ) {
   ordered = std::is_sorted( std::begin( set_to ), std::end( set_to ) );
   if constexpr( std::is_same_v< SetFrom , Subset > ) {
     // Both SetFrom and SetTo are Subset. So, we can reorder them if we wish.

     // If SetFrom is a Range and SetTo is an unordered Subset, then
     // reordering SetTo means reordering Range (which is not possible; the
     // Range would have to became a Subset, which is also not possible). So,
     // it is possible to sort set_from and set_to only when they are both
     // Subset.
    if( ! ordered ) {
     // TODO Should we sort the Subsets? Maybe not if one expects the same
     // Subsets when the DataMapping is serialized. This could be an option of
     // DataMapping.
    }
   }
  }

  if( cardinality( set_from ) != cardinality( set_to ) ) {
   throw( std::invalid_argument( "DataMapping::deserialize: SetFrom and SetTo "
                                 "must have the same cardinality." ) );
  }
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
  * @param group The NcGroup that contains the description of the
  *              SimpleDataMappings to be deserialized.
  *
  * @return A vector with the SimpleDataMapping.
  */

 static void vector_deserialize( const netCDF::NcGroup & group ,
             std::vector< std::unique_ptr< DataMapping > > & data_mappings ) {
  // TODO
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
 /** Serialize a SimpleDataMapping into a netCDF::NcGroup. The format is
  * specified in the comments of the deserialize() method.
  */

 virtual void serialize( netCDF::NcGroup & group ,
                         Block * block_reference ) const override {

  auto function_name = Block::get_method_name( function );
  ::SMSpp_di_unipi_it::serialize< std::string >
   ( group , "FunctionName" , netCDF::NcString() , function_name );

  if constexpr( std::is_base_of_v< Block , Caller > ) {
   const auto path = AbstractPath::build_path< Caller >( caller ,
                                                         block_reference );
   AbstractPath::serialize( path , group );
  }

  this->serialize( group , "From" , set_from );
  this->serialize( group , "To" , set_to );

  constexpr char template_parameter_types[3] =
   { get_id< SetFrom >() , get_id< SetTo >() , get_id< DataType >() };

  group.putAtt( "TemplateParameterTypes " , template_parameter_types );
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

 using Range = Block::Range;
 using Subset = Block::Subset;

/*--------------------------------------------------------------------------*/
/*--------------------------- PUBLIC METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
 /// constructs a SimpleDataMapping
 /** This function constructs a SimpleDataMapping whose template arguments are
  * given by the given \p types string. We consider a SimpleDataMapping with
  * three template parameters: SetFrom, SetTo, and DataType. The first one is
  * the type of the set that specifies the relevant entries of the data
  * vector. The second one, SetTo, is the type of the set that specifies which
  * part of the object's data that must be modified or is affected. The
  * DataType parameter indicates the numerical type of the data that the
  * method that modifies the object expects. The argument for each of these
  * template parameters is given by a character. The supported sets for
  * SetFrom and SetTo are Range and Subset. These sets are identified by the
  * characters 'R' and 'S', respectively. The DataType can be either int or
  * double, which are identified by the characters 'I' and 'D',
  * respectively. For instance, to create a SimpleDataMapping having SetFrom
  * as Range, SetTo as Subset, and DataType as double, one should pass the
  * "RSD" string to this function. If a non-supported type is given, an
  * exception is thrown.
  *
  * @param types A string indicating the template arguments of the
  *              SimpleDataMapping.
  *
  * @return A pointer to the SimpleDataMapping that was constructed.
  */

 static DataMapping * new_SimpleDataMapping( const std::string & types ) {
       if( types == "RRD" ) return new SimpleDataMapping<Range ,Range ,double>;
  else if( types == "RRI" ) return new SimpleDataMapping<Range ,Range ,int>;
  else if( types == "RSD" ) return new SimpleDataMapping<Range ,Subset,double>;
  else if( types == "RSI" ) return new SimpleDataMapping<Range ,Subset,int>;
  else if( types == "SRD" ) return new SimpleDataMapping<Subset,Range ,double>;
  else if( types == "SRI" ) return new SimpleDataMapping<Subset,Range ,int>;
  else if( types == "SSD" ) return new SimpleDataMapping<Subset,Subset,double>;
  else if( types == "SSI" ) return new SimpleDataMapping<Subset,Subset,int>;
  else
   throw std::invalid_argument( "new_SimpleDataMapping: invalid template "
                                "parameter types string: " + types );
 }

};  // end( class( SimpleDataMappingFactory ) )

/** @} end( group( DataMapping_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* DataMapping.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File DataMapping.h ---------------------------*/
/*--------------------------------------------------------------------------*/

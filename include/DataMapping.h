/*--------------------------------------------------------------------------*/
/*------------------------ File DataMapping.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the abstract class IDataMapping that defines an interface
 * for all class that implements a mechanism for mapping some data into the
 * data of some object. A concrete class called DataMapping is also defined
 * for some common kinds of data mapping.
 *
 * \version 0.1
 *
 * \date 03 - 12 - 2019
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
/*------------------------ CLASS IDataMapping ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// IDataMapping defines an interface for all types of data mappings.
/** IDataMapping defines an interface for all types of data mappings. The idea
 * of a data mapping is to map the data given by a vector of double into the
 * data of some object. It has three pure virtual functions. The first one is
 * set_data(), which has the following signature:
 *
 *    virtual void set_data( const std::vector< double > & data ,
 *                           c_ModParam issueMod = eModBlck ,
 *                           c_ModParam issueAMod = eModBlck ) const;
 *
 * The idea of this function is that the values of some data of an object can
 * be modified considering the given "data" parameter. The other two functions
 * are for serializing and deserializing an IDataMapping. Typically, an
 * IDataMapping could be used to set the data of a Block. In this case, a
 * pointer to that Block must be available. Pointers to a Block can be
 * serialized and deserialized considering its AbstractPath, which is relative
 * to some reference Block. For this reason, the serialize() and deserialize()
 * functions have a parameter which is a pointer to the reference Block:
 *
 *
 *  virtual void serialize( netCDF::NcGroup & group ,
 *                          Block * block_reference = nullptr ) const;
 *
 *  virtual void deserialize( const netCDF::NcGroup & group ,
 *                            Block * block_reference = nullptr );
 */

class IDataMapping {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */



/**@} ----------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING IDataMapping -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing IDataMapping
 *  @{ */

 /// constructor
 IDataMapping() = default;

/*--------------------------------------------------------------------------*/

 /// destructor
 virtual ~IDataMapping() {}

/*--------------------------------------------------------------------------*/

 virtual void deserialize( const netCDF::NcGroup & group ,
                           Block * block_reference = nullptr ) = 0;

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/**@} ----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE IDataMapping ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the IDataMapping
 *  @{ */

 /// sets the value of the associated data
 /** This function sets the value of the data associated with this
  * IDataMapping.
  *
  * @param data a (const) reference to the vector that stores the value of the
  *        data that will be set.
  *
  * @param issueMod indicates if and how the a "physical" Modification should
  *        be issued.
  *
  * @param issueMod indicates if and how the an "abstract" Modification should
  *        be issued.
  */

 virtual void set_data( const std::vector< double > & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const = 0;

/*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ,
                         Block * block_reference = nullptr ) const = 0;

/**@} ----------------------------------------------------------------------*/

};  // end( class( IDataMapping ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DataMapping -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/

/// DataMapping derives from IDataMapping
/**
 * DataMapping is a template class that derives from IDataMapping and has the
 * following template parameters:
 *
 * - SetFrom: This is the type of the set that selects the appropriate data
 *            from data vector. It must be either Block::Range or Block::Set.
 *
 * - SetTo: This is the type of the set that indicates which part of the data
 *          of the caller object must be set. It must be either Block::Range
 *          or Block::Set.
 *
 * - DataType: This is the type of the data that must be set in the caller
 *             object.
 *
 * - Caller: This is the type of the caller object. By default, it is Block.
 *
 * The function of the caller object that this DataMapping is associated with
 * is defined to have the following type:
 *
 *   Block::FunctionType< std::vector< DataType > , SetTo >
 */

template< class SetFrom = Block::Range , class SetTo = Block::Range ,
          class DataType = double , class Caller = Block >
class DataMapping : public IDataMapping {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 using F = Block::FunctionType< std::vector< DataType > , SetTo >;

/**@} ----------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING DataMapping -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing DataMapping
 *  @{ */

 /// constructor of DataMapping
 /** The constructor of DataMapping must receive the following parameters:
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
 DataMapping( const F * function = nullptr , Caller * caller = nullptr ,
              const SetFrom & set_from = {} , const SetTo & set_to = {} ) :
  function( function ), caller( caller ), set_from( set_from ),
  set_to( set_to ) { }

/*--------------------------------------------------------------------------*/

 /// destructor
 virtual ~DataMapping() {}

/*--------------------------------------------------------------------------*/

 /// deserialize a DataMapping from a netCDF::NcGroup
 /** Deserialize a DataMapping from a netCDF::NcGroup, with the following
  * format:
  *
  * - The variable "FunctionName" contains the name of the function.
  *
  * - The group "Path" contains the AbstractPath to the caller.
  *
  * - The group "SetFrom" contains the description of the set SetFrom.
  *
  * - The group "SetTo" contains the description of the set SetTo.
  *
  * If a set (SetFrom or SetTo) is a Block::Subset, then the corresponding
  * group has a netCDF dimension called "Size" and a one-dimensional variable
  * called "Subset" whose type is netCDF::NcUint64() and whose dimension is
  * given by "Size". This one-dimensional variable contains the Block::Subset.
  *
  * If a set (SetFrom or SetTo) is a Block::Range, then the corresponding
  * group has a netCDF dimension has two variables called "First" and "Second"
  * whose types are netCDF::NcUint64(). These variables contain the first and
  * the second element of a Range, respectively.
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
   const auto path_group = group.getGroup( "Path" );
   if( path_group.isNull() )
    throw std::invalid_argument( "DataMapping::deserialize: group named 'Path' "
                                 "containing the path to the Block must be "
                                 "present." );
   const auto path = AbstractPath::deserialize( path_group );
   caller = AbstractPath::get_element< Block >( path , block_reference );
  }

  this->deserialize( group , "SetFrom" , set_from , false );
  this->deserialize( group , "SetTo" , set_to , false );
 }

/**@} ----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE DataMapping -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the DataMapping
 *  @{ */

 virtual void set_data( const std::vector< double > & data ,
                         c_ModParam issueMod = eModBlck ,
                         c_ModParam issueAMod = eModBlck ) const override {
  auto sub_data = extract< DataType >( data, set_from );
  std::invoke( * function , caller , sub_data , set_to , issueMod , issueAMod );
 }

/*--------------------------------------------------------------------------*/

 /// serialize a DataMapping into a netCDF::NcGroup
 /** Serialize a DataMapping into a netCDF::NcGroup. The format is specified
  * in the comments of the deserialize() method.
  */

 virtual void serialize( netCDF::NcGroup & group ,
                         Block * block_reference ) const override {

  auto function_name = Block::get_method_name( function );
  ::SMSpp_di_unipi_it::serialize< std::string >
   ( group , "FunctionName" , netCDF::NcString() , function_name );

  if constexpr( std::is_base_of_v< Block , Caller > ) {
   auto path_group = group.addGroup( "Path" );
   const auto path = AbstractPath::build_path< Caller >( caller ,
                                                         block_reference );
   AbstractPath::serialize( path , path_group );
  }

  this->serialize( group , "SetFrom" , set_from );
  this->serialize( group , "SetTo" , set_to );
 }

/*--------------------------------------------------------------------------*/
/*----------- METHODS FOR READING THE DATA OF THE DataMapping --------------*/
/*--------------------------------------------------------------------------*/

 /// returns a pointer to the function that is associated with this DataMapping
 /** Returns a pointer to the function that is associated with this
  * DataMapping.
  */

 const F * get_function() const {
  return function;
 }

/*--------------------------------------------------------------------------*/

 /// returns a pointer to the caller of the function
 /** Returns a pointer to the caller of the function that is associated with
  * this DataMapping.
  */

 Caller * get_caller() const {
  return caller;
 }

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
 static std::vector< T > extract( const std::vector< T > & data ,
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

 bool deserialize( const netCDF::NcGroup & group ,
                   const std::string & group_name , Block::Range & range ,
                   bool optional = true ) {

  auto range_group = group.getGroup( group_name );
  if( range_group.isNull() ) {
   if( ! optional ) {
    throw( std::invalid_argument
           ( "DataMapping::deserialize(): group " + group_name +
             " is not present in group '" + group.getName() + "'." ) );
   }
   return false;
  }

  {
   auto ncVar = range_group.getVar( "First" );
   if( ncVar.isNull() ) {
    throw( std::invalid_argument
           ( "DataMapping::deserialize(): variable 'First' is not "
             "present in group '" + group_name + "'." ) );
   }
   ncVar.getVar( & range.first );
  }

  {
   auto ncVar = range_group.getVar( "Second" );
   if( ncVar.isNull() ) {
    throw( std::invalid_argument
           ( "deserialize(): variable 'Second' is not present in group '" +
             group_name + "'." ) );
   }
   ncVar.getVar( & range.second );
  }

  return true;
 }

/*--------------------------------------------------------------------------*/

 bool deserialize( const netCDF::NcGroup & group ,
                   const std::string & group_name , Block::Subset & subset ,
                   bool optional = true ) {

  auto subset_group = group.getGroup( group_name );
  if( subset_group.isNull() ) {
   if( ! optional ) {
    throw( std::invalid_argument
           ( "DataMapping::deserialize(): group " + group_name +
             " is not present in group '" + group.getName() + "'." ) );
   }
   return false;
  }

  auto dim = subset_group.getDim( "Size" );
  return ::SMSpp_di_unipi_it::deserialize( subset_group , "Subset" ,
                                           subset , optional );
 }

/*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ,
                         const std::string & group_name ,
                         const Block::Range & range ) const {
  auto range_group = group.addGroup( group_name );
  ::SMSpp_di_unipi_it::serialize( range_group , "First" ,
                                  netCDF::NcUint64() , range.first );
  ::SMSpp_di_unipi_it::serialize( range_group , "Second" ,
                                  netCDF::NcUint64() , range.second );
 }

 /*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ,
                         const std::string & group_name ,
                         const Block::Subset & subset ) const {
  auto range_group = group.addGroup( group_name );
  auto dim = group.addDim( "Size" , subset.size() );
  ::SMSpp_di_unipi_it::serialize( range_group , "Subset" ,
                                  netCDF::NcUint64() , dim , subset , false );
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

};  // end( class( DataMapping ) )

/** @} end( group( DataMapping_CLASSES ) ) ---------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* DataMapping.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File DataMapping.h ---------------------------*/
/*--------------------------------------------------------------------------*/

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
 * \date 10 - 12 - 2019
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
 * of a data mapping is to map the values given by a vector of double into the
 * data of some object. It has four pure virtual functions. The first two are
 * set_data(), which have the following signature:
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
  * @param issueMod indicates if and how the an "abstract" Modification should
  *        be issued.
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
  * @param issueMod indicates if and how the an "abstract" Modification should
  *        be issued.
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
 * SimpleDataMapping is a template class that derives from DataMapping and has
 * the following template parameters:
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
 * - Caller: This is the type of the caller object, which is the object that
 *           will invoke the function. By default, Caller is Block.
 *
 * The function of the caller object that this SimpleDataMapping is associated
 * with is defined to have the following type:
 *
 *   Block::FunctionType< std::vector< DataType > , SetTo >
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

 using F = Block::FunctionType< std::vector< DataType > , SetTo >;

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
  set_to( set_to ) { }

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
  *   "SecondFrom" which contain the limits of the closed-open interval
  *   defining the Range. If SetFrom is a Subset, then this set is specified
  *   by the dimension "SizeFrom" indicating the size of the set and the
  *   variable "SubsetFrom" containing the elements of the set.
  *
  * - All that is necessary to describe the SetTo set. Its representation is
  *   the same as that of the SetFrom set, except for the names of the
  *   variables, which should be "FirstTo" and "SecondTo" if SetTo is a Range,
  *   and "SubsetTo" if the set is a Subset (accompanied by the dimension
  *   "SizeTo").
  *
  * - The variable "TemplateParameterTypes", whose type is netCDF::NcString,
  *   is a string with three characters that indicate the types of the
  *   template parameters of this class. The first and second characters
  *   indicate the type of the SetFrom and SetTo template parameters. Each of
  *   these two first characters can be either 'R', indicating the set has
  *   type Block::Range, or 'S', indicating the set has type
  *   Block::Subset. The third character indicates the type of the data that
  *   this SimpleDataMapping sets. This character can be either 'I',
  *   indicating the type of the data is int, or 'D', indicating the type of
  *   the data is double.
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
  this->deserialize( group , "To" , set_to , false );
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
  std::invoke( * function , caller , sub_data , set_to , issueMod , issueAMod );
 }

/*--------------------------------------------------------------------------*/

 virtual void set_data( const Eigen::ArrayXd & data ,
                        c_ModParam issueMod = eModBlck ,
                        c_ModParam issueAMod = eModBlck ) const override {
  auto sub_data = extract< DataType >( data, set_from );
  std::invoke( * function , caller , sub_data , set_to , issueMod , issueAMod );
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

  ::SMSpp_di_unipi_it::serialize< std::string >
    ( group , "TemplateParameterTypes " , netCDF::NcString() ,
      template_parameter_types );
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
  auto dim = group.addDim( "Size" , subset.size() );
  ::SMSpp_di_unipi_it::serialize( group , "Subset" + suffix ,
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

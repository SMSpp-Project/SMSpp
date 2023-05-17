/*--------------------------------------------------------------------------*/
/*------------------------ File DataMapping.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the abstract class DataMapping that defines an interface
 * for all class that implements a mechanism for mapping some data into the
 * data of some object. A concrete class called SimpleDataMapping is also
 * defined for some common kinds of data mapping.
 *
 * \author Rafael Durbano Lobato \n
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
 * vector of double into the data of some object. It has three pure virtual
 * functions. The first one is set_data(), which has the following
 * signature:
 *
 *    virtual void set_data( std::vector< double >::const_iterator ,
 *                           c_ModParam issueMod = eModBlck ,
 *                           c_ModParam issueAMod = eModBlck ) const;
 *
 * The idea of this function is that the values of some data of an object can
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

/*--------------------------------------------------------------------------*/

 /// sets the value of the associated data
 /** This function sets the value of the data associated with this
  * DataMapping.
  *
  * @param data An iterator to the first element of the data.
  *
  * @param issueMod indicates if and how the a "physical" Modification should
  *        be issued.
  *
  * @param issueAMod indicates if and how the an "abstract" Modification
  *        should be issued.
  */

 virtual void set_data( std::vector< double >::const_iterator data ,
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

/** @} ---------------------------------------------------------------------*/

 };  // end( class( DataMapping ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SimpleDataMappingBase -----------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- GENERAL NOTES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// SimpleDataMappingBase derives from DataMapping
/** SimpleDataMappingBase is a class intended to be the base class for all
 * SimpleDataMapping. It provides (pure virtual) function for setting the
 * SetFrom and SetTo sets, the caller object, and the function associated
 * with the SimpleDataMapping. See SimpleDataMapping for details. */

class SimpleDataMappingBase : public DataMapping {

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected Types
 *  @{ */

 /// APnetCDF is a struct to store netCDF dimensions and variables of path
 /** This struct is used simply to store the netCDF dimensions and variables
  * used to represent an AbstractPath or a vector of AbstractPath.
  */
 struct SDMBnetCDF {
  netCDF::NcDim NumberDataMappings;
  netCDF::NcVar DataType;
  netCDF::NcVar Caller;
  netCDF::NcVar FunctionName;
  netCDF::NcVar SetSize;
  netCDF::NcVar SetElements;
  netCDF::NcGroup AbstractPath;
  AbstractPath::APnetCDF ap_netCDF;
 };

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected Static Fields
    @{ */

 /// Name of the netCDF dimension that stores the number of DataMappings
 inline static const std::string NumberDataMappings_name = "NumberDataMappings";

 /// Name of the netCDF variable that stores the (array with the) type(s) of
 /// the data to be set
 inline static const std::string DataType_name = "DataType";

 /// Name of the netCDF variable that stores the (array with the) type(s) of
 /// the caller object(s)
 inline static const std::string Caller_name = "Caller";

 /// Name of the netCDF variable that stores the (array with the) name(s) of
 /// the Function(s) as registered in the methods factory
 inline static const std::string FunctionName_name = "FunctionName";

 /// Name of the netCDF variable that stores the array with the sizes (or
 /// types) of the SetFrom and SetTo sets
 inline static const std::string SetSize_name = "SetSize";

 /// Name of the netCDF dimension of the SetSize variable
 inline static const std::string SetSize_dim_name = "SetSize_dim";

 /// Name of the netCDF variable that stores the (array with the) elements of
 /// the SetFrom and SetTo sets
 inline static const std::string SetElements_name = "SetElements";

 /// Name of the netCDF dimension of the SetElements variable
 inline static const std::string SetElements_dim_name = "SetElements_dim";

 /// Name of the netCDF group that stores the (vector of) AbstractPath
 inline static const std::string AbstractPath_name = "AbstractPath";

/** @} ---------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 using Index = Block::Index;

/** @} ---------------------------------------------------------------------*/
/*---------- CONSTRUCTING AND DESTRUCTING SimpleDataMappingBase ------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing SimpleDataMappingBase
 *  @{ */

 /// constructor
 SimpleDataMappingBase() = default;

/*--------------------------------------------------------------------------*/

 /// destructor
 virtual ~SimpleDataMappingBase() {}

/*--------------------------------------------------------------------------*/

 using DataMapping::deserialize;

/*--------------------------------------------------------------------------*/

 /// deserialize a SimpleDataMappingBase with a given index
 /** This function deserializes the SimpleDataMappingBase whose index is \p
  * index from the given netCDF dimensions and variables in \p
  * sdmb_netCDF. The variables in \p sdmb_netCDF may contain the description
  * of a single SimpleDataMappingBase or a vector of SimpleDataMappingBase. If
  * the dimension "NumberDataMappings" is not present, then \p sdmb_netCDF
  * contains the description of a single SimpleDataMappingBase. In this case,
  * \p index must be 0; otherwise, an exception is thrown. If the dimension
  * "NumberDataMappings" is present, then it indicates the number N of stored
  * SimpleDataMappingBase. The value for \p index must be between 0 and N - 1;
  * otherwise, an exception is thrown. Please refer to the comments of the
  * SimpleDataMapping::deserialize() function for a description of the format
  * of a SimpleDataMapping.
  *
  * @param sdmb_netCDF The struct containing the netCDF dimensions and
  *        variables describing the vector of SimpleDataMappingBase.
  *
  * @param index The index of the SimpleDataMapping to be deserialized from the
  *        vector of SimpleDataMappingBase.
  *
  * @param set_elements_start_index The index of the "SetElements" array at
  *        which the description of the elements of the SetFrom and SetTo sets
  *        of this SimpleDataMappingBase start. On return, it stores the index
  *        of the "SetElements" array at which the description of the elements
  *        of the SetFrom and SetTo sets of the next SimpleDataMappingBase
  *        (whose index is "index + 1") start.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller object from its AbstractPath.
  */

 virtual void deserialize( const SDMBnetCDF & sdmb_netCDF ,
                           Index index , Index & set_elements_start_index ,
                           Block * block_reference ) = 0;

/*--------------------------------------------------------------------------*/

 /// deserializes a vector of SimpleDataMappingBase
 /** This function deserializes a vector of SimpleDataMappingBase and returns
  * it. The format is specified in the comments of the static serialize()
  * method.
  *
  * @param group The NcGroup that contains the description of the vector of
  *              SimpleDataMappingBase to be deserialized.
  *
  * @param data_mappings The vector to which the pointers to the
  *        SimpleDataMappingBase will be added.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        obtaining the pointer to the caller together with its AbstractPath.
  */

 static void deserialize( const netCDF::NcGroup & group ,
   std::vector< std::unique_ptr< SimpleDataMappingBase > > & data_mappings ,
   Block * block_reference );

/** @} ----------------------------------------------------------------------*/
/*------ METHODS DESCRIBING THE BEHAVIOR OF THE SimpleDataMappingBase ------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the SimpleDataMappingBase
 *  @{ */

 using DataMapping::serialize;

/*--------------------------------------------------------------------------*/

 /// serialize a SimpleDataMappingBase with a given index
 /** This function serializes a SimpleDataMappingBase at index \p index of the
  * vector of SimpleDataMappingBase given by the netCDF dimensions and
  * variables in \p sdmb_netCDF. The variables in \p sdmb_netCDF may contain
  * the description of a single SimpleDataMappingBase or a vector of
  * SimpleDataMappingBase. If the dimension "NumberDataMappings" is not
  * present, then \p sdmb_netCDF contains the description of a single
  * SimpleDataMappingBase. In this case, \p index must be 0; otherwise, an
  * exception is thrown. If the dimension "NumberDataMappings" is present,
  * then it indicates the number N of stored SimpleDataMappingBase. The value
  * for \p index must be between 0 and N - 1; otherwise, an exception is
  * thrown. Please refer to the comments of the static
  * SimpleDataMappingBase::serialize() function for a description of the
  * format of a vector of SimpleDataMappingBase.
  *
  * @param sdmb_netCDF The struct containing the netCDF dimensions and
  *        variables describing the vector of SimpleDataMappingBase.
  *
  * @param index The index of the SimpleDataMappingbase to be serialized in
  *        the vector of SimpleDataMappingBase.
  *
  * @param set_elements_start_index The index of the "SetElements" array at
  *        which the description of the elements of the SetFrom and SetTo sets
  *        of this SimpleDataMappingBase start. On return, it stores the index
  *        of the "SetElements" array at which the description of the elements
  *        of the SetFrom and SetTo sets of the next SimpleDataMappingBase
  *        (whose index is "index + 1") start.
  *
  * @param block_reference The pointer to the reference Block that is used for
  *        constructing the AbstractPath to the caller object.
  */

 virtual void serialize( SDMBnetCDF & sdmb_netCDF ,
                         Index index , Index & set_elements_start_index ,
                         Index & path_start_index ,
                         Block * block_reference ) const = 0;

/*--------------------------------------------------------------------------*/

 /// serialize a vector of SimpleDataMappingBase from a netCDF::NcGroup
 /** Serialize a vector of SimpleDataMappingBase from a netCDF::NcGroup. A
  * vector of SimpleDataMappingBase is specified as follows.
  *
  * - The "NumberDataMappings" dimension indicates the number of
  *   SimpleDataMappingBase that is present in the vector of
  *   SimpleDataMappingBase. This dimension is optional. If it is not
  *   provided, then NumberDataMappings = 0 is assumed and every element in
  *   this group is ignored.
  *
  * - The "SetSize_dim" dimension has size 2*NumberDataMappings and is the
  *   dimension of the "SetSize" variable (see below). This dimension is
  *   optional.
  *
  * - The "SetElements_dim" dimension is the unlimited dimension of the
  *   "SetElements" variable (see below).
  *
  * - The one-dimensional variable "DataType" indexed over the
  *   "NumberDataMappings" dimension is an array of type netCDF::NcChar that
  *   specifies the type of the data that is associated with each
  *   SimpleDataMappingBase of the vector. This is the type of the data that
  *   can be set by the SimpleDataMappingBase (i.e., the DataType template
  *   parameter of SimpleDataMappingBase). This variable is optional. If it is
  *   not present, then the data type associated with each
  *   SimpleDataMappingBase in this vector is assumed to be double. If it is
  *   present then, for each i in {0, ..., NumberDataMappings-1}, DataType[ i
  *   ] is the type of the data associated with the i-th SimpleDataMappingBase
  *   and can be either 'I' or 'D', indicating that the type of the data is
  *   int or double, respectively.
  *
  * - The one-dimensional variable "SetSize" is an array of type
  *   netCDF::NcUint indexed over the "SetSize_dim" dimension and indicates
  *   the size of the sets that define each SimpleDataMappingBase (the
  *   "SetFrom" and "SetTo" sets). This variable is optional. If it is not
  *   present, then all sets are assumed to be Range. If it is present, then
  *   SetSize[ 2i + k ] is the size of the SetFrom set of the i-th
  *   SimpleDataMappingBase if k = 0 or the size of the SetTo set of the i-th
  *   SimpleDataMappingBase if k = 1. If SetSize[ j ] == 0, then the
  *   corresponding set is a Range. Otherwise, the corresponding set is a
  *   Subset of size SetSize[ j ].
  *
  * - The one-dimensional variable "SetElements", of type netCDF::NcUint, is
  *   an array containing the concatenation of the representations of the sets
  *   SetFrom and SetTo. A Subset is represented by a sequence of indices
  *   (which are the elements of the Subset); while a Range is represented by
  *   two indices a and b such that the Range set is given by the integers in
  *   the closed-open interval [a, b). If we let SetFrom_i and SetTo_i denote
  *   the representations of the SetFrom and SetTo sets of the i-th
  *   SimpleDataMappingBase, then "SetElements" is the array
  *
  *   ( SetFrom_0 , SetTo_0 , SetFrom_1 , SetTo_1 , ..., SetFrom_N, SetTo_N )
  *
  *   where N = NumberDataMappings - 1.
  *
  * - The one-dimensional variable "FunctionName" of type netCDF::NcString and
  *   indexed over "NumberDataMappings" contains the names of the functions
  *   associated with each SimpleDataMappingBase. FunctionName[ i ] gives the
  *   name of the function (as registered in the methods factory) associated
  *   with the i-th SimpleDataMappingBase.
  *
  * - A sub-group called "AbstractPath", containing a vector of AbstractPath
  *   with the paths to the Block. The i-th path in this vector of
  *   AbstractPath is the path to the Block associated with the i-th
  *   SimpleDataMappingBase.
  *
  * - The one-dimensional variable "Caller", of type netCDF::NcChar and
  *   indexed over "NumberDataMappings", containing the types of the caller
  *   objects associated with each SimpleDataMappingBase. Caller[ i ] gives
  *   the type of the caller object associated with the i-th
  *   SimpleDataMappingBase and can be either 'B', indicating that the caller
  *   is a Block, or 'F', indicating that the caller is a Function. This
  *   variable is optional. If it is not provided, then we assume that Caller[
  *   i ] = 'B' for each i in {0, ..., NumberDataMappings - 1}, that is, we
  *   assume that all callers are Block.
  *
  * @param group The netCDF::NcGroup from which to read the data.
  *
  * @param block_reference The pointer to the Block that will serve as the
  *        reference Block when constructing the AbstractPath to the caller
  *        objects.
  */

 static void serialize
 ( netCDF::NcGroup & group ,
   const std::vector< std::unique_ptr< SimpleDataMappingBase > > & data_mappings ,
   Block * block_reference ) {
  SDMBnetCDF sdmb_netCDF;
  pre_serialize( data_mappings , sdmb_netCDF , group );
  Index set_elements_start_index = 0;
  Index path_start_index = 0;
  for( Index i = 0 ; i < data_mappings.size() ; ++i ) {
   data_mappings[ i ]->serialize( sdmb_netCDF , i , set_elements_start_index ,
                                  path_start_index , block_reference );
  }
 }

/*--------------------------------------------------------------------------*/

 // Specializations for this method are defined below, outside the class.
 template< class T >
 static constexpr char get_id();

/** @} ---------------------------------------------------------------------*/
/*----------------------- PRIVATE PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*----------------------------- PRIVATE METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Methods
    @{ */

 static SDMBnetCDF pre_deserialize( const netCDF::NcGroup & group ) {

  SDMBnetCDF sdmb_netCDF;

  sdmb_netCDF.NumberDataMappings = group.getDim( NumberDataMappings_name );

  if( sdmb_netCDF.NumberDataMappings.isNull() ) {
   return( sdmb_netCDF );
  }

  sdmb_netCDF.DataType = group.getVar( DataType_name );
  sdmb_netCDF.Caller = group.getVar( Caller_name );
  sdmb_netCDF.FunctionName = group.getVar( FunctionName_name );
  sdmb_netCDF.SetSize = group.getVar( SetSize_name );
  sdmb_netCDF.SetElements = group.getVar( SetElements_name );
  sdmb_netCDF.AbstractPath = group.getGroup( AbstractPath_name );
  sdmb_netCDF.ap_netCDF = AbstractPath::pre_deserialize
   ( sdmb_netCDF.AbstractPath );

  auto num_data_mappings = sdmb_netCDF.NumberDataMappings.getSize();

  if( num_data_mappings > 0 ) {

   if( ! sdmb_netCDF.DataType.isNull() &&
       ( sdmb_netCDF.DataType.getDimCount() != 1 ||
         sdmb_netCDF.DataType.getDim( 0 ).getSize() != num_data_mappings ) )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                           "'" + DataType_name + "' array." ) );

   if( ! sdmb_netCDF.Caller.isNull() &&
       ( sdmb_netCDF.Caller.getDimCount() != 1 ||
         sdmb_netCDF.Caller.getDim( 0 ).getSize() != num_data_mappings ) )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                           "'" + Caller_name + "' array." ) );

   if( sdmb_netCDF.FunctionName.isNull() ||
       sdmb_netCDF.FunctionName.getDimCount() != 1 ||
       sdmb_netCDF.FunctionName.getDim( 0 ).getSize() != num_data_mappings )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                             "'" + FunctionName_name + "' array." ) );

   if( ! sdmb_netCDF.SetSize.isNull() &&
       ( sdmb_netCDF.SetSize.getDimCount() != 1 ||
         sdmb_netCDF.SetSize.getDim( 0 ).getSize() != 2 * num_data_mappings ) )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                             "'" + SetSize_name + "' array." ) );

   if( sdmb_netCDF.SetElements.isNull() ||
       sdmb_netCDF.SetElements.getDimCount() != 1 )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                             "'" + SetElements_name + "' array." ) );

   if( sdmb_netCDF.ap_netCDF.PathDim.isNull() ||
       sdmb_netCDF.ap_netCDF.PathDim.getSize() != num_data_mappings )
    throw( std::logic_error( "SimpleDataMappingBase::pre_deserialize: invalid "
                             "number of AbstractPath." ) );
  }

  return( sdmb_netCDF );
 }

/*--------------------------------------------------------------------------*/

 static void pre_serialize
 ( const std::vector< std::unique_ptr< SimpleDataMappingBase > > & data_mappings ,
   SDMBnetCDF & sdmb_netCDF , netCDF::NcGroup & group ) {

  auto num_data_mappings = data_mappings.size();
  sdmb_netCDF.NumberDataMappings = group.addDim( NumberDataMappings_name ,
                                                 num_data_mappings );

  sdmb_netCDF.DataType = group.addVar( DataType_name , netCDF::NcChar() ,
                                       sdmb_netCDF.NumberDataMappings );

  sdmb_netCDF.Caller = group.addVar( Caller_name , netCDF::NcChar() ,
                                     sdmb_netCDF.NumberDataMappings );

  sdmb_netCDF.FunctionName = group.addVar( FunctionName_name ,
                                           netCDF::NcString() ,
                                           sdmb_netCDF.NumberDataMappings );

  auto set_size_dim = group.addDim( SetSize_dim_name , 2 * num_data_mappings );
  sdmb_netCDF.SetSize = group.addVar( SetSize_name , netCDF::NcUint() ,
                                      set_size_dim );

  auto set_elements_dim = group.addDim( SetElements_dim_name );
  sdmb_netCDF.SetElements = group.addVar( SetElements_name ,
                                          netCDF::NcUint() ,
                                          set_elements_dim );

  sdmb_netCDF.AbstractPath = group.addGroup( AbstractPath_name );

  AbstractPath::pre_serialize( num_data_mappings , sdmb_netCDF.ap_netCDF ,
                               sdmb_netCDF.AbstractPath );
 }

/*--------------------------------------------------------------------------*/

 static void get_sets_type( const netCDF::NcVar & set_size_var ,
                            char & set_from_type , char & set_to_type ,
                            const Index index ) {
  std::vector< Index > set_size( 2 );
  set_size_var.getVar( { 2 * index } , { 2 } , set_size.data() );
  set_from_type = set_size[ 0 ] > 0 ? 'S' : 'R';
  set_to_type   = set_size[ 1 ] > 0 ? 'S' : 'R';
 }

/** @} ---------------------------------------------------------------------*/

 };  // end( class( SimpleDataMappingBase ) )

/*--------------------------------------------------------------------------*/

 // Apparently, GCC wants template specializations
 // outside the class declaration.
template<>
constexpr char SimpleDataMappingBase::get_id< Block::Range >() { return( 'R' ); }

template<>
constexpr char SimpleDataMappingBase::get_id< Block::Subset >() { return( 'S'); }

template<>
constexpr char SimpleDataMappingBase::get_id< double >() { return( 'D' ); }

template<>
constexpr char SimpleDataMappingBase::get_id< int >() { return( 'I' ); }


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
 * vector refers to the vector that is given as input to the set_data() method
 * (by means of an iterator to the beginning of this vector). This is the
 * vector containing all the data that can be used by the
 * SimpleDataMapping. The small vector is a vector formed by a subset of the
 * elements of the large vector. This is the vector that will effectively be
 * used to perform some computation. This computation is typically the task of
 * changing the data of some object based on this small vector. There is a
 * mapping defined by the SetFrom set that specifies which elements of the
 * large vector are used to compose the small vector. This SetFrom set
 * contains the indices of these elements in the large vector. The small
 * vector is the one that will typically impact the data of some object. The
 * SetTo set can be used to specify which part of this data is
 * affected. Actually, SetFrom and SetTo are ordered multisets, but we will
 * refer to them as sets for simplicity.
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
 * Usually, the SetFrom and the SetTo sets will have the same cardinality, so
 * that the i-th element of the SetFrom set will be associated with the i-th
 * element of the SetTo set. However, the SetFrom set is also allowed to be
 * smaller than the SetTo set. In this case, the cardinality of the SetTo set
 * must be a positive multiple of the cardinality of the SetFrom set and the
 * i-th element of the SetTo set will be associated with the element of the
 * SetFrom set located at position floor(i/r), where r is the ratio of the
 * cardinalities of the SetTo and SetFrom sets.
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
 *                      std::vector< double >::const_iterator capacities ,
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
                         SetTo > >;

/** @} ---------------------------------------------------------------------*/
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

 void deserialize( const netCDF::NcGroup & group ,
                   Block * block_reference ) override {

  // FunctionName

  auto FunctionName_var = group.getVar( FunctionName_name );
  if( FunctionName_var.isNull() ) {
   throw( std::logic_error( "SimpleDataMapping::deserialize: variable '" +
                            FunctionName_name + "' is not present." ) );
  }

  // TODO The following implementation should change when netCDF provides a
  // better C++ interface.
  char * fname = nullptr;
  FunctionName_var.getVar( & fname );
  std::string function_name( fname );
  free( fname );

  function = Block::get_method< F >( function_name );

  // AbstractPath

  {
   auto path_group = group.getGroup( AbstractPath_name );
   if( path_group.isNull() )
    std::logic_error( "SimpleDataMapping::deserialize: group '" +
                      AbstractPath_name + "' was not found." );

   AbstractPath path( path_group );

   caller = path.get_element< Caller >( block_reference );
  }

  // SetFrom and SetTo

  {
   std::vector< Index > set_size = { 0 , 0 };
   if( ::SMSpp_di_unipi_it::deserialize( group , SetSize_name ,
                                         set_size , true ) ) {
    if( set_size.size() != 2 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: array '" +
                              SetSize_name + "' must have size 2." ) );
   }

   std::vector< Index > set_elements;
   ::SMSpp_di_unipi_it::deserialize( group , SetElements_name ,
                                     set_elements , false );

   Index next_index = 0;
   if constexpr( std::is_same_v< SetFrom , Range > ) {
    if( set_elements.size() < 3 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'" + SetElements_name + "' array." ) );
    set_from = Range( set_elements[ 0 ] , set_elements[ 1 ] );
    next_index = 2;
   }
   else {
    if( set_elements.size() < set_size[ 0 ] + 1 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'" + SetElements_name + "' array." ) );
    set_from.resize( set_size[ 0 ] );
    for( Index i = 0; i < set_size[ 0 ]; ++i )
     set_from[ i ] = set_elements[ i ];
    next_index = set_size[ 0 ];
   }

   ordered = true;
   if constexpr( std::is_same_v< SetTo , Range > ) {
    if( set_elements.size() < next_index + 2 )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'" + SetElements_name + "' array." ) );
    set_to = Range( set_elements[ next_index ] , set_elements[ next_index + 1 ] );
   }
   else {
    if( set_elements.size() < next_index + set_size[ 1 ] )
     throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                              "'" + SetElements_name + "' array." ) );
    set_to.resize( set_size[ 1 ] );
    for( Index i = 0; i < set_size[ 1 ]; ++i )
     set_to[ i ] = set_elements[ next_index + i ];
    ordered = std::is_sorted( std::begin( set_to ), std::end( set_to ) );
   }
  }

  if( cardinality( set_from ) != 0 ) {
   if( cardinality( set_to ) < cardinality( set_from ) ||
       cardinality( set_to ) % cardinality( set_from ) != 0 ) {
    throw( std::logic_error( "SimpleDataMapping::deserialize: the cardinality "
                             "of 'SetTo' must be a positive multiple of the "
                             "cardinality of 'SetFrom'." ) );
   }
  }
 }

/*--------------------------------------------------------------------------*/

 void deserialize( const SDMBnetCDF & sdmb_netCDF , Index index ,
                   Index & set_elements_start_index ,
                   Block * block_reference ) override {

  if( ! sdmb_netCDF.NumberDataMappings.isNull() ) {
   // a vector of SimpleDataMappingBase
   if( index >= sdmb_netCDF.NumberDataMappings.getSize() )
    throw( std::invalid_argument( "SimpleDataMapping::deserialize: Invalid "
                                  "index: " + std::to_string( index ) ) );
  }
  else if( index != 0 ) {
   // There is only one SimpleDataMapping. So the index must be 0.
   throw( std::invalid_argument( "SimpleDataMapping::deserialize: Invalid "
                                 "index: " + std::to_string( index ) ) );
  }

  // FunctionName

  // TODO The following implementation should change when netCDF provides a
  // better C++ interface.
  char * fname = nullptr;
  sdmb_netCDF.FunctionName.getVar( { index } , { 1 } , & fname );
  std::string function_name( fname );
  free( fname );

  function = Block::get_method< F >( function_name );

  // AbstractPath

  AbstractPath path( index , sdmb_netCDF.ap_netCDF );
  caller = path.get_element< Caller >( block_reference );

  // SetFrom and SetTo

  auto set_elements_size = sdmb_netCDF.SetElements.getDim( 0 ).getSize();

  Index next_index = set_elements_start_index;

  std::vector< Index > set_size = { 0 , 0 };
  if( ! sdmb_netCDF.SetSize.isNull() )
   sdmb_netCDF.SetSize.getVar( { 2 * index } , { 2 } , set_size.data() );

  if constexpr( std::is_same_v< SetFrom , Range > ) {
   if( set_elements_size < next_index + 3 )
    throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                             "'" + SetElements_name + "' array." ) );
   std::vector< Index > set_from_elements( 2 );
   sdmb_netCDF.SetElements.getVar( { next_index } , { 2 } ,
                                   set_from_elements.data() );
   set_from = Range( set_from_elements[ 0 ] , set_from_elements[ 1 ] );
   next_index += 2;
  }
  else {
   if( set_elements_size < set_size[ 0 ] + 1 )
    throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                             "'" + SetElements_name + "' array." ) );
   set_from.resize( set_size[ 0 ] );
   sdmb_netCDF.SetElements.getVar( { next_index } , { set_from.size() } ,
                                   set_from.data() );
   next_index += set_from.size();
  }

  ordered = true;
  if constexpr( std::is_same_v< SetTo , Range > ) {
   if( set_elements_size < next_index + 2 )
    throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                             "'" + SetElements_name + "' array." ) );
   std::vector< Index > set_to_elements( 2 );
   sdmb_netCDF.SetElements.getVar( { next_index } , { 2 } ,
                                   set_to_elements.data() );
   set_to = Range( set_to_elements[ 0 ] , set_to_elements[ 1 ] );
   next_index += 2;
  }
  else {
   if( set_elements_size < next_index + set_size[ 1 ] )
    throw( std::logic_error( "SimpleDataMapping::deserialize: invalid "
                             "'" + SetElements_name + "' array." ) );
   set_to.resize( set_size[ 1 ] );
   sdmb_netCDF.SetElements.getVar( { next_index } , { set_to.size() } ,
                                   set_to.data() );
   ordered = std::is_sorted( std::begin( set_to ), std::end( set_to ) );
   next_index += set_to.size();
  }

  if( cardinality( set_from ) != 0 ) {
   if( cardinality( set_to ) < cardinality( set_from ) ||
       cardinality( set_to ) % cardinality( set_from ) != 0 ) {
    throw( std::logic_error( "SimpleDataMapping::deserialize: the cardinality "
                             "of 'SetTo' must be a positive multiple of the "
                             "cardinality of 'SetFrom'." ) );
   }
  }

  set_elements_start_index = next_index;
 }

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE SimpleDataMapping --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the SimpleDataMapping
 *  @{ */

 void set_data( std::vector< double >::const_iterator data ,
                c_ModParam issueMod = eModBlck ,
                c_ModParam issueAMod = eModBlck ) const override {

  std::vector< DataType > sub_data;
  decltype( sub_data.cbegin() ) begin;

  if( ( std::is_same_v< SetFrom , Subset > ) ||
      ( cardinality( set_to ) > cardinality( set_from ) ) ) {

   /* Subset usually contains a non-range set, so we cannot pass an iterator
    * to the given data forward to the function. Also, when the cardinality of
    * the SetFrom set is smaller than that of the SetTo set, we have to expand
    * the given data. In both cases, we need to construct a new vector to
    * accomodate the data to be passed to the function. */

   assert( empty( set_from ) ||
           cardinality( set_to ) % cardinality( set_from ) == 0 );

   sub_data = extract< DataType >( data , set_from );
   expand( sub_data , cardinality( set_to ) );
   begin = sub_data.cbegin();
  }
  else if constexpr( std::is_same_v< SetFrom , Range > ) {
   if constexpr( std::is_same_v< DataType , double > )
    begin = std::next( data , set_from.first );
   else {
    // Convert the data type.
    sub_data.assign( std::next( data , set_from.first ) ,
                     std::next( data , set_from.second ) );
    begin = sub_data.cbegin();
   }
  }

  if constexpr( std::is_same_v< SetTo , Subset > )
   std::invoke( * function , caller , begin , Subset( set_to ) ,
                ordered , issueMod , issueAMod );
  else
   std::invoke( * function , caller , begin , set_to , issueMod , issueAMod );
 }

/*--------------------------------------------------------------------------*/

 void serialize( SDMBnetCDF & sdmb_netCDF , Index index ,
                 Index & set_elements_start_index , Index & path_start_index ,
                 Block * block_reference ) const override {
  // FunctionName

  auto function_name = Block::get_method_name( function );
  sdmb_netCDF.FunctionName.putVar( { index } , function_name );

  // AbstractPath

  AbstractPath path;
  if constexpr( std::is_base_of_v< Function , Caller > ) {
   path.build< Function >( caller , block_reference );
  }
  else {
   path.build< Caller >( caller , block_reference );
  }

  sdmb_netCDF.ap_netCDF.PathStart.putVar( { index } , path_start_index );
  path.serialize( index , sdmb_netCDF.ap_netCDF );
  path_start_index += path.length();

  // SetFrom and SetTo

  Index next_index = set_elements_start_index;

  std::vector< Index > set_size( 2 );

  if constexpr( std::is_same_v< SetFrom , Range > ) {
   sdmb_netCDF.SetSize.putVar( { 2 * index } , 0 );
   sdmb_netCDF.SetElements.putVar( { next_index } , set_from.first );
   sdmb_netCDF.SetElements.putVar( { next_index + 1 } , set_from.second );
   next_index += 2;
  }
  else {
   sdmb_netCDF.SetSize.putVar( { 2 * index } ,
                               (const unsigned long long) set_from.size() );
   sdmb_netCDF.SetElements.putVar( { next_index } , { set_from.size() } ,
                                   set_from.data() );
   next_index += set_from.size();
  }

  if constexpr( std::is_same_v< SetTo , Range > ) {
   sdmb_netCDF.SetSize.putVar( { 2 * index + 1 } , 0 );
   sdmb_netCDF.SetElements.putVar( { next_index } , set_to.first );
   sdmb_netCDF.SetElements.putVar( { next_index + 1 } , set_to.second );
   next_index += 2;
  }
  else {
   sdmb_netCDF.SetSize.putVar( { 2 * index + 1 } ,
                               (const unsigned long long) set_to.size() );
   sdmb_netCDF.SetElements.putVar( { next_index } , { set_to.size() } ,
                                   set_to.data() );
   next_index += set_to.size();
  }

  set_elements_start_index = next_index;

  // DataType

  auto data_type = get_id< DataType >();
  sdmb_netCDF.DataType.putVar( { index } , & data_type );

  // Caller type

  char caller_type;
  if constexpr( std::is_base_of_v< Function , Caller > )
   caller_type = 'F';
  else
   caller_type = 'B';

  sdmb_netCDF.Caller.putVar( { index } , & caller_type );
 }

/*--------------------------------------------------------------------------*/

 /// serialize a SimpleDataMapping into a netCDF::NcGroup
 /** Serialize a SimpleDataMapping into a netCDF::NcGroup with the following
  * format:
  *
  * - The "SetSize_dim" dimension, which contains the size of the "SetSize"
  *   variable (see below). This dimension is optional.
  *
  * - The one-dimensional variable "SetSize", an array of type
  *   netCDF::NcUint with two elements indicating the sizes (or types) of
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
  * - The "SetElements_dim" dimension, containing the size of the
  *   "SetElements" variable (see below).
  *
  * - The one-dimensional variable "SetElements", of type netCDF::NcUint,
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

 void serialize( netCDF::NcGroup & group ,
                 Block * block_reference ) const override {

  // FunctionName

  auto function_name = Block::get_method_name( function );
  ::SMSpp_di_unipi_it::serialize< std::string >
   ( group , FunctionName_name , netCDF::NcString() , function_name );

  // AbstractPath

  if constexpr( std::is_base_of_v< Function , Caller > ) {
   AbstractPath path;
   path.build< Function >( caller , block_reference );
   auto path_group = group.addGroup( AbstractPath_name );
   path.serialize( path_group );
  }
  else {
   AbstractPath path;
   path.build< Caller >( caller , block_reference );
   auto path_group = group.addGroup( AbstractPath_name );
   path.serialize( path_group );
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

  auto SetSize_dim = group.addDim( SetSize_dim_name , set_size.size() );

  ::SMSpp_di_unipi_it::serialize( group , SetSize_name , netCDF::NcUint() ,
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

  auto SetElements_dim = group.addDim( SetElements_dim_name ,
                                       set_elements.size() );

  ::SMSpp_di_unipi_it::serialize( group , SetElements_name ,
                                  netCDF::NcUint() , SetElements_dim ,
                                  set_elements , false );

  // DataType

  ::SMSpp_di_unipi_it::serialize( group , DataType_name , netCDF::NcChar() ,
                                  get_id< DataType >() );

  // Caller type

  char caller_type = 'B';
  if constexpr( std::is_base_of_v< Function , Caller > )
   caller_type = 'F';

  ::SMSpp_di_unipi_it::serialize( group , Caller_name , netCDF::NcChar() ,
                                  caller_type );
 }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Methods
 *  @{ */

 template< class S >
 static void expand( std::vector< S > & data ,
                     const typename std::vector< S >::size_type size ) {
  if( data.size() >= size )
   return;

  assert( size % data.size() == 0 );

  if( data.size() == 1 ) {
   data.resize( size , data[ 0 ] );
   return;
  }

  auto original_data = data;
  data.resize( size );

  auto subvector_size = data.size() / original_data.size();

  for( typename std::vector< S >::size_type i = 0 ; i < original_data.size() ;
       ++i ) {
   std::fill_n( data.begin() + i * subvector_size , subvector_size ,
                original_data[ i ] );
  }
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract
 ( const typename std::vector< T >::const_iterator & data ,
   const Block::Range & range ) {
  if( range.first >= range.second )
   return {};
  return( std::vector< S >( data + range.first , data + range.second ) );
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract_from_ordered_subset
 ( const typename std::vector< T >::const_iterator & data ,
   const Block::Subset & subset ) {
  std::size_t size = subset.size();
  std::vector< S > output( size );
  auto it = data;
  Block::Subset::size_type previous = 0;
  for( std::size_t i = 0 ; i < size ; ++i ) {
   it = std::next( it , subset[ i ] - previous );
   output[ i ] = *it;
   previous = subset[ i ];
  }
  return( output );
 }

/*--------------------------------------------------------------------------*/

 template< class S = double , class T = double >
 static std::vector< S > extract
 ( const typename std::vector< T >::const_iterator & data ,
   const Block::Subset & subset, bool subset_is_ordered = false ) {
  if( subset_is_ordered )
   return( extract_from_ordered_subset< S , T >( data , subset ) );
  else {
   auto ordered_subset = Block::Subset( subset );
   std::sort( ordered_subset.begin() , ordered_subset.end() );
   return( extract_from_ordered_subset< S , T >( data , ordered_subset ) );
  }
 }

/*--------------------------------------------------------------------------*/

 static Index cardinality( const Range & range ) {
  if( range.second > range.first )
   return( range.second - range.first );
  return( 0 );
 }

/*--------------------------------------------------------------------------*/

 static Index cardinality( const Subset & subset ) {
  return( subset.size() );
 }

/*--------------------------------------------------------------------------*/

 template< class T >
 static bool empty( const T & t ) {
  return( cardinality( t ) == 0 );
 }

/** @} ---------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Private Fields
 *  @{ */

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

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

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
 ( const std::string & types );

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
                                   Block * block_reference );

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 static void get_sets_type( const netCDF::NcGroup & group ,
                            char & set_from_type , char & set_to_type ) {

  std::vector< Index > set_size = { 0 , 0 };
  if( ::SMSpp_di_unipi_it::deserialize( group , "SetSize" ,
                                        set_size , true ) ) {
   if( set_size.size() != 2 )
    throw( std::logic_error( "SimpleDataMappingFactory::get_sets_type: array "
                             "'SetSize' must have size 2." ) );
  }

  set_from_type = set_size[ 0 ] > 0 ?
   SimpleDataMappingBase::get_id< Block::Subset >() :
   SimpleDataMappingBase::get_id< Block::Range >();

  set_to_type   = set_size[ 1 ] > 0 ?
   SimpleDataMappingBase::get_id< Block::Subset >() :
   SimpleDataMappingBase::get_id< Block::Range >();
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

/*--------------------------------------------------------------------------*/
/*------------------------ File RBlockConfig.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for classes derived from BlockConfig, which are intended to
 * offer support to configure not only a Block but also its sub-Block and
 * "indirect sub-Block". Three main classes are defined:
 *
 * - RBlockConfig : BlockConfig ("recursive" BlockConfig), which also
 *   configures (potentially) all sub-Block (recursively) of the given Block;
 *
 * - CBlockConfig : BlockConfig ("Constraint" BlockConfig), which also
 *   configures (potentially) all Constraint of the given Block;
 *
 * - OBlockConfig : BlockConfig ("Objective" BlockConfig), which also
 *   configures the Objective of the given Block.
 *
 * The three classes above are combined to produce classes that are useful in
 * more general situations:
 *
 * - ORBlockConfig : RBlockConfig, which also configures (potentially) all
 *   sub-Block (recursively) and the Objective of the given Block;
 *
 * - CRBlockConfig : RBlockConfig, which also configures (potentially) all
 *   sub-Block (recursively) and (potentially) all Constraint of the given
 *   Block;
 *
 * - OCBlockConfig : CBlockConfig, which also configures (potentially) all
 *   Constraint and the Objective of the given Block;
 *
 * - OCRBlockConfig : CRBlockConfig, which also configures (potentially) all
 *   sub-Block (recursively) and (potentially) all Constraint and the
 *   Objective of the given Block.
 *
 * All these BlockConfig support the notion of a "cleared" Configuration (see
 * Configuration::clear()). When the clear() method is invoked, any pointers
 * to a sub-Configuration that the :BlockConfig may have (for instance,
 * pointers to the BlockConfig of the sub-Block; pointers to ComputeConfig of
 * Constraint or Objective) are deleted. The value of the BlockConfig::f_diff
 * field and the structure of the :BlockConfig are preserved. The structures
 * that may be preserved are that of a :BlockConfig that deals with sub-Block
 * (namely, RBlockConfig, ORBlockConfig, CRBlockConfig, and OCRBlockConfig;
 * shortly referred as *R*BlockConfig) or Constraint (namely, CBlockConfig,
 * CRBlockConfig, OCBlockConfig, and OCRBlockConfig; shortly referred as
 * *C*BlockConfig).
 *
 * A Block can have a huge number of Constraint, but usually very few
 * Constraint (if any) require a Configuration. Therefore, a *C*BlockConfig
 * has a very sparse structure: it handles only the Constraint that require a
 * Configuration. When a *C*BlockConfig is constructed out of a Block (see
 * CBlockConfig::get()), it may be necessary to scan all Constraint of that
 * Block in order to determine which ones require a Configuration (i.e., the
 * ones that have a non-default set of parameters). This operation can be
 * potentially costly. If the Constraint that need a Configuration are known
 * in advance, the scanning operation can be avoided. This is where a
 * "cleared" *C*BlockConfig shows its usefulness. The structure of a
 * *C*BlockConfig is formed by the list of (identification to the) Constraint
 * that require a Configuration (see CBlockConfig::v_Constraint_id). Whenever
 * a *C*BlockConfig is constructed out of a Block and
 * CBlockConfig::v_Constraint_id is non-empty, only the Constraint indicated
 * by v_Constraint_id are considered and no scan is performed.
 *
 * The reasoning behind a "cleared" *R*BlockConfig is analogous. The structure
 * of a *R*BlockConfig is formed by the list of (identification to the)
 * sub-Block that require a Configuration. A sub-Block can be identified
 * either by its name or by its index in the list of sub-Blocks of its father
 * Block. Whenever a *R*BlockConfig is constructed out of a Block and
 * RBlockConfig::v_sub_Block_id is non-empty, only the sub-Block indicated by
 * v_sub_Block_id are considered.
 *
 * \version 0.33
 *
 * \date 03 - 08 - 2020
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

 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __RBlockConfig
#define __RBlockConfig
                /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

#include "netcdf"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Constraint;          // forward definition of Constraint

 class Objective;           // forward definition of Objective

 class BlockConfig;         // forward definition of BlockConfig

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RBlockConfig_CLASSES Classes in RBlockConfig.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the sub-Block of a Block
/** The RBlockConfig class ("recursive" BlockConfig) derives from BlockConfig
 * and offers support for configuring also the sub-Block of a Block
 * (recursively). The RBlockConfig contains the following fields:
 *
 * - a vector of pointers to BlockConfig for (all or some of) the sub-Block of
 *   the Block.
 *
 * - a vector associating each BlockConfig to a sub-Block of the Block. */

class RBlockConfig : public BlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING RBlockConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing RBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty RBlockConfig

 RBlockConfig( bool diff = true ) : BlockConfig( diff ) {}

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig out of the given netCDF \p group
 /** It constructs a RBlockConfig out of the given netCDF \p group.  Please
  * refer to the deserialize() method for the format of the netCDF::NcGroup of
  * a RBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        RBlockConfig. */

 RBlockConfig( netCDF::NcGroup & group ) : BlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig out of an istream
 /** It constructs a RBlockConfig out of the given istream \p input.
  * Please refer to the load() method for the format of a RBlockConfig.
  *
  * @param input The istream containing the description of the
  *        RBlockConfig. */

 RBlockConfig( std::istream &input ) : BlockConfig() { load( input ); }

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig for the given Block
 /** It constructs a RBlockConfig for the given \p block. It creates an empty
  * RBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which a RBlockConfig will be
  *        constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 RBlockConfig( Block * block , bool diff = false ) : BlockConfig( diff )
 { get( block ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 RBlockConfig( const RBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a RBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should contain the following:
  *
  * - A description of a BlockConfig object for the Block, as described in
  *   BlockConfig::deserialize().
  *
  * - The dimension "n_sub_Block" containing the number of BlockConfig
  *   descriptions for the sub-Block of the current Block; this dimension is
  *   optional; if it is not provided, then n_sub_Block = 0 is assumed.
  *
  * - With n being the size of n_sub_Block, n groups, with name
  *   "sub-BlockConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a BlockConfig for one of the sub-Block of the current
  *   :Block. Each of these groups is optional. If a group is absent then the
  *   pointer to the BlockConfig for the corresponding sub-Block is
  *   considered to be a nullptr (default configuration).
  *
  * - With n being the size of n_sub_Block, a one-dimensional variable with
  *   name "sub-Block-id", of size n and type netCDF::NcString, containing
  *   the identification of the sub-Block such that "sub-BlockConfig_<i>"
  *   contains the BlockConfig for the sub-Block whose identification is
  *   "sub-Block-id[ i ]" for all i = 0, ..., n - 1. The identification of
  *   the sub-Block can be either its name (see Block::name()) or its index in
  *   the list of sub-Block of its father Block. This variable is optional. If
  *   it is not provided, then the i-th BlockConfig is associated with the
  *   i-th sub-Block of the Block for all i = 0, ..., n - 1 (i.e., i is taken
  *   as the index of the sub-Block and "sub-Block-id[ i ]" is assumed to be
  *   "i").
  *
  *       IF THE NAME OF THE Block IS USED AS ITS IDENTIFICATION, THEN
  *       THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all sub-BlockConfig
 virtual ~RBlockConfig()
 {
  for( auto sBC : v_sub_BlockConfig )
   delete sBC;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the RBlockConfig of the given Block
 /** This method gets information about the current set of parameters of the
  * given Block (and its sub-Block, recursively) and stores in this
  * RBlockConfig. This information consists of that supported by the
  * BlockConfig (see BlockConfig::get()) plus the BlockConfig of each
  * sub-Block of the given Block. Notice that any Configuration that this
  * RBlockConfig may have is deleted and the Configuration of the BlockConfig
  * of the given Block is cloned into the Configuration of this RBlockConfig.
  *
  * If #v_sub_Block_id is not empty then its content is preserved and only the
  * BlockConfig associated with the sub-Block (of the given \p block)
  * specified by #v_sub_Block_id are retrieved. If #v_sub_Block_id is empty
  * then the BlockConfig of every sub-Block in the given \p block is
  * retrieved. In this case, for each sub-Block of the given \p block, its
  * BlockConfig is stored in this RBlockConfig if it has a non-default set of
  * parameters.
  *
  * If #v_sub_Block_id is not empty but one wants all sub-Block to be
  * inspected (for instance, one is not sure that every sub-Block that is not
  * specified in #v_sub_Block_id has a default set of parameters), then
  * #v_sub_Block_id must be cleared before this method is called.
  *
  * @param block A pointer to the Block whose RBlockConfig must be filled.
  */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE RBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the RBlockConfig
 *  @{ */

 /// add a BlockConfig for a sub-Block
 /** This function adds the pointer to the BlockConfig of the sub-Block with
  * the given \p index.
  *
  * @param config A pointer to a BlockConfig.
  *
  * @param index The index of the sub-Block whose BlockConfig is being
  *        added. */

 void add_sub_BlockConfig( BlockConfig * config , Block::Index index ) {
  v_sub_BlockConfig.push_back( config );
  v_sub_Block_id.push_back( std::to_string( index ) );
  }

/*--------------------------------------------------------------------------*/

 /// add a BlockConfig for a sub-Block
 /** This function adds the pointer to the BlockConfig of the sub-Block with
  * the given \p name.
  *
  * @param config A pointer to a BlockConfig.
  *
  * @param index The name of the sub-Block whose BlockConfig is being
  *        added. */

 void add_sub_BlockConfig( BlockConfig * config , std::string && name ) {
  v_sub_BlockConfig.push_back( config );
  v_sub_Block_id.push_back( std::move( name ) );
  }

/*--------------------------------------------------------------------------*/

 /// remove a BlockConfig for a sub-Block
 /** This function removes the BlockConfig at the given \p index. Notice that
  * \p index is not the index of the sub-Block, but the index of the
  * BlockConfig being handled by this RBlockConfig.
  *
  * @param index The index of the BlockConfig to be removed. */

 void remove_sub_BlockConfig( Block::Index index ) {
  if( index >= v_sub_BlockConfig.size() )
   throw ( std::invalid_argument( "RBlockConfig::remove_sub_BlockConfig: "
                                  "invalid index: " +
                                  std::to_string( index ) + "." ) );
  v_sub_BlockConfig.erase( std::begin( v_sub_BlockConfig ) + index );
  v_sub_Block_id.erase( std::begin( v_sub_Block_id ) + index );
  }

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the RBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its sub-Block (recursively)
 /** Method for configuring the given Block and all its sub-Block,
  * recursively. The configuration depends on the field #f_diff, which
  * indicates whether it has to be interpreted in "differential mode". Please
  * refer to Block::set_BlockConfig() for understanding how #f_diff and \p
  * deleteold affect the configuration of a Block. The behaviour of this
  * method is the following:
  *
  * First, BlockConfig::apply() is invoked for configuring the given
  * Block. Then, for each sub-Block of the given Block handled by this
  * RBlockConfig (see #v_sub_Block_id), apply() is invoked for the
  * corresponding BlockConfig for configuring the sub-Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current Configuration of Block must
  *        be deleted (see BlockConfig::apply()). */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this RBlockConfig
 /** This method clears this RBlockConfig by first calling
  * BlockConfig::clear() and then deleting the pointer to the BlockConfig of
  * each sub-Block and finally clearing the vector #v_sub_BlockConfig. The
  * vector #v_sub_Block_id is left unchanged. */

 void clear( void ) override {
  BlockConfig::clear();
  for( auto config : v_sub_BlockConfig )
   delete config;
  v_sub_BlockConfig.clear();
  }

/*--------------------------------------------------------------------------*/

 /// returns the number of sub-BlockConfig in this RBlockConfig
 /** This method returns the number of BlockConfig (for the sub-Block)
  * currently being handled by this RBlockConfig . */

 Block::Index num_sub_BlockConfig( void ) {
  return v_sub_BlockConfig.size();
  }

/*-------------------------------- CLONE -----------------------------------*/

 RBlockConfig * clone( void ) const override
 {
  return( new RBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE RBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the RBlockConfig
 * @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of a RBlockConfig. See RBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the RBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this RBlockConfig out of an istream
 /** Load this RBlockConfig out of an istream. The format is defined as that
  * of BlockConfig (see BlockConfig::load()) followed by:
  *
  * a number k such that abs(k) is the number of the sub-BlockConfig objects
  *
  * for i = 1 ... abs(k)
  *  - if k < 0, the identification of the sub-Block
  *
  *  - a string containing the class type of a BlockConfig object, '*' means
  *    none (nullptr)
  *
  *  - if the above is not '*', the description of the :BlockConfig object
  *
  * Notice that the sign of k determines whether the identification of the
  * sub-Block must be provided. If k < 0, then the identification of the
  * sub-Block must be provided. The identification of the sub-Block can be
  * either its name (see Block::name()) or its index in the list of sub-Block
  * or its father Block. If k >= 0, then the identification of the sub-Block
  * must not be provided. In this case, the i-th BlockConfig is associated
  * with the i-th sub-Block of the Block (i.e., i is taken as the index of the
  * sub-Block and v_sub_Block_id[ i ] = "i").
  *
  *     IF THE NAME OF THE Block IS USED AS ITS IDENTIFICATION, THEN
  *     THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT.
  */

 void load( std::istream &input ) override;

/**@} ----------------------------------------------------------------------*/
/*---------------------- PROTECTED FIELDS OF THE CLASS ---------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected fields of the class
 *  @{ */

 /// the vector of sub-BlockConfig for each of the sub-Block of the Block
 std::vector<BlockConfig *> v_sub_BlockConfig;

 /// correspondence between v_sub_BlockConfig and the sub-Block of the Block
 /** This vector specifies the correspondence between the BlockConfig in
  * #v_sub_BlockConfig and the sub-Block of the Block. v_sub_Block_id[ i ]
  * contains the identification of the sub-Block whose BlockConfig is
  * v_sub_BlockConfig[ i ]. A sub-Block can be identified either by its name
  * or by its index in the list of sub-Blocks of its father Block. If the name
  * of the sub-Block is used, then the first character of this name cannot be
  * a digit. */
 std::vector<std::string> v_sub_Block_id;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( RBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the Constraint of a Block
/** The CBlockConfig class ("Constraint" BlockConfig) derives from BlockConfig
 * and offers support for configuring the Constraint of a Block. The
 * CBlockConfig contains the following fields:
 *
 * - a vector identifying the set of Constraint that require a ComputeConfig
 *
 * - a vector of pointers to ComputeConfig for each indicated Constraint of a
 *   Block.
 */

class CBlockConfig : public BlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING CBlockConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing CBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty CBlockConfig

 CBlockConfig( bool diff = true ) : BlockConfig( diff ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig out of the given netCDF \p group
 /** It constructs an CBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an CBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        CBlockConfig. */

 CBlockConfig( netCDF::NcGroup & group ) : CBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig out of an istream
 /** It constructs an CBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * CBlockConfig.
  *
  * @param input The istream containing the description of the
  *        CBlockConfig. */

 CBlockConfig( std::istream &input ) : CBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig for the given Block
 /** It constructs an CBlockConfig for the given \p block. It creates
  * an empty CBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an CBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 CBlockConfig( Block * block , bool diff = false ) : CBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 CBlockConfig( const CBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a CBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a BlockConfig, the
  * group should also contain the following:
  *
  * - the dimension "n_Config_Constraint" containing the number of
  *   ComputeConfig descriptions associated with the Constraint of the current
  *   Block; this dimension is optional; if it is not provided, then
  *   n_Config_Constraint = 0 is assumed.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "Constraint_group_id", of size p and type
  *   netCDF::NcString, containing the identification of the group of
  *   Constraint that require a ComputeConfig. The i-th element of this
  *   vector, Constraint_group_id[ i ], is the identification of the group to
  *   which the i-th Constraint belongs. For each i = 0, ..., p - 1,
  *   Constraint_group_id[ i ] can be indicated in two ways: it is either (i)
  *   the name of the group of Constraint (see Block::get_s_const_name() and
  *   Block::get_d_const_name()) or the index of the group as defined in
  *   Block::ConstraintID.
  *
  *       IF A Constraint group IS BEING IDENTIFIED USING THE NAME OF THE
  *       GROUP (RATHER THAN THE INDEX OF THE GROUP), THEN
  *
  *       - THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT;
  *
  *       - THE STATIC GROUP HAS PRIORITY OVER THE DYNAMIC GROUP OF Constraint:
  *         IF THERE IS A GROUP OF STATIC Constraint WITH THE GIVEN NAME, THEN
  *         THIS GROUP IS CONSIDERED. OTHERWISE, THE GROUP OF DYNAMIC Constraint
  *         WITH THAT NAME IS CONSIDERED.
  *
  *   This variable is mandatory if n_Config_Constraint > 0.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "Constraint_index", of size p and type netCDF::NcUint,
  *   containing the index of the Constraint that require a ComputeConfig. The
  *   i-th element of this vector, Constraint_index[ i ], is the index of the
  *   i-th Constraint (which belongs to the group indicated by
  *   Constraint_group_id[ i ]). See Block::ConstraintID for the definition of
  *   the index of a Constraint in a group. This variable is optional. If it
  *   is not provided, then Constraint_index[ i ] = i for all i = 0, ..., p -
  *   1 is assumed.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a ComputeConfig associated with the
  *   i-th Constraint indicated by the pair ( Constraint_group_id[ i ],
  *   Constraint_index[ i ] ); these groups are optional; if
  *   "Config_Constraint_<i>" is not provided, then nullptr (default
  *   configuration) is assumed for the i-th Constraint. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all ComputeConfig of the Constraint
 /** It deletes all ComputeConfig of the Constraint handled by this
  * CBlockConfig. */
 virtual ~CBlockConfig()
 {
  for( auto config : v_Config_Constraint )
   delete config;
  }

/**@} ----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE CBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the CBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its Constraint
 /** Method for configuring the given Block and its Constraint. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block. The behaviour of this method is the
  * following:
  *
  * First, BlockConfig::apply() is invoked. Then, set_ComputeConfig() is
  * invoked for each Constraint of the given Block handled by this
  * CBlockConfig.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this CBlockConfig
 /** This method clears this CBlockConfig by first calling
  * BlockConfig::clear() and then deleting the pointer to the ComputeConfig of
  * each Constraint considered by this CBlockConfig and finally clearing the
  * vector #v_Config_Constraint. Notice that the vector #v_Constraint_id is
  * preserved. */

 void clear( void ) override {
  BlockConfig::clear();
  for( auto config : v_Config_Constraint )
   delete config;
  v_Config_Constraint.clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 CBlockConfig * clone( void ) const override
 {
  return( new CBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the CBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Constraint) and stores in this CBlockConfig. This information consists
  * of that supported by the BlockConfig (see BlockConfig::get()) plus any
  * ComputeConfig that may be associated with the Constraint of the given
  * Block. Any Configuration that this CBlockConfig may have at the moment
  * this function is invoked is deleted. If #v_Constraint_id is not empty then
  * its content is preserved and only the ComputeConfig associated with the
  * Constraint (of the given \p block) specified by #v_Constraint_id are
  * considered. If #v_Constraint_id is empty then every Constraint in the given
  * \p block is inspected. In this case, for each Constraint of the given \p
  * block, its ComputeConfig is stored in this CBlockConfig if it has a
  * non-default set of parameters.
  *
  * If #v_Constraint_id is not empty but one wants all Constraint to be
  * inspected (for instance, one is not sure that every Constraint that is not
  * specified in #v_Constraint_id has a default set of parameters), then
  * #v_Constraint_id must be cleared before this method is called.
  *
  * Note that if #v_Constraint_id is empty then
  *
  *     CALLING CBlockConfig::get() IS A POTENTIALLY COSTLY OPERATION BECAUSE
  *     IT ENTAILS SCANNING ALL Constraint OF THE Block IN ORDER TO OBTAIN
  *     THEIR ComputeConfig.
  *
  * @param block A pointer to the Block whose CBlockConfig must be filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE CBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the CBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an CBlockConfig. See CBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*--------------------- PUBLIC FIELDS OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public fields of the class
 *  @{ */

 /// the vector that identifies the Constraint that require a ComputeConfig
 /** This vector indicates which Constraint of the Block require a
  * ComputeConfig. Each element of this vector identifies one Constraint. A
  * Constraint is identified by a pair. The first element of the pair is an
  * identification of the group to which the Constraint belongs and the second
  * one is the index of the Constraint in that group (see Block::ConstraintID
  * for the definition of the index of a Constraint in a group). The group to
  * which the Constraint belongs can be indicated in two ways: it is either
  * (i) the name of the group of Constraint (see Block::get_s_const_name() and
  * Block::get_d_const_name()) or the index of the group as defined in
  * Block::ConstraintID. */
 std::vector< std::pair<std::string , Block::Index> > v_Constraint_id;

 /// the vector of (pointer to the) ComputeConfig for the Constraint
 /** The vector of (pointer to the) ComputeConfig for the Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector #v_Constraint_id. */
 std::vector<ComputeConfig *> v_Config_Constraint;

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the CBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this CBlockConfig out of an istream
 /** Load this CBlockConfig out of an istream. The format is defined as that
  * specified in BlockConfig::load(), followed by:
  *
  * - the number k of the ComputeConfig for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   - the identification of the Constraint
  *   - a string containing the class type of a ComputeConfig object,
  *     '*' means none (nullptr)
  *   - if the above is not '*', the description of the :ComputeConfig object
  *   (clearly, if k == 0 this is empty)
  *
  * The identification of the Constraint can be either (i) two integers
  * representing the Block::ConstraintID of the Constraint (see
  * Block::ConstraintID for details) or (ii) the name of the group to which
  * the Constraint belongs followed by the index of the Constraint in that
  * group (see Block::ConstraintID for the definition of the index of a
  * Constraint in a group).
  *
  *     IF THE Constraint IS BEING IDENTIFIED USING THE NAME OF THE GROUP TO
  *     WHICH IT BELONGS, THEN
  *
  *     - THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT;
  *
  *     - THE STATIC GROUP HAS PRIORITY OVER THE DYNAMIC GROUP OF Constraint:
  *       IF THERE IS A GROUP OF STATIC Constraint WITH THE GIVEN NAME, THEN
  *       THIS GROUP IS CONSIDERED. OTHERWISE, THE GROUP OF DYNAMIC Constraint
  *       WITH THAT NAME IS CONSIDERED. */

 void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( CBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the Objective of a Block
/** The OBlockConfig class ("Objective" BlockConfig) derives from BlockConfig
 * and offers support for configuring the Objective of a Block. The
 * OBlockConfig contains the following field:
 *
 * - a pointer to ComputeConfig for the Objective of the Block.
 */

class OBlockConfig : public BlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING OBlockConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing OBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty OBlockConfig

 OBlockConfig( bool diff = true ) : BlockConfig( diff ) ,
                                    f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an OBlockConfig out of the given netCDF \p group
 /** It constructs an OBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OBlockConfig. */

 OBlockConfig( netCDF::NcGroup & group ) : OBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OBlockConfig out of an istream
 /** It constructs an OBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OBlockConfig. */

 OBlockConfig( std::istream &input ) : OBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OBlockConfig for the given Block
 /** It constructs an OBlockConfig for the given \p block. It creates
  * an empty OBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an OBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 OBlockConfig( Block * block , bool diff = false ) : OBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OBlockConfig( const OBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a OBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a BlockConfig, the
  * group should also contain the following:
  *
  * - a group with name "Config_Objective", containing the description of a
  *   ComputeConfig associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes the ComputeConfig of the Objective
 /** It deletes the ComputeConfig of the Objective of the Block. */
 virtual ~OBlockConfig()
 {
  delete f_Config_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE OBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its Objective
 /** Method for configuring the given Block and its Objective. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block. The behaviour of this method is the
  * following:
  *
  * First, BlockConfig::apply() is invoked. Then, set_ComputeConfig() is
  * invoked for the Objective of the given Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this OBlockConfig
 /** This method clears this OBlockConfig by first calling
  * BlockConfig::clear() and then deleting the pointer to the ComputeConfig
  * of the Objective. */

 void clear( void ) override {
  BlockConfig::clear();
  delete f_Config_Objective;
  }

/*------------------------------- CLONE ------------------------------------*/

 OBlockConfig * clone( void ) const override
 {
  return( new OBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the OBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OBlockConfig. This information consists
  * of that supported by the BlockConfig (see BlockConfig::get()) plus the
  * ComputeConfig that may be associated with the Objective of the given
  * Block. Any Configuration that this OBlockConfig may have at the moment
  * this function is invoked is deleted.
  *
  * @param block A pointer to the Block whose OBlockConfig must be filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE OBlockConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an OBlockConfig. See OBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE OBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the OBlockConfig
 *  @{ */

 /// sets the ComputeConfig of the Objective
 /** This function sets the pointer to the ComputeConfig of the Objective of
  * the Block.
  *
  * @param config A pointer to the ComputeConfig of the Objective.
  *
  * @param deleteold It indicates whether the currently stored ComputeConfig
  *        for the Objective (if any) must be destroyed. */

 void set_Config_Objective( ComputeConfig * config , bool deleteold = true ) {
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/**@} ----------------------------------------------------------------------*/
/*------------ Methods for reading the data of the OBlockConfig ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the OBlockConfig
 *  @{ */

 /// returns the ComputeConfig of the Objective
 /** This function returns a pointer to the ComputeConfig of the Objective of
  * the Block. */

 ComputeConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this OBlockConfig out of an istream
 /** Load this OBlockConfig out of an istream. The format is defined as that
  * specified in BlockConfig::load(), followed by:
  *
  * - a string containing the class type of a ComputeConfig object for the
  *   Objective, '*' means none (nullptr)
  *
  * - if the above is not '*', the description of the :ComputeConfig object
  *   for the Objective. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the (pointer to the) ComputeConfig for the Objective of the Block
 ComputeConfig * f_Config_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( OBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ORBlockConfig ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from RBlockConfig for configuring the Objective of a Block
/** The ORBlockConfig class ("Objective" RBlockConfig) derives from
 * RBlockConfig and offers support for configuring also the Objective of a
 * Block. The ORBlockConfig contains the following field (besides those
 * defined in RBlockConfig):
 *
 * - a pointer to the ComputeConfig of the Objective of the
 *   Block.
 */

class ORBlockConfig : public RBlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING ORBlockConfig ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing ORBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty ORBlockConfig

 ORBlockConfig( bool diff = true ) : RBlockConfig( diff ) ,
                                     f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an ORBlockConfig out of the given netCDF \p group
 /** It constructs an ORBlockConfig out of the given netCDF \p group.  Please
  * refer to the deserialize() method for the format of the netCDF::NcGroup of
  * a ORBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        ORBlockConfig. */

 ORBlockConfig( netCDF::NcGroup & group ) : ORBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an ORBlockConfig out of an istream
 /** It constructs an ORBlockConfig out of the given istream \p input.
  * Please refer to the load() method for the format of an ORBlockConfig.
  *
  * @param input The istream containing the description of the
  *        ORBlockConfig. */

 ORBlockConfig( std::istream &input ) : ORBlockConfig() { load( input ); }

/*--------------------------------------------------------------------------*/
 /// constructs an ORBlockConfig for the given Block
 /** It constructs an ORBlockConfig for the given \p block. It creates an
  * empty ORBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an ORBlockConfig will be
  *        constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 ORBlockConfig( Block * block , bool diff = false ) : ORBlockConfig( diff )
 { get( block ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 ORBlockConfig( const ORBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of an ORBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should contain the following:
  *
  * - a description of a RBlockConfig object for the Block, as described in
  *   RBlockConfig::deserialize().
  *
  * - a group with name "Config_Objective", containing the description of a
  *   ComputeConfig associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes the ComputeConfig of the Objective
 virtual ~ORBlockConfig()
 {
  delete f_Config_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the ORBlockConfig of the given Block
 /** This method gets information about the current set of parameters of the
  * given Block (and its Objective and sub-Block, recursively) and stores in
  * this ORBlockConfig. This information consists of that supported by the
  * RBlockConfig (see RBlockConfig::get()) plus the ComputeConfig of the
  * Objective of the given Block. Notice that any Configuration that this
  * ORBlockConfig may have is deleted and the Configuration of the BlockConfig
  * of the given Block is cloned into the Configuration of this ORBlockConfig.
  *
  * @param block A pointer to the Block whose ORBlockConfig must be filled.
  */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE ORBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the ORBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block, its Objective,  and its sub-Block (recursively)
 /** Method for configuring the given Block, its Objective and all its
  * sub-Block, recursively. The configuration depends on the field #f_diff,
  * which indicates whether it has to be interpreted in "differential
  * mode". Please refer to Block::set_BlockConfig() for understanding how
  * #f_diff and \p deleteold affect the configuration of a Block. The
  * behaviour of this method is the following:
  *
  * First, RBlockConfig::apply() is invoked for configuring the given
  * Block. Then, set_ComputeConfig() is invoked for the Objective of the given
  * Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current Configuration of Block must
  *        be deleted (see BlockConfig::apply()). */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this ORBlockConfig
 /** This method clears this ORBlockConfig by first calling
  * RBlockConfig::clear() and then deleting the pointer to the ComputeConfig
  * of the Objective. */

 void clear( void ) override {
  RBlockConfig::clear();
  delete f_Config_Objective;
  }

/*------------------------------- CLONE ------------------------------------*/

 ORBlockConfig * clone( void ) const override
 {
  return( new ORBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE ORBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the ORBlockConfig
 * @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an ORBlockConfig. See ORBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE OBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the OBlockConfig
 *  @{ */

 /// sets the ComputeConfig of the Objective
 /** This function sets the pointer to the ComputeConfig of the Objective of
  * the Block.
  *
  * @param config A pointer to the ComputeConfig of the Objective.
  *
  * @param deleteold It indicates whether the currently stored ComputeConfig
  *        for the Objective (if any) must be destroyed. */

 void set_Config_Objective( ComputeConfig * config , bool deleteold = true ) {
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/**@} ----------------------------------------------------------------------*/
/*------------- Methods for reading the data of the OBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the OBlockConfig
 *  @{ */

 /// returns the ComputeConfig of the Objective
 /** This function returns a pointer to the ComputeConfig of the Objective of
  * the Block. */

 ComputeConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the ORBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this ORBlockConfig out of an istream
 /** Load this ORBlockConfig out of an istream. The format is defined as that
  * of RBlockConfig (see RBlockConfig::load()) followed by:
  *
  * - a string containing the class type of a ComputeConfig object for the
  *   Objective, '*' means none (nullptr)
  *
  * - if the above is not '*', the description of the :ComputeConfig object
  *   for the Objective. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the (pointer to the) ComputeConfig for the Objective of the Block
 ComputeConfig * f_Config_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ORBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CRBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from RBlockConfig for configuring the Constraint of a Block
/** The CRBlockConfig class ("Constraint" RBlockConfig) derives from
 * RBlockConfig and offers support for configuring the Constraint of a
 * Block. The CRBlockConfig contains the following fields (besides those
 * defined in RBlockConfig):
 *
 * - a vector identifying the set of Constraint that require a ComputeConfig
 *
 * - a vector of pointers to ComputeConfig for each indicated Constraint of a
 *   Block.
 */

class CRBlockConfig : public RBlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING CRBlockConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing CRBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty CRBlockConfig

 CRBlockConfig( bool diff = true ) : RBlockConfig( diff ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an CRBlockConfig out of the given netCDF \p group
 /** It constructs an CRBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an CRBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        CRBlockConfig. */

 CRBlockConfig( netCDF::NcGroup & group ) : CRBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an CRBlockConfig out of an istream
 /** It constructs an CRBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * CRBlockConfig.
  *
  * @param input The istream containing the description of the
  *        CRBlockConfig. */

 CRBlockConfig( std::istream &input ) : CRBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an CRBlockConfig for the given Block
 /** It constructs an CRBlockConfig for the given \p block. It creates
  * an empty CRBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an CRBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 CRBlockConfig( Block * block , bool diff = false ) : CRBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 CRBlockConfig( const CRBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a CRBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a RBlockConfig, the
  * group should also contain the following:
  *
  * - the dimension "n_Config_Constraint" containing the number of
  *   ComputeConfig descriptions associated with the Constraint of the current
  *   Block; this dimension is optional; if it is not provided, then
  *   n_Config_Constraint = 0 is assumed.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "Constraint_group_id", of size p and type
  *   netCDF::NcString, containing the identification of the group of
  *   Constraint that require a ComputeConfig. The i-th element of this
  *   vector, Constraint_group_id[ i ], is the identification of the group to
  *   which the i-th Constraint belongs. For each i = 0, ..., p - 1,
  *   Constraint_group_id[ i ] can be indicated in two ways: it is either (i)
  *   the name of the group of Constraint (see Block::get_s_const_name() and
  *   Block::get_d_const_name()) or the index of the group as defined in
  *   Block::ConstraintID.
  *
  *       IF A Constraint group IS BEING IDENTIFIED USING THE NAME OF THE
  *       GROUP (RATHER THAN THE INDEX OF THE GROUP), THEN
  *
  *       - THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT;
  *
  *       - THE STATIC GROUP HAS PRIORITY OVER THE DYNAMIC GROUP OF
  *         Constraint: IF THERE IS A GROUP OF STATIC Constraint WITH THE
  *         GIVEN NAME, THEN THIS GROUP IS CONSIDERED. OTHERWISE, THE GROUP OF
  *         DYNAMIC Constraint WITH THAT NAME IS CONSIDERED.
  *
  *   This variable is mandatory if n_Config_Constraint > 0.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "Constraint_index", of size p and type netCDF::NcUint,
  *   containing the index of the Constraint that require a ComputeConfig. The
  *   i-th element of this vector, Constraint_index[ i ], is the index of the
  *   i-th Constraint (which belongs to the group indicated by
  *   Constraint_group_id[ i ]). See Block::ConstraintID for the definition of
  *   the index of a Constraint in a group. This variable is optional. If it
  *   is not provided, then Constraint_index[ i ] = i for all i = 0, ..., p -
  *   1 is assumed.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a ComputeConfig associated with the
  *   i-th Constraint indicated by the pair ( Constraint_group_id[ i ],
  *   Constraint_index[ i ] ); these groups are optional; if
  *   "Config_Constraint_<i>" is not provided, then nullptr (default
  *   configuration) is assumed for the i-th Constraint. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all ComputeConfig of the Constraint
 /** It deletes all ComputeConfig of the Constraint handled by this
  * CRBlockConfig. */
 virtual ~CRBlockConfig()
 {
  for( auto config : v_Config_Constraint )
   delete config;
  }

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE CRBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the CRBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its Constraint
 /** Method for configuring the given Block and its Constraint. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block. The behaviour of this method is the
  * following:
  *
  * First, RBlockConfig::apply() is invoked. Then, set_ComputeConfig() is
  * invoked for each Constraint of the given Block handled by this
  * CRBlockConfig.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this CRBlockConfig
 /** This method clears this CRBlockConfig by first calling
  * RBlockConfig::clear() and then deleting the pointer to the ComputeConfig
  * of each Constraint considered by this CRBlockConfig and finally clearing
  * the vector #v_Config_Constraint. Notice that the vector #v_Constraint_id
  * is preserved. */

 void clear( void ) override {
  RBlockConfig::clear();
  for( auto config : v_Config_Constraint )
   delete config;
  v_Config_Constraint.clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 CRBlockConfig * clone( void ) const override
 {
  return( new CRBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the CRBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Constraint) and stores in this CRBlockConfig. This information
  * consists of that supported by the RBlockConfig (see RBlockConfig::get())
  * plus any ComputeConfig that may be associated with the Constraint of the
  * given Block. Any Configuration that this CRBlockConfig may have at the
  * moment this function is invoked is deleted. If #v_Constraint_id is not
  * empty then its content is preserved and only the ComputeConfig associated
  * with the Constraint (of the given \p block) specified by #v_Constraint_id
  * are considered. If #v_Constraint_id is empty then every Constraint in the
  * given \p block is inspected. In this case, for each Constraint of the
  * given \p block, its ComputeConfig is stored in this CRBlockConfig if it
  * has a non-default set of parameters.
  *
  * If #v_Constraint_id is not empty but one wants all Constraint to be
  * inspected (for instance, one is not sure that every Constraint that is not
  * specified in #v_Constraint_id has a default set of parameters), then
  * #v_Constraint_id must be cleared before this method is called.
  *
  * Note that if #v_Constraint_id is empty then
  *
  *     CALLING CRBlockConfig::get() IS A POTENTIALLY COSTLY OPERATION BECAUSE
  *     IT ENTAILS SCANNING ALL Constraint OF THE Block IN ORDER TO OBTAIN
  *     THEIR ComputeConfig.
  *
  * @param block A pointer to the Block whose CRBlockConfig must be filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE CRBlockConfig -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the CRBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an CRBlockConfig. See CRBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*--------------------- PUBLIC FIELDS OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public fields of the class
 *  @{ */

 /// the vector that identifies the Constraint that require a ComputeConfig
 /** This vector indicates which Constraint of the Block require a
  * ComputeConfig. Each element of this vector identifies one Constraint. A
  * Constraint is identified by a pair. The first element of the pair is an
  * identification of the group to which the Constraint belongs and the second
  * one is the index of the Constraint in that group (see Block::ConstraintID
  * for the definition of the index of a Constraint in a group). The group to
  * which the Constraint belongs can be indicated in two ways: it is either
  * (i) the name of the group of Constraint (see Block::get_s_const_name() and
  * Block::get_d_const_name()) or the index of the group as defined in
  * Block::ConstraintID. */
 std::vector< std::pair<std::string , Block::Index> > v_Constraint_id;

 /// the vector of (pointer to the) ComputeConfig for the Constraint
 /** The vector of (pointer to the) ComputeConfig for the Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector #v_Constraint_id. */
 std::vector<ComputeConfig *> v_Config_Constraint;

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the CRBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this CRBlockConfig out of an istream
 /** Load this CRBlockConfig out of an istream. The format is defined as that
  * specified in RBlockConfig::load(), followed by:
  *
  * - the number k of the ComputeConfig for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   - the identification of the Constraint
  *   - a string containing the class type of a ComputeConfig object,
  *     '*' means none (nullptr)
  *   - if the above is not '*', the description of the :ComputeConfig object
  *   (clearly, if k == 0 this is empty)
  *
  * The identification of the Constraint can be either (i) two integers
  * representing the Block::ConstraintID of the Constraint (see
  * Block::ConstraintID for details) or (ii) the name of the group to which
  * the Constraint belongs followed by the index of the Constraint in that
  * group (see Block::ConstraintID for the definition of the index of a
  * Constraint in a group).
  *
  *     IF THE Constraint IS BEING IDENTIFIED USING THE NAME OF THE GROUP TO
  *     WHICH IT BELONGS, THEN
  *
  *     - THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT;
  *
  *     - THE STATIC GROUP HAS PRIORITY OVER THE DYNAMIC GROUP OF Constraint:
  *       IF THERE IS A GROUP OF STATIC Constraint WITH THE GIVEN NAME, THEN
  *       THIS GROUP IS CONSIDERED. OTHERWISE, THE GROUP OF DYNAMIC Constraint
  *       WITH THAT NAME IS CONSIDERED. */

 void load( std::istream &input ) override;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( CRBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OCBlockConfig ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from CBlockConfig for configuring the Objective of a Block
/** The OCBlockConfig class ("Objective" CBlockConfig) derives from
 * CBlockConfig and offers support for configuring the Objective of a
 * Block. The OCBlockConfig contains the following field (besides those
 * defined in CBlockConfig):
 *
 * - a pointer to ComputeConfig for the Objective of the Block.
 */

class OCBlockConfig : public CBlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING OCBlockConfig ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing OCBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty OCBlockConfig

 OCBlockConfig( bool diff = true ) : CBlockConfig( diff ) ,
                                     f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an OCBlockConfig out of the given netCDF \p group
 /** It constructs an OCBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OCBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OCBlockConfig. */

 OCBlockConfig( netCDF::NcGroup & group ) : OCBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OCBlockConfig out of an istream
 /** It constructs an OCBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OCBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OCBlockConfig. */

 OCBlockConfig( std::istream &input ) : OCBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OCBlockConfig for the given Block
 /** It constructs an OCBlockConfig for the given \p block. It creates
  * an empty OCBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an OCBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 OCBlockConfig( Block * block , bool diff = false ) : OCBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OCBlockConfig( const OCBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends CBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends CBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a OCBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a CBlockConfig, the
  * group should also contain the following:
  *
  * - a group with name "Config_Objective", containing the description of a
  *   ComputeConfig associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes the ComputeConfig of the Objective
 /** It deletes the ComputeConfig of the Objective of the Block. */
 virtual ~OCBlockConfig()
 {
  delete f_Config_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE OCBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OCBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its Objective
 /** Method for configuring the given Block and its Objective. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block. The behaviour of this method is the
  * following:
  *
  * First, CBlockConfig::apply() is invoked. Then, set_ComputeConfig() is
  * invoked for the Objective of the given Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this OCBlockConfig
 /** This method clears this OCBlockConfig by first calling
  * CBlockConfig::clear() and then deleting the pointer to the ComputeConfig
  * of the Objective. */

 void clear( void ) override {
  CBlockConfig::clear();
  delete f_Config_Objective;
  }

/*------------------------------- CLONE ------------------------------------*/

 OCBlockConfig * clone( void ) const override
 {
  return( new OCBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the OCBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OCBlockConfig. This information consists
  * of that supported by the CBlockConfig (see CBlockConfig::get()) plus the
  * ComputeConfig that may be associated with the Objective of the given
  * Block. Any Configuration that this OCBlockConfig may have at the moment
  * this function is invoked is deleted.
  *
  * @param block A pointer to the Block whose OCBlockConfig must be filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*------- METHODS FOR LOADING, PRINTING & SAVING THE OCBlockConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OCBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends CBlockConfig::serialize( netCDF::NcGroup )
 /** Extends CBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an OCBlockConfig. See OCBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE OCBlockConfig -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the OCBlockConfig
 *  @{ */

 /// sets the ComputeConfig of the Objective
 /** This function sets the pointer to the ComputeConfig of the Objective of
  * the Block.
  *
  * @param config A pointer to the ComputeConfig of the Objective.
  *
  * @param deleteold It indicates whether the currently stored ComputeConfig
  *        for the Objective (if any) must be destroyed. */

 void set_Config_Objective( ComputeConfig * config , bool deleteold = true ) {
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/**@} ----------------------------------------------------------------------*/
/*----------- Methods for reading the data of the OCBlockConfig ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the OCBlockConfig
 *  @{ */

 /// returns the ComputeConfig of the Objective
 /** This function returns a pointer to the ComputeConfig of the Objective of
  * the Block. */

 ComputeConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OCBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this OCBlockConfig out of an istream
 /** Load this OCBlockConfig out of an istream. The format is defined as that
  * specified in CBlockConfig::load(), followed by:
  *
  * - a string containing the class type of a ComputeConfig object for the
  *   Objective, '*' means none (nullptr)
  *
  * - if the above is not '*', the description of the :ComputeConfig object
  *   for the Objective. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the (pointer to the) ComputeConfig for the Objective of the Block
 ComputeConfig * f_Config_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( OCBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OCRBlockConfig --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from CRBlockConfig for configuring the Objective of a Block
/** The OCRBlockConfig class ("Objective" CRBlockConfig) derives from
 * CRBlockConfig and offers support for configuring the Objective of a
 * Block. The OCRBlockConfig contains the following field (besides those
 * defined in CRBlockConfig):
 *
 * - a pointer to ComputeConfig for the Objective of the Block.
 */

class OCRBlockConfig : public CRBlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING OCRBlockConfig ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing OCRBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty OCRBlockConfig

 OCRBlockConfig( bool diff = true ) : CRBlockConfig( diff ) ,
                                      f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an OCRBlockConfig out of the given netCDF \p group
 /** It constructs an OCRBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OCRBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OCRBlockConfig. */

 OCRBlockConfig( netCDF::NcGroup & group ) : OCRBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OCRBlockConfig out of an istream
 /** It constructs an OCRBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OCRBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OCRBlockConfig. */

 OCRBlockConfig( std::istream &input ) : OCRBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OCRBlockConfig for the given Block
 /** It constructs an OCRBlockConfig for the given \p block. It creates
  * an empty OCRBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an OCRBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one. */

 OCRBlockConfig( Block * block , bool diff = false ) : OCRBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OCRBlockConfig( const OCRBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends CRBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends CRBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a OCRBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a CRBlockConfig, the
  * group should also contain the following:
  *
  * - a group with name "Config_Objective", containing the description of a
  *   ComputeConfig associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes the ComputeConfig of the Objective
 /** It deletes the ComputeConfig of the Objective of the Block. */
 virtual ~OCRBlockConfig()
 {
  delete f_Config_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE OCRBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OCRBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its Objective
 /** Method for configuring the given Block and its Objective. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block. The behaviour of this method is the
  * following:
  *
  * First, CRBlockConfig::apply() is invoked. Then, set_ComputeConfig() is
  * invoked for the Objective of the given Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*--------------------------------------------------------------------------*/

 /// clear this OCRBlockConfig
 /** This method clears this OCRBlockConfig by first calling
  * CRBlockConfig::clear() and then deleting the pointer to the ComputeConfig
  * of the Objective. */

 void clear( void ) override {
  CRBlockConfig::clear();
  delete f_Config_Objective;
  }

/*------------------------------- CLONE ------------------------------------*/

 OCRBlockConfig * clone( void ) const override
 {
  return( new OCRBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the OCRBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OCRBlockConfig. This information
  * consists of that supported by the CRBlockConfig (see CRBlockConfig::get())
  * plus the ComputeConfig that may be associated with the Objective of the
  * given Block. Any Configuration that this OCRBlockConfig may have at the
  * moment this function is invoked is deleted.
  *
  * @param block A pointer to the Block whose OCRBlockConfig must be
  *        filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*------- METHODS FOR LOADING, PRINTING & SAVING THE OCRBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OCRBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends CRBlockConfig::serialize( netCDF::NcGroup )
 /** Extends CRBlockConfig::serialize( netCDF::NcGroup ) to the specific
  * format of an OCRBlockConfig. See OCRBlockConfig::deserialize(
  * netCDF::NcGroup ) for details of the format of the created netCDF
  * group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*--------------- METHODS FOR MODIFYING THE OCRBlockConfig -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the OCRBlockConfig
 *  @{ */

 /// sets the ComputeConfig of the Objective
 /** This function sets the pointer to the ComputeConfig of the Objective of
  * the Block.
  *
  * @param config A pointer to the ComputeConfig of the Objective.
  *
  * @param deleteold It indicates whether the currently stored ComputeConfig
  *        for the Objective (if any) must be destroyed. */

 void set_Config_Objective( ComputeConfig * config , bool deleteold = true ) {
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/**@} ----------------------------------------------------------------------*/
/*----------- Methods for reading the data of the OCRBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the OCRBlockConfig
 *  @{ */

 /// returns the ComputeConfig of the Objective
 /** This function returns a pointer to the ComputeConfig of the Objective of
  * the Block. */

 ComputeConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OCRBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this OCRBlockConfig out of an istream
 /** Load this OCRBlockConfig out of an istream. The format is defined as that
  * specified in CRBlockConfig::load(), followed by:
  *
  * - a string containing the class type of a ComputeConfig object for the
  *   Objective, '*' means none (nullptr)
  *
  * - if the above is not '*', the description of the :ComputeConfig object
  *   for the Objective. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the (pointer to the) ComputeConfig for the Objective of the Block
 ComputeConfig * f_Config_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( OCRBlockConfig ) )

/** @}  end( group( RBlockConfig_CLASSES ) ) */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* RBlockConfig.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File RBlockConfig.h --------------------------*/
/*--------------------------------------------------------------------------*/

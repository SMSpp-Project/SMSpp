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
 * \version 0.33
 *
 * \date 31 - 08 - 2020
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
/** @defgroup Block_CLASSES Classes in RBlockConfig.h
 *
 * Three main classes are defined to offer support to configure not only a
 * Block but also its sub-Block and "indirect sub-Block":
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
 * - OCBlockConfig : CBlockConfig ("Constraint" BlockConfig), which also
 *   configures (potentially) all Constraint and the Objective of the given
 *   Block;
 *
 * - OCRBlockConfig : CRBlockConfig ("Objective" BlockConfig), which also
 *   configures (potentially) all sub-Block (recursively) and (potentially)
 *   all Constraint and the Objective of the given Block.
 *
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the sub-Block of a Block
/** The RBlockConfig class ("recursive" BlockConfig) derives from BlockConfig
 * and offers support for configuring also the sub-Block of a Block
 * (recursively). The RBlockConfig contains the following field:
 *
 * - a vector of pointers to BlockConfig for each of the sub-Block of the
 *   Block.
 */

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
  * - a description of a BlockConfig object for the Block, as described in
  *   BlockConfig::deserialize().
  *
  * - the dimension "n_sub_Block" containing the number of BlockConfig
  *   descriptions for the sub-Block of the current Block; this dimension is
  *   optional; if it is not provided, then n_sub_Block = 0 is assumed.
  *
  * - with n being the size of n_sub_Block, n groups, with name
  *   "sub-BlockConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a BlockConfig for one of the sub-Block of the current
  *   :Block. Each of these groups is optional. If a group is absent then the
  *   pointer to the BlockConfig for the corresponding sub-Block is
  *   considered to be a nullptr (default configuration).
  *
  * Note that that the matching between the sub-BlockConfig and the
  * sub-Block is positional: the BlockConfig found in the group
  * "sub-BlockConfig_<i>" is that for the i-th sub-Block. Note that the
  * vector of sub-BlockConfig is allowed to be of different size than the
  * number of sub-Block; if it is larger any extra BlockConfig is simply
  * ignored, if it shorted then all missing sub-BlockConfig are treated as
  * nullptr (default configuration). */

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
  * @param block A pointer to the Block whose RBlockConfig must be filled.
  */

 void get( Block * block ) override;

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
  * Block. Then, for each sub-Block of the given Block, apply() is invoked for
  * the corresponding BlockConfig for configuring the sub-Block.
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
  * each sub-Block and finally clearing the vector #v_sub_BlockConfig. */

 void clear( void ) override {
  BlockConfig::clear();
  for( auto config : v_sub_BlockConfig )
   delete config;
  v_sub_BlockConfig.clear();
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
/*--------------------- PUBLIC FIELDS OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public fields of the class
 *  @{ */

 /// the vector of sub-BlockConfig for each of the sub-Block of the Block
 std::vector<BlockConfig *> v_sub_BlockConfig;

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
  * number k of the sub-BlockConfig objects
  *
  * for i = 1 ... k
  *  - a string containing the class type of a BlockConfig object, '*' means
  *    none (nullptr)
  *
  *  - if the above is not '*', the description of the :BlockConfig object
  */

 void load( std::istream &input ) override;

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
 * CBlockConfig contains the following field:
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
  * - with p being the size of "n_Config_Constraint", a two-dimensional
  *   variable called "ConstraintID", of size p x 2 and type netCDF::ncUint,
  *   containing the Block::ConstraintID of the Constraint that need a
  *   ComputeConfig. The i-th row of this variable contains the ConstraintID
  *   of the i-th Constraint: for each i = 0, ..., p - 1, the pair
  *   ( ConstraintID[ i ][ 0 ], ConstraintID[ i ][ 1 ] ) is the ConstraintID
  *   of the i-th Constraint that needs a ComputeConfig, i.e., ConstraintID[ i
  *   ][ 0 ] provides the index of the group to which the i-th Constraint
  *   belongs and ConstraintID[ i ][ 1 ] provides the index of the i-th
  *   Constraint (see Block::ConstraintID for the definition of an index of a
  *   Constraint); this variable is mandatory if n_Config_Constraint > 0.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a ComputeConfig associated with the
  *   i-th Constraint indicated by the "ConstraintID" variable (which is given
  *   by the pair ( ConstraintID[ 2i ], ConstraintID[ 2i + 1] )); these groups
  *   are optional; if "Config_Constraint_<i>" is not provided, then nullptr
  *   (default configuration) is assumed for the i-th Constraint. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all ComputeConfig of the Constraint
 /** It deletes all ComputeConfig of the Constraint handled by this
  * CBlockConfig. */
 virtual ~CBlockConfig()
 {
  for( auto config : v_Config_Constraints )
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
  * vector #v_Config_Constraints. Notice that the vector #v_ConstraintID is
  * preserved. */

 void clear( void ) override {
  BlockConfig::clear();
  for( auto config : v_Config_Constraints )
   delete config;
  v_Config_Constraints.clear();
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
  * this function is invoked is deleted. If #v_ConstraintID is not empty then
  * its content is preserved and only the ComputeConfig associated with the
  * Constraint (of the given \p block) specified by #v_ConstraintID are
  * considered. If #v_ConstraintID is empty then every Constraint in the given
  * \p block is inspected. In this case, for each Constraint of the given \p
  * block, its ComputeConfig is stored in this CBlockConfig if it has a
  * non-default set of parameters.
  *
  * If #v_ConstraintID is not empty but one wants all Constraint to be
  * inspected (for instance, one is not sure that every Constraint that is not
  * specified in #v_ConstraintID has a default set of parameters), then
  * #v_ConstraintID must be cleared before this method is called.
  *
  * Note that
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

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a
  * ComputeConfig. */
 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) ComputeConfig for Constraint
 /** The vector of (pointer to the) ComputeConfig for Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector v_ConstraintID. */
 std::vector<ComputeConfig *> v_Config_Constraints;

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
  *   - two integers representing the ConstraintID for the Constraint
  *   - a string containing the class type of a ComputeConfig object,
  *     '*' means none (nullptr)
  *   - if the above is not '*', the description of the :ComputeConfig object
  *   (clearly, if k == 0 this is empty) */

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
/*-------------------------- CLASS CRBlockConfig ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from RBlockConfig for configuring the Constraint of a Block
/** The CRBlockConfig class ("Constraint" RBlockConfig) derives from
 * RBlockConfig and offers support for configuring the Constraint of a
 * Block. The CRBlockConfig contains the following field (besides those
 * defined in RBlockConfig):
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
  * - with p being the size of "n_Config_Constraint", a two-dimensional
  *   variable called "ConstraintID", of size p x 2 and type netCDF::ncUint,
  *   containing the Block::ConstraintID of the Constraint that need a
  *   ComputeConfig. The i-th row of this variable contains the ConstraintID
  *   of the i-th Constraint: for each i = 0, ..., p - 1, the pair
  *   ( ConstraintID[ i ][ 0 ], ConstraintID[ i ][ 1 ] ) is the ConstraintID
  *   of the i-th Constraint that needs a ComputeConfig, i.e., ConstraintID[ i
  *   ][ 0 ] provides the index of the group to which the i-th Constraint
  *   belongs and ConstraintID[ i ][ 1 ] provides the index of the i-th
  *   Constraint (see Block::ConstraintID for the definition of an index of a
  *   Constraint); this variable is mandatory if n_Config_Constraint > 0.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a ComputeConfig associated with the
  *   i-th Constraint indicated by the "ConstraintID" variable (which is given
  *   by the pair ( ConstraintID[ 2i ], ConstraintID[ 2i + 1] )); these groups
  *   are optional; if "Config_Constraint_<i>" is not provided, then nullptr
  *   (default configuration) is assumed for the i-th Constraint. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all ComputeConfig of the Constraint
 /** It deletes all ComputeConfig of the Constraint handled by this
  * CRBlockConfig. */
 virtual ~CRBlockConfig()
 {
  for( auto config : v_Config_Constraints )
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
  * the vector #v_Config_Constraints. Notice that the vector #v_ConstraintID
  * is preserved. */

 void clear( void ) override {
  RBlockConfig::clear();
  for( auto config : v_Config_Constraints )
   delete config;
  v_Config_Constraints.clear();
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
  * moment this function is invoked is deleted. If #v_ConstraintID is not
  * empty then its content is preserved and only the ComputeConfig associated
  * with the Constraint (of the given \p block) specified by #v_ConstraintID
  * are considered. If #v_ConstraintID is empty then every Constraint in the
  * given \p block is inspected. In this case, for each Constraint of the
  * given \p block, its ComputeConfig is stored in this CRBlockConfig if it
  * has a non-default set of parameters.
  *
  * If #v_ConstraintID is not empty but one wants all Constraint to be
  * inspected (for instance, one is not sure that every Constraint that is not
  * specified in #v_ConstraintID has a default set of parameters), then
  * #v_ConstraintID must be cleared before this method is called.
  *
  * Note that
  *
  *     CALLING CRBlockConfig::get() IS A POTENTIALLY COSTLY OPERATION
  *     BECAUSE IT ENTAILS SCANNING ALL Constraint OF THE Block IN ORDER
  *     TO OBTAIN THEIR ComputeConfig.
  *
  * @param block A pointer to the Block whose CRBlockConfig must be filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE CRBlockConfig --------*/
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

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a
  * ComputeConfig. */
 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) ComputeConfig for Constraint
 /** The vector of (pointer to the) ComputeConfig for Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector v_ConstraintID. */
 std::vector<ComputeConfig *> v_Config_Constraints;

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
  *   - two integers representing the ConstraintID for the Constraint
  *   - a string containing the class type of a ComputeConfig object,
  *     '*' means none (nullptr)
  *   - if the above is not '*', the description of the :ComputeConfig object
  *   (clearly, if k == 0 this is empty) */

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

/*--------------------------------------------------------------------------*/
/*------------------------ File RBlockConfig.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class RBlockConfig, derived
 * from BlockConfig, which is intended as a useful tool to configure the
 * sub-Block of a given Block, recursively.
 *
 * \version 0.33
 *
 * \date 06 - 08 - 2020
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
 * The RBlockConfig ("recursive" BlockConfig) class derives from BlockConfig
 * and allows one to configure (potentially) all sub-Block (recursively) of
 * the given Block.
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
 {  get( block ); }

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

/*------------------------------- CLONE -----------------------------------*/

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
/*---------------- METHODS FOR MODIFYING THE RBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the RBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// sets the (pointer to) the BlockConfig of each sub-Block
 /** This function sets the vector containing the (pointer to) the
  * BlockConfig of every sub-Block. If \p deleteold is true then all
  * BlockConfig for the sub-Block currently stored in this RBlockConfig are
  * destroyed.
  *
  * @param bc A vector of pointers to BlockConfig.
  *
  * @param deleteold It indicates whether the currently stored BlockConfig
  *        for the sub-Block (if any) must be destroyed. */

 void set_sub_BlockConfig( std::vector<BlockConfig *> && bc ,
                           bool deleteold = true ) {
  if( deleteold ) {
   for( auto sBC : v_sub_BlockConfig )
    delete sBC;
   }
  v_sub_BlockConfig = std::move( bc );
  }

/**@} ----------------------------------------------------------------------*/
/*------------- Methods for reading the data of the RBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the RBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// returns the (pointer to) the BlockConfig of every sub-Block
 /** This function returns a const reference to the vector containing the
  * (pointer to) the BlockConfig of every sub-Block. */

 const std::vector<BlockConfig *> & get_sub_BlockConfig( void )
  const { return( v_sub_BlockConfig ); }

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

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 /// the vector of sub-BlockConfig for each of the sub-Block of the Block
 std::vector<BlockConfig *> v_sub_BlockConfig;

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
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "ConstraintID", of size 2p and type netCDF::ncUint,
  *   containing the Block::ConstraintID of the Constraint that need a
  *   ComputeConfig: for each i = 0, ..., p - 1, the pair ( ConstraintID[ 2i
  *   ], ConstraintID[ 2i + 1] ) is the ConstraintID of the i-th Constraint
  *   that needs a ComputeConfig, i.e., ConstraintID[ 2i ] provides the index of
  *   the group to which the i-th Constraint belongs and ConstraintID[ 2i + 1
  *   ] provides the index of the i-th Constraint (see Block::ConstraintID for
  *   the definition of an index of a Constraint); this variable is mandatory
  *   if n_Config_Constraint > 0.
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
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE CBlockConfig ----------*/
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

/*------------------------------- CLONE -----------------------------------*/

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
  * this function is invoked is deleted.
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
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE CBlockConfig --------*/
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
/*--------------- METHODS FOR MODIFYING THE CBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the CBlockConfig
 *  @{ */

 /// sets the ComputeConfig of the Constraint
 /** This function sets the vector containing the (pointer to the)
  * ComputeConfig of the Constraint of the Block. The i-th ComputeConfig in
  * this vector is associated with the Constraint given by the i-th element of
  * the ConstraintID vector (see get_ConstraintID()). If \p deleteold is true
  * then all ComputeConfig for the Constraint currently stored in this
  * CBlockConfig are destroyed.
  *
  * @param configs A vector of pointers to ComputeConfig.
  *
  * @param deleteold It indicates whether the currently stored ComputeConfig
  *        for the Constraint (if any) must be destroyed. */

 void set_Config_Constraints( std::vector<ComputeConfig *> && configs ,
                              bool deleteold = true ) {
  if( deleteold ) {
   for( auto config : v_Config_Constraints )
    delete config;
   }
  v_Config_Constraints = std::move( configs );
  }

/*--------------------------------------------------------------------------*/
 /// adds a ComputeConfig of a Constraint
 /** This function adds a (pointer to the) ComputeConfig of the Constraint of
  * the Block whose Block::ConstraintID is \p constraint_id.
  *
  * @param config A pointer to a ComputeConfig.
  *
  * @param constraint_id A Block::ConstraintID indicating the Constraint of
  *        the Block. */

 void add_Config_Constraint( ComputeConfig * config ,
                             Block::ConstraintID constraint_id ) {
  v_Config_Constraints.push_back( config );
  v_ConstraintID.push_back( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the vector of ConstraintID indicating the Constraint of the Block
 /** This function sets the vector of ConstraintID, which indicates the set of
  * Constraint of the Block that have a ComputeConfig. */

 void set_ConstraintID( std::vector<Block::ConstraintID> && constraint_id ) {
  v_ConstraintID = std::move( constraint_id );
  }

/**@} ----------------------------------------------------------------------*/
/*------------ Methods for reading the data of the CBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the CBlockConfig
 *  @{ */

 /// returns the ComputeConfig of the Constraint
 /** This function returns a const reference to the vector containing the
  * (pointer to the) ComputeConfig of the Constraint of the Block. */

 const std::vector<ComputeConfig *> & get_Config_Constraints( void ) const {
  return( v_Config_Constraints );
  }

/*--------------------------------------------------------------------------*/
 /// returns the vector of ConstraintID indicating the Constraint
 /** This function returns the vector of ConstraintID, which indicates the set
  * of Constraint of the Block that have a ComputeConfig. */

 const std::vector<Block::ConstraintID> & get_ConstraintID( void ) const {
  return( v_ConstraintID );
  }

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

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a
  * ComputeConfig. */
 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) ComputeConfig for Constraint
 /** The vector of (pointer to the) ComputeConfig for Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector v_ConstraintID. */
 std::vector<ComputeConfig *> v_Config_Constraints;

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
/// derived class from BlockConfig for configuring the Constraint of a Block
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
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE OBlockConfig ----------*/
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

/*------------------------------- CLONE -----------------------------------*/

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
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE OBlockConfig --------*/
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
/*--------------- METHODS FOR MODIFYING THE OBlockConfig ------------------*/
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
/*------------ Methods for reading the data of the OBlockConfig -----------*/
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

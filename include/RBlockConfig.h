/*--------------------------------------------------------------------------*/
/*------------------------ File RBlockConfig.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class RBlockConfig and ERBlockConfig classes, derived
 * from BlockConfig, which are intended as a useful tool to configure the
 * sub-Block of a given Block, including its indirect sub-Block. Two classes
 * are defined:
 *
 * - RBlockConfig : BlockConfig ("recursive" BlockConfig), which configure
 *   (potentially) all sub-Block (recursively) of the given Block;
 *
 * - ERBlockConfig : RBlockConfig ("extended" RBlockConfig), which also
 *   configure all "indirect sub-Block" (Block that may be part of the
 *   Objective or Constraint) of a given Block. In the current implementation,
 *   these are the inner Block of either a LagBFunction or a BendersBFunction
 *   occurring as the Function in either a FRealObjective or a FRowConstraint.
 *
 * \version 0.33
 *
 * \date 20 - 07 - 2020
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
 * The RBlockConfig and ERBlockConfig classes are defined that allow to
 * handle more and more complex cases, at the cost of more memory, a more
 * complex input, and potentially a higher computational cost for some
 * operations:
 *
 * - RBlockConfig : BlockConfig ("recursive" BlockConfig), which also
 *   configure (potentially) all sub-Block (recursively) of the given Block;
 *
 * - ERBlockConfig : RBlockConfig ("extended" RBlockConfig), which also
 *   configure all "indirect sub-Block" (Block that may be part of the
 *   Objective or Constraint) of a given Block. In the current implementation,
 *   these are the inner Block of of either a LagBFunction or a
 *   BendersBFunction occurring as the Function in either a FRealObjective or
 *   a FRowConstraint.
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
  * - the attribute "diff" of netCDF::NcInt type containing the value for the
  *   f_diff field (basically, a bool telling if the information in it has to
  *   be taken as "the configuration to be set" or as "the changes to be made
  *   from the current configuration"); this attribute is optional: if it is
  *   not provided, then diff = false is assumed;
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
 /// destructor: deletes the BlockConfig and all sub-BlockConfig
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
  * - a binary number b to determine the value of #f_diff. If b == 0 then
  *   #f_diff = false, otherwise, #f_diff = true.
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
/*------------------------- CLASS ERBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from RBlockConfig for configuring "indirect sub-Block"
/** The ERBlockConfig class ("extended" RBlockConfig) derives from
 * RBlockConfig in order to also takes into account the configuration
 * of the "indirect sub-Block" of the Block, i.e., those that do not appear
 * in its sub-Block list but upon on which the Block still depends. Some
 * Constraint and Objective are complicated enough that they may depend on
 * some Block. This class is meant to offer support for configuring
 * these "indirect sub-Block". For instance, a Constraint (e.g. and
 * FRowConstraint) may have a BendersBFunction or LagBFunction, which in turn
 * has an inner Block tha may have to be configured. Since not every
 * Constraint may have a "sub-Block" (or one that needs configuration), it is
 * necessary to indicate which Constraints will have an associated
 * BlockConfig. These Constraints can be indicated by means of a
 * Block::ConstraintID.
 *
 * The ERBlockConfig contains the following fields:
 *
 * - a vector of Block::ConstraintID indicating the set of Constraint of this
 *   Block that needs a BlockConfig alongside a vector of BlockConfig for
 *   those Constraint.
 *
 * - a BlockConfig for the Objective of the Block. */

class ERBlockConfig : public RBlockConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING ERBlockConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing ERBlockConfig
 *  @{ */

/*---------------------------- CONSTRUCTORS --------------------------------*/
 /// constructor: creates an empty ERBlockConfig

 ERBlockConfig( bool diff = true ) :
  RBlockConfig( diff ) , f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockConfig out of the given netCDF \p group
 /** It constructs an ERBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an ERBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        ERBlockConfig. */

 ERBlockConfig( netCDF::NcGroup & group ) : ERBlockConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockConfig out of an istream
 /** It constructs an ERBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * ERBlockConfig.
  *
  * @param input The istream containing the description of the
  *        ERBlockConfig. */

 ERBlockConfig( std::istream &input ) : ERBlockConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockConfig for the given Block
 /** It constructs an ERBlockConfig for the given \p block. It creates
  * an empty ERBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an ERBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 ERBlockConfig( Block * block , bool diff = false ) : ERBlockConfig( diff ) {
  get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 ERBlockConfig( const ERBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of an ERBlockConfig. Besides the mandatory "type" attribute of any
  * :BlockConfig, and all that is needed to describe a RBlockConfig, the
  * group should also contain the following:
  *
  * - the dimension "n_Config_Constraint" containing the number of
  *   BlockConfig descriptions associated with the Constraint of the current
  *   Block; this dimension is optional; if it is not provided, then
  *   n_Config_Constraint = 0 is assumed.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "ConstraintID", of size 2p and type netCDF::ncUint,
  *   containing the Block::ConstraintID of the Constraint that need a
  *   BlockConfig: for each i = 0, ..., p - 1, the pair ( ConstraintID[ 2i ],
  *   ConstraintID[ 2i + 1] ) is the ConstraintID of the i-th Constraint that
  *   needs a BlockConfig, i.e., ConstraintID[ 2i ] provides the index of the
  *   group to which the i-th Constraint belongs and ConstraintID[ 2i + 1 ]
  *   provides the index of the i-th Constraint (see Block::ConstraintID for
  *   the definition of an index of a Constraint); this variable is mandatory
  *   if n_Config_Constraint > 0.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a BlockConfig associated with the
  *   i-th Constraint indicated by the "ConstraintID" variable (which is given
  *   by the pair ( ConstraintID[ 2i ], ConstraintID[ 2i + 1] )); these groups
  *   are optional; if "Config_Constraint_<i>" is not provided, then nullptr
  *   (default configuration) is assumed for the i-th Constraint;
  *
  * - a group with name "Config_Objective", containing the description of a
  *   BlockConfig associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all sub-BlockConfig
 virtual ~ERBlockConfig()
 {
  for( auto sBCC : v_Config_Constraints )
   delete sBCC;

  delete f_Config_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE ERBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the ERBlockConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// configure the given Block and its sub-Block (recursively)
 /** Method for configuring the given Block and all its sub-Block, including
  * the indirect sub-Block, recursively. The configuration depends on the
  * field #f_diff, which indicates whether it has to be interpreted in
  * "differential mode". Please refer to Block::set_BlockConfig() for
  * understanding how #f_diff and \p deleteold affect the configuration of a
  * Block. The behaviour of this method is the following:
  *
  * First, RBlockConfig::apply() is invoked. Then, this apply() method is
  * invoked for each BlockConfig for the indirect sub-Block of the given
  * Block, recursively.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override;

/*------------------------------- CLONE -----------------------------------*/

 ERBlockConfig * clone( void ) const override
 {
  return( new ERBlockConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the ERBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its sub-Block and "indirect sub-Block", recursively) and stores in this
  * ERBlockConfig. This information consists of that supported by the
  * RBlockConfig (see RBlockConfig::get()) plus any BlockConfig that may be
  * associated with Constraint and/or Objective of the given Block. Any
  * Configuration that this ERBlockConfig may have at the moment this function
  * is invoked is deleted.
  *
  * Note that
  *
  *     CALLING ERBlockConfig::get() IS A POTENTIALLY COSTLY OPERATION BECAUSE
  *     IT ENTAILS SCANNING ALL Constraint AND Objective OF THE Block, AND ALL
  *     ITS sub-Block RECURSIVELY, IN ORDER TO FIND THE "INDIRECT" sub-Block.
  *
  * Also, the current implementation only supports the case where the
  * "indirect" sub-Block are within a LagBFunction or a BendersBFunction
  * inside a FRowConstraint or FRealObjective.
  *
  * @param block A pointer to the Block whose ERBlockConfig must be
  *        filled. */

 void get( Block * block ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE ERBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the ERBlockConfig
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an ERBlockConfig. See ERBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*--------------- METHODS FOR MODIFYING THE ERBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the ERBlockConfig
 *  @{ */

 /// sets the BlockConfig of the "indirect sub-Block" of Constraint
 /** This function sets the vector containing the (pointer to the)
  * BlockConfig of the "indirect sub-Block" associated with the Constraint
  * of the Block. The i-th BlockConfig in this vector is associated with the
  * Constraint given by the i-th element of the ConstraintID vector (see
  * get_ConstraintID()). If \p deleteold is true then all BlockConfig for
  * the Constraint currently stored in this ERBlockConfig are destroyed.
  *
  * @param bc A vector of pointers to BlockConfig.
  *
  * @param deleteold It indicates whether the currently stored BlockConfig
  *        for the Constraint (if any) must be destroyed. */

 void set_Config_Constraints( std::vector<BlockConfig *> && configs ,
                              bool deleteold = true ) {
  if( deleteold ) {
   for( auto sBCC : v_Config_Constraints )
    delete sBCC;
   }
  v_Config_Constraints = std::move( configs );
  }

/*--------------------------------------------------------------------------*/
 /// adds a BlockConfig of an "indirect sub-Block" of Constraint
 /** This function adds a (pointer to the) BlockConfig of an "indirect
  * sub-Block" associated with the Constraint of the Block whose
  * Block::ConstraintID is \p constraint_id. */

 void add_Config_Constraint( BlockConfig * config ,
                             Block::ConstraintID constraint_id ) {
  v_Config_Constraints.push_back( config );
  v_ConstraintID.push_back( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function sets the vector of ConstraintID, which indicates the set of
  * Constraint of the Block that have a BlockConfig for their inner
  * Block. */

 void set_ConstraintID( std::vector<Block::ConstraintID> && constraint_id ) {
  v_ConstraintID = std::move( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the BlockConfig of the "indirect sub-Block" of Objective
 /** This function sets the (pointer to the) BlockConfig of the "indirect
  * sub-Block" associated with the Objective of the Block. If the given
  * pointer is equal to the one currently stored in this ERBlockConfig, a call
  * to this function has no effect (no operation is performed; in particular,
  * no pointer is deleted). Otherwise, if \p deleteold is true then the
  * pointer currently stored in this ERBlockConfig is deleted, destroying the
  * previous BlockConfig for the Objective.
  *
  * @param config A pointer to the BlockConfig for the Objective.
  *
  * @param deleteold Indicates whether the previous BlockConfig for
  *        the Objective must be deleted. */

 void set_Config_Objective( BlockConfig * config = nullptr ,
                            bool deleteold = true ) {
  if( config == f_Config_Objective )
   return;
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/**@} ----------------------------------------------------------------------*/
/*------------ Methods for reading the data of the ERBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the ERBlockConfig
 *  @{ */

 /// returns the BlockConfig of "indirect sub-Block" of Constraint
 /** This function returns a const reference to the vector containing the
  * (pointer to the) BlockConfig of the "indirect sub-Block" associated with
  * the Constraint of the Block. */

 const std::vector<BlockConfig *> & get_Config_Constraints( void ) const {
  return( v_Config_Constraints );
  }

/*--------------------------------------------------------------------------*/
 /// returns the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function returns the vector of ConstraintID, which indicates the set
  * of Constraint of the Block that have a BlockConfig for their inner
  * Block. */

 const std::vector<Block::ConstraintID> & get_ConstraintID( void ) const {
  return( v_ConstraintID );
  }

/*--------------------------------------------------------------------------*/
 /// returns the BlockConfig of the "indirect sub-Block" of Objective
 /** This function returns the (pointer to the) BlockConfig of the
  *  "indirect sub-Block" associated with the Objective of the Block. */

 BlockConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the ERBlockConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this ERBlockConfig out of an istream
 /** Load this ERBlockConfig out of an istream. The format is defined as that
  * specified in RBlockConfig::load(), followed by:
  *
  * - the number k of the BlockConfig for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   = two integers representing the ConstraintID for the Constraint
  *   = a string containing the class type of a BlockConfig object,
  *     '*' means none (nullptr)
  *   = if the above is not '*', the description of the :BlockConfig
  *     object
  *   (clearly, if k == 0 this is empty)
  *
  * - a string containing the class type of a BlockConfig object for the
  *   Objective, '*' means none (nullptr)
  * - if the above is not '*', the description of the :BlockConfig object
  *   for the Objective. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a BlockConfig
  * for their inner Block. */
 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) BlockConfig for Constraint
 /** The vector of (pointer to the) BlockConfig for Constraint. The i-th
  * BlockConfig in this vector is that of the Block associated with the
  * Constraint identified by the i-th element in the vector v_ConstraintID. */
 std::vector<BlockConfig *> v_Config_Constraints;

 /// the (pointer to the) BlockConfig for the Objective of the Block
 BlockConfig * f_Config_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ERBlockConfig ) )

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

/*--------------------------------------------------------------------------*/
/*------------------------ File RBlockConfig.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class RBlockConfig,
 *
 * \version 0.33
 *
 * \date 15 - 07 - 2020
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
#include "Configuration.h"
#include "SMSTypedefs.h"

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
 *  @{ */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Configuration for configuring the sub-Block of a Block
/** The RBlockConfig class ("recursive" BlockConfig) derives from
 * Configuration and offers support for configuring the sub-Block of a Block
 * (recursively). The RBlockConfig contains the following field:
 *
 * - a pointer to a BlockConfig
 *
 * - a vector of pointers to Configuration for each of the sub-Block of the
 *   Block. */

class RBlockConfig : public Configuration
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

 RBlockConfig( void ) : Configuration() , f_BlockConfig( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 RBlockConfig( const RBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a RBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should contain the following:
  *
  * - the group "BlockConfig" containing the description of a BlockConfig
  *   object for the Block. This group is optional; if it is not provided,
  *   then the pointer to the BlockConfig for the Block is considered to be a
  *   nullptr (default configuration);
  *
  * - the dimension "n_sub_Block" containing the number of Configuration
  *   descriptions for the sub-Block of the current Block; this dimension is
  *   optional; if it is not provided, then n_sub_Block = 0 is assumed.
  *
  * - with n being the size of n_sub_Block, n groups, with name
  *   "sub-BlockConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a Configuration for one of the sub-Block of the current
  *   :Block. Each of these groups is optional. If a group is absent then the
  *   pointer to the Configuration for the corresponding sub-Block is
  *   considered to be a nullptr (default configuration).
  *
  * Note that that the matching between the sub-Configuration and the
  * sub-Block is positional: the Configuration found in the group
  * "sub-BlockConfig_<i>" is that for the i-th sub-Block. Note that the
  * vector of sub-Configuration is allowed to be of different size than the
  * number of sub-Block; if it is larger any extra Configuration is simply
  * ignored, if it shorted then all missing sub-Configuration are treated as
  * nullptr (default configuration). */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes the BlockConfig and all sub-BlockConfig
 virtual ~RBlockConfig()
 {
  delete f_BlockConfig;
  for( auto sBC : v_sub_BlockConfig )
   delete sBC;
  }

/*------------------------------- CLONE -----------------------------------*/

 virtual RBlockConfig * clone( void ) const override
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

 virtual void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*---------------- METHODS FOR MODIFYING THE RBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the RBlockConfig
 *  @{ */

 /// sets the (pointer to) the BlockConfig of the Block
 /** This function sets the pointer to the BlockConfig of the Block. If the
  * given pointer is equal to the one currently stored in this RBlockConfig, a
  * call to this function has no effect (no operation is performed; in
  * particular, no pointer is deleted). Otherwise, if \p deleteold is true
  * then the pointer currently stored in this RBlockConfig is deleted,
  * destroying the previous BlockConfig. This means that a call to
  * set_BlockConfig() (with all parameters taking their default values)
  * deletes the pointer to the BlockConfig stored in this RBlockConfig.
  *
  * @param bc A pointer to a BlockConfig.
  *
  * @param deleteold It indicates whether the currently stored BlockConfig (if
  *        any) must be destroyed. */

 void set_BlockConfig( BlockConfig * bc = nullptr , bool deleteold = true ) {
  if( bc == f_BlockConfig )
   return;
  if( deleteold )
   delete f_BlockConfig;
  f_BlockConfig = bc;
  }

/*--------------------------------------------------------------------------*/

 /// sets the (pointer to) the Configuration of each sub-Block
 /** This function sets the vector containing the (pointer to) the
  * Configuration of every sub-Block. If \p deleteold is true then all
  * Configuration for the sub-Block currently stored in this RBlockConfig are
  * destroyed.
  *
  * @param bc A vector of pointers to Configuration.
  *
  * @param deleteold It indicates whether the currently stored Configuration
  *        for the sub-Block (if any) must be destroyed. */

 void set_sub_BlockConfig( std::vector<Configuration *> && bc ,
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

 /// returns the (pointer to) the Configuration of every sub-Block
 /** This function returns a const reference to the vector containing the
  * (pointer to) the Configuration of every sub-Block. */

 const std::vector<Configuration *> & get_sub_BlockConfig( void )
  const { return( v_sub_BlockConfig ); }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the RBlockConfig
 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this RBlockConfig out of an istream
 /** Load this RBlockConfig out of an istream. The format is defined as
  * follows:
  *
  * - a string containing the class type of a BlockConfig object, '*' means
  *   none (nullptr)
  *
  * - if the above is not '*', the description of the :BlockConfig object for
  *   the Block
  *
  * number k of the sub-Configuration objects
  *
  * for i = 1 ... k
  *  - a string containing the class type of a Configuration object, '*' means
  *    none (nullptr)
  *
  *  - if the above is not '*', the description of the :Configuration object
  */

 virtual void load( std::istream &input ) override;

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 /// A pointer to the BlockConfig of the Block
 BlockConfig * f_BlockConfig;

 /// the vector of sub-Configuration for each of the sub-Block of the Block
 std::vector<Configuration *> v_sub_BlockConfig;

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
 * Configuration. These Constraints can be indicated by means of a
 * Block::ConstraintID.
 *
 * The ERBlockSolverConfig contains the following fields:
 *
 * - a vector of Block::ConstraintID indicating the set of Constraint of this
 *   Block that needs a Configuration alongside a vector of Configuration for
 *   those Constraint.
 *
 * - a Configuration for the Objective of the Block. */

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

 ERBlockConfig( void ) : RBlockConfig() , f_Config_Objective( nullptr ) {}

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 ERBlockConfig( const ERBlockConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of an ERBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, and all that is needed to describe a RBlockConfig, the
  * group should also contain the following:
  *
  * - the dimension "n_Config_Constraint" containing the number of
  *   Configuration descriptions associated with the Constraint of the current
  *   Block; this dimension is optional; if it is not provided, then
  *   n_Config_Constraint = 0 is assumed.
  *
  * - with p being the size of "n_Config_Constraint", a one-dimensional
  *   variable called "ConstraintID", of size 2p and type netCDF::ncUint,
  *   containing the Block::ConstraintID of the Constraint that need a
  *   Configuration: for each i = 0, ..., p - 1, the pair ( ConstraintID[ 2i ],
  *   ConstraintID[ 2i + 1] ) is the ConstraintID of the i-th Constraint that
  *   needs a Configuration, i.e., ConstraintID[ 2i ] provides the index of the
  *   group to which the i-th Constraint belongs and ConstraintID[ 2i + 1 ]
  *   provides the index of the i-th Constraint (see Block::ConstraintID for
  *   the definition of an index of a Constraint); this variable is mandatory
  *   if n_Config_Constraint > 0.
  *
  * - p groups, with name "Config_Constraint_<i>" for all i = 0, ..., p - 1,
  *   containing each the description of a Configuration associated with the
  *   i-th Constraint indicated by the "ConstraintID" variable (which is given
  *   by the pair ( ConstraintID[ 2i ], ConstraintID[ 2i + 1] )); these groups
  *   are optional; if "Config_Constraint_<i>" is not provided, then nullptr
  *   (default configuration) is assumed for the i-th Constraint;
  *
  * - a group with name "Config_Objective", containing the description of a
  *   Configuration associated with the Objective of the current Block; this
  *   group is optional; if it is not provided, then nullptr (default
  *   configuration) is assumed. */

 virtual void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor: deletes all sub-BlockConfig
 virtual ~ERBlockConfig()
 {
  for( auto sBCC : v_Config_Constraints )
   delete sBCC;

  delete f_Config_Objective;
  }

/*------------------------------- CLONE -----------------------------------*/

 virtual ERBlockConfig * clone( void ) const override
 {
  return( new ERBlockConfig( *this ) );
  }

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

 virtual void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*--------------- METHODS FOR MODIFYING THE ERBlockConfig ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the ERBlockConfig
 *  @{ */

 /// sets the Configuration of the "indirect sub-Block" of Constraint
 /** This function sets the vector containing the (pointer to the)
  * Configuration of the "indirect sub-Block" associated with the Constraint
  * of the Block. The i-th Configuration in this vector is associated with the
  * Constraint given by the i-th element of the ConstraintID vector (see
  * get_ConstraintID()). If \p deleteold is true then all Configuration for
  * the Constraint currently stored in this ERBlockConfig are destroyed.
  *
  * @param bc A vector of pointers to Configuration.
  *
  * @param deleteold It indicates whether the currently stored Configuration
  *        for the Constraint (if any) must be destroyed. */

 void set_Config_Constraints( std::vector<Configuration *> && configs ,
                              bool deleteold = true ) {
  if( deleteold ) {
   for( auto sBCC : v_Config_Constraints )
    delete sBCC;
   }
  v_Config_Constraints = std::move( configs );
  }

/*--------------------------------------------------------------------------*/
 /// adds a Configuration of an "indirect sub-Block" of Constraint
 /** This function adds a (pointer to the) Configuration of an "indirect
  * sub-Block" associated with the Constraint of the Block whose
  * Block::ConstraintID is \p constraint_id. */

 void add_Config_Constraint( Configuration * config ,
                             Block::ConstraintID constraint_id ) {
  v_Config_Constraints.push_back( config );
  v_ConstraintID.push_back( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function sets the vector of ConstraintID, which indicates the set of
  * Constraint of the Block that have a Configuration for their inner
  * Block. */

 void set_ConstraintID( std::vector<Block::ConstraintID> && constraint_id ) {
  v_ConstraintID = std::move( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the Configuration of the "indirect sub-Block" of Objective
 /** This function sets the (pointer to the) Configuration of the "indirect
  * sub-Block" associated with the Objective of the Block. If the given
  * pointer is equal to the one currently stored in this ERBlockConfig, a call
  * to this function has no effect (no operation is performed; in particular,
  * no pointer is deleted). Otherwise, if \p deleteold is true then the
  * pointer currently stored in this ERBlockConfig is deleted, destroying the
  * previous Configuration for the Objective.
  *
  * @param config A pointer to the Configuration for the Objective.
  *
  * @param deleteold Indicates whether the previous Configuration for
  *        the Objective must be deleted. */

 void set_Config_Objective( Configuration * config = nullptr ,
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

 /// returns the Configuration of "indirect sub-Block" of Constraint
 /** This function returns a const reference to the vector containing the
  * (pointer to the) Configuration of the "indirect sub-Block" associated with
  * the Constraint of the Block. */

 const std::vector<Configuration *> & get_Config_Constraints( void ) const {
  return( v_Config_Constraints );
  }

/*--------------------------------------------------------------------------*/
 /// returns the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function returns the vector of ConstraintID, which indicates the set
  * of Constraint of the Block that have a Configuration for their inner
  * Block. */

 const std::vector<Block::ConstraintID> & get_ConstraintID( void ) const {
  return( v_ConstraintID );
  }

/*--------------------------------------------------------------------------*/
 /// returns the Configuration of the "indirect sub-Block" of Objective
 /** This function returns the (pointer to the) Configuration of the
  *  "indirect sub-Block" associated with the Objective of the Block. */

 Configuration * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the ERBlockConfig
 virtual void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this ERBlockConfig out of an istream
 /** Load this ERBlockConfig out of an istream. The format is defined as that
  * specified in RBlockConfig::load(), followed by:
  *
  * - the number k of the Configuration for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   = two integers representing the ConstraintID for the Constraint
  *   = a string containing the class type of a Configuration object,
  *     '*' means none (nullptr)
  *   = if the above is not '*', the description of the :Configuration
  *     object
  *   (clearly, if k == 0 this is empty)
  *
  * - a string containing the class type of a Configuration object for the
  *   Objective, '*' means none (nullptr)
  * - if the above is not '*', the description of the :Configuration object
  *   for the Objective. */

 virtual void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a Configuration
  * for their inner Block. */
 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) Configuration for Constraint
 /** The vector of (pointer to the) Configuration for Constraint. The i-th
  * Configuration in this vector is that of the Block associated with the
  * Constraint identified by the i-th element in the vector v_ConstraintID. */
 std::vector<Configuration *> v_Config_Constraints;

 /// the (pointer to the) Configuration for the Objective of the Block
 Configuration * f_Config_Objective = nullptr;

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

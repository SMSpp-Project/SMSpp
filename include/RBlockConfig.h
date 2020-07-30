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

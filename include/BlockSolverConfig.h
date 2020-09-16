/*--------------------------------------------------------------------------*/
/*---------------------- File BlockSolverConfig.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *BlockSolverConfig classes, derived from Configuration,
 * which are intended as useful tools to configure in one blow all the Solver
 * of a given Block. Two classes are defined:
 *
 * - BlockSolverConfig : Configuration, used to only configure the Solver
 *   directly registered to the Block;
 *
 * - RBlockSolverConfig : BlockSolverConfig ("recursive" BlockSolverConfig),
 *   which also configure (potentially) all sub-Block (recursively) of the
 *   given Block;
 *
 * \version 0.33
 *
 * \date 15 - 09 - 2020
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
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BlockSolverConfig
#define __BlockSolverConfig
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Block.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------- BlockSolverConfig-RELATED TYPES ----------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BlockSolverConfig_TYPES BlockSolverConfig-related types
 *  @{ */

 using Index = Block::Index;

/** @}  end( group( BlockSolverConfig_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BlockSolverConfig_CLASSES Classes in BlockSolverConfig.h
 *
 * Three *BlockSolverConfig classes are defined that allow to handle more and
 * more complex cases, at the cost of more memory, a more complex input, and
 * potentially a higher computational cost for some operations:
 *
 * - BlockSolverConfig : Configuration, used to only configure the Solver
 *   directly registered to the Block;
 *
 * - RBlockSolverConfig : BlockSolverConfig ("recursive" BlockSolverConfig),
 *   which also configure (potentially) all sub-Block (recursively) of the
 *   given Block;
 *
 * The destruction (as well as the construction) of the Solver attached to a
 * Block is not a responsibility of the Block and is not automatically
 * performed when the Block is destructed (or constructed). Unregistering and
 * deleting some (or all) Solver attached to a Block and those attached to its
 * sub-Block, recursively, is a procedure that commonly appear right before a
 * Block is destroyed, when those Solver are no longer needed. Although this
 * or similar procedures can be implemented by the user, tailored to their own
 * needs, the BlockSolverConfig provides a convenient way to accomplish this
 * particular (and probably frequent) task.
 *
 * If an appropriate BlockSolverConfig for a Block is available, then it can
 * be used to perform this task by simply invoking clear() and then apply(),
 * passing a pointer to the Block as argument. This will unregisters and
 * deletes all the Solver attached to the Block and those attached to the
 * sub-Block (recursively). By appropriate BlockSolverConfig, we mean one that
 * covers all Solver attached to the Block and to its sub-Block,
 * recursively. If all the Solver of the Block have been created by means of a
 * single BlockSolverConfig, then that specific BlockSolverConfig is clearly
 * appropriate.
 *
 * Indeed, consider the most obvious use case: a Block is created, Solver
 * are attached, the Block is solved, and then everything is deleted. This
 * can be easily performed by the following pseudo-code:
 *
 *     Block * myBlock = < some way to create it, say a netCDF file >
 *     BlockSolverConfig * myBSC = < some way to create it, say a netCDF file >
 *     myBSC->apply( myBlock );
 *     myBSC->clear();
 *     < solve the Block with the created Solver >
 *     myBSC->apply( myBlock );
 *     delete myBSC;
 *     delete myBlock;
 *
 * Note that if myBlock has Solver attached to the sub-Block then the myBSC
 * object needs to be of class RBlockSolverConfig (this is not difficult to do
 * via the factory).
 *
 * More complex use cases will require adapting, but still BlockSolverConfig
 * can be useful. For instance, if one needs to solve many Block with the same
 * structure (different instances of the same problem) sequentially with the
 * same Solver configuration, then it can define myBSC only once and use it
 * for all the Block; only, in this case it must not be clear()-ed. Also, if
 * some Solver have to be added/deleted during the solution process, it is
 * possible to use myBSC to do that; by keeping myBSC "up to date" with the
 * position of all Solver in the Block, it is possible to clear them all with
 * a call to clear() and then to apply().
 *
 * If an appropriate, up-to-date BlockSolverConfig is not available, one can
 * be constructed as follows:
 *
 *     RBlockSolverConfig myBSC( myBlock );
 *
 * where myBlock is a pointer to the Block of interest. This constructs the
 * full BlockSolverConfig of the given Block; then,
 *
 *     myBSC.clear();
 *     myBSC.apply( myBlock );
 *
 * does the trick. The construction of an RBlockSolverConfig out of a Block,
 * which is what the above constructor does, is a reasonably cheap operation,
 * especially if clear == true. It requires, however, scanning all the
 * sub-Block of the given Block, recursively.
 *
 * If the user is positive that the Block has no sub-Block or if there aren't
 * solver attached to its sub-Block, then using BlockSolverConfig is cheaper.
 *
 * Alternatively, if myBlock is of a specific type that depend on a Block that
 * lives in specific locations, that cannot be reached by a path of sub-Block,
 * then the user may want to define an appropriate myRBlockSolverConfig :
 * [R]BlockSolverConfig that caters for that specific structure.
 *
 * Finally, note that if the only purpose of the thusly constructed
 * [R]BlockSolverConfig is to unregister the Solver of the Block (and those
 * attached to its sub-Block, recursively), then a "cleared"
 * RBlockSolverConfig (one that contains no ComputeConfig for the Solver) can
 * be directly constructed by
 *
 *     RBlockSolverConfig myBSC( block , false , true );
 *
 * This is clearly smarter than constructing a "full" RBlockSolverConfig, only
 * to clear() it immediately afterwards. Of course, such a "cleared"
 * RBlockSolverConfig is likely only useful to call apply() to unregister the
 * Solver.
 * @{ */

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BlockSolverConfig --------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from Configuration for configuring the Solver of the Block
/** Derived class from Configuration to configure in one blow all the Solver
 * *directly registered with* a given Block, each with all its algorithmic
 * parameters.
 *
 * The BlockSolverConfig contains the following fields:
 *
 * - a vector of strings containing the names of Solver to be attached to
 *   the Block;
 *
 * - a vector of ComputeConfig* for these same Solver.
 *
 * Crucially, a BlockSolverConfig can be used in two different ways:
 * "setting mode" and "differential mode". How the current object has to be
 * interpreted is specified by the field #f_diff. A full description of the
 * difference between the two modes is provided in the comments to the
 * apply() method; however, the general gist is that in "setting mode"
 * (#f_diff == false) the Solver previously registered with the Block are
 * un-registered and destroyed, and/or their current algorithmic parameters
 * are completely reset with those in the provided ComputeConfig. Instead,
 * in "differential mode" (#f_diff == true), all Solver whose name is not
 * specified (empty string) are left in their current state, and all nullptr
 * ComputeConfig correspond to not changing the configuration of the Solver.
 * Note that ComputeConfig objects themselves have a #f_diff field with the
 * same meaning, which means that a BlockSolverConfig in "differential mode"
 * coupled with ComputeConfig objects themselves in "differential mode" can
 * change any specific subset of the algorithmic parameters of the Solver
 * registered with the Block without affecting any of the other ones. */

class BlockSolverConfig : public Configuration
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING BlockSolverConfig --------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BlockSolverConfig
 *  @{ */

 /// constructor: creates an empty BlockSolverConfig
 /** It constructs an empty BlockSolverConfig, which can then be initialized
  * by calling the methods deserialize(), load() or get(), or by calls to
  * set_SolverNames(), set_SolverConfigs(), and set_diff(). The \p diff
  * parameter indicates whether this BlockSolverConfig must be considered as a
  * "differential" one. This parameter has true as default value, so that this
  * can be used as the void constructor.
  *
  * @param diff indicates if this configuration is a "differential" one. */

 BlockSolverConfig( bool diff = true ) : Configuration() , f_diff( diff ) {}

/*--------------------------------------------------------------------------*/
 /// constructs a BlockSolverConfig out of the given netCDF \p group
 /** It constructs a BlockSolverConfig out of the given netCDF \p group.
  * Please refer to the deserialize() method for the format of the
  * netCDF::NcGroup of a BlockSolverConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        BlockSolverConfig. */

 BlockSolverConfig( netCDF::NcGroup & group ) : Configuration() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs a BlockSolverConfig out of an istream
 /** It constructs a BlockSolverConfig out of the given istream \p input.
  * Please refer to the load() method for the format of a BlockSolverConfig.
  *
  * @param input The istream containing the description of the
  *        BlockSolverConfig. */

 BlockSolverConfig( std::istream &input ) : Configuration() { load( input ); }

/*--------------------------------------------------------------------------*/
 /// constructs a BlockSolverConfig for the given Block
 /** It constructs a BlockSolverConfig for the given \p block. It creates an
  * empty BlockSolverConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which a BlockSolverConfig will be
  *        constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  *
  * @param clear It indicates whether a "cleared" BlockSolverConfig is
  *        desired. See BlockSolverConfig:get() for details. */

 BlockSolverConfig( Block * block , bool diff = false , bool clear = false ) :
  Configuration() , f_diff( diff ) {  get( block , clear ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 BlockSolverConfig( const BlockSolverConfig &old );

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted
 BlockSolverConfig & operator=( const BlockSolverConfig & ) = delete;

/*--------------------------------------------------------------------------*/
 /// extends Configuration::deserialize( netCDF::NcFile ) to eProbFile
 /** Since a BlockSolverConfig knows it is a BlockSolverConfig, it "knows its
  * place" in an eProbFile netCDF SMS++ file. */

 static BlockSolverConfig * deserialize( netCDF::NcFile & f ,
                                         const unsigned int idx = 0 );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::deserialize( netCDF::NcGroup )
 /** Extends Configuration::deserialize( netCDF::NcGroup ) to the specific
  * format of a BlockSolverConfig. Besides the mandatory "type" attribute of
  * any :Configuration, the group should contain the following:
  *
  * - the attribute "diff" of netCDF::NcInt type containing the value for the
  *   #f_diff field (basically, a bool telling if the information in it has to
  *   be taken as "the configuration to be set" or as "the changes to be made
  *   from the current configuration"); this attribute is optional: if it is
  *   not provided, then diff = false is assumed;
  *
  * - the dimension "n_SolverConfig" containing the number of Solver that are
  *   to be attached to the Block, and therefore the number of their
  *   SolverConfig objects; this dimension is optional; if it is not provided,
  *   then n_SolverConfig = 0 is considered;
  *
  * - the variable "SolverNames", of type string and indexed over the
  *   dimension "n_SolverConfig"; the i-th entry of the variable is assumed to
  *   contain the classname of a :Solver object to be attached to the Block
  *   (this must be exact, i.e., exactly as returned by the protected virtual
  *   method Solver::classname(), since it is used in the factory when
  *   creating the object; this variable is mandatory if n_SolverConfig > 0;
  *
  * - with n being the size of "n_SolverConfig", n groups, with name
  *   "SolverConfig_<i>" for all i = 0, ..., n - 1, containing each the
  *   description of a ComputeConfig object for the i-th :Solver; these groups
  *   are optional; if "SolverConfig_<i>" is not provided, then nullptr is
  *   considered for the i-th ComputeConfig. */

 void deserialize( netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// destructor

 virtual ~BlockSolverConfig()
 {
  for( auto config : v_SolverConfigs )
   delete config;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the BlockSolverConfig of the given Block
 /** This method gets information about the current set of Solver attached to
  * the given Block and stores in this BlockSolverConfig. This information
  * consists of the names of the Solver attached to the given Block and the
  * ComputeConfig of these Solver. The \p clear parameter indicates whether
  * this BlockSolverConfig must be a "cleared" one, i.e., one whose vectors of
  * Solver names and ComputeConfig are empty. A "cleared" BlockSolverConfig is
  * useful to delete all the Solver registered to the Block (and although a
  * "cleared" BlockSolverConfig is quite simple, this is no longer true for
  * "cleared" objects of the derived class RBlockSolverConfig). Passing \p
  * clear = true is usually done when the sole purpose of using this
  * BlockSolverConfig is to reset the Solver of a Block (usually the given
  * Block \p block). The default value of this parameter is false, in which
  * case a "full" BlockSolverConfig is constructed.
  *
  * Note that BlockSolverConfig::get() is a reasonably cheap operation,
  * especially if clear == true, but this may not be true for all the
  * derived classes.
  *
  * @param block A pointer to the Block whose BlockSolverConfig must be
  *        filled.
  *
  * @param clear It indicates whether this BlockSolverConfig must be a clear
  *        one. */

 virtual void get( Block * block , bool clear = false );

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE BlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the BlockSolverConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// create and set all the Solver attached to the Block
 /** Method for creating, configuring and registering all the Solver that the
  * given Block may need. The configuration depends on the field #f_diff,
  * which indicates whether it has to be interpreted in "differential
  * mode". The behaviour of this method is the following:
  *
  * - First, the list of Solver registered to the given Block is scanned,
  *   and for each of them the corresponding elements in this
  *   BlockSolverConfig are examined. Then:
  *
  *   - If #f_diff == true
  *     - if the name of the Solver in this BlockSolverConfig is empty then
  *       the Solver is left there, otherwise the existing solver is
  *       un-registered and deleted and a new Solver is created and registered
  *       in that position
  *     - if the corresponding SolverConfig * is null then nothing is done,
  *       otherwise the SolverConfig * is passed to the Solver
  *
  *   - If #f_diff == false
  *     the existing Solver is un-registered and deleted, then
  *     - if the name of the Solver is empty in this BlockSolverConfig then
  *       the vector of registered Solver is shortened by one (any
  *       SolverConfig is ignored)
  *     - otherwise a new solver is created and registered in that position,
  *       and the corresponding SolverConfig is passed to it.
  *
  *   Note that this would seem to not allow completely resetting the
  *   configuration of some existing Solver without changing it, but this is
  *   not true: it is sufficient to pass it a SolverConfig object (hence, not
  *   nullptr) which is "empty" (no parameter set) but with its #f_diff field
  *   == false [see SolverConfig].
  *
  * - After the end of the list of currently registered Solver is reached (if
  *   ever), the behaviour is instead independent on the value of #f_diff
  *   (adding to nothing is setting). If the name of the Solver is empty then
  *   the entry of the vector is ignored, otherwise a new solver is created
  *   and registered in that position (at the end) and the corresponding
  *   SolverConfig is passed to it (unless it is nullptr, because setting a
  *   nullptr configuration to a newly minted solver is useless).
  *
  * Important note: the moment when the Block is passed to the Solver, the
  * Solver should in principle do all the necessary initializations, since
  * immediately afterwards compute() may be called already. However, some
  * of the initializations could be heavily impacted by the algorithmic
  * parameters of the Solver. This means that
  *
  *     IT IS EXPECTED THAT, IN A Solver, set_ComputeConfig() SHOULD BE
  *     CALLED *BEFORE* set_Block() IS
  *
  * This is in fact how this is done here inside.
  *
  * This method underlines the crucial difference between using a
  * BlockSolverConfig and directly using Block::register_Solver(),
  * Block::unregister_Solvers() and Block::replace_Solver() (which this
  * method uses on the user's behalf). In all the Block methods, it is
  * assumed that the new Solver have to be already constructed outside of
  * Block; consequently, the ones that get un-registered are *not* deleted,
  * since it is expected that this is to be done by whomever created them in
  * the first place outside the Block. BlockSolverConfig precisely provides
  * a single entity "outside of Block" that takes care of constructing the
  * Solver (using the Solver factory); correspondingly, each Solver that gets
  * un-registered by a BlockSolverConfig is also immediately deleted. While
  * the user is free to do whatever she wants, it is clear that
  *
  *     MIXING THE TWO STYLES OF MANAGING THE Solver, I.E., USING A
  *     BlockSolverConfig VS DIRECTLY CALLING Block::register_Solver(),
  *     Block::unregister_Solvers() AND Block::replace_Solver(), IS CLEARLY
  *     TRICKY AND CAUTION SHOULD BE EXERCISED.
  *
  * @param block A pointer to the Block that must be configured. */

 virtual void apply( Block * block ) const;

/*--------------------------------------------------------------------------*/
 /// delete all the ComputeConfig and empty the names of the Solver
 /** This method deletes all pointers to ComputeConfig stored in this
  * BlockSolverConfig and empties the names of the Solver. Morever, #f_diff is
  * set to false. */

 void clear( void ) override {
  set_diff( false );

  for( auto & config : v_SolverConfigs ) {
   delete config;
   config = nullptr;
   }

  for( auto & name : v_SolverNames ) {
   name.empty();
   }
  }

/*------------------------------- CLONE -----------------------------------*/

 BlockSolverConfig * clone( void ) const override
 {
  return( new BlockSolverConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE BlockSolverConfig ------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the BlockSolverConfig
 * @{ */

/*--------------------------------------------------------------------------*/
 /// "extends" Configuration::serialize( netCDF::NcFile , type ) to eProbFile
 /** Since a BlockSolverConfig knows it is a BlockSolverConfig, it "knows its
  * place" in an eProbFile netCDF SMS++ file. */

 void serialize( netCDF::NcFile & f , const int type ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends Configuration::serialize( netCDF::NcGroup )
 /** Extends Configuration::serialize( netCDF::NcGroup ) to the specific
  * format of a BlockSolverConfig. See
  * BlockSolverConfig::deserialize( netCDF::NcGroup ) for details of the
  * format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE BlockSolverConfig ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the BlockSolverConfig
 *  @{ */

 /// change the mode of this configuration
 /** This function changes the mode of this BlockSolverConfig. If \p diff is
  * true, then this BlockSolverConfig starts to be "interpreted in a
  * differential sense". */

 void set_diff( bool diff = true ) { f_diff = diff; }

/*--------------------------------------------------------------------------*/

 /// adds a ComputeConfig
 /** This function adds, to this BlockSolverConfig, a ComputeConfig for a
  * Solver of the Block.
  *
  * @param name The name of the Solver.
  *
  * @param solver_config A pointer to a ComputeConfig. */

 void add_ComputeConfig( std::string && name ,
                         ComputeConfig * solver_config = nullptr) {
  v_SolverConfigs.push_back( solver_config );
  v_SolverNames.push_back( std::move( name ) );
  }

/*--------------------------------------------------------------------------*/

 /// removes a ComputeConfig
 /** This function removes the ComputeConfig (and the corresponding name of
  * the Solver) at the given \p index.
  *
  * @param index The index of the ComputeConfig (and the name of the Solver)
  *              to be removed (this must be an index between 0 and
  *              num_ComputeConfig() - 1).
  *
  * @param destroy It indicates whether the pointer to the ComputeConfig at
  *        the given index must be deleted. */

 void remove_ComputeConfig( Index index , bool destroy = true ) {
  if( index >= v_SolverConfigs.size() )
   throw ( std::invalid_argument( "BlockSolverConfig::remove_"
                                  "ComputeConfig: invalid index: " +
                                  std::to_string( index ) + "." ) );
  if( destroy )
   delete v_SolverConfigs[ index ];
  v_SolverConfigs.erase( std::begin( v_SolverConfigs ) + index );
  v_SolverNames.erase( std::begin( v_SolverNames ) + index );
  }

/*--------------------------------------------------------------------------*/

 /// changes the name of a Solver
 /** This function changes the name of the Solver at the given \p index.
  *
  * @param index The index of the name of the Solver to be changed (this must
  *              be an index between 0 and num_ComputeConfig() - 1). */

 void set_Solver_name( Index index , std::string && name = "" ) {
  if( index >= v_SolverNames.size() )
   throw ( std::invalid_argument( "BlockSolverConfig::set_Solver_name: invalid "
                                  "index: " + std::to_string( index ) + "." ) );
  v_SolverNames[ index ] = std::move( name );
  }

/*--------------------------------------------------------------------------*/

 /// change a ComputeConfig
 /** This function replaces the ComputeConfig at the given \p index.
  *
  * @param index The index of the ComputeConfig to be replaced (this must be
  *              an index between 0 and num_ComputeConfig() - 1).
  *
  * @param destroy It indicates whether the pointer to the ComputeConfig at
  *        the given index must be deleted. */

 void set_ComputeConfig( Index index , ComputeConfig * config = nullptr ,
                         bool destroy = true ) {
  if( index >= v_SolverConfigs.size() )
   throw ( std::invalid_argument( "BlockSolverConfig::set_ComputeConfig: "
                                  "invalid index: "
                                  + std::to_string( index ) + "." ) );
  if( destroy )
   delete v_SolverConfigs[ index ];
  v_SolverConfigs[ index ] = config;
  }

/**@} ----------------------------------------------------------------------*/
/*---------- Methods for reading the data of the BlockSolverConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the BlockSolverConfig
 *  @{ */

 /// tells if the configuration is a "differential" one (reads #f_diff)

 bool is_diff( void ) const { return( f_diff ); }

/*--------------------------------------------------------------------------*/

 /// returns the number of ComputeConfig in this BlockSolverConfig
 /** This method returns the number of ComputeConfig currently being handled
  * by this BlockSolverConfig . */

 Block::Index num_ComputeConfig( void ) const {
  return( v_SolverConfigs.size() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the names of all Solver of the Block
 /** This function returns a const reference to the vector containing the
  * names of all Solver of the Block. */

 const std::vector<std::string> & get_SolverNames( void ) const {
  return( v_SolverNames );
  }

/*--------------------------------------------------------------------------*/
 /// returns the name of the Solver at the given position
 /** This function returns the name of the Solver at the given \p index.
  *
  * @param index An index between 0 and num_ComputeConfig() - 1.
  *
  * @return The name of the Solver at the given \p index. */

 const std::string & get_SolverName( Index index ) const {
  if( index >= v_SolverConfigs.size() )
   throw ( std::invalid_argument( "BlockSolverConfig::get_SolverName: invalid "
                                  "index: " + std::to_string( index ) + "." ) );
  return( v_SolverNames[ index ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (pointer to) the ComputeConfig of all Solver of the Block
 /** This function returns a const reference to the vector containing the
  * (pointer to) the ComputeConfig of all Solver of the Block. */

 const std::vector<ComputeConfig *> & get_SolverConfigs( void ) const {
  return( v_SolverConfigs );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (pointer to) the ComputeConfig at the given index
 /** This function returns the pointer to the ComputeConfig for the Solver
  * at the given \p index.
  *
  * @param index An index between 0 and num_ComputeConfig() - 1.
  *
  * @return The pointer to the ComputeConfig at the given \p index. */

 ComputeConfig * get_SolverConfig( Index index ) const {
  if( index >= v_SolverConfigs.size() )
   throw ( std::invalid_argument( "BlockSolverConfig::get_SolverConfig: "
                                  "invalid index: " +
                                  std::to_string( index ) + "." ) );
  return( v_SolverConfigs[ index ] );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the BlockSolverConfig
 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this BlockSolverConfig out of an istream
 /** Load this BlockSolverConfig out of an istream, with the format:
  *
  * - a binary number b to determine the value of #f_diff. If b == 0 then
  *   #f_diff = false, otherwise, #f_diff = true.
  *
  * - the number k of the names of Solver for the Block
  *
  * - for i = 1 ... k
  *   - a string containing the class type of a Solver object, '*' means
  *     none (empty string)
  *   (clearly, if k == 0 this is empty)
  *
  * - the number k of the ComputeConfig for the Solver for the Block
  *
  * - for i = 1 ... k
  *   - a string containing the class type of a ComputeConfig object,
  *    '*' means none (nullptr)
  *   - if the above is not '*', the description of the :ComputeConfig object
  *   (clearly, if k == 0 this is empty)
  */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 bool f_diff;  ///< tells if the configuration is a "differential" one

 /// the names of all Solver of the Block
 std::vector<std::string> v_SolverNames;

 /// (pointer to) the ComputeConfig of all Solver of the Block
 std::vector<ComputeConfig *> v_SolverConfigs;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( BlockSolverConfig ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS RBlockSolverConfig -------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockSolverConfig for configuring also sub-Block
/** Derived class from BlockSolverConfig to configure in one all the Solver of
 * a given Block, comprised those of the sub-Block (recursively), each with
 * all its algorithmic parameters.
 *
 * Besides the fields in BlockSolverConfig, the RBlockSolverConfig contains
 * the following fields:
 *
 * - a vector of BlockSolverConfig for each of the sub-Block of the Block;
 *
 * - a vector indicating which sub-Block have a BlockSolverConfig.
 *
 * As the last field above hints, the RBlockSolverConfig is able to handle a
 * subset of the sub-Block of the Block. This means that it does not need to
 * store a BlockSolverConfig for each sub-Block of the Block, but only those
 * that are needed.
 *
 * The meaning of the field #f_diff, inherited from BlockSolverConfig, is also
 * extended to the sub-Block: if #f_diff is true, then all nullptr
 * BlockSolverConfig correspond to not changing any of the configurations of
 * any of the Solver attached to the corresponding sub-Block. */

class RBlockSolverConfig : public BlockSolverConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*------------ CONSTRUCTING AND DESTRUCTING RBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing RBlockSolverConfig
 *  @{ */

 /// constructor: creates an empty RBlockSolverConfig
 /** It constructs an empty RBlockSolverConfig, which can then be initialized
  * by calling the methods deserialize(), load() or get(), or by calls to
  * set_SolverNames(), set_SolverConfigs(), set_BlockSolverConfigs(), and
  * set_diff().
  *
  * @param diff indicates if this configuration is a "differential" one. */

 RBlockSolverConfig( bool diff = true ) : BlockSolverConfig( diff ) { }

/*--------------------------------------------------------------------------*/
 /// constructs an RBlockSolverConfig out of the given netCDF \p group
 /** It constructs an RBlockSolverConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an RBlockSolverConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        RBlockSolverConfig. */

 RBlockSolverConfig( netCDF::NcGroup & group ) : BlockSolverConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an RBlockSolverConfig out of an istream
 /** It constructs an RBlockSolverConfig out of the given istream \p input.
  * Please refer to the load() method for the format of an RBlockSolverConfig.
  *
  * @param input The istream containing the description of the
  *        RBlockSolverConfig. */

 RBlockSolverConfig( std::istream &input ) : BlockSolverConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an RBlockSolverConfig for the given Block
 /** It constructs an RBlockSolverConfig for the given \p block. It creates an
  * empty RBlockSolverConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an RBlockSolverConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  *
  * @param clear It indicates whether a "cleared" RBlockSolverConfig is
  *        desired. See RBlockSolverConfig:get() for details. */

 RBlockSolverConfig( Block * block , bool diff = false ,
		     bool clear = false ) : BlockSolverConfig( diff ) {
  get( block , clear );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 RBlockSolverConfig( const RBlockSolverConfig &old );

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted
 RBlockSolverConfig & operator=( const RBlockSolverConfig & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockSolverConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockSolverConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of an RBlockSolverConfig. Besides the mandatory "type" attribute of
  * any :Configuration, and the dimensions and variables of a
  * BlockSolverConfig, the group should also contain the following:
  *
  * - the dimension "n_BlockSolverConfig" containing the number of
  *   BlockSolverConfig descriptions for the sub-Block of the current Block;
  *   it is optional; if it is not provided, then we assume
  *   n_BlockSolverConfig = 0.
  *
  * - with m being the size of "n_BlockSolverConfig", m groups, with name
  *   "BlockSolverConfig_<i>" for all i = 0, ..., m - 1, containing each
  *   the description of a BlockSolverConfig for one of the sub-Block of the
  *   current Block.
  *
  * - With n being the size of "n_BlockSolverConfig", a one-dimensional
  *   variable with name "sub-Block-id", of size n and type netCDF::NcString,
  *   containing the identification of the sub-Block such that
  *   "BlockSolverConfig_<i>" contains the BlockSolverConfig for the sub-Block
  *   whose identification is "sub-Block-id[ i ]" for all i = 0, ..., n -
  *   1. The identification of the sub-Block can be either its name (see
  *   Block::name()) or its index in the list of sub-Block of its father
  *   Block. This variable is optional. If it is not provided, then the i-th
  *   BlockSolverConfig is associated with the i-th sub-Block of the Block for
  *   all i = 0, ..., n - 1 (i.e., i is taken as the index of the sub-Block
  *   and "sub-Block-id[ i ]" is assumed to be "i").
  *
  *       IF THE NAME OF THE Block IS USED AS ITS IDENTIFICATION, THEN
  *       THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT.
  *
  * The individual groups "BlockSolverConfig_<i>" are optional. If
  * "BlockSolverConfig_<i>" is not provided, then nullptr is considered. Note
  * that the size of the vector of sub-BlockSolverConfig is allowed to be
  * different than the number of sub-Block. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor

 virtual ~RBlockSolverConfig()
 {
  for( auto config : v_BlockSolverConfig )
   delete config;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the RBlockSolverConfig of the given Block
 /** This method gets information about the current set of Solver attached to
  * the given Block (and its sub-Block, recursively) and stores in this
  * RBlockSolverConfig. This information consists of that supported by the
  * BlockSolverConfig (see BlockSolverConfig::get()) plus the
  * BlockSolverConfig of possibly all sub-Block of the given Block. If this
  * RBlockSolverConfig contains no BlockSolverConfig (i.e.,
  * num_BlockSolverConfig() returns zero) then all sub-Block of the given
  * Block are scanned and, at the end of this call, it will have stored a
  * BlockSolverConfig for each sub-Block of the given Block. If
  * num_BlockSolverConfig() returns non-zero, then only the BlockSolverConfig
  * of the sub-Block currently being handled by this RBlockSolverConfig are
  * considered. For each i in {0, ..., num_BlockSolverConfig() - 1},
  * get_sub_Block_id( i ) returns the identification of the i-th sub-Block
  * handled by this RBlockSolverConfig.
  *
  * Note that RBlockSolverConfig::get() is a reasonably cheap operation,
  * especially if clear == true, except, of course, for the fact that
  * (possibly) all the sub-Block of the given Block must be scanned,
  * recursively.
  *
  * @param block A pointer to the Block whose RBlockSolverConfig must be
  *        filled.
  *
  * @param clear It indicates whether this RBlockSolverConfig must be a clear
  *        one. See the comments to BlockSolverConfig::get() for the
  *        definition of this parameter. */

 void get( Block * block , bool clear = false ) override;

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the RBlockSolverConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// create and set all the Solver attached to the Block (and its sub-Block)
 /** Method for creating, configuring and registering all the Solver that the
  * given Block may need, including those of its sub-Block, recursively. This
  * method first invoke the method BlockSolverConfig::apply() and then
  * proceeds configuring the Solver of the sub-Block. This is done by calling
  * apply() on each sub-Block handled by this RBlockSolverConfig, recursively.
  *
  * @param block A pointer to the Block that must be configured. */

 void apply( Block * block ) const override;

/*--------------------------------------------------------------------------*/
 /// clear this RBlockSolverConfig
 /** This method first invokes BlockSolverConfig::clear(). Then, clear() is
  * invoked for each non-nullptr BlockSolverConfig * handled by this
  * RBlockSolverConfig (num_BlockSolverConfig() and
  * get_BlockSolverConfig()). */

 void clear( void ) override {
  BlockSolverConfig::clear();
  for( auto config : v_BlockSolverConfig )
   if( config )
    config->clear();
  }

/*------------------------------- CLONE -----------------------------------*/

 RBlockSolverConfig * clone( void ) const override
 {
  return( new RBlockSolverConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*------ METHODS FOR LOADING, PRINTING & SAVING THE RBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the RBlockSolverConfig
 * @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockSolverConfig::serialize( netCDF::NcGroup )
 /** Extends BlockSolverConfig::serialize( netCDF::NcGroup ) to the specific
  * format of an RBlockSolverConfig. See RBlockSolverConfig::deserialize(
  * netCDF::NcGroup ) for details of the format of the created netCDF
  * group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE RBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the RBlockSolverConfig
 *  @{ */

/*--------------------------------------------------------------------------*/

 /// adds a (pointer to a) BlockSolverConfig for a sub-Block
 /** This function adds a (pointer to a) BlockSolverConfig for the sub-Block
  * with the given \p id. The identification of a sub-Block is either its name
  * (see Block::name()) or its index in the list of nested Block of its father
  * Block.
  *
  * @param config A pointer to a BlockSolverConfig for the sub-Block with the
  *        given \p index.
  *
  * @param id The identification of a sub-Block. */

 void add_BlockSolverConfig( BlockSolverConfig * config , std::string id ) {
  v_BlockSolverConfig.push_back( config );
  v_sub_Block_id.push_back( id );
  }

/*--------------------------------------------------------------------------*/

 /// adds a (pointer to a) BlockSolverConfig for a sub-Block
 /** This function adds a (pointer to a) BlockSolverConfig for the sub-Block
  * with the given \p index. The index of a sub-Block is its index in the list
  * of nested Block of its father Block.
  *
  * @param config A pointer to a BlockSolverConfig for the sub-Block with the
  *        given \p index.
  *
  * @param index The index of a sub-Block.  */

 void add_BlockSolverConfig( BlockSolverConfig * config , Block::Index index ) {
  v_BlockSolverConfig.push_back( config );
  v_sub_Block_id.push_back( std::to_string( index ) );
  }

/*--------------------------------------------------------------------------*/

 /// removes a BlockSolverConfig for a sub-Block
 /** This function removes the BlockSolverConfig at the given \p index. Notice
  * that \p index is not the index of the sub-Block, but the index of the
  * BlockSolverConfig being handled by this RBlockSolverConfig.
  *
  * @param index The index of the BlockSolverConfig to be removed.
  *
  * @param destroy It indicates whether the pointer to the BlockSolverConfig
  *        at the given index must be deleted. */

 void remove_BlockSolverConfig( Block::Index index , bool destroy = true ) {
  if( index >= v_BlockSolverConfig.size() )
   throw ( std::invalid_argument( "RBlockSolverConfig::remove_"
                                  "BlockSolverConfig: invalid index: " +
                                  std::to_string( index ) + "." ) );
  if( destroy )
   delete v_BlockSolverConfig[ index ];
  v_BlockSolverConfig.erase( std::begin( v_BlockSolverConfig ) + index );
  v_sub_Block_id.erase( std::begin( v_sub_Block_id ) + index );
  }

/**@} ----------------------------------------------------------------------*/
/*---------- Methods for reading the data of the RBlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the RBlockSolverConfig
 *  @{ */

/*--------------------------------------------------------------------------*/

 /// returns the number of BlockSolverConfig in this RBlockSolverConfig
 /** This method returns the number of BlockSolverConfig (for the sub-Block)
  * currently being handled by this RBlockSolverConfig . */

 Block::Index num_BlockSolverConfig( void ) const {
  return v_BlockSolverConfig.size();
  }

/*--------------------------------------------------------------------------*/

 /// returns the (pointer to) the BlockSolverConfig at the given \p index
 /** This function returns the pointer to the BlockSolverConfig at the given
  * \p index.
  *
  * @param index The index of the BlockSolverConfig to be retrieved (an index
  *        between 0 and num_BlockSolverConfig() - 1).
  *
  * @return A pointer to the BlockSolverConfig at the given \p index. */

 BlockSolverConfig * get_BlockSolverConfig( Block::Index index ) const {
  if( index >= v_BlockSolverConfig.size() )
   throw ( std::invalid_argument( "RBlockSolverConfig::get_BlockSolverConfig: "
                                  "invalid index: " +
                                  std::to_string( index ) + "." ) );
  return( v_BlockSolverConfig[ index ] );
 }

/*--------------------------------------------------------------------------*/

 /// returns the id of the sub-Block associated with the given \p index
 /** This function returns the identification of the sub-Block whose
  * BlockSolverConfig is located at position \p index in this
  * RBlockSolverConfig. The identification of a sub-Block is either its name
  * (see Block::name()) or its index in the list of nested Block of its father
  * Block.
  *
  * @param index The index of a BlockSolverConfig in this RBlockSolverConfig
  *        (it must be an index between 0 and num_BlockSolverConfig() - 1).
  *
  * @return The identification of the sub-Block associated with the
  *         BlockSolverConfig located at the given \p index. */

 const std::string & get_sub_Block_id( Block::Index index ) const {
  if( index >= v_sub_Block_id.size() )
   throw ( std::invalid_argument( "RBlockSolverConfig::get_sub_Block_id: "
                                  "invalid index: "
                                  + std::to_string( index ) + "." ) );
  return( v_sub_Block_id[ index ] );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the RBlockSolverConfig

 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this RBlockSolverConfig out of an istream
 /** Load this RBlockSolverConfig out of an istream. The format is defined as
  * that specified in BlockSolverConfig::load(), followed by:
  *
  * - a number k such that abs(k) is the number of BlockSolverConfig objects
  *
  * for i = 1 ... abs(k)
  *  - if k < 0, the identification of the sub-Block
  *
  *  - a string containing the class type of a BlockSolverConfig object, '*'
  *    means none (nullptr)
  *
  *  - if the above is not '*', the description of the :BlockSolverConfig
  *    object
  *
  * Notice that the sign of k determines whether the identification of the
  * sub-Block must be provided. If k < 0, then the identification of the
  * sub-Block must be provided. The identification of the sub-Block can be
  * either its name (see Block::name()) or its index in the list of sub-Block
  * or its father Block. If k >= 0, then the identification of the sub-Block
  * must not be provided. In this case, the i-th BlockSolverConfig is
  * associated with the i-th sub-Block of the Block (i.e., i is taken as the
  * index of the sub-Block and v_sub_Block_id[ i ] = "i").
  *
  *     IF THE NAME OF THE Block IS USED AS ITS IDENTIFICATION, THEN
  *     THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT. */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of (pointer to) the sub-BlockSolverConfig for each sub-Block
 std::vector<BlockSolverConfig *> v_BlockSolverConfig;

 /// correspondence between v_BlockSolverConfig and the sub-Block of the Block
 /** This vector specifies the correspondence between the BlockSolverConfig in
  * #v_BlockSolverConfig and the sub-Block of the Block. v_sub_Block_id[ i ]
  * contains the identification of the sub-Block whose BlockSolverConfig is
  * v_BlockSolverConfig[ i ]. A sub-Block can be identified either by its name
  * (see Block::name()) or by its index in the list of sub-Blocks of its
  * father Block. If the name of the sub-Block is used, then the first
  * character of this name cannot be a digit. */
 std::vector<std::string> v_sub_Block_id;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( RBlockSolverConfig ) )

/** @}  end( group( BlockSolverConfig_CLASSES ) ) */

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BlockSolverConfig.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File BlockSolverConfig.h -----------------------*/
/*--------------------------------------------------------------------------*/

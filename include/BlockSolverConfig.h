/*--------------------------------------------------------------------------*/
/*---------------------- File BlockSolverConfig.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *BlockSolverConfig classes, derived from Configuration,
 * which are intended as a useful tools to configure in one blow all the
 * Solver of a given Block. Three classes are defined:
 *
 * - BlockSolverConfig : Configuration, used to only configure the Solver
 *   directly registered to the Block;
 *
 * - RBlockSolverConfig : BlockSolverConfig ("recursive" BlockSolverConfig),
 *   which also configure (potentially) all sub-Block (recursively) of the
 *   given Block;
 *
 * - ERBlockSolverConfig : RBlockSolverConfig ("extended" RBlockSolverConfig),
 *   which also configure all "indirect sub-Block" (Block that may be part of
 *   the Objective or Constraint) of a given Block. In the current
 *   implementation, these are the inner Block of either a LagBFunction or a
 *   BendersBFunction occurring as the Function in either a FRealObjective or
 *   a FRowConstraint.
 *
 * \version 0.33
 *
 * \date 16 - 06 - 2020
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
#include "Configuration.h"
#include "SMSTypedefs.h"
#include "Solver.h"

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
 * - ERBlockSolverConfig : RBlockSolverConfig ("extended" RBlockSolverConfig),
 *   which also configure all "indirect sub-Block" (Block that may be part of
 *   the Objective or Constraint) of a given Block. In the current
 *   implementation, these are the inner Block of of either a LagBFunction or
 *   a BendersBFunction occurring as the Function in either a FRealObjective
 *   or a FRowConstraint.
 *
 * Basically, ERBlockSolverConfig is the "complete" class that covers all the
 * bases, whereas the other two classes are cheaper and simpler versions for
 * when the full-fledged generality of ERBlockSolverConfig is not needed.
 *
 * The destruction (as well as the construction) of the Solver attached to a
 * Block is not a responsibility of the Block and is not automatically
 * performed when the Block is destructed (or constructed). Unregistering and
 * deleting all Solver attached to a Block and those attached to its sub-Block
 * and "indirect sub-Block", recursively, is a procedure that commonly appear
 * right before a Block is destroyed, when those Solver are no longer
 * needed. Although this or similar procedures can be implemented by the user,
 * tailored to their own needs, the BlockSolverConfig provides a convenient
 * way to accomplish this particular (and probably frequent) task.
 *
 * If an appropriate BlockSolverConfig for a Block is available, then it can
 * be used to perform this task by simply invoking reset_Solver(), passing a
 * pointer to the Block as argument. This will unregister and delete all the
 * Solver attached to the Block and those attached to the sub-Block and
 * "indirect sub-Block" (recursively). By appropriate BlockSolverConfig, we
 * mean one that covers all Solver attached to the Block and to its
 * ("indirect") sub-Block, recursively. If all the Solver of the Block have
 * been created by means of a single BlockSolverConfig, then that specific
 * BlockSolverConfig is clearly appropriate, even if clear() has been called
 * for it.
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
 *     myBSC->reset_Solver( myBlock );
 *     delete myBSC;
 *     delete myBlock;
 *
 * The myBSC->clear() is not mandatory, but it will free all the memory in
 * the BlockSolverConfig that is not needed for reset_Solver() to work.
 *
 * Note that if myBlock has Solver attached to the sub-Block then the myBSC
 * object needs be of class RBlockSolverConfig, and if it also has "indirect
 * sub-Block" then it must be of class ERBlockSolverConfig (this is not
 * difficult to do via the factory).
 *
 * More complex use cases will require adapting, but still BlockSolverConfig
 * can be useful. For instance, if one needs to solve many Block with the
 * same structure (different instances of the same problem) sequentially with
 * the same Solver configuration, then it can define myBSC only once and use
 * it for all the Block; only, in this case it must not be clear()-ed. Also,
 * if some Solver have to be added/deleted during the solution process, it is
 * possible to use myBSC to do that; by keeping myBSC "up to date" with the
 * position of all Solver in the Block, it is possible to clear them all with
 * a single call to reset_Solver() (note thay myBSC can also be used to change
 * the SolverConfig of the Block, but doing so has no effect on reset_Solver()).
 *
 * If an appropriate, up-to-date BlockSolverConfig is not available, one can be
 * constructed as follows:
 *
 *     ERBlockSolverConfig myBSC( myBlock );
 *
 * where block is a pointer to the Block of interest. This constructs the full
 * BlockSolverConfig of the given Block; then,
 *
 *     myBSC.reset_Solver( myBlock );
 *
 * does the trick. However, note that
 *
 *     CALLING ERBlockSolverConfig::get(), WHICH IS WHAT THE ABOVE CONSTRUCTOR
 *     DOES, IS A POTENTIALLY COSTLY OPERATION BECAUSE IT ENTAILS SCANNING ALL
 *     Constraint AND Objective OF THE Block, AND ALL ITS sub-Block
 *     RECURSIVELY, IN ORDER TO FIND THE "INDIRECT" sub-Block.
 *
 * If the user is positve that the Block has no "indirect" sub-Block, then
 * using RBlockSolverConfig is cheaper, and similarly for BlockSolverConfig
 * if there aren't solver attached even to "normal" sub-Block.
 *
 * Alternatively, if myBlock is of a specific type where the "indirect"
 * sub-Block can only be in specific locations, then the user may want to
 * define an appropriate myRBlockSolverConfig : [R]BlockSolverConfig that
 * caters for that specific structure.
 *
 * Finally, note that if the only purpose of the thusly constructed
 * [ER]BlockSolverConfig is to reset the Solver of the Block (and those
 * attached to its sub-Block and "indirect sub-Block", recursively), then a
 * "cleared" ERBlockSolverConfig (one that contains no ComputeConfig for the
 * Solver) can be directly constructed by
 *
 *     ERBlockSolverConfig myBSC( block , false , true );
 *
 * This is clearly smarter than constructing a "full" ERBlockSolverConfig,
 * only to clear() it immediately afterwards. Of course, such a "cleared"
 * ERBlockSolverConfig is likely only useful to call reset_Solver().
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
  *   f_diff field (basically, a bool telling if the information in it has to
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
  for( auto sSC : v_SolverConfigs )
   delete sSC;
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
  * this BlockSolverConfig must be a "cleared" one, i.e., one whose vectors
  * of Solver names and ComputeConfig are empty. A "cleared"
  * BlockSolverConfig is useful to delete all the Solver registered to the
  * Block (and although a "cleared" BlockSolverConfig is quite simple, this
  * is no longer true for "cleared" objects of the derived classes
  * EBlockSolverConfig and ERBlockSolverConfig). Passing \p clear = true is
  * usually done when the sole purpose of using this BlockSolverConfig is to
  * reset the Solver of a Block (usually the given Block \p block). The
  * default value of this parameter is false, in which case a "full"
  * BlockSolverConfig is constructed. 
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
  *   = If #f_diff == true
  *     * if the name of the Solver in this BlockSolverConfig is empty then
  *       the Solver is left there, otherwise the existing solver is
  *       un-registered and deleted and a new Solver is created and registered
  *       in that position
  *     * if the corresponding SolverConfig * is null then nothing is done,
  *       otherwise the SolverConfig * is passed to the Solver
  *
  *   = If #f_diff == false
  *     the existing Solver is un-registered and deleted, then
  *     * if the name of the Solver is empty in this BlockSolverConfig then
  *       the vector of registered Solver is shortened by one (any
  *       SolverConfig is ignored)
  *     * otherwise a new solver is created and registered in that position,
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
  * Block::unregister_Solver() and Block::replace_Solver() (which this
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
  *     Block::unregister_Solver() AND Block::replace_Solver(), IS CLEARLY
  *     TRICKY AND CAUTION SHOULD BE EXERCISED.
  *
  * @param block A pointer to the Block that must be configured. */

 virtual void apply( Block * block ) const;

/*--------------------------------------------------------------------------*/
 /// unregister and delete all the Solver attached to the given Block
 /** This method unregisters and deletes all Solver attached to the given
  * Block.
  *
  * @param block A pointer to the Block whose Solver will be reset. */

 virtual void reset_Solver( Block * block ) const;

/*--------------------------------------------------------------------------*/
 /// clear the names of the Solver and delete all the ComputeConfig
 /** This method clears the vector holding the names of the Solver and
  * deletes all the ComputeConfig (clearing also the vector holding the
  * ComputeConfig). */

 virtual void clear( void ) {
  v_SolverNames.clear();
  v_SolverNames.shrink_to_fit();

  for( auto sSC : v_SolverConfigs )
   delete sSC;

  v_SolverConfigs.clear();
  v_SolverConfigs.shrink_to_fit();
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
 /// sets the names of all Solver of the Block
 /** This function sets the vector containing the names of all Solver of the
  * Block. */

 void set_SolverNames( std::vector<std::string> && solver_names ) {
  v_SolverNames = std::move( solver_names );
  }

/*--------------------------------------------------------------------------*/
 /// sets the (pointer to) the ComputeConfig of all Solver of the Block
 /** This function sets the vector containing the (pointer to) the
  * ComputeConfig of all Solver of the Block. */

 void set_SolverConfigs( std::vector<ComputeConfig *> && solver_configs ) {
  v_SolverConfigs = std::move( solver_configs );
  }

/**@} ----------------------------------------------------------------------*/
/*---------- Methods for reading the data of the BlockSolverConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the BlockSolverConfig
 *  @{ */

 /// tells if the configuration is a "differential" one (reads #f_diff)

 bool is_diff( void ) const { return( f_diff ); }

/*--------------------------------------------------------------------------*/
 /// returns the names of all Solver of the Block
 /** This function returns a const reference to the vector containing the
  * names of all Solver of the Block. */

 const std::vector<std::string> & get_SolverNames( void ) const {
  return( v_SolverNames );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (pointer to) the ComputeConfig of all Solver of the Block
 /** This function returns a const reference to the vector containing the
  * (pointer to) the ComputeConfig of all Solver of the Block. */

 const std::vector<ComputeConfig *> & get_SolverConfigs( void ) const {
  return( v_SolverConfigs );
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
  *   = a string containing the class type of a Solver object, '*' means
  *     none (nullptr)
  *   (clearly, if k == 0 this is empty)
  *
  * - the number k of the ComputeConfig for the Solver for the Block
  *
  * - for i = 1 ... k
  *   = a string containing the class type of a ComputeConfig object,
  *    '*' means none (nullptr)
  *   = if the above is not '*', the description of the :ComputeConfig object
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
 * The RBlockSolverConfig contains the following field:
 *
 * - a vector of BlockSolverConfig for each of the sub-Block of the Block.
 *
 * The meaning of the field f_diff, inherited from BlockSolverConfig, is also
 * extended to the sub-Block: if f_diff is true, then all nullptr
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
  * The individual groups "BlockSolverConfig_<i>" are optional. If
  * "BlockSolverConfig_<i>" is not provided, then nullptr is considered. Note
  * that the matching between the sub-BlockSolverConfig and the sub-Block is
  * positional: the BlockSolverConfig found in the group
  * "BlockSolverConfig_<i>" is that for the i-th sub-Block. Note that the
  * vector of sub-BlockSolverConfig is allowed to be of different size than
  * the number of sub-Block; if it is larger any extra BlockSolverConfig is
  * simply ignored, if it shorted then all missing sub-BlockConfig are treated
  * as nullptr (default configuration). */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor

 virtual ~RBlockSolverConfig()
 {
  for( auto sBSC : v_BlockSolverConfigs )
   delete sBSC;
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
  * BlockSolverConfig of each sub-Block of the given Block.
  *
  * Note that BlockSolverConfig::get() is a reasonably cheap operation,
  * especially if clear == true, except of course for the fact the the
  * whole Block, and all its sub-Block recursively, must be scanned.
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
  * apply() recursively on each sub-Block.
  *
  * @param block A pointer to the Block that must be configured. */

 void apply( Block * block ) const override;

/*--------------------------------------------------------------------------*/
 /// unregister and delete all Solver attached to \p block (and its sub-Block)
 /** This method unregisters and deletes all Solver attached to the given
  * Block and to each of its sub-Block.
  *
  * @param block A pointer to the Block whose Solver will be reset. */

 void reset_Solver( Block * block ) const override;

/*--------------------------------------------------------------------------*/
 /// clear this RBlockSolverConfig
 /** This method first invokes BlockSolverConfig::clear(). Then, clear() is
  * invoked for each non-nullptr BlockSolverConfig * handled by this
  * RBlockSolverConfig (the BlockSolverConfig for each sub-Block). */

 void clear( void ) override {
  BlockSolverConfig::clear();

  for( auto config : v_BlockSolverConfigs )
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

 /// sets the (pointer to) the BlockSolverConfig of each sub-Block
 /** This function sets the vector containing the (pointer to) the
  *  BlockSolverConfig of every sub-Block. */

 void set_BlockSolverConfigs( std::vector<BlockSolverConfig *> && bsc ) {
  v_BlockSolverConfigs = std::move( bsc );
  }

/**@} ----------------------------------------------------------------------*/
/*---------- Methods for reading the data of the RBlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the RBlockSolverConfig
 *  @{ */

 /// returns the (pointer to) the BlockSolverConfig of every sub-Block
 /** This function returns a const reference to the vector containing the
  * (pointer to) the BlockSolverConfig of every sub-Block. */

 const std::vector<BlockSolverConfig *> & get_BlockSolverConfigs( void )
  const { return( v_BlockSolverConfigs ); }

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
  * - the number k of the BlockSolverConfig for the sub-Block of the Block
  *
  * - for i = 1 ... k
  *   = a string containing the class type of a BlockSolverConfig object,
  *     '*' means none (nullptr)
  *   = if the above is not '*', the description of the :BlockSolverConfig
  *     object
  *   (clearly, if k == 0 this is empty) */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of (pointer to) the sub-BlockSolverConfig for each sub-Block
 std::vector<BlockSolverConfig *> v_BlockSolverConfigs;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( RBlockSolverConfig ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ERBlockSolverConfig -------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from RBlockSolverConfig for configuring "indirect sub-Block"
/** The ERBlockSolverConfig class ("extended RBlockSolverConfig") derives from
 * RBlockSolverConfig in order to also takes into account the configuration
 * of the "indirect syb-Block" of the Block, i.e., those that do not appear
 * in its sub-Block list but upon on which the Block still depends. Some
 * Constraint and Objective are complicated enough that they may depend on
 * some Block. This class is meant to offer support for configuring the
 * Solver of these "indirect sub-Block". For instance, a Constraint (e.g. and
 * FRowConstraint) may have a BendersBFunction or LagBFunction, which in turn
 * has an inner Block whose Solver may have to be configured. Since not every
 * Constraint may have a "sub-Block" (or one that needs configuration), it is
 * necessary to indicate which Constraints will have an associated
 * BlockSolverConfig. These Constraints can be indicated by means of a
 * Block::ConstraintID.
 *
 * The ERBlockSolverConfig contains the following fields:
 *
 * - a vector of Block::ConstraintID indicating the set of Constraint of this
 *   Block that needs a BlockSolverConfig alongside a vector of
 *   BlockSolverConfig for those Constraint.
 *
 * - a BlockSolverConfig for the Objective of the Block.
 *
 * The meaning of the field f_diff, inherited from RBlockSolverConfig, is also
 * extended to these "indirect sub-Block": if f_diff is true, then all nullptr
 * BlockSolverConfig correspond to not changing any of the configurations of
 * any of the Solver attached to the corresponding "indirect sub-Block". */

class ERBlockSolverConfig : public RBlockSolverConfig
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*----------- CONSTRUCTING AND DESTRUCTING ERBlockSolverConfig -------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing ERBlockSolverConfig
 *  @{ */

 /// constructor: creates an empty ERBlockSolverConfig
 /** It constructs an empty ERBlockSolverConfig, which can then be initialized
  * by calling the methods deserialize(), load() or get(), or by calls to
  * set_SolverNames(), set_SolverConfigs(), set_BlockSolverConfigs(),
  * set_diff(), set_BlockSolverConfig_Constraints(), and
  * set_BlockSolverConfig_Objective().
  *
  * @param diff indicates if this configuration is a "differential" one. */

 ERBlockSolverConfig( bool diff = true ) : RBlockSolverConfig( diff ) { }

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockSolverConfig out of the given netCDF \p group
 /** It constructs an ERBlockSolverConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an ERBlockSolverConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        ERBlockSolverConfig. */

 ERBlockSolverConfig( netCDF::NcGroup & group ) : RBlockSolverConfig() {
  deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockSolverConfig out of an istream
 /** It constructs an ERBlockSolverConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * ERBlockSolverConfig.
  *
  * @param input The istream containing the description of the
  *        ERBlockSolverConfig. */

 ERBlockSolverConfig( std::istream &input ) : RBlockSolverConfig() {
  load( input );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an ERBlockSolverConfig for the given Block
 /** It constructs an ERBlockSolverConfig for the given \p block. It creates
  * an empty ERBlockSolverConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an ERBlockSolverConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  *
  * @param clear It indicates whether a "cleared" ERBlockSolverConfig is
  *        desired. See ERBlockSolverConfig:get() for details. */

 ERBlockSolverConfig( Block * block , bool diff = false ,
		      bool clear = false ) : RBlockSolverConfig( diff ) {
  get( block , clear );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 ERBlockSolverConfig( const ERBlockSolverConfig &old );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockSolverConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockSolverConfig::deserialize( netCDF::NcGroup ) to the
  * specific format of an ERBlockSolverConfig. Besides the mandatory "type"
  * attribute of any :Configuration, and the dimensions and variables of an
  * RBlockSolverConfig, the group should also contain the following:
  *
  * - the dimension "n_BlockSolverConfig_Constraint" containing the number of
  *   BlockSolverConfig descriptions associated with the Constraint of the
  *   current Block; this dimension is optional; if it is not provided, then
  *   n_BlockSolverConfig_Constraint = 0 is assumed.
  *
  * - with p being the size of "n_BlockSolverConfig_Constraint", a
  *   one-dimensional variable called "ConstraintID", of size 2p and type
  *   netCDF::ncUint, containing the Block::ConstraintID of the Constraint
  *   that need a BlockSolverConfig: for each i = 0, ..., p - 1, the pair (
  *   ConstraintID[ 2i ], ConstraintID[ 2i + 1] ) is the ConstraintID of the
  *   i-th Constraint that needs a BlockSolverConfig, i.e., ConstraintID[ 2i ]
  *   provides the index of the group to which the i-th Constraint belongs and
  *   ConstraintID[ 2i + 1 ] provides the index of the i-th Constraint (see
  *   Block::ConstraintID for the definition of an index of a Constraint);
  *   this variable is mandatory if n_BlockSolverConfig_Constraint > 0.
  *
  * - p groups, with name "BlockSolverConfig_Constraint_<i>" for all i = 0,
  *   ..., p - 1, containing each the description of a BlockSolverConfig
  *   associated with the i-th Constraint indicated by the "ConstraintID"
  *   variable (which is given by the pair ( ConstraintID[ 2i ], ConstraintID[
  *   2i + 1] )); these groups are optional; if
  *   "BlockSolverConfig_Constraint_<i>" is not provided, then nullptr is
  *   assumed for the i-th Constraint;
  *
  * - a group with name "BlockSolverConfig_Objective", containing the
  *   description of a BlockSolverConfig associated with the Objective of the
  *   current Block; this group is optional; if it is not provided, then
  *   nullptr is assumed. */

 void deserialize( netCDF::NcGroup & group ) override;

/*------------------------------ DESTRUCTOR --------------------------------*/
 /// destructor

 virtual ~ERBlockSolverConfig()
 {
  for( auto sBSCC : v_BlockSolverConfig_Constraints )
   delete sBSCC;

  delete f_BlockSolverConfig_Objective;
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// getting the ERBlockSolverConfig of the given Block
 /** This method gets information about the current set of Solver attached to
  * the given Block (and its sub-Block and "indirect sub-Block", recursively)
  * and stores in this ERBlockSolverConfig. This information consists of that
  * supported by the RBlockSolverConfig (see RBlockSolverConfig::get()) plus
  * any BlockSolverConfig that may be associated with Constraint and/or
  * Objective of the given Block.
  *
  * Note that
  *
  *     CALLING ERBlockSolverConfig::get() IS A POTENTIALLY COSTLY OPERATION
  *     BECAUSE IT ENTAILS SCANNING ALL Constraint AND Objective OF THE
  *     Block, AND ALL ITS sub-Block RECURSIVELY, IN ORDER TO FIND THE
  *     "INDIRECT" sub-Block.
  *
  * Also, the current implementation only supports the case where the
  * "indirect" sub-Block are within a LagBFunction or a BendersBFunction
  * inside a FRowConstraint or FRealObjective.
  *
  * @param block A pointer to the Block whose ERBlockSolverConfig must be
  *        filled.
  *
  * @param clear It indicates whether this RBlockSolverConfig must be a clear
  *        one. See the comments to BlockSolverConfig::get() for the
  *        definition of this parameter. */

 void get( Block * block , bool clear = false ) override;

/**@} ----------------------------------------------------------------------*/
/*------- METHODS DESCRIBING THE BEHAVIOR OF THE ERBlockSolverConfig -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the ERBlockSolverConfig
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// create and set all the Solver attached to the Block and its sub-Block
 /** Method for creating, configuring and registering all the Solver that the
  * given Block may need, including those of its sub-Block and those of its
  * "indirect sub-Block" (i.e., those associated with Constraint and/or
  * Objective of the Block), recursively. This method first invoke the method
  * RBlockSolverConfig::apply() and then proceeds configuring the Solver of
  * the "indirect sub-Block". The latter is done by calling apply()
  * recursively on each "indirect sub-Block" that is considered by this
  * ERBlockSolverConfig (see get_ConstraintID()).
  *
  * @param block A pointer to the Block that must be configured. */

 void apply( Block * block ) const override;

/*--------------------------------------------------------------------------*/
 /// unregister and delete all Solver attached to \p block (and its sub-Block)
 /** This method unregisters and deletes all Solver attached to the given
  * Block and to each of its sub-Block and "indirect sub-Block".
  *
  * @param block A pointer to the Block whose Solver will be reset. */

 void reset_Solver( Block * block ) const override;

/*--------------------------------------------------------------------------*/
 /// clear this ERBlockSolverConfig
 /** This method first invokes RBlockSolverConfig::clear(). Then, clear() is
  * invoked for each non-nullptr BlockSolverConfig * handled by this
  * ERBlockSolverConfig (the BlockSolverConfig associated with each Constraint
  * and the BlockSolverConfig associated with the Objective).
  */

 void clear( void ) override {
  RBlockSolverConfig::clear();

  for( auto config : v_BlockSolverConfig_Constraints )
   if( config )
    config->clear();

  if( f_BlockSolverConfig_Objective )
   f_BlockSolverConfig_Objective->clear();
  }

/*------------------------------- CLONE -----------------------------------*/

 ERBlockSolverConfig * clone( void ) const override
 {
  return( new ERBlockSolverConfig( *this ) );
  }

/**@} ----------------------------------------------------------------------*/
/*----- METHODS FOR LOADING, PRINTING & SAVING THE ERBlockSolverConfig -----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the ERBlockSolverConfig
 * @{ */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockSolverConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockSolverConfig::serialize( netCDF::NcGroup ) to the specific
  * format of an ERBlockSolverConfig. See ERBlockSolverConfig::deserialize(
  * netCDF::NcGroup ) for details of the format of the created netCDF
  * group. */

 void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR MODIFYING THE ERBlockSolverConfig ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the ERBlockSolverConfig
 *  @{ */

 /// sets the BlockSolverConfig of the "indirect sub-Block" of Constraint
 /** This function sets the vector containing the (pointer to the)
  *  BlockSolverConfig of the "indirect sub-Block" associated with the
  *  Constraint of the Block. The i-th BlockSolverConfig in this vector is
  *  associated with the Constraint given by the i-th element of the
  *  ConstraintID vector (see get_ConstraintID()). */

 void set_BlockSolverConfig_Constraints(
			         std::vector<BlockSolverConfig *> && bsc ) {
  v_BlockSolverConfig_Constraints = std::move( bsc );
  }

/*--------------------------------------------------------------------------*/
 /// adds a BlockSolverConfig of an "indirect sub-Block" of Constraint
 /** This function adds a (pointer to the) BlockSolverConfig of an "indirect
  *  sub-Block" associated with the Constraint of the Block whose
  *  Block::ConstraintID is \p constraint_id. */

 void add_BlockSolverConfig_Constraint( BlockSolverConfig * bsc ,
                                        Block::ConstraintID constraint_id ) {
  v_BlockSolverConfig_Constraints.push_back( bsc );
  v_ConstraintID.push_back( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function sets the vector of ConstraintID, which indicates the set of
  * Constraint of the Block that have a BlockSolverConfig for their inner
  * Block. */

 void set_ConstraintID( std::vector<Block::ConstraintID> && constraint_id ) {
  v_ConstraintID = std::move( constraint_id );
  }

/*--------------------------------------------------------------------------*/
 /// sets the BlockSolverConfig of the "indirect sub-Block" of Objective
 /** This function sets the (pointer to the) BlockSolverConfig of the
  *  "indirect sub-Block" associated with the Objective of the Block. */

 void set_BlockSolverConfig_Objective( BlockSolverConfig * bsc ) {
  f_BlockSolverConfig_Objective = bsc;
  }

/**@} ----------------------------------------------------------------------*/
/*--------- Methods for reading the data of the ERBlockSolverConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the ERBlockSolverConfig
 *  @{ */

 /// returns the BlockSolverConfig of "indirect sub-Block" of Constraint
 /** This function returns a const reference to the vector containing the
  *  (pointer to the) BlockSolverConfig of the "indirect sub-Block" associated
  *  with the Constraint of the Block. */

 const std::vector<BlockSolverConfig *> &
  get_BlockSolverConfig_Constraints( void ) const {
  return( v_BlockSolverConfig_Constraints );
  }

/*--------------------------------------------------------------------------*/
 /// returns the vector of ConstraintID indicating the "indirect sub-Block"
 /** This function returns the vector of ConstraintID, which indicates the set
  * of Constraint of the Block that have a BlockSolverConfig for their inner
  * Block. */

 const std::vector<Block::ConstraintID> & get_ConstraintID( void ) const {
  return( v_ConstraintID );
  }

/*--------------------------------------------------------------------------*/
 /// returns the BlockSolverConfig of the "indirect sub-Block" of Objective
 /** This function returns the (pointer to the) BlockSolverConfig of the
  *  "indirect sub-Block" associated with the Objective of the Block. */

 BlockSolverConfig * get_BlockSolverConfig_Objective( void ) const {
  return( f_BlockSolverConfig_Objective );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the ERBlockSolverConfig

 void print( std::ostream &output ) const override;

/*--------------------------------------------------------------------------*/
 /// load this ERBlockSolverConfig out of an istream
 /** Load this ERBlockSolverConfig out of an istream. The format is defined as
  * that specified in RBlockSolverConfig::load(), followed by:
  *
  * - the number k of the BlockSolverConfig for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   = two integers representing the ConstraintID for the Constraint
  *   = a string containing the class type of a BlockSolverConfig object,
  *     '*' means none (nullptr)
  *   = if the above is not '*', the description of the :BlockSolverConfig
  *     object
  *   (clearly, if k == 0 this is empty)
  *
  * a binary number b indicating whether a BlockSolverConfig associated with
  * the Objective of the Block is provided
  *
  * If b != 0:
  *  - a string containing the class type of a BlockSolverConfig object,
  *    '*' means none (nullptr)
  *  - if the above is not '*', the description of the :BlockSolverConfig
  *    object */

 void load( std::istream &input ) override;

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 /// the vector of indices identifying the set of Constraint
 /** This vector indicates which Constraint of the Block have a
  * BlockSolverConfig for their inner Block. */

 std::vector<Block::ConstraintID> v_ConstraintID;

 /// the vector of (pointer to the) BlockSolverConfig for Constraint
 /** The vector of (pointer to the) BlockSolverConfig for Constraint. The i-th
  * BlockSolverConfig in this vector is that of the Block associated with the
  * Constraint identified by the i-th element in the vector v_ConstraintID.
  */

 std::vector<BlockSolverConfig *> v_BlockSolverConfig_Constraints;

 /// the (pointer to the) BlockSolverConfig for the Objective of the Block
 BlockSolverConfig * f_BlockSolverConfig_Objective = nullptr;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

 };  // end( class( ERBlockSolverConfig ) )

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

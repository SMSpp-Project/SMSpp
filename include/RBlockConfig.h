/*--------------------------------------------------------------------------*/
/*------------------------ File RBlockConfig.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for classes derived from BlockConfig, which are intended to
 * offer support to configure not only a Block but also its sub-Block
 * (recursively), its Objective and Constraint. Three "base" classes are
 * defined:
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
 * Note that, technically, this is not obtained by having the "more complex"
 * classes to derive from the "base" ones, as this would not work without
 * virtual inheritance; rather, three "handler" classes (RHandler, CHandler
 * and OHandler) are defined that handle each individual aspect, and then the
 * *BlockConfig classes are defined by deriving from the proper subset of the
 * *Handler).
 *
 * All these BlockConfig support the notion of a "cleared" Configuration (see
 * Configuration::clear()). When the clear() method is invoked, any pointers
 * to a sub-Configuration that the :BlockConfig may have (for instance,
 * pointers to the BlockConfig of the sub-Block; pointers to ComputeConfig of
 * Constraint or Objective) are deleted. The value of the BlockConfig::f_diff
 * field and the structure of the :BlockConfig are preserved. The structures
 * that are preserved are that of a :BlockConfig that deals with sub-Block
 * (namely, RBlockConfig, ORBlockConfig, CRBlockConfig, and OCRBlockConfig;
 * shortly referred as *R*BlockConfig) or Constraint (namely, CBlockConfig,
 * CRBlockConfig, OCBlockConfig, and OCRBlockConfig; shortly referred as
 * *C*BlockConfig).
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*------------------------ BlockConfigHandlers -----------------------------*/
/*--------------------------------------------------------------------------*/
/* Uncommented auxiliary classes that separately handle the three different
 * aspects of configuration: sub-Block (R), Constraints (C), Objective (O). */

namespace BlockConfigHandlers
{
/*--------------------------------------------------------------------------*/
/*------------------------------- RHandler ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** Auxiliary classes that handles configuration of sub-Block (recursively).
 */

class RHandler
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*----------------- CONSTRUCTING AND DESTRUCTING RHandler ------------------*/

 // empty constructor

 RHandler() = default;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // copy constructor

 RHandler( const RHandler & old ) {
  v_sub_Block_id = old.v_sub_Block_id;

  v_sub_BlockConfig.resize( old.v_sub_BlockConfig.size(), nullptr );
  for( std::size_t i = 0; i < v_sub_BlockConfig.size(); ++i )
   if( old.v_sub_BlockConfig[ i ] )
    v_sub_BlockConfig[ i ] = old.v_sub_BlockConfig[ i ]->clone();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // move constructor

 RHandler( RHandler && old ) noexcept {
  v_sub_Block_id = std::move( old.v_sub_Block_id );
  v_sub_BlockConfig = std::move( old.v_sub_BlockConfig );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // deserialize

 void deserialize( const netCDF::NcGroup & group );

/*------------------------------ DESTRUCTOR --------------------------------*/

 // destructor (not virtual, RHandler is not)

 ~RHandler() {
  for( auto config : v_sub_BlockConfig )
   delete config;
  }

/*-------------------------- OTHER INITIALIZATIONS -------------------------*/

 // getting the RHandler from the given Block

 void get( Block * block );

/*------------------ METHODS FOR MODIFYING THE RHandler --------------------*/

 /// add a BlockConfig for a sub-Block (Index)
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

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// add a BlockConfig for a sub-Block (name)
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

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// remove a BlockConfig for a sub-Block
 /** This function removes the BlockConfig at the given \p index. Notice that
  * \p index is not the index of the sub-Block, but the index of the
  * BlockConfig being handled by this RBlockConfig.
  *
  * @param index The index of the BlockConfig to be removed.
  *
  * @param destroy It indicates whether the pointer to the BlockConfig at the
  *        given index must be deleted. */

 void remove_sub_BlockConfig( Block::Index index , bool destroy = true ) {
  if( index >= v_sub_BlockConfig.size() )
   throw( std::invalid_argument( "RBlockConfig::remove_sub_BlockConfig: "
				 "invalid index: " + std::to_string( index )
				 ) );
  if( destroy )
   delete v_sub_BlockConfig[ index ];
  v_sub_BlockConfig.erase( std::begin( v_sub_BlockConfig ) + index );
  v_sub_Block_id.erase( std::begin( v_sub_Block_id ) + index );
  }

/*------------ METHODS DESCRIBING THE BEHAVIOR OF THE RHandler -------------*/

 // configure the given Block and its sub-Block (recursively)

 void apply( Block * block , bool deleteold , bool diff );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // clear this RHandler

 void clear( void ) {
  for( auto config : v_sub_BlockConfig )
   if( config )
    config->clear();
  }

/*-------------- METHODS FOR READING THE DATA OF THE RHandler --------------*/

 /// returns true if the RHandler is empty

 bool empty( void ) { return( v_sub_BlockConfig.empty() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the current number of sub-BlockConfig
 /** This method returns the number of BlockConfig (for the sub-Block)
  * currently being handled. */

 [[nodiscard]] Block::Index num_sub_BlockConfig( void ) const {
  return( v_sub_BlockConfig.size() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the (pointer to) the BlockConfig at the given \p index
 /** This method returns the pointer to the BlockConfig at the given \p
  * index. Notice that \p index is not the index of the sub-Block, but the
  * index of the BlockConfig being handled by this *RBlockConfig.
  *
  * @param index the index of the BlockConfig to be returned (it must be an
  *        index between 0 and num_sub_BlockConfig() - 1).
  *
  * @return the pointer to the BlockConfig at the given \p index. */

 [[nodiscard]] BlockConfig * get_sub_BlockConfig( Block::Index index ) const {
  if( index >= v_sub_BlockConfig.size() )
   throw( std::invalid_argument( "get_sub_BlockConfig: invalid index: "
				 + std::to_string( index ) ) );
  return( v_sub_BlockConfig[ index ] );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the id of the sub-Block associated with the given \p index
 /** This function returns the identification of the sub-Block whose
  * BlockConfig is located at position \p index in this *RBlockConfig. The
  * identification of the sub-Block is either its name (see Block::name()) or
  * its index in the list of nested Block of its father Block.
  *
  * @param index The index of a BlockConfig in this *RBlockConfig (it must be
  *        an index between 0 and num_sub_BlockConfig() - 1).
  *
  * @return The identification of the sub-Block associated with the
  *         BlockConfig located at the given \p index. */

 [[nodiscard]] const std::string & get_sub_Block_id( Block::Index index )
  const {
  if( index >= v_sub_Block_id.size() )
   throw( std::invalid_argument( "get_sub_Block_id: invalid index: "
				 + std::to_string( index ) ) );
  return( v_sub_Block_id[ index ] );
  }

/*----------- METHODS FOR LOADING, PRINTING & SAVING THE RHandler ----------*/

 /// serialize the RHandler

 void serialize( netCDF::NcGroup & group ) const;

/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the RHandler

 void print( std::ostream & output ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // load this RHandler out of an istream

 void load( std::istream & input );

/*---------------------- PROTECTED FIELDS OF THE CLASS ---------------------*/

 /// the vector of sub-BlockConfig for each of the sub-Block of the Block
 std::vector< BlockConfig * > v_sub_BlockConfig;

 /// correspondence between v_sub_BlockConfig and the sub-Block of the Block
 /** This vector specifies the correspondence between the BlockConfig in
  * #v_sub_BlockConfig and the sub-Block of the Block. v_sub_Block_id[ i ]
  * contains the identification of the sub-Block whose BlockConfig is
  * v_sub_BlockConfig[ i ]. A sub-Block can be identified either by its name
  * (see Block::name()) or by its index in the list of sub-Blocks of its
  * father Block. If the name of the sub-Block is used, then the first
  * character of this name cannot be a digit. */
 std::vector< std::string > v_sub_Block_id;

/*--------------------------------------------------------------------------*/

};  // end( class RHandler )

/*--------------------------------------------------------------------------*/
/*------------------------------- CHandler ---------------------------------*/
/*--------------------------------------------------------------------------*/
/* Uncommented auxiliary classes that handles configuration of Constraint */

class CHandler {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*----------------- CONSTRUCTING AND DESTRUCTING CHandler ------------------*/

 // empty constructor

 CHandler() = default;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // copy constructor

 CHandler( const CHandler & old ) {
  v_Constraint_id = old.v_Constraint_id;

  v_Config_Constraint.resize( old.v_Config_Constraint.size() , nullptr );
  auto this_it = v_Config_Constraint.begin();
  auto old_it = old.v_Config_Constraint.cbegin();
  for( ; this_it != v_Config_Constraint.end() ; ++this_it , ++old_it )
   if( *old_it )
    *this_it = ( *old_it )->clone();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // move constructor

 CHandler( CHandler && old ) noexcept {
  v_Constraint_id = std::move( old.v_Constraint_id );
  v_Config_Constraint = std::move( old.v_Config_Constraint );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // deserialize

 void deserialize( const netCDF::NcGroup & group );

/*------------------------------ DESTRUCTOR --------------------------------*/

 // destructor (not virtual, CHandler is not)

 ~CHandler() {
  for( auto config : v_Config_Constraint )
   delete config;
  }

/*-------------------------- OTHER INITIALIZATIONS -------------------------*/

 // getting the CHandler from the given Block

 void get( Block * block );

/*------------------ METHODS FOR MODIFYING THE CHandler --------------------*/

 /// add a ComputeConfig for a Constraint
 /** This function adds the pointer to the ComputeConfig of the Constraint
  * whose group is identified by \p constraint_group_id (which must be either
  * the name or the index of the group to which the Constraint belongs) and \p
  * constraint_index (the index of the Constraint in its group). See
  * Block::ConstraintID for the definition of the index of a group and the
  * index of a Constraint in a group.
  *
  * @param config A pointer to a ComputeConfig.
  *
  * @param constraint_group_id The identification (either the name or the
  *        index) of the group to which the Constraint belongs.
  *
  * @param constraint_index The index of the Constraint in its group. */

 void add_ComputeConfig_Constraint( ComputeConfig * config,
				    std::string && constraint_group_id ,
                                    Block::Index constraint_index ) {
  v_Constraint_id.emplace_back( std::move( constraint_group_id ) ,
                                constraint_index );
  v_Config_Constraint.push_back( config );
  }

/*--------------------------------------------------------------------------*/
 /// add a ComputeConfig for a Constraint
 /** This function adds the pointer to the ComputeConfig of the Constraint
  * that belongs to the group whose index is \p constraint_group_index and
  * whose index in that group is \p constraint_index. See Block::ConstraintID
  * for the definition of the index of a group and the index of a Constraint
  * in a group.
  *
  * @param config A pointer to a ComputeConfig.
  *
  * @param constraint_group_index The index of the group to which the
  *        Constraint belongs.
  *
  * @param constraint_index The index of the Constraint in its group. */

 void add_ComputeConfig_Constraint( ComputeConfig * config,
                                    Block::Index constraint_group_index ,
                                    Block::Index constraint_index ) {
  v_Constraint_id.emplace_back( std::to_string( constraint_group_index ) ,
                                constraint_index );
  v_Config_Constraint.push_back( config );
  }

/*--------------------------------------------------------------------------*/
 /// remove a ComputeConfig of a Constraint
 /** This function removes the ComputeConfig at the given \p index. Notice
  * that \p index is not the index of the Constraint, but the index of the
  * ComputeConfig being handled by this CBlockConfig.
  *
  * @param index The index of the ComputeConfig to be removed.
  *
  * @param destroy It indicates whether the pointer to the ComputeConfig at
  *        the given index must be deleted. */

 void remove_ComputeConfig_Constraint( Block::Index index ,
				       bool destroy = true ) {
  if( index >= v_Constraint_id.size() )
   throw( std::invalid_argument( "CBlockConfig::remove_ComputeConfig_"
				 "Constraint: invalid index: " +
				 std::to_string( index ) ) );
  if( destroy )
   delete v_Config_Constraint[ index ];
  v_Constraint_id.erase( std::begin( v_Constraint_id ) + index );
  v_Config_Constraint.erase( std::begin( v_Config_Constraint ) + index );
  }

/*------------ METHODS DESCRIBING THE BEHAVIOR OF THE CHandler -------------*/

 // configure the given Block

 void apply( Block * block , bool deleteold , bool diff );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // clear this CHandler

 void clear() {
  for( auto config : v_Config_Constraint )
   if( config )
    config->clear();
  }

/*-------------- METHODS FOR READING THE DATA OF THE CHandler --------------*/

 /// returns true if the CHandler is empty

 bool empty( void ) { return( v_Config_Constraint.empty() ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the number of ComputeConfig for Constraint in this *CBlockConfig
 /** This method returns the number of ComputeConfig (for the Constraint)
  * currently being handled by this *CBlockConfig . */

 [[nodiscard]] Block::Index num_ComputeConfig_Constraint( void ) const {
  return( v_Config_Constraint.size() );
  }

/*--------------------------------------------------------------------------*/
 /// returns the (pointer to) the ComputeConfig at the given \p index
 /** This method returns the pointer to the ComputeConfig at the given \p
  * index. Notice that \p index is not the index of the Constraint, but the
  * index of the ComputeConfig being handled by this *CBlockConfig.
  *
  * @param index the index of the ComputeConfig to be returned (it must be an
  *        index between 0 and num_ComputeConfig_Constraint() - 1).
  *
  * @return the pointer to the ComputeConfig at the given \p index. */

 [[nodiscard]] ComputeConfig * get_ComputeConfig_Constraint(
						 Block::Index index ) const {
  if( index >= v_Config_Constraint.size() )
   throw( std::invalid_argument( "get_ComputeConfig_Constraint: invalid index "
				 + std::to_string( index ) ) );
  return( v_Config_Constraint[ index ] );
  }

/*--------------------------------------------------------------------------*/
 /// returns the id of the Constraint associated with the given \p index
 /** This function returns the identification of the Constraint whose
  * ComputeConfig is located at position \p index in this *CBlockConfig.
  *
  * @param index the index of a ComputeConfig in this *CBlockConfig (it must
  *        be an index between 0 and num_ComputeConfig_Constraint() - 1)
  *
  * @return the identification of the Constraint associated with the
  *         ComputeConfig located at the given \p index. */

 [[nodiscard]] const std::pair< std::string, Block::Index > &
 get_Constraint_id( Block::Index index ) const {
  if( index >= v_Constraint_id.size() )
   throw( std::invalid_argument( "get_Constraint_id: invalid index "
				 + std::to_string( index ) ) );
  return( v_Constraint_id[ index ] );
  }

/*----------- METHODS FOR LOADING, PRINTING & SAVING THE RHandler ----------*/

 /// serialize the RHandler

 void serialize( netCDF::NcGroup & group ) const;

/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 // print the CHandler

 void print( std::ostream & output ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // load this CHandler out of an istream

 void load( std::istream & input );

/*---------------------- PROTECTED FIELDS OF THE CLASS ---------------------*/

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
 std::vector< std::pair< std::string, Block::Index > > v_Constraint_id;

 /// the vector of (pointer to the) ComputeConfig for the Constraint
 /** The vector of (pointer to the) ComputeConfig for the Constraint. The i-th
  * ComputeConfig in this vector is that of the Constraint identified by the
  * i-th element in the vector #v_Constraint_id. */
 std::vector< ComputeConfig * > v_Config_Constraint;

/*--------------------------------------------------------------------------*/

};  // end( class CHandler )

/*--------------------------------------------------------------------------*/
/*------------------------------- OHandler ---------------------------------*/
/*--------------------------------------------------------------------------*/
/* Uncommented auxiliary classes that handles configuration of Objective */

class OHandler {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*----------------- CONSTRUCTING AND DESTRUCTING CHandler ------------------*/

 // empty constructor

 OHandler() : f_Config_Objective( nullptr ) {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // copy constructor

 OHandler( const OHandler & old ) : f_Config_Objective( nullptr ) {
  if( old.f_Config_Objective )
   f_Config_Objective = old.f_Config_Objective->clone();
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // move constructor

 OHandler( OHandler && old ) noexcept
  : f_Config_Objective( old.f_Config_Objective ) {
  old.f_Config_Objective = nullptr;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // deserialize

 void deserialize( const netCDF::NcGroup & group );

/*------------------------------ DESTRUCTOR --------------------------------*/

 // destructor (not virtual, OHandler is not)

 ~OHandler() { delete f_Config_Objective; }

/*-------------------------- OTHER INITIALIZATIONS -------------------------*/

 // getting the OHandler from the given Block

 void get( Block * block );

/*------------------ METHODS FOR MODIFYING THE OHandler --------------------*/

 /// sets the ComputeConfig of the Objective
 /** Sets the pointer to the ComputeConfig of the Objective of the Block.
  *
  * @param config a pointer to the ComputeConfig of the Objective.
  *
  * @param deleteold indicates whether the currently stored ComputeConfig
  *        for the Objective (if any) must be destroyed. */

 void set_Config_Objective( ComputeConfig * config , bool deleteold = true ) {
  if( deleteold )
   delete f_Config_Objective;
  f_Config_Objective = config;
  }

/*------------ METHODS DESCRIBING THE BEHAVIOR OF THE OHandler -------------*/

 // configure the given Block

 void apply( Block * block , bool deleteold , bool diff );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // clear this OHandler

 void clear( void ) { if( f_Config_Objective ) f_Config_Objective->clear(); }

/*-------------- METHODS FOR READING THE DATA OF THE OHandler --------------*/

 /// returns true if the OHandler is empty

 bool empty(void ) { return( ! f_Config_Objective ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the ComputeConfig of the Objective

 [[nodiscard]] ComputeConfig * get_Config_Objective( void ) const {
  return( f_Config_Objective );
  }

/*----------- METHODS FOR LOADING, PRINTING & SAVING THE OHandler ----------*/

 /// serialize the OHandler

 void serialize( netCDF::NcGroup & group ) const;

/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 // print the OHandler

 void print( std::ostream & output ) const;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 // load this OHandler out of an istream

 void load( std::istream & input );

/*---------------------- PROTECTED FIELDS OF THE CLASS ---------------------*/

 /// the (pointer to the) ComputeConfig for the Objective of the Block
 ComputeConfig * f_Config_Objective;

/*--------------------------------------------------------------------------*/

};  // end( class OHandler )

/*--------------------------------------------------------------------------*/

}  // end( namespace BlockConfigHandlers )

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup RBlockConfig_CLASSES Classes in RBlockConfig.h
 *  @{
 *
 * This file declares some classes derived from BlockConfig, which are
 * intended to offer support to configure not only a Block but also its
 * sub-Block (recursively), it Objective and Constraint. Three "base" classes
 * are defined:
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
 * The issue is that a Block can be a rather complex tree-structured object,
 * which potentially need to be configured "in many places":
 *
 * - the Block itself (which is what the base BlockConfig does);
 *
 * - (a subset of) the sub-Block, recursively;
 *
 * - (a subset of) the Constraint of the Block, comprised those of the
 *   sub-Block, recursively;
 *
 * - the Objective of the Block and those of the sub-Block, recursively.
 *
 * However, most Block will only need a small subset of these to be
 * Configured, which is why *BlockConfig all use a "sparse" representation;
 * also, an effort is always made to get() the "simplest" possible type of
 * *BlockConfig object, and to only record the pieces that are strictly
 * necessary to deal with the parts that actually need configuring.
 *
 * All these BlockConfig support the notion of a "cleared" Configuration (see
 * Configuration::clear()). When the clear() method is invoked, any pointers
 * to a sub-Configuration that the *BlockConfig may have (for instance,
 * pointers to ComputeConfig of Constraint or Objective) are deleted. The
 * value of the BlockConfig::f_diff field and the structure of the
 * :BlockConfig are preserved. The structures that are preserved are that
 * of a :BlockConfig that deals with sub-Block (namely, RBlockConfig,
 * ORBlockConfig, CRBlockConfig, and OCRBlockConfig; shortly referred as
 * *R*BlockConfig) or Constraint (namely, CBlockConfig, CRBlockConfig,
 * OCBlockConfig, and OCRBlockConfig; shortly referred as *C*BlockConfig).
 *
 * The rationale for preserving the structure is that a clear()-ed
 * *BlockConfig is a good start for incrementally changing the Configuration
 * of a Block "in a few places". The most obvious use case is when a Block
 * is created and Configured, such as in
 *
 *     Block * myBlock = < some way to create it, say a netCDF file >
 *     BlockConfig * myBC = < some way to create it, say a netCDF file >
 *     myBC->apply( myBlock );
 *
 * At this point, if myBlock no longer needs to update its configuration,
 * myBC can be deleted. However, if different parts of the Block may need
 * to be re-configured, perhaps multiple times, at a later stage, a
 * viable approach is to keep myBC and clear it
 *
 *     myBC->clear();
 *
 * Now, changing the appropriate places of myBC and apply()-ing it again
 * to myBlock allows to change the configuration in disparate places with
 * one call. The clear()-ed *BlockConfig contains just the minimum
 * amount of memory to keep track of where in the Block the parts to be
 * configured are.
 *
 * Of course, it is also possible to add/remove specific parts to myBC
 * programmatically if the Block needs to be configured differently, but
 * still the *BlockConfig is likely to be a convenient object to organise
 * the process around. */

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OCRBlockConfig --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// BlockConfig for configuring everything of a Block

class OCRBlockConfig : public BlockConfig ,
                       public BlockConfigHandlers::OHandler ,
                       public BlockConfigHandlers::CHandler ,
                       public BlockConfigHandlers::RHandler {

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

 explicit OCRBlockConfig( bool diff = true )
  : BlockConfig( diff ) , OHandler() , CHandler() , RHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an OCRBlockConfig out of the given netCDF \p group
 /** It constructs an OCRBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OCRBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OCRBlockConfig. */

 explicit OCRBlockConfig( const netCDF::NcGroup & group ) : BlockConfig() ,
  OHandler() , CHandler() , RHandler() {
  OCRBlockConfig::deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// constructs an OCRBlockConfig out of an istream
 /** It constructs an OCRBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OCRBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OCRBlockConfig. */

 explicit OCRBlockConfig( std::istream & input ) : BlockConfig() ,
  OHandler() , CHandler() , RHandler() {
  OCRBlockConfig::load( input );
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

 explicit OCRBlockConfig( Block * block, bool diff = false ) :
  BlockConfig( diff ) , OHandler() , CHandler() , RHandler() {
   OCRBlockConfig::get( block );
   }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OCRBlockConfig( const OCRBlockConfig & old ) : BlockConfig( old ) ,
   OHandler( old ) , CHandler( old ) , RHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 OCRBlockConfig( OCRBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) ,
   CHandler( std::move( old ) ) , RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// construct the "right" BlockConfig out of a Block
 /** This static method inputs a Block and constructs the "minimal possible"
  * *BlockConfig out of it. This is done by first get()-ing a OCRBlockConfig,
  * which is the most general case, and then progressively bumping down all
  * empty features until possibly a nullptr remains if the Block has exactly
  * no configuration different from the default.
  *
  * Since the method is static it has to be called as
  *
  *     auto BC = OCRBlockConfig::get_right_BlockConfig( myBlock );
  *
  * and therefore it can also be used as constructor of OCRBlockConfig, or
  * in fact any other *BlockConfig. */

 static BlockConfig * get_right_BlockConfig( const Block * block );

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 OCRBlockConfig & operator=( const OCRBlockConfig & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a OCRBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should also contain the following:
  *
  * - all that is needed to describe a BlockConfig
  *   (see BlockConfig::deserialize()):
  *
  * - all that is needed to describe a OBlockConfig
  *   (see OBlockConfig::deserialize()):
  *
  * - all that is needed to describe a CBlockConfig
  *   (see CBlockConfig::deserialize()):
  *
  * - all that is needed to describe a RBlockConfig
  *   (see RBlockConfig::deserialize()). */

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ! ( BlockConfig::empty() && OHandler::empty() &&
	  CHandler::empty() && RHandler::empty() ) )
   throw( std::logic_error( "deserializing a non-empty OCRBlockConfig" ) );

  BlockConfig::deserialize( group );
  OHandler::deserialize( group );
  CHandler::deserialize( group );
  RHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~OCRBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the OCRBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OCRBlockConfig. This information
  * consists of that supported by the CRBlockConfig (see CRBlockConfig::get())
  * plus the ComputeConfig that may be associated with the Objective of the
  * given Block. If the pointer to the ComputeConfig currently stored in this
  * OCRBlockConfig is not nullptr, it is used to retrieve the configuration of
  * the Objective of the given \p block (see
  * ThinComputeInterface::get_ComputeConfig()).
  *
  * @param block A pointer to the Block whose OCRBlockConfig must be
  *        filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  OHandler::get( block );
  CHandler::get( block );
  RHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE OCRBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OCRBlockConfig
 *  @{ */

 /// configure the given Block and its Objective
 /** Method for configuring the given Block and its Objective. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block, bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block, deleteold );
  OHandler::apply( block, deleteold, f_diff );
  CHandler::apply( block, deleteold, f_diff );
  RHandler::apply( block, deleteold, f_diff );
  }

/*--------------------------------------------------------------------------*/
 /// clear this OCRBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  OHandler::clear();
  CHandler::clear();
  RHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] OCRBlockConfig * clone( void ) const override {
  return( new OCRBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------- METHODS FOR LOADING, PRINTING & SAVING THE OCRBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OCRBlockConfig
 *  @{ */

 /// extends CRBlockConfig::serialize( netCDF::NcGroup )
 /** Extends CRBlockConfig::serialize( netCDF::NcGroup ) to the specific
  * format of an OCRBlockConfig. See OCRBlockConfig::deserialize(
  * netCDF::NcGroup ) for details of the format of the created netCDF
  * group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  OHandler::serialize( group );
  CHandler::serialize( group );
  RHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------------------ FRIENDS -----------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class CBlockConfig;
 // make CBlockConfig friend for the weird move-constructor

 friend class OCBlockConfig;
 // make OCBlockConfig friend for its weird move-constructor

 friend class ORBlockConfig;
 // make ORBlockConfig friend for its weird move-constructor

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OCRBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  OHandler::print( output );
  CHandler::print( output );
  RHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this OCRBlockConfig out of an istream
 /** Load this OCRBlockConfig out of an istream. The format is:
  *
  * - that specified in BlockConfig::load(), followed by
  *
  * - that specified in OBlockConfig::load(), followed by
  *
  * - that specified in CBlockConfig::load(), followed by
  *
  * - that specified in RBlockConfig::load(). */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  OHandler::load( input );
  CHandler::load( input );
  RHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( OCRBlockConfig ) )

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

class CRBlockConfig : public BlockConfig ,
                      public BlockConfigHandlers::CHandler ,
                      public BlockConfigHandlers::RHandler {

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

 explicit CRBlockConfig( bool diff = true )
  : BlockConfig( diff ) , CHandler() , RHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an CRBlockConfig out of the given netCDF \p group
 /** It constructs an CRBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an CRBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        CRBlockConfig. */

 explicit CRBlockConfig( const netCDF::NcGroup & group )
  : BlockConfig() , CHandler() , RHandler() {
   CRBlockConfig::deserialize( group );
   }

/*--------------------------------------------------------------------------*/
 /// constructs an CRBlockConfig out of an istream
 /** It constructs an CRBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * CRBlockConfig.
  *
  * @param input The istream containing the description of the
  *        CRBlockConfig. */

 explicit CRBlockConfig( std::istream & input )
  : BlockConfig() , CHandler() , RHandler() {
   CRBlockConfig::load( input );
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

 explicit CRBlockConfig( Block * block , bool diff = false )
  : BlockConfig( diff ) , CHandler() , RHandler() {
   CRBlockConfig::get( block );
   }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 CRBlockConfig( const CRBlockConfig & old )
  : BlockConfig( old ) , CHandler( old ) , RHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 CRBlockConfig( CRBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , CHandler( std::move( old ) ) ,
  RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although CRBlockConfig is formally unrelated to OCRBlockConfig, the
  * latter in fact is a "superset" of the former, and therefore it can be
  * used to move-construct it. */

 explicit CRBlockConfig( OCRBlockConfig && old )
  : BlockConfig( std::move( old ) ) , CHandler( std::move( old ) ) ,
  RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 CRBlockConfig & operator=( const CRBlockConfig & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends BlockConfig::deserialize( netCDF::NcGroup )
 /** Extends BlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a CRBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should also contain the following:
  *
  * - all that is needed to describe a BlockConfig
  *   (see BlockConfig::deserialize()):
  *
  * - all that is needed to describe a CBlockConfig
  *   (see CBlockConfig::deserialize()):
  *
  * - all that is needed to describe a RBlockConfig
  *   (see RBlockConfig::deserialize()). */

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ! ( BlockConfig::empty() && CHandler::empty() && RHandler::empty() ) )
   throw( std::logic_error( "deserializing a non-empty CRBlockConfig" ) );

  BlockConfig::deserialize( group );
  CHandler::deserialize( group );
  RHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~CRBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the CRBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Constraint) and stores in this CRBlockConfig. This information
  * consists of that supported by the RBlockConfig (see RBlockConfig::get())
  * plus any ComputeConfig that may be associated with the Constraint of the
  * given Block.
  *
  * If num_ComputeConfig_Constraint() returns zero then every Constraint in
  * the given \p block is inspected. In this case, for each Constraint of the
  * given \p block, its ComputeConfig is stored in this CRBlockConfig if it
  * has a non-default set of parameters.
  *
  * If num_ComputeConfig_Constraint() returns a nonzero value, only the
  * ComputeConfig associated with the Constraint (of the given \p block)
  * handled by this CRBlockConfig (see get_Constraint_id()) are
  * considered. Moreover, if the pointer to a ComputeConfig in this
  * CRBlockConfig is not nullptr, it is used to retrieve the configuration of
  * its associated Constraint (see ThinComputeInterface::get_ComputeConfig()).
  *
  * If num_ComputeConfig_Constraint() returns a nonzero value but one wants
  * all Constraint of the given Block to be inspected (for instance, one is
  * not sure that every Constraint that is not handled by this CRBlockConfig
  * has a default set of parameters), then all ComputeConfig should be removed
  * before calling this method (see remove_ComputeConfig_Constraint()).
  *
  * Note that if num_ComputeConfig_Constraint() returns zero then
  *
  *     CALLING CRBlockConfig::get() IS A POTENTIALLY COSTLY
  *     OPERATION BECAUSE IT ENTAILS SCANNING ALL Constraint
  *     OF THE Block IN ORDER TO OBTAIN THEIR ComputeConfig.
  *
  * @param block A pointer to the Block whose CRBlockConfig must be filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  CHandler::get( block );
  RHandler::get( block );
 }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE CRBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the CRBlockConfig
 *  @{ */

 /// configure the given Block and its Constraint
 /** Method for configuring the given Block and its Constraint. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block, deleteold );
  CHandler::apply( block, deleteold, f_diff );
  RHandler::apply( block, deleteold, f_diff );
  }

/*--------------------------------------------------------------------------*/
 /// clear this CRBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  CHandler::clear();
  RHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] CRBlockConfig * clone( void ) const override {
  return( new CRBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE CRBlockConfig -------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the CRBlockConfig
 *  @{ */

 /// extends RBlockConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an CRBlockConfig. See CRBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  CHandler::serialize( group );
  RHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------------------ FRIENDS -----------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class CBlockConfig;
 // make CBlockConfig friend for the weird move-constructor

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the CRBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  CHandler::print( output );
  RHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this CRBlockConfig out of an istream
 /** Load this CRBlockConfig out of an istream. The format is:
  *
  * - that specified in BlockConfig::load(), followed by
  *
  * - that specified in CBlockConfig::load(), followed by
  *
  * - that specified in RBlockConfig::load(). */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  CHandler::load( input );
  RHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( CRBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ORBlockConfig ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// BlockConfig for configuring Objective and sub-Block (recursively)

class ORBlockConfig : public BlockConfig ,
                      public BlockConfigHandlers::OHandler ,
                      public BlockConfigHandlers::RHandler {

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

 explicit ORBlockConfig( bool diff = true )
  : BlockConfig( diff ) , OHandler() , RHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an ORBlockConfig out of the given netCDF \p group
 /** It constructs an ORBlockConfig out of the given netCDF \p group.  Please
  * refer to the deserialize() method for the format of the netCDF::NcGroup of
  * a ORBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        ORBlockConfig. */

 explicit ORBlockConfig( const netCDF::NcGroup & group ) : BlockConfig() ,
  OHandler() , RHandler() { ORBlockConfig::deserialize( group ); }

/*--------------------------------------------------------------------------*/
 /// constructs an ORBlockConfig out of an istream
 /** It constructs an ORBlockConfig out of the given istream \p input.
  * Please refer to the load() method for the format of an ORBlockConfig.
  *
  * @param input The istream containing the description of the
  *        ORBlockConfig. */

 explicit ORBlockConfig( std::istream & input ) : BlockConfig() ,
  OHandler() , RHandler() { ORBlockConfig::load( input ); }

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

 explicit ORBlockConfig( Block * block, bool diff = false )
  : BlockConfig( diff ) , OHandler() , RHandler() {
  ORBlockConfig::get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 ORBlockConfig( const ORBlockConfig & old ) : BlockConfig( old ) ,
  OHandler( old ) , RHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 ORBlockConfig( ORBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) ,
  RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although ORBlockConfig is formally unrelated to OCRBlockConfig, the
  * latter in fact is a "superset" of the former, and therefore it can be
  * used to move-construct it. */

 explicit ORBlockConfig( OCRBlockConfig && old )
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) ,
  RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 ORBlockConfig & operator=( const ORBlockConfig & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends RBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends RBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of an ORBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should also contain the following:
  *
  * - all that is needed to describe a BlockConfig
  *   (see BlockConfig::deserialize()):
  *
  * - all that is needed to describe a OBlockConfig
  *   (see OBlockConfig::deserialize()):
  *
  * - all that is needed to describe a RBlockConfig
  *   (see RBlockConfig::deserialize()). */

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ! ( BlockConfig::empty() && OHandler::empty() && RHandler::empty() ) )
   throw( std::logic_error( "deserializing a non-empty ORBlockConfig" ) );

  BlockConfig::deserialize( group );
  OHandler::deserialize( group );
  RHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~ORBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the ORBlockConfig of the given Block
 /** This method gets information about the current set of parameters of the
  * given Block (and its Objective and sub-Block, recursively) and stores in
  * this ORBlockConfig. This information consists of that supported by the
  * RBlockConfig (see RBlockConfig::get()) plus the ComputeConfig of the
  * Objective of the given Block. If the pointer to the ComputeConfig
  * currently stored in this ORBlockConfig is not nullptr, it is used to
  * retrieve the configuration of the Objective of the given \p block (see
  * ThinComputeInterface::get_ComputeConfig()).
  *
  * @param block A pointer to the Block whose ORBlockConfig must be filled.
  */

 void get( Block * block ) override {
  BlockConfig::get( block );
  OHandler::get( block );
  RHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE ORBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the ORBlockConfig
 *  @{ */

 /// configure the given Block, its Objective, and its sub-Block (recursively)
 /** Method for configuring the given Block, its Objective and all its
  * sub-Block, recursively. The configuration depends on the field #f_diff,
  * which indicates whether it has to be interpreted in "differential
  * mode". Please refer to Block::set_BlockConfig() for understanding how
  * #f_diff and \p deleteold affect the configuration of a Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current Configuration of Block must
  *        be deleted (see BlockConfig::apply()). */

 void apply( Block * block, bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block, deleteold );
  OHandler::apply( block, deleteold, f_diff );
  RHandler::apply( block, deleteold, f_diff );
  }

/*--------------------------------------------------------------------------*/
 /// clear this ORBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  OHandler::clear();
  RHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] ORBlockConfig * clone( void ) const override {
  return( new ORBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE ORBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the ORBlockConfig
 * @{ */

 /// extends RBlockConfig::serialize( netCDF::NcGroup )
 /** Extends RBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an ORBlockConfig. See ORBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  OHandler::serialize( group );
  RHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*------------------------------ FRIENDS -----------------------------------*/
/*--------------------------------------------------------------------------*/

 friend class OBlockConfig;
 // make OBlockConfig friend for its weird move-constructor

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the ORBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  OHandler::print( output );
  RHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this ORBlockConfig out of an istream
 /** Load this ORBlockConfig out of an istream. The format is:
  *
  * - that specified in BlockConfig::load(), followed by
  *
  * - that specified in OBlockConfig::load(), followed by
  *
  * - that specified in RBlockConfig::load(). */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  OHandler::load( input );
  RHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( ORBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OCBlockConfig ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// BlockConfig for configuring Objective and Constraints

class OCBlockConfig : public BlockConfig ,
                      public BlockConfigHandlers::OHandler ,
                      public BlockConfigHandlers::CHandler {

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

 explicit OCBlockConfig( bool diff = true )
  : BlockConfig( diff ) , OHandler() , CHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an OCBlockConfig out of the given netCDF \p group
 /** It constructs an OCBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OCBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OCBlockConfig. */

 explicit OCBlockConfig( const netCDF::NcGroup & group ) : BlockConfig() ,
  OHandler() , CHandler() { OCBlockConfig::deserialize( group ); }

/*--------------------------------------------------------------------------*/
 /// constructs an OCBlockConfig out of an istream
 /** It constructs an OCBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OCBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OCBlockConfig. */

 explicit OCBlockConfig( std::istream & input ) : BlockConfig() ,
  OHandler() , CHandler() { OCBlockConfig::load( input ); }

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

 explicit OCBlockConfig( Block * block , bool diff = false )
  : BlockConfig( diff ) , OHandler() , CHandler() {
  OCBlockConfig::get( block );
  }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OCBlockConfig( const OCBlockConfig & old ) : BlockConfig( old ) ,
  OHandler( old ) , CHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 OCBlockConfig( OCBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) ,
  CHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although OCBlockConfig is formally unrelated to OCRBlockConfig, the
  * latter in fact is a "superset" of the former, and therefore it can be
  * used to move-construct it. */

 explicit OCBlockConfig( OCRBlockConfig && old )
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) ,
  CHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 OCBlockConfig & operator=( const OCBlockConfig & ) = delete;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// extends CBlockConfig::deserialize( netCDF::NcGroup )
 /** Extends CBlockConfig::deserialize( netCDF::NcGroup ) to the specific
  * format of a OCBlockConfig. Besides the mandatory "type" attribute of any
  * :Configuration, the group should also contain the following:
  *
  * - all that is needed to describe a BlockConfig
  *   (see BlockConfig::deserialize()):
  *
  * - all that is needed to describe a OBlockConfig
  *   (see OBlockConfig::deserialize()):
  *
  * - all that is needed to describe a CBlockConfig
  *   (see CBlockConfig::deserialize()). */

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ! ( BlockConfig::empty() && OHandler::empty() && CHandler::empty() ) )
   throw( std::logic_error( "deserializing a non-empty OCBlockConfig" ) );

  BlockConfig::deserialize( group );
  OHandler::deserialize( group );
  CHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~OCBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the OCBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OCBlockConfig. This information consists
  * of that supported by the CBlockConfig (see CBlockConfig::get()) plus the
  * ComputeConfig that may be associated with the Objective of the given
  * Block. If the pointer to the ComputeConfig currently stored in this
  * OCBlockConfig is not nullptr, it is used to retrieve the configuration of
  * the Objective of the given \p block (see
  * ThinComputeInterface::get_ComputeConfig()).
  *
  * @param block A pointer to the Block whose OCBlockConfig must be filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  OHandler::get( block );
  CHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE OCBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OCBlockConfig
 *  @{ */

 /// configure the given Block, its Objective and its Constraint
 /** Method for configuring the given Block, its Objective and its
  * Constraint. The configuration depends on the field #f_diff, which
  * indicates whether it has to be interpreted in "differential mode".
  * Please refer to Block::set_BlockConfig() for understanding how #f_diff
  * and \p deleteold affect the configuration of a Block.
  *
  * @param block A pointer to the Block that must be configured.
  *
  * @param deleteold Indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block, deleteold );
  OHandler::apply( block, deleteold, f_diff );
  CHandler::apply( block, deleteold, f_diff );
 }

/*--------------------------------------------------------------------------*/
 /// clear this OCBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  OHandler::clear();
  CHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] OCBlockConfig * clone( void ) const override {
  return( new OCBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------- METHODS FOR LOADING, PRINTING & SAVING THE OCBlockConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OCBlockConfig
 *  @{ */

 /// extends CBlockConfig::serialize( netCDF::NcGroup )
 /** Extends CBlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an OCBlockConfig. See OCBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  OHandler::serialize( group );
  CHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OCBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  OHandler::print( output );
  CHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this OCBlockConfig out of an istream
 /** Load this OCBlockConfig out of an istream. The format is:
  *
  * - that specified in BlockConfig::load(), followed by
  *
  * - that specified in OBlockConfig::load(), followed by
  *
  * - that specified in CBlockConfig::load(). */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  OHandler::load( input );
  CHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( OCBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the Objective of a Block
/** The OBlockConfig class ("Objective" BlockConfig) derives from BlockConfig
 * and offers support for configuring the Objective of a Block. Besides the
 * fields in BlockConfig, the OBlockConfig contains the following field:
 *
 * - a pointer to ComputeConfig for the Objective of the Block.
 */

class OBlockConfig : public BlockConfig ,
                     public BlockConfigHandlers::OHandler {

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

 explicit OBlockConfig( bool diff = true )
  : BlockConfig( diff ) , OHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an OBlockConfig out of the given netCDF \p group
 /** It constructs an OBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an OBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        OBlockConfig. */

 explicit OBlockConfig( const netCDF::NcGroup & group ) : BlockConfig() ,
  OHandler() { OBlockConfig::deserialize( group ); }

/*--------------------------------------------------------------------------*/
 /// constructs an OBlockConfig out of an istream
 /** It constructs an OBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * OBlockConfig.
  *
  * @param input The istream containing the description of the
  *        OBlockConfig. */

 explicit OBlockConfig( std::istream & input ) : BlockConfig() ,
  OHandler() { OBlockConfig::load( input ); }

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

 explicit OBlockConfig( Block * block , bool diff = false )
  : BlockConfig( diff ) , OHandler() { OBlockConfig::get( block ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 OBlockConfig( const OBlockConfig & old )
  : BlockConfig( old ) , OHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 OBlockConfig( OBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although OBlockConfig is formally unrelated to ORBlockConfig, the
  * latter in fact is a "superset" of the former, and therefore it can be
  * used to move-construct it. */

 explicit OBlockConfig( ORBlockConfig && old )
  : BlockConfig( std::move( old ) ) , OHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 OBlockConfig & operator=( const OBlockConfig & ) = delete;

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

 void deserialize( const netCDF::NcGroup & group ) override {
  BlockConfig::deserialize( group );
  OHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~OBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the OBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Objective) and stores in this OBlockConfig. This information consists
  * of that supported by the BlockConfig (see BlockConfig::get()) plus the
  * ComputeConfig that may be associated with the Objective of the given
  * Block. If the pointer to the ComputeConfig currently stored in this
  * OBlockConfig is not nullptr, it is used to retrieve the configuration of
  * the Objective of the given \p block (see
  * ThinComputeInterface::get_ComputeConfig()).
  *
  * @param block A pointer to the Block whose OBlockConfig must be filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  OHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE OBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the OBlockConfig
 *  @{ */

 /// configure the given Block and its Objective
 /** Method for configuring the given Block and its Objective. The
  * configuration depends on the field #f_diff, which indicates whether it has
  * to be interpreted in "differential mode". Please refer to
  * Block::set_BlockConfig() for understanding how #f_diff and \p deleteold
  * affect the configuration of a Block.
  *
  * @param block a pointer to the Block that must be configured.
  *
  * @param deleteold indicates whether the current BlockConfig of Block must
  *        be deleted. */

 void apply( Block * block , bool deleteold = true ) override {
  BlockConfig::apply( block , deleteold );
  OHandler::apply( block , deleteold , f_diff );
 }

/*--------------------------------------------------------------------------*/
 /// clear this OBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  OHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] OBlockConfig * clone( void ) const override {
  return( new OBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE OBlockConfig ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the OBlockConfig
 *  @{ */

 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an OBlockConfig. See OBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  OHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the OBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  OHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this OBlockConfig out of an istream
 /** Load this OBlockConfig out of an istream. The format is defined as that
  * specified in BlockConfig::load(), followed by the information describing
  * the corresponding ComputeConfig in the format accepted by
  * Configuration::deserialize( std::istream ), with all the corresponding
  * input options, like '*' for  nullptr and "*<filename>"  for loading it
  * out of a different file. */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  OHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( OBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the Constraint of a Block
/** The CBlockConfig class ("Constraint" BlockConfig) derives from BlockConfig
 * and offers support for configuring the Constraint of a Block. Besides the
 * fields in BlockConfig, the CBlockConfig contains the following fields:
 *
 * - a vector identifying the set of Constraint that require a ComputeConfig
 *
 * - a vector of pointers to ComputeConfig for each indicated Constraint of a
 *   Block. */

class CBlockConfig : public BlockConfig ,
                     public BlockConfigHandlers::CHandler {

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

 explicit CBlockConfig( bool diff = true )
  : BlockConfig( diff ) , CHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig out of the given netCDF \p group
 /** It constructs an CBlockConfig out of the given netCDF \p
  * group. Please refer to the deserialize() method for the format of a
  * netCDF::NcGroup of an CBlockConfig.
  *
  * @param group the netCDF::NcGroup containing the description of the
  *        CBlockConfig. */

 explicit CBlockConfig( const netCDF::NcGroup & group )
  : BlockConfig() , CHandler() { CBlockConfig::deserialize( group ); }

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig out of an istream
 /** It constructs an CBlockConfig out of the given istream \p
  * input. Please refer to the load() method for the format of an
  * CBlockConfig.
  *
  * @param input the istream containing the description of the
  *        CBlockConfig. */

 explicit CBlockConfig( std::istream & input )
  : BlockConfig() , CHandler() { CBlockConfig::load( input ); }

/*--------------------------------------------------------------------------*/
 /// constructs an CBlockConfig for the given Block
 /** It constructs an CBlockConfig for the given \p block. It creates
  * an empty CBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which an CBlockConfig will
  *        be constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one. */

 explicit CBlockConfig( Block * block , bool diff = false )
  : BlockConfig( diff ) , CHandler() { CBlockConfig::get( block ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 CBlockConfig( const CBlockConfig & old )
  : BlockConfig( old ) , CHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 CBlockConfig( CBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , CHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although CBlockConfig is formally unrelated to CRBlockConfig, the latter
  * in fact is a "superset" of the former, and therefore it can be used to
  * move-construct it. */

 explicit CBlockConfig( CRBlockConfig && old ) :
  BlockConfig( std::move( old ) ) , CHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from another formally unrelated class
 /** Although CBlockConfig is formally unrelated to OCRBlockConfig, the latter
  * in fact is a "superset" of the former, and therefore it can be used to
  * move-construct it. */

 explicit CBlockConfig( OCRBlockConfig && old )
  : BlockConfig( std::move( old ) ) , CHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 CBlockConfig & operator=( const CBlockConfig & ) = delete;

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
  *       - THE STATIC GROUP HAS PRIORITY OVER THE DYNAMIC GROUP OF
  *         Constraint: IF THERE IS A GROUP OF STATIC Constraint WITH THE
  *         GIVEN NAME, THEN THIS GROUP IS CONSIDERED. OTHERWISE, THE GROUP
  *         OF DYNAMIC Constraint WITH THAT NAME IS CONSIDERED.
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

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ( ! v_Config_Constraint.empty() ) || ( ! v_Constraint_id.empty() ) )
   throw( std::logic_error( "CBlockConfig::deserialize: deserializing a "
			    "non-empty CBlockConfig" ) );

  BlockConfig::deserialize( group );
  CHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~CBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the CBlockConfig of the given Block
 /** This method gets information about the parameter of the given Block (and
  * its Constraint) and stores in this CBlockConfig. This information consists
  * of that supported by the BlockConfig (see BlockConfig::get()) plus any
  * ComputeConfig that may be associated with the Constraint of the given
  * Block.
  *
  * If num_ComputeConfig_Constraint() returns zero then every Constraint in
  * the given \p block is inspected. In this case, for each Constraint of the
  * given \p block, its ComputeConfig is stored in this CBlockConfig if it has
  * a non-default set of parameters.
  *
  * If num_ComputeConfig_Constraint() returns a nonzero value, only the
  * ComputeConfig associated with the Constraint (of the given \p block)
  * handled by this CBlockConfig (see get_Constraint_id()) are
  * considered. Moreover, if the pointer to a ComputeConfig in this
  * CBlockConfig is not nullptr, it is used to retrieve the configuration of
  * its associated Constraint (see ThinComputeInterface::get_ComputeConfig()).
  *
  * If num_ComputeConfig_Constraint() returns a nonzero value but one wants
  * all Constraint of the given Block to be inspected (for instance, one is
  * not sure that every Constraint that is not handled by this CBlockConfig
  * has a default set of parameters), then all ComputeConfig should be removed
  * before calling this method (see remove_ComputeConfig_Constraint()).
  *
  * Note that if num_ComputeConfig_Constraint() returns zero then
  *
  *     CALLING CBlockConfig::get() IS A POTENTIALLY COSTLY
  *     OPERATION BECAUSE IT ENTAILS SCANNING ALL Constraint
  *     OF THE Block IN ORDER TO OBTAIN THEIR ComputeConfig.
  *
  * @param block A pointer to the Block whose CBlockConfig must be filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  CHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF THE CBlockConfig ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the CBlockConfig
 *  @{ */

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

 void apply( Block * block , bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block , deleteold );
  CHandler::apply( block , deleteold , f_diff );
  }

/*--------------------------------------------------------------------------*/
 /// clear this CBlockConfig

 void clear( void ) override {
  BlockConfig::clear();
  CHandler::clear();
  }

/*------------------------------- CLONE ------------------------------------*/

 [[nodiscard]] CBlockConfig * clone( void ) const override {
  return( new CBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE CBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the CBlockConfig
 *  @{ */

 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of an CBlockConfig. See CBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  CHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the CBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  CHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this CBlockConfig out of an istream
 /** Load this CBlockConfig out of an istream. The format is defined as that
  * specified in BlockConfig::load(), followed by:
  *
  * - the number k of the ComputeConfig for the Constraint of the Block
  *
  * - for i = 1 ... k
  *   - the identification of the Constraint
  *   - the information describing the corresponding ComputeConfig in the 
  *     format accepted by Configuration::deserialize( std::istream ), with
  *     all the corresponding input options, like '*' for  nullptr and
  *     "*<filename>" for loading it out of a different file
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

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  CHandler::load( input );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( CBlockConfig ) )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS RBlockConfig ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from BlockConfig for configuring the sub-Block of a Block
/** The RBlockConfig class ("recursive" BlockConfig) derives from BlockConfig
 * and offers support for configuring also the sub-Block of a Block
 * (recursively). Besides the fields in BlockConfig, the RBlockConfig contains
 * the following fields:
 *
 * - a vector of pointers to BlockConfig for (some of) the sub-Block of the
 *   Block;
 *
 * - a vector associating each BlockConfig to a sub-Block of the Block. */

class RBlockConfig : public BlockConfig ,
                     public BlockConfigHandlers::RHandler {

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

 explicit RBlockConfig( bool diff = true )
  : BlockConfig( diff ), RHandler() {}

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig out of the given netCDF \p group
 /** Constructs a RBlockConfig out of the given netCDF \p group; refer to the
  * deserialize() method for the format of the netCDF::NcGroup of a
  * RBlockConfig.
  *
  * @param group The netCDF::NcGroup containing the description of the
  *        RBlockConfig. */

 explicit RBlockConfig( const netCDF::NcGroup & group )
  : BlockConfig() , RHandler() { RBlockConfig::deserialize( group ); }

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig out of an istream
 /** Constructs a RBlockConfig out of the given istream \p input; refer to
  * the load() method for the format of a RBlockConfig.
  *
  * @param input The istream containing the description of the
  *        RBlockConfig. */

 explicit RBlockConfig( std::istream & input )
  : BlockConfig() , RHandler() { RBlockConfig::load( input ); }

/*--------------------------------------------------------------------------*/
 /// constructs a RBlockConfig for the given Block
 /** Constructs a RBlockConfig for the given \p block. It creates an empty
  * RBlockConfig and invoke the method get().
  *
  * @param block A pointer to the Block for which a RBlockConfig will be
  *        constructed.
  *
  * @param diff It indicates if this configuration is a "differential" one.
  */

 explicit RBlockConfig( Block * block , bool diff = false )
  : BlockConfig( diff ) { RBlockConfig::get( block ); }

/*--------------------------------------------------------------------------*/
 /// copy constructor: does what it says on the tin

 RBlockConfig( const RBlockConfig & old )
  : BlockConfig( old ) , RHandler( old ) {}

/*--------------------------------------------------------------------------*/
 /// move constructor: does what it says on the tin

 RBlockConfig( RBlockConfig && old ) noexcept
  : BlockConfig( std::move( old ) ) , RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// weird move constructor from formally unrelated class
 /** Although RBlockConfig is formally unrelated to CRBlockConfig, the
  * latter in fact is a "superset" of the former, and therefore it can be
  * used to move-construct it. */

 explicit RBlockConfig( CRBlockConfig && old )
  : BlockConfig( std::move( old ) ) , RHandler( std::move( old ) ) {}

/*--------------------------------------------------------------------------*/
 /// copy assignment operator: it is deleted

 RBlockConfig & operator=( const RBlockConfig & ) = delete;

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

 void deserialize( const netCDF::NcGroup & group ) override {
  if( ! ( BlockConfig::empty() && RHandler::empty() ) )
   throw( std::logic_error( "deserializing a non-empty RBlockConfig" ) );

  BlockConfig::deserialize( group );
  RHandler::deserialize( group );
  }

/*------------------------------ DESTRUCTOR --------------------------------*/

 /// destructor: silently calls the destructors of base classes

 ~RBlockConfig() override = default;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// getting the RBlockConfig of the given Block
 /** This method gets information about the current set of parameters of the
  * given Block (and its sub-Block, recursively) and stores it in this
  * RBlockConfig. This information consists of that supported by the base
  * BlockConfig (see BlockConfig::get()) plus the BlockConfig of each
  * sub-Block of the given Block. Notice that any existing Configuration
  * currently in the BlockConfig is deleted (see BlockConfig::get()) and the
  * Configuration stored in the BlockConfig of the given Block are cloned
  * into the corresponding Configuration of this BlockConfig.
  *
  * If num_sub_BlockConfig() returns a nonzero value, only the BlockConfig
  * associated with the sub-Block (of the given \p block) currently being
  * handled by this RBlockConfig (see get_sub_Block_id()) are retrieved.
  *
  * If num_sub_BlockConfig() returns zero then the BlockConfig of every
  * sub-Block in the given \p block is retrieved. In this case, for each
  * sub-Block of the given \p block, its BlockConfig is stored in this
  * RBlockConfig.
  *
  * This means that if one wants all sub-Block to be inspected (for instance,
  * one is not sure that every sub-Block is handled by this RBlockConfig; or
  * one is not sure the pointers to the BlockConfig in this RBlockConfig are
  * of the right type), all BlockConfig should be removed before calling this
  * method (see remove_sub_BlockConfig()).
  *
  * @param block A pointer to the Block whose RBlockConfig must be filled. */

 void get( Block * block ) override {
  BlockConfig::get( block );
  RHandler::get( block );
  }

/** @} ---------------------------------------------------------------------*/
/*---------- METHODS DESCRIBING THE BEHAVIOR OF THE RBlockConfig -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the RBlockConfig
 *  @{ */

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

 void apply( Block * block , bool deleteold = true ) override {
  if( ! block ) return;
  BlockConfig::apply( block , deleteold );
  RHandler::apply( block , deleteold , f_diff );
  }

/*--------------------------------------------------------------------------*/
 /// clear this RBlockConfig
 /** This method clears this RBlockConfig by first calling
  * BlockConfig::clear() and then invoking clear() for each BlockConfig
  * currently being handled by this RBlockConfig. */

 void clear( void ) override {
  BlockConfig::clear();
  RHandler::clear();
  }

/*-------------------------------- CLONE -----------------------------------*/

 [[nodiscard]] RBlockConfig * clone( void ) const override {
  return( new RBlockConfig( *this ) );
  }

/** @} ---------------------------------------------------------------------*/
/*--------- METHODS FOR LOADING, PRINTING & SAVING THE RBlockConfig --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the RBlockConfig
 * @{ */

 /// extends BlockConfig::serialize( netCDF::NcGroup )
 /** Extends BlockConfig::serialize( netCDF::NcGroup ) to the specific format
  * of a RBlockConfig. See RBlockConfig::deserialize( netCDF::NcGroup ) for
  * details of the format of the created netCDF group. */

 void serialize( netCDF::NcGroup & group ) const override {
  BlockConfig::serialize( group );
  RHandler::serialize( group );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the RBlockConfig

 void print( std::ostream & output ) const override {
  BlockConfig::print( output );
  RHandler::print( output );
  }

/*--------------------------------------------------------------------------*/
 /// load this RBlockConfig out of an istream
 /** Load this RBlockConfig out of an istream. The format is defined as that
  * of BlockConfig (see BlockConfig::load()) followed by:
  *
  * - a number k such that abs(k) is the number of the sub-BlockConfig objects
  *
  * for i = 1 ... abs(k)
  *  - if k < 0, the identification of the sub-Block
  *  - the information describing the corresponding *BlockConfig in the 
  *     format accepted by Configuration::deserialize( std::istream ), with
  *     all the corresponding input options, like '*' for  nullptr and
  *     "*<filename>" for loading it out of a different file
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
  *     THE FIRST CHARACTER OF THIS NAME CANNOT BE A DIGIT. */

 void load( std::istream & input ) override {
  BlockConfig::load( input );
  RHandler::load( input );
  }

/*---------------------- PROTECTED FIELDS OF THE CLASS ---------------------*/

 /// the vector of sub-BlockConfig for each of the sub-Block of the Block
 std::vector< BlockConfig * > v_sub_BlockConfig;

 /// correspondence between v_sub_BlockConfig and the sub-Block of the Block
 /** This vector specifies the correspondence between the BlockConfig in
  * #v_sub_BlockConfig and the sub-Block of the Block. v_sub_Block_id[ i ]
  * contains the identification of the sub-Block whose BlockConfig is
  * v_sub_BlockConfig[ i ]. A sub-Block can be identified either by its name
  * (see Block::name()) or by its index in the list of sub-Blocks of its
  * father Block. If the name of the sub-Block is used, then the first
  * character of this name cannot be a digit. */
 std::vector< std::string > v_sub_Block_id;

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};  // end( class( RBlockConfig ) )

/*--------------------------------------------------------------------------*/
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

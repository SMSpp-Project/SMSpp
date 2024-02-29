/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class AbstractBlock, which implements the Block
 * concept in two relevant cases:
 *
 * 1) THE Block ONLY HAVE THE ABSTRACT REPRESENTATION, WHICH IS EXTERNALLY
 *    PROVIDED, I.E., IT IS PROGRAMMATICALLY CONSTRUCTED BY SOME PIECE OF
 *    CODE OUTSIDE THE Block ITSELF.
 *
 *    This means that a lot of Block mechanisms do not apply, but on the
 *    other hand a few mechanisms that are usually thought to be implemented
 *    by derived classes, and therefore are protected, need to be made
 *    public.
 *
 * 2) THE Block HAS "PARTLY" A SPECIFIC STRUCTURE, BUT THEN A (MORE OR LESS)
 *    ARBITRARY SET OF Variable AND Constraint CAN BE ADDED WITH BASICALLY
 *    ANY STRUCTURE.
 *
 *    The idea is that any such Block can then be implemented into a derived
 *    class PartiallyStructuredBlock : AbstractBlock, where the derived class
 *    manages the "specific structure" part and AbstractBlock the "arbitrary"
 *    one. For this purpose, AbstractBlock has a few methods that allow
 *    derived classes to "reserve for themselves a part of the abstract
 *    representation", ensuring that AbstractBlock will not mess up with it
 *    (but, on the other hand, completely making the derived class' job to
 *    handle all its aspects).
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AbstractBlock
 #define __AbstractBlock
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
/*------------------------- CLASS AbstractBlock ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// implementation of the Block concept for the "purely abstract" case
/** The class AbstractBlock implements the Block concept for the case for 
 * two relevant cases:
 *
 * 1) THE Block ONLY HAVE THE ABSTRACT REPRESENTATION, WHICH IS EXTERNALLY
 *    PROVIDED, I.E., IT IS PROGRAMMATICALLY CONSTRUCTED BY SOME PIECE OF
 *    CODE OUTSIDE THE Block ITSELF.
 *
 *    This means that a lot of Block mechanisms do not apply, but on the
 *    other hand a few mechanisms that are usually thought to be implemented
 *    by derived classes, and therefore are protected, need to be made
 *    public.
 *
 * 2) THE Block HAS "PARTLY" A SPECIFIC STRUCTURE, BUT THEN A (MORE OR LESS)
 *    ARBITRARY SET OF Variable AND Constraint CAN BE ADDED WITH BASICALLY
 *    ANY STRUCTURE.
 *
 *    The idea is that any such Block can then be implemented into a derived
 *    class PartiallyStructuredBlock : AbstractBlock, where the derived class
 *    manages the "specific structure" part and AbstractBlock the "arbitrary"
 *    one. For this purpose, AbstractBlock has a few methods that allow
 *    derived classes to "reserve for themselves a part of the abstract
 *    representation", ensuring that AbstractBlock will not mess up with it
 *    (but, on the other hand, completely making the derived class' job to
 *    handle all its aspects).
 *
 * The current implementation of AbstractBlock requires (for the "arbitrary"
 * part, as any specific part need necessarily be handled by a derived class):
 *
 * - all Variable to be ColVariable;
 *
 * - all Constraint to be either FRowConstraint or (any concrete class
 *   derived from the abstract) OneVarConstraint;
 *
 * - the Objective to be a FRealObjective.
 *
 * These are to be constructed outside and passed to the object via the
 * corresponding add_[static/dynamic]_[constraint/variable]() methods, that
 * for this reason *are made public* (while they are protected in the base
 * Block class), together with the other ones for handling the corresponding
 * data structures. Similarly, AbstractBlock allows unfettered access to the
 * list of inner Block by returning a non-const reference to the
 * corresponding vector of pointers (unlike Block). Of course, changes in
 * the size of these vectors are *not* supposed to happen "in flight", i.e.,
 * when there is any Solver attached to the AbstractBlock, but only during
 * the initial construction phase. Yet, during the destructor of
 * AbstractBlock, the corresponding Variable / Constraint / Objective have
 * to be destroyed. This requires the( pointer to the)m to be fetched out of
 * the corresponding std::vector< boost::any >, which can only be done by
 * knowing their exact type; whence the need of specifying exactly which
 * types are handled by the class.
 *
 * Note that, conversely, add_dynamic_constraint*s*() and
 * add_dynamic_variable*s*() (the methods for adding/removing stuff from an
 * existing list, rather than for creating a new group of constraint, which
 * in the dynamic case entails one or more lists), are already public in the
 * base class. While "concrete" Block may want to redefine them, the
 * implementations in the base class do all that is needed here.
 *
 * In AbstractBlock, *the abstract representation is the physical
 * representation*: there is no other reference to the Variable/Constraint
 * save that in those data structures. Hence, the destructor of AbstractBlock
 * (unlike that of Block) takes care of (clear()-ing first, and) destroying
 * (then) all the "abstract" Constraint/Variable (and the Objective).
 * Similarly, the pointers to the inner Block in the v_Block vector are
 * likely the only live references to these Block, and therefore they are
 * destroyed in the AbstractBlock destructor.
 *
 * Among other things, AbstractBlock can be a useful target for the
 * construction of some R3Block for some "concrete" Block.
 *
 * However, AbstractBlock also has a mechanism that allows to indicate that
 * some static/dynamic Variable/Constraint, inner Block, and the (only)
 * Objective, are "reserved for the derived class use". The "reserved" ones
 * are the first groups/inner Block, and therefore they are specified by the
 * following 6 protected fields
 *
 * - f_1st_stat_var: the first groups of static Variable;
 *
 * - f_1st_dyn_var: the first groups of dynamic Variable;
 *
 * - f_1st_stat_cnst: the first groups of static Constraint;
 *
 * - f_1st_dyn_cnst: the first groups of dynamic Constraint;
 *
 * - f_res_obj: the (only) Objective;
 *
 * - f_1st_sub_block: the first inner Block;
 *
 * are "reserved". These fields are set to 0/false by AbstractBlock, and
 * therefore have to be set to the proper value (if nonzero/true) by
 * derived classes, *before* generate_[abstract_*,objective]() are called,
 * so that the base AbstractBlock will not ever touch these elements.
 *
 * One may wonder why using protected fields rather than virtual methods.
 * The point is that this information has to be available during the
 * destructor of AbstractBlock, but virtual functions do not properly work
 * in there (the method of AbstractBlock is called, rather than the method
 * of the derived class); hence, the data has to be stored in a place where it
 * can be safely accessed by AbstractBlock. In fact, the "reserved" part of
 * AbstractBlock (not handled by the base class) must be directly handled by
 * the derived, which means that it must be completely destroyed in the
 * destructor of the derived class, which by definition is executed *before*
 * that of AbstractBlock; thus, the destructor of AbstractBlock must not
 * touch that part, and hence it has to know what the part is.
 *
 * It should also be noted that, to conform to the standard, the "abstract"
 * representation of an AbstractBlock has to be constructed within
 * generate_[abstract_*,objective](). Yet, when an AbstractBlock is
 * deserialize()-d, its "abstract" representation is
 *
 *     OR, BETTER, WILL WHEN deserialize() WILL BE PROPERLY IMPLEMENTED
 *
 * be constructed right away. This creates a potential issue, since
 *
 *     WHAT GROUPS OF Variable/Constraint, sub-Block AND Objective ARE
 *     AVAILABLE DEPEND ON HOW THE "ABSTRACT" REPRESENTATION OF THE
 *     :AbstractBlock IS CONSTRUCTED, AND THEREFORE ON THE Configuration
 *     PARAMETERS PASSED TO generate_[abstract_*,objective](). THUS, THE
 *     VALUES OF f_1st_*_* AND f_res_obj ARE ONLY EXPECTED TO BE SIGNIFICANT
 *     INSIDE THE CALL TO generate_[abstract_*,objective]()
 *
 * This means that if AbstractBlock::generate_[abstract_*,objective]()
 * should be called inside the generate_[abstract_*,objective]() of any
 * derived class only after that the information on the reserved part has
 * been constructed. However, this still creates problem is the abstract
 * representation of the AbstractBlock actually has been loaded during
 * deserialize(). However, the add_X_Y() and set_X_Y() methods of Block
 * allow the derived class to "slip its group before the already present
 * ones", and therefore the derived class can ensure that the right set of
 * groups is reserved. Note that, unlike the std::vector< boost::any >
 * implementing the groups that are private, the v_Block vector of the
 * sub-Block is protected and therefore can be freely manipulated by
 * derived classes. Yet, doing all this right is entirely the responsibility
 * of the derived classes themselves. */

class AbstractBlock : public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING AbstractBlock ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing AbstractBlock
 *  @{ */

 /// constructor of AbstractBlock, taking a pointer to the father Block
 /** Constructor of AbstractBlock. It accepts a pointer to the father Block
  * (defaulting to nullptr, both because the root Block has no father and so
  * that this can also be used as the void constructor), passes it to the
  * Block constructor, and does little else except initializing all the
  * fields to default values. */

 explicit AbstractBlock( Block * father = nullptr ) : Block( father ) ,
  f_ub( Inf< double >() ) , f_lb( - Inf< double >() ) , f_ub_cond( false ) ,
  f_lb_cond( false ) , f_1st_stat_var( 0 ) , f_1st_dyn_var( 0 ) ,
  f_1st_stat_cnst( 0 ) , f_1st_dyn_cnst( 0 ) , f_res_obj( false ) ,
  f_1st_sub_block( 0 ) {}

/*--------------------------------------------------------------------------*/
 /// load the AbstractBlock out of an istream
 /** Method to deserialize the AbstractBlock out of an istream. It currently
  * supports two formats:
  *
  * - by default (frmt == 0 or frmt == 'M') the standard MPS (Mathematical
  *   Programming System) file format;
  *
  * - frmt == 'L' is the CPLEX LP file format (although this is not fully
  *   implemented yet). */

 void load( std::istream & input , char frmt = 0 ) override;

/*--------------------------------------------------------------------------*/
 /// de-serialize the current :AbstractBlock out of netCDF::NcGroup
 /** The AbstractBlock de-serializes itself out of a netCDF::NcGroup. The
  * method of the base AbstractBlock class only handles
  *
  *      OR, BETTER, WILL HANDLE WHEN THIS METHOD WILL BE IMPLEMENTED
  *
  * the "arbitrary" part, i.e., that after the "reserved" one as dictated
  * by the get_first_*_*() methods.
  *
  * In the current, partial implementation of the method, besides what is
  * managed by the serialize() method of the base Block class, the group
  * should contain the following:
  *
  * - the dimension "NumberInnerBlock", containing the number of the
  *   inner Block. The dimension is optional, it is not provided 0 is
  *   assumed.
  *
  * - if NumberInnerBlock > 0, and in particular it is
  *   > get_first_inner_Block(), then the groups "Block_< i >", for i = 0,. ...
  *   get_first_inner_Block() - 1; each one must contain the deserialization
  *   of the i-th inner Block.
  *
  * The method dispatches the protected guts_of_deserialize(), and then the
  * method of the base class, in this order. This is done so that the
  * necessary NBModification is issued last, while allowing derived classes
  * to call the (equivalent to) AbstractBlock::deserialize() at any point in
  * their deserialize() is they so need; see comments to
  * Block::deserialize() for a complete discussion. */

 void deserialize( const netCDF::NcGroup & group ) override {
  guts_of_deserialize( group );
  Block::deserialize( group );
  }

/*--------------------------------------------------------------------------*/
 /// destructor of AbstractBlock, destroys the abstract representation
 /** The destructor of AbstractBlock (unlike that of Block) takes care of
  * (clear()-ing first, and) destroying (then) all the "abstract"
  * Constraint/Variable (and the Objective). Similarly, the pointers to the
  * inner Block in the v_Block vector are likely the only live references to
  * these Block, and therefore they are destroyed in the AbstractBlock
  * destructor.
  *
  * Derived classes which specific structures will have to (but, anyway, they
  * necessarily have to) define their destructor to take care of them, which
  * is executed before that of AbstractBlock; this therefore assumes that all
  * specific structures have been dealt with and happily proceeds with
  * deleting the "arbitrary part". */

 ~AbstractBlock() override;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set a (either globally valid or conditionally valid) upper bound
 /** Upper bounds (either globally valid or conditionally valid ones) cannot
  * reasonably be computed by the AbstractBlock itself; so, if any is known
  * "from the outside", it has to be explicitly passed to the AbstractBlock
  * so that the latter can rely it to any attached Solver. Only one of the
  * two kinds of bounds makes sense at any given time (if a globally valid
  * upper bound is known then the problem is not unbounded above and there
  * is no point in checking for a conditionally valid one), so the method
  * sets the globally valid bound if the conditional parameter is false, and
  * the conditionally valid one otherwise, with the other being automatically
  * set to + infinity. */

 void set_valid_upper_bound( double newub = Inf< double >() ,
                             bool conditional = false ) {
  f_ub = newub;
  f_ub_cond = conditional;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// set a (either globally valid or conditionally valid) lower bound
 /** Lower bounds (either globally valid or conditionally valid ones) cannot
  * reasonably be computed by the AbstractBlock itself; so, if any is known
  * "from the outside", it has to be explicitly passed to the AbstractBlock
  * so that the latter can rely it to any attached Solver. Only one of the
  * two kinds of bounds makes sense at any given time (if a globally valid
  * lower bound is known then the problem is not unbounded below and there
  * is no point in checking for a conditionally valid one), so the method
  * sets the globally valid bound if the conditional parameter is false, and
  * the conditionally valid one otherwise, with the other being automatically
  * set to - infinity. */

 void set_valid_lower_bound( double newlb = - Inf< double >() ,
                             bool conditional = false ) {
  f_lb = newlb;
  f_lb_cond = conditional;
  }

/** @} ---------------------------------------------------------------------*/
/*------------- Methods for reading the data of the AbstractBlock ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the AbstractBlock
 *  @{ */

 /// returns the index of the first group of "available" static Constraint
 /** This method returns the index of the first group of static Constraint in
  * the "arbitrary" part of the AbstractBlock, by just returning the value of
  * the corresponding field. Setting which, however, is responsibility of
  * the derived classes. */

 Index get_first_static_Constraint( void ) const {
  return( f_1st_stat_cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" dynamic Constraint
 /** This method returns the index of the first group of dynamic Constraint in
  * the "arbitrary" part of the AbstractBlock, by just returning the value of
  * the corresponding field. Setting which, however, is responsibility of
  * the derived classes. */

 Index get_first_dynamic_Constraint( void ) const {
  return( f_1st_dyn_cnst );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" static Variable
 /** This method returns the index of the first group of static Variable in
  * the "arbitrary" part of the AbstractBlock, by just returning the value of
  * the corresponding field. Setting which, however, is responsibility of
  * the derived classes. */

 Index get_first_static_Variable( void ) const { return( f_1st_stat_var ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first group of "available" dynamic Variable
 /** This method returns the index of the first group of dynamic Variable in
  * the "arbitrary" part of the AbstractBlock, by just returning the value of
  * the corresponding field. Setting which, however, is responsibility of
  * the derived classes. */

 Index get_first_dynamic_Variable( void ) const { return( f_1st_dyn_var ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// returns the index of the first "available" inner Block
 /** This method returns the index of the first inner Block in the
  * "arbitrary" part of the AbstractBlock, by just returning the value of
  * the corresponding field. Setting which, however, is responsibility of
  * the derived classes. */

 Index get_first_inner_Block( void ) const { return( f_1st_sub_block ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// tells if the Objective is reserved
 /** This method returns true if the Objective is handled by the derived
  * class, i.e., it is not in the "arbitrary" part of the AbstractBlock, by
  * just returning the value of the corresponding field. Setting which,
  * however, is responsibility of the derived classes. */

 bool is_Objective_reserved( void ) const { return( f_res_obj ); }

/*--------------------------------------------------------------------------*/
 /// allow unfettered access to nested Block
 /** Returns a non-const reference to the vector of pointers of inner Block,
  * so that it can be freely modified. Of course, this has to be done with
  * great care, and *not* as soon as there is any solver attached to the
  * AbstractBlock. */

 Vec_Block & access_nested_Blocks() { return( v_Block ); }

/*--------------------------------------------------------------------------*/

 double get_valid_upper_bound( bool conditional = false ) override {
  return( f_ub_cond == conditional ? f_ub : Inf< double >() );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 double get_valid_lower_bound( bool conditional = false ) override {
  return( f_lb_cond == conditional ? f_lb : - Inf< double >() );
  }

/** @} ---------------------------------------------------------------------*/
/*----- Methods for adding/removing (dynamic) Variables and Constraints ----*/
/*--------------------------------------------------------------------------*/
/** @name Methods for changing Variables, Constraints and Objective
 *
 * The protected methods of the base Block class
 *
 * - reset_nested_Block
 *
 * - add_nested_Block
 *
 * - reset_static_constraints
 *
 * - reset_static_variables
 *
 * - reset_dynamic_constraints
 *
 * - reset_dynamic_variables
 *
 * - reset_objective
 *
 * - add_static_constraint (all versions)
 *
 * - add_static_variable (all versions)
 *
 * - add_dynamic_constraint (all versions)
 *
 * - add_dynamic_constraints (all versions)
 *
 * - add_dynamic_variable (all versions)
 *
 * - set_static_constraint (all versions)
 *
 * - set_static_variable (all versions)
 *
 * - set_dynamic_constraint (all versions)
 *
 * - set_dynamic_variable (all versions)
 *
 * - remove_dynamic_constraint (all versions)
 *
 * are made public in AbstractBlock, so that they can be used "from outside"
 * the AbstractBlock to manage its abstract representation;
 *  @{ */

 using Block::reset_nested_Block;

 using Block::add_nested_Block;

 using Block::reset_static_constraints;

 using Block::reset_static_variables;

 using Block::reset_dynamic_constraints;

 using Block::reset_dynamic_variables;

 using Block::reset_objective;

 using Block::add_static_constraint;

 using Block::add_static_variable;

 using Block::add_dynamic_constraint;

 using Block::add_dynamic_constraints;

 using Block::add_dynamic_variable;

 using Block::set_static_constraint;

 using Block::set_static_variable;

 using Block::set_dynamic_constraint;

 using Block::set_dynamic_variable;

 using Block::remove_dynamic_constraint;

/** @} ---------------------------------------------------------------------*/
/*----------------- Methods for checking the AbstractBlock -----------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for checking solution information in the AbstractBlock
 **/

 /// returns true if the current solution is (approximately) feasible
 /** Returns true if the solution encoded in the current value of the Variable
  * of the Block is approximately feasible within the given tolerance. The
  * method works by basically calling ColVariable::is_feasible() on all the
  * [Col]Variable and checking the violation (relative or absolute) of each
  * [Row]Constraint of the abstract representation, for there clearly is no
  * other possible way to do this.
  *
  * The parameters for deciding what "approximately feasible" exactly means
  * are a double and an int (representing a boolean) value. The double value
  * represents the tolerance for satisfaction of all [FRow/OneVar]Constraint,
  * and domain restrictions for [Col]Variable. The int value indicates whether
  * the tolerance is with respect to the relative or the absolute violation of
  * the RowConstraint (see RowConstraint::rel_viol() and
  * RowConstraint::abs_viol()). These values are to be found as:
  *
  * - If \p fsbc is not nullptr and it is a SimpleConfiguration< double >,
  *   then the tolerance is fsbc->f_value and the relative violation is
  *   considered;
  *
  * - If \p fsbc is not nullptr and it is a SimpleConfiguration<
  *   std::pair< double, int > >, then the tolerance is fsbc->f_value.first and
  *   the type of violation is fsbc->f_value.second (any nonzero number for
  *   relative and zero for absolute violation);
  *
  * - Otherwise, if #f_BlockConfig is not nullptr,
  *   #f_BlockConfig->f_is_feasible_Configuration is not nullptr and it is a
  *   pointer to either a SimpleConfiguration< double > or to a
  *   SimpleConfiguration< std::pair< double , int > >, then the values of the
  *   parameters are obtained analogously as above;
  *
  * - Otherwise, the tolerance is 0 and the relative violation is
  *   considered. */

 bool is_feasible( bool useabstract = false ,
                   Configuration * fsbc = nullptr ) override;

/*--------------------------------------------------------------------------*/
 /// several sanity checks
 /** This debug method implements a bunch of sanity checks on the "abstract"
  * (which is the only) representation of the AbstractBlock, such as:
  *
  * - every Variable and Constraint of the AbstractBlock belongs to the
  *   AbstractBlock;
  *
  * - every sub-Block of the AbstractBlock belongs to the AbstractBlock;
  *
  * - for any Variable of the AbstractBlock, and for any of the "active stuff"
  *   of that Variable, the Variable is found among the list of "active"
  *   Variable of that stuff;
  *
  * - for each Constraint and for the Objective, and for each of the "active"
  *   Variable in that, the Constraint / Objective if found among the "active
  *   stuff" of that Variable;
  *
  * - ...
  *
  * These are very generic checks that should be done at the level of Block,
  * but they can not because it is not possible to just "extract Variable"
  * or "extract Constraint" from the "abstract representation": one can only
  * extract :Variable (say, ColVariable) or :Constraint (say, FRowConstraint).
  * This is a very bad consequence of the initial design choice about using
  * boost::any, and one of the reasons why these will be put to the wall when
  * the revolution will come. */

 void is_correct( void );

/** @} ---------------------------------------------------------------------*/
/*----------------------- Methods for handling Solution --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Solution
 *  @{ */

 /// returns a Solution representing the current solution of this Block
 /** This method must construct and return a (pointer to a) Solution object
  * representing the current "solution state" of this Block. For an
  * AbstractBlock, the "only reasonable" Solution are ColVariableSolution,
  * RowConstraintSolution, and ColRowSolution.
  *
  * The parameter for deciding which kind of Solution must be returned is a
  * single int value. If this value is
  *
  * - 1, then a RowConstraintSolution is returned;
  *
  * - 2, then a ColRowSolution is returned;
  *
  * - any other value, then a ColVariable Solution is returned.
  *
  * This value is to be found as:
  *
  * - if solc is not nullptr and it is a SimpleConfiguration< int >, then it is
  *   solc->f_value;
  *
  * - otherwise, if f_BlockConfig is not nullptr,
  *   f_BlockConfig->f_solution_Configuration is not nullptr and it is a
  *   SimpleConfiguration< int >, then it is
  *   f_BlockConfig->f_solution_Configuration->f_value;
  *
  * - otherwise, it is 0. */

 Solution * get_Solution( Configuration * solc = nullptr ,
                          bool emptys = true ) override;

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR PRINTING & SAVING THE AbstractBlock -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing & saving the AbstractBlock
 * @{ */

 /// print information about the AbstractBlock on an ostream 
 /** Protected method intended to print information about the AbstractBlock.
  *
  * With default verbosity (vlvl == 0) prints some basic statistics.
  *
  * vlvl == 'M' and vlvl == 'L' should correspond to output in MPS format
  * and LP format as required by load(), but these are not implemented yet.
  *
  * With any other verbosity goes over all the abstract representation
  * (static and dynamic Variable and Constraint, Objective, and inner Block)
  * and asks everyone to print itself. */

 void print( std::ostream & output , char vlvl = 0 ) const override;

/*--------------------------------------------------------------------------*/
 /// serialize the AbstractBlock (recursively) to a netCDF NcGroup
 /** The AbstractBlock serializes itself out of a netCDF::NcGroup. Besides
  * what is managed by the serialize() method of the base Block class, the
  * method of the base AbstractBlock class only handles
  *
  *      OR, BETTER, WILL HANDLE WHEN THIS METHOD WILL BE IMPLEMENTED
  *
  * the "arbitrary" part, i.e., that after the "reserved" one as dictated
  * by the get_first_*_*() methods; all the rest must be handled by the
  * derived classes (if any).
  *
  * For the format of the produced netCDF::NcGroup, see
  * AbstractBlock::deserialize(). */

 void serialize( netCDF::NcGroup & group ) const override;

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
 /// do all the dirty work for deserialize()

 void guts_of_deserialize( const netCDF::NcGroup & group );

/*--------------------------------------------------------------------------*/

 void check_Variable( Variable * var );

 void check_Constraint( Constraint * cnst );

 void check_Objective( Objective * obj );

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 double f_ub;             ///< the global upper bound
 double f_lb;             ///< the global lower bound

 bool f_ub_cond;          ///< whether f_ub is only conditionally valid
 bool f_lb_cond;          ///< whether f_lb is only conditionally valid

 Index f_1st_stat_var;    ///< the first available group of static Variable
 Index f_1st_dyn_var;     ///< the first available group of dynamic Variable
 Index f_1st_stat_cnst;   ///< the first available group of static Constraint
 Index f_1st_dyn_cnst;    ///< the first available group of dynamic Constraint

 bool f_res_obj;          ///< if the Objective is not available

 Index f_1st_sub_block;   ///< the first available inner Block;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 /// Loads the block from a MPS file
 void read_mps( std::istream & file );

/*--------------------------------------------------------------------------*/
/*------------------------------ LP READER ---------------------------------*/
/*--------------------------------------------------------------------------*/

 // An enumeration of all the possible sections of a .lp file
 enum LP_sections {
   LP_OBJECTIVE ,
   LP_ROW ,
   LP_BOUND ,
   LP_GENERAL ,
   LP_BINARY ,
   LP_SEMI_CON ,
   LP_SOS ,
   LP_END
 };

 // Function used to update the current section analyzed in the .lp file
 void sec_reached( int * actual_sec , std::string word );

 /// Loads the block from an LP file
 void read_lp( std::istream & file );

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

};  // end( class( AbstractBlock ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* AbstractBlock.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File AbstractBlock.h ------------------------*/
/*--------------------------------------------------------------------------*/

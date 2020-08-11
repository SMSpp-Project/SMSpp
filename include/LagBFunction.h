/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class LagBFunction, which
 * implements C05Function and Block with a Lagrangian function.
 *
 * \version 0.07
 *
 * \date 20 - 11 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Enrico Gorgone.
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __LagBFunction
 #define __LagBFunction
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "C05Function.h"
#include "Block.h"
#include "ColVariable.h"
#include "FRealObjective.h"
#include "LinearFunction.h"
#include "Solution.h"
#include "Configuration.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

 class BlockSolverConfig;

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup LagFun_CLASSES Classes in LagBFunction.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LagBFunction -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Lagrangian Function
/**< The class LagBFunction is a convenience class implementing the "abstract"
 * concept of Lagrangian Relaxation of "any" Block. LagBFunction derives from
 * *both* C05Function and Block.
 *
 * The main ingredients of a LagBFunction are three:
 *
 * 1) A "base" Block B, representing "any" optimization problem
 *
 *      (B)    max { c(x) : x \in X }
 *
 *    This will be the one, and only, sub-Block of LagBFunction (when "seen"
 *    as a Block).
 *
 * 2) A vector of pairs of relaxed functions and the Lagrangian multipliers
 *    thereof : [ y , g(x) ] = [ ( y_i , g_i (x) ) ]_{i \in I} which handle
 *    the static relaxation of constraints. The vector-valued
 *    function g(x) = [ g_i( x ) ]_{i \in I} is defined in the same Variable
 *    x as B, which should be thought as a part of the "complicating constraints"
 *    of some original problem
 *
 *      (O)    max { c(x) : g(x) [ + ... ] [<]= 0 , x \in X }
 *
 *    that are relaxed to make it easier. To make the notation easier the
 *    relaxed constraints are also referred to as (RCs).The "[ + ... ]" term
 *    underlines the fact that (O) may have other variables that, once the
 *    complicating constraints are relaxed, become independent from x; these
 *    will be typically put into one (or more) other LagBFunction, and therefore
 *    are not a concern of this specific (B). The notation "[<]=" means that the
 *    constraints can (almost) indifferently be equalities or inequalities,
 *    the difference simply yielding (or not) sign constraints on the
 *    Lagrangian multipliers [see right below]. The vector y = [ y_i ]_{i \in I}
 *    of Lagrangian multipliers, real-valued Variable (ColVariable) each one
 *    associated to one of the complicating constraints. Note that these must
 *    *not* be Variable of the LagBFunction seen as a Block, because they are
 *    conceptually fixed when the Block is solved. The y corresponding to
 *    inequality constraints will have an appropriate sign constraint on them;
 *    however, this will be a concern of the "outer" Block defining them,
 *    and therefore is not a concern of the LagBFunction.
 *
 * 3) A list of pairs of relaxed functions and the Lagrangian multipliers
 *    thereof : { ( y_i , g_i (x) ) }_{i \in \bar{I}} which handle the
 *    dynamic generation/removal which handle the dynamic relaxation of
 *    constraints.
 *
 * Note that the LagBFunction is not supposed to have any Constraint or
 * Variable in itself, that is, besides "x" and "x in X"  (respectively) that
 * come with (B).
 *
 * With these elements, LagBFunction represents the Lagrangian function
 *
 *   (L_y)   l( y ) = max { c(x) + \sum_{i \in I} y_i g_i(x) : x \in X }
 *
 * The function l(y) is convex in y (concave if (B) is a minimization problem),
 * since it is the pointwise maximum of (possibly, infinitely many) linear
 * functions in y. As such it is continuous in the interior of its domain;
 * however, it typically is non differentiable. Indeed, if x(y) is the
 * (eps-)optimal solution of (L_y), then
 *
 *       h = g( x(y) ) = [ g_i( x(y) ) ]_{i \in I}
 *
 * is a(n eps-)subgradient of l( y ) at y (supergradient if (B) is a
 * minimization problem and therefore l is concave). Hence, the gradient of l
 * at y is well-defined only if x(y) is unique, which may easily not happen.
 * This is why in general LagBFunction is a C05Function depending on the
 * variables y; note that the LagBFunction can still easily declare to be
 * smooth [see C05Function::is_continuously_differentiable()] is it knows it
 * is the case.
 *
 * The aim of LagBFunction is to automate the process of turning the block B
 * (given g and y) into the Lagrangian function l( y ), implementing all the
 * notrivial mechanics about local and global pool of linearizations (=
 * eps-subgradients = eps-optimal solutions to (L_y)), transforming
 * Modification of the block (B) into Modification of the C05Function, and so
 * on. To do this in the most general way, no assumption is made on (B) and g
 * save that:
 *
 * - the Objective c(x) of (B) is a "simple" function, i.e., it belongs to
 *   following classes:
 *
 *   = FRealObjective whose inside function is a LinearFunction
 *
 * - g is represented by a std::vector<Function *>, with each g[ i ] being a
 *    "simple" function, i.e., belonging to following classes:
 *
 *   = LinearFunction
 *
 * IMPORTANT: THERE MUST BE A WAY TO UNDERSTAND WHICH Modification OF THE
 *            INNER Block (B) CORRESPOND TO CHANGES OF THE Objective c(x)
 *            AND WHICH ONES CORRESPOND TO CHANGES OF THE FEASIBLE REGION X,
 *            SINCE THE LagBFunction WILL HAVE TO REACT DIFFERENTLY; IT IS
 *            NOT ENTIRELY CLEAR TO ME NOW HOW TO DO THIS.
 *
 * Under these assumptions, LagBFunction can implement the required machinery
 * to use the inner Block (B), with any attached Solver, to implement the
 * C05Function interface for the Lagrangian function l( y ).
 *
 * An important note, however, is that LagBFunction is both a C05Function and
 * a Block. This is done in order for it to be able to "intercept" any
 * Modification from its sub-Block (B) and properly react to it; however, the
 * current implementation
 *
 *    KNOWINGLY AND INTENTIONALLY VIOLATE SOME OF THE STANDARD ASSUMPTIONS
 *    OF Block, WHICH IMPLIES THAT LagBFunction SHOULD NOT BE DIRECTLY
 *    PASSED TO A GENERAL-PURPOSE SOLVER TO BE SOLVED.
 *
 * The point is about the Objective of the LagBFunction and that of its
 * sub-Block (B). To properly represent, in an "abstract" form, the
 * mathematical reality of (L_y), these should be arranged as follows:
 *
 * - the Objective of the LagBFunction should be the function
 *
 *       \sum_{i \in I} y_i g_i(x)
 *
 * - the Objective of (B) should be its original function c(x).
 *
 * However, this is not how the object is implemented. The point is that
 * LagBFunction has to compute l( y ) using (B) and its Solver; this means
 * that it has to "translate" the Objective of (B) in a form that (B) (and
 * its solver) accepts. For instance, if c() is a linear function, then
 * (B) will typically allow its coefficients to be changed, but *not* it
 * to be transformed into another kind of function. If, say, g() are also
 * linear functions (g(x) = Gx), the LagBFunction will, each time when
 * compute() is called [roughly], compute the Lagrangian costs
 *
 *      c^y = c + yA
 *
 * and change the Objective of (B) accordingly.
 *
 * This is not mathematically required; in principle, one could define a
 * BilinearFunction l( x , y ) = cx + yAx, and insist that the objective
 * of (B) be that. This would work, since y are not variables of (B), and
 * therefore are fixed when it is solved. However, it would require (B) and
 * its Solver to specifically cater for this case. The choice has been to
 * rather have LagBFunction to perform the necessary machinery, in order to
 * leave (B) and its solver completely unaware of what is happening.
 *
 * The downside of this choice is that, when examining the "abstract"
 * implementation of the LagBFunction, one finds:
 *
 * - the Objective of the LagBFunction is empty
 *
 * - the Objective of (B) is that corresponding to the last value of
 *   \bar{y} on which compute() has been called (in the linear case,
 *   c^\bar{y} x).
 *
 * Thus, this violates the intended mathematical definition of Block. Yet,
 * this is acceptable in the following two scenarios. When the LagBFunction
 * is used as a C05Function (say, inside a FRealObjective), its "innards"
 * (B) are not looked at, and do not really appear in the "abstract
 * representation": C05Function is a "black box", and any Solver seeing it
 * makes no assumption about what's there inside. However, some Solver may
 * exploit the specific form of a LagBFunction. Yet, if they do this, they
 * will check if the C05Function actually is a LagBFunction, and in this
 * case they will know exactly what to do with it. In other words, even if
 * the Objective of the LagBFunction (seen as a Block) is empty, such a
 * Solver, which is *not* general-purpose but specialized for the case, will
 * be able to "pretend" to see the right term c(x) + \sum_{i \in I} y_i g_i(x)
 * in there. Thus, while it will not be possible to use a LagBFunction as
 * "any" Block to be passed to "any" general-purpose Solver, the most
 * important use cases for it are covered.
 *
 * It would actually be possible to make the "abstract representation" of the
 * LagBFunction to exactly match its intended mathematical semantic. To do
 * that, one should (in the linear case, for notational simplicity):
 *
 * - set the Objective of the LagBFunction as c x + y A x - c^\bar{y} x
 *   (a BiLinearFunction);
 *
 * - keep the Objective of (B) to c^\bar{y} x.
 *
 * Since the Objective of the son is (implicitly) summed to that of the father,
 * this would work being the f_sense of Objective of both LagBFunction
 * and its sub-block B is eMax. By contrast, the "outer" block defining
 * LagBFunction has to minimize (L_y) in the Variable y, so f_sense of the
 * Objective of the "outer" block must be eMin. In fact, the Lagrangian dual
 * is a min-max problem (max-min if (B) is a minimization).
 *
 * However, the Objective would change each time y changes and
 * compute() is called, which is unwieldy. Also, it would require the definition
 * of BiLinearFunction and extensions. More importantly, so far there is no
 * evidence that a specialized Solver exists that it may be appropriate to
 * use to solve this kind of problem, and therefore there does not appear to
 * be any compelling reason to implement this kludge.
 *
 * The Objective of a Lagrangian function is its Observer, but in turn a
 * LagBFunction is also the Observer of its relaxed (linear) function (RCs).
 * This means this a Lagrangian function handles the Modification which come
 * to it from both way: the sub-Block (B) and the constraints (RCs).
 * In addition, the Observer of a LagBFunction is assumed to be a
 * FRealObjective. */

class LagBFunction : public C05Function , public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */

 /* Since LagBFunction is both a ThinVarDepInterface and a Block, it "sees"
  * two definitions of "Index", "Range" and "Subset". These are actually the
  * same, but compilers still don't like it. Disambiguate by declaring we
  * use the ThinVarDepInterface versions (but it could have been the Block
  * versions, as they are the same. */
 using Index = ThinVarDepInterface::Index;
 using c_Index = ThinVarDepInterface::c_Index;
 using Range = ThinVarDepInterface::Range;
 using c_Range = ThinVarDepInterface::c_Range;
 using Subset = ThinVarDepInterface::Subset;
 using c_Subset = ThinVarDepInterface::c_Subset;
 using v_coeff_pair = LinearFunction::v_coeff_pair;

 ///< a vector of dual_pair (a constraint and its dual variable)
 using dual_pair = std::pair< ColVariable * , Function * >;
 using  v_dual_pair = std::vector< dual_pair > ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 typedef std::tuple< p_Solution , bool , bool > linearization_tuple;
 /* a solution equipped with two boolean, one which defines the type of
    and the other one states if the solution has to be checked for
    feasibility. */

 typedef std::vector< linearization_tuple > v_linearization_tuple;
 ///< a vector of linearization_pair

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 typedef std::pair< LinearFunction::Coefficient , LinearFunction::v_coeff_pair > col_pair;
 ///< a pair to represent c_i and < y_i , A_i >

 typedef std::vector< col_pair > m_column;
 ///< a map of col_pair

 /*--------------------------------------------------------------------------*/
  /// virtualized concrete iterator
  /** A concrete class deriving from ThinVarDepInterface::v_iterator and
   * implementing the concrete iterator for "sifting through" the "active"
   * Variable of a LagBFunction. */

  class v_iterator : public ThinVarDepInterface::v_iterator
  {
   public:

   explicit v_iterator( v_dual_pair::iterator & itr ) : itr_( itr ) {}
   explicit v_iterator( v_dual_pair::iterator && itr )
    : itr_( std::move( itr ) ) {}

   v_iterator * clone( void ) override final {
    return( new v_iterator( itr_ ) );
    }

   void operator++( void ) override final { ++(itr_); }

   reference operator*( void ) const override final {
    return( *((*itr_).first) );
    }
   pointer operator->( void ) const override final {
    return( (*itr_).first );
    }

   bool operator==( const ThinVarDepInterface::v_iterator & rhs )
    const override final {
    #ifdef NDEBUG
     auto tmp = static_cast<const LagBFunction::v_iterator *>( & rhs );
     return( itr_ == tmp->itr_ );
    #else
     auto tmp = dynamic_cast<const LagBFunction::v_iterator *>( & rhs );
     return( tmp ? itr_ == tmp->itr_ : false );
    #endif
    }
   bool operator!=( const ThinVarDepInterface::v_iterator & rhs )
    const override final {
    #ifdef NDEBUG
     auto tmp = static_cast<const LagBFunction::v_iterator *>( & rhs );
     return( itr_ != tmp->itr_ );
    #else
     auto tmp = dynamic_cast<const LagBFunction::v_iterator *>( & rhs );
     return( tmp ? itr_ != tmp->itr_ : false );
    #endif
    }

   private:

   v_dual_pair::iterator itr_;
   };

 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /// virtualized concrete const_iterator
  /** A concrete class deriving from ThinVarDepInterface::v_const_iterator and
    * implementing the concrete iterator for sifting through the "active"
    * Variable of a LinearFunction. */

  class v_const_iterator : public ThinVarDepInterface::v_const_iterator
  {
   public:

   explicit v_const_iterator( v_dual_pair::const_iterator & itr )
    : itr_( itr ) {}
   explicit v_const_iterator( v_dual_pair::const_iterator && itr )
    : itr_( std::move( itr ) ) {}

   v_const_iterator * clone( void ) override final {
    return( new v_const_iterator( itr_ ) );
    }

   void operator++( void ) override final { (itr_)++; }
   reference operator*( void ) const override final {
    return( *((*itr_).first) );
    }
   pointer operator->( void ) const override final {
    return( (*itr_).first );
    }

   bool operator==( const ThinVarDepInterface::v_const_iterator & rhs )
    const override final {
    #ifdef NDEBUG
     auto tmp = static_cast<const LagBFunction::v_const_iterator *>( & rhs );
     return( itr_ == tmp->itr_ );
    #else
     auto tmp = dynamic_cast<const LagBFunction::v_const_iterator *>( & rhs
  								       );
     return( tmp ? itr_ == tmp->itr_ : false );
    #endif
    }
   bool operator!=( const ThinVarDepInterface::v_const_iterator & rhs )
    const override final {
    #ifdef NDEBUG
     auto tmp = static_cast<const LagBFunction::v_const_iterator *>( & rhs );
     return( itr_ != tmp->itr_ );
    #else
     auto tmp = dynamic_cast<const LagBFunction::v_const_iterator *>( & rhs );
     return( tmp ? itr_ != tmp->itr_ : false );
    #endif
    }

   private:

   v_dual_pair::const_iterator itr_;
   };

/**@} ----------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

/*--------------------------------------------------------------------------*/
  /// constructor of LagBFunction: does nothing

 /// constructor of LagBFunction, taking the static Lagrangian pairs.
 /** Constructor of LagBFunction. It accepts a pointer both to a Block and
  * an Observer, in particular the Block is the sub-block (B).
  *
  * It is assumed the sub-block (B) has no children. The constructor does not
  * accept an array of dual pair, which consists of a pair of relaxed
  * constraints g_i(x) and its Lagrangian multiplier y_i, for some i \in I.
  * The assumption makes sure that, before saving the Lagrangian multipliers,
  * the Observer has been registered. */

 LagBFunction( Block* innerblock = nullptr ,
	       Observer * const observer = nullptr );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 /// destructor of LagBFunction: delete the allocated memory.
 /** destructor of LagBFunction. It deletes delete the global pool and the
     LagMatrix which is used to change the Lagrangian costs. */

 virtual ~LagBFunction( void ) { guts_of_destructor(); };

/*--------------------------------------------------------------------------*/
 
 virtual void clear( void ) override;

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the sub-block pointer.
 /** This method to set the pointer to the sub-Block (B), which is assumed
  *  to be without children.
  *
  * If a set of "static" dual pairs <y, g(x)> have been already accommodated
  * [ see set_dual_pairs(), LagBFunction() ], the cost vector c of (obj_B) will
  * be saved in LagMatrix. This because the evaluation of
  * Lagrangian function l(y) requires the vector c -stored in (B)- to be
  * replaced by c^y = c + yA, and consequently the original vector c
  * would be unavailable.
  *
  * In addition, since c(x) of (obj_B) is stored in a *sparse* format,
  * LagBFunction has to add -to the *active* variable set of (obj_B)-
  * the Variable with coefficient zero which are involved in the definition
  * of the relaxed constraints (RCs).  */

 void set_inner_block( Block* innerblock );

/*--------------------------------------------------------------------------*/

 /// set a bunch of *static* Lagrangian pairs <y, g(x)>
 /** This method must be called after the construction of the
  * class and *only if* an empty constructor has been used. By calling
  * this method a bunch of Lagrangian pairs <y, g(x)> is stored.
  *
  * If a pointer to (B) has been passed, [ see set_inner_block(Block*),
  * LagBFunction( ) ], the cost vector c of (obj_B) will be saved in LagMatrix.
  * This because the evaluation of Lagrangian function l(y)
  * requires the vector c -stored in (B)- to be replaced by c^y = c + yA,
  * and consequently the original vector c would be unavailable.
  *
  * In addition, since c(x) of (obj_B) is stored in a *sparse* format,
  * the LagBFunction has to add -to the *active* variable set of (obj_B)-
  * the Variable with coefficient zero which are involved in the definition
  * of the relaxed constraints (RCs).
  *
  * This function must be called after set_inner_block, the sub-Block has been
  * already defined. */

 void set_dual_pairs( v_dual_pair && lp , c_ModParam issueMod = eModBlck );

/*--------------------------------------------------------------------------*/

 /// register LagBFunction as the Observer of g_i, with i \in I.
 /** This method registers this class as the Observer of the relaxed
  * constraints given in input. This method *must be* called for all the
  * relaxed constraints (RCs). No variable is registered because the
  * Lagrangian multipliers are the variables of LagBFuntion while the primal
  * variables x are the variables of (RCs), and then the set of variables
  * are not compatible with each other. */

 virtual void set_relaxed_function( Function * const function = nullptr  );

/*--------------------------------------------------------------------------*/

  /// set the whole (empty) set of parameters in one blow
  /** Although a LagBFunction formally has a lot of parameters, in fact it
   * "listens to no-one"; hence, the implementation of set_ComputeConfig() is
   * quite a trivial one.
   *
   * ComputeConfig is assumed to be of the SimpleConfig_p_p type wherein
   * the field f_value is a Configuration pointers pair. The first element
   * of that pair is a BlockSolverConfig and the second one is a
   * BlockConfig. */

  virtual void set_ComputeConfig( ComputeConfig *scfg = nullptr )
   override final;

/*--------------------------------------------------------------------------*/

 /// set a given integer (int) numerical parameter
 /** Set a given integer (int) numerical parameter. The method sets the maximum
  *  size of both the local and the global pool  */

 virtual void set_par( const idx_type par , const int value ) override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 /// set a given float (double) numerical parameter
 /** Set a given float (double) numerical parameter. The method sets both the
  *  relative and absolute accuracy in any linearization.  */

 virtual void set_par( const idx_type par , const double value ) override;

/*--------------------------------------------------------------------------*/

 virtual void deserialize( netCDF::NcGroup& group ) override;

/*--------------------------------------------------------------------------*/
 /** As stated above, the Observer of a LagBFunction is assumed to be a
  * FRealObjective. */

 virtual void register_Observer( Observer * const observer = nullptr ) override
 {
  if( observer && ! dynamic_cast<FRealObjective *>( observer ) )
   throw( std::logic_error(
	       "the Observer of a LagBFunction must be a FRealObjective" ) );

  Function::register_Observer( observer );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

 /// add a bunch of *dynamic* Lagrangian pairs <y, g(x)>,
 /** This method adds a bunch of Lagrangian pairs. The part of cost vector
   * c of (obj_B) -not already saved- will be stored in LagMatrix.
   * The evaluation of the Lagrangian function l(y) requires the vector c
   * -stored in (B)- to be replaced by c^y = c + yA, and consequently the
   * original vector c would be unavailable from (B).
   *
   * In addition, since c(x) of (obj_B) is stored in a *sparse* format,
   * the LagBFunction has to add -to the *active* variable set of (obj_B)-
   * the Variable with coefficient zero which are involved in the definition
   * of the relaxed constraints (RCs).  */

 void add_dual_pairs( v_dual_pair && lp, c_ModParam issueMod = eNoBlck );

/*--------------------------------------------------------------------------*/

 void remove_variable( Index i , c_ModParam issueMod = eModBlck ) override;

 void remove_variables( Range range , c_ModParam issueMod = eModBlck )
  override final;

 void remove_variables( Subset && nms , bool ordered = false ,
			c_ModParam issueMod = eModBlck )  override final;

/*--------------------------------------------------------------------------*/

 void add_Modification( sp_Mod mod , ChnlName chnl = 0 ) override;

/**@} ----------------------------------------------------------------------*/
/*---------- METHODS FOR Loading/Saving THE DATA OF THE LagBFunction -------*/
/*--------------------------------------------------------------------------*/
/** @name Saving the data of the LagBFunction
 *  @{ */

 /// serialize a LagBFunction into a netCDF::NcGroup
 /** Serialize a LagBFunction into a netCDF::NcGroup. Note that, LagBFunction
  * being both a Function and a Block, the netCDF::NcGroup will have to have
  * the "standard format of a :Block", meaning whatever is managed by the
  * serialize() method of the base Block class, plus the
  * LagBFunction-specific data with the following format:
  *
  *     TO BE DONE
  */

 void serialize( netCDF::NcGroup& group ) const override final;

/**@} ----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LagBFunction
 *  @{ */

 bool has_linearization( bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/

 bool compute_new_linearization( bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/

 void store_linearization( Index name ,
			   c_ModParam issueMod = eModBlck  ) override final;

/*--------------------------------------------------------------------------*/

 bool is_linearization_there( Index name ) const override final {
  //!! TO BE CHANGED
  return( false );
  }

/*--------------------------------------------------------------------------*/

 bool is_linearization_vertical( Index name ) const override final {
  //!! TO BE CHANGED
  return( false );
  }

/*--------------------------------------------------------------------------*/

 void store_combination_of_linearizations( LinearCombination & coefficients ,
					   Index name  ,
					   c_ModParam issueMod = eModBlck )
  override final;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
 /// set the important linearization
 /** This method *must* be called after store_combination_of_linearizations()
  * in such a way the important linearization already exists when
  * set_important_linearization() is called. It sets the name of the important
  * linearization and saves the combination used to form it.
  * This method also writes the solution relative to the important linearizaiton
  * into the sub-Block (B).  */

 void set_important_linearization( LinearCombination && coefficients ,
				   Index name ) override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
 /// get the name of the important linearization
 /** This method reads the name of the important linearization */

 Index get_important_linearization_name( void ) override { return( zName ); }

 /*--------------------------------------------------------------------------*/
 /// get the name of the important linearization
  /** This method retrieves the linear combination used to form the important
   * linearization */

 c_LinearCombination & get_important_linearization_coefficients( void )
  override final { return( zLC ); }

/*--------------------------------------------------------------------------*/

 void delete_linearization( Index name ,
			    c_ModParam issueMod = eModBlck ) override final;

/*--------------------------------------------------------------------------*/

 void delete_linearizations( Subset && which , bool ordered = true ,
			     c_ModParam issueMod = eModBlck ) override final
 {
  //!! TO BE CHANGED
  C05Function::delete_linearizations( std::move( which ) , ordered ,
				      issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// compute the Function
 /** It has to compute the Function. ?? The parameter changedvars is false if
  *  the constant vector b of (RCs) is changed and the remaining data problem
  *  is still valid. ??
  *
  *  It is assumed that the sub-Block (B) does not have Variable defined
  *  in other Blocks. Then, the re-optimization of (B) can be performed starting
  *  from the warm-start (the old solution). No relevant Variable are defined
  *  in (B). */

 virtual int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// returns the value of the Function
 /** It returns the value of the Function that was computed in the most recent
  * call to compute(); if the latter has never been invoked, then the value
  * returned by this method is meaningless.
  *
  * If (B) is computed with a low accuracy and the function value lays in an
  * interval, the upper bound shall be returned (the lower bound if (B) is a
  * minimization problem). */

 virtual FunctionValue get_value( void ) const override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual FunctionValue get_lower_estimate( void ) const override final{
  return( slv->get_lb() );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual FunctionValue get_upper_estimate( void ) const override final{
  return( slv->get_ub() );
  }

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g ,
			   Range range = std::make_pair( 0 , Inf<Index>() ) ,
				      Index name = Inf<Index>() ) override;

/*--------------------------------------------------------------------------*/

 void get_linearization_coefficients( FunctionValue * g , c_Subset & subset  ,
				      const bool ordered = false ,
				      Index name = Inf<Index>() ) override;

/*--------------------------------------------------------------------------*/

 FunctionValue get_linearization_constant( c_Index name = Inf<Index>() )
  override final;

/*--------------------------------------------------------------------------*/

 Block* get_inner_block( void ) const;

/*--------------------------------------------------------------------------*/

 int get_NzMat( void );

/*--------------------------------------------------------------------------*/

 void get_MatDesc( int *Abeg , int *Aind , double *Aval , const int strt ,
		   int stp );

/**@} ----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LagBFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a LagBFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one.
  *
  * ComputeConfig is assumed to be of the SimpleConfig_p_p type wherein
  * the field f_value is a Configuration pointers pair. The first element
  * of that pair is a BlockSolverConfig and the second one is a
  * BlockConfig. */

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
					    ComputeConfig * ocfg = nullptr )
  const override final;

/*--------------------------------------------------------------------------*/

 virtual int get_int_par( const idx_type par ) const override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual double get_dbl_par( const idx_type par ) const override;

/*--------------------------------------------------------------------------*/

 virtual int get_dflt_int_par( const idx_type par ) const override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual double get_dflt_dbl_par( const idx_type par ) const override;

/**@} ----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LagBFunction; this is the actual concrete implementation exploiting the
 * vector lag_p of Lagrangian pairs.
 * @{ */

 Index get_num_active_var( void ) const override final {
  return( LagPairs.size() );
  }

/*--------------------------------------------------------------------------*/

 Index is_active( const Variable * const var ) const override final;

/*--------------------------------------------------------------------------*/

 void map_active( c_Vec_p_Var & vars , Subset & map ,
		  const bool ordered = false ) const override final;

/*--------------------------------------------------------------------------*/

 Variable *get_active_var( const Index i ) const override final {
  return( ( LagPairs.begin() + i )->first );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_begin( void ) override final {
  return( new LagBFunction::v_iterator( LagPairs.begin() ) );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 v_const_iterator * v_begin( void ) const override final {
  return( new LagBFunction::v_const_iterator( LagPairs.begin() ) );
  }

/*--------------------------------------------------------------------------*/

 v_iterator * v_end( void ) override final {
  return( new LagBFunction::v_iterator( LagPairs.end() ) );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 v_const_iterator * v_end( void ) const override final {
  return( new LagBFunction::v_const_iterator( LagPairs.end() ) );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 *  @{ */

 /// printing the LagBFunction
 virtual void print( std::ostream &output ) const override;

 virtual void load( std::istream &input ) override final;

/**@} ----------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 FRealObjective * obj;
 ///< the (linear) objective function of the sub-Block (B)

 Solver* slv;
 ///< the Solver of (B)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 v_dual_pair LagPairs;
 ///< vector of Lagrangian dual pairs

 v_linearization_tuple g_pool;
 ///< global pool

 m_column CostMatrix;
 ///< the matrix < x , <c,yA> > used to update the Lagrangian cost vector

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 Index LastSolution;
 ///< the last solution read by get_linearization

 bool VarType;
 ///< the type of variable contained in the solver


 Index zName;
 ///< the name of the important linearization

 LinearCombination zLC;
 ///< the LinearCombination of the important linearization

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 int GPMaxSz;
 ///< maximum size of the "global pool"

 int LPMaxSz;
 ///< maximum size of the "local pool"

 double RAccLin;
 ///< maximum relative error in any linearization

 double AAccLin;
 ///< maximum absolute error in any reported solution

 BlockSolverConfig * svcc;
 ///< the block solver configuration of the sub-block

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

   void set_objective_and_solver();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

   void initialize_cost_matrix( void );

   Subset add_columns( v_dual_pair & v_lag_pair );
   Subset update_columns( v_dual_pair & v_lag_pair );
   void rm_columns( c_Subset & subset );
   void rm_columns( c_Range & range );

/*--------------------------------------------------------------------------*/

   void set_original_costs( c_Subset & subset = {} );
   void compute_Lagrangian_costs( );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

   void guts_of_destructor( );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

   void guts_of_add_Modification( sp_Mod mod , ChnlName chnl );

/*--------------------------------------------------------------------------*/
/*---------------------------- PRIVATE FIELDS ------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;        // insert LagBFunction in the Block factory

 };  // end( class( LagBFunction ) )

/** @} end( group( LagFun_CLASSES ) ) --------------------------------------*/
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LagBFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LagBFunction.h ----------------------------*/
/*--------------------------------------------------------------------------*/

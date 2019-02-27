/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class LagBFunction, which
 * implements C05Function and Block with a Lagrangian function.
 *
 * \version 0.02
 *
 * \date 18 - 02 - 2019
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
 * Copyright &copy by Antonio Frangioni, Enrico Gorgone
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup LFun_CLASSES Classes in LagBFunction.h
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
 *    that are relaxed to make it easier. The "[ + ... ]" term underlines the
 *    fact that (O) may have other variables that, once the complicating
 *    constraints are relaxed, become independent from x; these will be
 *    typically put into one (or more) other LagBFunction, and therefore are
 *    not a concern of this specific (B). The notation "[<]=" means that the
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
 * be any compelling reason to implement this kludge. */


class LagBFunction : public C05Function , public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------- PUBLIC TYPES OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 typedef std::pair< ColVariable * , Function * > dual_pair;
 ///< a constraint and its dual variable

 typedef std::vector< dual_pair > v_dual_pair;
 ///< a vector of dual_pair

 typedef std::list< dual_pair > l_dual_pair;
 ///< a list of dual_pair

 typedef std::pair< p_Solution , bool > linearization_pair;
 ///< a solution equipped with boolean which defines the type of linearization

 typedef std::vector< linearization_pair > v_linearization_pair;
 ///< a vector of linearization_pair

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 typedef std::pair< LinearFunction::Coefficient , LinearFunction::v_coeff_pair > col_pair;
 ///< a pair to represent c_i and < y_i , A_i >

 typedef std::map< ColVariable * , col_pair > m_column;
 ///< a map of col_pair

/*@}------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and Destructor
 *  @{ */

/*--------------------------------------------------------------------------*/
  /// constructor of LagBFunction: does nothing

 /// constructor of LagBFunction, taking the static Lagrangian pairs.
 /** Constructor of LagBFunction. It accepts a vector of dual pairs <y, g(x)>
  * of the form < pointer to ColVariable , pointer to Function >, and a bool
  * telling if the given vector is already ordered by ColVariable
  * "name = pointer" or not, in which case it is ordered. As the the && tells,
  * the dual pairs are "consumed" by the constructor
  * and its resource become property of the LagBFunction object.
  *
  * By default the vector of dual pairs is assumed to *not* be ordered. */

 LagBFunction( v_dual_pair && v_lag_pairs = {} , const bool static_is_ordered = false ,
		 Block* innerblock = nullptr );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 /// destructor of LagBFunction: delete the static and dynamic dual Lagrangian
 /** destructor of LagBFunction. It deletes the static and dynamic dual
   * pairs and the global pool, if exists */

 virtual ~LagBFunction( ) { guts_of_destructor(); };

/*--------------------------------------------------------------------------*/
 
 virtual void clear( ) override;

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// set the sub-block pointer.
 /** Method to set the pointer to the sub-Block, which is assumed to be
  *  only one. lineare vincoli e funzione obiettivo
  *
  * If a set of static dual pairs <y, g(x)> have been already accommodated
  * [ see set_static_pairs(), LagBFunction(v_dual_pair &&, bool) ],
  * the cost vector c of (B) will be saved. This because the evaluation of
  * Lagrangian function l(y) requires the vector c -stored in (B)- to be
  * replaced by c^y = c + yA, and consequently the original vector c
  * would be unavailable.
  *
  * In addition, since c(x) of (B) is stored in a *sparse* format,
  * the LagBFunction has to add -to the *active* variable set of the
  * LinearFunction of (B)- those Variable with coefficient zero
  * which are involved in the definition of g(x)  */

 void set_inner_block( Block* innerblock );

/*--------------------------------------------------------------------------*/

 /// set a bunch of *static* dual pairs <y, g(x)>
 /** This method must be called after the construction of the
  * class and only if an empty constructor has been used. By calling
  * this method a bunch of dual pairs <y, g(x)> is stored.
  *
  * If the a pointer to (B) has been passed, [ see set_inner_block(Block*),
  * LagBFunction(v_dual_pair &&, bool) ], the cost vector c of (B)
  * will be saved. This because the evaluation of Lagrangian function l(y)
  * requires the vector c -stored in (B)- to be replaced by c^y = c + yA,
  * and consequently the original vector c would be unavailable.
  *
  * In addition, since c(x) of (B) is stored in a *sparse* format,
  * the LagBFunction has to add -to the *active* variable set of the
  * LinearFunction of (B)- those Variable with coefficient zero
  * which are involved in the definition of g(x)  */

 void set_dual_pairs( v_dual_pair && v_lag_pairs ,
 		 const bool static_is_ordered = false );

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

/*@} -----------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling Modification
 *  @{ */

 /// add a bunch of dual pairs <y, g(x)>

 void add_dual_pairs( l_dual_pair && l_lag_pairs ,
 		 const bool static_is_ordered = false );


/*@} -----------------------------------------------------------------------*/
/*---------- METHODS FOR READING THE DATA OF THE LagBFunction --------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the LagBFunction
    @{ */

/*@} -----------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of the LagBFunction
 *  @{ */

 virtual bool has_linearization( const bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/

 virtual bool compute_new_linearization( const bool diagonal = true ) override final;

/*--------------------------------------------------------------------------*/

 virtual void store_linearization( const LinearizationName name ) override final;

/*--------------------------------------------------------------------------*/
 /// compute the Function
 /** It has to compute the Function. The parameter changedvars is ignored.
  *
  *  It is assumed that the sub-Block (B) has its own Variable only. The
  *  re-optimization of (B) shall be performed starting from the old
  *  solution and any problem shouldn't occur. No relevant Variable are
  *  defined in (B). */

 virtual int compute( bool changedvars = true ) override;

/*--------------------------------------------------------------------------*/
 /// returns the value of the Function
 /** It returns the value of the Function that was computed in the most recent
  * call to compute(); if the latter has never been invoked, then the value returned
  * by this method is meaningless.
  *
  * If (B) is computed with a low accuracy and the function value lays in an
  * interval, the upper bound shall be returned (the lower bound if (B) is a
  * minimization problem). */

 virtual FunctionValue get_value( void ) const override;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual FunctionValue get_lower_estimate( void ) const override final{
  Solver* slv = v_Block[0]->get_registered_solvers().back();
  return( slv->get_lb() );
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

 virtual FunctionValue get_upper_estimate( void ) const override final{
  Solver* slv = v_Block[0]->get_registered_solvers().back();
  return( slv->get_ub() );
  }

/*--------------------------------------------------------------------------*/
/// retrieve the coefficients (g vector) of a linearization in a vector
/** This method retrieves the vector of coefficients g that is the (largest)
  * part of the linearization with the given name.
  *
  * This implements the virtual function of class C05Function. If name is
  * INF<LinearizationName>, the current linearization of the local pool
  * will be unavailable. */

 virtual void get_linearization_coefficients( FunctionValue * g ,
   const LinearizationName name =
                              std::numeric_limits<LinearizationName>::max() ,
   c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
   c_Index end = std::numeric_limits<Index>::max() ) override final;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/// retrieve the coefficients (g) of a linearization in a sparse vector
/** This method retrieves the sparse vector of coefficients g that is part
  * of a linearization.
  *
  * This implements the virtual function of class C05Function. If name is
  * INF<LinearizationName>, the current linearization of the local pool
  * will be unavailable.  */

 virtual void get_linearization_coefficients( SparseVector &g ,
   const LinearizationName name =
                              std::numeric_limits<LinearizationName>::max() ,
   c_Vec_Index * const indices = nullptr , c_Index start = 0 ,
   c_Index end = std::numeric_limits<Index>::max() ) override final;

/*--------------------------------------------------------------------------*/

 /** There is only one linearization in a LagBFunction, its value being
  * the opposite of its constant term. */

 virtual double get_linearization_constant( const LinearizationName name =
   std::numeric_limits<Index>::max() ) const override final;

/*@} -----------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/
/** @name Handling the parameters of the LagBFunction
 *  @{ */

 ///< get the whole (empty) set of parameters in one blow
 /** Although a LagBFunction formally has a lot of parameters, in fact it
  * "listens to no-one"; hence, the implementation of get_ComputeConfig() is
  * quite a trivial one. */

 virtual ComputeConfig * get_ComputeConfig( bool all = false ,
		       ComputeConfig * ocfg = nullptr ) const override final
 {
  return( nullptr );
  }

/*--------------------------------------------------------------------------*/

 virtual int get_dflt_int_par( const idx_type par ) const override;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 virtual double get_dflt_dbl_par( const idx_type par ) const override;

/*@} -----------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for handling the set of "active" Variable in the
 * LagBFunction; this is the actual concrete implementation exploiting the
 * vector v_pairs of pairs.
 * @{ */


/*@} -----------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for modifying the LagBFunction
 *  @{ */

/*@} -----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
    @{ */

 /// printing the LagBFunction
 virtual void print( std::ostream &output ) const override {

  } // end LagBFunction::print( )

/*@} -----------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/


 v_dual_pair slag_p;
 ///< vector of static Lagrangian pairs

 l_dual_pair dlag_p;
 ///< list of dynamic Lagrangian pairs

 v_linearization_pair g_pool_var;
 ///< global pool of variables

 std::vector<double> g_pool_cns;
 ///< global pool of constants

 LinearizationName LastSolution;
 ///< the last solution read by get_linearization

 char VarType;
 ///< the type of variable contained in the solver

 FRealObjective obj;
 ///< the (linear) objective function

 // FunctionValue lb_value;
 ///< the lower bound to the value of (B)

 // FunctionValue ub_value;
 ///< the upper bound to the value of (B)

 m_column LagMatrix;
 ///< the matrix yA in the Lagrangian function

 int GPMaxSz;
 ///< maximum size of the "global pool"

 int LPMaxSz;
 ///< maximum size of the "local pool"

 double RAccLin;
 ///< maximum relative error in any linearization

 double AAccLin;
 ///< maximum absolute error in any reported solution

/*@}------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

   void set_static_structure( v_dual_pair & vdp );
   void set_dynamic_structure( l_dual_pair & ldp );

   void store_function( );

   void update_function( );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

   void guts_of_destructor( );

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunction ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS LagBFunctionModRngd ---------------------------*/
/*--------------------------------------------------------------------------*/
/// derived from C05FunctionModVarsRngd for changes in a range of coefficients
/** Derived class from C05FunctionModVarsRngd to describe changes specific to
 * a LagBFunction, i.e., those of a range of coefficients.
 *
 * The change of some of the coefficients in a linear function perfectly
 * coincides with what the type of modification of LagBFunctionModRngd
 * "SomeEntriesChange" postulates. Indeed, there is no real reason for
 * defining this class, as it is identical to LagBFunctionModRngd, save
 * for the fact that some Block / Solver may want to be sure that the
 * Modification is actually coming out of a LagBFunction. */

 class LagBFunctionModRngd : public C05FunctionModVarsRngd
 {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of C05FunctionModVarsRngd

 LagBFunctionModRngd( Function * const f , const int mod ,
			Variable * const strt = nullptr ,
			Variable * const stop = nullptr ,
			const FunctionValue shift = 0 , const bool cB = true )
  : C05FunctionModVarsRngd( f , mod , strt , stop , shift , cB ) { }

 virtual ~LagBFunctionModRngd() { }  ///< destructor, does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// print the LagBFunctionModRngd

 virtual inline void print( std::ostream &output ) const {

  } // end LagBFunctionModRngd::print( ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunctionModRngd ) )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS LagBFunctionModSbst -------------------------*/
/*--------------------------------------------------------------------------*/
/// derived from C05FunctionModVarsSbst for changes in subset of coefficients
/** Derived class from C05FunctionModVarsSbst to describe changes specific to
 * a LagBFunction, i.e., those of a subset of coefficients.
 *
 * The change of some of the coefficients in a linear function perfectly
 * coincides with what the type of modification of C05FunctionModVarsSbst
 * "SomeEntriesChange" postulates. Indeed, there is no real reason for
 * defining this class, as it is identical to C05FunctionModVarsSbst, save
 * for the fact that some Block / Solver may want to be sure that the
 * Modification is actually coming out of a LagBFunction. */

 class LagBFunctionModSbst : public C05FunctionModVarsSbst
 {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*-------------------- CONSTRUCTOR AND DESTRUCTOR --------------------------*/

 /// constructor: identical to that of C05FunctionModVarsSbst

 LagBFunctionModSbst( Function * const f , const int mod ,
			std::vector<Variable *> && vars ,
			FunctionValue shift = 0 , const bool cB = true )
  : C05FunctionModVarsSbst( f , mod , std::move( vars ) , shift , cB ) { }

 virtual ~LagBFunctionModSbst() { }  ///< destructor, does nothing

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the LagBFunctionModSbst

 virtual inline void print( std::ostream &output ) const {

  } // end LagBFunctionModSbst::print( ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

 };  // end( class( LagBFunctionModSbst ) )

/*@}  end( group( LFun_CLASSES ) ) */
/*--------------------------------------------------------------------------*/

 }  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* LagBFunction.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File LagBFunction.h --------------------------*/
/*--------------------------------------------------------------------------*/

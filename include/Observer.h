/*--------------------------------------------------------------------------*/
/*---------------------------- File Observer.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Observer class, an abstract base class implementing
 * the concept of an observer which can be notified about Modifications.
 *
 * \version 0.31
 *
 * \date 02 - 02 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Observer
#define __Observer  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Modification.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Observer_CLASSES Classes in Observer.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Observer -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// an observer that is interested in Modification
/** The Observer class implements an observer as in the design pattern of the
 * same name. This Observer is only interested in Modification and has the
 * (pure virtual, which is why this is an abstract class) add_Modification()
 * method whereby the subjects can report the Modification.
 *
 * In SMS++, Modification has to reach all the interested Solver. If the
 * Modification is directly issued by, say, a Variable, Constraint or
 * Objective, this can be done directly via the Block (which is an Observer),
 * to which both the Modification-spewing object and the interested Solver
 * are attached (directly, or to any ancestor). However, the reason for a
 * separate Observer class is that some objects (e.g., Function) can issue
 * Modification, but they may not be directly attached to a Block. Rather,
 * they can may "live inside another object"; a Function, for instance, may
 * be used by some Constraint or Objective to "implement themselves". In
 * this case, the object that "hosts" a Modification-spewing one has to be
 * an Observer in order for the Modification to be received, and ultimately
 * routed to the appropriate Block. A possible example of this behaviour is
 * that of "Function of Function" objects (SumFuction, CompositeFunction,
 * ...), which may be easily implemented provided that that they themselves
 * are made Observer.
 *
 * The Observer class also supports the following notions:
 *
 * - Modification can be sent to "different channels"; besides the "default
 *   channel", shipping them immediately to the Solver, new channels can
 *   be dynamically opened and closed that allow to bunch set of "logically
 *   related Modification" together in order to make it easier for the Solver
 *   to react to them.
 *
 * - The Observer has a way for telling the observed object whether "nobody
 *   is listening", and therefore there is no need to issue the Modification
 *   at all.
 *
 * - The Observer provides a few convenience methods for formatting in an
 *   uniform way the parameter of calls to methods that produce Modification
 *   which specifies if, how and where the Modification has to be issued.
 */

class Observer {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the "name" of a Modification "channel"
 typedef unsigned short int ChnlName;

 /// a const ChnlName
 typedef const ChnlName c_ChnlName;

/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *  @{ */

 /// constructor of Observer, it has nothing to do
 Observer() = default;

/*--------------------------------------------------------------------------*/
 ///< copy constructor, it has nothing to do either
 Observer( const Observer & ) = default;

/*--------------------------------------------------------------------------*/
 ///< destructor: it is virtual, and empty
 virtual ~Observer() = default;

/**@} ----------------------------------------------------------------------*/
/*------------- METHODS DESCRIBING THE BEHAVIOR OF AN Observer -------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of an Observer
 *  @{ */

 /// returns the Block to which this Observer belongs, or is
 /** Any Observer either is a Block, or belongs to one. This method has to
  * return a pointer to such Block. */

 [[nodiscard]] virtual Block * get_Block() const = 0;

/*--------------------------------------------------------------------------*/

 /// returns true if there is "anybody listening to Modification"
 /** Returns true if there is "anybody listening to Modification". In case of
  * a Block, this typically means that there is some Solver "listening to
  * this Block", which means either registered to this Bock or registered to
  * any ancestor (father, father of father, ...) of this Block. Other
  * Observer may either just redirect the method to the Block they belong to,
  * or force a different response in case they are "interested" to the
  * Modification themselves because they need to "intercept them and change
  * them along the way". */

 [[nodiscard]] virtual bool anyone_there() const = 0;

/*--------------------------------------------------------------------------*/
 /// notify this Observer about a Modification
 /** This method notifies this Observer about a Modification.
  *
  * mod is a smart pointer to an object of class (derived from)  Modification
  * (std::shared_ptr<Modification>, sp_Mod in Modification.h), that has been
  * created by some object (Block, Constraint, Variable, Function, ...) and it
  * is passed to this Observer. Ultimately the Modification will have to reach
  * a Block for being dispatched to all Solver attached to it, as well as to
  * all Solver attached to its ancestors (the father, the father of the
  * father, ...). Some Modification-spewing objects (typically, Function) may
  * not be directly observed by a Block but by something else (typically,
  * FRowConstraint and FRealObjective), which however ultimately belongs to
  * a Block; if the Observer is not a Block, it is its responsibility that
  * the Modification object is ultimately dispatched to the Block. Indeed,
  * :Block may also use "abstract" Modification to update their "physical
  * representation" to keep it in synch with their "abstract representation"
  * [see Modification::concerns_Block()]. Furthermore, note that a non-Block
  * Observer may need to "translate" the Modification object into one or more
  * different Modification objects, according to the effect that the original
  * Modification (say, to a Function) has on the Observer (say, a
  * FRowConstraint).
  *
  * Since what is passed to this method is a shared pointer, the Modification
  * object is automatically destroyed after that all Solver have deleted it
  * (or, if a non-Block Observer translates it into a new set of
  * Modification, immediately after that). Note that the preferred way of
  * calling this method is
  *
  *     <Observer>.add_Modification(
  *                       std::make_shared<DerivedModification>( <params> ) );
  *
  * where <params> are the parameters of the constructor of
  * DerivedModification that should be called. Using std::make_shared<> is
  * slightly faster then doing "new DerivedModification( <params> )" and
  * then converting the returned ordinary pointer to a smart one, and calling
  * it within the call to add_Modification() may avoid one copy of the
  * shared pointer to be made only to be immediately destroyed. Not that this
  * matter much, because copies of the smart pointer (say, in the Solver
  * attached to the Block to which <Observer> belongs, with Block ==
  * <Observer> a definite possibility, and its ancestors, if any) will have
  * been made by then. Hence, even if a local copy were done, destroying
  * it would only decrease the counter by 1, effectively deleting it if and
  * only if there are no interested Solver, which is the intended semantic.
  *
  * Also, note that the Solver (or, possibly, the Observer itself if it has
  * to "translate" it) will have to downcast the Modification object to an
  * actual DerivedModification in order to gather the information about what
  * has been modified. These being smart pointers, this cannot be achieved
  * with the ordinary dynamic_cast<>; rather,
  *
  *     auto dmod = std::dynamic_pointer_cast< DerivedModification >( mod );
  *
  * has to be used. Note that one can check that the dynamic cast worked as
  * for an ordinary pointer, i.e., with
  *
  *     dmod != nullptr       or        ! dmod
  *
  * (thanks to shared_ptr operator bool). The alternative is to first extract
  * the original Modification * from mod using get() and then use the "normal"
  * dynamic_cast, as in
  *
  *     auto dmod = dynamic_cast< DerivedModification * const >( mod.get() );
  *
  * (note that the "const" above is not strictly necessary, but in general an
  * Observer is not supposed to change a Modification, and in fact basically
  * all methods of Modification are const anyway).
  *
  * It may be helpful to Solver that sets of "logically related Modification"
  * be dispatched together. For this reason, Observer supports the notion that
  * set of Modification can be bunched together into a GroupModification
  * object. This is done by defining different "channels" where Modification
  * can be sent, which are opened with open_channel(). The parameter chnl
  * specifies to which of the currently "open" GroupModification objects mod
  * has to be appended, with the following format:
  *
  * - If chnl is the name of a currently open channel, which in particular
  *   implies that it is != 0, then mod is added to the corresponding
  *   GroupModification object (possibly "nested inside it", see
  *   open_channel() for details).
  *
  * - If chnl is not the name of a currently open channel, (say, it is 0 as
  *   in the default), then mod is "sent to the default channel". Unless this
  *   is changed with set_default_channel(), this means that mod is not added
  *   to any GroupModification, and just immediately sent along to the
  *   interested Solver / Block.
  *
  * This mechanism is implemented into Block::add_Modification(), and it is
  * not assumed to be re-implemented in different ways by :Block or other
  * :Observer. This is important in that it allows to enforce a useful
  * property:
  *
  *     THE EXACT :Block IN WHICH A :Modification HAPPEN THAT IS BEING
  *     SENT TO SOME NON-0 CHANNEL WILL NOT "SEE" THE GroupModification,
  *     BUT RATHER THE INDIVIDUAL :Modification THAT WILL EVENTUALLY BE
  *     PACKED INTO IT
  *
  * This property is *only* true for the very specific :Block, and it does
  * not hold true for either its ancestor Block (which also receive the
  * Modification) and the Solver. However, this is important for the handling
  * of "abstract" Modification, since it is very useful that that :Block 
  * (and that :Block only) is able to "see the Modification immediately" to
  * handle the corresponding changes to the "physical representation". See
  * the comments to Block::add_Modification() for more details. */

 virtual void add_Modification( sp_Mod mod, ChnlName chnl = 0 ) = 0;

/*--------------------------------------------------------------------------*/
 /// "open" a channel
 /** This method allows to start "bunching together" a set of "logically
  * related Modification". 
  *
  * When it is invoked, a "new channel is opened". This means that a
  * GroupModification object is (created, unless it is provided, and it is)
  * assigned a new unique name, which is returned. Then, being chnl the
  * returned value, a call to add_Modification( mod , chnl ) adds mod at the
  * end of the STL container of the GroupModification, rather than
  * dispatching it to the Solver / Block. The latter operaton is done when
  * close_channel( chnl ) is called.
  *
  * If the parameter gmpmod is != nullptr, then the pointed object is taken
  * as the GroupModification that is "opened". This means that the object
  * becomes "property" of the Observer; the raw pointer is later packaged in
  * a smart pointer (when the channel is closed), so it is crucial that no
  * copies of the raw pointer are retained. This is done in order to allow
  * the caller to provide objects of *derived classes* from
  * GroupModification; these may contain other data that is useful to the
  * Solver / Block to process the GroupModification, and even just being of
  * a specific :GroupModification class may help. If gmpmod is == nullptr,
  * an object of the base GroupModification class is automatically
  * constructed by the method. */

 virtual ChnlName open_channel( GroupModification * gmpmod = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// create a new level in the given channel
 /** This method allows to nest a new GroupModification into the existing
  * GroupModification associated with the given channel.
  *
  * When it is invoked, a GroupModification object is (created, unless it is
  * provided, and it is) appended at the end of the STL container in the
  * "outer" GroupModification associated to the channel. Then, a call to
  * add_Modification( mod , chnl ) adds mod to the "inner" GroupModification.
  * This lasts until the method is called again, on which case an
  * inner-inner-GroupModification is started (...), or either 
  * un_nest_channel( chnl ) or close_channel( chnl ) are called.
  *
  * The parameter gmpmod has the same meaning as in open_channel() and is
  * provided for the same reason.
  *
  * Calling the method with chnl non being the name of an open channel is an
  * error and should throw exception. */

 virtual void
 nest_channel( ChnlName chnl, GroupModification * gmpmod = nullptr ) = 0;

/*--------------------------------------------------------------------------*/
 /// push back to the previous level of a channel
 /** This method allows to "finalize" the "inner" (...) GroupModification
  * associated with the given channe, in the sense that and addition of
  * Modification is resumed for the "father" GroupModification of that
  * object (which is still associated to the same channel).
  *
  * Calling this method on a channel that is either not open, or which is in
  * "root mode" (the current GroupModification object is not contained into
  * any other GroupModification) is an error and should throw exception. */

 virtual void un_nest_channel( ChnlName chnl ) = 0;

/*--------------------------------------------------------------------------*/
 /// "close" a channel
 /** This method allows to "finalize" the set of "logically related
  * Modification" contained in the GroupModification associated with the
  * channel chnl. Whatever the "state" of the channel, i.e., whether or not
  * the channel is in "root mode" (currently adding to the outermost
  * GroupModification of the channel, as opposed to some GroupModification
  * inside another GroupModification), the "outermost" GroupModification is
  * finally shipped to the interested Solver and Block. This also closes the
  * channel, i.e., chnl is no longer the name of an open channel until it is
  * produced again by a call to open_channel().
  *
  * Closing a non-open channel is an error and should throw exception. */

 virtual void close_channel( ChnlName chnl ) = 0;

/*--------------------------------------------------------------------------*/
 /// set the "default" channel
 /** This method allows to "silently redirect" any Modification that is added
  * with add_Modification( mod , 0 ) to the specified open channel. This
  * mechanism is provided in case the Modification is generated by a
  * "complex reaction", i.e., in a method called by a method called by a
  * method ... called by the method that performs the changes whose
  * corresponding Modification should be bunched together (and where, most
  * likely, the GroupModification is opened). The point is that to send a
  * particular Modification to a particular channel it is necessary
  * to specify the channel name in add_Modification(), but it may not always
  * be possible to control what name is used in a method called by a method
  * called by a method ... Thus, this mechanism allows to (temporarily)
  * "silently hijack" the "standard channel" 0. Of course, some care has to
  * be exercised while using this.
  * 
  * If chnl is the name of a currently "open" GroupModification object, which
  * in particular implies that it is != 0, then all subsequent Modification
  * sent to "channel 0" are redirected to that GroupModification. This ends
  * if set_default_channel( 0 ) is called, or if the channel to which 0 has
  * been redirected is closed. Calling the method with chnl not the name of
  * an open channel or zero is an error and should throw exception. */

 virtual void set_default_channel( ChnlName chnl = 0 ) = 0;

/*--------------------------------------------------------------------------*/
 /// method to "pack" all info about issuing Modification in one parameter
 /** When a method is called that may produce a Modification, it is necessary
  * to specify some information about if, how and where the Modification has
  * to be issued. In particular, one has to decide if the Modification has to
  * be issued at all, and its concerns_Block() value, according to the format
  * set by the amododification_type enum [see Modification.h]:
  *
  * - eDryRun   the change that the method is supposed to perform, which would
  *             result in a Modification would be issued, must *not* be done;
  *             as a consequence, no Modification should be issued. Allowing
  *             to call a method and actually not doing the change that the
  *             method should do is useful in particular for methods that
  *             change both the "abstract" and the "physical" representation
  *             (say, of a Block), but for which it may be useful to switch
  *             off the changes in one of the two, say becaise one is sure
  *             that the changes have already been done (for instance, because
  *             one is reacting to one AModification implying that the
  *             "abstract" one has changed already).
  *
  * - eNoMod    the Modification is *not* issued: this should not be done
  *             unless for some reason it is guaranteed that neither the
  *             Block nor any Solver will ever need this information;
  *
  * - eNoBlck   the Modification is issued, but *only* if there is "anyone
  *             listening"; furthermore, the concerns_Block() value is set
  *             to false, meaning that the Block receiving this Modification
  *             can safely ignore it on knowledge that the corresponding
  *             change in the Block has already happened;
  *
  * - eModBlck  the Modification is issued whether or not there is "anyone
  *             listening", and the concerns_Block() value is set to true;
  *             this is the default, "most conservative" setting.
  *
  * Furthemore, it is necessary to specify to which channel the Modification
  * is sent. This information can be "packed" into one single parameter, which
  * is what this method does: iM is the parameter containing one of the above
  * three values, and chnl the "name" of the channel. It is guaranteed that
  * make_par( iM , 0 ) == iM, so this method is only needed when sending to
  * a non-default channel. */

 static inline ModParam make_par( c_ModParam iM, c_ChnlName chnl ) {
  return ( iM + 4 * chnl );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the channel information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the channel information alone. */

 static inline ChnlName par2chnl( c_ModParam issueMod ) {
  return ( issueMod / 4 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the ModParam information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the ModParam information alone. */

 static inline ModParam par2mod( c_ModParam issueMod ) {
  return ( issueMod & 3 );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 /// method extracting the concerns_Block information
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns the boolean to become the concerns_Block. */

 static inline bool par2concern( c_ModParam issueMod ) {
  return ( par2mod( issueMod ) == eModBlck );
 }

/*--------------------------------------------------------------------------*/
 /// method for checking if a(n abstract) Modification has to be issued
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that a Modification must be issued.
  * It is intended that the Modification is an "abstract" one, because the
  * value eModBlck only makes sense for them. This also depends on the value
  * reported by anyone_there(), which is why the method is not static. */

 inline bool issue_mod( c_ModParam issueMod ) const {
  return ( ( par2mod( issueMod ) == eModBlck ) ||
           ( ( par2mod( issueMod ) == eNoBlck ) && anyone_there() ) );
 }

/*--------------------------------------------------------------------------*/
 /// method for checking if a (physical) Modification has to be issued
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that a Modification must be issued.
  * This is intended only for "physical" Modification, in that it ignores the
  * value eModBlck and treats it as if it were eNoBlck. This also depends on
  * the value reported by anyone_there(), which is why the method is not
  * static. */

 [[nodiscard]] inline bool issue_pmod( c_ModParam issueMod ) const {
  return ( par2mod( issueMod ) && anyone_there() );
 }

/*--------------------------------------------------------------------------*/
 /// method for checking if a the change has to be made
 /** Given a "composite" parameter as produced by make_par(), this method
  * returns true if the parameter implies that the actual change has to be
  * made (if not, clearly no Modification must be issued). Allowing to call
  * a method and actually not doing the change that the method should do is
  * useful in particular for methods that change both the "abstract" and the
  * "physical" representation (say, of a Block), but for which it may be
  * useful to switch off the changes in one of the two, say becaise one is
  * sure that the changes have already been done (for instance, because one
  * is reacting to one AModification implying that the "abstract" one has
  * changed already). */

 static inline bool not_dry_run( c_ModParam issueMod ) {
  return ( par2mod( issueMod ) );
 }

/**@} ----------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

};  // end( class( Observer ) )

/*--------------------------------------------------------------------------*/

/** @}  end( group( Observer_CLASSES ) ) -----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* Observer.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Observer.h ---------------------------*/
/*--------------------------------------------------------------------------*/

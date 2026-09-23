/*--------------------------------------------------------------------------*/
/*--------------------------- File Change.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the classes defined in Change.h: everything is defined
 * in the header, only the factory registration of the concrete GroupChange
 * lives here.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Filippo Magi, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Change.h"

#include "Block.h"
#include "ColVariable.h"
#include "Objective.h"
#include "Function.h"
#include "FRealObjective.h"
#include "LinearFunction.h"
#include "DQuadFunction.h"


/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register GroupChange to the Change factory

SMSpp_insert_in_factory_cpp_0( GroupChange );

using Index = Block::Index;

void AbstractChange::deserialize(const netCDF::NcGroup &group)
{
    auto ftype = group.getAtt("AbstractChange_type");
    if (ftype.isNull())
        throw std::invalid_argument("AbstractChange_type attribute not found in netCDF group");
    ftype.getValues(&f_type);
    // read data
    netCDF::NcDim ni = group.getDim("dim");
    netCDF::NcVar data = group.getVar("Data");
    if (data.isNull())
        v_data.clear();
    else
    {
        v_data.resize(ni.getSize());
        data.getVar(v_data.data());
    }
    // read AbstractPath
    auto pg = group.getGroup("VariablesPath");
    if (pg.isNull())
        v_paths.clear();
    else
        v_paths = AbstractPath::vector_deserialize(pg);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void AbstractChange::serialize(netCDF::NcGroup &group) const
{

    // always call the method of the base class first
    Change::serialize(group);

    group.putAtt("AbstractChange_type", netCDF::NcInt(), f_type);

    netCDF::NcDim ni = group.addDim("dim", v_data.size());
    (group.addVar("Data",
                  netCDF::NcDouble(), ni))
        .putVar(v_data.data());
    if (!v_paths.empty())
    {
        auto pg = group.addGroup("VariablesPath");
        AbstractPath::serialize(v_paths, pg);
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

Change *AbstractChange::apply(Block *block, bool doUndo,
                                ModParam issueMod,
                                ModParam issueAMod)
{
    Change *returnChange = nullptr;
    switch (f_type)
    {
    case eEmpty:
        throw std::invalid_argument("AbstractChange: empty change cannot be applied");
        // Change Obj require in data[0] index, data[1] new coefficient, data[2] new quadratic coefficient (if quadratic)
        // in this case constains AbstractPath contains only 2 element, the variable and the objective function
    case eChgObj:
    {
        ColVariable *pv = nullptr;
        Function *fobj = nullptr;
        if (v_paths.size() != 2)
            throw std::invalid_argument(
                "AbstractChange: eChgObj requires 2 AbstractPath elements (variable and objective function)");
        for (const auto &path : v_paths)
        {
            auto node_type = path.get_last_node(block).type;

            if (node_type == 'V' || node_type == 'v')
                pv = path.get_element<ColVariable>(block);
            else if (node_type == 'O')
            {
                auto *obj = dynamic_cast<FRealObjective *>(
                    path.get_element<Objective>(block));
                if (obj)
                    fobj = obj->get_function();
            }
        }
        if (!pv || !fobj)
            throw std::invalid_argument(
                "AbstractChange: eChgObj requires a variable and an objective function in AbstractPath");

        // --- Quadratic Case -----------------------------------------------
        if (auto qf = dynamic_cast<DQuadFunction *>(fobj))
        {
            if (v_data.size() != 2)
                throw std::invalid_argument(
                    "AbstractChange: eChgObj on quadratic objective needs 2 values");

            auto pos = qf->is_active(pv);
            const auto new_c1 = v_data[0];
            const auto new_c2 = v_data[1];

            double old_c1 = 0.0, old_c2 = 0.0;
            if (pos < qf->get_num_active_var())
            {
                old_c1 = qf->get_linear_coefficient(pos);
                old_c2 = qf->get_quadratic_coefficient(pos);
                qf->modify_term(pos, new_c1, new_c2, issueMod);
            }
            else
                qf->add_variable(pv, new_c1, new_c2, issueMod);

            if (doUndo)
                returnChange = (new AbstractChange(eChgObj,
                                                     {old_c1, old_c2}, std::vector<AbstractPath>{v_paths}));
        }

        // --- Linear Case -----------------------------------------------
        else if (auto lf = dynamic_cast<LinearFunction *>(fobj))
        {
            if (v_data.size() != 1)
                throw std::invalid_argument(
                    "AbstractChange: eChgObj on linear objective needs 1 value");

            const auto new_c1 = v_data[0];
            auto pos = lf->is_active(pv);

            const double old_c1 = (pos < lf->get_num_active_var())
                                      ? lf->get_coefficient(pos)
                                      : 0.0;

            if (pos < lf->get_num_active_var())
                lf->modify_coefficient(pos, new_c1, issueMod);
            else
                lf->add_variable(pv, new_c1, issueMod);

            if (doUndo)
                returnChange = (new AbstractChange(eChgObj, {old_c1}, std::vector<AbstractPath>{v_paths}));
        }
        else
            throw std::invalid_argument(
                "AbstractChange: objective Function type not supported");
        break;
    }

    // v_paths contains only the objective function, and v_data[0] contains the new sense
    case eChgSense:
    {
        // apply change to the block
        auto obj = v_paths[0].get_element<Objective>(block);
        if (doUndo)
            returnChange = new AbstractChange(eChgSense, std::vector<double>{static_cast<double>(block->get_objective()->get_sense())}, std::vector<AbstractPath>{v_paths});
        obj->set_sense(static_cast<int>(v_data[0]));
        break;
    }
    // v_paths constains the variable, v_data[0] contains the the new integrality
    case eChgIntegrality:
    {
        const bool new_integer = (v_data[0] != 0.0);
        auto pv = v_paths[0].get_element<ColVariable>(block);
        const bool old_integer = pv->is_integer();

        pv->is_integer(new_integer, issueMod);
        if (doUndo)
            returnChange = new AbstractChange(eChgIntegrality,
                                                {old_integer ? 1.0 : 0.0}, std::vector<AbstractPath>{v_paths});
        break;
    }
    // v_paths constains the variable, v_data[0] contains the new fixed value
    case eFixX:
    {
        const auto fix_value = v_data[0];

        auto pv = v_paths[0].get_element<ColVariable>(block);
        const bool was_fixed = pv->is_fixed();

        if (doUndo)
            returnChange = was_fixed ? new AbstractChange(eFixX, {pv->get_value()}, std::vector<AbstractPath>{v_paths}) : new AbstractChange(eUnfixX, {}, std::vector<AbstractPath>{v_paths});
        pv->set_value(fix_value);
        pv->is_fixed(true, issueMod);
        break;
    }
    // v_paths constains the variable
    case eUnfixX:
    {
        auto pv = v_paths[0].get_element<ColVariable>(block);

        pv->is_fixed(false, issueMod);

        if (doUndo)
            returnChange = new AbstractChange(eFixX, {pv->get_value()}, std::vector<AbstractPath>{v_paths});
        break;
    }
    // v_paths constains the variable, v_data[0] contains the new lower bound
    case eChgLB:
    {
        auto pv = v_paths[0].get_element<ColVariable>(block);
        const auto new_lb = v_data[0];
        bool found = false;
        for (Index i = 0; i < pv->get_num_active(); ++i)
        {
            auto *dep = pv->get_active(i);
            if (auto *c = dynamic_cast<BoxConstraint *>(dep))
            {
                if (doUndo)
                    returnChange = new AbstractChange(eChgLB, {c->get_lhs()}, std::vector<AbstractPath>{v_paths});
                c->set_lhs(new_lb, issueMod);
                found = true;
                break;
            }
            else if (auto *c = dynamic_cast<LBConstraint *>(dep))
            {
                if (doUndo)
                    returnChange = new AbstractChange(eChgLB, {c->get_lhs()}, std::vector<AbstractPath>{v_paths});
                c->set_lhs(new_lb, issueMod);
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw std::invalid_argument("AbstractChange: eChgLB requires a variable with an existing BoxConstraint or LBConstraint");
            /*                     if (doUndo)
                                    returnChange = new AbstractChange(eChgLB, {pv->get_lb()}, std::vector<AbstractPath>{v_paths});
                                // auto con = new LBConstraint(pv->get_Block(), pv, new_lb);
                                Block *blk = pv->get_Block();
                                Index idx = Inf<Index>();

                                auto &d_constraints = blk->get_dynamic_constraints(); // c_Vec_any &

                                for (Index i = 0; i < d_constraints.size(); ++i)
                                {
                                    if (d_constraints[i].type() == typeid(std::list<LBConstraint>))
                                    {
                                        idx = i;
                                        break;
                                    }
                                }

                                if (idx == Inf<Index>())
                                {
                                    // nessun gruppo di LBConstraint dinamici: lo registriamo ora
                                        throw std::invalid_argument("No dynamic LBConstraint group found in block. Please register a dynamic LBConstraint group before applying AbstractChange eChgLB.");
                                }

                                auto *d_list = boost::any_cast<std::list<LBConstraint>>(&d_constraints[idx]);
                                // d_list è garantito non-nullptr qui, perché idx è stato appena
                                // verificato/creato per contenere esattamente std::list<LBConstraint>

                                std::list<LBConstraint> newlist;
                                newlist.emplace_back(blk, pv, new_lb);

                                blk->add_dynamic_constraints(*d_list, newlist, issueMod);
             */
        }
        break;
    }
    case eChgUB:
    {
        auto pv = v_paths[0].get_element<ColVariable>(block);
        const auto new_ub = v_data[0];
        bool found = false;
        for (Index i = 0; i < pv->get_num_active(); ++i)
        {
            auto *dep = pv->get_active(i);
            if (auto *c = dynamic_cast<BoxConstraint *>(dep))
            {
                if (doUndo)
                    returnChange = new AbstractChange(eChgUB, {c->get_rhs()}, std::vector<AbstractPath>{v_paths});
                c->set_rhs(new_ub, issueMod);
                found = true;
                break;
            }
            else if (auto *c = dynamic_cast<UBConstraint *>(dep))
            {
                if (doUndo)
                    returnChange = new AbstractChange(eChgUB, {c->get_rhs()}, std::vector<AbstractPath>{v_paths});
                c->set_rhs(new_ub, issueMod);
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw std::invalid_argument("AbstractChange: eChgUB requires a variable with an existing BoxConstraint or UBConstraint");
            /*                     if (doUndo)
                                    returnChange = new AbstractChange(eChgUB, {pv->get_ub()}, std::vector<AbstractPath>{v_paths});
                                // auto con = new UBConstraint(pv->get_Block(), pv, new_ub);
                                auto blk = pv->get_Block();
                                Index idx = Inf<Index>();
                                auto &d_constraints = blk->get_dynamic_constraints();
                                for (Index i = 0; i < d_constraints.size(); ++i)
                                {
                                    if (d_constraints[i].type() == typeid(std::list<UBConstraint>))
                                    {
                                        idx = i;
                                        break;
                                    }
                                }
                                if (idx == Inf<Index>())
                                {
                                    throw std::invalid_argument("No dynamic UBConstraint group found in block. Please register a dynamic UBConstraint group before applying AbstractChange eChgUB.");
                                }
                                auto *d_list = boost::any_cast<std::list<UBConstraint>>(&d_constraints[idx]);
                                std::list<UBConstraint> newlist;
                                newlist.emplace_back(blk, pv, new_ub);
                                blk->add_dynamic_constraints(*d_list, std::move(newlist), issueMod);
             */
        }
        break;
    }
    default:
    {
        throw std::invalid_argument("AbstractChange: unknown change type");
    }
    }
    return returnChange;
}
SMSpp_insert_in_factory_cpp_0( AbstractChange );



/*--------------------------------------------------------------------------*/
/*-------------------------- End File Change.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/

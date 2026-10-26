/*--------------------------------------------------------------------------*/
/*---------------------------- File SMSpp.cppm -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module smspp, which exports every name that the
 * headers of SMS++ declare at namespace scope; a translation unit can then
 * write
 *
 *     import smspp;
 *
 * in place of the #include of the headers. The macros (e.g., those of the
 * factory in SMSFactory.h) are not exported by a module, so a file that uses
 * them still includes the header that defines them.
 *
 * This file is written by cmake/smspp_module_exports.py, which reads the
 * names from the AST of the headers: it is not to be edited by hand.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*------------------------- GLOBAL MODULE FRAGMENT -------------------------*/
/*--------------------------------------------------------------------------*/

module;

#include "AbstractChange.h"
#include "BendersBlock.h"
#include "BlockSolverConfig.h"
#include "BoxSolver.h"
#include "C05SumFunction.h"
#include "ChangeSolver.h"
#include "ColRowSolution.h"
#include "DataMapping.h"
#include "FakeSolver.h"
#include "LagBFunction.h"
#include "LinearConstraint.h"
#include "MasterProblemBlock.h"
#include "QuadFunction.h"
#include "RBlockConfig.h"
#include "UpdateSolver.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module smspp;

export namespace SMSpp_di_unipi_it {

 // AbstractBlock.h
 using SMSpp_di_unipi_it::AbstractBlock;

 // AbstractChange.h
 using SMSpp_di_unipi_it::AbstractChange;

 // AbstractPath.h
 using SMSpp_di_unipi_it::AbstractPath;

 // BendersBFunction.h
 using SMSpp_di_unipi_it::BendersBFunction;
 using SMSpp_di_unipi_it::BendersBFunctionMod;
 using SMSpp_di_unipi_it::BendersBFunctionModAddd;
 using SMSpp_di_unipi_it::BendersBFunctionModRngd;
 using SMSpp_di_unipi_it::BendersBFunctionModSbst;
 using SMSpp_di_unipi_it::BendersBFunctionState;

 // BendersBlock.h
 using SMSpp_di_unipi_it::BendersBlock;

 // Block.h
 using SMSpp_di_unipi_it::Block;
 using SMSpp_di_unipi_it::BlockConfig;
 using SMSpp_di_unipi_it::BlockMod;
 using SMSpp_di_unipi_it::BlockModAD;
 using SMSpp_di_unipi_it::BlockModAdd;
 using SMSpp_di_unipi_it::BlockModRmv;
 using SMSpp_di_unipi_it::BlockModRmvRngd;
 using SMSpp_di_unipi_it::BlockModRmvSbst;
 using SMSpp_di_unipi_it::Vec_Block;
 using SMSpp_di_unipi_it::Vec_Block_it;
 using SMSpp_di_unipi_it::c_Vec_Block;
 using SMSpp_di_unipi_it::for_each_by_classname;
 using SMSpp_di_unipi_it::p_Block;

 // BlockSolverConfig.h
 using SMSpp_di_unipi_it::BlockSolverConfig;
 using SMSpp_di_unipi_it::Index;
 using SMSpp_di_unipi_it::MetaBlockSolverConfig;
 using SMSpp_di_unipi_it::RBlockSolverConfig;

 // BoxSolver.h
 using SMSpp_di_unipi_it::BoxSolver;

 // C05Function.h
 using SMSpp_di_unipi_it::C05Function;
 using SMSpp_di_unipi_it::C05FunctionMod;
 using SMSpp_di_unipi_it::C05FunctionModLin;
 using SMSpp_di_unipi_it::C05FunctionModLinRngd;
 using SMSpp_di_unipi_it::C05FunctionModLinSbst;
 using SMSpp_di_unipi_it::C05FunctionModRngd;
 using SMSpp_di_unipi_it::C05FunctionModSbst;
 using SMSpp_di_unipi_it::C05FunctionModVarsAddd;
 using SMSpp_di_unipi_it::C05FunctionModVarsRngd;
 using SMSpp_di_unipi_it::C05FunctionModVarsSbst;

 // C05SumFunction.h
 using SMSpp_di_unipi_it::C05SumFunction;

 // C15Function.h
 using SMSpp_di_unipi_it::C15Function;
 using SMSpp_di_unipi_it::C15FunctionMod;

 // CDASolver.h
 using SMSpp_di_unipi_it::CDASolver;

 // Change.h
 using SMSpp_di_unipi_it::Change;
 using SMSpp_di_unipi_it::GroupChange;

 // ChangeSolver.h
 using SMSpp_di_unipi_it::ChangeSolver;
 using SMSpp_di_unipi_it::RelaxationSolver;

 // ColRowSolution.h
 using SMSpp_di_unipi_it::ColRowSolution;

 // ColVariable.h
 using SMSpp_di_unipi_it::ColVariable;

 // ColVariableSolution.h
 using SMSpp_di_unipi_it::ColVariableSolution;
 using SMSpp_di_unipi_it::Vec_ColVariableSolution;
 using SMSpp_di_unipi_it::c_Vec_ColVariableSolution;

 // Configuration.h
 using SMSpp_di_unipi_it::Configuration;
 using SMSpp_di_unipi_it::SimpleConfiguration;
 using SMSpp_di_unipi_it::Vec_p_Conf;
 using SMSpp_di_unipi_it::p_Conf;

 // Constraint.h
 using SMSpp_di_unipi_it::Constraint;
 using SMSpp_di_unipi_it::ConstraintMod;

 // DQuadFunction.h
 using SMSpp_di_unipi_it::DQuadFunction;
 using SMSpp_di_unipi_it::DQuadFunctionModRngd;
 using SMSpp_di_unipi_it::DQuadFunctionModSbst;
 using SMSpp_di_unipi_it::DQuadFunctionModVarsAddd;

 // DataMapping.h
 using SMSpp_di_unipi_it::DataMapping;
 using SMSpp_di_unipi_it::SimpleDataMapping;
 using SMSpp_di_unipi_it::SimpleDataMappingBase;
 using SMSpp_di_unipi_it::SimpleDataMappingFactory;

 // FRealObjective.h
 using SMSpp_di_unipi_it::FRealObjective;
 using SMSpp_di_unipi_it::FRealObjectiveMod;

 // FRowConstraint.h
 using SMSpp_di_unipi_it::FRowConstraint;
 using SMSpp_di_unipi_it::FRowConstraintMod;

 // FakeSolver.h
 using SMSpp_di_unipi_it::FakeSolver;

 // Function.h
 using SMSpp_di_unipi_it::Function;
 using SMSpp_di_unipi_it::FunctionMod;
 using SMSpp_di_unipi_it::FunctionModVars;
 using SMSpp_di_unipi_it::FunctionModVarsAddd;
 using SMSpp_di_unipi_it::FunctionModVarsRngd;
 using SMSpp_di_unipi_it::FunctionModVarsSbst;

 // GlobalInformation.h
 using SMSpp_di_unipi_it::Collection;
 using SMSpp_di_unipi_it::CollectionBase;
 using SMSpp_di_unipi_it::CollectionMap;
 using SMSpp_di_unipi_it::GlobalInformation;

 // Group.h
 using SMSpp_di_unipi_it::BaseGroup;
 using SMSpp_di_unipi_it::CellGroup;
 using SMSpp_di_unipi_it::DynamicGroup;
 using SMSpp_di_unipi_it::JaggedGroup;
 using SMSpp_di_unipi_it::StaticGroup;
 using SMSpp_di_unipi_it::Vec_Group;
 using SMSpp_di_unipi_it::check_group_form;
 using SMSpp_di_unipi_it::for_each_as_any_of;
 using SMSpp_di_unipi_it::group_element;
 using SMSpp_di_unipi_it::group_for;
 using SMSpp_di_unipi_it::group_form;
 using SMSpp_di_unipi_it::group_form_multi_array;
 using SMSpp_di_unipi_it::make_group;

 // LagBFunction.h
 using SMSpp_di_unipi_it::LagBFunction;
 using SMSpp_di_unipi_it::LagBFunctionMod;
 using SMSpp_di_unipi_it::LagBFunctionState;

 // LinearConstraint.h
 using SMSpp_di_unipi_it::LinearConstraint;

 // LinearFunction.h
 using SMSpp_di_unipi_it::LinearFunction;
 using SMSpp_di_unipi_it::LinearFunctionModVarsAddd;

 // MasterProblemBlock.h
 using SMSpp_di_unipi_it::MasterProblemBlock;
 using SMSpp_di_unipi_it::MasterProblemLowerBoundMod;
 using SMSpp_di_unipi_it::MasterProblemMod;
 using SMSpp_di_unipi_it::MasterProblemParamMod;
 using SMSpp_di_unipi_it::MasterProblemRngdMod;
 using SMSpp_di_unipi_it::MasterProblemSbstMod;

 // Modification.h
 using SMSpp_di_unipi_it::AModification;
 using SMSpp_di_unipi_it::GroupModification;
 using SMSpp_di_unipi_it::Lst_p_Mod;
 using SMSpp_di_unipi_it::Lst_sp_Mod;
 using SMSpp_di_unipi_it::ModParam;
 using SMSpp_di_unipi_it::Modification;
 using SMSpp_di_unipi_it::NBModification;
 using SMSpp_di_unipi_it::NModification;
 using SMSpp_di_unipi_it::VariableGroupMod;
 using SMSpp_di_unipi_it::Vec_p_Mod;
 using SMSpp_di_unipi_it::Vec_sp_Mod;
 using SMSpp_di_unipi_it::amododification_type;
 using SMSpp_di_unipi_it::c_ModParam;
 using SMSpp_di_unipi_it::c_p_Mod;
 using SMSpp_di_unipi_it::eDryRun;
 using SMSpp_di_unipi_it::eModBlck;
 using SMSpp_di_unipi_it::eNoBlck;
 using SMSpp_di_unipi_it::eNoMod;
 using SMSpp_di_unipi_it::p_Mod;
 using SMSpp_di_unipi_it::sp_Mod;

 // Objective.h
 using SMSpp_di_unipi_it::Objective;
 using SMSpp_di_unipi_it::ObjectiveMod;
 using SMSpp_di_unipi_it::RealObjective;
 using SMSpp_di_unipi_it::p_ObjF;

 // Observer.h
 using SMSpp_di_unipi_it::Observer;

 // OneVarConstraint.h
 using SMSpp_di_unipi_it::BoxConstraint;
 using SMSpp_di_unipi_it::LB0Constraint;
 using SMSpp_di_unipi_it::LBConstraint;
 using SMSpp_di_unipi_it::NNConstraint;
 using SMSpp_di_unipi_it::NPConstraint;
 using SMSpp_di_unipi_it::OneVarConstraint;
 using SMSpp_di_unipi_it::OneVarConstraintMod;
 using SMSpp_di_unipi_it::UB0Constraint;
 using SMSpp_di_unipi_it::UBConstraint;
 using SMSpp_di_unipi_it::ZOConstraint;

 // PolyhedralFunction.h
 using SMSpp_di_unipi_it::PolyhedralFunction;
 using SMSpp_di_unipi_it::PolyhedralFunctionMod;
 using SMSpp_di_unipi_it::PolyhedralFunctionModAddd;
 using SMSpp_di_unipi_it::PolyhedralFunctionModRngd;
 using SMSpp_di_unipi_it::PolyhedralFunctionModSbst;
 using SMSpp_di_unipi_it::PolyhedralFunctionState;

 // PolyhedralFunctionBlock.h
 using SMSpp_di_unipi_it::PolyhedralFunctionBlock;

 // QuadFunction.h
 using SMSpp_di_unipi_it::QuadFunction;
 using SMSpp_di_unipi_it::QuadFunctionModSbst;
 using SMSpp_di_unipi_it::QuadFunctionModVarsAddd;

 // RBlockConfig.h
 using SMSpp_di_unipi_it::CBlockConfig;
 using SMSpp_di_unipi_it::CRBlockConfig;
 using SMSpp_di_unipi_it::MetaBlockConfig;
 using SMSpp_di_unipi_it::OBlockConfig;
 using SMSpp_di_unipi_it::OCBlockConfig;
 using SMSpp_di_unipi_it::OCRBlockConfig;
 using SMSpp_di_unipi_it::ORBlockConfig;
 using SMSpp_di_unipi_it::RBlockConfig;

 // RowConstraint.h
 using SMSpp_di_unipi_it::RowConstraint;
 using SMSpp_di_unipi_it::RowConstraintMod;

 // RowConstraintSolution.h
 using SMSpp_di_unipi_it::RowConstraintSolution;
 using SMSpp_di_unipi_it::Vec_RowConstraintSolution;
 using SMSpp_di_unipi_it::c_Vec_RowConstraintSolution;

 // SMSFactory.h
 using SMSpp_di_unipi_it::SMSpp_classname_normalise;

 // SMSStreams.h
 using SMSpp_di_unipi_it::eatcomments;
 using SMSpp_di_unipi_it::is_std_pair;
 using SMSpp_di_unipi_it::operator<<;
 using SMSpp_di_unipi_it::operator>>;
 using SMSpp_di_unipi_it::out_el;

 // SMSTypedefs.h
 using SMSpp_di_unipi_it::Inf;
 using SMSpp_di_unipi_it::KD_Vec_List_p_Const;
 using SMSpp_di_unipi_it::KD_Vec_List_p_Var;
 using SMSpp_di_unipi_it::KD_Vec_p_Const;
 using SMSpp_di_unipi_it::KD_Vec_p_Var;
 using SMSpp_di_unipi_it::KD_c_Vec_List_p_Const;
 using SMSpp_di_unipi_it::KD_c_Vec_List_p_Var;
 using SMSpp_di_unipi_it::KD_c_Vec_p_Const;
 using SMSpp_di_unipi_it::KD_c_Vec_p_Var;
 using SMSpp_di_unipi_it::List_p_Const;
 using SMSpp_di_unipi_it::List_p_Const_it;
 using SMSpp_di_unipi_it::List_p_Var;
 using SMSpp_di_unipi_it::List_p_Var_it;
 using SMSpp_di_unipi_it::Vec_List_p_Const;
 using SMSpp_di_unipi_it::Vec_List_p_Var;
 using SMSpp_di_unipi_it::Vec_p_Const;
 using SMSpp_di_unipi_it::Vec_p_Var;
 using SMSpp_di_unipi_it::Vec_string;
 using SMSpp_di_unipi_it::Vec_string_it;
 using SMSpp_di_unipi_it::c_List_p_Const;
 using SMSpp_di_unipi_it::c_List_p_Const_it;
 using SMSpp_di_unipi_it::c_List_p_Var;
 using SMSpp_di_unipi_it::c_List_p_Var_it;
 using SMSpp_di_unipi_it::c_Vec_List_p_Const;
 using SMSpp_di_unipi_it::c_Vec_List_p_Var;
 using SMSpp_di_unipi_it::c_Vec_p_Const;
 using SMSpp_di_unipi_it::c_Vec_p_Var;
 using SMSpp_di_unipi_it::c_Vec_string;
 using SMSpp_di_unipi_it::c_Vec_string_it;
 using SMSpp_di_unipi_it::c_p_Const;
 using SMSpp_di_unipi_it::eBlockFile;
 using SMSpp_di_unipi_it::eConfigFile;
 using SMSpp_di_unipi_it::eLastFileParam;
 using SMSpp_di_unipi_it::eProbFile;
 using SMSpp_di_unipi_it::eSolutionFile;
 using SMSpp_di_unipi_it::eStateFile;
 using SMSpp_di_unipi_it::p_Const;
 using SMSpp_di_unipi_it::p_Var;
 using SMSpp_di_unipi_it::smspp_netCDF_file_type;

 // SMSnetCDF.h
 using SMSpp_di_unipi_it::NcMemFile;
 using SMSpp_di_unipi_it::_nc_read_type;
 using SMSpp_di_unipi_it::check_dimensions;
 using SMSpp_di_unipi_it::check_variables;
 using SMSpp_di_unipi_it::deserialize;
 using SMSpp_di_unipi_it::deserialize_dim;
 using SMSpp_di_unipi_it::get_sizes_dimensions;
 using SMSpp_di_unipi_it::get_var_values;
 using SMSpp_di_unipi_it::is_netCDF_type_v;
 using SMSpp_di_unipi_it::read_type_t;
 using SMSpp_di_unipi_it::serialize;
 using SMSpp_di_unipi_it::typ2nCDF;

 // Solution.h
 using SMSpp_di_unipi_it::Solution;
 using SMSpp_di_unipi_it::Vec_Solution;
 using SMSpp_di_unipi_it::p_Solution;

 // Solver.h
 using SMSpp_di_unipi_it::Lst_Solver;
 using SMSpp_di_unipi_it::Lst_Solver_it;
 using SMSpp_di_unipi_it::Solver;
 using SMSpp_di_unipi_it::Vec_Solver;
 using SMSpp_di_unipi_it::Vec_Solver_it;
 using SMSpp_di_unipi_it::c_Lst_Solver;
 using SMSpp_di_unipi_it::c_Lst_Solver_it;
 using SMSpp_di_unipi_it::c_Vec_Solver;
 using SMSpp_di_unipi_it::c_Vec_Solver_it;
 using SMSpp_di_unipi_it::p_Solver;

 // ThinComputeInterface.h
 using SMSpp_di_unipi_it::ComputeConfig;
 using SMSpp_di_unipi_it::State;
 using SMSpp_di_unipi_it::ThinComputeInterface;

 // ThinVarDepInterface.h
 using SMSpp_di_unipi_it::ThinVarDepInterface;

 // UpdateSolver.h
 using SMSpp_di_unipi_it::UpdateSolver;

 // Variable.h
 using SMSpp_di_unipi_it::Variable;
 using SMSpp_di_unipi_it::VariableMod;
}

export namespace SMSpp_di_unipi_it {
 namespace BlockConfigHandlers {

  // RBlockConfig.h
  using SMSpp_di_unipi_it::BlockConfigHandlers::CHandler;
  using SMSpp_di_unipi_it::BlockConfigHandlers::OHandler;
  using SMSpp_di_unipi_it::BlockConfigHandlers::RHandler;
 }
}

export namespace SMSpp_di_unipi_it {
 namespace inspection {

  // BlockInspection.h
  using SMSpp_di_unipi_it::inspection::Index;
  using SMSpp_di_unipi_it::inspection::describe_groups;
  using SMSpp_di_unipi_it::inspection::element_at;
  using SMSpp_di_unipi_it::inspection::fill_ComputeConfig_Constraint;
  using SMSpp_di_unipi_it::inspection::for_each_concrete;
  using SMSpp_di_unipi_it::inspection::for_each_named_as;
  using SMSpp_di_unipi_it::inspection::for_each_named_as_any_of;
  using SMSpp_di_unipi_it::inspection::get_Constraint;
  using SMSpp_di_unipi_it::inspection::get_block_index;
  using SMSpp_di_unipi_it::inspection::get_element;
  using SMSpp_di_unipi_it::inspection::get_element_index;
  using SMSpp_di_unipi_it::inspection::get_element_size;
  using SMSpp_di_unipi_it::inspection::get_groups;
  using SMSpp_di_unipi_it::inspection::index_in_group;
  using SMSpp_di_unipi_it::inspection::name_format;
  using SMSpp_di_unipi_it::inspection::name_in_group;
  using SMSpp_di_unipi_it::inspection::name_of;
  using SMSpp_di_unipi_it::inspection::name_of_cell;
 }
}

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SMSpp.cppm ---------------------------*/
/*--------------------------------------------------------------------------*/

// -----------------------------------------------------------------------------
// Copyright (C) 2017  Ludovic LE FRIOUX
//
// This file is part of PaInleSS.
//
// PaInleSS is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
// -----------------------------------------------------------------------------

// KissatBonus includes
extern "C" {
   #include "src/application.h"
   #include "src/parse.h"
   #include "src/internal.h"
   #include "src/witness.h"
   #include "src/import.h"
}
#include "../utils/Logger.h"
#include "../utils/System.h"
#include "../utils/Parameters.h"
#include "../clauses/ClauseManager.h"
#include "../solvers/KissatBonus.h"


// Macros for minisat literal representation conversion
#define MIDX(LIT) (((unsigned)(LIT)) >> 1)
#define MNEGATED(LIT) (((LIT) & 1u))
#define MNOT(LIT) (((LIT) ^ 1u))



void cbkKissatBonusExportClause(void * issuer, int lbd, cvec *cls)
{
	KissatBonusSolver* mp = (KissatBonusSolver*)issuer;

	// if (lbd > mp->lbdLimit || cls->sz > 2)
	if (lbd > mp->lbdLimit)
      return;

	ClauseExchange * ncls = ClauseManager::allocClause(cls->sz);

	for (int i = 0; i < cls->sz; i++) {
      int v = cvec_data(cls, i);
      int iidx = MIDX(v);
      int sign = MNEGATED(v); //sign = 1 -; 0 +
      if (v & 1) assert(sign == 1);
      else assert(sign == 0);
      int eidx = PEEK_STACK(mp->solver->exportk, iidx);
      assert(iidx == eidx - 1);
		ncls->lits[i] = sign == 1 ? -eidx : eidx;      
      // mp->outputf << ncls->lits[i] << " ";
	}
   // mp->outputf << "0" << endl;
   ncls->lbd  = lbd;
   ncls->from = mp->id;

   mp->clausesToExport.addClause(ncls);
}

int cbkKissatBonusImportUnit(void * issuer)
{
   KissatBonusSolver * mp = (KissatBonusSolver*)issuer;

   int l = -1;

   ClauseExchange * cls = NULL;

   if (mp->unitsToImport.getClause(&cls) == false)
      return l;

   int eidx = abs(cls->lits[0]);
   import *import = &PEEK_STACK (mp->solver->import, eidx);
   if (import->eliminated) {
      l = -10;   
   }
   else {
      assert(import->imported);
      l = import->lit;
      if (cls->lits[0] < 0) l = MNOT(l);
   }
   ClauseManager::releaseClause(cls);

   return l;
}

int cbkKissatBonusImportClause(void * issuer, int * lbd, cvec *mcls)
{
   KissatBonusSolver* mp = (KissatBonusSolver*)issuer;

   ClauseExchange * cls = NULL;

   if (mp->clausesToImport.getClause(&cls) == false)
      return -1;
   assert(mcls->sz==0);
   bool alreadySat = false;
   for (size_t i = 0; i < cls->size; i++) {
      int eidx = abs(cls->lits[i]);
      import *import = &PEEK_STACK (mp->solver->import, eidx);
      if (import->eliminated) {
         alreadySat = true;
      }
      else {
         assert(import->imported);
         int ilit = import->lit;
         assert(ilit == 2 * eidx - 2);
         if (cls->lits[i] < 0) ilit = MNOT(ilit);
         cvec_push(mcls, ilit);
      }
   }

   *lbd = cls->lbd;

   ClauseManager::releaseClause(cls);

   if (alreadySat) 
      return -10;

   return 1;
}

KissatBonusSolver::KissatBonusSolver(int id) : SolverInterface(id, MAPLE)
{
	lbdLimit = Parameters::getIntParam("lbd-limit", 2);

	solver = kissat_init();
   // outputf.open((to_string(id) + ".txt").c_str());
	solver->cbkExportClause = cbkKissatBonusExportClause;
	solver->cbkImportClause = cbkKissatBonusImportClause;
	solver->cbkImportUnit   = cbkKissatBonusImportUnit;
	solver->issuer          = this;
}

KissatBonusSolver::~KissatBonusSolver()
{
	delete solver;
}

void 
KissatBonusSolver::addOriginClauses(simplify *S) {
   solver->max_var = S->vars;
   kissat_mab_parse(solver);
   kissat_reserve(solver, S->vars);
   for (int i = 1; i <= S->clauses; i++) {
      int l = S->clause[i].size();
      for (int j = 0; j < l; j++)
         kissat_add(solver, S->clause[i][j]);
      kissat_add(solver, 0);
   }
}

bool
KissatBonusSolver::loadFormula(const char* filename)
{
   kissat_mab_parse(solver);
   strictness strict = NORMAL_PARSING;
   file in;
   uint64_t lineno;
   kissat_open_to_read_file(&in, filename);
   kissat_parse_dimacs(solver, strict, &in, &lineno, &solver->max_var);
   kissat_close_file(&in);
   // print_options(solver);
   return true;
}

//Get the number of variables of the formula
int
KissatBonusSolver::getVariablesCount()
{
	return solver->vars;
}

// Get a variable suitable for search splitting
int
KissatBonusSolver::getDivisionVariable()
{
   return (rand() % getVariablesCount()) + 1;
}

// Set initial phase for a given variable
void
KissatBonusSolver::setPhase(const int var, const bool phase)
{
   int idx = MIDX(kissat_import_literal(solver, var));
   solver->init_phase[idx] = phase ? 1 : -1;}

// Bump activity for a given variable
void
KissatBonusSolver::bumpVariableActivity(const int var, const int times)
{
}

// Interrupt the SAT solving, so it can be started again with new assumptions
void
KissatBonusSolver::setSolverInterrupt()
{
   stopSolver = true;

   kissat_terminate(solver);
}

void
KissatBonusSolver::unsetSolverInterrupt()
{
   stopSolver = false;
}

void
KissatBonusSolver::setBumpVar(int v) {
   bump_var = v;
}

// Diversify the solver
void
KissatBonusSolver::diversify(int id)
{
   if (Parameters::getBoolParam("bump")) {
      if (id && solver->initshuffle == -1) solver->bump_one=bump_var;
   }
   // if (id == ID_XOR) {
   //    solver->GE = true;
   // } else {
   //    solver->GE = false;
   // }

   // if (id % 2) {
   //    solver->VSIDS = false;
   // } else {
   //    solver->VSIDS = true;
   // }
   if (Parameters::getBoolParam("verso")) {
      if (id % 2 == 0) {
         solver->verso = 0;
      } else {
         solver->verso = 1;
      }
   }
}

void
KissatBonusSolver::initshuffle(int id)
{
   if (id) solver->initshuffle = id;
   // if (id) solver->bump_one=rand()%solver->max_var+1;
   // if (id == ID_XOR) {
   //    solver->GE = true;
   // } else {
   //    solver->GE = false;
   // }

   // if (id % 2) {
   //    solver->VSIDS = false;
   // } else {
   //    solver->VSIDS = true;
   // }

   // if (id % 4 >= 2) {
   //    solver->verso = false;
   // } else {
   //    solver->verso = true;
   // }
}

void 
KissatBonusSolver::setParameter(parameter p)
{

   kissat_set_option (solver, "tier1", p.tier1);
   kissat_set_option (solver, "chrono", p.chrono);
   kissat_set_option (solver, "stable", p.stable);
   kissat_set_option (solver, "walkinitially", p.walkinitially);
   kissat_set_option (solver, "target", p.target);
   kissat_set_option (solver, "phase", p.phase);
   kissat_set_option (solver, "targetinc", p.targetinc);
   kissat_set_option (solver, "ccanr", p.ccanr);
   if (p.ccanr == 1) {
      kissat_set_option (solver, "rephaseint", 80);
      kissat_set_option (solver, "rephaseinit", 80);
   }
   
   // printf("c\t\t id %d: chrono=%d stable=%d target=%d phase=%d targetinc=%d ccanr=%d initshuffle=%d\n",
   // id,
   // kissat_get_option(solver, "chrono"),
   // kissat_get_option(solver, "stable"),
   // kissat_get_option(solver, "target"),
   // kissat_get_option(solver, "phase"),
   // kissat_get_option(solver, "targetinc"),
   // kissat_get_option(solver, "ccanr"),
   // solver->initshuffle
   // );
   
}
// Solve the formula with a given set of assumptions
// return 10 for SAT, 20 for UNSAT, 0 for UNKNOWN
SatResult
KissatBonusSolver::solve(const std::vector<int> & cube)
{
   unsetSolverInterrupt();

   std::vector<ClauseExchange *> tmp;

   tmp.clear();
   clausesToAdd.getClauses(tmp);
   for (size_t ind = 0; ind < tmp.size(); ind++) {
      for (size_t i = 0; i < tmp[ind]->size; i++)
         kissat_add(solver, tmp[ind]->lits[i]);
      kissat_add(solver, 0);
      ClauseManager::releaseClause(tmp[ind]);
   }
   assert(cube.size() == 0);
   int res = kissat_solve(solver);
   if (res == 10)
      return SAT;
   if (res == 20)
      return UNSAT;
   return UNKNOWN;
}


void
KissatBonusSolver::addClause(ClauseExchange * clause)
{
}

void
KissatBonusSolver::addLearnedClause(ClauseExchange * clause)
{
   if (clause->size == 1) {
      unitsToImport.addClause(clause);
   } else {
      clausesToImport.addClause(clause);
   }
}

void
KissatBonusSolver::addClauses(const std::vector<ClauseExchange *> & clauses)
{
}


void
KissatBonusSolver::addInitialClauses(const std::vector<ClauseExchange *> & clauses)
{
}

void
KissatBonusSolver::addLearnedClauses(const std::vector<ClauseExchange *> & clauses)
{
   for (size_t i = 0; i < clauses.size(); i++) {
      addLearnedClause(clauses[i]);
   }
}

void
KissatBonusSolver::getLearnedClauses(std::vector<ClauseExchange *> & clauses)
{
   clausesToExport.getClauses(clauses);
}

void
KissatBonusSolver::increaseClauseProduction()
{
   //lbdLimit++;
}

void
KissatBonusSolver::decreaseClauseProduction()
{
   if (lbdLimit > 2) {
      lbdLimit--;
   }
}

SolvingStatistics
KissatBonusSolver::getStatistics()
{
   SolvingStatistics stats;

   stats.conflicts    = solver->statistics.conflicts;
   stats.propagations = solver->statistics.propagations;
   stats.restarts     = solver->statistics.restarts;
   stats.decisions    = solver->statistics.decisions;
   stats.memPeak      = 0;

   return stats;
}

std::vector<int>
KissatBonusSolver::getModel()
{
   std::vector<int> model;

   for (int i = 1; i <= solver->max_var; i++) {
      int tmp = kissat_value(solver, i);
      if (!tmp) tmp = i;
      model.push_back(tmp);
   }

   return model;
}


std::vector<int>
KissatBonusSolver::getFinalAnalysis()
{
   std::vector<int> outCls;
   return outCls;
}


std::vector<int>
KissatBonusSolver::getSatAssumptions() {
   std::vector<int> outCls;
   return outCls;
};

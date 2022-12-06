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

#include "../solvers/KissatBonus.h"
#include "../solvers/SolverFactory.h"
#include "../utils/Parameters.h"
#include "../utils/System.h"

void
SolverFactory::sparseRandomDiversification(
      const std::vector<SolverInterface *> & solvers)
{
   if (solvers.size() == 0)
      return;

   int vars = solvers[0]->getVariablesCount();

   // The first solver of the group (1 LRB/1 VSIDS) keeps polarity = false for all vars
   for (int sid = 1; sid < solvers.size(); sid++) {
      //srand(sid);
      for (int var = 1; var <= vars; var++) {
         if (rand() % solvers.size() == 0) {
            solvers[sid]->setPhase(var, rand() % 2 == 1);
         }
      }
   }
}

void
SolverFactory::nativeDiversification(const std::vector<SolverInterface *> & solvers)
{
   for (int sid = 0; sid < solvers.size(); sid++) {
      solvers[sid]->diversify(sid);
   }
}

void
SolverFactory::initshuffleDiversification(const std::vector<SolverInterface *> & solvers)
{
   for (int sid = 0; sid < solvers.size(); sid++) {
      solvers[sid]->initshuffle(sid);
   }
}

void
SolverFactory::parameterDiversification(const std::vector<SolverInterface *> & solvers)
{
   for (int sid = 0; sid < solvers.size(); sid++) {
      parameter p;
      p.tier1 = 2;
      p.chrono = 1;
      p.stable = 1;
      p.walkinitially = 0;
      p.target = 2;
      p.phase = 1;
      p.heuristic = 0;
      p.margin = 10;
      p.ccanr = 0;
      p.targetinc = 0;

      if (sid == 2)
         p.stable = 0; 
      else if (sid == 6)
         p.stable = 2; 

      if (sid == 7 || sid == 8)
         p.target = 0;
      else if (sid == 0 || sid == 2 || sid == 3 || sid == 4 || sid == 5 || sid == 6 || sid == 10 || sid == 23)
         p.target = 1;
      
      if (sid == 4 || sid == 5 || sid == 8 || sid == 9 || sid == 12 || sid == 13 || sid == 15 || sid == 18 || sid == 19)
         p.phase = 0;
      if (sid == 30)
         p.ccanr = 1;
      solvers[sid]->setParameter(p);
   }
}


SolverInterface *
SolverFactory::createKissatBonusSolver()
{
   int id = currentIdSolver.fetch_add(1);
   SolverInterface * solver = new KissatBonusSolver(id);

   //solver->loadFormula(Parameters::getFilename());

   return solver;
}


void
SolverFactory::createKissatBonusSolvers(int maxSolvers,
                                        std::vector<SolverInterface *> & solvers)
{
   for (size_t i = 0; i < maxSolvers; i++) {
      KissatBonusSolver* kissat = (KissatBonusSolver*) createKissatBonusSolver();
      solvers.push_back(kissat);
   }
}


void
SolverFactory::printStats(const std::vector<SolverInterface *> & solvers)
{
   printf("c | ID | conflicts  | propagations |  restarts  | decisions  " \
          "| memPeak |\n");

   for (size_t i = 0; i < solvers.size(); i++) {
      SolvingStatistics stats = solvers[i]->getStatistics();

      printf("c | %2zu | %10ld | %12ld | %10ld | %10ld | %7d |\n",
             solvers[i]->id, stats.conflicts, stats.propagations,
             stats.restarts, stats.decisions, (int)stats.memPeak);
   }
}

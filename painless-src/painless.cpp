// -----------------------------------------------------------------------------
// Copyright (C) 2017  Ludovic LE FRIOUX
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

#include "painless.h"

#include "utils/Logger.h"
#include "utils/Parameters.h"
#include "utils/System.h"
#include "utils/SatUtils.h"

#include "solvers/SolverFactory.h"

#include "clauses/ClauseManager.h"

#include "sharing/HordeSatSharing.h"
#include "sharing/Sharer.h"

#include "working/SequentialWorker.h"
#include "working/Portfolio.h"
#include "simplify/simplify.h"

#include <unistd.h>

// -------------------------------------------
// Declaration of global variables
// -------------------------------------------
atomic<bool> globalEnding(false);

Sharer ** sharers = NULL;

int nSharers = 0;

WorkingStrategy * working = NULL;

SatResult finalResult = UNKNOWN;

std::vector<int> finalModel;
simplify *S;

void * readWorker(void *arg)
{
   SolverInterface * sq = (SolverInterface *)arg;
   if (Parameters::getBoolParam("simp"))
      sq->addOriginClauses(S);
   else 
      sq->loadFormula(Parameters::getFilename());
   return NULL;
}

// -------------------------------------------
// Main of the framework
// -------------------------------------------
int main(int argc, char ** argv)
{
   Parameters::init(argc, argv);

   if (Parameters::getFilename() == NULL ||
       Parameters::getBoolParam("h"))
   {
      cout << "USAGE: " << argv[0] << " [options] input.cnf" << endl;
      cout << "Options:" << endl;
      cout << "\t-c=<INT>\t\t number of cpus, default is 24" << endl;
      cout << "\t-max-memory=<INT>\t memory limit in GB, default is 200" << \
	      endl;
      cout << "\t-t=<INT>\t\t timeout in seconds, default is no limit" << endl;
      cout << "\t-lbd-limit=<INT>\t LBD limit of exported clauses, default is" \
	      " 2" << endl;
      cout << "\t-shr-sleep=<INT>\t time in useconds a sharer sleep each " \
         "round, default is 500000 (0.5s)" << endl;
      cout << "\t-shr-lit=<INT>\t\t number of literals shared per round, " \
         "default is 1500" << endl;
      cout << "\t-v=<INT>\t\t verbosity level, default is 0" << endl;
      cout << "\t-simp\t\t simplify" << endl;
      cout << "\t-initshuffle\t\t initshuffle" << endl;
      return 0;
   }

   // Parameters::printParams();
   // printf("c file: %s\n", Parameters::getFilename());
   int cpus = Parameters::getIntParam("c", 30);
   setVerbosityLevel(Parameters::getIntParam("v", 0));


   // Create and init solvers
   std::vector<SolverInterface *> solvers;
   std::vector<SolverInterface *> solvers_VSIDS;
   std::vector<SolverInterface *> solvers_LRB;

   SolverFactory::createKissatBonusSolvers(cpus, solvers);
   if (Parameters::getBoolParam("initshuffle")) {
      SolverFactory::initshuffleDiversification(solvers);
   }
   SolverFactory::parameterDiversification(solvers);
   int nSolvers = solvers.size();
   
   if (Parameters::getBoolParam("simp")) {
      S = new simplify();
      S->readfile(Parameters::getFilename());
      // printf("c finish read, var: %d, clause: %d, use: %.2lf s\n", S->vars, S->clauses, getRelativeTime());
      S->simplify_init();
      int res = S->simplify_easy_clause();
      if (!res) {
         S->release();
         delete []S->mapto;
         delete []S->mapval;
         S->clause.clear(true);
         delete S;
         cout << "s UNSATISFIABLE" << endl;
         return 0;
      }
      // if (S->clauses <= 15000000) {
      res = S->simplify_resolution();
      if (!res) {
         S->release();
         delete []S->mapto;
         delete []S->mapval;
         S->clause.clear(true);
         S->res_clause.clear(true);
         S->resolution.clear(true);
         delete S;
         cout << "s UNSATISFIABLE" << endl;
         return 0;
      }
      // }
      if (S->clauses <= 15000000) {
         res = S->simplify_binary();
         if (!res) {
            S->release();
            delete []S->mapto;
            delete []S->mapval;
            S->clause.clear(true);
            S->res_clause.clear(true);
            S->resolution.clear(true);
            delete S;
            cout << "s UNSATISFIABLE" << endl;
            return 0;
         }
      }
      // printf("c Finish Simplify, use %.2lf seconds\n", getRelativeTime());
      srand(S->vars);
      S->release();
   }
   
   pthread_t *ptr;
   ptr = new pthread_t[nSolvers];
   for (int i = 0; i < nSolvers; i++) {
      pthread_create(&ptr[i], NULL, readWorker, solvers[i]);
   }   
   for (int i = 0; i < nSolvers; i++) {
      pthread_join(ptr[i], NULL);
   }    
   delete []ptr;
   
   if (Parameters::getBoolParam("simp")) 
      S->clause.clear(true);

   // printf("c finish solver read, use %.2lf seconds\n", getRelativeTime());
   SolverFactory::nativeDiversification(solvers);
   // Init Sharing
   // 15 CDCL, 1 Reducer producers by Sharer
   std::vector<SolverInterface* > prod1;
   std::vector<SolverInterface* > prod2;
   std::vector<SolverInterface *> reducerCons1;
   std::vector<SolverInterface *> reducerCons2;
   std::vector<SolverInterface* > cons1;
   std::vector<SolverInterface* > cons2;
   std::vector<SolverInterface*> consCDCL;

   switch (Parameters::getIntParam("shr-strat", 1))
   {
   case 1:
      // prod1.insert(prod1.end(), solvers.begin(), solvers.begin() + (cpus/2 - 1));
      // prod1.push_back(solvers[solvers.size() - 2]);
      // prod2.insert(prod2.end(), solvers.begin() + (cpus/2 - 1), solvers.end() - 2);
      // prod2.push_back(solvers[solvers.size() - 1]);
      // // 30 CDCL, 1 Reducer consumers by Sharer
      // cons1.insert(cons1.end(), solvers.begin(), solvers.end() - 1);
      // cons2.insert(cons2.end(), solvers.begin(), solvers.end() - 2);
      // cons2.push_back(solvers[solvers.size() - 1]);
      
      // prod1.insert(prod1.end(), solvers.begin(), solvers.begin() + cpus/2);
      // prod2.insert(prod2.end(), solvers.begin() + cpus/2, solvers.end());
      // cons1.insert(cons1.end(), solvers.begin(), solvers.end());
      // cons2.insert(cons2.end(), solvers.begin(), solvers.end());
      // nSharers = 2;
      // sharers  = new Sharer*[nSharers];
      // sharers[0] = new Sharer(1, new HordeSatSharing(), prod1, cons1);
      // sharers[1] = new Sharer(2, new HordeSatSharing(), prod2, cons2);
      
      prod1.insert(prod1.end(), solvers.begin(), solvers.end());
      cons1.insert(cons1.end(), solvers.begin(), solvers.end());
      nSharers = 1;
      sharers  = new Sharer*[nSharers];
      sharers[0] = new Sharer(1, new HordeSatSharing(), prod1, cons1);
      break;
   case 2:
      prod1.insert(prod1.end(), solvers.begin(), solvers.begin() + (cpus/2 - 1));
      prod2.insert(prod2.end(), solvers.begin() + (cpus/2 - 1), solvers.end() - 2);
      reducerCons1.push_back(solvers[solvers.size() - 2]);
      reducerCons2.push_back(solvers[solvers.size() - 1]);

      cons1.insert(cons1.end(), prod1.begin(), prod1.end());
      cons1.push_back(solvers[solvers.size() - 2]);
      cons2.insert(cons2.end(), prod2.begin(), prod2.end());
      cons2.push_back(solvers[solvers.size() - 1]);
      consCDCL.insert(consCDCL.end(), prod1.begin(), prod1.end());
      consCDCL.insert(consCDCL.end(), prod2.begin(), prod2.end());

      nSharers = 4;
      sharers  = new Sharer*[nSharers];
      sharers[0] = new Sharer(1, new HordeSatSharing(), prod1, cons1);
      sharers[1] = new Sharer(2, new HordeSatSharing(), prod2, cons2);
      sharers[2] = new Sharer(3, new HordeSatSharing(), reducerCons1, consCDCL);
      sharers[3] = new Sharer(4, new HordeSatSharing(), reducerCons2, consCDCL);
      break;
   default:
      break;
   }

   // Init working
   working = new Portfolio();
   for (size_t i = 0; i < nSolvers; i++) {
      working->addSlave(new SequentialWorker(solvers[i]));
   }


   // Init the management of clauses
   ClauseManager::initClauseManager();


   // Launch working
   std::vector<int> cube;
   working->solve(cube);


   // Wait until end or timeout
   int timeout = Parameters::getIntParam("t", -1);

   while(globalEnding == false) {
      sleep(1);

      if (timeout > 0 && getRelativeTime() >= timeout) {
         globalEnding = true;
         working->setInterrupt();
      }
   }


   // Delete sharers
   // for (int id = 0; id < nSharers; id++) {
   //    sharers[id]->printStats();
   //    delete sharers[id];
   // }
   // delete sharers;


   // Print solver stats
   // SolverFactory::printStats(solvers);


   // Delete working strategy
   // delete working;


   // Delete shared clauses
   ClauseManager::joinClauseManager();


   // Print the result and the model if SAT
   // cout << "c Resolution time: " << getRelativeTime() << "s" << endl;

   if (finalResult == SAT) {
      cout << "s SATISFIABLE" << endl;
      if (Parameters::getBoolParam("no-model") == false) {
         if (Parameters::getBoolParam("simp")) {
            for (int i = 1; i <= S->orivars; i++)
               if (S->mapto[i]) S->mapval[i] = (finalModel[abs(S->mapto[i])-1] > 0 ? 1 : -1) * (S->mapto[i] > 0 ? 1 : -1);
            S->print_complete_model();
            finalModel.clear();
            for (int i = 1; i <= S->orivars; i++) {
               finalModel.push_back(i * S->mapval[i]);
            }
            printModel(finalModel);
         }
         else
            printModel(finalModel);
      }
   } else if (finalResult == UNSAT) {
      cout << "s UNSATISFIABLE" << endl;
   } else {
      cout << "s UNKNOWN" << endl;
   }

   return 0;
}

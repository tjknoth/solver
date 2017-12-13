#ifndef CDCL_H
#define CDCL_H

#include <vector>
#include "solver.h"
#include "dpll.h"

struct learnedClause {
  int level;
  Clause clause;
};

class CDCLSolver : public DPLLSolver {

  public:
    int decisionLevel;
    conflict conf;
    CDCLSolver( std::vector<Clause> form, std::vector<int> sigma, int numClauses, int numVars
        , std::vector<int> activeClauses, int decisionLevel );
    CDCLSolver();
    // decisionLimits is indexed inclusively:
    // [0, m, n] where there are m-1 elements in decision 0, etc
    std::vector< std::vector<Clause*> > reasons; // implication clauses
    std::vector<int> level; // decision level for each variable
    std::vector<bool> finalVars; // Un-reversable variables
    void recordDecision( decision d );
};

void cdclEval( CDCLSolver solver, int time, int verbose );

#endif

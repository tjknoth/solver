#ifndef DPLL_H 
#define DPLL_H

#include <vector>
#include "solver.h"
#include <stack>

struct decision {
  int var;
  int assignment;
};

class DPLLSolver : public Solver {

  public:
    std::vector<int> activeClauses;
    std::stack<decision> decisionStack;
    DPLLSolver ( std::vector<Clause> form, std::vector<int> sigma, int numClauses, int numVars
        , std::vector<int> activeClauses );
    DPLLSolver ();
    std::vector<int> getSingletons ();
};

decision unitRule ( Clause* clause, std::vector<int>* sigma, std::vector<int>* sigmaStack ); 

decision makeDecision ( DPLLSolver* solver );

void testWrapper( DPLLSolver solver );

void dpllEval ( DPLLSolver solver, int time ); 

void pickClause ( DPLLSolver* solver, Clause* c );

template<typename T>
void dump_vec(std::vector<T>* vec); 

#endif

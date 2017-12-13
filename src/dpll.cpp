#include "solver.h"
#include <vector>
#include "dpll.h"
#include <iostream>
#include <stdlib.h>
#include <stack>

using namespace std;

DPLLSolver::DPLLSolver( std::vector<Clause> f, std::vector<int> s, int numc, int numv, std::vector<int> activec )
  : Solver ( f, s, numc, numv ) {
  activeClauses = activec;

}

DPLLSolver::DPLLSolver () : Solver() {
  std::vector<int> activeClauses;
  std::stack<decision> decisionStack;
}

std::vector<int> DPLLSolver::getSingletons() {
  std::vector<int> singletons;
  int numv;
  int unassigned;
  // check each clause:
  for( int i = 0; i <  numClauses; i++ ) {
    Clause current = formula[i];
    numv = current.numVars;
    unassigned = numv;
    for ( int j = 0; j < numv; j++ ) {
      if ( sigma[current.vars[j]] != 0 ) unassigned--;
    }
    if( unassigned == 1 ) {
      singletons.push_back( i );
    }
  }
  return singletons;
}

// debugging utility
template <typename T>
void dump_stack(std::stack<T>* stack) {
  std::stack<T> temp;
  while (!stack.empty()) {
    T top = stack.top(); stack.pop();
    std::cout << top << " ";
    temp.push(top);
  }
  while (!temp.empty()) {
    T top = temp.top(); temp.pop();
    stack.push(top);
  }
}

// another debugging utility
template <typename T>
void dump_vec(std::vector<T>* vec) {
  for (int i = 0; i < (*vec).size(); i++)
    std::cout << (*vec)[i] << " ";
  std::cout << "\n";
}

void pickClause ( DPLLSolver* solver, Clause* c ) {
  while ( !(solver->activeClauses).empty() ) {
    int clauseIndex = (solver->activeClauses).back();
    //bool verb = (clauseIndex == 0);
    (solver->activeClauses).pop_back();
    *c = (solver->formula)[clauseIndex];
    if ( ( c->watchedLits.snd == 0 ) && ( solver->sigma[ abs( c->watchedLits.fst ) ] == 0 ) )
      return; //special case based on enco1ding of watched lits
    int wl1 = c->watchedLits.fst;
    int wl2 = c->watchedLits.snd;
    // TODO: this guard is super ugly 
    if ( ( ( solver->sigma[abs( wl1 ) ] * wl1 ) > 0 ) || ( ( solver->sigma[abs( wl2 ) ] * wl2 ) > 0 ) )  {
      continue; // clause is true
    }
    else if ( (solver->sigma)[ abs( wl1 ) ] == 0 ) {
      if ( (solver->sigma)[ abs( wl2 ) ] == 0 )
        continue; // not a unit clause
      else {
        if ( (*solver).moveWatchedLit(2, clauseIndex) )
          continue;
        else return;
      }
    }
    else { // sigma(c.w_1) != 0
      if ( (*solver).moveWatchedLit(1, clauseIndex) ) {
        (solver->activeClauses).push_back(clauseIndex);
        continue;
      }
      else {
        if ( ( wl2 != 0 ) && ( (solver->sigma)[ abs( wl2 ) ] == 0 ) ) {
          return; // unit clause
        }
        else {
          (solver->activeClauses).clear();
          (*c) = Clause ( clauseIndex ); //TODO: this?
          return;
        }
      }
    }
  }
}

decision unitRule ( Clause* clause, std::vector<int>* sigma, std::vector<int>* sigmaStack ) {
  int numv = clause->numVars;
  int unitLit, var, unitIndex;
  bool polarity;
  int numUnits = 0;
  decision d;
  d.var = -1;
  d.assignment = 0;
  for ( int i = 0; i < numv; i++ ) {
    var = clause->vars[i];
    polarity = clause->polarity[i];
    int value = (*sigma)[var];
    if ( ( ( (*sigma)[var] == 1 ) ^ polarity ) && ( (*sigma)[var] != 0 ) ) { // something already evaluates to true, no unit rule
      return d;
    }
    if ( (*sigma)[var] == 0 ) {
      unitIndex = i;
      numUnits++;
      unitLit = var;
    }
  }
  if ( numUnits == 1 ) {
    d.var = unitLit;
    d.assignment = (clause->polarity[unitIndex]) ? -1 : 1;
    return d;
  }
  else if ( numUnits < 1 )
    return d;
  else {
    return d;
  }
}


void unitPropagate ( DPLLSolver* solver, bool* conflict ) {
  while ( true ) {
    Clause c;
    pickClause ( solver, &c );
    if ( c.numVars == 0 ) {
      //std::cout << "NULL\n";
      return; // indicates null clause
    }
    else if ( c.conflictIndex >= 0 ) {
      (*conflict) = true;
      return;
    }
    else {
      // no conflict! apply unit rule
      decision d = unitRule( &c, &(solver->sigma), &(solver->sigmaStack) );
      // expand active clauses:
      if ( d.var <= 0 ) continue;
      (solver->sigma)[d.var] = d.assignment;
      (solver->sigmaStack).push_back( d.var );
      for ( int i = 0; i < solver->numClauses; i++ ) {
        Clause c = solver->formula[i];
        if ( c.hasLit( d.var ) ) {
          (solver->activeClauses).push_back( i );
        }
      }
    }
  }
}

// Utility function for running manual tests of the sub-algorithms
void testWrapper ( DPLLSolver solver ) {
  solver.activeClauses = solver.getSingletons();
  bool con;
  unitPropagate ( &solver, &con );
}

//  propagate more
decision makeDecision ( DPLLSolver* solver ) {
  decision d;
  for ( int i = 1; i < solver->numVars + 1; i++)  {
    if ( (solver->sigma)[i] == 0 ) {
      d.var = i;
      d.assignment = -1;
      return d;
    }
  }
  std::cout << "Error: exhausted possible decisions\n";
  return { -1, 0 };
}


void dpllEval ( DPLLSolver solver, int timeout ) {
  solver.activeClauses = solver.getSingletons();
  bool conflict;
  bool shouldOverride = false;
  time_t endTime = time( NULL ) + timeout;
  //Main loop: go until timeout
  while( time( NULL ) <= endTime ) {
    conflict = false;
    // unit propagate
    unitPropagate ( &solver, &conflict );
    shouldOverride = false;
    // If no conflicts, respond
    if ( !conflict ) {
      if ( solver.sigmaStack.size() == solver.numVars && ( solver.activeClauses.size() == 0 ) ) {
        // All assigned
        std::cout << "sat";
        std::cout << "lit  val \n";
        for ( int i = 1; i < solver.numVars + 1; i++) {
          std::cout << " " << i << "    " << ( solver.sigma[i] == 1 ) << "\n";
        }

        return;
      } else {
        // make decision
        decision d = makeDecision ( &solver );
        (solver.sigma)[d.var] = d.assignment;
        solver.sigmaStack.push_back(d.var);
        solver.decisionStack.push(d);
        // expand active clauses:
        for ( int i = 0; i < solver.numClauses; i++ ) {
          Clause c = solver.formula[i];
          if ( c.hasLit( d.var ) ) {
            solver.activeClauses.push_back( i );
          }
        }
      }
    }
    // otherwise UNSAT or backtrack
    else {
      if ( solver.decisionStack.empty() ) {
        //UNSAT
        std::cout << "unsat";
        return;
      }
      else {
        decision d = solver.decisionStack.top();
        solver.decisionStack.pop();
        int v = solver.sigmaStack.back();
        // backtrack:
        while ( v != d.var ) {
          solver.sigma[v] = 0;
          solver.sigmaStack.pop_back();
          v = solver.sigmaStack.back();
        }
        solver.sigma[d.var] = 1;
        d.assignment = 1;
        // expand active clauses:
        for ( int i = 0; i < solver.numClauses; i++ ) {
          Clause c = solver.formula[i];
          if ( c.hasLit( d.var ) )
            solver.activeClauses.push_back( i );
        }
      }
    }
  }
  if (time (NULL) > endTime)
    std::cout << "unknown";
}

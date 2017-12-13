#include "solver.h"
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <stack>

using namespace std;

// constructor
Clause::Clause(std::vector<unsigned int>* v, std::vector<bool>* isNot, int numv, int i) {
  numVars = numv;
  vars = *v;
  polarity = *isNot;
  index = i;
  if ( numVars > 1 ) {
    watchedLits.fst = ( polarity[0] ? -1 : 1 ) * vars[0];
    watchedLits.snd = ( polarity[1] ? -1 : 1 ) * vars[1];
  }
  else {
    watchedLits.fst = ( polarity[0] ? -1 : 1 ) * vars[0];
    watchedLits.snd = 0;
  }
  conflictIndex = -1;
}

Clause::Clause(std::vector<unsigned int> v, std::vector<bool> isNot, int numv, int i) 
  : Clause(&v, &isNot, numv, i)
{}

// conflict constructor
Clause::Clause( int cause  ) {
  numVars = -1;
  index = cause;
  conflictIndex = cause;
}

Clause::Clause() {
  numVars = 0;
  conflictIndex = -1;
}

bool Clause::hasLit( int lit ) {
  for( int i = 0; i < numVars; i++ ) {
    if ( vars[i] == lit ) return true;
  }
  return false;
}

bool Solver::moveWatchedLit( int index, int clause ) {
  Clause c = formula[clause];
  if ( ( index != 2 ) && ( index != 1 ) )
    return false; // invalid input
  if ( ( c.watchedLits.fst == 0 ) || ( c.watchedLits.snd == 0 ) )
    return false;
  int otherLit = index == 1 ? c.watchedLits.snd : c.watchedLits.fst;
  int current = index == 1 ? c.watchedLits.fst : c.watchedLits.snd;
  for ( int i = 0; i < c.numVars; i++ ) {
    int var = c.vars[i];
    int adjVar = var * ( c.polarity[i] ? -1 : 1 ); // holds sign information
    if ( ( adjVar == current ) || ( adjVar == otherLit ) )
      continue;
    if ( ( ( ( sigma[var] == 1 ) ^ c.polarity[i] ) && ( sigma[var] != 0 ) ) || ( sigma[var] == 0 ) ) {
      index == 1 ? c.watchedLits.fst = adjVar : c.watchedLits.snd = adjVar;
      formula[clause] = c;
      return true;
    }
  }
  formula[clause] = c;
  return false; 
}

// constructor
Solver::Solver(std::vector<Clause> f, std::vector<int> s, int numc, int numv) {
  formula = f;
  sigma = s;
  numClauses = numc;
  numVars = numv;
  numAssigned = 0; // TODO: make this correct
}

// constructor
Solver::Solver() {
  std::vector<Clause> formula;
  std::vector<int> sigma;
  std::vector<int> sigmaStack;
  numClauses = 0;
  numVars = 0;
  numAssigned = 0;
}

// clear current evaluation
void Solver::clear() {
  sigma.clear();
  numAssigned = 0;
  while( !sigmaStack.empty() ) sigmaStack.pop_back();
}

long long int generateRandomAssignment( int numVars, std::vector<int>* sigma ) {
  int val;
  long long int assignmentHash = 0;
  (*sigma).push_back(0);
  for ( int i = 0; i < numVars; i++ ) {
    val = rand() % 2;  
    (*sigma).push_back(val == 0 ? -1 : 1);
    if ( val ) assignmentHash += pow ( 2, i );
  }
  return assignmentHash;
}

// evaluate a given clause
bool evaluateClause ( Clause clause, std::vector<int> sigma ) {
  int numv = clause.numVars;
  for ( int i = 0; i < numv; i++ ) { 
    unsigned int var = clause.vars[i];
    bool isNegative = clause.polarity[i];
    bool val = sigma[var]; 
    //std::cout << "Evaluating var: " << var << " is negative: " << isNegative << " val: " << val << "\n";
    if ( isNegative ^ ( val != -1 ) ) {
      return true;
    }
  }
  return false;
}

// evaluate formula clause-by-clause
bool evaluate ( Solver* solver ) {
  std::vector<Clause> formula = solver->formula;
  std::vector<int> sigma = solver->sigma;
  int numc = solver->numClauses;
  //std::cout << "evaluating " << numc << " clauses\n";
  for ( int i = 0; i < numc; i++ ) {
    // evaluate each clause, returning as soon as one is false
    if ( !evaluateClause( formula[i], sigma ) ) {
      return false;
    }
  }
  return true; 
}




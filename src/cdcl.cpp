#include <vector>
#include <deque>
#include "dpll.h"
#include "cdcl.h"
#include "solver.h"
#include <stdlib.h>
#include <iostream>
#include <iterator>
#include <algorithm>


// CDCL Class implementation
CDCLSolver::CDCLSolver() : DPLLSolver() {
  decisionLevel = 0;
  conf = { nullptr, 0, false };
  numAssigned = 0;
}

CDCLSolver::CDCLSolver( std::vector<Clause> form, std::vector<int> sigma, int numClauses, int numVars
        , std::vector<int> activeClauses, int dLevel )
  : DPLLSolver (form, sigma, numClauses, numVars, activeClauses) {
  conf = { nullptr, 0, false };
  decisionLevel = dLevel;
  std::vector<bool> finalVars;
}

void CDCLSolver::recordDecision ( decision d ) {
  decisionLevel++;
  sigma[ d.var ] = d.assignment;
  sigmaStack.push_back( d.var );
  decisionStack.push( d );
  numAssigned++;
  level[ d.var ] = decisionLevel;
}

// Add node to implication graph
void addNode ( CDCLSolver* solver, int index, int var ) {
  (solver->reasons)[ var ].push_back( &(solver->formula[ index ]) ); 
  solver->level[ var ] = solver->decisionLevel;
}

// Remove node from implication graph
void removeNode ( CDCLSolver* solver ) {

}

// unit propagation
void unitPropagate ( CDCLSolver* solver ) {
  while ( true ) {
    Clause c;
    pickClause( solver, &c );
    if ( c.numVars == 0 ) {
      // null clause
      return;
    }
    else if ( c.conflictIndex >= 0 ) {
      // Create conflict
      conflict con = { &( ( solver->formula )[c.conflictIndex] ), solver->decisionLevel, true };
      solver->conf = con;
      return;
    }
    else {
      // no conflict, apply unit rule
      decision d = unitRule( &c, &(solver->sigma), &(solver->sigmaStack) );
      // expand active clauses
      if ( d.var <= 0 ) continue;
      solver->numAssigned++;
      (solver->sigma)[d.var] = d.assignment;
      (solver->sigmaStack).push_back( d.var );
      for ( int i = 0; i < solver->numClauses; i++ ) {
        Clause c = solver->formula[i];
        if ( c.hasLit( d.var ) ) { 
          (solver->activeClauses).push_back( i );
        }
      }
      // Update implication info
      addNode ( solver, c.index, d.var ); 
    }
  }
}

bool operator==(const pair<int> lhs, const int rhs ) {
  return ( lhs.fst == rhs );
}

std::deque<pair<int>> getReasonQueue( CDCLSolver* solver, pair<int> current, std::deque<pair<int>>* cReasons ) {
  std::deque<pair<int>> reasonQueue;
  std::vector<Clause*> tmp;
  tmp = (solver->reasons)[current.fst];
  for ( int i= 0; i < tmp.size(); i++ ) {
    Clause* c = tmp[i];
    for (int j = 0; j < (c->vars).size(); j++ ) {
      int v = c->vars[j];
      if ( v == current.fst || (std::find( cReasons->begin(), cReasons->end(), v ) != cReasons->end() ) ) continue;
      reasonQueue.push_back( { v, solver->level[v] } );
    }
  }
  return reasonQueue;
}

bool isUIP ( std::deque<pair<int>> reasons, int levels ) {
  std::vector<int> sums = std::vector<int>( levels + 1, 0 );
  for (int i = 0; i < reasons.size(); i++ ) {
    int level = reasons[i].snd;
    if ( level > levels )
      return false; 
    sums[level]++;
    if (sums[level] > 1)
      return false;
  }
  return true; //currentLevel;
}

int secondHighestLevel( std::deque<pair<int>>* reasons ) {
  int max = 0;
  int sndmax = 0;
  for ( int i = 0; i < (*reasons).size(); i++ ) {
    int cmp = (*reasons)[i].snd;
    if ( cmp > max ) {
      sndmax = max;
      max = (*reasons)[i].snd;
    }
    else if ( cmp > sndmax )
      sndmax = cmp; 
  }
  return sndmax;
}

Clause assembleClause( std::deque<pair<int>>* reasons, CDCLSolver* solver ) {
  int numv = (*reasons).size();
  if ( numv == 0 )
    return Clause ();
  int index = solver->formula.size();
  std::vector<unsigned int> vs;
  std::vector<bool> isn;
  for ( int i = 0; i < numv; i++ ) {
    int v = (*reasons)[i].fst;
    vs.push_back( v );
    // reverse current state
    isn.push_back( solver->sigma[ v ] > 0 );
  }
  return Clause(vs, isn,  numv, index);
}

int reasonIndex( std::deque<pair<int>>* reasons, int key ) {
  int i;
  for ( i = 0; i < (*reasons).size(); i++ ) {
    if ( (*reasons)[i].fst == key )
      return i;
  }
  return -1;
}

void analyzeAndLearn ( CDCLSolver* solver, learnedClause* lc ) {
  learnedClause learned = {-1, Clause()};
  std::vector<unsigned int> conflictVars = solver->conf.clause->vars;
  std::deque<pair<int>> reasons; //fst: var, snd: decision level
  std::vector<bool> checked = std::vector<bool>(solver->numVars + 1, false);
  // Initialize reasons with elements of conflict clause
  for ( int i = 0; i < conflictVars.size(); i++ ) {
    int v = conflictVars[i];
    checked[v] = true;
    reasons.push_back({ v, solver->level[v] });
  }
  pair<int> tmpr;
  while ( !reasons.empty() && !isUIP( reasons, solver->decisionLevel ) ) { 
    pair<int> cp = reasons.front();
    int v = cp.fst;
    reasons.pop_front();
    std::deque<pair<int>> tmpc = getReasonQueue( solver, reasons.front() , &reasons );
    if (!checked[v]) {
      checked[v] = true;
      reasons.push_back( { v, solver->level[v] } ); 
    } 
    reasons.insert( reasons.end(), tmpc.begin(), tmpc.end() );
  }
  int d = secondHighestLevel( &reasons );
  Clause c = assembleClause( &reasons, solver );
  learned.level = d;
  learned.clause = c;
  *lc = learned;
}

// Back jump to given level
void backjump ( CDCLSolver* solver, int level ) {
  // get relevant decision
  decision dec = solver->decisionStack.top();
  solver->decisionStack.pop();
  while ( !solver->decisionStack.empty() && ( solver->level[ dec.var ] != level ) ) {
    dec = solver->decisionStack.top();
    solver->decisionStack.pop();
  }
  //level = solver->level[ dec.var ];
  int orig = solver->sigma[ dec.var ];
  // Reset all relevant variables:
  solver->decisionLevel = level; 
  for ( int i = 1; i < solver->numVars + 1; i++ ) {
    if ( solver->level[ i ] >= level && !solver->finalVars[ i ]) {
      if ( solver->sigma[ i ] != 0 )
        solver->numAssigned--;
      solver->level[ i ] = 0;
      solver->sigma[ i ] = 0;
      solver->reasons[ i ].clear();
    } 
  }

  // undo sigma stack
  int vr = solver->sigmaStack.back();
  while ( vr != dec.var ) {
    solver->sigmaStack.pop_back();
    vr = solver->sigmaStack.back();
  }
  // Reverse relevant decision
  solver->sigma[dec.var] = -orig;
  solver->sigmaStack.push_back( dec.var );
  solver->level[dec.var] = level;
  solver->finalVars[ dec.var ] = true;
  solver->numAssigned++;
  // Update active clauses:
  Clause c;
  for ( int i = 0; i < solver->numClauses; i++ ) {
    c = solver->formula[i];
    if ( c.hasLit( dec.var ) ) {
      (solver->activeClauses).push_back( i );
    }
  }
}


void cdclEval ( CDCLSolver solver, int timeout, int verbose ) {
  solver.activeClauses = solver.getSingletons();
  bool shouldOverride = false;
  solver.decisionLevel = 0;
  // initialize graph vectors:
  solver.reasons = std::vector< std::vector<Clause*> > ( solver.numVars + 1, std::vector<Clause*>() );
  solver.level = std::vector<int>( solver.numVars + 1, 0 );
  solver.finalVars = std::vector<bool>( solver.numVars + 1, true );
  for ( int i = 1; i < solver.numVars + 1; i++ ) {
    bool bf = solver.sigma[ i ] != 0;
    solver.finalVars[ i ] = bf;
  }
  time_t endTime = time( NULL ) + timeout;
  std::vector<Clause*> reasonQueue;
  learnedClause lc;
  //Main loop: go until timeout
  while( time( NULL ) <= endTime ) {
    int realsum = 0;
    for ( int cr = 1 ; cr < solver.numVars + 1; cr++ )
      if ( solver.sigma[ cr ] != 0 )
        realsum++;
    // reset conflict tracker
    solver.conf = { nullptr, solver.decisionLevel, false }; 
    // unit propagate
    unitPropagate ( &solver );
    shouldOverride = false;
    // If no conflicts, respond
    if ( !solver.conf.isConflict ) {
      if ( solver.numAssigned == solver.numVars ) {
        // All assigned
        std::cout << "sat\n";
        if ( verbose ) {
          std::cout << "lit  val \n";
          for ( int i = 1; i < solver.numVars + 1; i++) {
            std::cout << " " << i << "    " << solver.sigma[ i ] << "\n";
          }
        }
        return;
      } else {
        // make decision
        decision d = makeDecision ( &solver );
        if ( d.var < 0 ) {
          return;
        }
        solver.recordDecision( d ); 
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
      // Analyze and learn!
      analyzeAndLearn( &solver, &lc );
      //analyzeAndLearn ( &solver );
      if ( lc.level < 0  || solver.decisionStack.empty() ) {
        //UNSAT
        std::cout << "unsat\n";
        if ( verbose ) {
          Clause* last = &solver.formula[ solver.numClauses - 1 ];
          int tmpv;
          for ( int k = 0; k < last->numVars; k++ ) {
            tmpv = last->vars[k];
            std::cout << ( last->polarity[k] ? "-" : "" ) << tmpv << " ";
          }
          std::cout << "\n";
        }
        return;
      }
      else {
        // Back jump
        backjump( &solver, lc.level ); //lc.level );
        solver.formula.push_back( lc.clause );
        solver.numClauses++;
        solver.activeClauses.push_back( lc.clause.index );
        lc = { -1, Clause() };
      }
    }
  }
  if ( time (NULL) > endTime ) {
    std::cout << "unknown";
    if ( verbose ) {
      std::cout << "decisions: \n";
      decision dec;
      while ( !solver.decisionStack.empty() ) {
        dec = solver.decisionStack.top();
        std::cout << dec.var << ": " << ( dec.assignment == 1 ) << "\n";
        solver.decisionStack.pop();
      }
    }
  }
}

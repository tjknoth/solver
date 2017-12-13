#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "solver.h"
#include "parser.h"
#include "dpll.h"
#include <sstream>
#include <iterator>
#include <string>
#include <vector>
#include <stack>

using namespace std;

// Helper to print the solver's formula, used for debugging parsing
void printFormula (Solver* s) {
  std::vector<Clause> cVec = s->formula;
  int clauses = s->numClauses;
  int vars = s->numVars;
  std::cout << "Printing formula with " << clauses << " clauses and " << vars << " variables\n";
  for ( int i = 0; i < clauses; i++) {
    int vs = cVec[i].numVars;
    std::cout << "clause " << i << " with " << vs << " variables: ";
    std::vector<unsigned int> vars = cVec[i].vars;
    std::vector<bool> isNeg = cVec[i].polarity;
    int numv = cVec[i].numVars;
    for ( int j = 0; j < numv; j++ ) {
      if ( isNeg[j] )
        std::cout << "-";
      std::cout << vars[j] << "  ";
    }
    std::cout << "\n";
  }
}

// After eliminating pure literal clauses, fix clause indices
void reindexClauses( Solver* solver ) {
  int l = (solver->formula).size();
  for ( int i = 0; i < l; i++ ) {
    (solver->formula)[i].index = i;
  }
}


// Parse SAT problem in DIMACS format.
bool simpleParse (Solver* solver, char* filename) {
  ifstream file ( filename );
  string line = "c";
  int vars = 0;
  int clauses = 0;
  std::vector<bool> isPure;
  std::vector<int> lastPolarity;

  
  if ( file.is_open() ) {
    // parse (consume) comments
    while ( line[0] == 'c' && file ) {
      getline ( file, line ); 
    }
    // parse problem statement
    if ( line[0] == 'p' ) {
      std::istringstream iss( line );
      std::vector<std::string> tokens( ( std::istream_iterator<std::string>( iss ) ),
              std::istream_iterator<std::string>() );
      vars = stoi( tokens[2] );
      clauses = stoi( tokens[3] );
      getline( file, line );
    }

    solver->numClauses = clauses;
    solver->numVars = vars;
    solver->sigma = std::vector<int> ( vars + 1, 0 ); 
    isPure = std::vector<bool> (vars + 1, true);
    isPure[0] = false; // Since isPure is indexed by literals, which start at 1
    // with lastPolarity, 0 means the variable hasn't been processed yet
    // -1 means -, 1 means +. 
    lastPolarity = std::vector<int> (vars + 1, 0);
    // parse clauses:
    std::vector<Clause> form;
    while ( file ) {
      std::vector<unsigned int> parsedVars;
      std::vector<bool> polarity;
      std::stringstream iss( line );
      int v;
      int numv = 0;
      int temp;
      int absVar;
      while ( iss >> v ) {
        if ( v != 0 ) {
          absVar = abs ( v );
          temp = v < 0 ? -1 : 1;
          if ( ( lastPolarity[absVar] != 0 ) && ( temp != lastPolarity[absVar] ) ) {
            isPure[absVar] = false;
          }
          lastPolarity[absVar] = temp;
          polarity.push_back( v < 0 );
          parsedVars.push_back( absVar );
          numv++;
        }
      }
      if ( numv > 0 ) { // ensures empty lines at end will not cause seg fault
        Clause clause ( &parsedVars, &polarity, numv, form.size() );
        form.push_back ( clause );
      }
      getline ( file, line );
    }
    // remove pure literals and update sigma
    std::vector<int> pureLits;
    int numPureLits = 0;
    for( int i = 1; i < vars + 1; i++ ) {
      if( isPure[i] ) {
        pureLits.push_back(i);
        solver->sigma[i] = ( lastPolarity[i] == 1 ? 1 : -1 );
        solver->sigmaStack.push_back( i );
        (solver->numAssigned)++;
        numPureLits++;
      }
    }
    int i = 0;
    while( i < clauses ) {
      for( int j = 0; j < numPureLits; j++ ) {
        if ( i >= clauses ) break; 
        if ( form[i].hasLit( pureLits[j] ) ) {
          form.erase( form.begin() + i );
          clauses--;
          solver->numClauses = clauses;
          continue;
        }
        else
          i++;
      }
      i++;
    }
    solver->formula = form;
    reindexClauses( solver );
    return true; 
  }

  else {
    std::cout << "Invalid filename: " << filename << "\n";
    return false;
  }
}

// parsing function for DPLL: more complex than basic parsing above, includes
//   preprocessing for pure literals
void parse ( DPLLSolver* solver, char* file ) {

}

#ifndef SOLVER_H
#define SOLVER_H

#include <vector>
#include <stack>

template<typename T>
struct pair {
  T fst;
  T snd;
};

// Data structure representing a clause
class Clause {
  public:
    int numVars;
    int conflictIndex;
    std::vector<unsigned int> vars;
    std::vector<bool> polarity; //true indicates variable is negative in SAT formula
    Clause ( std::vector<unsigned int>* v, std::vector<bool>* isNot, int numv, int i );
    Clause( std::vector<unsigned int> v, std::vector<bool> isNot, int numv, int i ); 
    Clause ( int index );
    Clause ();
    bool hasLit ( int lit );
    pair<int> watchedLits;
    int index;
};

// Data structure storing information on a conflict
struct conflict {
  Clause* clause;
  int level;
  bool isConflict;
};

// Data structure containing the formula and variable assignments
class Solver {
  public:
    std::vector<Clause> formula;
    std::vector<int> sigma;
    std::vector<int> sigmaStack;
    int numClauses;
    int numVars;
    int numAssigned;
    Solver ( std::vector<Clause> f, std::vector<int> s, int numc, int numv );
    Solver ();
    void clear ();
    bool moveWatchedLit ( int index, int clause );
};


long long int generateRandomAssignment( int numVars, std::vector<int>* sigma );

bool evaluate ( Solver* solver );

#endif

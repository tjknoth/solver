#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <time.h>
#include <unistd.h>
#include <string>
#include <getopt.h>
#include "solver.h"
#include "parser.h"
#include "dpll.h"
#include "cdcl.h"

using namespace std;

// Randomly evaluate formula for 10 seconds
void randomEval ( Solver solver, int timeout ) {
  int numv = solver.numVars;
  long long int numAssignments = pow ( 2, numv );  
  if ( numAssignments < 0 ) {
    std::cout << "Random evaluation only appropriate for smaller problems (<63 variables)\n";
    std::cout << "Number possible assignments exceeds MAX_INT and it is impossible\nto track which assignments have been explored.\n";
    return;
  }
  time_t end = time( NULL ) + timeout;
  int assignmentsExplored = 0; 
  bool* assignmentMap = (bool*) malloc ( sizeof( bool ) * numAssignments ); //maps assignments to integers by interpreting assignment as binary expression
  for (int i = 0; i < numAssignments; i++) {
    assignmentMap[i] = false;
  }
  long long int currentAssignment;
  srand( time( NULL ) );
  // Loop for 10 seconds:
  while ( time( NULL )  <= end ) {
    currentAssignment = generateRandomAssignment( numv, &(solver.sigma) );
    if ( assignmentMap[currentAssignment] ) {
      solver.clear();
      continue;
    }
    assignmentsExplored++;
    if ( evaluate( &solver ) ) {
      std::cout << "SAT: explored " << assignmentsExplored << " out of " << numAssignments << " possibilities\n";
      for ( int i = 0; i < solver.numVars; i++) {
        std::cout << "Sigma(" << i + 1 << ")= " << solver.sigma[i] << "\n";
      }
      return;
    } else if ( assignmentsExplored == numAssignments ) {
      std::cout << "UNSAT\n";
      return;
    }
    assignmentMap[currentAssignment] = true;
    solver.clear();
  }
  std::cout << "Unknown:\n";
  std::cout << assignmentsExplored << " out of " << numAssignments << " possible assignments explored.\n";
}

void help () {
  std::cout << "Options: short flags are also acceptable\n";
  std::cout << "--time i    Sets time out to i seconds (default is 300)\n";
  std::cout << "--verbose   Verbose mode\n";
  std::cout << "--dpll      Use DPLL Solving algorithm\n"; 
  std::cout << "--rand      Randomly evaluate\n";
  std::cout << "--help      Get help\n";
}

// take file for input, run selected algorithm. 
int main (int argc, char** argv) {
  char* satFile = NULL;
  int dpllFlag = 0;
  int cdclFlag = 0;
  int randFlag = 0;
  int c;
  int verbose = false;
  int time = 300;

  if ( argc <= 1 ) {
    std::cout << "Please provide an input file and select an algorithm\n";
    return -1;
  }
  
  static struct option long_options[] = {
    // flags
    { "verbose", no_argument, &verbose, 1 },
    { "dpll", no_argument, &dpllFlag, 1 },
    { "cdcl", no_argument, &cdclFlag, 1 },
    { "rand", no_argument, &randFlag, 1 },
    { "random", no_argument, &randFlag, 1 },
    // options
    { "time", required_argument, 0, 't' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  int optionIndex;

  while (( c = getopt_long ( argc, argv, "rcdvht:", long_options, &optionIndex )) != -1 ) {
    switch ( c ) {
      case 0:
        if ( long_options[ optionIndex ].flag != 0 ) 
          break;
      case 'h':
        help();
        return -1;
      case 'r':
        randFlag = 1;
      case 'c':
        cdclFlag = 1;
        break;
      case 'd':
        dpllFlag = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      case 't':
        if ( optarg ) {
          time = std::stoi( optarg );
        } else {
          std::cout << "Please specify a time\n";
          return -1;
        }
        break;
      case '?':
        help();
        return -1;
      default:
        break;
    }
  }

  if ( argc - optind > 0 ) {
    satFile = argv[optind];
  } else {
    std::cout << "Please provide an input file\n";
    return -1;
  }
  
  if ( dpllFlag && !cdclFlag && !randFlag) {
    DPLLSolver solver;
    if ( simpleParse( &solver, satFile ) )
      dpllEval( solver, time );
    return 1;
  } else if ( cdclFlag && !randFlag ) {
    CDCLSolver solver;
    if ( simpleParse( &solver, satFile ) )
      cdclEval( solver, time, verbose );
    return -1;
  } else if ( randFlag ){
    Solver solver;
    if ( simpleParse( &solver, satFile ) )
      randomEval ( solver, time );
    return 1;
  } else {
    CDCLSolver solver;
    if ( simpleParse( &solver, satFile ) )
      cdclEval( solver, time, verbose );
    return 1;
  }
}



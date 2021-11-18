/* Provides additional math functions for sqlite:
- Weighted frequencies
- Weighted mean
- Weighted variance
- Weighted standard deviation
*/

#define COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE 1

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include <math.h>
#include <assert.h>
#include <stdint.h>

typedef uint8_t         u8;
typedef int64_t         i64;

/* Weighted Frequency */

typedef struct FreqCtx FreqCtx;
struct FreqCtx {
  double sumWt;
  i64 cnt;
};

static void wtdFreqStep(sqlite3_context *context, int argc, sqlite3_value **argv) {
  FreqCtx *p;

  assert(argc == 1);
  p = sqlite3_aggregate_context(context, sizeof(*p));

  if(sqlite3_value_numeric_type(argv[0])) {
    p->cnt++;
    p->sumWt += sqlite3_value_double(argv[0]);
  }
}

static void wtdFreqFinalize(sqlite3_context *context) {
  FreqCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if(p && p->cnt>0) {
    sqlite3_result_double(context, p->cnt * (p->sumWt/p->cnt));
  }
}

/* Weighted Mean */

typedef struct MeanCtx MeanCtx;
struct MeanCtx {
  double xSum;
  double wtSum;
  i64 cnt;
};

static void wtdMeanStep(sqlite3_context *context, int argc, sqlite3_value **argv) {
  MeanCtx *p;
  double wt;

  assert( argc == 2);
  p = sqlite3_aggregate_context(context, sizeof(*p));

  if( SQLITE_NULL != sqlite3_value_numeric_type(argv[0]) &&
      SQLITE_NULL != sqlite3_value_numeric_type(argv[1])) {
    p->cnt++;
    wt = sqlite3_value_double(argv[1]);
    p->xSum += sqlite3_value_double(argv[0]) * wt;
    p->wtSum += wt;
  }
}

static void wtdMeanFinalize(sqlite3_context *context) {
  MeanCtx *p;
  p = sqlite3_aggregate_context(context, 0);
  if (p && p->cnt>0){
    sqlite3_result_double(context, p->xSum/p->wtSum);
  }
}

/* Weighted Variance and Standard Deviation */

typedef struct VarCtx VarCtx;
struct VarCtx {
  double rM;
  double rS;
  double wtSum;
  i64 cnt;
};

static void wtdVarStep(sqlite3_context *context, int argc, sqlite3_value **argv){
  VarCtx *p;
  double delta;
  double x;
  double wt;

  assert(argc==2);
  p = sqlite3_aggregate_context(context, sizeof(*p));

  if (sqlite3_value_numeric_type(argv[0]) != SQLITE_NULL &&
      sqlite3_value_numeric_type(argv[1]) != SQLITE_NULL) {
    x = sqlite3_value_double(argv[0]);
    wt = sqlite3_value_double(argv[1]);
    p->cnt++;
    p->wtSum += wt;
    delta = (x-p->rM);
    p->rM += (wt/p->wtSum) * delta;
    p->rS += wt * delta * (x-p->rM);
  }
}

static void wtdVarFinalize(sqlite3_context *context) {
  VarCtx *p;
  p = sqlite3_aggregate_context(context, 0);

  if(p && p->cnt>1) {
    sqlite3_result_double(context, p->rS/(p->wtSum-1));
  } else {
    sqlite3_result_double(context, 0.0);
  }
}

static void wtdStdFinalize(sqlite3_context *context) {
  VarCtx *p;
  p = sqlite3_aggregate_context(context, 0);

  if(p && p->cnt>1) {
    sqlite3_result_double(context, sqrt(p->rS/(p->wtSum-1)));
  } else {
    sqlite3_result_double(context, 0.0);
  }
}

int RegisterExtensionFunctions(sqlite3 *db) {
  static const struct FuncDefAgg {
    char *zName;
    signed char nArg;
    u8 argType;
    u8 needCollSeq;
    void (*xStep)(sqlite3_context*,int,sqlite3_value**);
    void (*xFinalize)(sqlite3_context*);
  } aAggs[] = {
    { "wtd_mean", 2, 0, 0, wtdMeanStep, wtdMeanFinalize },
    { "wtd_var", 2, 0, 0, wtdVarStep, wtdVarFinalize },
    { "wtd_sd", 2, 0, 0, wtdVarStep, wtdStdFinalize },
    { "wtd_freq", 1, 0, 0, wtdFreqStep, wtdFreqFinalize }
  };
  int i;

  for(i=0; i<sizeof(aAggs)/sizeof(aAggs[0]); i++){
    void *pArg = 0;
    switch( aAggs[i].argType ){
      case 1: pArg = db; break;
      case 2: pArg = (void *)(-1); break;
    }
    //sqlite3CreateFunc
    /* LMH no error checking */
    sqlite3_create_function(db, aAggs[i].zName, aAggs[i].nArg, SQLITE_UTF8,
        pArg, 0, aAggs[i].xStep, aAggs[i].xFinalize);
#if 0
    if( aAggs[i].needCollSeq ){
      struct FuncDefAgg *pFunc = sqlite3FindFunction( db, aAggs[i].zName,
          strlen(aAggs[i].zName), aAggs[i].nArg, SQLITE_UTF8, 0);
      if( pFunc && aAggs[i].needCollSeq ){
        pFunc->needCollSeq = 1;
      }
    }
#endif
  }
  return 0;
}

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
int sqlite3_extension_init(
    sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi){
  SQLITE_EXTENSION_INIT2(pApi);
  RegisterExtensionFunctions(db);
  return 0;
}
#endif /* COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE */

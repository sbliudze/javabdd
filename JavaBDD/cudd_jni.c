#include <jni.h>
#include <stdlib.h>
#include "util.h"
#include "cudd.h"
#include "cuddInt.h"

#include "cudd_jni.h"

/*
** When casting from `int' to a pointer type, you should
** first cast to `intptr_cast_type'.  This is a type
** that is (a) the same size as a pointer, on most platforms,
** to avoid compiler warnings about casts from pointer to int of
** different size; and (b) guaranteed to be at least as big as
** `int'.
*/
#if __STDC_VERSION__ >= 199901
  #include <inttypes.h>
  #if INTPTR_MAX >= INT_MAX
    typedef intptr_t intptr_cast_type;
  #else /* no intptr_t, or intptr_t smaller than `int' */
    typedef intmax_t intptr_cast_type;
  #endif
#else
  #include <stddef.h>
  #include <limits.h>
  #if PTRDIFF_MAX >= INT_MAX
    typedef ptrdiff_t intptr_cast_type;
  #else
    typedef int intptr_cast_type;
  #endif
#endif

static DdManager *manager;
static jlong bdd_one, bdd_zero;
static int varcount, varnum;

#define INVALID_BDD 0L

static void die(JNIEnv *env, char* msg)
{
    jclass cls;
    cls = (*env)->FindClass(env, "java/lang/InternalError");
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, msg);
    }
    (*env)->DeleteLocalRef(env, cls);
}

/**** START OF NATIVE METHOD IMPLEMENTATIONS ****/

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_registerNatives
  (JNIEnv *env, jclass cl)
{
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    initialize0
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_initialize0
  (JNIEnv *env, jclass cl, jint numSlots, jint cacheSize)
{
    jfieldID one_fid;
    jfieldID zero_fid;
    
    if (manager != NULL) {
        die(env, "init called twice!");
        return;
    }
    
    //manager = Cudd_Init(nodenum, 0, numSlots, cachesize, 0);
    manager = Cudd_Init(0, 0, numSlots, cacheSize, 0);
    if (manager == NULL) {
        die(env, "unable to initialize CUDD");
        return;
    }

    // we cannot use ReadZero because it returns the arithmetic zero,
    // which is different than logical zero.
    bdd_one  = (jlong) (intptr_cast_type) Cudd_ReadOne(manager);
    bdd_zero = (jlong) (intptr_cast_type) Cudd_Not(Cudd_ReadOne(manager));
    
    Cudd_Ref((DdNode *)(intptr_cast_type) bdd_one);
    Cudd_Ref((DdNode *)(intptr_cast_type) bdd_zero);
    
    one_fid = (*env)->GetStaticFieldID(env, cl, "one", "J");
    zero_fid = (*env)->GetStaticFieldID(env, cl, "zero", "J");
    
    if (!one_fid || !zero_fid) {
        die(env, "cannot find members: version mismatch?");
        return;
    }
    (*env)->SetStaticLongField(env, cl, one_fid, bdd_one);
    (*env)->SetStaticLongField(env, cl, zero_fid, bdd_zero);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    isInitialized0
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_net_sf_javabdd_CUDDFactory_isInitialized0
  (JNIEnv *env, jclass cl)
{
    return manager != NULL;
}
  
/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    done0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_done0
  (JNIEnv *env, jclass cl)
{
    int bdds;
    DdManager* m;

    Cudd_Deref((DdNode *)(intptr_cast_type) bdd_one);
    Cudd_Deref((DdNode *)(intptr_cast_type) bdd_zero);
    
    fprintf(stderr, "Garbage collections: %d  Time spent: %ldms\n",
    	Cudd_ReadGarbageCollections(manager), Cudd_ReadGarbageCollectionTime(manager));
    
    bdds = Cudd_CheckZeroRef(manager);
    if (bdds > 0) fprintf(stderr, "Note: %d BDDs still in memory when terminating\n", bdds);
    m = manager;
    manager = NULL; // race condition with delRef
	Cudd_Quit(m);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    varNum0
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_varNum0
  (JNIEnv *env, jclass cl)
{
    return varnum;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    setVarNum0
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_setVarNum0
  (JNIEnv *env, jclass cl, jint x)
{
    jint old = varnum;
    varnum = varcount = x;
    return old;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    ithVar0
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_ithVar0
  (JNIEnv *env, jclass cl, jint i)
{
  DdNode* d;
  jlong result;
  if (i >= CUDD_MAXINDEX - 1) return INVALID_BDD;
  d = Cudd_bddIthVar(manager, i);
  result = (jlong) (intptr_cast_type) d;
  return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    level2Var0
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_level2Var0
  (JNIEnv *env, jclass cl, jint level)
{
	//return manager->invperm[level];
	return (jint) Cudd_ReadInvPerm(manager, level);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    var2Level0
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_var2Level0
  (JNIEnv *env, jclass cl, jint v)
{
	//return (jint) cuddI(manager, v);
	return (jint) Cudd_ReadPerm(manager, v);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    setVarOrder0
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_setVarOrder0
  (JNIEnv *env, jclass cl, jintArray arr)
{
  int *a;
  jint size = (*env)->GetArrayLength(env, arr);
  if (size != varnum) {
    jclass cls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    (*env)->ThrowNew(env, cls, "array size != number of vars");
    (*env)->DeleteLocalRef(env, cls);
    return;
  }
  a = (int*) (*env)->GetPrimitiveArrayCritical(env, arr, NULL);
  if (a == NULL) return;
  Cudd_ShuffleHeap(manager, a);
  (*env)->ReleasePrimitiveArrayCritical(env, arr, a, JNI_ABORT);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    getAllocNum0
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_getAllocNum0
  (JNIEnv *env, jclass cl)
{
  return Cudd_ReadPeakNodeCount(manager);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory
 * Method:    getNodeNum0
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_getNodeNum0
  (JNIEnv *env, jclass cl)
{
  return Cudd_ReadNodeCount(manager);
}

/* class net_sf_javabdd_CUDDFactory_CUDDBDD */

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    var0
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_var0
  (JNIEnv *env, jclass cl, jlong b)
{
    DdNode* d;
    d = (DdNode*) (intptr_cast_type) b;
    return Cudd_Regular(d)->index;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    high0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_high0
  (JNIEnv *env, jclass cl, jlong b)
{
    DdNode* d;
    DdNode* res;
    jlong result;
    d = (DdNode*) (intptr_cast_type) b;
    
    // TODO: check if d is a constant.
    res = Cudd_T(d);
    res = Cudd_NotCond(res, Cudd_IsComplement(d));
    
    result = (jlong) (intptr_cast_type) res;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    low0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_low0
  (JNIEnv *env, jclass cl, jlong b)
{
    DdNode* d;
    DdNode* res;
    jlong result;
    d = (DdNode*) (intptr_cast_type) b;
    
    // TODO: check if d is a constant.
    res = Cudd_E(d);
    res = Cudd_NotCond(res, Cudd_IsComplement(d));
    
    result = (jlong) (intptr_cast_type) res;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    not0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_not0
  (JNIEnv *env, jclass cl, jlong b)
{
    DdNode* d;
    jlong result;
    d = (DdNode*) (intptr_cast_type) b;
    d = Cudd_Not(d);
    result = (jlong) (intptr_cast_type) d;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    ite0
 * Signature: (JJJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_ite0
  (JNIEnv *env, jclass cl, jlong a, jlong b, jlong c)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    DdNode* g;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = (DdNode*) (intptr_cast_type) c;
    g = Cudd_bddIte(manager, d, e, f);
    result = (jlong) (intptr_cast_type) g;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    relprod0
 * Signature: (JJJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_relprod0
  (JNIEnv *env, jclass cl, jlong a, jlong b, jlong c)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    DdNode* g;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = (DdNode*) (intptr_cast_type) c;
    g = Cudd_bddAndAbstract(manager, d, e, f);
    result = (jlong) (intptr_cast_type) g;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    compose0
 * Signature: (JJI)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_compose0
  (JNIEnv *env, jclass cl, jlong a, jlong b, jint i)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = Cudd_bddCompose(manager, d, e, i);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    exist0
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_exist0
  (JNIEnv *env, jclass cl, jlong a, jlong b)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = Cudd_bddExistAbstract(manager, d, e);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    forAll0
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_forAll0
  (JNIEnv *env, jclass cl, jlong a, jlong b)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = Cudd_bddUnivAbstract(manager, d, e);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    restrict0
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_restrict0
  (JNIEnv *env, jclass cl, jlong a, jlong b)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    f = Cudd_bddRestrict(manager, d, e);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    support0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_support0
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode *d;
    DdNode *e;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = Cudd_Support(manager, d);
    result = (jlong) (intptr_cast_type) e;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    apply0
 * Signature: (JJI)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_apply0
  (JNIEnv *env, jclass cl, jlong a, jlong b, jint oper)
{
    DdNode* d;
    DdNode* e;
    DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode*) (intptr_cast_type) b;
    switch (oper) {
    case 0: /* and */
        f = Cudd_bddAnd(manager, d, e);
        break;
    case 1: /* xor */
        f = Cudd_bddXor(manager, d, e);
        break;
    case 2: /* or */
        f = Cudd_bddOr(manager, d, e);
        break;
    case 3: /* nand */
        f = Cudd_bddNand(manager, d, e);
        break;
    case 4: /* nor */
        f = Cudd_bddNor(manager, d, e);
        break;
    case 5: /* imp */
        d = Cudd_Not(d);
        Cudd_Ref(d);
        f = Cudd_bddOr(manager, d, e);
        Cudd_RecursiveDeref(manager, d);
        break;
    case 6: /* biimp */
        f = Cudd_bddXnor(manager, d, e);
        break;
    case 7: /* diff */
        e = Cudd_Not(e);
        Cudd_Ref(e);
        f = Cudd_bddAnd(manager, d, e);
        Cudd_RecursiveDeref(manager, e);
        break;
    case 8: /* less */
        d = Cudd_Not(d);
        Cudd_Ref(d);
        f = Cudd_bddAnd(manager, d, e);
        Cudd_RecursiveDeref(manager, d);
        break;
    case 9: /* invimp */
        e = Cudd_Not(e);
        Cudd_Ref(e);
        f = Cudd_bddOr(manager, d, e);
        Cudd_RecursiveDeref(manager, e);
        break;
    default:
        die(env, "operation not supported");
        return INVALID_BDD;
    }
    result = (jlong) (intptr_cast_type) f;
    return result;
}

static DdNode* satone_rec(DdNode* f)
{
    DdNode* zero = (DdNode*)(intptr_cast_type)bdd_zero;
    DdNode* one = (DdNode*)(intptr_cast_type)bdd_one;
    DdNode* F = Cudd_Regular(f);
    DdNode* high;
    DdNode* low;
    DdNode* r;
    unsigned int index;
    
    if (F == zero ||
    	F == one) {
    	return f;
    }
  	
    index = F->index;
    high = cuddT(F);
    low = cuddE(F);
    if (Cudd_IsComplement(f)) {
	high = Cudd_Not(high);
	low = Cudd_Not(low);
    }
    if (low == (DdNode*)(intptr_cast_type)bdd_zero) {
        DdNode* res = satone_rec(high);
        if (res == NULL) {
            return NULL;
  	}
        cuddRef(res);
        if (Cudd_IsComplement(res)) {
            r = cuddUniqueInter(manager, (int)index, Cudd_Not(res), one);
            if (r == NULL) {
                Cudd_IterDerefBdd(manager, res);
                return NULL;
            }
            r = Cudd_Not(r);
        } else {
            r = cuddUniqueInter(manager, (int)index, res, zero);
            if (r == NULL) {
                Cudd_IterDerefBdd(manager, res);
                return NULL;
            }
        }
        cuddDeref(res);
    } else {
        DdNode* res = satone_rec(low);
        if (res == NULL) return NULL;
        cuddRef(res);
        r = cuddUniqueInter(manager, (int)index, one, Cudd_Not(res));
        if (r == NULL) {
            Cudd_IterDerefBdd(manager, res);
            return NULL;
        }
        r = Cudd_Not(r);
        cuddDeref(res);
    }
    
    return r;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    satOne0
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_satOne0
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    DdNode* res;
    jlong result;
    
    d = (DdNode*) (intptr_cast_type) a;
    
    do {
        manager->reordered = 0;
        res = satone_rec(d);
    } while (manager->reordered == 1);
    
    result = (jlong) (intptr_cast_type) res;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    nodeCount0
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_nodeCount0
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    d = (DdNode*) (intptr_cast_type) a;
    return Cudd_DagSize(d)-1;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    pathCount0
 * Signature: (J)D
 */
JNIEXPORT jdouble JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_pathCount0
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    d = (DdNode*) (intptr_cast_type) a;
    return Cudd_CountPathsToNonZero(d);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    satCount0
 * Signature: (J)D
 */
JNIEXPORT jdouble JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_satCount0
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    d = (DdNode*) (intptr_cast_type) a;
    return Cudd_CountMinterm(manager, d, varcount);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    addRef
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_addRef
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    d = (DdNode*) (intptr_cast_type) a;
    Cudd_Ref(d);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    delRef
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_delRef
  (JNIEnv *env, jclass cl, jlong a)
{
    DdNode* d;
    if (manager == NULL) return;
    d = (DdNode*) (intptr_cast_type) a;
    if (d != INVALID_BDD)
        Cudd_IterDerefBdd(manager, d);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    veccompose0
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_veccompose0
  (JNIEnv *env, jclass cl, jlong a, jlong b)
{
	DdNode* d;
	DdNode** e;
	DdNode* f;
    jlong result;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode**) (intptr_cast_type) b;
    f = Cudd_bddVectorCompose(manager, d, e);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDD
 * Method:    replace0
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDD_replace0
  (JNIEnv *env, jclass cl, jlong a, jlong b)
{
	DdNode* d;
	DdNode** e;
	DdNode* f;
    jlong result;
	int n;
	int *arr;
	arr = (int*) malloc(sizeof(int)*varnum);
	if (arr == NULL) return INVALID_BDD;
    d = (DdNode*) (intptr_cast_type) a;
    e = (DdNode**) (intptr_cast_type) b;
	for (n=0; n<varnum; ++n) {
		DdNode* node = e[n];
		int var = Cudd_Regular(node)->index;
		int level = var;
		//int level = Cudd_ReadPerm(manager, var);
		arr[n] = level;
	}
    f = Cudd_bddPermute(manager, d, arr);
    free(arr);
    result = (jlong) (intptr_cast_type) f;
    return result;
}

/* class net_sf_javabdd_CUDDFactory_CUDDBDDPairing */

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDDPairing
 * Method:    alloc
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDDPairing_alloc
  (JNIEnv *env, jclass cl)
{
	int n;
	DdNode **r = (DdNode**) malloc(sizeof(DdNode*)*varnum);
	if (r == NULL) return 0;
	for (n=0 ; n<varnum ; n++) {
		int var = n;
		//int var = Cudd_ReadInvPerm(manager, n); // level2var
		r[n] = Cudd_bddIthVar(manager, var);
		Cudd_Ref(r[n]);
	}
	return (jlong) (intptr_cast_type) r;
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDDPairing
 * Method:    set0
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDDPairing_set0
  (JNIEnv *env, jclass cl, jlong p, jint var, jint b)
{
	DdNode **r = (DdNode**) (intptr_cast_type) p;
	int level1 = var;
	//int level1 = Cudd_ReadPerm(manager, var);
	//int level2 = Cudd_ReadPerm(manager, b);
	Cudd_RecursiveDeref(manager, r[level1]);
    r[level1] = Cudd_bddIthVar(manager, b);
    Cudd_Ref(r[level1]);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDDPairing
 * Method:    set2
 * Signature: (JIJ)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDDPairing_set2
  (JNIEnv *env, jclass cl, jlong p, jint var, jlong b)
{
	DdNode **r = (DdNode**) (intptr_cast_type) p;
	DdNode *d = (DdNode*) (intptr_cast_type) b;
	int level1 = var;
	//int level1 = Cudd_ReadPerm(manager, var);
	Cudd_RecursiveDeref(manager, r[level1]);
    r[level1] = d;
    Cudd_Ref(r[level1]);
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDDPairing
 * Method:    reset0
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDDPairing_reset0
  (JNIEnv *env, jclass cl, jlong p)
{
	int n;
	DdNode **r = (DdNode**) (intptr_cast_type) p;
	for (n=0 ; n<varnum ; n++) {
		int var;
		Cudd_RecursiveDeref(manager, r[n]);
		var = n;
		//int var = Cudd_ReadInvPerm(manager, n); // level2var
		r[n] = Cudd_bddIthVar(manager, var);
		Cudd_Ref(r[n]);
	}
}

/*
 * Class:     net_sf_javabdd_CUDDFactory_CUDDBDDPairing
 * Method:    free0
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_sf_javabdd_CUDDFactory_00024CUDDBDDPairing_free0
  (JNIEnv *env, jclass cl, jlong p)
{
	int n;
	DdNode **r = (DdNode**) (intptr_cast_type) p;
	for (n=0 ; n<varnum ; n++) {
		Cudd_RecursiveDeref(manager, r[n]);
	}
	free(r);
}

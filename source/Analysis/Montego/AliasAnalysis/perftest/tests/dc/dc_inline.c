/*
!-------------------------------------------------------------------------!
!                                                                         !
!        N  A  S     P A R A L L E L     B E N C H M A R K S  3.3         !
!                                                                         !
!                      S E R I A L     V E R S I O N                      !
!                                                                         !
!                                   D C                                   !
!                                                                         !
!-------------------------------------------------------------------------!
!                                                                         !
!    DC creates all specifided data-cube views.                           !
!    Refer to NAS Technical Report 03-005 for details.                    !
!    It calculates all groupbys in a top down manner using well known     !
!    heuristics and optimizations.                                        !
!                                                                         !
!    Permission to use, copy, distribute and modify this software         !
!    for any purpose with or without fee is hereby granted.  We           !
!    request, however, that all derived work reference the NAS            !
!    Parallel Benchmarks 3.3. This software is provided "as is"           !
!    without express or implied warranty.                                 !
!                                                                         !
!    Information on NPB 3.3, including the technical report, the          !
!    original specifications, source code, results and information        !
!    on how to submit new results, is available at:                       !
!                                                                         !
!           http://www.nas.nasa.gov/Software/NPB/                         !
!                                                                         !
!    Send comments or suggestions to  npb@nas.nasa.gov                    !
!                                                                         !
!          NAS Parallel Benchmarks Group                                  !
!          NASA Ames Research Center                                      !
!          Mail Stop: T27A-1                                              !
!          Moffett Field, CA   94035-1000                                 !
!                                                                         !
!          E-mail:  npb@nas.nasa.gov                                      !
!          Fax:     (650) 604-3957                                        !
!                                                                         !
!-------------------------------------------------------------------------!
! Author: Michael Frumkin                                                 !
!         Leonid Shabanov                                                 !
!-------------------------------------------------------------------------!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include "adc.h"
#include "macrodef.h"
#include "npbparams.h"
#include "protots.h"

void wtime( double * );
double elapsed_time( void )
{
    double t;

    wtime( &t );
    return( t );
}

double start[64], elapsed[64];

void timer_clear( int n )
{
    elapsed[n] = 0.0;
}

void timer_start( int n )
{
    start[n] = elapsed_time();
}

void timer_stop( int n )
{
    double t, now;

    now = elapsed_time();
    t = now - start[n];
    elapsed[n] += t;

}

double timer_read( int n )
{
    return( elapsed[n] );
}

uint32 NumberOfOnes(uint64 s);
void swap8(void *a);
void SetOneBit(uint64 *s, int32 pos){ uint64 ob = MLB; ob >>= pos; *s |= ob;}
void SetOneBit32(uint32 *s, uint32 pos){ 
   uint32 ob = 0x80000000;
   ob >>= pos; 
   *s |= ob;
}
uint32 Mlo32(uint32 x){
   uint32 om = 0x80000000;
   uint32 i;
   uint32 k;
              
   for ( k = 0, i = 0; i < 32; i++ ) {
       if (om&x) break;
       om >>= 1;
       k++;
   } 
   return(k);   
}
int32 mro32(uint32 x){
   uint32 om = 0x00000001;
   uint32 i;
   uint32 k;
              
   for ( k = 32, i = 0; i < 32; i++ ) {
       if (om&x) break;
       om <<= 1;
       k--;
   } 
   return(k);   
}
uint32 setLeadingOnes32(uint32 n){
    int32 om = 0x80000000;
   uint32 x;
   uint32 i;
         
   for ( x = 0, i = 0; i < n; i++ ) {
         x |= om;
         om >>= 1;
   } 
   return (x);
}
int32 DeleteOneFile(const char * file_name) {
#  ifdef WINNT
      return(remove(file_name));
#  else
      return(unlink(file_name));
#  endif
}
void WriteOne32Tuple(char * t, uint32 s, uint32 l, FILE * logf) {
  uint64 ob = MLB32;
  uint32 i;
            
  fprintf(logf, "\n %s", t);
  for ( i = 0; i < l; i++ ) {
    if (s&ob) fprintf(logf, "1"); else fprintf(logf, "0");
    ob >>= 1;
  }
}
uint32 NumOfCombsFromNbyK( uint32 n, uint32 k ){
  uint32 l, combsNbyK;
  if ( k > n ) return 0;
  for(combsNbyK=1, l=1;l<=k;l++)combsNbyK = combsNbyK*(n-l+1)/l;
  return  combsNbyK;
}
void JobPoolUpdate(ADC_VIEW_CNTL *avp){
   uint32 l = avp->nv;
   uint32 k;
  
   k = avp->lpp[l].layerIndex + avp->lpp[l].layerCurrentPopulation;
   avp->jpp[k].grpb = avp->groupby;
   avp->jpp[k].nv = l;
   avp->jpp[k].nRows = avp->nViewRows;
   avp->jpp[k].viewOffset = avp->accViewFileOffset;
   avp->lpp[l].layerCurrentPopulation++;
} 
int32 GetParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple){
   uint32 level, levelPop, i;
   uint32 ig;
   uint32 igOfSmallestParent;
   uint32 igOfPrefixedParent;
   uint32 igOfSharedSortParent;
   uint32 spMinNumOfRows;
   uint32 pfMinNumOfRows;
   uint32 ssMinNumOfRows;
   uint32 tgrpb;
   uint32 pg;
   uint32 pfm;
   uint32 mlo = 0;
   uint32 lom;
   uint32 l = NumberOfOnes(binRepTuple);
   uint32 spFound;
   uint32 pfFound;
   uint32 ssFound;
   uint32 found;
   uint32 spFt;
   uint32 pfFt;   
   uint32 ssFt;

   found = noneParent;
   pfm = setLeadingOnes32(mro32(avp->groupby));
   SetOneBit32(&mlo, Mlo32(avp->groupby));
   lom = setLeadingOnes32(Mlo32(avp->groupby)); 

   for(spFound=pfFound=ssFound=0, level=l;level<=avp->nTopDims;level++){
      levelPop = avp->lpp[level].layerCurrentPopulation;
      
      if(levelPop != 0);
      {
           for ( spFt = pfFt = ssFt = 1, ig = avp->lpp[level].layerIndex,
                 i = 0; i < levelPop; i++ )
           {
               tgrpb = avp->jpp[ig].grpb;
               if ( (avp->groupby & tgrpb) == avp->groupby ) { 
                  spFound = 1;
                  if (spFt) { spMinNumOfRows = avp->jpp[ig].nRows; 
                              igOfSmallestParent = ig; spFt = 0; }
                  else   if ( spMinNumOfRows > avp->jpp[ig].nRows ) 
                            { spMinNumOfRows = avp->jpp[ig].nRows; 
                              igOfSmallestParent = ig; }

				  pg = tgrpb & pfm;
				  if (pg == binRepTuple) {
                     pfFound = 1;
                     if (pfFt) { pfMinNumOfRows = avp->jpp[ig].nRows; 
                                 igOfPrefixedParent = ig; pfFt = 0; }
                     else   if ( pfMinNumOfRows > avp->jpp[ig].nRows) 
                               { pfMinNumOfRows = avp->jpp[ig].nRows; 
                                 igOfPrefixedParent = ig; }
				  }

				  if ( (tgrpb & mlo) && !(tgrpb & lom)) {
                     ssFound = 1;
                     if (ssFt) { ssMinNumOfRows = avp->jpp[ig].nRows; 
                                 igOfSharedSortParent = ig; ssFt = 0; }
                     else   if ( ssMinNumOfRows > avp->jpp[ig].nRows) 
                               { ssMinNumOfRows = avp->jpp[ig].nRows; 
                                 igOfSharedSortParent = ig; }
				  }
               }
               ig++;
           }
      }
      if (pfFound) found = prefixedParent;
      else if (ssFound) found = sharedSortParent;
           else if (spFound) found = smallestParent;

      switch(found){
         case prefixedParent:
           avp->smallestParentLevel = level;
           avp->viewOffset      = avp->jpp[igOfPrefixedParent].viewOffset;
           avp->nParentViewRows = avp->jpp[igOfPrefixedParent].nRows;
           avp->parBinRepTuple  = avp->jpp[igOfPrefixedParent].grpb;
           break;
         case sharedSortParent:
           avp->smallestParentLevel = level;
           avp->viewOffset	    = avp->jpp[igOfSharedSortParent].viewOffset;
           avp->nParentViewRows = avp->jpp[igOfSharedSortParent].nRows;
           avp->parBinRepTuple  = avp->jpp[igOfSharedSortParent].grpb;
           break;
         case smallestParent:
           avp->smallestParentLevel = level;
           avp->viewOffset	    = avp->jpp[igOfSmallestParent].viewOffset;
           avp->nParentViewRows = avp->jpp[igOfSmallestParent].nRows;
           avp->parBinRepTuple  = avp->jpp[igOfSmallestParent].grpb;
           break;
         default: break;
      }
      if(   found == prefixedParent 
         || found == sharedSortParent 
	 || found == smallestParent) break;
   }
  return found;
} 
uint32 GetSmallestParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple){
   uint32 found, level, levelPop, i, ig, igOfSmallestParent;
   uint32 minNumOfRows;
   uint32 tgrpb;
   uint32 ft;
   uint32 l = NumberOfOnes(binRepTuple);
  
   for(found=0, level=l; level<=avp->nTopDims;level++){
      levelPop = avp->lpp[level].layerCurrentPopulation;
      if(levelPop){
        for(ft=1, ig=avp->lpp[level].layerIndex, i=0;i<levelPop;i++){
          tgrpb = avp->jpp[ig].grpb;
          if ( (avp->groupby & tgrpb) == avp->groupby ) { 
            found = 1;
            if(ft){
	      minNumOfRows=avp->jpp[ig].nRows;
	      igOfSmallestParent = ig; 
	      ft = 0;
	    }else if(minNumOfRows > avp->jpp[ig].nRows){ 
	      minNumOfRows = avp->jpp[ig].nRows;
	      igOfSmallestParent = ig;
	    }
          }
          ig++;
        }
      }
      if( found ){      
         avp->smallestParentLevel = level;
         avp->viewOffset = avp->jpp[igOfSmallestParent].viewOffset;
         avp->nParentViewRows = avp->jpp[igOfSmallestParent].nRows;
         avp->parBinRepTuple = avp->jpp[igOfSmallestParent].grpb;
         break;
      }
   }
   return found;
} 
int32 GetPrefixedParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple){
   uint32 found, level, levelPop, i, ig, igOfSmallestParent;
   uint32 minNumOfRows;
   uint32 tgrpb;
   uint32 ft;
   uint32 pg, tm;
   uint32 l = NumberOfOnes(binRepTuple);
   
   tm = setLeadingOnes32(mro32(avp->groupby));

   for(found=0, level=l; level<=avp->nTopDims; level++){
      levelPop = avp->lpp[level].layerCurrentPopulation;
  
      if (levelPop != 0);
      {
           for(ft = 1, ig = avp->lpp[level].layerIndex, 
                i = 0; i < levelPop; i++ ) {
               tgrpb = avp->jpp[ig].grpb;
               if ( (avp->groupby & tgrpb) == avp->groupby ) { 
				  pg = tgrpb & tm;
				  if (pg == binRepTuple) {
                     found = 1;
                     if (ft) { minNumOfRows = avp->jpp[ig].nRows; 
                               igOfSmallestParent = ig; ft = 0; }
                     else if ( minNumOfRows > avp->jpp[ig].nRows) 
                             { minNumOfRows = avp->jpp[ig].nRows; 
                               igOfSmallestParent = ig; }
				  }
               }
               ig++;
           }
      }
      if ( found ) {      
         avp->smallestParentLevel = level;
         avp->viewOffset = avp->jpp[igOfSmallestParent].viewOffset;
         avp->nParentViewRows = avp->jpp[igOfSmallestParent].nRows;
         avp->parBinRepTuple = avp->jpp[igOfSmallestParent].grpb;
         break;
      }
   }
  return found;
} 
void JobPoolInit(JOB_POOL *jpp, uint32 n, uint32 nd){
  uint32 i;

  for ( i = 0; i < n; i++ ) {
      jpp[i].grpb = 0;
	  jpp[i].nv = 0;  
      jpp[i].nRows = 0;
      jpp[i].viewOffset = 0;
  }    
}
void WriteOne64Tuple(char * t, uint64 s, uint32 l, FILE * logf){
   uint64 ob = MLB;
   uint32 i;
            
   fprintf(logf, "\n %s", t);
   for ( i = 0; i < l; i++ ) {
      if (s&ob) fprintf(logf, "1"); else fprintf(logf, "0");
      ob >>= 1;
   }
}
uint32 NumberOfOnes(uint64 s){
   uint64 ob = MLB;
   uint32 i;
   uint32 nOnes;

   for ( nOnes = 0, i = 0; i < 64; i++ ) {
      if (s&ob) nOnes++;
      ob >>= 1;
   }
   return nOnes;
}
void GetRegTupleFromBin64(
           uint64 binRepTuple, 
	       uint32 *selTuple,
	       uint32 numDims, 
	       uint32 *numOfUnits){
   uint64 oc = MLB;
   uint32 i;
   uint32 j;
  
   *numOfUnits = 0;  
   for( j = 0, i = 0; i < numDims; i++ ) {
     if (binRepTuple & oc) { selTuple[j++] = i+1; (*numOfUnits)++;}  
     oc >>= 1;
   }    
}
void getRegTupleFromBin32(
           uint32 binRepTuple, 
	       uint32 *selTuple,
	       uint32 numDims, 
	       uint32 *numOfUnits){
   uint32 oc = MLB32;
   uint32 i;
   uint32 j;
  
   *numOfUnits = 0;
   for( j = 0, i = 0; i < numDims; i++ ) {
     if (binRepTuple & oc) { selTuple[j++] = i+1; (*numOfUnits)++;}  
     oc >>= 1;
   }    
}
void GetRegTupleFromParent(
               uint64 bin64RepTuple,
               uint32 bin32RepTuple, 
	       uint32 *selTuple,
	       uint32 nd){
   uint32 oc = MLB32;
   uint32 i, j, k;
   uint32 ut32; 
  
   ut32 = (uint32)(bin64RepTuple>>(64-nd)); 
   ut32 <<= (32-nd);
   
   for ( j = 0, k = 0, i = 0; i < nd; i++ ) {
     if (bin32RepTuple & oc) k++;
     if (bin32RepTuple & oc && ut32 & oc) selTuple[j++] = k; 
     oc >>= 1;
   }    
}
void CreateBinTuple(uint64 *binRepTuple, uint32 *selTuple, uint32 numDims){
   uint32 i;

   *binRepTuple = 0;
   for(i = 0; i < numDims; i++ ){
     SetOneBit( binRepTuple, selTuple[i]-1 );
   }    
}
void d32v( char * t, uint32 *v, uint32 n){
   uint32 i;
   
   printf("\n%s ", t);
   for ( i = 0; i < n; i++ ) printf(" %d", v[i]);
}
void WriteOne64Tuple(char * t, uint64 s, uint32 l, FILE * logf);
int32 Comp8gbuf(const void *a, const void *b){
   if ( a < b ) return -1;
   else if (a > b) return 1;
   else return 0;
}
void restore(TUPLE_VIEWSIZE x[], uint32 f, uint32 l ){ 
   uint32 j, m, tj, mm1, jm1, hl;
   uint64 iW;
   uint64 iW64;

   j = f;
   hl = l>>1;
   while( j <= hl ) {
      tj = j*2;
      if (tj < l && x[tj-1].viewsize < x[tj].viewsize) m = tj+1;
      else m = tj;
      mm1 = m - 1;
      jm1 = j - 1;
      if ( x[mm1].viewsize > x[jm1].viewsize ) {
         iW = x[mm1].viewsize; 
	 x[mm1].viewsize = x[jm1].viewsize; 
	 x[jm1].viewsize = iW;  
         iW64 = x[mm1].tuple; 
	 x[mm1].tuple = x[jm1].tuple; 
	 x[jm1].tuple = iW64;  
         j = m;
      }else j = l;
   }
}
void vszsort( TUPLE_VIEWSIZE x[], uint32 n){
  int32 i, im1;
  uint64 iW;
  uint64 iW64;
  
  for ( i = n>>1; i >= 1; i-- ) restore( x, i, n );
  for ( i = n; i >= 2; i-- ) {
     im1 = i - 1;
     iW = x[0].viewsize; x[0].viewsize = x[im1].viewsize; x[im1].viewsize = iW;  
     iW64 = x[0].tuple; x[0].tuple = x[im1].tuple; x[im1].tuple = iW64;  
     restore( x, 1, im1);
  }
}
uint32 countTupleOnes(uint64 binRepTuple, uint32 numDims){
  uint32 i, cnt = 0;
  uint64 ob = 0x0000000000000001; 

  for(i = 0; i < numDims; i++ ){
    if ( binRepTuple&ob) cnt++;
    ob <<= 1;
  }    
  return cnt;
}
void restoreo( TUPLE_ONES x[], uint32 f, uint32 l ){ 
   uint32 j, m, tj, mm1, jm1, hl;
   uint32 iW;
   uint64 iW64;

   j = f;
   hl = l>>1;
   while( j <= hl ) {
      tj = j*2;
      if (tj < l && x[tj-1].nOnes < x[tj].nOnes) m = tj+1;
      else m = tj;
      mm1 = m - 1; jm1 = j - 1;
      if ( x[mm1].nOnes > x[jm1].nOnes ){
         iW = x[mm1].nOnes;
	     x[mm1].nOnes = x[jm1].nOnes; 
	     x[jm1].nOnes = iW;  
         iW64 = x[mm1].tuple; 
	     x[mm1].tuple = x[jm1].tuple; 
	     x[jm1].tuple = iW64;  
         j = m;
      }else j = l;
   }
}
void onessort( TUPLE_ONES x[], uint32 n){
   int32 i, im1;
  uint32 iW;
  uint64 iW64;
  
  for ( i = n>>1; i >= 1; i-- ) restoreo( x, i, n );
  for ( i = n; i >= 2; i-- ) {
     im1 = i - 1;
     iW = x[0].nOnes; 
     x[0].nOnes = x[im1].nOnes; 
     x[im1].nOnes = iW;  
     iW64 = x[0].tuple; 
     x[0].tuple = x[im1].tuple; 
     x[im1].tuple = iW64;  
     restoreo( x, 1, im1);
  }
}
uint32 MultiFileProcJobs( TUPLE_VIEWSIZE *tuplesAndSizes, 
		                          uint32 nViews, 
                           ADC_VIEW_CNTL *avp ){
   uint32 i;
    int32 ii; /* it should be int */
   uint32 j;
   uint32 pn;
   uint32 direction = 0;
   uint32 dChange = 0;
   uint32 gbi;
   uint32 maxn;
   uint64 *gbuf;
   uint64      vszs[MAX_NUMBER_OF_TASKS];
   uint32 nGroupbys[MAX_NUMBER_OF_TASKS];
   TUPLE_ONES *toptr;

   gbuf = (uint64*) &avp->memPool[0];

   for(i = 0; i < avp->nTasks; i++ ){ nGroupbys[i] = 0; vszs[i] = 0; }

   for(pn = 0, gbi = 0, ii = nViews-1; ii >= 0; ii-- ){
     if(pn == avp->taskNumber) gbuf[gbi++]=tuplesAndSizes[ii].tuple;
     nGroupbys[pn]++;
     vszs[pn] += tuplesAndSizes[ii].viewsize; 
     if(direction == 0 && pn == avp->nTasks-1 ) { 
       direction = 1; 
       dChange = 1; 
     }
     if(direction == 1 && pn == 0 ){ 
       direction = 0; 
       dChange = 1; 
     }
     if (!dChange){ if (direction) pn--; else pn++;}
     dChange = 0;
   }
   for(maxn = 0, i = 0; i < avp->nTasks; i++) 
     if (nGroupbys[i] > maxn) maxn = nGroupbys[i];

   toptr = (TUPLE_ONES*) malloc(sizeof(TUPLE_ONES)*maxn);
   if(!toptr) return 1; 

   for(i = 0; i < avp->nTasks; i++ ){
     if(i == avp->taskNumber){
       for(j = 0; j < nGroupbys[i]; j++ ){
         toptr[j].tuple = gbuf[j];
         toptr[j].nOnes  = countTupleOnes(gbuf[j], avp->nTopDims);
       }
       qsort((void*)gbuf,  nGroupbys[i], 8, Comp8gbuf );
       onessort(toptr, nGroupbys[i]);

       for(j = 0; j < nGroupbys[i]; j++){
         toptr[nGroupbys[i]-1-j].tuple <<= (64-avp->nTopDims);
         swap8(&toptr[nGroupbys[i]-1-j].tuple);
         fwrite(&toptr[nGroupbys[i]-1-j].tuple, 8, 1, avp->groupbyFile);
       }
     }
   }
   FSEEK(avp->groupbyFile, 0L, SEEK_SET);
   if (toptr) free(toptr);
   return 0;
}
int32 PartitionCube(ADC_VIEW_CNTL *avp){
    TUPLE_VIEWSIZE *tuplesAndSizes;
    uint32 it = 0;
    uint64 sz;
    uint32 sel[64];
    uint32 k;
    uint64 tx;
    uint32 i;
      char inps[256];
      
    tuplesAndSizes = 
       (TUPLE_VIEWSIZE*) malloc(avp->nViewLimit*sizeof(TUPLE_VIEWSIZE));
    if(tuplesAndSizes == NULL){
       printf(" PartitionCube(): memory allocation failure'\n");
       return ADC_MEMORY_ALLOCATION_FAILURE;
    }
    k = 0;
    while( fscanf(avp->adcViewSizesFile, "%s", inps) != EOF ){
       if( strcmp(inps, "Selection:") == 0 ) {
         while ( fscanf(avp->adcViewSizesFile, "%s", inps)) {
           if ( strcmp(inps, "View") == 0 ) break; 
           sel[k++] = atoi(inps);	
         }
       }
       if( strcmp(inps, "Size:") == 0 ){
         fscanf(avp->adcViewSizesFile, "%s", inps);
         sz = atoi(inps);
         CreateBinTuple(&tx, sel, k);
         if (sz > avp->nInputRecs) sz = avp->nInputRecs;
         tuplesAndSizes[it].viewsize = sz;
         tuplesAndSizes[it].tuple = tx; 
         it++;
         k = 0;
       }  
    }
    vszsort(tuplesAndSizes, it);
    for( i = 0; i < it; i++){
        tuplesAndSizes[i].tuple >>= (64-avp->nTopDims);
    }
    if(MultiFileProcJobs( tuplesAndSizes, it, avp )){
       printf("MultiFileProcJobs() is failed \n");
       fprintf(avp->logf, "MultiFileProcJobs() is failed.\n");
       fflush(avp->logf);
       return 1;
    }
    FSEEK(avp->adcViewSizesFile, 0L, SEEK_SET);
    free(tuplesAndSizes);
    return 0;
}

int32 KeyComp( const uint32 *a, const uint32 *b, uint32 n ) {
  uint32 i;
  for ( i = 0; i < n; i++ ) {
    if (a[i] < b[i]) return(-1);
    else if (a[i] > b[i]) return(1);
  }
  return(0);
}
int32 TreeInsert(RBTree *tree, uint32 *attrs){
   uint32  sl = 1;			    	
   uint32 *attrsP;
    int32  cmpres;
 treeNode *xNd, *yNd, *tmp;

  tmp = &tree->root;
  xNd = tmp->left;

  if (xNd == NULL){
    tree->count++;
    NEW_TREE_NODE(tree->mp,tree->memPool,
        	      tree->memaddr,tree->treeNodeSize,
        	      tree->freeNodeCounter,tree->memoryIsFull)
    xNd = tmp->left = tree->mp;
    memcpy(&(xNd->nodeMemPool[0]), &attrs[0], tree->nodeDataSize);
    xNd->left = xNd->right = NULL;
    xNd->clr = BLACK;
    return 0;
  }

  tree->drcts[0] = 0;
  tree->nodes[0] = &tree->root;

  while(1){
    attrsP = (uint32*) &(xNd->nodeMemPool[tree->nm]);
    cmpres = KeyComp( &attrs[tree->nm<<1], attrsP, tree->nd );

    if (cmpres < 0){
      tree->nodes[sl] = xNd;
      tree->drcts[sl++] = 0;
      yNd = xNd->left;

      if(yNd == NULL){
	    NEW_TREE_NODE(tree->mp,tree->memPool,
	  	              tree->memaddr,tree->treeNodeSize,
	  	              tree->freeNodeCounter,tree->memoryIsFull)
        xNd = xNd->left = tree->mp;
        break;
      }
    }else if (cmpres > 0){
      tree->nodes[sl] = xNd;
      tree->drcts[sl++] = 1;
      yNd = xNd->right;
      if(yNd == NULL){
        NEW_TREE_NODE(tree->mp,tree->memPool,
		              tree->memaddr,tree->treeNodeSize,
		              tree->freeNodeCounter,tree->memoryIsFull)
        xNd = xNd->right = tree->mp; 
        break;
      }
    }else{  
      uint64 ii; 
      int64 *mx;
      mx = (int64*) &attrs[0];
      for ( ii = 0; ii < tree->nm; ii++ ) xNd->nodeMemPool[ii] += mx[ii];
      return 0; 
    }
    xNd = yNd;
  }
  tree->count++;
  memcpy(&(xNd->nodeMemPool[0]), &attrs[0], tree->nodeDataSize);
  xNd->left = xNd->right = NULL;
  xNd->clr  = RED;

  while(1){
    if ( tree->nodes[sl-1]->clr != RED || sl<3 ) break;
      
    if (tree->drcts[sl-2] == 0){
      yNd = tree->nodes[sl-2]->right;
      if (yNd != NULL && yNd->clr == RED){
        tree->nodes[sl-1]->clr = BLACK;
        yNd->clr = BLACK;
        tree->nodes[sl-2]->clr = RED;
        sl -= 2;
      }else{
        if (tree->drcts[sl-1] == 1){
	      xNd = tree->nodes[sl-1];
	      yNd = xNd->right;
	      xNd->right = yNd->left;
	      yNd->left  = xNd;
	      tree->nodes[sl-2]->left = yNd;
        }else
          yNd = tree->nodes[sl-1];
	  
        xNd = tree->nodes[sl-2];
        xNd->clr = RED;
        yNd->clr = BLACK;

        xNd->left  = yNd->right;
        yNd->right = xNd;

        if(tree->drcts[sl-3])
          tree->nodes[sl-3]->right = yNd;
	    else  
          tree->nodes[sl-3]->left = yNd;
        break;
      }
    }else{
      yNd = tree->nodes[sl-2]->left;
      if (yNd != NULL && yNd->clr == RED){
         tree->nodes[sl-1]->clr = BLACK;
         yNd->clr = BLACK;
         tree->nodes[sl-2]->clr = RED;
         sl -= 2;
      }else{
    	if(tree->drcts[sl-1] == 0){
          xNd = tree->nodes[sl-1];
          yNd = xNd->left;
          xNd->left  = yNd->right;
          yNd->right = xNd;
          tree->nodes[sl-2]->right = yNd;
   	    }else
          yNd = tree->nodes[sl-1];

   	    xNd = tree->nodes[sl-2];
     	xNd->clr = RED;
    	yNd->clr = BLACK;

    	xNd->right = yNd->left;
    	yNd->left  = xNd;

   	    if (tree->drcts[sl-3])
   	      tree->nodes[sl-3]->right = yNd;
     	else  
   	      tree->nodes[sl-3]->left  = yNd;
   	    break;
      }
    }
  }
  tree->root.left->clr = BLACK;
  return 0;
}
int32 WriteViewToDisk(ADC_VIEW_CNTL *avp, treeNode *t){
  uint32 i;
  if(!t) return ADC_OK;
  if(WriteViewToDisk( avp, t->left)) return ADC_WRITE_FAILED;
  for(i=0;i<avp->nm;i++){
    avp->mSums[i] += t->nodeMemPool[i];  
  }	   
  WriteToFile(t->nodeMemPool,avp->outRecSize,1,avp->viewFile,avp->logf);
  if(WriteViewToDisk( avp, t->right)) return ADC_WRITE_FAILED;
  return ADC_OK;
}
int32 WriteViewToDiskCS(ADC_VIEW_CNTL *avp, treeNode *t,uint64 *ordern){
  uint32 i;
  if(!t) return ADC_OK;
  if(WriteViewToDiskCS( avp, t->left,ordern)) return ADC_WRITE_FAILED;
  for(i=0;i<avp->nm;i++){
    avp->mSums[i] += t->nodeMemPool[i];  
    avp->checksums[i] += (++(*ordern))*t->nodeMemPool[i]%measbound;
  }	   
  WriteToFile(t->nodeMemPool,avp->outRecSize,1,avp->viewFile,avp->logf);
  if(WriteViewToDiskCS( avp, t->right,ordern)) return ADC_WRITE_FAILED;
  return ADC_OK;
}
int32 computeChecksum(ADC_VIEW_CNTL *avp, treeNode *t,uint64 *ordern){
  uint32 i;
  if(!t) return ADC_OK;
  if(computeChecksum(avp,t->left,ordern)) return ADC_WRITE_FAILED;
  for(i=0;i<avp->nm;i++){
    avp->checksums[i] += (++(*ordern))*t->nodeMemPool[i]%measbound;
  }	   
  if(computeChecksum(avp,t->right,ordern)) return ADC_WRITE_FAILED;
  return ADC_OK;
}
int32 WriteChunkToDisk(uint32 recordSize,FILE *fileOfChunks,
		       treeNode *t, FILE *logFile){   
  if(!t) return ADC_OK;
  if(WriteChunkToDisk( recordSize, fileOfChunks, t->left, logFile)) 
    return ADC_WRITE_FAILED; 
  WriteToFile( t->nodeMemPool, recordSize, 1, fileOfChunks, logFile);
  if(WriteChunkToDisk( recordSize, fileOfChunks, t->right, logFile)) 
    return ADC_WRITE_FAILED;
  return ADC_OK;
}
RBTree * CreateEmptyTree(uint32 nd, uint32 nm, 
                         uint32 memoryLimit, unsigned char * memPool){
  RBTree *tree = (RBTree*)  malloc(sizeof(RBTree));
  if (!tree) return NULL;

  tree->root.left = NULL;    
  tree->root.right = NULL;     
  tree->count = 0;
  tree->memaddr = 0;
  tree->treeNodeSize = sizeof(struct treeNode) + DIM_FSZ*(nd-1)+MSR_FSZ*nm;
  if (tree->treeNodeSize%8 != 0) tree->treeNodeSize += 4;
  tree->memoryLimit = memoryLimit;
  tree->memoryIsFull = 0;
  tree->nodeDataSize = DIM_FSZ*nd + MSR_FSZ*nm;
  tree->mp = NULL;
  tree->nNodesLimit = tree->memoryLimit/tree->treeNodeSize;
  tree->freeNodeCounter = tree->nNodesLimit;
  tree->nd = nd;
  tree->nm = nm;
  tree->memPool = memPool;
  tree->nodes = (treeNode**) malloc(sizeof(treeNode*)*MAX_TREE_HEIGHT);
  if (!(tree->nodes)) return NULL;
  tree->drcts = (uint32*) malloc( sizeof(uint32)*MAX_TREE_HEIGHT);
  if (!(tree->drcts)) return NULL;
  return tree;
}
void InitializeTree(RBTree *tree, uint32 nd, uint32 nm){
  tree->root.left = NULL;    
  tree->root.right = NULL;     
  tree->count = 0;
  tree->memaddr = 0;
  tree->treeNodeSize = sizeof(struct treeNode) + DIM_FSZ*(nd-1)+MSR_FSZ*nm;
  if (tree->treeNodeSize%8 != 0) tree->treeNodeSize += 4;
  tree->memoryIsFull = 0;
  tree->nodeDataSize = DIM_FSZ*nd + MSR_FSZ*nm;
  tree->mp = NULL;
  tree->nNodesLimit = tree->memoryLimit/tree->treeNodeSize;
  tree->freeNodeCounter = tree->nNodesLimit;
  tree->nd = nd;
  tree->nm = nm;
}
int32 DestroyTree(RBTree *tree) {
  if (tree==NULL) return ADC_TREE_DESTROY_FAILURE;
  if (tree->memPool!=NULL) free(tree->memPool);
  if (tree->nodes) free(tree->nodes);
  if (tree->drcts) free(tree->drcts);
  free(tree);
  return ADC_OK;
}


extern int32 computeChecksum(ADC_VIEW_CNTL *avp,treeNode *t,uint64 *ordern);
extern int32 WriteViewToDiskCS(ADC_VIEW_CNTL *avp,treeNode *t,uint64 *ordern);

int32 ReadWholeInputData(ADC_VIEW_CNTL *avp, FILE *inpf){
  uint32 iRec = 0;
  uint32 inpBufferLineSize, inpBufferPace, inpRecSize, ib = 0;

  FSEEK(inpf, 0L, SEEK_SET);
  inpRecSize = 8*avp->nm+4*avp->nTopDims;
  inpBufferLineSize = inpRecSize;
  if (inpBufferLineSize%8) inpBufferLineSize += 4;
  inpBufferPace = inpBufferLineSize/4;

  while(fread(&avp->inpDataBuffer[ib], inpRecSize, 1, inpf)){
     iRec++;
     ib += inpBufferPace;      
  }
  avp->nRowsToRead = iRec;
  FSEEK(inpf, 0L, SEEK_SET);
  
  if(avp->nInputRecs != iRec){
     printf(" ReadWholeInputData(): wrong input data reading.\n");
     return ADC_INTERNAL_ERROR;
  }  
  return ADC_OK;
}
int32 ComputeMemoryFittedView (ADC_VIEW_CNTL *avp){
  uint32 iRec = 0;
  uint32 viewBuf[MAX_VIEW_ROW_SIZE_IN_INTS];
  uint32 inpBufferLineSize, inpBufferPace, inpRecSize,ib;
  uint64 ordern=0;
#ifdef VIEW_FILE_OUTPUT
  uint32 retCode;
#endif

  FSEEK(avp->viewFile, 0L, SEEK_END);
  inpRecSize = 8*avp->nm+4*avp->nTopDims;
  inpBufferLineSize = inpRecSize;
  if (inpBufferLineSize%8) inpBufferLineSize += 4;
  inpBufferPace = inpBufferLineSize/4;

  InitializeTree(avp->tree, avp->nv, avp->nm);

  ib=0;
  for ( iRec = 1; iRec <= avp->nRowsToRead; iRec++ ){ 
      SelectToView( &avp->inpDataBuffer[ib], avp->selection, viewBuf, 
  		             avp->nd, avp->nm, avp->nv );
      ib += inpBufferPace;
      TreeInsert(avp->tree, viewBuf);
      if(avp->tree->memoryIsFull){
  	printf("ComputeMemoryFittedView(): Not enough memory.\n");
  	return 1; 
      }
  }

#ifdef VIEW_FILE_OUTPUT
  if( retCode = WriteViewToDiskCS(avp, avp->tree->root.left,&ordern) ){ 
    printf("ComputeMemoryFittedView() Write error is occured.\n");
    return retCode;
  }
#else
  computeChecksum(avp,avp->tree->root.left,&ordern);
#endif
 
  avp->nViewRows = avp->tree->count;
  avp->totalOfViewRows += avp->nViewRows; 			      
  InitializeTree(avp->tree, avp->nv, avp->nm);
  return ADC_OK;
}

int32 SharedSortAggregate(ADC_VIEW_CNTL *avp){
   int32 retCode;
  uint32 iRec = 0;
  uint32   attrs[MAX_VIEW_ROW_SIZE_IN_INTS]; 
  uint32 currBuf[MAX_VIEW_ROW_SIZE_IN_INTS]; 
   int64 chunkOffset = 0;
   int64 inpfOffset;
  uint32 nPart = 0;
  uint32 prevV;
  uint32 currV;
  uint32 total = 0;
  unsigned char *ib;
  uint32 ibsize = SSA_BUFFER_SIZE;
  uint32 nib;
  uint32 iib;
  uint32 nreg;
  uint32 nlst;
  uint32 nsgs;
  uint32 ncur;
  uint32 ibOffset = 0;
  uint64 ordern=0;
   
  ib = (unsigned char*) malloc(ibsize); 
  if (!ib){ 
    printf("SharedSortAggregate: memory allocation failed\n"); 
    return ADC_MEMORY_ALLOCATION_FAILURE; 
  }
  
  nib = ibsize/avp->inpRecSize;
  nsgs = avp->nRowsToRead/nib;
  
  if (nsgs == 0){
      nreg = avp->nRowsToRead; 
      nlst = nreg; 
      nsgs = 1; 
  }else{
     nreg = nib;
     if (avp->nRowsToRead%nib) {
       nsgs++; 
       nlst = avp->nRowsToRead%nib;
     }else{
       nlst = nreg;			   
     }
  }
  
  avp->nViewRows = 0; 
  for( iib = 1; iib <= nsgs; iib++ ){ 
    if(iib > 1) FSEEK(avp->viewFile, inpfOffset, SEEK_SET);
    if( iib == nsgs ) ncur = nlst; else ncur = nreg;
    	  
    fread(ib, ncur*avp->inpRecSize, 1, avp->viewFile);
    inpfOffset = ftell(avp->viewFile);

    for( ibOffset = 0, iRec = 1; iRec <= ncur; iRec++ ){
      memcpy(attrs, &ib[ibOffset], avp->inpRecSize);
      ibOffset += avp->inpRecSize;
      SelectToView(attrs, avp->selection, currBuf, avp->nd, avp->nm, avp->nv); 
      currV = currBuf[2*avp->nm];

      if(iib == 1 && iRec == 1){ 
        prevV = currV; 
        nPart = 1;
        InitializeTree(avp->tree, avp->nv, avp->nm);
        TreeInsert(avp->tree, currBuf);
      }else{
         if (currV == prevV){
            nPart++;
	    TreeInsert (avp->tree, currBuf);
            if (avp->tree->memoryIsFull){
	      avp->chunksParams[avp->numberOfChunks].curChunkNum =
	                                             avp->tree->count;
	      avp->chunksParams[avp->numberOfChunks].chunkOffset = chunkOffset;
              (avp->numberOfChunks)++;
	      if(avp->numberOfChunks >= MAX_NUM_OF_CHUNKS){
                printf("Too many chunks were created.\n"); 
		exit(1);
              }
              chunkOffset += (uint64)(avp->tree->count*avp->outRecSize);
              retCode=WriteChunkToDisk(avp->outRecSize, avp->fileOfChunks,
	                               avp->tree->root.left, avp->logf);                                       
              if(retCode!=ADC_OK){
		printf("SharedSortAggregate: Write error occured.\n"); 
		return retCode;
	      }
              InitializeTree(avp->tree, avp->nv, avp->nm);
	    } /* memoryIsFull */
         }else{
	   if(avp->numberOfChunks && avp->tree->count!=0){ 
	     avp->chunksParams[avp->numberOfChunks].curChunkNum =
	        				     avp->tree->count;
	     avp->chunksParams[avp->numberOfChunks].chunkOffset = chunkOffset;
             (avp->numberOfChunks)++;
             chunkOffset += 
	    	      (uint64)(avp->tree->count*(4*avp->nv + 8*avp->nm));
	     retCode=WriteChunkToDisk( avp->outRecSize, avp->fileOfChunks,
	   				 avp->tree->root.left, avp->logf);
             if(retCode!=ADC_OK){
	       printf("SharedSortAggregate: Write error occured.\n");
	       return retCode;    
	      }
	    }
            FSEEK(avp->viewFile, 0L, SEEK_END);
            if(!avp->numberOfChunks){
               avp->nViewRows += avp->tree->count;
	       retCode = WriteViewToDiskCS(avp, avp->tree->root.left,&ordern);
	       if(retCode!=ADC_OK){ 
	          printf( 
	        	 "SharedSortAggregate: Write error occured.\n");
	          return retCode;
	       }
 	     }else{
	       retCode=MultiWayMerge(avp);
	       if(retCode!=ADC_OK) {
	         printf("SharedSortAggregate.MultiWayMerge: failed.\n");
	         return retCode;
	       } 
	     }
             InitializeTree(avp->tree, avp->nv, avp->nm);
             TreeInsert(avp->tree, currBuf);
             total += nPart;
             nPart = 1;
          }
       }
       prevV = currV;
    } /* iRec */
  } /* iib */

  if(avp->numberOfChunks && avp->tree->count!=0) { 
    avp->chunksParams[avp->numberOfChunks].curChunkNum = avp->tree->count;
    avp->chunksParams[avp->numberOfChunks].chunkOffset = chunkOffset;
    (avp->numberOfChunks)++;
    chunkOffset += (uint64)(avp->tree->count*(4*avp->nv + 8*avp->nm));
    retCode=WriteChunkToDisk(avp->outRecSize, avp->fileOfChunks,
    			     avp->tree->root.left, avp->logf);
    if(retCode!=ADC_OK){
      printf("SharedSortAggregate: Write error occured.\n");
      return retCode;	 
    }
  }
  FSEEK(avp->viewFile, 0L, SEEK_END);
  if(!avp->numberOfChunks){
    avp->nViewRows += avp->tree->count;
    if( retCode = WriteViewToDiskCS(avp, avp->tree->root.left,&ordern)){ 
      printf( "SharedSortAggregate: Write error occured.\n");
      return retCode;
    }	 
  }else{
     retCode=MultiWayMerge(avp);
     if(retCode!=ADC_OK) {
       printf("SharedSortAggregate.MultiWayMerge failed.\n");
       return retCode;
     } 
  }
  FSEEK(avp->fileOfChunks, 0L, SEEK_SET);
  
  total += nPart;
  avp->totalOfViewRows += avp->nViewRows;
  if(ib) free(ib);
  return  ADC_OK;
}
int32 PrefixedAggregate(ADC_VIEW_CNTL *avp, FILE *iof){
   uint32 i;
   uint32 iRec = 0;
   uint32   attrs[MAX_VIEW_ROW_SIZE_IN_INTS]; 
   uint32 aggrBuf[MAX_VIEW_ROW_SIZE_IN_INTS];
   uint32 currBuf[MAX_VIEW_ROW_SIZE_IN_INTS]; 
   uint32 prevBuf[MAX_VIEW_ROW_SIZE_IN_INTS];
    int64 *aggrmp;
    int64 *currmp;
    int32 compRes;
   uint32 nOut = 0; 
   uint32 mpOffset = 0;
   uint32 nOutBufRecs;
   uint32 nViewRows = 0;
    int64 inpfOffset;

    aggrmp = (int64*) &aggrBuf[0];
    currmp = (int64*) &currBuf[0];
    
    for(i = 0; i < 2*avp->nm+avp->nv; i++){prevBuf[i] = 0; aggrBuf[i] = 0;}
    nOutBufRecs = avp->memoryLimit/avp->outRecSize;

    for(iRec = 1; iRec <= avp->nRowsToRead; iRec++ ){ 
      fread(attrs, avp->inpRecSize, 1, iof);
      SelectToView(attrs, avp->selection, currBuf, avp->nd, avp->nm, avp->nv);
      if (iRec == 1) memcpy(aggrBuf, currBuf, avp->outRecSize);
      else{
       compRes = KeyComp( &currBuf[2*avp->nm], &prevBuf[2*avp->nm], avp->nv);

       switch(compRes){
	  case  1: 
	    memcpy(&avp->memPool[mpOffset], aggrBuf, avp->outRecSize);
	    mpOffset += avp->outRecSize;
	    nOut++;
	    for ( i = 0; i < avp->nm; i++ ){
	      avp->mSums[i] += aggrmp[i];
	      avp->checksums[i] += nOut*aggrmp[i]%measbound;
	    }    
	    memcpy(aggrBuf, currBuf, avp->outRecSize);
	    break;
	  case  0: 
	    for ( i = 0; i < avp->nm; i++ ) aggrmp[i] += currmp[i];
	    break;
	  case -1: 
	    printf("PrefixedAggregate: wrong parent view order.\n"); 
	    exit(1);
	    break; 
	  default: 
	    printf("PrefixedAggregate: wrong KeyComp() result.\n"); 
	    exit(1);
	    break;
       }     
    
       if (nOut == nOutBufRecs){
	     inpfOffset = ftell(iof);
	     FSEEK(iof, 0L, SEEK_END);
	     WriteToFile(avp->memPool, nOut*avp->outRecSize, 1, iof, stderr);
	     FSEEK(iof, inpfOffset, SEEK_SET);
	     mpOffset = 0;
	     nViewRows += nOut;
	     nOut = 0; 
       }
     }
     memcpy(prevBuf, currBuf, avp->outRecSize);
   }
   memcpy(&avp->memPool[mpOffset], aggrBuf, avp->outRecSize);
   nOut++;
   for ( i = 0; i < avp->nm; i++ ){
     avp->mSums[i] += aggrmp[i];
     avp->checksums[i] += nOut*aggrmp[i]%measbound;
   }
   FSEEK(iof, 0L, SEEK_END);
   WriteToFile(avp->memPool, nOut*avp->outRecSize, 1, iof, stderr);
   avp->nViewRows	 = nViewRows+nOut;
   avp->totalOfViewRows += avp->nViewRows;
   return ADC_OK;
}
int32 RunFormation (ADC_VIEW_CNTL *avp, FILE *inpf){
   uint32 iRec = 0;
   uint32 viewBuf[MAX_VIEW_ROW_SIZE_IN_INTS];
   uint32   attrs[MAX_VIEW_ROW_SIZE_IN_INTS]; 
    int64 chunkOffset = 0;

   InitializeTree(avp->tree, avp->nv, avp->nm);

   for(iRec = 1; iRec <= avp->nRowsToRead; iRec++ ){ 
     fread(attrs, avp->inpRecSize, 1, inpf);
     SelectToView(attrs, avp->selection, viewBuf, avp->nd, avp->nm, avp->nv); 
     TreeInsert(avp->tree, viewBuf);

     if(avp->tree->memoryIsFull) {
        avp->chunksParams[avp->numberOfChunks].curChunkNum = avp->tree->count;
	    avp->chunksParams[avp->numberOfChunks].chunkOffset  = chunkOffset;		 
        (avp->numberOfChunks)++;
	    if (avp->numberOfChunks >= MAX_NUM_OF_CHUNKS) {
          printf( "RunFormation: Too many chunks were created.\n"); 
          return ADC_INTERNAL_ERROR;
        }
        chunkOffset += (uint64)(avp->tree->count*avp->outRecSize);
        if(WriteChunkToDisk( avp->outRecSize, avp->fileOfChunks,
	                         avp->tree->root.left, avp->logf )){
	       printf( 
	         "RunFormation.WriteChunkToDisk: Write error is occured.\n");
	       return ADC_WRITE_FAILED;
	    }
        InitializeTree(avp->tree, avp->nv, avp->nm);
       }
   } /* Insertion ... */
   if(avp->numberOfChunks && avp->tree->count!=0) { 
     avp->chunksParams[avp->numberOfChunks].curChunkNum = avp->tree->count;
     avp->chunksParams[avp->numberOfChunks].chunkOffset  = chunkOffset;
     (avp->numberOfChunks)++;
     chunkOffset += (uint64)(avp->tree->count*(4*avp->nv + 8*avp->nm));
     if(WriteChunkToDisk(avp->outRecSize, avp->fileOfChunks,
                         avp->tree->root.left, avp->logf)){
       printf( 
            "RunFormation(.WriteChunkToDisk: Write error is occured.\n");
       return ADC_WRITE_FAILED;  
     }
   }
   FSEEK(avp->viewFile, 0L, SEEK_END);
   return ADC_OK;
}
void SeekAndReadNextSubChunk( uint32 multiChunkBuffer[], 
                              uint32 k,
                              FILE *inFile,
		              uint32 chunkRecSize, 
		              uint64 inFileOffs,
		              uint32 subChunkNum){
   int64 ret;
  
   ret = FSEEK(inFile, inFileOffs, SEEK_SET);
   if (ret < 0){
      printf("SeekAndReadNextSubChunk.fseek() < 0 "); 
      exit(1); 
   }
   fread(&multiChunkBuffer[k], chunkRecSize*subChunkNum, 1, inFile);
}
void ReadSubChunk(
            uint32 chunkRecSize,
            uint32 *multiChunkBuffer,
            uint32 mwBufRecSizeInInt,
            uint32 iChunk,
            uint32 regSubChunkSize,
            CHUNKS *chunks,  
              FILE *fileOfChunks
            ){
   if (chunks[iChunk].curChunkNum > 0){
      if(chunks[iChunk].curChunkNum < regSubChunkSize){
	SeekAndReadNextSubChunk(multiChunkBuffer,
	   			(iChunk*regSubChunkSize +
	   			(regSubChunkSize-chunks[iChunk].curChunkNum))*
	   			mwBufRecSizeInInt,
	   			fileOfChunks,
	   			chunkRecSize,
	   			chunks[iChunk].chunkOffset,
	   			chunks[iChunk].curChunkNum);
	chunks[iChunk].posSubChunk=regSubChunkSize-chunks[iChunk].curChunkNum;
	chunks[iChunk].curSubChunk=chunks[iChunk].curChunkNum;
	chunks[iChunk].curChunkNum=0;
	chunks[iChunk].chunkOffset=-1;
      }else{
	SeekAndReadNextSubChunk(multiChunkBuffer,
	   			iChunk*regSubChunkSize*mwBufRecSizeInInt,
	   			fileOfChunks,
	   			chunkRecSize,
	   			chunks[iChunk].chunkOffset,
	   			regSubChunkSize);
	chunks[iChunk].posSubChunk = 0;
	chunks[iChunk].curSubChunk = regSubChunkSize;
	chunks[iChunk].curChunkNum -= regSubChunkSize;
	chunks[iChunk].chunkOffset += regSubChunkSize * chunkRecSize;
      }
   }
}
int32 MultiWayMerge(ADC_VIEW_CNTL *avp){
   uint32 outputBuffer[OUTPUT_BUFFER_SIZE];
   uint32 r_buf       [OUTPUT_BUFFER_SIZE];
   uint32 min_r_buf   [OUTPUT_BUFFER_SIZE];
   uint32 first_one;
   uint32 i;
   uint32 iChunk;
   uint32 min_r_chunk;
   uint32 sPos;
   uint32 iPos;
   uint32 numEmptyBufs;
   uint32 numEmptyRuns;
   uint32 mwBufRecSizeInInt;
   uint32 chunkRecSize;
   uint32 *multiChunkBuffer;
   uint32   regSubChunkSize;
    int32 compRes;
    int64 *m_min_r_buf;
    int64 *m_outputBuffer;

   FSEEK(avp->fileOfChunks, 0L, SEEK_SET);

   multiChunkBuffer = (uint32*) &avp->memPool[0];
   first_one = 1;
   avp->nViewRows  = 0; 

   chunkRecSize = avp->outRecSize;
   mwBufRecSizeInInt = chunkRecSize/4;
   m_min_r_buf = (int64*)&min_r_buf[0];
   m_outputBuffer = (int64*)&outputBuffer[0];

   mwBufRecSizeInInt = chunkRecSize/4;
   regSubChunkSize = (avp->memoryLimit/avp->numberOfChunks)/chunkRecSize;
	 
   if (regSubChunkSize==0) {
     printf(
             "MultiWayMerge: Not enough memory to run the external sort\n");
     return ADC_INTERNAL_ERROR;
   }
   multiChunkBuffer = (uint32*) &avp->memPool[0];

   for(i = 0; i < avp->numberOfChunks; i++ ){
      ReadSubChunk( 
                   chunkRecSize,
                   multiChunkBuffer,
                   mwBufRecSizeInInt,
                   i,
                   regSubChunkSize,
                   avp->chunksParams,  
                   avp->fileOfChunks
      );
   }
   while(1){
     for(iChunk = 0;iChunk<avp->numberOfChunks;iChunk++){
       if (avp->chunksParams[iChunk].curSubChunk > 0){
     	sPos = iChunk*regSubChunkSize*mwBufRecSizeInInt;
    	iPos = sPos+mwBufRecSizeInInt*avp->chunksParams[iChunk].posSubChunk;
     	memcpy(&min_r_buf[0], &multiChunkBuffer[iPos], avp->outRecSize);
	    min_r_chunk = iChunk;
     	break;
       }
     }
     for ( iChunk = min_r_chunk; iChunk < avp->numberOfChunks; iChunk++ ){
       uint32 iPos;

       if (avp->chunksParams[iChunk].curSubChunk > 0){
          iPos = mwBufRecSizeInInt*(iChunk*regSubChunkSize+
                                   avp->chunksParams[iChunk].posSubChunk);
          memcpy(&r_buf[0],&multiChunkBuffer[iPos],avp->outRecSize);

          compRes=KeyComp(&r_buf[2*avp->nm],&min_r_buf[2*avp->nm],avp->nv);	
          if(compRes < 0) {
     	      memcpy(&min_r_buf[0], &r_buf[0], avp->outRecSize);
	          min_r_chunk = iChunk;
          }
       }
     }
     /* Step forward */
     if(avp->chunksParams[min_r_chunk].curSubChunk != 0){
       avp->chunksParams[min_r_chunk].curSubChunk--;
       avp->chunksParams[min_r_chunk].posSubChunk++;
     }

       /* Aggreagation if a duplicate is encountered */
       if(first_one){
         memcpy( &outputBuffer[0], &min_r_buf[0], avp->outRecSize);
         first_one = 0;
       }else{
         compRes = KeyComp( &outputBuffer[2*avp->nm], 
        		    &min_r_buf[2*avp->nm], avp->nv );
         if(!compRes){
           for(i = 0; i < avp->nm; i++ ){ 
             m_outputBuffer[i] += m_min_r_buf[i]; 
           }
         }else{
           WriteToFile(outputBuffer,avp->outRecSize,1,avp->viewFile,stderr);
           avp->nViewRows++;
           for(i=0;i<avp->nm;i++){
	     avp->mSums[i]+=m_outputBuffer[i];
	     avp->checksums[i] += avp->nViewRows*m_outputBuffer[i]%measbound;
	   }
           memcpy( &outputBuffer[0], &min_r_buf[0], avp->outRecSize );
        }
      }

      for(numEmptyBufs = 0, 
          numEmptyRuns = 0, i = 0; i < avp->numberOfChunks; i++ ){
	     if (avp->chunksParams[i].curSubChunk == 0) numEmptyBufs++;
         if (avp->chunksParams[i].curChunkNum == 0) numEmptyRuns++;
      }
      if(   numEmptyBufs == avp->numberOfChunks 
          &&numEmptyRuns == avp->numberOfChunks) break;

      if(avp->chunksParams[min_r_chunk].curSubChunk == 0) {
        ReadSubChunk( 
        	 chunkRecSize,
        	 multiChunkBuffer,
        	 mwBufRecSizeInInt,
        	 min_r_chunk,
        	 regSubChunkSize,
        	 avp->chunksParams,
        	 avp->fileOfChunks);
      }
   } /* while(1) */

   WriteToFile( outputBuffer, avp->outRecSize, 1, avp->viewFile, stderr);	  
   avp->nViewRows++;
   for(i = 0; i < avp->nm; i++ ){ 
     avp->mSums[i] += m_outputBuffer[i]; 
     avp->checksums[i] += avp->nViewRows*m_outputBuffer[i]%measbound;
   }

   avp->totalOfViewRows += avp->nViewRows;
   return ADC_OK;
}
void SelectToView( uint32 * ib, uint32 *ix, uint32 *viewBuf, 
                   uint32 nd, uint32 nm, uint32 nv ){
   uint32 i, j;
   for ( j = 0, i = 0; i < nv; i++ ) viewBuf[2*nm+j++] = ib[2*nm+ix[i]-1];
   memcpy(&viewBuf[0], &ib[0], MSR_FSZ*nm);
}
FILE * AdcFileOpen(const char *fileName, const char *mode){
   FILE *fr;
   if ((fr = (FILE*) fopen(fileName, mode))==NULL)
      printf("AdcFileOpen: Cannot open the file %s errno = %d\n",  
                       fileName, 1);
   return fr;
}
void AdcFileName(char *adcFileName, const char *adcName, 
		 const char *fileName, uint32 taskNumber){
  sprintf(adcFileName, "%s.%s.%d",adcName,fileName,taskNumber);
}
ADC_VIEW_CNTL * NewAdcViewCntl(ADC_VIEW_PARS *adcpp, uint32 pnum){
   ADC_VIEW_CNTL *adccntl;
   uint32 i, j, k;
#ifdef IN_CORE
   uint32 ux;
#endif
   char id[8+1];
   
   adccntl = (ADC_VIEW_CNTL *) malloc(sizeof(ADC_VIEW_CNTL));
   if (adccntl==NULL) return NULL;
   
   adccntl->ndid = adcpp->ndid;
   adccntl->taskNumber = pnum;
   adccntl->retCode = 0;
   adccntl->swapIt = 0;
   strcpy(adccntl->adcName, adcpp->adcName);
   adccntl->nTopDims = adcpp->nd;
   adccntl->nd = adcpp->nd;
   adccntl->nm = adcpp->nm;
   adccntl->nInputRecs = adcpp->nInputRecs;
   adccntl->inpRecSize = GetRecSize(adccntl->nd,adccntl->nm);
   adccntl->outRecSize = GetRecSize(adccntl->nv,adccntl->nm);
   adccntl->accViewFileOffset = 0;
   adccntl->totalViewFileSize = 0;
   adccntl->numberOfMadeViews = 0;
   adccntl->numberOfViewsMadeFromInput = 0;
   adccntl->numberOfPrefixedGroupbys = 0;
   adccntl->numberOfSharedSortGroupbys = 0;
   adccntl->totalOfViewRows = 0;
   adccntl->memoryLimit = adcpp->memoryLimit;
   adccntl->nTasks = adcpp->nTasks;
   strcpy(adccntl->inpFileName, adcpp->adcInpFileName);
   sprintf(id, ".%d", adcpp->ndid);
   
   AdcFileName(adccntl->adcLogFileName, 
               adccntl->adcName, "logf", adccntl->taskNumber);
   strcat(adccntl->adcLogFileName, id);            
   adccntl->logf = AdcFileOpen(adccntl->adcLogFileName, "w");

   AdcFileName(adccntl->inpFileName, adccntl->adcName, "dat", adcpp->ndid);
   adccntl->inpf = AdcFileOpen(adccntl->inpFileName, "rb");
   if(!adccntl->inpf){ 
     adccntl->retCode = ADC_FILE_OPEN_FAILURE; 
     return(adccntl);
   } 

   AdcFileName(adccntl->viewFileName, adccntl->adcName, 
               "view.dat", adccntl->taskNumber);
   strcat(adccntl->viewFileName, id);            
   adccntl->viewFile = AdcFileOpen(adccntl->viewFileName, "wb+");

   AdcFileName(adccntl->chunksFileName, adccntl->adcName, 
               "chunks.dat", adccntl->taskNumber);
   strcat(adccntl->chunksFileName, id);            
   adccntl->fileOfChunks = AdcFileOpen(adccntl->chunksFileName,"wb+");

   AdcFileName(adccntl->groupbyFileName, adccntl->adcName, 
               "groupby.dat", adccntl->taskNumber);
   strcat(adccntl->groupbyFileName, id);
   adccntl->groupbyFile = AdcFileOpen(adccntl->groupbyFileName,"wb+");

   AdcFileName(adccntl->adcViewSizesFileName, adccntl->adcName, 
               "view.sz", adcpp->ndid);
   adccntl->adcViewSizesFile = AdcFileOpen(adccntl->adcViewSizesFileName,"r");
   if(!adccntl->adcViewSizesFile){
     adccntl->retCode = ADC_FILE_OPEN_FAILURE;
     return(adccntl);
   }

   AdcFileName(adccntl->viewSizesFileName, adccntl->adcName, 
               "viewsz.dat", adccntl->taskNumber);
   strcat(adccntl->viewSizesFileName, id);            
   adccntl->viewSizesFile = AdcFileOpen(adccntl->viewSizesFileName, "wb+");
   
   adccntl->chunksParams = (CHUNKS*) malloc(MAX_NUM_OF_CHUNKS*sizeof(CHUNKS));
   if(adccntl->chunksParams==NULL){ 
     fprintf(adccntl->logf,"NewAdcViewCntl: Cannot allocate 'chunksParsms'\n");
     adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE;
     return(adccntl);
   }
   adccntl->memPool = (unsigned char*) malloc(adccntl->memoryLimit);
   if(adccntl->memPool == NULL ){
      fprintf(adccntl->logf, 
              "NewAdcViewCntl: Cannot allocate 'main memory pool'\n"); 
      adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE;
      return(adccntl);
   }
   
#ifdef IN_CORE   
   /* add a condition to allocate this memory buffer, THIS is IMPORTANT */
   ux = 4*adccntl->nTopDims + 8*adccntl->nm;
   if (adccntl->nTopDims%8) ux += 4;
   adccntl->inpDataBuffer = (uint32*) malloc(adccntl->nInputRecs*ux);
   if(adccntl->inpDataBuffer == NULL ){
      fprintf(adccntl->logf,
              "NewAdcViewCntl: Cannot allocate 'input data buffer'\n"); 
      adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE;
      return(adccntl);
   }
#endif
   adccntl->numberOfChunks = 0;

   for ( i = 0; i < adccntl->nm; i++ ){
     adccntl->mSums[i] = 0;
     adccntl->checksums[i] = 0;
     adccntl->totchs[i] = 0;
  }
   adccntl->tree = CreateEmptyTree(adccntl->nd, adccntl->nm, 
                                   adccntl->memoryLimit, adccntl->memPool);
   if(!adccntl->tree){
      fprintf(adccntl->logf,"\nNewAdcViewCntl.CreateEmptyTree failed.\n");
      adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE;
      return(adccntl);
   }

   adccntl->nv = adcpp->nd; /* default */
   for ( i = 0; i < adccntl->nv; i++ ) adccntl->selection[i]=i+1;
   
   adccntl->nViewLimit = (1<<adcpp->nd)-1;
   adccntl->jpp=(JOB_POOL *) malloc((adccntl->nViewLimit+1)*sizeof(JOB_POOL));
   if ( adccntl->jpp == NULL){
      fprintf(adccntl->logf,
        "\n Not enough space to allocate %ld byte for a job pool.", 
        (long)(adccntl->nViewLimit+1)*sizeof(JOB_POOL));
      adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE; 
      return(adccntl);
   }
   adccntl->lpp = (LAYER * ) malloc( (adcpp->nd+1)*sizeof(LAYER));
   if ( adccntl->lpp == NULL){
      fprintf(adccntl->logf,
        "\n Not enough space to allocate %ld byte for a layer reference array.", 
        (long)(adcpp->nd+1)*sizeof(LAYER));
      adccntl->retCode = ADC_MEMORY_ALLOCATION_FAILURE;
      return(adccntl);
   }

   for ( j = 1, i = 1; i <= adcpp->nd; i++ ) {
      k =  NumOfCombsFromNbyK ( adcpp->nd, i );
      adccntl->lpp[i].layerIndex = j;
      j += k;
      adccntl->lpp[i].layerQuantityLimit = k;
      adccntl->lpp[i].layerCurrentPopulation = 0;
   }    
      
   JobPoolInit ( adccntl->jpp, (adccntl->nViewLimit+1), adcpp->nd );

   fprintf(adccntl->logf,"\nMeaning of the log file colums is as follows:\n");
   fprintf(adccntl->logf,
     "Row Number | Groupby | View Size | Measure Sums | Number of Chunks\n");

   adccntl->verificationFailed = 1;
   return adccntl;
}
void InitAdcViewCntl(ADC_VIEW_CNTL *adccntl, 
		     uint32 nSelectedDims, 
		     uint32 *selection, 
		     uint32 fromParent ){
   uint32 i;
   
   adccntl->nv = nSelectedDims;
   
   for (i = 0; i < adccntl->nm; i++ ) adccntl->mSums[i] = 0;
   for (i = 0; i < adccntl->nv; i++ ) adccntl->selection[i] = selection[i];

   adccntl->outRecSize = GetRecSize(adccntl->nv,adccntl->nm);
   adccntl->numberOfChunks = 0;
   adccntl->fromParent = fromParent;
   adccntl->nViewRows = 0;

   if(fromParent){
     adccntl->nd = adccntl->smallestParentLevel;
     FSEEK(adccntl->viewFile, adccntl->viewOffset, SEEK_SET);
     adccntl->nRowsToRead = adccntl->nParentViewRows;
   }else{
     adccntl->nd = adccntl->nTopDims;
     adccntl->nRowsToRead = adccntl->nInputRecs;
   }
   adccntl->inpRecSize = GetRecSize(adccntl->nd,adccntl->nm);
   adccntl->outRecSize = GetRecSize(adccntl->nv,adccntl->nm);
}
int32 CloseAdcView(ADC_VIEW_CNTL *adccntl){
   if (adccntl->inpf) fclose(adccntl->inpf);
   if (adccntl->viewFile) fclose(adccntl->viewFile);
   if (adccntl->fileOfChunks) fclose(adccntl->fileOfChunks);
   if (adccntl->groupbyFile) fclose(adccntl->groupbyFile);
   if (adccntl->adcViewSizesFile) fclose(adccntl->adcViewSizesFile);
   if (adccntl->viewSizesFile) fclose(adccntl->viewSizesFile);
   
   if (DeleteOneFile(adccntl->chunksFileName))       
      return ADC_FILE_DELETE_FAILURE;
   if (DeleteOneFile(adccntl->viewSizesFileName))    
      return ADC_FILE_DELETE_FAILURE;

   if (DeleteOneFile(adccntl->groupbyFileName))      
      return ADC_FILE_DELETE_FAILURE;

   if (adccntl->chunksParams){ 
     free(adccntl->chunksParams); 
     adccntl->chunksParams=NULL; 
   }  
   if (adccntl->memPool){ free(adccntl->memPool); adccntl->memPool=NULL;} 
   if (adccntl->jpp){ free(adccntl->jpp); adccntl->jpp=NULL; } 
   if (adccntl->lpp){ free(adccntl->lpp); adccntl->lpp=NULL; } 

   if (adccntl->logf) fclose(adccntl->logf);
   free(adccntl);
   return ADC_OK;
}
void AdcCntlLog(ADC_VIEW_CNTL *adccntlp){
  fprintf(adccntlp->logf,"    memoryLimit = %20d\n",
    adccntlp->memoryLimit);
  fprintf(adccntlp->logf,"    treeNodeSize = %20d\n",
    adccntlp->tree->treeNodeSize);
  fprintf(adccntlp->logf," treeMemoryLimit = %20d\n",
    adccntlp->tree->memoryLimit);
  fprintf(adccntlp->logf,"    nNodesLimit = %20d\n",
    adccntlp->tree->nNodesLimit);
  fprintf(adccntlp->logf,"freeNodeCounter = %20d\n",
    adccntlp->tree->freeNodeCounter);
  fprintf(adccntlp->logf,"	nViewRows = %20d\n",
    adccntlp->nViewRows);
}
int32 ViewSizesVerification(ADC_VIEW_CNTL *adccntlp){
     char inps[MAX_PARAM_LINE_SIZE];
     char msg[64];
     uint32 *viewCounts;
     uint32 selection_viewSize[2];
     uint32 sz;
     uint32 sel[64];
     uint32 i;
     uint32 k;
     uint64 tx;
     uint32 iTx; 
   
     viewCounts = (uint32 *) &adccntlp->memPool[0];
     for ( i = 0; i <= adccntlp->nViewLimit; i++) viewCounts[i] = 0;
     
     FSEEK(adccntlp->viewSizesFile, 0L, SEEK_SET);
     FSEEK(adccntlp->adcViewSizesFile, 0L, SEEK_SET);     

     while(fread(selection_viewSize, 8, 1, adccntlp->viewSizesFile)){
        viewCounts[selection_viewSize[0]] = selection_viewSize[1];
     }
     k = 0;
     while ( fscanf(adccntlp->adcViewSizesFile, "%s", inps) != EOF ){
        if ( strcmp(inps, "Selection:") == 0 ) {
           while ( fscanf(adccntlp->adcViewSizesFile, "%s", inps)) {
             if ( strcmp(inps, "View") == 0 ) break; 
             sel[k++] = atoi(inps);	  
           }
        }
        
        if ( strcmp(inps, "Size:") == 0 ) {
           fscanf(adccntlp->adcViewSizesFile, "%s", inps);
           sz = atoi(inps);
           CreateBinTuple(&tx, sel, k);
           iTx = (int32)(tx>>(64-adccntlp->nTopDims)); 
           adccntlp->verificationFailed = 0;
           if (!adccntlp->numberOfMadeViews) adccntlp->verificationFailed = 1;

           if ( viewCounts[iTx] != 0){
              if (viewCounts[iTx] != sz) {
                 if (viewCounts[iTx] != adccntlp->nInputRecs){
                   fprintf(adccntlp->logf, 
                           "A view size is wrong: genSz=%d calcSz=%d\n",
                   	                               sz, viewCounts[iTx]);
                   adccntlp->verificationFailed = 1;
                   return ADC_VERIFICATION_FAILED;
                 }
              }               
           }
           k = 0;
        }  
     } /* of while() */

     fprintf(adccntlp->logf,
       "\n\nMeaning of the log file colums is as follows:\n");
     fprintf(adccntlp->logf, 
       "Row Number | Groupby | View Size | Measure Sums | Number of Chunks\n");

     if (!adccntlp->verificationFailed) 
          strcpy(msg, "Verification=passed");
     else strcpy(msg, "Verification=failed");
     FSEEK(adccntlp->logf, 0L, SEEK_SET);
     fprintf(adccntlp->logf, msg);
     FSEEK(adccntlp->logf, 0L, SEEK_END);
     FSEEK(adccntlp->viewSizesFile, 0L, SEEK_SET);
     return ADC_OK;
}
int32 ComputeGivenGroupbys(ADC_VIEW_CNTL *adccntlp){
    int32 retCode;
   uint32 i;
   uint64 binRepTuple;
   uint32 ut32;
   uint32 nViews = 0;
   uint32 nSelectedDims;
   uint32 smp;
#ifdef IN_CORE
   uint32 firstView = 1;
#endif
   uint32 selection_viewsize[2];
   char ttout[16];

   while (fread(&binRepTuple, 8, 1, adccntlp->groupbyFile )){
     for(i = 0; i < adccntlp->nm; i++) adccntlp->checksums[i]=0;
     nViews++;
     swap8(&binRepTuple);

     GetRegTupleFromBin64(binRepTuple, adccntlp->selection,
                          adccntlp->nTopDims, &nSelectedDims);
     ut32 = (uint32)(binRepTuple>>(64-adccntlp->nTopDims));
     selection_viewsize[0] = ut32;
     ut32 <<= (32-adccntlp->nTopDims);
     adccntlp->groupby = ut32;
#ifndef IN_CORE
     smp = GetParent(adccntlp, ut32);
#endif
#ifdef IN_CORE
     if (firstView) {
       firstView = 0;
       if(ReadWholeInputData(adccntlp, adccntlp->inpf)) {
          printf( "ReadWholeInputData failed.\n");
          return ADC_INTERNAL_ERROR;   
       }
     }
     smp = noneParent;
#endif

     if (smp != noneParent)
     GetRegTupleFromParent(binRepTuple, 
                           adccntlp->parBinRepTuple, 
                           adccntlp->selection,
                           adccntlp->nTopDims);
     InitAdcViewCntl(adccntlp, nSelectedDims, 
                     adccntlp->selection, (smp == noneParent)?0:1);
#ifdef IN_CORE
      if(retCode = ComputeMemoryFittedView(adccntlp)) {
         printf( "ComputeMemoryFittedView failed.\n");
         return retCode;
      }
#else
#ifdef OPTIMIZATION
     if (smp == prefixedParent){
        if (retCode = PrefixedAggregate(adccntlp, adccntlp->viewFile)) {
           printf( 
	     "ComputeGivenGroupbys.PrefixedAggregate failed.\n");
           return retCode;
        }
        adccntlp->numberOfPrefixedGroupbys++;
     }else if (smp == sharedSortParent) {
        if (retCode = SharedSortAggregate(adccntlp)) {
           printf( 
	     "ComputeGivenGroupbys.SharedSortAggregate failed.\n");
           return retCode;
        }
        adccntlp->numberOfSharedSortGroupbys++;
     }else
#endif /* OPTIMIZATION */     
     { 
        if( smp != noneParent ) {
	  retCode = RunFormation(adccntlp, adccntlp->viewFile);
          if(retCode!=ADC_OK){
              printf( 
	  	  "ComputrGivenGroupbys.RunFormation failed.\n");
              return retCode; 
            }
	  }else{
	    if ((retCode=RunFormation (adccntlp, adccntlp->inpf)) != ADC_OK){
              printf( 
	  	  "ComputrGivenGroupbys.RunFormation failed.\n");
              return retCode;
            }
	    adccntlp->numberOfViewsMadeFromInput++;
	  }
        if(!adccntlp->numberOfChunks){
          uint64 ordern=0;
          adccntlp->nViewRows        = adccntlp->tree->count;
          adccntlp->totalOfViewRows += adccntlp->nViewRows;
	  retCode=WriteViewToDiskCS(adccntlp,adccntlp->tree->root.left,&ordern);
	  if(retCode!=ADC_OK){
            printf(
	            "ComputeGivenGroupbys.WriteViewToDisk: Write error.\n");
	    return ADC_WRITE_FAILED;
	  }
        }else { 
          retCode=MultiWayMerge(adccntlp);
          if(retCode!=ADC_OK) {
	     printf("ComputeGivenGroupbys.MultiWayMerge failed.\n");
	     return retCode;
	  } 
        } 
      }
     
     JobPoolUpdate(adccntlp);

     adccntlp->accViewFileOffset += 
       (int64)(adccntlp->nViewRows*adccntlp->outRecSize);
     FSEEK(adccntlp->fileOfChunks, 0L, SEEK_SET);
     FSEEK(adccntlp->inpf, 0L, SEEK_SET);
#endif /* IN_CORE */
     for( i = 0; i < adccntlp->nm; i++) 
       adccntlp->totchs[i]+=adccntlp->checksums[i];
     selection_viewsize[1] = adccntlp->nViewRows;
     fwrite(selection_viewsize, 8, 1, adccntlp->viewSizesFile);
     adccntlp->totalViewFileSize += 
                            adccntlp->outRecSize*adccntlp->nViewRows;
     sprintf(ttout, "%7d ", nViews);
     WriteOne32Tuple(ttout, adccntlp->groupby, 
                     adccntlp->nTopDims, adccntlp->logf);
     fprintf(adccntlp->logf, " |  %15d | ", adccntlp->nViewRows); 
     for ( i = 0; i < adccntlp->nm; i++ ){ 
        fprintf(adccntlp->logf, " %20lld", adccntlp->checksums[i]);
     }
     fprintf(adccntlp->logf, " | %5d", adccntlp->numberOfChunks);
   }
   adccntlp->numberOfMadeViews = nViews;  
   if(ViewSizesVerification(adccntlp)) return ADC_VERIFICATION_FAILED;
   return ADC_OK;
}


#define BlockSize 1024

void swap4(void * num){
  char t, *p;
  p = (char *) num;
  t = *p; *p = *(p + 3); *(p + 3) = t;
  t = *(p + 1); *(p + 1) = *(p + 2); *(p + 2) = t;
}
void swap8(void * num){
  char t, *p;
  p = (char *) num;	  
  t = *p; *p = *(p + 7); *(p + 7) = t;
  t = *(p + 1); *(p + 1) = *(p + 6); *(p + 6) = t;
  t = *(p + 2); *(p + 2) = *(p + 5); *(p + 5) = t;
  t = *(p + 3); *(p + 3) = *(p + 4); *(p + 4) = t;
}
void initADCpar(ADC_PAR *par){
  par->ndid=0;
  par->dim=5;
  par->mnum=1;
  par->tuplenum=100;
/*  par->isascii=1; */
  par->inverse_endian=0;
  par->filename="ADC";
  par->clss='U';
}
int ParseParFile(char* parfname,ADC_PAR *par);
int GenerateADC(ADC_PAR *par);

typedef struct Factorization{
  long int *mlt;
  long int *exp;
  long int dim;
} Factorization;

void ShowFactorization(Factorization *nmbfct){
  int i=0;
  for(i=0;i<nmbfct->dim;i++){
    if(nmbfct->mlt[i]==1){
      if(i==0) printf("prime.");
      break;
    }
    if(i>0) printf("*");
    if(nmbfct->exp[i]==1)
      printf("%ld",nmbfct->mlt[i]);    
    else 
      printf("%ld^%ld",nmbfct->mlt[i],
                               nmbfct->exp[i]);
  }
  printf("\n");
}

long int adcprime[]={
  421,601,631,701,883,
  419,443,647,21737,31769,
  1427,18353,22817,34337,98717,
  3527,8693,9677,11093,18233};
  
long int ListFirstPrimes(long int mpr,long int *prlist){
/*
  fprintf(stdout,"ListFirstPrimes: listing primes less than %ld...\n",
                 mpr);
*/
  long int prnum=0;
  int composed=0;
  long int nmb=0,j=0;
  prlist[prnum++]=2;
  prlist[prnum++]=3;
  prlist[prnum++]=5;
  prlist[prnum++]=7;
  for(nmb=8;nmb<mpr;nmb++){
    composed=0;
    for(j=0;prlist[j]*prlist[j]<=nmb;j++){
      if(nmb-prlist[j]*((long int)(nmb/prlist[j]))==0){
        composed=1;
	break;
      }
    }
    if(composed==0) prlist[prnum++]=nmb;
  }
/*  fprintf(stdout,"ListFirstPrimes: Done.\n"); */
  return prnum;
}

long long int LARGE_NUM=0x4FFFFFFFFFFFFFFFLL;
long long int maxprmfctr=59;

long long int GetLCM(long long int mask,
                     Factorization **fctlist,
		     long int *adcexpons){
  int i=0,j=0,k=0;
  int* expons=(int*) calloc(maxprmfctr+1,sizeof(int));
  long long int LCM=1;
  long int pr=2;
  int genexp=1,lexp=1,fct=2;

  for(i=0;i<maxprmfctr+1;i++)expons[i]=0;
  i=0;
  while(mask>0){
    if(mask==2*(mask/2)){
      mask=mask>>1;
      i++;  
      continue;
    }
    pr=adcprime[i];
    genexp=adcexpons[i];
/*
  fprintf(stdout,"[%ld,%ld]\n",pr,genexp);
  ShowFactorization(fctlist[genexp]);
*/
    for(j=0;j<fctlist[pr-1]->dim;j++){
      fct=fctlist[pr-1]->mlt[j];
      lexp=fctlist[pr-1]->exp[j];

      for(k=0;k<fctlist[genexp]->dim;k++){
        if(fctlist[genexp]->mlt[k]==1) break;
        if(fct!=fctlist[genexp]->mlt[k]) continue;
        lexp-=fctlist[genexp]->exp[k];
	break;
      }
      if(expons[fct]<lexp)expons[fct]=lexp;
    }
    mask=mask>>1;
    i++;
  }
/*
for(i=0;i<maxprmfctr;i++){
  if(expons[i]>0) fprintf(stdout,"*%ld^%ld",i,expons[i]);
}
fprintf(stdout,"\n");
*/
  for(i=0;i<=maxprmfctr;i++){
    while(expons[i]>0){
      LCM*=i;
      if(LCM>LARGE_NUM/maxprmfctr) return LCM;
      expons[i]--;
    }
  }
/*  fprintf(stdout,"==== %lld\n",LCM); */
  free(expons);
  return LCM;
}
void ExtendFactors(long int nmb,long int firstdiv,
                   Factorization *nmbfct,Factorization **fctlist){
  Factorization *divfct=fctlist[nmb/firstdiv];
  int fdivused=0;
  int multnum=0;
  int i=0;
/*  fprintf(stdout,"==== %lld %ld %ld\n",divfct->dim,nmb,firstdiv); */
   for(i=0;i<divfct->dim;i++){
    if(divfct->mlt[i]==1){
      if(fdivused==0){
        nmbfct->mlt[multnum]=firstdiv;
        nmbfct->exp[multnum]=1;   
      }
      break;
    }
    if(divfct->mlt[i]<firstdiv){
      nmbfct->mlt[i]=divfct->mlt[i];
      nmbfct->exp[i]=divfct->exp[i];
      multnum++;
    }else if(divfct->mlt[i]==firstdiv){
      nmbfct->mlt[i]=divfct->mlt[i];
      nmbfct->exp[i]=divfct->exp[i]+1;   
      fdivused=1;
    }else{
      int j=i;
      if(fdivused==0) j=i+1;
      nmbfct->mlt[j]=divfct->mlt[i];
      nmbfct->exp[j]=divfct->exp[i];    
    }
  }
}
void GetFactorization(long int prnum,long int *prlist,
                            Factorization **fctlist){
/*fprintf(stdout,"GetFactorization: factorizing first %ld numbers.\n",
                prnum);*/
  long int i=0,j=0;
  Factorization *fct=(Factorization*)malloc(2*sizeof(Factorization)); 
  long int len=0,isft=0,div=1,firstdiv=1;

  fct->dim=2;
  fct->mlt=(long int*)malloc(2*sizeof(long int));
  fct->exp=(long int*)malloc(2*sizeof(long int));
  for(i=0;i<fct->dim;i++){
    fct->mlt[i]=1;
    fct->exp[i]=0;
  }
  fct->mlt[0]=2;
  fct->exp[0]=1;
  fctlist[2]=fct;

  fct=(Factorization*)malloc(2*sizeof(Factorization));
  fct->dim=2;
  fct->mlt=(long int*)malloc(2*sizeof(long int));
  fct->exp=(long int*)malloc(2*sizeof(long int));
  for(i=0;i<fct->dim;i++){
    fct->mlt[i]=1;
    fct->exp[i]=0;
  }
  fct->mlt[0]=3;
  fct->exp[0]=1;
  fctlist[3]=fct;
 
  for(i=0;i<prlist[prnum-1];i++){
    len=0;
    isft=i;
    while(isft>0){
      len++;
      isft=isft>>1;
    }
    fct=(Factorization*)malloc(2*sizeof(Factorization));
    fct->dim=len;
    if (len==0) len=1;
    fct->mlt=(long int*)malloc(len*sizeof(long int));
    fct->exp=(long int*)malloc(len*sizeof(long int));
    for(j=0;j<fct->dim;j++){
      fct->mlt[j]=1;
      fct->exp[j]=0;
    }
    div=1;
    for(j=0;prlist[j]*prlist[j]<=i;j++){
      firstdiv=prlist[j];
      if(i-firstdiv*((long int)i/firstdiv)==0){
        div=firstdiv;
        if(firstdiv*firstdiv==i){
          fct->mlt[0]=firstdiv;
          fct->exp[0]=2;	  
	}else{
	  ExtendFactors(i,firstdiv,fct,fctlist);
        }
	break;
      }
    }
    if(div==1){
      fct->mlt[0]=i;
      fct->exp[0]=1;   
    }
    fctlist[i]=fct;
/*
     ShowFactorization(fct);
*/
  }
/*  fprintf(stdout,"GetFactorization: Done.\n"); */
}

long int adcexp[]={
  11,13,17,19,23,
  23,29,31,37,41,	     	  
  41,43,47,53,59,	     	  
  3,5,7,11,13};
long int adcexpS[]={
  11,13,17,19,23};
long int adcexpW[]={  
  2*2,2*2*2*5,2*3,2*2*5,2*3*7,
  23,29,31,2*2,2*2*19};
long int adcexpA[]={  
  2*2,2*2*2*5,2*3,2*2*5,2*3*7,
  2*19,2*13,2*19,2*2*2*13*19,2*2*2*19*19,                    
  2*23,2*2*2*2,2*2*2*2*2*23,2*2*2*2*2,2*2*23};
long int adcexpB[]={  
  2*2*7,2*2*2*5,2*3*7,2*2*5*7,2*3*7*7,
  2*19,2*13,2*19,2*2*2*13*19,2*2*2*19*19,                      
  2*31,2*2*2*2*31,2*2*2*2*2*31,2*2*2*2*2*29,2*2*29,
  2*43,2*2,2*2,2*2*47,2*2*2*43};  
long int UpPrimeLim=100000;

typedef struct dc_view{
  long long int vsize;
  long int vidx;
} DC_view;

int CompareSizesByValue( const void* sz0, const void* sz1) {
long long int *size0=(long long int*)sz0,
              *size1=(long long int*)sz1;
  int res=0;
  if(*size0-*size1>0) res=1;
  else if(*size0-*size1<0) res=-1;
  return res;
}
int CompareViewsBySize( const void* vw0, const void* vw1) {
DC_view *lvw0=(DC_view *)vw0, *lvw1=(DC_view *)vw1;
  int res=0;
  if(lvw0->vsize>lvw1->vsize) res=1;
  else if(lvw0->vsize<lvw1->vsize) res=-1;
  else if(lvw0->vidx>lvw1->vidx) res=1;
  else if(lvw0->vidx<lvw1->vidx) res=-1;
  return res;
}

int CalculateVeiwSizes(ADC_PAR *par){
  unsigned long long totalInBytes = 0;
  unsigned long long nViewDims, nCubeTuples = 0;
 
  const char *adcfname=par->filename;
  int NDID=par->ndid;
  char clss=par->clss;
  int dcdim=par->dim;
  long long int tnum=par->tuplenum;
  long long int i=0,j=0;
  Factorization  
    **fctlist=(Factorization **) calloc(UpPrimeLim,sizeof(Factorization *));
  long int *prlist=(long int *) calloc(UpPrimeLim,sizeof(long int));
  int prnum=ListFirstPrimes(UpPrimeLim,prlist);
  DC_view *dcview=(DC_view *)calloc((1<<dcdim),sizeof(DC_view));
  const char* vszefname0;
  char *vszefname=NULL;
  FILE* view=NULL;
  int minvn=1, maxvn=(1<<dcdim), vinc=1;
  long idx=0;

  GetFactorization(prnum,prlist,fctlist); 
  for(i=1;i<(1<<dcdim);i++){   
    long long int LCM=1;
    switch(clss){
      case 'U':
        LCM=GetLCM(i,fctlist,adcexp);
      break;
      case 'S':
        LCM=GetLCM(i,fctlist,adcexpS);
      break;
      case 'W':
        LCM=GetLCM(i,fctlist,adcexpW);
      break;
      case 'A':
        LCM=GetLCM(i,fctlist,adcexpA);
      break;
      case 'B':
        LCM=GetLCM(i,fctlist,adcexpB);
      break;
    }
    if(LCM>tnum) LCM=tnum;
    dcview[i].vsize=LCM;
    dcview[i].vidx=i;
  }
  for(i=0;i<UpPrimeLim;i++){
    if(!fctlist[i]) continue;
    if(fctlist[i]->mlt) free(fctlist[i]->mlt); 
    if(fctlist[i]->exp) free(fctlist[i]->exp); 
    free(fctlist[i]);
  }
  free(fctlist);
  free(prlist);
   
  vszefname0="view.sz";
  vszefname=(char*)calloc(BlockSize,sizeof(char));
  sprintf(vszefname,"%s.%s.%d",adcfname,vszefname0,NDID);
  if(!(view = fopen(vszefname, "w+")) ) {
    printf("CalculateVeiwSizes: Can't open file: %s\n",vszefname);
    return 0;
  }
  qsort( dcview, (1<<dcdim), sizeof(DC_view),CompareViewsBySize);	

  switch(clss){
    case 'U':
      vinc=1<<3;
    break;
    case 'S':
    break;
    case 'W':
    break;
    case 'A':
      vinc=1<<6;
    break;
    case 'B':
      vinc=1<<14;
    break;
  }
   for(i=minvn;i<maxvn;i+=vinc){   
    nViewDims = 0;
    fprintf(view,"Selection:");
    idx=dcview[i].vidx;
    for(j=0;j<dcdim;j++) 
      if((idx>>j)&0x1==1) { fprintf(view," %lld",j+1); nViewDims++;}
    fprintf(view,"\nView Size: %lld\n",dcview[i].vsize);

    totalInBytes += (8+4*nViewDims)*dcview[i].vsize;
    nCubeTuples += dcview[i].vsize;

  }
  fprintf(view,"\nTotal in bytes: %lld  Number of tuples: %lld\n", 
          totalInBytes, nCubeTuples);
  
  fclose(view);
  free(dcview);
  printf("View sizes are written into %s\n",vszefname);
  free(vszefname);
  return 1;
}

int ParseParFile(char* parfname,ADC_PAR *par){
  char line[BlockSize];
  FILE* parfile=NULL;
  char* pos=strchr(parfname,'.');
  int linenum=0,i=0;
  const char *kwd;

  if(!(parfile = fopen(parfname, "r")) ) {
    printf("ParseParFile: Can't open file: %s\n",parfname);
    return 0;
  }
  if(pos) pos=strchr(pos+1,'.');
  if(pos) sscanf(pos+1,"%d",&(par->ndid));
  linenum=0;
  while(fgets(&line[0],BlockSize,parfile)){
    i=0;
    kwd=adcKeyword[i];
    while(kwd){
      if(strstr(line,"#")) {
        ;/*comment line, do nothing*/
      }else if(strstr(line,kwd)){
        char *pos=line+strlen(kwd)+1;
        switch(i){
          case 0:
            sscanf(pos,"%d",&(par->dim));
          break;
          case 1:
            sscanf(pos,"%d",&(par->mnum));
          break;
          case 2:
            sscanf(pos,"%lld",&(par->tuplenum));
          break;
          case 3:
/*            sscanf(pos,"%d",&(par->isascii)); */
          break;
          case 4:
            sscanf(pos,"%d",&(par->inverse_endian));
          break;
          case 5:
            par->filename=(char*) malloc(strlen(pos)*sizeof(char));
            sscanf(pos,"%s",par->filename);
          break;
          case 6:
            sscanf(pos,"%c",&(par->clss));
          break;
        }
        break;        
      }
      i++;
      kwd=adcKeyword[i];
    }
    linenum++;
  }
  fclose(parfile);
  switch(par->clss){/* overwriting parameters according the class */
    case 'S':
      par->dim=5;
      par->mnum=1;
      par->tuplenum=1000;
    break;
    case 'W':
      par->dim=10;
      par->mnum=1;
      par->tuplenum=100000;
    break;
    case 'A':
      par->dim=15;
      par->mnum=1;
      par->tuplenum=1000000;
    break;
    case 'B':
      par->dim=20;
      par->mnum=1;
      par->tuplenum=10000000;
    break;
  }  
  return 1;
}
int WriteADCPar(ADC_PAR *par,char* fname){
  char *lname=(char*) calloc(BlockSize,sizeof(char));
  FILE *parfile=NULL;

  sprintf(lname,"%s",fname);
  parfile=fopen(lname,"w");
  if(!parfile){
    printf("WriteADCPar: can't open file %s\n",lname);
    return 0;
  }
  fprintf(parfile,"attrNum=%d\n",par->dim);
  fprintf(parfile,"measuresNum=%d\n",par->mnum);
  fprintf(parfile,"tuplesNum=%lld\n",par->tuplenum);
  fprintf(parfile,"class=%c\n",par->clss);
/*  fprintf(parfile,"isASCII=%d\n",par->isascii); */
  fprintf(parfile,"INVERSE_ENDIAN=%d\n",par->inverse_endian);
  fprintf(parfile,"fileName=%s\n",par->filename);
  fclose(parfile);
  return 1;
}
void ShowADCPar(ADC_PAR *par){
  printf("********************* ADC paramters\n");
  printf(" id		%d\n",par->ndid);
  printf(" attributes 	%d\n",par->dim);
  printf(" measures   	%d\n",par->mnum);
  printf(" tuples     	%lld\n",par->tuplenum);
  printf(" class	\t%c\n",par->clss);
  printf(" filename       %s\n",par->filename);
  printf("***********************************\n");
}

long int adcgen[]={
  2,7,3,2,2,
  2,2,5,31,7,
  2,3,3,3,2,
  5,2,2,2,3};
  
int GetNextTuple(int dcdim, int measnum,
                 long long int* attr,long long int* meas,
		 char clss){
  static int tuplenum=0;
  static const int maxdim=20;
  static int measbound=31415;
  int i=0,j=0;
  int maxattr=0;
  static long int seed[20];
  long int *locexp=NULL;

  if(dcdim>maxdim){
    printf("GetNextTuple: number of dcdim is too large:%d",
                    dcdim);
    return 0;
  }
  if(measnum>measbound){
    printf("GetNextTuple: number of mes is too large:%d",
                    measnum);
    return 0;
  }
  locexp=adcexp;
  switch(clss){
    case 'S':
    locexp=adcexpS;
    break;
    case 'W':
    locexp=adcexpW;
    break;
    case 'A':
    locexp=adcexpA;
    break;
    case 'B':
    locexp=adcexpB;
    break;
  }  
  if(tuplenum==0){
    for(i=0;i<maxdim;i++){
      int tmpgen=adcgen[i];
      for(j=0;j<locexp[i]-1;j++){
        tmpgen*=adcgen[i];
	tmpgen=tmpgen%adcprime[i];
      }
      adcgen[i]=tmpgen;
    }
    printf("Prime \tGenerator \tSeed\n");
    for(i=0;i<dcdim;i++){
      seed[i]=(adcprime[i]+1)/2;
      printf(" %ld\t %ld\t\t %ld\n",adcprime[i],adcgen[i],seed[i]);
     }
  }
  tuplenum++;
  maxattr=0;
  for(i=0;i<dcdim;i++){
    attr[i]=seed[i]*adcgen[i];
    attr[i]-=adcprime[i]*((long long int)attr[i]/adcprime[i]); 
    seed[i]=attr[i];
    if(seed[i]>maxattr) maxattr=seed[i];
  }		     	  
  for(i=0;i<measnum;i++){
    meas[i]=(long long int)(seed[i]*maxattr);
    meas[i]-=measbound*(meas[i]/measbound);
  }		     	  
  return 1;
}

int GenerateADC(ADC_PAR *par){
  int dcdim=par->dim,
      mesnum=par->mnum,
      tplnum=par->tuplenum;
  char *adcfname=(char*)calloc(BlockSize,sizeof(char));
  
  FILE *adc;
  int i=0,j=0;
  long long int* attr=NULL,*mes=NULL; 
/*
   if(par->isascii==1){
    sprintf(adcfname,"%s.tpl.%d",par->filename,par->ndid);
    if(!(adc = fopen(adcfname, "w+"))) {
      printf("GenerateADC: Can't open file: %s\n",adcfname);
      return 0;
    }
  }else{
*/
  sprintf(adcfname,"%s.dat.%d",par->filename,par->ndid);
    if(!(adc = fopen(adcfname, "wb+"))){
      printf("GenerateADC: Can't open file: %s\n",adcfname);
       return 0;
    }
/*  } */
  attr=(long long int *)malloc(dcdim*sizeof(long long int));
  mes=(long long int *)malloc(mesnum*sizeof(long long int));

  printf("\nGenerateADC: writing %d tuples of %d attributes and %d measures to %s\n",
		  tplnum,dcdim,mesnum,adcfname);
   for(i=0;i<tplnum;i++){
    if(!GetNextTuple(dcdim,mesnum,attr,mes,par->clss)) return 0;
/*
     if(par->isascii==1){
      for(int j=0;j<dcdim;j++)fprintf(adc,"%lld ",attr[j]);
      for(int j=0;j<mesnum;j++)fprintf(adc,"%lld ",mes[j]);
      fprintf(adc,"\n");
    }else{
*/
      for(j=0;j<mesnum;j++){ 
    	long long mv =  mes[j];
	    if(par->inverse_endian==1) swap8(&mv);
	    fwrite(&mv, 8, 1, adc); 
      }
      for(j=0;j<dcdim;j++){ 
    	int av = attr[j]; 
	if(par->inverse_endian==1) swap4(&av);
	fwrite(&av, 4, 1, adc); 
      }
    }
/*  } */
  fclose(adc);
  printf("Binary ADC file %s ",adcfname);
  printf("have been generated.\n");
  free(attr);
  free(mes);
  free(adcfname);
  CalculateVeiwSizes(par);
  return 1;
}


void c_print_results( char   *name,
                      char   clss,
                      int    n1, 
                      int    n2,
                      int    n3,
                      int    niter,
                      double t,
                      double mops,
		      char   *optype,
                      int    passed_verification,
                      char   *npbversion,
                      char   *compiletime,
                      char   *cc,
                      char   *clink,
                      char   *c_lib,
                      char   *c_inc,
                      char   *cflags,
                      char   *clinkflags );

void initADCpar(ADC_PAR *par);
int ParseParFile(char* parfname, ADC_PAR *par); 
int GenerateADC(ADC_PAR *par);
void ShowADCPar(ADC_PAR *par);
int32 DC(ADC_VIEW_PARS *adcpp);
int Verify(long long int checksum,ADC_VIEW_PARS *adcpp);

#define BlockSize 1024

int main ( int argc, char * argv[] ) 
{
  ADC_PAR *parp;
  ADC_VIEW_PARS *adcpp;
  int32 retCode;

  printf("\n\n NAS Parallel Benchmarks (NPB3.3-SER) - DC Benchmark\n\n" );
  if(argc!=3){
    printf(" No Paramter file. Using compiled defaults\n");
  }
  if(argc>3 || (argc>1 && !isdigit(argv[1][0]))){
    printf("Usage: <program name> <amount of memory>\n");
    printf("       <file of parameters>\n");
    printf("Example: bin/dc.S 1000000 DC/ADC.par\n");
    printf("The last argument, (a parameter file) can be skipped\n");
    exit(1);
  }

  if(  !(parp = (ADC_PAR*) malloc(sizeof(ADC_PAR)))
     ||!(adcpp = (ADC_VIEW_PARS*) malloc(sizeof(ADC_VIEW_PARS)))){
     PutErrMsg("main: malloc failed")
     exit(1);
  }
  initADCpar(parp);
  parp->clss=CLASS;
  if(argc!=3){
    parp->dim=attrnum;
    parp->tuplenum=input_tuples;    
  }else if( (argc==3)&&(!ParseParFile(argv[2], parp))) {
    PutErrMsg("main.ParseParFile failed")
    exit(1);
  }
  ShowADCPar(parp); 
  if(!GenerateADC(parp)) {
     PutErrMsg("main.GenerateAdc failed")
     exit(1);
  }

  adcpp->ndid = parp->ndid;  
  adcpp->clss = parp->clss;
  adcpp->nd = parp->dim;
  adcpp->nm = parp->mnum;
  adcpp->nTasks = 1;
  if(argc>=2)
    adcpp->memoryLimit = atoi(argv[1]);
  else
    adcpp->memoryLimit = 0;
  if(adcpp->memoryLimit <= 0){
    /* size of rb-tree with tuplenum nodes */
    adcpp->memoryLimit = parp->tuplenum*(50+5*parp->dim); 
    printf("Estimated rb-tree size = %d \n", adcpp->memoryLimit);
  }
  adcpp->nInputRecs = parp->tuplenum;
  strcpy(adcpp->adcName, parp->filename);
  strcpy(adcpp->adcInpFileName, parp->filename);

  if((retCode=DC(adcpp))) {
     PutErrMsg("main.DC failed")
     printf( "main.ParRun failed: retcode = %d\n", retCode);
     exit(1);
  }

  if(parp)  { free(parp);   parp = 0; }
  if(adcpp) { free(adcpp); adcpp = 0; }
  return 0;
}

int32		 CloseAdcView(ADC_VIEW_CNTL *adccntl);  
int32		 PartitionCube(ADC_VIEW_CNTL *avp);				
ADC_VIEW_CNTL *NewAdcViewCntl(ADC_VIEW_PARS *adcpp, uint32 pnum);
int32		 ComputeGivenGroupbys(ADC_VIEW_CNTL *adccntl);

int32 DC(ADC_VIEW_PARS *adcpp) {
   int32 itsk=0;
   double t_total=0.0;
   int verified;

   typedef struct { 
      int    verificationFailed;
      uint32 totalViewTuples;
      uint64 totalViewSizesInBytes;
      uint32 totalNumberOfMadeViews;
      uint64 checksum;
      double tm_max;
   } PAR_VIEW_ST;
   
   PAR_VIEW_ST *pvstp;
   ADC_VIEW_CNTL *adccntlp;

   pvstp = (PAR_VIEW_ST*) malloc(sizeof(PAR_VIEW_ST));
   pvstp->verificationFailed = 0;
   pvstp->totalViewTuples = 0;
   pvstp->totalViewSizesInBytes = 0;
   pvstp->totalNumberOfMadeViews = 0;
   pvstp->checksum = 0;
   
   adccntlp = NewAdcViewCntl(adcpp, itsk);
   if (!adccntlp) { 
      PutErrMsg("ParRun.NewAdcViewCntl: returned NULL")
      return ADC_INTERNAL_ERROR;
   }else{
     if (adccntlp->retCode!=0) {
   	printf( 
   		 "DC.NewAdcViewCntl: return code = %d\n",
   						adccntlp->retCode); 
     }
   }
   if( PartitionCube(adccntlp) ) {
      PutErrMsg("DC.PartitionCube failed");
   }
   timer_clear(itsk);
   timer_start(itsk);
   if( ComputeGivenGroupbys(adccntlp) ) {
      PutErrMsg("DC.ComputeGivenGroupbys failed");
   }
   timer_stop(itsk);
   pvstp->tm_max = timer_read(itsk);
   pvstp->verificationFailed += adccntlp->verificationFailed;
   if (!adccntlp->verificationFailed) {
     pvstp->totalNumberOfMadeViews += adccntlp->numberOfMadeViews;
     pvstp->totalViewSizesInBytes += adccntlp->totalViewFileSize;
     pvstp->totalViewTuples += adccntlp->totalOfViewRows;
     pvstp->checksum += adccntlp->totchs[0];
   }   
   if(CloseAdcView(adccntlp)) {
     PutErrMsg("ParRun.CloseAdcView: is failed");
     adccntlp->verificationFailed = 1;
   }

   t_total=pvstp->tm_max; 

   pvstp->verificationFailed=Verify(pvstp->checksum,adcpp);
   verified = (pvstp->verificationFailed == -1)? -1 :
              (pvstp->verificationFailed ==  0)?  1 : 0;

   printf("\n*** DC Benchmark Results:\n");
   printf(" Benchmark Time   = %20.3f\n", t_total);
   printf(" Input Tuples     =         %12d\n", (int) adcpp->nInputRecs);
   printf(" Number of Views  =         %12d\n",
           (int) pvstp->totalNumberOfMadeViews);
   printf(" Number of Tasks  =         %12d\n", (int) adcpp->nTasks);
   printf(" Tuples Generated = %20.0f\n",
           (double) pvstp->totalViewTuples);
   printf(" Tuples/s         = %20.2f\n", 
           (double) pvstp->totalViewTuples / t_total);
   printf(" Checksum         = %20.12e\n", (double) pvstp->checksum);
   if (pvstp->verificationFailed)
      printf( " Verification failed\n");

   c_print_results("DC",
  		   adcpp->clss,
  		   (int)adcpp->nInputRecs,
                   0,
                   0,
                   1,
  		   t_total,
  		   (double) pvstp->totalViewTuples * 1.e-6 / t_total, 
  		   "Tuples generated", 
  		   verified,
  		   NPBVERSION,
  		   COMPILETIME,
  		   CC,
  		   CLINK,
  		   C_LIB,
  		   C_INC,
  		   CFLAGS,
  		   CLINKFLAGS); 
   return ADC_OK;
}

long long checksumS=464620213;
long long checksumWlo=434318;
long long checksumWhi=1401796;
long long checksumAlo=178042;
long long checksumAhi=7141688;
long long checksumBlo=700453;
long long checksumBhi=9348365;

int Verify(long long int checksum,ADC_VIEW_PARS *adcpp){
  switch(adcpp->clss){
    case 'S':
      if(checksum==checksumS) return 0;
      break;
    case 'W':
      if(checksum==checksumWlo+1000000*checksumWhi) return 0;
      break;
    case 'A':
      if(checksum==checksumAlo+1000000*checksumAhi) return 0;
      break;
    case 'B':
      if(checksum==checksumBlo+1000000*checksumBhi) return 0;
      break;
    default:
      return -1; /* CLASS U */
  }
  return 1;
}


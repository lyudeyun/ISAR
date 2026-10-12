#include <math.h>
#include "fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc.h"
#include "fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_private.h"
#include <stdio.h>
#include "slexec_vm_simstruct_bridge.h"
#include "slexec_vm_zc_functions.h"
#include "slexec_vm_lookup_functions.h"
#include "slsv_diagnostic_codegen_c_api.h"
#include "simtarget/slSimTgtMdlrefSfcnBridge.h"
#include "simstruc.h"
#include "fixedpoint.h"
#define CodeFormat S-Function
#define AccDefine1 Accelerator_S-Function
#include "simtarget/slAccSfcnBridge.h"
#ifndef __RTW_UTFREE__  
extern void * utMalloc ( size_t ) ; extern void utFree ( void * ) ;
#endif
boolean_T
fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_rt_TDelayUpdateTailOrGrowBuf (
int_T * bufSzPtr , int_T * tailPtr , int_T * headPtr , int_T * lastPtr ,
real_T tMinusDelay , real_T * * uBufPtr , boolean_T isfixedbuf , boolean_T
istransportdelay , int_T * maxNewBufSzPtr ) { int_T testIdx ; int_T tail = *
tailPtr ; int_T bufSz = * bufSzPtr ; real_T * tBuf = * uBufPtr + bufSz ;
real_T * xBuf = ( NULL ) ; int_T numBuffer = 2 ; if ( istransportdelay ) {
numBuffer = 3 ; xBuf = * uBufPtr + 2 * bufSz ; } testIdx = ( tail < ( bufSz -
1 ) ) ? ( tail + 1 ) : 0 ; if ( ( tMinusDelay <= tBuf [ testIdx ] ) && !
isfixedbuf ) { int_T j ; real_T * tempT ; real_T * tempU ; real_T * tempX = (
NULL ) ; real_T * uBuf = * uBufPtr ; int_T newBufSz = bufSz + 1024 ; if (
newBufSz > * maxNewBufSzPtr ) { * maxNewBufSzPtr = newBufSz ; } tempU = (
real_T * ) utMalloc ( numBuffer * newBufSz * sizeof ( real_T ) ) ; if ( tempU
== ( NULL ) ) { return ( false ) ; } tempT = tempU + newBufSz ; if (
istransportdelay ) tempX = tempT + newBufSz ; for ( j = tail ; j < bufSz ; j
++ ) { tempT [ j - tail ] = tBuf [ j ] ; tempU [ j - tail ] = uBuf [ j ] ; if
( istransportdelay ) tempX [ j - tail ] = xBuf [ j ] ; } for ( j = 0 ; j <
tail ; j ++ ) { tempT [ j + bufSz - tail ] = tBuf [ j ] ; tempU [ j + bufSz -
tail ] = uBuf [ j ] ; if ( istransportdelay ) tempX [ j + bufSz - tail ] =
xBuf [ j ] ; } if ( * lastPtr > tail ) { * lastPtr -= tail ; } else { *
lastPtr += ( bufSz - tail ) ; } * tailPtr = 0 ; * headPtr = bufSz ; utFree (
uBuf ) ; * bufSzPtr = newBufSz ; * uBufPtr = tempU ; } else { * tailPtr =
testIdx ; } return ( true ) ; } real_T
fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_rt_VTDelayfindtDInterpolate (
real_T x , real_T * uBuf , int_T bufSz , int_T head , int_T tail , int_T *
pLast , real_T t , real_T tStart , boolean_T discrete , boolean_T
minorStepAndTAtLastMajorOutput , real_T initOutput , real_T * appliedDelay )
{ int_T n , k ; real_T f ; int_T kp1 ; real_T tminustD , tL , tR , uD , uL ,
uR , fU ; real_T * tBuf = uBuf + bufSz ; real_T * xBuf = uBuf + 2 * bufSz ;
if ( minorStepAndTAtLastMajorOutput ) { if ( * pLast == head ) { * pLast = (
* pLast == 0 ) ? bufSz - 1 : * pLast - 1 ; } head = ( head == 0 ) ? bufSz - 1
: head - 1 ; } if ( x <= 1 ) { return initOutput ; } k = * pLast ; n = 0 ;
for ( ; ; ) { n ++ ; if ( n > bufSz ) break ; if ( x - xBuf [ k ] > 1.0 ) {
if ( k == head ) { int_T km1 ; f = ( x - 1.0 - xBuf [ k ] ) / ( x - xBuf [ k
] ) ; tminustD = ( 1.0 - f ) * tBuf [ k ] + f * t ; km1 = k - 1 ; if ( km1 <
0 ) km1 = bufSz - 1 ; tL = tBuf [ km1 ] ; tR = tBuf [ k ] ; uL = uBuf [ km1 ]
; uR = uBuf [ k ] ; break ; } kp1 = k + 1 ; if ( kp1 == bufSz ) kp1 = 0 ; if
( x - xBuf [ kp1 ] <= 1.0 ) { f = ( x - 1.0 - xBuf [ k ] ) / ( xBuf [ kp1 ] -
xBuf [ k ] ) ; tL = tBuf [ k ] ; tR = tBuf [ kp1 ] ; uL = uBuf [ k ] ; uR =
uBuf [ kp1 ] ; tminustD = ( 1.0 - f ) * tL + f * tR ; break ; } k = kp1 ; }
else { if ( k == tail ) { f = ( x - 1.0 ) / xBuf [ k ] ; if ( discrete ) {
return ( uBuf [ tail ] ) ; } kp1 = k + 1 ; if ( kp1 == bufSz ) kp1 = 0 ;
tminustD = ( 1 - f ) * tStart + f * tBuf [ k ] ; tL = tBuf [ k ] ; tR = tBuf
[ kp1 ] ; uL = uBuf [ k ] ; uR = uBuf [ kp1 ] ; break ; } k = k - 1 ; if ( k
< 0 ) k = bufSz - 1 ; } } * pLast = k ; if ( tR == tL ) { fU = 1.0 ; } else {
fU = ( tminustD - tL ) / ( tR - tL ) ; } if ( discrete ) { uD = ( fU > ( 1.0
- fU ) ) ? uR : uL ; } else { uD = ( 1.0 - fU ) * uL + fU * uR ; } *
appliedDelay = t - tminustD ; return uD ; } real_T look2_binlxpw ( real_T u0
, real_T u1 , const real_T bp0 [ ] , const real_T bp1 [ ] , const real_T
table [ ] , const uint32_T maxIndex [ ] , uint32_T stride ) { real_T
fractions [ 2 ] ; real_T frac ; real_T yL_0d0 ; real_T yL_0d1 ; uint32_T
bpIndices [ 2 ] ; uint32_T bpIdx ; uint32_T iLeft ; uint32_T iRght ; if ( u0
<= bp0 [ 0U ] ) { iLeft = 0U ; frac = ( u0 - bp0 [ 0U ] ) / ( bp0 [ 1U ] -
bp0 [ 0U ] ) ; } else if ( u0 < bp0 [ maxIndex [ 0U ] ] ) { bpIdx = maxIndex
[ 0U ] >> 1U ; iLeft = 0U ; iRght = maxIndex [ 0U ] ; while ( iRght - iLeft >
1U ) { if ( u0 < bp0 [ bpIdx ] ) { iRght = bpIdx ; } else { iLeft = bpIdx ; }
bpIdx = ( iRght + iLeft ) >> 1U ; } frac = ( u0 - bp0 [ iLeft ] ) / ( bp0 [
iLeft + 1U ] - bp0 [ iLeft ] ) ; } else { iLeft = maxIndex [ 0U ] - 1U ; frac
= ( u0 - bp0 [ maxIndex [ 0U ] - 1U ] ) / ( bp0 [ maxIndex [ 0U ] ] - bp0 [
maxIndex [ 0U ] - 1U ] ) ; } fractions [ 0U ] = frac ; bpIndices [ 0U ] =
iLeft ; if ( u1 <= bp1 [ 0U ] ) { iLeft = 0U ; frac = ( u1 - bp1 [ 0U ] ) / (
bp1 [ 1U ] - bp1 [ 0U ] ) ; } else if ( u1 < bp1 [ maxIndex [ 1U ] ] ) {
bpIdx = maxIndex [ 1U ] >> 1U ; iLeft = 0U ; iRght = maxIndex [ 1U ] ; while
( iRght - iLeft > 1U ) { if ( u1 < bp1 [ bpIdx ] ) { iRght = bpIdx ; } else {
iLeft = bpIdx ; } bpIdx = ( iRght + iLeft ) >> 1U ; } frac = ( u1 - bp1 [
iLeft ] ) / ( bp1 [ iLeft + 1U ] - bp1 [ iLeft ] ) ; } else { iLeft =
maxIndex [ 1U ] - 1U ; frac = ( u1 - bp1 [ maxIndex [ 1U ] - 1U ] ) / ( bp1 [
maxIndex [ 1U ] ] - bp1 [ maxIndex [ 1U ] - 1U ] ) ; } bpIdx = iLeft * stride
+ bpIndices [ 0U ] ; yL_0d0 = table [ bpIdx ] ; yL_0d0 += ( table [ bpIdx +
1U ] - yL_0d0 ) * fractions [ 0U ] ; bpIdx += stride ; yL_0d1 = table [ bpIdx
] ; return ( ( ( table [ bpIdx + 1U ] - yL_0d1 ) * fractions [ 0U ] + yL_0d1
) - yL_0d0 ) * frac + yL_0d0 ; } void rt_ssGetBlockPath ( SimStruct * S ,
int_T sysIdx , int_T blkIdx , char_T * * path ) { _ssGetBlockPath ( S ,
sysIdx , blkIdx , path ) ; } void rt_ssSet_slErrMsg ( void * S , void * diag
) { SimStruct * castedS = ( SimStruct * ) S ; if ( !
_ssIsErrorStatusAslErrMsg ( castedS ) ) { _ssSet_slErrMsg ( castedS , diag )
; } else { _ssDiscardDiagnostic ( castedS , diag ) ; } } void
rt_ssReportDiagnosticAsWarning ( void * S , void * diag ) {
_ssReportDiagnosticAsWarning ( ( SimStruct * ) S , diag ) ; } void
rt_ssReportDiagnosticAsInfo ( void * S , void * diag ) {
_ssReportDiagnosticAsInfo ( ( SimStruct * ) S , diag ) ; } static void
mdlOutputs ( SimStruct * S , int_T tid ) {
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtB ;
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtDW ;
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtP ;
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtX ; real_T B_1_38_0_0 ;
real_T B_1_39_0_0 ; real_T B_1_40_0_0 ; real_T B_1_41_0_0 ; real_T B_1_42_0_0
; real_T B_1_43_0_0 ; real_T B_1_44_0_0 ; real_T B_1_45_0_0 ; real_T
B_1_46_0_0 ; real_T B_1_47_0_0 ; real_T B_1_48_0_0 ; real_T B_1_49_0_0 ;
real_T B_1_50_0_0 ; real_T B_1_51_0_0 ; real_T B_1_52_0_0 ; real_T B_1_53_0_0
; real_T B_1_54_0_0 ; real_T B_1_55_0_0 ; real_T B_1_56_0_0 ; real_T
B_1_57_0_0 ; real_T B_1_58_0_0 ; real_T B_1_59_0_0 ; real_T B_1_60_0_0 ;
real_T B_1_61_0_0 ; real_T B_1_62_0_0 ; real_T B_1_63_0_0 ; real_T B_1_64_0_0
; real_T B_1_65_0_0 ; real_T B_1_66_0_0 ; real_T rtb_B_1_37_0 ; real_T u0 ;
int32_T i ; _rtDW = ( ( DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
ssGetRootDWork ( S ) ) ; _rtX = ( (
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetContStates ( S ) ) ;
_rtP = ( ( P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp ( S
) ) ; _rtB = ( ( B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
_ssGetModelBlockIO ( S ) ) ; _rtB -> B_1_0_0 = _rtX -> p00543bar_CSTATE ; if
( ( ( ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU ( S
) ) -> Engine_Speed > _rtP -> P_1 ) { _rtB -> B_1_1_0 = _rtP -> P_1 ; } else
if ( ( ( ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU (
S ) ) -> Engine_Speed < _rtP -> P_2 ) { _rtB -> B_1_1_0 = _rtP -> P_2 ; }
else { _rtB -> B_1_1_0 = ( (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU ( S ) ) ->
Engine_Speed ; } _rtB -> B_1_2_0 = _rtP -> P_3 * _rtB -> B_1_1_0 ; _rtB ->
B_1_4_0 = ( ( ( 0.08979 * _rtB -> B_1_0_0 * _rtB -> B_1_2_0 + - 0.366 ) -
0.0337 * _rtB -> B_1_2_0 * _rtB -> B_1_0_0 * _rtB -> B_1_0_0 ) + 0.0001 *
_rtB -> B_1_0_0 * _rtB -> B_1_2_0 * _rtB -> B_1_2_0 ) * _rtP -> P_150 ; i =
ssIsSampleHit ( S , 1 , 0 ) ; if ( i != 0 ) { { if ( _rtDW ->
TAQSigLogging_InsertedFor_IntakeManifold_at_outport_0_PWORK . AQHandles &&
ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_IntakeManifold_at_outport_0_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & _rtB -> B_1_4_0 + 0 ) ; } } { if (
_rtDW -> TAQSigLogging_InsertedFor_IntakeManifold_at_outport_1_PWORK .
AQHandles && ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_IntakeManifold_at_outport_1_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & _rtB -> B_1_0_0 + 0 ) ; } } } _rtB ->
B_1_7_0 = 0.0 ; _rtB -> B_1_7_0 += _rtP -> P_5 * _rtX -> Throttledelay_CSTATE
; _rtB -> B_1_8_0 = _rtB -> B_1_7_0 + _rtB -> B_1_1_0_c ; if ( _rtB ->
B_1_8_0 > _rtP -> P_6 ) { _rtB -> B_1_9_0 = _rtP -> P_6 ; } else if ( _rtB ->
B_1_8_0 < _rtP -> P_7 ) { _rtB -> B_1_9_0 = _rtP -> P_7 ; } else { _rtB ->
B_1_9_0 = _rtB -> B_1_8_0 ; } _rtB -> B_1_11_0 = _rtB -> B_1_0_0 / _rtB ->
B_1_0_0_m ; _rtB -> B_1_12_0 = 1.0 / _rtB -> B_1_0_0 * _rtB -> B_1_0_0_m ;
_rtB -> B_1_13_0 = muDoubleScalarMin ( _rtB -> B_1_11_0 , _rtB -> B_1_12_0 )
; if ( _rtB -> B_1_13_0 >= _rtP -> P_8 ) { rtb_B_1_37_0 = _rtB -> B_1_13_0 -
_rtB -> B_1_13_0 * _rtB -> B_1_13_0 ; if ( rtb_B_1_37_0 < 0.0 ) {
rtb_B_1_37_0 = - muDoubleScalarSqrt ( - rtb_B_1_37_0 ) ; } else {
rtb_B_1_37_0 = muDoubleScalarSqrt ( rtb_B_1_37_0 ) ; } _rtB -> B_1_15_0 = 2.0
* rtb_B_1_37_0 ; } else { _rtB -> B_1_15_0 = _rtB -> B_1_101_0 ; } _rtB ->
B_1_16_0 = _rtB -> B_1_0_0_m - _rtB -> B_1_0_0 ; _rtB -> B_1_17_0 =
muDoubleScalarSign ( _rtB -> B_1_16_0 ) ; _rtB -> B_1_18_0 = ( ( ( 2.821 -
0.05231 * _rtB -> B_1_9_0 ) + 0.10299 * _rtB -> B_1_9_0 * _rtB -> B_1_9_0 ) -
0.00063 * _rtB -> B_1_9_0 * _rtB -> B_1_9_0 * _rtB -> B_1_9_0 ) * _rtB ->
B_1_15_0 * _rtB -> B_1_17_0 ; i = ssIsSampleHit ( S , 1 , 0 ) ; if ( i != 0 )
{ { if ( _rtDW -> TAQSigLogging_InsertedFor_Throttle_at_outport_0_PWORK .
AQHandles && ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_Throttle_at_outport_0_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & _rtB -> B_1_18_0 + 0 ) ; } } } _rtB ->
B_1_20_0 = _rtP -> P_9 * _rtB -> B_1_2_0 ; _rtB -> B_1_21_0 = _rtB -> B_1_4_0
/ _rtB -> B_1_2_0 ; _rtB -> B_1_22_0 = _rtP -> P_10 * _rtB -> B_1_21_0 ; _rtB
-> B_1_24_0 = _rtP -> P_14 * look2_binlxpw ( _rtB -> B_1_20_0 , _rtB ->
B_1_22_0 , _rtP -> P_12 , _rtP -> P_13 , _rtP -> P_11 , _rtP -> P_151 , 5U )
; _rtB -> B_1_25_0_e = ( real32_T ) _rtB -> B_1_2_0 ; _rtB -> B_1_26_0_b = (
real32_T ) _rtB -> B_1_9_0 ; _rtB -> B_1_27_0 = _rtP -> P_15 * _rtB ->
B_1_18_0 ; _rtB -> B_1_28_0_j = ( real32_T ) _rtB -> B_1_27_0 ; _rtB ->
B_1_30_0 = _rtX -> Integrator_CSTATE ; _rtB -> B_1_31_0 = _rtP -> P_20 * _rtB
-> B_1_30_0 ; if ( ssGetTaskTime ( S , 0 ) < _rtP -> P_16 ) { rtb_B_1_37_0 =
_rtP -> P_17 ; } else { rtb_B_1_37_0 = _rtP -> P_18 ; } if ( rtb_B_1_37_0 >=
_rtP -> P_21 ) { _rtB -> B_1_32_0 = _rtB -> B_1_5_0 ; } else { _rtB ->
B_1_32_0 = _rtB -> B_1_31_0 ; } _rtB -> B_1_33_0 = _rtP -> P_22 * _rtB ->
B_1_32_0 ; _rtB -> B_1_34_0_f = ( real32_T ) _rtB -> B_1_33_0 ; _rtB ->
B_1_35_0 [ 0 ] = _rtB -> B_1_25_0_e - _rtB -> B_1_2_0_k [ 0 ] ; _rtB ->
B_1_35_0 [ 1 ] = _rtB -> B_1_26_0_b - _rtB -> B_1_2_0_k [ 1 ] ; _rtB ->
B_1_35_0 [ 2 ] = _rtB -> B_1_28_0_j - _rtB -> B_1_2_0_k [ 2 ] ; _rtB ->
B_1_35_0 [ 3 ] = _rtB -> B_1_34_0_f - _rtB -> B_1_2_0_k [ 3 ] ; rtb_B_1_37_0
= 0.0 ; B_1_38_0_0 = 0.0 ; B_1_39_0_0 = 0.0 ; B_1_40_0_0 = 0.0 ; B_1_41_0_0 =
0.0 ; B_1_42_0_0 = 0.0 ; B_1_43_0_0 = 0.0 ; B_1_44_0_0 = 0.0 ; B_1_45_0_0 =
0.0 ; B_1_46_0_0 = 0.0 ; B_1_47_0_0 = 0.0 ; B_1_48_0_0 = 0.0 ; B_1_49_0_0 =
0.0 ; B_1_50_0_0 = 0.0 ; B_1_51_0_0 = 0.0 ; B_1_52_0_0 = 0.0 ; B_1_53_0_0 =
0.0 ; B_1_54_0_0 = 0.0 ; B_1_55_0_0 = 0.0 ; B_1_56_0_0 = 0.0 ; B_1_57_0_0 =
0.0 ; B_1_58_0_0 = 0.0 ; B_1_59_0_0 = 0.0 ; B_1_60_0_0 = 0.0 ; B_1_61_0_0 =
0.0 ; B_1_62_0_0 = 0.0 ; B_1_63_0_0 = 0.0 ; B_1_64_0_0 = 0.0 ; B_1_65_0_0 =
0.0 ; B_1_66_0_0 = 0.0 ; for ( i = 0 ; i < 4 ; i ++ ) { _rtB -> B_1_36_0 [ i
] = 0.0 ; _rtB -> B_1_36_0 [ i ] += _rtP -> P_23 [ i ] * _rtB -> B_1_35_0 [ 0
] ; _rtB -> B_1_36_0 [ i ] += _rtP -> P_23 [ i + 4 ] * _rtB -> B_1_35_0 [ 1 ]
; _rtB -> B_1_36_0 [ i ] += _rtP -> P_23 [ i + 8 ] * _rtB -> B_1_35_0 [ 2 ] ;
_rtB -> B_1_36_0 [ i ] += _rtP -> P_23 [ i + 12 ] * _rtB -> B_1_35_0 [ 3 ] ;
rtb_B_1_37_0 += _rtB -> B_1_6_0 [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_38_0_0
+= _rtB -> B_1_17_0_p [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_39_0_0 += _rtB ->
B_1_28_0 [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_40_0_0 += _rtB -> B_1_30_0_d [
i ] * _rtB -> B_1_36_0 [ i ] ; B_1_41_0_0 += _rtB -> B_1_31_0_l [ i ] * _rtB
-> B_1_36_0 [ i ] ; B_1_42_0_0 += _rtB -> B_1_32_0_o [ i ] * _rtB -> B_1_36_0
[ i ] ; B_1_43_0_0 += _rtB -> B_1_33_0_b [ i ] * _rtB -> B_1_36_0 [ i ] ;
B_1_44_0_0 += _rtB -> B_1_34_0 [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_45_0_0 +=
_rtB -> B_1_35_0_n [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_46_0_0 += _rtB ->
B_1_7_0_b [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_47_0_0 += _rtB -> B_1_8_0_p [
i ] * _rtB -> B_1_36_0 [ i ] ; B_1_48_0_0 += _rtB -> B_1_9_0_c [ i ] * _rtB
-> B_1_36_0 [ i ] ; B_1_49_0_0 += _rtB -> B_1_10_0 [ i ] * _rtB -> B_1_36_0 [
i ] ; B_1_50_0_0 += _rtB -> B_1_11_0_f [ i ] * _rtB -> B_1_36_0 [ i ] ;
B_1_51_0_0 += _rtB -> B_1_12_0_g [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_52_0_0
+= _rtB -> B_1_13_0_g [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_53_0_0 += _rtB ->
B_1_14_0 [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_54_0_0 += _rtB -> B_1_15_0_m [
i ] * _rtB -> B_1_36_0 [ i ] ; B_1_55_0_0 += _rtB -> B_1_16_0_n [ i ] * _rtB
-> B_1_36_0 [ i ] ; B_1_56_0_0 += _rtB -> B_1_18_0_l [ i ] * _rtB -> B_1_36_0
[ i ] ; B_1_57_0_0 += _rtB -> B_1_19_0 [ i ] * _rtB -> B_1_36_0 [ i ] ;
B_1_58_0_0 += _rtB -> B_1_20_0_j [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_59_0_0
+= _rtB -> B_1_21_0_d [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_60_0_0 += _rtB ->
B_1_22_0_g [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_61_0_0 += _rtB -> B_1_23_0 [
i ] * _rtB -> B_1_36_0 [ i ] ; B_1_62_0_0 += _rtB -> B_1_24_0_l [ i ] * _rtB
-> B_1_36_0 [ i ] ; B_1_63_0_0 += _rtB -> B_1_25_0 [ i ] * _rtB -> B_1_36_0 [
i ] ; B_1_64_0_0 += _rtB -> B_1_26_0 [ i ] * _rtB -> B_1_36_0 [ i ] ;
B_1_65_0_0 += _rtB -> B_1_27_0_d [ i ] * _rtB -> B_1_36_0 [ i ] ; B_1_66_0_0
+= _rtB -> B_1_29_0 [ i ] * _rtB -> B_1_36_0 [ i ] ; } _rtB -> B_1_67_0 [ 0 ]
= rtb_B_1_37_0 + _rtB -> B_1_36_0_b [ 0 ] ; _rtB -> B_1_67_0 [ 1 ] =
B_1_38_0_0 + _rtB -> B_1_36_0_b [ 1 ] ; _rtB -> B_1_67_0 [ 2 ] = B_1_39_0_0 +
_rtB -> B_1_36_0_b [ 2 ] ; _rtB -> B_1_67_0 [ 3 ] = B_1_40_0_0 + _rtB ->
B_1_36_0_b [ 3 ] ; _rtB -> B_1_67_0 [ 4 ] = B_1_41_0_0 + _rtB -> B_1_36_0_b [
4 ] ; _rtB -> B_1_67_0 [ 5 ] = B_1_42_0_0 + _rtB -> B_1_36_0_b [ 5 ] ; _rtB
-> B_1_67_0 [ 6 ] = B_1_43_0_0 + _rtB -> B_1_36_0_b [ 6 ] ; _rtB -> B_1_67_0
[ 7 ] = B_1_44_0_0 + _rtB -> B_1_36_0_b [ 7 ] ; _rtB -> B_1_67_0 [ 8 ] =
B_1_45_0_0 + _rtB -> B_1_36_0_b [ 8 ] ; _rtB -> B_1_67_0 [ 9 ] = B_1_46_0_0 +
_rtB -> B_1_36_0_b [ 9 ] ; _rtB -> B_1_67_0 [ 10 ] = B_1_47_0_0 + _rtB ->
B_1_36_0_b [ 10 ] ; _rtB -> B_1_67_0 [ 11 ] = B_1_48_0_0 + _rtB -> B_1_36_0_b
[ 11 ] ; _rtB -> B_1_67_0 [ 12 ] = B_1_49_0_0 + _rtB -> B_1_36_0_b [ 12 ] ;
_rtB -> B_1_67_0 [ 13 ] = B_1_50_0_0 + _rtB -> B_1_36_0_b [ 13 ] ; _rtB ->
B_1_67_0 [ 14 ] = B_1_51_0_0 + _rtB -> B_1_36_0_b [ 14 ] ; _rtB -> B_1_67_0 [
15 ] = B_1_52_0_0 + _rtB -> B_1_36_0_b [ 15 ] ; _rtB -> B_1_67_0 [ 16 ] =
B_1_53_0_0 + _rtB -> B_1_36_0_b [ 16 ] ; _rtB -> B_1_67_0 [ 17 ] = B_1_54_0_0
+ _rtB -> B_1_36_0_b [ 17 ] ; _rtB -> B_1_67_0 [ 18 ] = B_1_55_0_0 + _rtB ->
B_1_36_0_b [ 18 ] ; _rtB -> B_1_67_0 [ 19 ] = B_1_56_0_0 + _rtB -> B_1_36_0_b
[ 19 ] ; _rtB -> B_1_67_0 [ 20 ] = B_1_57_0_0 + _rtB -> B_1_36_0_b [ 20 ] ;
_rtB -> B_1_67_0 [ 21 ] = B_1_58_0_0 + _rtB -> B_1_36_0_b [ 21 ] ; _rtB ->
B_1_67_0 [ 22 ] = B_1_59_0_0 + _rtB -> B_1_36_0_b [ 22 ] ; _rtB -> B_1_67_0 [
23 ] = B_1_60_0_0 + _rtB -> B_1_36_0_b [ 23 ] ; _rtB -> B_1_67_0 [ 24 ] =
B_1_61_0_0 + _rtB -> B_1_36_0_b [ 24 ] ; _rtB -> B_1_67_0 [ 25 ] = B_1_62_0_0
+ _rtB -> B_1_36_0_b [ 25 ] ; _rtB -> B_1_67_0 [ 26 ] = B_1_63_0_0 + _rtB ->
B_1_36_0_b [ 26 ] ; _rtB -> B_1_67_0 [ 27 ] = B_1_64_0_0 + _rtB -> B_1_36_0_b
[ 27 ] ; _rtB -> B_1_67_0 [ 28 ] = B_1_65_0_0 + _rtB -> B_1_36_0_b [ 28 ] ;
_rtB -> B_1_67_0 [ 29 ] = B_1_66_0_0 + _rtB -> B_1_36_0_b [ 29 ] ;
rtb_B_1_37_0 = 0.0 ; B_1_38_0_0 = 0.0 ; B_1_39_0_0 = 0.0 ; B_1_40_0_0 = 0.0 ;
B_1_41_0_0 = 0.0 ; B_1_42_0_0 = 0.0 ; B_1_43_0_0 = 0.0 ; B_1_44_0_0 = 0.0 ;
B_1_45_0_0 = 0.0 ; B_1_46_0_0 = 0.0 ; B_1_47_0_0 = 0.0 ; B_1_48_0_0 = 0.0 ;
B_1_49_0_0 = 0.0 ; B_1_50_0_0 = 0.0 ; B_1_51_0_0 = 0.0 ; B_1_52_0_0 = 0.0 ;
B_1_53_0_0 = 0.0 ; B_1_54_0_0 = 0.0 ; B_1_55_0_0 = 0.0 ; B_1_56_0_0 = 0.0 ;
B_1_57_0_0 = 0.0 ; B_1_58_0_0 = 0.0 ; B_1_59_0_0 = 0.0 ; B_1_60_0_0 = 0.0 ;
B_1_61_0_0 = 0.0 ; B_1_62_0_0 = 0.0 ; B_1_63_0_0 = 0.0 ; B_1_64_0_0 = 0.0 ;
B_1_65_0_0 = 0.0 ; B_1_66_0_0 = 0.0 ; for ( i = 0 ; i < 30 ; i ++ ) { u0 =
_rtB -> B_1_67_0 [ i ] ; if ( u0 > _rtP -> P_24 ) { _rtB -> B_1_68_0 [ i ] =
_rtP -> P_24 ; } else if ( u0 < _rtP -> P_25 ) { _rtB -> B_1_68_0 [ i ] =
_rtP -> P_25 ; } else { _rtB -> B_1_68_0 [ i ] = u0 ; } rtb_B_1_37_0 += _rtB
-> B_1_37_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_38_0_0 += _rtB -> B_1_48_0 [
i ] * _rtB -> B_1_68_0 [ i ] ; B_1_39_0_0 += _rtB -> B_1_59_0 [ i ] * _rtB ->
B_1_68_0 [ i ] ; B_1_40_0_0 += _rtB -> B_1_61_0 [ i ] * _rtB -> B_1_68_0 [ i
] ; B_1_41_0_0 += _rtB -> B_1_62_0 [ i ] * _rtB -> B_1_68_0 [ i ] ;
B_1_42_0_0 += _rtB -> B_1_63_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_43_0_0 +=
_rtB -> B_1_64_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_44_0_0 += _rtB ->
B_1_65_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_45_0_0 += _rtB -> B_1_66_0 [ i
] * _rtB -> B_1_68_0 [ i ] ; B_1_46_0_0 += _rtB -> B_1_38_0 [ i ] * _rtB ->
B_1_68_0 [ i ] ; B_1_47_0_0 += _rtB -> B_1_39_0 [ i ] * _rtB -> B_1_68_0 [ i
] ; B_1_48_0_0 += _rtB -> B_1_40_0 [ i ] * _rtB -> B_1_68_0 [ i ] ;
B_1_49_0_0 += _rtB -> B_1_41_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_50_0_0 +=
_rtB -> B_1_42_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_51_0_0 += _rtB ->
B_1_43_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_52_0_0 += _rtB -> B_1_44_0 [ i
] * _rtB -> B_1_68_0 [ i ] ; B_1_53_0_0 += _rtB -> B_1_45_0 [ i ] * _rtB ->
B_1_68_0 [ i ] ; B_1_54_0_0 += _rtB -> B_1_46_0 [ i ] * _rtB -> B_1_68_0 [ i
] ; B_1_55_0_0 += _rtB -> B_1_47_0 [ i ] * _rtB -> B_1_68_0 [ i ] ;
B_1_56_0_0 += _rtB -> B_1_49_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_57_0_0 +=
_rtB -> B_1_50_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_58_0_0 += _rtB ->
B_1_51_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_59_0_0 += _rtB -> B_1_52_0 [ i
] * _rtB -> B_1_68_0 [ i ] ; B_1_60_0_0 += _rtB -> B_1_53_0 [ i ] * _rtB ->
B_1_68_0 [ i ] ; B_1_61_0_0 += _rtB -> B_1_54_0 [ i ] * _rtB -> B_1_68_0 [ i
] ; B_1_62_0_0 += _rtB -> B_1_55_0 [ i ] * _rtB -> B_1_68_0 [ i ] ;
B_1_63_0_0 += _rtB -> B_1_56_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_64_0_0 +=
_rtB -> B_1_57_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_65_0_0 += _rtB ->
B_1_58_0 [ i ] * _rtB -> B_1_68_0 [ i ] ; B_1_66_0_0 += _rtB -> B_1_60_0 [ i
] * _rtB -> B_1_68_0 [ i ] ; } _rtB -> B_1_99_0 [ 0 ] = rtb_B_1_37_0 + _rtB
-> B_1_67_0_l [ 0 ] ; _rtB -> B_1_99_0 [ 1 ] = B_1_38_0_0 + _rtB ->
B_1_67_0_l [ 1 ] ; _rtB -> B_1_99_0 [ 2 ] = B_1_39_0_0 + _rtB -> B_1_67_0_l [
2 ] ; _rtB -> B_1_99_0 [ 3 ] = B_1_40_0_0 + _rtB -> B_1_67_0_l [ 3 ] ; _rtB
-> B_1_99_0 [ 4 ] = B_1_41_0_0 + _rtB -> B_1_67_0_l [ 4 ] ; _rtB -> B_1_99_0
[ 5 ] = B_1_42_0_0 + _rtB -> B_1_67_0_l [ 5 ] ; _rtB -> B_1_99_0 [ 6 ] =
B_1_43_0_0 + _rtB -> B_1_67_0_l [ 6 ] ; _rtB -> B_1_99_0 [ 7 ] = B_1_44_0_0 +
_rtB -> B_1_67_0_l [ 7 ] ; _rtB -> B_1_99_0 [ 8 ] = B_1_45_0_0 + _rtB ->
B_1_67_0_l [ 8 ] ; _rtB -> B_1_99_0 [ 9 ] = B_1_46_0_0 + _rtB -> B_1_67_0_l [
9 ] ; _rtB -> B_1_99_0 [ 10 ] = B_1_47_0_0 + _rtB -> B_1_67_0_l [ 10 ] ; _rtB
-> B_1_99_0 [ 11 ] = B_1_48_0_0 + _rtB -> B_1_67_0_l [ 11 ] ; _rtB ->
B_1_99_0 [ 12 ] = B_1_49_0_0 + _rtB -> B_1_67_0_l [ 12 ] ; _rtB -> B_1_99_0 [
13 ] = B_1_50_0_0 + _rtB -> B_1_67_0_l [ 13 ] ; _rtB -> B_1_99_0 [ 14 ] =
B_1_51_0_0 + _rtB -> B_1_67_0_l [ 14 ] ; _rtB -> B_1_99_0 [ 15 ] = B_1_52_0_0
+ _rtB -> B_1_67_0_l [ 15 ] ; _rtB -> B_1_99_0 [ 16 ] = B_1_53_0_0 + _rtB ->
B_1_67_0_l [ 16 ] ; _rtB -> B_1_99_0 [ 17 ] = B_1_54_0_0 + _rtB -> B_1_67_0_l
[ 17 ] ; _rtB -> B_1_99_0 [ 18 ] = B_1_55_0_0 + _rtB -> B_1_67_0_l [ 18 ] ;
_rtB -> B_1_99_0 [ 19 ] = B_1_56_0_0 + _rtB -> B_1_67_0_l [ 19 ] ; _rtB ->
B_1_99_0 [ 20 ] = B_1_57_0_0 + _rtB -> B_1_67_0_l [ 20 ] ; _rtB -> B_1_99_0 [
21 ] = B_1_58_0_0 + _rtB -> B_1_67_0_l [ 21 ] ; _rtB -> B_1_99_0 [ 22 ] =
B_1_59_0_0 + _rtB -> B_1_67_0_l [ 22 ] ; _rtB -> B_1_99_0 [ 23 ] = B_1_60_0_0
+ _rtB -> B_1_67_0_l [ 23 ] ; _rtB -> B_1_99_0 [ 24 ] = B_1_61_0_0 + _rtB ->
B_1_67_0_l [ 24 ] ; _rtB -> B_1_99_0 [ 25 ] = B_1_62_0_0 + _rtB -> B_1_67_0_l
[ 25 ] ; _rtB -> B_1_99_0 [ 26 ] = B_1_63_0_0 + _rtB -> B_1_67_0_l [ 26 ] ;
_rtB -> B_1_99_0 [ 27 ] = B_1_64_0_0 + _rtB -> B_1_67_0_l [ 27 ] ; _rtB ->
B_1_99_0 [ 28 ] = B_1_65_0_0 + _rtB -> B_1_67_0_l [ 28 ] ; _rtB -> B_1_99_0 [
29 ] = B_1_66_0_0 + _rtB -> B_1_67_0_l [ 29 ] ; rtb_B_1_37_0 = 0.0 ;
B_1_38_0_0 = 0.0 ; B_1_39_0_0 = 0.0 ; B_1_40_0_0 = 0.0 ; B_1_41_0_0 = 0.0 ;
B_1_42_0_0 = 0.0 ; B_1_43_0_0 = 0.0 ; B_1_44_0_0 = 0.0 ; B_1_45_0_0 = 0.0 ;
B_1_46_0_0 = 0.0 ; B_1_47_0_0 = 0.0 ; B_1_48_0_0 = 0.0 ; B_1_49_0_0 = 0.0 ;
B_1_50_0_0 = 0.0 ; B_1_51_0_0 = 0.0 ; B_1_52_0_0 = 0.0 ; B_1_53_0_0 = 0.0 ;
B_1_54_0_0 = 0.0 ; B_1_55_0_0 = 0.0 ; B_1_56_0_0 = 0.0 ; B_1_57_0_0 = 0.0 ;
B_1_58_0_0 = 0.0 ; B_1_59_0_0 = 0.0 ; B_1_60_0_0 = 0.0 ; B_1_61_0_0 = 0.0 ;
B_1_62_0_0 = 0.0 ; B_1_63_0_0 = 0.0 ; B_1_64_0_0 = 0.0 ; B_1_65_0_0 = 0.0 ;
B_1_66_0_0 = 0.0 ; for ( i = 0 ; i < 30 ; i ++ ) { u0 = _rtB -> B_1_99_0 [ i
] ; if ( u0 > _rtP -> P_26 ) { _rtB -> B_1_100_0 [ i ] = _rtP -> P_26 ; }
else if ( u0 < _rtP -> P_27 ) { _rtB -> B_1_100_0 [ i ] = _rtP -> P_27 ; }
else { _rtB -> B_1_100_0 [ i ] = u0 ; } rtb_B_1_37_0 += _rtB -> B_1_68_0_h [
i ] * _rtB -> B_1_100_0 [ i ] ; B_1_38_0_0 += _rtB -> B_1_79_0 [ i ] * _rtB
-> B_1_100_0 [ i ] ; B_1_39_0_0 += _rtB -> B_1_90_0 [ i ] * _rtB -> B_1_100_0
[ i ] ; B_1_40_0_0 += _rtB -> B_1_92_0 [ i ] * _rtB -> B_1_100_0 [ i ] ;
B_1_41_0_0 += _rtB -> B_1_93_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_42_0_0
+= _rtB -> B_1_94_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_43_0_0 += _rtB ->
B_1_95_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_44_0_0 += _rtB -> B_1_96_0 [ i
] * _rtB -> B_1_100_0 [ i ] ; B_1_45_0_0 += _rtB -> B_1_97_0 [ i ] * _rtB ->
B_1_100_0 [ i ] ; B_1_46_0_0 += _rtB -> B_1_69_0 [ i ] * _rtB -> B_1_100_0 [
i ] ; B_1_47_0_0 += _rtB -> B_1_70_0 [ i ] * _rtB -> B_1_100_0 [ i ] ;
B_1_48_0_0 += _rtB -> B_1_71_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_49_0_0
+= _rtB -> B_1_72_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_50_0_0 += _rtB ->
B_1_73_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_51_0_0 += _rtB -> B_1_74_0 [ i
] * _rtB -> B_1_100_0 [ i ] ; B_1_52_0_0 += _rtB -> B_1_75_0 [ i ] * _rtB ->
B_1_100_0 [ i ] ; B_1_53_0_0 += _rtB -> B_1_76_0 [ i ] * _rtB -> B_1_100_0 [
i ] ; B_1_54_0_0 += _rtB -> B_1_77_0 [ i ] * _rtB -> B_1_100_0 [ i ] ;
B_1_55_0_0 += _rtB -> B_1_78_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_56_0_0
+= _rtB -> B_1_80_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_57_0_0 += _rtB ->
B_1_81_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_58_0_0 += _rtB -> B_1_82_0 [ i
] * _rtB -> B_1_100_0 [ i ] ; B_1_59_0_0 += _rtB -> B_1_83_0 [ i ] * _rtB ->
B_1_100_0 [ i ] ; B_1_60_0_0 += _rtB -> B_1_84_0 [ i ] * _rtB -> B_1_100_0 [
i ] ; B_1_61_0_0 += _rtB -> B_1_85_0 [ i ] * _rtB -> B_1_100_0 [ i ] ;
B_1_62_0_0 += _rtB -> B_1_86_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_63_0_0
+= _rtB -> B_1_87_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_64_0_0 += _rtB ->
B_1_88_0 [ i ] * _rtB -> B_1_100_0 [ i ] ; B_1_65_0_0 += _rtB -> B_1_89_0 [ i
] * _rtB -> B_1_100_0 [ i ] ; B_1_66_0_0 += _rtB -> B_1_91_0 [ i ] * _rtB ->
B_1_100_0 [ i ] ; } _rtB -> B_1_131_0 [ 0 ] = rtb_B_1_37_0 + _rtB -> B_1_98_0
[ 0 ] ; _rtB -> B_1_131_0 [ 1 ] = B_1_38_0_0 + _rtB -> B_1_98_0 [ 1 ] ; _rtB
-> B_1_131_0 [ 2 ] = B_1_39_0_0 + _rtB -> B_1_98_0 [ 2 ] ; _rtB -> B_1_131_0
[ 3 ] = B_1_40_0_0 + _rtB -> B_1_98_0 [ 3 ] ; _rtB -> B_1_131_0 [ 4 ] =
B_1_41_0_0 + _rtB -> B_1_98_0 [ 4 ] ; _rtB -> B_1_131_0 [ 5 ] = B_1_42_0_0 +
_rtB -> B_1_98_0 [ 5 ] ; _rtB -> B_1_131_0 [ 6 ] = B_1_43_0_0 + _rtB ->
B_1_98_0 [ 6 ] ; _rtB -> B_1_131_0 [ 7 ] = B_1_44_0_0 + _rtB -> B_1_98_0 [ 7
] ; _rtB -> B_1_131_0 [ 8 ] = B_1_45_0_0 + _rtB -> B_1_98_0 [ 8 ] ; _rtB ->
B_1_131_0 [ 9 ] = B_1_46_0_0 + _rtB -> B_1_98_0 [ 9 ] ; _rtB -> B_1_131_0 [
10 ] = B_1_47_0_0 + _rtB -> B_1_98_0 [ 10 ] ; _rtB -> B_1_131_0 [ 11 ] =
B_1_48_0_0 + _rtB -> B_1_98_0 [ 11 ] ; _rtB -> B_1_131_0 [ 12 ] = B_1_49_0_0
+ _rtB -> B_1_98_0 [ 12 ] ; _rtB -> B_1_131_0 [ 13 ] = B_1_50_0_0 + _rtB ->
B_1_98_0 [ 13 ] ; _rtB -> B_1_131_0 [ 14 ] = B_1_51_0_0 + _rtB -> B_1_98_0 [
14 ] ; _rtB -> B_1_131_0 [ 15 ] = B_1_52_0_0 + _rtB -> B_1_98_0 [ 15 ] ; _rtB
-> B_1_131_0 [ 16 ] = B_1_53_0_0 + _rtB -> B_1_98_0 [ 16 ] ; _rtB ->
B_1_131_0 [ 17 ] = B_1_54_0_0 + _rtB -> B_1_98_0 [ 17 ] ; _rtB -> B_1_131_0 [
18 ] = B_1_55_0_0 + _rtB -> B_1_98_0 [ 18 ] ; _rtB -> B_1_131_0 [ 19 ] =
B_1_56_0_0 + _rtB -> B_1_98_0 [ 19 ] ; _rtB -> B_1_131_0 [ 20 ] = B_1_57_0_0
+ _rtB -> B_1_98_0 [ 20 ] ; _rtB -> B_1_131_0 [ 21 ] = B_1_58_0_0 + _rtB ->
B_1_98_0 [ 21 ] ; _rtB -> B_1_131_0 [ 22 ] = B_1_59_0_0 + _rtB -> B_1_98_0 [
22 ] ; _rtB -> B_1_131_0 [ 23 ] = B_1_60_0_0 + _rtB -> B_1_98_0 [ 23 ] ; _rtB
-> B_1_131_0 [ 24 ] = B_1_61_0_0 + _rtB -> B_1_98_0 [ 24 ] ; _rtB ->
B_1_131_0 [ 25 ] = B_1_62_0_0 + _rtB -> B_1_98_0 [ 25 ] ; _rtB -> B_1_131_0 [
26 ] = B_1_63_0_0 + _rtB -> B_1_98_0 [ 26 ] ; _rtB -> B_1_131_0 [ 27 ] =
B_1_64_0_0 + _rtB -> B_1_98_0 [ 27 ] ; _rtB -> B_1_131_0 [ 28 ] = B_1_65_0_0
+ _rtB -> B_1_98_0 [ 28 ] ; _rtB -> B_1_131_0 [ 29 ] = B_1_66_0_0 + _rtB ->
B_1_98_0 [ 29 ] ; rtb_B_1_37_0 = 0.0 ; for ( i = 0 ; i < 30 ; i ++ ) { u0 =
_rtB -> B_1_131_0 [ i ] ; if ( u0 > _rtP -> P_28 ) { _rtB -> B_1_132_0 [ i ]
= _rtP -> P_28 ; } else if ( u0 < _rtP -> P_29 ) { _rtB -> B_1_132_0 [ i ] =
_rtP -> P_29 ; } else { _rtB -> B_1_132_0 [ i ] = u0 ; } rtb_B_1_37_0 += _rtB
-> B_1_99_0_b [ i ] * _rtB -> B_1_132_0 [ i ] ; } _rtB -> B_1_134_0 =
rtb_B_1_37_0 + _rtB -> B_1_100_0_d ; _rtB -> B_1_135_0 = _rtB -> B_1_134_0 /
_rtB -> B_1_3_0 ; _rtB -> B_1_136_0 = _rtB -> B_1_4_0_c + _rtB -> B_1_135_0 ;
_rtB -> B_1_137_0 = _rtB -> B_1_136_0 ; _rtB -> B_1_138_0 = _rtP -> P_30 *
_rtB -> B_1_137_0 ; _rtB -> B_1_139_0 = _rtB -> B_1_24_0 * _rtB -> B_1_138_0
; _rtB -> B_1_140_0 = _rtX -> Integrator_CSTATE_c ; _rtB -> B_1_142_0 = _rtP
-> P_35 * look2_binlxpw ( _rtB -> B_1_20_0 , _rtB -> B_1_22_0 , _rtP -> P_33
, _rtP -> P_34 , _rtP -> P_32 , _rtP -> P_152 , 5U ) ; _rtB -> B_1_143_0 =
_rtB -> B_1_140_0 / _rtB -> B_1_142_0 ; _rtB -> B_1_144_0 = _rtB -> B_1_139_0
+ _rtB -> B_1_143_0 ; i = ssIsSampleHit ( S , 1 , 0 ) ; if ( i != 0 ) { { if
( _rtDW -> TAQSigLogging_InsertedFor_Wallwetting_at_outport_0_PWORK .
AQHandles && ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_Wallwetting_at_outport_0_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & _rtB -> B_1_144_0 + 0 ) ; } } } _rtB
-> B_1_146_0 = _rtX -> Integrator_CSTATE_h ; _rtB -> B_1_147_0 = _rtB ->
B_1_146_0 - _rtB -> B_1_30_0 ; _rtB -> B_1_148_0 = _rtP -> P_37 * _rtB ->
B_1_147_0 ; _rtB -> B_1_149_0 = _rtB -> B_1_4_0 / _rtB -> B_1_144_0 ; {
real_T * * uBuffer = ( real_T * * ) & _rtDW -> fuelsystemtransportdelay_PWORK
. TUbufferPtrs [ 0 ] ; real_T simTime = ssGetT ( S ) ; real_T appliedDelay ;
_rtB -> B_1_150_0 =
fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_rt_VTDelayfindtDInterpolate ( ( (
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetContStates ( S ) ) ->
fuelsystemtransportdelay_CSTATE , * uBuffer , _rtDW ->
fuelsystemtransportdelay_IWORK . CircularBufSize , _rtDW ->
fuelsystemtransportdelay_IWORK . Head , _rtDW ->
fuelsystemtransportdelay_IWORK . Tail , & _rtDW ->
fuelsystemtransportdelay_IWORK . Last , simTime , 0.0 , 0 , ( boolean_T ) (
ssIsMinorTimeStep ( S ) && ( ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) )
, _rtP -> P_39 , & appliedDelay ) ; } _rtB -> B_1_151_0 = _rtB -> B_1_150_0 -
_rtB -> B_1_146_0 ; _rtB -> B_1_152_0 = _rtP -> P_40 * _rtB -> B_1_151_0 ;
_rtB -> B_1_153_0 = _rtP -> P_41 * _rtB -> B_1_2_0 ; _rtB -> B_1_154_0 =
look2_binlxpw ( _rtB -> B_1_153_0 , _rtB -> B_1_22_0 , _rtP -> P_43 , _rtP ->
P_44 , _rtP -> P_42 , _rtP -> P_153 , 5U ) ; _rtB -> B_1_155_0 = _rtB ->
B_1_18_0 - _rtB -> B_1_4_0 ; _rtB -> B_1_156_0 = _rtP -> P_45 * _rtB ->
B_1_155_0 ; _rtB -> B_1_157_0 = _rtP -> P_46 * _rtB -> B_1_24_0 ; _rtB ->
B_1_158_0 = _rtB -> B_1_157_0 + _rtB -> B_1_102_0 ; _rtB -> B_1_159_0 = _rtB
-> B_1_138_0 * _rtB -> B_1_158_0 ; _rtB -> B_1_160_0 = _rtB -> B_1_159_0 -
_rtB -> B_1_143_0 ; ssCallAccelRunBlock ( S , 2 , 1 , SS_CALL_MDL_OUTPUTS ) ;
ssCallAccelRunBlock ( S , 2 , 2 , SS_CALL_MDL_OUTPUTS ) ; ssCallAccelRunBlock
( S , 2 , 3 , SS_CALL_MDL_OUTPUTS ) ; ssCallAccelRunBlock ( S , 2 , 4 ,
SS_CALL_MDL_OUTPUTS ) ; ssCallAccelRunBlock ( S , 2 , 5 , SS_CALL_MDL_OUTPUTS
) ; ssCallAccelRunBlock ( S , 2 , 6 , SS_CALL_MDL_OUTPUTS ) ; i =
ssIsSampleHit ( S , 1 , 0 ) ; if ( i != 0 ) { { if ( _rtDW ->
TAQSigLogging_InsertedFor_EngineSpeed_at_outport_0_PWORK . AQHandles &&
ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_EngineSpeed_at_outport_0_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & ( (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU ( S ) ) ->
Engine_Speed + 0 ) ; } } { if ( _rtDW ->
TAQSigLogging_InsertedFor_PedalAngle_at_outport_0_PWORK . AQHandles &&
ssGetLogOutput ( S ) ) { sdiWriteSignal ( _rtDW ->
TAQSigLogging_InsertedFor_PedalAngle_at_outport_0_PWORK . AQHandles ,
ssGetTaskTime ( S , 1 ) , ( char * ) & ( (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU ( S ) ) ->
Pedal_Angle + 0 ) ; } } } UNUSED_PARAMETER ( tid ) ; } static void
mdlOutputsTID2 ( SimStruct * S , int_T tid ) {
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtB ;
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtP ; _rtP = ( (
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp ( S ) ) ; _rtB
= ( ( B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) _ssGetModelBlockIO ( S
) ) ; _rtB -> B_1_0_0_m = _rtP -> P_47 ; _rtB -> B_1_1_0_c = _rtP -> P_48 ;
_rtB -> B_1_3_0 = _rtP -> P_50 ; _rtB -> B_1_4_0_c = _rtP -> P_51 ; _rtB ->
B_1_5_0 = _rtP -> P_52 ; _rtB -> B_1_2_0_k [ 0 ] = _rtP -> P_49 [ 0 ] ; _rtB
-> B_1_6_0 [ 0 ] = _rtP -> P_53 [ 0 ] ; _rtB -> B_1_7_0_b [ 0 ] = _rtP ->
P_54 [ 0 ] ; _rtB -> B_1_8_0_p [ 0 ] = _rtP -> P_55 [ 0 ] ; _rtB -> B_1_9_0_c
[ 0 ] = _rtP -> P_56 [ 0 ] ; _rtB -> B_1_10_0 [ 0 ] = _rtP -> P_57 [ 0 ] ;
_rtB -> B_1_11_0_f [ 0 ] = _rtP -> P_58 [ 0 ] ; _rtB -> B_1_12_0_g [ 0 ] =
_rtP -> P_59 [ 0 ] ; _rtB -> B_1_13_0_g [ 0 ] = _rtP -> P_60 [ 0 ] ; _rtB ->
B_1_14_0 [ 0 ] = _rtP -> P_61 [ 0 ] ; _rtB -> B_1_15_0_m [ 0 ] = _rtP -> P_62
[ 0 ] ; _rtB -> B_1_16_0_n [ 0 ] = _rtP -> P_63 [ 0 ] ; _rtB -> B_1_17_0_p [
0 ] = _rtP -> P_64 [ 0 ] ; _rtB -> B_1_18_0_l [ 0 ] = _rtP -> P_65 [ 0 ] ;
_rtB -> B_1_19_0 [ 0 ] = _rtP -> P_66 [ 0 ] ; _rtB -> B_1_20_0_j [ 0 ] = _rtP
-> P_67 [ 0 ] ; _rtB -> B_1_21_0_d [ 0 ] = _rtP -> P_68 [ 0 ] ; _rtB ->
B_1_22_0_g [ 0 ] = _rtP -> P_69 [ 0 ] ; _rtB -> B_1_23_0 [ 0 ] = _rtP -> P_70
[ 0 ] ; _rtB -> B_1_24_0_l [ 0 ] = _rtP -> P_71 [ 0 ] ; _rtB -> B_1_25_0 [ 0
] = _rtP -> P_72 [ 0 ] ; _rtB -> B_1_26_0 [ 0 ] = _rtP -> P_73 [ 0 ] ; _rtB
-> B_1_27_0_d [ 0 ] = _rtP -> P_74 [ 0 ] ; _rtB -> B_1_28_0 [ 0 ] = _rtP ->
P_75 [ 0 ] ; _rtB -> B_1_29_0 [ 0 ] = _rtP -> P_76 [ 0 ] ; _rtB -> B_1_30_0_d
[ 0 ] = _rtP -> P_77 [ 0 ] ; _rtB -> B_1_31_0_l [ 0 ] = _rtP -> P_78 [ 0 ] ;
_rtB -> B_1_32_0_o [ 0 ] = _rtP -> P_79 [ 0 ] ; _rtB -> B_1_33_0_b [ 0 ] =
_rtP -> P_80 [ 0 ] ; _rtB -> B_1_34_0 [ 0 ] = _rtP -> P_81 [ 0 ] ; _rtB ->
B_1_35_0_n [ 0 ] = _rtP -> P_82 [ 0 ] ; _rtB -> B_1_2_0_k [ 1 ] = _rtP ->
P_49 [ 1 ] ; _rtB -> B_1_6_0 [ 1 ] = _rtP -> P_53 [ 1 ] ; _rtB -> B_1_7_0_b [
1 ] = _rtP -> P_54 [ 1 ] ; _rtB -> B_1_8_0_p [ 1 ] = _rtP -> P_55 [ 1 ] ;
_rtB -> B_1_9_0_c [ 1 ] = _rtP -> P_56 [ 1 ] ; _rtB -> B_1_10_0 [ 1 ] = _rtP
-> P_57 [ 1 ] ; _rtB -> B_1_11_0_f [ 1 ] = _rtP -> P_58 [ 1 ] ; _rtB ->
B_1_12_0_g [ 1 ] = _rtP -> P_59 [ 1 ] ; _rtB -> B_1_13_0_g [ 1 ] = _rtP ->
P_60 [ 1 ] ; _rtB -> B_1_14_0 [ 1 ] = _rtP -> P_61 [ 1 ] ; _rtB -> B_1_15_0_m
[ 1 ] = _rtP -> P_62 [ 1 ] ; _rtB -> B_1_16_0_n [ 1 ] = _rtP -> P_63 [ 1 ] ;
_rtB -> B_1_17_0_p [ 1 ] = _rtP -> P_64 [ 1 ] ; _rtB -> B_1_18_0_l [ 1 ] =
_rtP -> P_65 [ 1 ] ; _rtB -> B_1_19_0 [ 1 ] = _rtP -> P_66 [ 1 ] ; _rtB ->
B_1_20_0_j [ 1 ] = _rtP -> P_67 [ 1 ] ; _rtB -> B_1_21_0_d [ 1 ] = _rtP ->
P_68 [ 1 ] ; _rtB -> B_1_22_0_g [ 1 ] = _rtP -> P_69 [ 1 ] ; _rtB -> B_1_23_0
[ 1 ] = _rtP -> P_70 [ 1 ] ; _rtB -> B_1_24_0_l [ 1 ] = _rtP -> P_71 [ 1 ] ;
_rtB -> B_1_25_0 [ 1 ] = _rtP -> P_72 [ 1 ] ; _rtB -> B_1_26_0 [ 1 ] = _rtP
-> P_73 [ 1 ] ; _rtB -> B_1_27_0_d [ 1 ] = _rtP -> P_74 [ 1 ] ; _rtB ->
B_1_28_0 [ 1 ] = _rtP -> P_75 [ 1 ] ; _rtB -> B_1_29_0 [ 1 ] = _rtP -> P_76 [
1 ] ; _rtB -> B_1_30_0_d [ 1 ] = _rtP -> P_77 [ 1 ] ; _rtB -> B_1_31_0_l [ 1
] = _rtP -> P_78 [ 1 ] ; _rtB -> B_1_32_0_o [ 1 ] = _rtP -> P_79 [ 1 ] ; _rtB
-> B_1_33_0_b [ 1 ] = _rtP -> P_80 [ 1 ] ; _rtB -> B_1_34_0 [ 1 ] = _rtP ->
P_81 [ 1 ] ; _rtB -> B_1_35_0_n [ 1 ] = _rtP -> P_82 [ 1 ] ; _rtB ->
B_1_2_0_k [ 2 ] = _rtP -> P_49 [ 2 ] ; _rtB -> B_1_6_0 [ 2 ] = _rtP -> P_53 [
2 ] ; _rtB -> B_1_7_0_b [ 2 ] = _rtP -> P_54 [ 2 ] ; _rtB -> B_1_8_0_p [ 2 ]
= _rtP -> P_55 [ 2 ] ; _rtB -> B_1_9_0_c [ 2 ] = _rtP -> P_56 [ 2 ] ; _rtB ->
B_1_10_0 [ 2 ] = _rtP -> P_57 [ 2 ] ; _rtB -> B_1_11_0_f [ 2 ] = _rtP -> P_58
[ 2 ] ; _rtB -> B_1_12_0_g [ 2 ] = _rtP -> P_59 [ 2 ] ; _rtB -> B_1_13_0_g [
2 ] = _rtP -> P_60 [ 2 ] ; _rtB -> B_1_14_0 [ 2 ] = _rtP -> P_61 [ 2 ] ; _rtB
-> B_1_15_0_m [ 2 ] = _rtP -> P_62 [ 2 ] ; _rtB -> B_1_16_0_n [ 2 ] = _rtP ->
P_63 [ 2 ] ; _rtB -> B_1_17_0_p [ 2 ] = _rtP -> P_64 [ 2 ] ; _rtB ->
B_1_18_0_l [ 2 ] = _rtP -> P_65 [ 2 ] ; _rtB -> B_1_19_0 [ 2 ] = _rtP -> P_66
[ 2 ] ; _rtB -> B_1_20_0_j [ 2 ] = _rtP -> P_67 [ 2 ] ; _rtB -> B_1_21_0_d [
2 ] = _rtP -> P_68 [ 2 ] ; _rtB -> B_1_22_0_g [ 2 ] = _rtP -> P_69 [ 2 ] ;
_rtB -> B_1_23_0 [ 2 ] = _rtP -> P_70 [ 2 ] ; _rtB -> B_1_24_0_l [ 2 ] = _rtP
-> P_71 [ 2 ] ; _rtB -> B_1_25_0 [ 2 ] = _rtP -> P_72 [ 2 ] ; _rtB ->
B_1_26_0 [ 2 ] = _rtP -> P_73 [ 2 ] ; _rtB -> B_1_27_0_d [ 2 ] = _rtP -> P_74
[ 2 ] ; _rtB -> B_1_28_0 [ 2 ] = _rtP -> P_75 [ 2 ] ; _rtB -> B_1_29_0 [ 2 ]
= _rtP -> P_76 [ 2 ] ; _rtB -> B_1_30_0_d [ 2 ] = _rtP -> P_77 [ 2 ] ; _rtB
-> B_1_31_0_l [ 2 ] = _rtP -> P_78 [ 2 ] ; _rtB -> B_1_32_0_o [ 2 ] = _rtP ->
P_79 [ 2 ] ; _rtB -> B_1_33_0_b [ 2 ] = _rtP -> P_80 [ 2 ] ; _rtB -> B_1_34_0
[ 2 ] = _rtP -> P_81 [ 2 ] ; _rtB -> B_1_35_0_n [ 2 ] = _rtP -> P_82 [ 2 ] ;
_rtB -> B_1_2_0_k [ 3 ] = _rtP -> P_49 [ 3 ] ; _rtB -> B_1_6_0 [ 3 ] = _rtP
-> P_53 [ 3 ] ; _rtB -> B_1_7_0_b [ 3 ] = _rtP -> P_54 [ 3 ] ; _rtB ->
B_1_8_0_p [ 3 ] = _rtP -> P_55 [ 3 ] ; _rtB -> B_1_9_0_c [ 3 ] = _rtP -> P_56
[ 3 ] ; _rtB -> B_1_10_0 [ 3 ] = _rtP -> P_57 [ 3 ] ; _rtB -> B_1_11_0_f [ 3
] = _rtP -> P_58 [ 3 ] ; _rtB -> B_1_12_0_g [ 3 ] = _rtP -> P_59 [ 3 ] ; _rtB
-> B_1_13_0_g [ 3 ] = _rtP -> P_60 [ 3 ] ; _rtB -> B_1_14_0 [ 3 ] = _rtP ->
P_61 [ 3 ] ; _rtB -> B_1_15_0_m [ 3 ] = _rtP -> P_62 [ 3 ] ; _rtB ->
B_1_16_0_n [ 3 ] = _rtP -> P_63 [ 3 ] ; _rtB -> B_1_17_0_p [ 3 ] = _rtP ->
P_64 [ 3 ] ; _rtB -> B_1_18_0_l [ 3 ] = _rtP -> P_65 [ 3 ] ; _rtB -> B_1_19_0
[ 3 ] = _rtP -> P_66 [ 3 ] ; _rtB -> B_1_20_0_j [ 3 ] = _rtP -> P_67 [ 3 ] ;
_rtB -> B_1_21_0_d [ 3 ] = _rtP -> P_68 [ 3 ] ; _rtB -> B_1_22_0_g [ 3 ] =
_rtP -> P_69 [ 3 ] ; _rtB -> B_1_23_0 [ 3 ] = _rtP -> P_70 [ 3 ] ; _rtB ->
B_1_24_0_l [ 3 ] = _rtP -> P_71 [ 3 ] ; _rtB -> B_1_25_0 [ 3 ] = _rtP -> P_72
[ 3 ] ; _rtB -> B_1_26_0 [ 3 ] = _rtP -> P_73 [ 3 ] ; _rtB -> B_1_27_0_d [ 3
] = _rtP -> P_74 [ 3 ] ; _rtB -> B_1_28_0 [ 3 ] = _rtP -> P_75 [ 3 ] ; _rtB
-> B_1_29_0 [ 3 ] = _rtP -> P_76 [ 3 ] ; _rtB -> B_1_30_0_d [ 3 ] = _rtP ->
P_77 [ 3 ] ; _rtB -> B_1_31_0_l [ 3 ] = _rtP -> P_78 [ 3 ] ; _rtB ->
B_1_32_0_o [ 3 ] = _rtP -> P_79 [ 3 ] ; _rtB -> B_1_33_0_b [ 3 ] = _rtP ->
P_80 [ 3 ] ; _rtB -> B_1_34_0 [ 3 ] = _rtP -> P_81 [ 3 ] ; _rtB -> B_1_35_0_n
[ 3 ] = _rtP -> P_82 [ 3 ] ; memcpy ( & _rtB -> B_1_36_0_b [ 0 ] , & _rtP ->
P_83 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_37_0 [ 0 ] ,
& _rtP -> P_84 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_38_0 [ 0 ] , & _rtP -> P_85 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_39_0 [ 0 ] , & _rtP -> P_86 [ 0 ] , 30U * sizeof ( real_T ) ) ;
memcpy ( & _rtB -> B_1_40_0 [ 0 ] , & _rtP -> P_87 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_41_0 [ 0 ] , & _rtP -> P_88 [ 0 ] , 30U *
sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_42_0 [ 0 ] , & _rtP -> P_89 [ 0
] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_43_0 [ 0 ] , & _rtP ->
P_90 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_44_0 [ 0 ] ,
& _rtP -> P_91 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_45_0 [ 0 ] , & _rtP -> P_92 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_46_0 [ 0 ] , & _rtP -> P_93 [ 0 ] , 30U * sizeof ( real_T ) ) ;
memcpy ( & _rtB -> B_1_47_0 [ 0 ] , & _rtP -> P_94 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_48_0 [ 0 ] , & _rtP -> P_95 [ 0 ] , 30U *
sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_49_0 [ 0 ] , & _rtP -> P_96 [ 0
] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_50_0 [ 0 ] , & _rtP ->
P_97 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_51_0 [ 0 ] ,
& _rtP -> P_98 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_52_0 [ 0 ] , & _rtP -> P_99 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_53_0 [ 0 ] , & _rtP -> P_100 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_54_0 [ 0 ] , & _rtP -> P_101 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_55_0 [ 0 ] , & _rtP -> P_102 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_56_0 [ 0 ] , & _rtP -> P_103 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_57_0 [ 0 ] , & _rtP
-> P_104 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_58_0 [ 0
] , & _rtP -> P_105 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_59_0 [ 0 ] , & _rtP -> P_106 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_60_0 [ 0 ] , & _rtP -> P_107 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_61_0 [ 0 ] , & _rtP -> P_108 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_62_0 [ 0 ] , & _rtP -> P_109 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_63_0 [ 0 ] , & _rtP -> P_110 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_64_0 [ 0 ] , & _rtP
-> P_111 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_65_0 [ 0
] , & _rtP -> P_112 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_66_0 [ 0 ] , & _rtP -> P_113 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_67_0_l [ 0 ] , & _rtP -> P_114 [ 0 ] , 30U * sizeof ( real_T )
) ; memcpy ( & _rtB -> B_1_68_0_h [ 0 ] , & _rtP -> P_115 [ 0 ] , 30U *
sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_69_0 [ 0 ] , & _rtP -> P_116 [ 0
] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_70_0 [ 0 ] , & _rtP ->
P_117 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_71_0 [ 0 ] ,
& _rtP -> P_118 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_72_0 [ 0 ] , & _rtP -> P_119 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_73_0 [ 0 ] , & _rtP -> P_120 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_74_0 [ 0 ] , & _rtP -> P_121 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_75_0 [ 0 ] , & _rtP -> P_122 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_76_0 [ 0 ] , & _rtP -> P_123 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_77_0 [ 0 ] , & _rtP
-> P_124 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_78_0 [ 0
] , & _rtP -> P_125 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_79_0 [ 0 ] , & _rtP -> P_126 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_80_0 [ 0 ] , & _rtP -> P_127 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_81_0 [ 0 ] , & _rtP -> P_128 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_82_0 [ 0 ] , & _rtP -> P_129 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_83_0 [ 0 ] , & _rtP -> P_130 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_84_0 [ 0 ] , & _rtP
-> P_131 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_85_0 [ 0
] , & _rtP -> P_132 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_86_0 [ 0 ] , & _rtP -> P_133 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_87_0 [ 0 ] , & _rtP -> P_134 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_88_0 [ 0 ] , & _rtP -> P_135 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_89_0 [ 0 ] , & _rtP -> P_136 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_90_0 [ 0 ] , & _rtP -> P_137 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_91_0 [ 0 ] , & _rtP
-> P_138 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_92_0 [ 0
] , & _rtP -> P_139 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB ->
B_1_93_0 [ 0 ] , & _rtP -> P_140 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy (
& _rtB -> B_1_94_0 [ 0 ] , & _rtP -> P_141 [ 0 ] , 30U * sizeof ( real_T ) )
; memcpy ( & _rtB -> B_1_95_0 [ 0 ] , & _rtP -> P_142 [ 0 ] , 30U * sizeof (
real_T ) ) ; memcpy ( & _rtB -> B_1_96_0 [ 0 ] , & _rtP -> P_143 [ 0 ] , 30U
* sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_97_0 [ 0 ] , & _rtP -> P_144 [
0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_98_0 [ 0 ] , & _rtP
-> P_145 [ 0 ] , 30U * sizeof ( real_T ) ) ; memcpy ( & _rtB -> B_1_99_0_b [
0 ] , & _rtP -> P_146 [ 0 ] , 30U * sizeof ( real_T ) ) ; _rtB -> B_1_100_0_d
= _rtP -> P_147 ; _rtB -> B_1_101_0 = _rtP -> P_148 ; _rtB -> B_1_102_0 =
_rtP -> P_149 ; UNUSED_PARAMETER ( tid ) ; }
#define MDL_UPDATE
static void mdlUpdate ( SimStruct * S , int_T tid ) {
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtB ;
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtDW ;
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtP ; _rtDW = ( (
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetRootDWork ( S ) ) ;
_rtP = ( ( P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp ( S
) ) ; _rtB = ( ( B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
_ssGetModelBlockIO ( S ) ) ; { real_T * * uBuffer = ( real_T * * ) & _rtDW ->
fuelsystemtransportdelay_PWORK . TUbufferPtrs [ 0 ] ; real_T simTime = ssGetT
( S ) ; _rtDW -> fuelsystemtransportdelay_IWORK . Head = ( ( _rtDW ->
fuelsystemtransportdelay_IWORK . Head < ( _rtDW ->
fuelsystemtransportdelay_IWORK . CircularBufSize - 1 ) ) ? ( _rtDW ->
fuelsystemtransportdelay_IWORK . Head + 1 ) : 0 ) ; if ( _rtDW ->
fuelsystemtransportdelay_IWORK . Head == _rtDW ->
fuelsystemtransportdelay_IWORK . Tail ) { if ( !
fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_rt_TDelayUpdateTailOrGrowBuf ( &
_rtDW -> fuelsystemtransportdelay_IWORK . CircularBufSize , & _rtDW ->
fuelsystemtransportdelay_IWORK . Tail , & _rtDW ->
fuelsystemtransportdelay_IWORK . Head , & _rtDW ->
fuelsystemtransportdelay_IWORK . Last , simTime - _rtP -> P_38 , uBuffer , (
boolean_T ) 0 , ( boolean_T ) 1 , & _rtDW -> fuelsystemtransportdelay_IWORK .
MaxNewBufSize ) ) { ssSetErrorStatus ( S , "vtdelay memory allocation error"
) ; return ; } } ( * uBuffer + _rtDW -> fuelsystemtransportdelay_IWORK .
CircularBufSize ) [ _rtDW -> fuelsystemtransportdelay_IWORK . Head ] =
simTime ; ( * uBuffer + 2 * _rtDW -> fuelsystemtransportdelay_IWORK .
CircularBufSize ) [ _rtDW -> fuelsystemtransportdelay_IWORK . Head ] = ( (
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetContStates ( S ) ) ->
fuelsystemtransportdelay_CSTATE ; ( * uBuffer ) [ _rtDW ->
fuelsystemtransportdelay_IWORK . Head ] = _rtB -> B_1_149_0 ; }
UNUSED_PARAMETER ( tid ) ; }
#define MDL_UPDATE
static void mdlUpdateTID2 ( SimStruct * S , int_T tid ) { UNUSED_PARAMETER (
tid ) ; }
#define MDL_DERIVATIVES
static void mdlDerivatives ( SimStruct * S ) {
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtB ;
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtDW ;
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtP ;
XDot_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtXdot ;
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * _rtX ; _rtDW = ( (
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetRootDWork ( S ) ) ;
_rtXdot = ( ( XDot_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetdX ( S
) ) ; _rtX = ( ( X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
ssGetContStates ( S ) ) ; _rtP = ( (
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp ( S ) ) ; _rtB
= ( ( B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) _ssGetModelBlockIO ( S
) ) ; _rtXdot -> p00543bar_CSTATE = _rtB -> B_1_156_0 ; _rtXdot ->
Throttledelay_CSTATE = 0.0 ; _rtXdot -> Throttledelay_CSTATE += _rtP -> P_4 *
_rtX -> Throttledelay_CSTATE ; _rtXdot -> Throttledelay_CSTATE += ( (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetU ( S ) ) ->
Pedal_Angle ; _rtXdot -> Integrator_CSTATE = _rtB -> B_1_148_0 ; _rtXdot ->
Integrator_CSTATE_c = _rtB -> B_1_160_0 ; _rtXdot -> Integrator_CSTATE_h =
_rtB -> B_1_152_0 ; { real_T instantDelay ; instantDelay = _rtB -> B_1_154_0
; if ( instantDelay > _rtP -> P_38 ) { instantDelay = _rtP -> P_38 ; } if (
instantDelay < 0.0 ) { ( ( XDot_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
ssGetdX ( S ) ) -> fuelsystemtransportdelay_CSTATE = 0 ; } else { ( (
XDot_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetdX ( S ) ) ->
fuelsystemtransportdelay_CSTATE = 1.0 / instantDelay ; } } } static void
mdlInitializeSizes ( SimStruct * S ) { ssSetChecksumVal ( S , 0 , 2984930131U
) ; ssSetChecksumVal ( S , 1 , 3648267324U ) ; ssSetChecksumVal ( S , 2 ,
1879621074U ) ; ssSetChecksumVal ( S , 3 , 818096608U ) ; { mxArray *
slVerStructMat = NULL ; mxArray * slStrMat = mxCreateString ( "simulink" ) ;
char slVerChar [ 10 ] ; int status = mexCallMATLAB ( 1 , & slVerStructMat , 1
, & slStrMat , "ver" ) ; if ( status == 0 ) { mxArray * slVerMat = mxGetField
( slVerStructMat , 0 , "Version" ) ; if ( slVerMat == NULL ) { status = 1 ; }
else { status = mxGetString ( slVerMat , slVerChar , 10 ) ; } }
mxDestroyArray ( slStrMat ) ; mxDestroyArray ( slVerStructMat ) ; if ( (
status == 1 ) || ( strcmp ( slVerChar , "10.3" ) != 0 ) ) { return ; } }
ssSetOptions ( S , SS_OPTION_EXCEPTION_FREE_CODE ) ; if ( ssGetSizeofDWork (
S ) != sizeof ( DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) {
ssSetErrorStatus ( S , "Unexpected error: Internal DWork sizes do "
"not match for accelerator mex file." ) ; } if ( ssGetSizeofGlobalBlockIO ( S
) != sizeof ( B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) {
ssSetErrorStatus ( S , "Unexpected error: Internal BlockIO sizes do "
"not match for accelerator mex file." ) ; } if ( ssGetSizeofDWork ( S ) != (
SLSize ) sizeof ( DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) { static
char msg [ 256 ] ; sprintf ( msg ,
"Unexpected error: Internal DWork sizes do "
"not match for accelerator mex file (%ld vs %lu)." , ( signed long )
ssGetSizeofDWork ( S ) , ( unsigned long ) sizeof (
DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) ; ssSetErrorStatus ( S , msg
) ; } if ( ssGetSizeofGlobalBlockIO ( S ) != ( SLSize ) sizeof (
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) { static char msg [ 256 ] ;
sprintf ( msg , "Unexpected error: Internal BlockIO sizes do "
"not match for accelerator mex file (%ld vs %lu)." , ( signed long )
ssGetSizeofGlobalBlockIO ( S ) , ( unsigned long ) sizeof (
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) ; ssSetErrorStatus ( S , msg
) ; } if ( ssGetSizeofU ( S ) != sizeof (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) { static char msg
[ 256 ] ; sprintf ( msg ,
"Unexpected error: Internal ExternalInputs sizes do "
"not match for accelerator mex file (%ld vs %lu)." , ( signed long )
ssGetSizeofU ( S ) , ( unsigned long ) sizeof (
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) ;
ssSetErrorStatus ( S , msg ) ; } { int ssSizeofParams ; ssGetSizeofParams ( S
, & ssSizeofParams ) ; if ( ssSizeofParams != sizeof (
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) { static char msg [ 256 ] ;
sprintf ( msg , "Unexpected error: Internal Parameters sizes do "
"not match for accelerator mex file (%d vs %lu)." , ssSizeofParams , (
unsigned long ) sizeof ( P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ) ) ;
ssSetErrorStatus ( S , msg ) ; } } _ssSetModelRtp ( S , ( real_T * ) &
fr_AFC_FFNN_tra_53497_w1957276_1_breach_rtDefaultP ) ; rt_InitInfAndNaN (
sizeof ( real_T ) ) ; ( ( P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * )
ssGetModelRtp ( S ) ) -> P_24 = rtInf ; ( (
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp ( S ) ) -> P_26
= rtInf ; ( ( P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T * ) ssGetModelRtp (
S ) ) -> P_28 = rtInf ; } static void mdlInitializeSampleTimes ( SimStruct *
S ) { slAccRegPrmChangeFcn ( S , mdlOutputsTID2 ) ; } static void
mdlTerminate ( SimStruct * S ) { }
#include "simulink.c"

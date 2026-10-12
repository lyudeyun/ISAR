#ifndef RTW_HEADER_fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_h_
#define RTW_HEADER_fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_h_
#include <stddef.h>
#include <string.h>
#ifndef fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_COMMON_INCLUDES_
#define fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_COMMON_INCLUDES_
#include <stdlib.h>
#define S_FUNCTION_NAME simulink_only_sfcn 
#define S_FUNCTION_LEVEL 2
#define RTW_GENERATED_S_FUNCTION
#include "sl_AsyncioQueue/AsyncioQueueCAPI.h"
#include "rtwtypes.h"
#include "simstruc.h"
#include "fixedpoint.h"
#endif
#include "fr_AFC_FFNN_tra_53497_w1957276_1_breach_acc_types.h"
#include "multiword_types.h"
#include "mwmathutil.h"
#include "rtGetInf.h"
#include "rt_defines.h"
#include "rt_nonfinite.h"
typedef struct { real_T B_1_0_0 ; real_T B_1_1_0 ; real_T B_1_2_0 ; real_T
B_1_4_0 ; real_T B_1_7_0 ; real_T B_1_8_0 ; real_T B_1_9_0 ; real_T B_1_11_0
; real_T B_1_12_0 ; real_T B_1_13_0 ; real_T B_1_15_0 ; real_T B_1_16_0 ;
real_T B_1_17_0 ; real_T B_1_18_0 ; real_T B_1_20_0 ; real_T B_1_21_0 ;
real_T B_1_22_0 ; real_T B_1_24_0 ; real_T B_1_27_0 ; real_T B_1_30_0 ;
real_T B_1_31_0 ; real_T B_1_32_0 ; real_T B_1_33_0 ; real_T B_1_35_0 [ 4 ] ;
real_T B_1_36_0 [ 4 ] ; real_T B_1_67_0 [ 30 ] ; real_T B_1_68_0 [ 30 ] ;
real_T B_1_99_0 [ 30 ] ; real_T B_1_100_0 [ 30 ] ; real_T B_1_131_0 [ 30 ] ;
real_T B_1_132_0 [ 30 ] ; real_T B_1_134_0 ; real_T B_1_135_0 ; real_T
B_1_136_0 ; real_T B_1_137_0 ; real_T B_1_138_0 ; real_T B_1_139_0 ; real_T
B_1_140_0 ; real_T B_1_142_0 ; real_T B_1_143_0 ; real_T B_1_144_0 ; real_T
B_1_146_0 ; real_T B_1_147_0 ; real_T B_1_148_0 ; real_T B_1_149_0 ; real_T
B_1_150_0 ; real_T B_1_151_0 ; real_T B_1_152_0 ; real_T B_1_153_0 ; real_T
B_1_154_0 ; real_T B_1_155_0 ; real_T B_1_156_0 ; real_T B_1_157_0 ; real_T
B_1_158_0 ; real_T B_1_159_0 ; real_T B_1_160_0 ; real_T B_1_0_0_m ; real_T
B_1_1_0_c ; real_T B_1_2_0_k [ 4 ] ; real_T B_1_3_0 ; real_T B_1_4_0_c ;
real_T B_1_5_0 ; real_T B_1_6_0 [ 4 ] ; real_T B_1_7_0_b [ 4 ] ; real_T
B_1_8_0_p [ 4 ] ; real_T B_1_9_0_c [ 4 ] ; real_T B_1_10_0 [ 4 ] ; real_T
B_1_11_0_f [ 4 ] ; real_T B_1_12_0_g [ 4 ] ; real_T B_1_13_0_g [ 4 ] ; real_T
B_1_14_0 [ 4 ] ; real_T B_1_15_0_m [ 4 ] ; real_T B_1_16_0_n [ 4 ] ; real_T
B_1_17_0_p [ 4 ] ; real_T B_1_18_0_l [ 4 ] ; real_T B_1_19_0 [ 4 ] ; real_T
B_1_20_0_j [ 4 ] ; real_T B_1_21_0_d [ 4 ] ; real_T B_1_22_0_g [ 4 ] ; real_T
B_1_23_0 [ 4 ] ; real_T B_1_24_0_l [ 4 ] ; real_T B_1_25_0 [ 4 ] ; real_T
B_1_26_0 [ 4 ] ; real_T B_1_27_0_d [ 4 ] ; real_T B_1_28_0 [ 4 ] ; real_T
B_1_29_0 [ 4 ] ; real_T B_1_30_0_d [ 4 ] ; real_T B_1_31_0_l [ 4 ] ; real_T
B_1_32_0_o [ 4 ] ; real_T B_1_33_0_b [ 4 ] ; real_T B_1_34_0 [ 4 ] ; real_T
B_1_35_0_n [ 4 ] ; real_T B_1_36_0_b [ 30 ] ; real_T B_1_37_0 [ 30 ] ; real_T
B_1_38_0 [ 30 ] ; real_T B_1_39_0 [ 30 ] ; real_T B_1_40_0 [ 30 ] ; real_T
B_1_41_0 [ 30 ] ; real_T B_1_42_0 [ 30 ] ; real_T B_1_43_0 [ 30 ] ; real_T
B_1_44_0 [ 30 ] ; real_T B_1_45_0 [ 30 ] ; real_T B_1_46_0 [ 30 ] ; real_T
B_1_47_0 [ 30 ] ; real_T B_1_48_0 [ 30 ] ; real_T B_1_49_0 [ 30 ] ; real_T
B_1_50_0 [ 30 ] ; real_T B_1_51_0 [ 30 ] ; real_T B_1_52_0 [ 30 ] ; real_T
B_1_53_0 [ 30 ] ; real_T B_1_54_0 [ 30 ] ; real_T B_1_55_0 [ 30 ] ; real_T
B_1_56_0 [ 30 ] ; real_T B_1_57_0 [ 30 ] ; real_T B_1_58_0 [ 30 ] ; real_T
B_1_59_0 [ 30 ] ; real_T B_1_60_0 [ 30 ] ; real_T B_1_61_0 [ 30 ] ; real_T
B_1_62_0 [ 30 ] ; real_T B_1_63_0 [ 30 ] ; real_T B_1_64_0 [ 30 ] ; real_T
B_1_65_0 [ 30 ] ; real_T B_1_66_0 [ 30 ] ; real_T B_1_67_0_l [ 30 ] ; real_T
B_1_68_0_h [ 30 ] ; real_T B_1_69_0 [ 30 ] ; real_T B_1_70_0 [ 30 ] ; real_T
B_1_71_0 [ 30 ] ; real_T B_1_72_0 [ 30 ] ; real_T B_1_73_0 [ 30 ] ; real_T
B_1_74_0 [ 30 ] ; real_T B_1_75_0 [ 30 ] ; real_T B_1_76_0 [ 30 ] ; real_T
B_1_77_0 [ 30 ] ; real_T B_1_78_0 [ 30 ] ; real_T B_1_79_0 [ 30 ] ; real_T
B_1_80_0 [ 30 ] ; real_T B_1_81_0 [ 30 ] ; real_T B_1_82_0 [ 30 ] ; real_T
B_1_83_0 [ 30 ] ; real_T B_1_84_0 [ 30 ] ; real_T B_1_85_0 [ 30 ] ; real_T
B_1_86_0 [ 30 ] ; real_T B_1_87_0 [ 30 ] ; real_T B_1_88_0 [ 30 ] ; real_T
B_1_89_0 [ 30 ] ; real_T B_1_90_0 [ 30 ] ; real_T B_1_91_0 [ 30 ] ; real_T
B_1_92_0 [ 30 ] ; real_T B_1_93_0 [ 30 ] ; real_T B_1_94_0 [ 30 ] ; real_T
B_1_95_0 [ 30 ] ; real_T B_1_96_0 [ 30 ] ; real_T B_1_97_0 [ 30 ] ; real_T
B_1_98_0 [ 30 ] ; real_T B_1_99_0_b [ 30 ] ; real_T B_1_100_0_d ; real_T
B_1_101_0 ; real_T B_1_102_0 ; real32_T B_1_25_0_e ; real32_T B_1_26_0_b ;
real32_T B_1_28_0_j ; real32_T B_1_34_0_f ; }
B_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { real_T
Minus_DWORK1 [ 4 ] ; struct { real_T modelTStart ; }
fuelsystemtransportdelay_RWORK ; struct { void * AQHandles ; }
TAQSigLogging_InsertedFor_EngineSpeed_at_outport_0_PWORK ; struct { void *
AQHandles ; } TAQSigLogging_InsertedFor_PedalAngle_at_outport_0_PWORK ;
struct { void * AQHandles ; }
TAQSigLogging_InsertedFor_IntakeManifold_at_outport_0_PWORK ; struct { void *
AQHandles ; } TAQSigLogging_InsertedFor_IntakeManifold_at_outport_1_PWORK ;
struct { void * AQHandles ; }
TAQSigLogging_InsertedFor_Throttle_at_outport_0_PWORK ; struct { void *
AQHandles ; } TAQSigLogging_InsertedFor_Wallwetting_at_outport_0_PWORK ;
struct { void * TUbufferPtrs [ 3 ] ; } fuelsystemtransportdelay_PWORK ;
int32_T AirFuelControlModel1_sysIdxToRun ; int32_T
TmpAtomicSubsysAtSwitchInport1_sysIdxToRun ; struct { int_T Tail ; int_T Head
; int_T Last ; int_T CircularBufSize ; int_T MaxNewBufSize ; }
fuelsystemtransportdelay_IWORK ; char_T pad_fuelsystemtransportdelay_IWORK [
4 ] ; } DW_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct {
real_T p00543bar_CSTATE ; real_T Throttledelay_CSTATE ; real_T
Integrator_CSTATE ; real_T Integrator_CSTATE_c ; real_T Integrator_CSTATE_h ;
real_T fuelsystemtransportdelay_CSTATE ; }
X_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { real_T
p00543bar_CSTATE ; real_T Throttledelay_CSTATE ; real_T Integrator_CSTATE ;
real_T Integrator_CSTATE_c ; real_T Integrator_CSTATE_h ; real_T
fuelsystemtransportdelay_CSTATE ; }
XDot_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { boolean_T
p00543bar_CSTATE ; boolean_T Throttledelay_CSTATE ; boolean_T
Integrator_CSTATE ; boolean_T Integrator_CSTATE_c ; boolean_T
Integrator_CSTATE_h ; boolean_T fuelsystemtransportdelay_CSTATE ; }
XDis_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { real_T
p00543bar_CSTATE ; real_T Throttledelay_CSTATE ; real_T Integrator_CSTATE ;
real_T Integrator_CSTATE_c ; real_T Integrator_CSTATE_h ; real_T
fuelsystemtransportdelay_CSTATE ; }
CStateAbsTol_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct {
real_T p00543bar_CSTATE ; real_T Throttledelay_CSTATE ; real_T
Integrator_CSTATE ; real_T Integrator_CSTATE_c ; real_T Integrator_CSTATE_h ;
real_T fuelsystemtransportdelay_CSTATE ; }
CXPtMin_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { real_T
p00543bar_CSTATE ; real_T Throttledelay_CSTATE ; real_T Integrator_CSTATE ;
real_T Integrator_CSTATE_c ; real_T Integrator_CSTATE_h ; real_T
fuelsystemtransportdelay_CSTATE ; }
CXPtMax_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct { real_T
Pedal_Angle ; real_T Engine_Speed ; }
ExternalUPtrs_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; typedef struct {
real_T * B_2_1 ; real32_T * B_2_2 ; real32_T * B_2_3 ; real32_T * B_2_4 ;
real32_T * B_2_5 ; real_T * B_2_6 ; }
ExtY_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T ; struct
P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T_ { real_T P_0 ; real_T P_1 ;
real_T P_2 ; real_T P_3 ; real_T P_4 ; real_T P_5 ; real_T P_6 ; real_T P_7 ;
real_T P_8 ; real_T P_9 ; real_T P_10 ; real_T P_11 [ 20 ] ; real_T P_12 [ 5
] ; real_T P_13 [ 4 ] ; real_T P_14 ; real_T P_15 ; real_T P_16 ; real_T P_17
; real_T P_18 ; real_T P_19 ; real_T P_20 ; real_T P_21 ; real_T P_22 ;
real_T P_23 [ 16 ] ; real_T P_24 ; real_T P_25 ; real_T P_26 ; real_T P_27 ;
real_T P_28 ; real_T P_29 ; real_T P_30 ; real_T P_31 ; real_T P_32 [ 20 ] ;
real_T P_33 [ 5 ] ; real_T P_34 [ 4 ] ; real_T P_35 ; real_T P_36 ; real_T
P_37 ; real_T P_38 ; real_T P_39 ; real_T P_40 ; real_T P_41 ; real_T P_42 [
20 ] ; real_T P_43 [ 5 ] ; real_T P_44 [ 4 ] ; real_T P_45 ; real_T P_46 ;
real_T P_47 ; real_T P_48 ; real_T P_49 [ 4 ] ; real_T P_50 ; real_T P_51 ;
real_T P_52 ; real_T P_53 [ 4 ] ; real_T P_54 [ 4 ] ; real_T P_55 [ 4 ] ;
real_T P_56 [ 4 ] ; real_T P_57 [ 4 ] ; real_T P_58 [ 4 ] ; real_T P_59 [ 4 ]
; real_T P_60 [ 4 ] ; real_T P_61 [ 4 ] ; real_T P_62 [ 4 ] ; real_T P_63 [ 4
] ; real_T P_64 [ 4 ] ; real_T P_65 [ 4 ] ; real_T P_66 [ 4 ] ; real_T P_67 [
4 ] ; real_T P_68 [ 4 ] ; real_T P_69 [ 4 ] ; real_T P_70 [ 4 ] ; real_T P_71
[ 4 ] ; real_T P_72 [ 4 ] ; real_T P_73 [ 4 ] ; real_T P_74 [ 4 ] ; real_T
P_75 [ 4 ] ; real_T P_76 [ 4 ] ; real_T P_77 [ 4 ] ; real_T P_78 [ 4 ] ;
real_T P_79 [ 4 ] ; real_T P_80 [ 4 ] ; real_T P_81 [ 4 ] ; real_T P_82 [ 4 ]
; real_T P_83 [ 30 ] ; real_T P_84 [ 30 ] ; real_T P_85 [ 30 ] ; real_T P_86
[ 30 ] ; real_T P_87 [ 30 ] ; real_T P_88 [ 30 ] ; real_T P_89 [ 30 ] ;
real_T P_90 [ 30 ] ; real_T P_91 [ 30 ] ; real_T P_92 [ 30 ] ; real_T P_93 [
30 ] ; real_T P_94 [ 30 ] ; real_T P_95 [ 30 ] ; real_T P_96 [ 30 ] ; real_T
P_97 [ 30 ] ; real_T P_98 [ 30 ] ; real_T P_99 [ 30 ] ; real_T P_100 [ 30 ] ;
real_T P_101 [ 30 ] ; real_T P_102 [ 30 ] ; real_T P_103 [ 30 ] ; real_T
P_104 [ 30 ] ; real_T P_105 [ 30 ] ; real_T P_106 [ 30 ] ; real_T P_107 [ 30
] ; real_T P_108 [ 30 ] ; real_T P_109 [ 30 ] ; real_T P_110 [ 30 ] ; real_T
P_111 [ 30 ] ; real_T P_112 [ 30 ] ; real_T P_113 [ 30 ] ; real_T P_114 [ 30
] ; real_T P_115 [ 30 ] ; real_T P_116 [ 30 ] ; real_T P_117 [ 30 ] ; real_T
P_118 [ 30 ] ; real_T P_119 [ 30 ] ; real_T P_120 [ 30 ] ; real_T P_121 [ 30
] ; real_T P_122 [ 30 ] ; real_T P_123 [ 30 ] ; real_T P_124 [ 30 ] ; real_T
P_125 [ 30 ] ; real_T P_126 [ 30 ] ; real_T P_127 [ 30 ] ; real_T P_128 [ 30
] ; real_T P_129 [ 30 ] ; real_T P_130 [ 30 ] ; real_T P_131 [ 30 ] ; real_T
P_132 [ 30 ] ; real_T P_133 [ 30 ] ; real_T P_134 [ 30 ] ; real_T P_135 [ 30
] ; real_T P_136 [ 30 ] ; real_T P_137 [ 30 ] ; real_T P_138 [ 30 ] ; real_T
P_139 [ 30 ] ; real_T P_140 [ 30 ] ; real_T P_141 [ 30 ] ; real_T P_142 [ 30
] ; real_T P_143 [ 30 ] ; real_T P_144 [ 30 ] ; real_T P_145 [ 30 ] ; real_T
P_146 [ 30 ] ; real_T P_147 ; real_T P_148 ; real_T P_149 ; real32_T P_150 ;
uint32_T P_151 [ 2 ] ; uint32_T P_152 [ 2 ] ; uint32_T P_153 [ 2 ] ; char_T
pad_P_153 [ 4 ] ; } ; extern P_fr_AFC_FFNN_tra_53497_w1957276_1_breach_T
fr_AFC_FFNN_tra_53497_w1957276_1_breach_rtDefaultP ;
#endif

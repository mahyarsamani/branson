#!/bin/bash

export LD_LIBRARY_PATH=${PWD}/../roi/papi/install/lib:$LD_LIBRARY_PATH

inputs=(3D_hohlraum_single_node.xml 3D_hohlraum_multi_node.xml)

for input in ${inputs[@]}; do
    echo $input
    export PAPI_EVENTS="PAPI_L1_DCR,PAPI_L1_DCW,PAPI_L1_DCM,PAPI_L1_DCA,PAPI_TLB_DM"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/backend0_data
    echo "backend0"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_L2_TCR,PAPI_L2_TCW,PAPI_L2_TCM,PAPI_L2_TCA,PAPI_L3_DCM,PAPI_L3_TCA"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/backend1_data
    echo "backend1"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_L1_ICM,PAPI_L1_ICH,PAPI_L1_ICA,PAPI_TLB_IM"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/frontend_data
    echo "frontend"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_TOT_INS,PAPI_INT_INS,PAPI_FP_INS,PAPI_LD_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/inst0_data
    echo "inst0"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_SR_INS,PAPI_BR_INS,PAPI_VEC_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/inst1_data
    echo "inst1"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_STL_ICY,PAPI_STL_CCY,PAPI_BR_MSP,PAPI_BR_PRC,PAPI_RES_STL,PAPI_TOT_CYC,PAPI_LST_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/pipe0_data
    echo "pipe0"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
    export PAPI_EVENTS="PAPI_SYC_INS,PAPI_FP_OPS,PAPI_REF_CYC"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/eight-core/branson/$input/pipe1_data
    echo "pipe1"
    numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON inputs/$input
done

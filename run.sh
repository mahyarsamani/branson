#!/bin/bash

# Initialize variables
MACHINE=""
CONFIG="socket"
SUFFIX=""
declare -a INPUT
declare -a FILES

# Print help message
print_help() {
  echo "Usage: ./run.sh --machine MACHINE --config CONFIG --suffix SUFFIX --input INPUT"
  echo ""
  echo "Options:"
  echo "  --machine MACHINE   Specify the machine name."
  echo "  --config CONFIG     Specify the config. Possible values are 'socket' and 'eight-core-def'. Default is 'socket'."
  echo "  --suffix SUFFIX     Specify the suffix."
  echo "  --input INPUT       Specify a comma-separated list of input file paths."
  echo ""
  echo "Example:"
  echo "  ./run.sh --machine grace --config eight-core-def --suffix test --input /path/to/input/file1.xml,/path/to/input/file2.xml"
}

# Parse command line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --machine) MACHINE="$2"; shift ;;
        --config) CONFIG="$2"; shift ;;
        --suffix) SUFFIX="$2"; shift ;;
        --input) IFS=',' read -ra INPUT <<< "$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; print_help; exit 1 ;;
    esac
    shift
done

# Check if machine, config, suffix, and input are set
if [[ -z "$MACHINE" || -z "$CONFIG" || -z "$SUFFIX" || -z "$INPUT" ]]; then
    echo "Error: --machine, --config, --suffix, and --input parameters are required."
    print_help
    exit 1
fi

# Check if config is valid
if [[ "$CONFIG" != "socket" && "$CONFIG" != "eight-core-def" ]]; then
    echo "Error: Invalid config. Possible values are 'socket' and 'eight-core-def'."
    print_help
    exit 1
fi

# Parse the input paths to get the filenames
for path in "${INPUT[@]}"; do
    file=$(basename "$path" .xml)
    FILES+=("$file")
done

# Print the machine, config, suffix, input, and files
echo "Machine: $MACHINE"
echo "Config: $CONFIG"
echo "Suffix: $SUFFIX"
echo "Input: ${INPUT[*]}"
echo "Files: ${FILES[*]}"
echo "You can find your papi data under ${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson."

# Co-iterate INPUT and FILES
for index in "${!INPUT[@]}"; do
    echo ${FILES[index]}
    export PAPI_EVENTS="PAPI_L1_DCR,PAPI_L1_DCW,PAPI_L1_DCM,PAPI_L1_DCA,PAPI_TLB_DM"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/backend0_data
    echo "backend0"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_L2_TCR,PAPI_L2_TCW,PAPI_L2_TCM,PAPI_L2_TCA,PAPI_L3_DCM,PAPI_L3_TCA"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/backend1_data
    echo "backend1"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_L1_ICM,PAPI_L1_ICH,PAPI_L1_ICA,PAPI_TLB_IM"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/frontend_data
    echo "frontend"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_TOT_INS,PAPI_INT_INS,PAPI_FP_INS,PAPI_LD_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/inst0_data
    echo "inst0"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_SR_INS,PAPI_BR_INS,PAPI_VEC_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/inst1_data
    echo "inst1"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_STL_ICY,PAPI_STL_CCY,PAPI_BR_MSP,PAPI_BR_PRC,PAPI_RES_STL,PAPI_TOT_CYC,PAPI_LST_INS"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/pipe0_data
    echo "pipe0"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
    export PAPI_EVENTS="PAPI_SYC_INS,PAPI_FP_OPS,PAPI_REF_CYC"
    export PAPI_OUTPUT_DIRECTORY=${PWD}/../data/${MACHINE}-${CONFIG}-${SUFFIX}/branson/${FILES[index]}/pipe1_data
    echo "pipe1"
    if [[ "$CONFIG" == "eight-core-def" ]]; then
        numactl --physcpubind=0,1,2,3,4,5,6,7 --membind=0 -- ./build/BRANSON ${INPUT[index]}
    else
        ./build/BRANSON ${INPUT[index]}
    fi
done

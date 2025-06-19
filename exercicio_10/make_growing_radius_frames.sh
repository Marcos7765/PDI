EXEC_FILE="build/pontilhismo"
PYTHON_EXEC="python3"

INPUT_FILE=$1
OUTPUT_FOLDER=$2
CELL_SIZE=$3

if [ -z "$INPUT_FILE" ]
then
    echo "Pass the path to an input file, use $0 <path/to/input> <path/to/output> [CELL_SIZE]"
    exit
fi
if [ -z "$OUTPUT_FOLDER" ]
then
    echo "Pass the path to an already existing output folder, use $0 $1 <path/to/output> [CELL_SIZE]"
    exit
fi
if [ -z "$CELL_SIZE" ]
then
    CELL_SIZE=30
    exit
fi

FINAL_RADIUS=$CELL_SIZE
SEED_VAL=7
RAD_REL_STDDEV=0.2

calc_stddev(){
    echo $($PYTHON_EXEC -c "print(int(\"$RAD\")*$RAD_REL_STDDEV)")
}

for RAD in $(seq -w $FINAL_RADIUS) #using -w to assure lexicographic order for 
do
    echo $EXEC_FILE $INPUT_FILE $CELL_SIZE $RAD $(calc_stddev) --seed $SEED_VAL -s $OUTPUT_FOLDER/frame_$RAD.png
    $EXEC_FILE $INPUT_FILE $CELL_SIZE $RAD $(calc_stddev) --seed $SEED_VAL -s $OUTPUT_FOLDER/frame_$RAD.png
    if [ $? -gt 0 ]
    then
        echo "Some error occurred, oops. Stopping early"
        exit
    fi
done
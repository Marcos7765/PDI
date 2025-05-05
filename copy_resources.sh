mkdir docs/resources/
for FOLDER in exercicio_*
do
    cp $FOLDER/resources/* docs/resources/
done

return 0 #added just to supress the error status from folders without resources
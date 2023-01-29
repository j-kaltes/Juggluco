sed  -z -e 's/ \* Method[^a-zA-Z]*\([a-zA-Z]*\)[^a-zA-Z]*Signature. \([^*]*\). \*\//{\"\1\",\"\2\",reinterpret_cast<void*>(Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_\1)},/g'|grep  reinterpret_cast

#sed -z -n -e 's/^.*Method:[ 	]*\([a-zA-Z]*\).*Signature:[ 	]*\(.*\)/{\"\1\",\"\2\",reinterpret_cast<void*>(Java_com_abbottdiabetescare_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative_\1)},/gp'







#!/bin/bash

# -------------------------------------------------------------------------------
# Filename:    imx6ull_complete
# Revision:    1.0
# Date:        2020/03/24
# Author:      hceng
# Email:       huangcheng.job@foxmail.com
# Website:     www.100ask.net
# Function:    For 100ASK_IMX6ULL module example program compilation.
# Notes:       None
# -------------------------------------------------------------------------------

outFile="imx6ull_app"

printModuleList() {
	find -name "*.ino" | sort -d | cut -d "/" -f 3,5
}

printUsage() {
	echo "Usage:" 
	echo "$0 list    # List supported modules and examples."
	echo "$0 xx      # Compile examples in the list."
}

completeAPP() {
	inoFilePath=$(find -name $inoFile)
	cppFilePath=${inoFilePath/ino/cpp}
	cp $inoFilePath $cppFilePath
	
	moduleName=$(echo $inoFilePath | cut -d "/" -f 3)
	cppDepend=""
	grep "#include <" libraries/$moduleName/src/* | cut -d "<" -f 2 | cut -d "." -f 1 | sed 's/$/.cpp/'>> tempcpp
	grep "#include \"" libraries/$moduleName/src/* | cut -d "\"" -f 2 | cut -d "." -f 1 | sed 's/$/.cpp/'>> tempcpp
	grep "#include <" $cppFilePath | cut -d "<" -f 2 | cut -d "." -f 1 | sed 's/$/.cpp/'>> tempcpp
	cat tempcpp | sort | uniq > tempcppsort 
	for i in $(cat tempcppsort)
	do
		result=$(find -name $i)
		cppDepend="$cppDepend $result"
	done
	# echo $cppDepend
	rm tempcpp
	rm tempcppsort
	
	find -name $outFile | grep -q $outFile
	if [ $? -eq 0 ]
	then
		rm $outFile
	fi

	arm-linux-gnueabihf-g++ -w -o $outFile \
	-I cores/100ask_imx6ull/ \
	$(find -name "src" | sed 's/^/-I /' | tr '\n' ' ' ) \
	$cppDepend \
	$cppFilePath
	
	rm $cppFilePath > /dev/null
	
	find -name $outFile | grep -q $outFile
	if [ $? -eq 0 ]
	then
		echo "--------------Compile successfully!--------------"
		echo "The file information:" 
		arm-linux-gnueabihf-size $outFile
		echo "Copy $outFile to 100ASK_IMX6ULL board, eg:"
		echo "scp $outFile root@192.168.1.35:/root/"
		echo "-------------------------------------------------"
	else
		printUsage
	fi
	
}

if [ "$1" == 'list' ] 
then
	printModuleList
else
	inoFile="$1.ino"
	find -name $inoFile | grep -q "ino"
	if [ $? -eq 0 ]
	then
		completeAPP
	else
		printUsage
	fi
fi

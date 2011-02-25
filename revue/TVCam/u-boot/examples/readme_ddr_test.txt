
1. DDR Test application codes are found in ddr_test.tar.gz and ddr_self_refresh.tar.gz 
2. Compilation of DDR test files using the u-boot/examples/Makefile. It is modified for setting the load address as 0x100
    hence in the binaries, the addresses are
    text = 0x100
    stack = U-Boot stack, it will be somewhere at 0x810XXXXX, as this test function is called from U-Boot "go" command
    stack for DDR test starts from 0x8101f258 on my board.
    The U-Boot area from 0x80000000 to 0x82000000 is excluded from DDR tests.
3. The steps to generate s-record files are as follows

    *     copy the files ddr_test.tar.gz and ddr_self_refresh.tar.gz to u-boot/examples folder
    *     cd u-boot/examples
    *     tar -xzvf ddr_test.tar.gz
    *     tar -xzvf ddr_self_refresh.tar.gz
    *     cd ..         (go to Logitech/trunk/Wolverine/u-boot)
    *     make clean
    *     make examples


    After these steps, the ddr_test.srec and ddr_self_refresh.srec files are generated in  u-boot/examples


Procedure to load the S-Rec into U-Boot is as follows
    *     Setup the board and communication through Hyperterminal or Minicom.
    *     On U-Boot Prompt type command "loads" and press enter
    *     In hyperterminal Transfer->send text file, select the srecord file and send 
	  In minicom CTRL+A S -> ascii and then enter the file name with complete path of srec and send
    *     press enter
    *     run the application using command "go 0x100"

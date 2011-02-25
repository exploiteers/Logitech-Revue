/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <exports.h>

#define SRPD		0x00800000
#define LPMODEN		0x80000000

#define SDRSR		0x2000000c;

int ddr_test (int argc, char *argv[])
{
    	int errors = 0;
    	int i,j=0;

        unsigned int ddr_base = 0x82000000;       // DDR memory
	unsigned int ddr_size = 0x06000000;       // 128 MB
	unsigned int pattern1 = 0xFFFFFFFF;      
	unsigned int pattern2 = 0x55555555;     
	unsigned int pattern3 = 0xAAAAAAAA;    
	unsigned int pattern4 = 0x00000000;   


        unsigned int start=ddr_base;
	unsigned int len = ddr_size;
	unsigned int end = start + len; 
	unsigned int val;
	
	unsigned int errorcount = 0;
	volatile unsigned int *pdata;



	/* Print the ABI version */
	app_startup(argv);
	printf ("Application expects ABI version %d\n", XF_VERSION);
	printf ("Actual U-Boot ABI version %d\n", (int)get_version());

	printf ("argc = %d\n", argc);

	for (i=0; i<=argc; ++i) {
		printf ("argv[%d] = \"%s\"\n",
			i,
			argv[i] ? argv[i] : "<NULL>");
	}


	/*******************
	 * self refresh mode	
	 * ****************/
	printf( "  > Enabling self refresh mode\n" );
	pdata = (volatile unsigned int *)SDRSR;
	(*pdata) &= ~SRPD;
	(*pdata) |= LPMODEN;


	printf( "  > Memory tests\n" );
	
	/*******************
	 * data test 1
	 * ****************/
	val = pattern1;
	errorcount = 0;
	printf( "    > Data Test - %d: Start:%08X, Size:%08X, Pattern: %08X\n", ++j, start, len, val);
	pdata = (unsigned int*)start;
		/* Write Pattern */
	for ( i = start; i < end; i += 4 )
	{
		*pdata++ = val;
	}
	/* Read Pattern */
	pdata = (unsigned int *)start;
	for ( i = start; i < end; i += 4 )
	{
		if ( *pdata++ != val )
		{
			errorcount++;
			break;
		}
	}
	if(errorcount)
	{
		errors += 1;
	}

	/*******************
	 * data test 2
	 * ****************/
	val = pattern2;
	printf( "    > Data Test - %d: Start:%08X, Size:%08X, Pattern: %08X\n",++j, start, len, val);
	errorcount = 0;
	pdata = (unsigned int*)start;
		/* Write Pattern */
	for ( i = start; i < end; i += 4 )
	{
		*pdata++ = val;
	}
	/* Read Pattern */
	pdata = (unsigned int *)start;
	for ( i = start; i < end; i += 4 )
	{
		if ( *pdata++ != val )
		{
			errorcount++;
			break;
		}
	}
	if(errorcount)
	{
		errors += 2;
	}

	/*******************
	 * data test 3
	 * ****************/
	val = pattern3;
	printf( "    > Data Test - %d: Start:%08X, Size:%08X, Pattern: %08X\n",++j, start, len, val);
	errorcount = 0;
	pdata = (unsigned int*)start;
		/* Write Pattern */
	for ( i = start; i < end; i += 4 )
	{
		*pdata++ = val;
	}
	/* Read Pattern */
	pdata = (unsigned int *)start;
	for ( i = start; i < end; i += 4 )
	{
		if ( *pdata++ != val )
		{
			errorcount++;
			break;
		}
	}
	if(errorcount)
	{
		errors += 4;
	}

	/*******************
	 * data test 4
	 * ****************/
	val = pattern4;
	printf( "    > Data Test - %d: Start:%08X, Size:%08X, Pattern: %08X\n",++j, start, len, val);
	errorcount = 0;
	pdata = (unsigned int*)start;
		/* Write Pattern */
	for ( i = start; i < end; i += 4 )
	{
		*pdata++ = val;
	}
	/* Read Pattern */
	pdata = (unsigned int *)start;
	for ( i = start; i < end; i += 4 )
	{
		if ( *pdata++ != val )
		{
			errorcount++;
			break;
		}
	}
	if(errorcount)
	{
		errors += 8;
	}

	/*******************
	 * exiting self refresh mode	
	 * ****************/
	printf( "  > Exiting self refresh mode\n" );
	pdata = (volatile unsigned int *)SDRSR;
	(*pdata) |= SRPD;
	(*pdata) &= ~LPMODEN;

	if ( errors )
		printf( "    > Error = 0x%x\n", errors );
	printf( "  > Memory tests over\n" );
	printf ("\n\n");
	return (0);
}


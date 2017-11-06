#include <stdio.h>
#include "files.h"

/****************************************************************************/
/*                                                                          */
/* 			     Module FILES                                   */
/*			External Declarations				    */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                          */
/*                                                                          */
/*                              Module FILESYS                              */
/*                             Internal Routines                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/





void files_init()
{
}



void openf(char *filename, OFILE *file)
{
}



EXIT_CODE closef(OFILE *file)
{
}



EXIT_CODE readf(OFILE *file, int position, int page_id, IORB  *iorb)
{
}



EXIT_CODE writef(OFILE *file, int position, int page_id, IORB *iorb)
{
}



void notify_files(IORB *iorb)
{
} 



EXIT_CODE allocate_blocks(INODE *inode, int numBlocksNeeded)
{
}



int search_file(char *filename)
{
}



int new_file(char *filename)
{ 
}



void delete_file(int dirNum)
{
}



/* end of module */

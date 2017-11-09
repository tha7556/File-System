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
    int i;
    for(i = 0; i < MAX_OPENFILE; i++) {
        theDirectory[i].free = true;
        theDirectory[i].filename = NULL;
        theDirectory[i].inode = &inodesTbl[i];
        inodesTbl[i].inode_id = i;
    }
    for(i = 0; i < MAX_DEV; i++) {
        Dev_Tbl[i].num_of_free_blocks = MAX_BLOCK;
        int j;
        for(j = 0; j < MAX_BLOCK; j++) {
            Dev_Tbl[i].free_blocks[j] = true;
        }
    }
}


//good luck
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
    int i;
    for(i = 0; i < MAX_OPENFILE; i++) {
        if(strcmp(filename,theDirectory[i].filename) == 0) { //they match
            return i;
        }
    }
    return -1;
}



int new_file(char *filename)
{
   int i;
   for(i = 0; i < MAX_OPENFILE; i++) {
        if(theDirectory[i].free == true) {
            theDirectory[i].filename = filename;
            theDirectory[i].free = false;
            theDirectory[i].inode->filesize = 0;
            theDirectory[i].inode->count = 0;

            int max = -1;
            int maxIndex = -1;
            int j;
            for(j = 0; j < MAX_DEV; j++) {
                if(Dev_Tbl[i].num_of_free_blocks > max) {
                    max = Dev_Tbl[i].num_of_free_blocks;
                    maxIndex = i;
                }
            }
            theDirectory[i].inode->dev_id = maxIndex;
            for(j = 0; j < MAX_BLOCK; j++) {
                theDirectory[i].inode->allocated_blocks[j] = -1;
            }
            return i;
        }
   }
   if (______trace_switch) printf("Error, no free entries");
   return -1;
}



void delete_file(int dirNum)
{
}



/* end of module */

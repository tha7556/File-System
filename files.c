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



void openf(char *filename, OFILE *file)
{
    INODE *node;
    if(search_file(filename) != -1) {
        node = theDirectory[search_file(filename)].inode;
    }
    else {
        node = theDirectory[new_file(filename)].inode;
    }
    file->inode = node;
    file->dev_id = file->inode->dev_id;
    file->iorb_count = 0;
    file->inode->count++;
}



EXIT_CODE closef(OFILE *file)
{
    if(file->iorb_count > 0){
        return fail; //pending I/O requests remaining, cannot be closed
    }
    file->inode->count = file->inode->count -1;
    if (file->inode->count == 0){
        delete_file(file->ofile_id);
    }
}




EXIT_CODE readf(OFILE *file, int position, int page_id, IORB  *iorb)
{
    PCB *pcb = PTBR->pcb;
    if(0 <= position && position < file->inode->filesize) {
       int num = position / PAGE_SIZE;
       file->iorb_count++;

       iorb->dev_id = file->inode->dev_id;
       iorb->block_id = file->inode->allocated_blocks[num];
       iorb->action = read;
       iorb->page_id = page_id;
       iorb->pcb = pcb;
       iorb->file = file;

       iorb->event->happened = false;
       Int_Vector.event = iorb->event;
       Int_Vector.iorb = iorb;
       Int_Vector.cause = iosvc;
       gen_int_handler();

       return ok;
    }
    else
        return fail;
}



EXIT_CODE writef(OFILE *file, int position, int page_id, IORB *iorb)
{
    PCB *pcb = PTBR->pcb;
    if(0 <= position && position < file->inode->filesize) {
       int num = position / PAGE_SIZE; //3
       int lastBlock;
       if(file->inode->filesize != 0) //potential error
        lastBlock = (file->inode->filesize - 1)/PAGE_SIZE;
       else
        lastBlock = -1;
        if(file->inode->allocated_blocks[num] > file->inode->allocated_blocks[lastBlock]) {
            if(allocate_blocks(file->inode,file->inode->filesize) == -1) {
                iorb->dev_id = -1;
                return fail;
            }
        }
        if(file->inode->filesize <= num) {
            file->inode->filesize = num+1;
        }
        file->iorb_count++;

       iorb->dev_id = file->inode->dev_id;
       iorb->block_id = file->inode->allocated_blocks[num];
       iorb->action = write;
       iorb->page_id = page_id;
       iorb->pcb = pcb;
       iorb->file = file;

       iorb->event->happened = false;
       Int_Vector.event = iorb->event;
       Int_Vector.iorb = iorb;
       Int_Vector.cause = iosvc;
       gen_int_handler();



       return ok;
    }
    else
        return fail;
}



void notify_files(IORB *iorb)
{
    iorb->file->iorb_count--;
}



EXIT_CODE allocate_blocks(INODE *inode, int numBlocksNeeded)
{
    if(Dev_Tbl[inode->dev_id].num_of_free_blocks < numBlocksNeeded) {
        return fail;
    }
    int blockNum;
    if(inode->filesize > 0)
        blockNum = inode->filesize -1 / (PAGE_SIZE + 1);
    else
        blockNum = 0;

    int i;
    for(i = 0; i < MAX_BLOCK; i++) {
        if(Dev_Tbl[inode->dev_id].free_blocks[i] == true) {
            Dev_Tbl[inode->dev_id].free_blocks[i] = false;
            Dev_Tbl[inode->dev_id].num_of_free_blocks--;
            int j;
            for(j = 0; j < MAX_BLOCK; j++) {
                if(inode->allocated_blocks[j] != -1) {
                    inode->allocated_blocks[j] = i;
                    break;
                }
            }
        }
    }
    return ok;
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

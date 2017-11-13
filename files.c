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

//Tyler Atkinson & Dylan Menchetti
void files_init() {
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



void openf(char *filename, OFILE *file) {
    INODE *node;
    if(search_file(filename) != -1) {
        node = theDirectory[search_file(filename)].inode;
    }
    else {
        node = theDirectory[new_file(filename)].inode;
    }
    file->inode = node;
    file->dev_id = node->dev_id;
    file->iorb_count = 0;
    file->inode->count++;
}



EXIT_CODE closef(OFILE *file) {
    if(file->iorb_count > 0){
        return fail;
    }
    file->inode->count--;
    if (file->inode->count == 0){
        delete_file(file->inode->inode_id);
    }
    return ok;
}




EXIT_CODE readf(OFILE *file, int position, int page_id, IORB  *iorb) {
    PCB *pcb = PTBR->pcb;
    if(0 <= position && position < file->inode->filesize) {
       int logicalBlock = position / PAGE_SIZE;
       file->iorb_count++;

       iorb->dev_id = file->inode->dev_id;
       iorb->block_id = file->inode->allocated_blocks[logicalBlock];
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
    else {
        iorb->dev_id = -1;
        return fail;
    }
}



EXIT_CODE writef(OFILE *file, int position, int page_id, IORB *iorb) {
    PCB *pcb = PTBR->pcb;
    if(0 <= position) {
        int logicalBlock = position / PAGE_SIZE;
        file->inode->dev_id = file->dev_id;
        int lastBlock = -1;
        if(file->inode->filesize != 0) {
            lastBlock = (file->inode->filesize - 1) / PAGE_SIZE;
        }
        if(logicalBlock > lastBlock) {
            if(allocate_blocks(file->inode,logicalBlock-lastBlock) == fail) {
                iorb->dev_id = -1;
                return fail;
            }
        }
        if(file->inode->filesize <= position) {
            file->inode->filesize = position + 1;
        }
        file->iorb_count++;
        iorb->dev_id = file->inode->dev_id;
        iorb->block_id = file->inode->allocated_blocks[logicalBlock];

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
    else {
        iorb->dev_id = -1;
        return fail;
    }
}



void notify_files(IORB *iorb) {
    iorb->file->iorb_count--;
}



EXIT_CODE allocate_blocks(INODE *inode, int numBlocksNeeded) {
    if(Dev_Tbl[inode->dev_id].num_of_free_blocks < numBlocksNeeded) {
        return fail;
    }
    int blockNum;
    if(inode->filesize > 0)
        blockNum = ((inode->filesize - 1) / PAGE_SIZE) + 1;
    else
        blockNum = 0;
    int i;
    int remaining = numBlocksNeeded;
    for(i = 0; i < MAX_BLOCK && remaining > 0; i++) {
        if(Dev_Tbl[inode->dev_id].free_blocks[i] == true) {
            Dev_Tbl[inode->dev_id].free_blocks[i] = false;
            Dev_Tbl[inode->dev_id].num_of_free_blocks--;
            inode->allocated_blocks[blockNum] = i;
            blockNum++;
            remaining--;
        }
    }
    return ok;
}



int search_file(char *filename) {
    int i;
    for(i = 0; i < MAX_OPENFILE; i++) {
        if(theDirectory[i].free != true && strcmp(filename,theDirectory[i].filename) == 0) {
            return i;
        }
    }
    return -1;
}



int new_file(char *filename) {
   int i;
   for(i = 0; i < MAX_OPENFILE; i++) {
        if(theDirectory[i].free == true)
        break;
   }
   if(theDirectory[i].free != true) {
    return -1;
   }
   theDirectory[i].filename = filename;
   theDirectory[i].free = false;
   theDirectory[i].inode->filesize = 0;
   theDirectory[i].inode->count = 0;
   int j, max = -1, maxIndex;
   for(j = 0; j < MAX_DEV; j++) {
        if(Dev_Tbl[j].num_of_free_blocks > max) {
            max = Dev_Tbl[j].num_of_free_blocks;
            maxIndex = j;
        }
   }
   theDirectory[i].inode->dev_id = maxIndex;
   for(j = 0; j < MAX_BLOCK; j++) {
    theDirectory[i].inode->allocated_blocks[j] = -1;
   }
   return i;
}



void delete_file(int dirNum) {
    int i;
    for(i = 0; i < MAX_BLOCK; i++) {
        int physicalBlock = theDirectory[dirNum].inode->allocated_blocks[i];
        if(physicalBlock != -1 && physicalBlock != 0) {
            Dev_Tbl[theDirectory[dirNum].inode->dev_id].free_blocks[physicalBlock] = true;
            Dev_Tbl[theDirectory[dirNum].inode->dev_id].num_of_free_blocks++;
            theDirectory[dirNum].inode->allocated_blocks[i] = -1;
        }
    }
    theDirectory[dirNum].free = true;
    theDirectory[dirNum].filename = NULL;
}



/* end of module */

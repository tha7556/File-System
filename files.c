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
    if (______trace_switch) printf("files_init()\n");
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
    if (______trace_switch) printf("openf(%s)\n",filename);
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
    if (______trace_switch) printf("closef(%d)\n",file->ofile_id);
    if(file->iorb_count > 0){
        return fail; //pending I/O requests remaining, cannot be closed
    }
    file->inode->count = file->inode->count -1;
    if (file->inode->count == 0){
        delete_file(file->ofile_id);
        return ok;
    }
}




EXIT_CODE readf(OFILE *file, int position, int page_id, IORB  *iorb)
{
    if (______trace_switch) printf("readf(%d)\n",file->inode);
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
    print_dir();
    print_disk_map();
    if (______trace_switch) printf("writef(file: %d, position: %d, page_id: %d)\n",file->ofile_id,position,page_id);

    PCB *pcb = PTBR->pcb; //1
    if(0 <= position) {//2
        int logicalBlock = position / PAGE_SIZE; //3
        file->inode->dev_id = file->dev_id;
        int lastBlock = -1;
        if (______trace_switch) printf("numFree: %d/%d\n",Dev_Tbl[file->dev_id].num_of_free_blocks,MAX_BLOCK);
        if(file->inode->filesize != 0) {
            lastBlock = (file->inode->filesize - 1) / PAGE_SIZE; //4
            if (______trace_switch) printf("lastBlock: %d\n",lastBlock);
        }
        if (______trace_switch) printf("logical: %d vs last: %d\n",logicalBlock,lastBlock);
        if (______trace_switch) printf("filesize: %d\n",file->inode->filesize);
        if(logicalBlock > lastBlock) { //5
            if(allocate_blocks(file->inode,logicalBlock-lastBlock) == fail) { //5 PROBLEM!
                iorb->dev_id = -1; //5
                if (______trace_switch) printf("RETURNING FAIL, not enough blocks!!\n");
                return fail; //5
            }
        }
        if(file->inode->filesize <= position) { //6
            file->inode->filesize = position + 1; //6
        }
        file->iorb_count++; //8
        iorb->dev_id = file->inode->dev_id; //9a
        iorb->block_id = file->inode->allocated_blocks[logicalBlock]; //9b
        if (______trace_switch) printf("block id: %d at index: %d\n",iorb->block_id,logicalBlock);

        iorb->action = write; // 9c
        iorb->page_id = page_id; //9d
        iorb->pcb = pcb; //9e
        iorb->file = file; //9f

        //10
        iorb->event->happened = false;
        Int_Vector.event = iorb->event;
        Int_Vector.iorb = iorb;
        Int_Vector.cause = iosvc;
        if (______trace_switch) printf("Handle it!\n");
        gen_int_handler();
        if (______trace_switch) printf("Done Writing\n");
        return ok; //11
    }
    else { //2
        if (______trace_switch) printf("RETURNING FAIL, position < 0!!\n");
        iorb->dev_id = -1;
        return fail;
    }
}



void notify_files(IORB *iorb)
{
    if (______trace_switch) printf("notify_files()\n");
    iorb->file->iorb_count--;
}



EXIT_CODE allocate_blocks(INODE *inode, int numBlocksNeeded)
{
    if (______trace_switch) printf("allocate_blocks() for: %d blocks\n",numBlocksNeeded);
    print_dir();
    print_disk_map();
    if (______trace_switch) printf("Need: %d and have: %d\n",numBlocksNeeded,Dev_Tbl[inode->dev_id].num_of_free_blocks);
    if(Dev_Tbl[inode->dev_id].num_of_free_blocks < numBlocksNeeded) {
            if (______trace_switch) printf("leaving allocate fail\nNeed: %d and have: %d\n",numBlocksNeeded,Dev_Tbl[inode->dev_id].num_of_free_blocks);
        return fail;
    }
    int blockNum;
    if(inode->filesize > 0)
        blockNum = ((inode->filesize - 1) / PAGE_SIZE) + 1;
    else
        blockNum = 0;
    if (______trace_switch) printf("Logical Block: %d\n",blockNum);
    int i;
    int remaining = numBlocksNeeded;
    for(i = 0; i < MAX_BLOCK && remaining > 0; i++) {
        if(Dev_Tbl[inode->dev_id].free_blocks[i] == true) {
            Dev_Tbl[inode->dev_id].free_blocks[i] = false;
            Dev_Tbl[inode->dev_id].num_of_free_blocks--;
            inode->allocated_blocks[blockNum] = i;
            if (______trace_switch) printf("Physical Block: %d at index: %d\n",inode->allocated_blocks[blockNum],blockNum);
            blockNum++;
            remaining--;
        }
    }
    if (______trace_switch) printf("leaving allocate ok\n");
    print_dir();
    print_disk_map();
    return ok;
}



int search_file(char *filename)
{
    if (______trace_switch) printf("search_file(%s)\n",filename);
    int i;
    for(i = 0; i < MAX_OPENFILE; i++) {
        if(theDirectory[i].filename != NULL && strcmp(filename,theDirectory[i].filename) == 0) { //they match
            return i;
        }
    }
    return -1;
}



int new_file(char *filename)
{
   if (______trace_switch) printf("new_file(%s)\n",filename);
   int i;
   for(i = 0; i < MAX_OPENFILE; i++) {
        if(theDirectory[i].free == true)
        break;
   }
   if(theDirectory[i].free != true) {
    if (______trace_switch) printf("Error, no free entries");
    return -1;
   }
   theDirectory[i].filename = filename;
   theDirectory[i].free = false;
   theDirectory[i].inode->filesize = 0;
   theDirectory[i].inode->count = 0;
   if (______trace_switch) printf("file: %s put at theDirectory[%d]\n",filename,i);
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



void delete_file(int dirNum)
{
    if (______trace_switch) printf("delete_file(%d)\n",dirNum);
    print_dir();
    print_disk_map();
    int i;
    for(i = 0; i < MAX_BLOCK; i++) {
        int b = theDirectory[dirNum].inode->allocated_blocks[i];
        if(b != -1 && b != 0) {
            Dev_Tbl[theDirectory[dirNum].inode->dev_id].free_blocks[b] = true;
            Dev_Tbl[theDirectory[dirNum].inode->dev_id].num_of_free_blocks++;
            theDirectory[dirNum].inode->allocated_blocks[i] = -1;
        }
    }
    theDirectory[dirNum].free = true;
    theDirectory[dirNum].filename = NULL;
    if (______trace_switch) printf("Done deleting");
    print_dir();
    print_disk_map();

}



/* end of module */

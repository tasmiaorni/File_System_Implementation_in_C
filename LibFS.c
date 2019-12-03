#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include "LibDisk.h"
#include "LibFS.h"

// set to 1 to have detailed debug print-outs and 0 to have none
#define FSDEBUG 0

#if FSDEBUG
#define dprintf printf
#else
#define dprintf noprintf
void noprintf(char* str, ...) {}
#endif

// the file system partitions the disk into five parts:

// 1. the superblock (one sector), which contains a magic number at
// its first four bytes (integer)
#define SUPERBLOCK_START_SECTOR 0

// the magic number chosen for our file system
#define OS_MAGIC 0xdeadbeef

// 2. the inode bitmap (one or more sectors), which indicates whether
// the particular entry in the inode table (#4) is currently in use
#define INODE_BITMAP_START_SECTOR 1

// the total number of bytes and sectors needed for the inode bitmap;
// we use one bit for each inode (whether it's a file or directory) to
// indicate whether the particular inode in the inode table is in use
#define INODE_BITMAP_SIZE ((MAX_FILES+7)/8)
#define INODE_BITMAP_SECTORS ((INODE_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)

// 3. the sector bitmap (one or more sectors), which indicates whether
// the particular sector in the disk is currently in use
#define SECTOR_BITMAP_START_SECTOR (INODE_BITMAP_START_SECTOR+INODE_BITMAP_SECTORS)

// the total number of bytes and sectors needed for the data block
// bitmap (we call it the sector bitmap); we use one bit for each
// sector of the disk to indicate whether the sector is in use or not
#define SECTOR_BITMAP_SIZE ((TOTAL_SECTORS+7)/8)
#define SECTOR_BITMAP_SECTORS ((SECTOR_BITMAP_SIZE+SECTOR_SIZE-1)/SECTOR_SIZE)

// 4. the inode table (one or more sectors), which contains the inodes
// stored consecutively
#define INODE_TABLE_START_SECTOR (SECTOR_BITMAP_START_SECTOR+SECTOR_BITMAP_SECTORS)

// an inode is used to represent each file or directory; the data
// structure supposedly contains all necessary information about the
// corresponding file or directory
typedef struct _inode
{
    int size; // the size of the file or number of directory entries
    int type; // 0 means regular file; 1 means directory
    int direntNo; // number of dirents written
    int data[MAX_SECTORS_PER_FILE]; // indices to sectors containing data blocks
} inode_t;

// the inode structures are stored consecutively and yet they don't
// straddle accross the sector boundaries; that is, there may be
// fragmentation towards the end of each sector used by the inode
// table; each entry of the inode table is an inode structure; there
// are as many entries in the table as the number of files allowed in
// the system; the inode bitmap (#2) indicates whether the entries are
// current in use or not
#define INODES_PER_SECTOR (SECTOR_SIZE/sizeof(inode_t))
#define INODE_TABLE_SECTORS ((MAX_FILES+INODES_PER_SECTOR-1)/INODES_PER_SECTOR)

// 5. the data blocks; all the rest sectors are reserved for data
// blocks for the content of files and directories
#define DATABLOCK_START_SECTOR (INODE_TABLE_START_SECTOR+INODE_TABLE_SECTORS)

// other file related definitions

// max length of a path is 256 bytes (including the ending null)
#define MAX_PATH 256

// max length of a filename is 16 bytes (including the ending null)
#define MAX_NAME 16

// max number of open files is 256
#define MAX_OPEN_FILES 256

// each directory entry represents a file/directory in the parent
// directory, and consists of a file/directory name (less than 16
// bytes) and an integer inode number
typedef struct _dirent
{
    char fname[MAX_NAME]; // name of the file
    int inode; // inode of the file
} dirent_t;

// the number of directory entries that can be contained in a sector
#define DIRENTS_PER_SECTOR (SECTOR_SIZE/sizeof(dirent_t))

// global errno value here
int osErrno;

// the name of the disk backstore file (with which the file system is booted)
static char bs_filename[1024];

/* the following functions are internal helper functions */

// check magic number in the superblock; return 1 if OK, and 0 if not
static int check_magic()
{
    char buf[SECTOR_SIZE];
    if(Disk_Read(SUPERBLOCK_START_SECTOR, buf) < 0)
        return 0;
    if(*(int*)buf == OS_MAGIC) return 1;
    else return 0;
}
int lookupArray[8]= {-128,-64,-32,-16,-8,-4,-2,-1};

void printSector(char *sec)
{
    for(int i=0; i< SECTOR_SIZE; i++)
        printf("%d", sec[i]);
}

// this function  gives a sector for a inode no
char* getSectorFromInode(int childInode, int *childSector)
{
    *childSector = INODE_TABLE_START_SECTOR + childInode/INODES_PER_SECTOR;
    char *sectorArray = malloc(SECTOR_SIZE);
    if(Disk_Read(*childSector, sectorArray) < 0)
    {
        printf("Garbage value \n");
        return "";
    }
    return sectorArray;
}

// this function  gives an inode reading from a sector
inode_t *getInodeFromSector(int sector, int inodeNumber, char *sectorArray)
{
    int start = (sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = inodeNumber-start;
    inode_t*  output;
    if(0 <= offset && offset < INODES_PER_SECTOR)
    {
        output = (inode_t*)(sectorArray+offset*sizeof(inode_t));
        return output;
    }
    inode_t*  error = (inode_t*)(0*sizeof(inode_t));
    return error;
}

int loopUp(int val)
{

    for (int k=0; k<8; k++)
    {
        if ( lookupArray[k]==val)
        {
            /* temp=lookupArray[k+1];
             index= (k+1);
             tempArray[j]=temp;
             Disk_Write(start + i, tempArray);
             return index;*/
            printf("matched\n");
            return 1;

        }
    }
    return 0;
}

// initialize a bitmap with 'num' sectors starting from 'start'
// sector; all bits should be set to zero except that the first
// 'nbits' number of bits are set to one
static void bitmap_init(int start, int num, int nbits)
{
    /* YOUR CODE */

    for (int i = 0; i < num; i++)   // loop through each sector of the disk
    {
        char tempArray[SECTOR_SIZE]; // take an empty  array of 512 byte for each sector and read data from disk
        memset(tempArray, 0, SECTOR_SIZE); //set 0 to that byte in disk
        for (int j = 0; j < SECTOR_SIZE; j++)   // loop through each byte of a sector
        {
            int bit = 7;
            while(bit >= 0)   // this loop will go through the 8 bits of a byte
            {
                if (nbits > 0) 	// check until nbits ==0 and set all bits  upto nbits one by one in a byte
                {
                    tempArray[j] = tempArray[j] | (1 << bit);
                }
                nbits--;
                bit--;
            }
        }
        Disk_Write(start + i, tempArray);     // write the updated value in disk
    }
}


// set the first unused bit from a bitmap of 'nbits' bits (flip the
// first zero appeared in the bitmap to one) and return its location;
// return -1 if the bitmap is already full (no more zeros)
static int bitmap_first_unused(int start, int num, int nbits)
{
    /* YOUR CODE */
    int index=0;

    for(int i=0; i< num; i++)   // loop through each sector of the disk
    {
        char tempArray[SECTOR_SIZE];       // take an empty  array of 512 byte for each sector and read data from disk
        Disk_Read(start + i, tempArray);    // read a byte from disk in tempArray
        for(int j =0 ; j< SECTOR_SIZE; j++)      // loop through each byte of a sector
        {
            int bit = 7;
            while(bit>=0)  // this loop will go through the 8 bits of a byte
            {
                char temp = tempArray[j] >> bit;   // take a bit
                temp = temp & 1;

                if( temp == 0)                   // check whether the bit is
                {
                    tempArray[j] =tempArray[j]  | (1 << bit);   // set the bit to 1
                    Disk_Write(start + i, tempArray);     // write the updated value in disk
                    return index;
                }
                bit--;
                index++;
            }
            //int temp = tempArray[j];
            //if (loopUp(temp))printf("matched");
        }
    }
    return -1;
}

// reset the i-th bit of a bitmap with 'num' sectors starting from
// 'start' sector; return 0 if successful, -1 otherwise
static int bitmap_reset(int start, int num, int ibit)
{
    /* YOUR CODE */
    int nthBit=0;  // this variable will keep track of the bit in the whole sector

    for ( int i=0; i<num; i++)   // loop through each sector of the disk
    {
        char tempArray[SECTOR_SIZE]; // take an empty  array of 512 byte for reading data from disk
        Disk_Read(start+i,tempArray);  // read a byte from disk in tempArray

        for (int j=0; j<SECTOR_SIZE; j++)     // loop through each byte of a sector
        {
            int bit=7;
            while(bit>=0)          // this loop will go through the 8 bits of a byte
            {
                if (nthBit== ibit) // if the bit number matches with the given inode number i.e we found the bit of that inode
                {
                    tempArray[j]=tempArray[j] &  (~(1<< bit));// set 0 to the matched  bit of the byte
                    Disk_Write(start+i,tempArray);  // write the updated value in disk
                    return 0;
                }
                nthBit++;
                bit--;
            }

        }
    }
    return -1;
}

// return 1 if the file name is illegal; otherwise, return 0; legal
// characters for a file name include letters (case sensitive),
// numbers, dots, dashes, and underscores; and a legal file name
// should not be more than MAX_NAME-1 in length
static int illegal_filename(char* name)
{
    /* YOUR CODE */

    if(strlen(name)>MAX_NAME-1)
    {
        return 1;
    }
    regex_t regex;
    int check=regcomp(&regex,"^[[:alnum:]._ -]*$",0);
    int status = regexec(&regex,name,0, NULL, 0);
    if(status == 0) return 0;
    else return 1;
}


// return the child inode of the given file name 'fname' from the
// parent inode; the parent inode is currently stored in the segment
// of inode table in the cache (we cache only one disk sector for
// this); once found, both cached_inode_sector and cached_inode_buffer
// may be updated to point to the segment of inode table containing
// the child inode; the function returns -1 if no such file is found;
// it returns -2 is something else is wrong (such as parent is not
// directory, or there's read error, etc.)
static int find_child_inode(int parent_inode, char* fname,
                            int *cached_inode_sector, char* cached_inode_buffer)
{
    int cached_start_entry = ((*cached_inode_sector)-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = parent_inode-cached_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* parent = (inode_t*)(cached_inode_buffer+offset*sizeof(inode_t));
    dprintf("... load parent inode: %d (size=%d, type=%d)\n",
            parent_inode, parent->size, parent->type);
    if(parent->type != 1)
    {
        dprintf("... parent not a directory\n");
        return -2;
    }

    int nentries = parent->size; // remaining number of directory entries
    int idx = 0;
    while(nentries > 0)
    {
        char buf[SECTOR_SIZE]; // cached content of directory entries
        if(Disk_Read(parent->data[idx], buf) < 0) return -2;
        for(int i=0; i<DIRENTS_PER_SECTOR; i++)
        {
            if(i>nentries) break;
            if(!strcmp(((dirent_t*)buf)[i].fname, fname))
            {
                // found the file/directory; update inode cache
                int child_inode = ((dirent_t*)buf)[i].inode;
                dprintf("... found child_inode=%d\n", child_inode);
                int sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
                if(sector != (*cached_inode_sector))
                {
                    *cached_inode_sector = sector;
                    if(Disk_Read(sector, cached_inode_buffer) < 0) return -2;
                    dprintf("... load inode table for child\n");
                }
                return child_inode;
            }
        }
        idx++;
        nentries -= DIRENTS_PER_SECTOR;
    }
    dprintf("... could not find child inode\n");
    return -1; // not found
}

// follow the absolute path; if successful, return the inode of the
// parent directory immediately before the last file/directory in the
// path; for example, for '/a/b/c/d.txt', the parent is '/a/b/c' and
// the child is 'd.txt'; the child's inode is returned through the
// parameter 'last_inode' and its file name is returned through the
// parameter 'last_fname' (both are references); it's possible that
// the last file/directory is not in its parent directory, in which
// case, 'last_inode' points to -1; if the function returns -1, it
// means that we cannot follow the path
static int follow_path(char* path, int* last_inode, char* last_fname)
{
    if(!path)
    {
        dprintf("... invalid path\n");
        return -1;
    }
    if(path[0] != '/')
    {
        dprintf("... '%s' not absolute path\n", path);
        return -1;
    }

    // make a copy of the path (skip leading '/'); this is necessary
    // since the path is going to be modified by strsep()
    char pathstore[MAX_PATH];
    strncpy(pathstore, path+1, MAX_PATH-1);
    pathstore[MAX_PATH-1] = '\0'; // for safety
    char* lpath = pathstore;

    int parent_inode = -1, child_inode = 0; // start from root
    // cache the disk sector containing the root inode
    int cached_sector = INODE_TABLE_START_SECTOR;
    char cached_buffer[SECTOR_SIZE];
    if(Disk_Read(cached_sector, cached_buffer) < 0) return -1;
    dprintf("... load inode table for root from disk sector %d\n", cached_sector);

    // for each file/directory name separated by '/'
    char* token;
    while((token = strsep(&lpath, "/")) != NULL)
    {
        dprintf("... process token: '%s'\n", token);
        if(*token == '\0') continue; // multiple '/' ignored
        if(illegal_filename(token))
        {
            dprintf("... illegal file name: '%s'\n", token);
            return -1;
        }
        if(child_inode < 0)
        {
            // regardless whether child_inode was not found previously, or
            // there was issues related to the parent (say, not a
            // directory), or there was a read error, we abort
            dprintf("... parent inode can't be established\n");
            return -1;
        }
        parent_inode = child_inode;
        child_inode = find_child_inode(parent_inode, token,
                                       &cached_sector, cached_buffer);
        if(last_fname) strcpy(last_fname, token);
    }
    if(child_inode < -1) return -1; // if there was error, abort
    else
    {
        // there was no error, several possibilities:
        // 1) '/': parent = -1, child = 0
        // 2) '/valid-dirs.../last-valid-dir/not-found': parent=last-valid-dir, child=-1
        // 3) '/valid-dirs.../last-valid-dir/found: parent=last-valid-dir, child=found
        // in the first case, we set parent=child=0 as special case
        if(parent_inode==-1 && child_inode==0) parent_inode = 0;
        dprintf("... found parent_inode=%d, child_inode=%d\n", parent_inode, child_inode);
        *last_inode = child_inode;
        return parent_inode;
    }
}

// add a new file or directory (determined by 'type') of given name
// 'file' under parent directory represented by 'parent_inode'
int add_inode(int type, int parent_inode, char* file)
{
    // get a new inode for child
    int child_inode = bitmap_first_unused(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, INODE_BITMAP_SIZE);
    if(child_inode < 0)
    {
        dprintf("... error: inode table is full\n");
        return -1;
    }
    dprintf("... new child inode %d\n", child_inode);

    // load the disk sector containing the child inode
    int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
    char inode_buffer[SECTOR_SIZE];
    if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
    dprintf("... load inode table for child inode from disk sector %d\n", inode_sector);

    // get the child inode
    int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    int offset = child_inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));

    // update the new child inode and write to disk
    memset(child, 0, sizeof(inode_t));
    child->type = type;
    if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
    dprintf("... update child inode %d (size=%d, type=%d), update disk sector %d\n",
            child_inode, child->size, child->type, inode_sector);

    // get the disk sector containing the parent inode
    inode_sector = INODE_TABLE_START_SECTOR+parent_inode/INODES_PER_SECTOR;
    if(Disk_Read(inode_sector, inode_buffer) < 0) return -1;
    dprintf("... load inode table for parent inode %d from disk sector %d\n",
            parent_inode, inode_sector);

    // get the parent inode
    inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
    offset = parent_inode-inode_start_entry;
    assert(0 <= offset && offset < INODES_PER_SECTOR);
    inode_t* parent = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
    dprintf("... get parent inode %d (size=%d, type=%d)\n",
            parent_inode, parent->size, parent->type);

    // get the dirent sector
    if(parent->type != 1)
    {
        dprintf("... error: parent inode is not directory\n");
        return -2; // parent not directory
    }
    //int group = parent->size/DIRENTS_PER_SECTOR;
    int group = parent->direntNo/DIRENTS_PER_SECTOR;
    char dirent_buffer[SECTOR_SIZE];
    if(group*DIRENTS_PER_SECTOR == parent->size)
        //if(group*DIRENTS_PER_SECTOR == parent->direntNo)
    {
        // new disk sector is needed
        int newsec = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
        if(newsec < 0)
        {
            dprintf("... error: disk is full\n");
            return -1;
        }
        parent->data[group] = newsec;
        memset(dirent_buffer, 0, SECTOR_SIZE);
        dprintf("... new disk sector %d for dirent group %d\n", newsec, group);
    }
    else
    {
        if(Disk_Read(parent->data[group], dirent_buffer) < 0)
            return -1;
        dprintf("... load disk sector %d for dirent group %d\n", parent->data[group], group);
    }

    // add the dirent and write to disk
    int start_entry = group*DIRENTS_PER_SECTOR;
    //offset = parent->size-start_entry;
    offset = parent->direntNo-start_entry;
    dirent_t* dirent = (dirent_t*)(dirent_buffer+offset*sizeof(dirent_t));
    strncpy(dirent->fname, file, MAX_NAME);
    dirent->inode = child_inode;
    if(Disk_Write(parent->data[group], dirent_buffer) < 0) return -1;
    dprintf("... append dirent %d (name='%s', inode=%d) to group %d, update disk sector %d\n",
            parent->size, dirent->fname, dirent->inode, group, parent->data[group]);

    // update parent inode and write to disk
    parent->size++;
    parent->direntNo++;
    if(Disk_Write(inode_sector, inode_buffer) < 0) return -1;
    dprintf("... update parent inode on disk sector %d\n", inode_sector);

    return 0;
}

// used by both File_Create() and Dir_Create(); type=0 is file, type=1
// is directory
int create_file_or_directory(int type, char* pathname)
{
    int child_inode;
    char last_fname[MAX_NAME];
    int parent_inode = follow_path(pathname, &child_inode, last_fname);
    if(parent_inode >= 0)
    {
        if(child_inode >= 0)
        {
            dprintf("... file/directory '%s' already exists, failed to create\n", pathname);
            osErrno = E_CREATE;
            return -1;
        }
        else
        {
            if(add_inode(type, parent_inode, last_fname) >= 0)
            {
                dprintf("... successfully created file/directory: '%s'\n", pathname);
                return 0;
            }
            else
            {
                dprintf("... error: something wrong with adding child inode\n");
                osErrno = E_CREATE;
                return -1;
            }
        }
    }
    else
    {
        dprintf("... error: something wrong with the file/path: '%s'\n", pathname);
        osErrno = E_CREATE;
        return -1;
    }
}

// remove the child from parent; the function is called by both
// File_Unlink() and Dir_Unlink(); the function returns 0 if success,
// -1 if general error, -2 if directory not empty, -3 if wrong type
int remove_inode(int type, int parent_inode, int child_inode)
{
    /* YOUR CODE */

    // first retrieve the child inode
    int *inode_sector;
    char *inode_buffer = getSectorFromInode(child_inode, &inode_sector); // get the sector no containing the child inode
    inode_t* child= getInodeFromSector(inode_sector, child_inode, inode_buffer);  // get the child inode from that sector

    // child inode is a directory with more than one directory or file. so we can't delete it
    if(child->type && child->size ) return -2;

    // Now remove all the data of the child inode from disk and update sector bitmap accordingly
    for (int sector=0; sector<MAX_SECTORS_PER_FILE; sector++)
    {
        if (child->data[sector])   // if child inode has data sector
        {
            bitmap_reset(SECTOR_BITMAP_START_SECTOR,SECTOR_BITMAP_SECTORS,child->data[sector]);
            // take the sector of the child from disk , make it 0, write back again in disk and reset sector bitmap
            char byteValue[SECTOR_SIZE];
            Disk_Read(child->data[sector], byteValue);
            memset(byteValue,0,SECTOR_SIZE);
            Disk_Write(child->data[sector], byteValue);
        }
    }

    // Next remove  the  child inode from disk and update inode  bitmap accordingly
    bitmap_reset(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, child_inode);
    memset(child, 0, sizeof(inode_t));
    Disk_Write(inode_sector, inode_buffer) ;

    // now deal with parent inode
    int * parentSector;
    char* parent_buffer = getSectorFromInode(parent_inode, &parentSector); // get the sector no containing the parent inode
    inode_t * parent=getInodeFromSector(parentSector, parent_inode,parent_buffer);  // get the parent inode from that sector

    if(parent->type != 1)    // check whether the parent inode  is a  directory or not
    {
        dprintf("... error: parent inode is not directory\n");
        return -1; // parent not directory
    }

    // now find the correct sector of parent inode to delete the child inode , here parent is always a directory
    for (int sector=0; sector<MAX_SECTORS_PER_FILE; sector++)
    {
        if (parent->data[sector])
        {
            char sectorArray[SECTOR_SIZE];
            if (Disk_Read(parent->data[sector],sectorArray)) return -1;;
            for(int j = 0; j < DIRENTS_PER_SECTOR; j++)      // loop thorugh the no of entries (25) in that sector
            {
                dirent_t* currentChild = (dirent_t*)(sectorArray + (j*sizeof(dirent_t)));
                if (currentChild->inode==child_inode)    // if the inode matches with the child inode that we want to remove
                {
                    memset(currentChild, 0, sizeof(dirent_t));  // clear the child inode value
                    parent->size--;
                    Disk_Write(parentSector, parent_buffer);    // first update the whole
                    Disk_Write(parent->data[sector], sectorArray);  // then update the new data sector where the child information is updated
                }
            }
        }
    }
    return 0;
}

// representing an open file
typedef struct _open_file
{
    int inode; // pointing to the inode of the file (0 means entry not used)
    int size;  // file size cached here for convenience
    int pos;   // read/write position
} open_file_t;
static open_file_t open_files[MAX_OPEN_FILES];

// return true if the file pointed to by inode has already been open
int is_file_open(int inode)
{
    for(int i=0; i<MAX_OPEN_FILES; i++)
    {
        if(open_files[i].inode == inode)
            return 1;
    }
    return 0;
}

// return a new file descriptor not used; -1 if full
int new_file_fd()
{
    for(int i=0; i<MAX_OPEN_FILES; i++)
    {
        if(open_files[i].inode <= 0)
            return i;
    }
    return -1;
}

/* end of internal helper functions, start of API functions */

int FS_Boot(char* backstore_fname)
{
    dprintf("FS_Boot('%s'):\n", backstore_fname);
    // initialize a new disk (this is a simulated disk)
    if(Disk_Init() < 0)
    {
        dprintf("... disk init failed\n");
        osErrno = E_GENERAL;
        return -1;
    }
    dprintf("... disk initialized\n");

    // we should copy the filename down; if not, the user may change the
    // content pointed to by 'backstore_fname' after calling this function
    strncpy(bs_filename, backstore_fname, 1024);
    bs_filename[1023] = '\0'; // for safety

    // we first try to load disk from this file
    if(Disk_Load(bs_filename) < 0)
    {
        dprintf("... load disk from file '%s' failed\n", bs_filename);

        // if we can't open the file; it means the file does not exist, we
        // need to create a new file system on disk
        if(diskErrno == E_OPENING_FILE)
        {
            dprintf("... couldn't open file, create new file system\n");

            // format superblock
            char buf[SECTOR_SIZE];
            memset(buf, 0, SECTOR_SIZE);
            *(int*)buf = OS_MAGIC;
            if(Disk_Write(SUPERBLOCK_START_SECTOR, buf) < 0)
            {
                dprintf("... failed to format superblock\n");
                osErrno = E_GENERAL;
                return -1;
            }
            dprintf("... formatted superblock (sector %d)\n", SUPERBLOCK_START_SECTOR);

            // format inode bitmap (reserve the first inode to root)
            bitmap_init(INODE_BITMAP_START_SECTOR, INODE_BITMAP_SECTORS, 1);
            dprintf("... formatted inode bitmap (start=%d, num=%d)\n",
                    (int)INODE_BITMAP_START_SECTOR, (int)INODE_BITMAP_SECTORS);

            // format sector bitmap (reserve the first few sectors to
            // superblock, inode bitmap, sector bitmap, and inode table)
            bitmap_init(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS,
                        DATABLOCK_START_SECTOR);
            dprintf("... formatted sector bitmap (start=%d, num=%d)\n",
                    (int)SECTOR_BITMAP_START_SECTOR, (int)SECTOR_BITMAP_SECTORS);

            // format inode tables
            for(int i=0; i<INODE_TABLE_SECTORS; i++)
            {
                memset(buf, 0, SECTOR_SIZE);
                if(i==0)
                {
                    // the first inode table entry is the root directory
                    ((inode_t*)buf)->size = 0;
                    ((inode_t*)buf)->type = 1;
                }
                if(Disk_Write(INODE_TABLE_START_SECTOR+i, buf) < 0)
                {
                    dprintf("... failed to format inode table\n");
                    osErrno = E_GENERAL;
                    return -1;
                }
            }
            dprintf("... formatted inode table (start=%d, num=%d)\n",
                    (int)INODE_TABLE_START_SECTOR, (int)INODE_TABLE_SECTORS);

            // we need to synchronize the disk to the backstore file (so
            // that we don't lose the formatted disk)
            if(Disk_Save(bs_filename) < 0)
            {
                // if can't write to file, something's wrong with the backstore
                dprintf("... failed to save disk to file '%s'\n", bs_filename);
                osErrno = E_GENERAL;
                return -1;
            }
            else
            {
                // everything's good now, boot is successful
                dprintf("... successfully formatted disk, boot successful\n");
                memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
                return 0;
            }
        }
        else
        {
            // something wrong loading the file: invalid param or error reading
            dprintf("... couldn't read file '%s', boot failed\n", bs_filename);
            osErrno = E_GENERAL;
            return -1;
        }
    }
    else
    {
        dprintf("... load disk from file '%s' successful\n", bs_filename);

        // we successfully loaded the disk, we need to do two more checks,
        // first the file size must be exactly the size as expected (thiis
        // supposedly should be folded in Disk_Load(); and it's not)
        int sz = 0;
        FILE* f = fopen(bs_filename, "r");
        if(f)
        {
            fseek(f, 0, SEEK_END);
            sz = ftell(f);
            fclose(f);
        }
        if(sz != SECTOR_SIZE*TOTAL_SECTORS)
        {
            dprintf("... check size of file '%s' failed\n", bs_filename);
            osErrno = E_GENERAL;
            return -1;
        }
        dprintf("... check size of file '%s' successful\n", bs_filename);

        // check magic
        if(check_magic())
        {
            // everything's good by now, boot is successful
            dprintf("... check magic successful\n");
            memset(open_files, 0, MAX_OPEN_FILES*sizeof(open_file_t));
            return 0;
        }
        else
        {
            // mismatched magic number
            dprintf("... check magic failed, boot failed\n");
            osErrno = E_GENERAL;
            return -1;
        }
    }
}

int FS_Sync()
{
    if(Disk_Save(bs_filename) < 0)
    {
        // if can't write to file, something's wrong with the backstore
        dprintf("FS_Sync():\n... failed to save disk to file '%s'\n", bs_filename);
        osErrno = E_GENERAL;
        return -1;
    }
    else
    {
        // everything's good now, sync is successful
        dprintf("FS_Sync():\n... successfully saved disk to file '%s'\n", bs_filename);
        return 0;
    }
}

int File_Create(char* file)
{
    dprintf("File_Create('%s'):\n", file);
    return create_file_or_directory(0, file);
}

int File_Unlink(char* file)
{
    /* YOUR CODE */

    int *childInode;
    char childfileName[MAX_NAME];
    int parentInode = follow_path(file, &childInode, childfileName);

    if(parentInode >= 0)  //if the parent is root or any other directory
    {
        if(childInode > 0)  //if the child is a valid inode
        {

            if( is_file_open(childInode))  // if the file is open
            {
                osErrno = E_FILE_IN_USE;
                return -1;
            }
            if( remove_inode(0, parentInode, childInode) >= 0)
            {
                dprintf("Successfully removed file: '%s'\n", childfileName);
                return 0;
            }

            else
            {
                osErrno=E_GENERAL;
                return -1;
            }

        }
        else    //if the child is not a valid inode
        {
            osErrno = E_NO_SUCH_FILE;
            return -1;
        }
    }

    return -1;
}

int File_Open(char* file)
{
    dprintf("File_Open('%s'):\n", file);
    int fd = new_file_fd();
    if(fd < 0)
    {
        dprintf("... max open files reached\n");
        osErrno = E_TOO_MANY_OPEN_FILES;
        return -1;
    }

    int child_inode;
    follow_path(file, &child_inode, NULL);
    if(child_inode >= 0)   // child is the one
    {
        // load the disk sector containing the inode
        int inode_sector = INODE_TABLE_START_SECTOR+child_inode/INODES_PER_SECTOR;
        char inode_buffer[SECTOR_SIZE];
        if(Disk_Read(inode_sector, inode_buffer) < 0)
        {
            osErrno = E_GENERAL;
            return -1;
        }
        dprintf("... load inode table for inode from disk sector %d\n", inode_sector);

        // get the inode
        int inode_start_entry = (inode_sector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
        int offset = child_inode-inode_start_entry;
        assert(0 <= offset && offset < INODES_PER_SECTOR);
        inode_t* child = (inode_t*)(inode_buffer+offset*sizeof(inode_t));
        dprintf("... inode %d (size=%d, type=%d)\n",
                child_inode, child->size, child->type);

        if(child->type != 0)
        {
            dprintf("... error: '%s' is not a file\n", file);
            osErrno = E_GENERAL;
            return -1;
        }

        // initialize open file entry and return its index
        open_files[fd].inode = child_inode;
        open_files[fd].size = child->size;
        open_files[fd].pos = 0;
        return fd;
    }
    else
    {
        dprintf("... file '%s' is not found\n", file);
        osErrno = E_NO_SUCH_FILE;
        return -1;
    }
}

int File_Read(int fd, void* buffer, int size)
{
    /* YOUR CODE */
    int fileInode= open_files[fd].inode;

    if (fileInode >=0)     // first check whether the file is open or not
    {
        // file is open so now load the disk sector containing the file inode
        int inodeSector = INODE_TABLE_START_SECTOR+open_files[fd].inode/INODES_PER_SECTOR;
        char inodeBuffer[SECTOR_SIZE];
        if(Disk_Read(inodeSector, inodeBuffer) < 0) return -1;
        dprintf("... load inode table for child inode from disk sector %d\n", inodeSector);


        // get the file  inode from the fetched sector
        int inodeStart = (inodeSector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
        int offset = open_files[fd].inode-inodeStart;
        assert(0 <= offset && offset < INODES_PER_SECTOR);
        inode_t* file = (inode_t*)(inodeBuffer+offset*sizeof(inode_t));

        // now read the data of the file

        int readPosition = open_files[fd].pos;
        if (readPosition == file->size)  // the file pointer is already at the end of the file
        {
            return 0;
        }

        int readCount = 0, totalRead;   // this will hold the no bytes we actually read from the file
        int startSector = readPosition / SECTOR_SIZE;  // In which sector the position belongs
        int startByte = readPosition % SECTOR_SIZE;   // in which byte of the sector
        int i = startSector;
        char tempBuffer[SECTOR_SIZE];           // the temp buffer we are putting our data ,for holding data which we read from disk

        while( i < MAX_SECTORS_PER_FILE)   //  start looking  from startsector upto Max sector(30)
        {
            totalRead = readPosition+readCount;
            if (file->data[i])              // check if there is data in this sector of the file
            {
                Disk_Read(file->data[i], tempBuffer);
                for(int j = startByte; j < SECTOR_SIZE; j++)   //start looking from startByte upto 512 byte
                {

                    if (totalRead < file->size && readCount < size) 	 //not exceeding the file
                    {
                        memcpy((void*) (buffer+readCount),(void*)(tempBuffer+j),sizeof(char));
                        readCount++;
                    }
                }

                startByte = 0;  // we need to make startbyte 0 for reading from the next sector
            }
            i++;
        }
        File_Seek(fd,open_files[fd].pos + readCount);     // update the position upto where we have done reading
        return readCount;
    }

    else  // if the file is not open
    {
        osErrno = E_BAD_FD;
        return -1;
    }


}

int File_Write(int fd, void* buffer, int size)
{
    /* YOUR CODE */

    int fileInode= open_files[fd].inode;
    int writePosition = open_files[fd].pos;

    if (fileInode >= 0)     //if the file is open
    {
        dprintf("File inode is found : %d\n",fileInode);
        // file is open so now load the disk sector containing the file inode
        int inodeSector = INODE_TABLE_START_SECTOR+open_files[fd].inode/INODES_PER_SECTOR;
        char inodeBuffer[SECTOR_SIZE];
        if(Disk_Read(inodeSector, inodeBuffer) < 0) return -1;

        // get the file  inode from the fetched sector
        int inodeStart = (inodeSector-INODE_TABLE_START_SECTOR)*INODES_PER_SECTOR;
        int offset = open_files[fd].inode-inodeStart;
        assert(0 <= offset && offset < INODES_PER_SECTOR);
        inode_t* file = (inode_t*)(inodeBuffer+offset*sizeof(inode_t));

        int checkSize = file->size + size;
        if( checkSize > MAX_FILE_SIZE)  //check whether the file exceeds the maximum file size
        {
            osErrno = E_FILE_TOO_BIG;
            return -1;
        }

        char* data = (char*) buffer; //the buffer which we are goinfg to write

        int writeCount = 0;                             // this will hold the no bytes we  wrote in the file
        int startSector = writePosition / SECTOR_SIZE;  // In which sector the position belongs
        int startByte = writePosition % SECTOR_SIZE;   // In which byte of the sector
        char tempBuffer[SECTOR_SIZE];                  //the temp buffer that will be used for holding sector data
        int tempSz = size;                   //temp to keep track we are not exceeding the size
        int i = startSector;

        while( i < MAX_SECTORS_PER_FILE && writeCount < size)  //  start looking  from startsector upto Max sector(30)
        {
            if (file->data[i])  // the sector of the file already contains some data
            {
                Disk_Read(file->data[i], tempBuffer);  // read the sector from disk

                for(int j = startByte; j < SECTOR_SIZE; j++) //loop through each byte of the sector
                {
                    if(tempSz>0)
                    {
                        memcpy((char*) (tempBuffer+j),(char*) (data+writeCount), sizeof(char));
                        writeCount++;
                        tempSz--;
                    }
                }
                Disk_Write(file->data[i], tempBuffer);   //update the disk sector with the new data of the file
                startByte = 0;    // we need to make startbyte 0 for reading again from the next sector

            }
            else     // allocate new sectors for this file
            {
                int newSector = bitmap_first_unused(SECTOR_BITMAP_START_SECTOR, SECTOR_BITMAP_SECTORS, SECTOR_BITMAP_SIZE);
                if(newSector < 0)  // no free space found in the sector bitmap
                {
                    osErrno = E_NO_SPACE;
                    return -1;
                }
                file->data[i] = newSector;
                char newBuf[SECTOR_BITMAP_SIZE];  // take an array of SECTOR_BITMAP_SIZE bytes as we don't know the size of buffer
                memset(newBuf, 0, SECTOR_SIZE);  //initialize them with 0
                char buffer[SECTOR_SIZE];  //take an array of 512 bytes
                for(int j = 0; j < SECTOR_SIZE; j++)
                {
                    if(tempSz>0)
                    {
                        memcpy((char*) (newBuf+j),(char*) (data+writeCount), sizeof(char));
                        writeCount++;
                        tempSz--;
                    }

                }
                Disk_Write(file->data[i], newBuf);
            }
            i++;
        }
        //now update the write position  and size of this file in the open files list
        File_Seek(fd,open_files[fd].pos+size);     // update the position upto where we have done reading
        file->size = file->size+size;   //increase the size of the file;
        open_files[fd].size = open_files[fd].size + size;
        Disk_Write(inodeSector, inodeBuffer);
        return size;
    }
    else   // if file is not open
    {
        osErrno = E_BAD_FD;
        return -1;
    }
}

int File_Seek(int fd, int offset)
{
    /* YOUR CODE */
    int fileInode= open_files[fd].inode; //getting the inode from the open file table.
    int fileSize = open_files[fd].size;
    if(offset < 0 || offset > fileSize) //if offset is negative or greater than size
    {
        osErrno = E_SEEK_OUT_OF_BOUNDS;
        return -1;
    }
    if (!fileInode)  // if file is not currently open
    {
        osErrno = E_BAD_FD ;
        return -1;
    }
    open_files[fd].pos =offset;  //update the the current location of the file pointer

    return open_files[fd].pos;
}

int File_Close(int fd)
{
    dprintf("File_Close(%d):\n", fd);
    if(0 > fd || fd > MAX_OPEN_FILES)
    {
        dprintf("... fd=%d out of bound\n", fd);
        osErrno = E_BAD_FD;
        return -1;
    }
    if(open_files[fd].inode <= 0)
    {
        dprintf("... fd=%d not an open file\n", fd);
        osErrno = E_BAD_FD;
        return -1;
    }

    dprintf("... file closed successfully\n");
    open_files[fd].inode = 0;
    return 0;
}

int Dir_Create(char* path)
{
    dprintf("Dir_Create('%s'):\n", path);
    return create_file_or_directory(1, path);
}

int Dir_Unlink(char* path)
{
    /* YOUR CODE */

    int *childInode;
    char childdirName[MAX_NAME];
    int parentInode = follow_path(path, &childInode, childdirName);

    if(parentInode >= 0)
    {
        if(childInode > 0)   // if it's  not a root directory
        {

            if (remove_inode(1, parentInode, childInode) >=0)
            {
                dprintf("Successfully removed directory from: '%s'\n", path);
                return 0;
            }
            else if(remove_inode(1, parentInode, childInode)== -2)
            {
                osErrno = E_DIR_NOT_EMPTY;
                return -1;
            }
            else
            {
                osErrno=E_GENERAL;
                return -1;

            }

        }
        else if (childInode == 0)  // if it's a root directory
        {
            osErrno=E_ROOT_DIR;
            return -1;
        }
        else
        {
            osErrno=E_NO_SUCH_DIR;
            return -1;
        }

    }
    return -1;

}


int Dir_Size(char* path)
{
    /* YOUR CODE */

    int childInode;
    char fileName[MAX_NAME];
    int parentInode=follow_path(path, &childInode, fileName);  // get the child inode number
    int * childSector;
    char *inode_buffer = getSectorFromInode(childInode, &childSector); // get the sector containing the child inode
    inode_t * result=getInodeFromSector(childSector, childInode, inode_buffer);  // get the inode from that sector
    if(result->type) return result->size * sizeof(dirent_t);
    return 0;
}

int Dir_Read(char* path, void* buffer, int size)
{
    /* YOUR CODE */

    int dirSize = Dir_Size(path);
    if(dirSize<size) //size is not big enough to contain all the entries
    {
        osErrno = E_BUFFER_TOO_SMALL;
        return -1;
    }
    int dirInode;
    char fileName[MAX_NAME];
    dirent_t* dirBuf[DIRENTS_PER_SECTOR] ;

    follow_path(path, &dirInode, fileName);
    if (dirInode == -1) return -1;
    int dirSector;

    char *inodeBuffer = getSectorFromInode(dirInode, &dirSector); //get inode buffer for dirInode via dirSector
    inode_t* directoryInode= getInodeFromSector(dirSector, dirInode, inodeBuffer); //get directory inode

    if(directoryInode->type)              // 1 means directory
    {
        int counter=0;
        for(int i=0; i<MAX_SECTORS_PER_FILE; i++)              // MAX_SECTORS_PER_FILE
        {

            char sectorBuffer[SECTOR_SIZE];
            if (Disk_Read(directoryInode->data[i], sectorBuffer) <0) return -1;

            for(int j=0; j<DIRENTS_PER_SECTOR; j++)                  // DIRENTS_PER_SECTOR= 512/20=25
            {
                dirent_t* entry = (dirent_t*)(sectorBuffer + (j*sizeof(dirent_t)));

                if(entry->inode )
                {
                    //dprintf("%-4d %-15s\t%-d\n", j, entry->fname, entry->inode);
                    memcpy((void*) (buffer+counter),(void*)entry, sizeof(dirent_t));
                    counter += sizeof(dirent_t);
                }
            }
        }
        return directoryInode->size;
    }
    return -1;
}



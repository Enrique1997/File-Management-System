/*
 * Name: Wazifa Jafar 
 * Description: This program creates an in-memory file system which will allow the users to copy a file into the file system,
 * retrieve a file or retrieve and place it in a new file, delete a file, list all the files in the file system and 
 * also find out the amount of space left in the file system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#define NUM_BLOCKS 1280		// Max number of blocks
#define BLOCK_SIZE 4096		// size of each block

unsigned char file_data[NUM_BLOCKS][BLOCK_SIZE]; // File System Blocks


// the directory containing all the information about each file
struct directory
{
  char name[255];
  int size;
  int block_number;
  char date[80];
  struct directory *next;
};

struct directory *head;

/*
 * Function: find_free_block
 * Parameter: bit_vector[] - an array that keeps track of free file blocks.
 * Returns: the index of the first unused block found
 * Description: it will iterate over the array to look for a free block 
 * whose index will be the corresponding index of the file block.
 * Hence, on finding this block it will change the bit vector to used and return the index.
 * 0s represent free blocks and 1s represent used.
 */
int find_free_block(int bit_vector[])
{
  int x = 0;
  for (x=0; x<1280; x++)
  {
    if (bit_vector[x] == 0)
    {
      bit_vector[x] = 1;
      return x;
    }
  }
  printf("No block is free.\n");
  return -1;
}

/*
 * Function: delete_index
 * Parameter: struct directory *head, char input[] - the first node of the linked list which contains 
 * file metadata and the input array which contains the user input of the file name 
 * that the user wants to delete.
 * Returns: the index number of the file if file is found;
 * else returns -1
 * Description: looks for the index number of the file, from the directory, that user wants to delete.
 * If file not found, returns -1; else returns the index number.
 */
int delete_index(struct directory *head, char input[])
{
  while(head != NULL)
  {
    if (strcmp(head->name, input)==0)
    {
      return head->block_number;
    }
    head = head->next;
  }
  return -1;
}


/*
 * Function: find_index
 * Parameter: index_table[128][32]- an array containing indexes of all the blocks of each file
 * Returns: index of a free block
 * Description: the index table contains indexes of all the blocks of each file
 * and has -1 for free blocks. The function will find such a free block and return its index.
 */
int find_index(int index_table[128][32])
{
  int x =0; 
  for (x =0; x<128; x++)
  {
    if (index_table[x][0] == -1)
    {
      return x;
    }
  }
}


/*
 * Function: list_files
 * Parameter: struct directory *head - the first node of the linked list
 * Returns - void
 * Description: The function will go through the linked list of each directory and 
 * print out the information of name, size and time of creation for the user.
 */
void list_files(struct directory *head)
{
  while (head != NULL)
  {
    printf("%d  %s  %s\n", head->size, head->date, head->name);
    head = head->next;
  }
}


int main( void ) 
{  
  // The array input will have the data given by the user and 
  // the tokenization will be done in the same array.
  // Each word will be stored in each row of **arr.
  
  char input[255], **arr;
  int status, i=0, blocks[1280], size_left=5242880, index_table[128][32], index, x =0;
  struct stat buf;
  struct directory *files;
  head = NULL;
  time_t rawtime;
  struct tm *info;
  char time_buffer[80];
  
  // filling up index table with -1 to show unused
  for (x =0; x<128; x++)
  {
    int y =0;
    for (y=0; y<32; y++)
    {
      index_table[x][y] = -1;
    }
  }           
   
  for (i=0; i<1280; i++)
  {
    blocks[i]= 0; // will keep track of free blocks; 0 represents free and 1 represents used
  }
  while (1)
  {
    printf("mfs> ");

    arr = malloc(4*sizeof(char*));
    char *temp;
    int i = -1;
    struct directory *files;

    //temp will hold each word of the tokenization and then 
    //store in the **arr
    fgets(input, 255, stdin);
    temp = strtok(input, " \r\n\0");

    // The program should go back to the beginning of the loop if the user doesn't enter anything
    if (temp == NULL)
    {
      continue;	
    } 
    
    arr[i] = malloc(strlen(temp));
    arr[i] = temp;
    i++;

    while (temp != NULL)
    {
      arr[i] = malloc(strlen(temp));
      arr[i] = temp;
      temp = strtok(NULL, " \r\n\0");
      i++;
    }

    if(strcmp(arr[0], "quit") == 0)
    {
      printf("Aborting!\n");
      exit(0);
    }
    
    // The put will copy the file into memory and also store its information in a directory	   
    else if (strcmp(arr[0], "put")== 0)
    {
      int exist =0;
      status =  stat( arr[1], &buf );
      if( status != -1 )
      {
        FILE *ifp = fopen (arr[1], "r" ); 
        int offset=0, copy_size=buf.st_size, block_index=-1;
        if (buf.st_size > size_left)
	{
	  printf("put error: Not enough disk space.\n");
	  continue;
	}
	else if(strlen(arr[1])> 255) 	// The file name cannot be more than 255 characters
	{
	  printf("put error: File name too long.");
	}
	else if (buf.st_size > 131072) 	// Not enough space to hold the file
	{
	  printf("The system does not support files larger than 131072 bytes.\n");
	  continue;
	}
	else
	{
	  struct directory *check_exist = head;
	  while (check_exist != NULL)
	  {
	    if (strcmp(check_exist->name, arr[1])==0)
   	    {
	      exist = 1;
	      break;
	    }
	    else
	    {
	      check_exist = check_exist->next;
	    }
	  }
	}
	if (exist == 1)
	{
	  printf("The file already exists in the system.\n");
	  continue;
	}
	index=find_index(index_table);   //calls the function to find free block from index table
	int pos=0;
	while( copy_size > 0 )
        {
          block_index = find_free_block(blocks);   // finding free block from bit vector
          fseek( ifp, offset, SEEK_SET );
          int bytes  = fread( file_data[block_index], BLOCK_SIZE, 1, ifp );
	  index_table[index][pos] = block_index;  
	  
          if( bytes == 0 && !feof( ifp ) )
          {
            printf("An error occured reading from the input file.\n");
            break;
          }
          clearerr( ifp );
          copy_size -= BLOCK_SIZE;
          offset    += BLOCK_SIZE;
	  ++pos;
        }		// finished copying
        struct directory* temp = malloc( sizeof(struct directory) );
        strcpy(temp->name, arr[1]);
	temp->size = buf.st_size;
        temp->next = head;
	temp->block_number = index;       
        size_left -= buf.st_size;
	time (&rawtime);
	info = localtime(&rawtime);
	strcpy(temp->date, asctime(info));
        temp->date[strlen(temp->date)-1] = '\0';
	head= temp;
        fclose(ifp);
      }
      else
      {
        printf("Unable to open file: %s\n", arr[1] );
        perror("Opening the input file returned: ");
        continue;
      }    
    }

    // will get the file from the file system and copy into the current directory
    else if (strcmp(arr[0], "get")== 0)
    {
      status =  stat( arr[1], &buf ); 
      if (status == -1)
      {
        printf("Unable to open file: %s\n", arr[1] );
        perror("Opening the input file returned: ");
    	continue;
      }
      FILE *ofp;
      FILE *ifp = fopen ( arr[1], "r" ); 
     
      if (arr[2] == NULL)
      {
	arr[2] = malloc(strlen(arr[1]));
        arr[2] = arr[1];
      }
      ofp = fopen(arr[2], "w");
      int offset = 0, copy_size = buf.st_size,pos=-1, ind= -1;
      struct directory *temp = head;
      while (temp != NULL)
      {
        if (strcmp(arr[1],temp->name)==0)
	{
	  ind = temp->block_number;
	  break;
	}
        else 
	{
	  temp = temp->next;
	}
      }
      if (ind == -1)
      {
	printf("Could not find the file.\n");
        continue;
      }
      int block_index;
      int bytes  = fread( file_data[block_index], BLOCK_SIZE, 1, ifp );
      if( ofp == NULL )
      {
        printf("Could not open output file: %s\n", arr[2] );
        perror("Opening output file returned");
        continue;
      }

      while(copy_size > 0)
      {
	block_index = index_table[ind][++pos];
        int num_bytes;
	if (copy_size < BLOCK_SIZE)
	{
	  num_bytes = copy_size;
	}
	else
	{
	  num_bytes = BLOCK_SIZE;
	}
	fwrite( file_data[block_index], num_bytes, 1, ofp ); 
	copy_size -= BLOCK_SIZE;
	offset    += BLOCK_SIZE;
      }
      fclose( ofp );
    }

    else if (strcmp(arr[0], "del")== 0)
    {
      int pos = delete_index(head, arr[1]); 
      if (pos != -1)
      {
	int x =0;
        for (x =0; x< 32; x++)	// setting the index table back free to be used by other files
	{
	  blocks[index_table[pos][x]]=0;		// setting bit vector to value of free
	  index_table[pos][x] = -1;
	}
        struct directory *del = head;
        if (strcmp(del->name, arr[1])==0)   // deleting for the first directory of the list
        {
	  size_left += del->size;
          head= del->next;
	  continue;
        }
        else
        {
          del = del->next;
	  struct directory *temp_head = head;
	  while (del != NULL)
	  {
	    if (strcmp(del->name, arr[1])== 0)
	    {
	      if (del->next != NULL)	// deleting for any directory in the middle of the list
	      {
		size_left += del->size;
	        temp_head->next = del->next;
		break;
	      }
	      else if (del->next == NULL)  // deleting for the last directory of the list
	      {
		size_left += del->size;
	        temp_head->next = NULL;
		break;
	      }
	    }
	    else 
	    {
	      temp_head = temp_head->next;
	      del = del->next;
	    }
	  }
        }
      }
      else
      {
        printf("del error: File not found.\n");
      }
    }

    // lists the existing files in the system
    else if (strcmp(arr[0], "list")== 0)
    {
      if (head == NULL)
      {
        printf("list: No files found.\n");
        continue;
      }
      list_files(head);
    }
  
    // displays the amount of free space left in the disk
    else if (strcmp(arr[0], "df")== 0)
    {
      printf("%d bytes left\n", size_left);
    }
    else
    {
      printf("Command not found.\n");
    }
  }
    
  return 0 ;
}

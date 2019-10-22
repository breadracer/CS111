/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "ext2_fs.h"

// helper functions
void error(char *msg, int code) {
  fprintf(stderr, "%s\n", msg);
  exit(code);
}

int btod(__u8 *b, int len) {
  int d = 0;
  for (int i = 0; i < len; i++)
    d += b[i] << (8 * i);
  return d;
}

void scan_indirect_directory_entries(__u8 *block_buf, int level, int fd,
				     int inode_num, int block_size) {
  if (level == 0) {
    // scan the entries in one data block
    int byte_offset = 0;
    while (byte_offset < block_size) {
      struct ext2_dir_entry de;
      de.inode = btod(block_buf + byte_offset, 4);
      de.rec_len = btod(block_buf + 4 + byte_offset, 2);
      de.name_len = btod(block_buf + 6 + byte_offset, 1);
      if (de.inode == 0) {
	byte_offset += de.rec_len;
	continue;		
      }
      for (int i = 0; i < de.name_len; i++)
	de.name[i] = btod(block_buf + 8 + i + byte_offset, 1);
      de.name[de.name_len] = 0;
      printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_num, byte_offset,
	     de.inode, de.rec_len, de.name_len, de.name);
      byte_offset += de.rec_len;
    }
  } else {
    // scan the indirect blocks
    __u32 *block_buf_ind = (__u32*)block_buf;
    for (int i = 0; i < block_size / 4; i++) {
      if (block_buf_ind[i] != 0) {
	int block_num = block_buf_ind[i];
	__u8 *new_buf = malloc(block_size);
	int offset = block_num * block_size;
	pread(fd, new_buf, block_size, offset);
	scan_indirect_directory_entries(new_buf, level - 1, fd, inode_num, block_size);
	free(new_buf);
      }
    }
  }
}

void scan_indirect_blocks(__u32 *block_arr, int level, int *block_offset, int fd,
			  int inode_number, int block_size, int block_number) {
  if (level == 1) {
    for (int i = 0; i < block_size / 4; i++) {
      if (block_arr[i] != 0) 
	printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_number, level,
	       *block_offset, block_number, block_arr[i]);
      (*block_offset)++;
    }
  } else {
    for (int i = 0; i < block_size / 4; i++) {
      if (block_arr[i] != 0) {
	printf("INDIRECT,%d,%d,%d,%d,%d\n", inode_number, level,
	       *block_offset, block_number, block_arr[i]);
	int block_num = block_arr[i];
	__u32 *block_arr_new = malloc(block_size);
	__u8 *ind_block_buf = malloc(block_size);
	int offset = block_num * block_size;
	pread(fd, ind_block_buf, block_size, offset);
	for (int j = 0; j < block_size / 4; j++)
	  block_arr_new[j] = btod(ind_block_buf + 4 * j, 4);
	scan_indirect_blocks(block_arr_new, level - 1, block_offset,
			     fd, inode_number, block_size, block_num);
	free(block_arr_new);
	free(ind_block_buf);
      } else
	*block_offset += 256;
    }
  }
}

int main(int argc, char **argv) {
  // initialize variables
  int fd, ngroups, block_size;
  int offset = 0;
  
  // check arguments
  if (argc != 2)
    error("usage: lab3a FILE", 1);
  
  // open fs image
  fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    error(strerror(errno), 1);

  // read superblock
  struct ext2_super_block sb;
  offset += EXT2_MIN_BLOCK_SIZE;
  __u8 *super_block_buf = malloc(sizeof(__u8) * EXT2_MIN_BLOCK_SIZE);
  pread(fd, super_block_buf, EXT2_MIN_BLOCK_SIZE, offset);
  sb.s_blocks_count = btod(super_block_buf + 4, 4);
  sb.s_inodes_count = btod(super_block_buf, 4);
  sb.s_log_block_size = btod(super_block_buf + 24, 4);
  sb.s_inode_size = btod(super_block_buf + 88, 2);
  sb.s_blocks_per_group = btod(super_block_buf + 32, 4);
  sb.s_inodes_per_group = btod(super_block_buf + 40, 4);
  sb.s_first_ino = btod(super_block_buf + 84, 4);
  ngroups = sb.s_blocks_count / sb.s_blocks_per_group + 1;
  block_size = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;
  printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",sb.s_blocks_count,
	 sb.s_inodes_count, block_size, sb.s_inode_size,
	 sb.s_blocks_per_group, sb.s_inodes_per_group, sb.s_first_ino);
  free(super_block_buf);
  
  // read group descriptor table
  struct ext2_group_desc *gds = malloc(sizeof(struct ext2_group_desc) * ngroups);
  offset += EXT2_MIN_BLOCK_SIZE;
  __u8 *group_desc_table_buf = malloc(sizeof(__u8) * block_size);
  pread(fd, group_desc_table_buf, block_size, offset);
  for (int i = 0; i < ngroups; i++) {
    gds[i].bg_free_blocks_count = btod(group_desc_table_buf + 12 + i * 32, 2);
    gds[i].bg_free_inodes_count = btod(group_desc_table_buf + 14 + i * 32, 2);
    gds[i].bg_block_bitmap = btod(group_desc_table_buf + i * 32, 4);
    gds[i].bg_inode_bitmap = btod(group_desc_table_buf + 4 + i * 32, 4);
    gds[i].bg_inode_table = btod(group_desc_table_buf + 8 + i * 32, 4);
    int nblocks = i == ngroups - 1 ? sb.s_blocks_count : sb.s_blocks_per_group;
    int ninodes = i == ngroups - 1 ? sb.s_inodes_count : sb.s_inodes_per_group;
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, nblocks, ninodes,
	   gds[i].bg_free_blocks_count, gds[i].bg_free_inodes_count,
	   gds[i].bg_block_bitmap, gds[i].bg_inode_bitmap, gds[i].bg_inode_table);

    __u8 *bitmap_buf = malloc(sizeof(__u8) * block_size);
    
    // read block bitmap
    offset = gds[i].bg_block_bitmap * block_size;
    pread(fd, bitmap_buf, block_size, offset);
    for (int j = 0; j < nblocks; j++)
      if (!(bitmap_buf[j / 8] & (1 << j % 8)))
	printf("BFREE,%d\n", j + 1);
    
    // read inode bitmap
    offset = gds[i].bg_inode_bitmap * block_size;
    pread(fd, bitmap_buf, block_size, offset);
    for (int j = 0; j < ninodes; j++)
      if (!(bitmap_buf[j / 8] & (1 << j % 8)))
	printf("IFREE,%d\n", j + 1);

    free(bitmap_buf);
    __u8 *inode_table_buf = malloc(sizeof(__u8) * ninodes * sb.s_inode_size);

    // read inodes
    struct ext2_inode *ins = malloc(sizeof(struct ext2_inode) * ninodes);
    offset = gds[i].bg_inode_table * block_size;
    pread(fd, inode_table_buf, ninodes * sb.s_inode_size, offset);
    for (int j = 0; j < ninodes; j++) {
      ins[j].i_mode = btod(inode_table_buf + j * sb.s_inode_size, 2);
      ins[j].i_uid = btod(inode_table_buf + 2 + j * sb.s_inode_size, 2);
      ins[j].i_gid = btod(inode_table_buf + 24 + j * sb.s_inode_size, 2);
      ins[j].i_links_count = btod(inode_table_buf + 26 + j * sb.s_inode_size, 2);
      ins[j].i_ctime = btod(inode_table_buf + 12 + j * sb.s_inode_size, 4);
      ins[j].i_mtime = btod(inode_table_buf + 16 + j * sb.s_inode_size, 4);
      ins[j].i_atime = btod(inode_table_buf + 8 + j * sb.s_inode_size, 4);
      ins[j].i_size = btod(inode_table_buf + 4 + j * sb.s_inode_size, 4);
      ins[j].i_blocks = btod(inode_table_buf + 28 + j * sb.s_inode_size, 4);
      for (int k = 0; k < EXT2_N_BLOCKS; k++)
	ins[j].i_block[k] = btod(inode_table_buf + 40 + k * 4 + j * sb.s_inode_size, 4);
      if (ins[j].i_mode == 0 || ins[j].i_links_count == 0)
	continue;
      int inode_number = j + 1 + i * sb.s_inodes_per_group;
      char file_type;
      switch (ins[j].i_mode >> 12) {
      case 0xa:
      	file_type = 's';
	break;
      case 0x8:
      	file_type = 'f';
	break;
      case 0x4:
      	file_type = 'd';
	break;
      default:
      	file_type = '?';
      }
      int mode = ins[j].i_mode & 0xfff;
      char ctime_str[18], mtime_str[18], atime_str[18];
      time_t ctime = ins[j].i_ctime;
      time_t mtime = ins[j].i_mtime;
      time_t atime = ins[j].i_atime;
      strftime(ctime_str, sizeof(ctime_str), "%D %T", gmtime(&ctime));
      strftime(mtime_str, sizeof(mtime_str), "%D %T", gmtime(&mtime));
      strftime(atime_str, sizeof(atime_str), "%D %T", gmtime(&atime));
      printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", inode_number,
      	     file_type, mode, ins[j].i_uid, ins[j].i_gid, ins[j].i_links_count,
      	     ctime_str, mtime_str, atime_str, ins[j].i_size, ins[j].i_blocks);
      switch (file_type) {
      case 'f':
      case 'd': {
	for (int k = 0; k < EXT2_N_BLOCKS; k++)
	  printf(",%d", ins[j].i_block[k]);
	printf("\n");
	break;
      }
      case 's': {
	if (ins[j].i_size >= 60)
	  for (int k = 0; k < EXT2_N_BLOCKS; k++)
	    printf(",%d", ins[j].i_block[k]);
	printf("\n");
	break;
      }
      default:
	printf("\n");
      }

      // dump directory information
      if (file_type == 'd') {
	// scan direct blocks
	for (int k = 0; k < EXT2_NDIR_BLOCKS; k++)
	  if (ins[j].i_block[k] == 0)
	    continue;
	  else {
	    int block_number = ins[j].i_block[k];
	    __u8 *data_block_buf = malloc(block_size);
	    offset = block_number * block_size;
	    pread(fd, data_block_buf, block_size, offset);
	    scan_indirect_directory_entries(data_block_buf, 0, fd,
					    inode_number, block_size);
	    free(data_block_buf);
	  }
	// scan indirect blocks
	for (int k = 0; k < 3; k++) {
	  __u8 *data_block_buf = malloc(block_size);
	  int block_number = ins[j].i_block[EXT2_IND_BLOCK + k];
	  offset = block_number * block_size;
	  pread(fd, data_block_buf, block_size, offset);
	  scan_indirect_directory_entries(data_block_buf, k + 1, fd,
					  inode_number, block_size);
	  free(data_block_buf);
	}
      }

      // scan indirect block references
      int block_offset = EXT2_NDIR_BLOCKS;
      int inode_num = inode_number;
      __u32 *block_arr = malloc(block_size);
      __u8 *ind_block_buf = malloc(block_size);
      for (int k = 0; k < 3; k++) {
	int block_num = ins[j].i_block[EXT2_IND_BLOCK + k];
	offset = block_num * block_size;
	pread(fd, ind_block_buf, block_size, offset);
	for (int l = 0; l < block_size / 4; l++)
	  block_arr[l] = btod(ind_block_buf + 4 * l, 4);
	scan_indirect_blocks(block_arr, k + 1, &block_offset, fd,
			     inode_num, block_size, block_num);	
      }      
      free(block_arr);
      free(ind_block_buf);
    }    
    free(ins);
    free(inode_table_buf);    
  }
  free(group_desc_table_buf);
  free(gds);  
  return 0;
}

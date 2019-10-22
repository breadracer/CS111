import sys
import csv

def check_fs_consistency(summary):
    # gather information from summary
    superblock_summary = [row for row in summary if row[0] == 'SUPERBLOCK'][0]
    group_summary = [row for row in summary if row[0] == 'GROUP'][0]
    inode_summary = [row for row in summary if row[0] == 'INODE']
    indirect_summary = [row for row in summary if row[0] == 'INDIRECT']
    dirent_summary = [row for row in summary if row[0] == 'DIRENT']
    
    free_block_table = [int(row[1]) for row in summary if row[0] == 'BFREE']
    free_inode_table = [int(row[1]) for row in summary if row[0] == 'IFREE']
    
    maximum_blocks = int(superblock_summary[1])
    first_inode = int(superblock_summary[7])
    n_inodes_per_group = int(group_summary[3])
    block_num_inode_table = int(group_summary[8])
    inode_size = int(superblock_summary[4])
    block_size = int(superblock_summary[3])

    block_num_first_data_block = int(block_num_inode_table + n_inodes_per_group * inode_size / block_size)
    data_block_nums = list(range(block_num_first_data_block, maximum_blocks))
    inode_nums = list(range(first_inode, n_inodes_per_group + 1))


    allocated_blocks = {}
    allocated_inodes = {}
    level_names = ('', 'INDIRECT ', 'DOUBLE INDIRECT ', 'TRIPLE INDIRECT ')
    
    # Start of Block Consistency Audits
    # check inconsistent blocks in each inode and record inode information for future use
    for inode in inode_summary:        
        file_size = int(inode[10])
        file_type = str(inode[2])
        inode_num = int(inode[1])
        link_count = int(inode[6])
        # record inode information
        allocated_inodes[inode_num] = {
            'link_count': link_count,
            'real_links': 0
        }
        if file_type == 's' and file_size <= 60:
            continue
        for i, block in enumerate(inode[slice(12, 27)]):
            block_num = int(block)
            if block_num == 0:
                continue
            level_offset = ((0, i), (1, i), (2, 268), (3, 65804))
            level, offset = level_offset[0] if i < 12 else level_offset[i - 11]
            # invalid block check
            if block_num < 0 or block_num >= maximum_blocks:
                print('INVALID {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                    level_names[level], block_num, inode_num, offset
                ))
                continue
            # reserved block check
            if block_num < block_num_first_data_block:
                print('RESERVED {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                    level_names[level], block_num, inode_num, offset
                ))
                continue
            # if legal block, check duplication
            new_block = {
                'inode': inode_num,
                'level': level,
                'offset': offset,
                'printed': False
            }
            if block_num in allocated_blocks:
                allocated_blocks[block_num].append(new_block)
                for b in allocated_blocks[block_num]:
                    if not b['printed']:
                        print('DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                            level_names[b['level']], block_num, b['inode'], b['offset']
                        ))
                    b['printed'] = True
            else:
                allocated_blocks[block_num] = [new_block]
                
    # check inconsistent blocks in each indirect block
    for indirect_block in indirect_summary:
        block_num = int(indirect_block[5])
        inode_num = int(indirect_block[1])
        level = int(indirect_block[2])
        offset = int(indirect_block[3])
        # invalid block check
        if block_num < 0 or block_num >= maximum_blocks:
            print('INVALID {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                level_names[level], block_num, inode_num, offset
            ))
            continue
        # reserved block check
        if block_num < block_num_first_data_block:
            print('RESERVED {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                level_names[level], block_num, inode_num, offset
            ))
            continue
        # if legal block, check duplication
        new_block = {
            'inode': inode_num,
            'level': level,
            'offset': offset,
            'printed': False
        }
        if block_num in allocated_blocks:
            allocated_blocks[block_num].append(new_block)
            for b in allocated_blocks[block_num]:
                if not b['printed']:
                    print('DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}'.format(
                        level_names[b['level']], block_num, b['inode'], b['offset']
                    ))
                    b['printed'] = True
        else:
            allocated_blocks[block_num] = [new_block]

    # check for unreferenced blocks
    for block in data_block_nums:
        block_num = int(block)
        if block_num not in free_block_table and block_num not in allocated_blocks:
            print('UNREFERENCED BLOCK {}'.format(block_num))
        
    # check for allocated blocks
    for block in allocated_blocks:
        block_num = int(block)
        if block_num in free_block_table:
            print('ALLOCATED BLOCK {} ON FREELIST'.format(block_num))


    # Start of I-node Allocation Audits
    # check for allocated inodes
    for inode_num in free_inode_table:
        if inode_num in allocated_inodes:
            print('ALLOCATED INODE {} ON FREELIST'.format(inode_num))

    # check for unallocated inodes
    for inode_num in inode_nums:
        if inode_num not in allocated_inodes and inode_num not in free_inode_table:
            print('UNALLOCATED INODE {} NOT ON FREELIST'.format(inode_num))

            
    # Start of Directory Consistency Audits
    # check for inconsistent link count
    dirent_parent_map = {}
    for d in dirent_summary:
        if int(d[3]) not in dirent_parent_map:
            dirent_parent_map[int(d[3])] = int(d[1])
    
    for dirent in dirent_summary:
        inode_num = int(dirent[1])
        file_inode_num = int(dirent[3])
        file_name = str(dirent[6])
        if file_inode_num < 0 or file_inode_num > n_inodes_per_group:
            print('DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(
                inode_num, file_name, file_inode_num
            ))
        elif file_inode_num not in allocated_inodes:
            print('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(
                inode_num, file_name, file_inode_num                
            ))
        else:
            allocated_inodes[file_inode_num]['real_links'] += 1
        if file_name == "'.'" and file_inode_num != inode_num:
            print('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(
                inode_num, file_name, file_inode_num, inode_num
            ))
        elif file_name == "'..'":
            parent_dirent_inode_num = dirent_parent_map[inode_num]
            if parent_dirent_inode_num != file_inode_num:
                print('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(
                    inode_num, file_name, file_inode_num, parent_dirent_inode_num
                ))

    for inode_n, inode in allocated_inodes.items():
        if inode['real_links'] != inode['link_count']:
            print('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(
                inode_n, inode['real_links'], inode['link_count']
            ))


if __name__ == '__main__':
    if len(sys.argv) > 2:
        sys.stderr.write('Usage: lab3b FILE')
        exit(1)
    try:
        with open(sys.argv[1]) as csv_file:
            reader = list(csv.reader(csv_file))
            check_fs_consistency(reader)
    except IOError as err:
        sys.stderr.write('I/O error ({0}): {1}'.format(err.errno, err.strerror))
        exit(1)


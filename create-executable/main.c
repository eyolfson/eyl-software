// Copyright 2015 Jonathan Eyolfson
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <http://www.gnu.org/licenses/>.

#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

unsigned char instructions[] = {
    0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00, /* mov $0x3c,%rax */
    0x48, 0xc7, 0xc7, 0x2a, 0x00, 0x00, 0x00, /* mov $0x2a,%rdi */
    0x0f, 0x05                                /* syscall */
};

int main(int argc, char **argv)
{
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    int fd = open("136-byte-executable", O_WRONLY | O_CREAT, mode);
    if (fd == -1) {
        printf("failed to open file\n");
    }
    Elf64_Ehdr header;

    header.e_ident[EI_MAG0] = ELFMAG0;
    header.e_ident[EI_MAG1] = ELFMAG1;
    header.e_ident[EI_MAG2] = ELFMAG2;
    header.e_ident[EI_MAG3] = ELFMAG3;
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;

    header.e_type = ET_EXEC;
    header.e_machine = EM_X86_64;
    header.e_version = EV_CURRENT;
    header.e_entry = 0x400000; /* Entry point virtual address */

    header.e_phoff = 64; /* Program header table file offset */
    header.e_shoff = 136; /* Section header table file offset */

    header.e_flags = 0; /* Processor-specific flags */
    header.e_ehsize = 64; /* ELF header size in bytes */ 
    header.e_phentsize = 56; /* Program header table entry size */
    header.e_phnum = 1; /* Program header table entry count */
    header.e_shentsize = 64; /* Section header table entry size */
    header.e_shnum = 0; /* Section header table entry count */
    header.e_shstrndx = 0; /* Section header string table index */

    Elf64_Phdr program_header;
    program_header.p_type = PT_LOAD; /* Segment type */
    program_header.p_flags = PF_R | PF_X; /* Segment flags */
    program_header.p_offset = 120; /* Segment file offset */
    program_header.p_vaddr = 0x400000; /* Segment virtual address */
    program_header.p_paddr = 0x400000; /* Segment physical address */
    program_header.p_filesz = 16; /* Segment size in file */
    program_header.p_memsz = 16; /* Segment size in memory */
    program_header.p_align = 4096; /* Segment alignment */

    write(fd, &header, sizeof(header));
    write(fd, &program_header, sizeof(program_header));
    write(fd, instructions, sizeof(instructions));
    
    close(fd);
    return 0;
}

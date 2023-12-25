#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "elf.h"
#include "proc.h"
#include "debug.h"

//kernel debug information 
static struct debuginfo  debug_info;
///static int load=0;


void debuginit(){
    initlock(&debug_info.lock,"debug");
}

//all the disk operation need use  
void  debug_info_load(){
    struct inode *ip;
    begin_op();
    if((ip = namei("/ls")) == 0){
        goto bad;
    }
    ilock(ip);
    
    struct elfhdr elf_header;
    struct shdr   shstr_shdr;
    struct shdr   sym_shdr;
    struct shdr   str_shdr;
    struct shdr   section_header;
    
    if(readi(ip, 0, (uint64)&elf_header, 0, sizeof(elf_header)) != sizeof(elf_header))
        goto bad;

    if(elf_header.magic != ELF_MAGIC)
        goto bad;

    
    uint64 off=elf_header.shoff+elf_header.shstrndx*sizeof(struct shdr);
    if(readi(ip, 0, (uint64)& shstr_shdr, off, sizeof(struct shdr)) != sizeof(struct shdr)){
        goto bad;
    }

    //read the symtab_shdr and strtab_shdr
    int i=0; 
    int  sym_find=0;
    int str_find=0;
    for(;i < elf_header.shnum; i++){

        if(sym_find==1&&str_find==1){
              break;
        }
        off=elf_header.shoff+i*sizeof(struct shdr);
        if(readi(ip,0,(uint64)& section_header,off,sizeof(struct shdr))!=sizeof(struct shdr)){
            goto bad;
        }
        if(section_header.sh_type==SHT_SYMTAB){
             sym_shdr=section_header;
             sym_find=1;
        }
        if(section_header.sh_type==SHT_STRTAB&&i!=elf_header.shstrndx){
            str_shdr=section_header;
            str_find=1;
        }
    }

    if(!sym_find|!str_find){
        goto bad;
    }


    //load the strtab to memory
    //fill in infromation

    char *strtab;
    if( (strtab=loadsection(ip,&str_shdr)) ==0){
      goto bad;
    }

    char *symtab;
    if((symtab=loadsection(ip,&sym_shdr))==0){
      goto bad;
    }

    debug_info.strtabaddr=(uint64)strtab;


    struct sym *p=(struct sym *)symtab;
    uint64 nsym=sym_shdr.sh_size/sizeof(struct sym);
    uint64 npage=(PGROUNDUP(nsym*sizeof(struct funcinfo))/PGSIZE);
    if((debug_info.func=kallocnpages(npage))==0){
      goto bad; 
    }

    uint64 j=0;
    for(uint64 i=0;i<nsym;i++){
      // symbol is a function
       if(ST_TYPE(p->st_info)==0x2){
           debug_info.func[j].funcname=debug_info.strtabaddr+p->st_name;
           debug_info.func[j].kfuncaddr=p->st_value;
           debug_info.func[j].kfuncend=p->st_value+p->st_size;
           j++;
       }
       p++;
    }

    debug_info.funcsz=j;

    unloadsection(symtab,&sym_shdr);
    iunlockput(ip);
    end_op();
    return ;

    bad:
    
    if(ip){
        iunlockput(ip);
        end_op();
    }
    panic("Kernel load debug information fail\n");
}

char* loadsection( struct inode *ip,  struct shdr *shdr){

   char *section_in_memory=(char *)0;
   uint64 npages=   PGROUNDUP ((*shdr).sh_size)/PGSIZE;
   char *addr;
   if((addr=kallocnpages(npages))==(char *)0){
     return (char *)0;
   }
    
    uint64 n;
   uint64 off=(*shdr).sh_offset;
    uint64 dn;

    for(n=0;n<(*shdr).sh_size;n+=dn,off+=dn){
      uint64 left=(*shdr).sh_size-n;
      dn=left>PGSIZE ?PGSIZE:left;
      if(n==0){
       section_in_memory=addr;
      }
     if(readi(ip,0,(uint64)addr,off,dn)!=dn){
       return (char *)0;
     }
    }
    return section_in_memory;
}

//the section must be load in memory from addr
void unloadsection(char *addr,  struct shdr *shdr){
    uint64 npages=   PGROUNDUP ((*shdr).sh_size)/PGSIZE;
    for(uint64 i=0;i<npages;i++){
      kfree(addr+i*PGSIZE);
    }
}

// kaadr->funcname
char * funcname(uint64 kaddr){
    struct funcinfo *fc=debug_info.func;
    int i=0;
    for(;i<debug_info.funcsz;i++){
        if(kaddr>=fc[i].kfuncaddr&&kaddr<fc[i].kfuncend){
            break;
        }
    }

    return     (char *)  ((i==debug_info.funcsz)  ?   0: fc[i].funcname);
}


void 
backtrace(void){
    // the one who can get load will load the kernel information
   // printf("Start back trace\n");
   printf("........................start\n");
    int load=0;
    if(!debug_info.load){
        acquire(&debug_info.lock);
        if(!debug_info.load){
            load=1;
            debug_info.load=1;
        }
      release(&debug_info.lock);
    }


    if(load){
        debug_info_load();
        debug_info.is_funcinfo_load=1;
    }

 /* uint64 fp=r_fp();
  while(PGROUNDDOWN(fp)==PGROUNDDOWN(myproc()->kstack)){
    uint64 ra=*((uint64*)(fp-8));
    //  we just serch table to find where is the function and the function is on which line
    char *s=funcname(ra);
    if(ra){
        printf("%s\n",s);
    }else{
        // some wrong
        return ;
    }
    //printf("%p\n",ra);
    fp=*((uint64*)(fp-16));
  }*/
  // printf("kernel test function_name %s\n",funcname(0x1c));
  // may doing while here\n\n
 
    while(!debug_info.is_funcinfo_load){};
 
    for(int i=0;i<debug_info.funcsz;i++){
        printf("func name:%s\n",funcname(debug_info.func[i].kfuncaddr));
    }
  // printf("Start back trace  end\n");
   printf("........................end\n");

}



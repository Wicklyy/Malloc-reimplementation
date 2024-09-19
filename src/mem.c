//------------------------------------------------------------------------------
// Projet : TP CSE (malloc)
// Cours  : Conception des systèmes d'exploitation et programmation concurrente
// Cursus : Université Grenoble Alpes - UFRIM²AG - Master 1 - Informatique
//------------------------------------------------------------------------------

#include "mem.h"
#include "mem_space.h"
#include "mem_os.h"
#include <assert.h>



mem_fit_function_t* Mff = NULL;



//-------------------------------------------------------------
// Structure liste chainée
//-------------------------------------------------------------

struct mem_free_block_s{
	char free;
	size_t size;
	struct fb *next;
};

typedef struct mem_used_block_s{
	char free;
	size_t size;
}mem_used_block_t;

//-------------------------------------------------------------
// mem_init
//-------------------------------------------------------------
/**
 * Initialize the memory allocator.
 * If already init it will re-init.
**/
void mem_init() {
	mem_free_block_t *maillon;
	maillon = mem_space_get_addr();
	maillon->free = (char)0xF;
	maillon->size = (size_t) (mem_space_get_size()-sizeof(mem_free_block_t));
	maillon->next = NULL;
}

//-------------------------------------------------------------
// mem_alloc
//-------------------------------------------------------------
/**
 * Allocate a bloc of the given size.
**/
void *mem_alloc(size_t size) {

	mem_free_block_t *block = Mff(mem_space_get_addr(),size);
	if(block != NULL){		/* Chargement en debut de structure*/
		void *adress = block + sizeof(mem_used_block_t);
		if(block == mem_space_get_addr()) adress += sizeof(mem_free_block_t);
		*(mem_used_block_t*)(adress - sizeof(mem_used_block_t)) = (mem_used_block_t) {free:(char)0x0, size:size};
		mem_free_block_t *new = (adress+size);
		new->next = block->next;
		new->size = block->size - size - sizeof(mem_used_block_t);
		new->free = (char)0xF;
		
			/* Nouveau chainage */
		mem_free_block_t *curent = mem_space_get_addr(), *past = curent;
		while(curent != block){
			curent = curent->next;
			past = past->next;
		}
		past->next = new;

	
		/*					   Chargement en fin de block 
		void* adress = block + sizeof(mem_free_block_t) + block->size - size; 	
		//mem_used_block_t init = {size};
		*(mem_used_block_t*)(adress - sizeof(mem_used_block_t)) = (mem_used_block_t) {size};
		block->size = block->size - size - sizeof(mem_used_block_t);*/
		
		return adress;
	}
	else return NULL;

}

//-------------------------------------------------------------
// mem_get_size
//-------------------------------------------------------------
size_t mem_get_size(void * zone)
{
	mem_used_block_t part = *((mem_used_block_t *)zone-sizeof(mem_used_block_t)); 
	if(part.free == (char)0xF) return part.size;
	return 0;
}

//-------------------------------------------------------------
// mem_free
//-------------------------------------------------------------
/**
 * Free an allocaetd bloc.
**/
void mem_free(void *zone) {


    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
}

//-------------------------------------------------------------
// Itérateur(parcours) sur le contenu de l'allocateur
// mem_show
//-------------------------------------------------------------
void mem_show(void (*print)(void *, size_t, int free)) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
}

//-------------------------------------------------------------
// mem_fit
//-------------------------------------------------------------
void mem_set_fit_handler(mem_fit_function_t *mff) {
	Mff = mff;

}

//-------------------------------------------------------------
// Stratégies d'allocation
//-------------------------------------------------------------
mem_free_block_t *mem_first_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}
//-------------------------------------------------------------
mem_free_block_t *mem_best_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}

//-------------------------------------------------------------
mem_free_block_t *mem_worst_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}

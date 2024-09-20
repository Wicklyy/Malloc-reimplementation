//------------------------------------------------------------------------------
// Projet : TP CSE (malloc)
// Cours  : Conception des systèmes d'exploitation et programmation concurrente
// Cursus : Université Grenoble Alpes - UFRIM²AG - Master 1 - Informatique
//------------------------------------------------------------------------------

#include "mem.h"
#include "mem_space.h"
#include "mem_os.h"
#include <assert.h>
#include <stdbool.h>


mem_fit_function_t* Mff = &mem_first_fit;
mem_free_block_t  *tete= NULL;


//-------------------------------------------------------------
// Structure liste chainée
//-------------------------------------------------------------

struct mem_free_block_s{
	size_t size;
	struct mem_free_block_s *next;
};



//-------------------------------------------------------------
// mem_init
//-------------------------------------------------------------
/**
 * Initialize the memory allocator.
 * If already init it will re-init.
**/
void mem_init() {
	tete = mem_space_get_addr();
	tete->size = (size_t) (mem_space_get_size()-sizeof(mem_free_block_t));
	tete->next = NULL;
}

//-------------------------------------------------------------
// mem_alloc
//-------------------------------------------------------------
/**
 * Allocate a bloc of the given size.
**/
void *mem_alloc(size_t size) {
	if (Mff == NULL) return NULL;
	size_t taille;
	if ((taille = (sizeof(mem_free_block_t) + size)%sizeof(int))!=0){
		size = size + sizeof(int) - taille; //On evite les erreurs d'alignement du cache
	}
	mem_free_block_t *block = Mff(tete,size);
	if(block != NULL){		/* Chargement en debut de structure*/
		void *adress = (char*)block + sizeof(mem_free_block_t);
		mem_free_block_t *new = NULL;
		if(size != block->size){
			new = (adress+size);
			new->next = block->next;
			new->size = block->size - size - sizeof(mem_free_block_t);
		}
		else{

		}
		block->size=size;
		//block->next = NULL; /* block aloué ne doit pas pointer dans la liste */	
		
			/* Nouveau chainage */
		if(tete == block) tete = new;
		else{
			mem_free_block_t *curent = tete, *past = curent;
			while(curent != block){
				past = curent;
				curent = curent->next;
			}
			if(new == NULL) past->next = block->next;
			else past->next = new;
		}
		
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
	//TODO
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
	void* adress = mem_space_get_addr();
	size_t size = mem_space_get_size(), ssize;
	bool free;
	mem_free_block_t* curent = tete;
	mem_free_block_t *maillon;
	while(size > 0){
		//printf("%ld\n",size);
		maillon = adress;
		ssize = maillon->size;
		
		if(curent == maillon){
			free = true;
			curent = curent->next;
		}else free = false;
		
		print((adress + sizeof(mem_free_block_t) - (size_t)mem_space_get_addr()),ssize,free);
		size = size - (ssize+ sizeof(mem_free_block_t));
		adress+=ssize+sizeof(mem_free_block_t);
	}
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
	if(first_free_block == NULL) return NULL;
	mem_free_block_t *renvoie = first_free_block;
	while((renvoie->size)<wanted_size){
		if(renvoie->next == NULL) return NULL; //On est a la fin de notre liste chainée et rien n'est assez grand
		renvoie = renvoie->next;
	}
	return renvoie;
}
//-------------------------------------------------------------
mem_free_block_t *mem_best_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    if(first_free_block == NULL) return NULL;
	mem_free_block_t *renvoie = NULL, *curent = first_free_block;
	while(curent->next != NULL){
		if(curent->size == wanted_size) return curent;
		if(((renvoie == NULL) || (curent->size < renvoie->size)) && curent->size < wanted_size){
			renvoie = curent;
		}	
	}

	return renvoie;
}

//-------------------------------------------------------------
mem_free_block_t *mem_worst_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    if(first_free_block == NULL) return NULL;
	mem_free_block_t *renvoie = NULL, *curent = first_free_block;
	while(curent->next != NULL){
		if(((renvoie == NULL) || (curent->size > renvoie->size)) && curent->size > wanted_size){
			renvoie = curent;
		}	
	}

	return renvoie;
}

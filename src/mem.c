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

//-------------------------------------------------------------
// Variable Globale
//-------------------------------------------------------------
mem_fit_function_t* Mff = &mem_first_fit;
mem_free_block_t  *head= NULL;

#define ALLIGNEMENT sizeof(int)


//-------------------------------------------------------------
// Structure liste chainée
//-------------------------------------------------------------

struct mem_free_block_s{
	size_t size;
	struct mem_free_block_s *next;
};

typedef struct mem_used_block_s{
	size_t size;
}mem_used_block_t;

//-------------------------------------------------------------
// Fonction auxilere
//-------------------------------------------------------------

/* renvoie vrai si le pointeur fait partie des zone allouer ou libre */
bool locate(mem_used_block_t *zone){		
	mem_used_block_t *curent = (mem_used_block_t *) mem_space_get_addr();
	size_t taille = 0;
	while(curent < zone){
		taille = curent->size + sizeof(mem_used_block_t);
		curent = (mem_used_block_t *)((void *)curent + taille);
	}
	if((void*) zone == (void *)curent) return true;
	
	return false;
}

//-------------------------------------------------------------
// mem_init
//-------------------------------------------------------------
/**
 * Initialize the memory allocator.
 * If already init it will re-init.
**/
void mem_init() {
	head = mem_space_get_addr();
	head->size = (size_t) (mem_space_get_size()-sizeof(mem_used_block_t));
	head->next = NULL;
}

//-------------------------------------------------------------
// mem_alloc
//-------------------------------------------------------------
/**
 * Allocate a bloc of the given size.
**/
void *mem_alloc(size_t size) {
	
	if (Mff == NULL) return NULL;		/* Si on veut que mem_alloc retourn NULL en cas de size=0 rajouter "|| size==0"*/
	size_t taille;
	if(size < sizeof(mem_free_block_t) - sizeof(mem_used_block_t)) size = sizeof(mem_used_block_t); 
	if ((taille = (sizeof(mem_used_block_t) + size)%ALLIGNEMENT)!=0){
		size = size + ALLIGNEMENT - taille; //On evite les erreurs d'alignement du cache
	}
	mem_free_block_t *block = Mff(head,size);
	if(block != NULL){		/* Chargement en debut de structure*/
		void *adress = (char*)block + sizeof(mem_used_block_t);			/* valeur a return*/

		if(block->size-size < sizeof(mem_free_block_t)) size = block->size;			/* si il n'y a pas assez de place pour stocker la struct d'un maillon on augmente la taille alouer*/

		mem_free_block_t *new = NULL;			/* calcule des donnee du nouveau maillon */
		if(size != block->size){
			new = (adress+size);
			new->next = block->next;
			new->size = block->size - size - sizeof(mem_used_block_t);
		}
		else new = block->next;
		
		block->size=size;
		
			/* Nouveau chainage */
		if(head == block) {
			head = new;
			}
		else{
			mem_free_block_t *curent = head, *past = curent;
			while(curent != block){
				past = curent;
				curent = curent->next;
			}
			past->next = new;
		}
		return adress;
	}
	else return NULL;

}

//-------------------------------------------------------------
// mem_get_size
//-------------------------------------------------------------
size_t mem_get_size(void * zone)
{
	if (zone == NULL) return 0;
	mem_used_block_t *current = (mem_used_block_t *)(char *)(zone - (sizeof(mem_used_block_t))); //Current est le pointeur associé à la structure de la zone
	return current->size;
}

//-------------------------------------------------------------
// mem_free
//-------------------------------------------------------------
/**
 * Free an allocaetd bloc.
**/
void mem_free(void *zone) {
	if(mem_space_get_addr() > zone || zone > mem_space_get_addr() + mem_space_get_size()) return;
	
	mem_free_block_t *info = (mem_free_block_t *)((char *)zone - sizeof(mem_used_block_t));
	
	if(!locate((mem_used_block_t *)info)) return;		/* verification si la zone est allouer */
	
	if(head == NULL){
		head = info;
		info->next = NULL;
		return;
	}

	mem_free_block_t *next = head, *past = next;
	void *addr = NULL;
	while(next < info && next != NULL ){		/* recuperer les deux maillon, precedent et suivant*/
		past = next;
		next = next->next;
	}
	if(info == past || info == next) return;

	addr = (char *)info + info->size + sizeof(mem_used_block_t);
	if(addr == next){
		info->size +=next->size + sizeof(mem_used_block_t);
		info->next = next->next;
	} else info->next = next;
	
	if(next != past) past->next = info;
	else head = info;						/* seul cas ou next == past, si on a pas avancer*/
	
	addr = (char *)past + past->size + sizeof(mem_used_block_t);
	if(addr == info) {
		past->size += info->size + sizeof(mem_used_block_t);
		past->next = info->next;
	}
}



//-------------------------------------------------------------
// Itérateur(parcours) sur le contenu de l'allocateur
// mem_show
//-------------------------------------------------------------
void mem_show(void (*print)(void *, size_t, int free)) {
	void* adress = mem_space_get_addr();
	void* toPrint = NULL;
	size_t size = mem_space_get_size(), ssize;
	bool free;
	mem_free_block_t* curent = head;
	mem_free_block_t *maillon;
	while(size > 0){
		maillon = adress;
		ssize = maillon->size;

		if(maillon == curent){
			free = true;
			curent = curent->next;
		}else {
			free = false;
		}
		
		toPrint = (void *)(adress + sizeof(mem_used_block_t) - (size_t)mem_space_get_addr());
		print(toPrint,ssize,free);
		size -= ssize + sizeof(mem_used_block_t);
		adress += ssize + sizeof(mem_used_block_t);
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
	while((renvoie->size) < wanted_size){
		if(renvoie->next == NULL) return NULL; //On est a la fin de notre liste chainée et rien n'est assez grand
		renvoie = renvoie->next;
	}
	return renvoie;
}
//-------------------------------------------------------------
mem_free_block_t *mem_best_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    if(first_free_block == NULL) return NULL;
	mem_free_block_t *renvoie = NULL, *curent = first_free_block;
	while(curent != NULL){
		if(curent->size == wanted_size) return curent;
		if(((renvoie == NULL) || (curent->size < renvoie->size)) && curent->size > wanted_size){
			renvoie = curent;
		}
		curent = curent->next;
	}

	return renvoie;
}

//-------------------------------------------------------------
mem_free_block_t *mem_worst_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    if(first_free_block == NULL) return NULL;
	mem_free_block_t *renvoie = NULL, *curent = first_free_block;
	while(curent != NULL){
		if(((renvoie == NULL) || (curent->size > renvoie->size)) && curent->size > wanted_size){
			renvoie = curent;
		}
		curent = curent->next;
	}

	return renvoie;
}

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
#include <string.h>

//-------------------------------------------------------------
// Variable Globale
//-------------------------------------------------------------
mem_fit_function_t* Mff = &mem_first_fit;
mem_free_block_t  *head= NULL;

/*Variable permettant d'aligner la mémoire*/
#define ALIGNEMENT sizeof(int) 


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

/* renvoie vrai si le pointeur zone est bien dirigé vers le debut d'une zone et faux s'il est au milieu d'une zone mémoire */
bool locate(mem_used_block_t *zone){		
	mem_used_block_t *curent = (mem_used_block_t *) mem_space_get_addr(); //curent est l'adresse de début de la mémoire
	mem_free_block_t *free_block = head;
	size_t taille = 0;
	while(curent < zone){ //On se déplace dans la mémoire jusqu'à arriver à la zone à vérifier
		if((void *)free_block == (void *)curent) free_block = free_block->next;
		taille = curent->size + sizeof(mem_used_block_t);
		curent = (mem_used_block_t *)((void *)curent + taille);
	}
	
	if( ((void *) zone == (void *)curent) && ((void *)curent != (void *)free_block) ) return true; //Si on est bien sur une block de memoire non allouer on renvoie true
	
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
	
	if (Mff == NULL) return NULL;		/* Si on veut que mem_alloc retourne NULL en cas de malloc(0), rajouter "|| size==0" (c'est un choix)*/
	size_t taille;
	if(size < sizeof(mem_free_block_t) - sizeof(mem_used_block_t)) size = sizeof(mem_free_block_t) - sizeof(mem_used_block_t); //vérifie qu'on ait de quoi stocker une mem_free_block_t lors de la libération de la zone
	if ((taille = (sizeof(mem_used_block_t) + size)%ALIGNEMENT)!=0){
		size = size + ALIGNEMENT - taille; //On evite les erreurs d'alignement du cache
	}
	mem_free_block_t *block = Mff(head,size);//Adresse vers la structure rattachée au bloc mémoire à allouer
	if(block != NULL){		/* Chargement en debut de structure*/
		void *adress = (char*)block + sizeof(mem_used_block_t);	/*adresse a return*/

		if(block->size-size < sizeof(mem_free_block_t)) size = block->size;//On augmente la taille de size à celle du bloc mémoire si 
										   //on ne peut pas stocker de nouvelle structure dans le bloc mémoire résiduel
		mem_free_block_t *new = NULL;			/* calcule des donnee du nouveau maillon */
		if(size != block->size){
			new = (adress+size);//On place une nouvelle structure dans le bloc mémoire résiduel
			new->next = block->next; //On remplace notre bloc dans la liste chainée par le nouveau
			new->size = block->size - size - sizeof(mem_used_block_t);//Taille du bloc residuel
		}
		else new = block->next;
		
		block->size=size;
		
			/* Nouveau chainage */
		if(head == block) {
			head = new;
			}
		else{
			mem_free_block_t *curent = head, *past = curent;
			while(curent != block){ //On se déplace dans la liste chainée jusqu'à arriver à l'endroit du nouveau chainage
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
	mem_used_block_t *current = (mem_used_block_t *)((char *)(zone - (sizeof(mem_used_block_t)))); //Current est le pointeur associé à la structure de la zone
	return current->size;
}

//-------------------------------------------------------------
// mem_realloc
//-------------------------------------------------------------

void* mem_realoc(void* ptr, size_t size){
	void *adress = NULL;
	mem_used_block_t *info = (mem_used_block_t *)((char*)ptr - sizeof(mem_used_block_t));

	if(!locate(info)) return adress;
	if( mem_get_size(ptr) > size ) return ptr;

	if((adress = mem_alloc(size)) != NULL){
		memcpy(adress, ptr, mem_get_size(ptr));
		mem_free(ptr);
		return adress;
	}

	/* check si l'allocation n'a pas fonctionné parceque memoire est trop fragmenter*/	
	mem_free_block_t *curent = head, *past = head;
	while((void *)curent < (void *)info && curent!=NULL){
		past = curent;
		curent = curent->next;
	}
	bool full = false;
	if((char*)info + sizeof(mem_used_block_t) + info->size == (char*) curent /* il existe un bloque situer directement apres le bloque*/
		&& (curent->size + info->size + sizeof(mem_used_block_t)) >= size){ /* on a assez d'espace */
		if((curent->size + info->size + sizeof(mem_used_block_t) - size) < sizeof(mem_free_block_t)) {
			size = info->size + curent->size + sizeof(mem_used_block_t);
			full = true;
		}
		mem_free_block_t *new;
		if(full) new = curent->next;
		else{
			new = (mem_free_block_t *)((char*) curent + size - info->size);
			new->size = curent->size - (size - info->size);
		}
		if(head != curent) past->next = new;
		else{
			if(new != NULL) new->next = head->next;
			head = new;
		}
		info->size = size;
		return ptr;
	}
	return NULL;


}

//-------------------------------------------------------------
// mem_free
//-------------------------------------------------------------
/**
 * Free an allocaetd bloc.
**/
void mem_free(void *zone) {
	if(mem_space_get_addr() > zone || zone > mem_space_get_addr() + mem_space_get_size()) return; //On vérifie que zone est bien dans la zone de la mémoire
	
	mem_free_block_t *info = (mem_free_block_t *)((char *)zone - sizeof(mem_used_block_t)); //Info est le pointeur vers la structure mémoire de zone
	
	if(!locate((mem_used_block_t *)info)) return;	/*verification si zone est bien un pointeur vers un bloc mémoire */
	
	if(head == NULL){ //La mémoire entière est allouée, donc on créer une nouvelle tête
		head = info;
		info->next = NULL;
		return;
	}

	mem_free_block_t *next = head, *past = next; //next et past sont des zones LIBRES, précédant et suivant la zone à libérer
	void *addr = NULL;
	while(next < info && next != NULL ){ //On se déplace dans nos zones libres pour trouver les zones "devant" et "derrière" la zone à libérer
		past = next;
		next = next->next;
	}
	if(info == past || info == next) return; //La zone est déjà libre

	addr = (char *)info + info->size + sizeof(mem_used_block_t); //addr est le pointeur vers la structure suivant info
	if(addr == next){ //On vérifie si le bloc mémoire suivant zone est libre
		info->size +=next->size + sizeof(mem_used_block_t);
		info->next = next->next; //Si oui on fusionne les deux zones
	} else info->next = next; //Si non on chaine notre zone avec la zone suivante
	
	if(next != past) past->next = info; //On lie past next avec info et on corrige après si info et past se fusionnent
	else head = info;/*Si next == past, cela veut dire qu'info est le premier element de la mémoire a être libre*/
	
	addr = (char *)past + past->size + sizeof(mem_used_block_t); //addr est le pointeur vers la structure suivant past
	if(addr == info) { //On regarde si past et info sont contigues
		past->size += info->size + sizeof(mem_used_block_t);
		past->next = info->next;//Si elles le sont on les fusionne
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
	Mff = mff; //On set une fonction de recherche de zone à allouer
}

//-------------------------------------------------------------
// Stratégies d'allocation
//-------------------------------------------------------------
//Stratégie d'allocation où on parcours notre liste chainée et on renvoie le pointeur vers la première zone mémoire assez grande pour contenir wanted_size
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
//Stratégie où on parcourt notre liste chainée et on cherche la zone avec la zone mémoire residuelle la plus petite
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
//Stratégie où on parcourt notre liste chainée et on cherche la zone avec la zone mémoire residuelle la plus grande
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

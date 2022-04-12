// Copyright (C) 2007-2018, RTE (https://www.rte-france.com)
// See AUTHORS.txt
// SPDX-License-Identifier: Apache-2.0

# ifndef SPX_MACROS_POUR_FONCTION_EXTERNES_DE_GESTION_MEMOIRE
# include "mem_fonctions.h"
/*****************************************************************
 
 
  Macros pour redefinir les primitives de gestion memoire lorsqu'on
  ne veut pas utiliser celles de lib de l'OS

	
*****************************************************************/
	
# define malloc(Taille)           MEM_Malloc(Spx->Tas,Taille)
# define free(Pointeur)           MEM_Free(Pointeur)
# define realloc(Pointeur,Taille) MEM_Realloc(Spx->Tas,Pointeur,Taille)

/*****************************************************************/
# define SPX_MACROS_POUR_FONCTION_EXTERNES_DE_GESTION_MEMOIRE
# endif

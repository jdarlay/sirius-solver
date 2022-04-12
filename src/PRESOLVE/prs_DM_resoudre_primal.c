// Copyright (C) 2007-2018, RTE (https://www.rte-france.com)
// See AUTHORS.txt
// SPDX-License-Identifier: Apache-2.0

/***********************************************************************

   FONCTION: Resolution d'un sous systeme issu de la decomposition
	           Dumalge-Mendelson
                
   AUTEUR: R. GONZALEZ

************************************************************************/

# include "prs_sys.h"

# include "prs_fonctions.h"
# include "prs_define.h"

# include "pne_define.h"
		
# ifdef PRS_UTILISER_LES_OUTILS_DE_GESTION_MEMOIRE_PROPRIETAIRE	
  # include "prs_memoire.h"
# endif

# include "lu_define.h"
# include "lu_fonctions.h"

# if DUMALGE_MENDELSON == OUI_PNE

# include "btf.h"
# include "btf_internal.h"

# include "cs.h"   

/*----------------------------------------------------------------------------*/

void PRS_DumalgeResoudrePrimal( PRESOLVE * Presolve,
                                void * MatriceFact,
																void * MatriceAFactoriser,                                  
                                int NbVarDuProblemeReduit,
															 	int NbCntDuProblemeReduit,																					
 	                              int IndexCCdeb,
															 	int IndexCCfin,					   
															 	int IndexRRdeb,
															 	int IndexRRfin,																																											
                                void * Csd_in,
																void * A_in,
																int * VarNewVarOld,
															 	int * CntNewCntOld,  
																double * B,
															  char * CodeRet,
																char FactorisationOK 
		                          )
{
int Var; int YaErreur; int NbVarFix;
int ic; int icMax; double X; csi ccDeb; csi ccFin; csi rrDeb; csi rrFin;
int i; int j; int l; csd * Csd; cs * A;  

PROBLEME_PNE * Pne;

MATRICE_A_FACTORISER * Matrice; MATRICE * MatriceFactorisee;

int Cnt;

double  * SecondMembreEtSolution; int CodeRetour; int NombreMaxIterationsDeRaffinement; double ValeurMaxDesResidus;   
double * Verif;


*CodeRet = FactorisationOK;

Matrice = (MATRICE_A_FACTORISER *) MatriceAFactoriser;
MatriceFactorisee = (MATRICE *) MatriceFact;

Csd = (csd *) Csd_in;
A = (cs *) A_in;

Pne = (PROBLEME_PNE *) Presolve->ProblemePneDuPresolve;

ccDeb = (csi) IndexCCdeb;
ccFin = (csi) IndexCCfin;
rrDeb = (csi) IndexRRdeb;
rrFin = (csi) IndexRRfin;

SecondMembreEtSolution = (double *) malloc( NbCntDuProblemeReduit * sizeof( double ) );
Verif                  = (double *) malloc( NbCntDuProblemeReduit * sizeof( double ) );

for ( j = 0 , i = rrDeb ; i < rrFin ; i++ ) {
	/* Csd->p[i]: dans la numerotation du probleme soumis a dmperm */
	l = Csd->p[i];
	Cnt = CntNewCntOld[l];
	if ( Cnt < 0 || Cnt >= Pne->NombreDeContraintesTrav ) {
	  printf("Bug Resolution DM Cnt = %d\n",Cnt);
		exit(0);  
	}
	SecondMembreEtSolution[j] = B[Cnt];

	/*if ( B[Cnt] != 0.0 ) printf("B[%d] = %e\n",Cnt,B[Cnt]);*/
	
	j++;
}

for ( l = 0 ; l < NbCntDuProblemeReduit ; l++ ) {
  Verif[l] = SecondMembreEtSolution[l];
}

NombreMaxIterationsDeRaffinement = 2;
ValeurMaxDesResidus = 1.e-9;

MatriceFactorisee->SauvegardeDuResultatIntermediaire = 0;
MatriceFactorisee->SecondMembreCreux = 0;

LU_LuSolv( MatriceFactorisee,
           SecondMembreEtSolution, /* Le vecteur du second membre et la solution */
           &CodeRetour,             /* Le code retour ( 0 si tout s'est bien passe ) */
	         Matrice, /* Peut etre NULL, dans ce cas on ne fait pas de raffinement */
	         NombreMaxIterationsDeRaffinement,
		       ValeurMaxDesResidus        /* En norme L infini i.e. le plus grand */
         );
				 
printf("Fin de resolution CodeRetour %d\n",CodeRetour);

for ( j = 0 ; j < NbVarDuProblemeReduit ; j++ ) {
  ic = Matrice->IndexDebutDesColonnes[j];  
  icMax = ic + Matrice->NbTermesDesColonnes[j];
	while ( ic < icMax ) {							
	  i = Matrice->IndicesDeLigne[ic];
		/*
		printf("ValeurDesTermesDeLaMatrice colonne %d   ligne %d  NbTermesDesColonnes %d = %e\n",j,i,
		        Matrice->NbTermesDesColonnes[j],Matrice->ValeurDesTermesDeLaMatrice[ic]);
		*/				
	  Verif[i] -= Matrice->ValeurDesTermesDeLaMatrice[ic] * SecondMembreEtSolution[j];
    ic++;
	}
}

YaErreur = 0;
for ( i = 0 ; i < NbCntDuProblemeReduit ; i++ ) {
  if ( fabs(Verif[i]) > 1.e-9 ) {
	  YaErreur = 1;
	  printf("Erreur[%d] = %e\n",i,Verif[i]);
	}
}
if ( YaErreur == 0 ) printf("Pas d'erreur de resolution\n");

YaErreur = 0;
NbVarFix = 0;
for ( j = 0 , i = ccDeb ; i < ccFin ; i++ , j++) {
  X = SecondMembreEtSolution[j];
	l = Csd->q[i];
	Var =	VarNewVarOld[l];
	if ( Pne->TypeDeBorneTrav[Var] == VARIABLE_FIXE ) {
    printf("BUG variable %d est de type fixe\n",Var);
		exit(0);
	}
	if ( Var < Pne->NombreDeVariablesTrav ) {
		if ( X < Pne->UminTrav[Var] || X > Pne->UmaxTrav[Var] ) {
	    printf("Erreur Variable %d = %e Xmin %e Xmax %e\n",Var,X,Pne->UminTrav[Var],Pne->UmaxTrav[Var]);
			YaErreur = 1;
		}

		
		printf("Fixation de la variable %d a %e Umin %e Umax %e\n",Var,X,Pne->UminTrav[Var],Pne->UmaxTrav[Var]);
	 
		if ( Pne->TypeDeVariableTrav[Var] == ENTIER ) {
		  printf("Variable entiere fixee\n");
			/* Verifier que c'est sur une borne sinon pas de solution */
		}

		
    Pne->UTrav[Var] = X;
    Pne->UminTrav[Var] = X;   
    Pne->UmaxTrav[Var] = X;   
    Pne->TypeDeBorneTrav[Var] = VARIABLE_FIXE;    
    Pne->TypeDeVariableTrav[Var] = REEL;


		
		NbVarFix++;

		/* Passer la colonne l dans le second membre */
		
	  ic = A->p[l];
	  icMax = A->p[l+1];		
	  while ( ic < icMax ) {				
			B[CntNewCntOld[A->i[ic]]] -= A->x[ic] * X;		  
		  ic++;
	  }
		
		
	}
	else {
	  Cnt = Var - Pne->NombreDeVariablesTrav;
		if ( X < 0.0 ) {
	    printf("Erreur Variable d'ecart contrainte %d %e \n",Cnt,SecondMembreEtSolution[j]);
			YaErreur = 1;
		}
		Pne->SensContrainteTrav[Cnt] = '=';
	}		
}
if ( YaErreur == 1 ) exit(0);

LU_LibererMemoireLU( (MATRICE *) MatriceFactorisee );

free( Matrice->IndexDebutDesColonnes );
free( Matrice->NbTermesDesColonnes );
free( Matrice->ValeurDesTermesDeLaMatrice );
free( Matrice->IndicesDeLigne );
free( Matrice );

free( SecondMembreEtSolution );
free( Verif );

printf("Nombre de variables fixees par le primal %d\n",NbVarFix);

return;
}

# endif



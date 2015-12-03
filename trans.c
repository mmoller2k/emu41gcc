/* trans.c */
/* traduction des labels HP41 */


#include <alloc.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

/* declaration type */
struct S_idx_add {
  long addr;
  long index;
};

struct S_idx_nam {
  char name[4];
  long index;
} ;



/* variables locales */
static int nb_label[2];
static int fic[2];

struct S_idx_add far *idx_add[2];
struct S_idx_nam far *idx_nam[2];



void cree_idx(char *nomfic)
{
  FILE *src;
  FILE *dest;
  char ligne[82];
  char nom[32];
  long a;
  struct S_idx_add far *idx_add;
  struct S_idx_nam far *idx_nam;
  struct S_idx_add far *pt_idx_add;
  struct S_idx_nam far *pt_idx_nam;
  struct S_idx_add s_add;
  struct S_idx_nam s_nam;
  int i, n;
  long pos;
  char *pt;

  printf("Building index %s\n",nom);

  strcpy(nom,nomfic);
  strcat(nom,".ADD");
  src=fopen(nom,"rt");
  if (src==NULL) {
    printf("Error : file %s not found\n",nom);
    return;
  }

  strcpy(nom,nomfic);
  strcat(nom,".IDX");
  dest=fopen(nom,"wb");
  if (dest==NULL) {
    printf("Error : %s cannot be created\n",nom);
    return;
  }

  /* allocation des structures d'index max. */
  idx_add = (struct S_idx_add far *) farmalloc(8000L*sizeof(struct S_idx_add));
  if (idx_add == NULL) {
    printf("Error : not enough memory\n");
    return;
  }
  idx_nam = (struct S_idx_nam far *) farmalloc(8000L*sizeof(struct S_idx_nam));
  if (idx_nam == NULL) {
    printf("Error : not enough memory\n");
    return;
  }

  /* parcours le fichier source, et cr‚‚ les index */
  n=0;
  pt_idx_nam=idx_nam;
  pt_idx_add=idx_add;
  pos=0;
  while (fgets(ligne,80,src) != NULL) {
    if (ligne[0]=='=') {
      pt=strtok(ligne," \t");
      strncpy(s_nam.name,pt+1,4);
      pt=strtok(NULL," \t");
      pt=strtok(NULL," \t");
      sscanf(pt,"%lx",&a);
      s_add.addr=a;
      s_nam.index=pos;
      s_add.index=pos;
      *pt_idx_nam++ = s_nam;
      *pt_idx_add++ = s_add;
      n++;
      if (n>8000) break;
    }
    pos=ftell(src);
  }
  fclose(src);

  /* ‚crit le fichier d'index */
  strcpy(s_nam.name,"2.0");
  s_nam.index=n;
  fwrite(&s_nam,sizeof(s_nam),1,dest);
  pt_idx_nam=idx_nam;
  pt_idx_add=idx_add;
  for (i=0;i<n;i++) {
    s_add=*pt_idx_add++;
    fwrite(&s_add,sizeof(s_add),1,dest);
  }
  for (i=0;i<n;i++) {
    s_nam=*pt_idx_nam++;
    fwrite(&s_nam,sizeof(s_nam),1,dest);
  }
  fclose(dest);

  farfree(idx_nam);
  farfree(idx_add);
}


void init_lbl(char *nomfic,int n)
{
  char nom[82];
  struct S_idx_nam s_nam[30];
  struct S_idx_add s_add[30];
  struct S_idx_add far *pt_idx_add;
  struct S_idx_nam far *pt_idx_nam;
  int i, j, nbre;

  idx_add[n]=NULL;
  idx_nam[n]=NULL;
  nb_label[n]=0;
  strcpy(nom,nomfic);
  strcat(nom,".IDX");
  fic[n]=open(nom,O_RDONLY|O_BINARY);
  if (fic[n] == -1)
    cree_idx(nomfic);
  fic[n]=open(nom,O_RDONLY|O_BINARY);

  if (fic[n] != -1) {
    printf("Loading %s ... ",nom);
    read(fic[n],s_nam,sizeof(struct S_idx_nam));
    nbre=s_nam[0].index;
    nb_label[n]=nbre;
    if (nbre>8000) {
      printf("Error : too many entries\n");
      return;
    }
    /* allocation m‚moire */
    idx_add[n] = farmalloc(nbre*sizeof(struct S_idx_add));
    idx_nam[n] = farmalloc(nbre*sizeof(struct S_idx_nam));
    if ( (idx_add[n]==NULL) ||  (idx_nam[n]==NULL) ) {
      printf("Error : not enough memory\n");
      return;
    }
    else {
      pt_idx_add=idx_add[n];
      for (i=0;i<(nbre-30);i+=30) {
	read(fic[n],s_add,sizeof(struct S_idx_add)*30);
	for (j=0;j<30;j++)
	  *pt_idx_add++ = s_add[j];
      }
      for (;i<nbre;i++) {
	read(fic[n],s_add,sizeof(struct S_idx_add));
	  *pt_idx_add++ = s_add[0];
      }
      pt_idx_nam=idx_nam[n];
      for (i=0;i<(nbre-30);i+=30) {
	read(fic[n],s_nam,sizeof(struct S_idx_nam)*30);
	for (j=0;j<30;j++)
	  *pt_idx_nam++ = s_nam[j];
      }
      for (;i<nbre;i++) {
	read(fic[n],s_nam,sizeof(struct S_idx_nam));
	*pt_idx_nam++ = s_nam[0];
      }
      printf("%d entries\n",nbre);
    }
    close(fic[n]);
  }
  else
    printf("Error : file %s not found\n",nom);

  strcpy(nom,nomfic);
  strcat(nom,".ADD");
  fic[n]=open(nom,O_RDONLY|O_TEXT);
  if (fic[n]<0)
    printf("Error : file %s not found\n",nom);


}



/* pour mon28 : */
void init_label(char *nomfic)
{
  init_lbl(nomfic,0);
  fic[1]=-1;  /* pas de uentr28 */
}


void fin_label(void)
{
}



int ch_lbl(long adr,char *s,int n)
{
  struct S_idx_add far *ptr_idx_add;
  register int i;
  char ligne[82];
  char nom[32];
  char *pt;


  /* cherche l'adresse */

  s[0]=0;
  ptr_idx_add = idx_add[n];
  if ( (ptr_idx_add != NULL) && (fic[n]!=-1) ) {
    i=0;
    while ( i < nb_label[n]) {
      if (ptr_idx_add->addr == adr) {
	lseek(fic[n],ptr_idx_add->index,SEEK_SET);
	read(fic[n],ligne,80);
	pt=strtok(ligne," \t");
	nom[0]=0;
	if (pt!=NULL) strncpy(nom,pt,16);
	nom[16]=0;
	do {
	  strcat(nom," ");
	} while (strlen(nom)<13);
	strcat(s,nom);
      }
      i++;
      ptr_idx_add++;
    }
  }
  return(s[0]);
}

int ch_label(long adr,char *s)
{
  int n;
  char s1[80];

  s[0]=0;
  n=ch_lbl(adr,s,0);
  if (n==0) {
    n=ch_lbl(adr,s1,1);
    if (n!=0) {
      strcpy(s,"_");
      strcat(s,s1);
    }
  }
  return(s[0]);
}




long ch_symb0(char *s,int n)
{
  int i;
  long a;
  struct S_idx_nam far *ptr_idx_nam;
  struct S_idx_nam s_nam;
  char ligne[82];
  char nom[32];
  char *pt;


  a=-1;
  ptr_idx_nam = idx_nam[n];
  if ( (ptr_idx_nam != NULL) && (fic[n]!=-1) ) {
    i=0;
    while ( i < nb_label[n]) {
      s_nam=*ptr_idx_nam;
      if (strncmp(s_nam.name,s,4) == 0) {
	lseek(fic[n],s_nam.index,SEEK_SET);
	read(fic[n],ligne,80);
	pt=strtok(ligne," \t");
	if (pt!=NULL) {
	  strncpy(nom,pt,14);
	  nom[14]=0;
	  if (strcmp(s,nom+1) == 0) {
	    pt=strtok(NULL," \t");
	    if (pt!=NULL) pt=strtok(NULL," \t");
	    if (pt!=NULL) sscanf(pt,"%lx",&a);
	    break;
	  }
	}
      }
      i++;
      ptr_idx_nam++;
    }
  }
  return(a);
}


long ch_symb(char *s)
{
  long a;

  switch (s[0]) {
    case '=': a=ch_symb0(&s[1],0); break;
    case '_': a=ch_symb0(&s[1],1); break;
    default:  a=ch_symb0(s,0);
  }
  return(a);
}




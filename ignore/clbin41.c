/* clbin41.c */
/* conversion fichier ASCII .lst en fichier binaire .bin */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* int mots[4096]; */


int convert(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  char buf[256];
  int a1, a2, n, m;
  int k;
  int v[4];
  char octets[6];

  src=fopen(f1,"rt");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wb");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  a1=0;
  while (fgets(buf,80,src)!=NULL) {
    if ((buf[0]!=0)&&(buf[0]!='\n')&&(buf[0]!=' ')&&(buf[0]!='#')) {
      if (buf[4]==':') buf[4]=' ';
      m=sscanf(buf,"%x %x",&a2,&n);
      if (m==2) {
        if ((a1&0x0fff)==(a2&0x0fff)) {
          v[k]=n;
          k++;
          if (k==4) {
            /* 4 mots de 10 bits lus, ecrit 5 octets */
            octets[0]=  v[0]    &0xff;
            octets[1]=((v[1]<<2)&0xfc) + ((v[0]>>8)&0x03);
            octets[2]=((v[2]<<4)&0xf0) + ((v[1]>>6)&0x0f);
            octets[3]=((v[3]<<6)&0xc0) + ((v[2]>>4)&0x3f);
            octets[4]=                   ((v[3]>>2)&0xff);
            fwrite(octets,1,5,dest);
            k=0;
          }
          a1++;
        }
      }
    }
  }
  fclose(src);
  fclose(dest);
  return(a1);
}

int cree(char *f2)
{
  FILE *dest;
  int i;
  char octets[6];

  dest=fopen(f2,"wb");
  if (dest==NULL) {
    return(-1);
  }

  for (i=0;i<5;i++)
    octets[i]=0;

  for (i=0;i<1024;i++) {
    /* 4 mots de 10 bits : ecrit 5 octets */
    fwrite(octets,1,5,dest);
  }

  fclose(dest);
  return(4096);
}


#if 0

/* conversion des fichier *.dmp issu du disque LIF en *.lst */
int dmp2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  char buf[256];
  int n, p;
  int i, j, k;
  int v[6];
  unsigned char octets[10];
  char *ptr;

  src=fopen(f1,"rt");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  p=0;
  while (fgets(buf,80,src)!=NULL) {
    if (buf[0]=='0') {
      strtok(buf," ");
      for (j=0;j<2;j++) {
	/* lit un registre */
	for (i=0;i<8;i++) {
	  ptr=strtok(NULL," ");
	  sscanf(ptr,"%x",&n);
	  octets[i]=n;
	}
	/* octet    0  1  2  3  4  5  6  7  */
	/*          ee s1 00 m4 m3 m2 m1 m0  */

	/* registre  1 m0 m1 m2 m3 m4 s ee */
	/* mots        ---01-----2 3-----4 */

	if (p==0) {
	  /* traite préfixe */
	  p=octets[5]-1;
	  k=octets[4]<<8;
	}
	else {
	  /* reconstruit les 5 mots */
	  v[0]=((octets[7]&0x3f)<<4) + (octets[6]>>4);
	  v[1]=((octets[6]&0x0f)<<6) + (octets[5]>>2);
	  v[2]=((octets[5]&0x03)<<8) + (octets[4]);
	  v[3]=((octets[3]&0xff)<<2) + (octets[1]>>6);
	  v[4]=((octets[1]&0x30)<<4) + (octets[0]);
	  for (i=0;i<5;i++) {
	    fprintf(dest,"%04x  %03x\n",k,v[i]);
	    k++;
	  }
	  p--;
	}
      }
    }
  }
  fclose(src);
  fclose(dest);
  return(0);
}



/* conversion des fichier *.r41 en *.lst */
int r41_2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, k;
  int v[6];
  unsigned char octets[10];

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  fseek(src,32,0);
  while (fread(octets,1,5,src)==5) {
    /* octet    0          1  2  3  4   */
    /*          high bits  w3 w2 w1 w0  */
    /* high bits : xx xx xx xx */
    /*             w3 w2 w1 w0 */

    /* reconstruit les 4 mots */
    v[0]=(((octets[0]&0x03)   )<<8) + octets[4];
    v[1]=(((octets[0]&0x0c)>>2)<<8) + octets[3];
    v[2]=(((octets[0]&0x30)>>4)<<8) + octets[2];
    v[3]=(((octets[0]&0xc0)>>6)<<8) + octets[1];
    for (i=0;i<4;i++) {
      fprintf(dest,"%04x  %03x\n",k,v[i]);
      k++;
    }
  }
  fclose(src);
  fclose(dest);
  return(0);
}



/* conversion des fichier *.hex du dump de la rom hepax en *.lst */
int hepax2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, j, k;
  int n;
  unsigned char octets[10];
  char ligne[82];

  src=fopen(f1,"rt");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  for (i=0;i<4;i++) {
    /* pour les 4 pages de 4kmots */
    for (j=0;j<256;j++) {
      /* lit 256 lignes de 16 octets contenant les bits 4-0 */
      fgets(ligne,80,src);
      for (k=0;k<16;k++) {
        /* extrait les 16 octets */
        strncpy(octets,&ligne[9+k*2],2);
        octets[3]=0;
        sscanf(octets,"%x",&n);
        mots[j*16+k]=n;
      }
    }
    for (j=0;j<256;j++) {
      /* lit 256 lignes de 16 octets contenant les bits 9-5 */
      fgets(ligne,80,src);
      for (k=0;k<16;k++) {
        /* extrait les 16 octets */
        strncpy(octets,&ligne[9+k*2],2);
        octets[3]=0;
        sscanf(octets,"%x",&n);
        mots[j*16+k] += n<<5 ;
      }
    }
    /* ecrit les 256*16 = 4kmots */
    for (j=0;j<256;j++) {
      for (k=0;k<16;k++) {
        fprintf(dest,"%04X %03X\n",(i*256+j)*16+k,mots[j*16+k]);
      }
    }

  }

  fclose(src);
  fclose(dest);
  return(0);
}



/* conversion des fichier *.bin en *.lst */
int bin2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, k;
  int v[6];
  unsigned char octets[10];

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  while (fread(octets,1,5,src)==5) {
    /* octet    0          1  2  3  4   */
    /*          high bits  w3 w2 w1 w0  */
    /* high bits : xx xx xx xx */
    /*             w3 w2 w1 w0 */

    /* reconstruit les 4 mots */
    v[0]=((octets[0]&0xff)   ) + ((octets[1]&0x03)<<8);
    v[1]=((octets[1]&0xfc)>>2) + ((octets[2]&0x0f)<<6);
    v[2]=((octets[2]&0xf0)>>4) + ((octets[3]&0x3f)<<4);
    v[3]=((octets[3]&0xc0)>>6) + ((octets[4]&0xff)<<2);
    for (i=0;i<4;i++) {
      fprintf(dest,"%04x  %03x\n",k,v[i]);
      k++;
    }
  }
  fclose(src);
  fclose(dest);
  return(0);
}


/* conversion des fichier *.41r en *.lst */
int _41r2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, k;
  unsigned char octets[10];

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  while (fread(octets,1,8,src)==8) {
    for (i=0;i<4;i++) {
      fprintf(dest,"%04x  %03x\n",k,(octets[i*2]<<8)+octets[i*2+1]);
      k++;
    }
  }
  fclose(src);
  fclose(dest);
  return(0);
}

/* extraction de la ROM de la lib Emu41x  */
int emux2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, k;
  unsigned char octets[10];

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
  fseek(src,0x168,0);
  while (fread(octets,1,2,src)==2) {
      fprintf(dest,"%04x  %03x  ",k,((octets[1]&0x3f)<<4)+(octets[0]>>4));
      if ((octets[0]&15)!=7) fprintf(dest,"*");
      fprintf(dest,"\n");
      if (k==32767) break;
      k++;
  }
  fclose(src);
  fclose(dest);
  return(0);
}





/* conversion du fichier ccdx en *.lst */
/*
00 D1   C3 B9 A8 00
00         D1         C3         B9         A8            00
0000 0000  1101 0001  1100 0011  1011 1001  1010 1000     0000 0000
0000 0000 11  01 0001 1100   0011 1011 10  01 1010 1000     0000 0000
*/

int ccdx_2lst(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  int i, k;
  int v[6];
  unsigned char octets[10];

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wt");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  k=0;
//  fseek(src,32,0);
  while (fread(octets,1,5,src)==5) {
    /* octet    0          1  2  3  4   */
    /*          high bits  w3 w2 w1 w0  */
    /* high bits : xx xx xx xx */
    /*             w3 w2 w1 w0 */

    /* reconstruit les 4 mots */
    v[0]=(((octets[0]&0x03)   )<<8) + octets[4];
    v[1]=(((octets[0]&0x0c)>>2)<<8) + octets[3];
    v[2]=(((octets[0]&0x30)>>4)<<8) + octets[2];
    v[3]=(((octets[0]&0xc0)>>6)<<8) + octets[1];
    for (i=0;i<4;i++) {
      fprintf(dest,"%04x  %03x\n",k,v[i]);
      k++;
    }
  }
  fclose(src);
  fclose(dest);
  return(k);
}



#endif





/* conversion des fichier *.rom en *.bin */
int rom2bin(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  unsigned char b[10];
  unsigned int  v[10];
  unsigned char octets[10];
  int n;

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wb");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  n=0;
  while (fread(b,1,8,src)==8) {
    v[0] = (b[0] << 8) + b[1];
    v[1] = (b[2] << 8) + b[3];
    v[2] = (b[4] << 8) + b[5];
    v[3] = (b[6] << 8) + b[7];
    /* 4 mots de 10 bits lus, ecrit 5 octets */
    octets[0]=  v[0]    &0xff;
    octets[1]=((v[1]<<2)&0xfc) + ((v[0]>>8)&0x03);
    octets[2]=((v[2]<<4)&0xf0) + ((v[1]>>6)&0x0f);
    octets[3]=((v[3]<<6)&0xc0) + ((v[2]>>4)&0x3f);
    octets[4]=                   ((v[3]>>2)&0xff);
    fwrite(octets,1,5,dest);
    n+=4;
  }

  fclose(src);
  fclose(dest);

  return(n);

}

/* conversion des fichier *.bin en *.rom */
int bin2rom(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  unsigned char b[10];
  unsigned int  v[10];
  unsigned char octets[10];
  int n;

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);
  dest=fopen(f2,"wb");
  if (dest==NULL) {
    fclose(src);
    return(-1);
  }

  n=0;
  while (fread(b,1,5,src)==5) {
    v[0]=((b[0]&0xff)   ) + ((b[1]&0x03)<<8);
    v[1]=((b[1]&0xfc)>>2) + ((b[2]&0x0f)<<6);
    v[2]=((b[2]&0xf0)>>4) + ((b[3]&0x3f)<<4);
    v[3]=((b[3]&0xc0)>>6) + ((b[4]&0xff)<<2);
    /* 4 mots de 10 bits lus, ecrit 5 octets */
    octets[0]= v[0]>>8;
    octets[1]= v[0]&0xff;
    octets[2]= v[1]>>8;
    octets[3]= v[1]&0xff;
    octets[4]= v[2]>>8;
    octets[5]= v[2]&0xff;
    octets[6]= v[3]>>8;
    octets[7]= v[3]&0xff;
    fwrite(octets,1,8,dest);
    n+=4;
  }

  fclose(src);
  fclose(dest);

  return(n);

}


#if 0

int lod2dat(char *f1)
{
  FILE *src;
  FILE *dest;
  char ligne[80];
  int f;

  src=fopen(f1,"rt");
  if (src==NULL)
    return(-1);

  dest=fopen("mem41","wb");
  if (dest==NULL)
    return(-1);

  do {
    fgets(ligne,80,src);


  } while (!f);


}



int dat2lod(char *f1)
{
}


#endif


/* conversin des fichiers Antoine AstroNav */
int a2rom(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  char ligne[80];
  unsigned char buf[4];
  int i, n;

  src=fopen(f1,"rt");
  if (src==NULL)
    return(-1);

  dest=fopen(f2,"wb");
  if (dest==NULL)
    return(-1);

  i=0;
  do {
    if (fgets(ligne,80,src)==NULL) break;
    if (ligne[0]==0) break;
/*0123456789012
/*  CON(3) #2FE
*/
    n=ligne[10]; ligne[10]=ligne[12]; ligne[12]=n;      /* added JFG 02/05/06 */
    sscanf(&ligne[10],"%x",&n);
    buf[0]=n>>8;
    buf[1]=n;
    fwrite(buf,1,2,dest);
    i++;
    if (i==4096) break;
  } while (1);
  fclose(dest);
  fclose(src);
}


/* conversion des fichiers LIF Michael F. */
int lif2rom(char *f1, char *f2)
{
  FILE *src;
  FILE *dest;
  unsigned char bufo[8];
  unsigned char bufi[5];
  int i, n;
  unsigned char c;

  src=fopen(f1,"rb");
  if (src==NULL)
    return(-1);

  dest=fopen(f2,"wb");
  if (dest==NULL)
    return(-1);

  i=0;
  do {
    if (fread(bufi,1,5,src)!=5) break;
    c=bufi[0];
    bufo[0]= (c>>0)&3;
    bufo[1]=bufi[4];
    bufo[2]= (c>>2)&3;
    bufo[3]=bufi[3];
    bufo[4]= (c>>4)&3;
    bufo[5]=bufi[2];
    bufo[6]= (c>>6)&3;
    bufo[7]=bufi[1];
    fwrite(bufo,1,8,dest);
    i+=4;
    if (i==4096) break;
  } while (1);
  fclose(dest);
  fclose(src);
  return(i);
}



void usage(void)
{
  printf("Clbin41 : convert lst files into bin files for emu41\n");
  printf("Syntax: clbin41 [options] sourcefile destfile \n");
  printf("        if sourcefile='null', create empty bin file\n");
  printf("Option: /r convert unpacked rom file into packed bin file\n");
  printf("        /b convert packed bin file into unpacked rom file\n");
  printf("V1.9 JFG 2005\n");
}




void main(int argc, char *argv[])
{
  int i, n;
  char fhelp, from, fbin, fx;
  char argument[80];
  char nsrc[80];
  char ndest[80];

  fhelp=0;
  from=0;
  fbin=0;
  fx=0;
  nsrc[0]=0;
  ndest[0]=0;
  for (i=1;i<argc;i++) {
    strcpy(argument,argv[i]);
    if (argument[0]=='/') {
      argument[1]=toupper(argument[1]);
      switch (argument[1]) {
	case 'H':
	case '?': fhelp=1; break;
	case 'R': from=1; break;
	case 'B': fbin=1; break;
        case 'X': fx=1; break;
      }
    }
    else {
      if (nsrc[0]==0)
        strcpy(nsrc,argument);
      else
        strcpy(ndest,argument);
    }
  }

  if (fhelp)
    usage();
  else {
    if (fx)
//      n=ccdx_2lst(nsrc, ndest);
//      n=a2rom(nsrc, ndest);
      n=lif2rom(nsrc, ndest);
    else if (from)
      n=rom2bin(nsrc, ndest);
    else if (fbin)
      n=bin2rom(nsrc, ndest);
    else {
      if (stricmp(nsrc,"null")!=0)
        n=convert(nsrc, ndest);
      else
        n=cree(ndest);
    }
    if (n>0)
      printf("file %s created, %d words long\n",ndest,n);
    else if (n<0)
      printf("Error, file %s NOT created !!\n",ndest);
  }
}

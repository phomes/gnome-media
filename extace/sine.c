#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
   short v;
   double a=0;
   int f,i,j;
   
   for(j=2;j<1000;j++)
     {
	f=j;
	fprintf(stderr,"%5.5f\n",44100/(double)j);
	for (i=0;i<10000/j;i++)
	  {
	     for(a=(M_PI/(double)f);a<(2*M_PI);a+=(2*M_PI/(double)f))
	       {
		  v=(short)(sin(a)*32767);
		  fwrite(&v,sizeof(short),1,stdout);
		  fwrite(&v,sizeof(short),1,stdout);
	       }
	  }
     }
}

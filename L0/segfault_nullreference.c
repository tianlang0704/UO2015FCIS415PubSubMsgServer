// program5.c
// The program stores pairs i and i+1 from i = 0 to 9.
// and then drops the 3rd pair, and prints them without the 3rd pair.


#include <stdio.h>
#include <stdlib.h>

typedef struct{
  int left;
  int right;
}pair_t;

int main(int argc, char *argv[]){
  int i;
  pair_t ** pairs = calloc(10, sizeof(pair_t *));

  for(i = 0 ; i < 10; i++){
    pairs[i] = malloc(sizeof(pair_t));
    pairs[i]->left = i;
    pairs[i]->right = i+1;
  }

  //...
  free(pairs[2]);
  pairs[2] = NULL;
  //...

  for(i = 0 ; i < 10; i++)
  {
    if(pairs[i] != NULL)
    {
    	printf("%p: %d: left: %d right: %d\n", 
            pairs[i], i, pairs[i]->left, pairs[i]->right);
  
    }
  }

  for(i = 0 ; i < 10; i++)
  {
    if(pairs[i] != NULL)
    {
	free(pairs[i]);
    }
  }

  free(pairs);

  return 0;
}

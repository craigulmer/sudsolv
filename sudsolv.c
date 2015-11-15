#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>


#define GUESS_LINEAR (1)
#define GUESS_LEAST  (2)
#define GUESS_MOST   (3)
#define GUESS_WEIRD  (4)
#define GUESS_RANDOM (5)

//#define GUESS_TYPE (GUESS_WEIRD)
#define GUESS_TYPE (GUESS_LINEAR)

char *guess_alg_names[] = { "Unknown", "Linear", "Least Choices First", "Most Choices First", "Linear, No-Revisit (ie Weird)", "Random" };


struct timeval start_time;

//Looks at spot and figures out the values this could be
int checkspot(int x[9][9], int row, int col, int *possible_mask){

  int num_possible=9;
  int i,j;
  int spot;

  for(i=0;i<9; i++) possible_mask[i]=1;

  //All in our row
  for(j=0; j<9; j++){

    if(x[row][j] != 0){
      spot = x[row][j] - 1;
      assert((spot>=0) && (spot<9));

      if(possible_mask[ spot ] != 0){
        num_possible--;
        possible_mask[ spot ] = 0;
      }
    }
  }

  //All in our col
  for(i=0; i<9; i++){
    if(x[i][col] != 0){
      spot = x[i][col] - 1;
      assert((spot>=0) && (spot<9));
     
      if(possible_mask[ spot ] != 0){
        num_possible--;
        possible_mask[ spot ] = 0;
      }
    }
  }


  //All in our tile
  int rstart = (row/3);
  int cstart = (col/3);

  for(i=3*rstart; i<(3*rstart)+3; i++)
    for(j=3*cstart; j<(3*cstart)+3; j++){

      if(x[i][j] != 0){

        spot = x[i][j] - 1;
        if( (spot>=0) && (possible_mask[spot]!=0) ){
          num_possible--;
          possible_mask[ spot ] = 0;
        }
      }
    }

  return num_possible;
}


int pickNextGuess_linear(int x[9][9], char *ignore_guess_mask){
  int i,j;
  for(i=0; i<9; i++)
    for(j=0;j<9; j++)
      if((x[i][j]==0) && (ignore_guess_mask[i*9+j]==0)) 
        return i*9 + j;

  return -1;
}

int pickNextGuess_least(int x[9][9], char *ignore_guess_mask){

  int i,j;
  int possible_mask[9];
  int least_num=9;
  int least_id=-1;
  int tmp_num;

  for(i=0; i<9; i++)
    for(j=0; j<9; j++){
      if((x[i][j]==0)&&(ignore_guess_mask[i*9+j]==0)){
        tmp_num = checkspot(x, i,j, possible_mask);
        if(tmp_num < least_num){
          least_num = tmp_num;
          least_id = (i*9)+j;
        }
      }
    }
  return least_id;
}
int pickNextGuess_most(int x[9][9], char *ignore_guess_mask){

  int i,j;
  int possible_mask[9];
  int max_num=0;
  int max_id=-1;
  int tmp_num;

  for(i=0; i<9; i++)
    for(j=0; j<9; j++){
      if((x[i][j]==0)&&(ignore_guess_mask[i*9+j]==0)){
        tmp_num = checkspot(x, i,j, possible_mask);
        if(tmp_num > max_num){
          max_num = tmp_num;
          max_id = (i*9)+j;
        }
      }
    }
  return max_id;
}

int pickNextGuess_random(int x[9][9], char *ignore_guess_mask){
  int num_spots,i,j;
  int spots[81];

  num_spots=0;
  for(i=0;i<9;i++)
    for(j=0;j<9;j++)
      if( (x[i][j]==0) && (ignore_guess_mask[i*9+j]==0)){
        spots[num_spots]=i*9+j;
        num_spots++;
      }
 
  switch(num_spots){
  case 0: return -1;
  case 1: return spots[0];
  default:
    do{
      //Doing wider range and -1 to prevent rounding unfairness
      //To first and last in list
      i = round((float)(num_spots+2-1) * ((float) rand())/(float)RAND_MAX);
      i--;
    }while((i<0)||(i>=num_spots)); //Just redo if not in range

    return spots[i];
  }
}


int pickNextGuess_weird(int x[9][9], char *ignore_guess_mask, int *guesses){
  int i;

  //Look for the right-most guess we've made
  for(i=80; (i>=0) && (guesses[i]==0); i--);
  i++; //Move one spot to the right

  //See if this spot can be used for guessing. If not, move right
  while(i<81){
    int row = i/9;
    int col = i%9;
    if((x[row][col]==0) && (ignore_guess_mask[i]==0)) return i;
    i++;
  }
  return -1; //Did not find a spot to guess

}


int pickNextGuess(int x[9][9], char *ignore_guess_mask,int *guesses){
  int spot;
  switch GUESS_TYPE {
    case GUESS_LINEAR: spot = pickNextGuess_linear(x,ignore_guess_mask); break;
    case GUESS_MOST:   spot = pickNextGuess_most(x,ignore_guess_mask); break;
    case GUESS_LEAST:  spot = pickNextGuess_least(x,ignore_guess_mask); break;
    case GUESS_RANDOM: spot = pickNextGuess_random(x,ignore_guess_mask); break;
    case GUESS_WEIRD:  spot = pickNextGuess_weird(x,ignore_guess_mask,guesses); break;
    default:
      printf("Guess type not set?\n"); exit(0);
  }
  if(spot>=0) ignore_guess_mask[spot]=1;
  return spot;
}




void dump(int x[9][9]){
  int i,j;
  for(i=0;i<9;i++){
    for(j=0; j<9; j++){
      printf(" %s%d", (j%3)?"":"  ",x[i][j]);
    }
    printf("%s\n", ((i+1)%3)?"":"\n");
  }
  printf("\n");
}


int checkElim(int x[9][9], int row, int col, int *outter_possible_mask){

  int v;
  int rstart=row/3;
  int cstart=col/3;
  int inner_possible_mask[9];
  int is_unique;
  int i,j;

  for(v=0; v<9; v++){
    if(!outter_possible_mask[v]) continue;

    is_unique=1;

    //Check within our square to see if nobody else is using this
    for(i=rstart*3; i<rstart*3+3; i++)
      for(j=cstart*3; j<cstart*3+3; j++){
        //printf(" xcheck %d,%d for val %d\n",i,j, v+1);
        if(!((row==i)&&(col==j))&&(x[i][j]==0)){
          checkspot(x, i,j,inner_possible_mask);
          if(inner_possible_mask[v]) {
            //printf(" xcheck square killed\n");
            is_unique=0;
            i=j=9;
          }
        }
      }
    if(is_unique) return v;

    int k;

    //Check row to see if unique
    is_unique=1;
    for(i=0; i<9; i++){
      if((i==row) || (x[i][col]!=0)) continue;
      checkspot(x,i,col,inner_possible_mask);
      //printf(" IR for [%d,%d] vals are ", i,col);
      //for(k=0;k<9;k++)
      //  if(inner_possible_mask[k]) printf(" %d", k+1);
      //printf("\n");
      if(inner_possible_mask[v]){
        //printf(" xcheck row killed\n");
        is_unique=0;
        i=j=9;
      }
    }
    if(is_unique) return v;

    //Check col to see if unique
    is_unique=1;
    for(j=0; j<9; j++){
      if((j==col)||(x[row][j]!=0)) continue;
      checkspot(x,row,j,inner_possible_mask);
      if(inner_possible_mask[v]){
        //printf(" xcheck col killed\n");
        is_unique=0;
        i=j=9;
      }
    }
    if(is_unique) return v;


  } 
  return -1;
}

int findAllPlain(int x[9][9]){

  int possible_mask[9];
  int num_possible;
  int i,j,k;

  int num_found;

  int rounds=0;

  do {

    num_found=0;
    
    //printf("Round %d --------\n", rounds);

    for(i=0; i<9; i++){
      for(j=0; j<9; j++){

        //printf("Checking <%d,%d>==%d\n", i,j,x[i][j]);
        if(x[i][j] != 0) continue;
        num_possible =  checkspot(x, i,j,possible_mask);

        //printf("[%d,%d] Num Possible=%d\t", i,j, num_possible);
        //for(k=0; k<9; k++)
        //  if(possible_mask[k]) printf(" %d",k+1);
        //printf("\n");


        if(num_possible==1){
          for(k=0; possible_mask[k]==0; k++);
          //printf("[%d,%d] Found===> %d\n", i,j, k+1);
          x[i][j] = k+1;
          num_found++;
        } else if(num_possible>1) {
          k = checkElim(x,i,j,possible_mask);
          if(k>=0){
            //printf("XXXXXXXXXXXX Found Elim at %d,%d => %d\n", i,j,k+1);
            x[i][j] = k+1;
            num_found++;
          }
        }

      }
    }
    rounds++;
  } while(num_found>0);
 
  int num_empty=0;
  for(i=0; i<9; i++)
    for(j=0; j<9; j++)
      if(x[i][j]==0) num_empty++;

  //printf("Did %d rounds, Empty=%d\n", rounds, num_empty);
 

  return num_empty;

}
int iterate(int x[9][9], int guess_depth, int guesses[81]){


  int num_left;
  int y[9][9];
  int possible[9];
  int i,j,k;
  static int num_rounds=0;
  
  char ignore_guess_mask[81]; //Mark off ones we've tried already
  int next_guess_spot;

  //Ignore mask is just for this depth
  memset(ignore_guess_mask, 0, 81*sizeof(char));

  num_rounds++;
  if(!(num_rounds & 0x0FFF)){
    printf("::New Iterate, Depth=%d:", guess_depth);
    for(i=0;i<81; i++)
      if(guesses[i]) printf("%d=%d ",i,guesses[i]);
    printf("\n");
  }

  num_left = findAllPlain(x);

  if(num_left==0){ // and valid
    struct timeval stop_time;
    gettimeofday(&stop_time,NULL);
    printf("Time=%d s Algorithm=%s\n", stop_time.tv_sec - start_time.tv_sec, guess_alg_names[GUESS_TYPE]);
    printf("Guess depth=%d guesses:",guess_depth);

    for(i=0;i<81; i++)
      if(guesses[i]) printf("%d=%d ",i,guesses[i]);
    printf("\n");
    dump(x);
    exit(0);
  }

  next_guess_spot = pickNextGuess(x, ignore_guess_mask, guesses);

  while(next_guess_spot>=0){

    i=next_guess_spot/9;
    j=next_guess_spot%9;

    assert(x[i][j]==0);

    checkspot(x,i,j,possible);
    for(k=0;k<9;k++){
      if(!possible[k])continue;
      memcpy(y,x,9*9*sizeof(int));
      y[i][j] = k+1;
      guesses[i*9+j]=k+1;
      iterate(y,guess_depth+1,guesses);
    }
    guesses[i*9+j]=0;

    next_guess_spot = pickNextGuess(x,ignore_guess_mask, guesses);
  }

  return -1;

}


void readFile(char *filename, int x[9][9]){

  FILE *f;
  char buf[160];
  int row,col,i;
  size_t len;

  for(row=0;row<9;row++)
    for(col=0;col<9;col++)
      x[row][col]=-1;

  f=fopen(filename, "r");
  if(!f){ printf("Couldn't open %s\n", filename); exit(0);}

  row=0;
  while((row<9) && (!feof(f))){
    fgets(buf,160, f);
    col=0;
    for(i=0; (i<160) && (col<9) && (buf[i]!='\n'); i++){
      if(isdigit(buf[i])){
        x[row][col] = buf[i]-'0';
        col++;
      }
    }
    if((col>0)&&(col<9)){
      printf("Short read on row %d. Found %d values\n", row, col);
      exit(0);
    }
    if(col==9)
      row++;
  }
  
  if(row!=9){ printf("Wrong number of rows? Got %d\n", row); exit(0);}

  fclose(f);
}


int main(int argc, char **argv){

  int x[9][9];

  if(argc<2){ printf("sudsolv filename\n"); exit(0); }

  readFile(argv[1], x);

  dump(x);


#if 0

#if 0
  int x[9][9] = {
    {8,4,0, 3,0,0, 5,0,2},
    {0,0,3, 0,0,7, 0,6,1},
    {0,0,0, 0,0,8, 3,7,4},

    {3,0,0, 0,0,0, 2,0,0},
    {0,9,0, 0,0,0, 0,3,0},
    {0,0,1, 0,0,0, 0,0,7},
    
    {0,0,0, 5,0,0, 0,0,0},
    {2,1,0, 8,0,0, 7,0,0},
    {5,0,6, 0,0,2, 0,1,3} };
#else

  int x[9][9] = {
    { 8,0,0, 0,0,0, 0,0,0},
    { 0,0,3, 6,0,0, 0,0,0},
    { 0,7,0, 0,9,0, 2,0,0},

    { 0,5,0, 0,0,7, 0,0,0},
    { 0,0,0, 0,4,5, 7,0,0},
    { 0,0,0, 1,0,0, 0,3,0},

    { 0,0,1, 0,0,0, 0,6,8},
    { 0,0,8, 5,0,0, 0,1,0},
    { 0,9,0, 0,0,0, 4,0,0} };

#endif

#endif

  int *guesses=calloc(81,sizeof(int));
  //findAllPlain(x);
  //dump(x);

  gettimeofday(&start_time, NULL);
  iterate(x, 0, guesses);



}

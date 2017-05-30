#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#define WIDTH 100
#define HEIGHT 100
#define NUM_NODES 5
#define NUM_NOISE 0
struct Node
{
  sem_t sema;
  int message;
  int id;
  int nodeNum;
  int dwell_duration;
  int dwell_probability;
  int transition_time;
  int talk_window_time;
  int talk_probability;
  int prevMessages[99];
  int prevIDs[99];
  int owners[99];
  int rChannel[99];
  struct timeval rtime[99];
  int messages;
  /**
  * Dest is 0 if node is unoccupied
  * 1 otherwise
  */
  int dest;
  int type;//0 for Normal, 1 for Noise
  int channel;
  int x;
  int y;
};
struct coord{
  int x;
  int y;
};
int nodes_left=NUM_NODES;

struct thread_info{
  pthread_t id;
  struct Node *point;
};

struct Node grid[WIDTH][HEIGHT];//Global 2D array of struct Nodes


void initGrid() {
  for (int i=0; i<WIDTH; i++) {
    for (int j=0; j<HEIGHT; j++) {
      grid[i][j].dest = 0;
      grid[i][j].x = i;
      grid[i][j].y = j;
    }
  }
}
struct coord findSpace(void) {
  struct coord xy;

  do{
      xy.x = rand() % WIDTH;
      xy.y = rand() % HEIGHT;
    }
    while(grid[xy.x][xy.y].dest == 1);
  
  return xy; 
}
int transmit(struct Node * sender, struct Node * receive)
{
  int flag=1;
  for(int i=0;i<receive->messages;i++)
  {
    if(receive->prevIDs[receive->messages]==sender->id)
      flag=0;
  }
  if(flag)
  {
    receive->prevMessages[receive->messages]=sender->message;
    receive->prevIDs[receive->messages]=sender->id;
    gettimeofday(&receive->rtime[receive->messages],NULL);
    receive->owners[receive->messages]=sender->nodeNum;
    receive->rChannel[receive->messages]=receive->channel;
      receive->messages++;
  }
    //printf("Node ID:%i Message:%i Message ID:%i\n",receive->id,receive->prevMessages[receive->messages],receive->prevIDs[receive->messages]);
    return 0;
}
  sem_t semb,semc;
  pthread_cond_t nodes;
  struct timeval start, end;
static void *createNode(void *args) {
  struct coord *xy = args;
  printf("hooo\n");
  sem_init(&semb, 0,NUM_NODES);
  sem_init(&semc, 0,1);
  sem_wait(&semc);
  grid[xy->x][xy->y].dest = 1;
  //printf("1st x:%d y:%d\n",xy->x,xy->y);
  //Global struct nodes being modified in start routine
  grid[xy->x][xy->y].id=rand();
  grid[xy->x][xy->y].nodeNum=NUM_NODES-nodes_left;
  grid[xy->x][xy->y].dwell_probability=rand()%100;
  grid[xy->x][xy->y].dwell_duration=rand()%100;
  grid[xy->x][xy->y].message=rand();
  grid[xy->x][xy->y].transition_time=rand()%100+10;
  grid[xy->x][xy->y].talk_window_time=rand()%1000+100;
  grid[xy->x][xy->y].talk_probability=rand()%100;
  grid[xy->x][xy->y].channel=rand()%3+1;
  grid[xy->x][xy->y].x=xy->x;
  grid[xy->x][xy->y].y=xy->y;
  printf("Thread");
  if(nodes_left>0)
    grid[xy->x][xy->y].type=0;
  else
    grid[xy->x][xy->y].type=1;
  nodes_left--;
  //printf("nodes left%d",nodes_left);
  sem_post(&semc);
  if(nodes_left==0)
  {
    for(int z=0;z<NUM_NODES-1;z++)
      sem_post(&semb);
  }
  else
    sem_wait(&semb);

  int ttime=0;
  int flag=1;
  int count=0;
  while(ttime<10*grid[xy->x][xy->y].talk_window_time){
    ttime++;
    int time=0;
    int talk_prob=rand()%1000;
    while(time<grid[xy->x][xy->y].talk_window_time)
    {
      time++;
      if(talk_prob<grid[xy->x][xy->y].talk_probability&&flag)
      {
        for(int a=-5;a<5;a++)
          for(int j=-5;j<5;j++)
            if(xy->x+a>-1&&xy->y+j>-1&&xy->x+a<100&&xy->y+j<100)
              if(grid[xy->x+a][xy->y+j].channel==grid[xy->x][xy->y].channel)
                {
                //printf("Blocked");
                sem_wait(&grid[xy->x+a][xy->y+j].sema);
                  if(!(a==0&&j==0))
                  {
                    transmit(&grid[xy->x][xy->y],&grid[xy->x+a][xy->y+j]);
                    printf("Send Node X:%dY:%d Receive Node X:%d Y:%d\n",xy->x,xy->y,xy->x+a,xy->y+j);
                  }
                  //printf("LOCK ID:%d\n",grid[xy->x][xy->y].id);
                }
        for(int a=-5;a<5;a++)
          for(int j=-5;j<5;j++)
            if(xy->x+a>-1&&xy->y+j>-1&&xy->x+a<100&&xy->y+j<100)
              if(grid[xy->x+a][xy->y+j].channel==grid[xy->x][xy->y].channel)
                {
                sem_post(&grid[xy->x+a][xy->y+j].sema);
                  //printf("UNLOCK\n");
                }
          flag=0;
        //pthread_cond_signal(&finish);
        
      }
    }
    grid[xy->x][xy->y].channel=(grid[xy->x][xy->y].channel+1)%3+1;
    if(grid[xy->x][xy->y].messages>0)
      grid[xy->x][xy->y].message=grid[xy->x][xy->y].prevMessages[count];
    if(count<grid[xy->x][xy->y].messages)
      count++;
  }
  pthread_exit(0);
}
 

int main(void) 
{
  initGrid();
  srand(time(NULL));
  gettimeofday(&start,NULL);
  struct thread_info *threads=malloc(NUM_NODES * sizeof(struct thread_info));

  for(int i = 0; i < NUM_NODES+NUM_NOISE; i++) {
    struct coord tmp = findSpace();
    struct coord *tmp1 = malloc(sizeof(struct coord));
    tmp1->x = tmp.x;
    tmp1->y = tmp.y;

    if(pthread_create(&threads[i].id, NULL, &createNode,tmp1) != 0) {
      printf("Oops something went wrong\n");
      return -1;
    }

    threads[i].point = &grid[tmp1->x][tmp1->y];    
  }
    for(int l=0; l<NUM_NODES+NUM_NOISE; l++) {
      pthread_join(threads[l].id, NULL);
    }
    FILE* fp=NULL;
    for(int f=0;f<NUM_NODES;f++)
    {
    char filename[80];
    sprintf(filename,"Node_%d.txt",f);
    fp=fopen(filename,"w");
    fprintf(fp, "Node X Coordinate:%i Y Coordinate%i\n",threads[f].point->x,threads[f].point->y);
    for(int j1=0;j1<threads[f].point->messages;j1++)
    {
      fprintf(fp,"From Node:%i Message:%i Message ID:%i\n",threads[f].point->owners[j1],threads[f].point->prevMessages[j1],threads[f].point->prevIDs[j1]);
      int time=(threads[f].point->rtime[j1].tv_sec*1000000+threads[f].point->rtime[j1].tv_usec)-(start.tv_sec*1000000+start.tv_usec);
      fprintf(fp, "Time:%i Channel:\n",time,threads[f].point->owners[j1] );
    }
    }
    free(threads);
    return 0;
}
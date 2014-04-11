#include <stdio.h>
#include <stdlib.h>

#define NUM_FRAMES      16      /* Number of Page Frames */
#define NUM_PAGES       128     /* Number of memory pages.
				   The pages are numbered 1..NUM_PAGES */
#define NUM_ACCESSES    10000    /* Number of memory accesses */
#define NUM_RUNS	50	/* Number of times to run each case */

int seed = 0;

/* Structure to hold a page information */
typedef struct {
  int page_id;  /* The page ID number. If no page exits, it is set to -1 */
  int time_of_access;  /* Time when the page was last accessed */
  int time_of_arrival; /* Time when the page was brought in the memory */
} PageFrame;

/* Structure to hold page fault totals */
typedef struct {
  int page_faults_FIFO_seq;
  int page_faults_LRU_seq;
  int page_faults_OPT_seq;
  int page_faults_CUST_seq;

  int page_faults_FIFO_rand;
  int page_faults_LRU_rand;
  int page_faults_OPT_rand;
  int page_faults_CUST_rand;

  int page_faults_FIFO_lr;
  int page_faults_LRU_lr;
  int page_faults_OPT_lr;
  int page_faults_CUST_lr;
} PageFaultTotal;



/* Function pointer to the handler for the page replacement algorithm */
typedef int (* PRAlgoType) (const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access);

/* Memory management Handler Algorithms prototypes */
int PRAlgo_FIFO(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access);
int PRAlgo_LRU(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access);
int PRAlgo_OPT(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access);
int PRAlgo_CUST(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access);


/* Initialize the page frames, set all values to -1 */
void initialize_page_frames(PageFrame * PageFrames, int size)
{
  int i;

  for(i = 0; i < size; i++)
  {
    PageFrames[i].page_id = -1;
    PageFrames[i].time_of_access = -1;
    PageFrames[i].time_of_arrival = -1;
  }

  return;
}

/* Functions to generate page request data */

/* Generate a sequence of random page accesses. */
int * build_random_access_seq(int * PageAccesses, int num_accesses)
{
  int i;

  srand(seed);

  for(i=0; i < num_accesses; i++)
    PageAccesses[i] = (rand() % NUM_PAGES) + 1;

  return PageAccesses;
}


/* Sequential page access */
int * build_sequential_access_seq(int * PageAccesses, int num_accesses)
{
  int i;

  for(i=0; i<num_accesses; i++)
    PageAccesses[i] = (i % NUM_PAGES) + 1;

  return PageAccesses;
}


/* LR_workload page access */
int * build_lr_workload_access_seq(int * PageAccesses, int num_accesses)
{
  int i;
  size_t t;

  srand(seed);

  for(i=0; i<num_accesses; i++)
  {
    if(i<5)                         /* Assign the first five just randomly */
      PageAccesses[i] = (rand() % NUM_PAGES) + 1;

    else if((rand() % 100) <90)     /* Then assign based on history for 90% cases */
      PageAccesses[i] = PageAccesses[i-(rand()%5)-1];

    else
      PageAccesses[i] = (rand() % NUM_PAGES) + 1; /* Assign randomly for the rest 20% cases */
  }

  return PageAccesses;
}


/* Function to read the page accesses. It uses the Page replacement Handlers. Returns the number of page faults */
int handle_page_accesses(PageFrame * PageFrames, int num_frames, int * PageAccesses, int num_accesses, PRAlgoType PRAlgo)
{
  int i;
  int next_page = 0; /* Page to be read */
  int page_faults = 0; /* No of page faults */
  int current_access = 0;  /* The current time of access */

  /* Check for all memory accesses one by one */
  for(current_access=0; current_access < num_accesses; current_access++)
  {
    next_page = PageAccesses[current_access]; /* Read the next page access */

    int page_to_be_replaced = -1;  /* Set to index of the page to be replaced */
    int go_to_next_page_access = 0;

    /* Now search for the page in the memory */
    for(i=0; i<num_frames; i++)
    {
      if(next_page == PageFrames[i].page_id)
      {
	/* Page in memory, just update the time of access */
	PageFrames[i].time_of_access = current_access;
	go_to_next_page_access = 1;
	break;
      }
    }

    if(go_to_next_page_access)  /* No fault, so check for the next access */
      continue;

    /* Page Fault as Page was not in memory */
    page_faults++;

    /* First check for free memory */
    for(i=0; i<num_frames; i++)
    {
      if(PageFrames[i].page_id == -1)
      {
	/* Empty memory location found. Insert Frame in memory */
	PageFrames[i].page_id = next_page;
	PageFrames[i].time_of_access  = current_access;
	PageFrames[i].time_of_arrival = current_access;

	go_to_next_page_access = 1;
	break;
      }
    }


    if(go_to_next_page_access)  /* No need for replacement, so check for the next access */
      continue;

    /* No free memory! Find a victim to replace in the memory using the handler */
    page_to_be_replaced = PRAlgo(PageFrames,num_frames,PageAccesses,num_accesses,current_access);

    PageFrames[page_to_be_replaced].page_id = next_page;
    PageFrames[page_to_be_replaced].time_of_access = current_access;
    PageFrames[page_to_be_replaced].time_of_arrival = current_access;

  }

  return page_faults;
}


int main ()
{
  int PageAccesses[NUM_ACCESSES];    /* Array to store the page accesses */
  PageFrame PageFrames[NUM_FRAMES];    /* Array representing the page frame */
  PageFaultTotal PageFaultTotals;	/* Track page fault totals across runs */
  /* Initialize all page faults for all cases to 0 */
  PageFaultTotals.page_faults_FIFO_seq = 0;
  PageFaultTotals.page_faults_LRU_seq = 0;
  PageFaultTotals.page_faults_OPT_seq = 0;
  PageFaultTotals.page_faults_CUST_seq = 0;
  PageFaultTotals.page_faults_FIFO_rand = 0;
  PageFaultTotals.page_faults_LRU_rand = 0;
  PageFaultTotals.page_faults_OPT_rand = 0;
  PageFaultTotals.page_faults_CUST_rand = 0;
  PageFaultTotals.page_faults_FIFO_lr = 0;
  PageFaultTotals.page_faults_LRU_lr = 0;
  PageFaultTotals.page_faults_OPT_lr = 0;
  PageFaultTotals.page_faults_CUST_lr = 0;

  int i;
  for(i = 0; i < NUM_RUNS; i++) {
  	/* Memory access analysis with random access */
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	build_random_access_seq(PageAccesses,NUM_ACCESSES);
  	PageFaultTotals.page_faults_FIFO_rand += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_FIFO);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_LRU_rand += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_LRU);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_OPT_rand += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_OPT);
	initialize_page_frames(PageFrames,NUM_FRAMES);
	PageFaultTotals.page_faults_CUST_rand += handle_page_accesses(PageFrames, NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_CUST);


  	/* Memory access analysis with sequential access */
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	build_sequential_access_seq(PageAccesses,NUM_ACCESSES);
  	PageFaultTotals.page_faults_FIFO_seq += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_FIFO);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_LRU_seq += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_LRU);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_OPT_seq += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_OPT);
	initialize_page_frames(PageFrames,NUM_FRAMES);
	PageFaultTotals.page_faults_CUST_seq += handle_page_accesses(PageFrames, NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_CUST);


  	/* Memory access analysis with LR workload access */
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	build_lr_workload_access_seq(PageAccesses,NUM_ACCESSES);
  	PageFaultTotals.page_faults_FIFO_lr += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_FIFO);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_LRU_lr += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_LRU);
  	initialize_page_frames(PageFrames,NUM_FRAMES);
  	PageFaultTotals.page_faults_OPT_lr += handle_page_accesses(PageFrames,NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_OPT);
	initialize_page_frames(PageFrames,NUM_FRAMES);
	PageFaultTotals.page_faults_CUST_lr += handle_page_accesses(PageFrames, NUM_FRAMES,PageAccesses,NUM_ACCESSES,PRAlgo_CUST);

	seed++;		// Increment seed to generate new sequence of accesses next iteration
  }

  printf("The average number of page faults for FIFO with Random Access is %d.\n",PageFaultTotals.page_faults_FIFO_rand / NUM_RUNS);
  printf("The average number of page faults for LRU with Random Access is %d.\n",PageFaultTotals.page_faults_LRU_rand / NUM_RUNS);
  printf("The average number of page faults for OPT with Random Access is %d.\n",PageFaultTotals.page_faults_OPT_rand / NUM_RUNS);
  printf("The average number of page faults for CUST with Random Access is %d.\n",PageFaultTotals.page_faults_CUST_rand / NUM_RUNS);

  printf("The average number of page faults for FIFO with Sequential Access is %d.\n",PageFaultTotals.page_faults_FIFO_seq / NUM_RUNS);
  printf("The average number of page faults for LRU with Sequential Access is %d.\n",PageFaultTotals.page_faults_LRU_seq / NUM_RUNS);
  printf("The average number of page faults for OPT with Sequential Access is %d.\n",PageFaultTotals.page_faults_OPT_seq / NUM_RUNS);
  printf("The average number of page faults for CUST with Sequential Access is %d.\n",PageFaultTotals.page_faults_CUST_seq / NUM_RUNS);

  printf("The average number of page faults for FIFO with LR Workload Access is %d.\n",PageFaultTotals.page_faults_FIFO_lr / NUM_RUNS);
  printf("The average number of page faults for LRU with LR Workload Access is %d.\n",PageFaultTotals.page_faults_LRU_lr / NUM_RUNS);
  printf("The average number of page faults for OPT with LR Workload Access is %d.\n",PageFaultTotals.page_faults_OPT_lr / NUM_RUNS);
  printf("The average number of page faults for CUST with LR Workload Access is %d.\n",PageFaultTotals.page_faults_CUST_lr / NUM_RUNS);

  return 0;
}


/* Memory allocation on the basis of Time of Arrival */
int PRAlgo_FIFO(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access)
{
  int least_time_of_arrival = PageFrames[0].time_of_arrival;
  int index_with_least_arrival_time = 0;
  int i;


  for(i = 1; i < num_frames; i++)
  {
    if(PageFrames[i].time_of_arrival < least_time_of_arrival)
    {
      least_time_of_arrival = PageFrames[i].time_of_arrival;
      index_with_least_arrival_time = i;
    }
  }

  return index_with_least_arrival_time;
}

/* Memory allocation on the basis of least recently used */
int PRAlgo_LRU(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access){
  int least_recently_used = PageFrames[0].time_of_access;
  int index_least_recently_used = 0;
  int i;

  for (i = 1; i < num_frames; i++){
    if (PageFrames[i].time_of_access < least_recently_used){
      least_recently_used = PageFrames[i].time_of_access;
      index_least_recently_used = i;
    }
  }
  return index_least_recently_used;
}

/* Memory allocation on the basis of locality */
int PRAlgo_CUST(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access){
  int i;
  int j;
  int k;
  int l = 0;
  int recent_pages[5] = {};
  /* Find the pages used in the past 5 cycles */
  for (i = current_access; (i >= 0 && i > (current_access - 5)); i--){
    recent_pages[l] = PageAccesses[i];
    l++;
  }
  for (j = 0; j< num_frames; j++){
    int flag = 0;
    for (k = 0; k < 5; k++){
      // printf("%d\n", recent_pages[k]);
      // printf("%d\n", PageFrames[j].page_id);
      if (recent_pages[k] == PageFrames[j].page_id){
        flag = 1;
        break;
      }
    }
    if (flag == 0){
      return j;
    }
  }
  return 0;

}

/* Memory allocation on the basis of used farthest in the future*/
int PRAlgo_OPT(const PageFrame * PageFrames, int num_frames, const int * PageAccesses, int num_accesses, int current_access){
  int i;
  int j;
  int k;
  int time_of_next_access[num_frames];
  /* Gather the next used time for each page frame */
  for (i = 0; i < num_frames; i++){
    time_of_next_access[i] = 99999;
    for (j = current_access+1; j < num_accesses; j++){
      if (PageAccesses[j] == PageFrames[i].page_id){
        // printf("time_of_next_access for page_frame %d is %d\n", i, j);
        time_of_next_access[i] = j;
        break;
      }
    }
  }

  /* return index of the time_of_next_access that is the largest */
  /*The time of next access indices match the indices of the PageFrame array */
  int used_farthest_in_the_future = time_of_next_access[0];
  int index_used_farthest_in_the_future = 0;
  for (k = 1; k < num_frames; k++){
    if (time_of_next_access[k] > used_farthest_in_the_future){
      used_farthest_in_the_future = time_of_next_access[k];
      index_used_farthest_in_the_future = k;
    }
  }
  return index_used_farthest_in_the_future;

}

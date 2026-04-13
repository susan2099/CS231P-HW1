#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_CYCLES 1000000
#define EPS        1e-6
#define WARMUP     100
#define PI         3.14159265358979323846

static unsigned long long rng_state = 0xDEADBEEFCAFEBABEULL;
static inline unsigned long long xrand(void){
    unsigned long long x=rng_state; x^=x<<13; x^=x>>7; x^=x<<17; return rng_state=x;
}
static inline int urand_mod(int n){return (int)(xrand()%(unsigned long long)n);}
static inline double urand01(void){return (xrand()>>11)*(1.0/9007199254740992.0);}

static int    nrand_have_spare=0;
static double nrand_spare=0.0;
static double nrand(void){
    if(nrand_have_spare){nrand_have_spare=0; return nrand_spare;}
    double u1,u2;
    do{u1=urand01();}while(u1<=1e-300);
    u2=urand01();
    double r=sqrt(-2.0*log(u1)), t=2.0*PI*u2;
    nrand_spare=r*sin(t); nrand_have_spare=1;
    return r*cos(t);
}

static inline int next_request(int i,int m,double sigma,const double*mu,char dist){
    if(dist=='u') return urand_mod(m);
    long r=(long)llround(nrand()*sigma+mu[i]);
    r%=m; if(r<0) r+=m;
    return (int)r;
}

static double run_one(int procs,int m,char dist,
                      int*request,long*granted,long*wait_acc,long*wt_curr,
                      double*mu,char*busy)
{
    for(int i=0;i<procs;i++){
        granted[i]=0; wait_acc[i]=0; wt_curr[i]=0;
        if(dist=='n') mu[i]=(double)urand_mod(m);
    }
    double sigma=(double)m/6.0;
    for(int i=0;i<procs;i++) request[i]=next_request(i,m,sigma,mu,dist);

    int base=0;
    double prev_W=0.0, curr_W=0.0;

    for(long c=1;c<=MAX_CYCLES;c++){
        memset(busy,0,(size_t)m);
        int new_base=-1;

        for(int k=0;k<procs;k++){
            int i=base+k; if(i>=procs) i-=procs;
           
            wt_curr[i]++;
            int req=request[i];
            if(!busy[req]){
                busy[req]=1;
                granted[i]++;
                wait_acc[i]+=wt_curr[i];   
                wt_curr[i]=0;
                request[i]=next_request(i,m,sigma,mu,dist);
            } else if(new_base==-1){
                new_base=i;                
            }
        }
        if(new_base!=-1) base=new_base;

        
        double sum=0.0;
        for(int i=0;i<procs;i++){
            double num=(double)(wait_acc[i]+wt_curr[i]);
            double den=(granted[i]>0)?(double)granted[i]:1.0;
            sum+=num/den;
        }
        curr_W=sum/(double)procs;

        if(c>WARMUP && curr_W>0.0){
            double d=1.0-(prev_W/curr_W); if(d<0) d=-d;
            if(d<EPS) break;
        }
        prev_W=curr_W;
    }
    return curr_W;
}

void simulate(double*avg_wait,int avg_wait_l,int procs,char dist){
    rng_state = 0xDEADBEEFCAFEBABEULL
              ^ ((unsigned long long)procs<<32)
              ^ (unsigned long long)dist;
    if(rng_state==0) rng_state=1;      
    nrand_have_spare=0;

    int   *request =malloc(sizeof(int)   *(size_t)procs);
    long  *granted =malloc(sizeof(long)  *(size_t)procs);
    long  *wait_acc=malloc(sizeof(long)  *(size_t)procs);
    long  *wt_curr =malloc(sizeof(long)  *(size_t)procs);
    double*mu      =malloc(sizeof(double)*(size_t)procs);
    char  *busy    =malloc(sizeof(char)  *(size_t)avg_wait_l);

    for(int idx=0;idx<avg_wait_l;idx++){
        avg_wait[idx]=run_one(procs,idx+1,dist,
                              request,granted,wait_acc,wt_curr,mu,busy);
    }
    free(request); free(granted); free(wait_acc);
    free(wt_curr); free(mu);      free(busy);
}

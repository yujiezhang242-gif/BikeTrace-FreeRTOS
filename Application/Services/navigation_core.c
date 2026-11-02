#include "navigation_core.h"
#include <string.h>
static int bearing(const char *s) {
    static const char *const dirs[]={"N","NE","E","SE","S","SW","W","NW"};unsigned i;
    for(i=0;i<8;i++)if(!strcmp(s,dirs[i]))return (int)i*45;return -1;
}
void NavigationCore_Tick(NavigationCore *n,const Navigation *p,const BikeSnapshot *b) {
    uint32_t delta;int target,diff,correct=1;
    if(!p->count||p->count>NAV_MAX_STEPS)return;
    if(n->version!=p->version||n->ride_id!=b->ride_id) {
        memset(n,0,sizeof(*n));n->version=p->version;n->ride_id=b->ride_id;n->last_distance_mm=b->distance_mm;n->remaining_mm=p->steps[0].distance*1000;
    }
    if(b->state!=BIKE_RIDE||n->done)return;
    delta=b->distance_mm>=n->last_distance_mm?b->distance_mm-n->last_distance_mm:0;n->last_distance_mm=b->distance_mm;
    target=bearing(p->steps[n->step].orientation);
    if(b->mag_valid&&target>=0) {diff=target-b->heading;while(diff>180)diff-=360;while(diff<-180)diff+=360;correct=diff>=-67&&diff<=67;}
    if(!correct) {
        if(delta>UINT32_MAX-n->wrong_mm)n->wrong_mm=UINT32_MAX;else n->wrong_mm+=delta;
        if(delta>UINT32_MAX-n->remaining_mm)n->remaining_mm=UINT32_MAX;else n->remaining_mm+=delta;
        if(n->wrong_mm>=100000&&!n->replan_pending)n->replan_pending=1;
        return;
    }
    n->wrong_mm=0;
    while(delta>=n->remaining_mm) {
        delta-=n->remaining_mm;
        if(n->step+1>=p->count) {n->remaining_mm=0;n->done=1;return;}
        n->step++;n->remaining_mm=p->steps[n->step].distance*1000;
    }
    n->remaining_mm-=delta;
}
int NavigationCore_NeedsReplan(const NavigationCore *n) {return n->replan_pending==1;}

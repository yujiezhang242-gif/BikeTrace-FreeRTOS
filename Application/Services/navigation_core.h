#ifndef NAVIGATION_CORE_H
#define NAVIGATION_CORE_H
#include "net_protocol.h"
typedef struct {
    uint32_t version,ride_id,last_distance_mm,remaining_mm,wrong_mm;
    unsigned step;
    uint8_t replan_pending,done;
} NavigationCore;
void NavigationCore_Tick(NavigationCore *n,const Navigation *path,const BikeSnapshot *bike);
int NavigationCore_NeedsReplan(const NavigationCore *n);
#endif

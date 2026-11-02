#include "geo.h"
#include <math.h>
/* Conversion retained from legacy utils.c; outside-China coordinates pass through. */
#define pi 3.1415926535897932384626
#define a 6378245.0
#define ee 0.00669342162296594323
/* 转换纬度 */
static double convert_lat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
    return ret;
}

/* 转换经度 */
static double convert_lon(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
    return ret;
}

/* WGS-84坐标转换为GCJ-02坐标 */
void Geo_Gcj02(double wgs_lat, double wgs_lon, double* gcj_lat, double* gcj_lon)
{
    if(wgs_lon<72.004||wgs_lon>137.8347||wgs_lat<0.8293||wgs_lat>55.8271) {*gcj_lat=wgs_lat;*gcj_lon=wgs_lon;return;}
    double dLat = convert_lat(wgs_lon - 105.0, wgs_lat - 35.0);
    double dLon = convert_lon(wgs_lon - 105.0, wgs_lat - 35.0);
    double radLat = wgs_lat / 180.0 * pi;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
    *gcj_lat = wgs_lat + dLat;
    *gcj_lon = wgs_lon + dLon;
}


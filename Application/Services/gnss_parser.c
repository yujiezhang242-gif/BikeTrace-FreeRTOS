#include "gnss_parser.h"
#include <string.h>
static int hex(char c) {
    if(c>='0' && c<='9') return c-'0';
    if(c>='A' && c<='F') return c-'A'+10;
    if(c>='a' && c<='f') return c-'a'+10;
    return -1;
}
static int digits(const char *s, unsigned n, uint32_t *v) {
    unsigned i; *v=0;
    for(i=0;i<n;i++) { if(s[i]<'0'||s[i]>'9') return 0; *v=*v*10U+(uint32_t)(s[i]-'0'); }
    return 1;
}
/* Decimal with at most 6 fraction digits, no signs/exponents/NaNs. */
static int decimal(const char *s, double *v) {
    unsigned count=0, frac=0; double scale=1; *v=0;
    while(*s>='0'&&*s<='9') { if(++count>9) return 0; *v=*v*10+(*s++-'0'); }
    if(!count) return 0;
    if(*s=='.') { s++; while(*s>='0'&&*s<='9') { if(++frac>6) return 0; scale*=10; *v+=(*s++-'0')/scale; } if(!frac)return 0; }
    return *s==0;
}
static int coordinate(const char *s, const char *dir, int lat, double *v) {
    double dm, minutes; int degrees;
    if(!decimal(s,&dm)||strlen(dir)!=1) return 0;
    degrees=(int)(dm/100); minutes=dm-degrees*100;
    if(minutes>=60||degrees>(lat?90:180)||(degrees==(lat?90:180)&&minutes>0)) return 0;
    if(lat ? (dir[0]!='N'&&dir[0]!='S') : (dir[0]!='E'&&dir[0]!='W')) return 0;
    *v=degrees+minutes/60;
    if(dir[0]=='S'||dir[0]=='W') *v=-*v;
    return 1;
}
int GpsFix_IsFresh(const GpsFix *f,uint32_t now) {
    return f && f->valid && (uint32_t)(now-f->received_ms)<GPS_STALE_MS;
}
void GnssParser_Reset(GnssParser *p) { memset(p,0,sizeof(*p)); }
static int parse(char *s,GpsFix *out,uint32_t now) {
    char *f[14], *star, *q; unsigned nf=0,i; uint8_t sum=0;
    uint32_t tm, date, day, month, year, days; double speed, seconds;
    GpsFix fix;
    star=strchr(s,'*');
    if(!star||strlen(star)!=5||star[3]!='\r'||star[4]!='\n'||hex(star[1])<0||hex(star[2])<0)return 0;
    for(q=s+1;q<star;q++)sum^=(uint8_t)*q;
    if(sum!=(uint8_t)((hex(star[1])<<4)|hex(star[2])))return 0;
    *star=0; f[nf++]=s;
    for(q=s;*q;q++) if(*q==',') { *q=0; if(nf>=14)return 0; f[nf++]=q+1; }
    if(nf<10||(strcmp(f[0],"$GPRMC")&&strcmp(f[0],"$GNRMC")))return 0;
    if(strlen(f[2])!=1||(f[2][0]!='A'&&f[2][0]!='V'))return 0;
    if(f[2][0]=='V') {memset(out,0,sizeof(*out));out->received_ms=now;return 1;}
    if(strlen(f[1])<6||!digits(f[1],6,&tm)||!decimal(f[1],&seconds))return 0;
    if(tm/10000>23||(tm/100)%100>59||tm%100>59)return 0;
    if(strlen(f[9])!=6||!digits(f[9],6,&date))return 0;
    day=date/10000;month=(date/100)%100;year=2000+date%100;
    if(month<1||month>12)return 0;
    { static const uint8_t md[12]={31,28,31,30,31,30,31,31,30,31,30,31}; days=md[month-1]; }
    if(month==2&&year%4==0)days++;
    if(day<1||day>days)return 0;
    memset(&fix,0,sizeof(fix)); fix.received_ms=now;fix.date_ddmmyy=date;
    fix.utc_seconds=(tm/10000)*3600+((tm/100)%100)*60+tm%100;
    if(strlen(f[2])!=1||(f[2][0]!='A'&&f[2][0]!='V'))return 0;
    fix.valid=(uint8_t)(f[2][0]=='A');
    if(fix.valid) {
        if(!coordinate(f[3],f[4],1,&fix.latitude_deg)||!coordinate(f[5],f[6],0,&fix.longitude_deg)||!decimal(f[7],&speed)||speed>200)return 0;
        fix.speed_milliknots=(uint32_t)(speed*1000+0.5);
    }
    /* RMC mode N invalidates otherwise A fixes. */
    for(i=12;i<nf;i++)if(!strcmp(f[i],"N"))fix.valid=0;
    *out=fix;return 1;
}
int GnssParser_Feed(GnssParser *p,uint8_t b,uint32_t now,GpsFix *out) {
    if(b=='$') { p->length=0;p->collecting=1;p->dropping=0; }
    if(!p->collecting)return 0;
    if(p->length>=sizeof(p->frame)-1) { p->dropping=1;p->collecting=0;p->oversized++;return -1; }
    p->frame[p->length++]=(char)b;
    if(b!='\n')return 0;
    p->frame[p->length]=0;p->collecting=0;
    if(strncmp(p->frame,"$GPRMC,",7)&&strncmp(p->frame,"$GNRMC,",7))return 0;
    if(parse(p->frame,out,now)) {p->good++;return 1;} p->bad++;return -1;
}

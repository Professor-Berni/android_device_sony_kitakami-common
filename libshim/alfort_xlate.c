/* Sony Adreno program binary v0x10002 -> v0x10007. The a4xx ISA is copied
 * verbatim because the driver never validates it. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAGIC 0xb10bcaceu
#define VER_V2 0x00010002u
#define VER_V7 0x00010007u
#define H14_V7 0x00000301u
#define SECCNT 13
#define TABOFF 0x50u
#define ENTSZ 0x20u
#define SYM_STRIDE 96u
#define SYM_BASE 0x4cu
/* The v7 ISA blob is 56 bytes longer so shift EVERY section offset, not just the first six. */
#define CODE_GROW 0x38u
#define OBJ_TAB_OFF 0x14u
#define OBJ_TAB_CNT 0x18u
#define OBJ_ENTSZ 20u
/* v7 looks up types 53..55 and needs them present-but-empty; v2 stopped at 52. */
#define OBJ_TYPES 56u
int g_code_grow = 1;
#define BUILD_LEN 20u

const char *g_driver_so = "/vendor/lib/egl/libGLESv2_adreno.so";
char g_build_key[BUILD_LEN];

static inline uint32_t rd(const unsigned char* b, uint32_t o){ uint32_t v; memcpy(&v,b+o,4); return v; }
static inline void wr(unsigned char* b, uint32_t o, uint32_t v){ memcpy(b+o,&v,4); }

typedef struct {
    uint32_t type, doff, psz, count, next;
    uint32_t rlen;
    uint32_t new_doff;
    uint32_t new_psz;
} Sec;

static uint32_t rotxor(const unsigned char* p, uint32_t n){
    uint32_t h = 0;
    for(uint32_t i=0;i<n;i++){ h ^= p[i]; h = (h >> 25) | (h << 7); }
    return h;
}

uint32_t g_driver_key = 0xa4a8ccc2u;
int g_symcount_delta = 3;

static int is_date(const unsigned char* p)
{
    static const char* mon = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char m[4] = { (char)p[0], (char)p[1], (char)p[2], 0 };
    if (!strstr(mon, m)) return 0;
    return p[3]==' ' && (p[4]==' '||(p[4]>='0'&&p[4]<='9')) && p[5]>='0' && p[5]<='9'
        && p[6]==' ' && p[7]>='0' && p[7]<='9' && p[8]>='0' && p[8]<='9'
        && p[9]>='0' && p[9]<='9' && p[10]>='0' && p[10]<='9' && p[11]==0;
}

static int is_time(const unsigned char* p)
{
    return p[0]>='0'&&p[0]<='9' && p[1]>='0'&&p[1]<='9' && p[2]==':'
        && p[3]>='0'&&p[3]<='9' && p[4]>='0'&&p[4]<='9' && p[5]==':'
        && p[6]>='0'&&p[6]<='9' && p[7]>='0'&&p[7]<='9' && p[8]==0;
}

int load_driver_key(const char* path, char* dst, int dstlen)
{
    FILE* f = fopen(path, "rb");
    unsigned char* b;
    long n;
    int found = 0;

    if (!f) return -1;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    b = (unsigned char*)malloc((size_t)n);
    if (!b) { fclose(f); return -1; }
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); fclose(f); return -1; }
    fclose(f);

    for (long i = 0; i + 12 < n && !found; i++) {
        long j;
        if (!is_date(b + i)) continue;
        for (j = i + 12; j < n && j < i + 32 && b[j] == 0; j++) { }
        if (j + 9 > n || !is_time(b + j)) continue;
        if (11 + 8 >= dstlen) break;
        memset(dst, 0, (size_t)dstlen);
        memcpy(dst, b + i, 11);
        memcpy(dst + 11, b + j, 8);
        found = 1;
    }
    free(b);
    return found ? 0 : -1;
}

int translate_v2_to_v7(const unsigned char* in, int len, unsigned char** out, int* outlen)
{
    if(!in || !out || !outlen || len < (int)(TABOFF + SECCNT*ENTSZ)) return -1;
    if(rd(in,0x00) != MAGIC) return -2;
    if(rd(in,0x04) != VER_V2) return -3;
    if(rd(in,0x08) != SECCNT) return -4;
    if(rd(in,0x0c) != TABOFF) return -5;

    Sec s[SECCNT];
    for(int i=0;i<SECCNT;i++){
        uint32_t o = TABOFF + (uint32_t)i*ENTSZ;
        s[i].type=rd(in,o+0x00); s[i].doff=rd(in,o+0x04); s[i].psz=rd(in,o+0x08);
        s[i].count=rd(in,o+0x0c); s[i].next=rd(in,o+0x10);
        s[i].new_doff=0; s[i].new_psz=s[i].psz;
    }

    for(int i=0;i<SECCNT;i++){
        uint32_t best=(uint32_t)len;
        for(int j=0;j<SECCNT;j++) if(s[j].doff>s[i].doff && s[j].doff<best) best=s[j].doff;
        s[i].rlen=best-s[i].doff;
    }

    for(int i=0;i<SECCNT;i++){
        switch(s[i].type){
            case 5: s[i].new_psz=(s[i].psz==16)?20:s[i].psz; break;
            case 2: s[i].new_psz=BUILD_LEN; break;
            case 1: s[i].new_psz=s[i].psz+(g_code_grow?CODE_GROW:0); break;
            default:s[i].new_psz=s[i].psz; break;
        }
    }

    uint32_t doffs[SECCNT]; int nd=0;
    for(int i=0;i<SECCNT;i++){
        int seen=0; for(int k=0;k<nd;k++) if(doffs[k]==s[i].doff){seen=1;break;}
        if(!seen) doffs[nd++]=s[i].doff;
    }
    for(int a=1;a<nd;a++){ uint32_t v=doffs[a]; int b=a-1; while(b>=0&&doffs[b]>v){doffs[b+1]=doffs[b];b--;} doffs[b+1]=v; }

    uint32_t map_old[SECCNT], map_new[SECCNT]; int nm=0;
    uint32_t cur = TABOFF + SECCNT*ENTSZ;

    for(int d=0; d<nd; d++){
        uint32_t od=doffs[d];
        if(od==0x1f0){
            int eof=-1, ph=-1;
            for(int i=0;i<SECCNT;i++){
                if(s[i].doff==od && s[i].type==2) eof=i;
                if(s[i].doff==od && s[i].type==10) ph=i;
            }
            if(eof>=0) s[eof].new_doff=cur;
            map_old[nm]=od; map_new[nm]=cur;
            cur += BUILD_LEN;
            cur = (cur + 7u) & ~7u;
            if(ph>=0) s[ph].new_doff=cur;
            cur += 64;
            nm++;
            continue;
        }
        int owner=-1; uint32_t orl=0;
        for(int i=0;i<SECCNT;i++) if(s[i].doff==od && s[i].rlen>=orl){ orl=s[i].rlen; owner=i; }
        uint32_t placed=orl;
        if(s[owner].type==5) placed=(s[owner].psz==16)?20:s[owner].rlen;
        if(s[owner].type==1 && g_code_grow) placed=orl+CODE_GROW;
        for(int i=0;i<SECCNT;i++) if(s[i].doff==od) s[i].new_doff=cur;
        map_old[nm]=od; map_new[nm]=cur; nm++;
        cur += placed;
        cur = (cur + 3u) & ~3u;
    }

    uint32_t newsize=cur;
    unsigned char* o=(unsigned char*)calloc(1,newsize);
    if(!o) return -10;

    wr(o,0x00,MAGIC);
    wr(o,0x04,VER_V7);
    wr(o,0x08,SECCNT);
    wr(o,0x0c,TABOFF);
    wr(o,0x10,rd(in,0x10));
    wr(o,0x14,H14_V7);
    wr(o,0x18,newsize);
    wr(o,0x1c,newsize);
    wr(o,0x20,rd(in,0x20)+(uint32_t)g_symcount_delta);

    uint32_t ph_old_base=0x1f0, ph_new_base=0;
    for(int i=0;i<SECCNT;i++) if(s[i].type==10){ ph_new_base=s[i].new_doff; break; }

    #define RELOC(OLD, OUTVAR) do { \
        uint32_t _ov=(OLD); uint32_t _nv=_ov; int _done=0; \
        if(_ov>=ph_old_base && _ov<ph_old_base+64){ _nv=ph_new_base+(_ov-ph_old_base); _done=1; } \
        if(!_done){ for(int _m=0;_m<nm;_m++){ \
            uint32_t _ml=0; \
            for(int _i=0;_i<SECCNT;_i++) if(s[_i].doff==map_old[_m]&&s[_i].rlen>_ml) _ml=s[_i].rlen; \
            if(map_old[_m]==0x1f0) _ml=64+BUILD_LEN; \
            if(_ov>=map_old[_m] && _ov<map_old[_m]+_ml){ _nv=map_new[_m]+(_ov-map_old[_m]); _done=1; break; } \
        } } \
        (OUTVAR)=_nv; \
    } while(0)

    #define RELOC_IF(OLD, OUTVAR, MATCHED) do { \
        uint32_t _ov=(OLD); uint32_t _nv=_ov; int _done=0; \
        if(_ov>=ph_old_base && _ov<ph_old_base+64){ _nv=ph_new_base+(_ov-ph_old_base); _done=1; } \
        if(!_done){ for(int _m=0;_m<nm;_m++){ \
            uint32_t _ml=0; \
            for(int _i=0;_i<SECCNT;_i++) if(s[_i].doff==map_old[_m]&&s[_i].rlen>_ml) _ml=s[_i].rlen; \
            if(map_old[_m]==0x1f0) _ml=64+BUILD_LEN; \
            if(_ov>=map_old[_m] && _ov<map_old[_m]+_ml){ _nv=map_new[_m]+(_ov-map_old[_m]); _done=1; break; } \
        } } \
        (OUTVAR)=_nv; (MATCHED)=_done; \
    } while(0)

    { int eof=-1; for(int i=0;i<SECCNT;i++) if(s[i].type==2){eof=i;break;}
      if(eof>=0){
          if(!g_build_key[0]){

              const char *p = getenv("XLATE_DRIVER_SO");
              /* Read the key from the running driver: a stale constant is rejected just as silently. */
              if(load_driver_key(p ? p : g_driver_so, g_build_key, BUILD_LEN)){
                  free(o); return -11;
              }
          }
          memcpy(o+s[eof].new_doff, g_build_key, BUILD_LEN);
      } }

    { int ph=-1; for(int i=0;i<SECCNT;i++) if(s[i].type==10){ph=i;break;}
      if(ph>=0){ memcpy(o+s[ph].new_doff, in+ph_old_base, 64); wr(o,s[ph].new_doff+0,rd(in,0x20)+(uint32_t)g_symcount_delta); } }

    for(int d=0; d<nd; d++){
        uint32_t od=doffs[d];
        if(od==0x1f0) continue;
        int owner=-1; uint32_t orl=0;
        for(int i=0;i<SECCNT;i++) if(s[i].doff==od && s[i].rlen>=orl){ orl=s[i].rlen; owner=i; }
        uint32_t nb=s[owner].new_doff;
        memcpy(o+nb, in+od, orl);

        if(s[owner].type==1){
            wr(o,nb+0,rd(in,0x20)+(uint32_t)g_symcount_delta);
            if(g_code_grow){
                uint32_t tab=rd(in,od+OBJ_TAB_OFF), cnt=rd(in,od+OBJ_TAB_CNT);
                uint32_t ins=rd(in,od+tab+1*4);
                memmove(o+nb+ins+CODE_GROW, o+nb+ins, orl-ins);
                memset(o+nb+ins, 0, CODE_GROW);

                for(uint32_t k=0;k<cnt;k++)
                    wr(o, nb+tab+k*OBJ_ENTSZ+4, rd(in, od+tab+k*OBJ_ENTSZ+4)+CODE_GROW);

                if(cnt < OBJ_TYPES){
                    uint32_t last=rd(o, nb+tab+(cnt-1)*OBJ_ENTSZ+4);
                    uint32_t need=(OBJ_TYPES-cnt)*OBJ_ENTSZ;
                    uint32_t end=tab+cnt*OBJ_ENTSZ;
                    uint32_t first=rd(o, nb+tab+4);
                    int clean=1;
                    for(uint32_t z=0; z<need; z++) if(o[nb+end+z]) { clean=0; break; }
                    if(clean && first>=end+need){
                        for(uint32_t t=cnt;t<OBJ_TYPES;t++){
                            uint32_t e=nb+tab+t*OBJ_ENTSZ;
                            wr(o,e+0,t); wr(o,e+4,last);
                            wr(o,e+8,0); wr(o,e+12,0); wr(o,e+16,0);
                        }
                        wr(o, nb+OBJ_TAB_CNT, OBJ_TYPES);
                    }
                }
            }
        }
        else if(s[owner].type==3){

            uint32_t cnt=s[owner].count;
            static const uint32_t PFOFF[6]={0x00,0x04,0x08,0x0c,0x10,0x14};
            for(uint32_t r=0;r<cnt;r++){
                uint32_t roff=SYM_BASE+r*SYM_STRIDE;
                if(roff+SYM_STRIDE > orl) break;
                for(int p=0;p<6;p++){
                    uint32_t fo=PFOFF[p];
                    uint32_t ov=rd(in, od+roff+fo);
                    if(!ov) continue;
                    int matched=0; uint32_t nv; RELOC_IF(ov,nv,matched);
                    if(matched) wr(o, nb+roff+fo, nv);
                }

                uint32_t aux=rd(in, od+roff+0x0c);
                if(aux && aux+8<=(uint32_t)len){
                    uint32_t naux; int ma=0; RELOC_IF(aux,naux,ma);
                    if(ma) for(int k=0;k<2;k++){
                        uint32_t av=rd(in, aux+k*4);
                        if(!av) continue;
                        int m=0; uint32_t nv; RELOC_IF(av,nv,m);
                        if(m) wr(o, naux+k*4, nv);
                    }
                }
            }

            uint32_t aux_start=SYM_BASE+cnt*SYM_STRIDE;
            for(uint32_t a=aux_start; a+32<=orl; a+=32){
                uint32_t bp=rd(in, od+a+0x00);
                if(rd(in, od+a+0x08)==0xffffffffu && bp){ uint32_t nv; RELOC(bp,nv); wr(o,nb+a+0x00,nv); }
            }
        }
        else if(s[owner].type==9){

            uint32_t p=rd(in,od+0x00); uint32_t nv; RELOC(p,nv); wr(o,nb+0x00,nv);
        }

    }

    for(int i=0;i<SECCNT;i++){
        uint32_t to=TABOFF+(uint32_t)i*ENTSZ;
        wr(o,to+0x00,s[i].type+1);
        wr(o,to+0x04,s[i].new_doff);
        wr(o,to+0x08,s[i].new_psz);
        wr(o,to+0x0c,s[i].count);
        wr(o,to+0x10,s[i].next);
        wr(o,to+0x14,0); wr(o,to+0x18,0); wr(o,to+0x1c,0);
    }

    wr(o,0x2c,g_driver_key);
    wr(o,0x24,0);
    wr(o,0x24,rotxor(o,newsize));

    *out=o; *outlen=(int)newsize;
    return 0;
    #undef RELOC
    #undef RELOC_IF
}

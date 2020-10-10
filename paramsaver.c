/****************************************************************************************

paramsaver component

V2 - uses dynamic array of structures and populates at runtime
There is now maximum limit of 1000 to pins that may be created
(limited by 3 digit number to pin ID 0 - 999)

ArcEye 2013 <arceyeATmgwareDOTcoDOTuk>

syntax 
loadusr -W paramsaver F(f)=NNN S(s)=NNN B(b)=NNN filename=xxxxx onstart=0/1 onexit=0/1 

NNN is number of IN and OUT pins to create for each data type

xxxxx is filename only, not ext or path, will default to /home/user/linuxcnc/param.sav

onstart will read from file at startup. default = 0
onexit will write to file at exit, default = 0 

Building paramsaver.c

compile with 'comp --install --userspace paramsaver.c'

*****************************************************************************************/
#include "rtapi.h"
#ifdef RTAPI
#include "rtapi_app.h"
#endif
#include "rtapi_string.h"
#include "rtapi_errno.h"
#include "hal.h"

#include <stdlib.h>
#include <stdio.h>    /* Standard input/output definitions for inituser()*/

#undef TRUE
#define TRUE (1)
#undef FALSE
#define FALSE (0)
#undef true
#define true (1)
#undef false
#define false (0)

static int comp_id;
static int float_pins = 0;
static int bit_pins = 0;
static int s32_pins = 0;
static int onstart = 0;
static int onexit = 0;

char filename[80];
char filepath[80];

typedef struct xstruct
    {
    hal_float_t *invalueF;
    hal_float_t *outvalueF;

    hal_s32_t *invalueS;
    hal_s32_t *outvalueS;

    hal_bit_t *invalueB;
    hal_bit_t *outvalueB;
    }XSTRUCT;

  

struct __comp_state 
    {
    hal_bit_t *readtrigger;
    hal_bit_t *writetrigger;
    XSTRUCT *structx[];
    };


static int __comp_get_data_size(void);



struct __comp_state *inst;

static int export(char *prefix, long extra_arg) 
{
char buf[HAL_NAME_LEN + 1];
int r = 0;
int j = 0;
XSTRUCT *st;

int pins = float_pins;
    if(s32_pins > pins)
        pins = s32_pins;
    if(bit_pins > pins)
        pins = bit_pins;
    if(pins > 1000)
        {
        fprintf(stderr,"pins limited to 1000 of each type");
        pins = 1000;
        }

int sz = sizeof(struct __comp_state) + (sizeof(struct xstruct) * pins)  + __comp_get_data_size();

    inst = hal_malloc(sz);

    memset(inst, 0, sz);

    for(j=0; j < pins; j++) 
        inst->structx[j] = (st = hal_malloc(sizeof(XSTRUCT )));
     
    if(float_pins)
        {
        for(j=0; j < (float_pins); j++) 
           {
           r = hal_pin_float_newf(HAL_IN, &(inst->structx[j]->invalueF), comp_id,"%s.invalueF-%03d", prefix, j);
           if(r != 0) 
              return r;
           r = hal_pin_float_newf(HAL_OUT, &(inst->structx[j]->outvalueF), comp_id,"%s.outvalueF-%03d", prefix, j);
           if(r != 0) 
              return r;
           }
        }   
   
    if(s32_pins)
        {
        for(j=0; j < (s32_pins); j++) 
            {
            r = hal_pin_s32_newf(HAL_IN, &(inst->structx[j]->invalueS), comp_id,"%s.invalueS-%03d", prefix, j);
            if(r != 0) 
                return r;
            r = hal_pin_s32_newf(HAL_OUT, &(inst->structx[j]->outvalueS), comp_id,"%s.outvalueS-%03d", prefix, j);
            if(r != 0) 
                return r;
            }
        }
    
    if(bit_pins)
        {
        for(j=0; j < (bit_pins); j++) 
            {
            r = hal_pin_bit_newf(HAL_IN, &(inst->structx[j]->invalueB), comp_id,"%s.invalueB-%03d", prefix, j);
            if(r != 0) 
                return r;
            r = hal_pin_bit_newf(HAL_OUT, &(inst->structx[j]->outvalueB), comp_id,"%s.outvalueB-%03d", prefix, j);
            if(r != 0) 
                return r;
            }
        }           
    
    r = hal_pin_bit_newf(HAL_IN, &(inst->readtrigger), comp_id,"%s.readtrigger", prefix);
    if(r != 0) 
        return r;
    *(inst->readtrigger) = 0;
    r = hal_pin_bit_newf(HAL_IN, &(inst->writetrigger), comp_id,"%s.writetrigger", prefix);
    if(r != 0) 
        return r;
    *(inst->writetrigger) = 0;
    
    return 0;
}

int rtapi_app_main(void) 
{
int r = 0;

    comp_id = hal_init("paramsaver");
    
    if(comp_id < 0) 
        return comp_id;
    
    r = export("paramsaver", 0);
    
    if(r) 
        hal_exit(comp_id);
    else 
        hal_ready(comp_id);
    return r;
}

void rtapi_app_exit(void) 
{
    hal_exit(comp_id);
}

static void user_mainloop(void);

static void userinit(int argc, char **argv);

int argc=0; char **argv=0;

int main(int argc_, char **argv_) 
{
    argc = argc_; argv = argv_;

    userinit(argc, argv);

    if(rtapi_app_main() < 0) 
        return 1;
        
    user_mainloop();
    rtapi_app_exit();
    
    return 0;
}

#undef readtrigger
#define readtrigger (0+*inst->readtrigger)
#undef writetrigger
#define writetrigger (0+*inst->writetrigger)

#include <stdint.h>   /* Standard types */
#include <string.h>   /* String function definitions */
#include <unistd.h>   /* UNIX standard function definitions */
#include <fcntl.h>    /* File control definitions */
#include <errno.h>    /* Error number definitions */
#include <signal.h>
#include <pwd.h>

int done = 0;

void writeout(void)
{
int i; 
FILE *fp2;

    fp2 = fopen(filepath, "w");
    if(fp2 != NULL)      
        {
        for(i = 0; i < float_pins; i++)
            fprintf(fp2, "%f\n", *(inst->structx[i]->invalueF));
       
        for(i = 0; i < s32_pins; i++)
            fprintf(fp2, "%d\n", *(inst->structx[i]->invalueS));
        
        for(i = 0; i < bit_pins; i++)
            fprintf(fp2, "%d\n", *(inst->structx[i]->invalueB));                                        
          
        fflush(fp2);
        fclose(fp2);
        }
    else
        fprintf(stderr,"Error: %s does not exist", filepath);
}


void adios(int sig) 
{
    if(onexit)
        writeout();

    done = 1; 
}

void userinit(int argc, char **argv)
{
char *str, *str2;
int i;

    bzero(filename, 80);
    
    if(argv[1] == NULL || argc < 2) 
        {
        fprintf(stderr,"paramsaver - Error: No pins specified\n");
        exit(0);
        }
        
    for (i=1; i < argc; i++) 
        {
        if( ((argv[i][0] == 'F') || (argv[i][0] == 'f')) && (argv[i][1] == '=') ) // ensure don't trap filename=
            float_pins = atoi(&argv[i][2]);
      
        else if( (argv[i][0] == 'S') || (argv[i][0] == 's') )
            s32_pins = atoi(&argv[i][2]);
                  
        else if( (argv[i][0] == 'B') || (argv[i][0] == 'b') )
            bit_pins = atoi(&argv[i][2]);
      
        else if( strncmp(argv[i], "onstart=", 8) == 0 )
            onstart = atoi(&argv[i][8]);
            
        else if( strncmp(argv[i], "onexit=", 7) == 0 )
            onexit = atoi(&argv[i][7]);

        else if( strncmp(argv[i], "filename=", 9) == 0 )
            strcpy(filename, &argv[i][9]);
        }
}


void user_mainloop(void)
{
char line[20];
float val1;
int val2, val3;
int read, write;
int i;

struct passwd *pw = getpwuid(getuid());
const char *homedir = pw->pw_dir;

    if(filename[0] != '\0')
        sprintf(filepath, "%s/linuxcnc/%s.sav", homedir, filename);
    else
        sprintf(filepath, "%s/linuxcnc/param.sav", homedir);

    read = write = 0;
    
    signal(SIGINT, adios);
    signal(SIGTERM, adios);
      
    while(!done)
        {
        usleep(250000);
        
        if(!readtrigger && read) read = 0;
        if(!writetrigger && write) write = 0;
        
        if((readtrigger && !read) || onstart)
            {
            FILE *fp1;
            fp1 = fopen(filepath, "r");
            if(fp1 != NULL)
                {
                for(i = 0; i < float_pins; i++)
                    {
                    fgets(line, sizeof(line), fp1);
                    if (sscanf(line, "%f", &val1) == 1)
                        *(inst->structx[i]->outvalueF) = val1;
                    }
                      
                for(i = 0; i < s32_pins; i++)
                    {
                    fgets(line, sizeof(line), fp1);                    
                    if(sscanf(line, "%d", &val2) == 1)
                        *(inst->structx[i]->outvalueS) = val2;
                    }
                  
                for(i = 0; i < bit_pins; i++)
                    {
                    fgets(line, sizeof(line), fp1);                    
                    if(sscanf(line, "%d", &val3) == 1)
                        *(inst->structx[i]->outvalueB) = val3;
                    }    
                fclose(fp1);
                read = 1; 
                onstart = 0; 
                }
            else
                {
                fprintf(stderr,"Error: %s does not exist\n", filepath);
                onstart = 0;
                }
            }
        else if(writetrigger && ! write)
            {
            writeout();
            write = 1;
            }           
        }
    exit(0);
}

static int __comp_get_data_size(void) { return 0; }

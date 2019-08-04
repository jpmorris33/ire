//
//      bitmap.h      The C versions of the bit-level routines
//

// Defines

//#define USE_ASM

#ifdef USE_ASM
    #define SetBit SetBit_ASM             // Use the ASM code
    #define ResetBit ResetBit_ASM             // Use the ASM code
    #define GetBit GetBit_ASM             // Use the ASM code
#else
    #define SetBit SetBit_C               // Use the C code
    #define ResetBit ResetBit_C               // Use the C code
    #define GetBit GetBit_C               // Use the C code
#endif

#ifdef __cplusplus
#define C_TYPE "C"
#else
#define C_TYPE
#endif

// variables

extern C_TYPE char *solid_map;            // The collision map
extern C_TYPE char *trigger_map;            // The collision map
extern C_TYPE int cmap_width;                 // Size of this map
extern C_TYPE int cmap_height;
extern C_TYPE int cmap_wxh;                   // Width x Height for speed

// Functions

extern C_TYPE void SetBit_C(int x,int y,char *map);
extern C_TYPE int GetBit_C(int x,int y,char *map);

extern C_TYPE void SetBit_ASM(int x,int y,char *map);
extern C_TYPE int GetBit_ASM(int x,int y,char *map);


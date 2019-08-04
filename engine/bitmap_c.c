//
//      bitmap_c.c      The C versions of the bit-level routines
//

// defines

// Variables

int cmap_width;                 // Size of this map
int cmap_height;
int cmap_wxh;                   // Width x Height for speed


// Functions

void SetBit_C(int x,int y,char *collision_map);
void ResetBit_C(int x,int y,char *collision_map);
int GetBit_C(int x,int y,char *collision_map);

// Code


/*
 *      SetBit_C - Set the appropriate bit in the collision map
 */

void SetBit_C(int x,int y,char *collision_map)
{
int bitmap_pos;
unsigned char bitmask;

bitmap_pos = (y*cmap_width) + (x>>3);
bitmask = 1<<((unsigned char)x&7);
collision_map[bitmap_pos] |= bitmask;
}

/*
 *      ResetBit_C - Reset the appropriate bit in the collision map
 */

void ResetBit_C(int x,int y,char *collision_map)
{
int bitmap_pos;
unsigned char bitmask;

bitmap_pos = (y*cmap_width) + (x>>3);
bitmask = (1<<((unsigned char)x&7))^0xff;
collision_map[bitmap_pos] &= bitmask;
}

/*
 *      GetBit_C - Get the appropriate bit in the collision map
 */

int GetBit_C(int x,int y,char *collision_map)
{
int bitmap_pos;
unsigned char bitmask;

bitmap_pos = (y*cmap_width) + (x>>3);
bitmask = 1<<((unsigned char)x&7);
return (collision_map[bitmap_pos] & bitmask);
}


/*
 *      A-Star algorithm by Martyn Smart
 */

#include "ithelib.h"
#include "media.hpp"
#include "init.hpp"
#include "core.hpp"
#include "gamedata.hpp"
#include "console.hpp"
#include "object.hpp"
#include "linklist.hpp"
#include "map.hpp"
#include "slotalloc.hpp"
#include <math.h>
#include <string.h>

// Defines

#define STACKSLOTS 512 // 256 was too small taking party members to the liche
#define NODESLOTS 1024

#define MAPSIZE 64
#define PF_BLANK 1.0
#define PF_WALL -1.0
#define PF_PERSON 10.0 // points to move around a person
#define PF_MOVEABLE 20.0 // points to move around a person

#define BACK_PROPAGATION // Better results but slower

// New vector heuristic tweak value
#define NVECT 0.041  // Cross multiple
#define DIAGONAL 1.4 // Cost for going diagonally, stops 'peeking in rooms'

#define TIMEOUT 512 // Cancel the pathfinding for null paths if it takes too long

// Variables

static IREBITMAP *imap;
static char imap_on=0;
static int vstartx,vstarty,do_diagonals;
static int Pwidth,Pheight;

// Prototypes

extern char show_imap;
extern int imapx,imapy;
extern char *BestName(OBJECT *o);
extern void gen_largemap();
extern void CentreMap(OBJECT *centre);
extern int OB_SetDir(OBJECT *objsel,int dir, int force);
extern int OB_TurnDir(OBJECT *objsel,int dir);
extern int MoveTurnObject(OBJECT *objsel,OBJECT *objdst,int dx, int dy);
extern OBJECT *GetRawSolidObject(int x,int y, OBJECT *except);
extern void CallVMnum(int v);
static int get_dir(OBJECT *o,int nx,int ny);
static int FindPath_main(OBJECT *start, OBJECT *end, int flags);

// Martyn's a-star pathfinder
static struct NODE *MakePath(int sx, int sy, int ex, int ey);
static struct NODE *ReturnBestNode(void);
static void GenerateSuccessors(struct NODE *BestNode, int ex, int ey, int ways);
static int FreeTile(int x, int y);
static void GenerateChild(struct NODE *BestNode, int x, int y, int ex, int ey, int diagonal);
static void PropagateDown(struct NODE *Old);
static void Add(struct NODE *Successor);
static void Push(struct NODE *Pusher);
static struct NODE *Pop(void);
static void CleanUp();
inline double vector_heuristic(int sx, int sy, int ex, int ey);
inline double basic_heuristic(int sx, int sy, int ex, int ey);


// Pathfinder data structures

struct NODE
	{
	NODE *parent;
	NODE *child[8];
	int x;
	int y;
	double f; // sum of g+h, theoretical total cost of the path
	double g; // 'goodness' - cost of getting from start to this node
	double h; // hypothetical cost to get from here to the finish
	NODE *next;
	} *OPEN, *CLOSED;

// List indices for faster searching
struct NODE *OpenIndex[MAPSIZE][MAPSIZE];
struct NODE *ClosedIndex[MAPSIZE][MAPSIZE];

struct STACKITEM
	{
	struct NODE *node;
	struct STACKITEM *next;
	}*STACK;

double pfmap[MAPSIZE][MAPSIZE];
double ideal_vector; // start to finish as the crow flies

static SLOTALLOC *StackSlots;
static SLOTALLOC *NodeSlots;

// and inline functions

#define cost(x,y) pfmap[x][y] // Cost of this tile
#define heuristic(sx,sy,ex,ey) vector_heuristic(sx,sy,ex,ey)
//#define heuristic(sx,sy,ex,ey) basic_heuristic(sx,sy,ex,ey)

#define zmax(a,b) ((a)>(b)?(a):(b))


void PathInit()
{
if(!StackSlots)
	StackSlots = new SLOTALLOC("Stackslots",sizeof(STACKITEM),STACKSLOTS);
if(!NodeSlots)
	NodeSlots = new SLOTALLOC("Nodeslots",sizeof(NODE),NODESLOTS);
}

void PathTerm()
{
if(StackSlots)
	delete StackSlots;
StackSlots=NULL;
if(NodeSlots)
	delete NodeSlots;
NodeSlots=NULL;
}


/*
 *      A-Star: top-level interface to the game engine
 *              Handles object direction, pathfinder timeouts and obstructions
 */

int FindPath(OBJECT *start, OBJECT *end, int diagonal)
{
int dx,dy,ret;
OBJECT *temp,*temp2;
int oldx,oldy,done,newdir;

// Something wicked this way comes
if(start->stats->radius>0)
	return PATH_BLOCKED; // Pathfinder won't work w/radius objs (shared variable)

// Centre around mover
CentreMap(start);
gen_largemap();

// Calculate a route to take.
ret=FindPath_main(start,end,diagonal);
if(ret == PATH_BLOCKED)
	{
	// Abort: move the window back over the player again
	CentreMap(player);
	gen_largemap();
	// Lost the target.. increment the timeout counter
	// This variable is also used by radius objects (eggs) hence the
	// check at the start of the function.  Eggs shouldn't be movable.
	start->user->counter++;
	// Let's go
	return ret;
	}

// Calculate new coordinates from the delta we got back
dx=start->x+start->user->dx;
dy=start->y+start->user->dy;

//if(!istricmp(start->personalname,"panther JMAC"))
//	printf("Move from %d,%d to %d,%d\n",start->x,start->y,dx,dy);

// Try to move, if we fail, call a script to signal failure and
// decide what to do about it with user-written code.
// This can open a door or whatever.

if(start->flags & IS_LARGE)
	{
	// Use the new mechanism, optimised for large objects
	done=MoveTurnObject(start,end,start->user->dx,start->user->dy);
	}
else
	{
	// Use the old mechanism, which works better for small objects
	newdir=get_dir(start,dx,dy); // Face direction you're heading

	// Previously we reset the animation before moving, but this
	// makes people stand up in their chairs if they can't move
	done=MoveObject(start,dx,dy,0);
	if(done && newdir >= 0)
		OB_SetDir(start,newdir,FORCE_FRAME);
	}

// Anyway, we tried to move.  Did it work?
if(!done)
	{
	temp = current_object;
	temp2 = victim;
	oldx = new_x;
	oldy = new_y;

	current_object = start;
	victim = end;
	new_x = dx;
	new_y = dy;

//	ilog_printf("%s calling trackstop at %d,%d\n",current_object->name,new_x,new_y);
	// No, run the callback
	CallVMnum(Sysfunc_trackstop);

	current_object = temp;
	victim = temp2;
	new_x = oldx;
	new_y = oldy;
	}

// Move the window back over the player again
CentreMap(player);
gen_largemap();

// It's possible to get there, so reset the timeout counter
start->user->counter=0;
return ret;
}


/*
 *      can_route - return the number of steps in the route, or -1
 */

int CanRoute(OBJECT *start, OBJECT *end, int diagonal)
{
int ret;

// Centre around mover
CentreMap(start);
gen_largemap();

// Calculate the path
ret=FindPath_main(start,end,diagonal|FP_FINDROUTE);

// Move the window back over the player again
CentreMap(player);
gen_largemap();
return ret;
}




/*
 *      Interface between game map and A-Star logic
 */

int FindPath_main(OBJECT *start, OBJECT *end, int flags)
{
int vsx,vsy,tx,ty,cx,cy,xctr,yctr;
int blockx,blocky,blockw,blockh;
OBJECT *temp;
NODE *p,*pstart;

// Set the size of the object that we're routing with
Pwidth = start->mw;
Pheight = start->mh;

// Make it square (helps routing)
if(Pwidth>Pheight)
	Pwidth=Pheight;
else
	Pheight=Pwidth;
//ilog_printf("Routing for square %d,%d\n",Pwidth,Pheight);

if(!imap_on)
	{
	imap = MakeIREBITMAP(MAPSIZE,MAPSIZE);
	imap_on = 1;
	}

// First, reset DX,DY to stop overshoot if we're already at destination
start->user->dx=0;
start->user->dy=0;

// If the two objects are on top of each other, return code 0 (found it)

// printf("st = %s, %d, %d, nd = %s T%d at %d,%d\n",start->name,start->x,start->y,end->name,end->tag,end->x,end->y);

if(start->x == end->x && start->y == end->y)
	return PATH_FINISHED;

// Centre the window around the START object

vsx = start->x-(MAPSIZE>>1);
if(vsx<0)
	vsx=0;
vsy = start->y-(MAPSIZE>>1);
if(vsy<0)
	vsy=0;

// Detect an out-of-bounds target and return abort (code 2)
if(end->x<=vsx || end->y<=vsy || end->x>=(vsx+MAPSIZE-1) || end->y>=(vsy+MAPSIZE-1))
	return PATH_BLOCKED;

// In pocket, move it outside
if(start->parent.objptr)
	{
	TransferObject(start,start->x,start->y);
	return PATH_WAITING; // Return code 1 (moved OK, but not yet found the object)
	}

// Now, fill in the actual map with details of the environment

for(cx=MAPSIZE-1;cx>0;cx--)
	{
	for(cy=MAPSIZE-1;cy>0;cy--)
		{
		tx = cx+vsx;
		ty = cy+vsy;

		// Get an initial opinion of the cost (and whether it's solid)
		pfmap[cx][cy] = (double)GetTileCost(tx,ty);

		// Get the solid object at this point, unless it is You
		temp = GetRawSolidObject(tx,ty,start);

		// If it is a living thing, it might move away.
		// But attach a high cost to it to avoid them if possible
		if(temp)
			if(temp->flags & IS_PERSON && temp->stats->hp > 0)
				{
				temp=NULL;
				pfmap[cx][cy] += PF_PERSON;	// Was '='
				}

		// If it can be pushed out of the way, add a high cost but assume it's doable.
		// This prevents the guards simply freezing if you barracade yourself in
		if(temp)
			if(!(temp->flags & IS_FIXED))
				{
				temp=NULL;
				pfmap[cx][cy] += PF_MOVEABLE;
				}

		// Check the objects.
		if(temp)
			{
			if(temp->flags & IS_LARGE)      // It's large, do lots of stuff
				{
				// First, generate the appropriate object shape
				if(temp->curdir>CHAR_D)
					{
					blockx=temp->hblock[BLK_X];
					blocky=temp->hblock[BLK_Y];
					blockw=temp->hblock[BLK_W];
					blockh=temp->hblock[BLK_H];
					}
				else
					{
					blockx=temp->vblock[BLK_X];
					blocky=temp->vblock[BLK_Y];
					blockw=temp->vblock[BLK_W];
					blockh=temp->vblock[BLK_H];
					}

				if(cx+blockw+blockx>MAPSIZE)
					blockw-=((cx+blockw+blockx)-MAPSIZE);
				if(cy+blockh+blocky>MAPSIZE)
					blockh-=((cy+blockh+blocky)-MAPSIZE);

				// Fill in the map where the solid parts of the shape are

				for(xctr=0;xctr<blockw;xctr++)
					for(yctr=0;yctr<blockh;yctr++)
						pfmap[cx+xctr+blockx][cy+yctr+blocky] = PF_WALL;

				// If it can be opened, punch out the Active Area

				if(temp->flags & CAN_OPEN && (!GetNPCFlag(start,NOT_OPEN_DOORS)))
					{
					if(temp->curdir>CHAR_D)
						{
						blockx=temp->harea[BLK_X];
						blocky=temp->harea[BLK_Y];
						blockw=temp->harea[BLK_W];
						blockh=temp->harea[BLK_H];
						}
					else
						{
						blockx=temp->varea[BLK_X];
						blocky=temp->varea[BLK_Y];
						blockw=temp->varea[BLK_W];
						blockh=temp->varea[BLK_H];
						}

					if(cx+blockw+blockx>MAPSIZE)
						blockw-=((cx+blockw+blockx)-MAPSIZE);
					if(cy+blockh+blocky>MAPSIZE)
						blockh-=((cy+blockh+blocky)-MAPSIZE);

					// Punch the holes

					for(xctr=0;xctr<blockw;xctr++)
						for(yctr=0;yctr<blockh;yctr++)
							pfmap[cx+xctr+blockx][cy+yctr+blocky] = PF_BLANK;
					}
				}
			else
				if(!(temp->flags & CAN_OPEN) || GetNPCFlag(start,NOT_OPEN_DOORS))
					{
					pfmap[cx][cy] = PF_WALL;    // It's just a small one
					}
			}
		}
	}

// Punch the dest and source out of the solidity map

pfmap[end->x-vsx][end->y-vsy] = PF_BLANK;
pfmap[start->x-vsx][start->y-vsy] = PF_BLANK;

// For debugging, draw the cute little map
if(show_imap && player->enemy.objptr == start)
	{
	for(cx=0;cx<MAPSIZE;cx++)
		for(cy=0;cy<MAPSIZE;cy++)
			if(pfmap[cx][cy]<0.0)
				imap->PutPixel(cx,cy,0,0,0);
			else
				imap->PutPixel(cx,cy,255,255,255);

	imap->PutPixel(start->x-vsx,start->y-vsy,0,0,255);
	imap->PutPixel(end->x-vsx,end->y-vsy,255,0,0);
//	stretch_sprite(swapscreen,imap,imapx,imapy,imap->w*2,imap->h*2);
	imap->DrawStretch(swapscreen,imapx,imapy,imap->GetW()*2,imap->GetH()*2);
	}

// Update large object window and request the path

// Set the diagonal flag
if(flags&FP_DIAGONAL)
	do_diagonals=1;
else
	do_diagonals=0;

// Build the path
p = MakePath(start->x-vsx,start->y-vsy,end->x-vsx,end->y-vsy);
pstart=p;

//
// Now we have actually calculated the path!
// What are we going to do with it?
//

// If only checking a route, not traversing one:

if(flags&FP_FINDROUTE)
	{
	// If it can't be done, return failure
	if(!p)
		{
		CleanUp();
		return PATH_BLOCKED;
		}

	if(!p->parent)
		{
		CleanUp();
		return PATH_BLOCKED;
		}

	cx=0;
	for(;p->parent->parent;p=p->parent) cx++; // count the steps in the path

	CleanUp();
	return cx;      // and return them
	}

//
// If we're calculating a route so an object can traverse it
//

// If it can't be done, return failure (code 2)
if(!p)
	{
	CleanUp();
	return PATH_BLOCKED;
	}

if(p->parent)
	{
	pstart=p;

	// Scan to the last node of the list for X,Y
	for(;p->parent->parent;p=p->parent); // printf("p = %p, parent = %p\n",p,p->parent);
	start->user->dx=(p->x+vsx)-start->x;        // Get delta x and y
	start->user->dy=(p->y+vsy)-start->y;
	}

CleanUp();
return PATH_WAITING; // Return code 1 (moved OK, but not yet found the object)
}

/*
 *      Make the object face the proper direction
 */

int get_dir(OBJECT *o,int nx,int ny)
{
// I don't think we want to force the rotation
//	OB_SetDir(o,CHAR_R,FORCE_FRAME);

if(o->x<nx)
	return CHAR_R;
if(o->x>nx)
	return CHAR_L;
if(o->y<ny)
	return CHAR_D;
if(o->y>ny)
	return CHAR_U;

return -1;
}


/**
 **
 **  Below this point is Martyn's A-star implementation
 **
 **/

// Build the path

struct NODE *MakePath(int sx, int sy, int ex, int ey)
{
struct NODE *Node, *BestNode;
int timeout;

// Calculate ideal vector (for original vector heuristic)
ideal_vector=(double)(sx-ex)/(double)(sy-ey);

// Store original starting point (for new vector heuristic)
vstartx = sx;
vstarty = sy;

// Build start node
Node = (struct NODE*)NodeSlots->Alloc();
Node->g = 0;
Node->h = heuristic(sx, sy, ex, ey);
Node->f = Node->h;
Node->x = sx;
Node->y = sy;
CLOSED = NULL;
OPEN = Node;

// Clear the Indexes and set the index for the first node
memset(OpenIndex, 0, sizeof(NODE*) * MAPSIZE * MAPSIZE);
memset(ClosedIndex, 0, sizeof(NODE*) * MAPSIZE * MAPSIZE);
OpenIndex[sx][sy] = Node;

timeout=0;
for(;;)
	{
	BestNode = ReturnBestNode();
	if(!BestNode) // Path must be impossible so stop
		break;
	if(BestNode->x == ex && BestNode->y == ey)	// We reached the end so stop
		break;
	if(do_diagonals)
		GenerateSuccessors(BestNode, ex, ey, 8);	// Check for successors to the 'best' node
	else
		GenerateSuccessors(BestNode, ex, ey, 4);	// Check for successors to the 'best' node
	timeout++;

	// Time out if it is taking too long.
	if(timeout>TIMEOUT)
		return NULL;
	}

// BestNode will either point to the end node or NULL if no path could be found
return BestNode;
}

// Find the best node and move it between the lists

struct NODE *ReturnBestNode(void)
{
struct NODE *temp;

// The 'best' node will always be at the start since we insert new nodes in the right order

temp = OPEN;
if(temp)
	{
	OPEN = OPEN->next;			// Take it off the OPEN list...
	OpenIndex[temp->x][temp->y] = NULL;

	temp->next = CLOSED;	// ...and put it on the CLOSED list
	CLOSED = temp;
	ClosedIndex[CLOSED->x][CLOSED->y] = CLOSED;
	}

return temp;
}

// Find a successor node

void GenerateSuccessors(struct NODE *BestNode, int ex, int ey, int ways)
{
// The first 4 are the cardinal points, the last 4 are the diagonals
// If we want a 4-way path, ways=4.  For diagonals, ways=8.
int xs4[] = { 0, 1, 0,-1, -1, 1, 1,-1};
int ys4[] = {-1, 0, 1, 0, -1,-1, 1, 1};

// This is the original 8-way only table
int xs8[] = {-1,  0,  1, 1, 1, 0, -1, -1};
int ys8[] = {-1, -1, -1, 0, 1, 1,  1,  0};
int i;
int x, y;
int *xs,*ys;

if(ways < 8)
	{
	xs=xs4;
	ys=ys4;
	ways=4;
	}
else
	{
	xs=xs8;
	ys=ys8;
	ways=8;
	}

// Look at the surrounding squares and generate the node if necessary
for(i = 0; i < ways; i++)
	{
	x = BestNode->x + xs[i];
	y = BestNode->y + ys[i];
	if(FreeTile(x, y))
		{
		if(xs[i] != 0 && ys[i] != 0)
			{
			// We are trying to go diagonally, double the cost to dissuade it
			GenerateChild(BestNode, x, y, ex, ey, 1);
			}
		else
			{
			// Go normally
			GenerateChild(BestNode, x, y, ex, ey, 0);
			}
		}
	}
}

// Is the given square usable?

int FreeTile(int x, int y)
{
static int w,h;
// Always return false if going off the map
if(x < 0 || y < 0 || x >= MAPSIZE || y >= MAPSIZE)
	return 0;

// If it's a large object, we're going to want extra checks
if(Pwidth != 1 || Pheight!= 1)
	{
	if(x+Pwidth >= MAPSIZE)
		return 0;
	if(y+Pheight >= MAPSIZE)
		return 0;

	// Now scan object's area
	for(h=0;h<Pheight;h++)
		for(w=0;w<Pwidth;w++)
			if(pfmap[x+w][y+h] == PF_WALL)
				return 0;
	return 1;
	}

// If whats at x,y is a wall then it will return false
return (pfmap[x][y] != PF_WALL);
}

// Find a child node

void GenerateChild(struct NODE *BestNode, int x, int y, int ex, int ey, int diagonal)
{
double g;
int c;
struct NODE *Old, *Successor;

if(diagonal)
	g = BestNode->g + (cost(x,y)*DIAGONAL);
else
	g = BestNode->g + cost(x,y);

if((Old = OpenIndex[x][y]) != NULL)
	{
	// We found this node already on the OPEN list
	// Make the existing node our child
	for(c = 0; c < 8; c++)
		if(!BestNode->child[c])
			{
			BestNode->child[c] = Old;
			break;
			}

	// If the new path is better reset the old node
	if(g < Old->g)
		{
		Old->parent = BestNode;
		Old->g = g;
		Old->f = g + Old->h;
		}
	}
else
	if((Old = ClosedIndex[x][y]) != NULL)
		{
		// We found this node already on the CLOSED list
		// Make the existing node our child
		for(c = 0; c < 8; c++)
			if(!BestNode->child[c])
				{
				BestNode->child[c] = Old;
				break;
				}

#ifdef BACK_PROPAGATION
		// If the new path is better reset the old node
		if(g < Old->g)
			{
			Old->parent = BestNode;
			Old->g = g;
			Old->f = g + Old->h;
			PropagateDown(Old);	// Propagate this path change down
			}
#endif
		}
	else
		{
		// This is a new node so put it on the OPEN list

		// Set up the new node
		Successor = (struct NODE *)NodeSlots->Alloc();
		Successor->parent = BestNode;
		Successor->g = g;
		Successor->h = heuristic(x, y, ex, ey);
		Successor->f = g + Successor->h;
		Successor->x = x;
		Successor->y = y;
		Add(Successor);		// Put the successor on the OPEN list
		for(c = 0; c < 8; c++)
			if(!BestNode->child[c])
				{
				BestNode->child[c] = Successor;
				break;
				}
		}
}

// Back-propagation of the path

void PropagateDown(struct NODE *Old)
{
int c;
double g, newg;
struct NODE *Child, *Parent;

g = Old->g;
for(c = 0; c < 8; c++)
	{
	if(!(Child = Old->child[c]))	// Alias for faster access
		break;

	newg = g + cost(Child->x, Child->y);	// Calculate new cost
	if(newg < Child->g)
		{
		// If this is a better way then change it
		Child->g = newg;
		Child->f = Child->g + Child->h;
		Child->parent = Old;
			Push(Child);					// We must save this node for further propagation
		}
	}

while(STACK)
	{
	Parent = Pop();
	for(c = 0; c < 8; c++)
		{
		if(!(Child = Parent->child[c]))	// Alias for faster access
			break;

		newg = Parent->g + cost(Child->x, Child->y);	// Calculate new cost
		if(newg < Child->g)
			{
			// If this is a better way then change it
			Child->g = newg;
			Child->f = Child->g + Child->h;
			Child->parent = Parent;
			Push(Child);					// We must save this node for further propagation
			}
		}
	}
}

// Add a node to the Open list

void Add(struct NODE *Successor)
{
struct NODE *Prev, *Current;
double f;

// Add the index to this Node
OpenIndex[Successor->x][Successor->y] = Successor;

if(!OPEN)
	{
	OPEN = Successor;
		return;
	}

Prev = OPEN;
Current = OPEN->next;
f = Successor->f;

while(Current && Current->f < f)
	{
	Prev = Current;
	Current = Current->next;
	}

Successor->next = Current;
Prev->next = Successor;
}

// Push a node onto the stack

void Push(struct NODE *pusher)
{
//static int pushes;
//printf("%d pushes\n",++pushes);

struct STACKITEM *temp;

temp = (struct STACKITEM *)StackSlots->Alloc();
temp->node = pusher;
if(STACK)
	temp->next = STACK->next;
STACK = temp;
}

// Take a node off the stack

struct NODE *Pop(void)
{
//static int pops;
//printf("%d pops\n",++pops);

struct STACKITEM *tempStack;
struct NODE *tempNode;

tempNode = STACK->node;
tempStack = STACK;
STACK = STACK->next;
//free(tempStack);
StackSlots->Free(tempStack);

return tempNode;
}

// Tidy up afterwards

void CleanUp()
{
/*
NODE *path;
path = OPEN;
while(path)
	{
	OPEN = OPEN->next;
	free(path);
	path = OPEN;
	}

path = CLOSED;
while(path)
	{
	CLOSED = CLOSED->next;
	free(path);
	path = CLOSED;
	}
*/
NodeSlots->Purge();
StackSlots->Purge();
}


// Martyn's new vector cross-product heuristic

inline double vector_heuristic(int sx, int sy, int ex, int ey)
{
double h,dx1,dy1,dx2,dy2,cross;

// Compute Manhattan distance and modify based on vector
dx1 = sx - ex;    // (dx1, dy1) is the vector from current node to goal
dy1 = sy - ey; 
dx2 = vstartx - ex;      // (dx2, dy2) is the vector from start node to goal
dy2 = vstarty - ey;

cross = dx1*dy2 - dx2*dy1;     // This is the cross product of the two vectors
if( cross<0 ) cross = -cross;  // Make sure product is positive

cross *= NVECT;

h = zmax(abs(sx - ex), abs(sy - ey));
h += cross;

return h;
}

// Bog-standard heuristic

inline double basic_heuristic(int sx, int sy, int ex, int ey)
{
return sqrt((double)(sx - ex) * (sx - ex) + (sy - ey) * (sy - ey));
}


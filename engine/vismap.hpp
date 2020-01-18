
#define VISMAP_IS_LIT		1
#define VISMAP_IS_DONE		2
#define VISMAP_IS_NOSPRITE	4


/*
 *      Remove rooms you can't see from the view (like U6)
 */

class VisMap {

	public:
		VisMap();
		void calculate();
		void calculateSimple();
		void project(IREBITMAP *dest);
		int isBlanked(int x, int y);
		int isSpriteBlanked(int x, int y);
		int getNoSpriteDirect(int x, int y);
		char getBlMap(int x, int y);
		char get(int x, int y);
		int getAltCode(int x, int y);
		void checkSprites(int x, int y);
		void clearBlMap();

	private:
		char vismap[LMSIZE][LMSIZE]; // visibility state
		char blmap[LMSIZE][LMSIZE];  // blocks light
		void punch(int x, int y);
		void floodfill(int x, int y);

};


#define MASK_MAXWIDTH 9
#define MASK_MAXHEIGHT 9

/*
 *      Remove rooms you can't see from the view (like U6)
 */

class LightMask {

	public:
		LightMask();
		~LightMask();
		void init();
		void calculate(int x, int y);
		void projectLight(int x, int y, IRELIGHTMAP *dest);
		void projectDark(int x, int y, IRELIGHTMAP *dest);

	private:
		IRELIGHTMAP *lightmap;
		IRELIGHTMAP *tile[MASK_MAXWIDTH][MASK_MAXHEIGHT];
		int w,h;
		int w2,h2; // Half size
		int xoffset,yoffset;
		int maskx,masky;
		char map[MASK_MAXWIDTH][MASK_MAXHEIGHT];
		void floodfill(int x, int y);
		int blocked(int x, int y);
		void maskLeft(int x, int y);
		void maskUp(int x, int y);

};

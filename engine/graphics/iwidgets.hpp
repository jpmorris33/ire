//

#ifndef _IRE_IREWIDGETS
#define _IRE_IREWIDGETS

#include "iregraph.hpp"
#include "iconsole.hpp"

class IREPICKLIST
	{
	public:
		IREPICKLIST();
		~IREPICKLIST();
		int Init(int x, int y, int w, int h, const char **list, int listlen);
		void Process(int key);
		void Update();
		int GetItem();
		int GetItem(char *output, int max);
		int SetItem(const char *string);
		int GetPos();
		int SetPos(int line);
		int GetPercent();
		int SetPercent(int percent);
		int SetColour(IRECOLOUR *col);
	
		const char *GetSelectText();
		void AddLine(const char *text);


	private:
		IRECONSOLE *win;
		int running,X,Y,W,H;
		int linesheight;
		int highlightposition;
	};


class IRETEXTEDIT
	{
	public:
		IRETEXTEDIT();
		~IRETEXTEDIT();
		void SetColour(IRECOLOUR *colour);
		void SetFont(IREFONT *font);
		int InitText(int x, int y, int w, int h, int maxchars);
		int InitNumber(int x, int y, int w, int h, int minnumber, int maxnumber);
		void Process(int key, int ascii);
		int GetNumber();
		const char *GetText();

	private:
		void Redraw();
		IREBITMAP *backing;
		IREFONT *font;
		IRECOLOUR col;
		IRECOLOUR invcol;
		int running,X,Y,W,H;
		int fontheight;
		int cursor,maxchars,offset,cx;
		int min,max;
		char *string;
	};

	
	
#endif
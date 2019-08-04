#ifndef __IRECONSOLE
#define __IRECONSOLE

struct CONSOLELINE
	{
	IRECOLOUR colour;
	IREFONT *font;
	char text[256];
	CONSOLELINE *next,*prev;
	};

class IRECONSOLE
	{
	public:
		IRECONSOLE();
		~IRECONSOLE();

		int Init(int x, int y, int cols, int rows, int maxlines);

		void Clear();
		void Print(const char *msg);
		void Printf(const char *fmt, ...);
		void AddLine(const char *msg);
		void Newline();
		void ClearLine();
		void SetCol(int r, int g, int b);
		void SetCol(unsigned int packed);
		void SetCol(IRECOLOUR col);
		void SetHighlight(int r, int g, int b);
		void SetHighlight(unsigned int packed);
		void SetHighlight(IRECOLOUR col);
		unsigned int GetPackedCol();
		unsigned int GetPackedHighlight();
		void Update();
		void FastUpdate();

		int GetPercent();
		int SetPercent(int percent);
		int GetPos();
		int SetPos(int lineno);
		int ScrollRel(int direction);
		int SelectLine(int lineno);
		int SelectText(const char *string);
		int GetSelect();
		int GetRows();
		int GetLines();
		const char *GetSelectText();


	private:
		int Add();
		void PrintfCore(const char *msg);
		int running;
		IREBITMAP *backing;
		IREFONT *font;
		IRECOLOUR col;
		IRECOLOUR highlight;
		IRECOLOUR inverse;
		int Cols,Rows,W,H,FW,FH;
		int LineMax;
		int X,Y;
		CONSOLELINE *linehead, *curline, *topline,*Select;
		int lines,linelen,conpos;
	};

#endif

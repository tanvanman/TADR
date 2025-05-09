// ElementHandler.h: interface for the CElementHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ELEMENTHANDLER_H__7B55B501_0A7A_11D5_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_ELEMENTHANDLER_H__7B55B501_0A7A_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Windows.h>
#include <list>
#include <set>
#include <vector>

#define ClassGraphicLine 1
#define ClassGraphicMarker 2
#define ClassGraphicText 3
#define ClassDeleteMarker 4
#define ClassTextMoved 5
#define ClassTextEdited 6

class GraphicElement
{
public:
	
	int x1;
	int y1;
	int Type;
	char Color;
	LONG ID;
	GraphicElement(int x, int y, int T, char C, LONG id) :x1(x), y1(y), Type(T), Color(C), ID(id) {};
	GraphicElement(int x, int y, int T, char C)
		:x1(x), y1(y), Type(T), Color(C)
	{
		ID = InterlockedIncrement(&ObjectCount);
	};
	virtual ~GraphicElement() {};
	void virtual Draw() {};
private:
	static LONG ObjectCount;
};

class GraphicLine : public GraphicElement
{
public:
	GraphicLine(int x1,int y1,int x2,int y2, char cC):GraphicElement(x1,y1,ClassGraphicLine,cC),x2(x2),y2(y2){};
	void virtual Draw() {};
	int x2,y2;
};

class GraphicMarker : public GraphicElement
{
public:
	GraphicMarker(int x,int y, char cC):GraphicElement(x,y,ClassGraphicMarker,cC){};
	void virtual Draw() {};
};

class GraphicText : public GraphicElement
{
public:
	GraphicText(int x,int y,char* intext,char cC)
		:GraphicElement(x,y,ClassGraphicText,cC)
		{
			size_t temp_Size= strnlen(intext, 50)+1;
			text=new char[temp_Size];
            memcpy(text, intext, temp_Size);
            text[temp_Size - 1] = 0;
		}
	void virtual Draw() {};
	char* text;
	virtual ~GraphicText(){delete[] text;};
};

//----------------------------------------------------------
//used for packet handler
class DeleteGraphic : public GraphicElement
{
public:
	DeleteGraphic(int x,int y, char Type):GraphicElement(x,y,ClassDeleteMarker,Type){};
};

class GraphicMoveText : public GraphicElement
{
public:
	GraphicMoveText(int x1, int y1, int x2, int y2, char cC, LONG id)
		:GraphicElement(x1, y1, ClassTextMoved, cC, id), x2(x2), y2(y2) { };
	int x2, y2;
};

class GraphicTextEdited : public GraphicElement
{
public:
	GraphicTextEdited(int x,int y,char* intext,char cC)
		:GraphicElement(x,y,ClassTextEdited,cC)
		{
			size_t temp_Size= strlen(intext)+1;
			text=new char[temp_Size];
			strcpy_s(text,temp_Size, intext);
		}
	char* text;
	virtual ~GraphicTextEdited(){delete[] text;};
};
//----------------------------------------------------------

// Largest map (Seven Islands) will have coordinates up to ~20000.
// We require ELEMENT_HASH_SIZE * 2^HASH_SQUARE_SIZE > 20,000
#define ELEMENT_HASH_SIZE 256		// should be a power of 2!
#define HASH_SQUARE_SIZE 8			// a bitshift, not a divide!

class CElementHandler  
{
public:
	GraphicElement* GetClosestElement(int x,int y);
	std::vector<GraphicElement*> GetArea(int x1,int y1,int x2,int y2);
	void DeleteBetween(int x1,int y1,int x2,int y2);
	void DeleteOn(int x,int y);
	void AddElement(GraphicElement* ge);
    bool IsElement(GraphicElement* ge);
    GraphicElement* MoveTextElement(GraphicElement* GE, int x, int y);
	CElementHandler();
	virtual ~CElementHandler();

private:
	typedef std::list<GraphicElement*> ElementList;

	ElementList map[ELEMENT_HASH_SIZE][ELEMENT_HASH_SIZE];
    std::set<GraphicElement*> allElements;
};

#endif // !defined(AFX_ELEMENTHANDLER_H__7B55B501_0A7A_11D5_AD55_0080ADA84DE3__INCLUDED_)

// ElementHandler.cpp: implementation of the CElementHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "ElementHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define OutptTxt IDDrawSurface::OutptTxt

LONG GraphicElement::ObjectCount = 0;

CElementHandler::CElementHandler()
{

}

CElementHandler::~CElementHandler()
{
//	GraphicElement * Curt_GElemPtr;


	allElements.clear();
	for(int y=0;y<ELEMENT_HASH_SIZE;y++)
		for(int x=0;x<ELEMENT_HASH_SIZE;x++)
		{
			//while(!map[y][x].empty())
			//{
				for (ElementList::iterator iter = map[y][x].begin(); iter != map[y][x].end () ; )
				{
					//map[y][x].pop_back();
					//delete Curt_GElemPtr;
					delete (*iter);
					iter = map[y][x].erase ( iter);
				}
				//Curt_GElemPtr= map[y][x].back();
				//delete ;

		//	}	
/*
			if (NULL== *map[y][x].end ( ))
			{
				OutptTxt  ( "WARNING 1! NULL in map[y][x]");
			}
			if (map[y][x].end ( )!= map[y][x].begin())
			{
				OutptTxt  ( "WARNING 2! end!=begin after empty!");
			}*/
			//map[y][x].clear ( );
			//map[y][x]._Nextnode ( map[y][x]._Myhead)= this->_Myhead;
			//assign ( map[y][x].begin (), map[y][x].begin ( ) );
		}


	//map->
}

void CElementHandler::AddElement(GraphicElement *ge)
{
	int x=(ge->x1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int y=(ge->y1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);

	map[y][x].push_back(ge);
    allElements.insert(ge);
}

bool CElementHandler::IsElement(GraphicElement* ge)
{
    return ge && (allElements.count(ge) > 0);
}

void CElementHandler::DeleteOn(int x, int y)
{
	DeleteBetween(x-10,y-10,x+10,y+10);
}

void CElementHandler::DeleteBetween(int x1, int y1, int x2, int y2)
{
	int sx1=(x1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sy1=(y1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sx2=(x2>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sy2=(y2>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);

	if(sx1>sx2){
		int tmp=sx1;
		sx1=sx2;
		sx2=tmp;
	}
	if(sy1>sy2){
		int tmp=sy1;
		sy1=sy2;
		sy2=tmp;
	}
	std::vector<GraphicElement*> toBeDeleted;

	for(int y=sy1;y<=sy2;y++)
		for(int x=sx1;x<=sx2;x++){
			ElementList::iterator eli;
			for(eli=map[y][x].begin();eli!=map[y][x].end();++eli){
				if(((*eli)->x1 >= x1) && ((*eli)->y1 >= y1))
					toBeDeleted.push_back(*eli);
			}
			while(!toBeDeleted.empty()){
				delete toBeDeleted.back();
				map[y][x].remove(toBeDeleted.back());
                allElements.erase(toBeDeleted.back());
				toBeDeleted.pop_back();
			}
		}
}

std::vector<GraphicElement*> CElementHandler::GetArea(int x1, int y1, int x2, int y2)
{
	int sx1=(x1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sy1=(y1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sx2=(x2>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
	int sy2=(y2>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);

	if(sx1>sx2){
		int tmp=sx1;
		sx1=sx2;
		sx2=tmp;
	}
	if(sy1>sy2){
		int tmp=sy1;
		sy1=sy2;
		sy2=tmp;
	}
	std::vector<GraphicElement*> r;
	if (sy1<0)
	{
		sy1= 0;
	}
	if (ELEMENT_HASH_SIZE<sy1)
	{
		sy1= ELEMENT_HASH_SIZE- 1;
	}

	if (sx1<0)
	{
		sx1= 0;
	}
	if (ELEMENT_HASH_SIZE<sx1)
	{
		sx1= ELEMENT_HASH_SIZE- 1;
	}

	if (sy2<0)
	{
		sy2= 0;
	}
	if (ELEMENT_HASH_SIZE<sy2)
	{
		sy2= ELEMENT_HASH_SIZE- 1;
	}

	if (sx2<0)
	{
		sx2= 0;
	}
	if (ELEMENT_HASH_SIZE<sx2)
	{
		sx2= ELEMENT_HASH_SIZE- 1;
	}


	try
	{
		for(int y=sy1;y<=sy2;y++)
		for(int x=sx1;x<=sx2;x++){
			ElementList::iterator eli;
			for(eli=map[y][x].begin();eli!=map[y][x].end();++eli)
			{// weird eli maybe invalid ptr
				if((((*eli)->x1 >= x1) && ((*eli)->x1 <= x2) && ((*eli)->y1 >= y1) && ((*eli)->y1 <= y2)))
					r.push_back(*eli);
			}
		}
		
	}
	catch(...)
	{
		;
	}

	return r;
}

GraphicElement* CElementHandler::GetClosestElement(int x, int y)
{

	return 0;
}

GraphicElement* CElementHandler::MoveTextElement(GraphicElement* GE, int x, int y)
{
	try
	{
		//add a new identical element then remove the old
        GraphicText *ge = (GraphicText*)GE;
		GraphicText *Nge = new GraphicText(x, y, ge->text, ge->Color);
		Nge->ID = ge->ID;

		int mx=(ge->x1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
		int my=(ge->y1>>HASH_SQUARE_SIZE) & (ELEMENT_HASH_SIZE-1);
		map[my][mx].remove(ge);
        allElements.erase(ge);
        if (ge) {
            delete ge;
        }

		AddElement(Nge);

		  //returns the new element
		return Nge;
	}
	catch(...)
	{
		return NULL;
	}
}


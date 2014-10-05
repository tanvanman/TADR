// MinimapHandler.h: interface for the CMinimapHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MINIMAPHANDLER_H__27BF2E5E_DFC8_4FCA_BF6E_EAF57A0FA5CD__INCLUDED_)
#define AFX_MINIMAPHANDLER_H__27BF2E5E_DFC8_4FCA_BF6E_EAF57A0FA5CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tamem.h"
#include "IRenderer.h"

class CMinimapHandler  
{
public:
	CMinimapHandler();
	virtual ~CMinimapHandler();
	void InitMinimap(SharedMem *DDDSharedMem);
	void DeinitMinimap();
	void FrameUpdate();
private:
	TAdynmemStruct *TAdynmem;
	SharedMem *DDDSharedMem;
	HANDLE MemMap;
	void *RadarPic;
};

#endif // !defined(AFX_MINIMAPHANDLER_H__27BF2E5E_DFC8_4FCA_BF6E_EAF57A0FA5CD__INCLUDED_)

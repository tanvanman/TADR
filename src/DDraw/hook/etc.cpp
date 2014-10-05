#include "..\oddraw.h"
#include "etc.h"

//禁止掉格式转化的warn提示，这个的原因是确定了转化没问题的:
#pragma warning(disable:4311)
#pragma warning(disable:4312)
extern "C"
{

	void *
		memfind(const void  *in_block,      /*  Block containing data            */
		size_t       block_size,    /*  Size of block in bytes           */
		const void  *in_pattern,    /*  Pattern to search for            */
		size_t       pattern_size,  /*  Size of pattern block            */
		size_t      *shift,         /*  Shift table (search buffer)      */
		BOOL        *repeat_find)   /*  TRUE: search buffer already init */
	{
		size_t
			byte_nbr,                       /*  Distance through block           */
			match_size;                     /*  Size of matched part             */
		const byte
			*match_base = NULL,             /*  Base of match of pattern         */
			*match_ptr  = NULL,             /*  Point within current match       */
			*limit      = NULL;             /*  Last potiental match point       */
		const byte
			*block   = (byte *) in_block,   /*  Concrete pointer to block data   */
			*pattern = (byte *) in_pattern; /*  Concrete pointer to search value */

		//ASSERT (block);                     /*  Expect non-NULL pointers, but    */
		//ASSERT (pattern);                   /*  fail gracefully if not debugging */
		/*  NULL repeat_find => is false     */
		if (block == NULL || pattern == NULL)
			return (NULL);
		static size_t shift_defaultBuffer[256];
		if (shift == NULL)
		{
			shift= shift_defaultBuffer;
		}
		/*  Pattern must be smaller or equal in size to string                   */
		if (block_size < pattern_size)
			return (NULL);                  /*  Otherwise it's not found         */

		if (pattern_size == 0)              /*  Empty patterns match at start    */
			return ((void *)block);

		/*  Build the shift table unless we're continuing a previous search      */

		/*  The shift table determines how far to shift before trying to match   */
		/*  again, if a match at this point fails.  If the byte after where the  */
		/*  end of our pattern falls is not in our pattern, then we start to     */
		/*  match again after that byte; otherwise we line up the last occurence */
		/*  of that byte in our pattern under that byte, and try match again.    */

		if (!repeat_find || !*repeat_find)
		{
			for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
				shift [byte_nbr] = pattern_size + 1;
			for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
				shift [(byte) pattern [byte_nbr]] = pattern_size - byte_nbr;

			if (repeat_find)
				*repeat_find = TRUE;
		}

		/*  Search for the block, each time jumping up by the amount             */
		/*  computed in the shift table                                          */

		limit = block + (block_size - pattern_size + 1);
//		ASSERT (limit > block);

		for (match_base = block;
			match_base < limit;
			match_base += shift [*(match_base + pattern_size)])
		{
			match_ptr  = match_base;
			match_size = 0;

			/*  Compare pattern until it all matches, or we find a difference    */
			while (*match_ptr++ == pattern [match_size++])
			{
//				ASSERT (match_size <= pattern_size &&
//					match_ptr == (match_base + match_size));

				/*  If we found a match, return the start address                */
				if (match_size >= pattern_size)
					return ((void*)(match_base));

			}
		}
		return (NULL);                      /*  Found nothing                    */
	}


	int X86ShallToRedirect (PBYTE pbCode, LPVOID OrgOpcodeVaddr, PDWORD AddrRtn_Pdw, LPBYTE OrgOpcodeStart, DWORD LenOfOrgOpcode_Dw)
	{//-1代表jcc, 0代表不需要处理这个跳转，1代表call; 非0的返回值代表AddrRtn_Pdw中已经被填充了跳转往的地址。
		int Rtn_I= 0;
		DWORD tempForTargat= 0;
		DWORD PtrToOrgCodeStart= reinterpret_cast<DWORD>(OrgOpcodeStart);
		DWORD PtrToOrgCodeEnd= PtrToOrgCodeStart+ LenOfOrgOpcode_Dw;
		switch (pbCode[0])
		{
			//x86的jmp和call归类	
			//jmp:
			// 8bits的偏移
			//0x7? 
			//0xe3
			//0xeb

			// 32bits的偏移
			//0xe9
			//0x0f 8?

			//call:
			//32bits
			//0xe8

			//jcc里的 0x0f 8?和0x7?的cc(flag condition)是一一对应的。
		case 0x0f:
			if ((0x7f<pbCode[1])&&(pbCode[1]<0x90))
			{
				tempForTargat= *((PDWORD)&pbCode[2])+ (DWORD)OrgOpcodeVaddr+ 6;
				if ((tempForTargat<PtrToOrgCodeStart)||(tempForTargat>=PtrToOrgCodeEnd))
				{
					*AddrRtn_Pdw= tempForTargat;
					Rtn_I= -1;
				}
			}
			break;
		case 0xe9:
			tempForTargat= *((PDWORD)&pbCode[1])+ (DWORD)OrgOpcodeVaddr+ 5;

			if ((tempForTargat<PtrToOrgCodeStart)||(tempForTargat>=PtrToOrgCodeEnd))
			{
				*AddrRtn_Pdw= tempForTargat;
				Rtn_I= -1;
			}
			break;
		case 0xe8:
			tempForTargat= *((PDWORD)&pbCode[1])+ (DWORD)OrgOpcodeVaddr+ 5;

			if ((tempForTargat<PtrToOrgCodeStart)||(tempForTargat>=PtrToOrgCodeEnd))
			{
				*AddrRtn_Pdw= tempForTargat;
				Rtn_I= 1;
			}
			break;
		case 0xe3:
		case 0xeb:
			tempForTargat= *((PBYTE)&pbCode[1])+ (DWORD)OrgOpcodeVaddr+ 2;

			if ((tempForTargat<PtrToOrgCodeStart)||(tempForTargat>=PtrToOrgCodeEnd))
			{
				*AddrRtn_Pdw= tempForTargat;
				Rtn_I= -1;
			}
			break;
		default:
			//在default里也处理 0x7?的情况。
			if ((0x6f<pbCode[0])&&(pbCode[0]<0x80))
			{
				tempForTargat= *((PBYTE)&pbCode[1])+ (DWORD)OrgOpcodeVaddr+ 2;

				if ((tempForTargat<PtrToOrgCodeStart)||(tempForTargat>=PtrToOrgCodeEnd))
				{
					*AddrRtn_Pdw= tempForTargat;
					Rtn_I= -1;
				}
			}
		}
		return Rtn_I;
	}

	DWORD X86RedirectOpcodeToNewBase (LPVOID NewBase, LPBYTE OrgOpcode, DWORD * LenOfOpcode_Dw, LPBYTE * Rtn_PPb)
	{//把一段opcode中的jcc和call受代码地址影响的code都处理成新地址的。返回的Rtn_PPb是需要用delete []删除的。
		DWORD ValidOpcodeLen= GetMinValidLenWithMatchOpcode ( OrgOpcode, *LenOfOpcode_Dw);
		DWORD tempOpcodeOff_Dw= 0;

		DWORD PureJmpAddr_Dw= 0;
		LPBYTE RtnBuffer= new BYTE[*LenOfOpcode_Dw* 4];//全部都是jecxz也最多只会扩大4倍。
		LPBYTE PtrIn_RtnBuffer= RtnBuffer;
		int tempForOpcodeLen= 0;

		int diffFromNewBaseAndRtnBuffer_I= (int)(RtnBuffer- (LPBYTE)NewBase);
		while (tempOpcodeOff_Dw<ValidOpcodeLen)
		{
			tempForOpcodeLen= GetOpCodeSize ( OrgOpcode+ tempOpcodeOff_Dw);
			switch (X86ShallToRedirect ( OrgOpcode+ tempOpcodeOff_Dw, OrgOpcode+ tempOpcodeOff_Dw, &PureJmpAddr_Dw, OrgOpcode, ValidOpcodeLen))
			{
			case -1:
				//jcc的需要根据不同opcode来处理:
				switch ((OrgOpcode+ tempOpcodeOff_Dw)[0])
				{
				case 0x0f:
					memcpy ( PtrIn_RtnBuffer, OrgOpcode+ tempOpcodeOff_Dw, 2);
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 2;
					*((PDWORD)(PtrIn_RtnBuffer))= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 2- diffFromNewBaseAndRtnBuffer_I)- 6;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
					break;

				case 0xe3:
					//这个很特殊，判断ecx的jecxz没有长跳转
					//用 85 c9 test ecx, ecx 
					//   0f 84 00000000 je addr来代替
					PtrIn_RtnBuffer[0]= 0x85;
					PtrIn_RtnBuffer[1]= 0xc9;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 2;

					PtrIn_RtnBuffer[0]= 0x0f;
					PtrIn_RtnBuffer[1]= 0x84;

					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 2;

					*((PDWORD)(PtrIn_RtnBuffer))= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 2- diffFromNewBaseAndRtnBuffer_I)- 6;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
					break;

				default:
					//在default里也处理 0x7?的情况。
					//这儿替换成对应的 0f 8?
					PtrIn_RtnBuffer[0]= 0x0f;
					PtrIn_RtnBuffer[1]= static_cast <BYTE> (0x80| (0xf& (OrgOpcode+ tempOpcodeOff_Dw)[0]));

					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 2;

					*((PDWORD)(PtrIn_RtnBuffer))= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 2- diffFromNewBaseAndRtnBuffer_I)- 6;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
					break;
				case 0xeb:
					//*不能动orgopcode,其实只是改成0xeb，然后和jmp一样处理。*//
					//
					*PtrIn_RtnBuffer= 0xe9;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 1;
					*(DWORD *) (PtrIn_RtnBuffer)= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 1- diffFromNewBaseAndRtnBuffer_I)- 5;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
					break;
				case 0xe9:
					*PtrIn_RtnBuffer= *(OrgOpcode+ tempOpcodeOff_Dw);
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 1;
					*(DWORD *) (PtrIn_RtnBuffer)= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 1- diffFromNewBaseAndRtnBuffer_I)- 5;
					PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
					//*32bits的jmp，可以和call那个一样处理*//
					//
				}
				break;
			case 1:
				//call的话,直接重新设置要跳转的地址就可以了。
				*PtrIn_RtnBuffer= *(OrgOpcode+ tempOpcodeOff_Dw);
				PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 1;
				*(DWORD *) (PtrIn_RtnBuffer)= PureJmpAddr_Dw- ((DWORD)PtrIn_RtnBuffer- 1- diffFromNewBaseAndRtnBuffer_I)- 5;
				PtrIn_RtnBuffer= PtrIn_RtnBuffer+ 4;
				break;
			case 0:
				memcpy ( PtrIn_RtnBuffer, OrgOpcode+ tempOpcodeOff_Dw, tempForOpcodeLen);
				PtrIn_RtnBuffer= PtrIn_RtnBuffer+ tempForOpcodeLen;
				break;
			}

			tempOpcodeOff_Dw+= tempForOpcodeLen;
		}

		//tempOpcodeOff_Dw+= tempForOpcodeLen;
		*LenOfOpcode_Dw= tempOpcodeOff_Dw;

		*Rtn_PPb= RtnBuffer;

		return (DWORD)(PtrIn_RtnBuffer- RtnBuffer);
	}

	int GetMinValidLenWithMatchOpcode ( LPBYTE OrgOpcode_Pb, int AtLeastLen_I)
	{//得到一个最少长为AtLeastLen_I的按opcode来的长度。
		if (NULL==OrgOpcode_Pb)
		{
			return 0;
		}
		int RtnLen_I= 0;
		while (AtLeastLen_I>(RtnLen_I+= GetOpCodeSize ( OrgOpcode_Pb+ RtnLen_I)));
		return RtnLen_I;
	}


	BOOL MemWriteWithBackup (LPVOID Addr_Pvoid, DWORD len_Dw, LPBYTE Org_Pvoid, LPBYTE New_Pvoid)
	{
		DWORD ProtectMode_Dw= 0;
		BOOL Rtn_B= FALSE;

		__try
		{
			if (0==IsBadWritePtr ( Org_Pvoid, len_Dw))
			{
				memcpy ( Org_Pvoid, Addr_Pvoid, len_Dw);
			}

			VirtualProtect ( Addr_Pvoid, len_Dw, PAGE_EXECUTE_READWRITE, &ProtectMode_Dw);

			memcpy ( Addr_Pvoid, New_Pvoid, len_Dw);

			VirtualProtect ( Addr_Pvoid, len_Dw, ProtectMode_Dw, &ProtectMode_Dw);
			Rtn_B= TRUE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			;
		}

		return Rtn_B;
	}

	// apex的getopcodelen
/*
	static unsigned long MaskTable[518]={
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000008, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000008, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000008, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00008000, 0x00008000, 0x00000008, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00004000, 0x00004000,
		0x00000008, 0x00000008, 0x00001008, 0x00000018,
		0x00002000, 0x00006000, 0x00000100, 0x00004100, // 
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00004100, 0x00006000, 0x00004100, 0x00004100,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00002002, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000020, 0x00000020, 0x00000020, 0x00000020,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000100, 0x00002000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00004100, 0x00004100, 0x00000200, 0x00000000,
		0x00004000, 0x00004000, 0x00004100, 0x00006000,
		0x00000300, 0x00000000, 0x00000200, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00000100, 0x00000100, 0x00000000, 0x00000000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00000100, 0x00000100, 0x00000100, 0x00000100,
		0x00002000, 0x00002000, 0x00002002, 0x00000100,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000008, 0x00000000, 0x00000008, 0x00000008,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00002000, 0x00002000, 0x00002000, 0x00002000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00000000, 0x00000000, 0x00000000, 0x00004000,
		0x00004100, 0x00004000, 0xFFFFFFFF, 0xFFFFFFFF,
		0x00000000, 0x00000000, 0x00000000, 0x00004000,
		0x00004100, 0x00004000, 0xFFFFFFFF, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0xFFFFFFFF, 0xFFFFFFFF, 0x00004100, 0x00004000,
		0x00004000, 0x00004000, 0x00004000, 0x00004000,
		0x00004000, 0x00004000, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF
	};
	int GetOpCodeSize (PBYTE Start)
	{
		DWORD* Tlb=(DWORD*)MaskTable;
		PBYTE pOPCode;
		DWORD t, c;
		BYTE dh, dl, al;
		int OpCodeSize =-1;

		t = 0;
		pOPCode = (PBYTE) Start;
		c = 0;

		do {
			t &= 0x0F7;
			c = *(BYTE *) pOPCode++;
			t |= Tlb[c] ;

		} while( ((t & 0x000000FF) & 8) != 0);

		if ((c == 0x0F6) || (c == 0x0F7))
		{
			t |= 0x00004000;
			if ( (0x38 & *(BYTE *) pOPCode++) == 0)
				t |= 0x00008000;
		}
		else if (c == 0x0CD)
		{
			t |= 0x00000100;
			if ( (*(BYTE *) pOPCode++) == 0x20)
				t |= 0x00000400;
		}
		else if (c == 0x0F)
		{
			al = *(BYTE *) pOPCode++;
			t |= Tlb[al + 0x100];
			if (t == 0xFFFFFFFF)
				return OpCodeSize;
		}

		if ((((t & 0x0000FF00) >> 8) & 0x80) != 0)
		{
			dh = static_cast<BYTE> ((t & 0x0000FF00) >> 8);
			dh ^= 0x20;
			if ((c & 1) == 0) 
				dh ^= 0x21;
			t &= 0xFFFF00FF;
			t |= (dh << 8);
		}

		if ((((t & 0x0000FF00) >> 8) & 0x40) != 0 ) 
		{
			al = *(BYTE *) pOPCode++;
			c = (DWORD)al;
			c |= (al << 8);
			c &= 0xC007;
			if ( (c & 0x0000FF00) != 0xC000 )
			{
				if ( ((t & 0x000000FF) & 0x10) == 0)
				{
					if ((c & 0x000000FF) == 4)
					{
						al = *(BYTE *) pOPCode++;
						al &= 7;
						c &= 0x0000FF00;
						c |= al;
					}

					if ((c & 0x0000FF00) != 0x4000)
					{
						if ((c & 0x0000FF00) == 0x8000)    t |= 4;
						else if (c==5) t |= 4;
					}
					else
						t |= 1;

				}
				else
				{
					if (c != 6)
					{
						if((c & 0x0000FF00) == 0x4000)
							t |= 1;
						else if ((c & 0x0000FF00) == 0x8000) 
							t |= 2;
					}
					else
						t |= 2;
				}
			}
		}

		if ((((t & 0x000000FF)) & 0x20) != 0)
		{
			dl = static_cast<BYTE> (t & 0x000000FF);
			dl ^= 2;
			t &= 0xFFFFFF00;
			t |= dl;
			if ((dl & 0x10) == 0)
			{
				dl ^= 6;
				t &= 0xFFFFFF00;
				t |= dl;
			}
		}

		if ((((t & 0x0000FF00) >> 8) & 0x20) != 0)
		{
			dh = static_cast<BYTE> ((t & 0x0000FF00) >> 8);
			dh ^= 2;   
			t &= 0xFFFF00FF;
			t |= (dh << 8);
			if ((dh & 0x10) == 0)
			{
				if (dh & 0x40) //是否是 0x6x
					dh ^= 1;   // 当dh = 0x2x 这里计算多2，当＝62的时候却是 异或1
				t &= 0xFFFFFF00;
				t |= dh;
			}
		}

		OpCodeSize = reinterpret_cast<DWORD> (pOPCode) - reinterpret_cast<DWORD> (Start);
		t &= 0x707;
		OpCodeSize += t & 0x000000FF;
		OpCodeSize += (t & 0x0000FF00) >> 8;

		if (((*(char*)Start) & 0x000000FF) == 0x66)    
			if ( OpCodeSize >= 6)   
				OpCodeSize -= 2;   //减2处理 ，将 dword 型转成 word 型

		return OpCodeSize;
	}

	*/


#define C_ERROR         0xFFFFFFFF
#define C_PREFIX        0x00000001
#define C_66            0x00000002
#define C_67            0x00000004
#define C_DATA66        0x00000008
#define C_DATA1         0x00000010
#define C_DATA2         0x00000020
#define C_DATA4         0x00000040
#define C_MEM67         0x00000080
#define C_MEM1          0x00000100
#define C_MEM2          0x00000200
#define C_MEM4          0x00000400
#define C_MODRM         0x00000800
#define C_DATAW0        0x00001000
#define C_FUCKINGTEST   0x00002000
#define C_TABLE_0F      0x00004000

static int table_1[256] =
{
	/* 00 */   C_MODRM
	/* 01 */,  C_MODRM
	/* 02 */,  C_MODRM
	/* 03 */,  C_MODRM
	/* 04 */,  C_DATAW0
	/* 05 */,  C_DATAW0
	/* 06 */,  0
	/* 07 */,  0
	/* 08 */,  C_MODRM
	/* 09 */,  C_MODRM
	/* 0A */,  C_MODRM
	/* 0B */,  C_MODRM
	/* 0C */,  C_DATAW0
	/* 0D */,  C_DATAW0
	/* 0E */,  0
	/* 0F */,  C_TABLE_0F
	/* 10 */,  C_MODRM
	/* 11 */,  C_MODRM
	/* 12 */,  C_MODRM
	/* 13 */,  C_MODRM
	/* 14 */,  C_DATAW0
	/* 15 */,  C_DATAW0
	/* 16 */,  0
	/* 17 */,  0
	/* 18 */,  C_MODRM
	/* 19 */,  C_MODRM
	/* 1A */,  C_MODRM
	/* 1B */,  C_MODRM
	/* 1C */,  C_DATAW0
	/* 1D */,  C_DATAW0
	/* 1E */,  0
	/* 1F */,  0
	/* 20 */,  C_MODRM
	/* 21 */,  C_MODRM
	/* 22 */,  C_MODRM
	/* 23 */,  C_MODRM
	/* 24 */,  C_DATAW0
	/* 25 */,  C_DATAW0
	/* 26 */,  C_PREFIX
	/* 27 */,  0
	/* 28 */,  C_MODRM
	/* 29 */,  C_MODRM
	/* 2A */,  C_MODRM
	/* 2B */,  C_MODRM
	/* 2C */,  C_DATAW0
	/* 2D */,  C_DATAW0
	/* 2E */,  C_PREFIX
	/* 2F */,  0
	/* 30 */,  C_MODRM
	/* 31 */,  C_MODRM
	/* 32 */,  C_MODRM
	/* 33 */,  C_MODRM
	/* 34 */,  C_DATAW0
	/* 35 */,  C_DATAW0
	/* 36 */,  C_PREFIX
	/* 37 */,  0
	/* 38 */,  C_MODRM
	/* 39 */,  C_MODRM
	/* 3A */,  C_MODRM
	/* 3B */,  C_MODRM
	/* 3C */,  C_DATAW0
	/* 3D */,  C_DATAW0
	/* 3E */,  C_PREFIX
	/* 3F */,  0
	/* 40 */,  0
	/* 41 */,  0
	/* 42 */,  0
	/* 43 */,  0
	/* 44 */,  0
	/* 45 */,  0
	/* 46 */,  0
	/* 47 */,  0
	/* 48 */,  0
	/* 49 */,  0
	/* 4A */,  0
	/* 4B */,  0
	/* 4C */,  0
	/* 4D */,  0
	/* 4E */,  0
	/* 4F */,  0
	/* 50 */,  0
	/* 51 */,  0
	/* 52 */,  0
	/* 53 */,  0
	/* 54 */,  0
	/* 55 */,  0
	/* 56 */,  0
	/* 57 */,  0
	/* 58 */,  0
	/* 59 */,  0
	/* 5A */,  0
	/* 5B */,  0
	/* 5C */,  0
	/* 5D */,  0
	/* 5E */,  0
	/* 5F */,  0
	/* 60 */,  0
	/* 61 */,  0
	/* 62 */,  C_MODRM
	/* 63 */,  C_MODRM
	/* 64 */,  C_PREFIX
	/* 65 */,  C_PREFIX
	/* 66 */,  C_PREFIX+C_66
	/* 67 */,  C_PREFIX+C_67
	/* 68 */,  C_DATA66
	/* 69 */,  C_MODRM+C_DATA66
	/* 6A */,  C_DATA1
	/* 6B */,  C_MODRM+C_DATA1
	/* 6C */,  0
	/* 6D */,  0
	/* 6E */,  0
	/* 6F */,  0
	/* 70 */,  C_DATA1
	/* 71 */,  C_DATA1
	/* 72 */,  C_DATA1
	/* 73 */,  C_DATA1
	/* 74 */,  C_DATA1
	/* 75 */,  C_DATA1
	/* 76 */,  C_DATA1
	/* 77 */,  C_DATA1
	/* 78 */,  C_DATA1
	/* 79 */,  C_DATA1
	/* 7A */,  C_DATA1
	/* 7B */,  C_DATA1
	/* 7C */,  C_DATA1
	/* 7D */,  C_DATA1
	/* 7E */,  C_DATA1
	/* 7F */,  C_DATA1
	/* 80 */,  C_MODRM+C_DATA1
	/* 81 */,  C_MODRM+C_DATA66
	/* 82 */,  C_MODRM+C_DATA1
	/* 83 */,  C_MODRM+C_DATA1
	/* 84 */,  C_MODRM
	/* 85 */,  C_MODRM
	/* 86 */,  C_MODRM
	/* 87 */,  C_MODRM
	/* 88 */,  C_MODRM
	/* 89 */,  C_MODRM
	/* 8A */,  C_MODRM
	/* 8B */,  C_MODRM
	/* 8C */,  C_MODRM
	/* 8D */,  C_MODRM
	/* 8E */,  C_MODRM
	/* 8F */,  C_MODRM
	/* 90 */,  0
	/* 91 */,  0
	/* 92 */,  0
	/* 93 */,  0
	/* 94 */,  0
	/* 95 */,  0
	/* 96 */,  0
	/* 97 */,  0
	/* 98 */,  0
	/* 99 */,  0
	/* 9A */,  C_DATA66+C_MEM2
	/* 9B */,  0
	/* 9C */,  0
	/* 9D */,  0
	/* 9E */,  0
	/* 9F */,  0
	/* A0 */,  C_MEM67
	/* A1 */,  C_MEM67
	/* A2 */,  C_MEM67
	/* A3 */,  C_MEM67
	/* A4 */,  0
	/* A5 */,  0
	/* A6 */,  0
	/* A7 */,  0
	/* A8 */,  C_DATA1
	/* A9 */,  C_DATA66
	/* AA */,  0
	/* AB */,  0
	/* AC */,  0
	/* AD */,  0
	/* AE */,  0
	/* AF */,  0
	/* B0 */,  C_DATA1
	/* B1 */,  C_DATA1
	/* B2 */,  C_DATA1
	/* B3 */,  C_DATA1
	/* B4 */,  C_DATA1
	/* B5 */,  C_DATA1
	/* B6 */,  C_DATA1
	/* B7 */,  C_DATA1
	/* B8 */,  C_DATA66
	/* B9 */,  C_DATA66
	/* BA */,  C_DATA66
	/* BB */,  C_DATA66
	/* BC */,  C_DATA66
	/* BD */,  C_DATA66
	/* BE */,  C_DATA66
	/* BF */,  C_DATA66
	/* C0 */,  C_MODRM+C_DATA1
	/* C1 */,  C_MODRM+C_DATA1
	/* C2 */,  C_DATA2
	/* C3 */,  0
	/* C4 */,  C_MODRM
	/* C5 */,  C_MODRM
	/* C6 */,  C_MODRM+C_DATA66
	/* C7 */,  C_MODRM+C_DATA66
	/* C8 */,  C_DATA2+C_DATA1
	/* C9 */,  0
	/* CA */,  C_DATA2
	/* CB */,  0
	/* CC */,  0
	/* CD */,  C_DATA1+C_DATA4
	/* CE */,  0
	/* CF */,  0
	/* D0 */,  C_MODRM
	/* D1 */,  C_MODRM
	/* D2 */,  C_MODRM
	/* D3 */,  C_MODRM
	/* D4 */,  0
	/* D5 */,  0
	/* D6 */,  0
	/* D7 */,  0
	/* D8 */,  C_MODRM
	/* D9 */,  C_MODRM
	/* DA */,  C_MODRM
	/* DB */,  C_MODRM
	/* DC */,  C_MODRM
	/* DD */,  C_MODRM
	/* DE */,  C_MODRM
	/* DF */,  C_MODRM
	/* E0 */,  C_DATA1
	/* E1 */,  C_DATA1
	/* E2 */,  C_DATA1
	/* E3 */,  C_DATA1
	/* E4 */,  C_DATA1
	/* E5 */,  C_DATA1
	/* E6 */,  C_DATA1
	/* E7 */,  C_DATA1
	/* E8 */,  C_DATA66
	/* E9 */,  C_DATA66
	/* EA */,  C_DATA66+C_MEM2
	/* EB */,  C_DATA1
	/* EC */,  0
	/* ED */,  0
	/* EE */,  0
	/* EF */,  0
	/* F0 */,  C_PREFIX
	/* F1 */,  0                       // 0xF1
	/* F2 */,  C_PREFIX
	/* F3 */,  C_PREFIX
	/* F4 */,  0
	/* F5 */,  0
	/* F6 */,  C_FUCKINGTEST
	/* F7 */,  C_FUCKINGTEST
	/* F8 */,  0
	/* F9 */,  0
	/* FA */,  0
	/* FB */,  0
	/* FC */,  0
	/* FD */,  0
	/* FE */,  C_MODRM
	/* FF */,  C_MODRM
}; // table_1

static int table_0F[256] =
{
	/* 00 */   C_MODRM
	/* 01 */,  C_MODRM
	/* 02 */,  C_MODRM
	/* 03 */,  C_MODRM
	/* 04 */,  -1
	/* 05 */,  -1
	/* 06 */,  0
	/* 07 */,  -1
	/* 08 */,  0
	/* 09 */,  0
	/* 0A */,  0
	/* 0B */,  0
	/* 0C */,  -1
	/* 0D */,  -1
	/* 0E */,  -1
	/* 0F */,  -1
	/* 10 */,  -1
	/* 11 */,  -1
	/* 12 */,  -1
	/* 13 */,  -1
	/* 14 */,  -1
	/* 15 */,  -1
	/* 16 */,  -1
	/* 17 */,  -1
	/* 18 */,  -1
	/* 19 */,  -1
	/* 1A */,  -1
	/* 1B */,  -1
	/* 1C */,  -1
	/* 1D */,  -1
	/* 1E */,  -1
	/* 1F */,  -1
	/* 20 */,  -1
	/* 21 */,  -1
	/* 22 */,  -1
	/* 23 */,  -1
	/* 24 */,  -1
	/* 25 */,  -1
	/* 26 */,  -1
	/* 27 */,  -1
	/* 28 */,  -1
	/* 29 */,  -1
	/* 2A */,  -1
	/* 2B */,  -1
	/* 2C */,  -1
	/* 2D */,  -1
	/* 2E */,  -1
	/* 2F */,  -1
	/* 30 */,  -1
	/* 31 */,  -1
	/* 32 */,  -1
	/* 33 */,  -1
	/* 34 */,  -1
	/* 35 */,  -1
	/* 36 */,  -1
	/* 37 */,  -1
	/* 38 */,  -1
	/* 39 */,  -1
	/* 3A */,  -1
	/* 3B */,  -1
	/* 3C */,  -1
	/* 3D */,  -1
	/* 3E */,  -1
	/* 3F */,  -1
	/* 40 */,  -1
	/* 41 */,  -1
	/* 42 */,  -1
	/* 43 */,  -1
	/* 44 */,  -1
	/* 45 */,  -1
	/* 46 */,  -1
	/* 47 */,  -1
	/* 48 */,  -1
	/* 49 */,  -1
	/* 4A */,  -1
	/* 4B */,  -1
	/* 4C */,  -1
	/* 4D */,  -1
	/* 4E */,  -1
	/* 4F */,  -1
	/* 50 */,  -1
	/* 51 */,  -1
	/* 52 */,  -1
	/* 53 */,  -1
	/* 54 */,  -1
	/* 55 */,  -1
	/* 56 */,  -1
	/* 57 */,  -1
	/* 58 */,  -1
	/* 59 */,  -1
	/* 5A */,  -1
	/* 5B */,  -1
	/* 5C */,  -1
	/* 5D */,  -1
	/* 5E */,  -1
	/* 5F */,  -1
	/* 60 */,  -1
	/* 61 */,  -1
	/* 62 */,  -1
	/* 63 */,  -1
	/* 64 */,  -1
	/* 65 */,  -1
	/* 66 */,  -1
	/* 67 */,  -1
	/* 68 */,  -1
	/* 69 */,  -1
	/* 6A */,  -1
	/* 6B */,  -1
	/* 6C */,  -1
	/* 6D */,  -1
	/* 6E */,  -1
	/* 6F */,  -1
	/* 70 */,  -1
	/* 71 */,  -1
	/* 72 */,  -1
	/* 73 */,  -1
	/* 74 */,  -1
	/* 75 */,  -1
	/* 76 */,  -1
	/* 77 */,  -1
	/* 78 */,  -1
	/* 79 */,  -1
	/* 7A */,  -1
	/* 7B */,  -1
	/* 7C */,  -1
	/* 7D */,  -1
	/* 7E */,  -1
	/* 7F */,  -1
	/* 80 */,  C_DATA66
	/* 81 */,  C_DATA66
	/* 82 */,  C_DATA66
	/* 83 */,  C_DATA66
	/* 84 */,  C_DATA66
	/* 85 */,  C_DATA66
	/* 86 */,  C_DATA66
	/* 87 */,  C_DATA66
	/* 88 */,  C_DATA66
	/* 89 */,  C_DATA66
	/* 8A */,  C_DATA66
	/* 8B */,  C_DATA66
	/* 8C */,  C_DATA66
	/* 8D */,  C_DATA66
	/* 8E */,  C_DATA66
	/* 8F */,  C_DATA66
	/* 90 */,  C_MODRM
	/* 91 */,  C_MODRM
	/* 92 */,  C_MODRM
	/* 93 */,  C_MODRM
	/* 94 */,  C_MODRM
	/* 95 */,  C_MODRM
	/* 96 */,  C_MODRM
	/* 97 */,  C_MODRM
	/* 98 */,  C_MODRM
	/* 99 */,  C_MODRM
	/* 9A */,  C_MODRM
	/* 9B */,  C_MODRM
	/* 9C */,  C_MODRM
	/* 9D */,  C_MODRM
	/* 9E */,  C_MODRM
	/* 9F */,  C_MODRM
	/* A0 */,  0
	/* A1 */,  0
	/* A2 */,  0
	/* A3 */,  C_MODRM
	/* A4 */,  C_MODRM+C_DATA1
	/* A5 */,  C_MODRM
	/* A6 */,  -1
	/* A7 */,  -1
	/* A8 */,  0
	/* A9 */,  0
	/* AA */,  0
	/* AB */,  C_MODRM
	/* AC */,  C_MODRM+C_DATA1
	/* AD */,  C_MODRM
	/* AE */,  -1
	/* AF */,  C_MODRM
	/* B0 */,  C_MODRM
	/* B1 */,  C_MODRM
	/* B2 */,  C_MODRM
	/* B3 */,  C_MODRM
	/* B4 */,  C_MODRM
	/* B5 */,  C_MODRM
	/* B6 */,  C_MODRM
	/* B7 */,  C_MODRM
	/* B8 */,  -1
	/* B9 */,  -1
	/* BA */,  C_MODRM+C_DATA1
	/* BB */,  C_MODRM
	/* BC */,  C_MODRM
	/* BD */,  C_MODRM
	/* BE */,  C_MODRM
	/* BF */,  C_MODRM
	/* C0 */,  C_MODRM
	/* C1 */,  C_MODRM
	/* C2 */,  -1
	/* C3 */,  -1
	/* C4 */,  -1
	/* C5 */,  -1
	/* C6 */,  -1
	/* C7 */,  -1
	/* C8 */,  0
	/* C9 */,  0
	/* CA */,  0
	/* CB */,  0
	/* CC */,  0
	/* CD */,  0
	/* CE */,  0
	/* CF */,  0
	/* D0 */,  -1
	/* D1 */,  -1
	/* D2 */,  -1
	/* D3 */,  -1
	/* D4 */,  -1
	/* D5 */,  -1
	/* D6 */,  -1
	/* D7 */,  -1
	/* D8 */,  -1
	/* D9 */,  -1
	/* DA */,  -1
	/* DB */,  -1
	/* DC */,  -1
	/* DD */,  -1
	/* DE */,  -1
	/* DF */,  -1
	/* E0 */,  -1
	/* E1 */,  -1
	/* E2 */,  -1
	/* E3 */,  -1
	/* E4 */,  -1
	/* E5 */,  -1
	/* E6 */,  -1
	/* E7 */,  -1
	/* E8 */,  -1
	/* E9 */,  -1
	/* EA */,  -1
	/* EB */,  -1
	/* EC */,  -1
	/* ED */,  -1
	/* EE */,  -1
	/* EF */,  -1
	/* F0 */,  -1
	/* F1 */,  -1
	/* F2 */,  -1
	/* F3 */,  -1
	/* F4 */,  -1
	/* F5 */,  -1
	/* F6 */,  -1
	/* F7 */,  -1
	/* F8 */,  -1
	/* F9 */,  -1
	/* FA */,  -1
	/* FB */,  -1
	/* FC */,  -1
	/* FD */,  -1
	/* FE */,  -1
	/* FF */,  -1
}; // table_0F

int __stdcall GetOpCodeSize(LPBYTE iptr0)
{
	BYTE* iptr = iptr0;

	DWORD f = 0;

prefix:
	BYTE b = *iptr++;

	f |= table_1[b];

	if (f&C_FUCKINGTEST)
		if (((*iptr)&0x38)==0x00)   // ttt
			f=C_MODRM+C_DATAW0;       // TEST
		else
			f=C_MODRM;                // NOT,NEG,MUL,IMUL,DIV,IDIV

	if (f&C_TABLE_0F)
	{
		b = *iptr++;
		f = table_0F[b];
	}

	if (f==C_ERROR)
	{
		//printf("error in %02X\n",b);
		return C_ERROR;
	}

	if (f&C_PREFIX)
	{
		f&=~C_PREFIX;
		goto prefix;
	}

	if (f&C_DATAW0) if (b&0x01) f|=C_DATA66; else f|=C_DATA1;

	if (f&C_MODRM)
	{
		b = *iptr++;
		BYTE mod = b & 0xC0;
		BYTE rm  = b & 0x07;
		if (mod!=0xC0)
		{
			if (f&C_67)         // modrm16
			{
				if ((mod==0x00)&&(rm==0x06)) f|=C_MEM2;
				if (mod==0x40) f|=C_MEM1;
				if (mod==0x80) f|=C_MEM2;
			}
			else                // modrm32
			{
				if (mod==0x40) f|=C_MEM1;
				if (mod==0x80) f|=C_MEM4;
				if (rm==0x04) rm = (*iptr++) & 0x07;    // rm<-sib.base
				if ((rm==0x05)&&(mod==0x00)) f|=C_MEM4;
			}
		}
	} // C_MODRM

	if (f&C_MEM67)  if (f&C_67) f|=C_MEM2;  else f|=C_MEM4;
	if (f&C_DATA66) if (f&C_66) f|=C_DATA2; else f|=C_DATA4;

	if (f&C_MEM1)  iptr++;
	if (f&C_MEM2)  iptr+=2;
	if (f&C_MEM4)  iptr+=4;

	if (f&C_DATA1) iptr++;
	if (f&C_DATA2) iptr+=2;
	if (f&C_DATA4) iptr+=4;

	return iptr - iptr0;
}

/// 
char * trim_crlf_ (char * Str)
{
	char C;
	char * Rtn;
	int len;
	C= Str[0];
	len= strlen ( Str);
	Rtn= Str;
	if (0==C)
	{
		return Str;
	}

	while (0!=C)
	{
		if (('\r'==C)
			||('\n'==C)
			||(' '==C)
			||('\t'==C))
		{

			++Rtn;
			C= Rtn[0];
		}
		else
		{
			C= 0;
		}

	}

	--len;
	C= Str[len];

	while (0!=C)
	{
		if (('\r'==C)
			||('\n'==C)
			||(' '==C)
			||('\t'==C))
		{
			Str[len--]= 0;
			C= Str[len];
		}
		else
		{
			C= 0;
		}
	}
	return Rtn;
}

LONG RegReadData (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Type, LPVOID Data, DWORD TypeLen)
{
	HKEY hKey= RootKey;
	DWORD dwDisposition;
	DWORD Size = sizeof(int);
	LONG Rtn= 0;
	if (NULL!=lpSubKey)
	{	
		Rtn= RegCreateKeyEx ( RootKey, lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
		if (ERROR_SUCCESS!=Rtn)
		{
			return Rtn;
		}
		
	}

	Rtn= RegQueryValueEx(
		hKey,	// handle of key to query 
		lpValueName,	// address of name of value to query 
		NULL,	// reserved 
		&Type,	// address of buffer for value type 
		static_cast<LPBYTE>(Data),	// address of data buffer 
		&TypeLen 	// address of data buffer size 
		);
	//
	if ((NULL!=lpSubKey)
		&&(ERROR_SUCCESS!=Rtn))
	{
		RegCloseKey(hKey);
	}


	return Rtn;
}

LONG RegWriteData ( HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Type, LPVOID Data, DWORD TypeLen)
{
	HKEY hKey= RootKey;
	DWORD dwDisposition;
	DWORD Size = sizeof(int);
	LONG Rtn= 0;
	if (NULL!=lpSubKey)
	{	
		Rtn= RegCreateKeyEx ( RootKey, lpSubKey, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
		if (ERROR_SUCCESS!=Rtn)
		{
			return Rtn;
		}
	}

	Rtn= RegSetValueEx ( hKey, lpValueName, 0, Type, (CONST BYTE *)Data, TypeLen);

	if (NULL!=lpSubKey)
	{	
		RegCloseKey ( hKey);
	}
	return Rtn;
}

LONG RegWriteDword (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Data)
{
	return RegWriteData ( RootKey, lpSubKey, lpValueName, REG_DWORD, (BYTE *)&Data, 4);
}

LONG RegWriteStr (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, LPCSTR Data, DWORD Strlen)
{
	return RegWriteData ( RootKey, lpSubKey, lpValueName, REG_SZ, (LPVOID)Data, Strlen);
}

LONG RegReadStr  (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, LPCSTR Data, DWORD Strlen)
{
	return RegReadData ( RootKey, lpSubKey, lpValueName, REG_SZ, (LPVOID)Data, Strlen);
}

LONG RegReadDword  (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD * Data)
{
	return RegReadData ( RootKey, lpSubKey, lpValueName, REG_DWORD, (BYTE *)Data, 4);
}

unsigned int __stdcall CalcCRC(char* data, int len)
{
	char tmp= 0;
	char tmp1= 0;
	char tmp2= 0;
	char tmp3= 0;
	char tmp4= 0;
	for (int i= 0; i<len; i++)
	{
		tmp= data[i];
		tmp1+= tmp;
		tmp2= tmp2^ tmp;
		tmp3= tmp3+ i^ tmp;
		tmp4= tmp4^ (i+ tmp);
	}

	return tmp1| (tmp2<< 8)| (tmp3<< 8)| (tmp4<< 8);
}


char * duplicate_str (char * Org)
{
	if (NULL==Org)
	{
		return NULL;
	}
	int len= strlen( Org);
	char * Rtn_buf= new char [len+ 1 ];
	strcpy_s ( Rtn_buf, len+ 1, Org);
	return Rtn_buf;
}


int getDBCSlen (char * DBCSstr)
{
	int temp= 0;
	while (0!=*DBCSstr)
	{
		if (IsDBCSLeadByte( *DBCSstr))
		{
			++DBCSstr;
			if (0==*DBCSstr)
			{
				++temp;
				break;
			}
		}
		++DBCSstr;
		++temp;
	}
	return temp;
}
char * clean_remark ( char * src, char remarkTag)
{
	if	((NULL==src)
		||('\0'==src[0]))
	{
		return NULL;
	}
	char * tag= strrchr ( src, remarkTag);
	if (tag)
	{
		*tag= '\0';
	}
	return src;
}

/*
char Number[]= "0123456789";
char LowerAbc[]= "abcdefghijklmnopqrstuvwxyz";

char UpperAbc[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
*/
LPSTR vkToStr(int vk, LPSTR Rtn, int Len) 
{
	#define caseStringify(x) case x: strcpy_s( c, 0x10,(#x)); break;
	char c[0x10] = {0};
	
	if (vk >= '0' && vk <= '9') 
	{
		c[0]=(char)vk; 
		strcpy_s ( Rtn, Len, c);
	} 

	if (vk >= 'A' && vk <= 'Z') 
	{
		c[0]=(char)vk; 
		strcpy_s ( Rtn, Len, c);
	}

	if (vk >= 'A' && vk <= 'Z') 
	{ 
		c[0]=(char)vk; 
		strcpy_s ( Rtn, Len, c);
	}

	if (0!=c[0])
	{
		return Rtn;
	}


	switch(vk) 
	{
		
		caseStringify(VK_LBUTTON);

		caseStringify(VK_RBUTTON);

		caseStringify(VK_CANCEL);

		caseStringify(VK_MBUTTON);
		
		caseStringify(VK_XBUTTON1);
		
		caseStringify(VK_XBUTTON2);
		
		caseStringify(VK_BACK);

		caseStringify(VK_TAB);

		caseStringify(VK_CLEAR);

		caseStringify(VK_RETURN);

		caseStringify(VK_SHIFT);

		caseStringify(VK_CONTROL);

		caseStringify(VK_MENU);

		caseStringify(VK_PAUSE);

		caseStringify(VK_CAPITAL);

		caseStringify(VK_KANA);

		caseStringify(VK_JUNJA);

		caseStringify(VK_FINAL);

		caseStringify(VK_KANJI);

		caseStringify(VK_ESCAPE);

		caseStringify(VK_CONVERT);

		caseStringify(VK_NONCONVERT);

		caseStringify(VK_ACCEPT);

		caseStringify(VK_MODECHANGE);

		caseStringify(VK_SPACE);

		caseStringify(VK_PRIOR);

		caseStringify(VK_NEXT);

		caseStringify(VK_END);

		caseStringify(VK_HOME);

		caseStringify(VK_LEFT);

		caseStringify(VK_UP);

		caseStringify(VK_RIGHT);

		caseStringify(VK_DOWN);

		caseStringify(VK_SELECT);

		caseStringify(VK_PRINT);

		caseStringify(VK_EXECUTE);

		caseStringify(VK_SNAPSHOT);

		caseStringify(VK_INSERT);

		caseStringify(VK_DELETE);

		caseStringify(VK_HELP);

		caseStringify(VK_LWIN);

		caseStringify(VK_RWIN);

		caseStringify(VK_APPS);

		caseStringify(VK_SLEEP);

		caseStringify(VK_NUMPAD0);

		caseStringify(VK_NUMPAD1);

		caseStringify(VK_NUMPAD2);

		caseStringify(VK_NUMPAD3);

		caseStringify(VK_NUMPAD4);

		caseStringify(VK_NUMPAD5);

		caseStringify(VK_NUMPAD6);

		caseStringify(VK_NUMPAD7);

		caseStringify(VK_NUMPAD8);

		caseStringify(VK_NUMPAD9);

		caseStringify(VK_MULTIPLY);

		caseStringify(VK_ADD);

		caseStringify(VK_SEPARATOR);

		caseStringify(VK_SUBTRACT);

		caseStringify(VK_DECIMAL);

		caseStringify(VK_DIVIDE);

		caseStringify(VK_F1);

		caseStringify(VK_F2);

		caseStringify(VK_F3);

		caseStringify(VK_F4);

		caseStringify(VK_F5);

		caseStringify(VK_F6);

		caseStringify(VK_F7);

		caseStringify(VK_F8);

		caseStringify(VK_F9);

		caseStringify(VK_F10);

		caseStringify(VK_F11);

		caseStringify(VK_F12);

		caseStringify(VK_F13);

		caseStringify(VK_F14);

		caseStringify(VK_F15);

		caseStringify(VK_F16);

		caseStringify(VK_F17);

		caseStringify(VK_F18);

		caseStringify(VK_F19);

		caseStringify(VK_F20);

		caseStringify(VK_F21);

		caseStringify(VK_F22);

		caseStringify(VK_F23);

		caseStringify(VK_F24);

		caseStringify(VK_NUMLOCK);

		caseStringify(VK_SCROLL);

		caseStringify(VK_OEM_NEC_EQUAL);
		

		caseStringify(VK_OEM_FJ_MASSHOU);
		

		caseStringify(VK_OEM_FJ_TOUROKU);
		

		caseStringify(VK_OEM_FJ_LOYA);
		

		caseStringify(VK_OEM_FJ_ROYA);
		

		caseStringify(VK_LSHIFT);

		caseStringify(VK_RSHIFT);

		caseStringify(VK_LCONTROL);

		caseStringify(VK_RCONTROL);

		caseStringify(VK_LMENU);

		caseStringify(VK_RMENU);

		caseStringify(VK_BROWSER_BACK);

		caseStringify(VK_BROWSER_FORWARD);

		caseStringify(VK_BROWSER_REFRESH);

		caseStringify(VK_BROWSER_STOP);

		caseStringify(VK_BROWSER_SEARCH);

		caseStringify(VK_BROWSER_FAVORITES);

		caseStringify(VK_BROWSER_HOME);

		caseStringify(VK_VOLUME_MUTE);

		caseStringify(VK_VOLUME_DOWN);

		caseStringify(VK_VOLUME_UP);

		caseStringify(VK_MEDIA_NEXT_TRACK);

		caseStringify(VK_MEDIA_PREV_TRACK);

		caseStringify(VK_MEDIA_STOP);

		caseStringify(VK_MEDIA_PLAY_PAUSE);

		caseStringify(VK_LAUNCH_MAIL);

		caseStringify(VK_LAUNCH_MEDIA_SELECT);

		caseStringify(VK_LAUNCH_APP1);

		caseStringify(VK_LAUNCH_APP2);

		caseStringify(VK_OEM_1);
		

		caseStringify(VK_OEM_PLUS);
		
		caseStringify(VK_OEM_COMMA);
		
		caseStringify(VK_OEM_MINUS);
		
		caseStringify(VK_OEM_PERIOD);
		
		caseStringify(VK_OEM_2);
		
		caseStringify(VK_OEM_3);
		
		caseStringify(VK_OEM_4);
		
		     
		caseStringify(VK_OEM_5);
		
		caseStringify(VK_OEM_6);
		caseStringify(VK_OEM_7);
		
		caseStringify(VK_OEM_8);

		caseStringify(VK_OEM_AX);
		
		caseStringify(VK_OEM_102);
		
		caseStringify(VK_ICO_HELP);
		
		caseStringify(VK_ICO_00);
		
		caseStringify(VK_PROCESSKEY);

		caseStringify(VK_ICO_CLEAR);

		caseStringify(VK_PACKET);
	}
	strcpy_s ( Rtn, Len, &c[3]);

	return Rtn;
}

};//extern "C" {


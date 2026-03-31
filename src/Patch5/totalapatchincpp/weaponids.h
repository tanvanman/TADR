#pragma once



#include "headers.h"
#include "builtin.h"
#include "defines.h"



BYTE* WeaponTypeDefinitions;


#pragma pack(1)

struct PacketFeatureDestroy
{
    BYTE PacketType;
    BYTE WeaponId; // old
    WORD xPos;
    WORD yPos;
    DWORD ExpandedWeaponId;
};


struct PacketWeaponProjectile
{
    BYTE PacketType;
    DWORD StartX;
    DWORD StartY;
    DWORD StartZ;
    DWORD EndX;
    DWORD EndY;
    DWORD EndZ;
    BYTE WeaponId; // old weapon id
    BYTE Interceptor;
    WORD Angle;
    WORD TrajectoryResult;
    WORD TargetUnitId;
    WORD AttackerUnitId;
    BYTE WeaponIndex;
    DWORD ExpandedWeaponId;
};


struct PacketAreaOfEffectDamage
{
    BYTE PacketType;
    DWORD xPos;
    DWORD yPos;
    DWORD zPos;
    BYTE WeaponId; // old weapon id
    DWORD ExpandedWeaponId;
};



struct PositionDWORD
{
    DWORD x;
    DWORD y;
    DWORD z;
};

#pragma pack(4)


PacketFeatureDestroy* SendFeaturePacketBuffer1;
PacketWeaponProjectile* SendFireWeaponPacketBuffer1;
PacketAreaOfEffectDamage* SendAreaOfEffectDamageBuffer1;


// asm x86 memory
DWORD Stor_1;
DWORD Stor_2;
DWORD Stor_3;
DWORD Stor_4;
DWORD Stor_5;
DWORD Stor_6;
DWORD Stor_7;
DWORD Stor_8;

WORD wStor_1;
WORD wStor_2;

BYTE bStor_1;
BYTE bStor_2;


// ____ReceiveWeaponFired_Hook_1 Variables
DWORD ExpandedWeaponId;
LPVOID CorrespondingWeaponIdOffset;



// asm x86 returns
LPVOID ____LoadAllWeapons_Hook_1_Return;// = (LPVOID)0x0042E345;
LPVOID ____LoadWeaponFromTDF_Hook_1_Return;// = (LPVOID)0x0042E490;
LPVOID ____LoadWeaponFromTDF_Hook_2_Return;// = (LPVOID)0x0042ECF9;
LPVOID ____LoadUnitInfoFromFBI_Hook_1_Return;// = (LPVOID)0x0042CDEE;
LPVOID ____WeaponUnknown_Hook_1_Return;// = (LPVOID)0x00409555;
LPVOID ____WeaponUnknown_Hook_2_Return;// = (LPVOID)0x00409682;
LPVOID ____WeaponUnknown_Hook_3_Return;// = (LPVOID)0x00409948;
LPVOID ____StartWeaponsScripts_Hook_1_Return;// = (LPVOID)0x0049E0CE;


LPVOID ____PacketDispatcher_Hook_1_Return;// = (LPVOID)0x0045548B;
LPVOID ____SendFireWeapon_Hook_1_Return;// = (LPVOID)0x00499B90;

//////////LPVOID ____PacketDispatcher_Hook_2_Return = (LPVOID);
////////LPVOID ____ReceiveWeaponFired_Hook_1_Return = (LPVOID)0x0049D2AF;
LPVOID ____FeaturesTakeWeaponDamage_Hook_1_Return;// = (LPVOID)0x00424569;
LPVOID ____FeaturesTakeWeaponDamage_Hook_2_Return;// = (LPVOID)0x00424591;
LPVOID ____FeaturesTakeWeaponDamage_Hook_3_Return;// = (LPVOID)0x00424632;
LPVOID ____FeaturesTakeWeaponDamage_Hook_4_Return;// = (LPVOID)0x00424664;
LPVOID ____FeaturesTakeWeaponDamage_Hook_5_Return;// = (LPVOID)0x00424694;

LPVOID ____AreaOfEffectDamage_Hook_1_Return;// = (LPVOID)0x0049A7EE;
LPVOID ____ReceiveAreaOfEffectDamage_Hook_1_Return;// = (LPVOID)0x0049AFD4;

LPVOID ____FireCallback1_Hook_1_Return;// = (LPVOID)0x0049D85E;
LPVOID ____FireCallback2_Hook_1_Return;// = (LPVOID)0x0049DD2C;
LPVOID ____FireCallback3_Hook_1_Return;// = (LPVOID)0x0049DEF3;
LPVOID ____FireCallback4_Hook_1_Return;// = (LPVOID)0x0049DB52;

LPVOID ____FireMapWeapon_Hook_1_Return;// = (LPVOID)0x0049DFFB;



LPVOID ____ReceiveWeaponFired_Hook_1_Return;// = (LPVOID)0x0049D2AD;
LPVOID ____ReceiveWeaponFired_Hook_1_Return_Jmp1;
LPVOID ____ReceiveWeaponFired_Hook_1_Return_JNZ;// = (LPVOID)0x0049D2AF;
LPVOID ____ReceiveWeaponFired_Hook_1_Return_JZ;// = (LPVOID)0x0049D329;


LPVOID ____SaveGame_SaveUnit_Hook_1_Return;// = (LPVOID)0x00487A22;
LPVOID ____LoadGame_LoadUnit_Hook_1_Return;// = (LPVOID)0x0048762E;



LPVOID ____PacketSize_PacketFeatureExplode_Hook_1_Return;// = (LPVOID)0x0045213B;


LPVOID ____TotalA_strcmpi;// = (LPVOID)0x004F8A70;
LPVOID ____TotalA_HAPIBroadcastMessage;// = (LPVOID)0x00451DF0;
LPVOID ____TotalA_GetLocalPlayerDPID;// = (LPVOID)0x0049DFF0;


LPVOID ____TotalA_jmpToCode_42ECF9;// = (LPVOID)0x0042ECF9;
LPVOID ____TotalA_jmpToCode_42F340;// = (LPVOID)0x0042F340;
LPVOID ____TotalA_jmpToCode_42EDA1;// = (LPVOID)0x0042EDA1;
LPVOID ____TotalA_jmpToCode_42ED7B;// = (LPVOID)0x0042ED7B;

LPVOID ____TotalA_jumpToCode_WeaponId253_4554D4;// = (LPVOID)0x004554D4;
LPVOID ____TotalA_jumpToCode_WeaponId254_4554BA;// = (LPVOID)0x004554BA;
LPVOID ____TotalA_jumpToCode_WeaponId255_4554A0;// = (LPVOID)0x004554A0;

LPVOID ____TotalA_jmpToCode_InCode_49D329;// = (LPVOID)0x0049D329;
LPVOID ____TotalA_jmpToCode_InCode_42469F;// = (LPVOID)0x0042469F;


LPVOID ____FeaturesDestroy253;// = (LPVOID)0x00423550;
LPVOID ____FeaturesDestroy254;
LPVOID ____FeaturesDestroy255;
LPVOID ____FeaturesDestroyDefault;

LPVOID ____GetGridPlotPos;



//LPVOID ____FeaturesTakeWeaponDamage_Hook_0_1;
LPVOID ____FeaturesTakeWeaponDamage_Hook_0_1_Return;



// the whole glory of missing something lmfao

LPVOID ____StartTreeBurn_Hook_1_Return;// = (LPVOID)0x00423531;
LPVOID ____ReclaimFinished_Hook_1_Return;
LPVOID ____Order_Resurrect_Hook_1_Return;


// prototypes
void AllocateNewArray();
void ApplyWeaponIdsPatches();
void WeaponIds();
void StaticInitializers_WeaponIds();




__declspec(naked) void ____LoadAllWeapons_Hook_1()
{
    __asm
    {
        push esi
        push edi

        pushad
        pushfd

        call AllocateNewArray

        popfd
        popad

        jmp ____LoadAllWeapons_Hook_1_Return
    }
}





// asm x86 patches
LPVOID WINAPI ____LoadWeaponFromTDF_Hook_1_HelperA(DWORD Id)
{
	return (LPVOID)((DWORD)WeaponTypeDefinitions + (Id * WEAPON_ID_STRUCT_SIZE));
}


__declspec(naked) void ____LoadWeaponFromTDF_Hook_1()
{
	__asm
	{
		pushad
		pushfd

		push eax
		call ____LoadWeaponFromTDF_Hook_1_HelperA
		mov [Stor_1], eax

		popfd
		popad

		mov ebp, [Stor_1]
        xor eax, eax

		push 0x005119B8
		push 0x40
		push 0x00503884

		// Not Needed as ebp is set
		//mov [Stor_1], ebp
		//add ebp, 0x20
		//push ebp
		//mov ebp, [Stor_1]

		jmp ____LoadWeaponFromTDF_Hook_1_Return

		//0042e46e 8d 0c 40        LEA        ECX, [EAX + EAX * 0x2]
		//0042e471 c1 e1 03        SHL        ECX, 0x3
		//0042e474 2b c8           SUB        ECX, EAX
		//0042e476 03 d0           ADD        EDX, EAX
		//0042e478 33 c0           XOR        EAX, EAX
		//0042e47a 68 b8 19        PUSH       lpData_005119b8 = 00000000
		//         51 00
		//0042e47f 8d 0c 49        LEA        ECX, [ECX + ECX * 0x2]
		//0042e482 6a 40           PUSH       0x40
		//0042e484 68 84 38        PUSH       DAT_00503884 = 6Eh    n
		//         50 00
		//0042e489 8d ac 8a        LEA        EBP, [EDX + ECX * 0x4 + 0x2cf3]
		//         f3 2c 00 00
	}
}











__declspec(naked) void ____LoadWeaponFromTDF_Hook_2()
{
	__asm
	{
		mov eax, [ebp+0x115]
        mov dword ptr [esp+0x18], 0
        test eax, eax
        mov [esp+0x10], eax
        jbe jmpToCode_42ECF9

    innerloop:

        pushad
        pushfd

        mov esi, [esp+0x24+0x18]
        push esi
        mov [Stor_2], esi
        
        call ____LoadWeaponFromTDF_Hook_1_HelperA
        mov [Stor_1], eax

        popfd
        popad
        
        mov eax, [Stor_1]
        mov esi, [Stor_2]

        add eax, 0x80
        lea ecx, [esp+0x40]
        push eax
        push ecx
        call [____TotalA_strcmpi]
        add esp, 8
        test eax, eax
        jz jmpToCode_42F340
        mov eax, [esp+0x18]
        mov ecx, [esp+0x10]
        inc eax
        cmp eax, ecx
        mov [esp+0x18], eax
        jb innerloop


		jmp ____LoadWeaponFromTDF_Hook_2_Return


    jmpToCode_42ECF9:
        jmp ____TotalA_jmpToCode_42ECF9
    jmpToCode_42F340:
        jmp ____TotalA_jmpToCode_42F340
        
        

	}
}





__declspec(naked) void ____LoadWeaponFromTDF_Hook_3()
{
    __asm
    {
        pushad
        pushfd

        mov eax, [esp+0x24+0x10]
        push eax
        call ____LoadWeaponFromTDF_Hook_1_HelperA
        mov [Stor_1], eax

        popfd
        popad

        mov eax, [Stor_1]
        add eax, 0x80
        mov byte ptr [eax], 0x00
        //mov esi, [esp+0x18] // extra, probs just use esi, actually xD ???
        
        pushad
        pushfd

        mov esi, [esp+0x24+0x18]
        push esi
        call ____LoadWeaponFromTDF_Hook_1_HelperA
        mov [Stor_2], eax

        popfd
        popad

        mov eax, [Stor_1]
        mov esi, [Stor_2]
        add eax, 0x74
        add esi, 0x74
        mov ecx, [esi]
        mov [eax], ecx // should work for ModelDebris?
        jmp ____TotalA_jmpToCode_42EDA1


    }
}




__declspec(naked) void ____LoadWeaponFromTDF_Hook_4()
{
    __asm
    {
        // dont forget ModelName!

        pushad
        pushfd

        // 0x24 from pushad and pushfd
        mov eax, [esp+0x24+0x10]
        push eax
        call ____LoadWeaponFromTDF_Hook_1_HelperA
        mov [Stor_1], eax

        popfd
        popad



        mov eax, [Stor_1]
        add eax, 0x74
        mov [eax], esi // ModelDebris

        // strcpy
        mov edx, [Stor_1]
        add edx, 0x80

        lea edi, [esp+0x40]

        jmp ____TotalA_jmpToCode_42ED7B


    }
}




__declspec(naked) void ____LoadUnitInfoFromFBI_Hook_1()
{
    __asm
    {
        // not needed, just get the ptr to new weapontypedef array
        //pushad
        //pushfd

        //push 0
        //push 



        //popfd
        //popad





        push 0x80
        lea ecx, [esp + 0xb0]
        push 0x00503810
        push ecx
        mov ecx, [esp + 0x24]

        // esi
        mov esi, WeaponTypeDefinitions


        jmp ____LoadUnitInfoFromFBI_Hook_1_Return


        //0042cdcd a1 e8 1d        MOV        EAX,[TAMainStruct]                               = ??
        //         51 00
        //0042cdd2 68 80 00        PUSH       0x80
        //         00 00
        //0042cdd7 8d 8c 24        LEA        ECX=>local_480,[ESP + 0xb0]
        //         b0 00 00 00
        //0042cdde 68 10 38        PUSH       s_weapon1_00503810                               = "weapon1"
        //         50 00
        //0042cde3 51              PUSH       ECX
        //0042cde4 8b 4c 24 24     MOV        ECX,dword ptr [ESP + local_514]
        //0042cde8 8d b0 f3        LEA        ESI,[EAX + 0x2cf3]
        //         2c 00 00

    }
}





/*__declspec(naked)*/ char* WINAPI ____WeaponNameToWeaponTypeDefinition_Replace(char* WeaponName)
{
    if (!WeaponName || !____strlen(WeaponName))
        return (char*)0;

    for (SIZE_T off = 0; off < MAX_NUMBER_OF_WEAPONS * WEAPON_ID_STRUCT_SIZE; off += WEAPON_ID_STRUCT_SIZE)
    {
        if (!____strcmpi(((const char*)((DWORD)WeaponTypeDefinitions + off)), WeaponName))
        {
            return ((char*)((DWORD)WeaponTypeDefinitions + off));
        }
    }

    return (char*)0;

}





__declspec(naked) void ____WeaponUnknown1_Hook_1()
{
    __asm
    {
        mov eax, [esi+0x115]
        test eax, eax
        jmp ____WeaponUnknown_Hook_1_Return
    }
}

__declspec(naked) void ____WeaponUnknown2_Hook_1()
{
    __asm
    {
        mov eax, [esi + 0x115]
        test eax, eax
        jmp ____WeaponUnknown_Hook_2_Return
    }
}

__declspec(naked) void ____WeaponUnknown3_Hook_1()
{
    __asm
    {
        mov eax, [esi + 0x115]
        test eax, eax
        jmp ____WeaponUnknown_Hook_3_Return
    }
}






__declspec(naked) void ____StartWeaponsScripts_Hook_1()
{
    __asm
    {
        mov ebx, [ecx+0x115]
        mov byte ptr [esi-0x1], 0
        test ebx, ebx

        jmp ____StartWeaponsScripts_Hook_1_Return
    }
}














__declspec(naked) void ____PacketDispatcher_Hook_1()
{
    __asm
    {
        mov edi, [esp+0x10]
        //inc edi
        mov [Stor_2], edi // feature destroy packet
        mov eax, [edi+6] // weapon id
        mov [Stor_1], eax
    }

    if (Stor_1 == 253) // weapon id
    {
        __asm
        {
            xor eax, eax
            xor ecx, ecx
            mov edi, [Stor_2]
            mov ax, [edi + 0x4]
            mov cx, [edi + 0x2]
            push 0
            push eax
            push ecx
            call ____FeaturesDestroy253
        }
    }
    else if (Stor_1 == 254)
    {
        __asm
        {
            xor ecx, ecx
            xor edx, edx
            mov edi, [Stor_2]
            mov cx, [edi + 0x4]
            mov dx, [edi + 0x2]
            push 1
            push ecx
            push edx
            call ____FeaturesDestroy254
        }
    }
    else if (Stor_1 == 255)
    {
        __asm
        {
            xor eax, eax
            xor ecx, ecx
            mov edi, [Stor_2]
            mov ax, [edi + 0x4]
            mov cx, [edi + 0x2]
            push 1
            push eax
            push ecx
            call ____FeaturesDestroy255
        }
    }
    else
    {
        __asm
        {
            xor edx, edx
            xor esi, esi
            mov edi, [Stor_2]
            mov dx, [edi + 0x4]
            mov si, [edi + 0x2]
            mov eax, [Stor_1]
            push eax
            call ____LoadWeaponFromTDF_Hook_1_HelperA // ------------------------------------------------------------------------------
            push eax // weapon ptr
            push edx
            push esi
            push edx
            push esi
            call ____GetGridPlotPos
            push eax
            call ____FeaturesDestroyDefault
        }
    }

    __asm
    {
        mov eax, 0x00455F50 // lazy atm - pls fix!
        jmp eax
    }


}





void WINAPI ____FeaturesTakeWeaponDamage_Hook_1_HelperA()
{
    if (SendFeaturePacketBuffer1)
    {
        ____free(SendFeaturePacketBuffer1);
    }

    SendFeaturePacketBuffer1 = (PacketFeatureDestroy*)____malloc(sizeof(PacketFeatureDestroy));
    ____memset(SendFeaturePacketBuffer1, 0, sizeof(PacketFeatureDestroy));
}








//DWORD FeaturesPacketInited;




__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_0_1() // 0x004244B0 // ret 0x004244B6
{
    __asm
    {
        mov [Stor_1], eax
		mov [Stor_2], edx
		mov [Stor_3], ecx
		mov [Stor_4], ebx
		mov [Stor_5], esi
		mov [Stor_6], edi
		mov [Stor_7], ebp

        call ____FeaturesTakeWeaponDamage_Hook_1_HelperA

        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        mov ecx, dword ptr ds:[0x511DE8]
        jmp ____FeaturesTakeWeaponDamage_Hook_0_1_Return
    }
}








__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_1()
{
    __asm
    {
        //pushad
        //pushfd

        //call ____FeaturesTakeWeaponDamage_Hook_1_HelperA
        //mov dword ptr [FeaturesPacketInited], 1

        //popfd
        //popad


        mov eax, [esp+0x28]
        mov eax, [eax+0x115]
        mov [Stor_1], eax

        
        mov ax, [esp+0x20] // eax is trashed after anyway
        mov [wStor_1], ax

        mov ax, [esp+0x24] // stack adjusted
        mov [wStor_2], ax

        mov [Stor_8], eax
		mov [Stor_2], edx
		mov [Stor_3], ecx
		mov [Stor_4], ebx
		mov [Stor_5], esi
		mov [Stor_6], edi
		mov [Stor_7], ebp
    }

    SendFeaturePacketBuffer1->PacketType = 15; // Packet Type:  0xF  (15)
    SendFeaturePacketBuffer1->ExpandedWeaponId = Stor_1;
    SendFeaturePacketBuffer1->xPos = wStor_1;
    SendFeaturePacketBuffer1->yPos = wStor_2;


        //mov [Stor_1], edi
        //mov edi, [SendFeaturePacketBuffer1] // ptr

        //mov byte ptr [edi+0x0], 15 // packet type
        //mov edx, [esp+0x28] // weapontypedef ptr
        //mov eax, [edx+0x115] // new weapon id
        //mov dword ptr [edi+0x6], eax // buffer new weapon id
        //mov cx, [esp+0x20]
        //push 10 // 0xA  new packet size
        //mov dx, [esp+0x20]
        //push edi // packet buffer start ptr
        //mov [edi+0x2], cx
        //mov [edi+0x4], dx

    __asm
    {
        mov eax, [Stor_8]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        push 6 + 4 // new packet size

        mov eax, [SendFeaturePacketBuffer1] // buffer ptr
        push eax

        jmp ____FeaturesTakeWeaponDamage_Hook_1_Return
    }
}





__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_2()
{

        __asm
        {
            //test dword ptr [FeaturesPacketInited], 0
            //jz skip
            mov [Stor_1], edi
            mov edi, [SendFeaturePacketBuffer1]
            mov dword ptr [edi+0x6], 252
            mov edi, [Stor_1]
            //skip:
            jmp ____FeaturesTakeWeaponDamage_Hook_2_Return
    }
}

__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_3()
{

        __asm
        {
            //test dword ptr [FeaturesPacketInited], 0
            //jz skip
            mov [Stor_1], edi
            mov edi, [SendFeaturePacketBuffer1]
            mov dword ptr [edi+0x6], 253
            mov edi, [Stor_1]

            //skip:
            jmp ____FeaturesTakeWeaponDamage_Hook_3_Return
    }
}

__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_4()
{

        __asm
        {
            //pushad
            //pushfd
            //test dword ptr [FeaturesPacketInited], 0
            //jz skip
            mov [Stor_1], edi
            mov edi, [SendFeaturePacketBuffer1]
            mov dword ptr [edi+0x6], 253
            mov edi, [Stor_1]
            //skip:
            //popfd
            //popad
            jmp ____FeaturesTakeWeaponDamage_Hook_4_Return
    }
}


BOOL WINAPI ____FeaturesTakeWeaponDamage_Hook_5_HelperA(DWORD Id)
{
    if (Id > 252 && Id < 256)
    {
        return true;
    }

    return false;
}




// should be done
__declspec(naked) void ____FeaturesTakeWeaponDamage_Hook_5()
{
    __asm
    {
        //pushad
        //pushfd

        //call ____FeaturesTakeWeaponDamage_Hook_1_HelperA

        //popfd
        //popad

        mov eax, [esp+0x28]
        mov eax, [eax+0x115]
        mov [Stor_1], eax

        push eax
        call ____FeaturesTakeWeaponDamage_Hook_5_HelperA
        test eax, eax
        jz ____TotalA_jmpToCode_42469F

        mov ax, [esp+0x20] // eax is trashed after anyway
        mov [wStor_1], ax

        mov ax, [esp+0x24] // stack adjusted
        mov [wStor_2], ax

        jmp skipJmpToCode
        ____TotalA_jmpToCode_42469F:
        jmp ____TotalA_jmpToCode_InCode_42469F // same as ____FeaturesTakeWeaponDamage_Hook_1_Return
        skipJmpToCode:
    }

    SendFeaturePacketBuffer1->PacketType = 15; // Packet Type:  0xF  (15)
    SendFeaturePacketBuffer1->ExpandedWeaponId = Stor_1;
    SendFeaturePacketBuffer1->xPos = wStor_1;
    SendFeaturePacketBuffer1->yPos = wStor_2;

    __asm
    {
        push 6 + 4 // new packet size
        //push SendFeaturePacketBuffer1 // buffer ptr
        mov eax, [SendFeaturePacketBuffer1]
        push eax

        jmp ____FeaturesTakeWeaponDamage_Hook_5_Return
    }
}












void WINAPI ____SendFireWeapon_Hook_1_HelperA()
{
    if (SendFireWeaponPacketBuffer1)
    {
        ____free(SendFireWeaponPacketBuffer1);
    }

    SendFireWeaponPacketBuffer1 = (PacketWeaponProjectile*)____malloc(sizeof(PacketWeaponProjectile));
    ____memset(SendFireWeaponPacketBuffer1, 0, sizeof(PacketWeaponProjectile));
}


void WINAPI ____AreaOfEffectDamage_Hook_1_HelperA()
{
    if (SendAreaOfEffectDamageBuffer1 != nullptr)
    {
        ____free(SendAreaOfEffectDamageBuffer1);
    }

    SendAreaOfEffectDamageBuffer1 = (PacketAreaOfEffectDamage*)____malloc(sizeof(PacketAreaOfEffectDamage));
    ____memset(SendAreaOfEffectDamageBuffer1, 0, sizeof(PacketAreaOfEffectDamage));
}

__declspec(naked) void ____ReceiveAreaOfEffectDamage_Hook_1()
{
    __asm
    {
        mov ebx, [ebx+0x115]
        cmp ebx, [edi+0xE]
        jmp ____ReceiveAreaOfEffectDamage_Hook_1_Return
    }
}



__declspec(naked) void ____PacketSize_PacketFeatureExplode_Hook_1()
{
    __asm
    {
        mov dword ptr ds:[0x00512B14], 6 + 4 // weapon ids

        jmp ____PacketSize_PacketFeatureExplode_Hook_1_Return
    }
}











PacketWeaponProjectile* IncomingPacketPtr;




// dont use function headers!!
__declspec(naked) void WINAPI ____ReceiveWeaponFired_Hook_1(/*LPVOID PlayerPtr, PacketWeaponProjectile* IncomingPacket*/)
{
    __asm
    {
        mov eax, [esp + 0x8]
        mov[IncomingPacketPtr], eax

        sub esp, 16 // 4 vars
        push ebx
        push ebp
        push esi
        push edi
    }

    ExpandedWeaponId = IncomingPacketPtr->ExpandedWeaponId;
    CorrespondingWeaponIdOffset = ____LoadWeaponFromTDF_Hook_1_HelperA(ExpandedWeaponId);

    __asm
    {
        mov ebp, [CorrespondingWeaponIdOffset]
        mov [esp + 32 - 4], ebp
        mov eax, [IncomingPacketPtr]
        mov ecx, dword ptr ds:[0x511DE8]

        mov edx, ebp
        mov edx, [edx+0x111]
        shr edx, 5
        test dl, 1
        jmp ____ReceiveWeaponFired_Hook_1_Return_Jmp1
    }


}























PositionDWORD* PositionStart;
PositionDWORD* PositionEnd;
DWORD AttackerDPID;




__declspec(naked) void ____FireCallback1_Hook_1()
{
    __asm
    {
        mov [Stor_1], eax
    }

    ____SendFireWeapon_Hook_1_HelperA();
    
    __asm
    {
        lea eax, [esp+0x18]
        mov [PositionStart], eax

        // ebp should still be the same
        mov [PositionEnd], ebp

        // esi should still be the same
        mov [Stor_5], esi

        mov eax, [esi+0xC]

        mov [Stor_6], eax

        mov edx, [eax+0x115]
        mov [Stor_2], edx

        // edi should still be the same
        mov [Stor_3], edi

        // ebx should still be the same
        mov [Stor_4], ebx
    }

    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->WeaponIndex = (Stor_1 >> 2) & 3;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    if (Stor_3) // attacker ptr
        SendFireWeaponPacketBuffer1->AttackerUnitId = *((WORD*)((DWORD)Stor_3 + 0xA8));
    else
        SendFireWeaponPacketBuffer1->AttackerUnitId = 0;

    if(Stor_4)
        SendFireWeaponPacketBuffer1->TargetUnitId = *((WORD*)((DWORD)Stor_4 + 0xA8));

    SendFireWeaponPacketBuffer1->Angle = *((WORD*)((DWORD)Stor_5 + 0x16));
    SendFireWeaponPacketBuffer1->TrajectoryResult = *((WORD*)((DWORD)Stor_5 + 0x18));
    SendFireWeaponPacketBuffer1->Interceptor ^= (SendFireWeaponPacketBuffer1->Interceptor ^ (*((DWORD*)((DWORD)Stor_6 + 0x111)) >> 30 )) & 1;

    AttackerDPID = *((DWORD*)(* ((DWORD*)((DWORD)Stor_3 + 0x96)) + 0x4));
    
    __asm
    {
        push 36 + 4
        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        mov eax, [AttackerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____FireCallback1_Hook_1_Return
    }
}











//PositionDWORD* PositionStart;
//PositionDWORD* PositionEnd;
//DWORD AttackerDPID;




__declspec(naked) void ____FireCallback2_Hook_1()
{
    //__asm
    //{
    //    mov [Stor_1], eax
    //}

    // DONT TOUCH

    ____SendFireWeapon_Hook_1_HelperA();
    
    __asm
    {
        lea eax, [esp+0x10]
        mov [PositionStart], eax

        // edi should still be the same
        mov [PositionEnd], edi

        // esi should still be the same
        mov [Stor_5], esi


        mov al, [esi+0x1B]
        mov [bStor_1], al

        
        mov eax, [esi+0xC]
        mov [Stor_6], eax

        // weapon
        mov eax, [Stor_6]
        mov edx, [eax+0x115]
        mov [Stor_2], edx

        // ebx should still be the same
        mov [Stor_3], ebx

        // ebp should still be the same
        mov eax, [esp+0x4C]
        mov [Stor_4], ebp
    }

    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->WeaponIndex = (bStor_1 >> 2) & 3;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    if (Stor_3) // attacker ptr
        SendFireWeaponPacketBuffer1->AttackerUnitId = *((WORD*)((DWORD)Stor_3 + 0xA8));
    else
        SendFireWeaponPacketBuffer1->AttackerUnitId = 0;

    if(Stor_4)
        SendFireWeaponPacketBuffer1->TargetUnitId = *((WORD*)((DWORD)Stor_4 + 0xA8));

    SendFireWeaponPacketBuffer1->Angle = *((WORD*)((DWORD)Stor_5 + 0x16));
    SendFireWeaponPacketBuffer1->TrajectoryResult = *((WORD*)((DWORD)Stor_5 + 0x18));
    SendFireWeaponPacketBuffer1->Interceptor ^= (SendFireWeaponPacketBuffer1->Interceptor ^ (*((DWORD*)((DWORD)Stor_6 + 0x111)) >> 30 )) & 1;

    AttackerDPID = *((DWORD*)(* ((DWORD*)((DWORD)Stor_3 + 0x96)) + 0x4));
    
    __asm
    {
        push 36 + 4
        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        mov eax, [AttackerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____FireCallback2_Hook_1_Return
    }
}













__declspec(naked) void ____FireCallback3_Hook_1()
{
    //__asm
    //{
    //    mov [Stor_1], eax
    //}

    // TOUCH - 3

    ____SendFireWeapon_Hook_1_HelperA();

    __asm
    {
        lea eax, [esp+0x10]
        mov [PositionStart], eax


        mov eax, [esp+0x50]
        mov [PositionEnd], eax


        // unitweapon
        mov [Stor_5], ebx


        mov al, [ebx+0x1B]
        mov [bStor_1], al

        // weapontypedef ptr
        mov eax, [ebx+0xC]
        mov [Stor_6], eax

        // weapon
        mov eax, [Stor_6]
        mov edx, [eax+0x115]
        mov [Stor_2], edx


        mov [Stor_3], edi

        // victim (target) unit
        mov ecx, [esp+0x4C]
        mov [Stor_4], ecx
    }

    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->WeaponIndex = (bStor_1 >> 2) & 3;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    if (Stor_3) // attacker ptr
        SendFireWeaponPacketBuffer1->AttackerUnitId = *((WORD*)((DWORD)Stor_3 + 0xA8));
    else
        SendFireWeaponPacketBuffer1->AttackerUnitId = 0;

    if (Stor_4)
        SendFireWeaponPacketBuffer1->TargetUnitId = *((WORD*)((DWORD)Stor_4 + 0xA8));

    SendFireWeaponPacketBuffer1->Angle = *((WORD*)((DWORD)Stor_5 + 0x16));
    SendFireWeaponPacketBuffer1->TrajectoryResult = *((WORD*)((DWORD)Stor_5 + 0x18));
    SendFireWeaponPacketBuffer1->Interceptor ^= (SendFireWeaponPacketBuffer1->Interceptor ^ (*((DWORD*)((DWORD)Stor_6 + 0x111)) >> 30)) & 1;

    AttackerDPID = *((DWORD*)(*((DWORD*)((DWORD)Stor_3 + 0x96)) + 0x4));

    __asm
    {
        push 36 + 4
        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        mov eax, [AttackerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____FireCallback3_Hook_1_Return
    }
}














__declspec(naked) void ____FireCallback4_Hook_1()
{
    //__asm
    //{
    //    mov [Stor_1], eax
    //}

    // TOUCH - 4

    ____SendFireWeapon_Hook_1_HelperA();

    __asm
    {
        // attacker ptr for pos
        lea eax, [edi+0x6A]
        mov [PositionStart], eax

        // is ebx
        mov [PositionEnd], ebx

        // is esi
        mov [Stor_5], esi


        mov al, [esi+0x1B]
        mov [bStor_1], al

        // weapontypedef
        mov eax, [esi+0xC]
        mov [Stor_6], eax

        // weapon
        mov eax, [Stor_6]
        mov edx, [eax+0x115]
        mov [Stor_2], edx

        // attacker
        mov [Stor_3], edi

        // victim (target)
        mov [Stor_4], ebp
    }

    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->WeaponIndex = (bStor_1 >> 2) & 3;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    if (Stor_3) // attacker ptr
        SendFireWeaponPacketBuffer1->AttackerUnitId = *((WORD*)((DWORD)Stor_3 + 0xA8));
    else
        SendFireWeaponPacketBuffer1->AttackerUnitId = 0;

    if (Stor_4)
        SendFireWeaponPacketBuffer1->TargetUnitId = *((WORD*)((DWORD)Stor_4 + 0xA8));

    SendFireWeaponPacketBuffer1->Angle = *((WORD*)((DWORD)Stor_5 + 0x16));
    SendFireWeaponPacketBuffer1->TrajectoryResult = *((WORD*)((DWORD)Stor_5 + 0x18));
    SendFireWeaponPacketBuffer1->Interceptor ^= (SendFireWeaponPacketBuffer1->Interceptor ^ (*((DWORD*)((DWORD)Stor_6 + 0x111)) >> 30)) & 1;

    AttackerDPID = *((DWORD*)(*((DWORD*)((DWORD)Stor_3 + 0x96)) + 0x4));

    __asm
    {
        push 36 + 4
        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        mov eax, [AttackerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____FireCallback4_Hook_1_Return
    }
}




__declspec(naked) void ____SendFireWeapon_Hook_1()
{
   //__asm
    //{
    //    mov [Stor_1], eax
    //}

    // TOUCH - 4

    ____SendFireWeapon_Hook_1_HelperA();

    __asm
    {
        push esi

        // attacker ptr for pos
        mov eax, [esp+0x34+0x4] // everything below stack adjusted already!
        mov [PositionStart], eax

        // position end
        mov eax, [esp+0x3C]
        mov [PositionEnd], eax

        // is 

        mov eax, [esp+0x2C]
        mov [Stor_5], eax

        mov eax, [Stor_5] // redundant but meh
        mov al, [eax+0x1B]
        mov [bStor_1], al

        // weapontypedef
        mov eax, [Stor_5]
        mov eax, [eax+0xC]
        mov [Stor_6], eax

        // weapon
        mov eax, [Stor_6] // redundant but idc
        mov edx, [eax+0x115]
        mov [Stor_2], edx

        // attacker
        mov eax, [esp+0x34]
        mov [Stor_3], eax

        // victim (target)
        mov eax, [esp+0x30]
        mov [Stor_4], eax
    }

    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->WeaponIndex = (bStor_1 >> 2) & 3;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    if (Stor_3) // attacker ptr
        SendFireWeaponPacketBuffer1->AttackerUnitId = *((WORD*)((DWORD)Stor_3 + 0xA8));
    else
        SendFireWeaponPacketBuffer1->AttackerUnitId = 0;

    if (Stor_4)
        SendFireWeaponPacketBuffer1->TargetUnitId = *((WORD*)((DWORD)Stor_4 + 0xA8));

    SendFireWeaponPacketBuffer1->Angle = *((WORD*)((DWORD)Stor_5 + 0x16));
    SendFireWeaponPacketBuffer1->TrajectoryResult = *((WORD*)((DWORD)Stor_5 + 0x18));
    SendFireWeaponPacketBuffer1->Interceptor ^= (SendFireWeaponPacketBuffer1->Interceptor ^ (*((DWORD*)((DWORD)Stor_6 + 0x111)) >> 30)) & 1;

    AttackerDPID = *((DWORD*)(*((DWORD*)((DWORD)Stor_3 + 0x96)) + 0x4));

    __asm
    {
        push 36 + 4

        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        mov eax, [AttackerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        pop esi

        jmp ____SendFireWeapon_Hook_1_Return
    }
}






DWORD OwnerDPID;


__declspec(naked) void ____AreaOfEffectDamage_Hook_1()
{
    ____AreaOfEffectDamage_Hook_1_HelperA();

    __asm
    {
        mov [PositionStart], ebx

        mov eax, [esi] // weapon projectile to weapon
        mov [Stor_1], eax
        
        mov eax, [Stor_1] // redundant
        mov eax, [eax+0x115]
        mov [Stor_2], eax // expanded weapon id

        mov eax, ebp // projectile ptr
        mov eax, [eax+8] // ebp stack +8
        // projectile ptr
        mov [Stor_3], eax

        mov eax, [Stor_3] // projectile ptr - redundant
        mov eax, [eax+0x52] // attacker unit ptr
        mov eax, [eax+0x96] // owner ptr
        mov eax, [eax+0x4] // DPID from
        mov [OwnerDPID], eax
    }

    SendAreaOfEffectDamageBuffer1->PacketType = 14; // 0xE
    SendAreaOfEffectDamageBuffer1->xPos = PositionStart->x;
    SendAreaOfEffectDamageBuffer1->yPos = PositionStart->y;
    SendAreaOfEffectDamageBuffer1->zPos = PositionStart->z;
    SendAreaOfEffectDamageBuffer1->ExpandedWeaponId = Stor_2;

    __asm
    {
        push 14 + 4
        mov eax, [SendAreaOfEffectDamageBuffer1]
        push eax
        mov eax, [OwnerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage
    }

    ____AreaOfEffectDamage_Hook_1_HelperA();

    __asm
    {
        mov eax, [Stor_3]
        lea eax, [eax+0x28]
        mov [PositionEnd], eax

        mov eax, [Stor_1] // stored weapon from above (double packet)
        mov eax, [eax+115] // weapon
        mov [Stor_4], eax
    }

    SendAreaOfEffectDamageBuffer1->PacketType = 14; // 0xE
    SendAreaOfEffectDamageBuffer1->xPos = PositionEnd->x;
    SendAreaOfEffectDamageBuffer1->yPos = PositionEnd->y;
    SendAreaOfEffectDamageBuffer1->zPos = PositionEnd->z;
    SendAreaOfEffectDamageBuffer1->ExpandedWeaponId = Stor_4;

    __asm
    {
        push 14 + 4
        mov eax, [SendAreaOfEffectDamageBuffer1]
        push eax
        mov eax, [OwnerDPID]
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____AreaOfEffectDamage_Hook_1_Return
    }

}





__declspec(naked) void ____FireMapWeapon_Hook_1()
{
    __asm
    {
        mov [Stor_1], eax
        mov [Stor_2], edx
        mov [Stor_3], ecx
        mov [Stor_4], ebx
        mov [Stor_5], esi
        mov [Stor_6], edi
        mov [Stor_7], ebp
    }

    ____SendFireWeapon_Hook_1_HelperA();

    __asm
    {
        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]
    }

    __asm
    {
        mov [Stor_1], eax

        mov eax, [ebx+0x115]
        mov [Stor_2], eax

        mov [PositionStart], edi

        mov eax, [Stor_1]
        mov [PositionEnd], eax
    }

    SendFireWeaponPacketBuffer1->PacketType = 13;
    SendFireWeaponPacketBuffer1->StartX = PositionStart->x;
    SendFireWeaponPacketBuffer1->StartY = PositionStart->y;
    SendFireWeaponPacketBuffer1->StartZ = PositionStart->z;
    SendFireWeaponPacketBuffer1->EndX = PositionEnd->x;
    SendFireWeaponPacketBuffer1->EndY = PositionEnd->y;
    SendFireWeaponPacketBuffer1->EndZ = PositionEnd->z;
    SendFireWeaponPacketBuffer1->ExpandedWeaponId = Stor_2;

    __asm
    {
        push 36 + 4
        mov eax, [SendFireWeaponPacketBuffer1]
        push eax
        call ____TotalA_GetLocalPlayerDPID
        push eax
        call ____TotalA_HAPIBroadcastMessage

        jmp ____FireMapWeapon_Hook_1_Return
    }
}













__declspec(naked) void ____StartTreeBurn_Hook_1()
{
    __asm
    {
        mov [Stor_1], eax
		mov [Stor_2], edx
		mov [Stor_3], ecx
		mov [Stor_4], ebx
		mov [Stor_5], esi
		mov [Stor_6], edi
		mov [Stor_7], ebp

        call ____FeaturesTakeWeaponDamage_Hook_1_HelperA

        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        mov [wStor_1], si
        mov [wStor_2], bp
    }

    
    
    SendFeaturePacketBuffer1->xPos = wStor_1;
    SendFeaturePacketBuffer1->yPos = wStor_2;
    SendFeaturePacketBuffer1->ExpandedWeaponId = 254;
    SendFeaturePacketBuffer1->PacketType = 0xF;




    __asm
    {
        push 6 + 4

        mov eax, [SendFeaturePacketBuffer1]
        push eax

        jmp ____StartTreeBurn_Hook_1_Return
    }

}






__declspec(naked) void ____ReclaimFinished_Hook_1()
{
    __asm
    {
        mov [Stor_1], eax
        mov [Stor_2], edx
        mov [Stor_3], ecx
        mov [Stor_4], ebx
        mov [Stor_5], esi
        mov [Stor_6], edi
        mov [Stor_7], ebp

        call ____FeaturesTakeWeaponDamage_Hook_1_HelperA

        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        mov [wStor_1], di
        mov [wStor_2], bp
    }

    __asm
    {
        mov [Stor_1], eax
        mov [Stor_2], edx
        mov [Stor_3], ecx
        mov [Stor_4], ebx
        mov [Stor_5], esi
        mov [Stor_6], edi
        mov [Stor_7], ebp
    }

    SendFeaturePacketBuffer1->PacketType = 0xF;
    SendFeaturePacketBuffer1->ExpandedWeaponId = 255;
    SendFeaturePacketBuffer1->xPos = wStor_1;
    SendFeaturePacketBuffer1->yPos = wStor_2;

    __asm
    {
        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        mov edx, [esi+0x96]

        push 6 + 4
        mov ecx, [SendFeaturePacketBuffer1]
        push ecx
        mov eax, [edx+0x4]
        push eax

        call ____TotalA_HAPIBroadcastMessage

        jmp ____ReclaimFinished_Hook_1_Return
    }
}






__declspec(naked) void ____Order_Resurrect_Hook_1()
{
    __asm
    {
        mov [Stor_1], eax
        mov [Stor_2], edx
        mov [Stor_3], ecx
        mov [Stor_4], ebx
        mov [Stor_5], esi
        mov [Stor_6], edi
        mov [Stor_7], ebp

        call ____FeaturesTakeWeaponDamage_Hook_1_HelperA

        mov eax, [Stor_1]
        mov edx, [Stor_2]
        mov ecx, [Stor_3]
        mov ebx, [Stor_4]
        mov esi, [Stor_5]
        mov edi, [Stor_6]
        mov ebp, [Stor_7]

        mov ebx, dword ptr ds:[0x511DE8]
        mov eax, 0x4EC4EC4F
        push 6 + 4
        sub edi, [ebx+0x14287]
        imul edi
        sar edx, 2
        mov eax, edx
        mov edi, [ebx+0x14233]
        shr eax, 0x1F
        add edx, eax
        mov ecx, edx
        mov eax, ecx
        cdq
        idiv edi
        mov eax, ecx

        //// ebx gets trashed later
        //mov ebx, [SendFeaturePacketBuffer1]
        //mov [ebx+0], 0xF // packet type
        //mov dword ptr [ebx+6], 255


        mov bx, dx
        cdq
        idiv edi


        //mov ecx, edx
        //mov eax, ecx
        //cdq
        //idiv edi
        //mov eax, ecx
        // lea and push buffer
        //
        //push ebx // done
        //

        //mov bx, dx
        //cdq
        //idiv edi
       
        mov [Stor_7], ebp
        mov ebp, [SendFeaturePacketBuffer1]
        push ebp
        mov byte ptr [ebp+0], 0xF // packet type
        mov dword ptr [ebp+6], 255
        mov word ptr [ebp+2], bx
        mov word ptr [ebp+4], ax
        mov ebp, [Stor_7]

        mov edx, [ebp+0x96]
        mov eax, [edx+4]
        push eax
        call ____TotalA_HAPIBroadcastMessage

    
        jmp ____Order_Resurrect_Hook_1_Return
    }
}












__declspec(naked) void ____SaveGame_SaveUnit_Hook_1()
{
    __asm
    {
        mov edx, [ecx+0x115]
        
        jmp ____SaveGame_SaveUnit_Hook_1_Return
    }
}



__declspec(naked) void ____LoadGame_LoadUnit_Hook_1()
{
    __asm
    {
        mov ecx, [edi-0x14]
        add eax, 0x1C
        mov [edx+0x115], ecx

        jmp ____LoadGame_LoadUnit_Hook_1_Return
    }
}




// dll functions
void ApplyWeaponIdsPatches()
{
    // singleplayer
    WriteJumpHook((LPVOID)0x0042E316, (LPVOID)____LoadAllWeapons_Hook_1);
	WriteJumpHook((LPVOID)0x0042E468, (LPVOID)____LoadWeaponFromTDF_Hook_1);
	WriteJumpHook((LPVOID)0x0042EC99, (LPVOID)____LoadWeaponFromTDF_Hook_2);
	WriteJumpHook((LPVOID)0x0042F340, (LPVOID)____LoadWeaponFromTDF_Hook_3);
	WriteJumpHook((LPVOID)0x0042ED46, (LPVOID)____LoadWeaponFromTDF_Hook_4);
	WriteJumpHook((LPVOID)0x0042CDCD, (LPVOID)____LoadUnitInfoFromFBI_Hook_1);
	WriteJumpHook((LPVOID)0x0049E5B0, (LPVOID)____WeaponNameToWeaponTypeDefinition_Replace);
	WriteJumpHook((LPVOID)0x0040954D, (LPVOID)____WeaponUnknown1_Hook_1);
	WriteJumpHook((LPVOID)0x00409682, (LPVOID)____WeaponUnknown2_Hook_1);
	WriteJumpHook((LPVOID)0x00409682, (LPVOID)____WeaponUnknown3_Hook_1);
	WriteJumpHook((LPVOID)0x0049E0C2, (LPVOID)____StartWeaponsScripts_Hook_1);


    // attempt 2 in cpp for Multiplayer
    // capture the stack 
    // capture each necessary pointer
    // cpp assign data to packet
    // send
    // receive


    WriteJumpHook((LPVOID)0x0049D270, (LPVOID)____ReceiveWeaponFired_Hook_1);
    WriteJumpHook((LPVOID)0x0049D7AD, (LPVOID)____FireCallback1_Hook_1);
    WriteJumpHook((LPVOID)0x0049DC71, (LPVOID)____FireCallback2_Hook_1); // re-test ballistics - test successful
    WriteJumpHook((LPVOID)0x0049DE32, (LPVOID)____FireCallback3_Hook_1);
    WriteJumpHook((LPVOID)0x0049DA9F, (LPVOID)____FireCallback4_Hook_1);






    //////// multiplayer
    //////// here comes the mess of code xD
    WriteJumpHook((LPVOID)0x0045544D, (LPVOID)____PacketDispatcher_Hook_1);
    ////////WriteJumpHook((LPVOID)0x00455474, (LPVOID)____PacketDispatcher_Hook_2);

    //////WriteJumpHook((LPVOID)0x0049D27B, (LPVOID)____ReceiveWeaponFired_Hook_1);

    //////

    //////// FeaturesTakeWeaponDamage
    ////////*((BYTE*)(0x004244B6+2)) = 8+10; // original stack size + new packet size

    WriteJumpHook((LPVOID)0x004244B0, ____FeaturesTakeWeaponDamage_Hook_0_1);
    WriteJumpHook((LPVOID)0x0042453B, (LPVOID)____FeaturesTakeWeaponDamage_Hook_1);
    WriteJumpHook((LPVOID)0x0042458C, (LPVOID)____FeaturesTakeWeaponDamage_Hook_2);
    WriteJumpHook((LPVOID)0x0042462D, (LPVOID)____FeaturesTakeWeaponDamage_Hook_3);
    WriteJumpHook((LPVOID)0x0042465F, (LPVOID)____FeaturesTakeWeaponDamage_Hook_4);
    WriteJumpHook((LPVOID)0x00424672, (LPVOID)____FeaturesTakeWeaponDamage_Hook_5);


    //////// SendFireWeapon
    WriteJumpHook((LPVOID)0x00499AC5, (LPVOID)____SendFireWeapon_Hook_1);

    //////// AreaOfEffectDamage
    WriteJumpHook((LPVOID)0x0049A769, (LPVOID)____AreaOfEffectDamage_Hook_1);
    WriteJumpHook((LPVOID)0x0049AFCB, (LPVOID)____ReceiveAreaOfEffectDamage_Hook_1);

    //////// Fire Callback 1
    //////WriteJumpHook((LPVOID)0x0049D7AD, (LPVOID)____FireCallBack1_Hook_1);
    //////WriteJumpHook((LPVOID)0x0049DA9F, (LPVOID)____FireCallBack2_Hook_1);
    //////WriteJumpHook((LPVOID)0x0049DC71, (LPVOID)____FireCallBack3_Hook_1);
    //////WriteJumpHook((LPVOID)0x0049DE32, (LPVOID)____FireCallBack4_Hook_1);
    WriteJumpHook((LPVOID)0x0049DFA3, (LPVOID)____FireMapWeapon_Hook_1);






    WriteJumpHook((LPVOID)0x00423516, (LPVOID)____StartTreeBurn_Hook_1);

    WriteJumpHook((LPVOID)0x0042397F, (LPVOID)____ReclaimFinished_Hook_1);




    WriteJumpHook((LPVOID)0x004051B9, (LPVOID)____Order_Resurrect_Hook_1);




    // packet classes
    *((DWORD*)(0x00452117+6)) = 36 + 4; // weapon ids
    *((DWORD*)(0x00452126+6)) = 14 + 4; // weapon ids

    WriteJumpHook((LPVOID)0x00452135, (LPVOID)____PacketSize_PacketFeatureExplode_Hook_1);

    //*((DWORD*)(0x00512B14)) = 6 + 4; // weapon ids






    // Save Games

    // Do Save Units
    WriteJumpHook((LPVOID)0x00487A1C, (LPVOID)____SaveGame_SaveUnit_Hook_1);

    // Do Load Units
    WriteJumpHook((LPVOID)0x00487622, (LPVOID)____LoadGame_LoadUnit_Hook_1);


}




void StaticInitializers_WeaponIds()
{
    // asm x86 returns
    ____LoadAllWeapons_Hook_1_Return = (LPVOID)0x0042E345;
    ____LoadWeaponFromTDF_Hook_1_Return = (LPVOID)0x0042E490;
    ____LoadWeaponFromTDF_Hook_2_Return = (LPVOID)0x0042ECF9;
    ____LoadUnitInfoFromFBI_Hook_1_Return = (LPVOID)0x0042CDEE;
    ____WeaponUnknown_Hook_1_Return = (LPVOID)0x00409555;
    ____WeaponUnknown_Hook_2_Return = (LPVOID)0x00409682;
    ____WeaponUnknown_Hook_3_Return = (LPVOID)0x00409948;
    ____StartWeaponsScripts_Hook_1_Return = (LPVOID)0x0049E0CE;


    ____PacketDispatcher_Hook_1_Return = (LPVOID)0x0045548B;
    ____SendFireWeapon_Hook_1_Return = (LPVOID)0x00499B90;

    //////////LPVOID ____PacketDispatcher_Hook_2_Return = (LPVOID);
    ////////LPVOID ____ReceiveWeaponFired_Hook_1_Return = (LPVOID)0x0049D2AF;
    ____FeaturesTakeWeaponDamage_Hook_1_Return = (LPVOID)0x00424569;
    ____FeaturesTakeWeaponDamage_Hook_2_Return = (LPVOID)0x00424591;
    ____FeaturesTakeWeaponDamage_Hook_3_Return = (LPVOID)0x00424632;
    ____FeaturesTakeWeaponDamage_Hook_4_Return = (LPVOID)0x00424664;
    ____FeaturesTakeWeaponDamage_Hook_5_Return = (LPVOID)0x00424694;

    ____AreaOfEffectDamage_Hook_1_Return = (LPVOID)0x0049A7EE;
    ____ReceiveAreaOfEffectDamage_Hook_1_Return = (LPVOID)0x0049AFD4;

    ____FireCallback1_Hook_1_Return = (LPVOID)0x0049D85E;
    ____FireCallback2_Hook_1_Return = (LPVOID)0x0049DD2C;
    ____FireCallback3_Hook_1_Return = (LPVOID)0x0049DEF3;
    ____FireCallback4_Hook_1_Return = (LPVOID)0x0049DB52;

    ____FireMapWeapon_Hook_1_Return = (LPVOID)0x0049DFFB;



    ____ReceiveWeaponFired_Hook_1_Return = (LPVOID)0x0049D2AD;

    ____ReceiveWeaponFired_Hook_1_Return_Jmp1 = (LPVOID)0x0049D2AD;


    ____ReceiveWeaponFired_Hook_1_Return_JNZ = (LPVOID)0x0049D2AF;
    ____ReceiveWeaponFired_Hook_1_Return_JZ = (LPVOID)0x0049D329;


    ____SaveGame_SaveUnit_Hook_1_Return = (LPVOID)0x00487A22;
    ____LoadGame_LoadUnit_Hook_1_Return = (LPVOID)0x0048762E;



    ____PacketSize_PacketFeatureExplode_Hook_1_Return = (LPVOID)0x0045213B;


    ____TotalA_strcmpi = (LPVOID)0x004F8A70;
    ____TotalA_HAPIBroadcastMessage = (LPVOID)0x00451DF0;
    ____TotalA_GetLocalPlayerDPID = (LPVOID)0x0044FDB0;


    ____TotalA_jmpToCode_42ECF9 = (LPVOID)0x0042ECF9;
    ____TotalA_jmpToCode_42F340 = (LPVOID)0x0042F340;
    ____TotalA_jmpToCode_42EDA1 = (LPVOID)0x0042EDA1;
    ____TotalA_jmpToCode_42ED7B = (LPVOID)0x0042ED7B;

    ____TotalA_jumpToCode_WeaponId253_4554D4 = (LPVOID)0x004554D4;
    ____TotalA_jumpToCode_WeaponId254_4554BA = (LPVOID)0x004554BA;
    ____TotalA_jumpToCode_WeaponId255_4554A0 = (LPVOID)0x004554A0;

    ____TotalA_jmpToCode_InCode_49D329 = (LPVOID)0x0049D329;
    ____TotalA_jmpToCode_InCode_42469F = (LPVOID)0x0042469F;




    ____FeaturesDestroy253 = (LPVOID)0x00423550; // same
    ____FeaturesDestroy254 = (LPVOID)0x004233A0; // diff
    ____FeaturesDestroy255 = (LPVOID)0x00423550; // same

    ____FeaturesDestroyDefault = (LPVOID)0x004244B0;


    ____GetGridPlotPos = (LPVOID)0x00481550;




    //____FeaturesTakeWeaponDamage_Hook_0_1 = (LPVOID)0x004244B0;
    ____FeaturesTakeWeaponDamage_Hook_0_1_Return = (LPVOID)0x004244B6;

    ____StartTreeBurn_Hook_1_Return = (LPVOID)0x00423531;



    ____ReclaimFinished_Hook_1_Return = (LPVOID)0x004239A9;
    ____Order_Resurrect_Hook_1_Return = (LPVOID)0x00405215;

}



void WeaponIds()
{
    StaticInitializers_WeaponIds();

	//AllocateNewArray(); // will be done by game, not here
	ApplyWeaponIdsPatches();
}




void AllocateNewArray()
{
    if (WeaponTypeDefinitions)
    {
        ____free(WeaponTypeDefinitions);
        WeaponTypeDefinitions = 0;
    }

	// implement new array
	if (WeaponTypeDefinitions == 0)
	{
		WeaponTypeDefinitions = (BYTE*)____malloc(MAX_NUMBER_OF_WEAPONS * WEAPON_ID_STRUCT_SIZE);

		if (WeaponTypeDefinitions != 0)
		{
            ____memset(WeaponTypeDefinitions, 0, MAX_NUMBER_OF_WEAPONS * WEAPON_ID_STRUCT_SIZE);

			for (SIZE_T off = 0, id = 0; off < MAX_NUMBER_OF_WEAPONS * WEAPON_ID_STRUCT_SIZE; off += WEAPON_ID_STRUCT_SIZE, id++)
			{
				// set ids, names already zero'd from memset
				*((DWORD*)((DWORD)WeaponTypeDefinitions + off + 0x115)) = (DWORD)id; // new offset for id [0x115]
			}
		}
	}

}
#pragma once

extern BYTE  Receive_Weapon_bits[];
extern BYTE  AreaOfDamage_bits[];
extern BYTE  Receive_AreaOfDamage_bits[];
extern BYTE  fire_callback0_bits[];
extern BYTE  fire_callback1_bits[];

extern BYTE  fire_callback2_bits[];
extern BYTE  fire_callback3_bits[];
extern BYTE  Set_Packetlen_bits[];

#define WEAPONPACKETARYLEN (10)


#define FEATUREPACKET_MDFARYLEN (9)
#define FEATUREPACKET_SGLARYLEN (4)

extern LPBYTE WeaponPacketBitsAry[];
extern DWORD WeaponPacketLenAry[];
extern unsigned int WeaponPacketAddressAry[];

extern BYTE FeaturePckt0_bits[];
extern BYTE FeaturePckt1_bits[]; 
extern BYTE FeaturePckt2_bits[];
extern BYTE FeaturePckt2_bits[];

extern BYTE FeaturePckt3_bits[];
extern BYTE FeaturePckt4_bits[];
extern BYTE FeaturePckt5_bits[];
extern BYTE FeaturePckt6_bits[];

extern DWORD FeaturePacke_mdf_addr[];
extern DWORD FeaturePacket_mdf_off[];
extern DWORD FeaturePacket_mdf_maxlen[];
extern DWORD FeaturePacket_mdf_bits_len[];
extern LPBYTE FeaturePacket_mdf_bits[];

extern BYTE PushFeaturePacketSize_bits[];

class ModifyWeaponPacket
{
private:
	SingleHook * WeaponPacketAryHook [WEAPONPACKETARYLEN];

	SingleHook * DplayxWeaponPacketLen;
	SingleHook * DplayxAofDPacketLen;
	SingleHook * DplayxFeatureDestroyPacketLen;

	ModifyHook * FeaturePacket_mdf_ary[FEATUREPACKET_MDFARYLEN];
	SingleHook * FeaturePacketSize_sgl_ary[FEATUREPACKET_SGLARYLEN];

public:
	ModifyWeaponPacket ();

	ModifyWeaponPacket (BOOL DoIt);

	~ModifyWeaponPacket ();
private:
	void ModifyRoutine (BOOL DoIt);
	void FeaturePacket (BOOL DoIt);
	void EnsureDplayx (BOOL DoIt);
};

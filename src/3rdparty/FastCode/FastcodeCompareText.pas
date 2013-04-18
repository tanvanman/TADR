unit FastcodeCompareText;

(* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Fastcode
 *
 * The Initial Developer of the Original Code is
 * Fastcode
 *
 * Portions created by the Initial Developer are Copyright (C) 2002-2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): John O'Harrow, Aleksandr Sharahov
 *
 * ***** END LICENSE BLOCK ***** *)

//Version : 1.0 Preliminary version
//Only plain function calls supported

interface

function CompareTextFastcode_JOH_IA32_2(const S1, S2: string): Integer;
function CompareTextFastcode_JOH_IA32_3(const S1, S2: string): Integer;

implementation

uses
  SysUtils, elCPUID, elCodeHook;

//Author:            John O'Harrow
//Date:              N/A
//Optimized for:     P3
//Instructionset(s): N/A
//Original Name:     CompareTextJOH_IA32_3

function CompareTextFastcode_JOH_IA32_3(const S1, S2: string): Integer;
asm
  test   eax, eax
  jnz    @@CheckS2
  test   edx, edx
  jz     @@Ret
  mov    eax, [edx-4]
  neg    eax
@@Ret:
  ret
@@CheckS2:
  test   edx, edx
  jnz    @@Compare
  mov    eax, [eax-4]
  ret
@@Compare:
  push   ebx
  push   ebp
  push   esi
  mov    ebp, [eax-4]     {length(S1)}
  mov    ebx, [edx-4]     {length(S2)}
  sub    ebp, ebx         {Result if All Compared Characters Match}
  sbb    ecx, ecx
  and    ecx, ebp
  add    ecx, ebx         {min(length(S1),length(S2)) = Compare Length}
  lea    esi, [eax+ecx]   {Last Compare Position in S1}
  add    edx, ecx         {Last Compare Position in S2}
  neg    ecx
  jz     @@SetResult      {Exit if Smallest Length = 0}
@@Loop: {Load Next 2 Chars from S1 and S2 - May Include Null Terminator}
  movzx  eax, word ptr [esi+ecx]
  movzx  ebx, word ptr [edx+ecx]
  cmp    eax, ebx
  je     @@Next           {Next 2 Chars Match}
  cmp    al, bl
  je     @@SecondPair     {First Char Matches}
  mov    ah, 0
  mov    bh, 0
  cmp    al, 'a'
  jl     @@UC1
  cmp    al, 'z'
  jg     @@UC1
  sub    eax, 'a'-'A'
@@UC1:
  cmp    bl, 'a'
  jl     @@UC2
  cmp    bl, 'z'
  jg     @@UC2
  sub    ebx, 'a'-'A'
@@UC2:
  sub    eax, ebx         {Compare Both Uppercase Chars}
  jne    @@Done           {Exit with Result in EAX if Not Equal}
  movzx  eax, word ptr [esi+ecx] {Reload Same 2 Chars from S1}
  movzx  ebx, word ptr [edx+ecx] {Reload Same 2 Chars from S2}
  cmp    ah, bh
  je     @@Next           {Second Char Matches}
@@SecondPair:
  shr    eax, 8
  shr    ebx, 8
  cmp    al, 'a'
  jl     @@UC3
  cmp    al, 'z'
  jg     @@UC3
  sub    eax, 'a'-'A'
@@UC3:
  cmp    bl, 'a'
  jl     @@UC4
  cmp    bl, 'z'
  jg     @@UC4
  sub    ebx, 'a'-'A'
@@UC4:
  sub    eax, ebx         {Compare Both Uppercase Chars}
  jne    @@Done           {Exit with Result in EAX if Not Equal}
@@Next:
  add    ecx, 2
  jl     @@Loop           {Loop until All required Chars Compared}
@@SetResult:
  mov    eax, ebp         {All Matched, Set Result from Lengths}
@@Done:
  pop    esi
  pop    ebp
  pop    ebx
end;

//Author:            John O'Harrow
//Date:              N/A
//Optimized for:     P4 Northwood
//Instructionset(s): N/A
//Original Name:     CompareTextJOH_IA32_2

function CompareTextFastcode_JOH_IA32_2(const S1, S2: string): Integer;
asm
  test   eax, eax
  jnz    @@CheckS2
  test   edx, edx         {S1 = '', if S2 = '' then Return 0 else Return -ve}
  jz     @@Ret
  mov    eax, [edx-4]     {length(S2)}
  neg    eax
@@Ret:
  ret
@@CheckS2:
  test   edx, edx
  jnz    @@Compare
  mov    eax, [eax-4]     {length(S1)}
  ret                     {S1 <> '' and S2 = '', Return +ve}
@@Compare:
  push   ebx
  push   ebp
  push   edi
  push   esi
  mov    ebp, [eax-4]     {length(S1)}
  mov    ebx, [edx-4]     {length(S2)}
  sub    ebp, ebx         {Result if All Compared Characters Match}
  sbb    ecx, ecx
  and    ecx, ebp
  add    ecx, ebx         {ECX = min(length(S1),length(S2)) = Compare
Length}
@@LengthSet:
  lea    esi, [eax+ecx]   {Last Compare Position in S1}
  lea    edi, [edx+ecx]   {Last Compare Position in S2}
  neg    ecx
  jz     @@SetResult      {Exit if Smallest Length = 0}
@@Loop:
  movzx  eax, [esi+ecx]   {Load Next Char from S1}
  movzx  ebx, [edi+ecx]   {Load Next Char from S2}
  cmp    eax, ebx
  je     @@Match1         {Repeat until Not Equal}
  add    eax, 5           {Convert Char in AL to Uppercase}
  cmp    al, 'a'+5
  setge  dl
  neg    edx
  and    edx, 32
  sub    eax, edx
  add    ebx, 5           {Convert Char in BL to Uppercase}
  cmp    bl, 'a'+5
  setge  dl
  neg    edx
  and    edx, 32
  sub    ebx, edx
  sub    eax, ebx         {Compare Both Uppercase Chars}
  jne    @@Done           {Exit with Result in EAX if Not Equal}
@@Match1:
  movzx  eax, [esi+ecx+1] {Load Next Char from S1 - May be Null Terminator}
  movzx  ebx, [edi+ecx+1] {Load Next Char from S2 - May be Null Terminator}
  cmp    eax, ebx
  je     @@Match2         {Repeat until Not Equal}
  add    eax, 5           {Convert Char in AL to Uppercase}
  cmp    al, 'a'+5
  setge  dl
  neg    edx
  and    edx, 32
  sub    eax, edx
  add    ebx, 5           {Convert Char in BL to Uppercase}
  cmp    bl, 'a'+5
  setge  dl
  neg    edx
  and    edx, 32
  sub    ebx, edx
  sub    eax, ebx         {Compare Both Uppercase Chars}
  jne    @@Done           {Exit with Result in EAX if Not Equal}
@@Match2:
  add    ecx, 2
  jl     @@Loop           {Loop until All required Chars Compared}
@@SetResult:
  mov    eax, ebp         {All Matched, Set Result from String Lengths}
@@Done:
  pop    esi
  pop    edi
  pop    ebp
  pop    ebx
end;

////////////////////////////////////////////////////////////////////////////////

type
  TFunc = function(const S1, S2: string): Integer;

const
  NewFuncs: array[TCPUType] of TFunc = (
    CompareTextFastcode_JOH_IA32_3,
    CompareTextFastcode_JOH_IA32_2,
    CompareTextFastcode_JOH_IA32_3,
    CompareTextFastcode_JOH_IA32_3,
    CompareTextFastcode_JOH_IA32_2,
    CompareTextFastcode_JOH_IA32_3
  );

var
  CodeHook: TCodeHook;
  OldFunc_: TFunc; // OldFunc_ & NewFunc_ have same type against overloads ambiguties
  OldFunc: Pointer absolute OldFunc_;
  NewFunc_: TFunc;
  NewFunc: Pointer absolute NewFunc_;

initialization
  OldFunc_ := CompareText;
  NewFunc_ := NewFuncs[GetCPUType];
  CodeHook := TCodeHook.Create(OldFunc, NewFunc);
finalization
  CodeHook.Free;
end.

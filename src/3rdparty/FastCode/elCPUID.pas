unit elCPUID;

interface

type
  TCPUType = (cpuPentium3, cpuPentium4, cpuAthlonXP, cpuOpteron, cpuPrescott, cpuBlended);

function GetCPUType: TCPUType;

implementation

var
  CPUType: TCPUType;

function GetCPUType: TCPUType;
begin
  Result := CPUType;
end;

////////////////////////////////////////////////////////////////////////////////

const
  ID_BIT = $200000;

type
  TCPUID = array[1..4] of Longint;

var
 Family, Model : Integer;

function IsCPUID_Available : Boolean; register;
asm
	pushfd							{direct access to flags no possible, only via stack}
  pop     eax 				{flags to EAX}
  mov     edx,eax			{save current flags}
  xor     eax,id_bit	{not ID bit}
  push    eax					{onto stack}
  popfd								{from stack to flags, with not ID bit}
  pushfd							{back to stack}
  pop     eax					{get back to EAX}
  xor     eax,edx			{check if ID bit affected}
  jz      @exit				{no, CPUID not availavle}
  mov     al,True			{Result=True}
@exit:
end;

function GetCPUID : TCPUID; assembler; register;
asm
  push    ebx         {Save affected register}
  push    edi
  mov     edi,eax     {@Resukt}
  mov     eax,1
  dw      $a20f       {CPUID Command}
  stosd			          {CPUID[1]}
  mov     eax,ebx
  stosd               {CPUID[2]}
  mov     eax,ecx
  stosd               {CPUID[3]}
  mov     eax,edx
  stosd               {CPUID[4]}
  pop     edi					{Restore registers}
  pop     ebx
end;


procedure DetectCPUType;
var
 P3Array : array[0..5] of Integer; // Family : 6
 P4Array : array[0..5] of Integer; // Family : 15
 XPArray : array[0..5] of Integer; // Family : 6
 OpteronArray : array[0..5] of Integer; // Family : 15
 PrescottArray : array[0..5] of Integer; // Family : 15
 CPUDetected : Boolean;
 I : Integer;

begin
 P3Array[0] := 7; // 0111
 P3Array[1] := 8; // 1000
 P3Array[2] := 10; // 1010
 P3Array[3] := 11; // 1011
 P4Array[0] := 0; // 0000
 P4Array[1] := 1; // 0001
 P4Array[2] := 2; // 0010
 XPArray[0] := 6; // 0110
 XPArray[1] := 8; // 1000
 XPArray[2] := 10; // 1010
 OpteronArray[0] := 5; // 0101
 PrescottArray[0] := 3; // 0011
 CPUDetected := False;
 begin
  if Family = 6 then
  begin
   for I := 0 to 5 do
   begin
    if Model = P3Array[I] then
     begin
      CPUType := cpuPentium3;
      Exit;
     end;
   end;
  end;
  if Family = 15 then
  begin
   for I := 0 to 5 do
   begin
    if Model = P4Array[I] then
     begin
      CPUType := cpuPentium4;
      Exit;
     end;
   end;
  end;
  if Family = 6 then
  begin
   for I := 0 to 5 do
   begin
    if Model = XPArray[I] then
     begin
      CPUType := cpuAthlonXP;
      Exit;
     end;
   end;
  end;
  if Family = 15 then
  begin
   for I := 0 to 5 do
   begin
    if Model = OpteronArray[I] then
     begin
      CPUType := cpuOpteron;
      Exit;
     end;
   end;
  end;
  if Family = 15 then
  begin
   for I := 0 to 5 do
   begin
    if Model = PrescottArray[I] then
     begin
      CPUType := cpuPrescott;
      Exit;
     end;
   end;
  end;
  if CPUDetected = False then
   begin
    CPUType := cpuBlended;
    Exit;
   end;
 end;
end;

procedure DeclareCPU;
var
 CPUID : TCPUID;
 I : Integer;
begin
 for I := Low(CPUID) to High(CPUID) do CPUID[I] := -1;
  if IsCPUID_Available then
   begin
    CPUID	:= GetCPUID;
    Family := (CPUID[1] shr 8 and $f);
    Model := (CPUID[1] shr 4 and $f);
   end;
 DetectCPUType;
end;

initialization
begin
  DeclareCPU;
end;

end.

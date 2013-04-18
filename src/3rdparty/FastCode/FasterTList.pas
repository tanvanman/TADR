{: FasterTList<p>

   Patches the VCL to speedup the execution of TList methods and its subclasses.<p>

   Minimally tested and debugged so far, released for experimentation and
   validation purposes, under MPL license.<p>  

   Eric Grange - http://glscene.org
}
unit FasterTList;

interface

uses Classes;

type

   // TFasterTList
   //
   {: TList replacement class faster when Notify isn't used. }
   TFasterTList = class (TList)
      protected
         procedure SetFCount(val : Integer); // set inherited (private) FCount
         procedure IndexError(index : Integer);

         function NotifyAddress : Pointer; // Address of Notify
         function NotifyOverriden : Boolean; // True if notify has been overriden

         function Add(item : Pointer) : Integer;
         function Get(index : Integer) : Pointer;
         procedure Put(index : Integer; item : Pointer);
         function First : Pointer;
         function Last : Pointer;

         procedure Exchange(index1, index2 : Integer);
         procedure Pack;
         function IndexOf(item : Pointer) : Integer;
         procedure Move(curIndex, newIndex : Integer);

         procedure SetCount(newCount : Integer);
   end;

   // TAddrTList
   //
   {: Used to retrieve the addresses of TList's static methods. }
   TAddrTList = class (TList) end;

implementation

uses Windows, {$ifdef VER130}Consts{$else}RTLConsts{$endif};

const
   // magic offsets to access TList private fields
   cListOffset       = $4;
   cCountOffset      = $8;
   cCapacityOffset   = $C;
   // magic offsets to VMT table
   cVMTGrowOffset    = $0;
   cVMTNotifyOffset  = $4;

type
   PPointer = ^Pointer;

var
   vDefaultTListNotifyAddr : Pointer;
   vIndexErrorMethod : Pointer;

// FillZeroes32
//
procedure FillZeroes32(nbDWords : Integer; p : Pointer);
// fill nbDWords DWORDs with zeroes starting at p, nbDWORDS assumed > 0
asm
   push  edi

   mov   ecx, eax
   mov   edi, edx
   xor   eax, eax

   rep   stosd

   pop   edi
end;

// NotifyAddress
//
function TFasterTList.NotifyAddress : Pointer;
asm
   mov eax, [eax]
   mov eax, [eax+cVMTNotifyOffset]
end;

// NotifyOverriden
//
function TFasterTList.NotifyOverriden : Boolean;
asm
   mov eax, [eax]
   mov eax, [eax+cVMTNotifyOffset]
   sub eax, vDefaultTListNotifyAddr
end;

// SetFCount
//
procedure TFasterTList.SetFCount(val : Integer);
asm
   mov [eax+cCountOffset], val
end;

// IndexError
//
procedure TFasterTList.IndexError(index : Integer);
begin
   Error(@SListIndexError, index)
end;

// Add
//
function TFasterTList.Add(item : Pointer) : Integer;
asm
   push  edi

   mov   ecx, [eax+cCountOffset]
   cmp   ecx, [eax+cCapacityOffset]
   jne   @@Assign

   pushad                           // not that heavy handed, and saves space
   mov   ecx, [eax]
   call  dword ptr [ecx+cVMTGrowOffset]
   popad

@@Assign:
   mov   edi, [eax+cListOffset]
   mov   [edi+ecx*4], edx
   inc   dword ptr [eax+cCountOffset]

   test  edx, edx
   je    @@End

   mov   edi, [eax]
   mov   edi, [edi+cVMTNotifyOffset]
   cmp   edi, vDefaultTListNotifyAddr
   je    @@End

   push  ecx
   xor   ecx, ecx                   // lnAdded = 0
   call  edi                        // Notify(Self, item, lnAdded)
   pop   ecx

@@End:
   mov   eax, ecx
   pop   edi
//}
{begin
   Result:=Count;
   if Result=Capacity then
      Grow;
   List[Result]:=item;
   SetFCount(Count+1);
   if item<>nil then
      Notify(item, lnAdded); //}
end;

// Get
//
function TFasterTList.Get(index : Integer): Pointer;
asm
   mov   ecx, [eax+cListOffset]
   cmp   edx, [eax+cCountOffset]
   jb    @@Assign
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps
@@Assign:
   mov   eax, [ecx+edx*4]
end;

// Put
//
procedure TFasterTList.Put(index : Integer; item : Pointer);
asm
   push  edi

   mov   edi, eax                      // edi gets Self

   mov   eax, [edi+cListOffset]        // precharge FList (and hope no IndexError)
   cmp   edx, [edi+cCountOffset]
   jb    @@Assign
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps

@@Assign:
   lea   edx, [eax+edx*4]              // edx gets @List[index]

   mov   eax, [edi]                    // get VMT address
   mov   eax, [eax+cVMTNotifyOffset]   // get Notify address from VMT
   cmp   eax, vDefaultTListNotifyAddr
   jnz   @@NotifyPut

   mov   [edx], ecx

   pop   edi
   ret

@@NotifyPut:
   push  esi

   mov   esi, eax                // esi gets Notify address

   mov   eax, [edx]              // eax gets current value of List[index]
   cmp   eax, ecx                // if similar to new value, exit
   jz    @@End

   mov   [edx], ecx              // otherwise, store it

   test  eax, eax                // if previous value nil, no notification
   jz    @@NoDeleteNotify

   push  ecx
   mov   edx, eax                // Notify(Self, prevValue, lnDeleted)
   mov   eax, edi
   mov   cl, lnDeleted
   call  esi
   pop   ecx

@@NoDeleteNotify:
   test  ecx, ecx                // if new value nil, no notification
   jz    @@End

   mov   eax, edi                // Notify(Self, newValue, lnAdded)
   mov   edx, ecx
   mov   cl, lnAdded
   call  esi

@@End:                           // cleanup stack
   pop   esi
   pop   edi
end;
{var
  temp : Pointer;
  p : PPointerList;
begin
   if Cardinal(index)>=Cardinal(Count) then
      IndexError(index);
   if NotifyOverriden then begin
      p:=List;
      temp:=p[index];
      if item<>temp then begin
         p[index]:=item;
         if temp<>nil then
            Notify(temp, lnDeleted);
         if item<>nil then
            Notify(item, lnAdded);
      end;
   end else List[index]:=item;
end;     }

// First
//
function TFasterTList.First : Pointer;
asm
   mov   edx, [eax+cListOffset]  // precharge FList in edx
   mov   ecx, [eax+cCountOffset]
   test  ecx, ecx
   jnz   @@Return
   xor   edx, edx
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps

@@Return:
   mov   eax, [edx]
end;

// First
//
function TFasterTList.Last : Pointer;
asm
   mov   edx, [eax+cListOffset]  // precharge FList in edx
   mov   ecx, [eax+cCountOffset]
   test  ecx, ecx
   jnz   @@Return
   lea   edx, [ecx-1]
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps

@@Return:
   mov   eax, [edx+ecx*4-4]
end;

// Exchange
//
procedure TFasterTList.Exchange(index1, index2 : Integer);
asm
   push  ebx

   mov   ebx, [eax+cListOffset]     // EBX gets FList
   cmp   edx, [eax+cCountOffset]
   jb    @@Index1_Ok
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps

@@Index1_Ok:
   lea   edx, [ebx+edx*4]           // EDX gets @FList[index1]
   cmp   ecx, [eax+cCountOffset]
   jb    @@Index2_Ok
   mov   edx, ecx
   call  [vIndexErrorMethod]  // our code here is relocatable, must use absolute jumps

@@Index2_Ok:
   lea   ecx, [ebx+ecx*4]           // ECX gets @FList[index2]
   mov   ebx, [edx]
   mov   eax, [ecx]
   mov   [ecx], ebx
   mov   [edx], eax

   pop   ebx
end;

// Pack
//
procedure TFasterTList.Pack;
var                         
   i, j, n : Integer;
   p : PPointerList;
   pk : PPointer;
begin
   p:=List;
   n:=Count-1;
   while (n>=0) and (p[n]=nil) do Dec(n);
   for i:=0 to n do begin
      if p[i]=nil then begin
         pk:=@p[i];
         for j:=i+1 to n do begin
            if p[j]<>nil then begin
               pk^:=p[j];
               Inc(pk);
            end;
         end;
         SetFCount((Integer(pk)-Integer(p)) shr 2);
         Exit;
      end;
   end;
   SetFCount(n+1);
end;

// IndexOf
//
function TFasterTList.IndexOf(item : Pointer) : Integer;
asm
   mov   ecx, [eax+cCountOffset]
   or    ecx, ecx
   jz    @@NotFound

@@Search:
   push  edi

   mov   edi, [eax+cListOffset]
   mov   eax, edx
   mov   edx, ecx

   repne scasd

   pop   edi

   je    @@FoundIt

@@NotFound:
   xor   eax, eax
   dec   eax
   ret

@@FoundIt:
   sub   edx, ecx
   dec   edx
   mov   eax, edx
end;

// SetCount
//
procedure TFasterTList.SetCount(newCount : Integer);
var
   i : Integer;
begin
   if Cardinal(newCount)>Cardinal(MaxListSize) then
      Error(@SListCountError, newCount);
   if newCount>Capacity then begin
      SetCapacity(newCount);
      FillZeroes32(newCount-Count, @List[Count]);
   end else begin
      if newCount>Count then
         FillZeroes32(newCount-Count, @List[Count])
      else if NotifyOverriden then
         for i:=Count-1 downto newCount do
            Delete(i);
   end;
   SetFCount(newCount);
end;

// Move
//
procedure TFasterTList.Move(curIndex, newIndex : Integer);
var
   item : Pointer;
begin
   if curIndex<>newIndex then begin
      if Cardinal(newIndex)>=Cardinal(Count) then
         IndexError(newIndex);
      if Cardinal(curIndex)>=Cardinal(Count) then
         IndexError(curIndex);
      item:=List[curIndex];
      if curIndex<newIndex then begin
         // curIndex+1 necessarily exists since curIndex<newIndex and newIndex<Count
         System.Move(List[curIndex+1], List[curIndex], (newIndex-curIndex-1)*SizeOf(Pointer));
      end else begin
         // newIndex+1 necessarily exists since newIndex<curIndex and curIndex<Count
         System.Move(List[newIndex], List[newIndex+1], (curIndex-newIndex-1)*SizeOf(Pointer));
      end;
      List[newIndex]:=item;
   end;
end;

// PatchVCL
//
procedure PatchVCL;

   procedure Redirect(oldRoutine, newRoutine : Pointer);
   var
      oldProtect, protect : Cardinal;
   begin
      VirtualProtect(oldRoutine, 256, PAGE_READWRITE, @oldProtect);
      PByte(oldRoutine)^:=$E9;
      PInteger(Integer(oldRoutine)+1)^:=Integer(newRoutine)-Integer(oldRoutine)-5;
      VirtualProtect(oldRoutine, 2048, oldProtect, @protect);
   end;

   procedure Replace(oldRoutine, newRoutine : Pointer; length, maxLength : Integer);
   var
      oldProtect, protect : Cardinal;
   begin
      if    (PChar(oldRoutine)^=#$FF)   // BPL linked, can't replace
         or (Cardinal(length)>Cardinal(maxLength)) then     // not enough space
         Redirect(oldRoutine, newRoutine)
      else begin
         VirtualProtect(oldRoutine, length+256, PAGE_READWRITE, @oldProtect);
         Move(newRoutine^, oldRoutine^, length);
         VirtualProtect(oldRoutine, length+256, oldProtect, @protect);
      end;
   end;

begin
   Redirect(@TAddrTList.SetCount,   @TFasterTList.SetCount);
   Replace (@TAddrTList.Add,        @TFasterTList.Add,
            Integer(@TFasterTList.Get)-Integer(@TFasterTList.Add),
            Integer(@TAddrTList.Clear)-Integer(@TAddrTList.Add));
   Replace (@TAddrTList.Get,        @TFasterTList.Get,
            Integer(@TFasterTList.Put)-Integer(@TFasterTList.Get),
            Integer(@TAddrTList.Grow)-Integer(@TAddrTList.Get));
   Replace (@TAddrTList.Put,        @TFasterTList.Put,
            Integer(@TFasterTList.First)-Integer(@TFasterTList.Put),
            Integer(@TAddrTList.Remove)-Integer(@TAddrTList.Put));
   Redirect(@TAddrTList.First,      @TFasterTList.First);
   Redirect(@TAddrTList.Last,       @TFasterTList.Last);
   Replace (@TAddrTList.Exchange,   @TFasterTList.Exchange,
            Integer(@TFasterTList.Pack)-Integer(@TFasterTList.Exchange),
            Integer(@TAddrTList.Expand)-Integer(@TAddrTList.Exchange));
   Redirect(@TAddrTList.Pack,       @TFasterTList.Pack);
   Redirect(@TAddrTList.IndexOf,    @TFasterTList.IndexOf);
   Redirect(@TAddrTList.Move,       @TFasterTList.Move);    
end;

procedure Init;
var
   list : TFasterTList;
begin
   // get Notify address, for override detection
   list:=TFasterTList.Create;
   vDefaultTListNotifyAddr:=list.NotifyAddress;
   if PChar(vDefaultTListNotifyAddr)^=#$FF then begin
      // points to jump table, so get the real deal instead
      // retrieve the indirect jump's address
      vDefaultTListNotifyAddr:=PPointer(Integer(vDefaultTListNotifyAddr)+2)^;
      // retrieve the address (in the BPL)
      vDefaultTListNotifyAddr:=PPointer(vDefaultTListNotifyAddr^);
   end;
   vIndexErrorMethod:=@TFasterTList.IndexError;
   list.Free;
   // redirect TList methods to TFasterTList methods
   PatchVCL;
end;

initialization

   Init;

end.

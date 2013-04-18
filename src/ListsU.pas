unit ListsU;
interface
uses
  classes;

type
  TStack = class
  Protected
    fList : array of pointer;
    fcount : LongInt;

  Public
    Constructor Create(aCapacity : cardinal = 16);

    Procedure Grow; overload;
    Procedure Grow( NewSize : Integer ); overload;

    Procedure Clear;
    Procedure Push(aptr : pointer);
    Function Pop : pointer;
    Function Examine : pointer;
    // the number of items on the stack
    Property Count : LongInt read fCount;
  end; {TStack}
  
  TQueue = class
  Protected
    fList : array of pointer;
    fTail,fhead : LongInt;
    fcount : LongInt;
    Procedure Grow; overload;
    Procedure Grow(NewSize : integer); overload;
  public
    Constructor Create(aCapacity : cardinal = 16);
    // add an item to the end of the queue
    Procedure EnQueue(const p : pointer);
    // remove an item from front of the queue
    Function DeQueue : pointer;
    // examine the item at the front of the queue
    Function Examine : pointer;

    Function Last : pointer;
    // clears the queue
    procedure Clear;
    // the number of items in the queue
    Property Count : LongInt read fCount;
  end; {TQueue}


  TStringQueue = class
  Protected
    fList : array of string;
    fTail,fhead : LongInt;
    fcount : LongInt;
    Procedure Grow;  
  public
    Constructor Create(aCapacity : cardinal = 16);
    // add an item to the end of the queue
    Procedure EnQueue(const p : string);
    // remove an item from front of the queue
    Function DeQueue : string;
    // examine the item at the front of the queue
    Function Examine : string;

    Function Last : string;
    // clears the queue
    procedure Clear;
    // the number of items in the queue
    Property Count : LongInt read fCount;
  end; {TStringQueue}

  TObjectQueue = class
  Protected
    fList : array of TObject;
    fTail,fhead : LongInt;
    fcount : LongInt;
    Procedure Grow;  virtual;
  public
    Constructor Create(aCapacity : cardinal = 16);
    // add an item to the end of the queue
    Procedure EnQueue(p : TObject);
    // remove an item from front of the queue
    Function DeQueue : TObject;
    // examine the item at the front of the queue
    Function Examine : TObject;

    Function Last : TObject;
    // clears the queue
    procedure Clear;
    // the number of items in the queue
    Property Count : LongInt read fCount;
  end; {TObjectQueue}

implementation
uses
  sysutils;

//------------------------------------------------------------------------------
//   TStack
//------------------------------------------------------------------------------

Constructor TStack.Create(aCapacity : cardinal = 16);
begin
Inherited create;
if aCapacity < 1 then
  aCapacity := 1;
// pre pad initial items
SetLength( flist, aCapacity );
end; {Create}

Procedure TStack.Grow;
begin
// grow the list by ~50%
SetLength(flist, (Length(flist) * 3) div 2 - 1 );
end; {Grow}

Procedure TStack.Grow( NewSize : Integer );
begin
if NewSize > length(flist) then
  SetLength(flist, NewSize );
end; {Grow}

Procedure TStack.Clear;
begin
fcount := 0;
end; {Clear}

Procedure TStack.Push(aptr : pointer);
begin
flist[fcount] := aptr;
inc(fcount);
if fcount >= length(flist) then
  Grow;
end; {Push}

Function TStack.Pop : pointer;
begin
if fcount <> 0 then
  begin
  dec(fcount);
  Result := flist[fcount];
  end
else
  result := nil;  
end; {Pop}

Function TStack.Examine : pointer;
begin
if fcount <> 0 then
  Result := flist[fcount-1]
else
  Result := nil;  
end; {Examine}
  
//------------------------------------------------------------------------------
//   TQueue
//------------------------------------------------------------------------------

Constructor TQueue.Create(aCapacity : cardinal = 16);
begin
Inherited create;
if aCapacity < 1 then
  aCapacity := 1;
// pre pad initial items
SetLength( flist, aCapacity );
end; {Create}

procedure TQueue.Clear;
begin
fCount := 0;
ftail := 0;
fHead := 0;
end; {Clear}

Procedure TQueue.Grow(NewSize : integer);
var
  Index,ToInx : cardinal;
//  Size: longword;
begin
SetLength(flist, NewSize );
if Fhead = 0 then
  ftail := Count
else
  begin
  ToInx := Length(flist);
  For index := pred(Count) downto Fhead do
    begin
    dec(ToInx);
    flist[ToInx] := flist[index];
    end;
  Fhead := ToInx;
  end;
end;

Procedure TQueue.Grow;
begin
// grow the list by ~50%
Grow((Length(flist) * 3) div 2 - 1 )
end; {Grow}

Procedure TQueue.EnQueue(const p : pointer);
begin
flist[Ftail] := p;
//Ftail := (Ftail + 1) mod length(fList);
inc(Ftail);
if Ftail >= length(fList) then
  Ftail := 0;
inc(Fcount);
if Ftail = FHead then
  Grow;
end; {EnQueue}

Function TQueue.DeQueue : pointer;
begin
if Count <> 0 then
  begin
  Result := flist[FHead];
  flist[fhead] := nil;
  inc(fHead);
  if fHead >= length(fList) then
    fHead := 0;
//    FHead := (FHead + 1) mod length(fList);
  dec(fCount);
  end
else
  Result := nil;
end; {DeQueue}

Function TQueue.Examine : pointer;
begin
if Count <> 0 then
  Result := flist[fHead]
else
  Result := nil;
end; {Examine}

Function TQueue.Last : pointer;
begin
if Count <> 0 then
  Result := flist[fTail-1]
else
  Result := nil;
end; {Last}

//------------------------------------------------------------------------------
//   TStringQueue
//------------------------------------------------------------------------------

Constructor TStringQueue.Create(aCapacity : cardinal = 16);
begin
Inherited create;
if aCapacity < 1 then
  aCapacity := 1;
// pre pad initial items
SetLength( flist, aCapacity );
end; {Create}

procedure TStringQueue.Clear;
begin
fCount := 0;
ftail := 0;
fHead := 0;
end; {Clear}

Procedure TStringQueue.Grow;
var
  Index,ToInx : cardinal;
//  Size: longword;
begin
// grow the list by ~50%
SetLength(flist, (Length(flist) * 3) div 2 - 1 );
if Fhead = 0 then
  ftail := Count
else
  begin
  ToInx := Length(flist);
  For index := pred(Count) downto Fhead do
    begin
    dec(ToInx);
    flist[ToInx] := flist[index];
    end;
  Fhead := ToInx;
  end;
end; {Grow}

Procedure TStringQueue.EnQueue(const p : string);
begin
flist[Ftail] := p;
//Ftail := (Ftail + 1) mod length(fList);
inc(Ftail);
if Ftail >= length(fList) then
  Ftail := 0;
inc(Fcount);
if Ftail = FHead then
  Grow;
end; {EnQueue}

Function TStringQueue.DeQueue : string;
begin
if Count <> 0 then
  begin
  Result := flist[FHead];
  flist[fhead] := '';
  inc(fHead);
  if fHead >= length(fList) then
    fHead := 0;
//    FHead := (FHead + 1) mod length(fList);
  dec(fCount);
  end
else
  Result := '';
end; {DeQueue}

Function TStringQueue.Examine : string;
begin
if Count <> 0 then
  Result := flist[fHead]
else
  Result := '';
end; {Examine}

Function TStringQueue.Last : string;
begin
if Count <> 0 then
  Result := flist[fTail-1]
else
  Result := '';
end; {Last}

//------------------------------------------------------------------------------
//   TObjectQueue
//------------------------------------------------------------------------------

Constructor TObjectQueue.Create(aCapacity : cardinal = 16);
begin
Inherited create;
if aCapacity < 1 then
  aCapacity := 1;
// pre pad initial items
SetLength( flist, aCapacity );
end; {Create}

procedure TObjectQueue.Clear;
begin
fCount := 0;
ftail := 0;
fHead := 0;
end; {Clear}

Procedure TObjectQueue.Grow;
var
  Index,ToInx : cardinal;
//  Size: longword;
begin
// grow the list by ~50%
SetLength(flist, (Length(flist) * 3) div 2 - 1 );
if Fhead = 0 then
  ftail := Count
else
  begin
  ToInx := Length(flist);
  For index := pred(Count) downto Fhead do
    begin
    dec(ToInx);
    flist[ToInx] := flist[index];
    end;
  Fhead := ToInx;
  end;
end; {Grow}

Procedure TObjectQueue.EnQueue(p : TObject);
begin
flist[Ftail] := p;
//Ftail := (Ftail + 1) mod length(fList);
inc(Ftail);
if Ftail >= length(fList) then
  Ftail := 0;
inc(Fcount);
if Ftail = FHead then
  Grow;
end; {EnQueue}

Function TObjectQueue.DeQueue : TObject;
begin
if Count <> 0 then
  begin
  Result := flist[FHead];
  flist[fhead] := nil;
  inc(fHead);
  if fHead >= length(fList) then
    fHead := 0;
//    FHead := (FHead + 1) mod length(fList);
  dec(fCount);
  end
else
  Result := nil;
end; {DeQueue}

Function TObjectQueue.Examine : TObject;
begin
if Count <> 0 then
  Result := flist[fHead]
else
  Result := nil;
end; {Examine}

Function TObjectQueue.Last : TObject;
begin
if Count <> 0 then
  Result := flist[fTail-1]
else
  Result := nil;
end; {Last}

end.


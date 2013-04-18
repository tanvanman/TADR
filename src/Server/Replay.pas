unit replay;

interface

uses
  DPlay, Classes, Forms;

type
  TEntry = class
    Data :pointer;
    Size :integer;
    Timing :integer;
    Next :TEntry;
    constructor Create (Data :pointer; Size :integer; Timing :integer);
  end;

type
  TReplay = class
    Entries :TEntry;
    Current :TEntry;
//    dp :IDirectPlay3;
//    from :TDPID;
    Total :integer;
    constructor Create;
    procedure ReadLog (name :string);
    procedure Send;
    function MoreData :boolean;
    function TheData :TStringList;
    function GetNext :string;    
  end;

implementation

uses
  TextData, SysUtils, main, logging;

constructor TEntry.Create (Data :pointer; Size :integer; Timing :integer);
begin
  inherited Create;

  Self.Data := Data;
  Self.Size := size;

  Self.Timing := timing;
  Self.Next := nil;
end;

{--------------------------------------------------------------------}

constructor TReplay.Create;
begin
  inherited Create;
  Entries := nil;
end;

procedure TReplay.ReadLog (name :string);
var
  t :TextFile;
  s :string;
  state :integer;
  tmp, cur :TEntry;
begin
  AssignFile (t, name);
  Reset (t);

  total := 0;
  state := 0;
  while not eof (t) do
  begin
    ReadLn (t, s);

    case state of
      0  :if Pos ('!IDPLAY(23).Send', s) > 0 then
//      0  :if Pos ('!IDPLAY2(3).Receive', s) > 0 then
            state := 1;
      1  :begin
            if (Pos ('+ data', s) > 0) or
               (Pos ('+ lpData', s) > 0) then
            begin
              tmp := TEntry.Create (StrToData (Copy (s, 19, 6000)), DataSize (Copy (s, 19, 6000)), 0);

  //            Log.Add ('damn' + DataToStr(tmp.Data, tmp.size));
              if (Entries = nil) then
                Entries := tmp
              else
                cur.Next := tmp;

              Inc(total);
              cur := tmp;
  //            state := 0;
            end;
            if Pos ('!IDPLAY', s) > 0 then
              if Pos ('!IDPLAY(23).Send', s) = 0 then
                state := 0;
          end;
    end;
  end;

  Current := Entries;
  CloseFile (t);
end;

procedure TReplay.Send;
var
  x :TDateTime;

begin
  if Current = nil then
    exit;


//  fmMain.lbTest.Caption := DataToStr (Current.Data, Current.Size);

//  fmMain.lbSend.ItemIndex := fmMain.lbSend.ItemIndex + 1;
  Current := Current.Next;
end;

function TReplay.GetNext :string;
begin
  Result := PtrToStr (Current.Data, Current.Size);
  Current := Current.Next;
end;

function TReplay.MoreData :boolean;
begin
  Result := Current <> nil;
end;

function TReplay.TheData :TStringList;
var
  tmp :TEntry;
begin
  Result := TStringList.Create;

  tmp := Entries;
  while tmp <> nil do
  begin
    Result.Add (IntToStr (tmp.Size) + ' - ' + DataToStr (tmp.data, tmp.size));
    tmp := tmp.next;
  end;
end;

end.

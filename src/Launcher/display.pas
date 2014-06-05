{

    The TA Launcher 1.x - Display unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit display;

interface

uses
  Windows, SysUtils, Classes,{ Controls, Forms, StdCtrls,
  INIFiles, Registry, Dialogs,} ShellAPI{, ExtCtrls};

procedure EnumerateDisplayModes;
function CompareResolutions(List: TStringList; Index1, Index2: Integer): Integer;

implementation
uses settings;

procedure EnumerateDisplayModes;
var
  DM: TDevMode;
  ModeNum: Longint;
  slDisplayModes: TStringList;
  notDone: boolean;
begin
  ModeNum := 0;
  slDisplayModes := TStringList.Create;
  try
    notdone := EnumDisplaySettings(nil, ModeNum, DM);
    slDisplayModes.Sorted := True;
    slDisplayModes.Duplicates := dupIgnore;
    slDisplayModes.Add(Format('%dx%d', [DM.dmPelsWidth, DM.dmPelsHeight]));
    while notDone do
    begin
      Inc(ModeNum);
      notdone := EnumDisplaySettings(nil, ModeNum, DM);
      slDisplayModes.Add(Format('%dx%d', [DM.dmPelsWidth, DM.dmPelsHeight]));
    end;
    slDisplayModes.Sorted := False;
    slDisplayModes.CustomSort(CompareResolutions);
    fmSettings.cbDisplayModes.Items.Assign(slDisplayModes);
  finally
    slDisplayModes.Free;
  end;
end;

function CompareResolutions(List: TStringList; Index1, Index2: Integer): Integer;
var
  s1, s2: string;
  d1x, d2x: Integer;
begin
  Result:= 0;
  s1:= List[Index1];
  s2:= List[Index2];

  d1x := StrToInt(copy(s1,1,pos('x',s1)-1));
  d2x := StrToInt(copy(s2,1,pos('x',s2)-1));

  if d1x < d2x then
      Result := -1
  else if d1x > d2x then
      Result := 1;
end;

end.

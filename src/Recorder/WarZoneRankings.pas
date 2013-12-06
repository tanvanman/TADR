unit WarZoneRankings;

interface

uses
  SysUtils, Windows, Classes, IdHttp, IdAntiFreeze, Forms, DateUtils;

type TPlayer = record
  Name: string[20];
  RankName: string[20];
  Rank: single;
  BestTeamMate: string[40];
  WorstTeamMate: string[40];
end;

const
  NONE              = $00; //Zero-value number
  INET_USERAGENT    = 'Mozilla/4.0, Total Annihilation Unofficial Patch (Windows; en-US)';
  INET_REDIRECT_MAX = 10;

var
  PlayersRankings: array of TPlayer;
  SearchRankResult: array of TPlayer;
  
function DownloadParseRankFile(pUrl,pFileName: pChar): byte;
function SearchRanks(name: String; RankType: byte; out ResultString: String): boolean;

implementation

uses
  StrUtils;

function FindWordInString(sWordToFind, sTheString: String): Integer;
begin
  Result := Pos(sWordToFind, sTheString);
  if Result > 0 then
    Inc(Result);
end;

function ForceDeleteFile(pFileName: PAnsiChar): boolean;
begin
 Windows.SetFileAttributes(pFileName,NONE); //clear file attributes
 Result:= Windows.DeleteFile(pFileName);     //then delete the file
end;

function DownloadParseRankFile(pUrl,pFileName: PChar): byte;
var
  fs: TFileStream;
  DateTimeStamp: integer;
  CurrentDateTime, FileDateTime: TDateTime;
  slRanks: TStringList;
  i: Integer;
  countp: byte;
  posit, posit2: integer;
  m: Int64;
  af: TIdAntiFreeze;
begin
  Result:= 0;
  if (pUrl=nil) or (pFileName=nil) then  //Check arguments
    Exit;
  DateTimeStamp := FileAge(pFileName);
  if DateTimeStamp > -1 then
  begin
    CurrentDateTime:= Now;
    FileDateTime := FileDateToDateTime(DateTimeStamp);
    m:= MinutesBetween(FileDateTime, CurrentDateTime);
    if m >= 5 then
    begin
      ForceDeleteFile(pFileName);       //Delete existing file
      try
        fs:= TFileStream.Create(pFileName,fmCreate); //Create file stream
      except
        Exit;
      end;
        Result:= 1;
      af:= TIdAntiFreeze.Create(nil);
      af.Active:= True;
      with TIdHttp.Create(nil) do    //Create http object
      begin
        Request.UserAgent:= INET_USERAGENT;                             //Define user agent
        RedirectMaximum:= INET_REDIRECT_MAX;                            //Redirect maxumum
        HandleRedirects:= INET_REDIRECT_MAX <> NONE;                      //Handle redirects
        try
          if Assigned(fs) then
            Get(pUrl,fs);     //Do the request
          Application.ProcessMessages;
        except
        end;
        Free;                                                           //Free the http object
      end;
      af.Active:= False;
      af.Destroy;
    end;
  end;

  if Assigned(fs) then
  begin
    if fs.Size > NONE then
      Result:= 2;
    fs.Free;                           //Free the file stream
  end else
    if FileExists(pFileName) then
      Result:= 2
    else
      Exit;

  if Result > 0 then
  begin
    slRanks := TStringList.Create;
    slRanks.LoadFromFile(pFileName);
    countp:= 0;
    SetLength(PlayersRankings, countp);
    for i := 0 to slRanks.Count - 1 do
    begin
      if Pos('<td onmouseover="javascript: gs_onmouseover(', slRanks[i]) <> 0 then
     //found player
      begin
        Inc(countp);
        SetLength(PlayersRankings, countp);

        posit:= Pos(';">', slRanks[i]) + 3;
        posit2:= PosEx('</s', slRanks[i], posit);
        PlayersRankings[countp-1].Name:= Copy(slRanks[i], posit, posit2-posit);

        posit:= posit2 + 45;
        posit2:= PosEx('</s', slRanks[i], posit);
        PlayersRankings[countp-1].RankName := Copy(slRanks[i], posit, posit2-posit);

        posit:= posit2 + 45;
        posit2:= PosEx('</s', slRanks[i], posit);
        PlayersRankings[countp-1].Rank := StrToFloat(StringReplace(Copy(slRanks[i], posit, posit2-posit), '.', ',', [rfReplaceAll]));
        //form1.Memo1.Lines.add(inttostr(countp) + ': ' + Players[countp-1].Name + ', ' + Players[countp-1].RankName + ', ' + FloatToStrF(Players[countp-1].Rank, ffFixed, 4, 2));
      end;
    end;
    if countp > 0 then Result:= 3;
  end;
end;

function SearchRanks(name: String; RankType: byte; out ResultString: String): boolean;
var
  i: integer;
  foundpos, foundcount: integer;
  foundid: integer;
begin
  Result:= False;
  foundpos:= 0;
  foundcount:= 0;
  foundid:= -1;
  ResultString:= '';
  if Assigned(PlayersRankings) then
  begin
      for i:= Low(PlayersRankings) to High(PlayersRankings) do
      begin
        foundpos:= Pos(LowerCase(name), LowerCase(PlayersRankings[i].Name));
        if foundpos > 0 then
        begin
          Inc(foundcount);
          if i = High(PlayersRankings) then
          begin
            foundid:= i;
            Break;
          end else
          begin
            ResultString:= PlayersRankings[i].Name + ', ' + ResultString;
            foundid := i;
            Continue;
          end;
        end;
      end;

      if foundcount = 0 then Exit;

      if foundcount > 1 then
      begin
        if foundcount < 5 then
          ResultString:= 'More than one player found. Possible names: '+ Copy(ResultString, 1, Length(ResultString) -2)
        else
          ResultString:= 'Too much results.';
        Result:= True;
        Exit;
      end;

      case RankType of
        1 : begin
              ResultString:= 'Current rank for ' + PlayersRankings[foundid].Name + ': ' + IntToStr(foundid+1) + '. ' +
                            PlayersRankings[foundid].RankName + ' [' + FloatToStrF(PlayersRankings[foundid].Rank, ffFixed, 4, 3) +']';
            end;
        2 : begin
              ResultString:= 'Best teammate for ' + PlayersRankings[foundid].Name + ': ' + PlayersRankings[foundid].BestTeamMate;
            end;
        3 : begin
              ResultString:= 'Worst teammate for ' + PlayersRankings[foundid].Name + ': ' + PlayersRankings[foundid].WorstTeamMate;
            end;
      end;

      Result:= True;
  end;
end;

end.

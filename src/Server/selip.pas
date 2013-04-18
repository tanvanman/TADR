unit selip;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls, StdCtrls, DPlay, DPLobby, ActiveX, tasv, textdata;

type
  TfmSelIP = class(TForm)
    lbAddresses: TListBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Button1: TButton;
    Button2: TButton;
    Bevel1: TBevel;
    procedure FormActivate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    function GetIp (dp :IDirectPlay3; server :TAServer) :string;
    function GetAddress (s :string) :string;    
  end;

var
  fmSelIP: TfmSelIP;

implementation

{$R *.DFM}

function EnumAddress (const guidDataType: TGUID;
      dwDataSize: DWORD; lpData: Pointer; lpContext: Pointer) : BOOL; stdcall;
var
  s :TStrings;
  p :Pchar;
begin
  s := lpcontext;
  if IsSameGuid (guidDataType, DPAID_INet) then
  begin
    p := lpdata;
    s.Add (p);
  end;
end;

function TfmSelIP.GetIp (dp :IDirectPlay3; server :TAServer):string;
var
  buf :pchar;
  bufsize :cardinal;
  dl1 :IDirectPlayLobby;
  dl :IDirectPlayLobby2;
begin
  DirectPlayLobbyCreate (@GUID_NULL, dl1, nil, nil, 0);
  dl1.QueryInterface (IID_IDirectPlayLobby2, dl);

  GetMem(buf, 1000);
  bufsize := 1000;
  server.test(dp.GetPlayerAddress (server.drones [1], buf, bufsize));

  lbAddresses.Items.Clear;

  server.test(dl.EnumAddress (EnumAddress, buf^, bufsize, lbAddresses.Items));

//  lbaddresses.items.add ('svenne');

  if lbAddresses.Items.Count = 0 then
  begin
    MessageDlg ('Your computer does not appear to have an IP address. Please see the readme.txt for further information', mtError, [mbok], 0);
    Result := 'localhost';
    exit;
  end;

  if lbAddresses.Items.Count = 1 then
  begin
    Result := lbaddresses.items[0];
    exit;
  end;

  lbAddresses.ItemIndex := 1;
  PostMessage (lbaddresses.handle, WM_KEYDOWN, vk_up, 0);

  if ShowModal = mrOk then
  begin
    if lbaddresses.itemindex = -1 then
      Result := ''
    else
      Result := lbaddresses.Items[lbaddresses.itemindex];
  end else
    Result := '';
end;

procedure TfmSelIP.FormActivate(Sender: TObject);
begin
  lbAddresses.SetFocus;
end;

//S är en getaddress-buffer
function TfmSelIP.GetAddress (s :string) :string;
var
  i :integer;
  dl1 :IDirectPlayLobby;
  dl :IDirectPlayLobby2;
begin
  Result := '';

  DirectPlayLobbyCreate (@GUID_NULL, dl1, nil, nil, 0);
  dl1.QueryInterface (IID_IDirectPlayLobby2, dl);

  lbAddresses.Items.Clear;
  if dl.EnumAddress (EnumAddress, s[1], Length (s), lbAddresses.Items) = DP_OK then
  begin
    for i := 1 to lbaddresses.items.count do
    begin
      Result := Result + lbaddresses.items [i - 1] + ' ';
    end;
  end;
end;

end.

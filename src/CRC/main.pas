unit main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    procedure Button1Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

function calccrc :longword;
var
  s :TFileStream;
  b :pchar;
  i :integer;
  cur :longword;
  fto :^longword;
begin
  s := TFileStream.Create ('c:\games\totala\dplayx.dll', fmOpenReadWrite );

  getmem (b, s.size);
  s.Read (b^, s.size);
  cur := 0;

  i := 0;
  repeat
    fto := @b[i];
    cur := cur xor fto^;
    inc (i, 4);
  until i > s.size - 6;

  s.seek (s.size, soFromBeginning);


  showmessage (inttostr (s.position) + ' - ' + inttostr (s.size));
  s.Write (cur, 4);
  showmessage (inttostr (s.position) + ' - ' + inttostr (s.size));
{  fto := @b[s.size - 4];

  if fto^ <> cur then
  begin
    log.add ('hey man');
  end;}

  result := cur;
  freemem (b, s.size);
  s.free;
end;


procedure TForm1.Button1Click(Sender: TObject);
begin
  showmessage (inttostr (calccrc));
end;

end.

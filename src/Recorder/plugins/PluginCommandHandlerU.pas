unit PluginCommandHandlerU;

interface
uses
  classes;

Type
  // return true to all TA to process the string(as a command),
  // otherwise to prevent TA from seeing the string
  TCommandHandlerEvent = function ( const Command : string;
                                    params : TStringList ) : Boolean of object;

  TCommandHandler = class
  protected
    fName : string;
    fHelpInfo : string;
    fSyntax : string;
    fRequiredParams : Integer;
    fIsServerOnly : boolean;
    fIsSelfOnly : Boolean;
    fCommandHandler : TCommandHandlerEvent;
    fIsDevModeOnly : Boolean;
    fRequireCompatibleTA : Boolean;
  public
    constructor Create( const aName : string;
                        const aHelpInfo : string;
                        const aSyntax : string;
                        aRequiredParams : Integer;
                        aIsServer : Boolean;
                        aIsSelfOnly : Boolean;
                        aRequireCompatibleTA : Boolean;
                        aIsDevModeOnly : Boolean;                        
                        aCommandHandler : TCommandHandlerEvent );


    property Name : string read fName;
    // the required syntax for any params
    property Syntax : string read fSyntax;

    property RequiredParams : Integer read fRequiredParams;
    // #10 is inteperated as a new line, will wrap as required
    property HelpInfo : string read fHelpInfo;
    property IsServerOnly : Boolean read fIsServerOnly;
    property IsSelfOnly : Boolean read fIsSelfOnly;
    property RequireCompatibleTA : Boolean read fRequireCompatibleTA;
    property IsDevModeOnly : Boolean read fIsDevModeOnly;
    property CommandHandler : TCommandHandlerEvent read fCommandHandler;
  end; {TCommandHandler}


  TPluginCommands = class
  protected
    fCommands : TStringList;
    function GetCount : Integer;
    function GetItem(i : Integer) : TCommandHandler;
  public
    constructor Create;
    destructor destroy; override;

    function LookUpCommand( const aName : string ) : TCommandHandler;

    procedure ClearCommands;
    procedure RemoveCommand( const aName : string );
    procedure AddCommand( const aName : string;
                          const aHelpInfo : string;
                          const aSyntax : string;
                          aRequiredParams : Integer;
                          aIsServer : Boolean;
                          aIsSelfOnly : Boolean;
                          aRequireCompatibleTA : Boolean;
                          aIsDevModeOnly : Boolean;
                          aCommandHandler : TCommandHandlerEvent );
    procedure AddCommandAlias( const aName : string;
                               const aNewAlias : string );

    property Count : Integer read GetCount;
    property Items[ i : integer] : TCommandHandler read GetItem; default;
  end;

implementation
uses
  sysutils;

// -----------------------------------------------------------------------------
//  TCommandHandler
// -----------------------------------------------------------------------------

constructor TCommandHandler.Create( const aName : string;
                                    const aHelpInfo : string;
                                    const aSyntax : string;
                                    aRequiredParams : Integer;
                                    aIsServer : Boolean;
                                    aIsSelfOnly : Boolean;
                                    aRequireCompatibleTA : Boolean;
                                    aIsDevModeOnly : Boolean;
                                    aCommandHandler : TCommandHandlerEvent );
begin
fName := aName;
fHelpInfo := aHelpInfo;
fSyntax := aSyntax;
fRequiredParams := aRequiredParams;
fIsServerOnly := aIsServer;
fIsSelfOnly := aIsSelfOnly;
fRequireCompatibleTA := aRequireCompatibleTA;
fIsDevModeOnly := aIsDevModeOnly;
fCommandHandler := aCommandHandler;
end; {Create}

// -----------------------------------------------------------------------------
//  TPluginCommands
// -----------------------------------------------------------------------------

constructor TPluginCommands.Create;
begin
inherited;
fCommands := TStringList.create;
fCommands.Sorted := True;
fCommands.Duplicates := dupError;
end; {Create}

destructor TPluginCommands.destroy;
begin
ClearCommands;
FreeAndNil(fCommands);
inherited;
end; {destroy}

function TPluginCommands.GetCount : Integer;
begin
if fCommands <> nil then
  Result := fCommands.Count
else
  Result := 0;
end; {GetCount}

function TPluginCommands.GetItem(i : Integer) : TCommandHandler;
begin
if fCommands <> nil then
  Result := TCommandHandler( fCommands.objects[i] )
else
  Result := nil;
end; {GetItem}

procedure TPluginCommands.ClearCommands;
var i : Integer;
begin
if fCommands = nil then Exit;
for i := 0 to fCommands.Count-1 do
  fCommands.Objects[i].Free;
fCommands.Clear;
end; {ClearCommands}

procedure TPluginCommands.RemoveCommand( const aName : string );
var
  i : Integer;
  CommandHandler : TCommandHandler;
begin
if fCommands = nil then Exit;
if fCommands.Find(aName, i) then
  begin
  CommandHandler := TCommandHandler(fCommands.objects[i]);
  try
    fCommands.Delete( i );
  finally
    CommandHandler.Free;
  end;
  end;
end; {RemoveCommand}

function TPluginCommands.LookUpCommand( const aName : string ) : TCommandHandler;
var i : Integer;
begin
if fCommands.Find(aName, i) then
  Result := TCommandHandler( fCommands.Objects[i] )
else
  Result := nil;
end; {LookUpCommand}

procedure TPluginCommands.AddCommandAlias( const aName : string;
                                     const aNewAlias : string );

var
  ReturnAddr: Pointer;
  i : Integer;
  CommandHandler : TCommandHandler;
  AliasCommandHandler : TCommandHandler;
begin
if fCommands = nil then Exit;
CommandHandler := LookUpCommand( aName );
if CommandHandler <> nil then
  begin
  if not fCommands.Find(aNewAlias, i) then
    begin
    AliasCommandHandler := TCommandHandler.Create( aNewAlias,
                                                   CommandHandler.HelpInfo,
                                                   CommandHandler.Syntax,
                                                   CommandHandler.RequiredParams,
                                                   CommandHandler.IsServerOnly,
                                                   CommandHandler.IsSelfOnly,
                                                   CommandHandler.RequireCompatibleTA,
                                                   CommandHandler.IsDevModeOnly,
                                                   CommandHandler.CommandHandler );
    try
      fCommands.AddObject( aNewAlias, AliasCommandHandler );
    except
      AliasCommandHandler.Free;
      raise;
    end;
    end
  else
    begin
    asm
      push eax;
      MOV EAX,[EBP+4];
      Mov ReturnAddr, EAX;
      pop eax;
    end;
    raise Exception.Create('Duplicate command '+aName) at ReturnAddr;
    end;
  end;
end; {AddCommandAlias}

procedure TPluginCommands.AddCommand( const aName : string;
                                const aHelpInfo : string;
                                const aSyntax : string;
                                aRequiredParams : Integer;
                                aIsServer : Boolean;
                                aIsSelfOnly : Boolean;
                                aRequireCompatibleTA : Boolean;
                                aIsDevModeOnly : Boolean;
                                aCommandHandler : TCommandHandlerEvent);

var
  ReturnAddr: Pointer;
  i : Integer;
  CommandHandler : TCommandHandler;
begin
if fCommands = nil then Exit;
if not fCommands.Find(aName, i) then
  begin
  CommandHandler := TCommandHandler.Create( aName, aHelpInfo, aSyntax, aRequiredParams,
                                            aIsServer, aIsSelfOnly, aRequireCompatibleTA, aIsDevModeOnly,
                                            aCommandHandler  );
  try
    fCommands.AddObject( aName, CommandHandler );
  except
    CommandHandler.Free;
    raise;
  end;
  end
else
  begin
  asm
    push eax;
    MOV EAX,[EBP+4];
    Mov ReturnAddr, EAX;
    pop eax;
  end;
  raise Exception.Create('Duplicate command '+aName) at ReturnAddr;
  end;
end; {AddCommand}

end.

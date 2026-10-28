procedure PragmaImportAccessErrors is
    procedure Stop;
    pragma Import (C, Stop, "__ada_test_no_return_returns");
    type Callback is access procedure;
    pragma Convention (C, Callback);
    Action : Callback := Stop'Access;
    pragma No_Return (Stop);
    procedure Imported;
    pragma Import (C, Imported, "__ada_test_no_return_returns");
    procedure Alias renames Imported;
    pragma No_Return (Alias);
    Renamed_Action : Callback := Alias'Access;
    procedure Ordinary is
    begin
        null;
    end Ordinary;
    procedure Guarded renames Ordinary;
    pragma No_Return (Guarded);
    type Ada_Callback is access procedure;
    Guarded_Action : Ada_Callback := Guarded'Access;
begin
    null;
end PragmaImportAccessErrors;

procedure PragmaImportAccessErrors is
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

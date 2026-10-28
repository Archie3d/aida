procedure PragmaNoReturnErrors is
    procedure Returning is
    begin
        return;
    end Returning;
    pragma No_Return (Returning);
    generic
    procedure Generic_Returning;
    procedure Generic_Returning is
    begin
        return;
    end Generic_Returning;
    pragma No_Return (Generic_Returning);
    package Model is
        type Root is tagged null record;
        procedure Stop (Object : Root);
        pragma No_Return (Stop);
        type Child is new Root with null record;
        overriding procedure Stop (Object : Child);
    end Model;
    package body Model is
        procedure Stop (Object : Root) is
        begin
            raise Program_Error;
        end Stop;
        procedure Stop (Object : Child) is
        begin
            null;
        end Stop;
    end Model;
    procedure Ordinary is
    begin
        null;
    end Ordinary;
    procedure Completion;
    pragma No_Return (Completion);
    procedure Completion renames Ordinary;
begin
    null;
end PragmaNoReturnErrors;

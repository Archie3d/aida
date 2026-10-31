with Ada.Text_IO;
procedure PragmaImport is
    procedure Stop;
    pragma Import (C, Stop, "__ada_test_no_return_returns");
    pragma No_Return (Stop);
begin
    begin
        Stop;
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Ada.Text_IO.Put_Line ("imported nonreturning procedure passed");
end PragmaImport;

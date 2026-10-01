with Controlled_Copy_Model; use Controlled_Copy_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledCopyLibrary is
    procedure Run (Depth : Integer) is
        Offset : Integer := Depth;
        type Local is new Guard with null record;
        overriding procedure Adjust (Object : in out Local) is
        begin
            Controlled_Copy_Model.Adjust (Guard (Object));
            Object.Value := Object.Value + Offset;
        end Adjust;
        A : Local;
        B : Local := A;
    begin
        if B.Value /= 42 + Depth then
            raise Program_Error;
        end if;
        if Depth > 0 then
            Run (Depth - 1);
        end if;
        B := A;
        if B.Value /= 42 + 2 * Depth then
            raise Program_Error;
        end if;
    end Run;
begin
    Run (2);
    if Live /= 0 then
        raise Program_Error;
    end if;
    declare
        A, B : Guard;
    begin
        B.Value := 99;
        Assign (A, B);
        if Live /= 2 or A.Value /= 99 then
            raise Program_Error;
        end if;
    end;
    if Live /= 0 then
        raise Program_Error;
    end if;
    Put_Line ("separate controlled copies passed");
end ControlledCopyLibrary;

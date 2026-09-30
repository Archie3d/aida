with Controlled_Model; use Controlled_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledLibrary is
    type Child is new Guard with null record;
    procedure Run (Offset : Integer) is
        type Local is new Child with null record;
        overriding procedure Finalize (Object : in out Local) is
        begin
            Put_Line ("local finalize" & Integer'Image (Offset));
            Controlled_Model.Finalize (Guard (Object));
        end Finalize;
        package Nested is
            A : Guard;
        end Nested;
        package body Nested is
            B : Child;
        begin
            Put_Line ("package body");
        end Nested;
        C : Local;
        procedure Borrow (Object : in out Guard'Class) is
        begin
            Put_Line ("borrowed");
        end Borrow;
    begin
        Borrow (C);
        if Live /= 3 then
            raise Program_Error;
        end if;
    end Run;
begin
    Run (42);
    if Live /= 0 then
        raise Program_Error;
    end if;
    Put_Line ("library passed");
end ControlledLibrary;

with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledViews is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    type Root is tagged null record;
    type Child is new Root with record
        Item : Guard;
    end record;
    type Derived is new Guard with null record;
    procedure Copy (Target : in out Root'Class; Source : Root'Class) is
    begin
        Target := Source;
    end Copy;
    procedure Own (Source : Root'Class) is
        Local : Root'Class := Source;
    begin
        raise Constraint_Error;
    end Own;
    procedure Copy_Ancestor (Target : in out Guard; Source : Guard) is
    begin
        Target := Source;
    end Copy_Ancestor;
begin
    declare
        A, B : Child;
        C, D : Derived;
    begin
        begin
            Copy (A, B);
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("class-wide assignment guarded");
        end;
        begin
            Own (A);
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("class-wide ownership guarded");
        end;
        begin
            Copy_Ancestor (Guard (C), Guard (D));
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("ancestor assignment guarded");
        end;
        if Live /= 4 then
            raise Constraint_Error;
        end if;
    end;
    if Live /= 0 then
        raise Constraint_Error;
    end if;
end ControlledViews;

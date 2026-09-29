with Ada.Text_IO; use Ada.Text_IO;
with Ada.Tags; use Ada.Tags;
procedure ClasswideChecks is
    Calls : Integer := 0;
    Fail_Equality : Boolean := False;
    package Model is
        type Root is tagged record
            X : Integer := 1;
        end record;
        function "=" (Left, Right : Root) return Boolean;
        type Child is new Root with record
            Y : Integer := 2;
        end record;
    end Model;
    package body Model is
        function "=" (Left, Right : Root) return Boolean is
        begin
            Calls := Calls + 1;
            if Fail_Equality then
                raise Constraint_Error;
            end if;
            return Left.X mod 10 = Right.X mod 10;
        end "=";
    end Model;
    use Model;
    A : Root'Class := Child'(Root'(X => 1) with Y => 3);
    B : Root'Class := Child'(Root'(X => 11) with Y => 3);
    R : Root'Class := Root'(X => 1);
    function Copy (Item : Root'Class) return Root'Class is
    begin
        Calls := Calls + 1;
        return Item;
    end Copy;
    function Fail return Root'Class is
        Temporary : Root'Class := Copy (A);
    begin
        raise Constraint_Error;
        return Temporary;
    end Fail;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (A = B);
    Check (Calls = 1);
    Check (A /= R);
    Check (Calls = 1);
    Child (B).Y := 4;
    Check (A /= B);
    Check (Model."=" (A, A));
    Check (Model."/=" (A, B));
    Calls := 0;
    Check (Copy (A) = Copy (A));
    Check (Calls = 3);
    Fail_Equality := True;
    begin
        Check (A = B);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Fail_Equality := False;
    for I in 1 .. 100 loop
        declare
            Kept : Root'Class := Copy (A);
        begin
            begin
                declare
                    Abandoned : Root'Class := Copy (B);
                begin
                    raise Constraint_Error;
                end;
            exception
                when Constraint_Error => null;
            end;
            Check (Child (Kept).Y = 3);
            begin
                Kept := Fail;
                raise Program_Error;
            exception
                when Constraint_Error => null;
            end;
            Check (Child (Kept).Y = 3);
        end;
    end loop;
    begin
        A := R;
        raise Program_Error;
    exception
        when Constraint_Error => Check (Child (A).Y = 3);
    end;
    declare
        Found : Tag := No_Tag;
    begin
        Found := Internal_Tag ("missing type");
        raise Program_Error;
    exception
        when Tag_Error => null;
    end;
    Put_Line ("class-wide checks ok");
end ClasswideChecks;

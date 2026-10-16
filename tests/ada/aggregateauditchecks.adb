with Ada.Text_IO; use Ada.Text_IO;
procedure AggregateAuditChecks is
    subtype Small is Integer range 1 .. 3;
    type Vector is array (Positive range <>) of Small;
    A : Vector (1 .. 3) := (others => 2);
    Bad : Integer := 4;
    Low : Integer := 0;
    Calls : Integer := 0;
    function Fail return Small is
    begin
        Calls := Calls + 1;
        raise Constraint_Error;
        return 1;
    end Fail;
    procedure Check is
    begin
        if A (1) /= 2 or A (2) /= 2 or A (3) /= 2 then
            raise Program_Error;
        end if;
    end Check;
    type Pair is record
        Left, Right : Small;
    end record;
    R : Pair := (2, 2);
begin
    begin
        A := (1, 1);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    begin
        A := (1, 1, 1, 1);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    begin
        A := (4 => 1, others => 1);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    begin
        A := (Low .. Low + 2 => Fail);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    if Calls /= 0 then
        raise Program_Error;
    end if;
    begin
        A := (1, Bad, 3);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    begin
        A := (1, Fail, 3);
        raise Program_Error;
    exception
        when Constraint_Error => Check;
    end;
    begin
        R := (1, Bad);
        raise Program_Error;
    exception
        when Constraint_Error =>
            if R.Left /= 2 or R.Right /= 2 then
                raise Program_Error;
            end if;
    end;
    if Calls /= 1 then
        raise Program_Error;
    end if;
    Put_Line ("aggregate shape, index and component checks preserve destinations");
end AggregateAuditChecks;

with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure AggregateCleanup is
    Live : Integer := 0;
    Defaults : Integer := 0;
    function Default_Value return Integer is
    begin
        Defaults := Defaults + 1;
        return Defaults;
    end Default_Value;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := Default_Value;
    end record;
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
    function Make (Fail : Boolean) return Guard is
        Object : Guard;
    begin
        if Fail then
            raise Constraint_Error;
        end if;
        return Object;
    end Make;
    type Pair is record
        Left, Right : Guard;
    end record;
    type Vector is array (1 .. 3) of Guard;
    type Matrix is array (1 .. 2, 1 .. 2) of Guard;
    procedure Check is
    begin
        if Live /= 0 then
            raise Program_Error;
        end if;
    end Check;
begin
    declare
        A : Vector := (2 => Make (False), others => Make (False));
    begin
        if Live /= 3 or Defaults /= 3 then
            raise Program_Error;
        end if;
    end;
    Check;
    for Iteration in 1 .. 3 loop
        begin
            declare
                A : Vector := (Make (False), Make (True), Make (False));
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check;
        end;
        begin
            declare
                R : Pair := (Make (False), Make (True));
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check;
        end;
        begin
            declare
                M : Matrix := ((Make (False), Make (False)),
                               (Make (True), Make (False)));
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check;
        end;
    end loop;
    Put_Line ("partial aggregate initialization cleanup passed");
end AggregateCleanup;

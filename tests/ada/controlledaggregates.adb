with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledAggregates is
    Live : Integer := 0;
    type Leaf is new Ada.Finalization.Controlled with record
        Value : Integer := 7;
    end record;
    overriding procedure Initialize (Object : in out Leaf) is
    begin
        Live := Live + 1;
        Put_Line ("leaf initialize");
    end Initialize;
    overriding procedure Adjust (Object : in out Leaf) is
    begin
        Live := Live + 1;
        Put_Line ("leaf adjust");
    end Adjust;
    overriding procedure Finalize (Object : in out Leaf) is
    begin
        Live := Live - 1;
        Put_Line ("leaf finalize");
    end Finalize;
    type Parent is new Ada.Finalization.Controlled with record
        Item : Leaf;
    end record;
    overriding procedure Initialize (Object : in out Parent) is
    begin
        Put_Line ("parent initialize");
    end Initialize;
    overriding procedure Adjust (Object : in out Parent) is
    begin
        Put_Line ("parent adjust");
    end Adjust;
    overriding procedure Finalize (Object : in out Parent) is
    begin
        Put_Line ("parent finalize");
    end Finalize;
    type Child is new Parent with record
        Extra : Integer := 8;
    end record;
    overriding procedure Initialize (Object : in out Child) is
    begin
        Put_Line ("child initialize");
    end Initialize;
    overriding procedure Adjust (Object : in out Child) is
    begin
        Put_Line ("child adjust");
    end Adjust;
    overriding procedure Finalize (Object : in out Child) is
    begin
        Put_Line ("child finalize");
    end Finalize;
    function Fail return Integer is
    begin
        raise Constraint_Error;
        return 0;
    end Fail;
begin
    declare
        A : Child := (Parent with Extra => 9);
        B : Child := A;
        C : Child := Child'(Parent (A) with Extra => 10);
    begin
        if Live /= 3 then
            raise Program_Error;
        end if;
        Put_Line ("constructed");
    end;
    if Live /= 0 then
        raise Program_Error;
    end if;
    begin
        declare
            Bad : Child := (Parent with Extra => Fail);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Put_Line ("ancestor cleanup");
    end;
    if Live /= 0 then
        raise Program_Error;
    end if;
    Put_Line ("aggregates passed");
end ControlledAggregates;

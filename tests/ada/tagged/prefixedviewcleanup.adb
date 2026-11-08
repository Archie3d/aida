with Ada.Finalization;
procedure PrefixedViewCleanup is
    Live : Integer := 0;
    Calls : Integer := 0;
    FailMake : Boolean := False;
    AggregateFinalizations : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 5;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    function Make return Guard;
    function Read (Object : Guard; Fail : Boolean) return Integer;
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
    function Make return Guard is
        Object : Guard;
    begin
        Calls := Calls + 1;
        if FailMake and Calls mod 2 = 0 then raise Constraint_Error; end if;
        return Object;
    end Make;
    function Read (Object : Guard; Fail : Boolean) return Integer is
    begin
        if Fail then raise Constraint_Error; end if;
        return Object.Value;
    end Read;
    procedure Exercise (Fail : Boolean) is
        function Bound (Fail : Boolean := False) return Integer renames Make.Read;
        generic
            with function Get (Fail : Boolean) return Integer;
        function GenericRead return Integer;
        function GenericRead return Integer is
        begin
            return Get (False);
        end GenericRead;
        function Instance is new GenericRead (Make.Read);
    begin
        if Calls mod 2 /= 0 or Live /= 2 then raise Program_Error; end if;
        if Bound /= 5 or Instance /= 5 then raise Program_Error; end if;
        if Bound (Fail) /= 5 then raise Program_Error; end if;
    end Exercise;
begin
    Exercise (False);
    if Live /= 0 or Calls /= 2 then raise Program_Error; end if;
    begin
        Exercise (True);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 or Calls /= 4 then raise Program_Error; end if;
    FailMake := True;
    begin
        Exercise (False);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 or Calls /= 6 then raise Program_Error; end if;
    declare
        type AggregateGuard is new Ada.Finalization.Controlled with record
            Value : Integer := 6;
        end record;
        overriding procedure Finalize (Object : in out AggregateGuard);
        function Get (Object : AggregateGuard) return Integer;
        overriding procedure Finalize (Object : in out AggregateGuard) is
        begin
            AggregateFinalizations := AggregateFinalizations + 1;
        end Finalize;
        function Get (Object : AggregateGuard) return Integer is
        begin
            return Object.Value;
        end Get;
        type Wrapper is record
            Part : AggregateGuard;
        end record;
        function Direct return Integer renames
            AggregateGuard'(Ada.Finalization.Controlled with Value => 6).Get;
        function Projected return Integer renames
            Wrapper'(Part => (Ada.Finalization.Controlled with Value => 7)).Part.Get;
    begin
        if Direct /= 6 or Projected /= 7 or AggregateFinalizations /= 0 then
            raise Program_Error;
        end if;
    end;
    if AggregateFinalizations /= 2 then raise Program_Error; end if;
end PrefixedViewCleanup;

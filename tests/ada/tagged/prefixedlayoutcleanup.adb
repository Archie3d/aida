with Ada.Finalization;
procedure PrefixedLayoutCleanup is
    Finalizations : Integer := 0;
    BuildCalls : Integer := 0;
    FailAt : Integer := 0;
    function NextValue return Integer is
    begin
        BuildCalls := BuildCalls + 1;
        if BuildCalls = FailAt then raise Constraint_Error; end if;
        return 9;
    end NextValue;
    function Size return Integer is begin return 3; end Size;
    N : Integer := Size;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 7;
    end record;
    overriding procedure Finalize (Object : in out Guard);
    function Read (Object : Guard) return Integer;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Finalizations := Finalizations + 1;
    end Finalize;
    function Read (Object : Guard) return Integer is begin return Object.Value; end Read;
    type Guards is array (Integer range <>) of Guard;
    subtype SizedGuards is Guards (1 .. N);
    procedure Exercise (Fail : Boolean) is
        function Bound return Integer renames
            SizedGuards'(others => (Ada.Finalization.Controlled with Value => NextValue))(2).Read;
        type Wrapper is record
            Items : SizedGuards;
            Tail : Guard;
        end record;
        generic
            with function Get return Integer;
        function GenericRead return Integer;
        function GenericRead return Integer is begin return Get; end GenericRead;
        function Instance is new GenericRead
            (Wrapper'(Items => (others => (Ada.Finalization.Controlled with Value => NextValue)),
                      Tail => (Ada.Finalization.Controlled with Value => 11)).Tail.Read);
    begin
        if Finalizations /= 0 or Bound /= 9 or Instance /= 11 then raise Program_Error; end if;
        if Fail then raise Constraint_Error; end if;
    end Exercise;
begin
    Exercise (False);
    if Finalizations /= 7 then raise Program_Error; end if;
    Finalizations := 0;
    begin
        Exercise (True);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Finalizations /= 7 then raise Program_Error; end if;
    Finalizations := 0;
    BuildCalls := 0;
    FailAt := 5;
    begin
        Exercise (False);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if BuildCalls /= 5 or Finalizations /= 4 then raise Program_Error; end if;
    Finalizations := 0;
    begin
        declare
            function InvalidIndex return Integer renames
                SizedGuards'(others => (Ada.Finalization.Controlled with Value => 12))(N + 1).Read;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Finalizations /= 3 then raise Program_Error; end if;
end PrefixedLayoutCleanup;

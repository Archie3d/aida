with Ada.Text_IO;
procedure BoxChecks is
    Calls : Integer := 0;
    N : Integer := 2;
    function Next return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Next;
    type Item is record
        Value : Integer := Next;
    end record;
    type Items is array (Integer range <>) of Item;
    C : Items (1 .. N) := (others => <>);
begin
    if Calls /= N then raise Program_Error; end if;
    begin
        C := (1 .. N + 1 => <>);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Calls /= N or C(1).Value + C(2).Value /= 3 then raise Program_Error; end if;
    Calls := 0;
    begin
        declare
            Limit : Integer := 1;
            subtype Tiny is Integer range 1 .. Limit;
            type Tight is record
                Value : Tiny := Next;
            end record;
            type Tights is array (1 .. 2) of Tight;
            A : Tights := (others => <>);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Calls /= 2 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("box checks ok");
end BoxChecks;

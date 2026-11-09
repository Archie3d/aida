with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;
procedure TaskEntryFamilies is
    BoundCalls : Integer := 0;
    function Bound return Integer is
    begin BoundCalls := BoundCalls + 1; return 2; end Bound;
    type Level is (Low, High);
    task S is
        entry Family (Level) (N : in out Integer);
        entry Ping (1 .. Bound);
        entry Fail (N : in out Integer);
        entry Outer;
        entry Inner;
        entry Skip;
        entry Caught (Value : out Integer);
        entry Jump;
    end S;
    task body S is
    begin
        if Family (High)'Count < 0 then raise Program_Error; end if;
        accept Family (High) (N : in out Integer) do N := N + 2; end Family;
        accept Ping (2);
        begin
            accept Fail (N : in out Integer) do
                N := 99;
                raise Constraint_Error with "rendezvous";
            end Fail;
        exception
            when Constraint_Error => null;
        end;
        accept Outer do
            accept Inner do return; end Inner;
        end Outer;
        for I in 1 .. 2 loop
            accept Skip do exit; end Skip;
        end loop;
        accept Caught (Value : out Integer) do
            raise Constraint_Error;
        exception
            when Constraint_Error => Value := 14;
        end Caught;
        accept Jump do goto Done; end Jump;
        <<Done>> null;
    end S;
    task Client;
    task body Client is begin S.Outer; end Client;
    X : Integer := 4;
begin
    if BoundCalls /= 1 then raise Program_Error; end if;
    begin S.Ping (3); raise Program_Error;
    exception when Constraint_Error => null; end;
    S.Family (High) (X);
    S.Ping (2);
    begin
        S.Fail (X);
        raise Program_Error;
    exception
        when E : Constraint_Error =>
            if Exception_Message (E) /= "rendezvous" or X /= 6 then raise Program_Error; end if;
    end;
    S.Inner;
    S.Skip;
    declare Value : Integer;
    begin
        S.Caught (Value);
        if Value /= 14 then raise Program_Error; end if;
    end;
    S.Jump;
    Put_Line (Integer'Image (X));
end TaskEntryFamilies;

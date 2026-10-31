with Ada.Text_IO;
with Ada.Exceptions;
with Controlled_Result_Model; use Controlled_Result_Model;
procedure InteractionOwnership is
    generic
        type Item is private;
        with function Build (value : Integer) return Item;
    function Produce (value : Integer := 7) return Item;
    function Produce (value : Integer := 7) return Item is
    begin
        return Build (value);
    end Produce;
    function Factory is new Produce (Guard, Make);
    function Create (seed : Integer := 9) return Guard renames Factory;
    function Create (seed : Integer := 9) return Integer is
    begin
        return seed + 100;
    end Create;
    calls : Integer := 0;
    function Component (fail : Boolean) return Guard is
    begin
        calls := calls + 1;
        if fail and calls = 2 then
            raise Constraint_Error with "component failure";
        end if;
        return Create (seed => calls);
    end Component;
    function BuildArray (fail : Boolean := False) return Guards is
    begin
        return (3 .. 5 => Component (fail));
    end BuildArray;
    function ArrayAlias (fail : Boolean := False) return Guards renames BuildArray;
    type Other is array (Integer range <>) of Guard;
    procedure Exercise (fail : Boolean) is
        outerGuard : Guard := Create;
    begin
        declare
            items : Other := Other (ArrayAlias (fail => fail));
        begin
            Check (items'First = 3 and items'Last = 5);
            Check (items (3).Data.Value = 1 and items (5).Data.Value = 3);
            Check (Objects = 4 and Resources = 4);
            return;
        end;
    end Exercise;
begin
    Check (Create (seed => 1) = 101);
    for iteration in 1 .. 3 loop
        calls := 0;
        Exercise (False);
        Check (Objects = 0 and Resources = 0 and calls = 3);
        calls := 0;
        begin
            Exercise (True);
            raise Program_Error;
        exception
            when occurrence : Constraint_Error =>
                Check (Ada.Exceptions.Exception_Message (occurrence) = "component failure");
                Check (Objects = 0 and Resources = 0 and calls = 2);
        end;
    end loop;
    -- Fail while the generic formal wrapper forwards an owned result, then
    -- verify a subsequent defaulted call has a clean exception/owner state.
    begin
        Fail_Adjust := 2;
        declare
            failed : Guard := Create;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    declare
        recovered : Guard := Create;
    begin
        Check (recovered.Data.Value = 9 and Objects = 1 and Resources = 1);
    end;
    Check (Objects = 0 and Resources = 0);
    Ada.Text_IO.Put_Line ("interaction ownership passed");
end InteractionOwnership;

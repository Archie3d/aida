with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure AsyncCleanup is
    Finalized : Natural := 0;
    Triggered : Natural := 0;
    type Item is new Ada.Finalization.Limited_Controlled with null record;
    function Extended return Item;
    function Abandoned return Item;
    overriding procedure Finalize (Object : in out Item) is
    begin
        Finalized := Finalized + 1;
    end Finalize;
    function Value return Integer is
    begin
        select
            delay 10.0;
            raise Program_Error;
        then abort
            declare Object : Item;
            begin return 17; end;
        end select;
        return 0;
    end Value;
    function Extended return Item is
    begin
        return Object : Item do
            select
                delay 10.0;
                raise Program_Error;
            then abort
                return;
            end select;
        end return;
    end Extended;
    function Abandoned return Item is
    begin
        return Object : Item do
            select
                delay 10.0;
                raise Program_Error;
            then abort
                goto Retry;
            end select;
        end return;
        <<Retry>>
        return Object : Item;
    end Abandoned;
begin
    if Value /= 17 or Finalized /= 1 then raise Program_Error; end if;
    select
        delay 0.001;
        Triggered := Triggered + 1;
    then abort
        declare
            Object : Item;
            task Child;
            task body Child is
            begin
                begin loop null; end loop;
                exception when others => raise Program_Error;
                end;
            end Child;
        begin
            delay 2.0;
        end;
    end select;
    if Finalized /= 2 or Triggered /= 1 then raise Program_Error; end if;
    -- Awaiting dependent tasks is itself an interruptible completion point.
    select
        delay 0.001;
        Triggered := Triggered + 1;
    then abort
        declare
            Object : Item;
            task Child;
            task body Child is begin loop delay 1.0; end loop; end Child;
        begin null; end;
    end select;
    if Finalized /= 3 or Triggered /= 2 then raise Program_Error; end if;
    loop
        select
            delay 10.0;
            raise Program_Error;
        then abort
            declare Object : Item;
            begin exit; end;
        end select;
    end loop;
    if Finalized /= 4 then raise Program_Error; end if;
    select
        delay 10.0;
        raise Program_Error;
    then abort
        declare Object : Item;
        begin goto After_Select; end;
    end select;
    <<After_Select>>
    if Finalized /= 5 then raise Program_Error; end if;
    begin
        select
            delay 10.0;
            raise Program_Error;
        then abort
            declare Object : Item;
            begin raise Constraint_Error; end;
        end select;
    exception when Constraint_Error => null;
    end;
    if Finalized /= 6 then raise Program_Error; end if;
    declare Object : Item := Extended;
    begin null; end;
    if Finalized /= 7 then raise Program_Error; end if;
    declare Object : Item := Abandoned;
    begin null; end;
    if Finalized /= 9 then raise Program_Error; end if;
    Put_Line ("asynchronous cleanup OK");
end AsyncCleanup;

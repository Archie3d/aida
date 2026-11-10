with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure SelectDeferral is
    Initialized, Adjusted, Finalized : Natural := 0;
    Fail_Init, Fail_Adjust, Spawn_Task : Boolean := False;
    type Item is new Ada.Finalization.Controlled with record
        Value : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Item);
    overriding procedure Adjust (Object : in out Item);
    overriding procedure Finalize (Object : in out Item);
    procedure Spawn is
        task Child;
        task body Child is begin null; end Child;
    begin null; end Spawn;
    overriding procedure Initialize (Object : in out Item) is
    begin
        delay 0.003;
        if Fail_Init then raise Constraint_Error; end if;
        if Spawn_Task then Spawn; end if;
        Object.Value := 7;
        Initialized := Initialized + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Item) is
    begin
        delay 0.003;
        if Fail_Adjust then raise Constraint_Error; end if;
        Object.Value := Object.Value + 10;
        Adjusted := Adjusted + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Item) is
    begin
        delay 0.001;
        Finalized := Finalized + 1;
    end Finalize;
begin
    select
        delay 0.001;
    then abort
        declare Object : Item;
        begin raise Program_Error; end;
    end select;
    if Initialized /= 1 or Finalized /= 1 then raise Program_Error; end if;
    declare Source, Target : Item;
    begin
        select
            delay 0.001;
        then abort
            Target := Source;
            raise Program_Error;
        end select;
        if Target.Value /= 17 or Adjusted /= 1 then raise Program_Error; end if;
        Fail_Adjust := True;
        begin Target := Source;
        exception when Program_Error => null;
        end;
        Fail_Adjust := False;
    end;
    Fail_Init := True;
    begin
        declare Object : Item;
        begin raise Program_Error; end;
    exception when Constraint_Error => null;
    end;
    Fail_Init := False;
    Spawn_Task := True;
    begin
        declare Object : Item;
        begin raise Constraint_Error; end;
    exception when Program_Error => null;
    end;
    Spawn_Task := False;
    -- Failed initialization/assignment must leave no deferral region behind.
    select delay 0.001;
    then abort loop null; end loop;
    end select;
    Put_Line ("select abort deferral OK");
end SelectDeferral;

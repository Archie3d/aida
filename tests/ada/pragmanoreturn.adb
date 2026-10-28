with Ada.Text_IO;
with Ada.Finalization;
with Ada.Assertions;
procedure PragmaNoReturn is
    procedure Check is
        Live : Integer := 0;
        Handled : Boolean := False;
        package Resources is
            type Guard is new Ada.Finalization.Limited_Controlled with null record;
            overriding procedure Initialize (Object : in out Guard);
            overriding procedure Finalize (Object : in out Guard);
        end Resources;
        package body Resources is
            procedure Initialize (Object : in out Guard) is
            begin
                Live := Live + 1;
            end Initialize;
            procedure Finalize (Object : in out Guard) is
            begin
                Live := Live - 1;
            end Finalize;
        end Resources;
        procedure Falls_Through;
        pragma No_Return (Falls_Through);
        procedure Falls_Through is
            Value : Resources.Guard;
        begin
            if Live /= 1 then raise Constraint_Error; end if;
        exception
            when Program_Error => Handled := True;
        end Falls_Through;
        procedure Fail;
        pragma No_Return (Fail);
        procedure Fail is
        begin
            raise Constraint_Error;
        end Fail;
        procedure Renamed renames Fail;
        procedure Ordinary is
        begin
            null;
        end Ordinary;
        procedure Guarded renames Ordinary;
        pragma No_Return (Guarded);
        generic
            N : Integer;
        procedure Generic_Fail;
        procedure Generic_Fail is
        begin
            if N = 1 then raise Constraint_Error; end if;
        end Generic_Fail;
        pragma No_Return (Generic_Fail);
        procedure Instance is new Generic_Fail (0);
        procedure Throwing is new Generic_Fail (1);
        package Model is
            type Root is tagged null record;
            procedure Stop (Object : Root);
            pragma No_Return (Stop);
            type Child is new Root with null record;
            overriding procedure Stop (Object : Child);
            pragma No_Return (Stop);
        end Model;
        package body Model is
            procedure Stop (Object : Root) is
            begin
                raise Constraint_Error;
            end Stop;
            procedure Stop (Object : Child) is
            begin
                null;
            end Stop;
        end Model;
        Object : Model.Root'Class := Model.Child'(null record);
        type Callback is access procedure;
        Action : Callback := Falls_Through'Access;
    begin
        begin
            Falls_Through;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Handled then raise Constraint_Error; end if;
        begin
            Guarded;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Ordinary;
        begin
            Action.all;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Handled then raise Constraint_Error; end if;
        begin
            declare
                Value : Resources.Guard;
            begin
                pragma Assert (False);
            end;
            raise Constraint_Error;
        exception
            when Ada.Assertions.Assertion_Error => null;
        end;
        if Live /= 0 then raise Constraint_Error; end if;
        begin
            Renamed;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Instance;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        begin
            Throwing;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Model.Stop (Object);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("nonreturning procedures passed");
end PragmaNoReturn;

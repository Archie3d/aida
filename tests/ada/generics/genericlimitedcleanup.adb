with Ada.Text_IO;
with Ada.Finalization;
procedure GenericLimitedCleanup is
    procedure Check is
        Live : Integer := 0;
        Finalized : Integer := 0;
        Order : Integer := 0;
        Calls : Integer := 0;
        Fail_Finalize : Boolean := False;
        type Link is access all Integer;
        type Resource is new Ada.Finalization.Limited_Controlled with record
            Id : Integer := 0;
            Value : aliased Integer := 0;
            Self : Link;
        end record;
        overriding procedure Initialize (Value : in out Resource);
        overriding procedure Finalize (Value : in out Resource);
        procedure Initialize (Value : in out Resource) is
        begin
            Live := Live + 1;
            Value.Self := Value.Value'Unchecked_Access;
        end Initialize;
        procedure Finalize (Value : in out Resource) is
        begin
            if Value.Self.all'Address /= Value.Value'Address then
                raise Constraint_Error;
            end if;
            Live := Live - 1;
            Finalized := Finalized + 1;
            Order := Order * 10 + Value.Id;
            if Fail_Finalize then
                Fail_Finalize := False;
                raise Constraint_Error;
            end if;
        end Finalize;
        function Make (Id : Integer; Fail : Boolean := False) return Resource is
        begin
            Calls := Calls + 1;
            return Result : Resource do
                Result.Id := Id;
                Result.Value := Id;
                if Fail then raise Program_Error; end if;
            end return;
        end Make;
        function Reject return Integer is
        begin
            raise Program_Error;
            return 0;
        end Reject;
        generic
            Value : Resource := Make (1);
            Guard : Integer := 0;
            Fail : Boolean := False;
        package Model is
            function Valid return Boolean;
        end Model;
        package body Model is
            function Valid return Boolean is
            begin
                return Value.Self.all'Address = Value.Value'Address;
            end Valid;
        begin
            if Fail then raise Program_Error; end if;
        end Model;
        generic
            Value : Resource'Class;
        package Wide_Model is
            function Valid return Boolean;
        end Wide_Model;
        package body Wide_Model is
            function Valid return Boolean is
            begin
                return Value.Id = 9 and Value.Self.all'Address = Value.Value'Address;
            end Valid;
        end Wide_Model;
        type Pair is limited record
            First : Resource;
            Second : Resource;
        end record;
        generic
            Value : Pair;
        package Pairs is end Pairs;
    begin
        declare
            package First is new Model;
            package Second is new Model (Make (2));
        begin
            if Live /= 2 or Calls /= 2 or not First.Valid or not Second.Valid then
                raise Program_Error;
            end if;
        end;
        if Live /= 0 or Finalized /= 2 or Order /= 21 then raise Program_Error; end if;
        begin
            declare
                package Failed is new Model (Make (3, True));
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Finalized /= 3 then raise Program_Error; end if;
        begin
            declare
                package Failed is new Model (Make (4), Guard => Reject);
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Finalized /= 4 then raise Program_Error; end if;
        begin
            declare
                package Failed is new Model (Make (5), Fail => True);
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Finalized /= 5 then raise Program_Error; end if;
        begin
            declare
                package Failed is new Pairs ((First => Make (6), Second => Make (7, True)));
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Finalized /= 7 then raise Program_Error; end if;
        begin
            declare
                package Instance is new Model (Make (8));
            begin
                if not Instance.Valid then raise Program_Error; end if;
                Fail_Finalize := True;
            end;
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        if Live /= 0 or Finalized /= 8 or Calls /= 8 then raise Program_Error; end if;
        declare
            package Wide is new Wide_Model (Make (9));
        begin
            if Live /= 1 or not Wide.Valid then raise Program_Error; end if;
        end;
        if Live /= 0 or Finalized /= 9 or Calls /= 9 then raise Program_Error; end if;
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
    Ada.Text_IO.Put_Line ("limited object cleanup passed");
end GenericLimitedCleanup;
